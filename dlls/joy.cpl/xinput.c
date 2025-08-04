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
#include <stdlib.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "xinput.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

struct device_state
{
    XINPUT_CAPABILITIES caps;
    XINPUT_STATE state;
    DWORD status;
    BOOL rumble;
};

static CRITICAL_SECTION state_cs;
static CRITICAL_SECTION_DEBUG state_cs_debug =
{
    0, 0, &state_cs,
    { &state_cs_debug.ProcessLocksList, &state_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": state_cs") }
};
static CRITICAL_SECTION state_cs = { &state_cs_debug, -1, 0, 0, 0, 0 };

static struct device_state devices_state[XUSER_MAX_COUNT] =
{
    {.status = ERROR_DEVICE_NOT_CONNECTED},
    {.status = ERROR_DEVICE_NOT_CONNECTED},
    {.status = ERROR_DEVICE_NOT_CONNECTED},
    {.status = ERROR_DEVICE_NOT_CONNECTED},
};
static HWND dialog_hwnd;

static void set_device_state( DWORD index, struct device_state *state )
{
    BOOL modified;

    EnterCriticalSection( &state_cs );
    state->rumble = devices_state[index].rumble;
    modified = memcmp( devices_state + index, state, sizeof(*state) );
    devices_state[index] = *state;
    LeaveCriticalSection( &state_cs );

    if (modified) SendMessageW( dialog_hwnd, WM_USER, index, 0 );
}

static void get_device_state( DWORD index, struct device_state *state )
{
    EnterCriticalSection( &state_cs );
    *state = devices_state[index];
    LeaveCriticalSection( &state_cs );
}

static DWORD WINAPI input_thread_proc( void *param )
{
    HANDLE thread_stop = param;
    DWORD i;

    while (WaitForSingleObject( thread_stop, 20 ) == WAIT_TIMEOUT)
    {
        for (i = 0; i < ARRAY_SIZE(devices_state); ++i)
        {
            XINPUT_VIBRATION vibration = {0};
            struct device_state state = {0};

            state.status = XInputGetCapabilities( i, 0, &state.caps );
            if (!state.status) state.status = XInputGetStateEx( i, &state.state );
            set_device_state( i, &state );

            if (state.rumble)
            {
                vibration.wLeftMotorSpeed = 2 * max( abs( state.state.Gamepad.sThumbLX ),
                                                     abs( state.state.Gamepad.sThumbLY ) ) - 1;
                vibration.wRightMotorSpeed = 2 * max( abs( state.state.Gamepad.sThumbRX ),
                                                      abs( state.state.Gamepad.sThumbRY ) ) - 1;
            }

            XInputSetState( i, &vibration );
        }
    }

    return 0;
}

static void draw_axis_view( HDC hdc, RECT rect, SHORT dx, SHORT dy, BOOL set )
{
    POINT center =
    {
        .x = (rect.left + rect.right) / 2,
        .y = (rect.top + rect.bottom) / 2,
    };
    POINT pos =
    {
        .x = center.x + MulDiv( dx, rect.right - rect.left - 20, 0xffff ),
        .y = center.y - MulDiv( dy, rect.bottom - rect.top - 20, 0xffff ),
    };

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    SetDCBrushColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    Ellipse( hdc, rect.left, rect.top, rect.right, rect.bottom );

    MoveToEx( hdc, center.x, center.y - 3, NULL );
    LineTo( hdc, center.x, center.y + 4 );
    MoveToEx( hdc, center.x - 3, center.y, NULL );
    LineTo( hdc, center.x + 4, center.y );

    if (!set) SetDCPenColor( hdc, GetSysColor( (dx || dy) ? COLOR_HIGHLIGHT : COLOR_WINDOWFRAME ) );

    MoveToEx( hdc, center.x, center.y, NULL );
    LineTo( hdc, pos.x, pos.y );

    Ellipse( hdc, pos.x - 4, pos.y - 4, pos.x + 4, pos.y + 4 );
}

