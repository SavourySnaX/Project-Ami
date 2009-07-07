/*
 *  cpu.c
 *  ami
 *
 *  Created by Lee Hammerton on 06/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"

#define SOFT_BREAK	{ char* bob=0; 	*bob=0; }

CPU_Regs cpu_regs;

void CPU_Reset()
{
	int a;
	
	for (a=0;a<8;a++)
	{
		cpu_regs.D[a]=0;
		cpu_regs.A[a]=0;
	}
	cpu_regs.SR = CPU_STATUS_S | CPU_STATUS_I2 | CPU_STATUS_I1 | CPU_STATUS_I0;
	
	cpu_regs.A[7] = MEM_getLong(0xFC0000);
	cpu_regs.PC = MEM_getLong(0xFC0004);
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

static char byteData[256];
static char mnemonicData[256];
static char tempData[256];

int decodeEffectiveAddress(u_int32_t adr, u_int16_t operand,char *mData,char *bData,int length)
{
    int adj=0;

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
	    sprintf(tempData,"D%d",operand);
	    strcat(mData,tempData);
	    return 0;
	case 0x39:		/// 111001
	    sprintf(tempData,"(%08x).L",MEM_getLong(adr));
	    strcat(mData,tempData);
	    strcat(bData,decodeLong(adr));
	    return 4;
	case 0x3C:		/// 111100
	    switch (length)
	    {
		case 1:
		    sprintf(tempData,"#%02x",MEM_getByte(adr+1));
		    strcat(bData,decodeWord(adr));
		    adj=2;
		    break;
		case 2:
		    sprintf(tempData,"#%04x",MEM_getWord(adr));
		    strcat(bData,decodeWord(adr));
		    adj=2;
		    break;
		case 4:
		    sprintf(tempData,"#%08x",MEM_getLong(adr));
		    strcat(bData,decodeLong(adr));
		    adj=4;
		    break;
	    }
	    strcat(mData,tempData);
	    return adj;
	default:
	    strcat(mData,"UNKNOWN");
	    break;
    }
    return 0;
}

u_int32_t getEffectiveAddress(u_int16_t operand,int length)
{
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
	case 0x39:		/// 111001
	    ea = MEM_getLong(cpu_regs.PC);
	    cpu_regs.PC+=4;
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
	    break;
    }
    return ea;
}

void CPU_DIS_LEA(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    adr+=2;
    strcpy(mnemonicData,"LEA ");
    strcpy(byteData,"");
    adr+=decodeEffectiveAddress(adr,op2,mnemonicData,byteData,4);
    sprintf(tempData,",A%d",op1);
    strcat(mnemonicData,tempData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_MOVE(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    static char tMData[256],tBData[256];
    adr+=2;
    strcpy(mnemonicData,"MOVE");
    strcpy(byteData,"");
    switch (op1)
    {
	case 0x00:
	    strcat(mnemonicData,".? ");
	    len=0;
	    break;
	case 0x01:
	    strcat(mnemonicData,".B ");
	    len=1;
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
    adr+=decodeEffectiveAddress(adr,op2,tMData,tBData,len);
    adr+=decodeEffectiveAddress(adr,op3,mnemonicData,byteData,len);
    strcat(mnemonicData,",");				    // needed cos assembler is source,dest
    strcat(mnemonicData,tMData);
    strcat(byteData,tBData);

    printf("%s\t%s\n",byteData,mnemonicData);
}

void CPU_DIS_SUBs(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    static char tMData[256],tBData[256];
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

void CPU_DIS_SUBQ(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    static char tMData[256],tBData[256];
    adr+=2;
    strcpy(mnemonicData,"SUBQ");
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

void CPU_LEA(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    u_int32_t ea;

    cpu_regs.PC+=2;
    ea = getEffectiveAddress(op2,4);
    cpu_regs.A[op1] = ea;
}

void CPU_MOVE(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas;

    cpu_regs.PC+=2;

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

    if ((op2 & 0x38)==0)	// destination is D register
    {
	eas = getEffectiveAddress(op3,len);
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
	SOFT_BREAK;
	ead = getEffectiveAddress(op2,len);
	eas = getEffectiveAddress(op3,len);
	switch (len)
	{
	    case 1:
		MEM_setByte(ead,eas&0xFF);
		break;
	    case 2:
		MEM_setWord(ead,eas&0xFFFF);
		break;
	    case 3:
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
}

void CPU_SUBs(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    SOFT_BREAK;
/*
    u_int32_t ea;

    cpu_regs.PC+=2;
    ea = getEffectiveAddress(op2,4);
    cpu_regs.A[op1] = ea;*/
}

