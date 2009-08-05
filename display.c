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

int pixelMask;
int applyMod;

int fetchSize=0;

void DSP_InitialiseDisplay()
{
	pixelMask=0x8000;
	applyMod=0;
}

extern u_int8_t		horizontalClock;
extern u_int16_t	verticalClock;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo);

void DSP_NotifyLineEnd(int ver)
{
	pixelMask=0x8000;
//	applyMod=1;
#if ENABLE_DISPLAY_WARNINGS
	printf("fetch size %d\n",fetchSize);
#endif
	fetchSize=0;
}

u_int32_t GetPixelAddress(u_int16_t bplBase,u_int16_t bplMod,u_int32_t hor,u_int32_t ver)
{
    u_int32_t bpl1;

	bpl1 = CST_GETLNGU(bplBase,0x0007FFFE);

    bpl1+=hor/8;	// This is all a big fat lie!
    bpl1+=ver*((320+CST_GETWRDS(bplMod,0xFFFE))/8);

    return bpl1;
}

int DecodePixel(u_int32_t hor,u_int32_t ver,u_int16_t bplPth,u_int16_t bplMod,u_int16_t bplDat)
{
    u_int32_t	bpl;
	
	if (pixelMask==0x8000)
	{			
		bpl = CST_GETLNGU(bplPth,0x0007FFFE);

/*		if (applyMod)
		{
			applyMod=0;
			bpl+=CST_GETWRDS(bplMod,0xFFFF);
		}
*/	
		CST_SETWRD(bplDat,MEM_getWord(bpl),0xFFFF);
		
		bpl+=2;
		fetchSize+=2;
		
		CST_SETLNG(bplPth,bpl,0x0007FFFE);
	}
	
	if (CST_GETWRDU(bplDat,pixelMask))
	{
		return 1;
	}

	return 0;
}

void DSP_HiRes()
{
	u_int8_t pixel1C = 0,pixel2C = 0,pixel3C = 0,pixel4C = 0,ch,cl;
	
	if (horizontalClock>=(CST_GETWRDU(CST_DDFSTRT,0x00FC)) && horizontalClock<=(CST_GETWRDU(CST_DDFSTOP,0x00FC)+11))
	{
    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 7:		// should never happen
	case 6:
//		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL6PTH,CST_BPL2MOD) << ;		// NOTE PROBABLY USING HAM
	case 5:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL5PTH,CST_BPL1MOD,CST_BPL5DAT) << 5;
	case 4:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL4PTH,CST_BPL2MOD,CST_BPL4DAT) << 4;
	case 3:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL3PTH,CST_BPL1MOD,CST_BPL3DAT) << 3;
	case 2:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL2PTH,CST_BPL2MOD,CST_BPL2DAT) << 2;
	case 1:
		pixel1C |= DecodePixel((horizontalClock-CST_GETWRDU(CST_DDFSTRT,0x00FC))*2,verticalClock,CST_BPL1PTH,CST_BPL1MOD,CST_BPL1DAT) << 1;
	case 0:
	    break;
    }
		pixelMask>>=1;
	if (!pixelMask)
		pixelMask=0x8000;
			
    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 7:		// should never happen
	case 6:
//		pixel2C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL6PTH,CST_BPL2MOD);
	case 5:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL5PTH,CST_BPL1MOD,CST_BPL5DAT) << 5;	
	case 4:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL4PTH,CST_BPL2MOD,CST_BPL4DAT) << 4;	
	case 3:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL3PTH,CST_BPL1MOD,CST_BPL3DAT) << 3;	
	case 2:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL2PTH,CST_BPL2MOD,CST_BPL2DAT) << 2;	
	case 1:
		pixel2C |= DecodePixel((horizontalClock-CST_GETWRDU(CST_DDFSTRT,0x00FC))*2+1,verticalClock,CST_BPL1PTH,CST_BPL1MOD,CST_BPL1DAT) << 1;	
	case 0:
	    break;
    }
	pixelMask>>=1;
	if (!pixelMask)
		pixelMask=0x8000;

    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 7:		// should never happen
	case 6:
//		pixel3C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL6PTH,CST_BPL2MOD) << ;		// NOTE PROBABLY USING HAM
	case 5:
		pixel3C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL5PTH,CST_BPL1MOD,CST_BPL5DAT) << 5;
	case 4:
		pixel3C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL4PTH,CST_BPL2MOD,CST_BPL4DAT) << 4;
	case 3:
		pixel3C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL3PTH,CST_BPL1MOD,CST_BPL3DAT) << 3;
	case 2:
		pixel3C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL2PTH,CST_BPL2MOD,CST_BPL2DAT) << 2;
	case 1:
		pixel3C |= DecodePixel((horizontalClock-CST_GETWRDU(CST_DDFSTRT,0x00FC))*2,verticalClock,CST_BPL1PTH,CST_BPL1MOD,CST_BPL1DAT) << 1;
	case 0:
	    break;
    }
		pixelMask>>=1;
	if (!pixelMask)
		pixelMask=0x8000;
			
    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 7:		// should never happen
	case 6:
