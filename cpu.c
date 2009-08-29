/*
 *  cpu.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "ciachip.h"
#include "cpu.h"
#include "memory.h"
#include "customchip.h"

CPU_Regs cpu_regs;

u_int8_t	cpuUsedTable[65536];


void CPU_Reset()
{
	int a;
	
	for (a=0;a<8;a++)
	{
		cpu_regs.D[a]=0;
		cpu_regs.A[a]=0;
	}
	cpu_regs.SR = CPU_STATUS_S | CPU_STATUS_I2 | CPU_STATUS_I1 | CPU_STATUS_I0;
	
	cpu_regs.A[7] = MEM_getLong(/*0xFC0000*/0);
	cpu_regs.PC = MEM_getLong(/*0xFC0004*/4);

	if (MEM_getLong(0xFC0000)!=cpu_regs.A[7])
		printf("%08x : %08x\n",MEM_getLong(0xFC0000),cpu_regs.A[7]);
	if (MEM_getLong(0xFC0004)!=cpu_regs.PC)
		printf("%08x : %08x\n",MEM_getLong(0xFC0004),cpu_regs.PC);

	for (a=0;a<65536;a++)
		cpuUsedTable[a]=0;
}

const char *decodeWord(u_int32_t address)
{
	static char buffer[256];
	
	sprintf(buffer,"%04X",MEM_getWord(address));
	
	return buffer;
}

const char *decodeLong(u_int32_t address)
{
	static char buffer[256];
	
	sprintf(buffer,"%04X %04X",MEM_getWord(address),MEM_getWord(address+2));
	
	return buffer;
}

static char mnemonicData[256];
static char byteData[256];
static char tempData[256];

int startDebug=0;
int cpuStopped=0;

void CPU_CHECK_SP(u_int16_t old,u_int16_t new)
{
	if (old & CPU_STATUS_S)
	{
		if (!(new & CPU_STATUS_S))
		{
			cpu_regs.ISP = cpu_regs.A[7];
			cpu_regs.A[7] = cpu_regs.USP;
		}
	}
	else
	{
		if (new & CPU_STATUS_S)
		{
			cpu_regs.USP = cpu_regs.A[7];
			cpu_regs.A[7]=cpu_regs.ISP;
		}
	}
}

void CPU_GENERATE_EXCEPTION(u_int32_t exceptionAddress)
{
	u_int16_t oldSR;
	
	oldSR=cpu_regs.SR;
	cpu_regs.SR|=CPU_STATUS_S;
	CPU_CHECK_SP(oldSR,cpu_regs.SR);
		
	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],cpu_regs.PC);
	cpu_regs.A[7]-=2;
	MEM_setWord(cpu_regs.A[7],oldSR);

	cpu_regs.PC=MEM_getLong(exceptionAddress);
}

////////////////////////////////////////////////////////////////////////

#include "cpu_src/cpuOps.c"
#include "cpu_src/cpuDis.c"
#include "cpu_src/cpuTable.c"

////////////////////////////////////////////////////////////////////////

void CPU_DIS_UNKNOWN(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	printf("UNKNOWN INSTRUCTION\n");
}

void CPU_UNKNOWN(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	printf("ILLEGAL INSTRUCTION %08x\n",cpu_regs.PC);
	startDebug=1;
}