void CPU_SUBQ(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
    u_int32_t ead,eas,ear;

    cpu_regs.PC+=2;

	if (op1==0)
		op1=8;
	
    switch(op2)
    {
	case 0x00:
	    len=1;
	    break;
	case 0x01:
	    len=2;
	    break;
	case 0x02:
	    len=4;
	    break;
    }

    if ((op3 & 0x38)==0)	// destination is D register
    {
		eas=op1;
		ead=cpu_regs.D[op3];
		switch(len)
		{
			case 1:
				ead=CPU_SUBTRACT_BYTE(op1&0xFF,cpu_regs.D[op3]&0xFF);
				cpu_regs.D[op3]&=0xFFFFFF00;
				cpu_regs.D[op3]|=ead&0xFF;
				break;
			case 2:
				ead=CPU_SUBTRACT_WORD(op1&0xFFFF,cpu_regs.D[op3]&0xFFFF);
				cpu_regs.D[op3]&=0xFFFF0000;
				cpu_regs.D[op3]|=ead&0xFFFF;
				break;
			case 4:
				ead=CPU_SUBTRACT_LONG(op1&0xFFFFFFFF,cpu_regs.D[op3]&0xFFFFFFFF);
				cpu_regs.D[op3]=ead&0xFFFFFFFF;
				break;
		}
    }
    else
    {
		SOFT_BREAK;
/*	ead = getEffectiveAddress(op2,len);
	eas = getEffectiveAddress(op3,len);
	switch (len)
	{
	    case 1:
		MEM_setByte(ead,eas&0xFF);
		break;
	    case 2:
		MEM_setWord(ead,eas&0xFFFF);
		break;
	    case 3:
		MEM_setLong(ead,eas);
		break;
	}*/
    }

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
	
	if (cpu_regs.SR & CPU_STATUS_C)
		cpu_regs.SR|=CPU_STATUS_X;
	else
		cpu_regs.SR&=~CPU_STATUS_X;
	if (ead & (~zMask))
		cpu_regs.SR|=CPU_STATUS_C;
	else
		cpu_regs.SR&=~CPU_STATUS_C;
	if ((eas & nMask) != (ead & nMask))
		cpu_regs.SR|=CPU_STATUS_V;
	else
		cpu_regs.SR&=~CPU_STATUS_V;
    if (ead & nMask)
		cpu_regs.SR|=CPU_STATUS_N;
    else
		cpu_regs.SR&=~CPU_STATUS_N;
    if (ead & zMask)
		cpu_regs.SR&=~CPU_STATUS_Z;
    else
		cpu_regs.SR|=CPU_STATUS_Z;
}


typedef void (*CPU_Decode)(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
typedef void (*CPU_Function)(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);

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
//		{"1001rrr0mmaaaaaa","SUB",CPU_SUBs,CPU_DIS_SUBs,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,12},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
//		{"1001rrr1mmaaaaaa","SUB",CPU_SUBd,CPU_DIS_SUBd,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,7},{{"rrr"},{"00","01","10"},{"010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
		{"0101ddd1zzaaaaaa","SUBQ",CPU_SUBQ,CPU_DIS_SUBQ,3,{0x0E00,0x00C0,0x003F},{9,6,0},{1,3,9},{{"rrr"},{"00","01","10"},{"000rrr","001!!!","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"}}},
		{"00zzaaaaaaAAAAAA","MOVE",CPU_MOVE,CPU_DIS_MOVE,3,{0x3000,0x0FC0,0x003F},{12,6,0},{3,8,12},{{"01","10","11"},{"000rrr","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001"},{"000rrr","001???","010rrr","011rrr","100rrr","101rrr","110rrr","111000","111001","111100","111010","111011"}}},
		{"0100rrr111aaaaaa","LEA",CPU_LEA,CPU_DIS_LEA,2,{0x0E00,0x003F},{9,0},{1,7},{{"rrr"},{"010rrr","101rrr","110rrr","111000","111001","111010","111011"}}},
		{0,0,0}
	};

CPU_Function	CPU_JumpTable[65536];
CPU_Decode	CPU_DisTable[65536];
CPU_Ins		*CPU_Information[65536];

