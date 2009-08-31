/*
 *  cpu_builder_opcode.h
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

/* Defines functions useful for creating the opcode emulation code - included directly to save prototyping everything */

////// OPCODE HELPERS ////////

u_int32_t	nxtStage;

u_int32_t	iMask,nMask,zMask;

void OUTPUT_OPCODE_START(const char *name,u_int16_t opcode)
{
	fprintf(opsTabFile,"\tCPU_JumpTable[0x%04X]=%s_%04X;\n",opcode,name,opcode);
	fprintf(opshFile,"\nu_int32_t %s_%04X(u_int32_t stage);\n",name,opcode);
	fprintf(opsFile,"\nu_int32_t %s_%04X(u_int32_t stage)\n",name,opcode);
	fprintf(opsFile,"{\n");
	fprintf(opsFile,"\tswitch (stage)\n");
	fprintf(opsFile,"\t{\n");
	nxtStage=1;
}

void OUTPUT_OPCODE_START_STAGE(u_int32_t stage)
{
	fprintf(opsFile,"\t\tcase %d:\n",stage);
	nxtStage=stage;
}

void OUTPUT_OPCODE_END_STAGE(u_int32_t newStage)
{
	fprintf(opsFile,"\t\t\treturn %d;\n",newStage);
	nxtStage=newStage;
}

void OUTPUT_OPCODE_START_CC(const char *name,u_int16_t opcode,u_int16_t op,int tf)
{
	static char ccString[256];
	static char iName[8];
	
	strcpy(iName,name);
	switch (op)
    {
		case 0x00:
			if (!tf)
			{
				printf("Illegal CC %d\n",op);
				exit(-1);
			}
			strcat(iName,"T");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=1;\n");
			break;
		case 0x01:
			if (!tf)
			{
				printf("Illegal CC %d\n",op);
				exit(-1);
			}
			strcat(iName,"F");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=0;\n");
			break;
		case 0x02:
			strcat(iName,"HI");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=((~cpu_regs.SR)&CPU_STATUS_C) && ((~cpu_regs.SR)&CPU_STATUS_Z);\n");
			break;
		case 0x03:
			strcat(iName,"LS");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=(cpu_regs.SR & CPU_STATUS_C) || (cpu_regs.SR & CPU_STATUS_Z);\n");
			break;
		case 0x04:
			strcat(iName,"CC");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=!(cpu_regs.SR & CPU_STATUS_C);\n");
			break;
		case 0x05:
			strcat(iName,"CS");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=(cpu_regs.SR & CPU_STATUS_C);\n");
			break;
		case 0x06:
			strcat(iName,"NE");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=!(cpu_regs.SR & CPU_STATUS_Z);\n");
			break;
		case 0x07:
			strcat(iName,"EQ");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=(cpu_regs.SR & CPU_STATUS_Z);\n");
			break;
		case 0x08:
			strcat(iName,"VC");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=!(cpu_regs.SR & CPU_STATUS_V);\n");
			break;
		case 0x09:
			strcat(iName,"VS");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=(cpu_regs.SR & CPU_STATUS_V);\n");
			break;
		case 0x0A:
			strcat(iName,"PL");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=!(cpu_regs.SR & CPU_STATUS_N);\n");
			break;
		case 0x0B:
			strcat(iName,"MI");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=(cpu_regs.SR & CPU_STATUS_N);\n");
			break;
		case 0x0C:
			strcat(iName,"GE");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V)) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)));\n");
			break;
		case 0x0D:
			strcat(iName,"LT");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));\n");
			break;
		case 0x0E:
			strcat(iName,"GT");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=((cpu_regs.SR & CPU_STATUS_N) && (cpu_regs.SR & CPU_STATUS_V) && (!(cpu_regs.SR & CPU_STATUS_Z))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (!(cpu_regs.SR & CPU_STATUS_V)) && (!(cpu_regs.SR & CPU_STATUS_Z)));\n");
			break;
		case 0x0F:
			strcat(iName,"LE");
			strcpy(ccString,"\t\t\tcpu_regs.tmpW=(cpu_regs.SR & CPU_STATUS_Z) || ((cpu_regs.SR & CPU_STATUS_N) && (!(cpu_regs.SR & CPU_STATUS_V))) || ((!(cpu_regs.SR & CPU_STATUS_N)) && (cpu_regs.SR & CPU_STATUS_V));\n");
			break;
    }

	OUTPUT_OPCODE_START(iName,opcode);
	
	OUTPUT_OPCODE_START_STAGE(nxtStage);
	fprintf(opsFile,ccString);
}

