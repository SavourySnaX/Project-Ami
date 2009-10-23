/*
 *  cpu_ops.c
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

#include "cpu_ops.h"

#include "cpu.h"
#include "memory.h"

///////////////////// New support functionality for cpu with stage and bus arbitration

u_int32_t BUS_Available(u_int32_t ea)			// TODO - Need to implement bus arbitration
{
	return 1;
}

//Applies a fudge to correct the cycle timings for some odd instructions (JMP)
u_int32_t FUDGE_EA_CYCLES(u_int32_t endCase,u_int32_t offs,u_int32_t stage,u_int16_t operand)
{
    switch (operand)
    {
		case 0x00:		// Dx
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			break;
		case 0x08:		// Ax
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			break;
		case 0x10:		// (Ax)
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			switch (stage)
			{
				case 0:
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 0x18:		// (Ax)+
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			break;
		case 0x20:		// -(Ax)
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			break;
		case 0x28:		// (XXXX,Ax)
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			switch (stage)
			{
				case 0:
					return 1+offs;
				case 1:
					break;
			}
			break;
		case 0x30:		// (XX,Ax,Xx)
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			switch (stage)
			{
				case 0:
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 0x38:		// (XXXX).W
			switch (stage)
			{
				case 0:
					return 1+offs;
				case 1:
					break;
			}
			break;
		case 0x39:		// (XXXXXXXX).L
			break;
		case 0x3A:		// (XXXX,PC)
			switch (stage)
			{
				case 0:
					return 1+offs;
				case 1:
					break;
			}
			break;
		case 0x3B:		/// (XX,PC,Xx)
			switch (stage)
			{
				case 0:
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 0x3C:		/// #XX.B #XXXX.W #XXXXXXXX.L
			break;
    }
	
	return endCase;
}

/*

 endCase signifies value when no need to re-enter EA calculation (calc uses upto 4 stages)
 offs is amount subtracted from stage to get 0 base (it is added to all return values (except endCase!)
 stage should be 0-4 on input

*/

u_int32_t COMPUTE_EFFECTIVE_ADDRESS(u_int32_t endCase,u_int32_t offs,u_int32_t stage,u_int32_t *ea,u_int16_t operand,int length,int asSrc)
{
    switch (operand)
    {
		case 0x00:		// Dx
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			*ea = cpu_regs.D[operand];
			return endCase;
		case 0x08:		// Ax
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			*ea = cpu_regs.A[operand-0x08];
			return endCase;
		case 0x10:		// (Ax)
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			*ea = cpu_regs.A[operand-0x10];
			return endCase;
		case 0x18:		// (Ax)+
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			if ((operand==0x1F) && (length==1))
			{
				length=2;
			}
			*ea = cpu_regs.A[operand-0x18];
			cpu_regs.A[operand-0x18]+=length;
			return endCase;
		case 0x20:		// -(Ax)
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			switch (stage)
			{
				case 0:
					if ((operand==0x27) && (length==1))
					{
						length=2;
					}
					cpu_regs.A[operand-0x20]-=length;
					if (asSrc)								// When used as src this costs 1 more cycle
						return 1+offs;
					break;
				case 1:
					break;
			}
			*ea=cpu_regs.A[operand-0x20];
			return endCase;
		case 0x28:		// (XXXX,Ax)
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			switch (stage)
			{
				case 0:
					*ea = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(*ea))
						return 1+offs;
					*ea = cpu_regs.A[operand-0x28] + (int16_t)MEM_getWord(*ea);
					return 2+offs;
				case 2:
					break;
			}
			return endCase;
		case 0x30:		// (XX,Ax,Xx)
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			switch (stage)
			{
				case 0:
					*ea = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(*ea))
						return 1+offs;
					cpu_regs.tmpW=MEM_getWord(*ea);
					return 2+offs;
				case 2:
					if (cpu_regs.tmpW&0x8000)
					{
						*ea = cpu_regs.A[(cpu_regs.tmpW>>12)&0x07];
					}
					else 
					{
						*ea = cpu_regs.D[(cpu_regs.tmpW>>12)&0x07];
					}
					return 3+offs;
				case 3:
					if (!(cpu_regs.tmpW&0x0800))
						*ea = (int16_t)*ea;
					*ea+=(int8_t)(cpu_regs.tmpW&0xFF);
					*ea+=cpu_regs.A[operand-0x30];
					break;
			}
			return endCase;
		case 0x38:		// (XXXX).W
			switch (stage)
			{
				case 0:
					*ea = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(*ea))
						return 1+offs;
					*ea = (int16_t)MEM_getWord(*ea);
					return 2+offs;
				case 2:
					break;
			}
			return endCase;
		case 0x39:		// (XXXXXXXX).L
			switch (stage)
			{
				case 0:
					cpu_regs.tmpL = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(cpu_regs.tmpL))
						return 1+offs;
					*ea = MEM_getWord(cpu_regs.tmpL)<<16;
					return 2+offs;
				case 2:
					cpu_regs.tmpL = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 3+offs;
				case 3:
					if (!BUS_Available(cpu_regs.tmpL))
						return 3+offs;
					*ea|=MEM_getWord(cpu_regs.tmpL);
					return 4+offs;
				case 4:
					break;
			}
			return endCase;
		case 0x3A:		// (XXXX,PC)
			switch (stage)
			{
				case 0:
					*ea = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(*ea))
						return 1+offs;
					*ea+=(int16_t)MEM_getWord(*ea);
					return 2+offs;
				case 2:
					break;
			}
			return endCase;
		case 0x3B:		/// (XX,PC,Xx)
			switch (stage)
			{
				case 0:
					cpu_regs.tmpL = cpu_regs.PC;
					cpu_regs.PC+=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(cpu_regs.tmpL))
						return 1+offs;
					cpu_regs.tmpW=MEM_getWord(cpu_regs.tmpL);
					return 2+offs;
				case 2:
					if (cpu_regs.tmpW&0x8000)
					{
						*ea = cpu_regs.A[(cpu_regs.tmpW>>12)&0x07];
					}
					else 
					{
						*ea = cpu_regs.D[(cpu_regs.tmpW>>12)&0x07];
					}
					return 3+offs;
				case 3:
					if (!(cpu_regs.tmpW&0x0800))
						*ea = (int16_t)*ea;
					*ea+=(int8_t)(cpu_regs.tmpW&0xFF);
					*ea+=cpu_regs.tmpL;
					break;
			}
			return endCase;
		case 0x3C:		/// #XX.B #XXXX.W #XXXXXXXX.L
			switch (length)
			{
				case 1:
					*ea = cpu_regs.PC+1;
					cpu_regs.PC+=2;
					return endCase;
				case 2:
					*ea = cpu_regs.PC;
					cpu_regs.PC+=2;
					return endCase;
				case 4:
					*ea = cpu_regs.PC;
					cpu_regs.PC+=4;
					break;
			}
			break;
    }
	
	return endCase;
}