/// 1100xxx10000ryyy  C100 -> FF0F	ABCD
/// 1101rrrmmmaaaaaa  D000 -> DFFF	ADD
/// 1101rrrmmmaaaaaa  D000 -> DFFF	ADDA
/// 00000110ssaaaaaa  0600 -> 06FF      ADDI   + 1,2,4 bytes extra dep wrd size
/// 0101ddd0ssaaaaaa  5000 -> 5E00      ADDQ
/// 1101xxx1ss00ryyy  D100 -> DFC0      ADDX
/// 1100rrrmmmaaaaaa  C000 -> CFFF      AND
/// 00000010ssaaaaaa  0200 -> 02FF      ANDI   + 1,2,4 bytes extra dep wrd size
/// 0000001000111100  022C -> 022C	ANDI,CCR + 00000000bbbbbbbb
/// 1110cccdssi00rrr  E000 -> EFF7      ASL,ASR
/// 0110ccccdddddddd  6000 -> 6FFF      Bcc    + 2 bytes dep wrd size
/// 0000rrr101aaaaaa  0140 -> 0F7F      BCHG
/// 0000100001aaaaaa  0840 -> 087F	BCHG + 00000000bbbbbbbb
/// 0000rrr110aaaaaa  0180 -> 0FBF	BCLR
/// 0000100010aaaaaa  0880 -> 08BF	BCLR + 00000000bbbbbbbb
/// 0100100001001vvv  4848 -> 484F	BKPT
/// 01100000dddddddd  6000 -> 60FF	BRA + 2 bytes dep wrd size
/// 0000rrr111aaaaaa  01C0 -> 0FFF	BSET
/// 0000100011aaaaaa  08C0 -> 08FF	BSET + 00000000bbbbbbbb
/// 01100001dddddddd  6100 -> 61FF	BSR + 2 bytes dep wrd size
/// 0000rrr100aaaaaa  0100 -> 0F3F	BTST
/// 0000100000aaaaaa  0800 -> 083F	BTST + 00000000bbbbbbbb
/// 0100rrrss0aaaaaa  4000 -> 4FBF	CHK
/// 01000010ssaaaaaa  4200 -> 42FF	CLR
/// 1011rrrmmmaaaaaa  B000 -> BFFF	CMP
/// 1011rrrmmmaaaaaa  B000 -> BFFF      CMPA
/// 00001100ssaaaaaa  0C00 -> 0CFF	CMPI + 1,2,4 bytes extra dep wrd size
/// 1011xxx1ss001yyy  B108 -> BFCF	CMPM
/// 0101cccc11001rrr  90C8 -> 9FCF	DBcc + 2 bytes disp
/// 1000rrr111aaaaaa  81C0 -> 8FFF	DIVS
/// 1000rrr011aaaaaa  80C0 -> 8EFF	DIVU
/// 1011rrrmmmaaaaaa  B000 -> BFFF	EOR
/// 00001010ssaaaaaa  0A00 -> 0AFF	EORI + 1,2,4 bytes extra dep wrd size
/// 0000101000111100  0A2C -> 0A2C	EORI,CCR + 00000000bbbbbbbb
/// 1100xxx1mmmmmyyy  C100 -> CFFF	EXG
/// 0100100mmm000rrr  4800 -> 49C7	EXT
/// 0100101011111100  4AFC -> 4AFC	ILLEGAL
/// 0100111011aaaaaa  4EC0 -> 4EFF	JMP
/// 0100111010aaaaaa  4E80 -> 4EBF	JSR
/// 0100111001010rrr  4E50 -> 4E57	LINK + 2 bytes disp
/// 1110cccdssi01rrr  E008 -> EFEF	LSL,LSR
/// 00zzddd001ssssss  0040 -> 3E7F	MOVEA
/// 0100001011aaaaaa  42C0 -> 42FF	MOVE from CCR
/// 0100010011aaaaaa  44C0 -> 44FF	MOVE to CCR
/// 0100000011aaaaaa  40C0 -> 40FF	MOVE from SR
/// 01001d001saaaaaa  4880 -> 4CFF	MOVEM + 2 byte mask
/// 0000dddmmm001aaa  0008 -> 0FCF	MOVEP + 2 byte disp
/// 0111rrr0dddddddd  7000 -> 7E00	MOVEQ
/// 1100rrr111aaaaaa  C1C0 -> CFFF	MULS
/// 1100rrr011aaaaaa  C0C0 -> CEFF	MULU
/// 0100100000aaaaaa  4800 -> 483F	NBCD
/// 01000100ssaaaaaa  4400 -> 44FF	NEG
/// 01000000ssaaaaaa  4000 -> 40FF	NEGX
/// 0100111001110001  4E71 -> 4E71	NOP
/// 01000110ssaaaaaa  4600 -> 46FF	NOT
/// 1000rrrmmmaaaaaa  8000 -> 8FFF	OR
/// 00000000ssaaaaaa  0000 -> 00FF	ORI + 1,2,4 bytes extra dep wrd size
/// 0000000000111100  003C -> 003C	ORI,CCR + 00000000bbbbbbbb
/// 0100100001aaaaaa  4840 -> 487F	PEA
/// 1110cccdssi11rrr  E018 -> EFFF	ROL,ROR
/// 1110cccdssi10rrr  E010 -> EFF7	ROXL,ROXR
/// 0100111001110111  4E77 -> 4E77	RTR
/// 0100111001110101  4E75 -> 4E75	RTS
/// 1000yyy10000rxxx  8100 -> 8F0F	SBCD
/// 0101cccc11aaaaaa  50C0 -> 5FFF	Scc
/// 1001rrrmmmaaaaaa  9000 -> 9FFF	SUBA
/// 00000100ssaaaaaa  0400 -> 04FF	SUBI + 1,2,4 bytes extra dep wrd size
/// 1001yyy1ss00rxxx  9100 -> 9FCF	SUBX
/// 0100100001000rrr  4840 -> 4847	SWAP
/// 0100101011aaaaaa  4AC0 -> 4AFF	TAS
/// 010011100100vvvv  4E40 -> 4E4F	TRAP
/// 0100111001110110  4E76 -> 4E76	TRAPV
/// 01001010ssaaaaaa  4A00 -> 4AFF	TST
/// 0100111001011rrr  4E58 -> 4E5F	UNLK

