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
#include "wine/server.h"
#include "user_private.h"
#include "winternl.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(key);
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
        return 0;
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
    INT iHoverTime; /* elapsed time the cursor has been inside of the hover rect */
} _TRACKINGLIST;

static _TRACKINGLIST TrackingList[10];
static int iTrackMax = 0;
static UINT_PTR timer;
static const INT iTimerInterval = 50; /* msec for timer interval */

static void CALLBACK TrackMouseEventProc(HWND hwndUnused, UINT uMsg, UINT_PTR idEvent,
    DWORD dwTime)
{
    int i = 0;
    POINT pos;
    POINT posClient;
    HWND hwnd;
    INT nonclient;
    INT hoverwidth = 0, hoverheight = 0;
    RECT client;

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

    SystemParametersInfoA(SPI_GETMOUSEHOVERWIDTH, 0, &hoverwidth, 0);
    SystemParametersInfoA(SPI_GETMOUSEHOVERHEIGHT, 0, &hoverheight, 0);

    /* loop through tracking events we are processing */
    while (i < iTrackMax) {
        if (TrackingList[i].tme.dwFlags & TME_NONCLIENT) {
            nonclient = 1;
        }
        else {
            nonclient = 0;
        }

        /* see if this tracking event is looking for TME_LEAVE and that the */
        /* mouse has left the window */
        if (TrackingList[i].tme.dwFlags & TME_LEAVE) {
            if (TrackingList[i].tme.hwndTrack != hwnd) {
                if (nonclient) {
                    PostMessageA(TrackingList[i].tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                }
                else {
                    PostMessageA(TrackingList[i].tme.hwndTrack, WM_MOUSELEAVE, 0, 0);
                }

                /* remove the TME_LEAVE flag */
                TrackingList[i].tme.dwFlags ^= TME_LEAVE;
            }
            else {
                GetClientRect(hwnd, &client);
                MapWindowPoints(hwnd, NULL, (LPPOINT)&client, 2);
                if(PtInRect(&client, pos)) {
                    if (nonclient) {
                        PostMessageA(TrackingList[i].tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                        /* remove the TME_LEAVE flag */
                        TrackingList[i].tme.dwFlags ^= TME_LEAVE;
                    }
                }
                else {
                    if (!nonclient) {
                        PostMessageA(TrackingList[i].tme.hwndTrack, WM_MOUSELEAVE, 0, 0);
                        /* remove the TME_LEAVE flag */
                        TrackingList[i].tme.dwFlags ^= TME_LEAVE;
                    }
                }
            }
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
                posClient.x = pos.x;
                posClient.y = pos.y;
                ScreenToClient(hwnd, &posClient);
                if (nonclient) {
                    PostMessageW(TrackingList[i].tme.hwndTrack, WM_NCMOUSEHOVER,
                                get_key_state(), MAKELPARAM( posClient.x, posClient.y ));
                }
                else {
                    PostMessageW(TrackingList[i].tme.hwndTrack, WM_MOUSEHOVER,
                                get_key_state(), MAKELPARAM( posClient.x, posClient.y ));
                }

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
    BOOL cancel = 0, hover = 0, leave = 0, query = 0, nonclient = 0, inclient = 0;
    HWND hwnd;
    POINT pos;
    RECT client;


    pos.x = 0;
    pos.y = 0;
    SetRectEmpty(&client);

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

    if ( flags & TME_NONCLIENT ) {
        flags &= ~ TME_NONCLIENT;
        nonclient = 1;
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
                if(nonclient) {
                    PostMessageA(ptme->hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                }
                else {
                    PostMessageA(ptme->hwndTrack, WM_MOUSELEAVE, 0, 0);
                }
            }
        } else {
            GetClientRect(ptme->hwndTrack, &client);
            MapWindowPoints(ptme->hwndTrack, NULL, (LPPOINT)&client, 2);
            if(PtInRect(&client, pos)) {
                inclient = 1;
            }
            if(nonclient && inclient) {
                PostMessageA(ptme->hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                return TRUE;
            }
            else if(!nonclient && !inclient) {
                PostMessageA(ptme->hwndTrack, WM_MOUSELEAVE, 0, 0);
                return TRUE;
            }

            /* See if this hwnd is already being tracked and update the tracking flags */
            for(i = 0; i < iTrackMax; i++) {
                if(TrackingList[i].tme.hwndTrack == ptme->hwndTrack) {
                    TrackingList[i].tme.dwFlags = 0;

                    if(hover) {
                        TrackingList[i].tme.dwFlags |= TME_HOVER;
                        TrackingList[i].tme.dwHoverTime = ptme->dwHoverTime;
                    }

                    if(leave)
                        TrackingList[i].tme.dwFlags |= TME_LEAVE;

                    if(nonclient)
                        TrackingList[i].tme.dwFlags |= TME_NONCLIENT;

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
