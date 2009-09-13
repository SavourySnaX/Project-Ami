/*
 *  cpu_ops.c
 *  ami
 *
 *  Created by Lee Hammerton on 31/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdio.h>

#include "cpu_ops.h"

#include "cpu.h"
#include "memory.h"

u_int32_t getEffectiveAddress(u_int16_t operand,int length)
{
	u_int16_t tmp;
    u_int32_t ea;
    switch (operand)
    {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			ea = cpu_regs.D[operand];
			break;
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			ea = cpu_regs.A[operand-0x08];
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			ea = cpu_regs.A[operand-0x10];
			break;
		case 0x18:
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			if ((operand==0x1F) && (length==1))
			{
				//startDebug=1;
				length=2;
			}
			ea = cpu_regs.A[operand-0x18];
			cpu_regs.A[operand-0x18]+=length;
			break;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			if ((operand==0x27) && (length==1))
			{
				//startDebug=1;
				length=2;
			}
			cpu_regs.A[operand-0x20]-=length;
			ea = cpu_regs.A[operand-0x20];
			break;
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			ea = cpu_regs.A[operand-0x28] + (int16_t)MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x30:	// 110rrr
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			tmp = MEM_getWord(cpu_regs.PC);
			if (tmp&0x8000)
			{
				ea = cpu_regs.A[(tmp>>12)&0x7];
				if (!(tmp&0x0800))
					ea=(int16_t)ea;
				ea += (int8_t)(tmp&0xFF);
				ea += cpu_regs.A[operand-0x30];
			}
			else
			{
				ea = cpu_regs.D[(tmp>>12)];
				if (!(tmp&0x0800))
					ea=(int16_t)ea;
				ea += (int8_t)(tmp&0xFF);
				ea += cpu_regs.A[operand-0x30];
			}
			cpu_regs.PC+=2;
			break;
		case 0x38:		/// 111000
			ea = (int16_t)MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x39:		/// 111001
			ea = MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
		case 0x3A:		/// 111010
			ea = cpu_regs.PC+(int16_t)MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x3B:		/// 111100
			tmp = MEM_getWord(cpu_regs.PC);
			if (tmp&0x8000)
			{
				ea = cpu_regs.A[(tmp>>12)&0x7];
				if (!(tmp&0x0800))
					ea=(int16_t)ea;
				ea += (int8_t)(tmp&0xFF);
				ea += cpu_regs.PC;
			}
			else
			{
				ea = cpu_regs.D[(tmp>>12)];
				if (!(tmp&0x0800))
					ea=(int16_t)ea;
				ea += (int8_t)(tmp&0xFF);
				ea += cpu_regs.PC;
			}
			cpu_regs.PC+=2;
			break;
		case 0x3C:		/// 111100
			switch (length)
	    {
			case 1:
				ea = MEM_getByte(cpu_regs.PC+1);
				cpu_regs.PC+=2;
				break;
			case 2:
				ea = MEM_getWord(cpu_regs.PC);
				cpu_regs.PC+=2;
				break;
			case 4:
				ea = MEM_getLong(cpu_regs.PC);
				cpu_regs.PC+=4;
				break;
	    }
			break;
		default:
			printf("[ERR] Unsupported effective addressing mode : %04X\n", operand);
			SOFT_BREAK;
			break;
    }
    return ea;
}

u_int32_t getSourceEffectiveAddress(u_int16_t operand,int length)
{
	u_int16_t opt;
	u_int32_t eas;

	eas = getEffectiveAddress(operand,length);
	opt=operand&0x38;
	if ( (opt < 0x10) || (operand == 0x3C) )
	{
	}
	else
	{
		switch (length)
		{
			case 1:
				return MEM_getByte(eas);
				break;
			case 2:
				return MEM_getWord(eas);
				break;
			case 4:
				return MEM_getLong(eas);
				break;
		}
	}
	return eas;
}

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

///////////////////////////////////////////////////////////////////////////////

u_int32_t CPU_LEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ea;
	
    ea = getEffectiveAddress(op2,4);
    cpu_regs.A[op1] = ea;
	
	return 0;
}

u_int32_t CPU_MOVE(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	u_int16_t t1,t2;
    u_int32_t ead,eas;
	
    switch(op1)
    {
		case 0x01:
			len=1;
			break;
		case 0x03:
			len=2;
			break;
		case 0x02:
			len=4;
			break;
    }
	
	t1 = (op2 & 0x38)>>3;
	t2 = (op2 & 0x07)<<3;
	op2 = t1|t2;
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		eas = getSourceEffectiveAddress(op3,len);
		switch (len)
		{
			case 1:
				cpu_regs.D[op2]&=0xFFFFFF00;
				cpu_regs.D[op2]|=eas&0xFF;
				break;
			case 2:
				cpu_regs.D[op2]&=0xFFFF0000;
				cpu_regs.D[op2]|=eas&0xFFFF;
				break;
			case 4:
				cpu_regs.D[op2]=eas;
				break;
		}
    }
    else
    {
		eas = getSourceEffectiveAddress(op3,len);
		ead = getEffectiveAddress(op2,len);
		switch (len)
		{
			case 1:
				MEM_setByte(ead,eas&0xFF);
				break;
			case 2:
				MEM_setWord(ead,eas&0xFFFF);
				break;
			case 4:
				MEM_setLong(ead,eas);
				break;
		}
    }
	
    cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
    u_int32_t nMask,zMask;
    switch (len)
    {
		case 1:
			nMask=0x80;
			zMask=0xFF;
			break;
		case 2:
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 4:
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
    if (eas & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (eas & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_SUBs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=getSourceEffectiveAddress(op3,len);
	ear=(ead-eas)&zMask;
	
	cpu_regs.D[op1]&=~zMask;
	cpu_regs.D[op1]|=ear;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_SUBQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
	if (op1==0)
		op1=8;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	eas=op1&zMask;
    if ((op3 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op3]&zMask;
		ear=(ead - eas)&zMask;
		cpu_regs.D[op3]&=~zMask;
		cpu_regs.D[op3]|=ear;
    }
    else
    {
		if ((op3 & 0x38)==0x08)
		{
			if (zMask==0xFF)
			{
				SOFT_BREAK;
			}
			ead=cpu_regs.A[op3&0x07]&zMask;
			ear=(ead - eas)&zMask;
			cpu_regs.A[op3&0x07]&=~zMask;
			cpu_regs.A[op3&0x07]|=ear;
		}
		else
		{
			ead=getEffectiveAddress(op3,len);
			switch (len)
			{
				case 1:
					eat=MEM_getByte(ead);
					ear=(eat - eas)&zMask;
					MEM_setByte(ead,ear);
					ead=eat;
					break;
				case 2:
					eat=MEM_getWord(ead);
					ear=(eat - eas)&zMask;
					MEM_setWord(ead,ear);
					ead=eat;
					break;
				case 4:
					eat=MEM_getLong(ead);
					ear=(eat - eas)&zMask;
					MEM_setLong(ead,ear);
					ead=eat;
					break;
			}
		}
    }
	
	if ((op3 & 0x38)==0x08)
		return 0;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_BCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int cc=0;
	u_int32_t ead;
	
    switch (op1)
    {
		default:
			SOFT_BREAK;
			break;
		case 0x02:
			cc = ((~cpu_regs.SR)&CPU_STATUS_C) && ((~cpu_regs.SR)&CPU_STATUS_Z);
			break;
		case 0x03:
			cc = (cpu_regs.SR & CPU_STATUS_C) || (cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x04:
			cc = !(cpu_regs.SR & CPU_STATUS_C);
			break;
		case 0x05:
			cc = (cpu_regs.SR & CPU_STATUS_C);
			break;
		case 0x06:
			cc = !(cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x07:
			cc = (cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x08:
			cc = !(cpu_regs.SR & CPU_STATUS_V);
			break;
		case 0x09:
			cc = (cpu_regs.SR & CPU_STATUS_V);
			break;
		case 0x0A:
			cc = !(cpu_regs.SR & CPU_STATUS_N);
			break;
		case 0x0B:
			cc = (cpu_regs.SR & CPU_STATUS_N);
			break;
		case 0x0C:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V)) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)));
			break;
		case 0x0D:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
			break;
		case 0x0E:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V) && (!(cpu_regs.SR & CPU_STATUS_Z))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)) && (!(cpu_regs.SR & CPU_STATUS_Z)));
			break;
		case 0x0F:
			cc = (cpu_regs.SR & CPU_STATUS_Z) || ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
			break;
    }
	
	ead = cpu_regs.PC;
    if (op2==0)
	{
		op2=MEM_getWord(cpu_regs.PC);
		cpu_regs.PC+=2;
	}
	else
	{
		if (op2&0x80)
		{
			op2|=0xFF00;
		}
	}
	
	if (cc)
	{
		cpu_regs.PC=ead+(int16_t)op2;
	}
	
	return 0;
}

u_int32_t CPU_CMPA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=2;
			break;
		case 0x01:
			len=4;
			break;
    }
	
	eas = getSourceEffectiveAddress(op3,len);
	if (len == 2)
	{
		eas = (int16_t)eas;
	}
	ead=cpu_regs.A[op1];
	ear=(ead - eas);
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=0x80000000;
	eas&=0x80000000;
	ead&=0x80000000;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=CPU_STATUS_C;
	else
		cpu_regs.SR&=~CPU_STATUS_C;
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_CMP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=getSourceEffectiveAddress(op3,len)&zMask;
	ear=(ead - eas)&zMask;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=CPU_STATUS_C;
	else
		cpu_regs.SR&=~CPU_STATUS_C;
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_CMPI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			eas=MEM_getByte(cpu_regs.PC+1);
			cpu_regs.PC+=2;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas=MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas=MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
    }
    else
    {
		ead=getEffectiveAddress(op2, len);
		switch (len)
		{
			case 1:
				ead=MEM_getByte(ead);
				break;
			case 2:
				ead=MEM_getWord(ead);
				break;
			case 4:
				ead=MEM_getLong(ead);
				break;
		}
    }
	ear=(ead - eas)&zMask;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=CPU_STATUS_C;
	else
		cpu_regs.SR&=~CPU_STATUS_C;
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_MOVEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t eas;
	
    switch(op1)
    {
		case 0x03:
			len=2;
			break;
		case 0x02:
			len=4;
			break;
    }
	
	eas = getSourceEffectiveAddress(op3,len);
	if (len == 2)
	{
		eas = (int16_t)eas;
	}
	cpu_regs.A[op2]=eas;
	
	return 0;
}

u_int32_t CPU_DBCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int cc=0;
	u_int32_t ead;
	
    switch (op1)
    {
		case 0x00:
			cc = 1;
			break;
		case 0x01:
			cc = 0;
			break;
		case 0x02:
			cc = ((~cpu_regs.SR)&CPU_STATUS_C) && ((~cpu_regs.SR)&CPU_STATUS_Z);
			break;
		case 0x03:
			cc = (cpu_regs.SR & CPU_STATUS_C) || (cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x04:
			cc = !(cpu_regs.SR & CPU_STATUS_C);
			break;
		case 0x05:
			cc = (cpu_regs.SR & CPU_STATUS_C);
			break;
		case 0x06:
			cc = !(cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x07:
			cc = (cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x08:
			cc = !(cpu_regs.SR & CPU_STATUS_V);
			break;
		case 0x09:
			cc = (cpu_regs.SR & CPU_STATUS_V);
			break;
		case 0x0A:
			cc = !(cpu_regs.SR & CPU_STATUS_N);
			break;
		case 0x0B:
			cc = (cpu_regs.SR & CPU_STATUS_N);
			break;
		case 0x0C:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V)) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)));
			break;
		case 0x0D:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
			break;
		case 0x0E:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V) && (!(cpu_regs.SR & CPU_STATUS_Z))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)) && (!(cpu_regs.SR & CPU_STATUS_Z)));
			break;
		case 0x0F:
			cc = (cpu_regs.SR & CPU_STATUS_Z) || ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
			break;
    }
	
	ead = cpu_regs.PC;
	
	op3=MEM_getWord(cpu_regs.PC);
	cpu_regs.PC+=2;
	
	if (!cc)
	{
		op4=(cpu_regs.D[op2]-1)&0xFFFF;
		cpu_regs.D[op2]&=0xFFFF0000;
		cpu_regs.D[op2]|=op4;
		if (op4!=0xFFFF)
		{
			cpu_regs.PC=ead+(int16_t)op3;
		}
	}
	
	return 0;
}

u_int32_t CPU_BRA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    if (op1==0)
	{
		op1=MEM_getWord(cpu_regs.PC);
	}
	else
	{
		if (op1&0x80)
		{
			op1|=0xFF00;
		}
	}
	
	cpu_regs.PC+=(int16_t)op1;
	
	return 0;
}

u_int32_t CPU_BTSTI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas;
	
	len=1;
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;
	
	ead=getSourceEffectiveAddress(op1,len);
	if (op1<8)
		eas&=0x1F;
	else
		eas&=0x07;

	eas = 1<<eas;

	if (ead & eas)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_ADDs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=getSourceEffectiveAddress(op3,len);
	ear=(eas+ead)&zMask;
	
	cpu_regs.D[op1]&=~zMask;
	cpu_regs.D[op1]|=ear;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_NOT(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(~ead)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		ead=getEffectiveAddress(op2,len);
		switch (len)
		{
			case 1:
				eas=MEM_getByte(ead);
				ear=(~eas)&zMask;
				MEM_setByte(ead,ear);
				break;
			case 2:
				eas=MEM_getWord(ead);
				ear=(~eas)&zMask;
				MEM_setWord(ead,ear);
				break;
			case 4:
				eas=MEM_getLong(ead);
				ear=(~eas)&zMask;
				MEM_setLong(ead,ear);
				break;
		}
    }

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	cpu_regs.SR&=~CPU_STATUS_V;
	
	return 0;
}

u_int32_t CPU_SUBA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=2;
			break;
		case 0x01:
			len=4;
			break;
    }
	
	eas = getSourceEffectiveAddress(op3,len);
	if (len == 2)
	{
		eas = (int16_t)eas;
	}
	ead=cpu_regs.A[op1];
	ear=(ead - eas);
	cpu_regs.A[op1]=ear;
	
	return 0;
}

u_int32_t CPU_ADDA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=2;
			break;
		case 0x01:
			len=4;
			break;
    }
	
	eas = getSourceEffectiveAddress(op3,len);
	if (len == 2)
	{
		eas = (int16_t)eas;
	}
	ead=cpu_regs.A[op1];
	ear=(ead + eas);
	cpu_regs.A[op1]=ear;
	
	return 0;
}

u_int32_t CPU_TST(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ear=getSourceEffectiveAddress(op2,len)&zMask;

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	cpu_regs.SR&=~CPU_STATUS_V;
	
	return 0;
}

u_int32_t CPU_JMP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ear;
	
	ear=getEffectiveAddress(op1,4);

	cpu_regs.PC=ear;
	
	return 0;
}

u_int32_t CPU_MOVEQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.D[op1]=(int8_t)op2;
	
    cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
    if (cpu_regs.D[op1] & 0x80000000)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (cpu_regs.D[op1])
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_LSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	if (op1>31)								// Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c
	{
		ead=0;
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		cpu_regs.SR&=~(CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_X|CPU_STATUS_C);
		cpu_regs.SR|=CPU_STATUS_Z;
		if ((eas&0x80000000) && (op1==32) && (op2==2))	// The one case where carry can occur
			cpu_regs.SR|=CPU_STATUS_X|CPU_STATUS_C;
	}
	else
	{
		ead = (eas >> op1)&zMask;
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		
		if (op1==0)
		{
			cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(1 << (op1-1)))
			{
				cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
			}
			else
			{
				cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
			}
		}
		cpu_regs.SR &= ~CPU_STATUS_V;
		if (cpu_regs.D[op4] & nMask)
			cpu_regs.SR|=CPU_STATUS_N;
		else
			cpu_regs.SR&=~CPU_STATUS_N;
		if (cpu_regs.D[op4] & zMask)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
	}
	
	return 0;
}

u_int32_t CPU_SWAP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t temp;

	temp = (cpu_regs.D[op1]&0xFFFF0000)>>16;
	cpu_regs.D[op1]=(cpu_regs.D[op1]&0x0000FFFF)<<16;
	cpu_regs.D[op1]|=temp;
	
    cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
    if (cpu_regs.D[op1] & 0x80000000)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (cpu_regs.D[op1])
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;

	return 0;
}

u_int32_t CPU_MOVEMs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x01:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	eas=MEM_getWord(cpu_regs.PC);
	cpu_regs.PC+=2;
	ead=getEffectiveAddress(op2,len);
	
	ear = 0;
	while (eas)
	{
		if (eas & 0x01)
		{
			if (ear < 8)
			{
				switch (len)
				{
					case 2:
						cpu_regs.D[ear] = (int16_t)MEM_getWord(ead);
						ead+=2;
						break;
					case 4:
						cpu_regs.D[ear] = MEM_getLong(ead);
						ead+=4;
						break;
				}
			}
			else
			{
				switch (len)
				{
					case 2:
						cpu_regs.A[ear-8] = (int16_t)MEM_getWord(ead);
						ead+=2;
						break;
					case 4:
						cpu_regs.A[ear-8] = MEM_getLong(ead);
						ead+=4;
						break;
				}
			}
		}
		ear++;
		eas>>=1;
	}
	
	if ((op2 & 0x38) == 0x18)			// handle post increment case
	{
		cpu_regs.A[op2-0x18] = ead;
	}
	
	return 0;
}

u_int32_t CPU_MOVEMd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x01:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	eas=MEM_getWord(cpu_regs.PC);
	cpu_regs.PC+=2;
	ead=getEffectiveAddress(op2,len);
	
	if ((op2 & 0x38) == 0x20)			// handle pre decrement case
	{
		ear = 15;
		ead+=len;
		cpu_regs.A[op2-0x20]+=len;
		while (eas)
		{
			if (eas & 0x01)
			{
				if (ear < 8)
				{
					switch (len)
					{
						case 2:
							ead-=2;
							MEM_setWord(ead,cpu_regs.D[ear]);
							break;
						case 4:
							ead-=4;
							MEM_setLong(ead,cpu_regs.D[ear]);
							break;
					}
				}
				else
				{
					switch (len)
					{
						case 2:
							ead-=2;
							MEM_setWord(ead,cpu_regs.A[ear-8]);
							break;
						case 4:
							ead-=4;
							MEM_setLong(ead,cpu_regs.A[ear-8]);
							break;
					}
				}
			}
			ear--;
			eas>>=1;
		}

		cpu_regs.A[op2-0x20] = ead;
	}
	else
	{
		ear = 0;
		while (eas)
		{
			if (eas & 0x01)
			{
				if (ear < 8)
				{
					switch (len)
					{
						case 2:
							MEM_setWord(ead,cpu_regs.D[ear]);
							ead+=2;
							break;
						case 4:
							MEM_setLong(ead,cpu_regs.D[ear]);
							ead+=4;
							break;
					}
				}
				else
				{
					switch (len)
					{
						case 2:
							MEM_setWord(ead,cpu_regs.A[ear-8]);
							ead+=2;
							break;
						case 4:
							MEM_setLong(ead,cpu_regs.A[ear-8]);
							ead+=4;
							break;
					}
				}
			}
			ear++;
			eas>>=1;
		}
	}	
	
	return 0;
}

u_int32_t CPU_SUBI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			eas=MEM_getByte(cpu_regs.PC+1);
			cpu_regs.PC+=2;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas=MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas=MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(ead - eas)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		ead=getEffectiveAddress(op2, len);
		switch (len)
		{
			case 1:
				eat=MEM_getByte(ead);
				ear=(eat - eas)&zMask;
				MEM_setByte(ead,ear&zMask);
				ead=eat;
				break;
			case 2:
				eat=MEM_getWord(ead);
				ear=(eat - eas)&zMask;
				MEM_setWord(ead,ear&zMask);
				ead=eat;
				break;
			case 4:
				eat=MEM_getWord(ead);
				ear=(eat - eas)&zMask;
				MEM_setLong(ead,ear&zMask);
				ead=eat;
				break;
		}
    }
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_BSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t nextInstruction;
	
	nextInstruction=cpu_regs.PC;
	
    if (op1==0)
	{
		op1=MEM_getWord(cpu_regs.PC);
		nextInstruction+=2;
	}
	else
	{
		if (op1&0x80)
		{
			op1|=0xFF00;
		}
	}
	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],nextInstruction);
	
	cpu_regs.PC+=(int16_t)op1;
	
	return 0;
}

u_int32_t CPU_RTS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.PC = MEM_getLong(cpu_regs.A[7]);
	cpu_regs.A[7]+=4;
	
	return 0;
}

u_int32_t CPU_ILLEGAL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	CPU_GENERATE_EXCEPTION(0x10);
	
	return 0;
}

u_int32_t CPU_ORd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=getEffectiveAddress(op3,len);
	switch (len)
	{
		case 1:
			eas=MEM_getByte(ead);
			ear=(cpu_regs.D[op1] | eas)&zMask;
			MEM_setByte(ead,ear);
			break;
		case 2:
			eas=MEM_getWord(ead);
			ear=(cpu_regs.D[op1] | eas)&zMask;
			MEM_setWord(ead,ear);
			break;
		case 4:
			eas=MEM_getLong(ead);
			ear=(cpu_regs.D[op1] | eas)&zMask;
			MEM_setLong(ead,ear);
			break;
    }

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	cpu_regs.SR&=~CPU_STATUS_V;
	
	return 0;
}

u_int32_t CPU_ADDQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
	if (op1==0)
		op1=8;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	eas=op1&zMask;
    if ((op3 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op3]&zMask;
		ear=(ead + eas)&zMask;
		cpu_regs.D[op3]&=~zMask;
		cpu_regs.D[op3]|=ear;
    }
    else
    {
		if ((op3 & 0x38)==0x08)
		{
			if (zMask==0xFF)
			{
				SOFT_BREAK;
			}
			ead=cpu_regs.A[op3&0x07]&zMask;
			ear=(ead + eas)&zMask;
			cpu_regs.A[op3&0x07]&=~zMask;
			cpu_regs.A[op3&0x07]|=ear;
		}
		else
		{
			ead=getEffectiveAddress(op3,len);
			switch (len)
			{
				case 1:
					eat=MEM_getByte(ead);
					ear=(eat + eas)&zMask;
					MEM_setByte(ead,ear);
					ead=eat;
					break;
				case 2:
					eat=MEM_getWord(ead);
					ear=(eat + eas)&zMask;
					MEM_setWord(ead,ear);
					ead=eat;
					break;
				case 4:
					eat=MEM_getLong(ead);
					ear=(eat + eas)&zMask;
					MEM_setLong(ead,ear);
					ead=eat;
					break;
			}
		}
    }

	if ((op3 & 0x38)==0x08)
		return 0;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_CLR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		cpu_regs.D[op2]&=~zMask;
    }
    else
    {
		ead=getEffectiveAddress(op2,len);
		switch (len)
		{
			case 1:
				MEM_getByte(ead);			//68000 memory location is read before it is cleared
				MEM_setByte(ead,0);
				break;
			case 2:
				MEM_getWord(ead);
				MEM_setWord(ead,0);
				break;
			case 4:
				MEM_getLong(ead);
				MEM_setLong(ead,0);
				break;
		}
    }

	cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_V|CPU_STATUS_N);
	cpu_regs.SR|=CPU_STATUS_Z;
	
	return 0;
}

u_int32_t CPU_ANDI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			eas=MEM_getByte(cpu_regs.PC+1);
			cpu_regs.PC+=2;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas=MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas=MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(ead & eas)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		ead=getEffectiveAddress(op2, len);
		switch (len)
		{
			case 1:
				ear=(MEM_getByte(ead) & eas)&zMask;
				MEM_setByte(ead,ear&zMask);
				break;
			case 2:
				ear=(MEM_getWord(ead) & eas)&zMask;
				MEM_setWord(ead,ear&zMask);
				break;
			case 4:
				ear=(MEM_getLong(ead) & eas)&zMask;
				MEM_setLong(ead,ear&zMask);
				break;
		}
    }
	
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
		
	return 0;
}

u_int32_t CPU_EXG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t temp;
	
	switch (op2)
	{
		case 0x08:			// Data & Data
			temp = cpu_regs.D[op1];
			cpu_regs.D[op1]=cpu_regs.D[op3];
			cpu_regs.D[op3]=temp;
			break;
		case 0x09:			// Address & Address
			temp = cpu_regs.A[op1];
			cpu_regs.A[op1]=cpu_regs.A[op3];
			cpu_regs.A[op3]=temp;
			break;
		case 0x11:			// Data & Address
			temp = cpu_regs.D[op1];
			cpu_regs.D[op1]=cpu_regs.A[op3];
			cpu_regs.A[op3]=temp;
			break;
	}
	
	return 0;
}

u_int32_t CPU_JSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ear;
	
	ear=getEffectiveAddress(op1,4);

	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],cpu_regs.PC);

	cpu_regs.PC=ear;
	
	return 0;
}

u_int32_t CPU_BCLRI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
	len=1;
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;
	
	ead=getEffectiveAddress(op1,len);
	if (op1<8)
	{
		eas&=0x1F;
		eas = 1<<eas;

		if (ead & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		cpu_regs.D[op1]&=~(eas);
	}
	else
	{
		eat=MEM_getByte(ead);
		eas&=0x07;
		eas = 1<<eas;

		if (eat & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		eat&=~(eas);
		MEM_setByte(ead,eat);
	}
	
	return 0;
}

u_int32_t CPU_ANDs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=getSourceEffectiveAddress(op3,len);
	ear=(ead&eas)&zMask;
	
	cpu_regs.D[op1]&=~zMask;
	cpu_regs.D[op1]|=ear;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_V);
	
	return 0;
}

u_int32_t CPU_SUBd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead = cpu_regs.D[op1]&zMask;
	eat=getEffectiveAddress(op3,len);
	switch (len)
	{
		case 1:
			eas=MEM_getByte(eat);
			ear=(eas - cpu_regs.D[op1])&zMask;
			MEM_setByte(eat,ear);
			break;
		case 2:
			eas=MEM_getWord(eat);
			ear=(eas - cpu_regs.D[op1])&zMask;
			MEM_setWord(eat,ear);
			break;
		case 4:
			eas=MEM_getLong(eat);
			ear=(eas - cpu_regs.D[op1])&zMask;
			MEM_setLong(eat,ear);
			break;
    }

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_BSET(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
	len=1;
	
	eas=cpu_regs.D[op1];
	if (op2<8)
	{
		eas&=0x1F;
		eas = 1<<eas;
		ead = cpu_regs.D[op2];

		if (ead & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		cpu_regs.D[op2]|=eas;
	}
	else
	{
		ead=getEffectiveAddress(op2,len);
		eat=MEM_getByte(ead);
		eas&=0x07;
		eas = 1<<eas;

		if (eat & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		eat|=eas;
		MEM_setByte(ead,eat);
	}
	
	return 0;
}

u_int32_t CPU_BSETI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
	len=1;
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;
	
	if (op1<8)
	{
		eas&=0x1F;
		eas = 1<<eas;
		ead = cpu_regs.D[op1];

		if (ead & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		cpu_regs.D[op1]|=eas;
	}
	else
	{
		ead=getEffectiveAddress(op1,len);
		eat=MEM_getByte(ead);
		eas&=0x07;
		eas = 1<<eas;

		if (eat & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		eat|=eas;
		MEM_setByte(ead,eat);
	}
	
	return 0;
}

u_int32_t CPU_MULU(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
	len=2;
	
	ead=cpu_regs.D[op1]&0xFFFF;
	eas=getSourceEffectiveAddress(op2,len)&0xFFFF;
	ear=eas * ead;

	cpu_regs.D[op1]=ear;

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=0x80000000;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	
	return 0;
}

u_int32_t CPU_LSL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	if (op1>31)								// Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c
	{
		ead=0;
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		cpu_regs.SR&=~(CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_X);
		cpu_regs.SR|=CPU_STATUS_Z;
		if ((eas==1) && (op1==32) && (op2==2))			// the one case where carry can occur
			cpu_regs.SR|=CPU_STATUS_X|CPU_STATUS_C;
	}
	else
	{
		ead = (eas << op1)&zMask;
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		
		if (op1==0)
		{
			cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(nMask >> (op1-1)))
			{
				cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
			}
			else
			{
				cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
			}
		}
		cpu_regs.SR &= ~CPU_STATUS_V;
		if (cpu_regs.D[op4] & nMask)
			cpu_regs.SR|=CPU_STATUS_N;
		else
			cpu_regs.SR&=~CPU_STATUS_N;
		if (cpu_regs.D[op4] & zMask)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
	}
	
	return 0;
}

u_int32_t CPU_ADDI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			eas=MEM_getByte(cpu_regs.PC+1);
			cpu_regs.PC+=2;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas=MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas=MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(ead + eas)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		ead=getEffectiveAddress(op2, len);
		switch (len)
		{
			case 1:
				eat=MEM_getByte(ead);
				ear=(eat + eas)&zMask;
				MEM_setByte(ead,ear&zMask);
				ead=eat;
				break;
			case 2:
				eat=MEM_getWord(ead);
				ear=(eat + eas)&zMask;
				MEM_setWord(ead,ear&zMask);
				ead=eat;
				break;
			case 4:
				eat=MEM_getLong(ead);
				ear=(eat + eas)&zMask;
				MEM_setLong(ead,ear&zMask);
				ead=eat;
				break;
		}
    }
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_EXT(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op1)
    {
		case 0x02:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas = (int8_t)(cpu_regs.D[op2]&0xFF);
			break;
		case 0x03:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas = (int16_t)(cpu_regs.D[op2]&0xFFFF);
			break;
    }

	ead = eas & zMask;
	cpu_regs.D[op2]&=~zMask;
	cpu_regs.D[op2]|=ead;
	
	if (cpu_regs.D[op2] & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (cpu_regs.D[op2] & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_V);
	
	return 0;
}

u_int32_t CPU_MULS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    int32_t ead,eas,ear;
	
	len=2;
	
	ead=(int16_t)(cpu_regs.D[op1]&0xFFFF);
	eas=(int16_t)(getSourceEffectiveAddress(op2,len)&0xFFFF);
	ear=eas * ead;

	cpu_regs.D[op1]=ear;

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=0x80000000;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	
	return 0;
}

u_int32_t CPU_NEG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(0 - ead)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		eas=getEffectiveAddress(op2,len);
		switch (len)
		{
			case 1:
				ead=MEM_getByte(eas);
				ear=(0-ead)&zMask;
				MEM_setByte(eas,ear);
				break;
			case 2:
				ead=MEM_getWord(eas);
				ear=(0-ead)&zMask;
				MEM_setWord(eas,ear);
				break;
			case 4:
				ead=MEM_getLong(eas);
				ear=(0-ead)&zMask;
				MEM_setLong(eas,ear);
				break;
		}
    }

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if (ear & ead)
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;

	if (ear | ead)
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
		
	return 0;
}

u_int32_t CPU_MOVEUSP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	if (cpu_regs.SR & CPU_STATUS_S)
	{
		if (op1)
		{
			cpu_regs.A[op2]=cpu_regs.USP;
		}
		else
		{
			cpu_regs.USP=cpu_regs.A[op2];
		}
	}
	else
	{
		cpu_regs.PC-=2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
	
	return 0;
}

u_int32_t CPU_SCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int cc=0;
	u_int32_t eas,ead;
	u_int8_t value;
	
    switch (op1)
    {
		case 0x00:
			cc = 1;
			break;
		case 0x01:
			cc = 0;
			break;
		case 0x02:
			cc = ((~cpu_regs.SR)&CPU_STATUS_C) && ((~cpu_regs.SR)&CPU_STATUS_Z);
			break;
		case 0x03:
			cc = (cpu_regs.SR & CPU_STATUS_C) || (cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x04:
			cc = !(cpu_regs.SR & CPU_STATUS_C);
			break;
		case 0x05:
			cc = (cpu_regs.SR & CPU_STATUS_C);
			break;
		case 0x06:
			cc = !(cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x07:
			cc = (cpu_regs.SR & CPU_STATUS_Z);
			break;
		case 0x08:
			cc = !(cpu_regs.SR & CPU_STATUS_V);
			break;
		case 0x09:
			cc = (cpu_regs.SR & CPU_STATUS_V);
			break;
		case 0x0A:
			cc = !(cpu_regs.SR & CPU_STATUS_N);
			break;
		case 0x0B:
			cc = (cpu_regs.SR & CPU_STATUS_N);
			break;
		case 0x0C:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V)) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)));
			break;
		case 0x0D:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
			break;
		case 0x0E:
			cc = ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V) && (!(cpu_regs.SR & CPU_STATUS_Z))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)) && (!(cpu_regs.SR & CPU_STATUS_Z)));
			break;
		case 0x0F:
			cc = (cpu_regs.SR & CPU_STATUS_Z) || ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
			break;
    }
	
	if (cc)
		value=0xFF;
	else
		value=0x00;
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		cpu_regs.D[op2]&=~0xFF;
		cpu_regs.D[op2]|=value;
    }
    else
    {
		eas=getEffectiveAddress(op2,1);
		ead=MEM_getByte(eas);
		MEM_setByte(eas,value);
    }
	
	return 0;
}

u_int32_t CPU_ORSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;
	
	if (cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR = cpu_regs.SR;
		cpu_regs.SR|=MEM_getWord(cpu_regs.PC);
		CPU_CHECK_SP(oldSR,cpu_regs.SR);
		cpu_regs.PC+=2;
	}
	else
	{
		cpu_regs.PC-=2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
	
	return 0;
}

u_int32_t CPU_PEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ear;
	
	ear=getEffectiveAddress(op1,4);

	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],ear);
	
	return 0;
}

// NB: This is oddly not a supervisor instruction on MC68000
u_int32_t CPU_MOVEFROMSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ear;

	if ((op1 & 0x38)==0)	// destination is D register
	{
		cpu_regs.D[op1]&=~0xFFFF;
		cpu_regs.D[op1]|=cpu_regs.SR;
	}
	else
	{
		ear=getEffectiveAddress(op1,2);
		MEM_getWord(ear);					// processor reads address before writing
		MEM_setWord(ear,cpu_regs.SR);
	}
	
	return 0;
}

u_int32_t CPU_RTE(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;

	if (cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR=cpu_regs.SR;

		cpu_regs.SR = MEM_getWord(cpu_regs.A[7]);
		cpu_regs.A[7]+=2;
		cpu_regs.PC = MEM_getLong(cpu_regs.A[7]);
		cpu_regs.A[7]+=4;

		CPU_CHECK_SP(oldSR,cpu_regs.SR);
	}
	else
	{
		cpu_regs.PC-=2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
	
	return 0;
}

u_int32_t CPU_ANDSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;
	
	if (cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR=cpu_regs.SR;
		cpu_regs.SR&=MEM_getWord(cpu_regs.PC);
		CPU_CHECK_SP(oldSR,cpu_regs.SR);
		cpu_regs.PC+=2;
	}
	else
	{
		cpu_regs.PC-=2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
	
	return 0;
}

u_int32_t CPU_MOVETOSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;
	u_int32_t ear;

	if (cpu_regs.SR & CPU_STATUS_S)
	{
		ear=getSourceEffectiveAddress(op1,2);
		oldSR=cpu_regs.SR;
		cpu_regs.SR=ear;
		CPU_CHECK_SP(oldSR,cpu_regs.SR);
	}
	else
	{
		cpu_regs.PC-=2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
	
	return 0;
}

u_int32_t CPU_LINK(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ear;

	ear = (int16_t)MEM_getWord(cpu_regs.PC);
	cpu_regs.PC+=2;

	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],cpu_regs.A[op1]);
	
	cpu_regs.A[op1]=cpu_regs.A[7];
	cpu_regs.A[7]+=ear;
	
	return 0;
}

u_int32_t CPU_CMPM(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t zMask,nMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			zMask = 0xFF;
			nMask = 0x80;
			ead = MEM_getByte(cpu_regs.A[op1]);
			eas = MEM_getByte(cpu_regs.A[op3]);
			cpu_regs.A[op1]+=1;
			cpu_regs.A[op3]+=1;
			break;
		case 0x01:
			zMask = 0xFFFF;
			nMask = 0x8000;
			ead = MEM_getWord(cpu_regs.A[op1]);
			eas = MEM_getWord(cpu_regs.A[op3]);
			cpu_regs.A[op1]+=2;
			cpu_regs.A[op3]+=2;
			break;
		case 0x03:
			zMask = 0xFFFFFFFF;
			nMask = 0x80000000;
			ead = MEM_getLong(cpu_regs.A[op1]);
			eas = MEM_getLong(cpu_regs.A[op3]);
			cpu_regs.A[op1]+=4;
			cpu_regs.A[op3]+=4;
			break;
    }
	
	ear=(ead - eas)&zMask;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=CPU_STATUS_C;
	else
		cpu_regs.SR&=~CPU_STATUS_C;
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_ADDd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eat=getEffectiveAddress(op3,len);
	switch (len)
	{
		case 1:
			eas=MEM_getByte(eat);
			ear=(cpu_regs.D[op1] + eas)&zMask;
			MEM_setByte(eat,ear);
			break;
		case 2:
			eas=MEM_getWord(eat);
			ear=(cpu_regs.D[op1] + eas)&zMask;
			MEM_setWord(eat,ear);
			break;
		case 4:
			eas=MEM_getLong(eat);
			ear=(cpu_regs.D[op1] + eas)&zMask;
			MEM_setLong(eat,ear);
			break;
    }

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_UNLK(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.A[7]=cpu_regs.A[op1];
	cpu_regs.A[op1]=MEM_getLong(cpu_regs.A[7]);
	cpu_regs.A[7]+=4;
	
	return 0;
}

u_int32_t CPU_ORs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=getSourceEffectiveAddress(op3,len);
	ear=(ead|eas)&zMask;
	
	cpu_regs.D[op1]&=~zMask;
	cpu_regs.D[op1]|=ear;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_V);
	
	return 0;
}

u_int32_t CPU_ANDd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=getEffectiveAddress(op3,len);
	switch (len)
	{
		case 1:
			eas=MEM_getByte(ead);
			ear=(cpu_regs.D[op1] & eas)&zMask;
			MEM_setByte(ead,ear);
			break;
		case 2:
			eas=MEM_getWord(ead);
			ear=(cpu_regs.D[op1] & eas)&zMask;
			MEM_setWord(ead,ear);
			break;
		case 4:
			eas=MEM_getLong(ead);
			ear=(cpu_regs.D[op1] & eas)&zMask;
			MEM_setLong(ead,ear);
			break;
    }

    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	cpu_regs.SR&=~CPU_STATUS_V;
	
	return 0;
}

u_int32_t CPU_ORI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			eas=MEM_getByte(cpu_regs.PC+1);
			cpu_regs.PC+=2;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas=MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas=MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(ead | eas)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		ead=getEffectiveAddress(op2, len);
		switch (len)
		{
			case 1:
				ear=(MEM_getByte(ead) | eas)&zMask;
				MEM_setByte(ead,ear&zMask);
				break;
			case 2:
				ear=(MEM_getWord(ead) | eas)&zMask;
				MEM_setWord(ead,ear&zMask);
				break;
			case 4:
				ear=(MEM_getLong(ead) | eas)&zMask;
				MEM_setLong(ead,ear&zMask);
				break;
		}
    }
	
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
		
	return 0;
}

u_int32_t CPU_ASL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	if (op1>31)								// Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c
	{
		ead=0;
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		cpu_regs.SR&=~(CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_X);
		cpu_regs.SR|=CPU_STATUS_Z;
		if ((eas==1) && (op1==32) && (op2==2))			// the one case where carry can occur
			cpu_regs.SR|=CPU_STATUS_X|CPU_STATUS_C;
		if (eas!=0)
			cpu_regs.SR|=CPU_STATUS_V;
	}
	else
	{
		ead = (eas << op1)&zMask;
	
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		
		if (op1==0)
		{
			cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(nMask >> (op1-1)))
			{
				cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
			}
			else
			{
				cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
			}
		}
		
		cpu_regs.SR &= ~CPU_STATUS_V;
		while(op1)							// This is rubbish, can't think of a better test at present though
		{
			if ( (eas & nMask) ^ ((eas & (nMask >> op1))<<op1) )
			{
				cpu_regs.SR |= CPU_STATUS_V;
				break;
			}
			
			op1--;		
		};
		
		if (cpu_regs.D[op4] & nMask)
			cpu_regs.SR|=CPU_STATUS_N;
		else
			cpu_regs.SR&=~CPU_STATUS_N;
		if (cpu_regs.D[op4] & zMask)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
	}
	
	return 0;
}

u_int32_t CPU_ASR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	if (op1>31)								// Its like this because the processor is modulo 64 on shifts and >31 with uint32 is not doable in c
	{
		if (eas&nMask)
		{
			ead=0xFFFFFFFF&zMask;
			cpu_regs.D[op4]&=~zMask;
			cpu_regs.D[op4]|=ead;
			cpu_regs.SR&=~(CPU_STATUS_Z|CPU_STATUS_V);
			cpu_regs.SR|=CPU_STATUS_N|CPU_STATUS_X|CPU_STATUS_C;
		}
		else
		{
			ead=0;
			cpu_regs.D[op4]&=~zMask;
			cpu_regs.D[op4]|=ead;
			cpu_regs.SR&=~(CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_X|CPU_STATUS_C);
			cpu_regs.SR|=CPU_STATUS_Z;
		}
	}
	else
	{
		ead = (eas >> op1)&zMask;
		if ((eas&nMask) && op1)
		{
			eas|=0xFFFFFFFF & (~zMask);		// correct eas for later carry test
			ear = (nMask >> (op1-1));
			if (ear)
			{
				ead|=(~(ear-1))&zMask;		// set sign bits
			}
			else
			{
				ead|=0xFFFFFFFF&zMask;		// set sign bits
			}
		}
		cpu_regs.D[op4]&=~zMask;
		cpu_regs.D[op4]|=ead;
		
		if (op1==0)
		{
			cpu_regs.SR &= ~CPU_STATUS_C;
		}
		else
		{
			if (eas&(1 << (op1-1)))
			{
				cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
			}
			else
			{
				cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
			}
		}
		
		cpu_regs.SR &= ~CPU_STATUS_V;
		
		if (cpu_regs.D[op4] & nMask)
			cpu_regs.SR|=CPU_STATUS_N;
		else
			cpu_regs.SR&=~CPU_STATUS_N;
		if (cpu_regs.D[op4] & zMask)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
	}
	
	return 0;
}

u_int32_t CPU_DIVU(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eaq,ear;
	
	len=2;
	
	ead=cpu_regs.D[op1];
	eas=getSourceEffectiveAddress(op2,len)&0xFFFF;
	
	if (!eas)
	{
		SOFT_BREAK;		// NEED TO DO A TRAP HERE
	}
	
	eaq=ead / eas;
	ear=ead % eas;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	if (eaq>65535)
	{
		cpu_regs.SR|=CPU_STATUS_V|CPU_STATUS_N;		// MC68000 docs claim N and Z are undefined. however real amiga seems to set N if V happens, Z is cleared
		cpu_regs.SR&=~CPU_STATUS_Z;
	}
	else
	{
		cpu_regs.SR&=~CPU_STATUS_V;
		
		cpu_regs.D[op1]=(eaq&0xFFFF)|((ear<<16)&0xFFFF0000);

		if (eaq&0x8000)
			cpu_regs.SR|=CPU_STATUS_N;
		else
			cpu_regs.SR&=~CPU_STATUS_N;
		
		if (eaq&0xFFFF)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
	}	
	
	return 0;
}

u_int32_t CPU_BCLR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
	len=1;
	
	eas=cpu_regs.D[op1];
	if (op2<8)
	{
		ead=cpu_regs.D[op2];
		eas&=0x1F;
		eas = 1<<eas;

		if (ead & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		cpu_regs.D[op2]&=~eas;
	}
	else
	{
		ead=getEffectiveAddress(op2,len);
		eat=MEM_getByte(ead);
		eas&=0x07;
		eas = 1<<eas;

		if (eat & eas)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
			
		eat&=~eas;
		MEM_setByte(ead,eat);
	}
	
	return 0;
}

u_int32_t CPU_EORd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
    if ((op3 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op3]&zMask;
		ear=(ead ^ cpu_regs.D[op1])&zMask;
		cpu_regs.D[op3]&=~zMask;
		cpu_regs.D[op3]|=ear;
    }
    else
	{
		ead=getEffectiveAddress(op3,len);
		switch (len)
		{
			case 1:
				eas=MEM_getByte(ead);
				ear=(cpu_regs.D[op1] ^ eas)&zMask;
				MEM_setByte(ead,ear);
				break;
			case 2:
				eas=MEM_getWord(ead);
				ear=(cpu_regs.D[op1] ^ eas)&zMask;
				MEM_setWord(ead,ear);
				break;
			case 4:
				eas=MEM_getLong(ead);
				ear=(cpu_regs.D[op1] ^ eas)&zMask;
				MEM_setLong(ead,ear);
				break;
		}
	}
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	cpu_regs.SR&=~CPU_STATUS_V;
	
	return 0;
}

u_int32_t CPU_BTST(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas;
	
	len=1;
	eas=cpu_regs.D[op1];
	
	ead=getSourceEffectiveAddress(op2,len);
	if (op2<8)
		eas&=0x1F;
	else
		eas&=0x07;

	eas = 1<<eas;

	if (ead & eas)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_STOP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;

	if (cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR=cpu_regs.SR;
		cpu_regs.SR = MEM_getWord(cpu_regs.PC);
		cpu_regs.PC+=2;

		CPU_CHECK_SP(oldSR,cpu_regs.SR);

		cpu_regs.stopped=1;
	}
	else
	{
		cpu_regs.PC-=2;
		CPU_GENERATE_EXCEPTION(0x20);
	}
	
	return 0;
}

u_int32_t CPU_ROL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	ead = eas;
	cpu_regs.SR &= ~(CPU_STATUS_V|CPU_STATUS_C);
	while (op1)
	{
		if (ead & nMask)
		{
			ead<<=1;
			ead|=1;
			ead&=zMask;
			cpu_regs.SR|=CPU_STATUS_C;
		}
		else
		{
			ead<<=1;
			ead&=zMask;
			cpu_regs.SR&=~CPU_STATUS_C;
		}
		op1--;
	}
	cpu_regs.D[op4]&=~zMask;
	cpu_regs.D[op4]|=ead;
	
	if (cpu_regs.D[op4] & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
	else
		cpu_regs.SR&=~CPU_STATUS_N;
	if (cpu_regs.D[op4] & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_ROR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	ead = eas;
	cpu_regs.SR &= ~(CPU_STATUS_V|CPU_STATUS_C);
	while (op1)
	{
		if (ead & 0x01)
		{
			ead>>=1;
			ead|=nMask;
			ead&=zMask;
			cpu_regs.SR|=CPU_STATUS_C;
		}
		else
		{
			ead>>=1;
			ead&=~nMask;
			ead&=zMask;
			cpu_regs.SR&=~CPU_STATUS_C;
		}
		op1--;
	}
	cpu_regs.D[op4]&=~zMask;
	cpu_regs.D[op4]|=ead;
	
	if (cpu_regs.D[op4] & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
	else
		cpu_regs.SR&=~CPU_STATUS_N;
	if (cpu_regs.D[op4] & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_NOP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	return 0;
}

u_int32_t CPU_BCHG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
	len=1;
	
	eas=cpu_regs.D[op1];
	if (op2<8)
	{
		ead=cpu_regs.D[op2];
		eas&=0x1F;
		eas = 1<<eas;

		if (ead & eas)
		{
			cpu_regs.SR&=~CPU_STATUS_Z;
			cpu_regs.D[op2]&=~eas;
		}
		else
		{
			cpu_regs.SR|=CPU_STATUS_Z;
			cpu_regs.D[op2]|=eas;
		}	
	}
	else
	{
		ead=getEffectiveAddress(op2,len);
		eat=MEM_getByte(ead);
		eas&=0x07;
		eas = 1<<eas;

		if (eat & eas)
		{
			cpu_regs.SR&=~CPU_STATUS_Z;
			eat&=~eas;
		}
		else
		{
			cpu_regs.SR|=CPU_STATUS_Z;
			eat|=eas;
		}
			
		MEM_setByte(ead,eat);
	}
	
	return 0;
}

u_int32_t CPU_BCHGI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
	len=1;
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;
	
	ead=getEffectiveAddress(op1,len);
	if (op1<8)
	{
		eas&=0x1F;
		eas = 1<<eas;

		if (ead & eas)
		{
			cpu_regs.SR&=~CPU_STATUS_Z;
			cpu_regs.D[op1]&=~(eas);
		}
		else
		{
			cpu_regs.SR|=CPU_STATUS_Z;
			cpu_regs.D[op1]|=eas;
		}
			
	}
	else
	{
		eat=MEM_getByte(ead);
		eas&=0x07;
		eas = 1<<eas;

		if (eat & eas)
		{
			cpu_regs.SR&=~CPU_STATUS_Z;
			eat&=~(eas);
		}
		else
		{
			cpu_regs.SR|=CPU_STATUS_Z;
			eat|=eas;
		}
			
		MEM_setByte(ead,eat);
	}
	
	return 0;
}

u_int32_t CPU_LSRm(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,ear;
	
	len=2;
	nMask=0x8000;
	zMask=0xFFFF;

	eas = getEffectiveAddress(op1, len);
	
	ead = MEM_getWord(eas);
	
	ear = (ead >> 1)&zMask;

	MEM_setWord(eas,ear);
	
	if (ead&(1 << (1-1)))
	{
		cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
	}
	else
	{
		cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
	}

	cpu_regs.SR &= ~CPU_STATUS_V;
	if (ear & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (ear & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_EORI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op1)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			eas=MEM_getByte(cpu_regs.PC+1);
			cpu_regs.PC+=2;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			eas=MEM_getWord(cpu_regs.PC);
			cpu_regs.PC+=2;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			eas=MEM_getLong(cpu_regs.PC);
			cpu_regs.PC+=4;
			break;
    }
	
    if ((op2 & 0x38)==0)	// destination is D register
    {
		ead=cpu_regs.D[op2]&zMask;
		ear=(ead ^ eas)&zMask;
		cpu_regs.D[op2]&=~zMask;
		cpu_regs.D[op2]|=ear;
    }
    else
    {
		ead=getEffectiveAddress(op2, len);
		switch (len)
		{
			case 1:
				ear=(MEM_getByte(ead) ^ eas)&zMask;
				MEM_setByte(ead,ear&zMask);
				break;
			case 2:
				ear=(MEM_getWord(ead) ^ eas)&zMask;
				MEM_setWord(ead,ear&zMask);
				break;
			case 4:
				ear=(MEM_getLong(ead) ^ eas)&zMask;
				MEM_setLong(ead,ear&zMask);
				break;
		}
    }
	
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
	
	ear&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
		
	return 0;
}

u_int32_t CPU_EORICCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t eas,ead;
	
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;

	ead=cpu_regs.SR;
	
	eas=(ead^eas)&(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
	
	return 0;
}

u_int32_t CPU_ROXL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	ead = eas;

	if (!op1)
	{
		if (cpu_regs.SR&CPU_STATUS_X)
			cpu_regs.SR|=CPU_STATUS_C;
		else
			cpu_regs.SR&=~CPU_STATUS_C;
	}
	
	while (op1)
	{
		if (ead & nMask)
		{
			ead<<=1;
			if (cpu_regs.SR&CPU_STATUS_X)
				ead|=1;
			ead&=zMask;
			cpu_regs.SR|=CPU_STATUS_C|CPU_STATUS_X;
		}
		else
		{
			ead<<=1;
			if (cpu_regs.SR&CPU_STATUS_X)
				ead|=1;
			ead&=zMask;
			cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
		}
		op1--;
	}
	cpu_regs.D[op4]&=~zMask;
	cpu_regs.D[op4]|=ead;
	
	if (cpu_regs.D[op4] & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
	else
		cpu_regs.SR&=~CPU_STATUS_N;
	if (cpu_regs.D[op4] & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_ROXR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
	}
	else
	{
		op1 = cpu_regs.D[op1]&0x3F;
	}

	eas = cpu_regs.D[op4]&zMask;
	ead = eas;

	if (!op1)
	{
		if (cpu_regs.SR&CPU_STATUS_X)
			cpu_regs.SR|=CPU_STATUS_C;
		else
			cpu_regs.SR&=~CPU_STATUS_C;
	}
	
	while (op1)
	{
		if (ead & 0x01)
		{
			ead>>=1;
			ead&=~nMask;
			if (cpu_regs.SR&CPU_STATUS_X)
				ead|=nMask;
			ead&=zMask;
			cpu_regs.SR|=CPU_STATUS_C|CPU_STATUS_X;
		}
		else
		{
			ead>>=1;
			ead&=~nMask;
			if (cpu_regs.SR&CPU_STATUS_X)
				ead|=nMask;
			ead&=zMask;
			cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
		}
		op1--;
	}
	cpu_regs.D[op4]&=~zMask;
	cpu_regs.D[op4]|=ead;
	
	if (cpu_regs.D[op4] & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
	else
		cpu_regs.SR&=~CPU_STATUS_N;
	if (cpu_regs.D[op4] & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_MOVETOCCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t eas,ead;
		
	eas=getSourceEffectiveAddress(op1,2);
		
	ead=cpu_regs.SR;
	
	eas&=(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
	
	return 0;
}

u_int32_t CPU_TRAP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	CPU_GENERATE_EXCEPTION(0x80 + (op1*4));
	
	return 0;
}

u_int32_t CPU_ADDX(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=cpu_regs.D[op3]&zMask;
	ear=(eas+ead)&zMask;
	if (cpu_regs.SR & CPU_STATUS_X)
		ear=(ear+1)&zMask;
	
	cpu_regs.D[op1]&=~zMask;
	cpu_regs.D[op1]|=ear;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_DIVS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    int32_t ead,eas,eaq,ear;
	
	len=2;
	
	ead=(int32_t)cpu_regs.D[op1];
	eas=(int16_t)(getSourceEffectiveAddress(op2,len)&0xFFFF);
	
	if (!eas)
	{
		SOFT_BREAK;		// NEED TO DO A TRAP HERE
	}
	
	eaq=ead / eas;
	ear=ead % eas;
	
	cpu_regs.SR&=~CPU_STATUS_C;
	if ((eaq<-32768) || (eaq>32767))
	{
		cpu_regs.SR|=CPU_STATUS_V|CPU_STATUS_N;		// MC68000 docs claim N and Z are undefined. however real amiga seems to set N if V happens, Z is cleared
		cpu_regs.SR&=~CPU_STATUS_Z;
	}
	else
	{
		cpu_regs.SR&=~CPU_STATUS_V;
		
		cpu_regs.D[op1]=(eaq&0xFFFF)|((ear<<16)&0xFFFF0000);

		if (eaq&0x8000)
			cpu_regs.SR|=CPU_STATUS_N;
		else
			cpu_regs.SR&=~CPU_STATUS_N;
		
		if (eaq&0xFFFF)
			cpu_regs.SR&=~CPU_STATUS_Z;
		else
			cpu_regs.SR|=CPU_STATUS_Z;
	}	
	
	return 0;
}

u_int32_t CPU_SUBX(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    switch(op2)
    {
		case 0x00:
			len=1;
			nMask=0x80;
			zMask=0xFF;
			break;
		case 0x01:
			len=2;
			nMask=0x8000;
			zMask=0xFFFF;
			break;
		case 0x02:
			len=4;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			break;
    }
	
	ead=cpu_regs.D[op1]&zMask;
	eas=cpu_regs.D[op3]&zMask;
	ear=(ead-eas)&zMask;
	if (cpu_regs.SR & CPU_STATUS_X)
		ear=(ear-1)&zMask;
	
	cpu_regs.D[op1]&=~zMask;
	cpu_regs.D[op1]|=ear;
	
    if (ear)
		cpu_regs.SR&=~CPU_STATUS_Z;
	
	ear&=nMask;
	eas&=nMask;
	ead&=nMask;
	
    if (ear)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
		
	return 0;
}

u_int32_t CPU_ASRm(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,eat;
	
	len=2;
	nMask=0x8000;
	zMask=0xFFFF;
	
	eat = getEffectiveAddress(op1,len);

	eas = MEM_getWord(eat);
	ead = (eas >> 1)&zMask;
	if (eas&nMask)
	{
		ead|=(~(nMask-1))&zMask;		// set sign bits
	}
	MEM_setWord(eat,ead&zMask);

	if (eas&1)
	{
		cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
	}
	else
	{
		cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
	}

	cpu_regs.SR &= ~CPU_STATUS_V;

	if (ead & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (ead & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
		
	return 0;
}

u_int32_t CPU_ASLm(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,eat;
	
	len=2;
	nMask=0x8000;
	zMask=0xFFFF;

	eat = getEffectiveAddress(op1,len);

	eas = MEM_getWord(eat);
	ead = (eas << 1)&zMask;
	MEM_setWord(eat,ead&zMask);

	if (eas&nMask)
	{
		cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
	}
	else
	{
		cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
	}

	if ((eas&nMask)!=(ead&nMask))
		cpu_regs.SR |= CPU_STATUS_V;
	else
		cpu_regs.SR &= ~CPU_STATUS_V;

	if (ead & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (ead & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;

	return 0;
}

u_int32_t CPU_ANDICCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t eas,ead;
	
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;

	ead=cpu_regs.SR;
	
	eas=(ead&eas)&(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;

	return 0;
}

u_int32_t CPU_ORICCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t eas,ead;
	
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;

	ead=cpu_regs.SR;
	
	eas=(ead|eas)&(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
	
	return 0;
}
