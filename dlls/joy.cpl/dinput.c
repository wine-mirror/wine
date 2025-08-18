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
#include "dbt.h"
#include "ddk/hidclass.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "joy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(joycpl);

struct effect
{
    struct list entry;
    IDirectInputEffect *effect;
};

struct device
{
    struct list entry;
    IDirectInputDevice8W *device;
};

static CRITICAL_SECTION state_cs;
static CRITICAL_SECTION_DEBUG state_cs_debug =
{
    0, 0, &state_cs,
    { &state_cs_debug.ProcessLocksList, &state_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": state_cs") }
};
static CRITICAL_SECTION state_cs = { &state_cs_debug, -1, 0, 0, 0, 0 };

static struct list effects = LIST_INIT( effects );
static IDirectInputEffect *effect_selected;

static struct list devices = LIST_INIT( devices );
static IDirectInputDevice8W *device_selected;

static HWND dialog_hwnd;
static HANDLE state_event;
static HDEVNOTIFY devnotify;

static BOOL CALLBACK enum_effects( const DIEFFECTINFOW *info, void *context )
{
    IDirectInputDevice8W *device = context;
    DWORD axes[2] = {DIJOFS_X, DIJOFS_Y};
    LONG direction[2] = {0};
    DIEFFECT params =
    {
        .dwSize = sizeof(DIEFFECT),
        .dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS,
        .dwDuration = INFINITE,
        .dwGain = DI_FFNOMINALMAX,
        .rglDirection = direction,
        .rgdwAxes = axes,
        .cAxes = 2,
    };
    DICONSTANTFORCE constant =
    {
        .lMagnitude = DI_FFNOMINALMAX,
    };
    DIPERIODIC periodic =
    {
        .dwMagnitude = DI_FFNOMINALMAX,
        .dwPeriod = DI_SECONDS / 2,
    };
    DICONDITION condition =
    {
        .dwPositiveSaturation = 10000,
        .dwNegativeSaturation = 10000,
        .lPositiveCoefficient = 10000,
        .lNegativeCoefficient = 10000,
    };
    DIRAMPFORCE ramp =
    {
        .lEnd = DI_FFNOMINALMAX,
    };
    IDirectInputEffect *effect;
    struct effect *entry;
    HRESULT hr;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return DIENUM_STOP;

    if (IsEqualGUID( &info->guid, &GUID_RampForce ))
    {
        params.cbTypeSpecificParams = sizeof(ramp);
        params.dwDuration = 2 * DI_SECONDS;
        params.lpvTypeSpecificParams = &ramp;
    }
    else if (IsEqualGUID( &info->guid, &GUID_ConstantForce ))
    {
        params.cbTypeSpecificParams = sizeof(constant);
        params.lpvTypeSpecificParams = &constant;
    }
    else if (IsEqualGUID( &info->guid, &GUID_Sine ) ||
             IsEqualGUID( &info->guid, &GUID_Square ) ||
             IsEqualGUID( &info->guid, &GUID_Triangle ) ||
             IsEqualGUID( &info->guid, &GUID_SawtoothUp ) ||
             IsEqualGUID( &info->guid, &GUID_SawtoothDown ))
    {
        params.cbTypeSpecificParams = sizeof(periodic);
        params.lpvTypeSpecificParams = &periodic;
    }
    else if (IsEqualGUID( &info->guid, &GUID_Spring ) ||
             IsEqualGUID( &info->guid, &GUID_Damper ) ||
             IsEqualGUID( &info->guid, &GUID_Inertia ) ||
             IsEqualGUID( &info->guid, &GUID_Friction ))
    {
        params.cbTypeSpecificParams = sizeof(condition);
        params.lpvTypeSpecificParams = &condition;
    }

    do hr = IDirectInputDevice2_CreateEffect( device, &info->guid, &params, &effect, NULL );
    while (FAILED(hr) && --params.cAxes);

    if (FAILED(hr))
    {
        FIXME( "Failed to create effect with type %s, hr %#lx\n", debugstr_guid( &info->guid ), hr );
        free( entry );
        return DIENUM_CONTINUE;
    }

    entry->effect = effect;
    list_add_tail( &effects, &entry->entry );

    return DIENUM_CONTINUE;
}

