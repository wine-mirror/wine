/*
 * Keyboard related functions
 *
 * Copyright 1993 Bob Amstadt
 */

static char Copyright[] = "Copyright  Bob Amstadt, 1993";

#include "win.h"

extern BOOL MouseButtonsStates[3];

/**********************************************************************
 *		GetKeyState	(USER.106)
 */
int GetKeyState(int keycode)
{
	switch(keycode) {
		case VK_LBUTTON:
		    return MouseButtonsStates[0];
		case VK_MBUTTON:
		    return MouseButtonsStates[1];
		case VK_RBUTTON:
		    return MouseButtonsStates[2];
		default:
		    return 0;
		}
}
