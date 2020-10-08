/*
 * Copyright 2001 Eric Pouech
 * Copyright 2020 Jacek Caban for CodeWeavers
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

#define NONAMELESSUNION
#include <stdlib.h>

#include "conhost.h"

#include <commctrl.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(conhost);

struct console_window
{
    unsigned int      ui_charset;      /* default UI charset */
};

static LRESULT window_create( HWND hwnd, const CREATESTRUCTW *create )
{
    struct console *console = create->lpCreateParams;

    TRACE( "%p\n", hwnd );

    SetWindowLongPtrW( hwnd, 0, (DWORD_PTR)console );
    console->win = hwnd;

    return 0;
}

static LRESULT WINAPI window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct console *console = (struct console *)GetWindowLongPtrW( hwnd, 0 );

    switch (msg)
    {
    case WM_CREATE:
        return window_create( hwnd, (const CREATESTRUCTW *)lparam );

    case WM_DESTROY:
        console->win = NULL;
        PostQuitMessage( 0 );
        break;

    default:
        return DefWindowProcW( hwnd, msg, wparam, lparam );
    }

    return 0;
}

BOOL init_window( struct console *console )
{
    WNDCLASSW wndclass;
    CHARSETINFO ci;

    static struct console_window console_window;

    console->window = &console_window;
    if (!TranslateCharsetInfo( (DWORD *)(INT_PTR)GetACP(), &ci, TCI_SRCCODEPAGE ))
        return FALSE;

    console->window->ui_charset = ci.ciCharset;

    wndclass.style         = CS_DBLCLKS;
    wndclass.lpfnWndProc   = window_proc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD_PTR);
    wndclass.hInstance     = GetModuleHandleW(NULL);
    wndclass.hIcon         = LoadIconW( 0, (const WCHAR *)IDI_WINLOGO );
    wndclass.hCursor       = LoadCursorW( 0, (const WCHAR *)IDC_ARROW );
    wndclass.hbrBackground = GetStockObject( BLACK_BRUSH );
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = L"WineConsoleClass";
    RegisterClassW(&wndclass);

    if (!CreateWindowW( wndclass.lpszClassName, NULL,
                        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|
                        WS_MAXIMIZEBOX|WS_HSCROLL|WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT,
                        0, 0, 0, 0, wndclass.hInstance, console ))
        return FALSE;

    return TRUE;
}
