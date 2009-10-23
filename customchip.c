/*
 *  customchip.c
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
#include <string.h>

#include "config.h"

#include "copper.h"
#include "customchip.h"
#include "ciachip.h"
#include "blitter.h"
#include "display.h"
#include "disk.h"

typedef u_int8_t (*CST_ReadMap)(u_int16_t reg);
typedef void (*CST_WriteMap)(u_int16_t reg,u_int8_t byte);

typedef enum
	{
		CST_READABLE=0x01,
		CST_WRITEABLE=0x02,
		CST_STROBEABLE=0x04,
		CST_FUNCTION=0x08,			// indicates we have a custom handler
		CST_SUPPORTED=0x10,			// indicates i think this is supported correctly
		CST_END=0x8000,
	} CST_TYPE;

typedef struct
	{
		char			name[32];
		CST_TYPE		type;
		CST_ReadMap		readFunc;
		CST_WriteMap	writeFunc;
	} CST_Regs;

#define CUSTOMCHIPMEMORY (0x1000)

u_int8_t	*cstMemory = 0;

CST_ReadMap		cst_read[CUSTOMCHIPMEMORY];
CST_WriteMap	cst_write[CUSTOMCHIPMEMORY];

u_int8_t	horizontalClock=0;
u_int16_t	verticalClock=0;

u_int8_t		leftMouseUp,rightMouseUp;

extern int g_newScreenNotify;

int cLineLength=LINE_LENGTH - 1;	// first line is a short line

void CST_Update()
{
	horizontalClock++;
	if (horizontalClock>cLineLength)
	{
		DSP_NotifyLineEnd();
	
		if (cLineLength!=LINE_LENGTH)
			cLineLength=LINE_LENGTH;
		else
			cLineLength=LINE_LENGTH-1;

		horizontalClock=0;
		if (todBStart)
			todBCnt++;
		verticalClock++;
		if (verticalClock>=313)
		{
			verticalClock=0;

			// I`m going to need to clean this all up really (need a proper bus arbitration too)

			CST_ORWRD(CST_INTREQR,0x0020);				// Have processor poll INTREQR for jobs

			cLineLength=LINE_LENGTH-1;
 
			// reload copper on vbl
			CPR_SetPC(CST_GETLNGU(CST_COP1LCH,0x0007FFFE));

			g_newScreenNotify=1;

			if (todAStart)
				todACnt++;
		}
	}
}

void CST_setByteCOPJMP1(u_int16_t reg,u_int8_t dontcarestrobe)
{
	CPR_SetPC(CST_GETLNGU(CST_COP1LCH,0x0007FFFE));
}

void CST_setByteCOPJMP2(u_int16_t reg,u_int8_t dontcarestrobe)
{
	CPR_SetPC(CST_GETLNGU(CST_COP2LCH,0x0007FFFE));
}

void CST_setByteINTENA(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		// second byte set do the operation
		if (CST_GETWRDU(CST_INTENA,0x8000))		// need to or in the set bits
		{
			CST_ORWRD(CST_INTENAR,CST_GETWRDU(CST_INTENA,0x7FFF));
		}
		else							// need to mask of the set bits
		{
			CST_ANDWRD(CST_INTENAR,~CST_GETWRDU(CST_INTENA,0xFFFF));
		}
	}

//	printf("INTENAR : %02X%02X\n",cstMemory[0x1C],cstMemory[0x1D]);
}

void CST_setByteINTREQ(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		// second byte set do the operation
		if (CST_GETWRDU(CST_INTREQ,0x8000))		// need to or in the set bits
		{
			CST_ORWRD(CST_INTREQR,CST_GETWRDU(CST_INTREQ,0x7FFF));
		}
		else							// need to mask of the set bits
		{
			CST_ANDWRD(CST_INTREQR,~CST_GETWRDU(CST_INTREQ,0xFFFF));
		}
	}
//	printf("INTREQR : %02X%02X\n",cstMemory[0x1E],cstMemory[0x1F]);
}

void CST_setByteADKCON(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		// second byte set do the operation
		if (CST_GETWRDU(CST_ADKCON,0x8000))		// need to or in the set bits
		{
			CST_ORWRD(CST_ADKCONR,CST_GETWRDU(CST_ADKCON,0x7FFF));
		}
		else							// need to mask of the set bits
		{
			CST_ANDWRD(CST_ADKCONR,~CST_GETWRDU(CST_ADKCON,0xFFFF));
		}
	}

//	printf("ADKCON : %04X\n",CST_GETWRDU(CST_ADKCONR,0xFFFF));
}


#define AGNUS_ID		0					// NTSC agnus or fat agnus

u_int8_t CST_getByteVPOSR(u_int16_t reg)
{
	CST_SETWRD(CST_VPOSR,(AGNUS_ID<<8) | 0 | (verticalClock>>8),0xFF01);
/*	   ((verticalClock&0x0100)|(AGNUS_ID | 0)),0xFFFF);
	if (reg&1)
	{
		CST_SET
		cstMemory[0x05]=(verticalClock>>8)&1;				// bit 1 is high part of vertical line (ie > 255)
	}
	else
	{
		cstMemory[0x04]=AGNUS_ID | 0;	// 0 is LOF long frame but i think thats an interlaced thing
	}*/
	return cstMemory[reg];
}

