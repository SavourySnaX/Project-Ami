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

	bpl1 = CST_GETLNGU(bplBase,0x0007FFFE);

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
		ch=CST_GETWRDU(CST_COLOR03,0xFF00)>>8;
		cl=CST_GETWRDU(CST_COLOR03,0x00FF);
    }
    else if ((!pix1) && pix2)
    {
		ch=CST_GETWRDU(CST_COLOR02,0xFF00)>>8;
		cl=CST_GETWRDU(CST_COLOR02,0x00FF);
    }
    else if (pix1 && (!pix2))
    {
		ch=CST_GETWRDU(CST_COLOR01,0xFF00)>>8;
		cl=CST_GETWRDU(CST_COLOR01,0x00FF);
    }
    else
    {
		ch=CST_GETWRDU(CST_COLOR00,0xFF00)>>8;
		cl=CST_GETWRDU(CST_COLOR00,0x00FF);
    }

	doPixel(hor,ver,ch,cl);
}

void DecodePixel0(u_int32_t hor,u_int32_t ver)
{
    u_int8_t	ch,cl;

	ch=CST_GETWRDU(CST_COLOR00,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00,0x00FF);

    doPixel(hor,ver,ch,cl);
}

void DSP_Update()
{
    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 0:
		DecodePixel0(horizontalClock*2,verticalClock);
		DecodePixel0(horizontalClock*2+1,verticalClock);
	    break;
	case 2:
//		if (horizontalClock>=(CST_GETWRDU(CST_DDFSTRT,0x00FC)) && horizontalClock<=(CST_GETWRDU(CST_DDFSTOP,0x00FC)))
		{
			DecodePixel2(horizontalClock*2,verticalClock);
			DecodePixel2(horizontalClock*2+1,verticalClock);
		}
/*		else
		{
			DecodePixel0(horizontalClock*2,verticalClock);
			DecodePixel0(horizontalClock*2+1,verticalClock);
		}*/		
	    break;
    }
}
