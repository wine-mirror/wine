/*
 * USER Input processing
 *
 * Copyright 1993 Bob Amstadt
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Albrecht Kleine
 * Copyright 1996 Frans van Dorsselaer
 * Copyright 1997 David Faure
 * Copyright 1998 Morten Welinder
 * Copyright 1998 Ulrich Weigand
 * Copyright 2001 Eric Pouech
 * Copyright 2002 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);

static const WCHAR keyboard_layouts_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l',
    '\\','K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','s'
};


LONG global_key_state_counter = 0;

/**********************************************************************
 *	     NtUserAttachThreadInput    (win32u.@)
 */
BOOL WINAPI NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach )
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

/***********************************************************************
 *           __wine_send_input  (win32u.@)
 *
 * Internal SendInput function to allow the graphics driver to inject real events.
 */
BOOL CDECL __wine_send_input( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput )
{
    return set_ntstatus( send_hardware_message( hwnd, input, rawinput, 0 ));
}

/***********************************************************************
 *		update_mouse_coords
 *
 * Helper for NtUserSendInput.
 */
static void update_mouse_coords( INPUT *input )
{
    if (!(input->mi.dwFlags & MOUSEEVENTF_MOVE)) return;

    if (input->mi.dwFlags & MOUSEEVENTF_ABSOLUTE)
    {
        RECT rc;

        if (input->mi.dwFlags & MOUSEEVENTF_VIRTUALDESK)
            rc = get_virtual_screen_rect( 0 );
        else
            rc = get_primary_monitor_rect( 0 );

        input->mi.dx = rc.left + ((input->mi.dx * (rc.right - rc.left)) >> 16);
        input->mi.dy = rc.top  + ((input->mi.dy * (rc.bottom - rc.top)) >> 16);
    }
    else
    {
        int accel[3];

        /* dx and dy can be negative numbers for relative movements */
        NtUserSystemParametersInfo( SPI_GETMOUSE, 0, accel, 0 );

        if (!accel[2]) return;

        if (abs( input->mi.dx ) > accel[0])
        {
            input->mi.dx *= 2;
            if (abs( input->mi.dx ) > accel[1] && accel[2] == 2) input->mi.dx *= 2;
        }
        if (abs(input->mi.dy) > accel[0])
        {
            input->mi.dy *= 2;
            if (abs( input->mi.dy ) > accel[1] && accel[2] == 2) input->mi.dy *= 2;
        }
    }
}

/***********************************************************************
 *           NtUserSendInput  (win32u.@)
 */
UINT WINAPI NtUserSendInput( UINT count, INPUT *inputs, int size )
{
    UINT i;
    NTSTATUS status = STATUS_SUCCESS;

    if (size != sizeof(INPUT))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!count)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!inputs)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
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
            RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
            return 0;
        }

        if (status)
        {
            RtlSetLastWin32Error( RtlNtStatusToDosError(status) );
            break;
        }
    }

    return i;
}

/***********************************************************************
 *	     NtUserSetCursorPos (win32u.@)
 */
BOOL WINAPI NtUserSetCursorPos( INT x, INT y )
{
    POINT pt = { x, y };
    BOOL ret;
    INT prev_x, prev_y, new_x, new_y;
    UINT dpi;

    if ((dpi = get_thread_dpi()))
    {
        HMONITOR monitor = monitor_from_point( pt, MONITOR_DEFAULTTOPRIMARY, get_thread_dpi() );
        pt = map_dpi_point( pt, dpi, get_monitor_dpi( monitor ));
    }

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
    if (ret && (prev_x != new_x || prev_y != new_y)) user_driver->pSetCursorPos( new_x, new_y );
    return ret;
}

/***********************************************************************
 *	     get_cursor_pos
 */
BOOL get_cursor_pos( POINT *pt )
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
    if (ret && NtGetTickCount() - last_change > 100) ret = user_driver->pGetCursorPos( pt );
    if (ret && (dpi = get_thread_dpi()))
    {
        HMONITOR monitor = monitor_from_point( *pt, MONITOR_DEFAULTTOPRIMARY, 0 );
        *pt = map_dpi_point( *pt, get_monitor_dpi( monitor ), dpi );
    }
    return ret;
}

/***********************************************************************
 *	     NtUserGetCursorInfo (win32u.@)
 */
BOOL WINAPI NtUserGetCursorInfo( CURSORINFO *info )
{
    BOOL ret;

    if (!info) return FALSE;

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = 0;
        if ((ret = !wine_server_call( req )))
        {
            info->hCursor = wine_server_ptr_handle( reply->cursor );
            info->flags = reply->show_count >= 0 ? CURSOR_SHOWING : 0;
        }
    }
    SERVER_END_REQ;
    get_cursor_pos( &info->ptScreenPos );
    return ret;
}

static void check_for_events( UINT flags )
{
    LARGE_INTEGER zero = { .QuadPart = 0 };
    if (user_driver->pMsgWaitForMultipleObjectsEx( 0, NULL, &zero, flags, 0 ) == WAIT_TIMEOUT)
        flush_window_surfaces( TRUE );
}

/**********************************************************************
 *           GetAsyncKeyState (win32u.@)
 */
SHORT WINAPI NtUserGetAsyncKeyState( INT key )
{
    struct user_key_state_info *key_state_info = get_user_thread_info()->key_state;
    INT counter = global_key_state_counter;
    BYTE prev_key_state;
    SHORT ret;

    if (key < 0 || key >= 256) return 0;

    check_for_events( QS_INPUT );

    if (key_state_info && !(key_state_info->state[key] & 0xc0) &&
        key_state_info->counter == counter && NtGetTickCount() - key_state_info->time < 50)
    {
        /* use cached value */
        return 0;
    }
    else if (!key_state_info)
    {
        key_state_info = calloc( 1, sizeof(*key_state_info) );
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

                key_state_info->time    = NtGetTickCount();
                key_state_info->counter = counter;
            }
        }
    }
    SERVER_END_REQ;

    return ret;
}

