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

u_int16_t BLT_GetNextBltSource(u_int16_t useMask,u_int8_t bltPtrStart,u_int8_t bltDataStart,u_int8_t bltModStart,int applyModulo)
{
	if (CST_GETWRDU(CST_BLTCON0,useMask))		// SRC is enabled
	{
		u_int32_t	bltAAddress = CST_GETLNGU(bltPtrStart,0x0007FFFE);

		CST_SETWRD(bltDataStart,MEM_getWord(bltAAddress),0xFFFF);
		bltAAddress+=2;
		
		if (applyModulo)
		{
			bltAAddress+=CST_GETWRDS(bltModStart,0xFFFE);
		}
		
		CST_SETLNG(bltPtrStart,bltAAddress,0x0007FFFE);
	}
						
	return CST_GETWRDU(bltDataStart,0xFFFF);
}

void BLT_SetDest(u_int16_t bltD,u_int8_t bltPtrStart,u_int8_t bltDataStart,u_int8_t bltModStart,int applyModulo)
{
	if (CST_GETWRDU(CST_BLTCON0,0x0100))		// DST is enabled
	{
		u_int32_t	bltAAddress = CST_GETLNGU(bltPtrStart,0x0007FFFE);

		CST_SETWRD(bltDataStart,bltD,0xFFFF);
		MEM_setWord(bltAAddress,bltD);
		bltAAddress+=2;
		
		if (applyModulo)
		{
			bltAAddress+=CST_GETWRDS(bltModStart,0xFFFE);
		}
		
		CST_SETLNG(bltPtrStart,bltAAddress,0x0007FFFE);
	}
}

extern int startDebug;

void BLT_Update()
{
	if (bltStart && CST_GETWRDU(CST_DMACONR,0x0240))
	{
		CST_ORWRD(CST_DMACONR,0x4000);
		
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
						
						bltA = BLT_GetNextBltSource(0x0800,0x50,0x74,0x64,bltX==(bltWidth-1)) & bltAMask;
						bltB = BLT_GetNextBltSource(0x0400,0x4C,0x72,0x62,bltX==(bltWidth-1));
						bltC = BLT_GetNextBltSource(0x0200,0x48,0x70,0x60,bltX==(bltWidth-1));
						
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
		bltWidth=CST_GETWRDU(CST_BLTSIZE,0x003F);
		if (!bltWidth)
			bltWidth=64;
		bltHeight=CST_GETWRDU(CST_BLTSIZE,0xFFC0)>>6;
		if (!bltHeight)
			bltHeight=1024;
		bltZero=1;
		bltStart=1;
		
		printf("[WRN] Blitter Is Being Used\n");

		printf("Blitter Registers : BLTAFWM : %04X\n",CST_GETWRDU(CST_BLTAFWM,0xFFFF));
		printf("Blitter Registers : BLTALWM : %04X\n",CST_GETWRDU(CST_BLTALWM,0xFFFF));
		printf("Blitter Registers : BLTCON0 : %04X\n",CST_GETWRDU(CST_BLTCON0,0xFFFF));
		printf("Blitter Registers : BLTCON1 : %04X\n",CST_GETWRDU(CST_BLTCON1,0xFFFF));
		printf("Blitter Registers : BLTSIZE : %04X\n",CST_GETWRDU(CST_BLTSIZE,0xFFFF));
		printf("Blitter Registers : BLTADAT : %04X\n",CST_GETWRDU(CST_BLTADAT,0xFFFF));
		printf("Blitter Registers : BLTBDAT : %04X\n",CST_GETWRDU(CST_BLTBDAT,0xFFFF));
		printf("Blitter Registers : BLTCDAT : %04X\n",CST_GETWRDU(CST_BLTCDAT,0xFFFF));
		printf("Blitter Registers : BLTDDAT : %04X\n",CST_GETWRDU(CST_BLTDDAT,0xFFFF));
		printf("Blitter Registers : BLTAMOD : %04X\n",CST_GETWRDU(CST_BLTAMOD,0xFFFF));
		printf("Blitter Registers : BLTBMOD : %04X\n",CST_GETWRDU(CST_BLTBMOD,0xFFFF));
		printf("Blitter Registers : BLTCMOD : %04X\n",CST_GETWRDU(CST_BLTCMOD,0xFFFF));
		printf("Blitter Registers : BLTDMOD : %04X\n",CST_GETWRDU(CST_BLTDMOD,0xFFFF));
		printf("Blitter Registers : BLTAPTH : %08X\n",CST_GETLNGU(CST_BLTAPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTBPTH : %08X\n",CST_GETLNGU(CST_BLTAPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTCPTH : %08X\n",CST_GETLNGU(CST_BLTAPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTDPTH : %08X\n",CST_GETLNGU(CST_BLTAPTH,0xFFFFFFFF));

}