void OUTPUT_OPCODE_END()
{
	fprintf(opsFile,"\t}\n");
	fprintf(opsFile,"\treturn 0;\n");
	fprintf(opsFile,"}\n");
}

void OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(u_int16_t operand,int length,char *ea)
{
    switch (operand)
    {
		case 0x00:		// D?
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.D[%d];\n",ea,operand);
			break;
		case 0x08:		// A?
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.A[%d];\n",ea,operand-0x08);
			break;
		case 0x10:		// (A?)
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.A[%d];\n",ea,operand-0x10);
			break;
		case 0x18:		// (A?)+
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
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.A[%d];\n",ea,operand-0x18);
			fprintf(opsFile,"\t\t\tcpu_regs.A[%d]+=%d;\n",operand-0x18,length);
			break;
		case 0x20:		// -(A?)
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			if ((operand==0x27) && (length==1))
			{
				length=2;
			}
			fprintf(opsFile,"\t\t\tcpu_regs.A[%d]-=%d;\n",operand-0x20,length);
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.A[%d];\n",ea,operand-0x20);
			break;
		case 0x28:		// (XXXX,A?)
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.%s=cpu_regs.A[%d]+(int16_t)MEM_getWord(cpu_regs.%s);\n",ea,operand-0x28,ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x30:		// (XX,A?,X?)
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpW=MEM_getWord(cpu_regs.%s);\n",ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (cpu_regs.tmpW&0x8000)\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.%s = cpu_regs.A[(cpu_regs.tmpW>>12)&0x07];\n",ea);
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			fprintf(opsFile,"\t\t\t}\n");
			fprintf(opsFile,"\t\t\telse\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.%s = cpu_regs.D[(cpu_regs.tmpW>>12)&0x07];\n",ea);
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\t}\n");
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (!(cpu_regs.tmpW&0x0800)) cpu_regs.%s=(int16_t)cpu_regs.%s;\n",ea,ea);
			fprintf(opsFile,"\t\t\tcpu_regs.%s+=(int8_t)(cpu_regs.tmpW&0xFF);\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.%s+=cpu_regs.A[%d];\n",ea,operand-0x30);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x38:		// (XXXX).W
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.%s=(int16_t)MEM_getWord(cpu_regs.%s);\n",ea,ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x39:		// (XXXXXXXX).L
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.%s=MEM_getWord(cpu_regs.tmpL)<<16;\n",ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.%s|=MEM_getWord(cpu_regs.tmpL);\n",ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x3A:		// (XXXX,PC)
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.%s+=(int16_t)MEM_getWord(cpu_regs.%s);\n",ea,ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x3B:		/// (XX,PC,X?)
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpW=MEM_getWord(cpu_regs.%s);\n",ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (cpu_regs.tmpW&0x8000)\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.%s = cpu_regs.A[(cpu_regs.tmpW>>12)&0x07];\n",ea);
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			fprintf(opsFile,"\t\t\t}\n");
			fprintf(opsFile,"\t\t\telse\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.%s = cpu_regs.D[(cpu_regs.tmpW>>12)&0x07];\n",ea);
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\t}\n");
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (!(cpu_regs.tmpW&0x0800)) cpu_regs.%s=(int16_t)cpu_regs.%s;\n",ea,ea);
			fprintf(opsFile,"\t\t\tcpu_regs.%s+=(int8_t)(cpu_regs.tmpW&0xFF);\n",ea);
			fprintf(opsFile,"\t\t\tcpu_regs.%s+=cpu_regs.PC;\n",ea);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x3C:		/// 111100
			switch (length)
			{
				case 1:
					fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC+1;\n",ea);
					fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
					break;
				case 2:
					fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
					fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
					break;
				case 4:
					fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.PC;\n",ea);
					fprintf(opsFile,"\t\t\tcpu_regs.PC+=4;\n");
					break;
			}
			break;
		default:
			printf("Unknown Effective Address Operand : %04X\n",operand);
			exit(-1);
    }
}

