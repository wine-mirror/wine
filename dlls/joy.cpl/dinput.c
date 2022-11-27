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
#include <math.h>

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

static void draw_axis_view( HDC hdc, RECT rect, const WCHAR *name, LONG value )
{
    POINT center =
    {
        .x = (rect.left + rect.right) / 2 + 10,
        .y = (rect.top + rect.bottom) / 2,
    };
    LONG w = (rect.bottom - rect.top + 1) / 3;
    LONG x = rect.left + 20 + (w + 1) / 2 + MulDiv( value, rect.right - rect.left - 20 - w, 0xffff );
    COLORREF color;
    HFONT font;

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    color = SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
    font = SelectObject( hdc, GetStockObject( ANSI_VAR_FONT ) );
    DrawTextW( hdc, name, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
    SetTextColor( hdc, color );
    SelectObject( hdc, font );

    SetDCBrushColor( hdc, GetSysColor( COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    RoundRect( hdc, rect.left + 20, rect.top, rect.right, rect.bottom, 5, 5 );

    if (x < center.x)
    {
        MoveToEx( hdc, center.x, center.y - 3, NULL );
        LineTo( hdc, center.x, center.y + 3 );
    }

    SetDCBrushColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
    SetDCPenColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );

    Rectangle( hdc, rect.left + 20, rect.top + w, x, rect.bottom - w );

    if (x > center.x)
    {
        MoveToEx( hdc, center.x, center.y - 3, NULL );
        LineTo( hdc, center.x, center.y + 3 );
    }
}

static void draw_pov_view( HDC hdc, RECT rect, DWORD value )
{
    POINT points[] =
    {
        /* 0° */
        {.x = round( rect.left * 0.71 + rect.right * 0.29 ), .y = round( rect.top * 1.00 + rect.bottom * 0.00 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 45° */
        {.x = round( rect.left * 0.29 + rect.right * 0.71 ), .y = round( rect.top * 1.00 + rect.bottom * 0.00 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 90° */
        {.x = round( rect.left * 0.00 + rect.right * 1.00 ), .y = round( rect.top * 0.71 + rect.bottom * 0.29 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 135° */
        {.x = round( rect.left * 0.00 + rect.right * 1.00 ), .y = round( rect.top * 0.29 + rect.bottom * 0.71 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 180° */
        {.x = round( rect.left * 0.29 + rect.right * 0.71 ), .y = round( rect.top * 0.00 + rect.bottom * 1.00 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 225° */
        {.x = round( rect.left * 0.71 + rect.right * 0.29 ), .y = round( rect.top * 0.00 + rect.bottom * 1.00 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 270° */
        {.x = round( rect.left * 1.00 + rect.right * 0.00 ), .y = round( rect.top * 0.29 + rect.bottom * 0.71 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 315° */
        {.x = round( rect.left * 1.00 + rect.right * 0.00 ), .y = round( rect.top * 0.71 + rect.bottom * 0.29 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        /* 360° */
        {.x = round( rect.left * 0.71 + rect.right * 0.29 ), .y = round( rect.top * 1.00 + rect.bottom * 0.00 )},
        {.x = round( rect.left * 0.50 + rect.right * 0.50 ), .y = round( rect.top * 0.50 + rect.bottom * 0.50 )},
        {.x = round( rect.left * 0.71 + rect.right * 0.29 ), .y = round( rect.top * 1.00 + rect.bottom * 0.00 )},
    };
    DWORD i;

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    SetDCBrushColor( hdc, GetSysColor( COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    for (i = 0; i < ARRAY_SIZE(points) - 1; i += 2)
    {
        MoveToEx( hdc, (points[i].x + points[i + 1].x) / 2, (points[i].y + points[i + 1].y) / 2, NULL );
        LineTo( hdc, points[i].x, points[i].y );
    }

    SetDCPenColor( hdc, GetSysColor( (value != -1) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWFRAME ) );
    SetDCBrushColor( hdc, GetSysColor( (value != -1) ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    if (value != -1) Polygon( hdc, points + value / 4500 * 2, 3 );
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

LRESULT CALLBACK test_di_axes_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        RECT rect, tmp_rect;
        PAINTSTRUCT paint;
        DIJOYSTATE state;
        DIDEVCAPS caps;
        HDC hdc;

        get_device_state( &state, &caps );

        hdc = BeginPaint( hwnd, &paint );

        GetClientRect( hwnd, &rect );
        rect.bottom = rect.top + (rect.bottom - rect.top - 2) / 4 - 2;
        rect.right = rect.left + (rect.right - rect.left) / 2 - 10;

        OffsetRect( &rect, 5, 2 );
        draw_axis_view( hdc, rect, L"X", state.lX );

        tmp_rect = rect;
        OffsetRect( &rect, rect.right - rect.left + 10, 0 );
        draw_axis_view( hdc, rect, L"Rx", state.lRx );
        rect = tmp_rect;

        OffsetRect( &rect, 0, rect.bottom - rect.top + 2 );
        draw_axis_view( hdc, rect, L"Y", state.lY );

        tmp_rect = rect;
        OffsetRect( &rect, rect.right - rect.left + 10, 0 );
        draw_axis_view( hdc, rect, L"Ry", state.lRy );
        rect = tmp_rect;

        OffsetRect( &rect, 0, rect.bottom - rect.top + 2 );
        draw_axis_view( hdc, rect, L"Z", state.lZ );

        tmp_rect = rect;
        OffsetRect( &rect, rect.right - rect.left + 10, 0 );
        draw_axis_view( hdc, rect, L"Rz", state.lRz );
        rect = tmp_rect;

        OffsetRect( &rect, 0, rect.bottom - rect.top + 2 );
        draw_axis_view( hdc, rect, L"S", state.rglSlider[0] );

        tmp_rect = rect;
        OffsetRect( &rect, rect.right - rect.left + 10, 0 );
        draw_axis_view( hdc, rect, L"Rs", state.rglSlider[1] );
        rect = tmp_rect;

        EndPaint( hwnd, &paint );

        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

LRESULT CALLBACK test_di_povs_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        PAINTSTRUCT paint;
        DIJOYSTATE state;
        DIDEVCAPS caps;
        RECT rect;
        HDC hdc;

        get_device_state( &state, &caps );

        hdc = BeginPaint( hwnd, &paint );

        GetClientRect( hwnd, &rect );
        rect.bottom = rect.top + (rect.bottom - rect.top - 5) / 2 - 5;
        rect.right = rect.left + (rect.bottom - rect.top);

        OffsetRect( &rect, 5, 5 );
        draw_pov_view( hdc, rect, state.rgdwPOV[0] );
        OffsetRect( &rect, rect.right - rect.left + 5, 0 );
        draw_pov_view( hdc, rect, state.rgdwPOV[1] );
        OffsetRect( &rect, rect.left - rect.right - 5, rect.bottom - rect.top + 5 );
        draw_pov_view( hdc, rect, state.rgdwPOV[1] );
        OffsetRect( &rect, rect.right - rect.left + 5, 0 );
        draw_pov_view( hdc, rect, state.rgdwPOV[2] );

        EndPaint( hwnd, &paint );

        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
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
