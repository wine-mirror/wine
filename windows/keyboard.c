/*
 * Keyboard related functions
 *
 * Copyright 1993 Bob Amstadt
 */
#include <string.h>
#include "win.h"
#include "windows.h"

extern BOOL MouseButtonsStates[3];
extern BOOL AsyncMouseButtonsStates[3];
extern BYTE KeyStateTable[256];
extern BYTE AsyncKeyStateTable[256];

/**********************************************************************
 *		GetKeyState			[USER.106]
 */
INT GetKeyState(INT keycode)
{
    INT retval;

    switch(keycode) {
     case VK_LBUTTON:
	return MouseButtonsStates[0];
     case VK_MBUTTON:
	return MouseButtonsStates[1];
     case VK_RBUTTON:
	return MouseButtonsStates[2];
     default:
	retval = ( (INT)(KeyStateTable[keycode] & 0x80) << 8 ) |
		   (INT)(KeyStateTable[keycode] & 0x01);
    }

    return retval;
}

/**********************************************************************
 *		GetKeyboardState			[USER.222]
 */
void GetKeyboardState(BYTE *lpKeyState)
{
    if (lpKeyState != NULL) {
	KeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] >> 8;
	KeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] >> 8;
	KeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] >> 8;
	memcpy(lpKeyState, KeyStateTable, 256);
    }
}

/**********************************************************************
 *      SetKeyboardState            [USER.223]
 */
void SetKeyboardState(BYTE *lpKeyState)
{
    if (lpKeyState != NULL) {
	memcpy(KeyStateTable, lpKeyState, 256);
	MouseButtonsStates[0] = KeyStateTable[VK_LBUTTON]? 0x8000: 0;
	MouseButtonsStates[1] = KeyStateTable[VK_MBUTTON]? 0x8000: 0;
	MouseButtonsStates[2] = KeyStateTable[VK_RBUTTON]? 0x8000: 0;
    }
}

/**********************************************************************
 *            GetAsyncKeyState        (USER.249)
 *
 *	Determine if a key is or was pressed.  retval has high-order 
 * bit set to 1 if currently pressed, low-order bit set to 1 if key has
 * been pressed.
 *
 *	This uses the variable AsyncMouseButtonsStates and
 * AsyncKeyStateTable (set in event.c) which have the mouse button
 * number or key number (whichever is applicable) set to true if the
 * mouse or key had been depressed since the last call to 
 * GetAsyncKeyState.
 */
int GetAsyncKeyState(int nKey)
{
    short retval;	

    switch (nKey) {
     case VK_LBUTTON:
	retval = AsyncMouseButtonsStates[0] | 
	MouseButtonsStates[0]? 0x0001: 0;
	break;
     case VK_MBUTTON:
	retval = AsyncMouseButtonsStates[1] |
	MouseButtonsStates[1]? 0x0001: 0;
	break;
     case VK_RBUTTON:
	retval = AsyncMouseButtonsStates[2] |
	MouseButtonsStates[2]? 0x0001: 0;
	break;
     default:
	retval = AsyncKeyStateTable[nKey] | 
	(KeyStateTable[nKey] ? 0x8000 : 0);
	break;
    }

    memset( AsyncMouseButtonsStates, 0, 3 );  /* all states to false */
    memset( AsyncKeyStateTable, 0, 256 );

    return retval;
}

