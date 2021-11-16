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

#if 0
#pragma makedep unix
#endif

#include "win32u_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(keyboard);


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
