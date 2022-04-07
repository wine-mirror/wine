/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 * Copyright 1995 Alex Korobka
 * Copyright 2022 Jacek Caban for CodeWeavers
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
#include "ntuser_private.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


static BOOL set_window_text( HWND hwnd, const void *text, BOOL ansi )
{
    static const WCHAR emptyW[] = { 0 };
    WCHAR *str;
    WND *win;

    /* check for string, as static icons, bitmaps (SS_ICON, SS_BITMAP)
     * may have child window IDs instead of window name */
    if (text && IS_INTRESOURCE(text)) return FALSE;

    if (text)
    {
        if (ansi) str = towstr( text );
        else str = wcsdup( text );
        if (!str) return FALSE;
    }
    else str = NULL;

    TRACE( "%s\n", debugstr_w(str) );

    if (!(win = get_win_ptr( hwnd )))
    {
        free( str );
        return FALSE;
    }

    free( win->text );
    win->text = str;
    SERVER_START_REQ( set_window_text )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (str) wine_server_add_data( req, str, lstrlenW( str ) * sizeof(WCHAR) );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    release_win_ptr( win );

    user_driver->pSetWindowText( hwnd, str ? str : emptyW );

    return TRUE;
}

static HICON set_window_icon( HWND hwnd, WPARAM type, HICON icon )
{
    HICON ret = 0;
    WND *win;

    if (!(win = get_win_ptr( hwnd ))) return 0;

    switch (type)
    {
    case ICON_SMALL:
        ret = win->hIconSmall;
        if (ret && !icon && win->hIcon && user_callbacks)
        {
            win->hIconSmall2 = user_callbacks->pCopyImage( win->hIcon, IMAGE_ICON,
                                                           get_system_metrics( SM_CXSMICON ),
                                                           get_system_metrics( SM_CYSMICON ), 0 );
        }
        else if (icon && win->hIconSmall2)
        {
            NtUserDestroyCursor( win->hIconSmall2, 0 );
            win->hIconSmall2 = NULL;
        }
        win->hIconSmall = icon;
        break;

    case ICON_BIG:
        ret = win->hIcon;
        if (win->hIconSmall2)
        {
            NtUserDestroyCursor( win->hIconSmall2, 0 );
            win->hIconSmall2 = NULL;
        }
        if (icon && !win->hIconSmall && user_callbacks)
        {
            win->hIconSmall2 = user_callbacks->pCopyImage( icon, IMAGE_ICON,
                                                           get_system_metrics( SM_CXSMICON ),
                                                           get_system_metrics( SM_CYSMICON ), 0 );
        }
        win->hIcon = icon;
        break;
    }
    release_win_ptr( win );

    user_driver->pSetWindowIcon( hwnd, type, icon );
    return ret;
}

static LRESULT handle_sys_command( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    if (!is_window_enabled( hwnd )) return 0;

    if (call_hooks( WH_CBT, HCBT_SYSCOMMAND, wparam, lparam, TRUE ))
        return 0;

    if (!user_driver->pSysCommand( hwnd, wparam, lparam ))
        return 0;

    return 1;
}

LRESULT default_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_NCCREATE:
        if (lparam)
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
            set_window_text( hwnd, cs->lpszName, ansi );
            result = 1;
        }
        break;

    case WM_NCDESTROY:
        {
            WND *win = get_win_ptr( hwnd );
            if (!win) return 0;
            free( win->text );
            win->text = NULL;
            if (user_callbacks) user_callbacks->free_win_ptr( win );
            win->pScroll = NULL;
            release_win_ptr( win );
            return 0;
        }

    case WM_SETTEXT:
        result = set_window_text( hwnd, (void *)lparam, ansi );
        break;

    case WM_SETICON:
        result = (LRESULT)set_window_icon( hwnd, wparam, (HICON)lparam );
        break;

    case WM_SYSCOMMAND:
        result = handle_sys_command( hwnd, wparam, lparam );
        break;
    }

    return result;
}
