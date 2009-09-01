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

#include "ciachip.h"
#include "cpu.h"
#include "memory.h"
#include "customchip.h"

static char mnemonicData[256];

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
			sprintf(&mnemonicData[strlen(mnemonicData)],"(%08X).L",MEM_getWord(adr));
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

int decodeLengthCC(u_int16_t op)
{
	if (op==0)
		return 2;
	return 1;
}

int decodeDisp(u_int32_t adr,u_int16_t op)
{
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u_int16_t CPU_DIS_LEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;	
	decodeInstruction("LEA",4);
    length+=decodeEffectiveAddress(adr,op2,4);
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
	
    length+=decodeEffectiveAddress(adr,op3,len);
    strcat(mnemonicData,",");
    length+=decodeEffectiveAddress(adr+length,op2,len);
	
	return length;
}

u_int16_t CPU_DIS_SUBs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLength(op2);
	
	decodeInstruction("SUB",len);
	
    length+=decodeEffectiveAddress(adr,op3,len);
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
    length+=decodeEffectiveAddress(adr,op3,len);
	
	return length;
}

u_int16_t CPU_DIS_BCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int length=0;
    int len=decodeLengthCC(op2);

	decodeInstructionCC("B",op1,len);

	length+=decodeDisp(adr,op2);

	return length;
}

