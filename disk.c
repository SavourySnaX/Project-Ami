/*
 *  disk.c
 *  ami

Copyright (c) 2009 Lee Hammerton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "memory.h"
#include "customchip.h"
#include "disk.h"

#define TRACKGAP_SIZEWRD	(348)			// FIXME _ I really need to alloc this based on disk format (350+250) works for long tracks
#define TRACKGAP_SIZE		(TRACKGAP_SIZEWRD*2)
#define TRACKBUFFER_SIZE	(TRACKGAP_SIZE + (4 + 2 + 2 + 4 + 4 + 16 + 16 + 4 + 4 + 4 + 4 + 512 + 512)*11)

typedef struct
{
	u_int8_t	mfmDiscBuffer[TRACKBUFFER_SIZE * 80 * 2];			// Represents complete disc image in MFM encoding

	int			diskInDrive;
	int			dskMotorOn;
	int			dskStepDir;
	int			dskSide;
	int			dskTrack;
	u_int32_t	dskIdBit;
	u_int32_t	dskIdCode;
} DiskDrive;

typedef struct
{
	DiskDrive diskDrive[4];
	int			dskSync;
	int			dskSyncDma;
	int			tbLastSide;
	int			tbLastTrack;
	int			tbBufferPos;
	int			curDiskDrive;
	u_int16_t	prevDskLen;
	int			doDiskDMA;
	u_int8_t	*trackBuffer;										// Current track pointer
} DSK_data;

DSK_data dsk_Data;

void DSK_SaveState(FILE *outStream)
{
	fwrite(&dsk_Data,1,sizeof(DSK_data),outStream);
}

void DSK_LoadState(FILE *inStream)
{
	fread(&dsk_Data,1,sizeof(DSK_data),inStream);

	dsk_Data.trackBuffer=&dsk_Data.diskDrive[dsk_Data.curDiskDrive].mfmDiscBuffer[dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack * TRACKBUFFER_SIZE * 2 + (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_MFM_Chk(u_int8_t *chk,u_int8_t *data,u_int32_t length)
{
	u_int32_t offs=0;
	
	chk[0]=0;
	chk[1]=0;
	chk[2]=0;
	chk[3]=0;
	
	while (offs<length)
	{
		chk[offs&3]^=*data++;
		offs++;
	}
}

void DSK_MFM_Enc(u_int8_t *even,u_int8_t *odd,u_int8_t *data,u_int32_t length)
{
	u_int8_t byte;
	
	while (length)
	{
		byte = *data++;
	
		*odd++ = (byte & 0x55) | 0xAA;
		*even++ = ((byte>>1) & 0x55) | 0xAA;
	
		length--;
	}
}

void DSK_Status()
{
#if ENABLE_DISK_WARNINGS
	printf("Current Drive : %d\n",dsk_Data.curDiskDrive);
	printf("DISK DMA ENABLED %04X %08X\n",CST_GETWRDU(CST_DSKLEN,0xFFFF),CST_GETLNGU(CST_DSKPTH,CUSTOM_CHIP_RAM_MASK));
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",0,dsk_Data.diskDrive[0].dskMotorOn ? "on" : "off",
		dsk_Data.diskDrive[0].dskTrack,dsk_Data.diskDrive[0].dskSide ? "lower" : "upper");
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",1,dsk_Data.diskDrive[1].dskMotorOn ? "on" : "off",
		dsk_Data.diskDrive[1].dskTrack,dsk_Data.diskDrive[1].dskSide ? "lower" : "upper");
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",2,dsk_Data.diskDrive[2].dskMotorOn ? "on" : "off",
		dsk_Data.diskDrive[2].dskTrack,dsk_Data.diskDrive[2].dskSide ? "lower" : "upper");
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",3,dsk_Data.diskDrive[3].dskMotorOn ? "on" : "off",
		dsk_Data.diskDrive[3].dskTrack,dsk_Data.diskDrive[3].dskSide ? "lower" : "upper");
	printf("Disk Sync : %04X\n", CST_GETWRDU(CST_DSKSYNC,0xFFFF));
#endif
}

void ConvertDiskTrackExtended(int side, int track,u_int8_t *firstBlock,int drive)
{
	int s=0;
	u_int8_t *writeTrack;
	
	writeTrack=&dsk_Data.diskDrive[drive].mfmDiscBuffer[track * TRACKBUFFER_SIZE * 2 + side * TRACKBUFFER_SIZE];
	
	for (s=0;s<TRACKGAP_SIZEWRD;s++)
	{
		*writeTrack++=0x00;		// Not sure what should go in GAP (try this though)
		*writeTrack++=0x00;
	}

	for (s=0;s<11;s++)
	{
		u_int8_t	temp[16];

		*writeTrack++=0xAA;		// Set Sync? Bytes
		*writeTrack++=0xAA;
		*writeTrack++=0xAA;
		*writeTrack++=0xAA;
		
		*writeTrack++=0x44;		// MFM Sync Marker
		*writeTrack++=0x89;
		*writeTrack++=0x44;
		*writeTrack++=0x89;
		
		temp[0]=0xFF;				// Amiga Dos Marker
		temp[1]=track*2+side;
		temp[2]=s;
		temp[3]=11-s;
		
		DSK_MFM_Enc(writeTrack,writeTrack+4,temp,4);
		writeTrack+=8;
		memset(temp,0,16);
		DSK_MFM_Enc(writeTrack,writeTrack+16,temp,16);	// Amiga Reserved
		writeTrack+=32;
		DSK_MFM_Chk(temp,writeTrack-40,40);
		DSK_MFM_Enc(writeTrack,writeTrack+4,temp,4);	// Header Checksum
		writeTrack+=8;

		// Insert Checksum here

		writeTrack+=8;
		DSK_MFM_Enc(writeTrack,writeTrack+512,firstBlock,512);	// Data
		firstBlock+=512;
		writeTrack-=8;
		DSK_MFM_Chk(temp,writeTrack+8,512*2);
		DSK_MFM_Enc(writeTrack,writeTrack+4,temp,4);	// Data checksum
		writeTrack+=8+512*2;
	}
}

void CopyDiskTrack(int side, int track,u_int8_t *firstBlock,int drive,u_int16_t length)
{
	u_int8_t *writeTrack;
	
	writeTrack=&dsk_Data.diskDrive[drive].mfmDiscBuffer[track * TRACKBUFFER_SIZE * 2 + side * TRACKBUFFER_SIZE];
	
	memcpy(writeTrack,firstBlock,length);
}

void ConvertDiskTrack(int side, int track,u_int8_t *dskData,int drive)
{
	u_int8_t *firstBlock;
	
	firstBlock = &dskData[track * 512 * 22 + side * 512 * 11];
	
	ConvertDiskTrackExtended(side,track,firstBlock,drive);
}

void ConvertExtendedADF2(u_int8_t *dskData,int drive)
{
	// Version 2 of extended adf format
	
	printf("Extended ADF format 2 is currently not supported\n");
}

void ConvertExtendedADF(u_int8_t *dskData,int drive)
{
	u_int8_t *data = &dskData[8 + 160 * 4];
	
	int t,s;
	for (t=0;t<80;t++)
	{
		for (s=0;s<2;s++)
		{
			u_int16_t sync,length;
			
			sync = dskData[8+(t*2+s)*4+0];
			sync<<=8;
			sync+= dskData[8+(t*2+s)*4+1];
			
			length=dskData[8+(t*2+s)*4+2];
			length<<=8;
			length+=dskData[8+(t*2+s)*4+3];
			
			if (sync==0)
			{
				// Standard dos track
				printf("DOS TRACK\n");
				ConvertDiskTrackExtended(s,t,data,drive);
				data+=length;
			}
			else
			{
				data-=2;
				data[0]=dskData[8+(t*2+s)*4+0];
				data[1]=dskData[8+(t*2+s)*4+1];
				printf("MFM TRACK : Length %04X  SYNC %04X\n",length+2,sync);
				CopyDiskTrack(s,t,data,drive,length);
				data+=length+2;
			}
		}
	}
}

void ConvertDiscImageToMFM(u_int8_t *dskData,int drive)
{
	int t,s;
	for (t=0;t<80;t++)
	{
		for (s=0;s<2;s++)
		{
			ConvertDiskTrack(s,t,dskData,drive);
		}
	}
}

void LoadDisk(char *disk,int drive)
{
    FILE *inDisk;
    unsigned long dskSize;
	u_int8_t *dskData;
	
    inDisk = fopen(disk,"rb");
    if (!inDisk)
    {
		dsk_Data.diskDrive[drive].diskInDrive=0;
		return;
    }
    fseek(inDisk,0,SEEK_END);
    dskSize = ftell(inDisk);
	fseek(inDisk,0,SEEK_SET);
    dskData = (unsigned char *)malloc(dskSize);
    if (dskSize != fread(dskData,1,dskSize,inDisk))
	{
		fclose(inDisk);
		dsk_Data.diskDrive[drive].diskInDrive=0;
		return;
	}
    fclose(inDisk);

	if (strncmp("UAE--ADF",(char *)dskData,8)==0)
	{
		ConvertExtendedADF(dskData,drive);
	}
	else
	if (strncmp("UAE-1ADF",(char *)dskData,8)==0)
	{
		ConvertExtendedADF2(dskData,drive);
	}
	else
	{
		ConvertDiscImageToMFM(dskData,drive);
	}

	free(dskData);

	dsk_Data.diskDrive[drive].diskInDrive=1;
	dsk_Data.diskDrive[drive].dskIdCode=0xFFFFFFFF;			// Amiga standard
}

void DSK_InitialiseDisk()
{
	int a;

	dsk_Data.curDiskDrive=0;
	for (a=0;a<4;a++)
	{
		dsk_Data.diskDrive[a].diskInDrive=0;
		dsk_Data.diskDrive[a].dskMotorOn=0;
		dsk_Data.diskDrive[a].dskStepDir=0;
		dsk_Data.diskDrive[a].dskSide=0;
		dsk_Data.diskDrive[a].dskTrack=10;				// Not on track 0 - mostly for testing
		dsk_Data.diskDrive[a].dskIdCode=0x00000000;		// no drive present
	}

	LoadDisk("wb.adf",0);
	if (!dsk_Data.diskDrive[0].diskInDrive)
	{
		LoadDisk("../../wb.adf",0);
//		LoadDisk("../../afd.adf",1);
	}

//	diskDrive[1].diskInDrive=1;		// Adds a completely unformatted disk to drive 1
//	diskDrive[1].dskIdCode=0xFFFFFFFF;

	dsk_Data.dskSync=0;
	dsk_Data.dskSyncDma=0;

	dsk_Data.trackBuffer=&dsk_Data.diskDrive[0].mfmDiscBuffer[dsk_Data.diskDrive[0].dskTrack * TRACKBUFFER_SIZE * 2 + (dsk_Data.diskDrive[0].dskSide?0:1) * TRACKBUFFER_SIZE];
	
	dsk_Data.tbLastSide=-1;
	dsk_Data.tbLastTrack=-1;
	dsk_Data.tbBufferPos=0;

	dsk_Data.prevDskLen=0;
	dsk_Data.doDiskDMA=0;
}

void DSK_NotifyDSKLEN(u_int16_t dskLen)
{
	if ((dskLen&0x8000) && (dsk_Data.prevDskLen&0x8000))
	{
#if ENABLE_DISK_WARNINGS
		if (dskLen&0x4000)
		{
			printf("Warning : Attempting to write to floppy - this may corrupt disk image in ram\n",dskLen&0x3FFF);
			printf("NOTE : Trackbuffer is %d && write size is %d\n",TRACKBUFFER_SIZE,(	dskLen&0x3FFF)*2);
		}
		else
		{
			printf("NOTE : Trackbuffer is %d && read size is %d\n",TRACKBUFFER_SIZE,(	dskLen&0x3FFF)*2);
		}
#endif
		if (CST_GETWRDU(CST_ADKCONR,0x0400))
		{
			dsk_Data.dskSyncDma=1;
		}
		dsk_Data.doDiskDMA=1;
	}
	dsk_Data.prevDskLen=dskLen;
	if (!(dsk_Data.prevDskLen&0x8000))
		dsk_Data.doDiskDMA=0;
}

int	DSK_OnSyncWord()
{
	return dsk_Data.dskSync;
}

void DSK_Update()
{
	// Handle sync word emulation (NB disk dma is supposed to start with next WORD - this order ensures that but will break anything trying to deal
	//with interrupts and direct reading of disk data via DSKBYTR)
	
	u_int16_t syncWord;
	
	if (horizontalClock == 0x07)
	{
		syncWord = CST_GETWRDU(CST_DSKSYNC,0xFFFF);
		if ((dsk_Data.trackBuffer[dsk_Data.tbBufferPos]==(syncWord>>8)) && (dsk_Data.trackBuffer[dsk_Data.tbBufferPos+1]==(syncWord&0xFF)))
		{
			dsk_Data.dskSync=1;
			CST_ORWRD(CST_INTREQR,0x1000);		// signal interrupt request
		}
		else
		{
			dsk_Data.dskSync=0;
		}
		
		dsk_Data.tbBufferPos+=2;							// Disk is always spinning - NB because the speed of disk is not emulated, DMA could fail in future
		dsk_Data.tbBufferPos%=TRACKBUFFER_SIZE;			//once BUS arbitration comes in.
		
		if (CST_GETWRDU(CST_DMACONR,0x0210)==0x0210 && CST_GETWRDU(CST_DSKLEN,0x8000) && 
			dsk_Data.doDiskDMA && ((dsk_Data.dskSyncDma && dsk_Data.dskSync)||!dsk_Data.dskSyncDma))
		{
			// Dma running. 
			u_int16_t sizeLeft = CST_GETWRDU(CST_DSKLEN,0x3FFF);
			
			dsk_Data.dskSyncDma=0;
			
			if (sizeLeft)
			{
				if (CST_GETWRDU(CST_DSKLEN,0x4000))
				{
					u_int32_t srcAddress = CST_GETLNGU(CST_DSKPTH,CUSTOM_CHIP_RAM_MASK);
					u_int16_t word;
					
					word=MEM_getWord(srcAddress);
					
					dsk_Data.trackBuffer[dsk_Data.tbBufferPos] = word>>8;
					dsk_Data.trackBuffer[dsk_Data.tbBufferPos+1]=word&0xFF;
					
					srcAddress+=2;
					
					CST_SETLNG(CST_DSKPTH,srcAddress,CUSTOM_CHIP_RAM_MASK);
				}
				else
				{
					u_int32_t destAddress = CST_GETLNGU(CST_DSKPTH,CUSTOM_CHIP_RAM_MASK);
					u_int16_t word;
					
					word = dsk_Data.trackBuffer[dsk_Data.tbBufferPos]<<8;
					word|= dsk_Data.trackBuffer[dsk_Data.tbBufferPos+1];
					
					MEM_setWord(destAddress,word);
					destAddress+=2;
					
					CST_SETLNG(CST_DSKPTH,destAddress,CUSTOM_CHIP_RAM_MASK);
				}
				
				sizeLeft--;
				CST_ANDWRD(CST_DSKLEN,0xC000);
				CST_ORWRD(CST_DSKLEN,sizeLeft);
				if (!sizeLeft)
				{
					dsk_Data.doDiskDMA=0;
					CST_ORWRD(CST_INTREQR,0x0002);				
				}
			}
		}
	}
}

int DSK_Removed()
{
	return !dsk_Data.diskDrive[dsk_Data.curDiskDrive].diskInDrive;
}

int DSK_Writeable()
{
	return 1;
}

int DSK_OnTrack(u_int8_t track)
{
	return dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack==track;
}

int DSK_Ready()
{
	int curCodeBit;
	
	if (!dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskMotorOn && dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskIdBit!=0)
	{
		// potentially running the disk drive identification loop
		curCodeBit = (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskIdCode & dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskIdBit);
		
		dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskIdBit>>=1;
		
		return curCodeBit;
	}
	return dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskMotorOn;
}

void DSK_Step()
{
	if (!dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskMotorOn)
		return;
		
	if (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskStepDir)
		dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack--;
	else
		dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack++;
		
	if (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack<0)
		dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack=0;
	if (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack>79)
		dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack=79;

	DSK_Status();
	
	dsk_Data.trackBuffer=&dsk_Data.diskDrive[dsk_Data.curDiskDrive].mfmDiscBuffer[dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack * TRACKBUFFER_SIZE * 2 + (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_SetMotor(int onOff)
{
	if (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskMotorOn && onOff)
	{
		// Reset disk identification shift register
		dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskIdBit=0x80000000;
	}
	dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskMotorOn = !onOff;
	DSK_Status();
}

void DSK_SetSide(int lower)
{
	dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskSide=lower;
	DSK_Status();

	dsk_Data.trackBuffer=&dsk_Data.diskDrive[dsk_Data.curDiskDrive].mfmDiscBuffer[dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskTrack * TRACKBUFFER_SIZE * 2 + (dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_SetDir(int toZero)
{
	dsk_Data.diskDrive[dsk_Data.curDiskDrive].dskStepDir=toZero;
	DSK_Status();
}

void DSK_SelectDrive(int drive,int motor)
{
	dsk_Data.curDiskDrive=drive;
	DSK_SetMotor(motor);
}