void DumpEmulatorState()
{
    printf("\n");
    printf("D0=%08X\tD1=%08X\tD2=%08x\tD3=%08x\n",cpu_regs.D[0],cpu_regs.D[1],cpu_regs.D[2],cpu_regs.D[3]);
    printf("D4=%08X\tD5=%08X\tD6=%08x\tD7=%08x\n",cpu_regs.D[4],cpu_regs.D[5],cpu_regs.D[6],cpu_regs.D[7]);
    printf("A0=%08X\tA1=%08X\tA2=%08x\tA3=%08x\n",cpu_regs.A[0],cpu_regs.A[1],cpu_regs.A[2],cpu_regs.A[3]);
    printf("A4=%08X\tA5=%08X\tA6=%08x\tA7=%08x\n",cpu_regs.A[4],cpu_regs.A[5],cpu_regs.A[6],cpu_regs.A[7]);
    printf("USP=%08X,ISP=%08x\n",cpu_regs.USP,cpu_regs.ISP);
    printf("\n");
    printf("          [ T1:T0: S: M:  :I2:I1:I0:  :  :  : X: N: Z: V: C ]\n");
    printf("SR = %04X [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]\n", cpu_regs.SR, 
		   cpu_regs.SR & 0x8000 ? " 1" : " 0",
		   cpu_regs.SR & 0x4000 ? " 1" : " 0",
		   cpu_regs.SR & 0x2000 ? " 1" : " 0",
		   cpu_regs.SR & 0x1000 ? " 1" : " 0",
		   cpu_regs.SR & 0x0800 ? " 1" : " 0",
		   cpu_regs.SR & 0x0400 ? " 1" : " 0",
		   cpu_regs.SR & 0x0200 ? " 1" : " 0",
		   cpu_regs.SR & 0x0100 ? " 1" : " 0",
		   cpu_regs.SR & 0x0080 ? " 1" : " 0",
		   cpu_regs.SR & 0x0040 ? " 1" : " 0",
		   cpu_regs.SR & 0x0020 ? " 1" : " 0",
		   cpu_regs.SR & 0x0010 ? " 1" : " 0",
		   cpu_regs.SR & 0x0008 ? " 1" : " 0",
		   cpu_regs.SR & 0x0004 ? " 1" : " 0",
		   cpu_regs.SR & 0x0002 ? " 1" : " 0",
		   cpu_regs.SR & 0x0001 ? " 1" : " 0");
    printf("\n");
}

void CPU_CheckForInterrupt()
{
	if (CST_GETWRDU(CST_INTENAR,0x4000))
	{
		// Interrupts are enabled, check for any pending interrupts in priority order, paying attention to 
		// interrupt level mask in SR
		//
/*	if ((CST_GETWRDU(CST_INTREQR,0x0008)&CST_GETWRDU(CST_INTENAR,0x0008))==0)
	{
		if ((ciaMemory[0x0D]&0x88)==0x88)
			printf("Should be responding to interrupt - but blocked by disabled interrupt!\n");
	}
		if ((ciaMemory[0x0D]&0x88)==0x88)
			printf("Should be responding to interrupt!\n");*/
	
		// Level 7
		// No interrupts - could be generated by external switch though
		//
		if ((cpu_regs.SR & 0x0700) >= 0x0600)
			return;

		// Level 6
		// CIAB Interrupts
		//
		if (CST_GETWRDU(CST_INTREQR,0x2000)&CST_GETWRDU(CST_INTENAR,0x2000))
		{
			CPU_GENERATE_EXCEPTION(0x78);
			cpu_regs.SR&=0xF8FF;
			cpu_regs.SR|=0x0600;
			cpuStopped=0;
			return;
		}

		if ((cpu_regs.SR & 0x0700) >= 0x0500)
			return;
		// Level 5
		// Disk Sync
		// RBF serial port recieve buffer full
		//
		if (CST_GETWRDU(CST_INTREQR,0x1800)&CST_GETWRDU(CST_INTENAR,0x1800))
		{
			CPU_GENERATE_EXCEPTION(0x74);
			cpu_regs.SR&=0xF8FF;
			cpu_regs.SR|=0x0500;
			cpuStopped=0;
			return;
		}

		if ((cpu_regs.SR & 0x0700) >= 0x0400)
			return;
		// Level 4
		// AUD3
		// AUD2
		// AUD1
		// AUD0
		//
		if (CST_GETWRDU(CST_INTREQR,0x0780)&CST_GETWRDU(CST_INTENAR,0x0780))
		{
			CPU_GENERATE_EXCEPTION(0x70);
			cpu_regs.SR&=0xF8FF;
			cpu_regs.SR|=0x0400;
			cpuStopped=0;
			return;
		}

		if ((cpu_regs.SR & 0x0700) >= 0x0300)
			return;
		// Level 3
		// BLIT
		// Start of VBL
		// Copper
		//
		if (CST_GETWRDU(CST_INTREQR,0x0070)&CST_GETWRDU(CST_INTENAR,0x0070))
		{
			CPU_GENERATE_EXCEPTION(0x6C);
			cpu_regs.SR&=0xF8FF;
			cpu_regs.SR|=0x0300;
			cpuStopped=0;
			return;
		}

		if ((cpu_regs.SR & 0x0700) >= 0x0200)
			return;
		// Level 2
		// CIAA Interrupts
		//
		if (CST_GETWRDU(CST_INTREQR,0x0008)&CST_GETWRDU(CST_INTENAR,0x0008))
		{
			if (ciaMemory[0x0D]&0x08)
				printf("Responding to interrupt\n");

			CPU_GENERATE_EXCEPTION(0x68);
			cpu_regs.SR&=0xF8FF;
			cpu_regs.SR|=0x0200;
			cpuStopped=0;
			return;
		}

		if ((cpu_regs.SR & 0x0700) >= 0x0100)
			return;
		// Level 1
		// SOFT
		// DSKBLK
		// TBE
		//
		if (CST_GETWRDU(CST_INTREQR,0x0007)&CST_GETWRDU(CST_INTENAR,0x0007))
		{
//			if (CST_GETWRDU(CST_INTREQR,0x0002))
//				startDebug=1;

			CPU_GENERATE_EXCEPTION(0x64);
			cpu_regs.SR&=0xF8FF;
			cpu_regs.SR|=0x0100;
			cpuStopped=0;
//			startDebug=1;
			return;
		}
	}
}

