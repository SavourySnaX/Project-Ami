/*
 *  memory.h
 *  ami
 *
 *  Created by Lee Hammerton on 06/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

void MEM_Initialise(unsigned char *_romPtr);

u_int8_t	MEM_getByte(u_int32_t address);
void		MEM_setByte(u_int32_t address,u_int8_t byte);
u_int16_t	MEM_getWord(u_int32_t address);
void		MEM_setWord(u_int32_t address,u_int16_t word);
u_int32_t	MEM_getLong(u_int32_t address);
void		MEM_setLong(u_int32_t address,u_int32_t dword);
