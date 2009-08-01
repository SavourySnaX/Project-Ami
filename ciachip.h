/*
 *  ciachip.h
 *  ami
 *
 *  Created by Lee Hammerton on 11/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "mytypes.h"

void CIA_InitialiseCustom();
void CIA_Update();

u_int8_t MEM_getByteCia(u_int32_t upper24,u_int32_t lower16);
void MEM_setByteCia(u_int32_t upper24,u_int32_t lower16,u_int8_t byte);

extern u_int32_t		todACnt;
extern u_int32_t		todBCnt;
extern u_int32_t		todAReadLatched;
extern u_int32_t		todBReadLatched;
extern u_int32_t		todAStart;
extern u_int32_t		todBStart;
