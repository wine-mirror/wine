/*
 * USER Input processing
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winnls.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/server.h"
#include "win.h"
#include "message.h"
#include "winternl.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DECLARE_DEBUG_CHANNEL(key);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);
WINE_DECLARE_DEBUG_CHANNEL(win);
WINE_DEFAULT_DEBUG_CHANNEL(event);

static BOOL InputEnabled = TRUE;
static BOOL SwappedButtons;

BYTE InputKeyStateTable[256];
BYTE AsyncKeyStateTable[256];

/* Storage for the USER-maintained mouse positions */
static DWORD PosX, PosY;

typedef union
{
    struct
    {
	unsigned long count : 16;
	unsigned long code : 8;
	unsigned long extended : 1;
	unsigned long unused : 2;
	unsigned long win_internal : 2;
	unsigned long context : 1;
	unsigned long previous : 1;
	unsigned long transition : 1;
    } lp1;
    unsigned long lp2;
} KEYLP;


/***********************************************************************
 *           get_key_state
 */
static WORD get_key_state(void)
{
    WORD ret = 0;

    if (SwappedButtons)
    {
        if (InputKeyStateTable[VK_RBUTTON] & 0x80) ret |= MK_LBUTTON;
        if (InputKeyStateTable[VK_LBUTTON] & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (InputKeyStateTable[VK_LBUTTON] & 0x80) ret |= MK_LBUTTON;
        if (InputKeyStateTable[VK_RBUTTON] & 0x80) ret |= MK_RBUTTON;
    }
    if (InputKeyStateTable[VK_MBUTTON] & 0x80)  ret |= MK_MBUTTON;
    if (InputKeyStateTable[VK_SHIFT] & 0x80)    ret |= MK_SHIFT;
    if (InputKeyStateTable[VK_CONTROL] & 0x80)  ret |= MK_CONTROL;
    if (InputKeyStateTable[VK_XBUTTON1] & 0x80) ret |= MK_XBUTTON1;
    if (InputKeyStateTable[VK_XBUTTON2] & 0x80) ret |= MK_XBUTTON2;
    return ret;
}


/***********************************************************************
 *           queue_raw_hardware_message
 *
 * Add a message to the raw hardware queue.
 * Note: the position is relative to the desktop window.
 */
static void queue_raw_hardware_message( UINT message, WPARAM wParam, LPARAM lParam,
                                        int xPos, int yPos, DWORD time, ULONG_PTR extraInfo )
{
    SERVER_START_REQ( send_message )
    {
        req->id     = GetCurrentThreadId();
        req->type   = MSG_HARDWARE_RAW;
        req->win    = 0;
        req->msg    = message;
        req->wparam = wParam;
        req->lparam = lParam;
        req->x      = xPos;
        req->y      = yPos;
        req->time   = time;
        req->info   = extraInfo;
        req->timeout = 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           queue_kbd_event
 *
 * Put a keyboard event into a thread queue
 */
static void queue_kbd_event( const KEYBDINPUT *ki, UINT injected_flags )
{
    UINT message;
    KEYLP keylp;
    KBDLLHOOKSTRUCT hook;

    keylp.lp2 = 0;
    keylp.lp1.count = 1;
    keylp.lp1.code = ki->wScan;
    keylp.lp1.extended = (ki->dwFlags & KEYEVENTF_EXTENDEDKEY) != 0;
    keylp.lp1.win_internal = 0; /* this has something to do with dialogs,
                                * don't remember where I read it - AK */
                                /* it's '1' under windows, when a dialog box appears
                                 * and you press one of the underlined keys - DF*/

    if (ki->dwFlags & KEYEVENTF_KEYUP )
    {
        BOOL sysKey = (InputKeyStateTable[VK_MENU] & 0x80) &&
                      !(InputKeyStateTable[VK_CONTROL] & 0x80);
        InputKeyStateTable[ki->wVk] &= ~0x80;
        keylp.lp1.previous = 1;
        keylp.lp1.transition = 1;
        message = sysKey ? WM_SYSKEYUP : WM_KEYUP;
    }
    else
    {
        keylp.lp1.previous = (InputKeyStateTable[ki->wVk] & 0x80) != 0;
        keylp.lp1.transition = 0;
        if (!(InputKeyStateTable[ki->wVk] & 0x80)) InputKeyStateTable[ki->wVk] ^= 0x01;
        InputKeyStateTable[ki->wVk] |= 0x80;
        AsyncKeyStateTable[ki->wVk] |= 0x80;

        message = (InputKeyStateTable[VK_MENU] & 0x80) && !(InputKeyStateTable[VK_CONTROL] & 0x80)
              ? WM_SYSKEYDOWN : WM_KEYDOWN;
    }

    if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP )
        keylp.lp1.context = (InputKeyStateTable[VK_MENU] & 0x80) != 0; /* 1 if alt */

    TRACE_(key)(" wParam=%04x, lParam=%08lx, InputKeyState=%x\n",
                ki->wVk, keylp.lp2, InputKeyStateTable[ki->wVk] );

    hook.vkCode      = ki->wVk;
    hook.scanCode    = ki->wScan;
    hook.flags       = (keylp.lp2 >> 24) | injected_flags;
    hook.time        = ki->time;
    hook.dwExtraInfo = ki->dwExtraInfo;
    if (!HOOK_CallHooks( WH_KEYBOARD_LL, HC_ACTION, message, (LPARAM)&hook, TRUE ))
        queue_raw_hardware_message( message, ki->wVk, keylp.lp2,
                                    PosX, PosY, ki->time, ki->dwExtraInfo );
}


/***********************************************************************
 *           queue_raw_mouse_message
 */
static void queue_raw_mouse_message( UINT message, UINT flags, INT x, INT y, const MOUSEINPUT *mi )
{
    MSLLHOOKSTRUCT hook;

    hook.pt.x        = x;
    hook.pt.y        = y;
    hook.mouseData   = MAKELONG( 0, mi->mouseData );
    hook.flags       = flags;
    hook.time        = mi->time;
    hook.dwExtraInfo = mi->dwExtraInfo;

    if (!HOOK_CallHooks( WH_MOUSE_LL, HC_ACTION, message, (LPARAM)&hook, TRUE ))
        queue_raw_hardware_message( message, MAKEWPARAM( get_key_state(), mi->mouseData ),
                                    0, x, y, mi->time, mi->dwExtraInfo );
}


/***********************************************************************
 *		queue_mouse_event
 */
static void queue_mouse_event( const MOUSEINPUT *mi, UINT flags )
{
    if (mi->dwFlags & MOUSEEVENTF_ABSOLUTE)
    {
        PosX = (mi->dx * GetSystemMetrics(SM_CXSCREEN)) >> 16;
        PosY = (mi->dy * GetSystemMetrics(SM_CYSCREEN)) >> 16;
    }
    else if (mi->dwFlags & MOUSEEVENTF_MOVE)
    {
        int width  = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);
        long posX = (long) PosX, posY = (long) PosY;
        int accel[3];
        int accelMult;

        /* dx and dy can be negative numbers for relative movements */
        SystemParametersInfoA(SPI_GETMOUSE, 0, accel, 0);

        accelMult = 1;
        if (mi->dx > accel[0] && accel[2] != 0)
        {
            accelMult = 2;
            if ((mi->dx > accel[1]) && (accel[2] == 2))
            {
                accelMult = 4;
            }
        }
        posX += (long)mi->dx * accelMult;

        accelMult = 1;
        if (mi->dy > accel[0] && accel[2] != 0)
        {
            accelMult = 2;
            if ((mi->dy > accel[1]) && (accel[2] == 2))
            {
                accelMult = 4;
            }
        }
        posY += (long)mi->dy * accelMult;

        /* Clip to the current screen size */
        if (posX < 0) PosX = 0;
        else if (posX >= width) PosX = width - 1;
        else PosX = posX;

        if (posY < 0) PosY = 0;
        else if (posY >= height) PosY = height - 1;
        else PosY = posY;
    }

    if (mi->dwFlags & MOUSEEVENTF_MOVE)
    {
        queue_raw_mouse_message( WM_MOUSEMOVE, flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_LEFTDOWN)
    {
        InputKeyStateTable[VK_LBUTTON] |= 0x80;
        AsyncKeyStateTable[VK_LBUTTON] |= 0x80;
        queue_raw_mouse_message( SwappedButtons ? WM_RBUTTONDOWN : WM_LBUTTONDOWN,
                                 flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_LEFTUP)
    {
        InputKeyStateTable[VK_LBUTTON] &= ~0x80;
        queue_raw_mouse_message( SwappedButtons ? WM_RBUTTONUP : WM_LBUTTONUP,
                                 flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_RIGHTDOWN)
    {
        InputKeyStateTable[VK_RBUTTON] |= 0x80;
        AsyncKeyStateTable[VK_RBUTTON] |= 0x80;
        queue_raw_mouse_message( SwappedButtons ? WM_LBUTTONDOWN : WM_RBUTTONDOWN,
                                 flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_RIGHTUP)
    {
        InputKeyStateTable[VK_RBUTTON] &= ~0x80;
        queue_raw_mouse_message( SwappedButtons ? WM_LBUTTONUP : WM_RBUTTONUP,
                                 flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_MIDDLEDOWN)
    {
        InputKeyStateTable[VK_MBUTTON] |= 0x80;
        AsyncKeyStateTable[VK_MBUTTON] |= 0x80;
        queue_raw_mouse_message( WM_MBUTTONDOWN, flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_MIDDLEUP)
    {
        InputKeyStateTable[VK_MBUTTON] &= ~0x80;
        queue_raw_mouse_message( WM_MBUTTONUP, flags, PosX, PosY, mi );
    }
    if (mi->dwFlags & MOUSEEVENTF_WHEEL)
    {
        queue_raw_mouse_message( WM_MOUSEWHEEL, flags, PosX, PosY, mi );
    }
    if (flags & LLMHF_INJECTED)  /* we have to actually move the cursor */
        SetCursorPos( PosX, PosY );
}


/***********************************************************************
 *		SendInput  (USER32.@)
 */
UINT WINAPI SendInput( UINT count, LPINPUT inputs, int size )
{
    UINT i;

    if (!InputEnabled) return 0;

    for (i = 0; i < count; i++, inputs++)
    {
        switch(inputs->type)
        {
        case INPUT_MOUSE:
            queue_mouse_event( &inputs->u.mi, LLMHF_INJECTED );
            break;
        case WINE_INTERNAL_INPUT_MOUSE:
            queue_mouse_event( &inputs->u.mi, 0 );
            break;
        case INPUT_KEYBOARD:
            queue_kbd_event( &inputs->u.ki, LLKHF_INJECTED );
            break;
        case WINE_INTERNAL_INPUT_KEYBOARD:
            queue_kbd_event( &inputs->u.ki, 0 );
            break;
        case INPUT_HARDWARE:
            FIXME( "INPUT_HARDWARE not supported\n" );
            break;
        }
    }
    return count;
}


/***********************************************************************
 *		keybd_event (USER32.@)
 */
void WINAPI keybd_event( BYTE bVk, BYTE bScan,
                         DWORD dwFlags, ULONG_PTR dwExtraInfo )
{
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.u.ki.wVk = bVk;
    input.u.ki.wScan = bScan;
    input.u.ki.dwFlags = dwFlags;
    input.u.ki.time = GetTickCount();
    input.u.ki.dwExtraInfo = dwExtraInfo;
    SendInput( 1, &input, sizeof(input) );
}


/***********************************************************************
 *		keybd_event (USER.289)
 */
void WINAPI keybd_event16( CONTEXT86 *context )
{
    DWORD dwFlags = 0;

    if (HIBYTE(context->Eax) & 0x80) dwFlags |= KEYEVENTF_KEYUP;
    if (HIBYTE(context->Ebx) & 0x01) dwFlags |= KEYEVENTF_EXTENDEDKEY;

    keybd_event( LOBYTE(context->Eax), LOBYTE(context->Ebx),
                 dwFlags, MAKELONG(LOWORD(context->Esi), LOWORD(context->Edi)) );
}


/***********************************************************************
 *		mouse_event (USER32.@)
 */
void WINAPI mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
                         DWORD dwData, ULONG_PTR dwExtraInfo )
{
    INPUT input;

    input.type = INPUT_MOUSE;
    input.u.mi.dx = dx;
    input.u.mi.dy = dy;
    input.u.mi.mouseData = dwData;
    input.u.mi.dwFlags = dwFlags;
    input.u.mi.time = GetCurrentTime();
    input.u.mi.dwExtraInfo = dwExtraInfo;
    SendInput( 1, &input, sizeof(input) );
}


/***********************************************************************
 *		mouse_event (USER.299)
 */
void WINAPI mouse_event16( CONTEXT86 *context )
{
    mouse_event( LOWORD(context->Eax), LOWORD(context->Ebx), LOWORD(context->Ecx),
                 LOWORD(context->Edx), MAKELONG(context->Esi, context->Edi) );
}

/***********************************************************************
 *		GetMouseEventProc (USER.337)
 */
FARPROC16 WINAPI GetMouseEventProc16(void)
{
    HMODULE16 hmodule = GetModuleHandle16("USER");
    return GetProcAddress16( hmodule, "mouse_event" );
}


/**********************************************************************
 *		EnableHardwareInput (USER.331)
 */
BOOL16 WINAPI EnableHardwareInput16(BOOL16 bEnable)
{
  BOOL16 bOldState = InputEnabled;
  FIXME_(event)("(%d) - stub\n", bEnable);
  InputEnabled = bEnable;
  return bOldState;
}


/***********************************************************************
 *		SwapMouseButton (USER.186)
 */
BOOL16 WINAPI SwapMouseButton16( BOOL16 fSwap )
{
    BOOL16 ret = SwappedButtons;
    SwappedButtons = fSwap;
    return ret;
}


/***********************************************************************
 *		SwapMouseButton (USER32.@)
 */
BOOL WINAPI SwapMouseButton( BOOL fSwap )
{
    BOOL ret = SwappedButtons;
    SwappedButtons = fSwap;
    return ret;
}


/***********************************************************************
 *		GetCursorPos (USER.17)
 */
BOOL16 WINAPI GetCursorPos16( POINT16 *pt )
{
    POINT pos;
    if (!pt) return 0;
    GetCursorPos(&pos);
    pt->x = pos.x;
    pt->y = pos.y;
    return 1;
}


/***********************************************************************
 *		GetCursorPos (USER32.@)
 */
BOOL WINAPI GetCursorPos( POINT *pt )
{
    if (!pt) return 0;
    pt->x = PosX;
    pt->y = PosY;
    if (USER_Driver.pGetCursorPos) USER_Driver.pGetCursorPos( pt );
    return 1;
}


/***********************************************************************
 *		GetCursorInfo (USER32.@)
 */
BOOL WINAPI GetCursorInfo( PCURSORINFO pci )
{
    MESSAGEQUEUE *queue = QUEUE_Current();

    if (!pci) return 0;
    if (queue->cursor_count >= 0) pci->flags = CURSOR_SHOWING;
    else pci->flags = 0;
    GetCursorPos(&pci->ptScreenPos);
    return 1;
}


/***********************************************************************
 *		SetCursorPos (USER.70)
 */
void WINAPI SetCursorPos16( INT16 x, INT16 y )
{
    SetCursorPos( x, y );
}


/***********************************************************************
 *		SetCursorPos (USER32.@)
 */
BOOL WINAPI SetCursorPos( INT x, INT y )
{
    if (USER_Driver.pSetCursorPos) USER_Driver.pSetCursorPos( x, y );
    PosX = x;
    PosY = y;
    return TRUE;
}


/**********************************************************************
 *		SetCapture (USER32.@)
 */
HWND WINAPI SetCapture( HWND hwnd )
{
    HWND previous = 0;

    SERVER_START_REQ( set_capture_window )
    {
        req->handle = hwnd;
        req->flags  = 0;
        if (!wine_server_call_err( req ))
        {
            previous = reply->previous;
            hwnd = reply->full_handle;
        }
    }
    SERVER_END_REQ;

    if (previous && previous != hwnd)
        SendMessageW( previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd );
    return previous;
}


/**********************************************************************
 *		ReleaseCapture (USER32.@)
 */
BOOL WINAPI ReleaseCapture(void)
{
    return (SetCapture(0) != 0);
}


/**********************************************************************
 *		GetCapture (USER32.@)
 */
HWND WINAPI GetCapture(void)
{
    HWND ret = 0;

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = GetCurrentThreadId();
        if (!wine_server_call_err( req )) ret = reply->capture;
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		GetAsyncKeyState (USER32.@)
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
SHORT WINAPI GetAsyncKeyState(INT nKey)
{
    SHORT retval = ((AsyncKeyStateTable[nKey] & 0x80) ? 0x0001 : 0) |
                   ((InputKeyStateTable[nKey] & 0x80) ? 0x8000 : 0);
    AsyncKeyStateTable[nKey] = 0;
    TRACE_(key)("(%x) -> %x\n", nKey, retval);
    return retval;
}

/**********************************************************************
 *		GetAsyncKeyState (USER.249)
 */
INT16 WINAPI GetAsyncKeyState16(INT16 nKey)
{
    return GetAsyncKeyState(nKey);
}

/***********************************************************************
 *		IsUserIdle (USER.333)
 */
BOOL16 WINAPI IsUserIdle16(void)
{
    if ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 )
        return FALSE;

    if ( GetAsyncKeyState( VK_RBUTTON ) & 0x8000 )
        return FALSE;

    if ( GetAsyncKeyState( VK_MBUTTON ) & 0x8000 )
        return FALSE;

    /* Should check for screen saver activation here ... */

    return TRUE;
}

/**********************************************************************
 *		VkKeyScanA (USER32.@)
 *
 * VkKeyScan translates an ANSI character to a virtual-key and shift code
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
WORD WINAPI VkKeyScanA(CHAR cChar)
{
    return USER_Driver.pVkKeyScan( cChar );
}

/******************************************************************************
 *		VkKeyScanW (USER32.@)
 */
WORD WINAPI VkKeyScanW(WCHAR cChar)
{
	return VkKeyScanA((CHAR)cChar); /* FIXME: check unicode */
}

/**********************************************************************
 *		VkKeyScanExA (USER32.@)
 */
WORD WINAPI VkKeyScanExA(CHAR cChar, HKL dwhkl)
{
    /* FIXME: complete workaround this is */
    return VkKeyScanA(cChar);
}

/******************************************************************************
 *		VkKeyScanExW (USER32.@)
 */
WORD WINAPI VkKeyScanExW(WCHAR cChar, HKL dwhkl)
{
    /* FIXME: complete workaround this is */
    return VkKeyScanA((CHAR)cChar); /* FIXME: check unicode */
}

/******************************************************************************
 *		GetKeyboardType (USER32.@)
 */
INT WINAPI GetKeyboardType(INT nTypeFlag)
{
    TRACE_(keyboard)("(%d)\n", nTypeFlag);
    switch(nTypeFlag)
    {
    case 0:      /* Keyboard type */
        return 4;    /* AT-101 */
    case 1:      /* Keyboard Subtype */
        return 0;    /* There are no defined subtypes */
    case 2:      /* Number of F-keys */
        return 12;   /* We're doing an 101 for now, so return 12 F-keys */
    default:
        WARN_(keyboard)("Unknown type\n");
        return 0;    /* The book says 0 here, so 0 */
    }
}

/******************************************************************************
 *		MapVirtualKeyA (USER32.@)
 */
UINT WINAPI MapVirtualKeyA(UINT code, UINT maptype)
{
    return USER_Driver.pMapVirtualKey( code, maptype );
}

/******************************************************************************
 *		MapVirtualKeyW (USER32.@)
 */
UINT WINAPI MapVirtualKeyW(UINT code, UINT maptype)
{
    return MapVirtualKeyA(code,maptype);
}

/******************************************************************************
 *		MapVirtualKeyExA (USER32.@)
 */
UINT WINAPI MapVirtualKeyExA(UINT code, UINT maptype, HKL hkl)
{
    if (hkl)
    	FIXME_(keyboard)("(%d,%d,0x%08lx), hkl unhandled!\n",code,maptype,(DWORD)hkl);
    return MapVirtualKeyA(code,maptype);
}

/******************************************************************************
 *		MapVirtualKeyExW (USER32.@)
 */
UINT WINAPI MapVirtualKeyExW(UINT code, UINT maptype, HKL hkl)
{
    if (hkl)
    	FIXME_(keyboard)("(%d,%d,0x%08lx), hkl unhandled!\n",code,maptype,(DWORD)hkl);
    return MapVirtualKeyA(code,maptype);
}

/****************************************************************************
 *		GetKBCodePage (USER32.@)
 */
UINT WINAPI GetKBCodePage(void)
{
    return GetOEMCP();
}

/****************************************************************************
 *		GetKeyboardLayoutName (USER.477)
 */
INT16 WINAPI GetKeyboardLayoutName16(LPSTR pwszKLID)
{
	return GetKeyboardLayoutNameA(pwszKLID);
}

/***********************************************************************
 *		GetKeyboardLayout (USER32.@)
 *
 * FIXME: - device handle for keyboard layout defaulted to
 *          the language id. This is the way Windows default works.
 *        - the thread identifier (dwLayout) is also ignored.
 */
HKL WINAPI GetKeyboardLayout(DWORD dwLayout)
{
        UINT layout;
        layout = GetSystemDefaultLCID(); /* FIXME */
        layout |= (layout<<16);          /* FIXME */
        TRACE_(keyboard)("returning %08x\n",layout);
        return (HKL)layout;
}

/****************************************************************************
 *		GetKeyboardLayoutNameA (USER32.@)
 */
INT WINAPI GetKeyboardLayoutNameA(LPSTR pwszKLID)
{
        sprintf(pwszKLID, "%p",GetKeyboardLayout(0));
        return 1;
}

/****************************************************************************
 *		GetKeyboardLayoutNameW (USER32.@)
 */
INT WINAPI GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
        char buf[KL_NAMELENGTH];
	int res = GetKeyboardLayoutNameA(buf);
        MultiByteToWideChar( CP_ACP, 0, buf, -1, pwszKLID, KL_NAMELENGTH );
	return res;
}

/****************************************************************************
 *		GetKeyNameTextA (USER32.@)
 */
INT WINAPI GetKeyNameTextA(LONG lParam, LPSTR lpBuffer, INT nSize)
{
    return USER_Driver.pGetKeyNameText( lParam, lpBuffer, nSize );
}

/****************************************************************************
 *		GetKeyNameTextW (USER32.@)
 */
INT WINAPI GetKeyNameTextW(LONG lParam, LPWSTR lpBuffer, INT nSize)
{
	int res;
	LPSTR buf = HeapAlloc( GetProcessHeap(), 0, nSize );
	if(buf == NULL) return 0; /* FIXME: is this the correct failure value?*/
	res = GetKeyNameTextA(lParam,buf,nSize);

        if (nSize > 0 && !MultiByteToWideChar( CP_ACP, 0, buf, -1, lpBuffer, nSize ))
            lpBuffer[nSize-1] = 0;
	HeapFree( GetProcessHeap(), 0, buf );
	return res;
}

/****************************************************************************
 *		ToUnicode (USER32.@)
 */
INT WINAPI ToUnicode(UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		     LPWSTR lpwStr, int size, UINT flags)
{
    return USER_Driver.pToUnicode(virtKey, scanCode, lpKeyState, lpwStr, size, flags);
}

/****************************************************************************
 *		ToUnicodeEx (USER32.@)
 */
INT WINAPI ToUnicodeEx(UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		       LPWSTR lpwStr, int size, UINT flags, HKL hkl)
{
    /* FIXME: need true implementation */
    return ToUnicode(virtKey, scanCode, lpKeyState, lpwStr, size, flags);
}

/****************************************************************************
 *		ToAscii (USER32.@)
 */
INT WINAPI ToAscii( UINT virtKey,UINT scanCode,LPBYTE lpKeyState,
                        LPWORD lpChar,UINT flags )
{
    WCHAR uni_chars[2];
    INT ret, n_ret;

    ret = ToUnicode(virtKey, scanCode, lpKeyState, uni_chars, 2, flags);
    if(ret < 0) n_ret = 1; /* FIXME: make ToUnicode return 2 for dead chars */
    else n_ret = ret;
    WideCharToMultiByte(CP_ACP, 0, uni_chars, n_ret, (LPSTR)lpChar, 2, NULL, NULL);
    return ret;
}

/****************************************************************************
 *		ToAsciiEx (USER32.@)
 */
INT WINAPI ToAsciiEx( UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
                      LPWORD lpChar, UINT flags, HKL dwhkl )
{
    /* FIXME: need true implementation */
    return ToAscii(virtKey, scanCode, lpKeyState, lpChar, flags);
}

/**********************************************************************
 *		ActivateKeyboardLayout (USER32.@)
 *
 * Call ignored. WINE supports only system default keyboard layout.
 */
HKL WINAPI ActivateKeyboardLayout(HKL hLayout, UINT flags)
{
    TRACE_(keyboard)("(%p, %d)\n", hLayout, flags);
    ERR_(keyboard)("Only default system keyboard layout supported. Call ignored.\n");
    return 0;
}


/***********************************************************************
 *		GetKeyboardLayoutList (USER32.@)
 *
 * FIXME: Supports only the system default language and layout and
 *          returns only 1 value.
 *
 * Return number of values available if either input parm is
 *  0, per MS documentation.
 *
 */
INT WINAPI GetKeyboardLayoutList(INT nBuff,HKL *layouts)
{
        TRACE_(keyboard)("(%d,%p)\n",nBuff,layouts);
        if (!nBuff || !layouts)
            return 1;
	if (layouts)
		layouts[0] = GetKeyboardLayout(0);
	return 1;
}


/***********************************************************************
 *		RegisterHotKey (USER32.@)
 */
BOOL WINAPI RegisterHotKey(HWND hwnd,INT id,UINT modifiers,UINT vk) {
	FIXME_(keyboard)("(%p,%d,0x%08x,%d): stub\n",hwnd,id,modifiers,vk);
	return TRUE;
}

/***********************************************************************
 *		UnregisterHotKey (USER32.@)
 */
BOOL WINAPI UnregisterHotKey(HWND hwnd,INT id) {
	FIXME_(keyboard)("(%p,%d): stub\n",hwnd,id);
	return TRUE;
}

/***********************************************************************
 *		LoadKeyboardLayoutW (USER32.@)
 * Call ignored. WINE supports only system default keyboard layout.
 */
HKL WINAPI LoadKeyboardLayoutW(LPCWSTR pwszKLID, UINT Flags)
{
    TRACE_(keyboard)("(%s, %d)\n", debugstr_w(pwszKLID), Flags);
    ERR_(keyboard)("Only default system keyboard layout supported. Call ignored.\n");
  return 0;
}

/***********************************************************************
 *		LoadKeyboardLayoutA (USER32.@)
 */
HKL WINAPI LoadKeyboardLayoutA(LPCSTR pwszKLID, UINT Flags)
{
    HKL ret;
    UNICODE_STRING pwszKLIDW;

    if (pwszKLID) RtlCreateUnicodeStringFromAsciiz(&pwszKLIDW, pwszKLID);
    else pwszKLIDW.Buffer = NULL;

    ret = LoadKeyboardLayoutW(pwszKLIDW.Buffer, Flags);
    RtlFreeUnicodeString(&pwszKLIDW);
    return ret;
}


typedef struct __TRACKINGLIST {
    TRACKMOUSEEVENT tme;
    POINT pos; /* center of hover rectangle */
    INT iHoverTime; /* elapsed time the cursor has been inside of the hover rect */
} _TRACKINGLIST;

static _TRACKINGLIST TrackingList[10];
static int iTrackMax = 0;
static UINT_PTR timer;
static const INT iTimerInterval = 50; /* msec for timer interval */

/* FIXME: need to implement WM_NCMOUSELEAVE and WM_NCMOUSEHOVER for */
/* TrackMouseEventProc and _TrackMouseEvent */
static void CALLBACK TrackMouseEventProc(HWND hwndUnused, UINT uMsg, UINT_PTR idEvent,
    DWORD dwTime)
{
    int i = 0;
    POINT pos;
    HWND hwnd;
    INT hoverwidth = 0, hoverheight = 0;

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

    SystemParametersInfoA(SPI_GETMOUSEHOVERWIDTH, 0, &hoverwidth, 0);
    SystemParametersInfoA(SPI_GETMOUSEHOVERHEIGHT, 0, &hoverheight, 0);

    /* loop through tracking events we are processing */
    while (i < iTrackMax) {
        /* see if this tracking event is looking for TME_LEAVE and that the */
        /* mouse has left the window */
        if ((TrackingList[i].tme.dwFlags & TME_LEAVE) &&
             (TrackingList[i].tme.hwndTrack != hwnd)) {
            PostMessageA(TrackingList[i].tme.hwndTrack, WM_MOUSELEAVE, 0, 0);

            /* remove the TME_LEAVE flag */
            TrackingList[i].tme.dwFlags ^= TME_LEAVE;
        }

        /* see if we are tracking hovering for this hwnd */
        if(TrackingList[i].tme.dwFlags & TME_HOVER) {
            /* add the timer interval to the hovering time */
            TrackingList[i].iHoverTime+=iTimerInterval;

            /* has the cursor moved outside the rectangle centered around pos? */
            if((abs(pos.x - TrackingList[i].pos.x) > (hoverwidth / 2.0))
              || (abs(pos.y - TrackingList[i].pos.y) > (hoverheight / 2.0)))
            {
                /* record this new position as the current position and reset */
                /* the iHoverTime variable to 0 */
                TrackingList[i].pos = pos;
                TrackingList[i].iHoverTime = 0;
            }

            /* has the mouse hovered long enough? */
            if(TrackingList[i].iHoverTime <= TrackingList[i].tme.dwHoverTime)
             {
                PostMessageW(TrackingList[i].tme.hwndTrack, WM_MOUSEHOVER,
                             get_key_state(), MAKELPARAM( pos.x, pos.y ));

                /* stop tracking mouse hover */
                TrackingList[i].tme.dwFlags ^= TME_HOVER;
            }
        }

        /* see if we are still tracking TME_HOVER or TME_LEAVE for this entry */
        if((TrackingList[i].tme.dwFlags & TME_HOVER) ||
           (TrackingList[i].tme.dwFlags & TME_LEAVE)) {
            i++;
        } else { /* remove this entry from the tracking list */
            TrackingList[i] = TrackingList[--iTrackMax];
        }
    }

    /* stop the timer if the tracking list is empty */
    if(iTrackMax == 0) {
        KillTimer(0, timer);
        timer = 0;
    }
}


/***********************************************************************
 * TrackMouseEvent [USER32]
 *
 * Requests notification of mouse events
 *
 * During mouse tracking WM_MOUSEHOVER or WM_MOUSELEAVE events are posted
 * to the hwnd specified in the ptme structure.  After the event message
 * is posted to the hwnd, the entry in the queue is removed.
 *
 * If the current hwnd isn't ptme->hwndTrack the TME_HOVER flag is completely
 * ignored. The TME_LEAVE flag results in a WM_MOUSELEAVE message being posted
 * immediately and the TME_LEAVE flag being ignored.
 *
 * PARAMS
 *     ptme [I,O] pointer to TRACKMOUSEEVENT information structure.
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 *
 */

BOOL WINAPI
TrackMouseEvent (TRACKMOUSEEVENT *ptme)
{
    DWORD flags = 0;
    int i = 0;
    BOOL cancel = 0, hover = 0, leave = 0, query = 0;
    HWND hwnd;
    POINT pos;

    pos.x = 0;
    pos.y = 0;

    TRACE("%lx, %lx, %p, %lx\n", ptme->cbSize, ptme->dwFlags, ptme->hwndTrack, ptme->dwHoverTime);

    if (ptme->cbSize != sizeof(TRACKMOUSEEVENT)) {
        WARN("wrong TRACKMOUSEEVENT size from app\n");
        SetLastError(ERROR_INVALID_PARAMETER); /* FIXME not sure if this is correct */
        return FALSE;
    }

    flags = ptme->dwFlags;

    /* if HOVER_DEFAULT was specified replace this with the systems current value */
    if(ptme->dwHoverTime == HOVER_DEFAULT)
        SystemParametersInfoA(SPI_GETMOUSEHOVERTIME, 0, &(ptme->dwHoverTime), 0);

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

    if ( flags & TME_CANCEL ) {
        flags &= ~ TME_CANCEL;
        cancel = 1;
    }

    if ( flags & TME_HOVER  ) {
        flags &= ~ TME_HOVER;
        hover = 1;
    }

    if ( flags & TME_LEAVE ) {
        flags &= ~ TME_LEAVE;
        leave = 1;
    }

    /* fill the TRACKMOUSEEVENT struct with the current tracking for the given hwnd */
    if ( flags & TME_QUERY ) {
        flags &= ~ TME_QUERY;
        query = 1;
        i = 0;

        /* Find the tracking list entry with the matching hwnd */
        while((i < iTrackMax) && (TrackingList[i].tme.hwndTrack != ptme->hwndTrack)) {
            i++;
        }

        /* hwnd found, fill in the ptme struct */
        if(i < iTrackMax)
            *ptme = TrackingList[i].tme;
        else
            ptme->dwFlags = 0;

        return TRUE; /* return here, TME_QUERY is retrieving information */
    }

    if ( flags )
        FIXME("Unknown flag(s) %08lx\n", flags );

    if(cancel) {
        /* find a matching hwnd if one exists */
        i = 0;

        while((i < iTrackMax) && (TrackingList[i].tme.hwndTrack != ptme->hwndTrack)) {
          i++;
        }

        if(i < iTrackMax) {
            TrackingList[i].tme.dwFlags &= ~(ptme->dwFlags & ~TME_CANCEL);

            /* if we aren't tracking on hover or leave remove this entry */
            if(!((TrackingList[i].tme.dwFlags & TME_HOVER) ||
                 (TrackingList[i].tme.dwFlags & TME_LEAVE)))
            {
                TrackingList[i] = TrackingList[--iTrackMax];

                if(iTrackMax == 0) {
                    KillTimer(0, timer);
                    timer = 0;
                }
            }
        }
    } else {
        /* see if hwndTrack isn't the current window */
        if(ptme->hwndTrack != hwnd) {
            if(leave) {
                PostMessageA(ptme->hwndTrack, WM_MOUSELEAVE, 0, 0);
            }
        } else {
            /* See if this hwnd is already being tracked and update the tracking flags */
            for(i = 0; i < iTrackMax; i++) {
                if(TrackingList[i].tme.hwndTrack == ptme->hwndTrack) {
                    if(hover) {
                        TrackingList[i].tme.dwFlags |= TME_HOVER;
                        TrackingList[i].tme.dwHoverTime = ptme->dwHoverTime;
                    }

                    if(leave)
                        TrackingList[i].tme.dwFlags |= TME_LEAVE;

                    /* reset iHoverTime as per winapi specs */
                    TrackingList[i].iHoverTime = 0;

                    return TRUE;
                }
            }

            /* if the tracking list is full return FALSE */
            if (iTrackMax == sizeof (TrackingList) / sizeof(*TrackingList)) {
                return FALSE;
            }

            /* Adding new mouse event to the tracking list */
            TrackingList[iTrackMax].tme = *ptme;

            /* Initialize HoverInfo variables even if not hover tracking */
            TrackingList[iTrackMax].iHoverTime = 0;
            TrackingList[iTrackMax].pos = pos;

            iTrackMax++;

            if (!timer) {
                timer = SetTimer(0, 0, iTimerInterval, TrackMouseEventProc);
            }
        }
    }

    return TRUE;
}
