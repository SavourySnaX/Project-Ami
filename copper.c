/*
 *  copper.c
 *  ami
 *
 *  Created by Lee Hammerton on 11/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#define __IGNORE_TYPES
#include "memory.h"
#include "copper.h"
#include "customchip.h"

extern u_int8_t		*cstMemory;
extern u_int8_t		horizontalClock;
extern u_int16_t	verticalClock;

u_int32_t	copperPC;
int		copperCycle;

void CPR_SetPC(u_int32_t pc)
{
	copperCycle=0;
	copperPC = pc;
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

	if ((cstMemory[0x03]&0x80) && (cstMemory[0x02]&0x02))
	{
		// copper is enabled
		
		if (copperCycle==0)
		{
			wrd = MEM_getWord(copperPC);
			copperPC+=2;
			cstMemory[0x8C]=wrd>>8;
			cstMemory[0x8D]=wrd&0xFF;
			copperCycle=1;
		}
		else
		{
			copperCycle=0;
			if (cstMemory[0x8D]&0x01)
			{
				// doing a skip or wait
				
				wrd = MEM_getWord(copperPC);

				if (!(wrd&0x01))
				{
					// doing a wait
					u_int8_t vpos=cstMemory[0x8C]&0xFF;
					u_int8_t hpos=cstMemory[0x8D]&0xFE;
					
					vpos&=wrd>>8;
					hpos&=wrd&0xFF;

//					printf("Copper Wait %02x%02x:%04x\n",cstMemory[0x8C],cstMemory[0x8D],wrd);
					if ((verticalClock&0xFF)>vpos || ((verticalClock&0xFF)==vpos && (horizontalClock&0xFF)>=hpos))
					{
					}
					else
					{
						copperCycle=1;		//wait
					}
				}
				else
				{
					// doing a skip
					printf("Copper Skip:\n");
				}
			}
			else
			{
				// doing a move
				u_int16_t destination;
			       
				wrd = MEM_getWord(copperPC);
				copperPC+=2;

//				printf("Copper Move:\n");
				destination = ((u_int16_t)(cstMemory[0x8C]&0x01))<<8;
				destination|=cstMemory[0x8D]&0xFE;

				cstMemory[0x8C]=wrd>>8;
				cstMemory[0x8D]=wrd&0xFF;

				if (destination>=0x20) // need to check copper danger bit and allow 0x10 and above addresses
				{
					MEM_setWord(0xdff000+destination,wrd);
				}
			}
		}
	}
}