u_int8_t CST_getByteVHPOSR(u_int16_t reg)
{
	CST_SETWRD(CST_VHPOSR,(verticalClock<<8)|horizontalClock,0xFFFF);
/*
	if (reg&1)
	{
		cstMemory[0x07]=horizontalClock;				// bit 1 is high part of vertical line (ie > 255)
	}
	else
	{
		cstMemory[0x06]=verticalClock&0xFF;
	}*/
	return cstMemory[reg];
}

void CST_setByteDMACON(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		// second byte set do the operation
		if (CST_GETWRDU(CST_DMACON,0x8000))		// need to or in the set bits
		{
			CST_ORWRD(CST_DMACONR,CST_GETWRDU(CST_DMACON,0x7FFF));
		}
		else							// need to mask of the set bits
		{
			CST_ANDWRD(CST_DMACONR,~CST_GETWRDU(CST_DMACON,0xFFFF));
		}
	}
}

void CST_setByteBLTSIZE(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		BLT_StartBlit();
	}
}

extern int startDebug;

u_int8_t CST_getByteJOY0DAT(u_int16_t reg)
{
	return cstMemory[reg];
}

void CST_setBytePOTGO(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		// | 0 | 0 | 0 | 0 | 0 | 0 | 0 | START
		cstMemory[CST_POTGOR+1]=0x00;					// PAULA ID = 0
	}
	else
	{
		// | OUTRY | DATRY | OUTRX | DATRX | OUTLY | DATLY | OUTLX | DATLX
		cstMemory[CST_POTGOR]=0x00;
		if ((byte&0x0C)==0x0C)	// check mouse right button
		{
			if (rightMouseUp)
			{
				cstMemory[CST_POTGOR]|=0x04;			// Note bit set means not pressed!
			}
		}
		if ((byte&0x03)==0x03)	// check ????
		{
			cstMemory[CST_POTGOR]|=0x01;			// Note bit set means not pressed!
		}
	}
}

void CST_setByteDSKLEN(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		DSK_NotifyDSKLEN(CST_GETWRDU(CST_DSKLEN,0xFFFF));
	}
}

u_int8_t CST_getByteDSKBYTR(u_int16_t reg)
{
	u_int16_t wrd;
	
	wrd = 0;
	if (CST_GETWRDU(CST_DMACONR,0x0210)==0x0210 && CST_GETWRDU(CST_DSKLEN,0x8000))
		wrd|=0x4000;
	if (CST_GETWRDU(CST_DSKLEN,0x4000))
		wrd|=0x2000;
	if (DSK_OnSyncWord())
		wrd|=0x1000;
	
	CST_SETWRD(CST_DSKLEN,wrd,0xFFFF);

	return cstMemory[reg];
}


