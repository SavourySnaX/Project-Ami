/*
 *  blitter.c
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

#include "config.h"

#include "blitter.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"

typedef enum
{
	BLT_Stopped = 0,
	BLT_Read,		// Handles blitter src pre read, Blitter will read A&|B&|C data into BLTxDAT registers
	BLT_Read2,		// Blitter will read A&|B&|C data into BLTxDAT registers
	BLT_Line
} BLT_Ops;

typedef struct
{
	u_int8_t bits[16];
} MTerms;

MTerms	mTermA[65536];
MTerms	mTermB[65536];
MTerms	mTermC[65536];

typedef struct
{
	int			bltZero;
	int			bltSrcDone;
	u_int16_t	bltWidth;
	u_int16_t	bltHeight;
	u_int16_t	bltSX,bltSY,bltDX,bltDY;
	u_int16_t	bltAOldDat,bltBOldDat;		// Stores last shifted out data
	int32_t		bltDelta;
	BLT_Ops		bltStart;
} BLT_data;

BLT_data blt_Data;

void BLT_SaveState(FILE *outStream)
{
	fwrite(&blt_Data,1,sizeof(BLT_data),outStream);
}

void BLT_LoadState(FILE *inStream)
{
	fread(&blt_Data,1,sizeof(BLT_data),inStream);
}

void BLT_InitialiseBlitter()
{
	int a,b;
	
	blt_Data.bltZero=0;
	blt_Data.bltSrcDone=0;
	blt_Data.bltStart=BLT_Stopped;

	for (a=0;a<65536;a++)
	{
		for (b=0;b<16;b++)
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
		u_int32_t	bltAAddress = CST_GETLNGU(bltPtrStart,CUSTOM_CHIP_RAM_MASK);
		u_int16_t	bltADat = MEM_getWord(bltAAddress);

		CST_SETWRD(bltDataStart,bltADat,0xFFFF);
		bltAAddress+=blt_Data.bltDelta*2;
		
		if (lastWrd)
		{
			bltAAddress+=blt_Data.bltDelta*CST_GETWRDS(bltModStart,0xFFFE);
		}
		
		CST_SETLNG(bltPtrStart,bltAAddress,CUSTOM_CHIP_RAM_MASK);
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

	blt_Data.bltZero = blt_Data.bltZero && (bltD==0);
	
	return bltD;
}

void BLT_UpdateSrc()
{
	if (!blt_Data.bltSrcDone)
	{
		// The first/last word masks and shifters are applied before the register is held - so they are done in the read portion
		BLT_ReadNextBltSource(0x0800,CST_BLTAPTH,CST_BLTADAT,CST_BLTAMOD,blt_Data.bltSX==0,blt_Data.bltSX==(blt_Data.bltWidth-1),
							  CST_GETWRDU(CST_BLTAFWM,0xFFFF),CST_GETWRDU(CST_BLTALWM,0xFFFF),CST_GETWRDU(CST_BLTCON0,0xF000)>>12,&blt_Data.bltAOldDat);
		BLT_ReadNextBltSource(0x0400,CST_BLTBPTH,CST_BLTBDAT,CST_BLTBMOD,blt_Data.bltSX==0,blt_Data.bltSX==(blt_Data.bltWidth-1),
							  0xFFFF,0xFFFF,CST_GETWRDU(CST_BLTCON1,0xF000)>>12,&blt_Data.bltBOldDat);
		BLT_ReadNextBltSource(0x0200,CST_BLTCPTH,CST_BLTCDAT,CST_BLTCMOD,blt_Data.bltSX==0,blt_Data.bltSX==(blt_Data.bltWidth-1),
							  0xFFFF,0xFFFF,0,0);
		
	}	
}

void BLT_UpdateDst()
{
	if (CST_GETWRDU(CST_BLTCON0,0x0100) && CST_GETWRDU(CST_DMACONR,0x0240)==0x0240)		// DST is enabled
	{
		u_int32_t	bltDAddress = CST_GETLNGU(CST_BLTDPTH,CUSTOM_CHIP_RAM_MASK);
		
		MEM_setWord(bltDAddress,CST_GETWRDU(CST_BLTDDAT,0xFFFF));
		bltDAddress+=blt_Data.bltDelta * 2;
		
		if (blt_Data.bltDX==(blt_Data.bltWidth-1))
		{
			bltDAddress+=blt_Data.bltDelta * CST_GETWRDS(CST_BLTDMOD,0xFFFE);
		}
		
		CST_SETLNG(CST_BLTDPTH,bltDAddress,CUSTOM_CHIP_RAM_MASK);
	}
	
	// Check End Condition
	blt_Data.bltDX++;
	if (blt_Data.bltDX == blt_Data.bltWidth)
	{
		blt_Data.bltDX = 0;
		blt_Data.bltDY++;
		if (blt_Data.bltDY>=blt_Data.bltHeight)
		{
			// blitter finished, clear BLTBUSY and set BLTComplete
			blt_Data.bltStart=BLT_Stopped;
			
			CST_ORWRD(CST_INTREQR,0x0040);			// set interrupt blitter finished
			CST_ANDWRD(CST_DMACONR,~0x4000);
			
			if (blt_Data.bltZero)
			{
				CST_ORWRD(CST_DMACONR,0x2000);		// BLT  all zeros
			}
			
		}
	}
}

u_int16_t BLT_Mask(u_int16_t bltA)
{
		if (blt_Data.bltSX==0)
		{
			bltA&=CST_GETWRDU(CST_BLTAFWM,0xFFFF);
		}
		if (blt_Data.bltSX==(blt_Data.bltWidth-1))
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

	bltA=BLT_Mask(bltA);
		
	bltA=BLT_Shift(bltA,CST_GETWRDU(CST_BLTCON0,0xF000)>>12,&blt_Data.bltAOldDat);	

	bltB=BLT_Shift(bltB,CST_GETWRDU(CST_BLTCON1,0xF000)>>12,&blt_Data.bltBOldDat);
	
	bltC=CST_GETWRDU(CST_BLTCDAT,0xFFFF);

	if (!CST_GETWRDU(CST_BLTCON1,0x0001))
	{
		// Check End Src Read Condition
		blt_Data.bltSX++;
		if (blt_Data.bltSX == blt_Data.bltWidth)
		{
			blt_Data.bltSX = 0;
			blt_Data.bltAOldDat=0;
			blt_Data.bltBOldDat=0;
			blt_Data.bltSY++;
			if (blt_Data.bltSY>=blt_Data.bltHeight)
			{
				blt_Data.bltSrcDone=1;
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
	
	switch (blt_Data.bltStart)
	{
		case BLT_Stopped:
			break;
		case BLT_Read:

			CST_ORWRD(CST_DMACONR,0x4000);

			BLT_UpdateSrc();
			
			BLT_UpdateClc();

//			BLT_UpdateDst();				// swap this and next line comment over if trying for Preread
			
			blt_Data.bltStart=BLT_Read2;
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
				CST_SETWRD(CST_BLTCDAT,MEM_getWord(CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK)),0xFFFF);
			}

			BLT_UpdateClc();
			
			if (!CST_GETWRDU(CST_BLTCON1,0x0040))
			{
				if (CST_GETWRDU(CST_BLTCON0,0x0800))
				{
					addr = CST_GETLNGU(CST_BLTAPTH,CUSTOM_CHIP_RAM_MASK);
					addr+=CST_GETWRDS(CST_BLTAMOD,0xFFFE);
					CST_SETLNG(CST_BLTAPTH,addr,CUSTOM_CHIP_RAM_MASK);
				}
				if (CST_GETWRDU(CST_BLTCON1,0x0010))
				{
					if (CST_GETWRDU(CST_BLTCON1,0x0008))
					{
						//decy
						addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
						addr-=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
						CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
					}
					else
					{
						//incy
						addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
						addr+=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
						CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
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
							addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
							addr-=2;
							CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
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
							addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
							addr+=2;
							CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
						}
					}
				}
			}
			else
			{
				if (CST_GETWRDU(CST_BLTCON0,0x0800))
				{
					addr = CST_GETLNGU(CST_BLTAPTH,CUSTOM_CHIP_RAM_MASK);
					addr+=CST_GETWRDS(CST_BLTBMOD,0xFFFE);
					CST_SETLNG(CST_BLTAPTH,addr,CUSTOM_CHIP_RAM_MASK);
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
						addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
						addr-=2;
						CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
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
						addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
						addr+=2;
						CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
					}
				}
			}
			else
			{
				if (CST_GETWRDU(CST_BLTCON1,0x0004))
				{
					//decy
					addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
					addr-=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
					CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
				}
				else
				{
					//incy
					addr = CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK);
					addr+=CST_GETWRDS(CST_BLTCMOD,0xFFFE);
					CST_SETLNG(CST_BLTCPTH,addr,CUSTOM_CHIP_RAM_MASK);
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
				MEM_setWord(CST_GETLNGU(CST_BLTDPTH,CUSTOM_CHIP_RAM_MASK), CST_GETWRDU(CST_BLTDDAT,0xFFFF));
			}

			CST_SETLNG(CST_BLTDPTH,CST_GETLNGU(CST_BLTCPTH,CUSTOM_CHIP_RAM_MASK),CUSTOM_CHIP_RAM_MASK);

			blt_Data.bltHeight--;
			if (blt_Data.bltHeight==0)
			{
			// blitter finished, clear BLTBUSY and set BLTComplete
			blt_Data.bltStart=BLT_Stopped;
			
			CST_ORWRD(CST_INTREQR,0x0040);			// set interrupt blitter finished
			CST_ANDWRD(CST_DMACONR,~0x4000);
			
			if (blt_Data.bltZero)
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
	
		blt_Data.bltDX=0;
		blt_Data.bltDY=0;
		blt_Data.bltSX=0;
		blt_Data.bltSY=0;
		blt_Data.bltAOldDat=0;
		blt_Data.bltBOldDat=0;
		
//		if (!CST_GETWRDU(CST_BLTCON1,0x0001))
//			return;
		
//		lCount++;
//		if (lCount<151 || lCount>151)
//			return;
//		printf("lCount : %d\n",lCount);

		if (CST_GETWRDU(CST_BLTCON1,0x0001))
		{
			blt_Data.bltBOldDat=CST_GETWRDU(CST_BLTBDAT,0xFFFF);		// fixes line drawing problems
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
				blt_Data.bltDelta=-1;
			}
			else
			{
				blt_Data.bltDelta=1;
			}
  		}
		
		blt_Data.bltWidth=CST_GETWRDU(CST_BLTSIZE,0x003F);
		if (!blt_Data.bltWidth)
			blt_Data.bltWidth=64;
		blt_Data.bltHeight=CST_GETWRDU(CST_BLTSIZE,0xFFC0)>>6;
		if (!blt_Data.bltHeight)
			blt_Data.bltHeight=1024;
			
		blt_Data.bltZero=1;
		blt_Data.bltSrcDone=0;

		if (!CST_GETWRDU(CST_BLTCON1,0x0001))
		{
			blt_Data.bltStart=BLT_Read;
/*		printf("[WRN] Blitter Is Being Used\n");

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
		printf("Blitter Registers : BLTBPTH : %08X\n",CST_GETLNGU(CST_BLTBPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTCPTH : %08X\n",CST_GETLNGU(CST_BLTCPTH,0xFFFFFFFF));
		printf("Blitter Registers : BLTDPTH : %08X\n",CST_GETLNGU(CST_BLTDPTH,0xFFFFFFFF));
*/
		}
		else
			blt_Data.bltStart=BLT_Line;
	
		
}