/***********************************************************************
 *           NtUserGetQueueStatus (win32u.@)
 */
DWORD WINAPI NtUserGetQueueStatus( UINT flags )
{
    DWORD ret;

    if (flags & ~(QS_ALLINPUT | QS_ALLPOSTMESSAGE | QS_SMRESULT))
    {
        RtlSetLastWin32Error( ERROR_INVALID_FLAGS );
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
 *           get_input_state
 */
DWORD get_input_state(void)
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

/***********************************************************************
 *           get_locale_kbd_layout
 */
static HKL get_locale_kbd_layout(void)
{
    LCID layout;
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

    NtQueryDefaultLocale( TRUE, &layout );

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

    return ULongToHandle( layout );
}

/***********************************************************************
 *	     NtUserGetKeyboardLayout    (win32u.@)
 *
 * Device handle for keyboard layout defaulted to
 * the language id. This is the way Windows default works.
 */
HKL WINAPI NtUserGetKeyboardLayout( DWORD thread_id )
{
    struct user_thread_info *thread = get_user_thread_info();
    HKL layout = thread->kbd_layout;

    if (thread_id && thread_id != GetCurrentThreadId())
        FIXME( "couldn't return keyboard layout for thread %04x\n", thread_id );

    if (!layout) return get_locale_kbd_layout();
    return layout;
}

/**********************************************************************
 *	     NtUserGetKeyState    (win32u.@)
 *
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.
 */
SHORT WINAPI NtUserGetKeyState( INT vkey )
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
 *	     NtUserGetKeyboardState    (win32u.@)
 */
BOOL WINAPI NtUserGetKeyboardState( BYTE *state )
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
 *	     NtUserSetKeyboardState    (win32u.@)
 */
BOOL WINAPI NtUserSetKeyboardState( BYTE *state )
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

/******************************************************************************
 *	     NtUserVkKeyScanEx    (win32u.@)
 */
WORD WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout )
{
    WORD shift = 0x100, ctrl = 0x200;
    SHORT ret;

    TRACE_(keyboard)( "chr %s, layout %p\n", debugstr_wn(&chr, 1), layout );

    if ((ret = user_driver->pVkKeyScanEx( chr, layout )) != -256) return ret;

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

static const char *kbd_en_vscname[] =
{
    0, "Esc", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Backspace", "Tab",
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Enter", "Ctrl", 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Shift", 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, "Right Shift", "Num *", "Alt", "Space", "Caps Lock", "F1", "F2", "F3", "F4", "F5",
    "F6", "F7", "F8", "F9", "F10", "Pause", "Scroll Lock", "Num 7", "Num 8", "Num 9", "Num -", "Num 4", "Num 5", "Num 6", "Num +", "Num 1",
    "Num 2", "Num 3", "Num 0", "Num Del", "Sys Req", 0, 0, "F11", "F12", 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "F13", "F14", "F15", "F16",
    "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* extended */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Num Enter", "Right Ctrl", 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, "Num /", 0, "Prnt Scrn", "Right Alt", 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, "Num Lock", "Break", "Home", "Up", "Page Up", 0, "Left", 0, "Right", 0, "End",
    "Down", "Page Down", "Insert", "Delete", "<00>", 0, "Help", 0, 0, 0, 0, "Left Windows", "Right Windows", "Application", 0, 0,
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
 *	     NtUserMapVirtualKeyEx    (win32u.@)
 */
UINT WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    const UINT *vsc2vk, *vk2char;
    UINT vsc2vk_size, vk2char_size;
    UINT ret;

    TRACE_(keyboard)( "code %u, type %u, layout %p.\n", code, type, layout );

    if ((ret = user_driver->pMapVirtualKeyEx( code, type, layout )) != -1) return ret;

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
 *	     NtUserGetKeyNameText    (win32u.@)
 */
INT WINAPI NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size )
{
    INT code = ((lparam >> 16) & 0x1ff), vkey, len;
    UINT vsc2vk_size, vscname_size;
    const char *const *vscname;
    const UINT *vsc2vk;

    TRACE_(keyboard)( "lparam %d, buffer %p, size %d.\n", lparam, buffer, size );

    if (!buffer || !size) return 0;
    if ((len = user_driver->pGetKeyNameText( lparam, buffer, size )) >= 0) return len;

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

    if (code < vscname_size)
    {
        if (vscname[code])
        {
            len = min( size - 1, strlen(vscname[code]) );
            ascii_to_unicode( buffer, vscname[code], len );
        }
        else if (size > 1)
        {
            HKL hkl = NtUserGetKeyboardLayout( 0 );
            vkey = NtUserMapVirtualKeyEx( code & 0xff, MAPVK_VSC_TO_VK, hkl );
            buffer[0] = NtUserMapVirtualKeyEx( vkey, MAPVK_VK_TO_CHAR, hkl );
            len = 1;
        }
    }
    buffer[len] = 0;

    TRACE_(keyboard)( "ret %d, str %s.\n", len, debugstr_w(buffer) );
    return len;
}

/****************************************************************************
 *	     NtUserToUnicodeEx    (win32u.@)
 */
INT WINAPI NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                              WCHAR *str, int size, UINT flags, HKL layout )
{
    BOOL shift, ctrl, alt, numlock;
    WCHAR buffer[2];
    INT len;

    TRACE_(keyboard)( "virt %u, scan %u, state %p, str %p, size %d, flags %x, layout %p.\n",
                      virt, scan, state, str, size, flags, layout );

    if (!state) return 0;
    if ((len = user_driver->pToUnicodeEx( virt, scan, state, str, size, flags, layout )) >= -1)
        return len;

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
    len = lstrlenW( buffer );
    if (buffer[0] == 0xffff) buffer[0] = 0;
    lstrcpynW( str, buffer, size );

    TRACE_(keyboard)( "ret %d, str %s.\n", len, debugstr_w(str) );
    return len;
}

/**********************************************************************
 *	     NtUserActivateKeyboardLayout    (win32u.@)
 */
HKL WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags )
{
    struct user_thread_info *info = get_user_thread_info();
    HKL old_layout;

    TRACE_(keyboard)( "layout %p, flags %x\n", layout, flags );

    if (flags) FIXME_(keyboard)( "flags %x not supported\n", flags );

    if (layout == (HKL)HKL_NEXT || layout == (HKL)HKL_PREV)
    {
        RtlSetLastWin32Error( ERROR_CALL_NOT_IMPLEMENTED );
        FIXME_(keyboard)( "HKL_NEXT and HKL_PREV not supported\n" );
        return 0;
    }

    if (!user_driver->pActivateKeyboardLayout( layout, flags ))
        return 0;

    old_layout = info->kbd_layout;
    info->kbd_layout = layout;
    if (old_layout != layout) info->kbd_layout_id = 0;

    if (!old_layout) return get_locale_kbd_layout();
    return old_layout;
}