void OUTPUT_LOAD_EFFECTIVE_VALUE_REG(int len, char *dst, char *src)
{
	OUTPUT_OPCODE_END_STAGE(nxtStage+1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
	switch (len)
	{
		default:
			printf("Illegal length value for load %d\n",len);
			exit(-1);
		case 1:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = MEM_getByte(cpu_regs.%s);\n",dst,src);
			break;
		case 2:
			fprintf(opsFile,"\t\t\tcpu_regs.%s = MEM_getWord(cpu_regs.%s);\n",dst,src);
			break;
		case 4:
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL = cpu_regs.%s;\n",src);
			fprintf(opsFile,"\t\t\tcpu_regs.%s = MEM_getWord(cpu_regs.tmpL)<<16;\n",dst);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL+=2;\n",src);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.%s = cpu_regs.%s + MEM_getWord(cpu_regs.tmpL);\n",dst,dst);
			break;
	}
	OUTPUT_OPCODE_END_STAGE(nxtStage+1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
}

void OUTPUT_LOAD_EFFECTIVE_VALUE(int len,u_int16_t op,char *ea)
{
	if (op<0x10)
		return;
	
	OUTPUT_LOAD_EFFECTIVE_VALUE_REG(len,ea,ea);
}

void OUTPUT_STORE_EFFECTIVE_VALUE_REG(int len,char *dst,char *src)
{
	OUTPUT_OPCODE_END_STAGE(nxtStage+1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
	switch (len)
	{
		default:
			printf("Illegal length value for load %d\n",len);
			exit(-1);
		case 1:
			fprintf(opsFile,"\t\t\tMEM_setByte(cpu_regs.%s,cpu_regs.%s&0x%08X);\n",dst,src,zMask);
			break;
		case 2:
			fprintf(opsFile,"\t\t\tMEM_setWord(cpu_regs.%s,cpu_regs.%s&0x%08X);\n",dst,src,zMask);
			break;
		case 4:
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL = cpu_regs.%s;\n",dst);
			fprintf(opsFile,"\t\t\tMEM_setWord(cpu_regs.tmpL,(cpu_regs.%s&0x%08X)>>16);\n",src,zMask);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tMEM_setWord(cpu_regs.tmpL,cpu_regs.%s&0x%08X);\n",src,zMask);
			break;
	}
	OUTPUT_OPCODE_END_STAGE(nxtStage+1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
}

void OUTPUT_STORE_EFFECTIVE_VALUE(int len, u_int16_t op)
{
	OUTPUT_STORE_EFFECTIVE_VALUE_REG(len,"ead","eas");
}

int OUTPUT_OPCODE_SETUP_LENGTHM(u_int16_t op)
{
    switch(op)
    {
		default:
			printf("Unknown size specifier in instruction %d\n",op);
			exit(-1);
		case 0x01:
			iMask=0xFFFFFF00;
			nMask=0x80;
			zMask=0xFF;
			return 1;
		case 0x03:
			iMask=0xFFFF0000;
			nMask=0x8000;
			zMask=0xFFFF;
			return 2;
		case 0x02:
			iMask=0x00000000;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			return 4;
	}
	return 0;
}

int OUTPUT_OPCODE_SETUP_LENGTH(u_int16_t op)
{
    switch(op)
    {
		default:
			printf("Unknown size specifier in instruction %d\n",op);
			exit(-1);
		case 0x00:
			iMask=0xFFFFFF00;
			nMask=0x80;
			zMask=0xFF;
			return 1;
		case 0x01:
			iMask=0xFFFF0000;
			nMask=0x8000;
			zMask=0xFFFF;
			return 2;
		case 0x02:
			iMask=0x00000000;
			nMask=0x80000000;
			zMask=0xFFFFFFFF;
			return 4;
    }
	return 0;
}

int OUTPUT_OPCODE_SETUP_LENGTHB(u_int16_t op)
{
	if (op==0)
		return 2;
	return 1;
}

void OUTPUT_WRITE_ZN_TESTS(char *ea)
{
    fprintf(opsFile,"\t\t\tif (cpu_regs.%s & 0x%08X)\n",ea,nMask);
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR|=CPU_STATUS_N;\n");
    fprintf(opsFile,"\t\t\telse\n");
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR&=~CPU_STATUS_N;\n");
    fprintf(opsFile,"\t\t\tif (cpu_regs.%s & 0x%08X)\n",ea,zMask);
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR&=~CPU_STATUS_Z;\n");
    fprintf(opsFile,"\t\t\telse\n");
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR|=CPU_STATUS_Z;\n");
}

void OUTPUT_WRITE_XCV_SUB_TESTS(char *src,char *dst,char *res)
{
	fprintf(opsFile,"\t\t\tif (((cpu_regs.%s&0x%08X) & (~(cpu_regs.%s&0x%08X))) | ((cpu_regs.%s&0x%08X) & (~(cpu_regs.%s&0x%08X))) | ((cpu_regs.%s&0x%08X) & (cpu_regs.%s&0x%08X)))\n",src,nMask,dst,nMask,res,nMask,dst,nMask,src,nMask,res,nMask);
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR|=(CPU_STATUS_C|CPU_STATUS_X);\n");
	fprintf(opsFile,"\t\t\telse\n");
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR&=~(CPU_STATUS_C|CPU_STATUS_X);\n");
	fprintf(opsFile,"\t\t\tif (((~(cpu_regs.%s&0x%08X)) & (cpu_regs.%s&0x%08X) & (~(cpu_regs.%s&0x%08X))) | ((cpu_regs.%s&0x%08X) & (~(cpu_regs.%s&0x%08X)) & (cpu_regs.%s&0x%08X)))\n",src,nMask,dst,nMask,res,nMask,src,nMask,dst,nMask,res,nMask);
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR|=CPU_STATUS_V;\n");
	fprintf(opsFile,"\t\t\telse\n");
	fprintf(opsFile,"\t\t\t\tcpu_regs.SR&=~CPU_STATUS_V;\n");
}

void OUTPUT_OPCODE_D_WRITE(u_int16_t op,char *function)
{
	fprintf(opsFile,"\t\t\tcpu_regs.ead=cpu_regs.D[%d]&0x%08X;\n",op,zMask);
	fprintf(opsFile,"\t\t\tcpu_regs.ear=(cpu_regs.ead %s cpu_regs.eas)&0x%08X;\n",function,zMask);
	fprintf(opsFile,"\t\t\tcpu_regs.D[%d]&=0x%08X;\n",op,iMask);
	fprintf(opsFile,"\t\t\tcpu_regs.D[%d]|=cpu_regs.ear;\n",op);
}

void OUTPUT_OPCODE_A_WRITE(u_int16_t op,char *function)
{
	fprintf(opsFile,"\t\t\tcpu_regs.ead=cpu_regs.A[%d]&0x%08X;\n",op,zMask);
	fprintf(opsFile,"\t\t\tcpu_regs.ear=(cpu_regs.ead %s cpu_regs.eas)&0x%08X;\n",function,zMask);
	fprintf(opsFile,"\t\t\tcpu_regs.A[%d]&=0x%08X;\n",op,iMask);
	fprintf(opsFile,"\t\t\tcpu_regs.A[%d]|=cpu_regs.ear;\n",op);
}

void OUTPUT_OPCODE_MEM_READ_OP_WRITE(int len,char *function)
{
	OUTPUT_LOAD_EFFECTIVE_VALUE_REG(len,"eat","ead");
	fprintf(opsFile,"\t\t\tcpu_regs.ear=(cpu_regs.ead %s cpu_regs.eas)&0x%08X;\n",function,zMask);
	OUTPUT_STORE_EFFECTIVE_VALUE_REG(len,"ead","ear");
	fprintf(opsFile,"\t\t\tcpu_regs.ead=cpu_regs.eat;\n");
}

////// OPCODE HANDLERS ///////

// LEA 2
void CPU_LEA(u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	OUTPUT_OPCODE_START("LEA",opcode);

	OUTPUT_OPCODE_START_STAGE(nxtStage);
	OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(op2,4,"eas");
	fprintf(opsFile,"\t\t\tcpu_regs.A[%d]=cpu_regs.eas;\n",op1);
	OUTPUT_OPCODE_END_STAGE(0);

	OUTPUT_OPCODE_END();
}

// MOVE 2
void CPU_MOVE(u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	u_int16_t t1,t2;

	t1 = (op2 & 0x38)>>3;		// move operands reversed
	t2 = (op2 & 0x07)<<3;
	op2 = t1|t2;

	OUTPUT_OPCODE_START("MOVE",opcode);
	len=OUTPUT_OPCODE_SETUP_LENGTHM(op1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
	OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(op3,len,"eas");
	OUTPUT_LOAD_EFFECTIVE_VALUE(len,op3,"eas");
	
	if ((op2 & 0x38)==0)	// destination is D register
	{
		fprintf(opsFile,"\t\t\tcpu_regs.D[%d]&=0x%08x;\n",op2,iMask);
		fprintf(opsFile,"\t\t\tcpu_regs.D[%d]|=(cpu_regs.eas&0x%08x);\n",op2,zMask);
	}
	else
	{
		OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(op2,len,"ead");

		OUTPUT_STORE_EFFECTIVE_VALUE(len,op2);
    }
	
    fprintf(opsFile,"\t\t\tcpu_regs.SR&=~(CPU_STATUS_V|CPU_STATUS_C);\n");

	OUTPUT_WRITE_ZN_TESTS("eas");

	OUTPUT_OPCODE_END_STAGE(0);

	OUTPUT_OPCODE_END();
}

// SUBQ 2
void CPU_SUBQ(u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
    int len;
	
	if (op1==0)
		op1=8;

	OUTPUT_OPCODE_START("SUBQ",opcode);
	len=OUTPUT_OPCODE_SETUP_LENGTH(op2);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
	
	fprintf(opsFile,"\t\t\tcpu_regs.eas=0x%08x;\n",op1&zMask);

	if (len==4 || (op3&0x38)==0x08)
	{
		if (op3>0x10 && len==4)
		{
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);	// Address register,.L and memory cost extra cycles
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);		
		}
	}
	
    if ((op3 & 0x38)==0)	// destination is D register
    {
		if (len==4)
		{
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);	// .L costs extra cycles (not sure why at this point)
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
		}

		OUTPUT_OPCODE_D_WRITE(op3,"-");
    }
    else
    {
		if ((op3 & 0x38)==0x08)
		{
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);	// Address register cost extra cycles
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			if (len==1)
			{
				printf("Invalid Length Specifier on operation %d\n",len);
				exit(-1);
			}
			OUTPUT_OPCODE_A_WRITE(op3&0x07,"-");
		}
		else
		{
			OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(op3,len,"ead");

			OUTPUT_OPCODE_MEM_READ_OP_WRITE(len,"-");
		}
    }
	
	if ((op3 & 0x38)!=0x08)			// Address register direct does not affect flags
	{
		OUTPUT_WRITE_ZN_TESTS("ead");
		OUTPUT_WRITE_XCV_SUB_TESTS("eas","ead","ear");
	}
	OUTPUT_OPCODE_END_STAGE(0);

	OUTPUT_OPCODE_END();
}

