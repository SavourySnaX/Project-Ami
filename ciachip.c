/*
 *  ciachip.c
 *  ami
 *
 *  Created by Lee Hammerton on 11/07/2009.
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
#include "ciachip.h"
#include "customchip.h"

typedef u_int8_t (*CIA_ReadMap)(u_int16_t reg);
typedef void (*CIA_WriteMap)(u_int16_t reg,u_int8_t byte);

typedef enum
	{
		CIA_READABLE=0x01,
		CIA_WRITEABLE=0x02,
		CIA_FUNCTION=0x08,			// indicates we have a custom handler
		CIA_END=0x8000,
	} CIA_TYPE;

typedef struct
	{
		char			name[32];
		CIA_TYPE		type;
		CIA_ReadMap		readFunc;
		CIA_WriteMap	writeFunc;
	} CIA_Regs;

#define CIACHIPMEMORY (0x10 * 2)		// 2 lots of 16 registers ciaa and  ciab

u_int8_t	*ciaMemory = 0;

CIA_ReadMap		cia_read[CIACHIPMEMORY];
CIA_WriteMap	cia_write[CIACHIPMEMORY];

u_int8_t		ciaa_icr=0;			// write copies of the register - since register is cleared on read
u_int8_t		ciab_icr=0;

u_int32_t		todAAlarm=0;
u_int32_t		todBAlarm=0;
u_int32_t		todACntLatch=0;
u_int32_t		todBCntLatch=0;
u_int32_t		todACntLatchR=0;
u_int32_t		todBCntLatchR=0;
u_int32_t		todAReadLatched=0;
u_int32_t		todBReadLatched=0;
u_int32_t		todAStart=1;
u_int32_t		todBStart=1;
u_int32_t		todACnt=0;
u_int32_t		todBCnt=0;

u_int16_t		aTBLatch=0xFFFF;
u_int16_t		aTBCnt=0;
u_int16_t		bTBLatch=0xFFFF;
u_int16_t		bTBCnt=0;

u_int16_t		timerSlow=0;

extern u_int8_t		*cstMemory;

void CIA_Update()
{
	timerSlow++;
	
	if (timerSlow>9)
	{
		timerSlow=0;
		if (ciaMemory[0x1F]&0x01)
		{
			bTBCnt--;
			if (bTBCnt==0xFFFF)
			{
				//Underflow occurred
				if (ciaMemory[0x1F]&0x08)
				{
					ciaMemory[0x1F]&=~0x01;		// stop timer
				}
				else
				{
					// continous mode
					bTBCnt=bTBLatch;
				}
				ciaMemory[0x1D]|=0x02;			// signal interrupt request (won't actually interrupt unless mask set however)
/*				if (ciab_icr&0x02)
				{
					ciaMemory[0x1D]|=0x80;		// set IR bit
					cstMemory[0x1F]|=0x08;		// set port interrupt
				}*/
			}
		}
		
		if (ciaMemory[0x0F]&0x01)
		{
//			printf("TBCnt %04X   TODCNT %08X\n",aTBCnt,todACnt);
			aTBCnt--;
			if (aTBCnt==0xFFFF)
			{
				//Underflow occurred
				if (ciaMemory[0x0F]&0x08)
				{
					ciaMemory[0x0F]&=~0x01;		// stop timer
				}
				else
				{
					// continous mode
					aTBCnt=aTBLatch;
				}
				ciaMemory[0x0D]|=0x02;			// signal interrupt request (won't actually interrupt unless mask set however)
				if (ciaa_icr&0x02)
				{
					ciaMemory[0x0D]|=0x80;		// set IR bit
					cstMemory[0x1F]|=0x08;		// set port interrupt
				}
			}
		}
	}
}

void CIA_setBytePRA(u_int16_t reg,u_int8_t byte)
{
	if (reg&0x10)
	{
		// DTR | RTS | CD | CTS | DSR | SEL | POUT | BUSY
		ciaMemory[reg]&=~ciaMemory[0x12];		// clear write bits
		ciaMemory[reg]|=(byte&ciaMemory[0x12]);
	}
	else
	{
		// FIR1 | FIR0 | RDY | TK0 | WPR0 | CHNG | LED | OVL
		ciaMemory[reg]&=~ciaMemory[0x02];		// clear write bits
		ciaMemory[reg]|=(byte&ciaMemory[0x02]);
	}
}

u_int8_t CIA_getBytePRA(u_int16_t reg)
{
	return ciaMemory[reg];
}

