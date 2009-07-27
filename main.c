/* 
 
 Decrypt cloanto rom
 
 */

#include <stdio.h>
#include <stdlib.h>

#define AMI_LINE_LENGTH ((228)*2)		// 228 / 227 alternating
#define WIDTH AMI_LINE_LENGTH
#define HEIGHT 262
#define BPP 4
#define DEPTH 32

#include "config.h"

#if !DISABLE_DISPLAY
#include "SDL.h"

#define __IGNORE_TYPES
#endif

#include "cpu.h"
#include "memory.h"
#include "customchip.h"
#include "ciachip.h"
#include "copper.h"
#include "blitter.h"
#include "display.h"

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

#if !DISABLE_DISPLAY
SDL_Surface *screen;


void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 *pixmem32;
    Uint32 colour;  
 
    colour = SDL_MapRGB( screen->format, r, g, b );
  
    pixmem32 = (Uint32*) screen->pixels  + y + x;
    *pixmem32 = colour;
}


Uint32 videoMemory[AMI_LINE_LENGTH*262];
#endif
int g_newScreenNotify = 0;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo)
{
#if !DISABLE_DISPLAY
	Uint32 *pixmem32;
	Uint32 colour;
	u_int8_t r = (colHi&0x0F)<<4;
	u_int8_t g = (colLo&0xF0);
	u_int8_t b = (colLo&0x0F)<<4;

	if (y>=262 || x>=AMI_LINE_LENGTH)
		return;

	colour = SDL_MapRGB(screen->format, r,g,b);
	pixmem32 = &videoMemory[y*AMI_LINE_LENGTH + x];
	*pixmem32 = colour;
#endif
}

#if !DISABLE_DISPLAY
void DrawScreen(SDL_Surface* screen, int h)
{ 
    int x, y, ytimesw;
  
    for(y = 0; y < screen->h; y++ ) 
    {
        ytimesw = y*screen->pitch/BPP;
	memcpy((Uint32*)screen->pixels + ytimesw,videoMemory + y*AMI_LINE_LENGTH,AMI_LINE_LENGTH*4);
    }

}
#endif

int main(int argc,char **argv)
{
    unsigned char *romPtr;
    
    romPtr=load_rom("../../out.rom");
    if (!romPtr)
    {
	romPtr=load_rom("out.rom");
	if (!romPtr)
	{
	    printf("[ERR] Failed to load rom image\n");
	    return -1;
	}
    }

    CPU_BuildTable();
	
    MEM_Initialise(romPtr);
	
	CST_InitialiseCustom();
	CPR_InitialiseCopper();
	CIA_InitialiseCustom();
	BLT_InitialiseBlitter();
	DSP_InitialiseDisplay();

    CPU_Reset();
    
    {	
#if !DISABLE_DISPLAY
	    SDL_Event event;

	    int keypress = 0;
	    int h=0; 
	    int unlock=0;

	    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;

	    if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, 0/*SDL_FULLSCREEN*/|SDL_HWSURFACE/**/)))
	    {
		    SDL_Quit();
		    return 1;
	    }

	    while(!keypress) 
#else
	    while (1)
#endif
	    {
			DSP_Update();
		    CST_Update();
		    CPR_Update();
		    CIA_Update();
			BLT_Update();
		    CPU_Step();

#if !DISABLE_DISPLAY		    
		    if (g_newScreenNotify)
		    {
			    if(SDL_MUSTLOCK(screen))
			    {
				    if(SDL_LockSurface(screen) >= 0) 
				    {
					unlock=1;
				    }
			    }
			DrawScreen(screen,1);
			
			if(unlock && SDL_MUSTLOCK(screen))
			{
				unlock=0;	
				SDL_UnlockSurface(screen);
			}
			SDL_Flip(screen); 

			    g_newScreenNotify=0;
		    }
			    

		    while(SDL_PollEvent(&event)) 
		    {      
			    switch (event.type) 
			    {
				    case SDL_QUIT:
					    keypress = 1;
					    break;
				    case SDL_KEYDOWN:
					    keypress = 1;
					    break;
			    }
		    }
#endif
	    }
    }
	
    return 0;
}


void waveOutClose()
{
}

void waveOutUnprepareHeader()
{
}

void waveOutWrite()
{
}

void waveOutGetErrorTextA()
{
}

void waveOutOpen()
{
}

void waveOutPrepareHeader()
{
}

void joyGetPosEx()
{
}

void joyGetNumDevs()
{
}

void joyGetDevCapsA()
{
}

void mciSendCommandA()
{
}

void mciGetErrorStringA()
{
}

void timeKillEvent()
{
}

void timeEndPeriod()
{
}

void timeBeginPeriod()
{
}

void timeSetEvent()
{
}

void timeGetTime()
{
}


