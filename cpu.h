/*
 *  cpu.h
 *  ami
 *
 *  Created by Lee Hammerton on 06/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

typedef struct 
{
	u_int32_t	D[8];
	u_int32_t	A[8];
	u_int32_t	PC;
	u_int16_t	SR;		// T1 T0 S M 0 I2 I1 I0 0 0 0 X N Z V C		(bit 15-0) NB low 8 = CCR
	
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