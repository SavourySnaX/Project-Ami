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
#include "SDL.h"

#define __IGNORE_TYPES
#include "memory.h"
#include "customchip.h"
#include "ciachip.h"

unsigned char *romPtr;
unsigned char *chpPtr;

#define CHIP_MEM_SIZE		(256*1024)

typedef u_int8_t (*MEM_ReadMap)(u_int32_t upper24,u_int32_t lower16);
typedef void (*MEM_WriteMap)(u_int32_t upper24,u_int32_t lower16,u_int8_t byte);

MEM_ReadMap		mem_read[256];
MEM_WriteMap	mem_write[256];

u_int8_t MEM_getByteChip(u_int32_t upper24,u_int32_t lower16)
{
	return chpPtr[ ((upper24 - 0x00)<<16) | lower16];
}

u_int8_t MEM_getByteKick(u_int32_t upper24,u_int32_t lower16)
{
	return romPtr[ ((upper24 - 0xFC)<<16) | lower16];
}

u_int8_t MEM_getByteUnmapped(u_int32_t upper24,u_int32_t lower16)
{
	if (upper24 >= 0xF0)
	{
		return 0;			// avoid a mass of warnings as the rom boots up
	}
	printf("[WRN] : Unmapped Read From Address %08X\n",(upper24<<16)|lower16);
	return 0x00;
}

u_int8_t MEM_getByte(u_int32_t address)
{
	u_int32_t	upper24 = (address & 0x00FF0000) >> 16;
	u_int32_t	lower16 = address & 0x0000FFFF;
	u_int8_t	memRegion = upper24;

	return mem_read[memRegion](upper24,lower16);
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

void MEM_setByteChip(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	chpPtr[ ((upper24 - 0x00)<<16) | lower16]=byte;
}

void MEM_setByteKick(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	printf("[WRN] : Unmapped Write to ROM %02X -> %08X\n",byte,(upper24<<16)|lower16);
}

void MEM_setByteUnmapped(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	printf("[WRN] : Unmapped Write to Address %02X -> %08X\n",byte,(upper24<<16)|lower16);
}

void MEM_setByte(u_int32_t address,u_int8_t byte)
{
	u_int32_t	upper24 = (address & 0x00FF0000) >> 16;
	u_int32_t	lower16 = address & 0x0000FFFF;
	u_int8_t	memRegion = upper24;
	
	mem_write[memRegion](upper24,lower16,byte);
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

void MEM_Initialise(unsigned char *_romPtr)
{
	int a=0;
	
	romPtr = _romPtr;
	chpPtr = malloc(1024*256);

	for (a=0;a<1024*256;a++)
		chpPtr[a]=0;

	for (a=0;a<256;a++)
	{
		mem_read[a]=MEM_getByteUnmapped;
		mem_write[a]=MEM_setByteUnmapped;
	}
	
	mem_read[0x00] = MEM_getByteChip;
	mem_read[0x01] = MEM_getByteChip;
	mem_read[0x02] = MEM_getByteChip;
	mem_read[0x03] = MEM_getByteChip;

	mem_read[0xBF] = MEM_getByteCia;

	mem_read[0xC0] = MEM_getByteCustom;
	mem_read[0xC1] = MEM_getByteCustom;
	mem_read[0xC2] = MEM_getByteCustom;
	mem_read[0xC3] = MEM_getByteCustom;
	mem_read[0xC4] = MEM_getByteCustom;
	mem_read[0xC5] = MEM_getByteCustom;
	mem_read[0xC6] = MEM_getByteCustom;
	mem_read[0xC7] = MEM_getByteCustom;
	mem_read[0xC8] = MEM_getByteCustom;
	mem_read[0xC9] = MEM_getByteCustom;
	mem_read[0xCA] = MEM_getByteCustom;
	mem_read[0xCB] = MEM_getByteCustom;
	mem_read[0xCC] = MEM_getByteCustom;
	mem_read[0xCD] = MEM_getByteCustom;
	mem_read[0xCE] = MEM_getByteCustom;
	mem_read[0xCF] = MEM_getByteCustom;
	mem_read[0xD0] = MEM_getByteCustom;
	mem_read[0xD1] = MEM_getByteCustom;
	mem_read[0xD2] = MEM_getByteCustom;
	mem_read[0xD3] = MEM_getByteCustom;
	mem_read[0xD4] = MEM_getByteCustom;
	mem_read[0xD5] = MEM_getByteCustom;
	mem_read[0xD6] = MEM_getByteCustom;
	mem_read[0xD7] = MEM_getByteCustom;
	mem_read[0xDF] = MEM_getByteCustom;

	mem_read[0xFC] = MEM_getByteKick;
	mem_read[0xFD] = MEM_getByteKick;
	mem_read[0xFE] = MEM_getByteKick;
	mem_read[0xFF] = MEM_getByteKick;



	mem_write[0x00] = MEM_setByteChip;
	mem_write[0x01] = MEM_setByteChip;
	mem_write[0x02] = MEM_setByteChip;
	mem_write[0x03] = MEM_setByteChip;

	mem_write[0xBF] = MEM_setByteCia;

	mem_write[0xC0] = MEM_setByteCustom;
	mem_write[0xC1] = MEM_setByteCustom;
	mem_write[0xC2] = MEM_setByteCustom;
	mem_write[0xC3] = MEM_setByteCustom;
	mem_write[0xC4] = MEM_setByteCustom;
	mem_write[0xC5] = MEM_setByteCustom;
	mem_write[0xC6] = MEM_setByteCustom;
	mem_write[0xC7] = MEM_setByteCustom;
	mem_write[0xC8] = MEM_setByteCustom;
	mem_write[0xC9] = MEM_setByteCustom;
	mem_write[0xCA] = MEM_setByteCustom;
	mem_write[0xCB] = MEM_setByteCustom;
	mem_write[0xCC] = MEM_setByteCustom;
	mem_write[0xCD] = MEM_setByteCustom;
	mem_write[0xCE] = MEM_setByteCustom;
	mem_write[0xCF] = MEM_setByteCustom;
	mem_write[0xD0] = MEM_setByteCustom;
	mem_write[0xD1] = MEM_setByteCustom;
	mem_write[0xD2] = MEM_setByteCustom;
	mem_write[0xD3] = MEM_setByteCustom;
	mem_write[0xD4] = MEM_setByteCustom;
	mem_write[0xD5] = MEM_setByteCustom;
	mem_write[0xD6] = MEM_setByteCustom;
	mem_write[0xD7] = MEM_setByteCustom;
	mem_write[0xDF] = MEM_setByteCustom;

	mem_write[0xFC] = MEM_setByteKick;
	mem_write[0xFD] = MEM_setByteKick;
	mem_write[0xFE] = MEM_setByteKick;
	mem_write[0xFF] = MEM_setByteKick;
}
