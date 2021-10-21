/*		DirectInput Mouse device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winternl.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Wine mouse driver object instances */
#define WINE_MOUSE_X_AXIS_INSTANCE   0
#define WINE_MOUSE_Y_AXIS_INSTANCE   1
#define WINE_MOUSE_Z_AXIS_INSTANCE   2
#define WINE_MOUSE_BUTTONS_INSTANCE  3

static const IDirectInputDevice8WVtbl SysMouseWvt;
static const struct dinput_device_vtbl mouse_internal_vtbl;

typedef struct SysMouseImpl SysMouseImpl;

typedef enum
{
    WARP_DEFAULT,
    WARP_DISABLE,
    WARP_FORCE_ON
} WARP_MOUSE;

struct SysMouseImpl
{
    struct IDirectInputDeviceImpl   base;

    /* SysMouseAImpl */
    /* These are used in case of relative -> absolute transitions */
    POINT                           org_coords;
    BOOL                            clipped;
    /* warping: whether we need to move mouse back to middle once we
     * reach window borders (for e.g. shooters, "surface movement" games) */
    BOOL                            need_warp;
    DWORD                           last_warped;

    WARP_MOUSE                      warp_override;
};

static inline SysMouseImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface), SysMouseImpl, base);
}

static void fill_mouse_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version) {
    DWORD dwSize;
    DIDEVICEINSTANCEW ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));

    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysMouse;/* DInput's GUID */
    ddi.guidProduct = GUID_SysMouse;
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
    else
        ddi.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_TRADITIONAL << 8);
    MultiByteToWideChar(CP_ACP, 0, "Mouse", -1, ddi.tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, "Wine Mouse", -1, ddi.tszProductName, MAX_PATH);

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}

static HRESULT mousedev_enum_device(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
    if (id != 0)
        return E_FAIL;

    if (dwFlags & DIEDFL_FORCEFEEDBACK)
        return S_FALSE;

    if ((dwDevType == 0) ||
	((dwDevType == DIDEVTYPE_MOUSE) && (version < 0x0800)) ||
	(((dwDevType == DI8DEVCLASS_POINTER) || (dwDevType == DI8DEVTYPE_MOUSE)) && (version >= 0x0800))) {
	TRACE("Enumerating the mouse device\n");
	
	fill_mouse_dideviceinstanceW(lpddi, version);
	
	return S_OK;
    }
    
    return S_FALSE;
}

static HRESULT alloc_device( REFGUID rguid, IDirectInputImpl *dinput, SysMouseImpl **out )
{
    SysMouseImpl* newDevice;
    WCHAR buffer[20];
    HKEY hkey, appkey;
    HRESULT hr;

    if (FAILED(hr = direct_input_device_alloc( sizeof(SysMouseImpl), &SysMouseWvt, &mouse_internal_vtbl,
                                               rguid, dinput, (void **)&newDevice )))
        return hr;
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SysMouseImpl*->base.crit");

    fill_mouse_dideviceinstanceW( &newDevice->base.instance, newDevice->base.dinput->dwVersion );
    newDevice->base.caps.dwDevType = newDevice->base.instance.dwDevType;
    newDevice->base.caps.dwFirmwareRevision = 100;
    newDevice->base.caps.dwHardwareRevision = 100;
    newDevice->base.dwCoopLevel = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;

    get_app_key(&hkey, &appkey);
    if (!get_config_key( hkey, appkey, L"MouseWarpOverride", buffer, sizeof(buffer) ))
    {
        if (!wcsnicmp( buffer, L"disable", -1 )) newDevice->warp_override = WARP_DISABLE;
        else if (!wcsnicmp( buffer, L"force", -1 )) newDevice->warp_override = WARP_FORCE_ON;
    }
    if (appkey) RegCloseKey(appkey);
    if (hkey) RegCloseKey(hkey);

    if (FAILED(hr = direct_input_device_init( &newDevice->base.IDirectInputDevice8W_iface )))
    {
        IDirectInputDevice_Release( &newDevice->base.IDirectInputDevice8W_iface );
        return hr;
    }

    if (dinput->dwVersion >= 0x0800)
    {
        newDevice->base.use_raw_input = TRUE;
        newDevice->base.raw_device.usUsagePage = 1; /* HID generic device page */
        newDevice->base.raw_device.usUsage = 2; /* HID generic mouse */
    }

    *out = newDevice;
    return DI_OK;
}