void CIA_setBytePRB(u_int16_t reg,u_int8_t byte)
{
	if (reg&0x10)
	{
		// MTR | SEL3 | SEL2 | SEL1 | SEL0 | SIDE | DIR | STEP
		ciaMemory[reg]&=~ciaMemory[0x13];		// clear write bits
		ciaMemory[reg]|=(byte&ciaMemory[0x13]);
	}
	else
	{
		// Parallel Port
		ciaMemory[reg]&=~ciaMemory[0x03];		// clear write bits
		ciaMemory[reg]|=(byte&ciaMemory[0x03]);
	}
}

u_int8_t CIA_getBytePRB(u_int16_t reg)
{
	return ciaMemory[reg];
}

void CIA_setByteDDRA(u_int16_t reg,u_int8_t byte)
{
	ciaMemory[reg]=byte;
}

u_int8_t CIA_getByteDDRA(u_int16_t reg)
{
	return ciaMemory[reg];
}

void CIA_setByteDDRB(u_int16_t reg,u_int8_t byte)
{
	ciaMemory[reg]=byte;
}

u_int8_t CIA_getByteDDRB(u_int16_t reg)
{
	return ciaMemory[reg];
}

void CIA_setByteICR(u_int16_t reg,u_int8_t byte)
{
	u_int8_t value;
	
	if (reg&0x10)
		value=ciab_icr;
	else
		value=ciaa_icr;

	// IR | 0 | 0 | FLG | SP | ALRM | TB | TA

	if (byte & 0x80)			// need to set mask bits
	{
		value|=byte&0x1F;
	}
	else
	{
		value&=~byte;
	}

/*	printf("CIAA TIMER interrupts : CIAA %02X    CIAB %02X\n",ciaa_icr,ciab_icr);

	printf("TOD ALARM : A %08X    B %08X\n",todAAlarm,todBAlarm);
*/
	if (reg&0x10)
		ciab_icr=value;
	else
		ciaa_icr=value;
}

u_int8_t CIA_getByteICR(u_int16_t reg)
{
	u_int8_t oldState = ciaMemory[reg];			// cleared on read

	// IR | 0 | 0 | FLG | SP | ALRM | TB | TA

	ciaMemory[reg]=0;
	return oldState;
}

void CIA_setByteCRA(u_int16_t reg,u_int8_t byte)
{
	// UNUSED | SPMODE | INMODE | LOAD | RUNMODE | OUTMODE | PBON | START
	
	if (byte&0x01)
		printf("Suspected Timer A Start %02X\n",byte);
	ciaMemory[reg]=byte&0x6F;
}

u_int8_t CIA_getByteCRA(u_int16_t reg)
{
	return ciaMemory[reg]&0x6F;		// load and unused set to zero
}

void CIA_setByteCRB(u_int16_t reg,u_int8_t byte)
{
	// ALARM | INMODE1 | INMODE0 | LOAD | RUNMODE | OUTMODE | PBON | START
	if (byte&0x01)
		printf("Suspected Timer B Start %02X\n",byte);
	
	if (byte&0x10)
	{
		if (reg&0x10)
		{
			bTBCnt=bTBLatch;
		}
		else
		{
			aTBCnt=aTBLatch;
		}
	}
	
	ciaMemory[reg]=byte&0xEF;
}

u_int8_t CIA_getByteCRB(u_int16_t reg)
{
	return ciaMemory[reg]&0xEF;		// load set to zero
}


void CIA_setByteTODLO(u_int16_t reg,u_int8_t byte)
{
	if (reg&0x10)
	{
		if (ciaMemory[0x1F]&0x80)
		{
			todBAlarm&=0xFFFF00;
			todBAlarm|=byte;
		}
		else
		{
			todBCntLatch&=0xFFFF00;
			todBCntLatch|=byte;
			todBCnt=todACntLatch;					// start timer from new set time
			todBStart=1;
		}
	}
	else
	{
		if (ciaMemory[0x0F]&0x80)
		{
			todAAlarm&=0xFFFF00;
			todAAlarm|=byte;
		}
		else
		{
			todACntLatch&=0xFFFF00;
			todACntLatch|=byte;
			todACnt=todACntLatch;					// start timer from new set time
			todAStart=1;
		}
	}
}

u_int8_t CIA_getByteTODLO(u_int16_t reg)
{
	if (reg&0x10)
	{
		printf("Reading TODB\n");
		if (todBReadLatched)
		{
			todBReadLatched=0;
			return todBCntLatchR&0xFF;
		}
		else
		{
			return todBCnt&0xFF;
		}
	}
	else
	{
		if (todAReadLatched)
		{
			todAReadLatched=0;
			return todACntLatchR&0xFF;
		}
		else
		{
			return todACnt&0xFF;
		}
	}
}

