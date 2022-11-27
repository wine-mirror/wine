/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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
 *
 */

#include <stdarg.h>
#include <stddef.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "dinput.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

static CRITICAL_SECTION state_cs;
static CRITICAL_SECTION_DEBUG state_cs_debug =
{
    0, 0, &state_cs,
    { &state_cs_debug.ProcessLocksList, &state_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": state_cs") }
};
static CRITICAL_SECTION state_cs = { &state_cs_debug, -1, 0, 0, 0, 0 };

static DIJOYSTATE last_state;
static DIDEVCAPS last_caps;

void set_di_device_state( HWND hwnd, DIJOYSTATE *state, DIDEVCAPS *caps )
{
    BOOL modified;

    EnterCriticalSection( &state_cs );
    modified = memcmp( &last_state, state, sizeof(*state) ) ||
               memcmp( &last_caps, caps, sizeof(*caps) );
    last_state = *state;
    last_caps = *caps;
    LeaveCriticalSection( &state_cs );

    if (modified) SendMessageW( hwnd, WM_USER, 0, 0 );
}

static void get_device_state( DIJOYSTATE *state, DIDEVCAPS *caps )
{
    EnterCriticalSection( &state_cs );
    *state = last_state;
    *caps = last_caps;
    LeaveCriticalSection( &state_cs );
}

static inline void draw_button_view( HDC hdc, RECT rect, BOOL set, const WCHAR *name )
{
    COLORREF color;
    HFONT font;
    INT mode;

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    SetDCBrushColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    Ellipse( hdc, rect.left, rect.top, rect.right, rect.bottom );

    color = SetTextColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT ) );
    font = SelectObject( hdc, GetStockObject( ANSI_VAR_FONT ) );
    mode = SetBkMode( hdc, TRANSPARENT );
    DrawTextW( hdc, name, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
    SetBkMode( hdc, mode );
    SetTextColor( hdc, color );
    SelectObject( hdc, font );
}

LRESULT CALLBACK test_di_buttons_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        UINT i, j, offs, size, step, space = 2;
        PAINTSTRUCT paint;
        DIJOYSTATE state;
        DIDEVCAPS caps;
        RECT rect;
        HDC hdc;

        get_device_state( &state, &caps );

        if (caps.dwButtons <= 48) step = 16;
        else step = 32;

        hdc = BeginPaint( hwnd, &paint );

        GetClientRect( hwnd, &rect );

        size = (rect.right - rect.left - space) / step;
        offs = (rect.right - rect.left - step * size - space) / 2;
        OffsetRect( &rect, offs, offs );
        rect.right = rect.left + size - space;
        rect.bottom = rect.top + size - space;

        for (i = 0; i < ARRAY_SIZE(state.rgbButtons) && i < caps.dwButtons;)
        {
            RECT first = rect;

            for (j = 0; j < step && i < caps.dwButtons; j++, i++)
            {
                WCHAR buffer[3];
                swprintf( buffer, ARRAY_SIZE(buffer), L"%d", i );
                draw_button_view( hdc, rect, state.rgbButtons[i], buffer );
                OffsetRect( &rect, size, 0 );
            }

            rect = first;
            OffsetRect( &rect, 0, size );
        }

        EndPaint( hwnd, &paint );

        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}
