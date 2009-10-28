/*
 *  debugger.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#if ENABLE_DEBUGGER

#include <OpenGL/OpenGL.h>

#include "/usr/local/include/GL/glfw.h"

#include "cpu.h"
#include "cpu_dis.h"
#include "debugger.h"
#include "display.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"
#include "sprite.h"

#include "font.h"

#define MAX_BPS	20
u_int32_t bpAddresses[MAX_BPS]={0x3CFA};//0xFFFFFFFF;//0x00087dc8;
int numBps=0;

int copNum=0;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo);

void DrawChar(u_int32_t x,u_int32_t y, char c,int cMask1,int cMask2)
{
	int a,b;
	unsigned char *fontChar=&FontData[c*6*8];
	
	x*=8;
	y*=8;
	
	for (a=0;a<8;a++)
	{
		for (b=0;b<6;b++)
		{
			doPixel(x+b+1,y+a,(*fontChar) * cMask1,(*fontChar) * cMask2);
			fontChar++;
		}
	}
}

void PrintAt(int cMask1,int cMask2,u_int32_t x,u_int32_t y,const char *msg,...)
{
	static char tStringBuffer[32768];
	char *pMsg=tStringBuffer;
	va_list args;

	va_start (args, msg);
	vsprintf (tStringBuffer,msg, args);
	va_end (args);

	while (*pMsg)
	{
		DrawChar(x,y,*pMsg,cMask1,cMask2);
		x++;
		pMsg++;
	}
}

void DisplayWindow(u_int32_t x,u_int32_t y, u_int32_t w, u_int32_t h)
{
	u_int32_t xx,yy;
	x*=8;
	y*=8;
	w*=8;
	h*=8;
	w+=x;
	h+=y;
	for (yy=y;yy<h;yy++)
	{
		for (xx=x;xx<w;xx++)
		{
			doPixel(xx,yy,0,0);
		}
	}
}

void ShowCPUState()
{
	DisplayWindow(0,0,66,10);
    PrintAt(0x0F,0xFF,1,1," D0=%08X  D1=%08X  D2=%08x  D3=%08x",cpu_regs.D[0],cpu_regs.D[1],cpu_regs.D[2],cpu_regs.D[3]);
    PrintAt(0x0F,0xFF,1,2," D4=%08X  D5=%08X  D6=%08x  D7=%08x",cpu_regs.D[4],cpu_regs.D[5],cpu_regs.D[6],cpu_regs.D[7]);
    PrintAt(0x0F,0xFF,1,3," A0=%08X  A1=%08X  A2=%08x  A3=%08x",cpu_regs.A[0],cpu_regs.A[1],cpu_regs.A[2],cpu_regs.A[3]);
    PrintAt(0x0F,0xFF,1,4," A4=%08X  A5=%08X  A6=%08x  A7=%08x",cpu_regs.A[4],cpu_regs.A[5],cpu_regs.A[6],cpu_regs.A[7]);
    PrintAt(0x0F,0xFF,1,5,"USP=%08X ISP=%08x\n",cpu_regs.USP,cpu_regs.ISP);
    PrintAt(0x0F,0xFF,1,7,"          [ T1:T0: S: M:  :I2:I1:I0:  :  :  : X: N: Z: V: C ]");
    PrintAt(0x0F,0xFF,1,8," SR=%04X  [ %s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s ]", cpu_regs.SR, 
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
}

u_int32_t GetOpcodeLength(u_int32_t address)
{
	u_int16_t	opcode;
	u_int16_t	operands[8];
	u_int32_t	insCount;
	int32_t	a;

	opcode = MEM_getWord(address);

	if (CPU_Information[opcode])
	{
		for (a=0;a<CPU_Information[opcode]->numOperands;a++)
		{
			operands[a] = (opcode & CPU_Information[opcode]->operandMask[a]) >> 
									CPU_Information[opcode]->operandShift[a];
		}
	}
			
	insCount=CPU_DisTable[opcode](address+2,operands[0],operands[1],operands[2],
						operands[3],operands[4],operands[5],operands[6],operands[7]);

	return insCount;
}

u_int32_t DissasembleAddress(u_int32_t x,u_int32_t y,u_int32_t address,int cursor)
{
	u_int32_t	insCount;
	int32_t	a;
	u_int32_t b;
	int cMask1=0x0F,cMask2=0xFF;
	
	insCount=GetOpcodeLength(address);		// note this also does the dissasemble

	for (a=0;a<numBps;a++)
	{
		if (address == bpAddresses[a])
		{
			cMask1=0x0F;
			cMask2=0x00;
			break;
		}
	}

	if (cursor)
	{
	PrintAt(cMask1,cMask2,x,y,"%08X >",address);
	}
	else
	{
	PrintAt(cMask1,cMask2,x,y,"%08X ",address);
	}
	
	for (b=0;b<(insCount+2)/2;b++)
	{
		PrintAt(cMask1,cMask2,x+10+b*5,y,"%02X%02X ",MEM_getByte(address+b*2+0),MEM_getByte(address+b*2+1));
	}
			
	PrintAt(cMask1,cMask2,x+30,y,"%s",mnemonicData);
	
	return insCount+2;
}

extern int g_newScreenNotify;

int g_pause=0;
u_int32_t stageCheck=0;


int CheckKey(int key);
void ClearKey(int key);

void DisplayHelp()
{
	DisplayWindow(84,0,30,20);

	PrintAt(0x0F,0xFF,85,1,"T - Step Instruction");
	PrintAt(0x0F,0xFF,85,2,"H - Step hardware cycle");
	PrintAt(0x0F,0xFF,85,3,"G - Toggle Run in debugger");
	PrintAt(0x0F,0xFF,85,4,"<space> - Toggle Breakpoint");
	PrintAt(0x0F,0xFF,85,5,"<up/dn> - Move cursor");
	PrintAt(0x0F,0xFF,85,6,"M - Switch to cpu debug");
	PrintAt(0x0F,0xFF,85,7,"C - Show active copper");
	PrintAt(0x0F,0xFF,85,8,"P - Show cpu history");
}

void DisplayCustomRegs()
{
	int x,y;
	static char buffer[256];

	DisplayWindow(0,31,14*8+1,512/16+2);
	
	for (y=0;y<512/16;y++)
	{
		for (x=0;x<8;x++)
		{
			MEM_GetHardwareDebug(y*8+x,buffer);
			PrintAt(0x0F,0xFF,x*14+1, y+32, buffer);
		}
	}
}

int dbMode=0;

void DecodeCopper(int cpReg,int offs)
{
	u_int32_t copperAddress;
	int a;

	if (cpReg==0)
		copperAddress = CST_GETLNGU(CST_COP1LCH,CUSTOM_CHIP_RAM_MASK);
	else
		copperAddress = CST_GETLNGU(CST_COP2LCH,CUSTOM_CHIP_RAM_MASK);

	DisplayWindow(0,0,14*8+1,31+34);

	copperAddress+=4*offs;

	for (a=0;a<32+31;a++)
	{
		u_int16_t ins1 = MEM_getWord(copperAddress);
		u_int16_t ins2 = MEM_getWord(copperAddress+2);

		if (ins1&0x0001)
		{
			if (ins2&0x0001)
			{
				// Skip
				PrintAt(0x0F,0xFF,1, a+1, "%08X    Skip : %04X:%04X\n",copperAddress,ins1,ins2);
			}
			else
			{
				// Wait

				u_int8_t maskv=0x80|((ins2>>8)&0x7F);
				u_int8_t maskh=(ins2&0xFE);
				u_int8_t vpos=ins1>>8;
				u_int8_t hpos=ins1&0xFE;
					
				PrintAt(0x0F,0xFF,1, a+1, "%08X    Wait : %04X:%04X\n -- VPOS&%02X >= %02X  -- HPOS&%02X >= %02X",copperAddress,ins1,ins2,maskv,vpos,maskh,hpos);
			}
		}
		else
		{
			// Move
			PrintAt(0x0F,0xFF,1, a+1, "%08X    Move : %04X:%04X  = %04X -> %08X (%s)\n",copperAddress,ins1,ins2,ins2,0xDFF000 + (ins1&0x01FE),MEM_GetHardwareName((ins1&0x01FE)/2));
		}

		copperAddress+=4;
	}
}

u_int32_t lastPC;
#define PCCACHESIZE	1000

u_int32_t	pcCache[PCCACHESIZE];
u_int32_t	cachePos=0;

void DecodePCHistory(int offs)
{
	int a;
	DisplayWindow(0,0,14*8+1,31+34);

	for (a=0;a<32+31;a++)
	{
		if (a+offs < PCCACHESIZE)
		{
			PrintAt(0x0F,0xFF,1,1+a,"%d : PC History : %08X\n",a+offs,pcCache[a+offs]);
		}
	}
}

int cpOffs = 0;
int cpuOffs = 0;
int hisOffs = 0;
			
int bpAt=0xFFFFFFFF;

void BreakpointModify(u_int32_t address)
{
	int a;
	int b;

	for (a=0;a<numBps;a++)
	{
		if (address==bpAddresses[a])
		{
			// Remove breakpoint
			numBps--;

			for (b=a;b<numBps;b++)
			{
				bpAddresses[b]=bpAddresses[b+1];
			}
			return;
		}
	}

	bpAddresses[numBps]=address;
	numBps++;
}


void DisplayDebugger()
{
	if (g_pause || (stageCheck==3))
	{
		u_int32_t a;
		u_int32_t address = cpu_regs.lastInstruction;
		
		if (cpu_regs.stage==0)
			address=cpu_regs.PC;

		if (dbMode==0)
		{
			ShowCPUState();
		
			DisplayHelp();
		
			DisplayCustomRegs();
		
			DisplayWindow(0,19,90,12);
			for (a=0;a<10;a++)
			{
				if (bpAt==a)
				{
					BreakpointModify(address);
					bpAt=0xFFFFFFFF;
				}

				address+=DissasembleAddress(1,20+a,address,cpuOffs==a);
			}
		}
		if (dbMode==1)
		{
			DecodeCopper(copNum,cpOffs);
		}
		if (dbMode==2)
		{
			DecodePCHistory(hisOffs);
		}
		
		g_newScreenNotify=1;
	}
}

int UpdateDebugger()
{
	int a;

	if (cpu_regs.stage==0)
	{
		lastPC=cpu_regs.PC;

		if (cachePos<PCCACHESIZE)
		{
			if (pcCache[cachePos]!=lastPC)
			{
				pcCache[cachePos++]=lastPC;
			}
		}
		else
		{
			if (pcCache[PCCACHESIZE-1]!=lastPC)
			{
				memmove(pcCache,pcCache+1,(PCCACHESIZE-1)*sizeof(u_int32_t));
			
				pcCache[PCCACHESIZE-1]=lastPC;
			}
		}
		
		for (a=0;a<numBps;a++)
		{
			if ((bpAddresses[a]==cpu_regs.PC))
			{
				g_pause=1;
			}
		}
	}
	
	if (CheckKey(GLFW_KEY_PAGEUP))
		g_pause=!g_pause;

	if (stageCheck)
	{
		if (stageCheck==1)
		{
			if (cpu_regs.stage==0)
			{
				g_pause=1;		// single step cpu
				stageCheck=0;
			}
		}
		if (stageCheck==2)
		{
			g_pause=1;			// single step hardware
			stageCheck=0;
		}
	}

	if (g_pause || (stageCheck==3))
	{
		if (CheckKey('P'))
			dbMode=2;
		if (dbMode==2)
		{
		if (CheckKey(GLFW_KEY_UP))
		{
			hisOffs-=32;
			if (hisOffs<0)
				hisOffs=0;
		}
		if (CheckKey(GLFW_KEY_DOWN))
		{
			hisOffs+=32;
			if (hisOffs>=PCCACHESIZE)
				hisOffs=PCCACHESIZE-1;
		}
		}
		if (CheckKey('C'))
			dbMode=1;
		if (dbMode==1)
		{
		if (CheckKey(GLFW_KEY_UP) && cpOffs>0)
			cpOffs--;
		if (CheckKey(GLFW_KEY_DOWN))
			cpOffs++;
		if (CheckKey(GLFW_KEY_LEFT))
		{
			copNum++;
			copNum&=1;
		}
		}
		if (CheckKey('M'))
			dbMode=0;
		if (dbMode==0)
		{
		if (CheckKey(GLFW_KEY_UP) && cpuOffs>0)
			cpuOffs--;
		if (CheckKey(GLFW_KEY_DOWN) && cpuOffs<9)
			cpuOffs++;
		if (CheckKey(' '))
			bpAt=cpuOffs;
		}
		// While paused - enable debugger keys
		if (CheckKey('T'))
		{
			// cpu step into instruction
			stageCheck=1;

			g_pause=0;
		}
		if (CheckKey('H'))
		{
			stageCheck=2;
			g_pause=0;
		}
		if (CheckKey('G'))
		{
			if (stageCheck==3)
			{
				stageCheck=0;
				g_pause=1;
			}
			else
			{
				stageCheck=3;
				g_pause=0;
			}
		}
		ClearKey(' ');
		ClearKey(GLFW_KEY_LEFT);
		ClearKey(GLFW_KEY_UP);
		ClearKey(GLFW_KEY_DOWN);
		ClearKey('P');
		ClearKey('C');
		ClearKey('M');
		ClearKey('T');
		ClearKey('H');
		ClearKey('G');
	}
	ClearKey(GLFW_KEY_PAGEUP);
	
	return g_pause;
}

void DEB_PauseEmulation(char *reason)
{
	g_pause=1;
	printf("Invoking debugger due to %s\n",reason);
}

#endif