int COMPUTE_CONDITION(u_int16_t op)
{
    switch (op)
    {
		case 0x00:
			return 1;
		case 0x01:
			return 0;
		case 0x02:
			return ((~cpu_regs.SR)&CPU_STATUS_C) && ((~cpu_regs.SR)&CPU_STATUS_Z);
		case 0x03:
			return (cpu_regs.SR & CPU_STATUS_C) || (cpu_regs.SR & CPU_STATUS_Z);
		case 0x04:
			return !(cpu_regs.SR & CPU_STATUS_C);
		case 0x05:
			return (cpu_regs.SR & CPU_STATUS_C);
		case 0x06:
			return !(cpu_regs.SR & CPU_STATUS_Z);
		case 0x07:
			return (cpu_regs.SR & CPU_STATUS_Z);
		case 0x08:
			return !(cpu_regs.SR & CPU_STATUS_V);
		case 0x09:
			return (cpu_regs.SR & CPU_STATUS_V);
		case 0x0A:
			return !(cpu_regs.SR & CPU_STATUS_N);
		case 0x0B:
			return (cpu_regs.SR & CPU_STATUS_N);
		case 0x0C:
			return ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V)) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)));
		case 0x0D:
			return ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
		case 0x0E:
			return ((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V) && (!(cpu_regs.SR & CPU_STATUS_Z))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)) && (!(cpu_regs.SR & CPU_STATUS_Z)));
		case 0x0F:
			return (cpu_regs.SR & CPU_STATUS_Z) || ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));
    }
	return 0;
}

u_int32_t LOAD_EFFECTIVE_VALUE(u_int32_t endCase,u_int32_t offs,u_int32_t stage,u_int16_t op,u_int32_t *ead,u_int32_t *eas,int length)
{
	if (op<0x10)
	{
		*ead=*eas;
		return endCase;
	}
	
	switch (length)
	{
		case 1:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(*eas))
						return 0+offs;
					*ead = MEM_getByte(*eas);
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 2:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(*eas))
						return 0+offs;
					*ead = MEM_getWord(*eas);
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					cpu_regs.tmpL = *eas;
					if (!BUS_Available(cpu_regs.tmpL))
						return 0+offs;
					*ead = MEM_getWord(cpu_regs.tmpL)<<16;
					return 1+offs;
				case 1:
					cpu_regs.tmpL+=2;
					return 2+offs;
				case 2:
					if (!BUS_Available(cpu_regs.tmpL))
						return 2+offs;
					*ead|=MEM_getWord(cpu_regs.tmpL);
					return 3+offs;
				case 3:
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

u_int32_t STORE_EFFECTIVE_VALUE(u_int32_t endCase,u_int32_t offs,u_int32_t stage,u_int16_t op,u_int32_t *ead,u_int32_t *eas)
{
	if (op<0x08)
	{
		cpu_regs.D[op]&=cpu_regs.iMask;
		cpu_regs.D[op]|=(*eas)&cpu_regs.zMask;
		return endCase;
	}
	if (op<0x10)
	{
		cpu_regs.A[op-0x08]&=cpu_regs.iMask;
		cpu_regs.A[op-0x08]|=(*eas)&cpu_regs.zMask;
		return endCase;
	}
	switch (cpu_regs.len)
	{
		case 1:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(*ead))
						return 0+offs;
					MEM_setByte(*ead,(*eas)&cpu_regs.zMask);
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 2:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(*ead))
						return 0+offs;
					MEM_setWord(*ead,(*eas)&cpu_regs.zMask);
					return 1+offs;
				case 1:
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					cpu_regs.tmpL=*ead;
					if (!BUS_Available(cpu_regs.tmpL))
						return 0+offs;
					MEM_setWord(cpu_regs.tmpL,((*eas)&cpu_regs.zMask)>>16);
					return 1+offs;
				case 1:
					cpu_regs.tmpL+=2;
					return 2+offs;
				case 2:
					if (!BUS_Available(cpu_regs.tmpL))
						return 2+offs;
					MEM_setWord(cpu_regs.tmpL,(*eas)&cpu_regs.zMask);
					return 3+offs;
				case 3:
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

