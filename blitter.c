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

int bltZero=0;
int bltSrcDone=0;

u_int16_t	bltWidth;
u_int16_t	bltHeight;
u_int16_t	bltSX,bltSY,bltDX,bltDY;
u_int16_t	bltAOldDat,bltBOldDat;		// Stores last shifted out data
int32_t		bltDelta;

typedef enum
{
	BLT_Stopped = 0,
	BLT_Read,		// Handles blitter src pre read, Blitter will read A&|B&|C data into BLTxDAT registers
	BLT_Read2,		// Blitter will read A&|B&|C data into BLTxDAT registers
	BLT_Line
} BLT_Ops;

BLT_Ops bltStart;

typedef struct
{
	u_int8_t bits[16];
} MTerms;

MTerms	mTermA[65536];
MTerms	mTermB[65536];
MTerms	mTermC[65536];

void BLT_InitialiseBlitter()
{
	int a;
	
	bltStart=BLT_Stopped;

	for (a=0;a<65536;a++)
	{
		for (int b=0;b<16;b++)
		{
			if (a & (1<<b))
			{
				mTermA[a].bits[15-b]=4;
				mTermB[a].bits[15-b]=2;
				mTermC[a].bits[15-b]=1;
			}
		}
	}
	
}

void BLT_ReadNextBltSource(u_int16_t useMask,u_int8_t bltPtrStart,u_int8_t bltDataStart,u_int8_t bltModStart,int firstWrd,int lastWrd,
						   u_int16_t fwdMask,u_int16_t lstMask,int shift,u_int16_t *old)
{
	if (CST_GETWRDU(CST_BLTCON0,useMask) && CST_GETWRDU(CST_DMACONR,0x0240)==0x0240)		// SRC is enabled
	{
		u_int32_t	bltAAddress = CST_GETLNGU(bltPtrStart,0x0007FFFE);
		u_int16_t	bltADat = MEM_getWord(bltAAddress);

		CST_SETWRD(bltDataStart,bltADat,0xFFFF);
		bltAAddress+=bltDelta*2;
		
		if (lastWrd)
		{
			bltAAddress+=bltDelta*CST_GETWRDS(bltModStart,0xFFFE);
		}
		
		CST_SETLNG(bltPtrStart,bltAAddress,0x0007FFFE);
	}
}


// Should handle descending mode now, fill to do.
u_int16_t BLT_CalculateD(u_int16_t bltA,u_int16_t bltB,u_int16_t bltC)
{
	u_int16_t	bltD=0;
	int a;

	for (a=0;a<16;a++)
	{
		if (CST_GETWRDU(CST_BLTCON0,1<<(mTermA[bltA].bits[a]+mTermB[bltB].bits[a]+mTermC[bltC].bits[a])))
			bltD|=(1<<(15-a));
	}

	bltZero = bltZero && (bltD==0);
	
	return bltD;
}

void BLT_UpdateSrc()
{
	if (!bltSrcDone)
	{
		// The first/last word masks and shifters are applied before the register is held - so they are done in the read portion
		BLT_ReadNextBltSource(0x0800,CST_BLTAPTH,CST_BLTADAT,CST_BLTAMOD,bltSX==0,bltSX==(bltWidth-1),
							  CST_GETWRDU(CST_BLTAFWM,0xFFFF),CST_GETWRDU(CST_BLTALWM,0xFFFF),CST_GETWRDU(CST_BLTCON0,0xF000)>>12,&bltAOldDat);
		BLT_ReadNextBltSource(0x0400,CST_BLTBPTH,CST_BLTBDAT,CST_BLTBMOD,bltSX==0,bltSX==(bltWidth-1),
							  0xFFFF,0xFFFF,CST_GETWRDU(CST_BLTCON1,0xF000)>>12,&bltBOldDat);
		BLT_ReadNextBltSource(0x0200,CST_BLTCPTH,CST_BLTCDAT,CST_BLTCMOD,bltSX==0,bltSX==(bltWidth-1),
							  0xFFFF,0xFFFF,0,0);
		
	}	
}