void CIA_setByteTODMED(u_int16_t reg,u_int8_t byte)
{
	u_int32_t rbyte = byte;
	
	if (reg&0x10)
	{
		if (ciaMemory[0x1F]&0x80)
		{
			todBAlarm&=0xFF00FF;
			todBAlarm|=rbyte<<8;
		}
		else
		{
			todBCntLatch&=0xFF00FF;
			todBCntLatch|=rbyte<<8;
			todBStart=0;
		}
	}
	else
	{
		if (ciaMemory[0x0F]&0x80)
		{
			todAAlarm&=0xFF00FF;
			todAAlarm|=rbyte<<8;
		}
		else
		{
			todACntLatch&=0xFF00FF;
			todACntLatch|=rbyte<<8;
			todAStart=0;
		}
	}
}

u_int8_t CIA_getByteTODMED(u_int16_t reg)
{
	if (reg&0x10)
	{
		printf("Reading TODB\n");
		if (todBReadLatched)
		{
			return (todBCntLatchR&0xFF00)>>8;
		}
		else
		{
			return (todBCnt&0xFF00)>>8;
		}
	}
	else
	{
		if (todAReadLatched)
		{
			return (todACntLatchR&0xFF00)>>8;
		}
		else
		{
			return (todACnt&0xFF00)>>8;
		}
	}
}

void CIA_setByteTODHI(u_int16_t reg,u_int8_t byte)
{
	u_int32_t rbyte = byte;

	if (reg&0x10)
	{
		if (ciaMemory[0x1F]&0x80)
		{
			todBAlarm&=0x00FFFF;
			todBAlarm|=rbyte<<16;
		}
		else
		{
			todBCntLatch&=0x00FFFF;
			todBCntLatch|=rbyte<<16;
			todBStart=0;
		}
	}
	else
	{
		if (ciaMemory[0x0F]&0x80)
		{
			todAAlarm&=0x00FFFF;
			todAAlarm|=rbyte<<16;
		}
		else
		{
			todACntLatch&=0x00FFFF;
			todACntLatch|=rbyte<<16;
			todAStart=0;
		}
	}
}

u_int8_t CIA_getByteTODHI(u_int16_t reg)
{
	if (reg&0x10)
	{
		printf("Reading TODB\n");
		todBReadLatched=1;
		todBCntLatchR=todBCnt;
		if (todBReadLatched)
		{
			return (todBCntLatchR&0xFF0000)>>16;
		}
		else
		{
			return (todBCnt&0xFF0000)>>16;
		}
	}
	else
	{
		todAReadLatched=1;
		todACntLatchR=todACnt;
		if (todAReadLatched)
		{
			return (todACntLatchR&0xFF0000)>>16;
		}
		else
		{
			return (todACnt&0xFF0000)>>16;
		}
	}
}

void CIA_setByteTBLO(u_int16_t reg,u_int8_t byte)
{
	u_int32_t rbyte = byte;

	if (reg&0x10)
	{
		bTBLatch&=0xFF00;
		bTBLatch|=rbyte;
	}
	else
	{
		aTBLatch&=0xFF00;
		aTBLatch|=rbyte;
	}
}

u_int8_t CIA_getByteTBLO(u_int16_t reg)
{
	if (reg&0x10)
	{
		return (bTBCnt&0x00FF);
	}
	else
	{
		return (aTBCnt&0x00FF);
	}
}

void CIA_setByteTBHI(u_int16_t reg,u_int8_t byte)
{
	u_int32_t rbyte = byte;

	if (reg&0x10)
	{
		bTBLatch&=0x00FF;
		bTBLatch|=rbyte<<8;
		if (!(ciaMemory[0x1F]&0x01))
		{
			bTBCnt=bTBLatch;
			if (ciaMemory[0x1F]&0x08)
			{
				ciaMemory[0x1F]|=0x01;
			}
		}
	}
	else
	{
		aTBLatch&=0x00FF;
		aTBLatch|=rbyte<<8;
		if (!(ciaMemory[0x0F]&0x01))
		{
			aTBCnt=aTBLatch;
			if (ciaMemory[0x0F]&0x08)
			{
				ciaMemory[0x0F]|=0x01;
			}
		}
	}
}

extern int startDebug;

u_int8_t CIA_getByteTBHI(u_int16_t reg)
{
	if (reg&0x10)
	{
		return (bTBCnt&0xFF00)>>8;
	}
	else
	{
//		startDebug=1;
		return (aTBCnt&0xFF00)>>8;
	}
}



