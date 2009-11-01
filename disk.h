/*
 *  disk.h
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

#include "mytypes.h"

void DSK_InitialiseDisk();
void DSK_Update();

void DSK_SaveState(FILE *outStream);
void DSK_LoadState(FILE *inStream);

void DSK_NotifyDSKLEN(u_int16_t dskLen);

int	DSK_OnSyncWord();

void DSK_SelectDrive(int drive,int motor);

int DSK_Removed();
int DSK_Writeable();
int DSK_OnTrack(u_int8_t track);
int DSK_Ready();

void DSK_Step();
void DSK_SetMotor(int onOff);
void DSK_SetSide(int lower);
void DSK_SetDir(int toZero);
