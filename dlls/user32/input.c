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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winternl.h"
#include "winerror.h"
#include "win.h"
#include "user_private.h"
#include "dbt.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);

INT global_key_state_counter = 0;

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
 *           get_locale_kbd_layout
 */
static HKL get_locale_kbd_layout(void)
{
    ULONG_PTR layout;
    LANGID langid;

    /* FIXME:
     *
     * layout = main_key_tab[kbd_layout].lcid;
     *
     * Winword uses return value of GetKeyboardLayout as a codepage
     * to translate ANSI keyboard messages to unicode. But we have
     * a problem with it: for instance Polish keyboard layout is
     * identical to the US one, and therefore instead of the Polish
     * locale id we return the US one.
     */

    layout = GetUserDefaultLCID();

    /*
     * Microsoft Office expects this value to be something specific
     * for Japanese and Korean Windows with an IME the value is 0xe001
     * We should probably check to see if an IME exists and if so then
     * set this word properly.
     */
    langid = PRIMARYLANGID( LANGIDFROMLCID( layout ) );
    if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
        layout = MAKELONG( layout, 0xe001 ); /* IME */
    else
        layout = MAKELONG( layout, layout );

    return (HKL)layout;
}


/**********************************************************************
 *		keyboard_init
 */
void keyboard_init(void)
{
    WCHAR layout[KL_NAMELENGTH];
    HKEY hkey;

    if (RegCreateKeyExW( HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, NULL, 0,
                         KEY_ALL_ACCESS, NULL, &hkey, NULL ))
        return;

    if (GetKeyboardLayoutNameW( layout ))
        RegSetValueExW( hkey, L"1", 0, REG_SZ, (const BYTE *)layout, sizeof(layout) );

    RegCloseKey( hkey );
}


/**********************************************************************
 *		set_capture_window
 */
BOOL set_capture_window( HWND hwnd, UINT gui_flags, HWND *prev_ret )
{
    HWND previous = 0;
    UINT flags = 0;
    BOOL ret;

    if (gui_flags & GUI_INMENUMODE) flags |= CAPTURE_MENU;
    if (gui_flags & GUI_INMOVESIZE) flags |= CAPTURE_MOVESIZE;

    SERVER_START_REQ( set_capture_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->flags  = flags;
        if ((ret = !wine_server_call_err( req )))
        {
            previous = wine_server_ptr_handle( reply->previous );
            hwnd = wine_server_ptr_handle( reply->full_handle );
        }
    }
    SERVER_END_REQ;

    if (ret)
    {
        USER_Driver->pSetCapture( hwnd, gui_flags );

        if (previous)
            SendMessageW( previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd );

        if (prev_ret) *prev_ret = previous;
    }
    return ret;
}


/***********************************************************************
 *		__wine_send_input  (USER32.@)
 *
 * Internal SendInput function to allow the graphics driver to inject real events.
 */
BOOL CDECL __wine_send_input( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput )
{
    NTSTATUS status = send_hardware_message( hwnd, input, rawinput, 0 );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *		update_mouse_coords
 *
 * Helper for SendInput.
 */
static void update_mouse_coords( INPUT *input )
{
    if (!(input->u.mi.dwFlags & MOUSEEVENTF_MOVE)) return;

    if (input->u.mi.dwFlags & MOUSEEVENTF_ABSOLUTE)
    {
        DPI_AWARENESS_CONTEXT context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        RECT rc;

        if (input->u.mi.dwFlags & MOUSEEVENTF_VIRTUALDESK)
            rc = get_virtual_screen_rect();
        else
            rc = get_primary_monitor_rect();

        input->u.mi.dx = rc.left + ((input->u.mi.dx * (rc.right - rc.left)) >> 16);
        input->u.mi.dy = rc.top  + ((input->u.mi.dy * (rc.bottom - rc.top)) >> 16);
        SetThreadDpiAwarenessContext( context );
    }
    else
    {
        int accel[3];

        /* dx and dy can be negative numbers for relative movements */
        SystemParametersInfoW(SPI_GETMOUSE, 0, accel, 0);

        if (!accel[2]) return;

        if (abs(input->u.mi.dx) > accel[0])
        {
            input->u.mi.dx *= 2;
            if ((abs(input->u.mi.dx) > accel[1]) && (accel[2] == 2)) input->u.mi.dx *= 2;
        }
        if (abs(input->u.mi.dy) > accel[0])
        {
            input->u.mi.dy *= 2;
            if ((abs(input->u.mi.dy) > accel[1]) && (accel[2] == 2)) input->u.mi.dy *= 2;
        }
    }
}

/***********************************************************************
 *		SendInput  (USER32.@)
 */
UINT WINAPI SendInput( UINT count, LPINPUT inputs, int size )
{
    UINT i;
    NTSTATUS status = STATUS_SUCCESS;

    if (size != sizeof(INPUT))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!count)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!inputs)
    {
        SetLastError( ERROR_NOACCESS );
        return 0;
    }

    for (i = 0; i < count; i++)
    {
        INPUT input = inputs[i];
        switch (input.type)
        {
        case INPUT_MOUSE:
            /* we need to update the coordinates to what the server expects */
            update_mouse_coords( &input );
            /* fallthrough */
        case INPUT_KEYBOARD:
            status = send_hardware_message( 0, &input, NULL, SEND_HWMSG_INJECTED );
            break;
        case INPUT_HARDWARE:
            SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
            return 0;
        }

        if (status)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            break;
        }
    }

    return i;
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
    input.u.ki.time = 0;
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
    input.u.mi.time = 0;
    input.u.mi.dwExtraInfo = dwExtraInfo;
    SendInput( 1, &input, sizeof(input) );
}