void CPU_DIS_UNKNOWN(u_int32_t adr,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	printf("UNKNOWN INSTRUCTION\n");
}

void CPU_UNKNOWN(u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	printf("ILLEGAL INSTRUCTION\n");
	SOFT_BREAK;
}

const char *byte_to_binary(u_int32_t x)
{
    static char b[17] = {0};

    b[0]=0;
    u_int32_t z;
    for (z = 65536; z > 0; z >>= 1)
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
		    return 1;
	    }
	}
    }

    return 0;
}

void CPU_BuildTable()
{
	int a,b,c,d;
	
	for (a=0;a<65536;a++)
	{
		CPU_JumpTable[a]=CPU_UNKNOWN;
		CPU_DisTable[a]=CPU_DIS_UNKNOWN;
		CPU_Information[a]=0;
	}
	
	a=0;
	while (cpu_instructions[a].opcode)
	{
	    int modifiableBitCount=0;

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
		    printf("Opcode Coding : %s : %04X %s\n", cpu_instructions[a].opcodeName, opcode,byte_to_binary(opcode));
		    if (CPU_JumpTable[opcode]!=CPU_UNKNOWN)
		    {
			printf("[ERR] Cpu Coding For Instruction Overlap\n");
			exit(-1);
		    }
		    CPU_JumpTable[opcode]=cpu_instructions[a].opcode;
		    CPU_DisTable[opcode]=cpu_instructions[a].decode;
		    CPU_Information[opcode]=&cpu_instructions[a];
		}
		b++;
		if (b>=modifiableBitCount)
		    break;
	    }
		
	    a++;
	}
}

void DumpEmulatorState()
{
    printf("\n");
    printf("D0=%08X\tD1=%08X\tD2=%08x\tD3=%08x\n",cpu_regs.D[0],cpu_regs.D[1],cpu_regs.D[2],cpu_regs.D[3]);
    printf("D4=%08X\tD5=%08X\tD6=%08x\tD7=%08x\n",cpu_regs.D[4],cpu_regs.D[5],cpu_regs.D[6],cpu_regs.D[7]);
    printf("A0=%08X\tA1=%08X\tA2=%08x\tA3=%08x\n",cpu_regs.A[0],cpu_regs.A[1],cpu_regs.A[2],cpu_regs.A[3]);
    printf("A4=%08X\tA5=%08X\tA6=%08x\tA7=%08x\n",cpu_regs.A[4],cpu_regs.A[5],cpu_regs.A[6],cpu_regs.A[7]);
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
void CPU_Step()
{
    u_int16_t operands[8];
    u_int16_t opcode = MEM_getWord(cpu_regs.PC);
    int a;

    // DEBUGGER
    DumpEmulatorState();

    if (CPU_Information[opcode])
    {
	for (a=0;a<CPU_Information[opcode]->numOperands;a++)
	{
	    operands[a] = (opcode & CPU_Information[opcode]->operandMask[a]) >> CPU_Information[opcode]->operandShift[a];
//	    printf("Operand %d = %08x\n", a,operands[a]);
	}
    }

    printf("%08x\t%s ",cpu_regs.PC,decodeWord(cpu_regs.PC));
    
    CPU_DisTable[opcode](cpu_regs.PC,operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);

    // END DEBUGGER

    CPU_JumpTable[opcode](operands[0],operands[1],operands[2],operands[3],operands[4],operands[5],operands[6],operands[7]);
}
