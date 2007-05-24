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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "win.h"
#include "user_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);


/***********************************************************************
 *           get_key_state
 */
static WORD get_key_state(void)
{
    WORD ret = 0;

    if (GetSystemMetrics( SM_SWAPBUTTON ))
    {
        if (GetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (GetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (GetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (GetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    if (GetAsyncKeyState(VK_MBUTTON) & 0x80)  ret |= MK_MBUTTON;
    if (GetAsyncKeyState(VK_SHIFT) & 0x80)    ret |= MK_SHIFT;
    if (GetAsyncKeyState(VK_CONTROL) & 0x80)  ret |= MK_CONTROL;
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x80) ret |= MK_XBUTTON1;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x80) ret |= MK_XBUTTON2;
    return ret;
}


/***********************************************************************
 *		SendInput  (USER32.@)
 */
UINT WINAPI SendInput( UINT count, LPINPUT inputs, int size )
{
    if (TRACE_ON(win))
    {
        UINT i;

        for (i = 0; i < count; i++)
        {
            switch(inputs[i].type)
            {
            case INPUT_MOUSE:
                TRACE("mouse: dx %d, dy %d, data %x, flags %x, time %u, info %lx\n",
                      inputs[i].u.mi.dx, inputs[i].u.mi.dy, inputs[i].u.mi.mouseData,
                      inputs[i].u.mi.dwFlags, inputs[i].u.mi.time, inputs[i].u.mi.dwExtraInfo);
                break;

            case INPUT_KEYBOARD:
                TRACE("keyboard: vk %x, scan %x, flags %x, time %u, info %lx\n",
                      inputs[i].u.ki.wVk, inputs[i].u.ki.wScan, inputs[i].u.ki.dwFlags,
                      inputs[i].u.ki.time, inputs[i].u.ki.dwExtraInfo);
                break;

            case INPUT_HARDWARE:
                TRACE("hardware: msg %d, wParamL %x, wParamH %x\n",
                      inputs[i].u.hi.uMsg, inputs[i].u.hi.wParamL, inputs[i].u.hi.wParamH);
                break;

            default:
                FIXME("unknown input type %u\n", inputs[i].type);
                break;
            }
        }
    }

    return USER_Driver->pSendInput( count, inputs, size );
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
 *		GetCursorPos (USER32.@)
 */
BOOL WINAPI GetCursorPos( POINT *pt )
{
    if (!pt) return FALSE;
    return USER_Driver->pGetCursorPos( pt );
}


/***********************************************************************
 *		GetCursorInfo (USER32.@)
 */
BOOL WINAPI GetCursorInfo( PCURSORINFO pci )
{
    if (!pci) return 0;
    if (get_user_thread_info()->cursor_count >= 0) pci->flags = CURSOR_SHOWING;
    else pci->flags = 0;
    GetCursorPos(&pci->ptScreenPos);
    return 1;
}


/***********************************************************************
 *		SetCursorPos (USER32.@)
 */
BOOL WINAPI SetCursorPos( INT x, INT y )
{
    return USER_Driver->pSetCursorPos( x, y );
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
    BOOL ret;
    HWND previous = 0;

    SERVER_START_REQ( set_capture_window )
    {
        req->handle = 0;
        req->flags  = 0;
        if ((ret = !wine_server_call_err( req ))) previous = reply->previous;
    }
    SERVER_END_REQ;

    if (previous) SendMessageW( previous, WM_CAPTURECHANGED, 0, 0 );

    /* Somebody may have missed some mouse movements */
    mouse_event( MOUSEEVENTF_MOVE, 0, 0, 0, 0 );

    return ret;
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
 */
SHORT WINAPI GetAsyncKeyState(INT nKey)
{
    return USER_Driver->pGetAsyncKeyState( nKey );
}


/***********************************************************************
 *		GetQueueStatus (USER32.@)
 */
DWORD WINAPI GetQueueStatus( UINT flags )
{
    DWORD ret = 0;

    if (flags & ~(QS_ALLINPUT | QS_ALLPOSTMESSAGE | QS_SMRESULT))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    /* check for pending X events */
    USER_Driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, flags, 0 );

    SERVER_START_REQ( get_queue_status )
    {
        req->clear = 1;
        wine_server_call( req );
        ret = MAKELONG( reply->changed_bits & flags, reply->wake_bits & flags );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		GetInputState   (USER32.@)
 */
BOOL WINAPI GetInputState(void)
{
    DWORD ret = 0;

    /* check for pending X events */
    USER_Driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, QS_INPUT, 0 );

    SERVER_START_REQ( get_queue_status )
    {
        req->clear = 0;
        wine_server_call( req );
        ret = reply->wake_bits & (QS_KEY | QS_MOUSEBUTTON);
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************
 *              GetLastInputInfo (USER32.@)
 */
BOOL WINAPI GetLastInputInfo(PLASTINPUTINFO plii)
{
    BOOL ret;

    TRACE("%p\n", plii);

    if (plii->cbSize != sizeof (*plii) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ( get_last_input_time )
    {
        ret = !wine_server_call_err( req );
        if (ret)
            plii->dwTime = reply->time;
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		AttachThreadInput (USER32.@)
 *
 * Attaches the input processing mechanism of one thread to that of
 * another thread.
 */
BOOL WINAPI AttachThreadInput( DWORD from, DWORD to, BOOL attach )
{
    BOOL ret;

    SERVER_START_REQ( attach_thread_input )
    {
        req->tid_from = from;
        req->tid_to   = to;
        req->attach   = attach;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		GetKeyState (USER32.@)
 *
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.
 */
SHORT WINAPI GetKeyState(INT vkey)
{
    SHORT retval = 0;

    SERVER_START_REQ( get_key_state )
    {
        req->tid = GetCurrentThreadId();
        req->key = vkey;
        if (!wine_server_call( req )) retval = (signed char)reply->state;
    }
    SERVER_END_REQ;
    TRACE("key (0x%x) -> %x\n", vkey, retval);
    return retval;
}


/**********************************************************************
 *		GetKeyboardState (USER32.@)
 */
BOOL WINAPI GetKeyboardState( LPBYTE state )
{
    BOOL ret;

    TRACE("(%p)\n", state);

    memset( state, 0, 256 );
    SERVER_START_REQ( get_key_state )
    {
        req->tid = GetCurrentThreadId();
        req->key = -1;
        wine_server_set_reply( req, state, 256 );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		SetKeyboardState (USER32.@)
 */
BOOL WINAPI SetKeyboardState( LPBYTE state )
{
    BOOL ret;

    TRACE("(%p)\n", state);

    SERVER_START_REQ( set_key_state )
    {
        req->tid = GetCurrentThreadId();
        wine_server_add_data( req, state, 256 );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
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
SHORT WINAPI VkKeyScanA(CHAR cChar)
{
    WCHAR wChar;

    if (IsDBCSLeadByte(cChar)) return -1;

    MultiByteToWideChar(CP_ACP, 0, &cChar, 1, &wChar, 1);
    return VkKeyScanW(wChar);
}

/******************************************************************************
 *		VkKeyScanW (USER32.@)
 */
SHORT WINAPI VkKeyScanW(WCHAR cChar)
{
    return VkKeyScanExW(cChar, GetKeyboardLayout(0));
}

/**********************************************************************
 *		VkKeyScanExA (USER32.@)
 */
WORD WINAPI VkKeyScanExA(CHAR cChar, HKL dwhkl)
{
    WCHAR wChar;

    if (IsDBCSLeadByte(cChar)) return -1;

    MultiByteToWideChar(CP_ACP, 0, &cChar, 1, &wChar, 1);
    return VkKeyScanExW(wChar, dwhkl);
}

/******************************************************************************
 *		VkKeyScanExW (USER32.@)
 */
WORD WINAPI VkKeyScanExW(WCHAR cChar, HKL dwhkl)
{
    return USER_Driver->pVkKeyScanEx(cChar, dwhkl);
}

/**********************************************************************
 *		OemKeyScan (USER32.@)
 */
DWORD WINAPI OemKeyScan(WORD wOemChar)
{
    TRACE("(%d)\n", wOemChar);
    return wOemChar;
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
    return MapVirtualKeyExA( code, maptype, GetKeyboardLayout(0) );
}

/******************************************************************************
 *		MapVirtualKeyW (USER32.@)
 */
UINT WINAPI MapVirtualKeyW(UINT code, UINT maptype)
{
    return MapVirtualKeyExW(code, maptype, GetKeyboardLayout(0));
}

/******************************************************************************
 *		MapVirtualKeyExA (USER32.@)
 */
UINT WINAPI MapVirtualKeyExA(UINT code, UINT maptype, HKL hkl)
{
    return MapVirtualKeyExW(code, maptype, hkl);
}

/******************************************************************************
 *		MapVirtualKeyExW (USER32.@)
 */
UINT WINAPI MapVirtualKeyExW(UINT code, UINT maptype, HKL hkl)
{
    TRACE_(keyboard)("(%d, %d, %p)\n", code, maptype, hkl);

    return USER_Driver->pMapVirtualKeyEx(code, maptype, hkl);
}

/****************************************************************************
 *		GetKBCodePage (USER32.@)
 */
UINT WINAPI GetKBCodePage(void)
{
    return GetOEMCP();
}

/***********************************************************************
 *		GetKeyboardLayout (USER32.@)
 *
 *        - device handle for keyboard layout defaulted to
 *          the language id. This is the way Windows default works.
 *        - the thread identifier (dwLayout) is also ignored.
 */
HKL WINAPI GetKeyboardLayout(DWORD dwLayout)
{
    return USER_Driver->pGetKeyboardLayout(dwLayout);
}

/****************************************************************************
 *		GetKeyboardLayoutNameA (USER32.@)
 */
BOOL WINAPI GetKeyboardLayoutNameA(LPSTR pszKLID)
{
    WCHAR buf[KL_NAMELENGTH];

    if (GetKeyboardLayoutNameW(buf))
        return WideCharToMultiByte( CP_ACP, 0, buf, -1, pszKLID, KL_NAMELENGTH, NULL, NULL ) != 0;
    return FALSE;
}

/****************************************************************************
 *		GetKeyboardLayoutNameW (USER32.@)
 */
BOOL WINAPI GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
    return USER_Driver->pGetKeyboardLayoutName(pwszKLID);
}

/****************************************************************************
 *		GetKeyNameTextA (USER32.@)
 */
INT WINAPI GetKeyNameTextA(LONG lParam, LPSTR lpBuffer, INT nSize)
{
    WCHAR buf[256];
    INT ret;

    if (!GetKeyNameTextW(lParam, buf, 256))
    {
        lpBuffer[0] = 0;
        return 0;
    }
    ret = WideCharToMultiByte(CP_ACP, 0, buf, -1, lpBuffer, nSize, NULL, NULL);
    if (!ret && nSize)
    {
        ret = nSize - 1;
        lpBuffer[ret] = 0;
    }
    return ret;
}

/****************************************************************************
 *		GetKeyNameTextW (USER32.@)
 */
INT WINAPI GetKeyNameTextW(LONG lParam, LPWSTR lpBuffer, INT nSize)
{
    return USER_Driver->pGetKeyNameText( lParam, lpBuffer, nSize );
}

/****************************************************************************
 *		ToUnicode (USER32.@)
 */
INT WINAPI ToUnicode(UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		     LPWSTR lpwStr, int size, UINT flags)
{
    return ToUnicodeEx(virtKey, scanCode, lpKeyState, lpwStr, size, flags, GetKeyboardLayout(0));
}

/****************************************************************************
 *		ToUnicodeEx (USER32.@)
 */
INT WINAPI ToUnicodeEx(UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		       LPWSTR lpwStr, int size, UINT flags, HKL hkl)
{
    return USER_Driver->pToUnicodeEx(virtKey, scanCode, lpKeyState, lpwStr, size, flags, hkl);
}

/****************************************************************************
 *		ToAscii (USER32.@)
 */
INT WINAPI ToAscii( UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
                    LPWORD lpChar, UINT flags )
{
    return ToAsciiEx(virtKey, scanCode, lpKeyState, lpChar, flags, GetKeyboardLayout(0));
}

/****************************************************************************
 *		ToAsciiEx (USER32.@)
 */
INT WINAPI ToAsciiEx( UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
                      LPWORD lpChar, UINT flags, HKL dwhkl )
{
    WCHAR uni_chars[2];
    INT ret, n_ret;

    ret = ToUnicodeEx(virtKey, scanCode, lpKeyState, uni_chars, 2, flags, dwhkl);
    if (ret < 0) n_ret = 1; /* FIXME: make ToUnicode return 2 for dead chars */
    else n_ret = ret;
    WideCharToMultiByte(CP_ACP, 0, uni_chars, n_ret, (LPSTR)lpChar, 2, NULL, NULL);
    return ret;
}

/**********************************************************************
 *		ActivateKeyboardLayout (USER32.@)
 */
HKL WINAPI ActivateKeyboardLayout(HKL hLayout, UINT flags)
{
    TRACE_(keyboard)("(%p, %d)\n", hLayout, flags);

    return USER_Driver->pActivateKeyboardLayout(hLayout, flags);
}

/**********************************************************************
 *		BlockInput (USER32.@)
 */
BOOL WINAPI BlockInput(BOOL fBlockIt)
{
    FIXME_(keyboard)("(%d): stub\n", fBlockIt);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/***********************************************************************
 *		GetKeyboardLayoutList (USER32.@)
 *
 * Return number of values available if either input parm is
 *  0, per MS documentation.
 */
UINT WINAPI GetKeyboardLayoutList(INT nBuff, HKL *layouts)
{
    TRACE_(keyboard)("(%d,%p)\n",nBuff,layouts);

    return USER_Driver->pGetKeyboardLayoutList(nBuff, layouts);
}


/***********************************************************************
 *		RegisterHotKey (USER32.@)
 */
BOOL WINAPI RegisterHotKey(HWND hwnd,INT id,UINT modifiers,UINT vk)
{
    FIXME_(keyboard)("(%p,%d,0x%08x,%d): stub\n",hwnd,id,modifiers,vk);
    return TRUE;
}

/***********************************************************************
 *		UnregisterHotKey (USER32.@)
 */
BOOL WINAPI UnregisterHotKey(HWND hwnd,INT id)
{
    FIXME_(keyboard)("(%p,%d): stub\n",hwnd,id);
    return TRUE;
}

/***********************************************************************
 *		LoadKeyboardLayoutW (USER32.@)
 */
HKL WINAPI LoadKeyboardLayoutW(LPCWSTR pwszKLID, UINT Flags)
{
    TRACE_(keyboard)("(%s, %d)\n", debugstr_w(pwszKLID), Flags);

    return USER_Driver->pLoadKeyboardLayout(pwszKLID, Flags);
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


/***********************************************************************
 *		UnloadKeyboardLayout (USER32.@)
 */
BOOL WINAPI UnloadKeyboardLayout(HKL hkl)
{
    TRACE_(keyboard)("(%p)\n", hkl);

    return USER_Driver->pUnloadKeyboardLayout(hkl);
}

typedef struct __TRACKINGLIST {
    TRACKMOUSEEVENT tme;
    POINT pos; /* center of hover rectangle */
} _TRACKINGLIST;

/* FIXME: move tracking stuff into a per thread data */
static _TRACKINGLIST tracking_info;
static UINT_PTR timer;

static void check_mouse_leave(HWND hwnd, int hittest)
{
    if (tracking_info.tme.hwndTrack != hwnd)
    {
        if (tracking_info.tme.dwFlags & TME_NONCLIENT)
            PostMessageW(tracking_info.tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
        else
            PostMessageW(tracking_info.tme.hwndTrack, WM_MOUSELEAVE, 0, 0);

        /* remove the TME_LEAVE flag */
        tracking_info.tme.dwFlags &= ~TME_LEAVE;
    }
    else
    {
        if (hittest == HTCLIENT)
        {
            if (tracking_info.tme.dwFlags & TME_NONCLIENT)
            {
                PostMessageW(tracking_info.tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                /* remove the TME_LEAVE flag */
                tracking_info.tme.dwFlags &= ~TME_LEAVE;
            }
        }
        else
        {
            if (!(tracking_info.tme.dwFlags & TME_NONCLIENT))
            {
                PostMessageW(tracking_info.tme.hwndTrack, WM_MOUSELEAVE, 0, 0);
                /* remove the TME_LEAVE flag */
                tracking_info.tme.dwFlags &= ~TME_LEAVE;
            }
        }
    }
}

static void CALLBACK TrackMouseEventProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                                         DWORD dwTime)
{
    POINT pos;
    INT hoverwidth = 0, hoverheight = 0, hittest;

    TRACE("hwnd %p, msg %04x, id %04lx, time %u\n", hwnd, uMsg, idEvent, dwTime);

    GetCursorPos(&pos);
    hwnd = WINPOS_WindowFromPoint(hwnd, pos, &hittest);

    TRACE("point %s hwnd %p hittest %d\n", wine_dbgstr_point(&pos), hwnd, hittest);

    SystemParametersInfoW(SPI_GETMOUSEHOVERWIDTH, 0, &hoverwidth, 0);
    SystemParametersInfoW(SPI_GETMOUSEHOVERHEIGHT, 0, &hoverheight, 0);

    TRACE("tracked pos %s, current pos %s, hover width %d, hover height %d\n",
           wine_dbgstr_point(&tracking_info.pos), wine_dbgstr_point(&pos),
           hoverwidth, hoverheight);

    /* see if this tracking event is looking for TME_LEAVE and that the */
    /* mouse has left the window */
    if (tracking_info.tme.dwFlags & TME_LEAVE)
    {
        check_mouse_leave(hwnd, hittest);
    }

    if (tracking_info.tme.hwndTrack != hwnd)
    {
        /* mouse is gone, stop tracking mouse hover */
        tracking_info.tme.dwFlags &= ~TME_HOVER;
    }

    /* see if we are tracking hovering for this hwnd */
    if (tracking_info.tme.dwFlags & TME_HOVER)
    {
        /* has the cursor moved outside the rectangle centered around pos? */
        if ((abs(pos.x - tracking_info.pos.x) > (hoverwidth / 2)) ||
            (abs(pos.y - tracking_info.pos.y) > (hoverheight / 2)))
        {
            /* record this new position as the current position */
            tracking_info.pos = pos;
        }
        else
        {
            if (hittest == HTCLIENT)
            {
                ScreenToClient(hwnd, &pos);
                TRACE("client cursor pos %s\n", wine_dbgstr_point(&pos));

                PostMessageW(tracking_info.tme.hwndTrack, WM_MOUSEHOVER,
                             get_key_state(), MAKELPARAM( pos.x, pos.y ));
            }
            else
            {
                if (tracking_info.tme.dwFlags & TME_NONCLIENT)
                    PostMessageW(tracking_info.tme.hwndTrack, WM_NCMOUSEHOVER,
                                 hittest, MAKELPARAM( pos.x, pos.y ));
            }

            /* stop tracking mouse hover */
            tracking_info.tme.dwFlags &= ~TME_HOVER;
        }
    }

    /* stop the timer if the tracking list is empty */
    if (!(tracking_info.tme.dwFlags & (TME_HOVER | TME_LEAVE)))
    {
        KillSystemTimer(tracking_info.tme.hwndTrack, timer);
        timer = 0;
        tracking_info.tme.hwndTrack = 0;
        tracking_info.tme.dwFlags = 0;
        tracking_info.tme.dwHoverTime = 0;
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
    HWND hwnd;
    POINT pos;
    DWORD hover_time;
    INT hittest;

    TRACE("%x, %x, %p, %u\n", ptme->cbSize, ptme->dwFlags, ptme->hwndTrack, ptme->dwHoverTime);

    if (ptme->cbSize != sizeof(TRACKMOUSEEVENT)) {
        WARN("wrong TRACKMOUSEEVENT size from app\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* fill the TRACKMOUSEEVENT struct with the current tracking for the given hwnd */
    if (ptme->dwFlags & TME_QUERY )
    {
        *ptme = tracking_info.tme;
        /* set cbSize in the case it's not initialized yet */
        ptme->cbSize = sizeof(TRACKMOUSEEVENT);

        return TRUE; /* return here, TME_QUERY is retrieving information */
    }

    if (!IsWindow(ptme->hwndTrack))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }

    hover_time = ptme->dwHoverTime;

    /* if HOVER_DEFAULT was specified replace this with the systems current value.
     * TME_LEAVE doesn't need to specify hover time so use default */
    if (hover_time == HOVER_DEFAULT || hover_time == 0 || !(ptme->dwHoverTime&TME_HOVER))
        SystemParametersInfoW(SPI_GETMOUSEHOVERTIME, 0, &hover_time, 0);

    GetCursorPos(&pos);
    hwnd = WINPOS_WindowFromPoint(ptme->hwndTrack, pos, &hittest);
    TRACE("point %s hwnd %p hittest %d\n", wine_dbgstr_point(&pos), hwnd, hittest);

    if (ptme->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT))
        FIXME("Unknown flag(s) %08x\n", ptme->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT));

    if (ptme->dwFlags & TME_CANCEL)
    {
        if (tracking_info.tme.hwndTrack == ptme->hwndTrack)
        {
            tracking_info.tme.dwFlags &= ~(ptme->dwFlags & ~TME_CANCEL);

            /* if we aren't tracking on hover or leave remove this entry */
            if (!(tracking_info.tme.dwFlags & (TME_HOVER | TME_LEAVE)))
            {
                KillSystemTimer(tracking_info.tme.hwndTrack, timer);
                timer = 0;
                tracking_info.tme.hwndTrack = 0;
                tracking_info.tme.dwFlags = 0;
                tracking_info.tme.dwHoverTime = 0;
            }
        }
    } else {
        /* In our implementation it's possible that another window will receive a
         * WM_MOUSEMOVE and call TrackMouseEvent before TrackMouseEventProc is
         * called. In such a situation post the WM_MOUSELEAVE now */
        if (tracking_info.tme.dwFlags & TME_LEAVE && tracking_info.tme.hwndTrack != NULL)
            check_mouse_leave(hwnd, hittest);

        if (timer)
        {
            KillSystemTimer(tracking_info.tme.hwndTrack, timer);
            timer = 0;
            tracking_info.tme.hwndTrack = 0;
            tracking_info.tme.dwFlags = 0;
            tracking_info.tme.dwHoverTime = 0;
        }

        if (ptme->hwndTrack == hwnd)
        {
            /* Adding new mouse event to the tracking list */
            tracking_info.tme = *ptme;
            tracking_info.tme.dwHoverTime = hover_time;

            /* Initialize HoverInfo variables even if not hover tracking */
            tracking_info.pos = pos;

            timer = SetSystemTimer(tracking_info.tme.hwndTrack, (UINT_PTR)&tracking_info.tme, hover_time, TrackMouseEventProc);
        }
    }

    return TRUE;
}
