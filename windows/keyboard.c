/*
 * Keyboard related functions
 *
 * Copyright 1993 Bob Amstadt
 */

static char Copyright[] = "Copyright  Bob Amstadt, 1993";

#include "win.h"
#include "windows.h"

extern BOOL MouseButtonsStates[3];
extern BOOL AsyncMouseButtonsStates[3];

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


/**********************************************************************
 *
 *            GetAsyncKeyState        (USER.249)
 *
 *	Determine if a key is or was pressed.  retval has high-order 
 * byte set to 1 if currently pressed, low-order byte 1 if key has
 * been pressed.
 *
 *	This uses the variable AsyncMouseButtonsStates (set in event.c)
 * which have the mouse button number set to true if the mouse had been
 * depressed since the last call to GetAsyncKeyState.
 *
 *   	There should also be some keyboard stuff here... it isn't here
 * yet.
 */
int GetAsyncKeyState(int nKey)
{
	short 	retval;	

	switch (nKey) {

           case VK_LBUTTON:
		retval = AsyncMouseButtonsStates[0] | 
                              (MouseButtonsStates[0] << 8);
		break;
           case VK_MBUTTON:
                retval = AsyncMouseButtonsStates[1] |
                              (MouseButtonsStates[1] << 8);
		break;
           case VK_RBUTTON:
                retval = AsyncMouseButtonsStates[2] |
                              MouseButtonsStates[2] << 8;
		break;
           default:
                retval = 0;
		break;
        }

	bzero(AsyncMouseButtonsStates, 3);	/* all states to false */

	return retval;
}