static HRESULT mousedev_create_device( IDirectInputImpl *dinput, REFGUID rguid, IDirectInputDevice8W **out )
{
    TRACE( "%p %s %p\n", dinput, debugstr_guid( rguid ), out );
    *out = NULL;

    if (IsEqualGUID(&GUID_SysMouse, rguid)) /* Wine Mouse */
    {
        SysMouseImpl *This;
        HRESULT hr;

        if (FAILED(hr = alloc_device( rguid, dinput, &This ))) return hr;

        TRACE( "Created a Mouse device (%p)\n", This );

        *out = &This->base.IDirectInputDevice8W_iface;
        return DI_OK;
    }

    return DIERR_DEVICENOTREG;
}

const struct dinput_device mouse_device = {
    "Wine mouse driver",
    mousedev_enum_device,
    mousedev_create_device
};

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

void dinput_mouse_rawinput_hook( IDirectInputDevice8W *iface, WPARAM wparam, LPARAM lparam, RAWINPUT *ri )
{
    SysMouseImpl *This = impl_from_IDirectInputDevice8W( iface );
    DIMOUSESTATE2 *state = (DIMOUSESTATE2 *)This->base.device_state;
    POINT rel, pt;
    DWORD seq;
    int i, wdata = 0;
    BOOL notify = FALSE;

    static const USHORT mouse_button_flags[] =
    {
        RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP,
        RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP,
        RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP,
        RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP,
        RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP
    };

    TRACE( "(%p) wp %08lx, lp %08lx\n", iface, wparam, lparam );

    if (ri->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP)
        FIXME( "Unimplemented MOUSE_VIRTUAL_DESKTOP flag\n" );
    if (ri->data.mouse.usFlags & MOUSE_ATTRIBUTES_CHANGED)
        FIXME( "Unimplemented MOUSE_ATTRIBUTES_CHANGED flag\n" );

    EnterCriticalSection( &This->base.crit );
    seq = This->base.dinput->evsequence++;

    rel.x = ri->data.mouse.lLastX;
    rel.y = ri->data.mouse.lLastY;
    if (ri->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        GetCursorPos( &pt );
        rel.x -= pt.x;
        rel.y -= pt.y;
    }

    state->lX += rel.x;
    state->lY += rel.y;

    if (This->base.data_format.user_df->dwFlags & DIDF_ABSAXIS)
    {
        pt.x = state->lX;
        pt.y = state->lY;
    }
    else
    {
        pt = rel;
    }

    if (rel.x)
    {
        queue_event( iface, DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS,
                     pt.x, GetCurrentTime(), seq );
        notify = TRUE;
    }

    if (rel.y)
    {
        queue_event( iface, DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS,
                     pt.y, GetCurrentTime(), seq );
        notify = TRUE;
    }

    if (rel.x || rel.y)
    {
        if ((This->warp_override == WARP_FORCE_ON) ||
            (This->warp_override != WARP_DISABLE && (This->base.dwCoopLevel & DISCL_EXCLUSIVE)))
            This->need_warp = TRUE;
    }

    if (ri->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
    {
        state->lZ += (wdata = (SHORT)ri->data.mouse.usButtonData);
        queue_event( iface, DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS,
                     wdata, GetCurrentTime(), seq );
        notify = TRUE;
    }

    for (i = 0; i < ARRAY_SIZE(mouse_button_flags); ++i)
    {
        if (ri->data.mouse.usButtonFlags & mouse_button_flags[i])
        {
            state->rgbButtons[i / 2] = 0x80 - (i % 2) * 0x80;
            queue_event( iface, DIDFT_MAKEINSTANCE( WINE_MOUSE_BUTTONS_INSTANCE + (i / 2) ) | DIDFT_PSHBUTTON,
                         state->rgbButtons[i / 2], GetCurrentTime(), seq );
            notify = TRUE;
        }
    }

    TRACE( "buttons %02x %02x %02x %02x %02x, x %d, y %d, w %d\n", state->rgbButtons[0],
           state->rgbButtons[1], state->rgbButtons[2], state->rgbButtons[3], state->rgbButtons[4],
           state->lX, state->lY, state->lZ );

    if (notify && This->base.hEvent) SetEvent( This->base.hEvent );
    LeaveCriticalSection( &This->base.crit );
}

/* low-level mouse hook */
int dinput_mouse_hook( IDirectInputDevice8W *iface, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    SysMouseImpl *This = impl_from_IDirectInputDevice8W( iface );
    DIMOUSESTATE2 *state = (DIMOUSESTATE2 *)This->base.device_state;
    int wdata = 0, inst_id = -1, ret = 0;
    BOOL notify = FALSE;

    TRACE("msg %lx @ (%d %d)\n", wparam, hook->pt.x, hook->pt.y);

    EnterCriticalSection(&This->base.crit);

    switch(wparam) {
        case WM_MOUSEMOVE:
        {
            POINT pt, pt1;

            GetCursorPos(&pt);
            state->lX += pt.x = hook->pt.x - pt.x;
            state->lY += pt.y = hook->pt.y - pt.y;

            if (This->base.data_format.user_df->dwFlags & DIDF_ABSAXIS)
            {
                pt1.x = state->lX;
                pt1.y = state->lY;
            } else
                pt1 = pt;

            if (pt.x)
            {
                inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_X_AXIS_INSTANCE) | DIDFT_RELAXIS;
                wdata = pt1.x;
            }
            if (pt.y)
            {
                /* Already have X, need to queue it */
                if (inst_id != -1)
                {
                    queue_event(iface, inst_id,
                                wdata, GetCurrentTime(), This->base.dinput->evsequence);
                    notify = TRUE;
                }
                inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_Y_AXIS_INSTANCE) | DIDFT_RELAXIS;
                wdata = pt1.y;
            }

            if (pt.x || pt.y)
            {
                if ((This->warp_override == WARP_FORCE_ON) ||
                    (This->warp_override != WARP_DISABLE && (This->base.dwCoopLevel & DISCL_EXCLUSIVE)))
                    This->need_warp = TRUE;
            }
            break;
        }
        case WM_MOUSEWHEEL:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_Z_AXIS_INSTANCE) | DIDFT_RELAXIS;
            state->lZ += wdata = (short)HIWORD( hook->mouseData );
            /* FarCry crashes if it gets a mouse wheel message */
            /* FIXME: should probably filter out other messages too */
            ret = This->clipped;
            break;
        case WM_LBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 0) | DIDFT_PSHBUTTON;
            state->rgbButtons[0] = wdata = 0x80;
            break;
        case WM_LBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 0) | DIDFT_PSHBUTTON;
            state->rgbButtons[0] = wdata = 0x00;
            break;
        case WM_RBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 1) | DIDFT_PSHBUTTON;
            state->rgbButtons[1] = wdata = 0x80;
            break;
        case WM_RBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 1) | DIDFT_PSHBUTTON;
            state->rgbButtons[1] = wdata = 0x00;
            break;
        case WM_MBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2) | DIDFT_PSHBUTTON;
            state->rgbButtons[2] = wdata = 0x80;
            break;
        case WM_MBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2) | DIDFT_PSHBUTTON;
            state->rgbButtons[2] = wdata = 0x00;
            break;
        case WM_XBUTTONDOWN:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2 + HIWORD(hook->mouseData)) | DIDFT_PSHBUTTON;
            state->rgbButtons[2 + HIWORD( hook->mouseData )] = wdata = 0x80;
            break;
        case WM_XBUTTONUP:
            inst_id = DIDFT_MAKEINSTANCE(WINE_MOUSE_BUTTONS_INSTANCE + 2 + HIWORD(hook->mouseData)) | DIDFT_PSHBUTTON;
            state->rgbButtons[2 + HIWORD( hook->mouseData )] = wdata = 0x00;
            break;
    }


    if (inst_id != -1)
    {
        queue_event(iface, inst_id,
                    wdata, GetCurrentTime(), This->base.dinput->evsequence++);
        notify = TRUE;
    }

    TRACE( "buttons %02x %02x %02x %02x %02x, x %d, y %d, w %d\n", state->rgbButtons[0],
           state->rgbButtons[1], state->rgbButtons[2], state->rgbButtons[3], state->rgbButtons[4],
           state->lX, state->lY, state->lZ );

    if (notify && This->base.hEvent) SetEvent( This->base.hEvent );
    LeaveCriticalSection(&This->base.crit);
    return ret;
}

