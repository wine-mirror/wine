/*
 * KEYBOARD driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine 
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "winuser.h"
#include "wine/keyboard16.h"
#include "win.h"
#include "heap.h"
#include "keyboard.h"
#include "message.h"
#include "debug.h"
#include "debugtools.h"
#include "struct32.h"
#include "winerror.h"

static LPKEYBD_EVENT_PROC DefKeybEventProc = NULL;
LPBYTE pKeyStateTable = NULL;

#ifndef X_DISPLAY_MISSING
extern KEYBOARD_DRIVER X11DRV_KEYBOARD_Driver;
#else /* X_DISPLAY_MISSING */
extern KEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver;
#endif /* X_DISPLAY_MISSING */

/***********************************************************************
 *           KEYBOARD_GetDriver
 */
KEYBOARD_DRIVER *KEYBOARD_GetDriver()
{
#ifndef X_DISPLAY_MISSING
  return &X11DRV_KEYBOARD_Driver;
#else /* X_DISPLAY_MISSING */
  return &TTYDRV_KEYBOARD_Driver;
#endif /* X_DISPLAY_MISSING */
}

/***********************************************************************
 *           KEYBOARD_Inquire			(KEYBOARD.1)
 */
WORD WINAPI KEYBOARD_Inquire(LPKBINFO kbInfo) 
{
  kbInfo->Begin_First_Range = 0;
  kbInfo->End_First_Range = 0;
  kbInfo->Begin_Second_Range = 0;
  kbInfo->End_Second_Range = 0;
  kbInfo->StateSize = 16; 
  
  return sizeof(KBINFO);
}

/***********************************************************************
 *           KEYBOARD_Enable			(KEYBOARD.2)
 */
VOID WINAPI KEYBOARD_Enable( LPKEYBD_EVENT_PROC lpKeybEventProc, 
                             LPBYTE lpKeyState )
{
  static BOOL initDone = FALSE;
  
  DefKeybEventProc = lpKeybEventProc;
  pKeyStateTable = lpKeyState;
  
  /* all states to false */
  memset( lpKeyState, 0, sizeof(lpKeyState) );
  
  if (!initDone) KEYBOARD_GetDriver()->pInit();
  initDone = TRUE;
}

/***********************************************************************
 *           KEYBOARD_Disable			(KEYBOARD.3)
 */
VOID WINAPI KEYBOARD_Disable(VOID)
{
  DefKeybEventProc = NULL;
  pKeyStateTable = NULL;
}

/***********************************************************************
 *           KEYBOARD_SendEvent
 */
void KEYBOARD_SendEvent( BYTE bVk, BYTE bScan, DWORD dwFlags,
                         DWORD posX, DWORD posY, DWORD time )
{
  WINE_KEYBDEVENT wke;
  
  if ( !DefKeybEventProc ) return;
  
  TRACE( event, "(%d,%d,%04lX)\n", bVk, bScan, dwFlags );
  
  wke.magic = WINE_KEYBDEVENT_MAGIC;
  wke.posX  = posX;
  wke.posY  = posY;
  wke.time  = time;
  
  DefKeybEventProc( bVk, bScan, dwFlags, (DWORD)&wke );
}

/**********************************************************************
 *           ScreenSwitchEnable      (KEYBOARD.100)
 */
VOID WINAPI ScreenSwitchEnable16(WORD unused)
{
  FIXME(keyboard,"(%04x): stub\n",unused);
}

/**********************************************************************
 *           OemKeyScan      (KEYBOARD.128)(USER32.401)
 */
DWORD WINAPI OemKeyScan(WORD wOemChar)
{
  TRACE(keyboard,"*OemKeyScan (%d)\n",wOemChar);

  return wOemChar;
}

/**********************************************************************
 *    	VkKeyScan			[KEYBOARD.129]
 */
/* VkKeyScan translates an ANSI character to a virtual-key and shift code
 * for the current keyboard.
 * high-order byte yields :
 *	0	Unshifted
 *	1	Shift
 *	2	Ctrl
 *	3-5	Shift-key combinations that are not used for characters
 *	6	Ctrl-Alt
 *	7	Ctrl-Alt-Shift
 *	I.e. :	Shift = 1, Ctrl = 2, Alt = 4.
 * FIXME : works ok except for dead chars :
 * VkKeyScan '^'(0x5e, 94) ... got keycode 00 ... returning 00
 * VkKeyScan '`'(0x60, 96) ... got keycode 00 ... returning 00
 */

WORD WINAPI VkKeyScan16(CHAR cChar)
{
  return KEYBOARD_GetDriver()->pVkKeyScan(cChar);
}

/******************************************************************************
 *    	GetKeyboardType16      (KEYBOARD.130)
 */
INT16 WINAPI GetKeyboardType16(INT16 nTypeFlag)
{
  TRACE(keyboard,"(%d)\n",nTypeFlag);
  switch(nTypeFlag)
    {
    case 0:      /* Keyboard type */
      return 4;    /* AT-101 */
      break;
    case 1:      /* Keyboard Subtype */
      return 0;    /* There are no defined subtypes */
      break;
    case 2:      /* Number of F-keys */
      return 12;   /* We're doing an 101 for now, so return 12 F-keys */
      break;
    default:     
      WARN(keyboard, "Unknown type\n");
      return 0;    /* The book says 0 here, so 0 */
    }
}

/******************************************************************************
 *    	MapVirtualKey16      (KEYBOARD.131)
 *
 * MapVirtualKey translates keycodes from one format to another
 */
UINT16 WINAPI MapVirtualKey16(UINT16 wCode, UINT16 wMapType)
{
  return KEYBOARD_GetDriver()->pMapVirtualKey(wCode,wMapType);
}

/****************************************************************************
 *	GetKBCodePage16   (KEYBOARD.132)
 */
INT16 WINAPI GetKBCodePage16(void)
{
  TRACE(keyboard,"(void)\n");
  return 850;
}

/****************************************************************************
 *	GetKeyNameText16   (KEYBOARD.133)
 */
INT16 WINAPI GetKeyNameText16(LONG lParam, LPSTR lpBuffer, INT16 nSize)
{
  return KEYBOARD_GetDriver()->pGetKeyNameText(lParam, lpBuffer, nSize);
}

/****************************************************************************
 *	ToAscii   (KEYBOARD.4)
 *
 * The ToAscii function translates the specified virtual-key code and keyboard
 * state to the corresponding Windows character or characters.
 *
 * If the specified key is a dead key, the return value is negative. Otherwise,
 * it is one of the following values:
 * Value	Meaning
 * 0	The specified virtual key has no translation for the current state of the keyboard.
 * 1	One Windows character was copied to the buffer.
 * 2	Two characters were copied to the buffer. This usually happens when a
 *      dead-key character (accent or diacritic) stored in the keyboard layout cannot
 *      be composed with the specified virtual key to form a single character.
 *
 * FIXME : should do the above (return 2 for non matching deadchar+char combinations)
 *
 */
INT16 WINAPI ToAscii16(UINT16 virtKey,UINT16 scanCode, LPBYTE lpKeyState, 
                       LPVOID lpChar, UINT16 flags) 
{
    return KEYBOARD_GetDriver()->pToAscii(
        virtKey, scanCode, lpKeyState, lpChar, flags
    );
}

