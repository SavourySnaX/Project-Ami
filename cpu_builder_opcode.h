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

void OUTPUT_OPCODE_START(const char *name,u_int16_t opcode)
{
	fprintf(opsFile,"\nu_int32_t %s_%04X(u_int32_t stage)\n",name,opcode);
	fprintf(opsFile,"{\n");
	fprintf(opsFile,"\tswitch (stage)\n");
	fprintf(opsFile,"\t{\n");
	nxtStage=1;
}

void OUTPUT_OPCODE_END()
{
	fprintf(opsFile,"\t}\n");
	fprintf(opsFile,"\treturn 0;\n");
	fprintf(opsFile,"}\n");
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

void OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(u_int16_t operand,int length)
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
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.D[%d];\n",operand);
			break;
		case 0x08:		// A?
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.A[%d];\n",operand-0x08);
			break;
		case 0x10:		// (A?)
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.A[%d];\n",operand-0x10);
			break;
		case 0x18:		// (A?)+
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
/*
			if ((operand==0x1F) && (length==1))
			{
				//startDebug=1;
				length=2;
			}
			ea = cpu_regs.A[operand-0x18];
			cpu_regs.A[operand-0x18]+=length;*/
			break;
		case 0x20:		// -(A?)
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
/*			if ((operand==0x27) && (length==1))
			{
				//startDebug=1;
				length=2;
			}
			cpu_regs.A[operand-0x20]-=length;
			ea = cpu_regs.A[operand-0x20];*/
			break;
		case 0x28:		// (XXXX,A?)
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea=cpu_regs.A[%d]+(int16_t)MEM_getWord(cpu_regs.ea);\n",operand-0x28);
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
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpW=MEM_getWord(cpu_regs.ea);\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (cpu_regs.tmpW&0x8000)\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.ea = cpu_regs.A[(cpu_regs.tmpW>>12)&0x07];\n");
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			fprintf(opsFile,"\t\t\t}\n");
			fprintf(opsFile,"\t\t\telse\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.ea = cpu_regs.D[(cpu_regs.tmpW>>12)&0x07];\n");
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\t}\n");
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (!(cpu_regs.tmpW&0x0800)) cpu_regs.ea=(int16_t)cpu_regs.ea;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.ea+=(int8_t)(cpu_regs.tmpW&0xFF);\n");
			fprintf(opsFile,"\t\t\tcpu_regs.ea+=cpu_regs.A[%d];\n",operand-0x30);
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x38:		// (XXXX).W
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea=(int16_t)MEM_getWord(cpu_regs.ea);\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x39:		// (XXXXXXXX).L
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea=MEM_getWord(cpu_regs.tmpL)<<16;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpL = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea|=MEM_getWord(cpu_regs.tmpL);\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x3A:		// (XXXX,PC)
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea+=(int16_t)MEM_getWord(cpu_regs.ea);\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x3B:		/// (XX,PC,X?)
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.ea = cpu_regs.PC;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.PC+=2;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tcpu_regs.tmpW=MEM_getWord(cpu_regs.ea);\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (cpu_regs.tmpW&0x8000)\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.ea = cpu_regs.A[(cpu_regs.tmpW>>12)&0x07];\n");
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			fprintf(opsFile,"\t\t\t}\n");
			fprintf(opsFile,"\t\t\telse\n");
			fprintf(opsFile,"\t\t\t{\n");
			fprintf(opsFile,"\t\t\t\tcpu_regs.ea = cpu_regs.D[(cpu_regs.tmpW>>12)&0x07];\n");
			fprintf(opsFile,"\t");
			OUTPUT_OPCODE_END_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\t}\n");
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			fprintf(opsFile,"\t\t\tif (!(cpu_regs.tmpW&0x0800)) cpu_regs.ea=(int16_t)cpu_regs.ea;\n");
			fprintf(opsFile,"\t\t\tcpu_regs.ea+=(int8_t)(cpu_regs.tmpW&0xFF);\n");
			fprintf(opsFile,"\t\t\tcpu_regs.ea+=cpu_regs.PC;\n");
			OUTPUT_OPCODE_END_STAGE(nxtStage+1);
			OUTPUT_OPCODE_START_STAGE(nxtStage);
			break;
		case 0x3C:		/// 111100
/*			switch (length)
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
			}*/
			break;
		default:
			printf("Unknown Effective Address Operand : %04X\n",operand);
			exit(-1);
    }
}

////// OPCODE HANDLERS ///////


void CPU_LEA(u_int16_t opcode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	OUTPUT_OPCODE_START("LEA",opcode);

	OUTPUT_COMPUTE_EFFECTIVE_ADDRESS(op2,4);
	fprintf(opsFile,"\t\t\tcpu_regs.A[%d]=cpu_regs.ea;\n",op1);
	OUTPUT_OPCODE_END_STAGE(0);

	OUTPUT_OPCODE_END();
}