u_int32_t PUSH_VALUE(u_int32_t endCase,u_int32_t offs,u_int32_t stage,u_int32_t val,int length)
{
	switch (length)
	{
		case 2:
			switch (stage)
			{
				case 0:
					cpu_regs.A[7]-=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(cpu_regs.A[7]))
						return 1+offs;
					MEM_setWord(cpu_regs.A[7],val);
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					cpu_regs.A[7]-=2;
					return 1+offs;
				case 1:
					if (!BUS_Available(cpu_regs.A[7]))
						return 1+offs;
					MEM_setWord(cpu_regs.A[7],val&0xFFFF);
					return 2+offs;
				case 2:
					cpu_regs.A[7]-=2;
					return 3+offs;
				case 3:
					if (!BUS_Available(cpu_regs.A[7]))
						return 3+offs;
					MEM_setWord(cpu_regs.A[7],val>>16);
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

u_int32_t POP_VALUE(u_int32_t endCase,u_int32_t offs,u_int32_t stage,u_int32_t *val,int length)
{
	switch (length)
	{
		case 2:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(cpu_regs.A[7]))
						return 0+offs;
					*val = MEM_getWord(cpu_regs.A[7]);
					return 1+offs;
				case 1:
					cpu_regs.A[7]+=2;
					return 2+offs;
				case 2:
					break;
			}
			break;
		case 4:
			switch (stage)
			{
				case 0:
					if (!BUS_Available(cpu_regs.A[7]))
						return 0+offs;
					*val = MEM_getWord(cpu_regs.A[7])<<16;
					return 1+offs;
				case 1:
					cpu_regs.A[7]+=2;
					return 2+offs;
				case 2:
					if (!BUS_Available(cpu_regs.A[7]))
						return 2+offs;
					*val |= MEM_getWord(cpu_regs.A[7]);
					return 3+offs;
				case 3:
					cpu_regs.A[7]+=2;
					return 4+offs;
				case 4:
					break;
			}
			break;
	}
	return endCase;
}

void OPCODE_SETUP_LENGTHM(u_int16_t op)
{
    switch(op)
    {
		case 0x01:
			cpu_regs.iMask=0xFFFFFF00;
			cpu_regs.nMask=0x80;
			cpu_regs.zMask=0xFF;
			cpu_regs.len=1;
			break;
		case 0x03:
			cpu_regs.iMask=0xFFFF0000;
			cpu_regs.nMask=0x8000;
			cpu_regs.zMask=0xFFFF;
			cpu_regs.len=2;
			break;
		case 0x02:
			cpu_regs.iMask=0x00000000;
			cpu_regs.nMask=0x80000000;
			cpu_regs.zMask=0xFFFFFFFF;
			cpu_regs.len=4;
			break;
	}
}

void OPCODE_SETUP_LENGTH(u_int16_t op)
{
    switch(op)
    {
		case 0x00:
			cpu_regs.iMask=0xFFFFFF00;
			cpu_regs.nMask=0x80;
			cpu_regs.zMask=0xFF;
			cpu_regs.len=1;
			break;
		case 0x01:
			cpu_regs.iMask=0xFFFF0000;
			cpu_regs.nMask=0x8000;
			cpu_regs.zMask=0xFFFF;
			cpu_regs.len=2;
			break;
		case 0x02:
			cpu_regs.iMask=0x00000000;
			cpu_regs.nMask=0x80000000;
			cpu_regs.zMask=0xFFFFFFFF;
			cpu_regs.len=4;
			break;
    }
}

void OPCODE_SETUP_LENGTHLW(u_int16_t op)
{
    switch(op)
    {
		case 0x00:
			cpu_regs.iMask=0xFFFF0000;
			cpu_regs.nMask=0x8000;
			cpu_regs.zMask=0xFFFF;
			cpu_regs.len=2;
			break;
		case 0x01:
			cpu_regs.iMask=0x00000000;
			cpu_regs.nMask=0x80000000;
			cpu_regs.zMask=0xFFFFFFFF;
			cpu_regs.len=4;
			break;
    }
}

void COMPUTE_Z_BIT(u_int32_t eas,u_int32_t ead)
{
	if (cpu_regs.ead & cpu_regs.eas)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
}

void COMPUTE_ZN_TESTS(u_int32_t ea)
{
	if (ea & cpu_regs.nMask)
		cpu_regs.SR|=CPU_STATUS_N;
	else
		cpu_regs.SR&=~CPU_STATUS_N;
	if (ea & cpu_regs.zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
	else
		cpu_regs.SR|=CPU_STATUS_Z;
}

void COMPUTE_ADD_XCV_TESTS(u_int32_t eas,u_int32_t ead,u_int32_t ear)
{
	ear&=cpu_regs.nMask;
	eas&=cpu_regs.nMask;
	ead&=cpu_regs.nMask;
	
	if ((eas & ead) | ((~ear) & ead) | (eas & (~ear)))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if ((eas & ead & (~ear)) | ((~eas) & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
}

void COMPUTE_SUB_XCV_TESTS(u_int32_t eas,u_int32_t ead,u_int32_t ear)
{
	ear&=cpu_regs.nMask;
	eas&=cpu_regs.nMask;
	ead&=cpu_regs.nMask;
	
	if ((eas & (~ead)) | (ear & (~ead)) | (eas & ear))
		cpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);
	else
		cpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);
	if (((~eas) & ead & (~ear)) | (eas & (~ead) & ear))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
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

// No end case on exception because its final act will be to return 0
u_int32_t PROCESS_EXCEPTION(u_int32_t offs,u_int32_t stage,u_int32_t vector)
{
	switch (stage)
	{
		case 0:
			cpu_regs.tmpW = cpu_regs.SR;
			return 1+offs;
		case 1:
			cpu_regs.SR|=CPU_STATUS_S;
			CPU_CHECK_SP(cpu_regs.tmpW,cpu_regs.SR);
			return 2+offs;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			return offs+PUSH_VALUE(7, 2, stage-2, cpu_regs.PC,4);
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			return offs+PUSH_VALUE(12,7, stage-7, cpu_regs.SR, 2);
		case 12:
			cpu_regs.tmpL=vector;
			return 13+offs;
		case 13:
			if (!BUS_Available(cpu_regs.tmpL))
				return 13+offs;
			cpu_regs.PC=MEM_getWord(cpu_regs.tmpL)<<16;
			return 14+offs;
		case 14:
			cpu_regs.tmpL+=2;
			return 15+offs;
		case 15:
			if (!BUS_Available(cpu_regs.tmpL))
				return 15+offs;
			cpu_regs.PC|=MEM_getWord(cpu_regs.tmpL);
			return 16+offs;
		case 16:
			return 17+offs;
		case 17:
			break;
	}

	return 0;
}

/////////// OLD SUPPORT TO BE REMOVED WHEN ALL INSTRUCTIONS WORKING

u_int32_t getEffectiveAddress(u_int16_t operand,int length)
{
	u_int16_t tmp;
    u_int32_t ea=0;
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
	u_int32_t ret;
	
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op2,4,1);
			if (ret!=6)
				return ret;
			break;
	}
	
	cpu_regs.A[op1]=cpu_regs.eas;
	return 0;
}