CST_Regs customChipRegisters[] =
{
{"BLTDDAT",CST_READABLE},
{"DMACONR",CST_READABLE|CST_SUPPORTED},
{"VPOSR",CST_READABLE|CST_SUPPORTED|CST_FUNCTION,CST_getByteVPOSR,0},
{"VHPOSR",CST_READABLE|CST_SUPPORTED|CST_FUNCTION,CST_getByteVHPOSR,0},
{"DSKDATR",CST_READABLE},
{"JOY0DAT",CST_READABLE|CST_SUPPORTED|CST_FUNCTION,CST_getByteJOY0DAT,0},
{"JOY1DAT",CST_READABLE},
{"CLXDAT",CST_READABLE},
{"ADKCONR",CST_READABLE|CST_SUPPORTED},
{"POT0DAT",CST_READABLE},
{"POT1DAT",CST_READABLE},
{"POTGOR",CST_READABLE|CST_SUPPORTED},
{"SERDATR",CST_READABLE},
{"DSKBYTR",CST_READABLE|CST_SUPPORTED|CST_FUNCTION,CST_getByteDSKBYTR,0},
{"INTENAR",CST_READABLE|CST_SUPPORTED},
{"INTREQR",CST_READABLE|CST_SUPPORTED},
{"DSKPTH",CST_WRITEABLE|CST_SUPPORTED},
{"DSKPTL",CST_WRITEABLE|CST_SUPPORTED},
{"DSKLEN",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteDSKLEN},
{"DSKDAT",CST_WRITEABLE},
{"REFPT",CST_WRITEABLE},
{"VPOSW",CST_WRITEABLE},
{"VHPOSW",CST_WRITEABLE},
{"COPCON",CST_WRITEABLE},
{"SERDAT",CST_WRITEABLE},
{"SERPER",CST_WRITEABLE},
{"POTGO",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setBytePOTGO},
{"JOYTEST",CST_WRITEABLE},
{"STREQU",CST_STROBEABLE},
{"STRVBL",CST_STROBEABLE},
{"STRHOR",CST_STROBEABLE},
{"STRLONG",CST_STROBEABLE},
{"BLTCON0",CST_WRITEABLE|CST_SUPPORTED},
{"BLTCON1",CST_WRITEABLE|CST_SUPPORTED},
{"BLTAFWM",CST_WRITEABLE|CST_SUPPORTED},
{"BLTALWM",CST_WRITEABLE|CST_SUPPORTED},
{"BLTCPTH",CST_WRITEABLE|CST_SUPPORTED},
{"BLTCPTL",CST_WRITEABLE|CST_SUPPORTED},
{"BLTBPTH",CST_WRITEABLE|CST_SUPPORTED},
{"BLTBPTL",CST_WRITEABLE|CST_SUPPORTED},
{"BLTAPTH",CST_WRITEABLE|CST_SUPPORTED},
{"BLTAPTL",CST_WRITEABLE|CST_SUPPORTED},
{"BLTDPTH",CST_WRITEABLE|CST_SUPPORTED},
{"BLTDPTL",CST_WRITEABLE|CST_SUPPORTED},
{"BLTSIZE",CST_WRITEABLE|CST_FUNCTION|CST_SUPPORTED,0,CST_setByteBLTSIZE},
{"05A(ECS)",0},
{"05C(ECS)",0},
{"05E(ECS)",0},
{"BLTCMOD",CST_WRITEABLE|CST_SUPPORTED},
{"BLTBMOD",CST_WRITEABLE|CST_SUPPORTED},
{"BLTAMOD",CST_WRITEABLE|CST_SUPPORTED},
{"BLTDMOD",CST_WRITEABLE|CST_SUPPORTED},
{"068",0},
{"06A",0},
{"06C",0},
{"06E",0},
{"BLTCDAT",CST_WRITEABLE|CST_SUPPORTED},
{"BLTBDAT",CST_WRITEABLE|CST_SUPPORTED},
{"BLTADAT",CST_WRITEABLE|CST_SUPPORTED},
{"076",0},
{"SPRHDAT",CST_WRITEABLE},
{"07A",0},
{"07C(ECS)",0},
{"DSKSYNC",CST_WRITEABLE|CST_SUPPORTED},
{"COP1LCH",CST_WRITEABLE|CST_SUPPORTED},
{"COP1LCL",CST_WRITEABLE|CST_SUPPORTED},
{"COP2LCH",CST_WRITEABLE|CST_SUPPORTED},
{"COP2LCL",CST_WRITEABLE|CST_SUPPORTED},
{"COPJMP1",CST_STROBEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteCOPJMP1},
{"COPJMP2",CST_STROBEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteCOPJMP2},
{"COPINS",CST_WRITEABLE|CST_SUPPORTED},
{"DIWSTRT",CST_WRITEABLE},
{"DIWSTOP",CST_WRITEABLE},
{"DDFSTRT",CST_WRITEABLE},
{"DDFSTOP",CST_WRITEABLE},
{"DMACON",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteDMACON},
{"CLXCON",CST_WRITEABLE},
{"INTENA",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteINTENA},
{"INTREQ",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteINTREQ},
{"ADKCON",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteADKCON},
{"AUD0LCH",CST_WRITEABLE},
{"AUD0LCL",CST_WRITEABLE},
{"AUD0LEN",CST_WRITEABLE},
{"AUD0PER",CST_WRITEABLE},
{"AUD0VOL",CST_WRITEABLE},
{"AUD0DAT",CST_WRITEABLE},
{"0AC",0},
{"0AE",0},
{"AUD1LCH",CST_WRITEABLE},
{"AUD1LCL",CST_WRITEABLE},
{"AUD1LEN",CST_WRITEABLE},
{"AUD1PER",CST_WRITEABLE},
{"AUD1VOL",CST_WRITEABLE},
{"AUD1DAT",CST_WRITEABLE},
{"0BC",0},
{"0BE",0},
{"AUD2LCH",CST_WRITEABLE},
{"AUD2LCL",CST_WRITEABLE},
{"AUD2LEN",CST_WRITEABLE},
{"AUD2PER",CST_WRITEABLE},
{"AUD2VOL",CST_WRITEABLE},
{"AUD2DAT",CST_WRITEABLE},
{"0CC",0},
{"0CE",0},
{"AUD3LCH",CST_WRITEABLE},
{"AUD3LCL",CST_WRITEABLE},
{"AUD3LEN",CST_WRITEABLE},
{"AUD3PER",CST_WRITEABLE},
{"AUD3VOL",CST_WRITEABLE},
{"AUD3DAT",CST_WRITEABLE},
{"0DC",0},
{"0DE",0},
{"BPL1PTH",CST_WRITEABLE|CST_SUPPORTED},
{"BPL1PTL",CST_WRITEABLE|CST_SUPPORTED},
{"BPL2PTH",CST_WRITEABLE|CST_SUPPORTED},
{"BPL2PTL",CST_WRITEABLE|CST_SUPPORTED},
{"BPL3PTH",CST_WRITEABLE|CST_SUPPORTED},
{"BPL3PTL",CST_WRITEABLE|CST_SUPPORTED},
{"BPL4PTH",CST_WRITEABLE|CST_SUPPORTED},
{"BPL4PTL",CST_WRITEABLE|CST_SUPPORTED},
{"BPL5PTH",CST_WRITEABLE|CST_SUPPORTED},
{"BPL5PTL",CST_WRITEABLE|CST_SUPPORTED},
{"BPL6PTH",CST_WRITEABLE|CST_SUPPORTED},
{"BPL6PTL",CST_WRITEABLE|CST_SUPPORTED},
{"0F8",0},
{"0FA",0},
{"0FC",0},
{"0FE",0},
{"BPLCON0",CST_WRITEABLE},
{"BPLCON1",CST_WRITEABLE},
{"BPLCON2",CST_WRITEABLE},
{"106(ECS)",0},
{"BPL1MOD",CST_WRITEABLE},
{"BPL2MOD",CST_WRITEABLE},
{"10C",0},
{"10E",0},
{"BPL1DAT",CST_WRITEABLE},
{"BPL2DAT",CST_WRITEABLE},
{"BPL3DAT",CST_WRITEABLE},
{"BPL4DAT",CST_WRITEABLE},
{"BPL5DAT",CST_WRITEABLE},
{"BPL6DAT",CST_WRITEABLE},
{"11C",0},
{"11E",0},
{"SPR0PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR0PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR1PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR1PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR2PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR2PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR3PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR3PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR4PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR4PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR5PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR5PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR6PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR6PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR7PTH",CST_WRITEABLE|CST_SUPPORTED},
{"SPR7PTL",CST_WRITEABLE|CST_SUPPORTED},
{"SPR0POS",CST_WRITEABLE},
{"SPR0CTL",CST_WRITEABLE},
{"SPR0DATA",CST_WRITEABLE},
{"SPR0DATB",CST_WRITEABLE},
{"SPR1POS",CST_WRITEABLE},
{"SPR1CTL",CST_WRITEABLE},
{"SPR1DATA",CST_WRITEABLE},
{"SPR1DATB",CST_WRITEABLE},
{"SPR2POS",CST_WRITEABLE},
{"SPR2CTL",CST_WRITEABLE},
{"SPR2DATA",CST_WRITEABLE},
{"SPR2DATB",CST_WRITEABLE},
{"SPR3POS",CST_WRITEABLE},
{"SPR3CTL",CST_WRITEABLE},
{"SPR3DATA",CST_WRITEABLE},
{"SPR3DATB",CST_WRITEABLE},
{"SPR4POS",CST_WRITEABLE},
{"SPR4CTL",CST_WRITEABLE},
{"SPR4DATA",CST_WRITEABLE},
{"SPR4DATB",CST_WRITEABLE},
{"SPR5POS",CST_WRITEABLE},
{"SPR5CTL",CST_WRITEABLE},
{"SPR5DATA",CST_WRITEABLE},
{"SPR5DATB",CST_WRITEABLE},
{"SPR6POS",CST_WRITEABLE},
{"SPR6CTL",CST_WRITEABLE},
{"SPR6DATA",CST_WRITEABLE},
{"SPR6DATB",CST_WRITEABLE},
{"SPR7POS",CST_WRITEABLE},
{"SPR7CTL",CST_WRITEABLE},
{"SPR7DATA",CST_WRITEABLE},
{"SPR7DATB",CST_WRITEABLE},
{"COLOR00",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR01",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR02",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR03",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR04",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR05",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR06",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR07",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR08",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR09",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR10",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR11",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR12",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR13",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR14",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR15",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR16",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR17",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR18",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR19",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR20",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR21",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR22",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR23",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR24",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR25",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR26",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR27",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR28",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR29",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR30",CST_WRITEABLE|CST_SUPPORTED},
{"COLOR31",CST_WRITEABLE|CST_SUPPORTED},
{"1C0(ECS)",0},
{"1C2(ECS)",0},
{"1C4(ECS)",0},
{"1C6(ECS)",0},
{"1C8(ECS)",0},
{"1CA(ECS)",0},
{"1CC(ECS)",0},
{"1CE(ECS)",0},
{"1D0",0},
{"1D2",0},
{"1D4",0},
{"1D6",0},
{"1D8",0},
{"1DA",0},
{"1DC(ECS)",0},
{"1DE(ECS)",0},
{"1E0(ECS)",0},
{"1E2(ECS)",0},
{"1E4(ECS)",0},
{"1E6",0},
{"1E8",0},
{"1EA",0},
{"1EC",0},
{"1EE",0},
{"1F0",0},
{"1F2",0},
{"1F4",0},
{"1F6",0},
{"1F8",0},
{"1FA",0},
{"1FC",0},
{"NO-OP",0},
{"",CST_END},
};

