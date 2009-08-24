/*
 *  copper.c
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


#include "memory.h"
#include "copper.h"
#include "customchip.h"

extern u_int8_t		horizontalClock;
extern u_int16_t	verticalClock;

u_int32_t	copperPC;
int		copperCycle;

void CPR_SetPC(u_int32_t pc)
{
	copperCycle=0;
	copperPC = pc;

#if ENABLE_COPPER_WARNINGS
	printf("COPPER JUMPED %08X\n",pc);
#endif
}

void CPR_InitialiseCopper()
{
	copperPC=0;		// copper dma defaults to off so doesn't
				//really matter
	copperCycle=0;
}

void CPR_Update()
{
	u_int16_t wrd;

	if (CST_GETWRDU(CST_DMACONR,0x0280))
	{
		// copper is enabled
		
		if (copperCycle==0)
		{
			wrd = MEM_getWord(copperPC);
			copperPC+=2;
			CST_SETWRD(CST_COPINS,wrd,0xFFFF);
			copperCycle=1;
		}
		else
		{
			copperCycle=0;
			if (CST_GETWRDU(CST_COPINS,0x0001))
			{
				// doing a skip or wait
				wrd = MEM_getWord(copperPC);

				if (!(wrd&0x01))
				{
					int bltFinished = 1;
					// doing a wait
					u_int8_t vpos=CST_GETWRDU(CST_COPINS,0xFF00)>>8;
					u_int8_t hpos=CST_GETWRDU(CST_COPINS,0x00FE);
					u_int8_t maskv;
					u_int8_t maskh;
					
					if (!(wrd&0x8000))
					{
						bltFinished = !(CST_GETWRDU(CST_DMACONR,0x4000));
					}
					
					maskv=0x80|((wrd>>8)&0x7F);
					maskh=(wrd&0xFE);
					
					if (bltFinished && ((verticalClock&maskv)>vpos || ((verticalClock&maskv)==vpos && (horizontalClock&maskh)>=hpos)))
					{
						copperPC+=2;
					}
					else
					{
						copperCycle=1;		//wait
					}
				}
				else
				{
					// doing a skip
					int bltFinished = 1;

					u_int8_t vpos=CST_GETWRDU(CST_COPINS,0xFF00)>>8;
					u_int8_t hpos=CST_GETWRDU(CST_COPINS,0x00FE);
					u_int8_t maskv;
					u_int8_t maskh;
					
					if (!(wrd&0x8000))
					{
						bltFinished = !(CST_GETWRDU(CST_DMACONR,0x4000));
					}
					
					maskv=0x80|((wrd>>8)&0x7F);
					maskh=(wrd&0xFE);
					
					if (bltFinished && ((verticalClock&maskv)>vpos || ((verticalClock&maskv)==vpos && (horizontalClock&maskh)>=hpos)))
					{
						copperPC+=6;		// skip next instruction
					}
					else
					{
						copperPC+=2;
					}
				}
			}
			else
			{
				// doing a move
				u_int16_t destination;
				u_int8_t  copperDanger=0x80;
			       
				wrd = MEM_getWord(copperPC);
				copperPC+=2;

				destination = CST_GETWRDU(CST_COPINS,0x01FE);

				CST_SETWRD(CST_COPINS,wrd,0xFFFF);

				if (CST_GETWRDU(CST_COPCON,0x0002))
				{
					copperDanger=0x40;
				}
				
				if (destination>=copperDanger) // need to check copper danger bit and allow 0x10 and above addresses
				{
					MEM_setWord(0xdff000+destination,wrd);
				}
			}
		}
	}
}