u_int32_t CPU_MOVE(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHM(op1);
			{
				u_int16_t t1,t2;
	
				t1 = (op2 & 0x38)>>3;			// needed to swizzle operand for correct later handling (move has inverted bit meanings for dest)
				t2 = (op2 & 0x07)<<3;
				cpu_regs.operands[1] = t1|t2;
			}
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, op3,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			ret = STORE_EFFECTIVE_VALUE(21,16,stage-16,op2,&cpu_regs.ead,&cpu_regs.eas);
			if (ret!=21)
				return ret;
			
			break;
	}
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.eas);
	return 0;
}

u_int32_t CPU_SUBQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			if (op1==0)
				cpu_regs.operands[0]=8;			// 0 means 8
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.ead,op3,cpu_regs.len,0);
			
			if (ret!=6)
				return ret;
			
			cpu_regs.eas=op1&cpu_regs.zMask;
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op3,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ear = (cpu_regs.eat - cpu_regs.eas)&cpu_regs.zMask;
			stage=11;
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret = STORE_EFFECTIVE_VALUE(16,11,stage-11,op3,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=16)
				return ret;
			
			cpu_regs.ead = cpu_regs.eat;
			stage=16;
		case 16:
			if ((op3 & 0x38)==0x00 && cpu_regs.len==4)	
				return 19;						// Data register direct long costs 2 cycles more
			if ((op3 & 0x38)==0x08)				// Address register direct does not affect flags
				return 17;						// also adds 2 cycles (but not worked out reason why yet)
			break;
		case 17:
			return 18;
		case 18:
			return 0;
		case 19:
			return 20;
		case 20:
			break;
			
	}

	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	
	return 0;
}

// Redone timings on this after reaching bra and realising a mistake was made
u_int32_t CPU_BCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
			cpu_regs.ead=cpu_regs.PC;
			return 2;
		case 2:
			if (!COMPUTE_CONDITION(op1))
			{
				return 6;		// costs 4 if byte form, or 6 if word
			}
			stage = 3;		// Falls through
		case 3:
			cpu_regs.eas=cpu_regs.PC;		// allways reads next word (even if it does not use it!)
			cpu_regs.PC+=2;
			return 4;
		case 4:
			if (!BUS_Available(cpu_regs.eas))
				return 4;
			cpu_regs.eas=MEM_getWord(cpu_regs.eas);
			return 5;
		case 5:
			if (op2!=0)
			{
				cpu_regs.eas=op2;
				if (op2&0x80)
				{
					cpu_regs.eas+=0xFF00;
				}
			}
			cpu_regs.PC=cpu_regs.ead+(int16_t)cpu_regs.eas;
			break;
		case 6:
			if (op2==0)
				return 7;
			break;
		case 7:
			cpu_regs.PC+=2;
			return 8;
		case 8:
			break;
	}

	return 0;
}

u_int32_t CPU_CMPA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHLW(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			
			if (ret!=6)
				return ret;
			
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op3,&cpu_regs.eas,&cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;
		case 11:
			cpu_regs.ead = cpu_regs.A[op1];
			if (cpu_regs.len==2)
			{
				cpu_regs.eas = (int16_t)cpu_regs.eas;
				OPCODE_SETUP_LENGTHLW(1);				// Rest of operation works as long
			}
			return 12;
		case 12:
			cpu_regs.ear = (cpu_regs.ead - cpu_regs.eas)&cpu_regs.zMask;
			break;
	}

	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	
	return 0;
}

u_int32_t CPU_CMPI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			ret = LOAD_EFFECTIVE_VALUE(21,16,stage-16,op2,&cpu_regs.ead,&cpu_regs.ead,cpu_regs.len);
			if (ret!=21)
				return ret;
			
			if ((op2&0x38)==0 && cpu_regs.len==4)
				return 21;							// Long reg direct takes 1 additional cycle
			stage=21;			// Fall through
		case 21:
			cpu_regs.ear = (cpu_regs.ead - cpu_regs.eas)&cpu_regs.zMask;
			break;
	}
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	return 0;
}

u_int32_t CPU_MOVEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHM(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, op3,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			if (cpu_regs.len==2)
			{
				cpu_regs.eas=(int16_t)cpu_regs.eas;
			}
			stage=11;			// Fall through
		case 11:
			cpu_regs.A[op2]=cpu_regs.eas;
			break;
	}
	return 0;
}
			