u_int8_t CST_getByteUnmapped(u_int16_t reg)
{
#if ENABLE_CHIP_WARNINGS
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
	
	printf("[WRN] Read from Unmapped Hardware Register %s %08x\n",name,reg);
#endif

	return 0;
}

u_int8_t CST_getByteCustom(u_int16_t reg)
{
#if ENABLE_CHIP_WARNINGS
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
	
	printf("[MSG] Read from Unsupported Hardware Register %s %08x\n",name,reg);
#endif
	return cstMemory[reg];
}

u_int8_t CST_getByteCustomSupported(u_int16_t reg)
{
	return cstMemory[reg];
}

void CST_setByteUnmapped(u_int16_t reg,u_int8_t byte)
{
#if ENABLE_CHIP_WARNINGS		
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;

	printf("[WRN] Write to Unmapped Hardware Register %s %02X -> %08x\n",name,byte,reg);
#endif
}

void CST_setByteCustom(u_int16_t reg,u_int8_t byte)
{
#if ENABLE_CHIP_WARNINGS		
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;

	printf("[MSG] Write to Unsupported Hardware Register %s %02X -> %08x\n",name,byte,reg);
#endif
	
	cstMemory[reg]=byte;
}

void CST_setByteCustomSupported(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
}

void CST_setByteStrobe(u_int16_t reg,u_int8_t byte)
{
#if ENABLE_CHIP_WARNINGS
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
	
	printf("[MSG] Strobe Unsupported Hardware Register %s %02X -> %08x\n",name,byte,reg);
#endif
}

