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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "win.h"
#include "user.h"
#include "message.h"
#include "callback.h"
#include "builtin16.h"
#include "debugtools.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(keyboard);

#include "pshpack1.h"
typedef struct _KBINFO
{
    BYTE Begin_First_Range;
    BYTE End_First_Range;
    BYTE Begin_Second_Range;
    BYTE End_Second_Range;
    WORD StateSize;
} KBINFO, *LPKBINFO;
#include "poppack.h"

/**********************************************************************/

typedef VOID CALLBACK (*LPKEYBD_EVENT_PROC)(BYTE,BYTE,DWORD,DWORD);

static LPKEYBD_EVENT_PROC DefKeybEventProc;
static LPBYTE pKeyStateTable;

/***********************************************************************
 *              KEYBOARD_CallKeybdEventProc
 */
static VOID WINAPI KEYBOARD_CallKeybdEventProc( FARPROC16 proc,
                                                BYTE bVk, BYTE bScan,
                                                DWORD dwFlags, DWORD dwExtraInfo )
{
    CONTEXT86 context;

    memset( &context, 0, sizeof(context) );
    context.SegCs = SELECTOROF( proc );
    context.Eip   = OFFSETOF( proc );
    context.Eax   = bVk | ((dwFlags & KEYEVENTF_KEYUP)? 0x8000 : 0);
    context.Ebx   = bScan | ((dwFlags & KEYEVENTF_EXTENDEDKEY) ? 0x100 : 0);
    context.Esi   = LOWORD( dwExtraInfo );
    context.Edi   = HIWORD( dwExtraInfo );

    wine_call_to_16_regs_short( &context, 0 );
}

/***********************************************************************
 *		Inquire (KEYBOARD.1)
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
 *		Enable (KEYBOARD.2)
 */
VOID WINAPI KEYBOARD_Enable( FARPROC16 proc, LPBYTE lpKeyState )
{
    if (DefKeybEventProc) THUNK_Free( (FARPROC)DefKeybEventProc );
    DefKeybEventProc = (LPKEYBD_EVENT_PROC)THUNK_Alloc( proc, (RELAY)KEYBOARD_CallKeybdEventProc );
    pKeyStateTable = lpKeyState;

    memset( lpKeyState, 0, 256 ); /* all states to false */
}

/***********************************************************************
 *		Disable (KEYBOARD.3)
 */
VOID WINAPI KEYBOARD_Disable(VOID)
{
  THUNK_Free( (FARPROC)DefKeybEventProc );
  
  DefKeybEventProc = NULL;
  pKeyStateTable = NULL;
}


/**********************************************************************
 *		SetSpeed (KEYBOARD.7)
 */
WORD WINAPI SetSpeed16(WORD unused)
{
    FIXME("(%04x): stub\n", unused);
    return 0xffff;
}

/**********************************************************************
 *		ScreenSwitchEnable (KEYBOARD.100)
 */
VOID WINAPI ScreenSwitchEnable16(WORD unused)
{
  FIXME("(%04x): stub\n", unused);
}

/**********************************************************************
 *		OemKeyScan (KEYBOARD.128)
 *		OemKeyScan (USER32.@)
 */
DWORD WINAPI OemKeyScan(WORD wOemChar)
{
  TRACE("*OemKeyScan (%d)\n", wOemChar);

  return wOemChar;
}

/**********************************************************************
 *		VkKeyScan (KEYBOARD.129)
 */
WORD WINAPI VkKeyScan16(CHAR cChar)
{
    return VkKeyScanA( cChar );
}

/******************************************************************************
 *		GetKeyboardType (KEYBOARD.130)
 */
INT16 WINAPI GetKeyboardType16(INT16 nTypeFlag)
{
    return GetKeyboardType( nTypeFlag );
}

/******************************************************************************
 *		MapVirtualKey (KEYBOARD.131)
 *
 * MapVirtualKey translates keycodes from one format to another
 */
UINT16 WINAPI MapVirtualKey16(UINT16 wCode, UINT16 wMapType)
{
    return MapVirtualKeyA(wCode,wMapType);
}

/****************************************************************************
 *		GetKBCodePage (KEYBOARD.132)
 */
INT16 WINAPI GetKBCodePage16(void)
{
    return GetKBCodePage();
}

/****************************************************************************
 *		GetKeyNameText (KEYBOARD.133)
 */
INT16 WINAPI GetKeyNameText16(LONG lParam, LPSTR lpBuffer, INT16 nSize)
{
    return GetKeyNameTextA( lParam, lpBuffer, nSize );
}

/****************************************************************************
 *		ToAscii (KEYBOARD.4)
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
    return ToAscii( virtKey, scanCode, lpKeyState, lpChar, flags );
}

/***********************************************************************
 *		MessageBeep (USER.104)
 */
void WINAPI MessageBeep16( UINT16 i )
{
    MessageBeep( i );
}

/***********************************************************************
 *		MessageBeep (USER32.@)
 */
BOOL WINAPI MessageBeep( UINT i )
{
    BOOL active = TRUE;
    SystemParametersInfoA( SPI_GETBEEP, 0, &active, FALSE );
    if (active) USER_Driver.pBeep();
    return TRUE;
}
