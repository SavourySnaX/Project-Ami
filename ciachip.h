/*
 *  ciachip.h
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

void CIA_InitialiseCustom();
void CIA_Update();

extern u_int8_t	*ciaMemory;

u_int8_t MEM_getByteCia(u_int32_t upper24,u_int32_t lower16);
void MEM_setByteCia(u_int32_t upper24,u_int32_t lower16,u_int8_t byte);

extern u_int32_t		todACnt;
extern u_int32_t		todBCnt;
extern u_int32_t		todAReadLatched;
extern u_int32_t		todBReadLatched;
extern u_int32_t		todAStart;
extern u_int32_t		todBStart;

extern u_int8_t			ciaa_icr;
extern u_int8_t			ciab_icr;

