/*
 *  cpu_dis.c
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

#include "cpu_dis.h"
#include "memory.h"

char mnemonicData[256];

int decodeEffectiveAddress(u_int32_t adr, u_int16_t operand,int length)
{
	u_int16_t tmp;
	
    switch (operand)
    {
		case 0x00:	/// 000rrr
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",operand);
			return 0;
		case 0x08:	/// 001rrr
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			sprintf(&mnemonicData[strlen(mnemonicData)],"A%d",operand-0x08);
			return 0;
		case 0x10:	/// 010rrr
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			sprintf(&mnemonicData[strlen(mnemonicData)],"(A%d)",operand-0x10);
			return 0;
		case 0x18:	/// 011rrr
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			sprintf(&mnemonicData[strlen(mnemonicData)],"(A%d)+",operand-0x18);
			return 0;
		case 0x20:	// 100rrr
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			sprintf(&mnemonicData[strlen(mnemonicData)],"-(A%d)",operand-0x20);
			return 0;
		case 0x28:	// 101rrr
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			sprintf(&mnemonicData[strlen(mnemonicData)],"(#%04X,A%d)",MEM_getWord(adr),operand-0x28);
			return 2;
		case 0x30:	// 110rrr
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			tmp = MEM_getWord(adr);
			if (tmp&0x8000)
			{
				sprintf(&mnemonicData[strlen(mnemonicData)],"(#%02X,A%d%s,A%d)",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? ".L" : ".W", operand-0x30);
			}
			else
			{
				sprintf(&mnemonicData[strlen(mnemonicData)],"(#%02X,D%d%s,A%d)",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? ".L" : ".W", operand-0x30);
			}
			return 2;
		case 0x38:		/// 111000
			sprintf(&mnemonicData[strlen(mnemonicData)],"(%04X).W",MEM_getWord(adr));
			return 2;
		case 0x39:		/// 111001
			sprintf(&mnemonicData[strlen(mnemonicData)],"(%08X).L",MEM_getLong(adr));
			return 4;
		case 0x3A:		/// 111010
			sprintf(&mnemonicData[strlen(mnemonicData)],"(#%04X,PC)",MEM_getWord(adr));
			return 2;
		case 0x3B:		/// 111100
			tmp = MEM_getWord(adr);
			if (tmp&0x8000)
			{
				sprintf(&mnemonicData[strlen(mnemonicData)],"(#%02X,A%d%s,PC)",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? ".L" : ".W");
			}
			else
			{
				sprintf(&mnemonicData[strlen(mnemonicData)],"(#%02X,D%d%s,PC)",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? ".L" : ".W");
			}
			return 2;
		case 0x3C:		/// 111100
			switch (length)
			{
				case 1:
					sprintf(&mnemonicData[strlen(mnemonicData)],"#%02X",MEM_getByte(adr+1));
					return 2;
				case 2:
					sprintf(&mnemonicData[strlen(mnemonicData)],"#%04X",MEM_getWord(adr));
					return 2;
				case 4:
					sprintf(&mnemonicData[strlen(mnemonicData)],"#%08X",MEM_getLong(adr));
					return 4;
			}
		default:
			strcat(mnemonicData,"UNKNOWN");
			break;
    }
    return 0;
}

int decodeRegsDst(u_int32_t adr)
{
	int a;
	u_int16_t tmp=MEM_getWord(adr);
	int doneOne=0;

	for (a=0;a<16;a++)
	{
		if (tmp&1)
		{
			if (a<8)
			{
				if (doneOne)
					strcat(mnemonicData,"/");
				sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",a);
				doneOne=1;
			}
			else 
			{
				if (doneOne)
					strcat(mnemonicData,"/");
				sprintf(&mnemonicData[strlen(mnemonicData)],"A%d",a-8);
				doneOne=1;
			}
		}
		tmp>>=1;
	}

	return 2;
}

int decodeRegsSrc(u_int32_t adr,u_int16_t op)
{
	int a;
	u_int16_t tmp=MEM_getWord(adr);
	int doneOne=0;

	if ((op & 0x38)==0x20)
	{
		for (a=15;a>=0;a--)
		{
			if (tmp&1)
			{
				if (a<8)
				{
					if (doneOne)
						strcat(mnemonicData,"/");
					sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",a);
					doneOne=1;
				}
				else 
				{
					if (doneOne)
						strcat(mnemonicData,"/");
					sprintf(&mnemonicData[strlen(mnemonicData)],"A%d",a-8);
					doneOne=1;
				}
			}
			tmp>>=1;
		}
	}
	else 
	{
		for (a=0;a<16;a++)
		{
			if (tmp&1)
			{
				if (a<8)
				{
					if (doneOne)
						strcat(mnemonicData,"/");
					sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",a);
					doneOne=1;
				}
				else 
				{
					if (doneOne)
						strcat(mnemonicData,"/");
					sprintf(&mnemonicData[strlen(mnemonicData)],"A%d",a-8);
					doneOne=1;
				}
			}
			tmp>>=1;
		}
	}
		
	return 2;
}

void decodeInstruction(char *name,int len)
{
	strcpy(mnemonicData,name);
	switch (len)
	{
		default:
			strcat(mnemonicData,".? ");
			break;
		case 1:
			strcat(mnemonicData,".B ");
			break;
		case 2:
			strcat(mnemonicData,".W ");
			break;
		case 4:
			strcat(mnemonicData,".L ");
			break;
	}
}

void decodeInstructionCC(char *baseName,u_int16_t op,int len)
{
	static char name[16];

	strcpy(name,baseName);
    switch (op)
    {
		case 0x00:
			strcat(name,"T");
			break;
		case 0x01:
			strcat(name,"F");
			break;
		case 0x02:
			strcat(name,"HI");
			break;
		case 0x03:
			strcat(name,"LS");
			break;
		case 0x04:
			strcat(name,"CC");
			break;
		case 0x05:
			strcat(name,"CS");
			break;
		case 0x06:
			strcat(name,"NE");
			break;
		case 0x07:
			strcat(name,"EQ");
			break;
		case 0x08:
			strcat(name,"VC");
			break;
		case 0x09:
			strcat(name,"VS");
			break;
		case 0x0A:
			strcat(name,"PL");
			break;
		case 0x0B:
			strcat(name,"MI");
			break;
		case 0x0C:
			strcat(name,"GE");
			break;
		case 0x0D:
			strcat(name,"LT");
			break;
		case 0x0E:
			strcat(name,"GT");
			break;
		case 0x0F:
			strcat(name,"LE");
			break;
    }

	decodeInstruction(name,len);
}

int decodeLengthM(u_int16_t op)
{
    switch (op)
    {
		case 0x01:
			return 1;
		case 0x03:
			return 2;
		case 0x02:
			return 4;
	}
	
	return 0;
}

int decodeLength(u_int16_t op)
{
    switch (op)
    {
		case 0x00:
			return 1;
		case 0x01:
			return 2;
		case 0x02:
			return 4;
	}
	return 0;
}

int decodeLengthWL(u_int16_t op)
{
	switch (op)
	{
		case 0:
			return 2;
		case 1:
			return 4;
	}
	return 0;
}

int decodeLengthCC(u_int16_t op)
{
	if (op==0)
		return 2;
	return 1;
}

int decodeDisp(u_int32_t adr,u_int16_t op)
{
	int length=0;
	
    if (op==0)
	{
		op=MEM_getWord(adr);
		length+=2;
	}
	else
	{
		if (op&0x80)
		{
			op|=0xFF00;
		}
	}
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"%08X",adr+(int16_t)op);
	
	return length;
}

void decodeIR(u_int16_t ir,u_int16_t i)
{
	if (ir==0)
	{
		if (i==0)
			i=8;
		sprintf(&mnemonicData[strlen(mnemonicData)],"#%02X",i);
	}
	else
	{
		sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",i);
	}
}

int decodeLengthReg(u_int16_t op)
{
	if (op<0x10)
		return 4;
	return 1;
}

int decodeLengthEXT(u_int16_t op)
{
    switch (op)
    {
		case 0x02:
			return 2;
		case 0x03:
			return 4;
    }
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u_int16_t CPU_DIS_LEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;	
	decodeInstruction("LEA",4);
    length+=decodeEffectiveAddress(adr+length,op2,4);
    sprintf(&mnemonicData[strlen(mnemonicData)],",A%d",op1);
	return length;
}

u_int16_t CPU_DIS_MOVE(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthM(op1);
	u_int16_t t1,t2;

	decodeInstruction("MOVE",len);
	
	t1 = (op2 & 0x38)>>3;
	t2 = (op2 & 0x07)<<3;
	op2 = t1|t2;
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_SUBs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("SUB",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_SUBQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("SUBQ",len);
	
    if (op1==0)
		op1=8;
    sprintf(&mnemonicData[strlen(mnemonicData)],"#%02X,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
	return length;
}

u_int16_t CPU_DIS_BCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthCC(op2);

	decodeInstructionCC("B",op1,len);

	length+=decodeDisp(adr+length,op2);

	return length;
}

u_int16_t CPU_DIS_CMPA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthWL(op2);
	
	decodeInstruction("CMPA",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",A%d",op1);

	return length;
}

u_int16_t CPU_DIS_CMP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("CMP",len);

    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_CMPI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("CMPI",len);
	
	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
    return length;
}

u_int16_t CPU_DIS_MOVEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthM(op1);
	
	decodeInstruction("MOVEA",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",A%d",op2);

	return length;
}

u_int16_t CPU_DIS_DBCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstructionCC("DB",op1,len);

    sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op2);
	length+=decodeDisp(adr+length,0);

	return length;
}

u_int16_t CPU_DIS_BRA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLengthCC(op1);
	
	decodeInstruction("BRA",len);
	
	length+=decodeDisp(adr+length,op1);
	
	return length;
}

u_int16_t CPU_DIS_BTSTI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op1);
	
	decodeInstruction("BTST",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,1);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op1,len);

	return length;
}

u_int16_t CPU_DIS_ADDs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("ADD",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_NOT(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("NOT",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
    return length;
}

u_int16_t CPU_DIS_SUBA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthWL(op2);
	
	decodeInstruction("SUBA",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",A%d",op1);

	return length;
}

u_int16_t CPU_DIS_ADDA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthWL(op2);
	
	decodeInstruction("ADDA",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
    sprintf(&mnemonicData[strlen(mnemonicData)],",A%d",op1);

	return length;
}

u_int16_t CPU_DIS_TST(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("TST",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);

	return length;
}

u_int16_t CPU_DIS_JMP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=4;
	
	decodeInstruction("JMP",len);
	
    length+=decodeEffectiveAddress(adr+length,op1,4);
	
	return length;
}

u_int16_t CPU_DIS_MOVEQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=4;
	
	decodeInstruction("MOVEQ",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"#%02X,D%d",op2,op1);

	return length;
}

u_int16_t CPU_DIS_LSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("LSR",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_SWAP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("SWAP",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",op1);

	return length;
}

u_int16_t CPU_DIS_MOVEMs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthWL(op1);
	
	decodeInstruction("MOVEM",len);

	length+=decodeRegsDst(adr+length);			// note displayed in wrong order at present
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_MOVEMd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthWL(op1);
	
	decodeInstruction("MOVEM",len);
	
	// ToDo. Actually decode register mappings and put operands in right order
	length+=decodeRegsSrc(adr+length,op2);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_SUBI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("SUBI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_BSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLengthCC(op1);
	
	decodeInstruction("BSR",len);
	
	length+=decodeDisp(adr+length,op1);
	
	return length;
}

u_int16_t CPU_DIS_RTS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	
    strcpy(mnemonicData,"RTS");

	return length;
}

u_int16_t CPU_DIS_ILLEGAL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	
    strcpy(mnemonicData,"ILLEGAL");
	
	return length;
}

u_int16_t CPU_DIS_ORd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("OR",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
	return length;
}

u_int16_t CPU_DIS_ADDQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("ADDQ",len);
	
    if (op1==0)
		op1=8;
    sprintf(&mnemonicData[strlen(mnemonicData)],"#%02X,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
	return length;	
}

u_int16_t CPU_DIS_CLR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);

	decodeInstruction("CLR",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_ANDI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("ANDI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_EXG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=4;
	
	decodeInstruction("EXG",len);

	switch(op2)
	{
		default:
			strcat(mnemonicData,"?,?");
			break;
		case 0x08:					// Data & Data
			sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,D%d",op1,op3);
			break;
		case 0x09:					// Address & Address
			sprintf(&mnemonicData[strlen(mnemonicData)],"A%d,A%d",op1,op3);
			break;
		case 0x11:					// Data & Address
			sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,A%d",op1,op3);
			break;
	}

	return length;
}

u_int16_t CPU_DIS_JSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=4;
	
	decodeInstruction("JSR",len);
	
    length+=decodeEffectiveAddress(adr+length,op1,len);
	
    return length;
}

u_int16_t CPU_DIS_BCLRI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op1);
	
	decodeInstruction("BCLR",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,1);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op1,len);

	return length;
}

u_int16_t CPU_DIS_ANDs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("AND",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_SUBd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("SUB",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
    return length;
}

u_int16_t CPU_DIS_BSET(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op2);
	
	decodeInstruction("BSET",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op2,len);

	return length;
}

u_int16_t CPU_DIS_BSETI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op1);
	
	decodeInstruction("BSET",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,1);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op1,len);

	return length;
}

u_int16_t CPU_DIS_MULU(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=2;
	
	decodeInstruction("MULU",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_LSL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("LSL",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_ADDI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("ADDI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_EXT(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLengthEXT(op1);

	decodeInstruction("EXT",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d",op2);

	return length;	
}

u_int16_t CPU_DIS_MULS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=2;
	
	decodeInstruction("MULS",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_NEG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("NEG",len);
	
    length+=decodeEffectiveAddress(adr,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_MOVEUSP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=4;
	
	decodeInstruction("MOVE",len);
	
	if (op1)
	{
		sprintf(&mnemonicData[strlen(mnemonicData)],"USP,A%d",op2);
	}
	else
	{
		sprintf(&mnemonicData[strlen(mnemonicData)],"A%d,USP",op2);
	}

	return length;
}

u_int16_t CPU_DIS_SCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=1;

	decodeInstructionCC("S",op1,len);

    length+=decodeEffectiveAddress(adr+length,op2,len);

	return length;
}

u_int16_t CPU_DIS_ORSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("OR",len);
	
	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",SR");

	return length;
}

u_int16_t CPU_DIS_PEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=4;
	
	decodeInstruction("PEA",len);
	
    length+=decodeEffectiveAddress(adr+length,op1,len);

	return length;
}

u_int16_t CPU_DIS_MOVEFROMSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("MOVE",len);
    strcat(mnemonicData,"SR,");
    length+=decodeEffectiveAddress(adr,op1,len);
	
	return length;
}

u_int16_t CPU_DIS_RTE(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	
    strcpy(mnemonicData,"RTE");

	return length;
}

u_int16_t CPU_DIS_ANDSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("AND",len);
	
	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",SR");

	return length;
}

u_int16_t CPU_DIS_MOVETOSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("MOVE",len);
    length+=decodeEffectiveAddress(adr,op1,len);
    strcat(mnemonicData,",SR");
	
	return length;
}

u_int16_t CPU_DIS_LINK(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=2;
	
	decodeInstruction("LINK",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"A%d,",op1);
	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	
	return length;
}

u_int16_t CPU_DIS_CMPM(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);
	
	decodeInstruction("CMPM",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"(A%d)+,(A%d)+",op3,op1);

	return length;
}

u_int16_t CPU_DIS_ADDd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("ADD",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
	return length;
}

u_int16_t CPU_DIS_UNLK(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=2;
	
	decodeInstruction("UNLK",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"A%d",op1);

	return length;
}

u_int16_t CPU_DIS_ORs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("OR",len);
	
    length+=decodeEffectiveAddress(adr+length,op3,len);
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_ANDd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);

	decodeInstruction("AND",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
	return length;
}

u_int16_t CPU_DIS_ORI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("ORI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_ASL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("ASL",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_ASR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("ASR",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_DIVU(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=2;
	
	decodeInstruction("DIVU",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_BCLR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op2);
	
	decodeInstruction("BCLR",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op2,len);

	return length;
}

u_int16_t CPU_DIS_EORd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);

	decodeInstruction("EOR",len);
	
	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op3,len);
	
	return length;
}

u_int16_t CPU_DIS_BTST(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op2);
	
	decodeInstruction("BTST",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op2,len);

	return length;
}

u_int16_t CPU_DIS_STOP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("STOP",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);

	return length;
}

u_int16_t CPU_DIS_ROL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("ROL",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_ROR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("ROR",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_NOP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	
    strcpy(mnemonicData,"NOP");

	return length;
}

u_int16_t CPU_DIS_BCHG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op2);
	
	decodeInstruction("BCHG",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,",op1);
    length+=decodeEffectiveAddress(adr+length,op2,len);

	return length;
}

u_int16_t CPU_DIS_BCHGI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthReg(op1);
	
	decodeInstruction("BCHG",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,1);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op1,len);

	return length;
}

u_int16_t CPU_DIS_LSRm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;

	decodeInstruction("LSR",len);

    length+=decodeEffectiveAddress(adr,op1,len);

	return length;
}

u_int16_t CPU_DIS_EORI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("EORI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_EORICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=1;
	
	decodeInstruction("EORI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",CCR");

	return length;
}

u_int16_t CPU_DIS_ROXL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("ROXL",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_ROXR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=decodeLength(op2);

	decodeInstruction("ROXR",len);

	decodeIR(op3,op1);	
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op4);
	
	return length;
}

u_int16_t CPU_DIS_MOVETOCCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;
	
	decodeInstruction("MOVE",len);
	
    length+=decodeEffectiveAddress(adr+length,op1,len);
	strcat(mnemonicData,",CCR");

	return length;
}

u_int16_t CPU_DIS_TRAP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=4;
	
	decodeInstruction("TRAP",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"%02X",op1);

	return length;
}

u_int16_t CPU_DIS_ADDX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("ADDX",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,D%d",op3,op1);

	return length;
}

u_int16_t CPU_DIS_DIVS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=2;
	
	decodeInstruction("DIVS",len);
	
    length+=decodeEffectiveAddress(adr+length,op2,len);
	sprintf(&mnemonicData[strlen(mnemonicData)],",D%d",op1);

	return length;
}

u_int16_t CPU_DIS_SUBX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("SUBX",len);

	sprintf(&mnemonicData[strlen(mnemonicData)],"D%d,D%d",op3,op1);

	return length;
}

u_int16_t CPU_DIS_ASRm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;

	decodeInstruction("ASR",len);

    length+=decodeEffectiveAddress(adr,op1,len);

	return length;
}

u_int16_t CPU_DIS_ASLm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=2;

	decodeInstruction("ASL",len);

    length+=decodeEffectiveAddress(adr,op1,len);

	return length;
}

u_int16_t CPU_DIS_ANDICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=1;
	
	decodeInstruction("ANDI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",CCR");

	return length;
}

u_int16_t CPU_DIS_ORICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
	int len=1;
	
	decodeInstruction("ORI",len);

	length+=decodeEffectiveAddress(adr+length,0x3C,len);
	strcat(mnemonicData,",CCR");

	return length;
}

u_int16_t CPU_DIS_NEGX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op1);
	
	decodeInstruction("NEGX",len);
	
    length+=decodeEffectiveAddress(adr,op2,len);
	
	return length;
}
