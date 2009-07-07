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

#include "cpu.h"
#include "memory.h"

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

void CPU_LEA()
{
	printf("LEA\n");
}

typedef void (*CPU_Function)();

typedef struct
{
	char baseTable[17];
	char opcodeName[32];
	CPU_Function opcode;
} CPU_Ins;

CPU_Ins cpu_instructions[] = 
	{
		{"0100rrr111aaaaaa","LEA",CPU_LEA},
		{0,0,0}
	};

CPU_Function CPU_JumpTable[65536];

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
/// 00zzddddddssssss  0000 -> 3FFF	MOVE
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
/// 1001rrrmmmaaaaaa  9000 -> 9FFF	SUB
/// 1001rrrmmmaaaaaa  9000 -> 9FFF	SUBA
/// 00000100ssaaaaaa  0400 -> 04FF	SUBI + 1,2,4 bytes extra dep wrd size
/// 0101ddd1ssaaaaaa  5100 -> 5FFF	SUBQ
/// 1001yyy1ss00rxxx  9100 -> 9FCF	SUBX
/// 0100100001000rrr  4840 -> 4847	SWAP
/// 0100101011aaaaaa  4AC0 -> 4AFF	TAS
/// 010011100100vvvv  4E40 -> 4E4F	TRAP
/// 0100111001110110  4E76 -> 4E76	TRAPV
/// 01001010ssaaaaaa  4A00 -> 4AFF	TST
/// 0100111001011rrr  4E58 -> 4E5F	UNLK

void CPU_UNKNOWN()
{
	printf("ILLEGAL INSTRUCTION\n");
	char* bob=0;
	*bob=0;
}

void CPU_BuildTable()
{
	int a,b;
	
	for (a=0;a<65536;a++)
	{
		CPU_JumpTable[a]=CPU_UNKNOWN;
	}
	
	a=0;
	while (cpu_instructions[a].opcode)
	{
		
		a++;
	}
}

void CPU_Step()
{
	printf("%08x\t%s\t",cpu_regs.PC,decodeWord(cpu_regs.PC));
	CPU_JumpTable[MEM_getWord(cpu_regs.PC)]();
}