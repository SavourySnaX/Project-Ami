/*
 *  memory.c
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

#include <stdlib.h>
#include <stdio.h>

#include "config.h"

#include "memory.h"
#include "autoconf.h"
#include "customchip.h"
#include "ciachip.h"

#define SLOW_MEM_SIZE		(512*1024)

unsigned char *romPtr;
unsigned char *chpPtr;
unsigned char *slowPtr;

typedef u_int8_t (*MEM_ReadMap)(u_int32_t upper24,u_int32_t lower16);
typedef void (*MEM_WriteMap)(u_int32_t upper24,u_int32_t lower16,u_int8_t byte);

MEM_ReadMap		mem_read[256];
MEM_WriteMap	mem_write[256];

u_int8_t MEM_getByteChip(u_int32_t upper24,u_int32_t lower16)
{
	return chpPtr[ ((upper24 - 0x00)<<16) | lower16];
}

u_int8_t MEM_getByteSlow(u_int32_t upper24,u_int32_t lower16)
{
	return slowPtr[ ((upper24 - 0xC0)<<16) | lower16];
}

u_int8_t MEM_getByteChipMirror(u_int32_t upper24,u_int32_t lower16)
{
	return chpPtr[ ((upper24 & ((CHIP_MEM_SIZE/65536)-1))<<16) | lower16];
}

u_int8_t MEM_getByteKick(u_int32_t upper24,u_int32_t lower16)
{
//	return romPtr[ ((upper24 - 0xF8)<<16) | lower16];
	return romPtr[ ((upper24 - 0xFC)<<16) | lower16];
}

u_int8_t MEM_getByteOvlKick(u_int32_t upper24,u_int32_t lower16)
{
//	return romPtr[ ((upper24 + 0x04)<<16) | lower16];
	return romPtr[ ((upper24 - 0x00)<<16) | lower16];
}

u_int8_t MEM_getByteUnmapped(u_int32_t upper24,u_int32_t lower16)
{
	if (upper24 >= 0xF0)
	{
//		DEB_PauseEmulation("F0 area read");
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
	if (address&1)
	{
		DEB_PauseEmulation("Mem Get Word (Unaligned Read)");
	}
	retVal = MEM_getByte(address)<<8;
	retVal|= MEM_getByte(address+1);
	
	return retVal;
}

u_int32_t MEM_getLong(u_int32_t address)
{
	u_int32_t retVal;
	if (address&1)
	{
		DEB_PauseEmulation("Mem Get Long (Unaligned Read)");
	}
	retVal = MEM_getWord(address)<<16;
	retVal|= MEM_getWord(address+2);
	
	return retVal;
}

extern int startDebug;

void MEM_setByteChip(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	chpPtr[ ((upper24 - 0x00)<<16) | lower16]=byte;
}

void MEM_setByteSlow(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	slowPtr[ ((upper24 - 0xC0)<<16) | lower16]=byte;
}

void MEM_setByteChipMirror(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	chpPtr[ ((upper24 & ((CHIP_MEM_SIZE/65536)-1))<<16) | lower16]=byte;
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
	if (address&1)
	{
		DEB_PauseEmulation("Mem Set Word (Unaligned Write)");
	}
	MEM_setByte(address,word>>8);
	MEM_setByte(address+1,word&0xFF);
}

extern int startDebug;

void MEM_setLong(u_int32_t address, u_int32_t dword)
{
	if (address&1)
	{
		DEB_PauseEmulation("Mem Set Long (Unaligned Write)");
	}

	MEM_setWord(address,dword>>16);
	MEM_setWord(address+2,dword&0xFFFF);
}

void MEM_Initialise(unsigned char *_romPtr)
{
	int a=0;
	
	romPtr = _romPtr;
	chpPtr = malloc(CHIP_MEM_SIZE);
	slowPtr = malloc(SLOW_MEM_SIZE);

	for (a=0;a<CHIP_MEM_SIZE;a++)
		chpPtr[a]=0;

	for (a=0;a<SLOW_MEM_SIZE;a++)
		slowPtr[a]=0;

	for (a=0;a<256;a++)
	{
		mem_read[a]=MEM_getByteUnmapped;
		mem_write[a]=MEM_setByteUnmapped;
	}

	for (a=0;a<CHIP_MEM_SIZE / 65536;a++)
	{
		mem_read[a] = MEM_getByteChip;
		mem_write[a]= MEM_setByteChip;
	}
	for (a=(CHIP_MEM_SIZE/65536);a<0x20;a++)
	{
		mem_read[a] = MEM_getByteChipMirror;
		mem_write[a]= MEM_setByteChipMirror;
	}
	
	for (a=0;a<SLOW_MEM_SIZE / 65536;a++)
	{
		mem_read[a+0xC0] = MEM_getByteSlow;
		mem_write[a+0xC0]= MEM_setByteSlow;
	}
	for (a=(SLOW_MEM_SIZE/65536);a<0x18;a++)
	{
		mem_read[a+0xC0] = MEM_getByteCustom;
		mem_write[a+0xC0]= MEM_setByteCustom;
	}

	mem_read[0xBF] = MEM_getByteCia;

	mem_read[0xDF] = MEM_getByteCustom;
/*
	mem_read[0xF8] = MEM_getByteKick;
	mem_read[0xF9] = MEM_getByteKick;
	mem_read[0xFA] = MEM_getByteKick;
	mem_read[0xFB] = MEM_getByteKick;
*/
	mem_read[0xE8] = MEM_getByteAutoConf;

	mem_read[0xFC] = MEM_getByteKick;
	mem_read[0xFD] = MEM_getByteKick;
	mem_read[0xFE] = MEM_getByteKick;
	mem_read[0xFF] = MEM_getByteKick;



	mem_write[0xBF] = MEM_setByteCia;

	mem_write[0xDF] = MEM_setByteCustom;
/*
	mem_write[0xF8] = MEM_setByteKick;
	mem_write[0xF9] = MEM_setByteKick;
	mem_write[0xFA] = MEM_setByteKick;
	mem_write[0xFB] = MEM_setByteKick;
*/
	mem_write[0xE8] = MEM_setByteAutoConf;

	mem_write[0xFC] = MEM_setByteKick;
	mem_write[0xFD] = MEM_setByteKick;
	mem_write[0xFE] = MEM_setByteKick;
	mem_write[0xFF] = MEM_setByteKick;


	MEM_MapKickstartLow(1);	// on boot OVL flag is set so kickstart rom should be mirrored into low addresses
}

void MEM_MapKickstartLow(int yes)
{
	if (yes)
	{
		mem_read[0x00] = MEM_getByteOvlKick;
		mem_read[0x01] = MEM_getByteOvlKick;
		mem_read[0x02] = MEM_getByteOvlKick;
		mem_read[0x03] = MEM_getByteOvlKick;

		mem_write[0x00] = MEM_setByteKick;
		mem_write[0x01] = MEM_setByteKick;
		mem_write[0x02] = MEM_setByteKick;
		mem_write[0x03] = MEM_setByteKick;
	}
	else
	{
		mem_read[0x00] = MEM_getByteChip;
		mem_read[0x01] = MEM_getByteChip;
		mem_read[0x02] = MEM_getByteChip;
		mem_read[0x03] = MEM_getByteChip;

		mem_write[0x00] = MEM_setByteChip;
		mem_write[0x01] = MEM_setByteChip;
		mem_write[0x02] = MEM_setByteChip;
		mem_write[0x03] = MEM_setByteChip;
	}
}
