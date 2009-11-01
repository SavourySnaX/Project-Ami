/*
 *  keyboard.c
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
#include <string.h>

#include "config.h"

#include "keyboard.h"
#include "ciachip.h"
#include "customchip.h"

#define MAX_KB_SIZE	(256)

typedef struct
{
	u_int8_t kbBuffer[MAX_KB_SIZE];
	int kbBufferPos;
	int acknowledged;
} KBD_data;

KBD_data kbd_Data;

void KBD_SaveState(FILE *outStream)
{
	fwrite(&kbd_Data,1,sizeof(KBD_data),outStream);
}

void KBD_LoadState(FILE *inStream)
{
	fread(&kbd_Data,1,sizeof(KBD_data),inStream);
}

void KBD_AddKeyEvent(u_int8_t keycode)
{
	if (kbd_Data.kbBufferPos<MAX_KB_SIZE)
	{
		kbd_Data.kbBuffer[kbd_Data.kbBufferPos++]=keycode;
	}
}

void KBD_InitialiseKeyboard()
{
	kbd_Data.kbBufferPos=0;
	kbd_Data.acknowledged=1;
	
	kbd_Data.kbBuffer[kbd_Data.kbBufferPos++]=0xFF;		// Start of missing keys
	kbd_Data.kbBuffer[kbd_Data.kbBufferPos++]=0xFB;		// Start of missing keys
	kbd_Data.kbBuffer[kbd_Data.kbBufferPos++]=0xFD;		// End of missing keys
}

u_int8_t KBD_GetNextKey()
{
	u_int8_t nextKey = kbd_Data.kbBuffer[0];
	
	kbd_Data.kbBufferPos--;
	memcpy(&kbd_Data.kbBuffer[0],&kbd_Data.kbBuffer[1],kbd_Data.kbBufferPos);
	return nextKey;
}

void KBD_Update()
{
	if ((horizontalClock==0) && kbd_Data.acknowledged)		// Make sure we don't have a pending interrupt before we do reload serial buffer and serial buffer is in input mode (this may not be correct!)
	{
		if (kbd_Data.kbBufferPos)				// if zero keybuffer is empty
		{
			u_int8_t keycode;
			
			keycode = KBD_GetNextKey();
			
			// Do send keyup to SDR
			ciaMemory[0x0C]=~keycode;
			
			ciaMemory[0x0D]|=0x08;			// signal interrupt request (won't actually interrupt unless mask set however)
			kbd_Data.acknowledged=0;
			if (ciaa_icr&0x08)
			{
				ciaMemory[0x0D]|=0x80;		// set IR bit
				CST_ORWRD(CST_INTREQR,0x0008);
			}

		}
	}
}

void KBD_Acknowledge()
{
	kbd_Data.acknowledged=1;
}
