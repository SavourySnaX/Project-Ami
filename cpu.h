/*
 *  cpu.h
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

#include "mytypes.h"

typedef struct 
{
	u_int32_t	D[8];
	u_int32_t	A[8];
	u_int32_t	PC;
	u_int16_t	SR;		// T1 T0 S M 0 I2 I1 I0 0 0 0 X N Z V C		(bit 15-0) NB low 8 = CCR
	
	u_int32_t	USP,ISP;
	
// Hidden registers used by emulation
    u_int16_t	operands[8];
    u_int16_t	opcode;
	
	u_int32_t	stage;
	
	u_int32_t	stopped;
	
	u_int32_t	eas;
	u_int32_t	ead;
	u_int32_t	ear;
	u_int32_t	eat;
	
	u_int32_t	len;
	u_int32_t	iMask;
	u_int32_t	nMask;
	u_int32_t	zMask;
	
	u_int32_t	tmpL;
	u_int16_t	tmpW;

	u_int32_t	lastInstruction;		// Used by debugger (since sub stage instruction emulation shifts PC as it goes)
}CPU_Regs;

typedef u_int16_t (*CPU_Decode)(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
typedef u_int32_t (*CPU_Function)(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);

typedef struct
{
	char baseTable[17];
	char opcodeName[32];
	CPU_Function opcode;
	CPU_Decode   decode;
	int numOperands;
	u_int16_t   operandMask[8];
	u_int16_t   operandShift[8];
	int numValidMasks[8];
	char validEffectiveAddress[8][64][10];
} CPU_Ins;

extern CPU_Ins		cpu_instructions[];
extern CPU_Function	CPU_JumpTable[65536];
extern CPU_Decode	CPU_DisTable[65536];
extern CPU_Ins		*CPU_Information[65536];

#define	CPU_STATUS_T1		(1<<15)
#define	CPU_STATUS_T0		(1<<14)
#define	CPU_STATUS_S		(1<<13)
#define	CPU_STATUS_M		(1<<12)
#define	CPU_STATUS_I2		(1<<10)
#define	CPU_STATUS_I1		(1<<9)
#define	CPU_STATUS_I0		(1<<8)
#define	CPU_STATUS_X		(1<<4)
#define	CPU_STATUS_N		(1<<3)
#define	CPU_STATUS_Z		(1<<2)
#define	CPU_STATUS_V		(1<<1)
#define	CPU_STATUS_C		(1<<0)

extern CPU_Regs cpu_regs;

void CPU_BuildTable();

void CPU_Reset();

void CPU_Step();

void CPU_SaveState(FILE *outStream);
void CPU_LoadState(FILE *inStream);
