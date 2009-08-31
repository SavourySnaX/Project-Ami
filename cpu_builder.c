/*
 *  cpu_builder.c
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

#if 0

CPU_Regs cpu_regs;

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

static char byteData[256];
static char mnemonicData[256];
static char tempData[256];

int startDebug=0;
int cpuStopped=0;

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

/////////////////////////////////////////////////

void CPU_DIS_SUBs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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
	
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    sprintf(tempData,",D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_CMPA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_CMP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_CMPI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MOVEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_DBCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BRA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BTSTI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ADDs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_NOT(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_SUBA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ADDA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_TST(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_JMP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"JMP ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,4);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_MOVEQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_LSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_SWAP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	adr+=2;
    strcpy(mnemonicData,"SWAP ");
    strcpy(byteData,"");

	sprintf(tempData,"D%d",op1);
    strcat(mnemonicData,tempData);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_MOVEMs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MOVEMd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_SUBI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_RTS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"RTS");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_ILLEGAL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"ILLEGAL");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_ORd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ADDQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_CLR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ANDI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_EXG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_JSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"JSR ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,4);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_BCLRI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ANDs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_SUBd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BSET(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BSETI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MULU(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_LSL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ADDI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_EXT(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MULS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_NEG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MOVEUSP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_SCC(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ORSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_PEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"PEA.L ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,4);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_MOVEFROMSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"MOVE.W SR,");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,2);
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_RTE(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"RTE");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_ANDSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MOVETOSR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"MOVE.W ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,2);
	strcat(mnemonicData,",SR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_LINK(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_CMPM(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ADDd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_UNLK(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ORs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ANDd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ORI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ASL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ASR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_DIVU(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BCLR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_EORd(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BTST(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_STOP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ROL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ROR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_NOP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	adr+=2;
    strcpy(mnemonicData,"NOP");
    strcpy(byteData,"");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_BCHG(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_BCHGI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_LSRm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;

	adr+=2;
    strcpy(mnemonicData,"LSR.W");
    strcpy(byteData,"");
	len=2;

    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,len);

    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_EORI(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_EORICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ROXL(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ROXR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_MOVETOCCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"MOVE.W ");
    strcpy(byteData,"");
	
    adr+=decodeEffectiveAddress(adr,op1,mnemonicData,byteData,2);
	strcat(mnemonicData,",CCR");
	
    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_TRAP(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"TRAP ");
    strcpy(byteData,"");
	
	sprintf(tempData,"%02X",op1);
	strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_ADDX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_DIVS(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_SUBX(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ASRm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ASLm(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ANDICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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

void CPU_DIS_ORICCR(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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


///////////////////////////////////////////////////////////////////////////////

void CPU_SUBs(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_CMPA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_CMP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_CMPI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_MOVEA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t eas;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_DBCC(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int cc=0;
	u_int32_t ead;
	
	cpu_regs.PC+=2;
	
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
}

void CPU_BRA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.PC+=2;
	
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
}

void CPU_BTSTI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ADDs(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_NOT(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_SUBA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ADDA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_TST(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_JMP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ear;
	
    cpu_regs.PC+=2;
	
	ear=getEffectiveAddress(op1,4);

	cpu_regs.PC=ear;
}

void CPU_MOVEQ(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    cpu_regs.PC+=2;

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
}

void CPU_LSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_SWAP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t temp;
	
    cpu_regs.PC+=2;

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
}

void CPU_MOVEMs(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_MOVEMd(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_SUBI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_BSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t nextInstruction;
	
	cpu_regs.PC+=2;
	
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
}

void CPU_RTS(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.PC = MEM_getLong(cpu_regs.A[7]);
	cpu_regs.A[7]+=4;
}

void CPU_ILLEGAL(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.PC+=2;
	
	CPU_GENERATE_EXCEPTION(0x10);
}

void CPU_ORd(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ADDQ(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    cpu_regs.PC+=2;
	
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
		return;
	
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
}

void CPU_CLR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ANDI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_EXG(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t temp;
	
    cpu_regs.PC+=2;

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
}

void CPU_JSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ear;
	
    cpu_regs.PC+=2;
	
	ear=getEffectiveAddress(op1,4);

	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],cpu_regs.PC);

	cpu_regs.PC=ear;
}

void CPU_BCLRI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ANDs(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_SUBd(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_BSET(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_BSETI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_MULU(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_LSL(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ADDI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_EXT(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_MULS(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_NEG(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_MOVEUSP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	if (cpu_regs.SR & CPU_STATUS_S)
	{
		cpu_regs.PC+=2;
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
		CPU_GENERATE_EXCEPTION(0x20);
	}
}

void CPU_SCC(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int cc=0;
	u_int32_t eas,ead;
	u_int8_t value;
	
	cpu_regs.PC+=2;
	
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
}

void CPU_ORSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;
	
	if (cpu_regs.SR & CPU_STATUS_S)
	{
		cpu_regs.PC+=2;
		oldSR = cpu_regs.SR;
		cpu_regs.SR|=MEM_getWord(cpu_regs.PC);
		CPU_CHECK_SP(oldSR,cpu_regs.SR);
		cpu_regs.PC+=2;
	}
	else
	{
		CPU_GENERATE_EXCEPTION(0x20);
	}
}

void CPU_PEA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ear;
	
    cpu_regs.PC+=2;
	
	ear=getEffectiveAddress(op1,4);

	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],ear);
}

// NB: This is oddly not a supervisor instruction on MC68000
void CPU_MOVEFROMSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ear;

	cpu_regs.PC+=2;
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
}

void CPU_RTE(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
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
		CPU_GENERATE_EXCEPTION(0x20);
	}
}

void CPU_ANDSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;
	
	if (cpu_regs.SR & CPU_STATUS_S)
	{
		cpu_regs.PC+=2;
		oldSR=cpu_regs.SR;
		cpu_regs.SR&=MEM_getWord(cpu_regs.PC);
		CPU_CHECK_SP(oldSR,cpu_regs.SR);
		cpu_regs.PC+=2;
	}
	else
	{
		CPU_GENERATE_EXCEPTION(0x20);
	}
}

void CPU_MOVETOSR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;
	u_int32_t ear;

	if (cpu_regs.SR & CPU_STATUS_S)
	{
		cpu_regs.PC+=2;
		
		ear=getSourceEffectiveAddress(op1,2);
		oldSR=cpu_regs.SR;
		cpu_regs.SR=ear;
		CPU_CHECK_SP(oldSR,cpu_regs.SR);
	}
	else
	{
		CPU_GENERATE_EXCEPTION(0x20);
	}
}

void CPU_LINK(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t ear;

	cpu_regs.PC+=2;

	ear = (int16_t)MEM_getWord(cpu_regs.PC);
	cpu_regs.PC+=2;

	cpu_regs.A[7]-=4;
	MEM_setLong(cpu_regs.A[7],cpu_regs.A[op1]);
	
	cpu_regs.A[op1]=cpu_regs.A[7];
	cpu_regs.A[7]+=ear;
}

void CPU_CMPM(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t zMask,nMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ADDd(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_UNLK(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.PC+=2;

	cpu_regs.A[7]=cpu_regs.A[op1];
	cpu_regs.A[op1]=MEM_getLong(cpu_regs.A[7]);
	cpu_regs.A[7]+=4;
}

void CPU_ORs(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ANDd(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ORI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ASL(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ASR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_DIVU(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eaq,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_BCLR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_EORd(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_BTST(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_STOP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t oldSR;

	if (cpu_regs.SR & CPU_STATUS_S)
	{
		oldSR=cpu_regs.SR;

		cpu_regs.PC+=2;

		cpu_regs.SR = MEM_getWord(cpu_regs.PC);
		cpu_regs.PC+=2;

		CPU_CHECK_SP(oldSR,cpu_regs.SR);

		cpuStopped=1;
	}
	else
	{
		CPU_GENERATE_EXCEPTION(0x20);
	}
}

void CPU_ROL(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ROR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_NOP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    cpu_regs.PC+=2;
}

void CPU_BCHG(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_BCHGI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_LSRm(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_EORI(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_EORICCR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t eas,ead;
	
    cpu_regs.PC+=2;
	
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;

	ead=cpu_regs.SR;
	
	eas=(ead^eas)&(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
}

void CPU_ROXL(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ROXR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_MOVETOCCR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int32_t eas,ead;

	cpu_regs.PC+=2;
		
	eas=getSourceEffectiveAddress(op1,2);
		
	ead=cpu_regs.SR;
	
	eas&=(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
}

void CPU_TRAP(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	cpu_regs.PC+=2;
	
	CPU_GENERATE_EXCEPTION(0x80 + (op1*4));
}

void CPU_ADDX(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_DIVS(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    int32_t ead,eas,eaq,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_SUBX(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t ead,eas,ear;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ASRm(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,eat;
	
    cpu_regs.PC+=2;
	
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
}

void CPU_ASLm(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t nMask,zMask;
    u_int32_t eas,ead,eat;
	
    cpu_regs.PC+=2;
	
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

}

void CPU_ANDICCR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t eas,ead;
	
    cpu_regs.PC+=2;
	
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;

	ead=cpu_regs.SR;
	
	eas=(ead&eas)&(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
}

void CPU_ORICCR(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	u_int16_t eas,ead;
	
    cpu_regs.PC+=2;
	
	eas=MEM_getByte(cpu_regs.PC+1);
	cpu_regs.PC+=2;

	ead=cpu_regs.SR;
	
	eas=(ead|eas)&(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);	// Only affects lower valid bits in flag

	ead&=~(CPU_STATUS_X|CPU_STATUS_N|CPU_STATUS_V|CPU_STATUS_C|CPU_STATUS_Z);
	
	ead|=eas;
	
	cpu_regs.SR=ead;
}



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
#endif

FILE *opsFile=NULL;
FILE *disFile=NULL;
FILE *opshFile;
FILE *dishFile;
FILE *opsTabFile;
FILE *disTabFile;

///////// DISSASEMBLY BUILDERS /////////

#include "cpu_builder_dissasemble.h"

///////// INSTRUCTION BUILDERS /////////

#include "cpu_builder_opcode.h"

typedef void (*CPU_Decode)(u_int32_t adr,u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
typedef void (*CPU_Function)(u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);

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

CPU_Ins cpu_instructions[] = 
{
/*
// Supervisor instructions
{"0100111001110010","STOP",CPU_STOP,CPU_DIS_STOP,0},
{"0100011011aaaaaa","MOVESR",CPU_MOVETOSR,CPU_DIS_MOVETOSR,1,{0x003F},{0},{11},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000001001111100","ANDSR",CPU_ANDSR,CPU_DIS_ANDSR,0},
{"0100111001110011","RTE",CPU_RTE,CPU_DIS_RTE,0},
{"0000000001111100","ORSR",CPU_ORSR,CPU_DIS_ORSR,0},
{"010011100110mrrr","MOVEUSP",CPU_MOVEUSP,CPU_DIS_MOVEUSP,2,{0x0008,0x0007},{3,0},{1,1},{{"r"},{"rrr"}}},
// Illegal instructions - that programs use
{"0100001011aaaaaa","010+MOVEfromCCR",CPU_ILLEGAL,CPU_DIS_ILLEGAL,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},	// ODYSSEY demo uses this
{"0100111001111011","ILLEGAL",CPU_ILLEGAL,CPU_DIS_ILLEGAL,0},		// KS does this one
// User instructions
{"0000000000111100","ORCCR",CPU_ORICCR,CPU_DIS_ORICCR,0},
{"0000001000111100","ANDCCR",CPU_ANDICCR,CPU_DIS_ANDICCR,0},
{"1110000011aaaaaa","ASR",CPU_ASRm,CPU_DIS_ASRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110000111aaaaaa","ASL",CPU_ASLm,CPU_DIS_ASLm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
/// 1001rrr1zz001ddd  9100 -> 9FCF	SUBX
{"1001rrr1zz000ddd","SUBX",CPU_SUBX,CPU_DIS_SUBX,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
//{"1101rrr1zz001ddd","ADDX",CPU_ADDXm,CPU_DIS_ADDXm,  D100 -> DFC0      ADDX
{"0100000011aaaaaa","MOVESR",CPU_MOVEFROMSR,CPU_DIS_MOVEFROMSR,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr111aaaaaa","DIVS",CPU_DIVS,CPU_DIS_DIVS,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1101rrr1zz000ddd","ADDX",CPU_ADDX,CPU_DIS_ADDX,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
{"010011100100rrrr","TRAP",CPU_TRAP,CPU_DIS_TRAP,1,{0x000F},{0},{1},{{"rrrr"}}},
{"0100010011aaaaaa","MOVECCR",CPU_MOVETOCCR,CPU_DIS_MOVETOCCR,1,{0x003F},{0},{11},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1110ccc0zzm10rrr","ROXR",CPU_ROXR,CPU_DIS_ROXR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1110ccc1zzm10rrr","ROXL",CPU_ROXL,CPU_DIS_ROXL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"0000101000111100","EORICCR",CPU_EORICCR,CPU_DIS_EORICCR,0},
{"00001010zzaaaaaa","EORI",CPU_EORI,CPU_DIS_EORI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110001011aaaaaa","LSR",CPU_LSRm,CPU_DIS_LSRm,1,{0x003F},{0},{7},{{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000100001aaaaaa","BCHGI",CPU_BCHGI,CPU_DIS_BCHGI,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000rrr101aaaaaa","BCHG",CPU_BCHG,CPU_DIS_BCHG,2,{0x0E00,0x003F},{9,0},{1,8},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111001110001","NOP",CPU_NOP,CPU_DIS_NOP,0},
{"1110ccc0zzm11rrr","ROR",CPU_ROR,CPU_DIS_ROR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1110ccc1zzm11rrr","ROL",CPU_ROL,CPU_DIS_ROL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"0000rrr100aaaaaa","BTST",CPU_BTST,CPU_DIS_BTST,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1011rrr1mmaaaaaa","EOR",CPU_EORd,CPU_DIS_EORd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,8},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000rrr110aaaaaa","BCLR",CPU_BCLR,CPU_DIS_BCLR,2,{0x0E00,0x003F},{9,0},{1,8},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr011aaaaaa","DIVU",CPU_DIVU,CPU_DIS_DIVU,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1110ccc0zzm00rrr","ASR",CPU_ASR,CPU_DIS_ASR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1110ccc1zzm00rrr","ASL",CPU_ASL,CPU_DIS_ASL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"00000000zzaaaaaa","ORI",CPU_ORI,CPU_DIS_ORI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr1mmaaaaaa","AND",CPU_ANDd,CPU_DIS_ANDd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr0mmaaaaaa","OR",CPU_ORs,CPU_DIS_ORs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,11},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0100111001011rrr","UNLK",CPU_UNLK,CPU_DIS_UNLK,1,{0x0007},{0},{1},{{"rrr"}}},
{"1101rrr1mmaaaaaa","ADD",CPU_ADDd,CPU_DIS_ADDd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1011rrr1zz001ddd","CMPM",CPU_CMPM,CPU_DIS_CMPM,3,{0x0E00,0x00C0,0x0007},{9,6,0},{1,3,1},{{"rrr"},{"00","01","10"},{"rrr"}}},
{"0100111001010rrr","LINK",CPU_LINK,CPU_DIS_LINK,1,{0x0007},{0},{1},{{"rrr"}}},
{"0100100001aaaaaa","PEA",CPU_PEA,CPU_DIS_PEA,1,{0x003F},{0},{7},{{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"0101cccc11aaaaaa","Scc",CPU_SCC,CPU_DIS_SCC,2,{0x0F00,0x003F},{8,0},{1,8},{{"rrrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"01000100zzaaaaaa","NEG",CPU_NEG,CPU_DIS_NEG,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr111aaaaaa","MULS",CPU_MULS,CPU_DIS_MULS,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"01001000mm000rrr","EXT",CPU_EXT,CPU_DIS_EXT,2,{0x00C0,0x0007},{6,0},{2,1},{{"10","11"},{"rrr"}}},
{"00000110zzaaaaaa","ADDI",CPU_ADDI,CPU_DIS_ADDI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1110ccc1zzm01rrr","LSL",CPU_LSL,CPU_DIS_LSL,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1100rrr011aaaaaa","MULU",CPU_MULU,CPU_DIS_MULU,2,{0x0E00,0x003F},{9,0},{1,11},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000100011aaaaaa","BSETI",CPU_BSETI,CPU_DIS_BSETI,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0000rrr111aaaaaa","BSET",CPU_BSET,CPU_DIS_BSET,2,{0x0E00,0x003F},{9,0},{1,8},{{"rrr"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1001rrr1mmaaaaaa","SUB",CPU_SUBd,CPU_DIS_SUBd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1100rrr0mmaaaaaa","AND",CPU_ANDs,CPU_DIS_ANDs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,11},{{"rrr"},{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000100010aaaaaa","BCLRI",CPU_BCLRI,CPU_DIS_BCLRI,1,{0x003F},{0},{8},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111010aaaaaa","JSR",CPU_JSR,CPU_DIS_JSR,1,{0x003F},{0},{7},{{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"1100rrr1mmmmmddd","EXG",CPU_EXG,CPU_DIS_EXG,3,{0x0E00,0x00F8,0x0007},{9,3,0},{1,3,1},{{"rrr"},{"01000","01001","10001"},{"rrr"}}},
{"00000010zzaaaaaa","ANDI",CPU_ANDI,CPU_DIS_ANDI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"01000010zzaaaaaa","CLR",CPU_CLR,CPU_DIS_CLR,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0101ddd0zzaaaaaa","ADDQ",CPU_ADDQ,CPU_DIS_ADDQ,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,9},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1000rrr1mmaaaaaa","OR",CPU_ORd,CPU_DIS_ORd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"0100111001110101","RTS",CPU_RTS,CPU_DIS_RTS,0},
{"01100001dddddddd","BSR",CPU_BSR,CPU_DIS_BSR,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
{"00000100zzaaaaaa","SUBI",CPU_SUBI,CPU_DIS_SUBI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"010010001zaaaaaa","MOVEM",CPU_MOVEMd,CPU_DIS_MOVEMd,2,{0x0040,0x003F},{6,0},{1,6},{{"r"},{"010rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"010011001zaaaaaa","MOVEM",CPU_MOVEMs,CPU_DIS_MOVEMs,2,{0x0040,0x003F},{6,0},{1,8},{{"r"},{"010rrr","011rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"0100100001000rrr","SWAP",CPU_SWAP,CPU_DIS_SWAP,1,{0x0007},{0},{1},{{"rrr"}}},
{"1110ccc0zzm01rrr","LSR",CPU_LSR,CPU_DIS_LSR,4,{0x0E00,0x000C0,0x0020,0x0007},{9,6,5,0},{1,3,1,1},{{"rrr"},{"00","01","10"},{"r"},{"rrr"}}},
{"1001rrr0mmaaaaaa","SUB",CPU_SUBs,CPU_DIS_SUBs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0111rrr0dddddddd","MOVEQ",CPU_MOVEQ,CPU_DIS_MOVEQ,2,{0x0E00,0x00FF},{9,0},{1,1},{{"rrr"},{"rrrrrrrr"}}},
{"0100111011aaaaaa","JMP",CPU_JMP,CPU_DIS_JMP,1,{0x003F},{0},{7},{{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"01001010zzaaaaaa","TST",CPU_TST,CPU_DIS_TST,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1101rrrm11aaaaaa","ADDA",CPU_ADDA,CPU_DIS_ADDA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1001rrrm11aaaaaa","SUBA",CPU_SUBA,CPU_DIS_SUBA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"01000110zzaaaaaa","NOT",CPU_NOT,CPU_DIS_NOT,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1101rrr0mmaaaaaa","ADD",CPU_ADDs,CPU_DIS_ADDs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0000100000aaaaaa","BTSTI",CPU_BTSTI,CPU_DIS_BTSTI,1,{0x003F},{0},{10},{{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{"01100000dddddddd","BRA",CPU_BRA,CPU_DIS_BRA,1,{0x00FF},{0},{1},{{"rrrrrrrr"}}},
{"0101cccc11001rrr","DBCC",CPU_DBCC,CPU_DIS_DBCC,2,{0x0F00,0x0007},{8,0},{1,1},{{"rrrr"},{"rrr"}}},
{"00zzddd001aaaaaa","MOVEA",CPU_MOVEA,CPU_DIS_MOVEA,3,{0x3000,0x0E00,0x003F},{12,9,0},{2,1,12},{{"11","10"},{"rrr"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"00001100zzaaaaaa","CMPI",CPU_CMPI,CPU_DIS_CMPI,2,{0x00C0,0x003F},{6,0},{3,8},{{"00","01","10"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"1011rrr0mmaaaaaa","CMP",CPU_CMP,CPU_DIS_CMP,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"1011rrrm11aaaaaa","CMPA",CPU_CMPA,CPU_DIS_CMPA,3,{0x0E00,0x0100,0x003F},{9,8,0},{1,1,12},{{"rrr"},{"r"},{"000rrr","001rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
*/
{"0110ccccdddddddd","BCC",CPU_BCC,CPU_DIS_BCC,2,{0x0F00,0x00FF},{8,0},{3,1},{{"rr1r","r1rr","1rrr"},{"rrrrrrrr"}}},
{"0101ddd1zzaaaaaa","SUBQ",CPU_SUBQ,CPU_DIS_SUBQ,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,9},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
{"00zzaaaaaaAAAAAA","MOVE",CPU_MOVE,CPU_DIS_MOVE,3,{0x3000,0x0FC0,0x003F},{12,6,0},{3,8,12},{{"01","10","11"},{"rrr000","rrr010","rrr011","rrr100","rrr101","rrr110","000111","001111"},{"000rrr","001???","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
{"0100rrr111aaaaaa","LEA",CPU_LEA,CPU_DIS_LEA,2,{0x0E00,0x003F},{9,0},{1,7},{{"rrr"},{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
{0,0,0}
};

CPU_Function	CPU_JumpTable[65536];


/// 1100xxx10000ryyy  C100 -> FF0F	ABCD
/// 0100100001001vvv  4848 -> 484F	BKPT
/// 0100rrrss0aaaaaa  4000 -> 4FBF	CHK
/// 0100101011111100  4AFC -> 4AFC	ILLEGAL
/// 0000dddmmm001aaa  0008 -> 0FCF	MOVEP + 2 byte disp
/// 0100100000aaaaaa  4800 -> 483F	NBCD
/// 01000000ssaaaaaa  4000 -> 40FF	NEGX
/// 0100111001110111  4E77 -> 4E77	RTR
/// 1000yyy10000rxxx  8100 -> 8F0F	SBCD
/// 0100101011aaaaaa  4AC0 -> 4AFF	TAS
/// 0100111001110110  4E76 -> 4E76	TRAPV


const char *byte_to_binary(u_int32_t x)
{
    static char b[17] = {0};
	
    b[0]=0;
    u_int32_t z;
    for (z = 32768; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }
	
    return b;
}

u_int8_t ValidateOpcode(int insNum,u_int16_t opcode)
{
    u_int8_t invalidMask;
    int a,b,c;
    char *mask;
    int operandNum=0;
    u_int8_t lastBase=0;
	
    for (a=0;a<16;a++)
    {
		if (cpu_instructions[insNum].baseTable[a]!=lastBase && cpu_instructions[insNum].baseTable[a]!='0' && cpu_instructions[insNum].baseTable[a]!='1')
		{
			lastBase = cpu_instructions[insNum].baseTable[a];
			
			u_int16_t operand = (opcode & cpu_instructions[insNum].operandMask[operandNum]) >> cpu_instructions[insNum].operandShift[operandNum];
			
			for (b=0;b<cpu_instructions[insNum].numValidMasks[operandNum];b++)
			{
				invalidMask = 0;
				mask = cpu_instructions[insNum].validEffectiveAddress[operandNum][b];
				while (*mask!=0)
				{
					c=strlen(mask)-1;
					switch (*mask)
					{
						case '0':
							if ((operand & (1<<c)) != 0)
								invalidMask=1;
							break;
						case '1':
							if ((operand & (1<<c)) == 0)
								invalidMask=1;
							break;
						case 'r':
							break;
						case '?':
							if ( (opcode & 0x3000) == 0x1000)
								invalidMask=1;			// special case - address register direct not supported for byte size operations
							break;
						case '!':
							if ( (opcode & 0x00C0) == 0x0000)
								invalidMask=1;			// special case - address register direct not supported for byte size operations
					}
					mask++;
				}
				if (!invalidMask)
					break;
			}
			if (invalidMask)
				return 0;
			operandNum++;
		}
    }
	
    return 1;
}

void WriteHeader(FILE *file,char *name)
{
	fprintf(file,"/*\n");
	fprintf(file," *  %s\n",name);
	fprintf(file," *  ami\n");
	fprintf(file,"\n");
	fprintf(file,"Copyright (c) 2009 Lee Hammerton\n");
	fprintf(file,"\n");
	fprintf(file,"Permission is hereby granted, free of charge, to any person obtaining a copy\n");
	fprintf(file,"of this software and associated documentation files (the \"Software\"), to deal\n");
	fprintf(file,"in the Software without restriction, including without limitation the rights\n");
	fprintf(file,"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
	fprintf(file,"copies of the Software, and to permit persons to whom the Software is\n");
	fprintf(file,"furnished to do so, subject to the following conditions:\n");
	fprintf(file,"\n");
	fprintf(file,"The above copyright notice and this permission notice shall be included in\n");
	fprintf(file,"all copies or substantial portions of the Software.\n");
	fprintf(file,"\n");
	fprintf(file,"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
	fprintf(file,"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
	fprintf(file,"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
	fprintf(file,"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
	fprintf(file,"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
	fprintf(file,"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n");
	fprintf(file,"THE SOFTWARE.\n");
	fprintf(file,"\n");
	fprintf(file," */\n");
	fprintf(file,"\n");
	fprintf(file,"#include <stdlib.h>\n");
	fprintf(file,"#include <stdio.h>\n");
	fprintf(file,"#include <string.h>\n");
	fprintf(file,"\n");
	fprintf(file,"#include \"config.h\"\n");
	fprintf(file,"\n");
	fprintf(file,"#include \"ciachip.h\"\n");
	fprintf(file,"#include \"cpu.h\"\n");
	fprintf(file,"#include \"memory.h\"\n");
	fprintf(file,"#include \"customchip.h\"\n");
	fprintf(file,"\n");
}

void OpenNextOpsFile()
{
	static int opID=0;
	static char opFileName[128];

	if (opsFile)
		fclose(opsFile);

	sprintf(opFileName,"../../cpu_src/cpuOps%04X.c",opID);
    opsFile = fopen(opFileName,"wb");		// needed when running in debugger
    if (!opsFile)
    {
		sprintf(opFileName,"cpu_src/cpuOps%04X.c",opID);
		opsFile = fopen(opFileName,"wb");
		if (!opsFile)
		{
			printf("Failed to open file for writing, this is bad\n");
			exit(-1);
		}
    }

	sprintf(opFileName,"cpuOps%04X.c",opID);
	WriteHeader(opsFile,opFileName);

	opID++;
}

void OpenNextDisFile()
{
	static int disID=0;
	static char disFileName[128];

	if (disFile)
		fclose(disFile);

	sprintf(disFileName,"../../cpu_src/cpuDis%04X.c",disID);
    disFile = fopen(disFileName,"wb");		// needed when running in debugger
    if (!disFile)
    {
		sprintf(disFileName,"cpu_src/cpuDis%04X.c",disID);
		disFile = fopen(disFileName,"wb");
		if (!disFile)
		{
			printf("Failed to open file for writing, this is bad\n");
			exit(-1);
		}
    }
	
	sprintf(disFileName,"cpuDis%04X.c",disID);
	WriteHeader(disFile,disFileName);

	disID++;
}

void CPU_BuildTable()
{
	int a,b,c,d,e;
	
	for (a=0;a<65536;a++)
	{
		CPU_JumpTable[a]=NULL;
	}
	
	a=0;
	while (cpu_instructions[a].opcode)
	{
	    int modifiableBitCount=0;

		OpenNextOpsFile();
		OpenNextDisFile();
		
	    // precount modifiable bits
	    for (b=0;b<16;b++)
	    {
			switch (cpu_instructions[a].baseTable[b])
			{
				case '0':
				case '1':
					// Fixed code
					break;
				default:
					modifiableBitCount++;
			}
	    }
	    modifiableBitCount=1<<modifiableBitCount;
		
	    b=0;
	    while (1)
	    {
			u_int8_t needValidation=0;
			u_int8_t validOpcode=1;
			u_int16_t opcode=0;
			
			d=0;
			// Create instruction code
			for (c=0;c<16;c++)
			{
				switch (cpu_instructions[a].baseTable[c])
				{
					case '0':
						break;
					case '1':
						opcode|=1<<(15-c);
						break;
					case 'r':
					case 'd':
					case 'a':
					case 'A':
					case 'z':
					case 'm':
					case 'c':
						opcode|=((b&(1<<d))>>d)<<(15-c);
						d++;
						needValidation=1;
						break;
				}
			}
			if (needValidation)
			{
				if (!ValidateOpcode(a,opcode))
				{
					validOpcode=0;
				}
			}
			if (validOpcode)
			{
				u_int16_t operands[8];

//				printf("Opcode Coding : %s : %04X %s\n", cpu_instructions[a].opcodeName, opcode,byte_to_binary(opcode));
				if (CPU_JumpTable[opcode])
				{
					printf("[ERR] Cpu Coding For Instruction Overlap\n");
					exit(-1);
				}
				
				// Here we output the instruction
				//
				// Reasonably simple to do : 

				// Build operands
				for (e=0;e<cpu_instructions[a].numOperands;e++)
				{
					operands[e] = (opcode & cpu_instructions[a].operandMask[e]) >> cpu_instructions[a].operandShift[e];
				}
				
				// Next run dissasemble function (which will expand and dump its function)
				cpu_instructions[a].decode(0,opcode,operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);
				
				// Next run cpu function (which will expand and dump its function)
				cpu_instructions[a].opcode(opcode,operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);

				CPU_JumpTable[opcode]=cpu_instructions[a].opcode;			// Store opcode away for error checking
				
/*
				CPU_DisTable[opcode]=cpu_instructions[a].decode;
				CPU_Information[opcode]=&cpu_instructions[a];
*/
			}
			b++;
			if (b>=modifiableBitCount)
				break;
	    }
		
	    a++;
	}
}

int main(int argc,char **argv)
{
    opshFile = fopen("../../cpu_src/cpuOps.h","wb");		// needed when running in debugger
    if (!opshFile)
    {
		opshFile = fopen("cpu_src/cpuOps.h","wb");
		if (!opshFile)
		{
			printf("Failed to open file for writing, this is bad\n");
			return -1;
		}
    }
    dishFile = fopen("../../cpu_src/cpuDis.h","wb");		// needed when running in debugger
    if (!dishFile)
    {
		dishFile = fopen("cpu_src/cpuDis.h","wb");
		if (!dishFile)
		{
			printf("Failed to open file for writing, this is bad\n");
			return -1;
		}
    }
    opsTabFile = fopen("../../cpu_src/cpuOpsTable.h","wb");		// needed when running in debugger
    if (!opsTabFile)
    {
		opsTabFile = fopen("cpu_src/cpuOpsTable.h","wb");
		if (!opsTabFile)
		{
			printf("Failed to open file for writing, this is bad\n");
			return -1;
		}
    }
    disTabFile = fopen("../../cpu_src/cpuDisTable.h","wb");		// needed when running in debugger
    if (!disTabFile)
    {
		disTabFile = fopen("cpu_src/cpuDisTable.h","wb");
		if (!disTabFile)
		{
			printf("Failed to open file for writing, this is bad\n");
			return -1;
		}
    }
	
	// Dump intial table setup
	fprintf(opsTabFile,"typedef u_int32_t (*CPU_Opcode)(u_int32_t stage);\n");
	fprintf(opsTabFile,"\nCPU_Opcode CPU_JumpTable[65536];\n");
	fprintf(opsTabFile,"\nvoid CPU_BuildOpsTable()\n");
	fprintf(opsTabFile,"{\n");
	fprintf(opsTabFile,"\tu_int32_t\ta;\n");
	fprintf(opsTabFile,"\n\tfor (a=0;a<65536;a++) CPU_JumpTable[a]=CPU_UNKNOWN;\n");
	fprintf(opsTabFile,"\n");

	fprintf(disTabFile,"typedef u_int32_t (*CPU_Decode)(u_int32_t adr);\n");
	fprintf(disTabFile,"\nCPU_Decode CPU_DisTable[65536];\n");
	fprintf(disTabFile,"\nvoid CPU_BuildDisTable()\n");
	fprintf(disTabFile,"{\n");
	fprintf(disTabFile,"\tu_int32_t\ta;\n");
	fprintf(disTabFile,"\n\tfor (a=0;a<65536;a++) CPU_DisTable[a]=CPU_DIS_UNKNOWN;\n");
	fprintf(disTabFile,"\n");
	
	CPU_BuildTable();
	
	fprintf(opsTabFile,"}\n");
	fprintf(disTabFile,"}\n");
	
    fclose(opsFile);
    fclose(disFile);
    fclose(opshFile);
    fclose(dishFile);
    fclose(opsTabFile);
    fclose(disTabFile);
	
    return 0;
}