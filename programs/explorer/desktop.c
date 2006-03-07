/*
 * Explorer desktop support
 *
 * Copyright 2006 Alexandre Julliard
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

#include <windows.h>
#include <wine/debug.h>
#include "explorer_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

#define DESKTOP_CLASS_ATOM MAKEINTATOMW(32769)

static LRESULT WINAPI desktop_wnd_proc( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
    WINE_TRACE( "got msg %x wp %x lp %lx\n", message, wp, lp );

    switch(message)
    {
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_CLOSE) ExitWindows( 0, 0 );
        break;

    case WM_SETCURSOR:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_ARROW ) );

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_ERASEBKGND:
        PaintDesktop( (HDC)wp );
        return TRUE;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint( hwnd, &ps );
            if (ps.fErase) PaintDesktop( ps.hdc );
            EndPaint( hwnd, &ps );
        }
        return 0;
    }
    return 0;
}

void manage_desktop(void)
{
    MSG msg;
    HWND hwnd = CreateWindowExW( 0, DESKTOP_CLASS_ATOM, NULL,
                                 WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                                 0, 0, 0, NULL );
    if (hwnd != GetDesktopWindow()) return;  /* some other process beat us to it */
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)desktop_wnd_proc );

    WINE_TRACE( "explorer starting on hwnd %p\n", hwnd );

    initialize_systray();
    while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );

    WINE_TRACE( "explorer exiting for hwnd %p\n", hwnd );
}
