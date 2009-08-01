/*
 *  blitter.c
 *  ami
 *
 *  Created by Lee Hammerton on 21/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#if !DISABLE_DISPLAY

#include "SDL.h"

#define __IGNORE_TYPES
#endif

#include "blitter.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"

int bltStart=0;
int bltZero=0;

u_int16_t	bltWidth;
u_int16_t	bltHeight;
u_int16_t	bltX,bltY;

void BLT_InitialiseBlitter()
{
	bltStart=0;
}

u_int16_t BLT_GetNextBltSource(u_int8_t useMask,u_int8_t bltPtrStart,u_int8_t bltDataStart,u_int8_t bltModStart,int applyModulo)
{
	if (cstMemory[0x40]&useMask)		// SRC is enabled
	{
		u_int32_t	bltAAddress = ((((u_int32_t)cstMemory[bltPtrStart])&0x00)<<24) |
								  ((((u_int32_t)cstMemory[bltPtrStart+1])&0x07)<<16) |
								  ((((u_int32_t)cstMemory[bltPtrStart+2])&0xFF)<<8) |
								  (cstMemory[bltPtrStart+3]&0xFE);
		cstMemory[bltDataStart]=MEM_getByte(bltAAddress);
		cstMemory[bltDataStart+1]=MEM_getByte(bltAAddress+1);
		bltAAddress+=2;
		
		if (applyModulo)
		{
			bltAAddress+=(int16_t)((((u_int16_t)cstMemory[bltModStart])<<8) | (cstMemory[bltModStart+1]&0xFE));
		}
		
		cstMemory[bltPtrStart]=(bltAAddress>>24)&0x00;
		cstMemory[bltPtrStart+1]=(bltAAddress>>16)&0x07;
		cstMemory[bltPtrStart+2]=(bltAAddress>>8)&0xFF;
		cstMemory[bltPtrStart+3]=(bltAAddress>>0)&0xFE;
	}
						
	return (((u_int16_t)cstMemory[bltDataStart])<<8) | cstMemory[bltDataStart+1];
}

void BLT_SetDest(u_int16_t bltD,u_int8_t bltPtrStart,u_int8_t bltDataStart,u_int8_t bltModStart,int applyModulo)
{
	if (cstMemory[0x40]&0x01)		// DST is enabled
	{
		u_int32_t	bltAAddress = ((((u_int32_t)cstMemory[bltPtrStart])&0x00)<<24) |
								  ((((u_int32_t)cstMemory[bltPtrStart+1])&0x07)<<16) |
								  ((((u_int32_t)cstMemory[bltPtrStart+2])&0xFF)<<8) |
								  (cstMemory[bltPtrStart+3]&0xFE);
		cstMemory[bltDataStart]=(bltD&0xFF00)>>8;
		cstMemory[bltDataStart+1]=bltD&0xFF;
		MEM_setWord(bltAAddress,bltD);
		bltAAddress+=2;
		
		if (applyModulo)
		{
			bltAAddress+=(int16_t)((((u_int16_t)cstMemory[bltModStart])<<8) | (cstMemory[bltModStart+1]&0xFE));
		}
		
		cstMemory[bltPtrStart]=(bltAAddress>>24)&0x00;
		cstMemory[bltPtrStart+1]=(bltAAddress>>16)&0x07;
		cstMemory[bltPtrStart+2]=(bltAAddress>>8)&0xFF;
		cstMemory[bltPtrStart+3]=(bltAAddress>>0)&0xFE;
	}
}

extern int startDebug;

void BLT_Update()
{
	if (bltStart && (cstMemory[0x03]&0x40) && (cstMemory[0x02]&0x02))
	{
		cstMemory[0x02]|=0x40;		// Set busy
		
		if (cstMemory[0x43]&0x01)
		{
			int16_t tmp;
			int16_t tmp2;
			
			printf("Blitter In Line Mode (unsupported)\n");
			
			switch ((cstMemory[0x43] & 0x1C)>>2)
			{
			case 0:
				printf("Blitter Octent : SSE\n");
				break;
			case 1:
				printf("Blitter Octent : NNE\n");
				break;
			case 2:
				printf("Blitter Octent : SSW\n");
				break;
			case 3:
				printf("Blitter Octent : NNW\n");
				break;
			case 4:
				printf("Blitter Octent : SEE\n");
				break;
			case 5:
				printf("Blitter Octent : SWW\n");
				break;
			case 6:
				printf("Blitter Octent : NEE\n");
				break;
			case 7:
				printf("Blitter Octent : NWW\n");
				break;
			}
			
			tmp = cstMemory[0x62];
			tmp<<=8;
			tmp|= cstMemory[0x63];
			
			printf("dy = %d (%08X)\n",tmp/4,tmp);

			tmp2 = cstMemory[0x64];
			tmp2<<=8;
			tmp2|= cstMemory[0x65];
			
			printf("dx = %d (%08X)\n",(tmp - tmp2)/4,tmp2);
			
			printf("line length = %d (%02X)\n",bltHeight,bltWidth);
			
//			startDebug=1;
			
								bltStart=0;
								cstMemory[0x1F]|=0x40;			// set interrupt blitter finished
								cstMemory[0x02]&=~(0x40);		// Clear busy
			//SOFT_BREAK;
		}
		else
		{
			if (cstMemory[0x43]&0x02)
			{
				printf("Blitter In Descending Mode (unsupported)\n");
								bltStart=0;
								cstMemory[0x1F]|=0x40;			// set interrupt blitter finished
								cstMemory[0x02]&=~(0x40);		// Clear busy
//				SOFT_BREAK;
			}
			else
			{
				if (cstMemory[0x43]&0x18)
				{
					printf("Blitter In Fill Mode (unsupported)\n");
					SOFT_BREAK;
				}
				else
				{
/*					if ( (cstMemory[0x42]&0xF0) || (cstMemory[0x40]&0xF0) )
					{
						printf("Blitter Using Shift (unsupported)\n");
								bltStart=0;
								cstMemory[0x1F]|=0x40;			// set interrupt blitter finished
								cstMemory[0x02]&=~(0x40);		// Clear busy
//						SOFT_BREAK;
					}
					else*/
					{
						u_int16_t	bltAMask=0xFFFF;
						u_int16_t	bltA,bltB,bltC,bltD;
						u_int16_t	bltMin0=0,bltMin1=0,bltMin2=0,bltMin3=0,bltMin4=0,bltMin5=0,bltMin6=0,bltMin7=0;
						
						if (bltX==0)
						{
							bltAMask&=(((u_int16_t)cstMemory[0x44])<<8) | cstMemory[0x45];
						}
						if (bltX==(bltWidth-1))
						{
							bltAMask&=(((u_int16_t)cstMemory[0x46])<<8) | cstMemory[0x47];
						}
						
						bltA = BLT_GetNextBltSource(0x08,0x50,0x74,0x64,bltX==(bltWidth-1)) & bltAMask;
						bltB = BLT_GetNextBltSource(0x04,0x4C,0x72,0x62,bltX==(bltWidth-1));
						bltC = BLT_GetNextBltSource(0x02,0x48,0x70,0x60,bltX==(bltWidth-1));
						
						if (cstMemory[0x41]&0x80)
						{
							bltMin7 = (~bltA) & (~bltB) & (~bltC);
						}
						if (cstMemory[0x41]&0x40)
						{
							bltMin6 = (bltA) & (bltB) & (~bltC);
						}
						if (cstMemory[0x41]&0x20)
						{
							bltMin5 = (bltA) & (~bltB) & (bltC);
						}
						if (cstMemory[0x41]&0x10)
						{
							bltMin4 = (bltA) & (~bltB) & (~bltC);
						}
						if (cstMemory[0x41]&0x08)
						{
							bltMin3 = (~bltA) & (bltB) & (bltC);
						}
						if (cstMemory[0x41]&0x04)
						{
							bltMin2 = (~bltA) & (bltB) & (~bltC);
						}
						if (cstMemory[0x41]&0x02)
						{
							bltMin1 = (~bltA) & (~bltB) & (bltC);
						}
						if (cstMemory[0x41]&0x01)
						{
							bltMin0 = (bltA) & (bltB) & (bltC);
						}
						
						bltD = bltMin0 | bltMin1 | bltMin2 | bltMin3 | bltMin4 | bltMin5 | bltMin6 | bltMin7;
						
						bltZero = bltZero && (bltD==0);
						
						BLT_SetDest(bltD,0x54,0x00,0x66,bltX==(bltWidth-1));
						
						if (bltX == (bltWidth-1))
						{
							bltX = 0;
							bltY++;
							if (bltY>=bltHeight)
							{
								// blitter finished, clear BLTBUSY and set BLTComplete
								bltStart=0;
								cstMemory[0x1F]|=0x40;			// set interrupt blitter finished
								cstMemory[0x02]&=~(0x40);		// Clear busy
								if (bltZero)
								{
									cstMemory[0x02]|=0x20;		// BLT  all zeros
								}
							}
						}
						else
						{
							bltX++;
						}
					}
				}
			}
		}
	}
}

