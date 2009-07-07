/* 
 
 Decrypt cloanto rom
 
 */

#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "memory.h"

int decrpyt_rom()
{
    FILE *inRom;
    FILE *inKey;
    FILE *outData;
    unsigned char *romData;
    unsigned char *keyData;
    unsigned long romSize;
    unsigned long keySize;
    int a,b;
	
    inRom = fopen("amiga-os-130.rom","rb");
    if (!inRom)
    {
		printf("FAIL\n");
		return -1;
    }
    fseek(inRom,0,SEEK_END);
    romSize = ftell(inRom);
    fseek(inRom,11,SEEK_SET);	    // skip over AMIROMTYPE1
    romData = (unsigned char *)malloc(romSize);
    romSize = fread(romData,1,romSize,inRom);
    fclose(inRom);
	
    inKey = fopen("rom.key","rb");
    if (!inKey)
    {
		printf("FAIL\n");
		return -1;
    }
    fseek(inKey,0,SEEK_END);
    keySize = ftell(inKey);
    fseek(inKey,0,SEEK_SET);
    keyData = (unsigned char *)malloc(keySize);
    keySize = fread(keyData,1,keySize,inKey);
    fclose(inKey);
	
    printf("Rom Size = %lu\n",romSize);
    printf("Key Size = %lu\n",keySize);
	
    outData = fopen("out.rom","wb");
    if (!outData)
    {
		printf("FAIL\n");
		return -1;
    }
	
    for (a=0;a<romSize;a++)
    {
		b = romData[a] ^ keyData[a%keySize];
		fputc(b,outData);
		//printf("%02X[%02X]",romData[a],b);
		/*	if (b>30)
		 {
		 printf("%c",b);
		 }*/
    }
    fclose(outData);
	
    return 0;
}

unsigned char *load_rom(char *romName)
{
    FILE *inRom;
    unsigned char *romData;
    unsigned long romSize;
	
    inRom = fopen(romName,"rb");
    if (!inRom)
    {
		printf("FAIL\n");
		return 0;
    }
    fseek(inRom,0,SEEK_END);
    romSize = ftell(inRom);
	fseek(inRom,0,SEEK_SET);
    romData = (unsigned char *)malloc(romSize);
    romSize = fread(romData,1,romSize,inRom);
    fclose(inRom);
	
	return romData;
}

/// OPCODES 68000

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
/// 0100rrr111aaaaaa  41C0 -> 4FFF	LEA
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


int main(int argc,char **argv)
{
	//    decrpyt_rom();
	
    unsigned char *romPtr=load_rom("../../out.rom");

	CPU_BuildTable();
	
	MEM_Initialise(romPtr);
	
	CPU_Reset();
	
	while (1)
	{
		CPU_Step();
	}
	
    return 0;
}
