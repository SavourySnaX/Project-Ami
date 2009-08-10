/* 
 
 Mac OS X main file (I`ll need to sort the other platforms out later)
 
 */

#include <stdio.h>
#include <stdlib.h>

#define AMI_LINE_LENGTH ((228)*4)		// 228 / 227 alternating
#define WIDTH AMI_LINE_LENGTH
#define HEIGHT 262
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
    romSize = fread(romData,1,romSize,inRom);
    fclose(inRom);
	
	return romData;
}

u_int8_t videoMemory[AMI_LINE_LENGTH*262*sizeof(u_int32_t)];

int g_newScreenNotify = 0;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo)
{
	u_int32_t *pixmem32;
	u_int32_t colour;
	u_int8_t r = (colHi&0x0F)<<4;
	u_int8_t g = (colLo&0xF0);
	u_int8_t b = (colLo&0x0F)<<4;

	if (y>=262 || x>=AMI_LINE_LENGTH)
		return;

	colour = (r<<16) | (g<<8) | (b<<0);
	pixmem32 = &((u_int32_t*)videoMemory)[y*AMI_LINE_LENGTH + x];
	*pixmem32 = colour;
}

void DrawScreen() 
{
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, 1);
	
	// glTexSubImage2D is faster when not using a texture range
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0, AMI_LINE_LENGTH, 262, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoMemory);
	//glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, IMAGE_SIZE, IMAGE_SIZE, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image[draw_image]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	
	glTexCoord2f(0.0f, 262.0f);
	glVertex2f(-1.0f, -1.0f);
	
	glTexCoord2f(AMI_LINE_LENGTH, 262.0f);
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
				 262, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoMemory);
	
	glDisable(GL_DEPTH_TEST);
}

int main(int argc,char **argv)
{
    unsigned char *romPtr;
	int running=1;
    
	Debugger();
	
	// Initialize GLFW 
	glfwInit(); 
	// Open an OpenGL window 
	if( !glfwOpenWindow( AMI_LINE_LENGTH, 262, 0,0,0,0,0,0, GLFW_WINDOW ) ) 
	{ 
		glfwTerminate(); 
		return 1; 
	} 
	
	glfwSetWindowTitle("MacAmi");
	glfwSetWindowPos(670,700);
	
	setupGL(AMI_LINE_LENGTH,262);	
	
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
	
    CPU_Reset();
    
	while (running)
	{
		SPR_Update();
		DSP_Update();			// Note need to priority order these ultimately
		CST_Update();
		DSK_Update();
		CPR_Update();
		CIA_Update();
		BLT_Update();
		CPU_Step();
		
		if (g_newScreenNotify)
		{
			static int lmx=0,lmy=0;
			int mx,my;
		
			DrawScreen();
			
			glfwSwapBuffers();
			
			g_newScreenNotify=0;

			glfwGetMousePos(&mx,&my);
			
			if (mx>=1 && mx<=AMI_LINE_LENGTH && my>=1 && my <=262)
			{
				int vertMove = my-lmy;
				int horiMove = mx-lmx;
				int oldMoveX = CST_GETWRDU(CST_JOY0DAT,0x00FF);
				int oldMoveY = CST_GETWRDU(CST_JOY0DAT,0xFF00)>>8;
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

