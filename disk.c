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

#define TRACKGAP_SIZEWRD	((350))
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
	printf("Current Drive : %d\n",curDiskDrive);
	printf("DISK DMA ENABLED %04X %08X\n",CST_GETWRDU(CST_DSKLEN,0xFFFF),CST_GETLNGU(CST_DSKPTH,0x0007FFFE));
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",0,diskDrive[0].dskMotorOn ? "on" : "off",diskDrive[0].dskTrack,diskDrive[0].dskSide ? "lower" : "upper");
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",1,diskDrive[1].dskMotorOn ? "on" : "off",diskDrive[1].dskTrack,diskDrive[1].dskSide ? "lower" : "upper");
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",2,diskDrive[2].dskMotorOn ? "on" : "off",diskDrive[2].dskTrack,diskDrive[2].dskSide ? "lower" : "upper");
	printf("Drive %d : Motor %s : Disk Track %d : Disk Side %s\n",3,diskDrive[3].dskMotorOn ? "on" : "off",diskDrive[3].dskTrack,diskDrive[3].dskSide ? "lower" : "upper");
	printf("Disk Sync : %04X\n", CST_GETWRDU(CST_DSKSYNC,0xFFFF));
#endif
}

void ConvertDiskTrack(int side, int track,u_int8_t *dskData,int drive)
{
	int s=0;
	u_int8_t *firstBlock;
	u_int8_t *writeTrack;
	
	if (side)
		side=0;
	else
		side=1;
	
	firstBlock = &dskData[track * 512 * 22 + side * 512 * 11];
	writeTrack=&diskDrive[drive].mfmDiscBuffer[track * TRACKBUFFER_SIZE * 2 + side * TRACKBUFFER_SIZE];
	
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
		diskDrive[drive].diskInDrive=0;
		return;
    }
    fseek(inDisk,0,SEEK_END);
    dskSize = ftell(inDisk);
	fseek(inDisk,0,SEEK_SET);
    dskData = (unsigned char *)malloc(dskSize);
    dskSize = fread(dskData,1,dskSize,inDisk);
    fclose(inDisk);

	ConvertDiscImageToMFM(dskData,drive);

	free(dskData);

	diskDrive[drive].diskInDrive=1;
	diskDrive[drive].dskIdCode=0xFFFFFFFF;			// Amiga standard
}

void DSK_InitialiseDisk()
{
	int a;

	curDiskDrive=0;
	for (a=0;a<4;a++)
	{
		diskDrive[a].diskInDrive=0;
		diskDrive[a].dskMotorOn=0;
		diskDrive[a].dskStepDir=0;
		diskDrive[a].dskSide=0;
		diskDrive[a].dskTrack=10;				// Not on track 0 - mostly for testing
		diskDrive[a].dskIdCode=0x00000000;		// no drive present
	}

	LoadDisk("wb.adf",0);
	if (!diskDrive[0].diskInDrive)
	{
		LoadDisk("../../wb.adf",0);
//		LoadDisk("../../wbe.adf",1);
	}

	diskDrive[1].diskInDrive=1;		// Adds a completely unformatted disk to drive 1
	diskDrive[1].dskIdCode=0xFFFFFFFF;

	dskSync=0;
	dskSyncDma=0;

	trackBuffer=&diskDrive[0].mfmDiscBuffer[diskDrive[0].dskTrack * TRACKBUFFER_SIZE * 2 + (diskDrive[0].dskSide?0:1) * TRACKBUFFER_SIZE];
	
	tbLastSide=-1;
	tbLastTrack=-1;
	tbBufferPos=0;

	prevDskLen=0;
	doDiskDMA=0;
}

void DSK_NotifyDSKLEN(u_int16_t dskLen)
{
	if ((dskLen&0x8000) && (prevDskLen&0x8000))
	{
		if (dskLen&0x4000)
		{
			printf("Warning : Attempting to write to floppy - this may corrupt disk image in ram\n",dskLen&0x3FFF);
			printf("NOTE : Trackbuffer is %d && write size is %d\n",TRACKBUFFER_SIZE,(	dskLen&0x3FFF)*2);
		}
		else
		{
			printf("NOTE : Trackbuffer is %d && read size is %d\n",TRACKBUFFER_SIZE,(	dskLen&0x3FFF)*2);
		}
		if (CST_GETWRDU(CST_ADKCONR,0x0400))
		{
			dskSyncDma=1;
		}
		doDiskDMA=1;
	}
	prevDskLen=dskLen;
	if (!(prevDskLen&0x8000))
		doDiskDMA=0;
}

int	DSK_OnSyncWord()
{
	return dskSync;
}