/***********************************************************************
 *	     NtUserGetKeyboardLayoutList    (win32u.@)
 *
 * Return number of values available if either input parm is
 *  0, per MS documentation.
 */
UINT WINAPI NtUserGetKeyboardLayoutList( INT size, HKL *layouts )
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key_info = (KEY_NODE_INFORMATION *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value_info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD count, tmp, i = 0;
    HKEY hkey, subkey;
    HKL layout;

    TRACE_(keyboard)( "size %d, layouts %p.\n", size, layouts );

    if ((count = user_driver->pGetKeyboardLayoutList( size, layouts )) != ~0) return count;

    layout = get_locale_kbd_layout();
    count = 0;

    count++;
    if (size && layouts)
    {
        layouts[count - 1] = layout;
        if (count == size) return count;
    }

    if ((hkey = reg_open_key( NULL, keyboard_layouts_keyW, sizeof(keyboard_layouts_keyW) )))
    {
        while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key_info,
                                sizeof(buffer) - sizeof(WCHAR), &tmp ))
        {
            if (!(subkey = reg_open_key( hkey, key_info->Name, key_info->NameLength ))) continue;
            key_info->Name[key_info->NameLength / sizeof(WCHAR)] = 0;
            tmp = wcstoul( key_info->Name, NULL, 16 );
            if (query_reg_ascii_value( subkey, "Layout Id", value_info, sizeof(buffer) ) &&
                value_info->Type == REG_SZ)
                tmp = MAKELONG( LOWORD( tmp ),
                                0xf000 | (wcstoul( (const WCHAR *)value_info->Data, NULL, 16 ) & 0xfff) );
            NtClose( subkey );

            if (layout == UlongToHandle( tmp )) continue;

            count++;
            if (size && layouts)
            {
                layouts[count - 1] = UlongToHandle( tmp );
                if (count == size) break;
            }
        }
        NtClose( hkey );
    }

    return count;
}

/****************************************************************************
 *	     NtUserGetKeyboardLayoutName    (win32u.@)
 */
BOOL WINAPI NtUserGetKeyboardLayoutName( WCHAR *name )
{
    struct user_thread_info *info = get_user_thread_info();
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (KEY_NODE_INFORMATION *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    WCHAR klid[KL_NAMELENGTH];
    DWORD tmp, i = 0;
    HKEY hkey, subkey;
    HKL layout;

    TRACE_(keyboard)( "name %p\n", name );

    if (!name)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }

    if (info->kbd_layout_id)
    {
        sprintf( buffer, "%08X", info->kbd_layout_id );
        asciiz_to_unicode( name, buffer );
        return TRUE;
    }

    layout = NtUserGetKeyboardLayout( 0 );
    tmp = HandleToUlong( layout );
    if (HIWORD( tmp ) == LOWORD( tmp )) tmp = LOWORD( tmp );
    sprintf( buffer, "%08X", tmp );
    asciiz_to_unicode( name, buffer );

    if ((hkey = reg_open_key( NULL, keyboard_layouts_keyW, sizeof(keyboard_layouts_keyW) )))
    {
        while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key,
                                sizeof(buffer) - sizeof(WCHAR), &tmp ))
        {
            if (!(subkey = reg_open_key( hkey, key->Name, key->NameLength ))) continue;
            memcpy( klid, key->Name, key->NameLength );
            klid[key->NameLength / sizeof(WCHAR)] = 0;
            if (query_reg_ascii_value( subkey, "Layout Id", value, sizeof(buffer) ) &&
                value->Type == REG_SZ)
                tmp = 0xf000 | (wcstoul( (const WCHAR *)value->Data, NULL, 16 ) & 0xfff);
            else
                tmp = wcstoul( klid, NULL, 16 );
            NtClose( subkey );

            if (HIWORD( layout ) == tmp)
            {
                lstrcpynW( name, klid, KL_NAMELENGTH );
                break;
            }
        }
        NtClose( hkey );
    }

    info->kbd_layout_id = wcstoul( name, NULL, 16 );

    TRACE_(keyboard)( "ret %s\n", debugstr_w( name ) );
    return TRUE;
}

/***********************************************************************
 *	     NtUserRegisterHotKey (win32u.@)
 */
BOOL WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk )
{
    BOOL ret;
    int replaced = 0;

    TRACE_(keyboard)( "(%p,%d,0x%08x,%X)\n", hwnd, id, modifiers, vk );

    if ((!hwnd || is_current_thread_window( hwnd )) &&
        !user_driver->pRegisterHotKey( hwnd, modifiers, vk ))
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
        user_driver->pUnregisterHotKey(hwnd, modifiers, vk);

    return ret;
}