//		pixel4C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL6PTH,CST_BPL2MOD) << ;		// NOTE PROBABLY USING HAM
	case 5:
		pixel4C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL5PTH,CST_BPL1MOD,CST_BPL5DAT) << 5;
	case 4:
		pixel4C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL4PTH,CST_BPL2MOD,CST_BPL4DAT) << 4;
	case 3:
		pixel4C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL3PTH,CST_BPL1MOD,CST_BPL3DAT) << 3;
	case 2:
		pixel4C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL2PTH,CST_BPL2MOD,CST_BPL2DAT) << 2;
	case 1:
		pixel4C |= DecodePixel((horizontalClock-CST_GETWRDU(CST_DDFSTRT,0x00FC))*2,verticalClock,CST_BPL1PTH,CST_BPL1MOD,CST_BPL1DAT) << 1;
	case 0:
	    break;
    }
		pixelMask>>=1;
	if (!pixelMask)
		pixelMask=0x8000;
			
	}
	ch=CST_GETWRDU(CST_COLOR00 + pixel1C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel1C,0x00FF);

	doPixel(horizontalClock*4,verticalClock,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel2C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel2C,0x00FF);

	doPixel(horizontalClock*4+1,verticalClock,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel3C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel3C,0x00FF);

	doPixel(horizontalClock*4+2,verticalClock,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel4C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel4C,0x00FF);

	doPixel(horizontalClock*4+3,verticalClock,ch,cl);
}

void DSP_LoRes()
{
	u_int8_t pixel1C = 0,pixel2C = 0,ch,cl;
	
	if (horizontalClock>=(CST_GETWRDU(CST_DDFSTRT,0x00FC)) && horizontalClock<=(CST_GETWRDU(CST_DDFSTOP,0x00FC)+7))
	{
    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 7:		// should never happen
	case 6:
//		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL6PTH,CST_BPL2MOD) << ;		// NOTE PROBABLY USING HAM
	case 5:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL5PTH,CST_BPL1MOD,CST_BPL5DAT) << 5;
	case 4:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL4PTH,CST_BPL2MOD,CST_BPL4DAT) << 4;
	case 3:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL3PTH,CST_BPL1MOD,CST_BPL3DAT) << 3;
	case 2:
		pixel1C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL2PTH,CST_BPL2MOD,CST_BPL2DAT) << 2;
	case 1:
		pixel1C |= DecodePixel((horizontalClock-CST_GETWRDU(CST_DDFSTRT,0x00FC))*2,verticalClock,CST_BPL1PTH,CST_BPL1MOD,CST_BPL1DAT) << 1;
	case 0:
	    break;
    }
		pixelMask>>=1;
	if (!pixelMask)
		pixelMask=0x8000;
			
    switch (CST_GETWRDU(CST_BPLCON0,0x7000)>>12)
    {
	case 7:		// should never happen
	case 6:
//		pixel2C |= DecodePixel(horizontalClock*2,verticalClock,CST_BPL6PTH,CST_BPL2MOD);
	case 5:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL5PTH,CST_BPL1MOD,CST_BPL5DAT) << 5;	
	case 4:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL4PTH,CST_BPL2MOD,CST_BPL4DAT) << 4;	
	case 3:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL3PTH,CST_BPL1MOD,CST_BPL3DAT) << 3;	
	case 2:
		pixel2C |= DecodePixel(horizontalClock*2+1,verticalClock,CST_BPL2PTH,CST_BPL2MOD,CST_BPL2DAT) << 2;	
	case 1:
		pixel2C |= DecodePixel((horizontalClock-CST_GETWRDU(CST_DDFSTRT,0x00FC))*2+1,verticalClock,CST_BPL1PTH,CST_BPL1MOD,CST_BPL1DAT) << 1;	
	case 0:
	    break;
    }
	pixelMask>>=1;
	if (!pixelMask)
		pixelMask=0x8000;

	}
	ch=CST_GETWRDU(CST_COLOR00 + pixel1C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel1C,0x00FF);

	doPixel(horizontalClock*4,verticalClock,ch,cl);
	doPixel(horizontalClock*4+1,verticalClock,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel2C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel2C,0x00FF);

	doPixel(horizontalClock*4+2,verticalClock,ch,cl);
	doPixel(horizontalClock*4+3,verticalClock,ch,cl);
}

void DSP_Update()
{
	if (CST_GETWRDU(CST_BPLCON0,0x8000))
	{
		DSP_HiRes();
	}
	else
	{
		DSP_LoRes();
	}
}
