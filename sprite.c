/*
 *  sprite.c
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

#include "config.h"


#include "sprite.h"
#include "display.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"

int spr[8];

void SPR_InitialiseSprites()
{
	int a;
	for (a=0;a<8;a++)
	{
		spr[a]=0;
	}
}

// doesn't handle attached sprites!
int SPR_GetColourNum(int spNum,u_int16_t sprPtr,u_int16_t sprCtl,u_int16_t sprPos,u_int16_t sprDatA,u_int16_t sprDatB, u_int16_t hpos,u_int16_t vpos)
{
	u_int16_t vStart = (CST_GETWRDU(sprCtl,0x0004) << 6) | (CST_GETWRDU(sprPos,0xFF00) >> 8);
	u_int16_t vStop = (CST_GETWRDU(sprCtl,0x0002) << 7) | (CST_GETWRDU(sprCtl,0xFF00) >> 8);
	u_int16_t hStart = (CST_GETWRDU(sprPos,0x00FF)<<1) | (CST_GETWRDU(sprCtl,0x0001)) ;
		
	if (vpos>=vStart && vpos<=vStop)
	{
		// data in register valid - hack
		int pixel = hpos-hStart;
		if (pixel>=0 && pixel<=15)
		{
			u_int16_t datA = CST_GETWRDU(sprDatA,(1<<(15-pixel)));
			u_int16_t datB = CST_GETWRDU(sprDatB,(1<<(15-pixel)));
			
			if (datA && datB)
				return 38 + (spNum/2)*4;
			if ((!datA) && datB)
				return 36 + (spNum/2)*4;
			if (datA && (!datB))
				return 34 + (spNum/2)*4;
		}
	}
	
	return 0;
}

void SPR_Process(int spNum,u_int16_t sprPtr,u_int16_t sprCtl,u_int16_t sprPos,u_int16_t sprDatA,u_int16_t sprDatB, u_int16_t vpos)
{
	u_int32_t sprAddr=CST_GETLNGU(sprPtr,0x0007FFFE);
	
	if (vpos==0)
	{
		spr[spNum]=0;				// Reset first word fetch on sprite (hack)
	}
	
	if (spr[spNum])
	{
		u_int16_t vStart = (CST_GETWRDU(sprCtl,0x0004) << 6) | (CST_GETWRDU(sprPos,0xFF00) >> 8);
		u_int16_t vStop = (CST_GETWRDU(sprCtl,0x0002) << 7) | (CST_GETWRDU(sprCtl,0xFF00) >> 8);
		
		if (vpos>=vStart)
		{
			if (vpos>vStop)
			{
				spr[spNum]=0;
			}
			else
			{
				CST_SETWRD(sprDatA,MEM_getWord(sprAddr),0xFFFF);
				CST_SETWRD(sprDatB,MEM_getWord(sprAddr+2),0xFFFF);
				sprAddr+=4;
			}
		}
	}
	else
	{
		// First fetch
		CST_SETWRD(sprPos,MEM_getWord(sprAddr),0xFFFF);
		CST_SETWRD(sprCtl,MEM_getWord(sprAddr+2),0xFFFF);
		if (CST_GETWRDU(sprPos,0xFFFF) && CST_GETWRDU(sprCtl,0xFFFF))
		{
			spr[spNum]=1;
			sprAddr+=4;
		}
	}
	
	CST_SETLNG(sprPtr,sprAddr,0x0007FFFE);
}

void SPR_Update()
{
	if (CST_GETWRDU(CST_DMACONR,0x0220)==0x0220)
	{
		switch (horizontalClock)
		{
			case 0x14:
				SPR_Process(0,CST_SPR0PTH,CST_SPR0CTL,CST_SPR0POS,CST_SPR0DATA,CST_SPR0DATB,verticalClock);
				break;
			case 0x18:
				SPR_Process(1,CST_SPR1PTH,CST_SPR1CTL,CST_SPR1POS,CST_SPR1DATA,CST_SPR1DATB,verticalClock);
				break;
			case 0x1C:
				SPR_Process(2,CST_SPR2PTH,CST_SPR2CTL,CST_SPR2POS,CST_SPR2DATA,CST_SPR2DATB,verticalClock);
				break;
			case 0x20:
				SPR_Process(3,CST_SPR3PTH,CST_SPR3CTL,CST_SPR3POS,CST_SPR3DATA,CST_SPR3DATB,verticalClock);
				break;
			case 0x24:
				SPR_Process(4,CST_SPR4PTH,CST_SPR4CTL,CST_SPR4POS,CST_SPR4DATA,CST_SPR4DATB,verticalClock);
				break;
			case 0x28:
				SPR_Process(5,CST_SPR5PTH,CST_SPR5CTL,CST_SPR5POS,CST_SPR5DATA,CST_SPR5DATB,verticalClock);
				break;
			case 0x2C:
				SPR_Process(6,CST_SPR6PTH,CST_SPR6CTL,CST_SPR6POS,CST_SPR6DATA,CST_SPR6DATB,verticalClock);
				break;
			case 0x30:
				SPR_Process(7,CST_SPR7PTH,CST_SPR7CTL,CST_SPR7POS,CST_SPR7DATA,CST_SPR7DATB,verticalClock);
				break;
		}
	}
}
