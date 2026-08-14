/* WinRT Windows.Xbox.Input implementation
 *
 * Bridges to the real Windows.Gaming.Input.Gamepad implementation shipped
 * in windows.gaming.input.dll. Cross-DLL WinRT activation normally goes
 * through RoGetActivationFactory, which does a registry lookup for the DLL
 * implementing a given class id; that registration data is not available
 * here, so windows.gaming.input.dll is instead loaded directly with
 * LoadLibraryW() + GetProcAddress(L"DllGetActivationFactory"), the same
 * mechanism combase uses internally, just without the registry lookup.
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
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "objbase.h"

#include "activation.h"
#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Gaming_Input
#include "windows.gaming.input.h"

#include "wine/debug.h"

#include "wgi_backend.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

/* This file deliberately does not "#include initguid.h": main.c is the
 * single place in this module that instantiates GUID storage for the
 * headers it includes (windows.foundation.h, input.h), and doing so here
 * too for windows.gaming.input.h would duplicate storage for the Foundation
 * generic interfaces both headers embed, causing duplicate symbols at link
 * time. windows.gaming.input.dll is a different module anyway, so its own
 * IID_IGamepadStatics symbol (were it importable) wouldn't help here; the
 * one WGI IID this file needs is given as a local literal below instead. */
static const IID wgi_IID_IGamepadStatics =
{ 0x8bbce529, 0xd49c, 0x39e9, { 0x95, 0x60, 0xe4, 0x7d, 0xde, 0x96, 0xb7, 0xc8 } };

typedef HRESULT (WINAPI *p_DllGetActivationFactory)( HSTRING classid, IActivationFactory **factory );

static CRITICAL_SECTION wgi_cs;
static CRITICAL_SECTION_DEBUG wgi_cs_debug =
{
    0, 0, &wgi_cs,
    { &wgi_cs_debug.ProcessLocksList, &wgi_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wgi_cs") }
};
static CRITICAL_SECTION wgi_cs = { &wgi_cs_debug, -1, 0, 0, 0, 0 };

static HMODULE wgi_module;
static IGamepadStatics *wgi_statics;

/* must be called with wgi_cs held */
static HRESULT load_wgi_statics(void)
{
    p_DllGetActivationFactory p_get_factory;
    IActivationFactory *factory = NULL;
    HSTRING classid = NULL;
    HRESULT hr = S_OK;

    if (wgi_statics) return S_OK;

    if (!wgi_module && !(wgi_module = LoadLibraryW( L"windows.gaming.input.dll" )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        WARN( "Failed to load windows.gaming.input.dll, hr %#lx.\n", hr );
        return hr;
    }

    if (!(p_get_factory = (p_DllGetActivationFactory)GetProcAddress( wgi_module, "DllGetActivationFactory" )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        WARN( "Failed to find DllGetActivationFactory in windows.gaming.input.dll, hr %#lx.\n", hr );
        return hr;
    }

    if (FAILED(hr = WindowsCreateString( RuntimeClass_Windows_Gaming_Input_Gamepad,
                                         ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_Gamepad) - 1, &classid )))
        return hr;

    hr = p_get_factory( classid, &factory );
    WindowsDeleteString( classid );
    if (FAILED(hr))
    {
        WARN( "DllGetActivationFactory(Windows.Gaming.Input.Gamepad) failed, hr %#lx.\n", hr );
        return hr;
    }

    hr = IActivationFactory_QueryInterface( factory, &wgi_IID_IGamepadStatics, (void **)&wgi_statics );
    IActivationFactory_Release( factory );
    if (FAILED(hr)) WARN( "IGamepadStatics not supported, hr %#lx.\n", hr );

    return hr;
}

static HRESULT get_wgi_statics( IGamepadStatics **out )
{
    HRESULT hr;

    EnterCriticalSection( &wgi_cs );
    if (SUCCEEDED(hr = load_wgi_statics())) IGamepadStatics_AddRef( (*out = wgi_statics) );
    LeaveCriticalSection( &wgi_cs );

    return hr;
}

HRESULT wgi_backend_get_gamepads( void ***handles, UINT32 *count )
{
    IVectorView_Gamepad *view;
    IGamepadStatics *statics;
    UINT32 size, i;
    void **out;
    HRESULT hr;

    *handles = NULL;
    *count = 0;

    if (FAILED(hr = get_wgi_statics( &statics ))) return hr;

    hr = IGamepadStatics_get_Gamepads( statics, &view );
    IGamepadStatics_Release( statics );
    if (FAILED(hr)) return hr;

    if (FAILED(hr = IVectorView_Gamepad_get_Size( view, &size )))
    {
        IVectorView_Gamepad_Release( view );
        return hr;
    }

    if (!size)
    {
        IVectorView_Gamepad_Release( view );
        return S_OK;
    }

    if (!(out = calloc( size, sizeof(*out) )))
    {
        IVectorView_Gamepad_Release( view );
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < size; ++i)
    {
        IGamepad *gamepad;
        if (FAILED(hr = IVectorView_Gamepad_GetAt( view, i, &gamepad ))) break;
        out[i] = gamepad; /* already AddRef'd by GetAt */
    }
    IVectorView_Gamepad_Release( view );

    if (FAILED(hr))
    {
        while (i--) IGamepad_Release( (IGamepad *)out[i] );
        free( out );
        return hr;
    }

    *handles = out;
    *count = size;
    return S_OK;
}

void wgi_backend_addref( void *handle )
{
    IGamepad_AddRef( (IGamepad *)handle );
}

void wgi_backend_release( void *handle )
{
    IGamepad_Release( (IGamepad *)handle );
}

HRESULT wgi_backend_get_reading( void *handle, struct wgi_gamepad_reading *reading )
{
    struct __x_ABI_CWindows_CGaming_CInput_CGamepadReading value;
    HRESULT hr;

    memset( reading, 0, sizeof(*reading) );

    if (FAILED(hr = IGamepad_GetCurrentReading( (IGamepad *)handle, &value ))) return hr;

    reading->timestamp = value.Timestamp;
    reading->buttons = value.Buttons;
    reading->left_trigger = (float)value.LeftTrigger;
    reading->right_trigger = (float)value.RightTrigger;
    reading->left_thumbstick_x = (float)value.LeftThumbstickX;
    reading->left_thumbstick_y = (float)value.LeftThumbstickY;
    reading->right_thumbstick_x = (float)value.RightThumbstickX;
    reading->right_thumbstick_y = (float)value.RightThumbstickY;

    return S_OK;
}

HRESULT wgi_backend_set_vibration( void *handle, const struct wgi_gamepad_vibration *vibration )
{
    struct __x_ABI_CWindows_CGaming_CInput_CGamepadVibration value =
    {
        .LeftMotor = vibration->left_motor,
        .RightMotor = vibration->right_motor,
        .LeftTrigger = vibration->left_trigger,
        .RightTrigger = vibration->right_trigger,
    };

    return IGamepad_put_Vibration( (IGamepad *)handle, value );
}