extern int startDebug;

void BLT_StartBlit()
{
//		startDebug=1;
		
		bltX=0;
		bltY=0;
		bltWidth=cstMemory[0x59]&0x3F;
		if (!bltWidth)
			bltWidth=64;
		bltHeight=((((u_int16_t)cstMemory[0x58])&0xFF)<<2)|((((u_int16_t)cstMemory[0x59])&0xC0)>>6);
		if (!bltHeight)
			bltHeight=1024;
		bltZero=1;
		bltStart=1;
		
		printf("[WRN] Blitter Is Being Used\n");

		printf("Blitter Registers : BLTAFWM : %02X%02X\n",cstMemory[0x44],cstMemory[0x45]);
		printf("Blitter Registers : BLTALWM : %02X%02X\n",cstMemory[0x46],cstMemory[0x47]);
		printf("Blitter Registers : BLTCON0 : %02X%02X\n",cstMemory[0x40],cstMemory[0x41]);
		printf("Blitter Registers : BLTCON1 : %02X%02X\n",cstMemory[0x42],cstMemory[0x43]);
		printf("Blitter Registers : BLTSIZE : %02X%02X\n",cstMemory[0x58],cstMemory[0x59]);
		printf("Blitter Registers : BLTADAT : %02X%02X\n",cstMemory[0x74],cstMemory[0x75]);
		printf("Blitter Registers : BLTBDAT : %02X%02X\n",cstMemory[0x72],cstMemory[0x73]);
		printf("Blitter Registers : BLTCDAT : %02X%02X\n",cstMemory[0x70],cstMemory[0x71]);
		printf("Blitter Registers : BLTDDAT : %02X%02X\n",cstMemory[0x00],cstMemory[0x01]);
		printf("Blitter Registers : BLTAMOD : %02X%02X\n",cstMemory[0x64],cstMemory[0x65]);
		printf("Blitter Registers : BLTBMOD : %02X%02X\n",cstMemory[0x62],cstMemory[0x63]);
		printf("Blitter Registers : BLTCMOD : %02X%02X\n",cstMemory[0x60],cstMemory[0x61]);
		printf("Blitter Registers : BLTDMOD : %02X%02X\n",cstMemory[0x66],cstMemory[0x67]);
		printf("Blitter Registers : BLTAPTH : %02X%02X\n",cstMemory[0x50],cstMemory[0x51]);
		printf("Blitter Registers : BLTAPTL : %02X%02X\n",cstMemory[0x52],cstMemory[0x53]);
		printf("Blitter Registers : BLTBPTH : %02X%02X\n",cstMemory[0x4C],cstMemory[0x4D]);
		printf("Blitter Registers : BLTBPTL : %02X%02X\n",cstMemory[0x4E],cstMemory[0x4F]);
		printf("Blitter Registers : BLTCPTH : %02X%02X\n",cstMemory[0x48],cstMemory[0x49]);
		printf("Blitter Registers : BLTCPTL : %02X%02X\n",cstMemory[0x4A],cstMemory[0x4B]);
		printf("Blitter Registers : BLTDPTH : %02X%02X\n",cstMemory[0x54],cstMemory[0x55]);
		printf("Blitter Registers : BLTDPTL : %02X%02X\n",cstMemory[0x56],cstMemory[0x57]);

}