static void warp_check( SysMouseImpl* This, BOOL force )
{
    DWORD now = GetCurrentTime();
    const DWORD interval = This->clipped ? 500 : 10;

    if (force || (This->need_warp && (now - This->last_warped > interval)))
    {
        RECT rect, new_rect;
        POINT mapped_center;

        This->last_warped = now;
        This->need_warp = FALSE;
        if (!GetClientRect(This->base.win, &rect)) return;
        MapWindowPoints( This->base.win, 0, (POINT *)&rect, 2 );
        if (!This->clipped)
        {
            mapped_center.x = (rect.left + rect.right) / 2;
            mapped_center.y = (rect.top + rect.bottom) / 2;
            TRACE("Warping mouse to %d - %d\n", mapped_center.x, mapped_center.y);
            SetCursorPos( mapped_center.x, mapped_center.y );
        }
        if (This->base.dwCoopLevel & DISCL_EXCLUSIVE)
        {
            /* make sure we clip even if the window covers the whole screen */
            rect.left = max( rect.left, GetSystemMetrics( SM_XVIRTUALSCREEN ) + 1 );
            rect.top = max( rect.top, GetSystemMetrics( SM_YVIRTUALSCREEN ) + 1 );
            rect.right = min( rect.right, rect.left + GetSystemMetrics( SM_CXVIRTUALSCREEN ) - 2 );
            rect.bottom = min( rect.bottom, rect.top + GetSystemMetrics( SM_CYVIRTUALSCREEN ) - 2 );
            TRACE("Clipping mouse to %s\n", wine_dbgstr_rect( &rect ));
            ClipCursor( &rect );
            This->clipped = GetClipCursor( &new_rect ) && EqualRect( &rect, &new_rect );
        }
    }
}

