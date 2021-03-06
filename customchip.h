/*
 *  customchip.h
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

#include 	"mytypes.h"

#define LINE_LENGTH	228

void CST_InitialiseCustom();
void CST_Update();

void CST_SaveState(FILE *outStream);
void CST_LoadState(FILE *inStream);

u_int8_t MEM_getByteCustom(u_int32_t upper24,u_int32_t lower16);
void MEM_setByteCustom(u_int32_t upper24,u_int32_t lower16,u_int8_t byte);

void MEM_GetHardwareDebug(u_int16_t regNum, char *buffer);
const char *MEM_GetHardwareName(u_int16_t regNum);

extern u_int8_t		horizontalClock;
extern u_int16_t	verticalClock;

extern u_int8_t		leftMouseUp,rightMouseUp;

extern u_int8_t	*cstMemory;

#define CST_GETWRDU(regNum,mask)		((u_int16_t)(((cstMemory[regNum]<<8)|(cstMemory[regNum+1]))&(mask)))
#define CST_GETWRDS(regNum,mask)		((int16_t)(((cstMemory[regNum]<<8)|(cstMemory[regNum+1]))&(mask)))
#define CST_GETLNGU(regNum,mask)		((u_int32_t)(((cstMemory[regNum]<<24)|(cstMemory[regNum+1]<<16)|(cstMemory[regNum+2]<<8)|(cstMemory[regNum+3]))&(mask)))

#define CST_SETWRD(regNum,value,mask)	{cstMemory[regNum]=((value)&(mask))>>8; cstMemory[regNum+1]=((value)&(mask));}
#define CST_SETLNG(regNum,value,mask)	{cstMemory[regNum]=((value)&(mask))>>24; cstMemory[regNum+1]=((value)&(mask))>>16; cstMemory[regNum+2]=((value)&(mask))>>8; cstMemory[regNum+3]=((value)&(mask));}

#define CST_ORWRD(regNum,value)			{cstMemory[regNum]|=((value)&0xFF00)>>8; cstMemory[regNum+1]|=((value)&0x00FF);}
#define CST_ANDWRD(regNum,value)		{cstMemory[regNum]&=((value)&0xFF00)>>8; cstMemory[regNum+1]&=((value)&0x00FF);}

#define CST_BLTDDAT	(0x0000)
#define CST_DMACONR	(0x0002)
#define CST_VPOSR	(0x0004)
#define CST_VHPOSR	(0x0006)
#define CST_DSKDATR	(0x0008)
#define CST_JOY0DAT	(0x000A)
#define CST_JOY1DAT	(0x000C)
#define CST_CLXDAT	(0x000E)
#define CST_ADKCONR	(0x0010)
#define CST_POT0DAT	(0x0012)
#define CST_POT1DAT	(0x0014)
#define CST_POTGOR	(0x0016)
#define CST_SERDATR	(0x0018)
#define CST_DSKBYTR	(0x001A)
#define CST_INTENAR	(0x001C)
#define CST_INTREQR	(0x001E)
#define CST_DSKPTH	(0x0020)
#define CST_DSKPTL	(0x0022)
#define CST_DSKLEN	(0x0024)
#define CST_DSKDAT	(0x0026)
#define CST_REFPT	(0x0028)
#define CST_VPOSW	(0x002A)
#define CST_VHPOSW	(0x002C)
#define CST_COPCON	(0x002E)
#define CST_SERDAT	(0x0030)
#define CST_SERPER	(0x0032)
#define CST_POTGO	(0x0034)
#define CST_JOYTEST	(0x0036)
#define CST_STREQU	(0x0038)
#define CST_STRVBL	(0x003A)
#define CST_STRHOR	(0x003C)
#define CST_STRLONG	(0x003E)
#define CST_BLTCON0	(0x0040)
#define CST_BLTCON1	(0x0042)
#define CST_BLTAFWM	(0x0044)
#define CST_BLTALWM	(0x0046)
#define CST_BLTCPTH	(0x0048)
#define CST_BLTCPTL	(0x004A)
#define CST_BLTBPTH	(0x004C)
#define CST_BLTBPTL	(0x004E)
#define CST_BLTAPTH	(0x0050)
#define CST_BLTAPTL	(0x0052)
#define CST_BLTDPTH	(0x0054)
#define CST_BLTDPTL	(0x0056)
#define CST_BLTSIZE	(0x0058)
#define CST_05A_ECS	(0x005A)
#define CST_05C_ECS	(0x005C)
#define CST_05E_ECS	(0x005E)
#define CST_BLTCMOD	(0x0060)
#define CST_BLTBMOD	(0x0062)
#define CST_BLTAMOD	(0x0064)
#define CST_BLTDMOD	(0x0066)
#define CST_068		(0x0068)
#define CST_06A		(0x006A)
#define CST_06C		(0x006C)
#define CST_06E		(0x006E)
#define CST_BLTCDAT	(0x0070)
#define CST_BLTBDAT	(0x0072)
#define CST_BLTADAT	(0x0074)
#define CST_076		(0x0076)
#define CST_SPRHDAT	(0x0078)
#define CST_07A		(0x007A)
#define CST_07C_ECS	(0x007C)
#define CST_DSKSYNC	(0x007E)
#define CST_COP1LCH	(0x0080)
#define CST_COP1LCL	(0x0082)
#define CST_COP2LCH	(0x0084)
#define CST_COP2LCL	(0x0086)
#define CST_COPJMP1	(0x0088)
#define CST_COPJMP2	(0x008A)
#define CST_COPINS	(0x008C)
#define CST_DIWSTRT	(0x008E)
#define CST_DIWSTOP	(0x0090)
#define CST_DDFSTRT	(0x0092)
#define CST_DDFSTOP	(0x0094)
#define CST_DMACON	(0x0096)
#define CST_CLXCON	(0x0098)
#define CST_INTENA	(0x009A)
#define CST_INTREQ	(0x009C)
#define CST_ADKCON	(0x009E)
#define CST_AUD0LCH	(0x00A0)
#define CST_AUD0LCL	(0x00A2)
#define CST_AUD0LEN	(0x00A4)
#define CST_AUD0PER	(0x00A6)
#define CST_AUD0VOL	(0x00A8)
#define CST_AUD0DAT	(0x00AA)
#define CST_0AC		(0x00AC)
#define CST_0AE		(0x00AE)
#define CST_AUD1LCH	(0x00B0)
#define CST_AUD1LCL	(0x00B2)
#define CST_AUD1LEN	(0x00B4)
#define CST_AUD1PER	(0x00B6)
#define CST_AUD1VOL	(0x00B8)
#define CST_AUD1DAT	(0x00BA)
#define CST_0BC		(0x00BC)
#define CST_0BE		(0x00BE)
#define CST_AUD2LCH	(0x00C0)
#define CST_AUD2LCL	(0x00C2)
#define CST_AUD2LEN	(0x00C4)
#define CST_AUD2PER	(0x00C6)
#define CST_AUD2VOL	(0x00C8)
#define CST_AUD2DAT	(0x00CA)
#define CST_0CC		(0x00CC)
#define CST_0CE		(0x00CE)
#define CST_AUD3LCH	(0x00D0)
#define CST_AUD3LCL	(0x00D2)
#define CST_AUD3LEN	(0x00D4)
#define CST_AUD3PER	(0x00D6)
#define CST_AUD3VOL	(0x00D8)
#define CST_AUD3DAT	(0x00DA)
#define CST_0DC		(0x00DC)
#define CST_0DE		(0x00DE)
#define CST_BPL1PTH	(0x00E0)
#define CST_BPL1PTL	(0x00E2)
#define CST_BPL2PTH	(0x00E4)
#define CST_BPL2PTL	(0x00E6)
#define CST_BPL3PTH	(0x00E8)
#define CST_BPL3PTL	(0x00EA)
#define CST_BPL4PTH	(0x00EC)
#define CST_BPL4PTL	(0x00EE)
#define CST_BPL5PTH	(0x00F0)
#define CST_BPL5PTL	(0x00F2)
#define CST_BPL6PTH	(0x00F4)
#define CST_BPL6PTL	(0x00F6)
#define CST_0F8		(0x00F8)
#define CST_0FA		(0x00FA)
#define CST_0FC		(0x00FC)
#define CST_0FE		(0x00FE)
#define CST_BPLCON0	(0x0100)
#define CST_BPLCON1	(0x0102)
#define CST_BPLCON2	(0x0104)
#define CST_106_ECS	(0x0106)
#define CST_BPL1MOD	(0x0108)
#define CST_BPL2MOD	(0x010A)
#define CST_10C		(0x010C)
#define CST_10E		(0x010E)
#define CST_BPL1DAT	(0x0110)
#define CST_BPL2DAT	(0x0112)
#define CST_BPL3DAT	(0x0114)
#define CST_BPL4DAT	(0x0116)
#define CST_BPL5DAT	(0x0118)
#define CST_BPL6DAT	(0x011A)
#define CST_11C		(0x011C)
#define CST_11E		(0x011E)
#define CST_SPR0PTH	(0x0120)
#define CST_SPR0PTL	(0x0122)
#define CST_SPR1PTH	(0x0124)
#define CST_SPR1PTL	(0x0126)
#define CST_SPR2PTH	(0x0128)
#define CST_SPR2PTL	(0x012A)
#define CST_SPR3PTH	(0x012C)
#define CST_SPR3PTL	(0x012E)
#define CST_SPR4PTH	(0x0130)
#define CST_SPR4PTL	(0x0132)
#define CST_SPR5PTH	(0x0134)
#define CST_SPR5PTL	(0x0136)
#define CST_SPR6PTH	(0x0138)
#define CST_SPR6PTL	(0x013A)
#define CST_SPR7PTH	(0x013C)
#define CST_SPR7PTL	(0x013E)
#define CST_SPR0POS	(0x0140)
#define CST_SPR0CTL	(0x0142)
#define CST_SPR0DATA	(0x0144)
#define CST_SPR0DATB	(0x0146)
#define CST_SPR1POS	(0x0148)
#define CST_SPR1CTL	(0x014A)
#define CST_SPR1DATA	(0x014C)
#define CST_SPR1DATB	(0x014E)
#define CST_SPR2POS	(0x0150)
#define CST_SPR2CTL	(0x0152)
#define CST_SPR2DATA	(0x0154)
#define CST_SPR2DATB	(0x0156)
#define CST_SPR3POS	(0x0158)
#define CST_SPR3CTL	(0x015A)
#define CST_SPR3DATA	(0x015C)
#define CST_SPR3DATB	(0x015E)
#define CST_SPR4POS	(0x0160)
#define CST_SPR4CTL	(0x0162)
#define CST_SPR4DATA	(0x0164)
#define CST_SPR4DATB	(0x0166)
#define CST_SPR5POS	(0x0168)
#define CST_SPR5CTL	(0x016A)
#define CST_SPR5DATA	(0x016C)
#define CST_SPR5DATB	(0x016E)
#define CST_SPR6POS	(0x0170)
#define CST_SPR6CTL	(0x0172)
#define CST_SPR6DATA	(0x0174)
#define CST_SPR6DATB	(0x0176)
#define CST_SPR7POS	(0x0178)
#define CST_SPR7CTL	(0x017A)
#define CST_SPR7DATA	(0x017C)
#define CST_SPR7DATB	(0x017E)
#define CST_COLOR00	(0x0180)
#define CST_COLOR01	(0x0182)
#define CST_COLOR02	(0x0184)
#define CST_COLOR03	(0x0186)
#define CST_COLOR04	(0x0188)
#define CST_COLOR05	(0x018A)
#define CST_COLOR06	(0x018C)
#define CST_COLOR07	(0x018E)
#define CST_COLOR08	(0x0190)
#define CST_COLOR09	(0x0192)
#define CST_COLOR10	(0x0194)
#define CST_COLOR11	(0x0196)
#define CST_COLOR12	(0x0198)
#define CST_COLOR13	(0x019A)
#define CST_COLOR14	(0x019C)
#define CST_COLOR15	(0x019E)
#define CST_COLOR16	(0x01A0)
#define CST_COLOR17	(0x01A2)
#define CST_COLOR18	(0x01A4)
#define CST_COLOR19	(0x01A6)
#define CST_COLOR20	(0x01A8)
#define CST_COLOR21	(0x01AA)
#define CST_COLOR22	(0x01AC)
#define CST_COLOR23	(0x01AE)
#define CST_COLOR24	(0x01B0)
#define CST_COLOR25	(0x01B2)
#define CST_COLOR26	(0x01B4)
#define CST_COLOR27	(0x01B6)
#define CST_COLOR28	(0x01B8)
#define CST_COLOR29	(0x01BA)
#define CST_COLOR30	(0x01BC)
#define CST_COLOR31	(0x01BE)
