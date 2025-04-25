/*
 * KEYBOARD driver
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1996 Albrecht Kleine
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(keyboard);

#pragma pack(push,1)
typedef struct _KBINFO
{
    BYTE Begin_First_Range;
    BYTE End_First_Range;
    BYTE Begin_Second_Range;
    BYTE End_Second_Range;
    WORD StateSize;
} KBINFO, *LPKBINFO;
#pragma pack(pop)

static FARPROC16 DefKeybEventProc;
static LPBYTE pKeyStateTable;

/***********************************************************************
 *		Inquire (KEYBOARD.1)
 */
WORD WINAPI Inquire16(LPKBINFO kbInfo)
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
VOID WINAPI Enable16( FARPROC16 proc, LPBYTE lpKeyState )
{
    DefKeybEventProc = proc;
    pKeyStateTable = lpKeyState;

    memset( lpKeyState, 0, 256 ); /* all states to false */
}

/***********************************************************************
 *		Disable (KEYBOARD.3)
 */
VOID WINAPI Disable16(VOID)
{
    DefKeybEventProc = NULL;
    pKeyStateTable = NULL;
}

/****************************************************************************
 *		ToAscii (KEYBOARD.4)
 */
INT16 WINAPI ToAscii16(UINT16 virtKey,UINT16 scanCode, LPBYTE lpKeyState,
                       LPVOID lpChar, UINT16 flags)
{
    return ToAscii( virtKey, scanCode, lpKeyState, lpChar, flags );
}

/***********************************************************************
 *           AnsiToOem   (KEYBOARD.5)
 */
INT16 WINAPI AnsiToOem16( LPCSTR s, LPSTR d )
{
    CharToOemA( s, d );
    return -1;
}

/***********************************************************************
 *           OemToAnsi   (KEYBOARD.6)
 */
INT16 WINAPI OemToAnsi16( LPCSTR s, LPSTR d )
{
    OemToCharA( s, d );
    return -1;
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
 */
DWORD WINAPI OemKeyScan16(WORD wOemChar)
{
    return OemKeyScan( wOemChar );
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

/***********************************************************************
 *           AnsiToOemBuff   (KEYBOARD.134)
 */
void WINAPI AnsiToOemBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    if (len != 0) CharToOemBuffA( s, d, len );
}

/***********************************************************************
 *           OemToAnsiBuff   (KEYBOARD.135)
 */
void WINAPI OemToAnsiBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    if (len != 0) OemToCharBuffA( s, d, len );
}