u_int16_t CPU_DIS_CMPA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"CMPA");
    strcpy(byteData,"");
    switch (op2)
    {
		case 0x00:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x01:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    sprintf(tempData,",A%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_CMP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"CMP");
    strcpy(byteData,"");
    switch (op2)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    sprintf(tempData,",D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_CMPI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"CMPI");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	switch (len)
	{
		case 1:
		case 2:
			strcat(mnemonicData,decodeWord(adr));
			strcat(byteData,decodeWord(adr));
			adr+=2;
			break;
		case 4:
			strcat(mnemonicData,decodeLong(adr));
			strcat(byteData,decodeLong(adr));
			adr+=4;
			break;
	}
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"MOVEA");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
		case 0x01:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x03:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	strcat(byteData," ");
	sprintf(tempData,",A%d",op2);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_DBCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"DB");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,"T ");
			break;
		case 0x01:
			strcat(mnemonicData,"F ");
			break;
		case 0x02:
			strcat(mnemonicData,"HI ");
			break;
		case 0x03:
			strcat(mnemonicData,"LS ");
			break;
		case 0x04:
			strcat(mnemonicData,"CC ");
			break;
		case 0x05:
			strcat(mnemonicData,"CS ");
			break;
		case 0x06:
			strcat(mnemonicData,"NE ");
			break;
		case 0x07:
			strcat(mnemonicData,"EQ ");
			break;
		case 0x08:
			strcat(mnemonicData,"VC ");
			break;
		case 0x09:
			strcat(mnemonicData,"VS ");
			break;
		case 0x0A:
			strcat(mnemonicData,"PL ");
			break;
		case 0x0B:
			strcat(mnemonicData,"MI ");
			break;
		case 0x0C:
			strcat(mnemonicData,"GE ");
			break;
		case 0x0D:
			strcat(mnemonicData,"LT ");
			break;
		case 0x0E:
			strcat(mnemonicData,"GT ");
			break;
		case 0x0F:
			strcat(mnemonicData,"LE ");
			break;
    }
	
	sprintf(tempData,"D%d,",op2);
    strcat(mnemonicData,tempData);
	
	op3=MEM_getWord(adr);
	strcat(byteData,decodeWord(adr));
	
	sprintf(tempData,"%08X",adr+(int16_t)op3);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BRA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"BRA ");
    strcpy(byteData,"");
	
    if (op1==0)
	{
		op1=MEM_getWord(adr);
		strcat(byteData,decodeWord(adr));
	}
	else
	{
		if (op1&0x80)
		{
			op1|=0xFF00;
		}
	}
	
	sprintf(tempData,"%08X",adr+(int16_t)op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BTSTI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BTST.L ");
	strcpy(byteData,"");
	len=1;
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ADDs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ADD");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    sprintf(tempData,",D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_NOT(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"NOT");
    strcpy(byteData,"");
    switch (op1)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_SUBA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"SUBA");
    strcpy(byteData,"");
    switch (op2)
    {
		case 0x00:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x01:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	strcat(byteData," ");
	sprintf(tempData,",A%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ADDA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ADDA");
    strcpy(byteData,"");
    switch (op2)
    {
		case 0x00:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x01:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	strcat(byteData," ");
	sprintf(tempData,",A%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_TST(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"TST");
    strcpy(byteData,"");
    switch (op1)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_JMP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"JMP ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,4);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVEQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	adr+=2;
    strcpy(mnemonicData,"MOVEQ ");
    strcpy(byteData,"");

	sprintf(tempData,"#%02X,",op2);
    strcat(mnemonicData,tempData);
	sprintf(tempData,"D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_LSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"LSR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_SWAP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	adr+=2;
    strcpy(mnemonicData,"SWAP ");
    strcpy(byteData,"");

	sprintf(tempData,"D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVEMs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"MOVEM");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x01:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVEMd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"MOVEM");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x01:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_SUBI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"SUBI");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	switch (len)
	{
		case 1:
		case 2:
			strcat(mnemonicData,decodeWord(adr));
			strcat(byteData,decodeWord(adr));
			adr+=2;
			break;
		case 4:
			strcat(mnemonicData,decodeLong(adr));
			strcat(byteData,decodeLong(adr));
			adr+=4;
			break;
	}
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"BSR ");
    strcpy(byteData,"");
	
    if (op1==0)
	{
		op1=MEM_getWord(adr);
		strcat(byteData,decodeWord(adr));
	}
	else
	{
		if (op1&0x80)
		{
			op1|=0xFF00;
		}
	}
	
	sprintf(tempData,"%08X",adr+(int16_t)op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_RTS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"RTS");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ILLEGAL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"ILLEGAL");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ORd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"OR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	sprintf(tempData,"D%d,",op1);
    strcat(mnemonicData,tempData);

    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ADDQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ADDQ");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    if (op1==0)
		op1=8;
    sprintf(tempData,"#%02X,",op1);
    strcat(mnemonicData,tempData);
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_CLR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"CLR");
    strcpy(byteData,"");
    switch (op1)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ANDI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ANDI");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	switch (len)
	{
		case 1:
		case 2:
			strcat(mnemonicData,decodeWord(adr));
			strcat(byteData,decodeWord(adr));
			adr+=2;
			break;
		case 4:
			strcat(mnemonicData,decodeLong(adr));
			strcat(byteData,decodeLong(adr));
			adr+=4;
			break;
	}
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_EXG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	adr+=2;
    strcpy(mnemonicData,"EXG ");
    strcpy(byteData,"");

	switch(op2)
	{
		default:
			strcat(mnemonicData,"?,?");
			break;
		case 0x08:					// Data & Data
			sprintf(tempData,"D%d,",op1);
			strcat(mnemonicData,tempData);
			sprintf(tempData,"D%d",op3);
			strcat(mnemonicData,tempData);
			break;
		case 0x09:					// Address & Address
			sprintf(tempData,"A%d,",op1);
			strcat(mnemonicData,tempData);
			sprintf(tempData,"A%d",op3);
			strcat(mnemonicData,tempData);
			break;
		case 0x11:					// Data & Address
			sprintf(tempData,"D%d,",op1);
			strcat(mnemonicData,tempData);
			sprintf(tempData,"A%d",op3);
			strcat(mnemonicData,tempData);
			break;
	}
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_JSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"JSR ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,4);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BCLRI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BCLR.L ");
	strcpy(byteData,"");
	len=1;
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ANDs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"AND");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    sprintf(tempData,",D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_SUBd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"SUB");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	sprintf(tempData,"D%d,",op1);
    strcat(mnemonicData,tempData);

    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BSET(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BSET.");
	strcpy(byteData,"");
	len=1;
	if (op2<8)
		strcat(mnemonicData,"L ");
	else
		strcat(mnemonicData,"B ");
	
	sprintf(tempData,"D%d",op1);
	strcat(mnemonicData,tempData);
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BSETI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BSET.L ");
	strcpy(byteData,"");
	len=1;
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MULU(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"MULU.W ");
	strcpy(byteData,"");
	len=2;
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
	sprintf(tempData,",D%d",op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_LSL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"LSL");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ADDI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ADDI");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	switch (len)
	{
		case 1:
		case 2:
			strcat(mnemonicData,decodeWord(adr));
			strcat(byteData,decodeWord(adr));
			adr+=2;
			break;
		case 4:
			strcat(mnemonicData,decodeLong(adr));
			strcat(byteData,decodeLong(adr));
			adr+=4;
			break;
	}
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_EXT(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"EXT");
    strcpy(byteData,"");
    switch (op1)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x02:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x03:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	sprintf(tempData,"D%d",op2);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MULS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"MULS.W ");
	strcpy(byteData,"");
	len=2;
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
	sprintf(tempData,",D%d",op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_NEG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"NEG");
    strcpy(byteData,"");
    switch (op1)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVEUSP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"MOVEUSP ");
	strcpy(byteData,"");
	len=1;
	
	if (op1)
	{
		strcat(mnemonicData,"USP,");
		sprintf(tempData,"A%d",op2);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"A%d",op2);
		strcat(mnemonicData,tempData);
		strcat(mnemonicData,",USP");
	}
	
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_SCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"S");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,"T.B  ");
			break;
		case 0x01:
			strcat(mnemonicData,"F.B  ");
			break;
		case 0x02:
			strcat(mnemonicData,"HI.B ");
			break;
		case 0x03:
			strcat(mnemonicData,"LS.B ");
			break;
		case 0x04:
			strcat(mnemonicData,"CC.B ");
			break;
		case 0x05:
			strcat(mnemonicData,"CS.B ");
			break;
		case 0x06:
			strcat(mnemonicData,"NE.B ");
			break;
		case 0x07:
			strcat(mnemonicData,"EQ.B ");
			break;
		case 0x08:
			strcat(mnemonicData,"VC.B ");
			break;
		case 0x09:
			strcat(mnemonicData,"VS.B ");
			break;
		case 0x0A:
			strcat(mnemonicData,"PL.B ");
			break;
		case 0x0B:
			strcat(mnemonicData,"MI.B ");
			break;
		case 0x0C:
			strcat(mnemonicData,"GE.B ");
			break;
		case 0x0D:
			strcat(mnemonicData,"LT.B ");
			break;
		case 0x0E:
			strcat(mnemonicData,"GT.B ");
			break;
		case 0x0F:
			strcat(mnemonicData,"LE.B ");
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,1);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ORSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"ORSR.W ");
    strcpy(byteData,"");
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_PEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"PEA.L ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,4);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVEFROMSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"MOVE.W SR,");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,2);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_RTE(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"RTE");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ANDSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"ANDSR.W ");
    strcpy(byteData,"");
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVETOSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"MOVE.W ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,2);
	strcat(mnemonicData,",SR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_LINK(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"LINK.W ");
	strcpy(byteData,"");
	len=2;
	
	sprintf(tempData,"A%d,",op1);
	strcat(mnemonicData,tempData);
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_CMPM(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"CMPM");
    strcpy(byteData,"");
    switch (op2)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			break;
    }
	
    sprintf(tempData,"(A%d)+,",op3);
    strcat(mnemonicData,tempData);

    sprintf(tempData,"(A%d)+",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ADDd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ADD");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	sprintf(tempData,"D%d,",op1);
    strcat(mnemonicData,tempData);

    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_UNLK(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"UNLK.L ");
	strcpy(byteData,"");
	len=2;
	
	sprintf(tempData,"A%d",op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ORs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"OR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    sprintf(tempData,",D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ANDd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"AND");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	sprintf(tempData,"D%d,",op1);
    strcat(mnemonicData,tempData);

    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ORI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ORI");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	switch (len)
	{
		case 1:
		case 2:
			strcat(mnemonicData,decodeWord(adr));
			strcat(byteData,decodeWord(adr));
			adr+=2;
			break;
		case 4:
			strcat(mnemonicData,decodeLong(adr));
			strcat(byteData,decodeLong(adr));
			adr+=4;
			break;
	}
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ASL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ASL");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ASR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ASR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_DIVU(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"DIVU.W ");
	strcpy(byteData,"");
	len=2;
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
	sprintf(tempData,",D%d",op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BCLR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BCLR.");
	strcpy(byteData,"");
	len=1;
	if (op2<8)
		strcat(mnemonicData,"L ");
	else
		strcat(mnemonicData,"B ");
	
	sprintf(tempData,"D%d",op1);
	strcat(mnemonicData,tempData);
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_EORd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"EOR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    sprintf(tempData,"D%d,",op1);
    strcat(mnemonicData,tempData);
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BTST(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BTST.L ");
	strcpy(byteData,"");
	len=1;
	
    sprintf(tempData,"D%d,",op1);
    strcat(mnemonicData,tempData);
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_STOP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"STOP ");
    strcpy(byteData,"");
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ROL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ROL");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ROR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ROR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_NOP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	adr+=2;
    strcpy(mnemonicData,"NOP");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BCHG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BCHG.");
	strcpy(byteData,"");
	len=1;
	if (op2<8)
		strcat(mnemonicData,"L ");
	else
		strcat(mnemonicData,"B ");
	
	sprintf(tempData,"D%d",op1);
	strcat(mnemonicData,tempData);
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_BCHGI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"BCHG.L ");
	strcpy(byteData,"");
	len=1;
	
	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_LSRm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"LSR.W");
    strcpy(byteData,"");
	len=2;

    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_EORI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"EORI");
    strcpy(byteData,"");
    switch (op1)
    {
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
	strcat(mnemonicData,"#");
	switch (len)
	{
		case 1:
		case 2:
			strcat(mnemonicData,decodeWord(adr));
			strcat(byteData,decodeWord(adr));
			adr+=2;
			break;
		case 4:
			strcat(mnemonicData,decodeLong(adr));
			strcat(byteData,decodeLong(adr));
			adr+=4;
			break;
	}
	
	strcat(mnemonicData,",");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_EORICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"EORI");
    strcpy(byteData,"");

	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",CCR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ROXL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ROXL");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ROXR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ROXR");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }

	if (op3==0)
	{
		if (op1==0)
			op1=8;
		sprintf(tempData,"#%02X,",op1);
		strcat(mnemonicData,tempData);
	}
	else
	{
		sprintf(tempData,"D%d,",op1);
		strcat(mnemonicData,tempData);
	}
	
	sprintf(tempData,"D%d",op4);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_MOVETOCCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"MOVE.W ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,2);
	strcat(mnemonicData,",CCR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_TRAP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"TRAP ");
    strcpy(byteData,"");
	
	sprintf(tempData,"%02X",op1);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ADDX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"ADDX");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    sprintf(tempData,"D%d,D%d",op3,op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_DIVS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
    adr+=2;
    strcpy(mnemonicData,"DIVS.W ");
	strcpy(byteData,"");
	len=2;
	
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,len);
	
	sprintf(tempData,",D%d",op1);
	strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_SUBX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;

    adr+=2;
    strcpy(mnemonicData,"SUBX");
    strcpy(byteData,"");
    switch (op2)
    {
		default:
			strcat(mnemonicData,".? ");
			len=0;
			break;
		case 0x00:
			strcat(mnemonicData,".B ");
			len=1;
			break;
		case 0x01:
			strcat(mnemonicData,".W ");
			len=2;
			break;
		case 0x02:
			strcat(mnemonicData,".L ");
			len=4;
			break;
    }
	
    sprintf(tempData,"D%d,D%d",op3,op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ASRm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ASR");
    strcpy(byteData,"");
	strcat(mnemonicData,".W ");
	len=2;

    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ASLm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"ASL");
    strcpy(byteData,"");
	strcat(mnemonicData,".W ");
	len=2;

    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);

    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ANDICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"ANDI");
    strcpy(byteData,"");

	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",CCR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

u_int16_t CPU_DIS_ORICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"ORI");
    strcpy(byteData,"");

	strcat(mnemonicData,"#");
	strcat(mnemonicData,decodeWord(adr));
	strcat(byteData,decodeWord(adr));
	adr+=2;
	
	strcat(mnemonicData,",CCR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}
