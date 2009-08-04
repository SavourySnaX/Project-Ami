/*
 *  disk.c
 *  ami
 *
 *  Created by Lee Hammerton on 03/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *
 * Note disk drive is running at ridiculous speeds at present because i`m not obeying any timings
 *
 *
 *  Going off amiga dos writing of tracks : 831 wrd track gap but i`m going with 830 cos i suspect it writes 1 wrd extra (from memory)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "memory.h"
#include "customchip.h"
#include "disk.h"

int			diskInDrive;
int			dskMotorOn;
int			dskStepDir;
int			dskSide;
int			dskTrack;

int			tbLastSide;
int			tbLastTrack;
int			tbBufferPos;

u_int16_t	prevDskLen;
int			doDiskDMA;

#define TRACKGAP_SIZEWRD	((350))
#define TRACKGAP_SIZE		(TRACKGAP_SIZEWRD*2)
#define TRACKBUFFER_SIZE	(TRACKGAP_SIZE + (4 + 2 + 2 + 4 + 4 + 16 + 16 + 4 + 4 + 4 + 4 + 512 + 512)*11)

u_int8_t	mfmDiscBuffer[TRACKBUFFER_SIZE * 80 * 2];			// Represents complete disc image in MFM encoding
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
	printf("DISK DMA ENABLED %04X %08X\n",CST_GETWRDU(CST_DSKLEN,0xFFFF),CST_GETLNGU(CST_DSKPTH,0x0007FFFE));
	printf("Motor %s : Disk Track %d : Disk Side %s\n",dskMotorOn ? "on" : "off",dskTrack,dskSide ? "lower" : "upper");
	printf("Disk Sync : %04X\n", CST_GETWRDU(CST_DSKSYNC,0xFFFF));
#endif
}

void ConvertDiskTrack(int side, int track,u_int8_t *dskData)
{
	int s=0;
	u_int8_t *firstBlock;
	u_int8_t *writeTrack;
	
	if (side)
		side=0;
	else
		side=1;
	
	firstBlock = &dskData[track * 512 * 22 + side * 512 * 11];
	writeTrack=&mfmDiscBuffer[track * TRACKBUFFER_SIZE * 2 + side * TRACKBUFFER_SIZE];
	
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

void ConvertDiscImageToMFM(u_int8_t *dskData)
{
	int t,s;
	for (int t=0;t<80;t++)
	{
		for (int s=0;s<2;s++)
		{
			ConvertDiskTrack(s,t,dskData);
		}
	}
}

void LoadDisk(char *disk)
{
    FILE *inDisk;
    unsigned long dskSize;
	u_int8_t *dskData;
	
    inDisk = fopen(disk,"rb");
    if (!inDisk)
    {
		diskInDrive=0;
		return;
    }
    fseek(inDisk,0,SEEK_END);
    dskSize = ftell(inDisk);
	fseek(inDisk,0,SEEK_SET);
    dskData = (unsigned char *)malloc(dskSize);
    dskSize = fread(dskData,1,dskSize,inDisk);
    fclose(inDisk);

	ConvertDiscImageToMFM(dskData);

	free(dskData);

	diskInDrive=1;
}

void DSK_InitialiseDisk()
{
	LoadDisk("wb.adf");
	if (!diskInDrive)
	{
		LoadDisk("../../wb.adf");
	}
	
	dskMotorOn=0;
	dskStepDir=0;
	dskSide=0;
	dskTrack=10;			// Not on track 0 - mostly for testing
	
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

		doDiskDMA=1;
	}
	prevDskLen=dskLen;
	if (!(prevDskLen&0x8000))
		doDiskDMA=0;
}

void DSK_Update()
{
	static int slow=10;
	
	if (CST_GETWRDU(CST_DMACONR,0x0210)==0x0210 && CST_GETWRDU(CST_DSKLEN,0x8000) && doDiskDMA)
	{
		// Dma running. 
		u_int16_t sizeLeft = CST_GETWRDU(CST_DSKLEN,0x3FFF);

		if (sizeLeft)
		{
			if (CST_GETWRDU(CST_DSKLEN,0x4000))
			{
				u_int32_t srcAddress = CST_GETLNGU(CST_DSKPTH,0x0007FFFE);
				u_int16_t word;
				
				word=MEM_getWord(srcAddress);
				
				trackBuffer[tbBufferPos] = word>>8;
				trackBuffer[tbBufferPos+1]=word&0xFF;

				tbBufferPos+=2;
				tbBufferPos%=TRACKBUFFER_SIZE;
				
				srcAddress+=2;

				CST_SETLNG(CST_DSKPTH,srcAddress,0x0007FFFE);
			}
			else
			{
				u_int32_t destAddress = CST_GETLNGU(CST_DSKPTH,0x0007FFFE);
				u_int16_t word;
				
				word = trackBuffer[tbBufferPos]<<8;
				word|= trackBuffer[tbBufferPos+1];
				
				tbBufferPos+=2;
				tbBufferPos%=TRACKBUFFER_SIZE;
				
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
	return !diskInDrive;
}

int DSK_Writeable()
{
	return 1;
}

int DSK_OnTrack(u_int8_t track)
{
	return dskTrack==track;
}

int DSK_Ready()
{
	return dskMotorOn;
}

void DSK_Step()
{
	if (!dskMotorOn)
		return;
		
	if (dskStepDir)
		dskTrack--;
	else
		dskTrack++;
		
	if (dskTrack<0)
		dskTrack=0;
	if (dskTrack>79)
		dskTrack=79;

	DSK_Status();
	
	trackBuffer=&mfmDiscBuffer[dskTrack * TRACKBUFFER_SIZE * 2 + (dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_SetMotor(int onOff)
{
	dskMotorOn = !onOff;
	DSK_Status();
}

void DSK_SetSide(int lower)
{
	dskSide=lower;
	DSK_Status();

	trackBuffer=&mfmDiscBuffer[dskTrack * TRACKBUFFER_SIZE * 2 + (dskSide?0:1) * TRACKBUFFER_SIZE];
}

void DSK_SetDir(int toZero)
{
	dskStepDir=toZero;
	DSK_Status();
}

