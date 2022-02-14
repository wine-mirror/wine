/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Martin Von Loewis
 * Copyright 1997 Alex Korobka
 * Copyright 1998 Turchanov Sergey
 * Copyright 2007 Henri Verbeet
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2016 Dmitry Timoshkov
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

WINE_DEFAULT_DEBUG_CHANNEL(cursor);


/***********************************************************************
 *	     NtUserShowCursor    (win32u.@)
 */
INT WINAPI NtUserShowCursor( BOOL show )
{
    HCURSOR cursor;
    int increment = show ? 1 : -1;
    int count;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_COUNT;
        req->show_count = increment;
        wine_server_call( req );
        cursor = wine_server_ptr_handle( reply->prev_handle );
        count = reply->prev_count + increment;
    }
    SERVER_END_REQ;

    TRACE("%d, count=%d\n", show, count );

    if (show && !count) user_driver->pSetCursor( cursor );
    else if (!show && count == -1) user_driver->pSetCursor( 0 );

    return count;
}


/***********************************************************************
 *	     NtUserGetCursor (win32u.@)
 */
HCURSOR WINAPI NtUserGetCursor(void)
{
    HCURSOR ret;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = 0;
        wine_server_call( req );
        ret = wine_server_ptr_handle( reply->prev_handle );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *	     NtUserClipCursor (win32u.@)
 */
BOOL WINAPI NtUserClipCursor( const RECT *rect )
{
    UINT dpi;
    BOOL ret;
    RECT new_rect;

    TRACE( "Clipping to %s\n", wine_dbgstr_rect(rect) );

    if (rect)
    {
        if (rect->left > rect->right || rect->top > rect->bottom) return FALSE;
        if ((dpi = get_thread_dpi()))
        {
            HMONITOR monitor = monitor_from_rect( rect, MONITOR_DEFAULTTOPRIMARY, dpi );
            new_rect = map_dpi_rect( *rect, dpi, get_monitor_dpi( monitor ));
            rect = &new_rect;
        }
    }

    SERVER_START_REQ( set_cursor )
    {
        req->clip_msg = WM_WINE_CLIPCURSOR;
        if (rect)
        {
            req->flags       = SET_CURSOR_CLIP;
            req->clip.left   = rect->left;
            req->clip.top    = rect->top;
            req->clip.right  = rect->right;
            req->clip.bottom = rect->bottom;
        }
        else req->flags = SET_CURSOR_NOCLIP;

        if ((ret = !wine_server_call( req )))
        {
            new_rect.left   = reply->new_clip.left;
            new_rect.top    = reply->new_clip.top;
            new_rect.right  = reply->new_clip.right;
            new_rect.bottom = reply->new_clip.bottom;
        }
    }
    SERVER_END_REQ;
    if (ret) user_driver->pClipCursor( &new_rect );
    return ret;
}

BOOL get_clip_cursor( RECT *rect )
{
    UINT dpi;
    BOOL ret;

    if (!rect) return FALSE;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = 0;
        if ((ret = !wine_server_call( req )))
        {
            rect->left   = reply->new_clip.left;
            rect->top    = reply->new_clip.top;
            rect->right  = reply->new_clip.right;
            rect->bottom = reply->new_clip.bottom;
        }
    }
    SERVER_END_REQ;

    if (ret && (dpi = get_thread_dpi()))
    {
        HMONITOR monitor = monitor_from_rect( rect, MONITOR_DEFAULTTOPRIMARY, 0 );
        *rect = map_dpi_rect( *rect, get_monitor_dpi( monitor ), dpi );
    }
    return ret;
}