void BLT_UpdateDst()
{
	if (CST_GETWRDU(CST_BLTCON0,0x0100) && CST_GETWRDU(CST_DMACONR,0x0240)==0x0240)		// DST is enabled
	{
		u_int32_t	bltDAddress = CST_GETLNGU(CST_BLTDPTH,0x0007FFFE);
		
		MEM_setWord(bltDAddress,CST_GETWRDU(CST_BLTDDAT,0xFFFF));
		bltDAddress+=bltDelta * 2;
		
		if (bltDX==(bltWidth-1))
		{
			bltDAddress+=bltDelta * CST_GETWRDS(CST_BLTDMOD,0xFFFE);
		}
		
		CST_SETLNG(CST_BLTDPTH,bltDAddress,0x0007FFFE);
	}
	
	// Check End Condition
	bltDX++;
	if (bltDX == bltWidth)
	{
		bltDX = 0;
		bltDY++;
		if (bltDY>=bltHeight)
		{
			// blitter finished, clear BLTBUSY and set BLTComplete
			bltStart=BLT_Stopped;
			
			CST_ORWRD(CST_INTREQR,0x0040);			// set interrupt blitter finished
			CST_ANDWRD(CST_DMACONR,~0x4000);
			
			if (bltZero)
			{
				CST_ORWRD(CST_DMACONR,0x2000);		// BLT  all zeros
			}
			
		}
	}
}

u_int16_t BLT_Mask(u_int16_t bltA)
{
		if (bltSX==0)
		{
			bltA&=CST_GETWRDU(CST_BLTAFWM,0xFFFF);
		}
		if (bltSX==(bltWidth-1))
		{
			bltA&=CST_GETWRDU(CST_BLTALWM,0xFFFF);
		}
		
	return bltA;
}

u_int16_t BLT_Shift(u_int16_t bltA, u_int16_t shift, u_int16_t *old)
{
	u_int16_t bltOldDat,bltOldTmp;
	
		if (shift)
		{
			bltOldDat=*old;
			if (CST_GETWRDU(CST_BLTCON1,0x0002) && !CST_GETWRDU(CST_BLTCON1,0x0001))			// descending mode shift left
			{
				bltOldTmp = (~((0x8000 >> (shift-1))-1));
				bltOldTmp&=bltA;
				bltOldTmp>>=(16-shift);
				*old = bltOldTmp;
				bltA<<=shift;
				bltA|=bltOldDat;
			}
			else
			{
				bltOldTmp = (((0x0001 << shift)-1));
				bltOldTmp&=bltA;
				bltOldTmp<<=(16-shift);
				*old = bltOldTmp;
				bltA>>=shift;
				bltA|=bltOldDat;
			}
		}
	return bltA;
}

void BLT_UpdateClc()
{
	u_int16_t	bltA,bltB,bltC,bltD;
	
	bltA=CST_GETWRDU(CST_BLTADAT,0xFFFF);
	bltB=CST_GETWRDU(CST_BLTBDAT,0xFFFF);

//	if (!CST_GETWRDU(CST_BLTCON1,0x0001))
	{
		bltA=BLT_Mask(bltA);
		
		bltA=BLT_Shift(bltA,CST_GETWRDU(CST_BLTCON0,0xF000)>>12,&bltAOldDat);	

		bltB=BLT_Shift(bltB,CST_GETWRDU(CST_BLTCON1,0xF000)>>12,&bltBOldDat);
	}
	
	bltC=CST_GETWRDU(CST_BLTCDAT,0xFFFF);

	if (!CST_GETWRDU(CST_BLTCON1,0x0001))
	{
		// Check End Src Read Condition
		bltSX++;
		if (bltSX == bltWidth)
		{
			bltSX = 0;
			bltAOldDat=0;
			bltBOldDat=0;
			bltSY++;
			if (bltSY>=bltHeight)
			{
				bltSrcDone=1;
			}
		}
	}

	bltD=BLT_CalculateD(bltA,bltB,bltC);
	
	CST_SETWRD(CST_BLTDDAT,bltD,0xFFFF);
}

// Modified BLT_Update to a simple state machine. Basically to cope with double src read before write as stated in HRM

