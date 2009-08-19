/*
 *  keyboard.c
 *  ami
 *
 *  Created by Lee Hammerton on 11/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *
 * Decided keyboard (and mouse eventually) should be seperate modules and work like the real hardware
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "keyboard.h"
#include "ciachip.h"
#include "customchip.h"

#define MAX_KB_SIZE	(256)

u_int8_t kbBuffer[MAX_KB_SIZE];

int kbBufferPos;
int acknowledged;
int kbErrorCnt=0;

void KBD_AddKeyEvent(u_int8_t keycode)
{
	if (kbBufferPos<MAX_KB_SIZE)
	{
		kbBuffer[kbBufferPos++]=keycode;
	}
}

void KBD_InitialiseKeyboard()
{
	kbBufferPos=0;
	acknowledged=1;
	
	kbBuffer[kbBufferPos++]=0xFF;		// Start of missing keys
	kbBuffer[kbBufferPos++]=0xFB;		// Start of missing keys
	kbBuffer[kbBufferPos++]=0xFD;		// End of missing keys
}

u_int8_t KBD_GetNextKey()
{
	u_int8_t nextKey = kbBuffer[0];
	
	kbBufferPos--;
	memcpy(&kbBuffer[0],&kbBuffer[1],kbBufferPos);
	printf("Sending Key to amiga : %02X\n",nextKey);
	return nextKey;
}

void KBD_Update()
{
/*	if (!acknowledged)
	{
		kbErrorCnt++;
		if (kbErrorCnt>10*2000)
		{
			// Attempt at fixing a sync problem between emulator and the keyboard
			acknowledged=1;
			kbErrorCnt=0;
		}
	}*/
	if (/*(!(ciaMemory[0x0D]&0x80)) &&*/(horizontalClock==0) && acknowledged)		// Make sure we don't have a pending interrupt before we do reload serial buffer and serial buffer is in input mode (this may not be correct!)
	{
		if (kbBufferPos)				// if zero keybuffer is empty
		{
			u_int8_t keycode;
			
			keycode = KBD_GetNextKey();
			
			// Do send keyup to SDR
			ciaMemory[0x0C]=~keycode;

			ciaMemory[0x0D]|=0x08;			// signal interrupt request (won't actually interrupt unless mask set however)
			acknowledged=0;
			if (ciaa_icr&0x08)
			{
				ciaMemory[0x0D]|=0x80;		// set IR bit
				CST_ORWRD(CST_INTREQR,0x0008);
				printf("signalled interrupt\n");
			}

		}
	}
}

void KBD_Acknowledge()
{
	acknowledged=1;
}