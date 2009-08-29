/*
 *  cpu_builder_dissasemble.h
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

/* Defines functions useful for creating the dissasembly code - included directly to save prototyping everything */

////// DISSASEMBLE HELPERS ////////

void OUTPUT_DISSASEMBLE_START(const char *name,u_int16_t oode)
{
	fprintf(disFile,"\nint DIS_%s_%04X(u_int32_t adr)\n",name,oode);
	fprintf(disFile,"{\n");
	fprintf(disFile,"\tint\tinsLength=0;\n");
	fprintf(disFile,"\n");
	fprintf(disFile,"\tstrcpy(mnemonicData,\"%s \");\n",name);
}

void OUTPUT_DISSASEMBLE_EFFECTIVE_ADDRESS(u_int16_t operand,int length)
{
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
			fprintf(disFile,"\tstrcat(mnemonicData,\"D%d\");\n",operand);
			break;
		case 0x08:	/// 001rrr
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			fprintf(disFile,"\tstrcat(mnemonicData,\"A%d\");\n",operand-0x08);
			break;
		case 0x10:	/// 010rrr
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			fprintf(disFile,"\tstrcat(mnemonicData,\"(A%d)\");\n",operand-0x10);
			break;
		case 0x18:	/// 011rrr
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
			fprintf(disFile,"\tstrcat(mnemonicData,\"(A%d)+\");\n",operand-0x18);
			break;
		case 0x20:	// 100rrr
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			fprintf(disFile,"\tstrcat(mnemonicData,\"-(A%d)\");\n",operand-0x20);
			break;
		case 0x28:	// 101rrr
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(#%%04X,A%d)\",MEM_getWord(adr));\n",operand-0x28);
			fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
			break;
		case 0x30:	// 110rrr
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			fprintf(disFile,"\t{\n");
			fprintf(disFile,"\t\tu_int16_t\ttmp;\n");
			fprintf(disFile,"\n");
			fprintf(disFile,"\t\ttmp=MEM_getWord(adr);\n");
			fprintf(disFile,"\t\tif (tmp&0x8000)\n");
			fprintf(disFile,"\t\t{\n");
			fprintf(disFile,"\t\t\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(#%%02X,A%%d%%s,A%d)\",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? \".L\" : \".W\");\n",operand-0x30);
			fprintf(disFile,"\t\t}\n");
			fprintf(disFile,"\t\telse\n");
			fprintf(disFile,"\t\t{\n");
			fprintf(disFile,"\t\t\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(#%%02X,D%%d%%s,A%d)\",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? \".L\" : \".W\");\n",operand-0x30);
			fprintf(disFile,"\t\t}\n");
			fprintf(disFile,"\t}\n");
			fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
			break;
		case 0x38:		/// 111000
			fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(%%04X).W\",MEM_getWord(adr));\n");
			fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
			break;
		case 0x39:		/// 111001
			fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(%%08X).L\",MEM_getLong(adr));\n");
			fprintf(disFile,"\tadr+=4;\n\tinsLength+=4;\n");
			break;
		case 0x3A:		/// 111010
			fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(,%%04X)\",MEM_getWord(adr));\n");
			fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
			break;
		case 0x3B:		/// 111100
			fprintf(disFile,"\t{\n");
			fprintf(disFile,"\t\tu_int16_t\ttmp;\n");
			fprintf(disFile,"\n");
			fprintf(disFile,"\t\ttmp=MEM_getWord(adr);\n");
			fprintf(disFile,"\t\tif (tmp&0x8000)\n");
			fprintf(disFile,"\t\t{\n");
			fprintf(disFile,"\t\t\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(#%%02X,A%%d%%s,)\",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? \".L\" : \".W\");\n");
			fprintf(disFile,"\t\t}\n");
			fprintf(disFile,"\t\telse\n");
			fprintf(disFile,"\t\t{\n");
			fprintf(disFile,"\t\t\tsprintf(&mnemonicData[strlen(mnemonicData)],\"(#%%02X,D%%d%%s,)\",tmp&0xFF,(tmp>>12)&0x7,(tmp&0x0800) ? \".L\" : \".W\");\n");
			fprintf(disFile,"\t\t}\n");
			fprintf(disFile,"\t}\n");
			fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
			break;
		case 0x3C:		/// 111100
			switch (length)
			{
			case 1:
				fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"#%%02X\",MEM_getByte(adr+1));\n");
				fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
				break;
			case 2:
				fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"#%%04X\",MEM_getWord(adr));\n");
				fprintf(disFile,"\tadr+=2;\n\tinsLength+=2;\n");
				break;
			case 4:
				fprintf(disFile,"\tsprintf(&mnemonicData[strlen(mnemonicData)],\"#%%08X\",MEM_getLong(adr));\n");
				fprintf(disFile,"\tadr+=4;\n\tinsLength+=4;\n");
				break;
			default:
				printf("Illegal Effective Address Operand Length : %04X\n",length);
				exit(-1);
			}
			break;
		default:
			printf("Unknown Effective Address Operand : %04X\n",operand);
			exit(-1);
    }
}

void OUTPUT_DISSASEMBLE_OPERAND(char *name, u_int16_t op)
{
	char tmp[256];
	sprintf(tmp,name,op);
	fprintf(disFile,"\tstrcat(mnemonicData,\"%s\");\n",tmp);
}

void OUTPUT_DISSASEMBLE_END()
{
	fprintf(disFile,"\n");
	fprintf(disFile,"\treturn insLength;\n");
	fprintf(disFile,"}\n");
}

////// DISSASEMBLE HANDLERS ///////


void CPU_DIS_LEA(u_int32_t ignored,u_int16_t oode,u_int16_t op1,u_int16_t op2,u_int16_t op3,u_int16_t op4,u_int16_t op5,u_int16_t op6,u_int16_t op7,u_int16_t op8)
{
	OUTPUT_DISSASEMBLE_START("LEA",oode);

	OUTPUT_DISSASEMBLE_EFFECTIVE_ADDRESS(op2,4);
	OUTPUT_DISSASEMBLE_OPERAND(",A%d",op1);

	OUTPUT_DISSASEMBLE_END();
}