void DSK_Update()
{
	// Handle sync word emulation (NB disk dma is supposed to start with next WORD - this order ensures that but will break anything trying to deal
	//with interrupts and direct reading of disk data via DSKBYTR)
	
	u_int16_t syncWord;
	
	syncWord = CST_GETWRDU(CST_DSKSYNC,0xFFFF);
	if ((trackBuffer[tbBufferPos]==(syncWord>>8)) && (trackBuffer[tbBufferPos+1]==(syncWord&0xFF)))
	{
		dskSync=1;
		CST_ORWRD(CST_INTREQR,0x1000);		// signal interrupt request (it does this regardless of mask?)
	}
	else
	{
		dskSync=0;
	}

	tbBufferPos+=2;							// Disk is always spinning - NB because the speed of disk is not emulated, DMA could fail in future
	tbBufferPos%=TRACKBUFFER_SIZE;			//once BUS arbitration comes in.
	
	if (CST_GETWRDU(CST_DMACONR,0x0210)==0x0210 && CST_GETWRDU(CST_DSKLEN,0x8000) && doDiskDMA && ((dskSyncDma && dskSync)||!dskSyncDma))
	{
		// Dma running. 
		u_int16_t sizeLeft = CST_GETWRDU(CST_DSKLEN,0x3FFF);

		dskSyncDma=0;

		if (sizeLeft)
		{
			if (CST_GETWRDU(CST_DSKLEN,0x4000))
			{
				u_int32_t srcAddress = CST_GETLNGU(CST_DSKPTH,0x0007FFFE);
				u_int16_t word;
				
				word=MEM_getWord(srcAddress);
				
				trackBuffer[tbBufferPos] = word>>8;
				trackBuffer[tbBufferPos+1]=word&0xFF;

				srcAddress+=2;

				CST_SETLNG(CST_DSKPTH,srcAddress,0x0007FFFE);
			}
			else
			{
				u_int32_t destAddress = CST_GETLNGU(CST_DSKPTH,0x0007FFFE);
				u_int16_t word;
				
				word = trackBuffer[tbBufferPos]<<8;
				word|= trackBuffer[tbBufferPos+1];
				
				MEM_setWord(destAddress,word);
				destAddress+=2;
				
				CST_SETLNG(CST_DSKPTH,destAddress,0x0007FFFE);
			}
				
			sizeLeft--;
			CST_ANDWRD(CST_DSKLEN,0xC000);
			CST_ORWRD(CST_DSKLEN,sizeLeft);
			if (!sizeLeft)
			{
				doDiskDMA=0;
				CST_ORWRD(CST_INTREQR,0x0002);				
			}
		}
	}
	
/*	if (CST_GETWRDU(CST_DMACONR,0x0210)==0x0210 && dskMotorOn)
	{
		slow--;
		if (slow==0)
		{
			slow=50;
			printf("DISK DMA ENABLED %04X %08X\n",CST_GETWRDU(CST_DSKLEN,0xFFFF),CST_GETLNGU(CST_DSKPTH,0x0007FFFE));
			printf("Motor %s : Disk Track %d : Disk Side %s\n",dskMotorOn ? "on" : "off",dskTrack,dskSide ? "lower" : "upper");
			printf("Disk Sync : %04X\n", CST_GETWRDU(CST_DSKSYNC,0xFFFF));
		}
	}*/
}

int DSK_Removed()
{
	return !diskDrive[curDiskDrive].diskInDrive;
}

int DSK_Writeable()
{
	return 1;
}

int DSK_OnTrack(u_int8_t track)
{
	return diskDrive[curDiskDrive].dskTrack==track;
}

int DSK_Ready()
{
	int curCodeBit;
	
	if (!diskDrive[curDiskDrive].dskMotorOn && diskDrive[curDiskDrive].dskIdBit!=0)
	{
		// potentially running the disk drive identification loop
		curCodeBit = (diskDrive[curDiskDrive].dskIdCode & diskDrive[curDiskDrive].dskIdBit);
		
		diskDrive[curDiskDrive].dskIdBit>>=1;
		
		return curCodeBit;
	}
	return diskDrive[curDiskDrive].dskMotorOn;
}

void DSK_Step()
{
	if (!diskDrive[curDiskDrive].dskMotorOn)
		return;
		
	if (diskDrive[curDiskDrive].dskStepDir)
		diskDrive[curDiskDrive].dskTrack--;
	else
		diskDrive[curDiskDrive].dskTrack++;
		
	if (diskDrive[curDiskDrive].dskTrack<0)
		diskDrive[curDiskDrive].dskTrack=0;
	if (diskDrive[curDiskDrive].dskTrack>79)
		diskDrive[curDiskDrive].dskTrack=79;

	DSK_Status();
	
	trackBuffer=&diskDrive[curDiskDrive].mfmDiscBuffer[diskDrive[curDiskDrive].dskTrack * TRACKBUFFER_SIZE * 2 + (diskDrive[curDiskDrive].dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_SetMotor(int onOff)
{
	if (diskDrive[curDiskDrive].dskMotorOn && onOff)
	{
		// Reset disk identification shift register
		diskDrive[curDiskDrive].dskIdBit=0x80000000;
	}
	diskDrive[curDiskDrive].dskMotorOn = !onOff;
	DSK_Status();
}

void DSK_SetSide(int lower)
{
	diskDrive[curDiskDrive].dskSide=lower;
	DSK_Status();

	trackBuffer=&diskDrive[curDiskDrive].mfmDiscBuffer[diskDrive[curDiskDrive].dskTrack * TRACKBUFFER_SIZE * 2 + (diskDrive[curDiskDrive].dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_SetDir(int toZero)
{
	diskDrive[curDiskDrive].dskStepDir=toZero;
	DSK_Status();
}

void DSK_SelectDrive(int drive,int motor)
{
	curDiskDrive=drive;
	DSK_SetMotor(motor);
}