// BCC 4 + 1 if taken +2 for .w
void CPU_BCC(u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	int len;
	u_int32_t	tmp;
	
	len = OUTPUT_OPCODE_SETUP_LENGTHB(op2);
	
	OUTPUT_OPCODE_START_CC("B",opcode,op1,0);
	
	fprintf(opsFile,"\t\t\tcpu_regs.ead=cpu_regs.PC;\n");

    if (len==2)
	{
		OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(0x38,2,"eas");
	}
	else
	{
		if (op2&0x80)
		{
			tmp=0xFFFFFF00 + op2;
		}
		else
		{
			tmp=op2;
		}
		fprintf(opsFile,"\t\t\tcpu_regs.eas=0x%08x;\n",tmp);

	}

	OUTPUT_OPCODE_END_STAGE(nxtStage+1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);
	fprintf(opsFile,"\t\t\tif (cpu_regs.tmpW)\n");
	fprintf(opsFile,"\t\t\t\treturn %d;\n",nxtStage+1);
	OUTPUT_OPCODE_END_STAGE(nxtStage+2);
	OUTPUT_OPCODE_START_STAGE(nxtStage-1);		
	fprintf(opsFile,"\t\t\tcpu_regs.PC=cpu_regs.ead+cpu_regs.eas;\n");
	OUTPUT_OPCODE_END_STAGE(nxtStage+1);
	OUTPUT_OPCODE_START_STAGE(nxtStage);		
	OUTPUT_OPCODE_END_STAGE(0);

	OUTPUT_OPCODE_END();
}

