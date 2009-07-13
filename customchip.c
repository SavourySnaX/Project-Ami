/*
 *  customchip.c
 *  ami
 *
 *  Created by Lee Hammerton on 11/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>

#include "customchip.h"
#include "ciachip.h"

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

void CST_Update()
{
	horizontalClock++;
	if (horizontalClock>160)
	{
		horizontalClock=0;
		if (todBStart)
			todBCnt++;
		verticalClock++;
		if (verticalClock>262)
		{
			verticalClock=0;
			if (todAStart)
				todACnt++;
		}
	}
}

void CST_setByteINTENA(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
	
	if (reg&1)
	{
		// second byte set do the operation
		if (cstMemory[0x9A] & 0x80)		// need to or in the set bits
		{
			cstMemory[0x1C] |= cstMemory[0x9A]&0x7F;
			cstMemory[0x1D] |= cstMemory[0x9B];
		}
		else							// need to mask of the set bits
		{
			cstMemory[0x1C] &= ~(cstMemory[0x9A]);
			cstMemory[0x1D] &= ~(cstMemory[0x9B]);
		}
	}
}

#define AGNUS_ID		0					// NTSC agnus or fat agnus

u_int8_t CST_getByteVPOSR(u_int16_t reg)
{
	if (reg&1)
	{
		cstMemory[0x05]=(verticalClock>>8)&1;				// bit 1 is high part of vertical line (ie > 255)
	}
	else
	{
		cstMemory[0x04]=AGNUS_ID | 0;	// 0 is LOF long frame but i think thats an interlaced thing
	}
	return cstMemory[reg];
}

u_int8_t CST_getByteVHPOSR(u_int16_t reg)
{
	if (reg&1)
	{
		cstMemory[0x07]=horizontalClock;				// bit 1 is high part of vertical line (ie > 255)
	}
	else
	{
		cstMemory[0x06]=verticalClock&0xFF;	// 0 is LOF long frame but i think thats an interlaced thing
	}
	return cstMemory[reg];
}

CST_Regs customChipRegisters[] =
{
{"BLTDDAT",CST_READABLE},
{"DMACONR",CST_READABLE},
{"VPOSR",CST_READABLE|CST_SUPPORTED|CST_FUNCTION,CST_getByteVPOSR,0},
{"VHPOSR",CST_READABLE|CST_SUPPORTED|CST_FUNCTION,CST_getByteVHPOSR,0},
{"DSKDATR",CST_READABLE},
{"JOY0DAT",CST_READABLE},
{"JOY1DAT",CST_READABLE},
{"CLXDAT",CST_READABLE},
{"ADKCONR",CST_READABLE},
{"POT0DAT",CST_READABLE},
{"POT1DAT",CST_READABLE},
{"POTGOR",CST_READABLE},
{"SERDATR",CST_READABLE},
{"DSKBYTR",CST_READABLE},
{"INTENAR",CST_READABLE|CST_SUPPORTED},
{"INTREQR",CST_READABLE},
{"DSKPTH",CST_WRITEABLE},
{"DSKPTL",CST_WRITEABLE},
{"DSKLEN",CST_WRITEABLE},
{"DSKDAT",CST_WRITEABLE},
{"REFPT",CST_WRITEABLE},
{"VPOSW",CST_WRITEABLE},
{"VHPOSW",CST_WRITEABLE},
{"COPCON",CST_WRITEABLE},
{"SERDAT",CST_WRITEABLE},
{"SERPER",CST_WRITEABLE},
{"POTGO",CST_WRITEABLE},
{"JOYTEST",CST_WRITEABLE},
{"STREQU",CST_STROBEABLE},
{"STRVBL",CST_STROBEABLE},
{"STRHOR",CST_STROBEABLE},
{"STRLONG",CST_STROBEABLE},
{"BLTCON0",CST_WRITEABLE},
{"BLTCON1",CST_WRITEABLE},
{"BLTAFWM",CST_WRITEABLE},
{"BLTALWM",CST_WRITEABLE},
{"BLTCPTH",CST_WRITEABLE},
{"BLTCPTL",CST_WRITEABLE},
{"BLTBPTH",CST_WRITEABLE},
{"BLTBPTL",CST_WRITEABLE},
{"BLTAPTH",CST_WRITEABLE},
{"BLTAPTL",CST_WRITEABLE},
{"BLTDPTH",CST_WRITEABLE},
{"BLTDPTL",CST_WRITEABLE},
{"BLTSIZE",CST_WRITEABLE},
{"05A(ECS)",0},
{"05C(ECS)",0},
{"05E(ECS)",0},
{"BLTCMOD",CST_WRITEABLE},
{"BLTBMOD",CST_WRITEABLE},
{"BLTAMOD",CST_WRITEABLE},
{"BLTDMOD",CST_WRITEABLE},
{"068",0},
{"06A",0},
{"06C",0},
{"06E",0},
{"BLTCDAT",CST_WRITEABLE},
{"BLTBDAT",CST_WRITEABLE},
{"BLTADAT",CST_WRITEABLE},
{"076",0},
{"SPRHDAT",CST_WRITEABLE},
{"07A",0},
{"07C(ECS)",0},
{"DSKSYNC",CST_WRITEABLE},
{"COP1LCH",CST_WRITEABLE},
{"COP1LCL",CST_WRITEABLE},
{"COP2LCH",CST_WRITEABLE},
{"COP2LCL",CST_WRITEABLE},
{"COPJMP1",CST_STROBEABLE},
{"COPJMP2",CST_STROBEABLE},
{"COPINS",CST_WRITEABLE},
{"DIWSTRT",CST_WRITEABLE},
{"DIWSTOP",CST_WRITEABLE},
{"DDFSTRT",CST_WRITEABLE},
{"DDFSTOP",CST_WRITEABLE},
{"DMACON",CST_WRITEABLE},
{"CLXCON",CST_WRITEABLE},
{"INTENA",CST_WRITEABLE|CST_SUPPORTED|CST_FUNCTION,0,CST_setByteINTENA},
{"INTREQ",CST_WRITEABLE},
{"ADKCON",CST_WRITEABLE},
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
{"BPL1PTH",CST_WRITEABLE},
{"BPL1PTL",CST_WRITEABLE},
{"BPL2PTH",CST_WRITEABLE},
{"BPL2PTL",CST_WRITEABLE},
{"BPL3PTH",CST_WRITEABLE},
{"BPL3PTL",CST_WRITEABLE},
{"BPL4PTH",CST_WRITEABLE},
{"BPL4PTL",CST_WRITEABLE},
{"BPL5PTH",CST_WRITEABLE},
{"BPL5PTL",CST_WRITEABLE},
{"BPL6PTH",CST_WRITEABLE},
{"BPL6PTL",CST_WRITEABLE},
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
{"SPR0PTH",CST_WRITEABLE},
{"SPR0PTL",CST_WRITEABLE},
{"SPR1PTH",CST_WRITEABLE},
{"SPR1PTL",CST_WRITEABLE},
{"SPR2PTH",CST_WRITEABLE},
{"SPR2PTL",CST_WRITEABLE},
{"SPR3PTH",CST_WRITEABLE},
{"SPR3PTL",CST_WRITEABLE},
{"SPR4PTH",CST_WRITEABLE},
{"SPR4PTL",CST_WRITEABLE},
{"SPR5PTH",CST_WRITEABLE},
{"SPR5PTL",CST_WRITEABLE},
{"SPR6PTH",CST_WRITEABLE},
{"SPR6PTL",CST_WRITEABLE},
{"SPR7PTH",CST_WRITEABLE},
{"SPR7PTL",CST_WRITEABLE},
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
{"COLOR00",CST_WRITEABLE},
{"COLOR01",CST_WRITEABLE},
{"COLOR02",CST_WRITEABLE},
{"COLOR03",CST_WRITEABLE},
{"COLOR04",CST_WRITEABLE},
{"COLOR05",CST_WRITEABLE},
{"COLOR06",CST_WRITEABLE},
{"COLOR07",CST_WRITEABLE},
{"COLOR08",CST_WRITEABLE},
{"COLOR09",CST_WRITEABLE},
{"COLOR10",CST_WRITEABLE},
{"COLOR11",CST_WRITEABLE},
{"COLOR12",CST_WRITEABLE},
{"COLOR13",CST_WRITEABLE},
{"COLOR14",CST_WRITEABLE},
{"COLOR15",CST_WRITEABLE},
{"COLOR16",CST_WRITEABLE},
{"COLOR17",CST_WRITEABLE},
{"COLOR18",CST_WRITEABLE},
{"COLOR19",CST_WRITEABLE},
{"COLOR20",CST_WRITEABLE},
{"COLOR21",CST_WRITEABLE},
{"COLOR22",CST_WRITEABLE},
{"COLOR23",CST_WRITEABLE},
{"COLOR24",CST_WRITEABLE},
{"COLOR25",CST_WRITEABLE},
{"COLOR26",CST_WRITEABLE},
{"COLOR27",CST_WRITEABLE},
{"COLOR28",CST_WRITEABLE},
{"COLOR29",CST_WRITEABLE},
{"COLOR30",CST_WRITEABLE},
{"COLOR31",CST_WRITEABLE},
{"",CST_END},
};

u_int8_t CST_getByteUnmapped(u_int16_t reg)
{
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
		
	printf("[WRN] Read from Unmapped Hardware Register %s %08x\n",name,reg);

	return 0;
}

u_int8_t CST_getByteCustom(u_int16_t reg)
{
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
		
	printf("[MSG] Read from Unsupported Hardware Register %s %08x\n",name,reg);
	
	return cstMemory[reg];
}

u_int8_t CST_getByteCustomSupported(u_int16_t reg)
{
	return cstMemory[reg];
}

void CST_setByteUnmapped(u_int16_t reg,u_int8_t byte)
{
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
		
	printf("[WRN] Write to Unmapped Hardware Register %s %02X -> %08x\n",name,byte,reg);
}

void CST_setByteCustom(u_int16_t reg,u_int8_t byte)
{
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
		
	printf("[MSG] Write to Unsupported Hardware Register %s %02X -> %08x\n",name,byte,reg);
	
	cstMemory[reg]=byte;
}

void CST_setByteCustomSupported(u_int16_t reg,u_int8_t byte)
{
	cstMemory[reg]=byte;
}

void CST_setByteStrobe(u_int16_t reg,u_int8_t byte)
{
	const char *name="";
	
	if ((reg>>1) < sizeof(customChipRegisters))
		name = customChipRegisters[(reg>>1)].name;
		
	printf("[MSG] Strobe Unsupported Hardware Register %s %02X -> %08x\n",name,byte,reg);
}

void CST_setByteStrobeSupported(u_int16_t reg,u_int8_t byte)
{
}

void CST_InitialiseCustom()
{
	int a;
	
	cstMemory=(u_int8_t*)malloc(CUSTOMCHIPMEMORY);
	
	for (a=0;a<CUSTOMCHIPMEMORY;a++)
	{
		cst_read[a] = CST_getByteUnmapped;
		cst_write[a] = CST_setByteUnmapped;
		cstMemory[a]=0;
	}
	
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