void BLT_Update()
{
	u_int32_t addr;
	
	switch (bltStart)
	{
		case BLT_Stopped:
			break;
		case BLT_Read:

			CST_ORWRD(CST_DMACONR,0x4000);

			BLT_UpdateSrc();
			
			BLT_UpdateClc();

//			BLT_UpdateDst();				// swap this and next line comment over if trying for Preread
			
			bltStart=BLT_Read2;
			break;
			
		case BLT_Read2:						// Doesn't seem to work quite right yet

			CST_ORWRD(CST_DMACONR,0x4000);

			BLT_UpdateSrc();

			BLT_UpdateDst();
			
			BLT_UpdateClc();
			
			break;
			
		case BLT_Line:
		
			CST_ORWRD(CST_DMACONR,0x4000);

			if (CST_GETWRDU(CST_BLTCON0,0x0200) && CST_GETWRDU(CST_DMACONR,0x0240)==0x0240)		// SRC is enabled
			{
				CST_SETWRD(CST_BLTCDAT,MEM_getWord(CST_GETLNGU(CST_BLTCPTH,0X0007FFFE)),0xFFFF);
			}

			BLT_UpdateClc();
			
			if (!CST_GETWRDU(CST_BLTCON1,0x0040))
			{
				if (CST_GETWRDU(CST_BLTCON0,0x0800))
				{
					addr = CST_GETLNGU(CST_BLTAPTH,0x0007FFFE);
					addr+=CST_GETWRDS(CST_BLTAMOD,0xFFFE);
					CST_SETLNG(CST_BLTAPTH,addr,0x0007FFFE);
				}
				if (CST_GETWRDU(CST_BLTCON1,0x0010))
				{
					if (CST_GETWRDU(CST_BLTCON1,0x0008))
					{
						//decy
						addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
						addr-=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
						CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
					}
					else
					{
						//incy
						addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
						addr+=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
						CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
					}
				}
				else
				{
					if (CST_GETWRDU(CST_BLTCON1,0x0008))
					{
						//decx
						u_int16_t oldShift = CST_GETWRDU(CST_BLTCON0,0xF000)>>12;
						oldShift--;
						CST_ANDWRD(CST_BLTCON0,0x0FFF);
						CST_ORWRD(CST_BLTCON0,(oldShift&0xF)<<12);
						if (oldShift==0xFFFF)
						{
							addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
							addr-=2;
							CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
						}
					}
					else
					{
						//incx
						u_int16_t oldShift = CST_GETWRDU(CST_BLTCON0,0xF000)>>12;
						oldShift++;
						CST_ANDWRD(CST_BLTCON0,0x0FFF);
						CST_ORWRD(CST_BLTCON0,(oldShift&0xF)<<12);
						if (oldShift==0x0010)
						{
							addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
							addr+=2;
							CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
						}
					}
				}
			}
			else
			{
				if (CST_GETWRDU(CST_BLTCON0,0x0800))
				{
					addr = CST_GETLNGU(CST_BLTAPTH,0x0007FFFE);
					addr+=CST_GETWRDS(CST_BLTBMOD,0xFFFE);
					CST_SETLNG(CST_BLTAPTH,addr,0x0007FFFE);
				}
			
			}
			if (CST_GETWRDU(CST_BLTCON1,0x0010))
			{
				if (CST_GETWRDU(CST_BLTCON1,0x0004))
				{
					//decx
					u_int16_t oldShift = CST_GETWRDU(CST_BLTCON0,0xF000)>>12;
					oldShift--;
					CST_ANDWRD(CST_BLTCON0,0x0FFF);
					CST_ORWRD(CST_BLTCON0,(oldShift&0xF)<<12);
					if (oldShift==0xFFFF)
					{
						addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
						addr-=2;
						CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
					}
				}
				else
				{
					//incx
					u_int16_t oldShift = CST_GETWRDU(CST_BLTCON0,0xF000)>>12;
					oldShift++;
					CST_ANDWRD(CST_BLTCON0,0x0FFF);
					CST_ORWRD(CST_BLTCON0,(oldShift&0xF)<<12);
					if (oldShift==0x0010)
					{
						addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
						addr+=2;
						CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
					}
				}
			}
			else
			{
				if (CST_GETWRDU(CST_BLTCON1,0x0004))
				{
					//decy
					addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
					addr-=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
					CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
				}
				else
				{
					//incy
					addr = CST_GETLNGU(CST_BLTCPTH,0x0007FFFE);
					addr+=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
					CST_SETLNG(CST_BLTCPTH,addr,0x0007FFFE);
				}
			}
						
			if (CST_GETLNGU(CST_BLTAPTH,0x00008000))
			{
				CST_ORWRD(CST_BLTCON1,0x0040);
			}
			else
			{
				CST_ANDWRD(CST_BLTCON1,~0x0040);
			}
				
			if (CST_GETWRDU(CST_BLTCON0,0x0100) && CST_GETWRDU(CST_DMACONR,0x0240)==0x0240)		// DST is enabled
			{
				MEM_setWord(CST_GETLNGU(CST_BLTDPTH,0x0007FFFE), CST_GETWRDU(CST_BLTDDAT,0xFFFF));
			}

			CST_SETLNG(CST_BLTDPTH,CST_GETLNGU(CST_BLTCPTH,0x0007FFFE),0x0007FFFE);

			bltHeight--;
			if (bltHeight==0)
			{
			// blitter finished, clear BLTBUSY and set BLTComplete
			bltStart=BLT_Stopped;
			
			CST_ORWRD(CST_INTREQR,0x0040);			// set interrupt blitter finished
			CST_ANDWRD(CST_DMACONR,~0x4000);
			
			if (bltZero)
			{
				CST_ORWRD(CST_DMACONR,0x2000);		// BLT  all zeros
			}
				
			}

			break;
	}
}