static HRESULT mouse_internal_poll( IDirectInputDevice8W *iface )
{
    SysMouseImpl *impl = impl_from_IDirectInputDevice8W( iface );
    check_dinput_events();
    warp_check( impl, FALSE );
    return DI_OK;
}

static HRESULT mouse_internal_acquire( IDirectInputDevice8W *iface )
{
    SysMouseImpl *impl = impl_from_IDirectInputDevice8W( iface );
    DIMOUSESTATE2 *state = (DIMOUSESTATE2 *)impl->base.device_state;
    POINT point;

    /* Init the mouse state */
    GetCursorPos( &point );
    if (impl->base.data_format.user_df->dwFlags & DIDF_ABSAXIS)
    {
        state->lX = point.x;
        state->lY = point.y;
    }
    else
    {
        state->lX = 0;
        state->lY = 0;
        impl->org_coords = point;
    }
    state->lZ = 0;
    state->rgbButtons[0] = GetKeyState( VK_LBUTTON ) & 0x80;
    state->rgbButtons[1] = GetKeyState( VK_RBUTTON ) & 0x80;
    state->rgbButtons[2] = GetKeyState( VK_MBUTTON ) & 0x80;

    if (impl->base.dwCoopLevel & DISCL_EXCLUSIVE)
    {
        ShowCursor( FALSE ); /* hide cursor */
        warp_check( impl, TRUE );
    }
    else if (impl->warp_override == WARP_FORCE_ON)
    {
        /* Need a window to warp mouse in. */
        if (!impl->base.win) impl->base.win = GetDesktopWindow();
        warp_check( impl, TRUE );
    }
    else if (impl->clipped)
    {
        ClipCursor( NULL );
        impl->clipped = FALSE;
    }

    return DI_OK;
}

static HRESULT mouse_internal_unacquire( IDirectInputDevice8W *iface )
{
    SysMouseImpl *impl = impl_from_IDirectInputDevice8W( iface );

    if (impl->base.dwCoopLevel & DISCL_EXCLUSIVE)
    {
        ClipCursor( NULL );
        ShowCursor( TRUE ); /* show cursor */
        impl->clipped = FALSE;
    }

    /* And put the mouse cursor back where it was at acquire time */
    if (impl->base.dwCoopLevel & DISCL_EXCLUSIVE || impl->warp_override == WARP_FORCE_ON)
    {
        TRACE( "warping mouse back to %s\n", wine_dbgstr_point( &impl->org_coords ) );
        SetCursorPos( impl->org_coords.x, impl->org_coords.y );
    }

    return DI_OK;
}