static void set_selected_effect( IDirectInputEffect *effect )
{
    IDirectInputEffect *previous;

    EnterCriticalSection( &state_cs );
    if ((previous = effect_selected)) IDirectInputEffect_Release( previous );
    if ((effect_selected = effect)) IDirectInputEffect_AddRef( effect );
    LeaveCriticalSection( &state_cs );
}

static IDirectInputEffect *get_selected_effect( DIJOYSTATE2 *state )
{
    IDirectInputDevice8W *device;
    IDirectInputEffect *effect;

    EnterCriticalSection( &state_cs );
    if (!(device = device_selected)) effect = NULL;
    else if (FAILED(IDirectInputDevice8_GetDeviceState( device, sizeof(*state), state ))) effect = NULL;
    else if ((effect = effect_selected)) IDirectInputEffect_AddRef( effect );
    LeaveCriticalSection( &state_cs );

    return effect;
}

static void clear_effects(void)
{
    struct effect *effect, *next;

    set_selected_effect( NULL );

    LIST_FOR_EACH_ENTRY_SAFE( effect, next, &effects, struct effect, entry )
    {
        list_remove( &effect->entry );
        IDirectInputEffect_Release( effect->effect );
        free( effect );
    }
}

static void set_selected_device( IDirectInputDevice8W *device )
{
    IDirectInputDevice8W *previous;

    EnterCriticalSection( &state_cs );

    set_selected_effect( NULL );

    if ((previous = device_selected))
    {
        IDirectInputDevice8_SetEventNotification( previous, NULL );
        IDirectInputDevice8_Unacquire( previous );
        IDirectInputDevice8_Release( previous );
    }
    if ((device_selected = device))
    {
        IDirectInputDevice8_AddRef( device );
        IDirectInputDevice8_SetEventNotification( device, state_event );
        IDirectInputDevice8_Acquire( device );
    }

    LeaveCriticalSection( &state_cs );
}

static IDirectInputDevice8W *get_selected_device(void)
{
    IDirectInputDevice8W *device;

    EnterCriticalSection( &state_cs );
    device = device_selected;
    if (device) IDirectInputDevice8_AddRef( device );
    LeaveCriticalSection( &state_cs );

    return device;
}

static BOOL CALLBACK enum_devices( const DIDEVICEINSTANCEW *instance, void *context )
{
    IDirectInput8W *dinput = context;
    struct device *entry;
    HRESULT hr;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return DIENUM_STOP;

    hr = IDirectInput8_CreateDevice( dinput, &instance->guidInstance, &entry->device, NULL );
    if (SUCCEEDED(hr)) hr = IDirectInputDevice8_SetDataFormat( entry->device, &c_dfDIJoystick2 );
    if (SUCCEEDED(hr)) hr = IDirectInputDevice8_SetCooperativeLevel( entry->device, GetAncestor( dialog_hwnd, GA_ROOT ),
                                                                     DISCL_BACKGROUND | DISCL_EXCLUSIVE );

    if (SUCCEEDED(hr)) list_add_tail( &devices, &entry->entry );
    else
    {
        if (entry->device) IDirectInputDevice8_Release( entry->device );
        free( entry );
    }

    return DIENUM_CONTINUE;
}

static void clear_devices(void)
{
    struct device *entry, *next;

    set_selected_device( NULL );

    LIST_FOR_EACH_ENTRY_SAFE( entry, next, &devices, struct device, entry )
    {
        list_remove( &entry->entry );
        IDirectInputDevice8_Release( entry->device );
        free( entry );
    }
}