extern int startDebug;

void BLT_StartBlit()
{
	//startDebug=1;
	
		bltDX=0;
		bltDY=0;
		bltSX=0;
		bltSY=0;
		bltAOldDat=0;
		bltBOldDat=0;
		
//		if (!CST_GETWRDU(CST_BLTCON1,0x0001))
//			return;
		
//		lCount++;
//		if (lCount<151 || lCount>151)
//			return;
//		printf("lCount : %d\n",lCount);

		if (CST_GETWRDU(CST_BLTCON1,0x0001))
		{
			if (CST_GETWRDU(CST_BLTCON1,0x0002))
			{
				printf("Warning.. single dot mode not supported\n");
				return;
			}
			
			if (CST_GETWRDU(CST_BLTBDAT,0xFFFF)!=0xFFFF)
			{
				printf("Warning.. textured line not supported\n");
				return;
			}
		}
		else
		{
			if (CST_GETWRDU(CST_BLTCON1,0x0018))
			{
				printf("Warning.. blitter fill unsupported\n");
				return;
			}
			if (CST_GETWRDU(CST_BLTCON1,0x0002))
			{
				bltDelta=-1;
			}
			else
			{
				bltDelta=1;
			}
  		}
		
		bltWidth=CST_GETWRDU(CST_BLTSIZE,0x003F);
		if (!bltWidth)
			bltWidth=64;
		bltHeight=CST_GETWRDU(CST_BLTSIZE,0xFFC0)>>6;
		if (!bltHeight)
			bltHeight=1024;
			
		bltZero=1;
		bltSrcDone=0;

		if (!CST_GETWRDU(CST_BLTCON1,0x0001))
		{
			bltStart=BLT_Read;
		printf("[WRN] Blitter Is Being Used\n");

		printf("Blitter Registers : BLTAFWM : %04X\n",CST_GETWRDU(CST_BLTAFWM,0xFFFF));
		printf("Blitter Registers : BLTALWM : %04X\n",CST_GETWRDU(CST_BLTALWM,0xFFFF));
/*		printf("Blitter Registers : BLTCON0 : %04X\n",CST_GETWRDU(CST_BLTCON0,0xFFFF));
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
		printf("Blitter Registers : BLTBPTH : %08X\n",CST_GETLNGU(CST_BLTBPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTCPTH : %08X\n",CST_GETLNGU(CST_BLTCPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTDPTH : %08X\n",CST_GETLNGU(CST_BLTDPTH,0xFFFFFFFF));
*/
		}
		else
			bltStart=BLT_Line;
	
		
}