/***********************************************************************
 *	     NtUserUnregisterHotKey    (win32u.@)
 */
BOOL WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id )
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
        user_driver->pUnregisterHotKey(hwnd, modifiers, vk);

    return ret;
}

/***********************************************************************
 *           NtUserGetMouseMovePointsEx    (win32u.@)
 */
int WINAPI NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                       int count, DWORD resolution )
{
    cursor_pos_t *pos, positions[64];
    int copied;
    unsigned int i;


    TRACE( "%d, %p, %p, %d, %d\n", size, ptin, ptout, count, resolution );

    if ((size != sizeof(MOUSEMOVEPOINT)) || (count < 0) || (count > ARRAY_SIZE( positions )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (!ptin || (!ptout && count))
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return -1;
    }

    if (resolution != GMMP_USE_DISPLAY_POINTS)
    {
        FIXME( "only GMMP_USE_DISPLAY_POINTS is supported for now\n" );
        RtlSetLastWin32Error( ERROR_POINT_NOT_FOUND );
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
        RtlSetLastWin32Error( ERROR_POINT_NOT_FOUND );
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

static WORD get_key_state(void)
{
    WORD ret = 0;

    if (get_system_metrics( SM_SWAPBUTTON ))
    {
        if (NtUserGetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (NtUserGetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (NtUserGetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (NtUserGetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    if (NtUserGetAsyncKeyState(VK_MBUTTON) & 0x80)  ret |= MK_MBUTTON;
    if (NtUserGetAsyncKeyState(VK_SHIFT) & 0x80)    ret |= MK_SHIFT;
    if (NtUserGetAsyncKeyState(VK_CONTROL) & 0x80)  ret |= MK_CONTROL;
    if (NtUserGetAsyncKeyState(VK_XBUTTON1) & 0x80) ret |= MK_XBUTTON1;
    if (NtUserGetAsyncKeyState(VK_XBUTTON2) & 0x80) ret |= MK_XBUTTON2;
    return ret;
}

struct tracking_list
{
    TRACKMOUSEEVENT info;
    POINT pos; /* center of hover rectangle */
};

/* FIXME: move tracking stuff into per-thread data */
static struct tracking_list tracking_info;

static void check_mouse_leave( HWND hwnd, int hittest )
{
    if (tracking_info.info.hwndTrack != hwnd)
    {
        if (tracking_info.info.dwFlags & TME_NONCLIENT)
            NtUserPostMessage( tracking_info.info.hwndTrack, WM_NCMOUSELEAVE, 0, 0 );
        else
            NtUserPostMessage( tracking_info.info.hwndTrack, WM_MOUSELEAVE, 0, 0 );

        tracking_info.info.dwFlags &= ~TME_LEAVE;
    }
    else
    {
        if (hittest == HTCLIENT)
        {
            if (tracking_info.info.dwFlags & TME_NONCLIENT)
            {
                NtUserPostMessage( tracking_info.info.hwndTrack, WM_NCMOUSELEAVE, 0, 0 );
                tracking_info.info.dwFlags &= ~TME_LEAVE;
            }
        }
        else
        {
            if (!(tracking_info.info.dwFlags & TME_NONCLIENT))
            {
                NtUserPostMessage( tracking_info.info.hwndTrack, WM_MOUSELEAVE, 0, 0 );
                tracking_info.info.dwFlags &= ~TME_LEAVE;
            }
        }
    }
}

void update_mouse_tracking_info( HWND hwnd )
{
    int hover_width = 0, hover_height = 0, hittest;
    POINT pos;

    TRACE( "hwnd %p\n", hwnd );

    get_cursor_pos( &pos );
    hwnd = window_from_point( hwnd, pos, &hittest );

    TRACE( "point %s hwnd %p hittest %d\n", wine_dbgstr_point(&pos), hwnd, hittest );

    NtUserSystemParametersInfo( SPI_GETMOUSEHOVERWIDTH, 0, &hover_width, 0 );
    NtUserSystemParametersInfo( SPI_GETMOUSEHOVERHEIGHT, 0, &hover_height, 0 );

    TRACE( "tracked pos %s, current pos %s, hover width %d, hover height %d\n",
           wine_dbgstr_point(&tracking_info.pos), wine_dbgstr_point(&pos),
           hover_width, hover_height );

    if (tracking_info.info.dwFlags & TME_LEAVE)
        check_mouse_leave( hwnd, hittest );

    if (tracking_info.info.hwndTrack != hwnd)
        tracking_info.info.dwFlags &= ~TME_HOVER;

    if (tracking_info.info.dwFlags & TME_HOVER)
    {
        /* has the cursor moved outside the rectangle centered around pos? */
        if ((abs( pos.x - tracking_info.pos.x ) > (hover_width / 2)) ||
            (abs( pos.y - tracking_info.pos.y ) > (hover_height / 2)))
        {
            tracking_info.pos = pos;
        }
        else
        {
            if (hittest == HTCLIENT)
            {
                screen_to_client(hwnd, &pos);
                TRACE( "client cursor pos %s\n", wine_dbgstr_point(&pos) );

                NtUserPostMessage( tracking_info.info.hwndTrack, WM_MOUSEHOVER,
                                   get_key_state(), MAKELPARAM( pos.x, pos.y ) );
            }
            else
            {
                if (tracking_info.info.dwFlags & TME_NONCLIENT)
                    NtUserPostMessage( tracking_info.info.hwndTrack, WM_NCMOUSEHOVER,
                                       hittest, MAKELPARAM( pos.x, pos.y ) );
            }

            /* stop tracking mouse hover */
            tracking_info.info.dwFlags &= ~TME_HOVER;
        }
    }

    /* stop the timer if the tracking list is empty */
    if (!(tracking_info.info.dwFlags & (TME_HOVER | TME_LEAVE)))
    {
        kill_system_timer( tracking_info.info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE );
        tracking_info.info.hwndTrack = 0;
        tracking_info.info.dwFlags = 0;
        tracking_info.info.dwHoverTime = 0;
    }
}

/***********************************************************************
 *           NtUserTrackMouseEvent    (win32u.@)
 */
BOOL WINAPI NtUserTrackMouseEvent( TRACKMOUSEEVENT *info )
{
    DWORD hover_time;
    int hittest;
    HWND hwnd;
    POINT pos;

    TRACE( "size %u, flags %#x, hwnd %p, time %u\n",
           info->cbSize, info->dwFlags, info->hwndTrack, info->dwHoverTime );

    if (info->cbSize != sizeof(TRACKMOUSEEVENT))
    {
        WARN( "wrong size %u\n", info->cbSize );
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (info->dwFlags & TME_QUERY)
    {
        *info = tracking_info.info;
        info->cbSize = sizeof(TRACKMOUSEEVENT);
        return TRUE;
    }

    if (!is_window( info->hwndTrack ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    hover_time = (info->dwFlags & TME_HOVER) ? info->dwHoverTime : HOVER_DEFAULT;

    if (hover_time == HOVER_DEFAULT || hover_time == 0)
        NtUserSystemParametersInfo( SPI_GETMOUSEHOVERTIME, 0, &hover_time, 0 );

    get_cursor_pos( &pos );
    hwnd = window_from_point( info->hwndTrack, pos, &hittest );
    TRACE( "point %s hwnd %p hittest %d\n", wine_dbgstr_point(&pos), hwnd, hittest );

    if (info->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT))
        FIXME( "ignoring flags %#x\n", info->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT) );

    if (info->dwFlags & TME_CANCEL)
    {
        if (tracking_info.info.hwndTrack == info->hwndTrack)
        {
            tracking_info.info.dwFlags &= ~(info->dwFlags & ~TME_CANCEL);

            /* if we aren't tracking on hover or leave remove this entry */
            if (!(tracking_info.info.dwFlags & (TME_HOVER | TME_LEAVE)))
            {
                kill_system_timer( tracking_info.info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE );
                tracking_info.info.hwndTrack = 0;
                tracking_info.info.dwFlags = 0;
                tracking_info.info.dwHoverTime = 0;
            }
        }
    }
    else
    {
        /* In our implementation, it's possible that another window will receive
         * WM_MOUSEMOVE and call TrackMouseEvent before TrackMouseEventProc is
         * called. In such a situation, post the WM_MOUSELEAVE now. */
        if ((tracking_info.info.dwFlags & TME_LEAVE) && tracking_info.info.hwndTrack != NULL)
            check_mouse_leave(hwnd, hittest);

        kill_system_timer( tracking_info.info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE );
        tracking_info.info.hwndTrack = 0;
        tracking_info.info.dwFlags = 0;
        tracking_info.info.dwHoverTime = 0;

        if (info->hwndTrack == hwnd)
        {
            /* Adding new mouse event to the tracking list */
            tracking_info.info = *info;
            tracking_info.info.dwHoverTime = hover_time;

            /* Initialize HoverInfo variables even if not hover tracking */
            tracking_info.pos = pos;

            NtUserSetSystemTimer( tracking_info.info.hwndTrack, SYSTEM_TIMER_TRACK_MOUSE, hover_time );
        }
    }

    return TRUE;
}

/*******************************************************************
 *           NtUserDragDetect (win32u.@)
 */
BOOL WINAPI NtUserDragDetect( HWND hwnd, int x, int y )
{
    WORD width, height;
    RECT rect;
    MSG msg;

    TRACE( "%p (%d,%d)\n", hwnd, x, y );

    if (!(NtUserGetKeyState( VK_LBUTTON ) & 0x8000)) return FALSE;

    width  = get_system_metrics( SM_CXDRAG );
    height = get_system_metrics( SM_CYDRAG );
    SetRect( &rect, x - width, y - height, x + width, y + height );

    NtUserSetCapture( hwnd );

    for (;;)
    {
        while (NtUserPeekMessage( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ))
        {
            if (msg.message == WM_LBUTTONUP)
            {
                release_capture();
                return FALSE;
            }
            if (msg.message == WM_MOUSEMOVE)
            {
                POINT tmp;
                tmp.x = (short)LOWORD( msg.lParam );
                tmp.y = (short)HIWORD( msg.lParam );
                if (!PtInRect( &rect, tmp ))
                {
                    release_capture();
                    return TRUE;
                }
            }
        }
        NtUserMsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, 0 );
    }
    return FALSE;
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
        user_driver->pSetCapture( hwnd, gui_flags );

        if (previous)
            send_message( previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd );

        if (prev_ret) *prev_ret = previous;
    }
    return ret;
}

/**********************************************************************
 *           NtUserSetCapture (win32u.@)
 */
HWND WINAPI NtUserSetCapture( HWND hwnd )
{
    HWND previous = 0;

    set_capture_window( hwnd, 0, &previous );
    return previous;
}

/**********************************************************************
 *           release_capture
 */
BOOL WINAPI release_capture(void)
{
    BOOL ret = set_capture_window( 0, 0, NULL );

    /* Somebody may have missed some mouse movements */
    if (ret)
    {
        INPUT input = { .type = INPUT_MOUSE };
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        NtUserSendInput( 1, &input, sizeof(input) );
    }

    return ret;
}

/*******************************************************************
 *           NtUserGetForegroundWindow  (win32u.@)
 */
HWND WINAPI NtUserGetForegroundWindow(void)
{
    HWND ret = 0;

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = 0;
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->foreground );
    }
    SERVER_END_REQ;
    return ret;
}

/* see GetActiveWindow */
HWND get_active_window(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndActive : 0;
}

/* see GetCapture */
HWND get_capture(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndCapture : 0;
}

/* see GetFocus */
HWND get_focus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndFocus : 0;
}

/*****************************************************************
 *		set_focus_window
 *
 * Change the focus window, sending the WM_SETFOCUS and WM_KILLFOCUS messages
 */
static HWND set_focus_window( HWND hwnd )
{
    HWND previous = 0, ime_hwnd;
    BOOL ret;

    SERVER_START_REQ( set_focus_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
            previous = wine_server_ptr_handle( reply->previous );
    }
    SERVER_END_REQ;
    if (!ret) return 0;
    if (previous == hwnd) return previous;

    if (previous)
    {
        send_message( previous, WM_KILLFOCUS, (WPARAM)hwnd, 0 );

        ime_hwnd = get_default_ime_window( previous );
        if (ime_hwnd)
            send_message( ime_hwnd, WM_IME_INTERNAL, IME_INTERNAL_DEACTIVATE,
                          HandleToUlong(previous) );

        if (hwnd != get_focus()) return previous;  /* changed by the message */
    }
    if (is_window(hwnd))
    {
        user_driver->pSetFocus(hwnd);

        ime_hwnd = get_default_ime_window( hwnd );
        if (ime_hwnd)
            send_message( ime_hwnd, WM_IME_INTERNAL, IME_INTERNAL_ACTIVATE,
                          HandleToUlong(hwnd) );

        if (previous)
            NtUserNotifyWinEvent( EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, 0 );

        send_message( hwnd, WM_SETFOCUS, (WPARAM)previous, 0 );
    }
    return previous;
}

/*******************************************************************
 *		set_active_window
 */
static BOOL set_active_window( HWND hwnd, HWND *prev, BOOL mouse, BOOL focus )
{
    HWND previous = get_active_window();
    BOOL ret;
    DWORD old_thread, new_thread;
    CBTACTIVATESTRUCT cbt;

    if (previous == hwnd)
    {
        if (prev) *prev = hwnd;
        return TRUE;
    }

    /* call CBT hook chain */
    cbt.fMouse     = mouse;
    cbt.hWndActive = previous;
    if (call_hooks( WH_CBT, HCBT_ACTIVATE, (WPARAM)hwnd, (LPARAM)&cbt, sizeof(cbt) )) return FALSE;

    if (is_window( previous ))
    {
        send_message( previous, WM_NCACTIVATE, FALSE, (LPARAM)hwnd );
        send_message( previous, WM_ACTIVATE,
                      MAKEWPARAM( WA_INACTIVE, is_iconic(previous) ), (LPARAM)hwnd );
    }

    SERVER_START_REQ( set_active_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
            previous = wine_server_ptr_handle( reply->previous );
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;
    if (prev) *prev = previous;
    if (previous == hwnd) return TRUE;

    if (hwnd)
    {
        /* send palette messages */
        if (send_message( hwnd, WM_QUERYNEWPALETTE, 0, 0 ))
            send_message_timeout( HWND_BROADCAST, WM_PALETTEISCHANGING, (WPARAM)hwnd, 0,
                                  SMTO_ABORTIFHUNG, 2000, FALSE );
        if (!is_window(hwnd)) return FALSE;
    }

    old_thread = previous ? get_window_thread( previous, NULL ) : 0;
    new_thread = hwnd ? get_window_thread( hwnd, NULL ) : 0;

    if (old_thread != new_thread)
    {
        HWND *list, *phwnd;

        if ((list = list_window_children( NULL, get_desktop_window(), NULL, 0 )))
        {
            if (old_thread)
            {
                for (phwnd = list; *phwnd; phwnd++)
                {
                    if (get_window_thread( *phwnd, NULL ) == old_thread)
                        send_message( *phwnd, WM_ACTIVATEAPP, 0, new_thread );
                }
            }
            if (new_thread)
            {
                for (phwnd = list; *phwnd; phwnd++)
                {
                    if (get_window_thread( *phwnd, NULL ) == new_thread)
                        send_message( *phwnd, WM_ACTIVATEAPP, 1, old_thread );
                }
            }
            free( list );
        }
    }

    if (is_window(hwnd))
    {
        send_message( hwnd, WM_NCACTIVATE, hwnd == NtUserGetForegroundWindow(), (LPARAM)previous );
        send_message( hwnd, WM_ACTIVATE,
                      MAKEWPARAM( mouse ? WA_CLICKACTIVE : WA_ACTIVE, is_iconic(hwnd) ),
                      (LPARAM)previous );
        if (NtUserGetAncestor( hwnd, GA_PARENT ) == get_desktop_window())
            NtUserPostMessage( get_desktop_window(), WM_PARENTNOTIFY, WM_NCACTIVATE, (LPARAM)hwnd );
    }

    /* now change focus if necessary */
    if (focus)
    {
        GUITHREADINFO info;

        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info );
        /* Do not change focus if the window is no more active */
        if (hwnd == info.hwndActive)
        {
            if (!info.hwndFocus || !hwnd || NtUserGetAncestor( info.hwndFocus, GA_ROOT ) != hwnd)
                set_focus_window( hwnd );
        }
    }

    return TRUE;
}

/**********************************************************************
 *           NtUserSetActiveWindow    (win32u.@)
 */
HWND WINAPI NtUserSetActiveWindow( HWND hwnd )
{
    HWND prev;

    TRACE( "%p\n", hwnd );

    if (hwnd)
    {
        LONG style;

        hwnd = get_full_window_handle( hwnd );
        if (!is_window( hwnd ))
        {
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }

        style = get_window_long( hwnd, GWL_STYLE );
        if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD)
            return get_active_window();  /* Windows doesn't seem to return an error here */
    }

    if (!set_active_window( hwnd, &prev, FALSE, TRUE )) return 0;
    return prev;
}

/*****************************************************************
 *           NtUserSetFocus  (win32u.@)
 */
HWND WINAPI NtUserSetFocus( HWND hwnd )
{
    HWND hwndTop = hwnd;
    HWND previous = get_focus();

    TRACE( "%p prev %p\n", hwnd, previous );

    if (hwnd)
    {
        /* Check if we can set the focus to this window */
        hwnd = get_full_window_handle( hwnd );
        if (!is_window( hwnd ))
        {
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (hwnd == previous) return previous;  /* nothing to do */
        for (;;)
        {
            HWND parent;
            LONG style = get_window_long( hwndTop, GWL_STYLE );
            if (style & (WS_MINIMIZE | WS_DISABLED)) return 0;
            if (!(style & WS_CHILD)) break;
            parent = NtUserGetAncestor( hwndTop, GA_PARENT );
            if (!parent || parent == get_desktop_window())
            {
                if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return 0;
                break;
            }
            if (parent == get_hwnd_message_parent()) return 0;
            hwndTop = parent;
        }

        /* call hooks */
        if (call_hooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd, (LPARAM)previous, 0 )) return 0;

        /* activate hwndTop if needed. */
        if (hwndTop != get_active_window())
        {
            if (!set_active_window( hwndTop, NULL, FALSE, FALSE )) return 0;
            if (!is_window( hwnd )) return 0;  /* Abort if window destroyed */

            /* Do not change focus if the window is no longer active */
            if (hwndTop != get_active_window()) return 0;
        }
    }
    else /* NULL hwnd passed in */
    {
        if (!previous) return 0;  /* nothing to do */
        if (call_hooks( WH_CBT, HCBT_SETFOCUS, 0, (LPARAM)previous, 0 )) return 0;
    }

    /* change focus and send messages */
    return set_focus_window( hwnd );
}

/*******************************************************************
 *		set_foreground_window
 */
BOOL set_foreground_window( HWND hwnd, BOOL mouse )
{
    BOOL ret, send_msg_old = FALSE, send_msg_new = FALSE;
    HWND previous = 0;

    if (mouse) hwnd = get_full_window_handle( hwnd );

    SERVER_START_REQ( set_foreground_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
        {
            previous = wine_server_ptr_handle( reply->previous );
            send_msg_old = reply->send_msg_old;
            send_msg_new = reply->send_msg_new;
        }
    }
    SERVER_END_REQ;

    if (ret && previous != hwnd)
    {
        if (send_msg_old)  /* old window belongs to other thread */
            NtUserMessageCall( previous, WM_WINE_SETACTIVEWINDOW, 0, 0,
                               0, NtUserSendNotifyMessage, FALSE );
        else if (send_msg_new)  /* old window belongs to us but new one to other thread */
            ret = set_active_window( 0, NULL, mouse, TRUE );

        if (send_msg_new)  /* new window belongs to other thread */
            NtUserMessageCall( hwnd, WM_WINE_SETACTIVEWINDOW, (WPARAM)hwnd, 0,
                               0, NtUserSendNotifyMessage, FALSE );
        else  /* new window belongs to us */
            ret = set_active_window( hwnd, NULL, mouse, TRUE );
    }
    return ret;
}

struct
{
    HBITMAP bitmap;
    unsigned int timeout;
} caret = {0, 500};

static void display_caret( HWND hwnd, const RECT *r )
{
    HDC dc, mem_dc;

    /* do not use DCX_CACHE here, since coördinates are in logical units */
    if (!(dc = NtUserGetDCEx( hwnd, 0, DCX_USESTYLE )))
        return;
    mem_dc = NtGdiCreateCompatibleDC(dc);
    if (mem_dc)
    {
        HBITMAP prev_bitmap;

        prev_bitmap = NtGdiSelectBitmap( mem_dc, caret.bitmap );
        NtGdiBitBlt( dc, r->left, r->top, r->right-r->left, r->bottom-r->top, mem_dc, 0, 0, SRCINVERT, 0, 0 );
        NtGdiSelectBitmap( mem_dc, prev_bitmap );
        NtGdiDeleteObjectApp( mem_dc );
    }
    NtUserReleaseDC( hwnd, dc );
}

static unsigned int get_caret_registry_timeout(void)
{
    char value_buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[11 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)value_buffer;
    unsigned int ret = 500;
    HKEY key;

    if (!(key = reg_open_hkcu_key( "Control Panel\\Desktop" )))
        return ret;

    if (query_reg_ascii_value( key, "CursorBlinkRate", value, sizeof(value_buffer) ))
        ret = wcstoul( (WCHAR *)value->Data, NULL, 10 );
    NtClose( key );
    return ret;
}

/*****************************************************************
 *           NtUserCreateCaret  (win32u.@)
 */
BOOL WINAPI NtUserCreateCaret( HWND hwnd, HBITMAP bitmap, int width, int height )
{
    HBITMAP caret_bitmap = 0;
    int old_state = 0;
    int hidden = 0;
    HWND prev = 0;
    BOOL ret;
    RECT r;

    TRACE( "hwnd %p, bitmap %p, width %d, height %d\n", hwnd, bitmap, width, height );

    if (!hwnd) return FALSE;

    if (bitmap && bitmap != (HBITMAP)1)
    {
        BITMAP bitmap_data;

        if (!NtGdiExtGetObjectW( bitmap, sizeof(bitmap_data), &bitmap_data )) return FALSE;
        caret_bitmap = NtGdiCreateBitmap( bitmap_data.bmWidth, bitmap_data.bmHeight,
                                          bitmap_data.bmPlanes, bitmap_data.bmBitsPixel, NULL );
        if (caret_bitmap)
        {
            size_t size = bitmap_data.bmWidthBytes * bitmap_data.bmHeight;
            BYTE *bits = malloc( size );

            NtGdiGetBitmapBits( bitmap, size, bits );
            NtGdiSetBitmapBits( caret_bitmap, size, bits );
            free( bits );
        }
    }
    else
    {
        HDC dc;

        if (!width) width = get_system_metrics( SM_CXBORDER );
        if (!height) height = get_system_metrics( SM_CYBORDER );

        /* create the uniform bitmap on the fly */
        dc = NtUserGetDCEx( hwnd, 0, DCX_USESTYLE );
        if (dc)
        {
            HDC mem_dc = NtGdiCreateCompatibleDC( dc );
            if (mem_dc)
            {
                if ((caret_bitmap = NtGdiCreateCompatibleBitmap( mem_dc, width, height )))
                {
                    HBITMAP prev_bitmap = NtGdiSelectBitmap( mem_dc, caret_bitmap );
                    SetRect( &r, 0, 0, width, height );
                    fill_rect( mem_dc, &r, GetStockObject( bitmap ? GRAY_BRUSH : WHITE_BRUSH ));
                    NtGdiSelectBitmap( mem_dc, prev_bitmap );
                }
                NtGdiDeleteObjectApp( mem_dc );
            }
            NtUserReleaseDC( hwnd, dc );
        }
    }
    if (!caret_bitmap) return FALSE;

    SERVER_START_REQ( set_caret_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->width  = width;
        req->height = height;
        if ((ret = !wine_server_call_err( req )))
        {
            prev      = wine_server_ptr_handle( reply->previous );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    if (prev && !hidden)  /* hide the previous one */
    {
        /* FIXME: won't work if prev belongs to a different process */
        kill_system_timer( prev, SYSTEM_TIMER_CARET );
        if (old_state) display_caret( prev, &r );
    }

    if (caret.bitmap) NtGdiDeleteObjectApp( caret.bitmap );
    caret.bitmap = caret_bitmap;
    caret.timeout = get_caret_registry_timeout();
    return TRUE;
}

/*******************************************************************
 *              destroy_caret
 */
BOOL destroy_caret(void)
{
    int old_state = 0;
    int hidden = 0;
    HWND prev = 0;
    BOOL ret;
    RECT r;

    SERVER_START_REQ( set_caret_window )
    {
        req->handle = 0;
        req->width  = 0;
        req->height = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            prev      = wine_server_ptr_handle( reply->previous );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && prev && !hidden)
    {
        /* FIXME: won't work if prev belongs to a different process */
        kill_system_timer( prev, SYSTEM_TIMER_CARET );
        if (old_state) display_caret( prev, &r );
    }
    if (caret.bitmap) NtGdiDeleteObjectApp( caret.bitmap );
    caret.bitmap = 0;
    return ret;
}

/*****************************************************************
 *           NtUserGetCaretBlinkTime  (win32u.@)
 */
UINT WINAPI NtUserGetCaretBlinkTime(void)
{
    return caret.timeout;
}

/*******************************************************************
 *              set_caret_blink_time
 */
BOOL set_caret_blink_time( unsigned int time )
{
    TRACE( "time %u\n", time );

    caret.timeout = time;
    /* FIXME: update the timer */
    return TRUE;
}

/*****************************************************************
 *           NtUserGetCaretPos  (win32u.@)
 */
BOOL WINAPI NtUserGetCaretPos( POINT *pt )
{
    BOOL ret;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = 0;  /* don't set anything */
        req->handle = 0;
        req->x      = 0;
        req->y      = 0;
        req->hide   = 0;
        req->state  = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            pt->x = reply->old_rect.left;
            pt->y = reply->old_rect.top;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*******************************************************************
 *              set_caret_pos
 */
BOOL set_caret_pos( int x, int y )
{
    int old_state = 0;
    int hidden = 0;
    HWND hwnd = 0;
    BOOL ret;
    RECT r;

    TRACE( "(%d, %d)\n", x, y );

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_POS|SET_CARET_STATE;
        req->handle = 0;
        req->x      = x;
        req->y      = y;
        req->hide   = 0;
        req->state  = CARET_STATE_ON_IF_MOVED;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;
    if (ret && !hidden && (x != r.left || y != r.top))
    {
        if (old_state) display_caret( hwnd, &r );
        r.right += x - r.left;
        r.bottom += y - r.top;
        r.left = x;
        r.top = y;
        display_caret( hwnd, &r );
        NtUserSetSystemTimer( hwnd, SYSTEM_TIMER_CARET, caret.timeout );
    }
    return ret;
}

/*****************************************************************
 *           NtUserShowCaret  (win32u.@)
 */
BOOL WINAPI NtUserShowCaret( HWND hwnd )
{
    int hidden = 0;
    BOOL ret;
    RECT r;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_HIDE | SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = -1;
        req->state  = CARET_STATE_ON;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && hidden == 1)  /* hidden was 1 so it's now 0 */
    {
        display_caret( hwnd, &r );
        NtUserSetSystemTimer( hwnd, SYSTEM_TIMER_CARET, caret.timeout );
    }
    return ret;
}

/*****************************************************************
 *           NtUserHideCaret  (win32u.@)
 */
BOOL WINAPI NtUserHideCaret( HWND hwnd )
{
    int old_state = 0;
    int hidden = 0;
    BOOL ret;
    RECT r;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_HIDE | SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = 1;
        req->state  = CARET_STATE_OFF;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && !hidden)
    {
        if (old_state) display_caret( hwnd, &r );
        kill_system_timer( hwnd, SYSTEM_TIMER_CARET );
    }
    return ret;
}

void toggle_caret( HWND hwnd )
{
    BOOL ret;
    RECT r;
    int hidden = 0;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = 0;
        req->state  = CARET_STATE_TOGGLE;
        if ((ret = !wine_server_call( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && !hidden) display_caret( hwnd, &r );
}