static DWORD WINAPI input_thread( void *param )
{
    HANDLE events[2] = {param, state_event};
    IDirectInputEffect *playing = NULL;

    while (WaitForMultipleObjects( 2, events, FALSE, INFINITE ) != 0)
    {
        IDirectInputEffect *effect;
        DIJOYSTATE2 state = {0};
        HRESULT hr;

        SendMessageW( dialog_hwnd, WM_USER, 0, 0 );

        if ((effect = get_selected_effect( &state )))
        {
            static const BYTE empty[sizeof(state.rgbButtons)] = {0};
            BOOL pressed = memcmp( empty, state.rgbButtons, sizeof(state.rgbButtons) );
            LONG direction[2] = {state.lX - 32768, state.lY - 32768};
            DIEFFECT params =
            {
                .dwSize = sizeof(DIEFFECT),
                .dwFlags = DIEFF_CARTESIAN,
                .rglDirection = direction,
                .cAxes = 2,
            };

            if (playing && (!pressed || playing != effect))
            {
                IDirectInputEffect_Stop( playing );
                IDirectInputEffect_Release( playing );
                playing = NULL;
            }

            if (pressed && !playing)
            {
                do hr = IDirectInputEffect_SetParameters( effect, &params, DIEP_DIRECTION | DIEP_NORESTART );
                while (FAILED(hr) && --params.cAxes);

                IDirectInputEffect_Start( effect, 1, 0 );
                IDirectInputEffect_AddRef( effect );
                playing = effect;
            }

            IDirectInputEffect_Release( effect );
        }
        else if (playing)
        {
            IDirectInputEffect_Stop( playing );
            IDirectInputEffect_Release( playing );
            playing = NULL;
        }
    }

    if (playing)
    {
        IDirectInputEffect_Stop( playing );
        IDirectInputEffect_Release( playing );
        playing = NULL;
    }

    return 0;
}

static void draw_axis_view( HDC hdc, RECT rect, const WCHAR *name, double value )
{
    POINT center =
    {
        .x = (rect.left + rect.right) / 2 + 10,
        .y = (rect.top + rect.bottom) / 2,
    };
    LONG w = (rect.bottom - rect.top + 1) / 3;
    LONG x = rect.left + 20 + (w + 1) / 2 + value * (rect.right - rect.left - 20 - w);
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

void paint_axes_view( HWND hwnd, UINT32 count, double *axes, const WCHAR **names )
{
    RECT rect, tmp_rect;
    PAINTSTRUCT paint;
    HDC hdc;

    hdc = BeginPaint( hwnd, &paint );

    GetClientRect( hwnd, &rect );
    rect.bottom = rect.top + (rect.bottom - rect.top - 2) / 4 - 2;
    rect.right = rect.left + (rect.right - rect.left) / 2 - 10;

    OffsetRect( &rect, 5, 2 );
    draw_axis_view( hdc, rect, names[0], axes[0] );

    tmp_rect = rect;
    OffsetRect( &rect, rect.right - rect.left + 10, 0 );
    draw_axis_view( hdc, rect, names[4], axes[4] );
    rect = tmp_rect;

    OffsetRect( &rect, 0, rect.bottom - rect.top + 2 );
    draw_axis_view( hdc, rect, names[1], axes[1] );

    tmp_rect = rect;
    OffsetRect( &rect, rect.right - rect.left + 10, 0 );
    draw_axis_view( hdc, rect, names[5], axes[5] );
    rect = tmp_rect;

    OffsetRect( &rect, 0, rect.bottom - rect.top + 2 );
    draw_axis_view( hdc, rect, names[2], axes[2] );

    tmp_rect = rect;
    OffsetRect( &rect, rect.right - rect.left + 10, 0 );
    draw_axis_view( hdc, rect, names[6], axes[6] );
    rect = tmp_rect;

    OffsetRect( &rect, 0, rect.bottom - rect.top + 2 );
    draw_axis_view( hdc, rect, names[3], axes[3] );

    tmp_rect = rect;
    OffsetRect( &rect, rect.right - rect.left + 10, 0 );
    draw_axis_view( hdc, rect, names[7], axes[7] );
    rect = tmp_rect;

    EndPaint( hwnd, &paint );
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

void paint_povs_view( HWND hwnd, UINT32 count, UINT32 *povs )
{
    PAINTSTRUCT paint;
    RECT rect;
    HDC hdc;

    hdc = BeginPaint( hwnd, &paint );

    GetClientRect( hwnd, &rect );
    rect.bottom = rect.top + (rect.bottom - rect.top - 5) / 2 - 5;
    rect.right = rect.left + (rect.bottom - rect.top);

    OffsetRect( &rect, 5, 5 );
    draw_pov_view( hdc, rect, povs[0] );
    OffsetRect( &rect, rect.right - rect.left + 5, 0 );
    draw_pov_view( hdc, rect, povs[1] );
    OffsetRect( &rect, rect.left - rect.right - 5, rect.bottom - rect.top + 5 );
    draw_pov_view( hdc, rect, povs[1] );
    OffsetRect( &rect, rect.right - rect.left + 5, 0 );
    draw_pov_view( hdc, rect, povs[2] );

    EndPaint( hwnd, &paint );
}

static void draw_button_view( HDC hdc, RECT rect, BOOL set, const WCHAR *name )
{
    COLORREF color;
    HFONT font;
    INT mode;

    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    SetDCBrushColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
    SetDCPenColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWFRAME ) );
    SelectObject( hdc, GetStockObject( DC_BRUSH ) );
    SelectObject( hdc, GetStockObject( DC_PEN ) );

    if (rect.right - rect.left < 16) Rectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );
    else Ellipse( hdc, rect.left, rect.top, rect.right, rect.bottom );

    color = SetTextColor( hdc, GetSysColor( set ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT ) );
    font = SelectObject( hdc, GetStockObject( ANSI_VAR_FONT ) );
    mode = SetBkMode( hdc, TRANSPARENT );
    DrawTextW( hdc, name, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP );
    SetBkMode( hdc, mode );
    SetTextColor( hdc, color );
    SelectObject( hdc, font );
}