/***********************************************************************
 *		GetCursorPos (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCursorPos( POINT *pt )
{
    BOOL ret;
    DWORD last_change;
    UINT dpi;

    if (!pt) return FALSE;

    SERVER_START_REQ( set_cursor )
    {
        if ((ret = !wine_server_call( req )))
        {
            pt->x = reply->new_x;
            pt->y = reply->new_y;
            last_change = reply->last_change;
        }
    }
    SERVER_END_REQ;

    /* query new position from graphics driver if we haven't updated recently */
    if (ret && GetTickCount() - last_change > 100) ret = USER_Driver->pGetCursorPos( pt );
    if (ret && (dpi = get_thread_dpi()))
    {
        DPI_AWARENESS_CONTEXT context;
        context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        *pt = map_dpi_point( *pt, get_monitor_dpi( MonitorFromPoint( *pt, MONITOR_DEFAULTTOPRIMARY )), dpi );
        SetThreadDpiAwarenessContext( context );
    }
    return ret;
}


/***********************************************************************
 *		GetCursorInfo (USER32.@)
 */
BOOL WINAPI GetCursorInfo( PCURSORINFO pci )
{
    BOOL ret;

    if (!pci) return FALSE;

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = 0;
        if ((ret = !wine_server_call( req )))
        {
            pci->hCursor = wine_server_ptr_handle( reply->cursor );
            pci->flags = (reply->show_count >= 0) ? CURSOR_SHOWING : 0;
        }
    }
    SERVER_END_REQ;
    GetCursorPos(&pci->ptScreenPos);
    return ret;
}


/***********************************************************************
 *		SetCursorPos (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCursorPos( INT x, INT y )
{
    POINT pt = { x, y };
    BOOL ret;
    INT prev_x, prev_y, new_x, new_y;
    UINT dpi;

    if ((dpi = get_thread_dpi()))
        pt = map_dpi_point( pt, dpi, get_monitor_dpi( MonitorFromPoint( pt, MONITOR_DEFAULTTOPRIMARY )));

    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_POS;
        req->x     = pt.x;
        req->y     = pt.y;
        if ((ret = !wine_server_call( req )))
        {
            prev_x = reply->prev_x;
            prev_y = reply->prev_y;
            new_x  = reply->new_x;
            new_y  = reply->new_y;
        }
    }
    SERVER_END_REQ;
    if (ret && (prev_x != new_x || prev_y != new_y)) USER_Driver->pSetCursorPos( new_x, new_y );
    return ret;
}

/**********************************************************************
 *		SetCapture (USER32.@)
 */
HWND WINAPI DECLSPEC_HOTPATCH SetCapture( HWND hwnd )
{
    HWND previous = 0;

    set_capture_window( hwnd, 0, &previous );
    return previous;
}


/**********************************************************************
 *		ReleaseCapture (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReleaseCapture(void)
{
    BOOL ret = set_capture_window( 0, 0, NULL );

    /* Somebody may have missed some mouse movements */
    if (ret) mouse_event( MOUSEEVENTF_MOVE, 0, 0, 0, 0 );

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
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->capture );
    }
    SERVER_END_REQ;
    return ret;
}


static void check_for_events( UINT flags )
{
    if (USER_Driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, flags, 0 ) == WAIT_TIMEOUT)
        flush_window_surfaces( TRUE );
}

/**********************************************************************
 *		GetAsyncKeyState (USER32.@)
 *
 *	Determine if a key is or was pressed.  retval has high-order
 * bit set to 1 if currently pressed, low-order bit set to 1 if key has
 * been pressed.
 */
