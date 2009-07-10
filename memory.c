/*
 *  memory.c
 *  ami
 *
 *  Created by Lee Hammerton on 06/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "memory.h"

unsigned char *romPtr;
unsigned char *chpPtr;

#define CHIP_MEM_SIZE		(256*1024)

void MEM_Initialise(unsigned char *_romPtr)
{
	int a=0;
	
	romPtr = _romPtr;
	chpPtr = malloc(1024*256);

	for (a=0;a<1024*256;a++)
		chpPtr[a]=0;
}

u_int8_t MEM_getByte(u_int32_t address)
{
	u_int32_t	upper24 = (address & 0x00FF0000) >> 16;
	u_int32_t	lower16 = address & 0x0000FFFF;
	u_int8_t	memRegion = upper24;
	
	switch (memRegion)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
			return chpPtr[ ((upper24 - 0x00)<<16) | lower16];
			
		case 0xFC:
		case 0xFD:
		case 0xFE:
		case 0xFF:
			// Rom Read
			return romPtr[ ((upper24 - 0xFC)<<16) | lower16];
	}
	
	printf("[WRN] : Unmapped Read From Address %08X\n",address);
	return 0xFF;
}

u_int16_t MEM_getWord(u_int32_t address)
{
	u_int16_t retVal;
	retVal = MEM_getByte(address)<<8;
	retVal|= MEM_getByte(address+1);
	
	return retVal;
}

u_int32_t MEM_getLong(u_int32_t address)
{
	u_int32_t retVal;
	retVal = MEM_getWord(address)<<16;
	retVal|= MEM_getWord(address+2);
	
	return retVal;
}

void MEM_setByte(u_int32_t address,u_int8_t byte)
{
	u_int32_t	upper24 = (address & 0x00FF0000) >> 16;
	u_int32_t	lower16 = address & 0x0000FFFF;
	u_int8_t	memRegion = upper24;
	
	switch (memRegion)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
			chpPtr[ ((upper24 - 0x00)<<16) | lower16] = byte;
			return;
		case 0xFC:
		case 0xFD:
		case 0xFE:
		case 0xFF:
			// Rom Write - I don't think so
//			romPtr[ ((upper24 - 0xFC)<<16) | lower16] = byte;
			return;
	}

	printf("[WRN] : Unmapped Write To Address %02X -> %08X\n",byte,address);
}

void MEM_setWord(u_int32_t address, u_int16_t word)
{
	MEM_setByte(address,word>>8);
	MEM_setByte(address+1,word&0xFF);
}

void MEM_setLong(u_int32_t address, u_int32_t dword)
{
	MEM_setWord(address,dword>>16);
	MEM_setWord(address+2,dword&0xFFFF);
}