void paint_buttons_view( HWND hwnd, UINT32 count, BYTE *buttons )
{
    UINT i, j, offs, size, step, space = 2;
    PAINTSTRUCT paint;
    RECT rect;
    HDC hdc;

    if (count <= 48) step = 16;
    else step = 24;

    hdc = BeginPaint( hwnd, &paint );

    GetClientRect( hwnd, &rect );
    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    size = (rect.right - rect.left - space) / step;
    offs = (rect.right - rect.left - step * size - space) / 2;
    OffsetRect( &rect, offs, offs );
    rect.right = rect.left + size - space;
    rect.bottom = rect.top + size - space;

    for (i = 0; i < count;)
    {
        RECT first = rect;

        for (j = 0; j < step && i < count; j++, i++)
        {
            WCHAR buffer[3];
            if (step == 24) swprintf( buffer, ARRAY_SIZE(buffer), L"%02x", i );
            else swprintf( buffer, ARRAY_SIZE(buffer), L"%d", i );
            draw_button_view( hdc, rect, buttons[i], buffer );
            OffsetRect( &rect, size, 0 );
        }

        rect = first;
        OffsetRect( &rect, 0, size );
    }

    EndPaint( hwnd, &paint );
}

LRESULT CALLBACK test_di_axes_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        static const WCHAR *names[] = { L"X", L"Y", L"Z", L"S", L"Rx", L"Ry", L"Rz", L"Rs" };
        DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
        double axes[16], *axis = axes;
        IDirectInputDevice8W *device;
        DIJOYSTATE2 state = {0};

        if ((device = get_selected_device()))
        {
            IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
            IDirectInputDevice8_GetCapabilities( device, &caps );
            IDirectInputDevice8_Release( device );
        }

        *axis++ = (double)state.lX / 65535.0;
        *axis++ = (double)state.lY / 65535.0;
        *axis++ = (double)state.lZ / 65535.0;
        *axis++ = (double)state.rglSlider[0] / 65535.0;
        *axis++ = (double)state.lRx / 65535.0;
        *axis++ = (double)state.lRy / 65535.0;
        *axis++ = (double)state.lRz / 65535.0;
        *axis++ = (double)state.rglSlider[1] / 65535.0;

        paint_axes_view( hwnd, min( caps.dwAxes, ARRAY_SIZE(axes) ), axes, names );
        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

