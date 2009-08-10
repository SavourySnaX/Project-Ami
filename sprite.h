/*
 *  sprite.h
 *  ami
 *
 *  Created by Lee Hammerton on 07/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "mytypes.h"

void SPR_InitialiseSprites();
void SPR_Update();

int SPR_GetColourNum(int spNum,u_int16_t sprPtr,u_int16_t sprCtl,u_int16_t sprPos,u_int16_t sprDatA,u_int16_t sprDatB, u_int16_t hpos,u_int16_t vpos);