static BOOL try_enum_object( const DIPROPHEADER *filter, DWORD flags, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                             DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    if (flags != DIDFT_ALL && !(flags & DIDFT_GETTYPE( instance->dwType ))) return DIENUM_CONTINUE;

    switch (filter->dwHow)
    {
    case DIPH_DEVICE:
        return callback( instance, data );
    case DIPH_BYOFFSET:
        if (filter->dwObj != instance->dwOfs) return DIENUM_CONTINUE;
        return callback( instance, data );
    case DIPH_BYID:
        if ((filter->dwObj & 0x00ffffff) != (instance->dwType & 0x00ffffff)) return DIENUM_CONTINUE;
        return callback( instance, data );
    }

    return DIENUM_CONTINUE;
}

static HRESULT mouse_internal_enum_objects( IDirectInputDevice8W *iface, const DIPROPHEADER *filter, DWORD flags,
                                            LPDIENUMDEVICEOBJECTSCALLBACKW callback, void *context )
{
    DIDEVICEOBJECTINSTANCEW instances[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_XAxis,
            .dwOfs = DIMOFS_X,
            .dwType = DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(0),
            .dwFlags = DIDOI_ASPECTPOSITION,
            .tszName = L"X-axis",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_YAxis,
            .dwOfs = DIMOFS_Y,
            .dwType = DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(1),
            .dwFlags = DIDOI_ASPECTPOSITION,
            .tszName = L"Y-axis",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_ZAxis,
            .dwOfs = DIMOFS_Z,
            .dwType = DIDFT_RELAXIS|DIDFT_MAKEINSTANCE(2),
            .dwFlags = DIDOI_ASPECTPOSITION,
            .tszName = L"Wheel",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = DIMOFS_BUTTON0,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(3),
            .tszName = L"Button 0",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = DIMOFS_BUTTON1,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(4),
            .tszName = L"Button 1",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = DIMOFS_BUTTON2,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(5),
            .tszName = L"Button 2",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = DIMOFS_BUTTON3,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(6),
            .tszName = L"Button 3",
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = DIMOFS_BUTTON4,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(7),
            .tszName = L"Button 4",
        },
    };
    BOOL ret;
    DWORD i;

    for (i = 0; i < ARRAY_SIZE(instances); ++i)
    {
        ret = try_enum_object( filter, flags, callback, instances + i, context );
        if (ret != DIENUM_CONTINUE) return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

static HRESULT mouse_internal_get_property( IDirectInputDevice8W *iface, DWORD property, DIPROPHEADER *header,
                                            DIDEVICEOBJECTINSTANCEW *instance )
{
    switch (property)
    {
    case (DWORD_PTR)DIPROP_RANGE:
    {
        DIPROPRANGE *range = (DIPROPRANGE *)header;
        range->lMin = DIPROPRANGE_NOMIN;
        range->lMax = DIPROPRANGE_NOMAX;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_GRANULARITY:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (instance->dwType == DIMOFS_Z) value->dwData = WHEEL_DELTA;
        else value->dwData = 1;
        return DI_OK;
    }
    }
    return DIERR_UNSUPPORTED;
}

static HRESULT mouse_internal_set_property( IDirectInputDevice8W *iface, DWORD property, const DIPROPHEADER *header,
                                            const DIDEVICEOBJECTINSTANCEW *instance )
{
    return DIERR_UNSUPPORTED;
}

static const struct dinput_device_vtbl mouse_internal_vtbl =
{
    NULL,
    mouse_internal_poll,
    NULL,
    mouse_internal_acquire,
    mouse_internal_unacquire,
    mouse_internal_enum_objects,
    mouse_internal_get_property,
    mouse_internal_set_property,
    NULL,
    NULL,
    NULL,
    NULL,
};

static const IDirectInputDevice8WVtbl SysMouseWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    IDirectInputDevice2WImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    IDirectInputDevice2WImpl_GetProperty,
    IDirectInputDevice2WImpl_SetProperty,
    IDirectInputDevice2WImpl_Acquire,
    IDirectInputDevice2WImpl_Unacquire,
    IDirectInputDevice2WImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    IDirectInputDevice2WImpl_GetObjectInfo,
    IDirectInputDevice2WImpl_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    IDirectInputDevice2WImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    IDirectInputDevice2WImpl_SendForceFeedbackCommand,
    IDirectInputDevice2WImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    IDirectInputDevice2WImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    IDirectInputDevice8WImpl_BuildActionMap,
    IDirectInputDevice8WImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