u_int32_t CPU_DBCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
			cpu_regs.ead=cpu_regs.PC;
			return 2;
		case 2:
			cpu_regs.eas=cpu_regs.PC;
			cpu_regs.PC+=2;
			return 3;
		case 3:
			if (!BUS_Available(cpu_regs.eas))
				return 3;
			cpu_regs.eas=MEM_getWord(cpu_regs.eas);
			return 4;
		case 4:
			if (COMPUTE_CONDITION(op1))
				return 7;						// burn one last cycle
			
			op4=(cpu_regs.D[op2]-1)&0xFFFF;
			cpu_regs.D[op2]&=0xFFFF0000;
			cpu_regs.D[op2]|=op4;
			if (op4==0xFFFF)
				return 6;						// burn two more cycles

			cpu_regs.PC=cpu_regs.ead+(int16_t)cpu_regs.eas;
			break;
		case 6:
			return 7;
		case 7:
			break;
	}

	return 0;
}

u_int32_t CPU_BRA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
			cpu_regs.ead=cpu_regs.PC;
			return 2;
		case 2:
			cpu_regs.eas=cpu_regs.PC;		// allways reads next word (even if it does not use it!)
			cpu_regs.PC+=2;
			return 3;
		case 3:
			if (!BUS_Available(cpu_regs.eas))
				return 3;
			cpu_regs.eas=MEM_getWord(cpu_regs.eas);
			return 4;
		case 4:
			if (op1!=0)
			{
				cpu_regs.eas=op1;
				if (op1&0x80)
				{
					cpu_regs.eas+=0xFF00;
				}
			}
			cpu_regs.PC=cpu_regs.ead+(int16_t)cpu_regs.eas;
			break;
	}

	return 0;
}

u_int32_t CPU_BTSTI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			if (op1<0x08)
				OPCODE_SETUP_LENGTH(2);			// Register direct uses long
			else
				OPCODE_SETUP_LENGTH(0);			// memory uses byte
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,1,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,1);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op1,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			ret = LOAD_EFFECTIVE_VALUE(21,16,stage-16,op1,&cpu_regs.ead,&cpu_regs.ead,cpu_regs.len);
			if (ret!=21)
				return ret;
			
			if ((op1&0x38)==0)
				return 21;							// Long reg direct takes 1 additional cycle
			stage=21;			// Fall through
		case 21:
			if (op1<0x08)
				cpu_regs.eas&=0x1F;
			else
				cpu_regs.eas&=0x07;
			cpu_regs.eas=1<<cpu_regs.eas;
			break;
	}

	COMPUTE_Z_BIT(cpu_regs.eas,cpu_regs.ead);
	return 0;
}

u_int32_t CPU_ADDs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, op3,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
			cpu_regs.ead = cpu_regs.D[op1]&cpu_regs.zMask;
			
			if (cpu_regs.len==4)
				return 12;							// Long reg direct takes 1 additional cycle
			stage=12;			// Fall through
		case 12:
			cpu_regs.ear = (cpu_regs.ead + cpu_regs.eas)&cpu_regs.zMask;
			
			cpu_regs.D[op1]&=cpu_regs.iMask;
			cpu_regs.D[op1]|=cpu_regs.ear;
			
			if (cpu_regs.len==4 && ((op3 == 0x3C)||(op3<0x10)))
				return 13;							// long source reg direct and immediate take 1 additional cycle
			break;
		case 13:
			break;
	}
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_ADD_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	return 0;
}

u_int32_t CPU_NOT(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=6)
				return ret;
			
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op2,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ear = (~cpu_regs.eat)&cpu_regs.zMask;
			stage=11;
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret = STORE_EFFECTIVE_VALUE(16,11,stage-11,op2,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=16)
				return ret;
			
			cpu_regs.ead = cpu_regs.eat;
			stage=16;
		case 16:
			if ((op2 & 0x38)==0x00)
			{
				if (cpu_regs.len==4)
					return 17;						// Data register direct long costs 1 cycles more
			}
			break;
		case 17:
			break;
	}

	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	return 0;
}

u_int32_t CPU_SUBA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHLW(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, op3,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
			cpu_regs.ead = cpu_regs.A[op1];
			
			return 12;
		case 12:
			if (cpu_regs.len==2)
			{
				cpu_regs.eas=(int16_t)cpu_regs.eas;		// sign extension costs an additional cycle
				return 13;
			}
			stage=13;			// Fall through
		case 13:
			cpu_regs.ear = (cpu_regs.ead - cpu_regs.eas);
			
			cpu_regs.A[op1]=cpu_regs.ear;
			
			if (cpu_regs.len==4 && ((op3 == 0x3C)||(op3<0x10)))
				return 14;							// long source reg direct and immediate take 1 additional cycle
			break;
		case 14:
			break;
	}
	return 0;
}

u_int32_t CPU_ADDA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHLW(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, op3,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
			cpu_regs.ead = cpu_regs.A[op1];
			
			return 12;
		case 12:
			if (cpu_regs.len==2)
			{
				cpu_regs.eas=(int16_t)cpu_regs.eas;		// sign extension costs an additional cycle
				return 13;
			}
			stage=13;			// Fall through
		case 13:
			cpu_regs.ear = (cpu_regs.ead + cpu_regs.eas);
			
			cpu_regs.A[op1]=cpu_regs.ear;
			
			if (cpu_regs.len==4 && ((op3 == 0x3C)||(op3<0x10)))
				return 14;							// long source reg direct and immediate take 1 additional cycle
			break;
		case 14:
			break;
	}
	return 0;
}

u_int32_t CPU_TST(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=6)
				return ret;
			
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op2,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ear = cpu_regs.eat&cpu_regs.zMask;
			break;
	}

	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	return 0;
}

