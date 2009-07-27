/*
 *  display.c
 *  ami
 *
 *  Created by Lee Hammerton on 26/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#if !DISABLE_DISPLAY

#include "SDL.h"

#define __IGNORE_TYPES
#endif

#include "display.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"

extern u_int8_t		*cstMemory;

void DSP_InitialiseDisplay()
{
}

extern u_int8_t		horizontalClock;
extern u_int16_t	verticalClock;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo);

void DSP_Update()
{
	u_int8_t ch=cstMemory[0x180],cl=cstMemory[0x181];

	// hack!
	u_int32_t bpl0 = cstMemory[0xE1]&0x07;
	bpl0 <<=8;
	bpl0|=cstMemory[0xE2];
	bpl0 <<=8;
	bpl0|=cstMemory[0xE3];

	bpl0+=(horizontalClock*2)/8;	// This is all a big fat lie!
	bpl0+=verticalClock*(320/8);

	u_int8_t bpldata = MEM_getByte(bpl0);

	if (bpldata & (1<<((horizontalClock*2)&7)))
	{
		ch=cstMemory[0x182];
		cl=cstMemory[0x183];
	}

	// end hack!

	doPixel(horizontalClock*2,verticalClock,ch,cl);

	ch=cstMemory[0x180],cl=cstMemory[0x181];

	// hack!
	bpl0 = cstMemory[0xE1]&0x07;
	bpl0 <<=8;
	bpl0|=cstMemory[0xE2];
	bpl0 <<=8;
	bpl0|=cstMemory[0xE3];

	bpl0+=(horizontalClock*2+1)/8;	// This is all a big fat lie!
	bpl0+=verticalClock*(320/8);

	bpldata = MEM_getByte(bpl0);

	if (bpldata & (1<<((horizontalClock*2+1)&7)))
	{
		ch=cstMemory[0x182];
		cl=cstMemory[0x183];
	}

	// end hack!
	doPixel(horizontalClock*2+1,verticalClock,ch,cl);
}
