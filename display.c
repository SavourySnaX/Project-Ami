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


#include "display.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"

void DSP_InitialiseDisplay()
{
}

extern u_int8_t		horizontalClock;
extern u_int16_t	verticalClock;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo);

u_int32_t GetPixelAddress(u_int8_t bplBase,u_int32_t hor,u_int32_t ver)
{
    u_int32_t bpl1;

    bpl1 = cstMemory[bplBase+1]&0x07;
    bpl1 <<= 8;
    bpl1|= cstMemory[bplBase+2];
    bpl1 <<= 8;
    bpl1|= cstMemory[bplBase+3];

    bpl1+=hor/8;	// This is all a big fat lie!
    bpl1+=ver*(320/8);

    return bpl1;
}

void DecodePixel2(u_int32_t hor,u_int32_t ver)
{
    u_int8_t	ch,cl;
    u_int32_t	bpl1,bpl2;
    u_int32_t	pix1,pix2;

    bpl1 = GetPixelAddress(0xE0,hor,ver);
    bpl2 = GetPixelAddress(0xE4,hor,ver);

    pix1 = MEM_getByte(bpl1) & (1<<(7-(hor&7)));
    pix2 = MEM_getByte(bpl2) & (1<<(7-(hor&7)));

    if (pix1 && pix2)
    {
	ch=cstMemory[0x186];
	cl=cstMemory[0x187];
    }
    else if ((!pix1) && pix2)
    {
	ch=cstMemory[0x184];
	cl=cstMemory[0x185];
    }
    else if (pix1 && (!pix2))
    {
	ch=cstMemory[0x182];
	cl=cstMemory[0x183];
    }
    else
    {
	ch=cstMemory[0x180];
	cl=cstMemory[0x181];
    }

	doPixel(hor,ver,ch,cl);
}

void DecodePixel0(u_int32_t hor,u_int32_t ver)
{
    u_int8_t	ch,cl;

    ch=cstMemory[0x180];
    cl=cstMemory[0x181];

    doPixel(hor,ver,ch,cl);
}

void DSP_Update()
{
    switch ((cstMemory[0x100]&0x70)>>4)
    {
	case 0:
	    DecodePixel0(horizontalClock*2,verticalClock);
	    DecodePixel0(horizontalClock*2+1,verticalClock);
	    break;
	case 2:
	    DecodePixel2(horizontalClock*2,verticalClock);
	    DecodePixel2(horizontalClock*2+1,verticalClock);
	    break;
    }
}