u_int32_t CPU_JMP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op1,4,1);
			if (ret!=6)
				return ret;
			stage=6;		// Fall through
		case 6:
		case 7:
		case 8:
			ret=FUDGE_EA_CYCLES(9,6,stage-6,op1);		//ODD cycle timings for JMP -this rebalances
			if (ret!=9)
				return ret;
		
			break;
	}
	
	cpu_regs.PC=cpu_regs.eas;
	return 0;
}

u_int32_t CPU_MOVEQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(2);
			return 1;
		case 1:
			cpu_regs.D[op1]=(int8_t)op2;
			break;
	}
	
    cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.D[op1]);
	return 0;
}

u_int32_t CPU_CMP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			
			if (ret!=6)
				return ret;
			
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op3,&cpu_regs.eas,&cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ead = cpu_regs.D[op1]&cpu_regs.zMask;
			cpu_regs.ear = (cpu_regs.ead - cpu_regs.eas)&cpu_regs.zMask;
			stage=11;
		case 11:
			if ((op2 & 0x38)==0x00 && cpu_regs.len==4)
				return 12;						// Data register direct long costs 1 cycles more
			break;
		case 12:
			break;
	}

	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	return 0;
}

u_int32_t CPU_SUBs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op3,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, op3,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
			cpu_regs.ead = cpu_regs.D[op1]&cpu_regs.zMask;
			
			if (cpu_regs.len==4)
				return 12;							// Long reg direct takes 1 additional cycle
			stage=12;			// Fall through
		case 12:
			cpu_regs.ear = (cpu_regs.ead - cpu_regs.eas)&cpu_regs.zMask;
			
			cpu_regs.D[op1]&=cpu_regs.iMask;
			cpu_regs.D[op1]|=cpu_regs.ear;
			
			if (cpu_regs.len==4 && ((op3 == 0x3C)||(op3<0x10)))
				return 13;							// long source reg direct and immediate take 1 additional cycle
			break;
		case 13:
			break;
	}
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	return 0;
}

u_int32_t CPU_LSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			return 1;
		case 1:
			if (op3==0)
			{
				if (op1==0)
					cpu_regs.tmpL=8;
				else
					cpu_regs.tmpL=op1;
			}
			else 
			{
				cpu_regs.tmpL=cpu_regs.D[op1]&0x3F;
			}
			cpu_regs.SR &= ~(CPU_STATUS_C|CPU_STATUS_V);
			return 2;
		case 2:
			if (cpu_regs.len==4)
				return 3;
			
			stage=3;		// Fall through
		case 3:
			if (cpu_regs.tmpL)
			{
				cpu_regs.eas = cpu_regs.D[op4]&cpu_regs.zMask;
				cpu_regs.ead = (cpu_regs.eas >> 1)&cpu_regs.zMask;
				cpu_regs.D[op4]&=cpu_regs.iMask;
				cpu_regs.D[op4]|=cpu_regs.ead;
				
				if (cpu_regs.eas & 1)
				{
					cpu_regs.SR |= CPU_STATUS_X|CPU_STATUS_C;
				}
				else
				{
					cpu_regs.SR &= ~(CPU_STATUS_X|CPU_STATUS_C);
				}
				cpu_regs.tmpL--;
				return 3;
			}
			break;
	}
	COMPUTE_ZN_TESTS(cpu_regs.D[op4]);
	return 0;
}

u_int32_t CPU_SWAP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(2);
			return 1;
		case 1:
			cpu_regs.tmpL = (cpu_regs.D[op1]&0xFFFF0000)>>16;
			cpu_regs.D[op1]=(cpu_regs.D[op1]&0x0000FFFF)<<16;
			cpu_regs.D[op1]|=cpu_regs.tmpL;
			break;
	}
    cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.D[op1]);
	return 0;
}

u_int32_t CPU_MOVEMs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHLW(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,2,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,2);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
			cpu_regs.ear=0;
			return 17;
		case 17:
			return 18;
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
			while(cpu_regs.eas)				// count leading zeros would be better here
			{
				if (cpu_regs.eas&1)
				{
					ret=LOAD_EFFECTIVE_VALUE(23, 18, stage-18, 0x3C, &cpu_regs.eat, &cpu_regs.ead, cpu_regs.len);
					
					if (ret!=23)
						return ret;
						
					stage=18;
					// load register value
					cpu_regs.ead+=cpu_regs.len;
					if (cpu_regs.len==2)
						cpu_regs.eat=(int16_t)cpu_regs.eat;
					if (cpu_regs.ear<8)
					{
						cpu_regs.D[cpu_regs.ear]=cpu_regs.eat;
					}
					else 
					{
						cpu_regs.A[cpu_regs.ear-8]=cpu_regs.eat;
					}
				}
				cpu_regs.eas>>=1;
				cpu_regs.ear++;
			}
			break;
	}	
	if ((op2 & 0x38) == 0x18)			// handle post increment case
	{
		cpu_regs.A[op2-0x18] = cpu_regs.ead;
	}
	return 0;
}

u_int32_t CPU_MOVEMd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTHLW(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,2,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,2);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			if ((op2 & 0x38) == 0x20)			// handle pre decrement case
			{
				cpu_regs.ear = 15;
				cpu_regs.ead+=cpu_regs.len;
				cpu_regs.A[op2-0x20]+=cpu_regs.len;
			}
			else
			{
				cpu_regs.ear=0;
			}
			stage=16;			// Fall through

		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			while(cpu_regs.eas)				// count leading zeros would be better here
			{
				if (cpu_regs.eas&1)
				{
					if (stage==16 && ((op2 & 0x38) == 0x20))
					{
						cpu_regs.ead-=cpu_regs.len;
					}
					if (stage==16)
					{
						if (cpu_regs.ear<8)
							cpu_regs.eat=cpu_regs.D[cpu_regs.ear]&cpu_regs.zMask;
						else
							cpu_regs.eat=cpu_regs.A[cpu_regs.ear-8]&cpu_regs.zMask;
					}
					
					ret=STORE_EFFECTIVE_VALUE(21, 16, stage-16, 0x3C, &cpu_regs.ead, &cpu_regs.eat);
					
					if (ret!=21)
						return ret;
						
					stage=16;
					// load register value
					if ((op2 & 0x38) != 0x20)
						cpu_regs.ead+=cpu_regs.len;
				}
				cpu_regs.eas>>=1;
				if ((op2 & 0x38) == 0x20)
					cpu_regs.ear--;
				else
					cpu_regs.ear++;
			}
			break;
	}	
	if ((op2 & 0x38) == 0x20)			// handle pre decrement case
		cpu_regs.A[op2-0x20] = cpu_regs.ead;

	return 0;
}

