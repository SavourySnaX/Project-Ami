/*
 *  keyboard.h
 *  ami
 *
 *  Created by Lee Hammerton on 11/08/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

void KBD_InitialiseKeyboard();
void KBD_Update();

void KBD_AddKeyEvent(u_int8_t keycode);
void KBD_Acknowledge();