CIA_Regs ciaChipRegisters[] =
{
{"A_PRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getBytePRA,CIA_setBytePRA},
{"A_PRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getBytePRB,CIA_setBytePRB},
{"A_DDRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteDDRA,CIA_setByteDDRA},
{"A_DDRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteDDRB,CIA_setByteDDRB},
{"A_TALO",CIA_READABLE|CIA_WRITEABLE},
{"A_TAHI",CIA_READABLE|CIA_WRITEABLE},
{"A_TBLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBLO,CIA_setByteTBLO},
{"A_TBHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBHI,CIA_setByteTBHI},
{"A_TODLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODLO,CIA_setByteTODLO},
{"A_TODMID",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODMED,CIA_setByteTODMED},
{"A_TODHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODHI,CIA_setByteTODHI},
{"A_unused",0},
{"A_SDR",CIA_READABLE|CIA_WRITEABLE},
{"A_ICR",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteICR,CIA_setByteICR},
{"A_CRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRA,CIA_setByteCRA},
{"A_CRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRB,CIA_setByteCRB},

{"B_PRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getBytePRA,CIA_setBytePRA},
{"B_PRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getBytePRB,CIA_setBytePRB},
{"B_DDRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteDDRA,CIA_setByteDDRA},
{"B_DDRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteDDRB,CIA_setByteDDRB},
{"B_TALO",CIA_READABLE|CIA_WRITEABLE},
{"B_TAHI",CIA_READABLE|CIA_WRITEABLE},
{"B_TBLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBLO,CIA_setByteTBLO},
{"B_TBHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBHI,CIA_setByteTBHI},
{"B_TODLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODLO,CIA_setByteTODLO},
{"B_TODMID",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODMED,CIA_setByteTODMED},
{"B_TODHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODHI,CIA_setByteTODHI},
{"B_unused",0},
{"B_SDR",CIA_READABLE|CIA_WRITEABLE},
{"B_ICR",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteICR,CIA_setByteICR},
{"B_CRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRA,CIA_setByteCRA},
{"B_CRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRB,CIA_setByteCRB},
{"",CIA_END},
};

u_int8_t CIA_getByteUnmapped(u_int16_t reg)
{
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
		
	printf("[WRN] Read from Unmapped Hardware Register %s %08x\n",name,reg);

	return 0;
}

u_int8_t CIA_getByteCustom(u_int16_t reg)
{
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
		
	printf("[MSG] Read from Unsupported Hardware Register %s %08x\n",name,reg);
	
	return ciaMemory[reg];
}

u_int8_t CIA_getByteCustomSupported(u_int16_t reg)
{
	return ciaMemory[reg];
}

void CIA_setByteUnmapped(u_int16_t reg,u_int8_t byte)
{
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
		
	printf("[WRN] Write to Unmapped Hardware Register %s %02X -> %08x\n",name,byte,reg);
}

void CIA_setByteCustom(u_int16_t reg,u_int8_t byte)
{
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
		
	printf("[MSG] Write to Unsupported Hardware Register %s %02X -> %08x\n",name,byte,reg);
	
	ciaMemory[reg]=byte;
}

void CIA_setByteCustomSupported(u_int16_t reg,u_int8_t byte)
{
	ciaMemory[reg]=byte;
}

void CIA_InitialiseCustom()
{
	int a;
	
	ciaMemory=(u_int8_t*)malloc(CIACHIPMEMORY);
	
	for (a=0;a<CIACHIPMEMORY;a++)
	{
		cia_read[a] = CIA_getByteUnmapped;
		cia_write[a] = CIA_setByteUnmapped;
		ciaMemory[a]=0;
	}
	
	a=0;
	while (!(ciaChipRegisters[a].type & CIA_END))
	{
		if (ciaChipRegisters[a].type & CIA_READABLE)
		{
			if (ciaChipRegisters[a].type & CIA_FUNCTION)
			{
				cia_read[a] = ciaChipRegisters[a].readFunc;
			}
			else
			{
				cia_read[a] = CIA_getByteCustom;
			}
		}
		if (ciaChipRegisters[a].type & CIA_WRITEABLE)
		{
			if (ciaChipRegisters[a].type & CIA_FUNCTION)
			{
				cia_write[a] = ciaChipRegisters[a].writeFunc;
			}
			else
			{
				cia_write[a] = CIA_setByteCustom;
			}
		}
		a++;
	}
}

u_int8_t MEM_getByteCia(u_int32_t upper24,u_int32_t lower16)
{
	if (lower16&0xF000 == 0xD)
		lower16 = ((lower16&0x0F00)>>8)+16;
	else
		lower16 = ((lower16&0x0F00)>>8);
	return cia_read[lower16](lower16);
}

void MEM_setByteCia(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	if (lower16&0xF000 == 0xD)
		lower16 = ((lower16&0x0F00)>>8)+16;
	else
		lower16 = ((lower16&0x0F00)>>8);
	cia_write[lower16](lower16,byte);
}