u_int32_t lastPC;
int readyToTrap=0;	

#define PCCACHESIZE	1000

u_int32_t	pcCache[PCCACHESIZE];
u_int32_t	cachePos=0;

extern int doNewOpcodeDebug;

u_int32_t	bpAddress=0x5c5e;

void CPU_Step()
{
    u_int16_t operands[8];
    u_int16_t opcode;
    int a;

    CPU_CheckForInterrupt();

	lastPC=cpu_regs.PC;
	
	if (cachePos<PCCACHESIZE)
	{
		pcCache[cachePos++]=lastPC;
	}
	else
	{
/*		for (a=0;a<PCCACHESIZE;a++)
		{
			printf("PC History : %08X\n",pcCache[a]);
		}
*/	
		memmove(pcCache,pcCache+1,(PCCACHESIZE-1)*sizeof(u_int32_t));
/*
		for (a=0;a<PCCACHESIZE;a++)
		{
			printf("PC History : %08X\n",pcCache[a]);
		}
*/
		pcCache[PCCACHESIZE-1]=lastPC;
	}
	
    opcode = MEM_getWord(cpu_regs.PC);
    
    if (CPU_Information[opcode])
    {
		for (a=0;a<CPU_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & CPU_Information[opcode]->operandMask[a]) >> CPU_Information[opcode]->operandShift[a];
//						printf("Operand %d = %08x\n", a,operands[a]);	
		}
    }
	
    // DEBUGGER

	if (cpu_regs.PC == bpAddress /*0x7013a*/ /*cpu_regs.PC > 0x1000000 && cpu_regs.PC < 0x2000000*//*== 0x107e068*/)			//FE961E  NOP
	{
/*		static once=0;
		if (once==0)
			startDebug=1;
		else
			once++;*/
	}

	if ((!cpuUsedTable[opcode]) && doNewOpcodeDebug)
		startDebug=1;
		
	if (startDebug)
	{
/*		for (a=0;a<PCCACHESIZE;a++)
		{
			printf("PC History : %08X\n",pcCache[a]);
		}*/

		DumpEmulatorState();
		
		printf("%08x\t%s ",cpu_regs.PC,decodeWord(cpu_regs.PC));
		
		CPU_DisTable[opcode](cpu_regs.PC,operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);
				
		startDebug=1;	// just so i can break after disassembly.
	}
	
    // END DEBUGGER
	
	if (!cpuStopped)			// Don't process instruction cpu halted waiting for interrupt
	{
		cpuUsedTable[opcode]=1;
		CPU_JumpTable[opcode](operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);
/*	if (cpuStopped)
	{
		printf("CPU STOPPED : %08x\n",cpu_regs.PC);
	}*/
	}

	if (MEM_getLong(0x6c)==0)
	{
		if (readyToTrap)
		{
//			startDebug=1;
			printf("Last PC %08X\n",lastPC);
		}
	}
	else
	{
		readyToTrap=1;
	}
}