LRESULT CALLBACK test_di_povs_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
        IDirectInputDevice8W *device;
        DIJOYSTATE2 state = {0};

        if ((device = get_selected_device()))
        {
            IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
            IDirectInputDevice8_GetCapabilities( device, &caps );
            IDirectInputDevice8_Release( device );
        }

        paint_povs_view( hwnd, caps.dwPOVs, (UINT32 *)state.rgdwPOV );
        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

LRESULT CALLBACK test_di_buttons_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
        IDirectInputDevice8W *device;
        DIJOYSTATE2 state = {0};

        if ((device = get_selected_device()))
        {
            IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
            IDirectInputDevice8_GetCapabilities( device, &caps );
            IDirectInputDevice8_Release( device );
        }

        paint_buttons_view( hwnd, min( caps.dwButtons, ARRAY_SIZE(state.rgbButtons) ), state.rgbButtons );
        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void update_di_effects( HWND hwnd, IDirectInputDevice8W *device )
{
    struct effect *effect;

    clear_effects();

    IDirectInputDevice8_EnumEffects( device, enum_effects, device, 0 );

    SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_RESETCONTENT, 0, 0 );
    SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_ADDSTRING, 0, (LPARAM)L"None" );

    LIST_FOR_EACH_ENTRY( effect, &effects, struct effect, entry )
    {
        DIEFFECTINFOW info = {.dwSize = sizeof(DIEFFECTINFOW)};
        GUID guid;

        if (FAILED(IDirectInputEffect_GetEffectGuid( effect->effect, &guid ))) continue;
        if (FAILED(IDirectInputDevice8_GetEffectInfo( device, &info, &guid ))) continue;
        SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_ADDSTRING, 0, (LPARAM)( info.tszName + 5 ) );
    }
}

static void handle_di_effects_change( HWND hwnd )
{
    struct list *entry;
    int sel;

    set_selected_effect( NULL );

    sel = SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_GETCURSEL, 0, 0 ) - 1;
    if (sel < 0) return;

    entry = list_head( &effects );
    while (sel-- && entry) entry = list_next( &effects, entry );
    if (!entry) return;

    set_selected_effect( LIST_ENTRY( entry, struct effect, entry )->effect );
}

static void create_device_views( HWND hwnd )
{
    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    HWND parent;
    LONG margin;
    RECT rect;

    parent = GetDlgItem( hwnd, IDC_DI_AXES );
    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 10 / 100;
    InflateRect( &rect, -margin, -margin );

    CreateWindowW( L"JoyCplDInputAxes", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );

    parent = GetDlgItem( hwnd, IDC_DI_POVS );
    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 10 / 100;
    InflateRect( &rect, -margin, -margin );

    CreateWindowW( L"JoyCplDInputPOVs", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );

    parent = GetDlgItem( hwnd, IDC_DI_BUTTONS );
    GetClientRect( parent, &rect );
    rect.top += 10;

    margin = (rect.bottom - rect.top) * 5 / 100;
    InflateRect( &rect, -margin, -margin );

    CreateWindowW( L"JoyCplDInputButtons", NULL, WS_CHILD | WS_VISIBLE, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top, parent, NULL, NULL, instance );
}

static void handle_di_devices_change( HWND hwnd )
{
    DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};
    IDirectInputDevice8W *device;
    struct list *entry;
    int i;

    set_selected_device( NULL );

    i = SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_GETCURSEL, 0, 0 );
    if (i < 0) return;

    entry = list_head( &devices );
    while (i-- && entry) entry = list_next( &devices, entry );
    if (!entry) return;

    device = LIST_ENTRY( entry, struct device, entry )->device;
    if (FAILED(IDirectInputDevice8_GetCapabilities( device, &caps ))) return;

    set_selected_device( device );
    update_di_effects( hwnd, device );
}

