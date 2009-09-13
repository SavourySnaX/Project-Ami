/*
 *  cpu_ops.h
 *  ami
 *
 *  Created by Lee Hammerton on 31/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "mytypes.h"

void CPU_GENERATE_EXCEPTION(u_int32_t exceptionAddress);

u_int32_t CPU_LEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVE(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SUBs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SUBQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_CMPA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_CMP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_CMPI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_DBCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BRA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BTSTI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ADDs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_NOT(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SUBA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ADDA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_TST(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_JMP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVEQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_LSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SWAP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVEMs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVEMd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SUBI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_RTS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ILLEGAL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ORd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ADDQ(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_CLR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ANDI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_EXG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_JSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BCLRI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ANDs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SUBd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BSET(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BSETI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MULU(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_LSL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ADDI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_EXT(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MULS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_NEG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVEUSP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SCC(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ORSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_PEA(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVEFROMSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_RTE(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ANDSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVETOSR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_LINK(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_CMPM(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ADDd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_UNLK(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ORs(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ANDd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ORI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ASL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ASR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_DIVU(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BCLR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_EORd(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BTST(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_STOP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ROL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ROR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_NOP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BCHG(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_BCHGI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_LSRm(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_EORI(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_EORICCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ROXL(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ROXR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_MOVETOCCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_TRAP(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ADDX(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_DIVS(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_SUBX(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ASRm(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ASLm(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ANDICCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
u_int32_t CPU_ORICCR(u_int32_t stage,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8);