SHORT WINAPI DECLSPEC_HOTPATCH GetAsyncKeyState( INT key )
{
    struct user_key_state_info *key_state_info = get_user_thread_info()->key_state;
    INT counter = global_key_state_counter;
    BYTE prev_key_state;
    SHORT ret;

    if (key < 0 || key >= 256) return 0;

    check_for_events( QS_INPUT );

    if (key_state_info && !(key_state_info->state[key] & 0xc0) &&
        key_state_info->counter == counter && GetTickCount() - key_state_info->time < 50)
    {
        /* use cached value */
        return 0;
    }
    else if (!key_state_info)
    {
        key_state_info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*key_state_info) );
        get_user_thread_info()->key_state = key_state_info;
    }

    ret = 0;
    SERVER_START_REQ( get_key_state )
    {
        req->async = 1;
        req->key = key;
        if (key_state_info)
        {
            prev_key_state = key_state_info->state[key];
            wine_server_set_reply( req, key_state_info->state, sizeof(key_state_info->state) );
        }
        if (!wine_server_call( req ))
        {
            if (reply->state & 0x40) ret |= 0x0001;
            if (reply->state & 0x80) ret |= 0x8000;
            if (key_state_info)
            {
                /* force refreshing the key state cache - some multithreaded programs
                 * (like Adobe Photoshop CS5) expect that changes to the async key state
                 * are also immediately available in other threads. */
                if (prev_key_state != key_state_info->state[key])
                    counter = InterlockedIncrement( &global_key_state_counter );

                key_state_info->time    = GetTickCount();
                key_state_info->counter = counter;
            }
        }
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *		GetQueueStatus (USER32.@)
 */
DWORD WINAPI GetQueueStatus( UINT flags )
{
    DWORD ret;

    if (flags & ~(QS_ALLINPUT | QS_ALLPOSTMESSAGE | QS_SMRESULT))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    check_for_events( flags );

    SERVER_START_REQ( get_queue_status )
    {
        req->clear_bits = flags;
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
    DWORD ret;

    check_for_events( QS_INPUT );

    SERVER_START_REQ( get_queue_status )
    {
        req->clear_bits = 0;
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
SHORT WINAPI DECLSPEC_HOTPATCH GetKeyState(INT vkey)
{
    SHORT retval = 0;

    SERVER_START_REQ( get_key_state )
    {
        req->key = vkey;
        if (!wine_server_call( req )) retval = (signed char)(reply->state & 0x81);
    }
    SERVER_END_REQ;
    TRACE("key (0x%x) -> %x\n", vkey, retval);
    return retval;
}


/**********************************************************************
 *		GetKeyboardState (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetKeyboardState( LPBYTE state )
{
    BOOL ret;
    UINT i;

    TRACE("(%p)\n", state);

    memset( state, 0, 256 );
    SERVER_START_REQ( get_key_state )
    {
        req->key = -1;
        wine_server_set_reply( req, state, 256 );
        ret = !wine_server_call_err( req );
        for (i = 0; i < 256; i++) state[i] &= 0x81;
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

    SERVER_START_REQ( set_key_state )
    {
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
WORD WINAPI VkKeyScanExW( WCHAR chr, HKL layout )
{
    WORD shift = 0x100, ctrl = 0x200;
    SHORT ret;

    TRACE_(keyboard)( "chr %s, layout %p\n", debugstr_wn(&chr, 1), layout );

    if ((ret = USER_Driver->pVkKeyScanEx( chr, layout )) != -256) return ret;

    /* FIXME: English keyboard layout specific */

    if (chr == VK_CANCEL || chr == VK_BACK || chr == VK_TAB || chr == VK_RETURN ||
        chr == VK_ESCAPE || chr == VK_SPACE) ret = chr;
    else if (chr >= '0' && chr <= '9') ret = chr;
    else if (chr == ')') ret = shift + '0';
    else if (chr == '!') ret = shift + '1';
    else if (chr == '@') ret = shift + '2';
    else if (chr == '#') ret = shift + '3';
    else if (chr == '$') ret = shift + '4';
    else if (chr == '%') ret = shift + '5';
    else if (chr == '^') ret = shift + '6';
    else if (chr == '&') ret = shift + '7';
    else if (chr == '*') ret = shift + '8';
    else if (chr == '(') ret = shift + '9';
    else if (chr >= 'a' && chr <= 'z') ret = chr - 'a' + 'A';
    else if (chr >= 'A' && chr <= 'Z') ret = shift + chr;
    else if (chr == ';') ret = VK_OEM_1;
    else if (chr == '=') ret = VK_OEM_PLUS;
    else if (chr == ',') ret = VK_OEM_COMMA;
    else if (chr == '-') ret = VK_OEM_MINUS;
    else if (chr == '.') ret = VK_OEM_PERIOD;
    else if (chr == '/') ret = VK_OEM_2;
    else if (chr == '`') ret = VK_OEM_3;
    else if (chr == '[') ret = VK_OEM_4;
    else if (chr == '\\') ret = VK_OEM_5;
    else if (chr == ']') ret = VK_OEM_6;
    else if (chr == '\'') ret = VK_OEM_7;
    else if (chr == ':') ret = shift + VK_OEM_1;
    else if (chr == '+') ret = shift + VK_OEM_PLUS;
    else if (chr == '<') ret = shift + VK_OEM_COMMA;
    else if (chr == '_') ret = shift + VK_OEM_MINUS;
    else if (chr == '>') ret = shift + VK_OEM_PERIOD;
    else if (chr == '?') ret = shift + VK_OEM_2;
    else if (chr == '~') ret = shift + VK_OEM_3;
    else if (chr == '{') ret = shift + VK_OEM_4;
    else if (chr == '|') ret = shift + VK_OEM_5;
    else if (chr == '}') ret = shift + VK_OEM_6;
    else if (chr == '\"') ret = shift + VK_OEM_7;
    else if (chr == 0x7f) ret = ctrl + VK_BACK;
    else if (chr == '\n') ret = ctrl + VK_RETURN;
    else if (chr == 0xf000) ret = ctrl + '2';
    else if (chr == 0x0000) ret = ctrl + shift + '2';
    else if (chr >= 0x0001 && chr <= 0x001a) ret = ctrl + 'A' + chr - 1;
    else if (chr >= 0x001c && chr <= 0x001d) ret = ctrl + VK_OEM_3 + chr;
    else if (chr == 0x001e) ret = ctrl + shift + '6';
    else if (chr == 0x001f) ret = ctrl + shift + VK_OEM_MINUS;
    else ret = -1;

    TRACE_(keyboard)( "ret %04x\n", ret );
    return ret;
}

/**********************************************************************
 *		OemKeyScan (USER32.@)
 */
DWORD WINAPI OemKeyScan( WORD oem )
{
    WCHAR wchr;
    DWORD vkey, scan;
    char oem_char = LOBYTE( oem );

    if (!OemToCharBuffW( &oem_char, &wchr, 1 ))
        return -1;

    vkey = VkKeyScanW( wchr );
    scan = MapVirtualKeyW( LOBYTE( vkey ), MAPVK_VK_TO_VSC );
    if (!scan) return -1;

    vkey &= 0xff00;
    vkey <<= 8;
    return vkey | scan;
}

/******************************************************************************
 *		GetKeyboardType (USER32.@)
 */
INT WINAPI GetKeyboardType(INT nTypeFlag)
{
    TRACE_(keyboard)("(%d)\n", nTypeFlag);
    if (LOWORD(GetKeyboardLayout(0)) == MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN))
    {
        /* scan code for `_', the key left of r-shift, in Japanese 106 keyboard */
        const UINT JP106_VSC_USCORE = 0x73;

        switch(nTypeFlag)
        {
        case 0:      /* Keyboard type */
            return 7;    /* Japanese keyboard */
        case 1:      /* Keyboard Subtype */
            /* Test keyboard mappings to detect Japanese keyboard */
            if (MapVirtualKeyW(VK_OEM_102, MAPVK_VK_TO_VSC) == JP106_VSC_USCORE
                && MapVirtualKeyW(JP106_VSC_USCORE, MAPVK_VSC_TO_VK) == VK_OEM_102)
                return 2;    /* Japanese 106 */
            else
                return 0;    /* AT-101 */
        case 2:      /* Number of F-keys */
            return 12;   /* It has 12 F-keys */
        }
    }
    else
    {
        switch(nTypeFlag)
        {
        case 0:      /* Keyboard type */
            return 4;    /* AT-101 */
        case 1:      /* Keyboard Subtype */
            return 0;    /* There are no defined subtypes */
        case 2:      /* Number of F-keys */
            return 12;   /* We're doing an 101 for now, so return 12 F-keys */
        }
    }
    WARN_(keyboard)("Unknown type\n");
    return 0;    /* The book says 0 here, so 0 */
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
    UINT ret;

    ret = MapVirtualKeyExW( code, maptype, hkl );
    if (maptype == MAPVK_VK_TO_CHAR)
    {
        BYTE ch = 0;
        WCHAR wch = ret;

        WideCharToMultiByte( CP_ACP, 0, &wch, 1, (LPSTR)&ch, 1, NULL, NULL );
        ret = ch;
    }
    return ret;
}


/* English keyboard layout (0x0409) */
static const UINT kbd_en_vsc2vk[] =
{
    0x00, 0x1b, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0xbd, 0xbb, 0x08, 0x09,
    0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0xdb, 0xdd, 0x0d, 0xa2, 0x41, 0x53,
    0x44, 0x46, 0x47, 0x48, 0x4a, 0x4b, 0x4c, 0xba, 0xde, 0xc0, 0xa0, 0xdc, 0x5a, 0x58, 0x43, 0x56,
    0x42, 0x4e, 0x4d, 0xbc, 0xbe, 0xbf, 0xa1, 0x6a, 0xa4, 0x20, 0x14, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x90, 0x91, 0x24, 0x26, 0x21, 0x6d, 0x25, 0x0c, 0x27, 0x6b, 0x23,
    0x28, 0x22, 0x2d, 0x2e, 0x2c, 0x00, 0xe2, 0x7a, 0x7b, 0x0c, 0xee, 0xf1, 0xea, 0xf9, 0xf5, 0xf3,
    0x00, 0x00, 0xfb, 0x2f, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xed,
    0x00, 0xe9, 0x00, 0xc1, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x09, 0x00, 0xc2, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0xe000 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xb1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x0d, 0xa3, 0x00, 0x00,
    0xad, 0xb7, 0xb3, 0x00, 0xb2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xae, 0x00,
    0xaf, 0x00, 0xac, 0x00, 0x00, 0x6f, 0x00, 0x2c, 0xa5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x24, 0x26, 0x21, 0x00, 0x25, 0x00, 0x27, 0x00, 0x23,
    0x28, 0x22, 0x2d, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x5c, 0x5d, 0x00, 0x5f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xab, 0xa8, 0xa9, 0xa7, 0xa6, 0xb6, 0xb4, 0xb5, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0xe100 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const UINT kbd_en_vk2char[] =
{
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00,
     ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7', '8',  '9', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  'A',  'B',  'C',  'D',  'E',  'F',  'G', 'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
     'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W', 'X',  'Y',  'Z', 0x00, 0x00, 0x00, 0x00, 0x00,
     '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7', '8',  '9',  '*',  '+', 0x00,  '-',  '.',  '/',
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  ';',  '=',  ',',  '-',  '.',  '/',
     '`', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  '[', '\\',  ']', '\'', 0x00,
    0x00, 0x00, '\\', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const WCHAR *kbd_en_vscname[] =
{
    0, L"Esc", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Backspace", L"Tab",
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Enter", L"Ctrl", 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Shift", 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, L"Right Shift", L"Num *", L"Alt", L"Space", L"Caps Lock", L"F1", L"F2", L"F3", L"F4", L"F5",
    L"F6", L"F7", L"F8", L"F9", L"F10", L"Pause", L"Scroll Lock", L"Num 7", L"Num 8", L"Num 9", L"Num -", L"Num 4", L"Num 5", L"Num 6", L"Num +", L"Num 1",
    L"Num 2", L"Num 3", L"Num 0", L"Num Del", L"Sys Req", 0, 0, L"F11", L"F12", 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"F13", L"F14", L"F15", L"F16",
    L"F17", L"F18", L"F19", L"F20", L"F21", L"F22", L"F23", L"F24", 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* extended */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Num Enter", L"Right Ctrl", 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, L"Num /", 0, L"Prnt Scrn", L"Right Alt", 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, L"Num Lock", L"Break", L"Home", L"Up", L"Page Up", 0, L"Left", 0, L"Right", 0, L"End",
    L"Down", L"Page Down", L"Insert", L"Delete", L"<00>", 0, L"Help", 0, 0, 0, 0, L"Left Windows", L"Right Windows", L"Application", 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


/******************************************************************************
 *		MapVirtualKeyExW (USER32.@)
 */
UINT WINAPI MapVirtualKeyExW( UINT code, UINT type, HKL layout )
{
    const UINT *vsc2vk, *vk2char;
    UINT vsc2vk_size, vk2char_size;
    UINT ret;

    TRACE_(keyboard)( "code %u, type %u, layout %p.\n", code, type, layout );

    if ((ret = USER_Driver->pMapVirtualKeyEx( code, type, layout )) != -1) return ret;

    /* FIXME: English keyboard layout specific */

    vsc2vk = kbd_en_vsc2vk;
    vsc2vk_size = ARRAYSIZE(kbd_en_vsc2vk);
    vk2char = kbd_en_vk2char;
    vk2char_size = ARRAYSIZE(kbd_en_vk2char);

    switch (type)
    {
    case MAPVK_VK_TO_VSC_EX:
    case MAPVK_VK_TO_VSC:
        switch (code)
        {
        case VK_SHIFT:   code = VK_LSHIFT; break;
        case VK_CONTROL: code = VK_LCONTROL; break;
        case VK_MENU:    code = VK_LMENU; break;
        case VK_NUMPAD0: code = VK_INSERT; break;
        case VK_NUMPAD1: code = VK_END; break;
        case VK_NUMPAD2: code = VK_DOWN; break;
        case VK_NUMPAD3: code = VK_NEXT; break;
        case VK_NUMPAD4: code = VK_LEFT; break;
        case VK_NUMPAD5: code = VK_CLEAR; break;
        case VK_NUMPAD6: code = VK_RIGHT; break;
        case VK_NUMPAD7: code = VK_HOME; break;
        case VK_NUMPAD8: code = VK_UP; break;
        case VK_NUMPAD9: code = VK_PRIOR; break;
        case VK_DECIMAL: code = VK_DELETE; break;
        }

        for (ret = 0; ret < vsc2vk_size; ++ret) if (vsc2vk[ret] == code) break;
        if (ret >= vsc2vk_size) ret = 0;

        if (type == MAPVK_VK_TO_VSC)
        {
            if (ret >= 0x200) ret = 0;
            else ret &= 0xff;
        }
        else if (ret >= 0x100) ret += 0xdf00;
        break;
    case MAPVK_VSC_TO_VK:
    case MAPVK_VSC_TO_VK_EX:
        if (code & 0xe000) code -= 0xdf00;
        if (code >= vsc2vk_size) ret = 0;
        else ret = vsc2vk[code];

        if (type == MAPVK_VSC_TO_VK)
        {
            switch (ret)
            {
            case VK_LSHIFT:   case VK_RSHIFT:   ret = VK_SHIFT; break;
            case VK_LCONTROL: case VK_RCONTROL: ret = VK_CONTROL; break;
            case VK_LMENU:    case VK_RMENU:    ret = VK_MENU; break;
            }
        }
        break;
    case MAPVK_VK_TO_CHAR:
        if (code >= vk2char_size) ret = 0;
        else ret = vk2char[code];
        break;
    default:
        FIXME_(keyboard)( "unknown type %d\n", type );
        return 0;
    }

    TRACE_(keyboard)( "returning 0x%04x\n", ret );
    return ret;
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
 *        - the thread identifier is also ignored.
 */
HKL WINAPI GetKeyboardLayout(DWORD thread_id)
{
    struct user_thread_info *thread = get_user_thread_info();
    HKL layout = thread->kbd_layout;

    if (thread_id && thread_id != GetCurrentThreadId())
        FIXME( "couldn't return keyboard layout for thread %04x\n", thread_id );

    if (!layout) return get_locale_kbd_layout();
    return layout;
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
BOOL WINAPI GetKeyboardLayoutNameW( WCHAR *name )
{
    struct user_thread_info *info = get_user_thread_info();
    WCHAR klid[KL_NAMELENGTH], value[5];
    DWORD value_size, tmp, i = 0;
    HKEY hkey;
    HKL layout;

    TRACE_(keyboard)( "name %p\n", name );

    if (!name)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (info->kbd_layout_id)
    {
        swprintf( name, KL_NAMELENGTH, L"%08X", info->kbd_layout_id );
        return TRUE;
    }

    layout = GetKeyboardLayout( 0 );
    tmp = HandleToUlong( layout );
    if (HIWORD( tmp ) == LOWORD( tmp )) tmp = LOWORD( tmp );
    swprintf( name, KL_NAMELENGTH, L"%08X", tmp );

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Keyboard Layouts", &hkey ))
    {
        while (!RegEnumKeyW( hkey, i++, klid, ARRAY_SIZE(klid) ))
        {
            value_size = sizeof(value);
            if (!RegGetValueW( hkey, klid, L"Layout Id", RRF_RT_REG_SZ, NULL, (void *)&value, &value_size ))
                tmp = 0xf000 | (wcstoul( value, NULL, 16 ) & 0xfff);
            else
                tmp = wcstoul( klid, NULL, 16 );

            if (HIWORD( layout ) == tmp)
            {
                lstrcpynW( name, klid, KL_NAMELENGTH );
                break;
            }
        }
        RegCloseKey( hkey );
    }

    info->kbd_layout_id = wcstoul( name, NULL, 16 );

    TRACE_(keyboard)( "ret %s\n", debugstr_w( name ) );
    return TRUE;
}

/****************************************************************************
 *		GetKeyNameTextA (USER32.@)
 */
INT WINAPI GetKeyNameTextA(LONG lParam, LPSTR lpBuffer, INT nSize)
{
    WCHAR buf[256];
    INT ret;

    if (!nSize || !GetKeyNameTextW(lParam, buf, 256))
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
    else ret--;

    return ret;
}

/****************************************************************************
 *		GetKeyNameTextW (USER32.@)
 */
INT WINAPI GetKeyNameTextW( LONG lparam, LPWSTR buffer, INT size )
{
    INT code = ((lparam >> 16) & 0x1ff), vkey, len;
    UINT vsc2vk_size, vscname_size;
    const WCHAR *const *vscname;
    const UINT *vsc2vk;
    WCHAR tmp[2];

    TRACE_(keyboard)( "lparam %d, buffer %p, size %d.\n", lparam, buffer, size );

    if (!buffer || !size) return 0;
    if ((len = USER_Driver->pGetKeyNameText( lparam, buffer, size )) >= 0) return len;

    /* FIXME: English keyboard layout specific */

    vsc2vk = kbd_en_vsc2vk;
    vsc2vk_size = ARRAYSIZE(kbd_en_vsc2vk);
    vscname = kbd_en_vscname;
    vscname_size = ARRAYSIZE(kbd_en_vscname);

    if (lparam & 0x2000000)
    {
        switch ((vkey = vsc2vk[code]))
        {
        case VK_RSHIFT:
        case VK_RCONTROL:
        case VK_RMENU:
            for (code = 0; code < vsc2vk_size; ++code)
                if (vsc2vk[code] == (vkey - 1)) break;
            break;
        }
    }

    if (code >= vscname_size) buffer[0] = 0;
    else if (vscname[code]) lstrcpynW( buffer, vscname[code], size );
    else
    {
        vkey = MapVirtualKeyW( code & 0xff, MAPVK_VSC_TO_VK );
        tmp[0] = MapVirtualKeyW( vkey, MAPVK_VK_TO_CHAR );
        tmp[1] = 0;
        lstrcpynW( buffer, tmp, size );
    }
    len = wcslen( buffer );

    TRACE_(keyboard)( "ret %d, str %s.\n", len, debugstr_w(buffer) );
    return len;
}

/****************************************************************************
 *		ToUnicode (USER32.@)
 */
INT WINAPI ToUnicode(UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
		     LPWSTR lpwStr, int size, UINT flags)
{
    return ToUnicodeEx(virtKey, scanCode, lpKeyState, lpwStr, size, flags, GetKeyboardLayout(0));
}

/****************************************************************************
 *		ToUnicodeEx (USER32.@)
 */
INT WINAPI ToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                        WCHAR *str, int size, UINT flags, HKL layout )
{
    BOOL shift, ctrl, alt, numlock;
    WCHAR buffer[2];
    INT len;

    TRACE_(keyboard)( "virt %u, scan %u, state %p, str %p, size %d, flags %x, layout %p.\n",
                      virt, scan, state, str, size, flags, layout );

    if (!state) return 0;
    if ((len = USER_Driver->pToUnicodeEx( virt, scan, state, str, size, flags, layout )) >= -1) return len;

    alt = state[VK_MENU] & 0x80;
    shift = state[VK_SHIFT] & 0x80;
    ctrl = state[VK_CONTROL] & 0x80;
    numlock = state[VK_NUMLOCK] & 0x01;

    /* FIXME: English keyboard layout specific */

    if (scan & 0x8000) buffer[0] = 0; /* key up */
    else if (virt == VK_ESCAPE) buffer[0] = VK_ESCAPE;
    else if (!ctrl)
    {
        switch (virt)
        {
        case VK_BACK:       buffer[0] = '\b'; break;
        case VK_OEM_1:      buffer[0] = shift ? ':' : ';'; break;
        case VK_OEM_2:      buffer[0] = shift ? '?' : '/'; break;
        case VK_OEM_3:      buffer[0] = shift ? '~' : '`'; break;
        case VK_OEM_4:      buffer[0] = shift ? '{' : '['; break;
        case VK_OEM_5:      buffer[0] = shift ? '|' : '\\'; break;
        case VK_OEM_6:      buffer[0] = shift ? '}' : ']'; break;
        case VK_OEM_7:      buffer[0] = shift ? '"' : '\''; break;
        case VK_OEM_COMMA:  buffer[0] = shift ? '<' : ','; break;
        case VK_OEM_MINUS:  buffer[0] = shift ? '_' : '-'; break;
        case VK_OEM_PERIOD: buffer[0] = shift ? '>' : '.'; break;
        case VK_OEM_PLUS:   buffer[0] = shift ? '+' : '='; break;
        case VK_RETURN:     buffer[0] = '\r'; break;
        case VK_SPACE:      buffer[0] = ' '; break;
        case VK_TAB:        buffer[0] = '\t'; break;
        case VK_MULTIPLY:   buffer[0] = '*'; break;
        case VK_ADD:        buffer[0] = '+'; break;
        case VK_SUBTRACT:   buffer[0] = '-'; break;
        case VK_DIVIDE:     buffer[0] = '/'; break;
        default:
            if (virt >= '0' && virt <= '9')
                buffer[0] = shift ? ")!@#$%^&*("[virt - '0'] : virt;
            else if (virt >= 'A' && virt <= 'Z')
                buffer[0] = shift || (state[VK_CAPITAL] & 0x01) ? virt : virt + 'a' - 'A';
            else if (virt >= VK_NUMPAD0 && virt <= VK_NUMPAD9 && numlock && !shift)
                buffer[0] = '0' + virt - VK_NUMPAD0;
            else if (virt == VK_DECIMAL && numlock && !shift)
                buffer[0] = '.';
            else
                buffer[0] = 0;
            break;
        }
    }
    else if (!alt) /* Control codes */
    {
        switch (virt)
        {
        case VK_OEM_4:     buffer[0] = 0x1b; break;
        case VK_OEM_5:     buffer[0] = 0x1c; break;
        case VK_OEM_6:     buffer[0] = 0x1d; break;
        case '6':          buffer[0] = shift ? 0x1e : 0; break;
        case VK_OEM_MINUS: buffer[0] = shift ? 0x1f : 0; break;
        case VK_BACK:      buffer[0] = 0x7f; break;
        case VK_RETURN:    buffer[0] = shift ? 0 : '\n'; break;
        case '2':          buffer[0] = shift ? 0xffff : 0xf000; break;
        case VK_SPACE:     buffer[0] = ' '; break;
        default:
            if (virt >= 'A' && virt <= 'Z') buffer[0] = virt - 'A' + 1;
            else buffer[0] = 0;
            break;
        }
    }
    else buffer[0] = 0;
    buffer[1] = 0;
    len = wcslen( buffer );
    if (buffer[0] == 0xffff) buffer[0] = 0;
    lstrcpynW( str, buffer, size );

    TRACE_(keyboard)( "ret %d, str %s.\n", len, debugstr_w(str) );
    return len;
}

/****************************************************************************
 *		ToAscii (USER32.@)
 */
INT WINAPI ToAscii( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                    LPWORD lpChar, UINT flags )
{
    return ToAsciiEx(virtKey, scanCode, lpKeyState, lpChar, flags, GetKeyboardLayout(0));
}

/****************************************************************************
 *		ToAsciiEx (USER32.@)
 */
INT WINAPI ToAsciiEx( UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
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
HKL WINAPI ActivateKeyboardLayout( HKL layout, UINT flags )
{
    struct user_thread_info *info = get_user_thread_info();
    HKL old_layout;

    TRACE_(keyboard)( "layout %p, flags %x\n", layout, flags );

    if (flags) FIXME_(keyboard)( "flags %x not supported\n", flags );

    if (layout == (HKL)HKL_NEXT || layout == (HKL)HKL_PREV)
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        FIXME_(keyboard)( "HKL_NEXT and HKL_PREV not supported\n" );
        return 0;
    }

    if (!USER_Driver->pActivateKeyboardLayout( layout, flags ))
        return 0;

    old_layout = info->kbd_layout;
    info->kbd_layout = layout;
    if (old_layout != layout) info->kbd_layout_id = 0;

    if (!old_layout) return get_locale_kbd_layout();
    return old_layout;
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
UINT WINAPI GetKeyboardLayoutList( INT size, HKL *layouts )
{
    WCHAR klid[KL_NAMELENGTH], value[5];
    DWORD value_size, count, tmp, i = 0;
    HKEY hkey;
    HKL layout;

    TRACE_(keyboard)( "size %d, layouts %p.\n", size, layouts );

    if ((count = USER_Driver->pGetKeyboardLayoutList( size, layouts )) != ~0) return count;

    layout = get_locale_kbd_layout();
    count = 0;

    count++;
    if (size && layouts)
    {
        layouts[count - 1] = layout;
        if (count == size) return count;
    }

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Keyboard Layouts", &hkey ))
    {
        while (!RegEnumKeyW( hkey, i++, klid, ARRAY_SIZE(klid) ))
        {
            tmp = wcstoul( klid, NULL, 16 );
            value_size = sizeof(value);
            if (!RegGetValueW( hkey, klid, L"Layout Id", RRF_RT_REG_SZ, NULL, (void *)&value, &value_size ))
                tmp = MAKELONG( LOWORD( tmp ), 0xf000 | (wcstoul( value, NULL, 16 ) & 0xfff) );

            if (layout == UlongToHandle( tmp )) continue;

            count++;
            if (size && layouts)
            {
                layouts[count - 1] = UlongToHandle( tmp );
                if (count == size) break;
            }
        }
        RegCloseKey( hkey );
    }

    return count;
}


/***********************************************************************
 *		RegisterHotKey (USER32.@)
 */
BOOL WINAPI RegisterHotKey(HWND hwnd,INT id,UINT modifiers,UINT vk)
{
    BOOL ret;
    int replaced=0;

    TRACE_(keyboard)("(%p,%d,0x%08x,%X)\n",hwnd,id,modifiers,vk);

    if ((hwnd == NULL || WIN_IsCurrentThread(hwnd)) &&
        !USER_Driver->pRegisterHotKey(hwnd, modifiers, vk))
        return FALSE;

    SERVER_START_REQ( register_hotkey )
    {
        req->window = wine_server_user_handle( hwnd );
        req->id = id;
        req->flags = modifiers;
        req->vkey = vk;
        if ((ret = !wine_server_call_err( req )))
        {
            replaced = reply->replaced;
            modifiers = reply->flags;
            vk = reply->vkey;
        }
    }
    SERVER_END_REQ;

    if (ret && replaced)
        USER_Driver->pUnregisterHotKey(hwnd, modifiers, vk);

    return ret;
}

/***********************************************************************
 *		UnregisterHotKey (USER32.@)
 */
BOOL WINAPI UnregisterHotKey(HWND hwnd,INT id)
{
    BOOL ret;
    UINT modifiers, vk;

    TRACE_(keyboard)("(%p,%d)\n",hwnd,id);

    SERVER_START_REQ( unregister_hotkey )
    {
        req->window = wine_server_user_handle( hwnd );
        req->id = id;
        if ((ret = !wine_server_call_err( req )))
        {
            modifiers = reply->flags;
            vk = reply->vkey;
        }
    }
    SERVER_END_REQ;

    if (ret)
        USER_Driver->pUnregisterHotKey(hwnd, modifiers, vk);

    return ret;
}

/***********************************************************************
 *		LoadKeyboardLayoutW (USER32.@)
 */
HKL WINAPI LoadKeyboardLayoutW( const WCHAR *name, UINT flags )
{
    WCHAR layout_path[MAX_PATH], value[5];
    DWORD value_size, tmp;
    HKEY hkey;
    HKL layout;

    FIXME_(keyboard)( "name %s, flags %x, semi-stub!\n", debugstr_w( name ), flags );

    tmp = wcstoul( name, NULL, 16 );
    if (HIWORD( tmp )) layout = UlongToHandle( tmp );
    else layout = UlongToHandle( MAKELONG( LOWORD( tmp ), LOWORD( tmp ) ) );

    wcscpy( layout_path, L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\" );
    wcscat( layout_path, name );

    if (!RegOpenKeyW( HKEY_LOCAL_MACHINE, layout_path, &hkey ))
    {
        value_size = sizeof(value);
        if (!RegGetValueW( hkey, NULL, L"Layout Id", RRF_RT_REG_SZ, NULL, (void *)&value, &value_size ))
            layout = UlongToHandle( MAKELONG( LOWORD( tmp ), 0xf000 | (wcstoul( value, NULL, 16 ) & 0xfff) ) );

        RegCloseKey( hkey );
    }

    if ((flags & KLF_ACTIVATE) && ActivateKeyboardLayout( layout, 0 )) return layout;

    /* FIXME: semi-stub: returning default layout */
    return get_locale_kbd_layout();
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
BOOL WINAPI UnloadKeyboardLayout( HKL layout )
{
    FIXME_(keyboard)( "layout %p, stub!\n", layout );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
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

    hover_time = (ptme->dwFlags & TME_HOVER) ? ptme->dwHoverTime : HOVER_DEFAULT;

    /* if HOVER_DEFAULT was specified replace this with the system's current value.
     * TME_LEAVE doesn't need to specify hover time so use default */
    if (hover_time == HOVER_DEFAULT || hover_time == 0)
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

/***********************************************************************
 * GetMouseMovePointsEx [USER32]
 *
 * RETURNS
 *     Success: count of point set in the buffer
 *     Failure: -1
 */
int WINAPI GetMouseMovePointsEx( UINT size, LPMOUSEMOVEPOINT ptin, LPMOUSEMOVEPOINT ptout, int count, DWORD resolution )
{
    cursor_pos_t *pos, positions[64];
    int copied;
    unsigned int i;


    TRACE( "%d, %p, %p, %d, %d\n", size, ptin, ptout, count, resolution );

    if ((size != sizeof(MOUSEMOVEPOINT)) || (count < 0) || (count > ARRAY_SIZE( positions )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (!ptin || (!ptout && count))
    {
        SetLastError( ERROR_NOACCESS );
        return -1;
    }

    if (resolution != GMMP_USE_DISPLAY_POINTS)
    {
        FIXME( "only GMMP_USE_DISPLAY_POINTS is supported for now\n" );
        SetLastError( ERROR_POINT_NOT_FOUND );
        return -1;
    }

    SERVER_START_REQ( get_cursor_history )
    {
        wine_server_set_reply( req, &positions, sizeof(positions) );
        if (wine_server_call_err( req )) return -1;
    }
    SERVER_END_REQ;

    for (i = 0; i < ARRAY_SIZE( positions ); i++)
    {
        pos = &positions[i];
        if (ptin->x == pos->x && ptin->y == pos->y && (!ptin->time || ptin->time == pos->time))
            break;
    }

    if (i == ARRAY_SIZE( positions ))
    {
        SetLastError( ERROR_POINT_NOT_FOUND );
        return -1;
    }

    for (copied = 0; copied < count && i < ARRAY_SIZE( positions ); copied++, i++)
    {
        pos = &positions[i];
        ptout[copied].x = pos->x;
        ptout[copied].y = pos->y;
        ptout[copied].time = pos->time;
        ptout[copied].dwExtraInfo = pos->info;
    }

    return copied;
}

/***********************************************************************
 *		EnableMouseInPointer (USER32.@)
 */
BOOL WINAPI EnableMouseInPointer(BOOL enable)
{
    FIXME("(%#x) stub\n", enable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static DWORD CALLBACK devnotify_window_callback(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header)
{
    SendMessageTimeoutW(handle, WM_DEVICECHANGE, flags, (LPARAM)header, SMTO_ABORTIFHUNG, 2000, NULL);
    return 0;
}

static DWORD CALLBACK devnotify_service_callback(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header)
{
    FIXME("Support for service handles is not yet implemented!\n");
    return 0;
}

struct device_notification_details
{
    DWORD (CALLBACK *cb)(HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header);
    HANDLE handle;
};

extern HDEVNOTIFY WINAPI I_ScRegisterDeviceNotification( struct device_notification_details *details,
        void *filter, DWORD flags );
extern BOOL WINAPI I_ScUnregisterDeviceNotification( HDEVNOTIFY handle );

/***********************************************************************
 *		RegisterDeviceNotificationA (USER32.@)
 *
 * See RegisterDeviceNotificationW.
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationA(HANDLE hRecipient, LPVOID pNotificationFilter, DWORD dwFlags)
{
    TRACE("(hwnd=%p, filter=%p,flags=0x%08x)\n",
        hRecipient,pNotificationFilter,dwFlags);
    if (pNotificationFilter)
        FIXME("The notification filter will requires an A->W when filter support is implemented\n");
    return RegisterDeviceNotificationW(hRecipient, pNotificationFilter, dwFlags);
}

/***********************************************************************
 *		RegisterDeviceNotificationW (USER32.@)
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationW( HANDLE handle, void *filter, DWORD flags )
{
    struct device_notification_details details;

    TRACE("handle %p, filter %p, flags %#x\n", handle, filter, flags);

    if (flags & ~(DEVICE_NOTIFY_SERVICE_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES))
        FIXME("unhandled flags %#x\n", flags);

    details.handle = handle;

    if (flags & DEVICE_NOTIFY_SERVICE_HANDLE)
        details.cb = devnotify_service_callback;
    else
        details.cb = devnotify_window_callback;

    return I_ScRegisterDeviceNotification( &details, filter, 0 );
}

/***********************************************************************
 *		UnregisterDeviceNotification (USER32.@)
 */
BOOL WINAPI UnregisterDeviceNotification( HDEVNOTIFY handle )
{
    TRACE("%p\n", handle);

    return I_ScUnregisterDeviceNotification( handle );
}
