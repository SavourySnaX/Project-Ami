/*
 *  audio.c
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


#include "audio.h"
#include "memory.h"
#include "copper.h"
#include "customchip.h"
#include "sprite.h"

typedef struct
{
	u_int32_t	audio_ptr[4];
	u_int16_t	audio_len[4];
	u_int16_t	audio_per[4];
	u_int16_t	audio_percnt[4];
	u_int16_t	audio_samples[4];
	u_int16_t	outAudioCnt;
	int16_t		outAudioSample[4];
} AUD_data;

AUD_data aud_Data;

void AUD_SaveState(FILE *outStream)
{
	fwrite(&aud_Data,1,sizeof(AUD_data),outStream);
}

void AUD_LoadState(FILE *inStream)
{
	fread(&aud_Data,1,sizeof(AUD_data),inStream);
}

void AUD_InitialiseAudio()
{
	int a;
	
	for (a=0;a<4;a++)
	{
		aud_Data.audio_ptr[a]=0;				// Track position of waveform internally
		aud_Data.audio_len[a]=0;
		aud_Data.audio_percnt[a]=0;
		aud_Data.audio_per[a]=0;
		aud_Data.audio_samples[a]=0;
		aud_Data.outAudioSample[a]=0;
		aud_Data.outAudioCnt=0;

	}

}

void AUD_UpdateAudioChannel(int chn,u_int16_t imask,u_int16_t datBase,u_int16_t pthBase,u_int16_t lenBase,u_int16_t perBase,u_int16_t volBase)
{
	if (!aud_Data.audio_len[chn])
	{
		// Channel reset (reload length and ptr values)
		aud_Data.audio_ptr[chn]=CST_GETLNGU(pthBase,CUSTOM_CHIP_RAM_MASK);
		aud_Data.audio_len[chn]=CST_GETWRDU(lenBase,0xFFFF);
		aud_Data.audio_per[chn]=CST_GETWRDU(perBase,0xFFFF);
		aud_Data.audio_percnt[chn]=0;
		aud_Data.audio_samples[chn]=2;
		
		// Signal value read interrupt.
		CST_ORWRD(CST_INTREQR,imask);			// set interrupt audio finished
	}
	
	// Transfer next word (if we have processed at least 2 samples!)
	if (aud_Data.audio_samples[chn]>=2)
	{
		u_int16_t audioData;
		
		audioData=MEM_getWord(aud_Data.audio_ptr[chn]);
		CST_SETWRD(datBase,audioData,0xFFFF);
		aud_Data.audio_len[chn]--;
		aud_Data.audio_ptr[chn]+=2;
		aud_Data.audio_samples[chn]=0;
	}
}

void AUD_Tick(int chn, u_int16_t datBase, u_int16_t volBase,u_int16_t dmaMask)
{
	if (CST_GETWRDU(CST_DMACONR,dmaMask)==dmaMask)
	{
		aud_Data.audio_percnt[chn]++;
		if (aud_Data.audio_per[chn]==aud_Data.audio_percnt[chn])
		{
			int16_t sample = cstMemory[datBase+(aud_Data.audio_samples[chn]&1)];		// get sample
			aud_Data.audio_percnt[chn]=0;
			aud_Data.audio_samples[chn]++;
			
			sample*=CST_GETWRDU(volBase,0x00FF);
			
			aud_Data.outAudioSample[chn]=sample;
		}
	}
}

#if ENABLE_AUDIO_OUT
void _AudioAddData(u_int16_t chn1,u_int16_t chn2,u_int16_t chn3,u_int16_t chn4);
#endif
void AUD_Update()
{

	AUD_Tick(0,CST_AUD0DAT,CST_AUD0VOL,0x0201);
	AUD_Tick(1,CST_AUD1DAT,CST_AUD1VOL,0x0202);
	AUD_Tick(2,CST_AUD2DAT,CST_AUD2VOL,0x0204);
	AUD_Tick(3,CST_AUD3DAT,CST_AUD3VOL,0x0208);

#if ENABLE_AUDIO_OUT
	aud_Data.outAudioCnt++;
	if (aud_Data.outAudioCnt==162)
	{
		aud_Data.outAudioCnt=0;
		
		_AudioAddData(aud_Data.outAudioSample[0],aud_Data.outAudioSample[1],aud_Data.outAudioSample[2],aud_Data.outAudioSample[3]);
	}
#endif
		
	switch (horizontalClock)
	{
		case 0x0D:
			if (CST_GETWRDU(CST_DMACONR,0x0201)==0x0201)
			{
				AUD_UpdateAudioChannel(0,0x0080,CST_AUD0DAT,CST_AUD0LCH,CST_AUD0LEN,CST_AUD0PER,CST_AUD0VOL);
			}
			break;
			
		case 0x0F:
			if (CST_GETWRDU(CST_DMACONR,0x0202)==0x0202)
			{
				AUD_UpdateAudioChannel(1,0x0100,CST_AUD1DAT,CST_AUD1LCH,CST_AUD1LEN,CST_AUD1PER,CST_AUD1VOL);
			}
			break;
			
		case 0x11:
			if (CST_GETWRDU(CST_DMACONR,0x0204)==0x0204)
			{
				AUD_UpdateAudioChannel(2,0x0200,CST_AUD2DAT,CST_AUD2LCH,CST_AUD2LEN,CST_AUD2PER,CST_AUD2VOL);
			}
			break;
			
		case 0x13:
			if (CST_GETWRDU(CST_DMACONR,0x0208)==0x0208)
			{
				AUD_UpdateAudioChannel(3,0x0400,CST_AUD3DAT,CST_AUD3LCH,CST_AUD3LEN,CST_AUD3PER,CST_AUD3VOL);
			}
			break;
	}
}
