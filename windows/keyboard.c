/*
 * Keyboard related functions
 *
 * Copyright 1993 Bob Amstadt
 */
#include <stdio.h>
#include <string.h>
#include "win.h"
#include "windows.h"
#include "debug.h"

extern BOOL MouseButtonsStates[3];
extern BOOL AsyncMouseButtonsStates[3];
extern BYTE InputKeyStateTable[256];
BYTE AsyncKeyStateTable[256];

extern BYTE QueueKeyStateTable[256];

/**********************************************************************
 *		GetKeyState			[USER.106]
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 390)
 */
INT GetKeyState(INT keycode)
{
    INT retval;

    if (keycode >= 'a' && keycode <= 'z')
        keycode += 'A' - 'a';
    switch(keycode) {
     case VK_LBUTTON:
	retval = MouseButtonsStates[0];
     case VK_MBUTTON:
	retval = MouseButtonsStates[1];
     case VK_RBUTTON:
	retval = MouseButtonsStates[2];
     default:
	retval = ( (INT)(QueueKeyStateTable[keycode] & 0x80) << 8 ) |
		   (INT)(QueueKeyStateTable[keycode] & 0x01);
    }

    dprintf_key(stddeb, "GetKeyState(%x) -> %x\n", keycode, retval);
    return retval;
}

/**********************************************************************
 *		GetKeyboardState			[USER.222]
 * An application calls the GetKeyboardState function in response to a
 * keyboard-input message.  This function retrieves the state of the keyboard
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 387)
 */
void GetKeyboardState(BYTE *lpKeyState)
{
    if (lpKeyState != NULL) {
	QueueKeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] >> 8;
	QueueKeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] >> 8;
	QueueKeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] >> 8;
	memcpy(lpKeyState, QueueKeyStateTable, 256);
    }
}

/**********************************************************************
 *      SetKeyboardState            [USER.223]
 */
void SetKeyboardState(BYTE *lpKeyState)
{
    if (lpKeyState != NULL) {
	memcpy(QueueKeyStateTable, lpKeyState, 256);
	MouseButtonsStates[0] = QueueKeyStateTable[VK_LBUTTON]? 0x8000: 0;
	MouseButtonsStates[1] = QueueKeyStateTable[VK_MBUTTON]? 0x8000: 0;
	MouseButtonsStates[2] = QueueKeyStateTable[VK_RBUTTON]? 0x8000: 0;
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
	(InputKeyStateTable[nKey] ? 0x8000 : 0);
	break;
    }

    memset( AsyncMouseButtonsStates, 0, 3 );  /* all states to false */
    memset( AsyncKeyStateTable, 0, 256 );

    dprintf_key(stddeb, "GetAsyncKeyState(%x) -> %x\n", nKey, retval);
    return retval;
}