static void draw_trigger_view( HDC hdc, RECT rect, BYTE dt )
{
    POINT center =
    {
        .x = (rect.left + rect.right) / 2,
        .y = (rect.top + rect.bottom) / 2,
    };
    LONG w = (rect.right - rect.left + 1) / 3;
    LONG y = rect.bottom - (w + 1) / 2 - MulDiv( dt, rect.bottom - rect.top - w, 0xff );

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    SetDCBrushColor( hdc, GetSysColor( COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    RoundRect( hdc, rect.left, rect.top, rect.right, rect.bottom, 5, 5 );

    if (y > center.y)
    {
        MoveToEx( hdc, center.x - 3, center.y, NULL );
        LineTo( hdc, center.x + 3, center.y );
    }

    SetDCBrushColor( hdc, GetSysColor( dt ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( dt ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWFRAME ) );

    Rectangle( hdc, center.x - w, y, center.x + w, rect.bottom );

    if (y < center.y)
    {
        MoveToEx( hdc, center.x - 3, center.y, NULL );
        LineTo( hdc, center.x + 3, center.y );
    }
}

static void draw_button_view( HDC hdc, RECT rect, BOOL set, const WCHAR *name )
{
    COLORREF color = SetTextColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT ) );
    HFONT font = SelectObject( hdc, GetStockObject( ANSI_VAR_FONT ) );
    INT mode = SetBkMode( hdc, TRANSPARENT );

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    SetDCBrushColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    Ellipse( hdc, rect.left, rect.top, rect.right, rect.bottom );
    if (name[0] >= 'A' && name[0] <= 'Z')
        DrawTextW( hdc, name, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
    else if (name[0] == '=')
    {
        RECT tmp_rect = {.right = 10, .bottom = 2};

        OffsetRect( &tmp_rect, rect.left, rect.top );
        OffsetRect( &tmp_rect, (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 );
        OffsetRect( &tmp_rect, (tmp_rect.left - tmp_rect.right) / 2, (tmp_rect.top - tmp_rect.bottom) / 2 );

        FillRect( hdc, &tmp_rect, (HBRUSH)((UINT_PTR)(set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) + 1) );
        OffsetRect( &tmp_rect, 0, 3 * (tmp_rect.top - tmp_rect.bottom) / 2 );
        FillRect( hdc, &tmp_rect, (HBRUSH)((UINT_PTR)(set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) + 1) );
        OffsetRect( &tmp_rect, 0, 6 * (tmp_rect.bottom - tmp_rect.top) / 2 );
        FillRect( hdc, &tmp_rect, (HBRUSH)((UINT_PTR)(set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) + 1) );
    }
    else
    {
        LOGFONTW logfont =
        {
            .lfHeight = 16,
            .lfWeight = FW_NORMAL,
            .lfCharSet = SYMBOL_CHARSET,
            .lfFaceName = L"Marlett",
        };
        WCHAR buffer[4] = {0};
        HFONT font;

        font = CreateFontIndirectW( &logfont );
        font = (HFONT)SelectObject( hdc, font );

        if (name[0] == '#') { buffer[0] = 0x32; OffsetRect( &rect, 1, 0 ); }
        if (name[0] == '<') buffer[0] = 0x33;
        if (name[0] == '>') buffer[0] = 0x34;
        if (name[0] == '^') buffer[0] = 0x35;
        if (name[0] == 'v') buffer[0] = 0x36;
        if (name[0] == '@') buffer[0] = 0x6e;
        DrawTextW( hdc, buffer, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );

        font = (HFONT)SelectObject( hdc, font );
        DeleteObject( font );
    }

    SetBkMode( hdc, mode );
    SetTextColor( hdc, color );
    SelectObject( hdc, font );
}

LRESULT CALLBACK test_xi_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        DWORD index = GetWindowLongW( hwnd, GWLP_USERDATA );
        UINT axis_size, trigger_size, button_size, horiz_space;
        struct device_state state;
        RECT rect, tmp_rect;
        PAINTSTRUCT paint;
        HDC hdc;

        GetClientRect( hwnd, &rect );
        axis_size = rect.bottom - rect.top;
        button_size = (axis_size - 1) / 3;
        trigger_size = axis_size / 4;
        horiz_space = (rect.right - rect.left - axis_size * 2 - trigger_size * 2 - button_size * 5) / 10;

        get_device_state( index, &state );

        hdc = BeginPaint( hwnd, &paint );

        rect.right = rect.left + axis_size;
        OffsetRect( &rect, horiz_space, 0 );
        draw_axis_view( hdc, rect, state.state.Gamepad.sThumbLX, state.state.Gamepad.sThumbLY,
                        state.state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_axis_view( hdc, rect, state.state.Gamepad.sThumbRX, state.state.Gamepad.sThumbRY,
                        state.state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB );

        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        rect.right = rect.left + trigger_size;
        draw_trigger_view( hdc, rect, state.state.Gamepad.bLeftTrigger );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_trigger_view( hdc, rect, state.state.Gamepad.bRightTrigger );

        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        rect.right = rect.left + button_size;
        rect.bottom = rect.top + button_size;
        tmp_rect = rect;
        OffsetRect( &rect, (rect.right - rect.left + horiz_space) / 2, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP, L"^" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER, L"L" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER, L"R" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_Y, L"Y" );

        rect = tmp_rect;
        OffsetRect( &rect, 0, rect.bottom - rect.top );
        tmp_rect = rect;
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT, L"<" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT, L">" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & 0x0400, L"@" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_X, L"X" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_B, L"B" );

        rect = tmp_rect;
        OffsetRect( &rect, (rect.right - rect.left + horiz_space) / 2, rect.bottom - rect.top );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN, L"v" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK, L"#" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_START, L"=" );
        OffsetRect( &rect, rect.right - rect.left + horiz_space, 0 );
        draw_button_view( hdc, rect, state.state.Gamepad.wButtons & XINPUT_GAMEPAD_A, L"A" );

        EndPaint( hwnd, &paint );

        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void create_user_view( HWND hwnd, DWORD index )
{
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    HWND parent, view;
    LONG margin;
    RECT rect;

    parent = GetDlgItem( hwnd, IDC_XI_USER_0 + index );

    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 15 / 100;
    InflateRect( &rect, -margin, -margin );

    view = CreateWindowW( L"JoyCplXInput", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                          rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );
    SetWindowLongW( view, GWLP_USERDATA, index );

    ShowWindow( parent, SW_HIDE );

    parent = GetDlgItem( hwnd, IDC_XI_RUMBLE_0 + index );
    ShowWindow( parent, SW_HIDE );
}

static void update_user_view( HWND hwnd, DWORD index )
{
    struct device_state state;
    HWND parent;

    get_device_state( index, &state );

    parent = GetDlgItem( hwnd, IDC_XI_NO_USER_0 + index );
    ShowWindow( parent, state.status ? SW_SHOW : SW_HIDE );

    parent = GetDlgItem( hwnd, IDC_XI_RUMBLE_0 + index );
    ShowWindow( parent, state.status ? SW_HIDE : SW_SHOW );

    parent = GetDlgItem( hwnd, IDC_XI_USER_0 + index );
    ShowWindow( parent, state.status ? SW_HIDE : SW_SHOW );

    if (!state.status)
    {
        HWND view = FindWindowExW( parent, NULL, L"JoyCplXInput", NULL );
        InvalidateRect( view, NULL, TRUE );
    }
}

static void update_rumble_state( HWND hwnd, DWORD index )
{
    HWND parent;
    LRESULT res;

    parent = GetDlgItem( hwnd, IDC_XI_RUMBLE_0 + index );
    res = SendMessageW( parent, BM_GETCHECK, 0, 0 );

    EnterCriticalSection( &state_cs );
    devices_state[index].rumble = res == BST_CHECKED;
    LeaveCriticalSection( &state_cs );
}

extern INT_PTR CALLBACK test_xi_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static HANDLE thread, thread_stop;

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    switch (msg)
    {
    case WM_INITDIALOG:
        create_user_view( hwnd, 0 );
        create_user_view( hwnd, 1 );
        create_user_view( hwnd, 2 );
        create_user_view( hwnd, 3 );
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_XI_RUMBLE_0:
        case IDC_XI_RUMBLE_1:
        case IDC_XI_RUMBLE_2:
        case IDC_XI_RUMBLE_3:
            update_rumble_state( hwnd, LOWORD(wparam) - IDC_XI_RUMBLE_0 );
            break;
        }
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR *)lparam)->code)
        {
        case PSN_SETACTIVE:
            dialog_hwnd = hwnd;
            thread_stop = CreateEventW( NULL, FALSE, FALSE, NULL );
            thread = CreateThread( NULL, 0, input_thread_proc, (void *)thread_stop, 0, NULL );
            break;

        case PSN_RESET:
        case PSN_KILLACTIVE:
            SetEvent( thread_stop );
            /* wait for the input thread to stop, processing any WM_USER message from it */
            while (MsgWaitForMultipleObjects( 1, &thread, FALSE, INFINITE, QS_ALLINPUT ) == 1)
            {
                MSG msg;
                while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
                {
                    TranslateMessage( &msg );
                    DispatchMessageW( &msg );
                }
            }
            CloseHandle( thread_stop );
            CloseHandle( thread );
            dialog_hwnd = 0;
            break;
        }
        return TRUE;

    case WM_USER:
        update_user_view( hwnd, wparam );
        return TRUE;
    }

    return FALSE;
}
