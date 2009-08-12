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
#include "sprite.h"

int pixelMask;
int applyMod;

int fetchSize=0;
int lFetchSize=0;

void DSP_InitialiseDisplay()
{
	pixelMask=0x8000;
	applyMod=0;
}

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo);

void DSP_NotifyLineEnd(int ver)
{
	pixelMask=0x8000;
//	applyMod=1;
#if ENABLE_DISPLAY_WARNINGS
	if (fetchSize!=lFetchSize)
	{
		printf("fetch size %d\n",fetchSize);
		lFetchSize=fetchSize;
	}
	
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

int DSP_DoSpriteCol(u_int32_t hClock,u_int32_t vClock)
{
	int colour;
	
	colour = SPR_GetColourNum(0,CST_SPR0PTH,CST_SPR0CTL,CST_SPR0POS,CST_SPR0DATA,CST_SPR0DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(1,CST_SPR1PTH,CST_SPR1CTL,CST_SPR1POS,CST_SPR1DATA,CST_SPR1DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(2,CST_SPR2PTH,CST_SPR2CTL,CST_SPR2POS,CST_SPR2DATA,CST_SPR2DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(3,CST_SPR3PTH,CST_SPR3CTL,CST_SPR3POS,CST_SPR3DATA,CST_SPR3DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(4,CST_SPR4PTH,CST_SPR4CTL,CST_SPR4POS,CST_SPR4DATA,CST_SPR4DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(5,CST_SPR5PTH,CST_SPR5CTL,CST_SPR5POS,CST_SPR5DATA,CST_SPR5DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(6,CST_SPR6PTH,CST_SPR6CTL,CST_SPR6POS,CST_SPR6DATA,CST_SPR6DATB,hClock,vClock);
	if (colour)
		return colour;
	colour = SPR_GetColourNum(7,CST_SPR7PTH,CST_SPR7CTL,CST_SPR7POS,CST_SPR7DATA,CST_SPR7DATB,hClock,vClock);

	return colour;
}

void DSP_HiRes()
{
	u_int8_t pixel1C = 0,pixel2C = 0,pixel3C = 0,pixel4C = 0,ch,cl;
	u_int8_t spr1C=0,spr2C=0;
	
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
	
	spr1C = DSP_DoSpriteCol(horizontalClock*2,verticalClock);
	spr2C = DSP_DoSpriteCol(horizontalClock*2+1,verticalClock);
	
	if (spr1C)
	{
		pixel1C = spr1C;
		pixel2C = spr1C;
	}
	if (spr2C)
	{
		pixel3C = spr2C;
		pixel4C = spr2C;
	}
	
	ch=CST_GETWRDU(CST_COLOR00 + pixel1C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel1C,0x00FF);

	doPixel(horizontalClock*4,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4,verticalClock*2+1,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel2C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel2C,0x00FF);

	doPixel(horizontalClock*4+1,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4+1,verticalClock*2+1,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel3C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel3C,0x00FF);

	doPixel(horizontalClock*4+2,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4+2,verticalClock*2+1,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel4C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel4C,0x00FF);

	doPixel(horizontalClock*4+3,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4+3,verticalClock*2+1,ch,cl);
}

void DSP_LoRes()
{
	u_int8_t pixel1C = 0,pixel2C = 0,ch,cl;
	u_int8_t spr1C=0,spr2C=0;
	
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

	spr1C = DSP_DoSpriteCol(horizontalClock*2,verticalClock);
	spr2C = DSP_DoSpriteCol(horizontalClock*2+1,verticalClock);
	
	if (spr1C)
	{
		pixel1C = spr1C;
	}
	if (spr2C)
	{
		pixel2C = spr2C;
	}

	ch=CST_GETWRDU(CST_COLOR00 + pixel1C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel1C,0x00FF);

	doPixel(horizontalClock*4,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4+1,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4,verticalClock*2+1,ch,cl);
	doPixel(horizontalClock*4+1,verticalClock*2+1,ch,cl);

	ch=CST_GETWRDU(CST_COLOR00 + pixel2C,0xFF00)>>8;
	cl=CST_GETWRDU(CST_COLOR00 + pixel2C,0x00FF);

	doPixel(horizontalClock*4+2,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4+3,verticalClock*2,ch,cl);
	doPixel(horizontalClock*4+2,verticalClock*2+1,ch,cl);
	doPixel(horizontalClock*4+3,verticalClock*2+1,ch,cl);
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
