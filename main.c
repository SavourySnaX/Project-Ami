/* 
 
 * Mac OS X main file (I`ll need to sort the other platforms out later)


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

#define AMI_LINE_LENGTH ((228)*4)		// 228 / 227 alternating
#define WIDTH AMI_LINE_LENGTH
#define HEIGHT (262*2)
#define BPP 4
#define DEPTH 32

#include "config.h"

#include <OpenGL/OpenGL.h>

#include "/usr/local/include/GL/glfw.h"

#include "cpu.h"
#include "memory.h"
#include "customchip.h"
#include "ciachip.h"
#include "copper.h"
#include "blitter.h"
#include "display.h"
#include "disk.h"
#include "sprite.h"
#include "keyboard.h"
#include "debugger.h"

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
	
    inRom = fopen("../../204.rom","rb");
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
	
    inKey = fopen("../../rom.key","rb");
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
	
    outData = fopen("../../out24.rom","wb");
    if (!outData)
    {
		printf("FAIL\n");
		return -1;
    }
	
    for (a=0;a<romSize;a++)
    {
		b = romData[a] ^ keyData[a%keySize];
		fputc(b,outData);
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
	return 0;
    }
    fseek(inRom,0,SEEK_END);
    romSize = ftell(inRom);
	fseek(inRom,0,SEEK_SET);
    romData = (unsigned char *)malloc(romSize);
    if (romSize != fread(romData,1,romSize,inRom))
	{
		fclose(inRom);
		return 0;
	}
    fclose(inRom);
	
	return romData;
}

extern int startDebug;

void CPU_runTests()
{
	FILE *inBin;
	unsigned long inSize;
	unsigned long a;
	unsigned int misCount=0;
	
    inBin = fopen("../../test1_22c18.bin","rb");
    fseek(inBin,0,SEEK_END);
    inSize = ftell(inBin)-1;
	fseek(inBin,0,SEEK_SET);
    fread(&chpPtr[0x22c18],1,inSize,inBin);
    fclose(inBin);

    inBin = fopen("../../test2_25cb0.bin","rb");
    fseek(inBin,0,SEEK_END);
    inSize = ftell(inBin)-1;
	fseek(inBin,0,SEEK_SET);
    fread(&chpPtr[0x25cb0],1,inSize,inBin);
    fclose(inBin);
	
	MEM_MapKickstartLow(0);

//	startDebug=1;

	cpu_regs.SR=0x0000;
	cpu_regs.PC=0x22c18;
	cpu_regs.A[7]=0x10000;
	
	while (cpu_regs.PC!=0x22c1e)
	{
		CPU_Step();
	}

    inBin = fopen("../../test3_31fb0.bin","rb");
    fseek(inBin,0,SEEK_END);
    inSize = ftell(inBin)-1;
	fseek(inBin,0,SEEK_SET);
    fread(&chpPtr[0x20000],1,inSize,inBin);				// load results to different address and then compare
    fclose(inBin);
	
	for (a=0;a<inSize;a++)
	{
		if (chpPtr[0x20000+a]!=chpPtr[0x31fb0+a])
		{
			printf("CPU mismatch at : %08x %02x!=%02x\n",(u_int32_t)(0x31fb0+a),chpPtr[0x20000+a],chpPtr[0x31fb0+a]);
			misCount++;
		}
	}
	
	printf("And we are done with %d faults\n",misCount);
	printf("oh well\n");
}

u_int8_t videoMemory[AMI_LINE_LENGTH*HEIGHT*sizeof(u_int32_t)];

int g_newScreenNotify = 0;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo)
{
	u_int32_t *pixmem32;
	u_int32_t colour;
	u_int8_t r = (colHi&0x0F)<<4;
	u_int8_t g = (colLo&0xF0);
	u_int8_t b = (colLo&0x0F)<<4;

	if (y>=HEIGHT || x>=AMI_LINE_LENGTH)
		return;

	colour = (r<<16) | (g<<8) | (b<<0);
	pixmem32 = &((u_int32_t*)videoMemory)[y*AMI_LINE_LENGTH + x];
	*pixmem32 = colour;
}

void DrawScreen() 
{
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, 1);
	
	// glTexSubImage2D is faster when not using a texture range
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0, AMI_LINE_LENGTH, HEIGHT, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoMemory);
	//glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, IMAGE_SIZE, IMAGE_SIZE, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image[draw_image]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	
	glTexCoord2f(0.0f, HEIGHT);
	glVertex2f(-1.0f, -1.0f);
	
	glTexCoord2f(AMI_LINE_LENGTH, HEIGHT);
	glVertex2f(1.0f, -1.0f);
	
	glTexCoord2f(AMI_LINE_LENGTH, 0.0f);
	glVertex2f(1.0f, 1.0f);
	glEnd();
	
	glFlush();
}

void setupGL(int w, int h) 
{
    //Tell OpenGL how to convert from coordinates to pixel values
    glViewport(0, 0, w, h);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glClearColor(1.0f, 0.f, 1.0f, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_EXT);
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, 1);
	
	glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
	
	glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE , GL_STORAGE_CACHED_APPLE);
	glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	
	glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, AMI_LINE_LENGTH,
				 HEIGHT, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoMemory);
	
	glDisable(GL_DEPTH_TEST);
}

u_int8_t keyArray[512*3];

int CheckKey(int key)
{
	return keyArray[key*3+2];
}

void ClearKey(int key)
{
	keyArray[key*3+2]=0;
}

int doNewOpcodeDebug=0;

void GLFWCALL kbHandler( int key, int action )
{
	keyArray[key*3 + 0]=keyArray[key*3+1];
	keyArray[key*3 + 1]=action;
	keyArray[key*3 + 2]|=(keyArray[key*3+0]==GLFW_RELEASE)&&(keyArray[key*3+1]==GLFW_PRESS);

	if (action==GLFW_PRESS)
		action=0;
	else
		action=1;

	switch (key)
	{
//		case GLFW_KEY_KP_MULTIPLY:					NOTE NUMPAD ON PC KB IS VERY DIFFERENT TO AMIGA : TODO
//			KBD_AddKeyEvent(0xB6 + action);
//			break;
		case GLFW_KEY_TAB:
			KBD_AddKeyEvent(0x84 + action);
			break;
		case '`':
			KBD_AddKeyEvent(0x00 + action);
			break;
		case GLFW_KEY_ESC:
			KBD_AddKeyEvent(0x8A + action);
			break;
//		case GLFW_KEY_KP_ADD:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'Z':
			KBD_AddKeyEvent(0x62 + action);
			break;
		case 'A':
			KBD_AddKeyEvent(0x40 + action);
			break;
		case 'Q':
			KBD_AddKeyEvent(0x20 + action);
			break;
		case '1':
			KBD_AddKeyEvent(0x02 + action);
			break;
//		case KP_OPENBRACK:
//			KBD_AddKeyEvent(0xB4 + action);
//			break;
//		case GLFW_KEY_KP_9:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'X':
			KBD_AddKeyEvent(0x64 + action);
			break;
		case 'S':
			KBD_AddKeyEvent(0x42 + action);
			break;
		case 'W':
			KBD_AddKeyEvent(0x22 + action);
			break;
		case '2':
			KBD_AddKeyEvent(0x04 + action);
			break;
		case GLFW_KEY_F1:
			KBD_AddKeyEvent(0xA0 + action);
			break;
//		case GLFW_KEY_KP_6:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'C':
			KBD_AddKeyEvent(0x66 + action);
			break;
		case 'D':
			KBD_AddKeyEvent(0x44 + action);
			break;
		case 'E':
			KBD_AddKeyEvent(0x24 + action);
			break;
		case '3':
			KBD_AddKeyEvent(0x06 + action);
			break;
		case GLFW_KEY_F2:
			KBD_AddKeyEvent(0xA2 + action);
			break;
//		case GLFW_KEY_KP_3:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'V':
			KBD_AddKeyEvent(0x68 + action);
			break;
		case 'F':
			KBD_AddKeyEvent(0x46 + action);
			break;
		case 'R':
			KBD_AddKeyEvent(0x26 + action);
			break;
		case '4':
			KBD_AddKeyEvent(0x08 + action);
			break;
		case GLFW_KEY_F3:
			KBD_AddKeyEvent(0xA4 + action);
			break;
//		case GLFW_KEY_KP_.:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'B':
			KBD_AddKeyEvent(0x6A + action);
			break;
		case 'G':
			KBD_AddKeyEvent(0x48 + action);
			break;
		case 'T':
			KBD_AddKeyEvent(0x28 + action);
			break;
		case '5':
			KBD_AddKeyEvent(0x0A + action);
			break;
		case GLFW_KEY_F4:
			KBD_AddKeyEvent(0xA6 + action);
			break;
//		case GLFW_KEY_KP_8:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'N':
			KBD_AddKeyEvent(0x6C + action);
			break;
		case 'H':
			KBD_AddKeyEvent(0x4A + action);
			break;
		case 'Y':
			KBD_AddKeyEvent(0x2A + action);
			break;
		case '6':
			KBD_AddKeyEvent(0x0C + action);
			break;
		case GLFW_KEY_F5:
			KBD_AddKeyEvent(0xA8 + action);
			break;
//		case GLFW_KEY_KP_5:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case 'M':
			KBD_AddKeyEvent(0x6E + action);
			break;
		case 'J':
			KBD_AddKeyEvent(0x4C + action);
			break;
		case 'U':
			KBD_AddKeyEvent(0x2C + action);
			break;
		case '7':
			KBD_AddKeyEvent(0x0E + action);
			break;
//		case GLFW_KEY_KP_CLOSEBRACK:
//			KBD_AddKeyEvent(0xA0 + action);
//			break;
//		case GLFW_KEY_KP_2:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case ',':
			KBD_AddKeyEvent(0x70 + action);
			break;
		case 'K':
			KBD_AddKeyEvent(0x4E + action);
			break;
		case 'I':
			KBD_AddKeyEvent(0x2E + action);
			break;
		case '8':
			KBD_AddKeyEvent(0x10 + action);
			break;
		case GLFW_KEY_F6:
			KBD_AddKeyEvent(0xAA + action);
			break;
//		case GLFW_KEY_KP_ENTER:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case '.':
			KBD_AddKeyEvent(0x72 + action);
			break;
		case 'L':
			KBD_AddKeyEvent(0x50 + action);
			break;
		case 'O':
			KBD_AddKeyEvent(0x30 + action);
			break;
		case '9':
			KBD_AddKeyEvent(0x12 + action);
			break;
//		case GLFW_KEY_KP_/:
//			KBD_AddKeyEvent(0xA0 + action);
//			break;
//		case GLFW_KEY_KP_7:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case '/':
			KBD_AddKeyEvent(0x74 + action);
			break;
		case ';':
			KBD_AddKeyEvent(0x52 + action);
			break;
		case 'P':
			KBD_AddKeyEvent(0x32 + action);
			break;
		case '0':
			KBD_AddKeyEvent(0x14 + action);
			break;
		case GLFW_KEY_F7:
			KBD_AddKeyEvent(0xAC + action);
			break;
//		case GLFW_KEY_KP_4:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
//		case 'X': (spare)
//			KBD_AddKeyEvent(0x64 + action);
//			break;
		case '\'':
			KBD_AddKeyEvent(0x54 + action);
			break;
		case '[':
			KBD_AddKeyEvent(0x34 + action);
			break;
		case '-':
			KBD_AddKeyEvent(0x16 + action);
			break;
		case GLFW_KEY_F8:
			KBD_AddKeyEvent(0xAE + action);
			break;
//		case GLFW_KEY_KP_1:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case GLFW_KEY_SPACE:
			KBD_AddKeyEvent(0x80 + action);
			break;
//		case GLFW_KEY_ret:
//			KBD_AddKeyEvent(0x42 + action);
//			break;
		case ']':
			KBD_AddKeyEvent(0x36 + action);
			break;
		case '=':
			KBD_AddKeyEvent(0x18 + action);
			break;
		case GLFW_KEY_F9:
			KBD_AddKeyEvent(0xB0 + action);
			break;
//		case GLFW_KEY_KP_0:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case GLFW_KEY_BACKSPACE:
			KBD_AddKeyEvent(0x82 + action);
			break;
		case GLFW_KEY_DEL:
			KBD_AddKeyEvent(0x8C + action);
			break;
		case GLFW_KEY_ENTER:
			KBD_AddKeyEvent(0x88 + action);
			break;
		case '\\':
			KBD_AddKeyEvent(0x1A + action);
			break;
		case GLFW_KEY_F10:
			KBD_AddKeyEvent(0xB2 + action);
			break;
//		case GLFW_KEY_KP_-:
//			KBD_AddKeyEvent(0xBC + action);
//			break;
		case GLFW_KEY_DOWN:
			KBD_AddKeyEvent(0x9A + action);
			break;
		case GLFW_KEY_RIGHT:
			KBD_AddKeyEvent(0x9C + action);
			break;
		case GLFW_KEY_LEFT:
			KBD_AddKeyEvent(0x9E + action);
			break;
		case GLFW_KEY_UP:
			KBD_AddKeyEvent(0x98 + action);
			break;
		case GLFW_KEY_HOME:		// HOME = HELP
			KBD_AddKeyEvent(0xBE + action);
			break;
		case GLFW_KEY_F11:		// left amiga key (cant read windows keys using glfw?!)
			KBD_AddKeyEvent(0xCC + action);
			break;
		case GLFW_KEY_LALT:
			KBD_AddKeyEvent(0xC8 + action);
			break;
		case GLFW_KEY_LSHIFT:
			KBD_AddKeyEvent(0xC0 + action);
			break;
		case GLFW_KEY_LCTRL:
		case GLFW_KEY_RCTRL:
			KBD_AddKeyEvent(0xC6 + action);
			break;
		case GLFW_KEY_F12:		// right amiga key (cant read windows keys using glfw?!)
			KBD_AddKeyEvent(0xCE + action);
			break;
		case GLFW_KEY_RALT:
			KBD_AddKeyEvent(0xCA + action);
			break;
		case GLFW_KEY_RSHIFT:
			KBD_AddKeyEvent(0xC2 + action);
			break;
	}
}

int captureMouse=0;

int main(int argc,char **argv)
{
    unsigned char *romPtr;
	int running=1;
	int a;
	
	for (a=0;a<512*3;a++)
	{
		keyArray[a]=0;
	}
    
//	Debugger();			 - My xcode is playing up at moment and without this it often doesn't stop on breakpoints
	
//	decrpyt_rom();
	
	// Initialize GLFW 
	glfwInit(); 
	// Open an OpenGL window 
	if( !glfwOpenWindow( AMI_LINE_LENGTH, HEIGHT, 0,0,0,0,0,0, GLFW_WINDOW ) ) 
	{ 
		glfwTerminate(); 
		return 1; 
	} 
	
	glfwSetWindowTitle("MacAmi");
	glfwSetWindowPos(670,700);
	
	setupGL(AMI_LINE_LENGTH,HEIGHT);	
	
    romPtr=load_rom("../../out.rom");
    if (!romPtr)
    {
		romPtr=load_rom("out.rom");
		if (!romPtr)
		{
			printf("[ERR] Failed to load rom image\n");
			glfwTerminate(); 
			return 1; 
		}
    }
	
    CPU_BuildTable();
	
    MEM_Initialise(romPtr);
	
	CST_InitialiseCustom();
	CPR_InitialiseCopper();
	CIA_InitialiseCustom();
	BLT_InitialiseBlitter();
	DSP_InitialiseDisplay();
	DSK_InitialiseDisk();
	SPR_InitialiseSprites();
	KBD_InitialiseKeyboard();
	
    CPU_Reset();
	
//////////
	
//	CPU_runTests();
	
//	return 0;
	
//////////
	
	glfwSetKeyCallback(kbHandler);
    
	while (running)
	{
		if (!UpdateDebugger())
		{
			KBD_Update();
			SPR_Update();
			DSP_Update();			// Note need to priority order these ultimately
			CST_Update();
			DSK_Update();
			CPR_Update();
			CIA_Update();
			BLT_Update();
			CPU_Step();
		}
		DisplayDebugger();

		if (g_newScreenNotify)
		{
			static int lmx=0,lmy=0;
			int mx,my;

			DrawScreen();
			
			glfwSwapBuffers();
			
			g_newScreenNotify=0;
			
			glfwGetMousePos(&mx,&my);
			
			if ((mx>=1 && mx<=AMI_LINE_LENGTH && my>=1 && my <=HEIGHT) || captureMouse)
			{
				int vertMove = my-lmy;
				int horiMove = mx-lmx;
				int oldMoveX = CST_GETWRDU(CST_JOY0DAT,0x00FF);
				int oldMoveY = CST_GETWRDU(CST_JOY0DAT,0xFF00)>>8;
				if (horiMove>127)
					horiMove=127;
				if (horiMove<-128)
					horiMove=-128;
				if (vertMove>127)
					vertMove=127;
				if (vertMove<-128)
					vertMove=-128;
				oldMoveX+=horiMove;
				oldMoveY+=vertMove;
				
				CST_SETWRD(CST_JOY0DAT,((oldMoveY&0xFF)<<8)|(oldMoveX&0xFF),0xFFFF);
				lmx=mx;
				lmy=my;
				
				if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT))
				{
					leftMouseUp=0;
				}
				else
				{
					leftMouseUp=1;
				}
				if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE))
				{
					if (captureMouse)
					{
						captureMouse=0;
						glfwEnable(GLFW_MOUSE_CURSOR);
					}
					else
					{
						captureMouse=1;
						glfwDisable(GLFW_MOUSE_CURSOR);
					}
				}
				if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
				{
					rightMouseUp=0;
				}
				else
				{
					rightMouseUp=1;
				}
			}
		}
		
		// Check if ESC key was pressed or window was closed 
		running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED ); 
	}
	
	// Close window and terminate GLFW 
	glfwTerminate(); 
	// Exit program 
	return 0; 
}

