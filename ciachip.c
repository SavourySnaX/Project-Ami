/*
 *  ciachip.c
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

#include "disk.h"
#include "ciachip.h"
#include "customchip.h"
#include "memory.h"
#include "keyboard.h"

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

CIA_ReadMap		cia_read[CIACHIPMEMORY];
CIA_WriteMap	cia_write[CIACHIPMEMORY];

u_int8_t	*ciaMemory = 0;

u_int8_t		ciaa_icr=0;			// write copies of the register - since register is cleared on read
u_int8_t		ciab_icr=0;

u_int32_t		todAAlarm=0;
u_int32_t		todBAlarm=0;
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

u_int16_t		aTALatch=0xFFFF;
u_int16_t		aTACnt=0;
u_int16_t		bTALatch=0xFFFF;
u_int16_t		bTACnt=0;

u_int16_t		timerSlow=0;

void CIA_SaveState(FILE *outStream)
{
	fwrite(ciaMemory,1,CIACHIPMEMORY,outStream);
	fwrite(&ciaa_icr,1,sizeof(u_int8_t),outStream);
	fwrite(&ciab_icr,1,sizeof(u_int8_t),outStream);
	fwrite(&todAAlarm,1,sizeof(u_int32_t),outStream);
	fwrite(&todBAlarm,1,sizeof(u_int32_t),outStream);
	fwrite(&todACntLatchR,1,sizeof(u_int32_t),outStream);
	fwrite(&todBCntLatchR,1,sizeof(u_int32_t),outStream);
	fwrite(&todAReadLatched,1,sizeof(u_int32_t),outStream);
	fwrite(&todBReadLatched,1,sizeof(u_int32_t),outStream);
	fwrite(&todAStart,1,sizeof(u_int32_t),outStream);
	fwrite(&todBStart,1,sizeof(u_int32_t),outStream);
	fwrite(&todACnt,1,sizeof(u_int32_t),outStream);
	fwrite(&todBCnt,1,sizeof(u_int32_t),outStream);
	fwrite(&aTBLatch,1,sizeof(u_int16_t),outStream);
	fwrite(&aTBCnt,1,sizeof(u_int16_t),outStream);
	fwrite(&bTBLatch,1,sizeof(u_int16_t),outStream);
	fwrite(&bTBCnt,1,sizeof(u_int16_t),outStream);
	fwrite(&aTALatch,1,sizeof(u_int16_t),outStream);
	fwrite(&aTACnt,1,sizeof(u_int16_t),outStream);
	fwrite(&bTALatch,1,sizeof(u_int16_t),outStream);
	fwrite(&bTACnt,1,sizeof(u_int16_t),outStream);
	fwrite(&timerSlow,1,sizeof(u_int16_t),outStream);
}

void CIA_LoadState(FILE *inStream)
{
	fread(ciaMemory,1,CIACHIPMEMORY,inStream);
	fread(&ciaa_icr,1,sizeof(u_int8_t),inStream);
	fread(&ciab_icr,1,sizeof(u_int8_t),inStream);
	fread(&todAAlarm,1,sizeof(u_int32_t),inStream);
	fread(&todBAlarm,1,sizeof(u_int32_t),inStream);
	fread(&todACntLatchR,1,sizeof(u_int32_t),inStream);
	fread(&todBCntLatchR,1,sizeof(u_int32_t),inStream);
	fread(&todAReadLatched,1,sizeof(u_int32_t),inStream);
	fread(&todBReadLatched,1,sizeof(u_int32_t),inStream);
	fread(&todAStart,1,sizeof(u_int32_t),inStream);
	fread(&todBStart,1,sizeof(u_int32_t),inStream);
	fread(&todACnt,1,sizeof(u_int32_t),inStream);
	fread(&todBCnt,1,sizeof(u_int32_t),inStream);
	fread(&aTBLatch,1,sizeof(u_int16_t),inStream);
	fread(&aTBCnt,1,sizeof(u_int16_t),inStream);
	fread(&bTBLatch,1,sizeof(u_int16_t),inStream);
	fread(&bTBCnt,1,sizeof(u_int16_t),inStream);
	fread(&aTALatch,1,sizeof(u_int16_t),inStream);
	fread(&aTACnt,1,sizeof(u_int16_t),inStream);
	fread(&bTALatch,1,sizeof(u_int16_t),inStream);
	fread(&bTACnt,1,sizeof(u_int16_t),inStream);
	fread(&timerSlow,1,sizeof(u_int16_t),inStream);
}

void CIA_Update()
{
	timerSlow++;
	
	if (timerSlow>9)
	{
		timerSlow=0;
		if (ciaMemory[0x1E]&0x01)
		{
			bTACnt--;
			if (bTACnt==0xFFFF)
			{
				//Underflow occurred
				if (ciaMemory[0x1E]&0x08)
				{
					ciaMemory[0x1E]&=~0x01;		// stop timer
				}
				
				bTACnt=bTALatch;				// timer allways re-latches

				ciaMemory[0x1D]|=0x01;			// signal interrupt request (won't actually interrupt unless mask set however)
				if (ciab_icr&0x01)
				{
					ciaMemory[0x1D]|=0x80;		// set IR bit
					CST_ORWRD(CST_INTREQR,0x2000);
				}
			}
		}
		
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
				
				bTBCnt=bTBLatch;				// timer allways re-latches

				ciaMemory[0x1D]|=0x02;			// signal interrupt request (won't actually interrupt unless mask set however)
				if (ciab_icr&0x02)
				{
					ciaMemory[0x1D]|=0x80;		// set IR bit
					CST_ORWRD(CST_INTREQR,0x2000);
				}
			}
		}
		
		if (ciaMemory[0x0E]&0x01)
		{
//			printf("TBCnt %04X   TODCNT %08X\n",aTBCnt,todACnt);
			aTACnt--;
			if (aTACnt==0xFFFF)
			{
				//Underflow occurred
				if (ciaMemory[0x0E]&0x08)
				{
					ciaMemory[0x0E]&=~0x01;		// stop timer
				}
				
				aTACnt=aTALatch;		// Timer allways re-latches

				ciaMemory[0x0D]|=0x01;			// signal interrupt request (won't actually interrupt unless mask set however)
				if (ciaa_icr&0x01)
				{
					ciaMemory[0x0D]|=0x80;		// set IR bit
					CST_ORWRD(CST_INTREQR,0x0008);
				}
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
				
				aTBCnt=aTBLatch;		// Timer allways re-latches

				ciaMemory[0x0D]|=0x02;			// signal interrupt request (won't actually interrupt unless mask set however)
				if (ciaa_icr&0x02)
				{
					ciaMemory[0x0D]|=0x80;		// set IR bit
					CST_ORWRD(CST_INTREQR,0x0008);
				}
			}
		}
	}
	
// Check tod alarms

	if (todBCnt==todBAlarm && todBStart)
	{
		ciaMemory[0x1D]|=0x04;			// signal interrupt request (won't actually interrupt unless mask set however)
		if (ciab_icr&0x04)
		{
			ciaMemory[0x1D]|=0x80;		// set IR bit
			CST_ORWRD(CST_INTREQR,0x2000);
		}
	}
	if (todACnt==todAAlarm && todAStart)
	{
		ciaMemory[0x0D]|=0x04;			// signal interrupt request (won't actually interrupt unless mask set however)
		if (ciaa_icr&0x04)
		{
			ciaMemory[0x0D]|=0x80;		// set IR bit
			CST_ORWRD(CST_INTREQR,0x0008);
		}
	}
	
}

extern int startDebug;

void CIA_setByteSDR(u_int16_t reg,u_int8_t byte)
{
	if (reg&0x10)
	{
		if (ciaMemory[0x1E]&0x40)
		{
			// Serial port set for output
			ciaMemory[reg]=byte;			// I`ll need to generate an interrupt to indicate data sent (based on timer - that can wait)
		}
	}
	else
	{
		if (ciaMemory[0x0E]&0x40)
		{
			// Serial port set for output
//			startDebug=1;
			ciaMemory[reg]=byte;			// I`ll need to generate an interrupt to indicate data sent (based on timer - that can wait)
		}
	}
}

u_int8_t CIA_getByteSDR(u_int16_t reg)
{
	if (reg&0x10)
	{
		return ciaMemory[reg];				// not attached to anything (could be a modem though for instance)
	}
	else
	{
//		startDebug=1;
		return ciaMemory[reg];				// attached to keyboard - see cia update for how this gets filled
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
		
		MEM_MapKickstartLow(ciaMemory[reg]&0x01);
	}
}

u_int8_t CIA_getBytePRA(u_int16_t reg)
{
	if (reg&0x10)
	{
		return ciaMemory[reg];		
	}
	else
	{
		u_int8_t byte=ciaMemory[reg]&0x03;		// FIR1 | FIR0 | ---- | LED | OVL

		byte|=0x80;				// second fire button released.	
		if (leftMouseUp)
			byte|=0x40;
		
		if (!DSK_Removed())
			byte|=0x04;
		if (DSK_Writeable())
			byte|=0x08;
		if (!DSK_OnTrack(0))
			byte|=0x10;
		if (!DSK_Ready())
			byte|=0x20;
			
		return byte;
	}
}

void CIA_setBytePRB(u_int16_t reg,u_int8_t byte)
{
	if (reg&0x10)
	{
		if ((byte&0x78) == 0x18)
		{
			printf("TWO DRIVES !\n");
		}
		if ((ciaMemory[reg]&0x40) && !(byte&0x40))				// I need to confirm what happens when multiple drive select bits are set (esp. disk dma!)
		{
			DSK_SelectDrive(3,ciaMemory[reg]&0x80);
		}
		if ((ciaMemory[reg]&0x20) && !(byte&0x20))
		{
			DSK_SelectDrive(2,ciaMemory[reg]&0x80);
		}
		if ((ciaMemory[reg]&0x10) && !(byte&0x10))
		{
			DSK_SelectDrive(1,ciaMemory[reg]&0x80);
		}
		if ((ciaMemory[reg]&0x08) && !(byte&0x08))
		{
			DSK_SelectDrive(0,ciaMemory[reg]&0x80);
		}
		
		if (!(ciaMemory[reg]&0x01) && (byte&0x01))
		{
			// half a step pulse (not 100% correct, but will do for testing
			DSK_Step();
		}
	
		// MTR | SEL3 | SEL2 | SEL1 | SEL0 | SIDE | DIR | STEP
		ciaMemory[reg]&=~ciaMemory[0x13];		// clear write bits
		ciaMemory[reg]|=(byte&ciaMemory[0x13]);
		
//		if ((ciaMemory[reg]&0x08)==0x00)	// drive active low
//		{
			// Drive selected.
			DSK_SetSide(ciaMemory[reg]&0x04);
			DSK_SetDir(ciaMemory[reg]&0x02);
//		}
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

	if (byte&0x10)
	{
		if (reg&0x10)
		{
			bTACnt=bTALatch;
		}
		else
		{
			aTACnt=aTALatch;
		}
	}

	if ((!(reg&0x10)) && (!(byte&0x40)) && (ciaMemory[reg]&0x40))
	{
		// CIAA and serial port buffer set to output - Most likely this is the keyboard ACK
		KBD_Acknowledge();
	}
		
	
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

			printf("Alarm being set : %08X(%08X)\n",todBAlarm,todBCnt);
		}
		else
		{
			todBCnt&=0xFFFF00;
			todBCnt|=byte;
			todBStart=1;
		}
	}
	else
	{
		if (ciaMemory[0x0F]&0x80)
		{
			todAAlarm&=0xFFFF00;
			todAAlarm|=byte;

			printf("Alarm being set : %08X(%08X)\n",todAAlarm,todACnt);
		}
		else
		{
			todACnt&=0xFFFF00;
			todACnt|=byte;
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
			
			printf("Alarm being set : %08X(%08X)\n",todBAlarm,todBCnt);
		}
		else
		{
			todBCnt&=0xFF00FF;
			todBCnt|=rbyte<<8;
			todBStart=0;
		}
	}
	else
	{
		if (ciaMemory[0x0F]&0x80)
		{
			todAAlarm&=0xFF00FF;
			todAAlarm|=rbyte<<8;

			printf("Alarm being set : %08X(%08X)\n",todAAlarm,todACnt);
		}
		else
		{
			todACnt&=0xFF00FF;
			todACnt|=rbyte<<8;
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

			printf("Alarm being set : %08X(%08X)\n",todBAlarm,todBCnt);
		}
		else
		{
			todBCnt&=0x00FFFF;
			todBCnt|=rbyte<<16;
			todBStart=0;
		}
	}
	else
	{
		if (ciaMemory[0x0F]&0x80)
		{
			todAAlarm&=0x00FFFF;
			todAAlarm|=rbyte<<16;

			printf("Alarm being set : %08X(%08X)\n",todAAlarm,todACnt);
		}
		else
		{
			todACnt&=0x00FFFF;
			todACnt|=rbyte<<16;
			todAStart=0;
		}
	}
}

u_int8_t CIA_getByteTODHI(u_int16_t reg)
{
	if (reg&0x10)
	{
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

void CIA_setByteTALO(u_int16_t reg,u_int8_t byte)
{
	u_int32_t rbyte = byte;

	if (reg&0x10)
	{
		bTALatch&=0xFF00;
		bTALatch|=rbyte;
	}
	else
	{
		aTALatch&=0xFF00;
		aTALatch|=rbyte;
	}
}

u_int8_t CIA_getByteTALO(u_int16_t reg)
{
	if (reg&0x10)
	{
		return (bTACnt&0x00FF);
	}
	else
	{
		return (aTACnt&0x00FF);
	}
}

void CIA_setByteTAHI(u_int16_t reg,u_int8_t byte)
{
	u_int32_t rbyte = byte;

	if (reg&0x10)
	{
		bTALatch&=0x00FF;
		bTALatch|=rbyte<<8;
		if (!(ciaMemory[0x1E]&0x01))
		{
			bTACnt=bTALatch;
			if (ciaMemory[0x1E]&0x08)
			{
				ciaMemory[0x1E]|=0x01;
			}
		}
	}
	else
	{
		aTALatch&=0x00FF;
		aTALatch|=rbyte<<8;
		if (!(ciaMemory[0x0E]&0x01))
		{
			aTACnt=aTALatch;
			if (ciaMemory[0x0E]&0x08)
			{
				ciaMemory[0x0E]|=0x01;
			}
		}
	}
}

u_int8_t CIA_getByteTAHI(u_int16_t reg)
{
	if (reg&0x10)
	{
		return (bTACnt&0xFF00)>>8;
	}
	else
	{
		return (aTACnt&0xFF00)>>8;
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
{"A_TALO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTALO,CIA_setByteTALO},
{"A_TAHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTAHI,CIA_setByteTAHI},
{"A_TBLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBLO,CIA_setByteTBLO},
{"A_TBHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBHI,CIA_setByteTBHI},
{"A_TODLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODLO,CIA_setByteTODLO},
{"A_TODMID",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODMED,CIA_setByteTODMED},
{"A_TODHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODHI,CIA_setByteTODHI},
{"A_unused",0},
{"A_SDR",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteSDR,CIA_setByteSDR},
{"A_ICR",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteICR,CIA_setByteICR},
{"A_CRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRA,CIA_setByteCRA},
{"A_CRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRB,CIA_setByteCRB},

{"B_PRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getBytePRA,CIA_setBytePRA},
{"B_PRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getBytePRB,CIA_setBytePRB},
{"B_DDRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteDDRA,CIA_setByteDDRA},
{"B_DDRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteDDRB,CIA_setByteDDRB},
{"B_TALO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTALO,CIA_setByteTALO},
{"B_TAHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTAHI,CIA_setByteTAHI},
{"B_TBLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBLO,CIA_setByteTBLO},
{"B_TBHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTBHI,CIA_setByteTBHI},
{"B_TODLO",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODLO,CIA_setByteTODLO},
{"B_TODMID",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODMED,CIA_setByteTODMED},
{"B_TODHI",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteTODHI,CIA_setByteTODHI},
{"B_unused",0},
{"B_SDR",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteSDR,CIA_setByteSDR},
{"B_ICR",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteICR,CIA_setByteICR},
{"B_CRA",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRA,CIA_setByteCRA},
{"B_CRB",CIA_READABLE|CIA_WRITEABLE|CIA_FUNCTION,CIA_getByteCRB,CIA_setByteCRB},
{"",CIA_END},
};

u_int8_t CIA_getByteUnmapped(u_int16_t reg)
{
#if ENABLE_CIA_WARNINGS
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
	
	printf("[WRN] Read from Unmapped Hardware Register %s %08x\n",name,reg);
#endif
	return 0;
}

u_int8_t CIA_getByteCustom(u_int16_t reg)
{
#if ENABLE_CIA_WARNINGS
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
	
	printf("[MSG] Read from Unsupported Hardware Register %s %08x\n",name,reg);
#endif
	return ciaMemory[reg];
}

u_int8_t CIA_getByteCustomSupported(u_int16_t reg)
{
	return ciaMemory[reg];
}

void CIA_setByteUnmapped(u_int16_t reg,u_int8_t byte)
{
#if ENABLE_CIA_WARNINGS
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
		
	printf("[WRN] Write to Unmapped Hardware Register %s %02X -> %08x\n",name,byte,reg);
#endif
}

void CIA_setByteCustom(u_int16_t reg,u_int8_t byte)
{
#if ENABLE_CIA_WARNINGS
	const char *name="";
	
	if (reg < sizeof(ciaChipRegisters))
		name = ciaChipRegisters[reg].name;
	
	printf("[MSG] Write to Unsupported Hardware Register %s %02X -> %08x\n",name,byte,reg);
#endif
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
	if ((lower16&0xF000) == 0xD000)
		lower16 = ((lower16&0x0F00)>>8)+16;
	else
		lower16 = ((lower16&0x0F00)>>8);
	return cia_read[lower16](lower16);
}

void MEM_setByteCia(u_int32_t upper24,u_int32_t lower16,u_int8_t byte)
{
	if ((lower16&0xF000) == 0xD000)
		lower16 = ((lower16&0x0F00)>>8)+16;
	else
		lower16 = ((lower16&0x0F00)>>8);
	cia_write[lower16](lower16,byte);
}