u_int32_t CPU_SUBI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			ret = LOAD_EFFECTIVE_VALUE(21,16,stage-16,op2,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=21)
				return ret;
			
			cpu_regs.ear = (cpu_regs.eat - cpu_regs.eas)&cpu_regs.zMask;

			if ((op2&0x38)==0 && cpu_regs.len==4)
				return 21;							// Long reg direct takes 1 additional cycle on read and write

			stage=21;			// Fall through
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
			ret = STORE_EFFECTIVE_VALUE(26,21,stage-21,op2,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=26)
				return ret;
			
			cpu_regs.ead=cpu_regs.eat;

			if ((op2&0x38)==0 && cpu_regs.len==4)
				return 26;							// Long reg direct takes 1 additional cycle on read and write
			break;
		case 26:
			break;
	}
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_SUB_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	return 0;
}

u_int32_t CPU_BSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
			cpu_regs.ead=cpu_regs.PC;
			return 2;
		case 2:
			cpu_regs.eas=cpu_regs.PC;		// allways reads next word (even if it does not use it!)
			cpu_regs.PC+=2;
			return 3;
		case 3:
			if (!BUS_Available(cpu_regs.eas))
				return 3;
			cpu_regs.eas=MEM_getWord(cpu_regs.eas);
			return 4;
		case 4:
			if (op1!=0)
			{
				cpu_regs.eas=op1;
				if (op1&0x80)
				{
					cpu_regs.eas+=0xFF00;
				}
			}

			cpu_regs.PC=cpu_regs.ead+(int16_t)cpu_regs.eas;
			
			if (op1==0)
			{
				cpu_regs.ead+=2;		// account for extra word
			}

			stage=5;	// Fall through
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			ret=PUSH_VALUE(10,5,stage-5,cpu_regs.ead,4);
			if (ret!=10)
				return ret;
			break;
	}

	return 0;
}

u_int32_t CPU_RTS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			return POP_VALUE(6,1,stage-1,&cpu_regs.ead,4);
		case 6:
			return 7;
		case 7:
			cpu_regs.PC=cpu_regs.ead;
			break;
	}

	return 0;
}

u_int32_t CPU_ILLEGAL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
			return PROCESS_EXCEPTION(1,stage-1,0x10);
	}
	return 0;
}

u_int32_t CPU_ORd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.ead,op3,cpu_regs.len,0);
			
			if (ret!=6)
				return ret;
			
			cpu_regs.eas=cpu_regs.D[op1]&cpu_regs.zMask;
			
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op3,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ear = (cpu_regs.eat | cpu_regs.eas)&cpu_regs.zMask;

			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret = STORE_EFFECTIVE_VALUE(16,11,stage-11,op3,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=16)
				return ret;
			
			cpu_regs.ead=cpu_regs.eat;
			break;
	}
    cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	return 0;
}

u_int32_t CPU_ADDQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op2);
			if (op1==0)
				cpu_regs.operands[0]=8;			// 0 means 8
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.ead,op3,cpu_regs.len,0);
			
			if (ret!=6)
				return ret;
			
			cpu_regs.eas=op1&cpu_regs.zMask;
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op3,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ear = (cpu_regs.eat + cpu_regs.eas)&cpu_regs.zMask;
			stage=11;
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret = STORE_EFFECTIVE_VALUE(16,11,stage-11,op3,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=16)
				return ret;
			
			cpu_regs.ead = cpu_regs.eat;
			stage=16;
		case 16:
			if ((op3 & 0x38)==0x00 && cpu_regs.len==4)	
				return 19;						// Data register direct long costs 2 cycles more
			if ((op3 & 0x38)==0x08)				// Address register direct does not affect flags
				return 17;						// also adds 2 cycles (but not worked out reason why yet)
			break;
		case 17:
			return 18;
		case 18:
			return 0;
		case 19:
			return 20;
		case 20:
			break;
			
	}

	COMPUTE_ZN_TESTS(cpu_regs.ear);
	COMPUTE_ADD_XCV_TESTS(cpu_regs.eas,cpu_regs.ead,cpu_regs.ear);
	
	return 0;
}

u_int32_t CPU_CLR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=6)
				return ret;
			
			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret = LOAD_EFFECTIVE_VALUE(11,6,stage-6,op2,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			cpu_regs.ear = 0;
			stage=11;
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret = STORE_EFFECTIVE_VALUE(16,11,stage-11,op2,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=16)
				return ret;
			
			cpu_regs.ead = cpu_regs.eat;
			stage=16;
		case 16:
			if ((op2 & 0x38)==0x00)
			{
				if (cpu_regs.len==4)
					return 17;						// Data register direct long costs 1 cycles more
			}
			break;
		case 17:
			break;
	}

	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	return 0;
}

