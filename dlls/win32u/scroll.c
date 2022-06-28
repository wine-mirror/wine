/*
 * Scrollbar control
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994, 1996 Alexandre Julliard
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scroll);


static BOOL show_scroll_bar( HWND hwnd, int bar, BOOL show_horz, BOOL show_vert )
{
    ULONG old_style, set_bits = 0, clear_bits = 0;

    TRACE( "hwnd=%p bar=%d horz=%d, vert=%d\n", hwnd, bar, show_horz, show_vert );

    switch (bar)
    {
    case SB_CTL:
        NtUserShowWindow( hwnd, show_horz ? SW_SHOW : SW_HIDE );
        return TRUE;

    case SB_BOTH:
    case SB_HORZ:
        if (show_horz) set_bits |= WS_HSCROLL;
        else clear_bits |= WS_HSCROLL;
        if (bar == SB_HORZ) break;
        /* fall through */
    case SB_VERT:
        if (show_vert) set_bits |= WS_VSCROLL;
        else clear_bits |= WS_VSCROLL;
        break;

    default:
        return FALSE;  /* Nothing to do! */
    }

    old_style = set_window_style( hwnd, set_bits, clear_bits );
    if ((old_style & clear_bits) != 0 || (old_style & set_bits) != set_bits)
    {
        /* frame has been changed, let the window redraw itself */
        NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0,
                            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
        return TRUE;
    }
    return FALSE; /* no frame changes */
}

/*************************************************************************
 *           NtUserShowScrollBar   (win32u.@)
 */
BOOL WINAPI NtUserShowScrollBar( HWND hwnd, INT bar, BOOL show )
{
    if (!hwnd) return FALSE;

    show_scroll_bar( hwnd, bar, bar == SB_VERT ? 0 : show, bar == SB_HORZ ? 0 : show );
    return TRUE;
}
