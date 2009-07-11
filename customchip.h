/*
 *  customchip.h
 *  ami
 *
 *  Created by Lee Hammerton on 11/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

void MEM_initialiseCustom();

u_int8_t MEM_getByteCustom(u_int32_t upper24,u_int32_t lower16);
void MEM_setByteCustom(u_int32_t upper24,u_int32_t lower16,u_int8_t byte);
