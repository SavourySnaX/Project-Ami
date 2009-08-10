/*
 *  disk.h
 *  ami
 *
 *  Created by Lee Hammerton on 03/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "mytypes.h"

void DSK_InitialiseDisk();
void DSK_Update();

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