void CST_setByteStrobeSupported(u_int16_t reg,u_int8_t byte)
{
}

void CST_InitialiseCustom()
{
	int a;
	
	cstMemory=(u_int8_t*)malloc(CUSTOMCHIPMEMORY);
	cLineLength=LINE_LENGTH-1;

	for (a=0;a<CUSTOMCHIPMEMORY;a++)
	{
		cst_read[a] = CST_getByteUnmapped;
		cst_write[a] = CST_setByteUnmapped;
		cstMemory[a]=0;
	}
	
	horizontalClock=0;
	verticalClock=0;
	leftMouseUp=1;
	rightMouseUp=1;
	
	a=0;
	while (!(customChipRegisters[a>>1].type & CST_END))
	{
		if (customChipRegisters[a>>1].type & CST_READABLE)
		{
			if (customChipRegisters[a>>1].type & CST_FUNCTION)
			{
				cst_read[a] = customChipRegisters[a>>1].readFunc;
			}
			else
			{
				if (customChipRegisters[a>>1].type & CST_SUPPORTED)
				{
					cst_read[a] = CST_getByteCustomSupported;
				}
				else
				{
					cst_read[a] = CST_getByteCustom;
				}
			}
		}
		if (customChipRegisters[a>>1].type & CST_WRITEABLE)
		{
			if (customChipRegisters[a>>1].type & CST_FUNCTION)
			{
				cst_write[a] = customChipRegisters[a>>1].writeFunc;
			}
			else
			{
				if (customChipRegisters[a>>1].type & CST_SUPPORTED)
				{
					cst_write[a] = CST_setByteCustomSupported;
				}
				else
				{
					cst_write[a] = CST_setByteCustom;
				}
			}
		}
		if (customChipRegisters[a>>1].type & CST_STROBEABLE)
		{
			if (customChipRegisters[a>>1].type & CST_FUNCTION)
			{
				cst_write[a] = customChipRegisters[a>>1].writeFunc;
			}
			else
			{
				if (customChipRegisters[a>>1].type & CST_SUPPORTED)
				{
					cst_write[a] = CST_setByteStrobeSupported;		// should never get here, as all strobe would need special handling
				}
				else
				{
					cst_write[a] = CST_setByteStrobe;
				}
			}
		}
		a++;
	}
}

u_int8_t MEM_getByteCustom(u_int32_t upper24,u_int32_t lower16)
{
	lower16&=0xFFF;
	return cst_read[lower16](lower16);
}

void MEM_setByteCustom(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	lower16&=0xFFF;
	cst_write[lower16](lower16,byte);
}

void MEM_GetHardwareDebug(u_int16_t regNum, char *buffer)
{
	u_int32_t a;
	static char spaces[8];

	strcpy(spaces," ");
	for (a=0;a<8-strlen(customChipRegisters[regNum].name);a++)
		strcat(spaces," ");
		
	sprintf(buffer,"%s%s%04X",customChipRegisters[regNum].name,spaces, CST_GETWRDU(regNum*2,0xFFFF));
}

const char *MEM_GetHardwareName(u_int16_t regNum)
{
	return customChipRegisters[regNum].name;
}
