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
}CPU_Regs;


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
