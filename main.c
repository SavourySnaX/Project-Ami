/* 
 
 Mac OS X main file (I`ll need to sort the other platforms out later)
 
 */

#include <stdio.h>
#include <stdlib.h>

#define AMI_LINE_LENGTH ((228)*2)		// 228 / 227 alternating
#define WIDTH AMI_LINE_LENGTH
#define HEIGHT 262
#define BPP 4
#define DEPTH 32

#include "config.h"

#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>
#include "/usr/local/include/GL/glfw.h"

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

/*
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
#endif*/
int g_newScreenNotify = 0;

void doPixel(int x,int y,u_int8_t colHi,u_int8_t colLo)
{
/*
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
#endif*/
}
/*
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
*/
int g_frameSkip=0;
#define FRAME_SKIP	4

#define BOX_SIZE	1.0f

void drawScene() 
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
    glBegin(GL_QUADS); //Begin quadrilateral coordinates
    
    //Trapezoid
    glVertex3f(-0.7f, -1.5f, -5.0f);
    glVertex3f(0.7f, -1.5f, -5.0f);
    glVertex3f(0.4f, -0.5f, -5.0f);
    glVertex3f(-0.4f, -0.5f, -5.0f);
    
    glEnd(); //End quadrilateral coordinates
}
/*
void update(int value) {

	glutPostRedisplay();
	glutTimerFunc(25, update, 0);
}
*/
void handleResize(int w, int h) {
    //Tell OpenGL how to convert from coordinates to pixel values
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION); //Switch to setting the camera perspective
    
    //Set the camera perspective
    glLoadIdentity(); //Reset the camera
    gluPerspective(45.0,                  //The camera angle
                   (double)w / (double)h, //The width-to-height ratio
                   1.0,                   //The near z clipping coordinate
                   200.0);                //The far z clipping coordinate
}

int main( void ) 
{ 
	int running = GL_TRUE; 
	// Initialize GLFW 
	glfwInit(); 
	// Open an OpenGL window 
	if( !glfwOpenWindow( 300,300, 0,0,0,0,0,0, GLFW_WINDOW ) ) 
	{ 
		glfwTerminate(); 
		return 0; 
	} 
	handleResize(300,300);
	// Main loop
	while( running ) 
	{ 
		// OpenGL rendering goes here... 
		glClear( GL_COLOR_BUFFER_BIT ); 
		drawScene();
		// Swap front and back rendering buffers 
		glfwSwapBuffers(); 
		// Check if ESC key was pressed or window was closed 
		running = !glfwGetKey( GLFW_KEY_ESC ) && 
		glfwGetWindowParam( GLFW_OPENED ); 
	} 
	// Close window and terminate GLFW 
	glfwTerminate(); 
	// Exit program 
	return 0; 
}

int omain(int argc,char **argv)
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
/*
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
		glutInitWindowSize(WIDTH, HEIGHT);
	
		glutCreateWindow("Ami");
	
		glutDisplayFunc(drawScene);
		glutReshapeFunc(handleResize);
		glutTimerFunc(25, update, 0);
		
	glEnable(GL_DEPTH_TEST);
	
		glutMainLoop();*/
/*
#if !DISABLE_DISPLAY
	    SDL_Event event;

	    int keypress = 0;
	    int h=0; 
	    int unlock=0;

	    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;

	    if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, 0|SDL_HWSURFACE)))
	    {
		    SDL_Quit();
		    return 1;
	    }

	    while(!keypress) 
#else*/
	    while (1)
//#endif
	    {
			DSP_Update();
		    CST_Update();
		    CPR_Update();
		    CIA_Update();
			BLT_Update();
		    CPU_Step();
/*
#if !DISABLE_DISPLAY		    
		    if (g_newScreenNotify)
		    {
				if (g_frameSkip==FRAME_SKIP)
				{
					g_frameSkip=0;
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
					
				}
			    g_newScreenNotify=0;
				g_frameSkip++;
			}
			

#endif*/
	    }
    }
	
    return 0;
}