static void update_di_devices( HWND hwnd )
{
    IDirectInput8W *dinput;
    struct device *entry;

    clear_devices();

    DirectInput8Create( GetModuleHandleW( NULL ), DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void **)&dinput, NULL );
    IDirectInput8_EnumDevices( dinput, DI8DEVCLASS_GAMECTRL, enum_devices, dinput, DIEDFL_ATTACHEDONLY );
    IDirectInput8_Release( dinput );

    SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_RESETCONTENT, 0, 0 );

    LIST_FOR_EACH_ENTRY( entry, &devices, struct device, entry )
    {
        DIDEVICEINSTANCEW info = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
        if (FAILED(IDirectInputDevice8_GetDeviceInfo( entry->device, &info ))) continue;
        SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_ADDSTRING, 0, (LPARAM)info.tszInstanceName );
    }
}

static void update_device_views( HWND hwnd )
{
    HWND parent, view;

    parent = GetDlgItem( hwnd, IDC_DI_AXES );
    view = FindWindowExW( parent, NULL, L"JoyCplDInputAxes", NULL );
    InvalidateRect( view, NULL, TRUE );

    parent = GetDlgItem( hwnd, IDC_DI_POVS );
    view = FindWindowExW( parent, NULL, L"JoyCplDInputPOVs", NULL );
    InvalidateRect( view, NULL, TRUE );

    parent = GetDlgItem( hwnd, IDC_DI_BUTTONS );
    view = FindWindowExW( parent, NULL, L"JoyCplDInputButtons", NULL );
    InvalidateRect( view, NULL, TRUE );
}

INT_PTR CALLBACK test_di_dialog_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    static HANDLE thread, thread_stop;
    DEV_BROADCAST_DEVICEINTERFACE_W filter =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_W),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
        .dbcc_classguid = GUID_DEVINTERFACE_HID,
    };

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    switch (msg)
    {
    case WM_INITDIALOG:
        create_device_views( hwnd );
        return TRUE;

    case WM_COMMAND:
        switch (wparam)
        {
        case MAKEWPARAM( IDC_DI_DEVICES, CBN_SELCHANGE ):
            handle_di_devices_change( hwnd );

            SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_SETCURSEL, 0, 0 );
            handle_di_effects_change( hwnd );

            update_device_views( hwnd );
            break;

        case MAKEWPARAM( IDC_DI_EFFECTS, LBN_SELCHANGE ):
            handle_di_effects_change( hwnd );
            break;
        }
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR *)lparam)->code)
        {
        case PSN_SETACTIVE:
            dialog_hwnd = hwnd;
            state_event = CreateEventW( NULL, FALSE, FALSE, NULL );
            thread_stop = CreateEventW( NULL, FALSE, FALSE, NULL );

            devnotify = RegisterDeviceNotificationW( hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE );
            update_di_devices( hwnd );

            SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_SETCURSEL, 0, 0 );
            handle_di_devices_change( hwnd );

            SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_SETCURSEL, 0, 0 );
            handle_di_effects_change( hwnd );

            thread = CreateThread( NULL, 0, input_thread, (void *)thread_stop, 0, NULL );
            break;

        case PSN_RESET:
        case PSN_KILLACTIVE:
            if (!devnotify) break;
            UnregisterDeviceNotification( devnotify );
            devnotify = NULL;
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
            CloseHandle( state_event );
            CloseHandle( thread_stop );
            CloseHandle( thread );

            clear_effects();
            clear_devices();
            break;
        }
        return TRUE;

    case WM_DEVICECHANGE:
        update_di_devices( hwnd );

        if (SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_GETCURSEL, 0, 0 ) >= 0) return TRUE;

        SendDlgItemMessageW( hwnd, IDC_DI_DEVICES, CB_SETCURSEL, 0, 0 );
        handle_di_devices_change( hwnd );

        SendDlgItemMessageW( hwnd, IDC_DI_EFFECTS, LB_SETCURSEL, 0, 0 );
        handle_di_effects_change( hwnd );
        return TRUE;

    case WM_USER:
        update_device_views( hwnd );
        return TRUE;
    }
    return FALSE;
}