u_int32_t CPU_ANDI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			OPCODE_SETUP_LENGTH(op1);
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,cpu_regs.len,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,cpu_regs.len);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op2,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			ret = LOAD_EFFECTIVE_VALUE(21,16,stage-16,op2,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=21)
				return ret;
			
			cpu_regs.ear = (cpu_regs.eat & cpu_regs.eas)&cpu_regs.zMask;

			if ((op2&0x38)==0 && cpu_regs.len==4)
				return 21;							// Long reg direct takes 1 additional cycle on read and write

			stage=21;			// Fall through
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
			ret = STORE_EFFECTIVE_VALUE(26,21,stage-21,op2,&cpu_regs.ead,&cpu_regs.ear);
			if (ret!=26)
				return ret;
			
			cpu_regs.ead=cpu_regs.eat;

			if ((op2&0x38)==0 && cpu_regs.len==4)
				return 26;							// Long reg direct takes 1 additional cycle on read and write
			break;
		case 26:
			break;
	}
	cpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);
	COMPUTE_ZN_TESTS(cpu_regs.ear);
	return 0;
}

u_int32_t CPU_EXG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	switch (stage)
	{
		case 0:
			switch (op2)
			{
				case 0x08:	// Data
				case 0x11:
					cpu_regs.tmpL=cpu_regs.D[op1];
					break;
				case 0x09:	// Address
					cpu_regs.tmpL=cpu_regs.A[op1];
					break;
			}
			return 1;
		case 1:
			switch (op2)
			{
				case 0x08:	// Data & Data
					cpu_regs.D[op1]=cpu_regs.D[op3];
					break;
				case 0x09:	// Address & Address
					cpu_regs.A[op1]=cpu_regs.A[op3];
					break;
				case 0x11:	// Data & Address
					cpu_regs.D[op1]=cpu_regs.A[op3];
					break;
			}
			return 2;
		case 2:
			switch (op2)
			{
				case 0x08:	// Data
					cpu_regs.D[op3]=cpu_regs.tmpL;
					break;
				case 0x09:	// Address
				case 0x11:
					cpu_regs.A[op3]=cpu_regs.tmpL;
					break;
			}
			break;
	}
	
	return 0;
}

u_int32_t CPU_JSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ret;
	
	switch (stage)
	{
		case 0:
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,op1,4,1);
			if (ret!=6)
				return ret;
			stage=6;		// Fall through
		case 6:
		case 7:
		case 8:
			ret=FUDGE_EA_CYCLES(9,6,stage-6,op1);		//ODD cycle timings for JSR -this rebalances
			if (ret!=9)
				return ret;
		
			stage=9;		// Fall through
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			ret=PUSH_VALUE(14,9,stage-9,cpu_regs.PC,4);
			if (ret!=14)
				return ret;
			break;
	}
	
	cpu_regs.PC=cpu_regs.eas;
	return 0;
}

u_int32_t CPU_BCLRI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
/*  I AM HERE - NEED TO TIME INSTRUCTION ON REAL AMIGA FOR CASE OF VARIOUS BIT VALUES FOR REGISTER DIRECT MODE
	u_int32_t ret;
	switch (stage)
	{
		case 0:
			if (op1<0x08)
				OPCODE_SETUP_LENGTH(2);			// Register direct uses long
			else
				OPCODE_SETUP_LENGTH(0);			// memory uses byte
			return 1;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret=COMPUTE_EFFECTIVE_ADDRESS(6,1,stage-1,&cpu_regs.eas,0x3C,1,1);
			if (ret!=6)
				return ret;

			stage=6;			// Fall through
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			ret=LOAD_EFFECTIVE_VALUE(11, 6, stage-6, 0x3C,&cpu_regs.eas, &cpu_regs.eas,1);
			if (ret!=11)
				return ret;
			
			stage=11;			// Fall through
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			ret=COMPUTE_EFFECTIVE_ADDRESS(16,11,stage-11,&cpu_regs.ead,op1,cpu_regs.len,0);
			
			if (ret!=16)
				return ret;
			
			stage=16;			// Fall through
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
			ret = LOAD_EFFECTIVE_VALUE(21,16,stage-16,op1,&cpu_regs.eat,&cpu_regs.ead,cpu_regs.len);
			if (ret!=21)
				return ret;
			
			stage=21;			// Fall through
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
			ret = STORE_EFFECTIVE_VALUE(26,21,stage-21,op1,&cpu_regs.ead,&cpu_regs.eat);
			if (ret!=26)
				return ret;
			
			stage=26;			// Fall through
		case 21:
			if (op1<0x08)
				cpu_regs.eas&=0x1F;
			else
				cpu_regs.eas&=0x07;
			cpu_regs.eas=1<<cpu_regs.eas;
			break;
	}

	COMPUTE_Z_BIT(cpu_regs.eas,cpu_regs.ead);
	return 0;*/
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
		DEB_PauseEmulation("Divide by Zero - Exception not implemented");
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
		DEB_PauseEmulation("Divide by Zero - Exception not implemented");
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

u_int32_t CPU_NEGX(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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
	  if (cpu_regs.SR & CPU_STATUS_X)
		 ear=(ear-1)&zMask;
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
	      if (cpu_regs.SR & CPU_STATUS_X)
				  ear=(ear-1)&zMask;
				MEM_setByte(eas,ear);
				break;
			case 2:
				ead=MEM_getWord(eas);
				ear=(0-ead)&zMask;
	      if (cpu_regs.SR & CPU_STATUS_X)
				  ear=(ear-1)&zMask;
				MEM_setWord(eas,ear);
				break;
			case 4:
				ead=MEM_getLong(eas);
				ear=(0-ead)&zMask;
	      if (cpu_regs.SR & CPU_STATUS_X)
				  ear=(ear-1)&zMask;
				MEM_setLong(eas,ear);
				break;
		}
    }

    if (ear)
		 cpu_regs.SR&=~CPU_STATUS_Z;
	
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
