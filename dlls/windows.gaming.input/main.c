/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include "winreg.h"
#include "winuser.h"

#include "dbt.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"
#include "setupapi.h"

#include "initguid.h"
#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

HINSTANCE windows_gaming_input;

DEFINE_GUID( GUID_DEVINTERFACE_WINEXINPUT,0x6c53d5fd,0x6480,0x440f,0xb6,0x18,0x47,0x67,0x50,0xc5,0xe1,0xa6 );

static LRESULT CALLBACK devnotify_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    DEV_BROADCAST_DEVICEINTERFACE_W *iface;

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_DEVICECHANGE)
    {
        switch (wparam)
        {
        case DBT_DEVICEARRIVAL:
            iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;
            provider_create( iface->dbcc_name );
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;
            provider_remove( iface->dbcc_name );
            break;
        default: break;
        }
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void initialize_providers( void )
{
    char buffer[offsetof( SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[MAX_PATH] )];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    GUID guid = GUID_DEVINTERFACE_WINEXINPUT;
    HDEVINFO set;
    DWORD i = 0;

    set = SetupDiGetClassDevsW( NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );

    while (SetupDiEnumDeviceInterfaces( set, NULL, &guid, i++, &iface ))
    {
        detail->cbSize = sizeof(*detail);
        if (!SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, NULL )) continue;
        provider_create( detail->DevicePath );
    }

    HidD_GetHidGuid( &guid );

    while (SetupDiEnumDeviceInterfaces( set, NULL, &guid, i++, &iface ))
    {
        detail->cbSize = sizeof(*detail);
        if (!SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, NULL )) continue;
        provider_create( detail->DevicePath );
    }

    SetupDiDestroyDeviceInfoList( set );
}

static DWORD WINAPI monitor_thread_proc( void *param )
{
    DEV_BROADCAST_DEVICEINTERFACE_W filter =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_W),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
    };
    WNDCLASSEXW wndclass =
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpszClassName = L"__wine_gaming_input_devnotify",
        .lpfnWndProc = devnotify_wndproc,
    };
    HANDLE start_event = param;
    HDEVNOTIFY devnotify;
    HMODULE module;
    HWND hwnd;
    MSG msg;

    SetThreadDescription( GetCurrentThread(), L"wine_wginput_worker" );

    GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (void *)windows_gaming_input, &module );
    RegisterClassExW( &wndclass );
    hwnd = CreateWindowExW( 0, wndclass.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
    devnotify = RegisterDeviceNotificationW( hwnd, &filter, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES );
    initialize_providers();
    SetEvent( start_event );

    do
    {
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    } while (!MsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE ));

    UnregisterDeviceNotification( devnotify );
    DestroyWindow( hwnd );
    UnregisterClassW( wndclass.lpszClassName, NULL );

    FreeLibraryAndExitThread( module, 0 );
    return 0;
}

static BOOL WINAPI start_monitor_thread( INIT_ONCE *once, void *param, void **context )
{
    HANDLE thread, start_event;

    start_event = CreateEventA( NULL, FALSE, FALSE, NULL );
    if (!start_event) ERR( "Failed to create start event, error %lu\n", GetLastError() );

    thread = CreateThread( NULL, 0, monitor_thread_proc, start_event, 0, NULL );
    if (!thread) ERR( "Failed to create monitor thread, error %lu\n", GetLastError() );
    else
    {
        WaitForSingleObject( start_event, INFINITE );
        CloseHandle( thread );
    }

    CloseHandle( start_event );
    return !!thread;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING class_str, IActivationFactory **factory )
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    const WCHAR *buffer = WindowsGetStringRawBuffer( class_str, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_w(buffer), factory );

    InitOnceExecuteOnce( &init_once, start_monitor_thread, NULL, NULL );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_RawGameController ))
        ICustomGameControllerFactory_QueryInterface( controller_factory, &IID_IActivationFactory, (void **)factory );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_Gamepad ))
        ICustomGameControllerFactory_QueryInterface( gamepad_factory, &IID_IActivationFactory, (void **)factory );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_RacingWheel ))
        ICustomGameControllerFactory_QueryInterface( racing_wheel_factory, &IID_IActivationFactory, (void **)factory );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_Custom_GameControllerFactoryManager ))
        IGameControllerFactoryManagerStatics2_QueryInterface( manager_factory, &IID_IActivationFactory, (void **)factory );

    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConstantForceEffect ))
        IInspectable_QueryInterface( constant_effect_factory, &IID_IActivationFactory, (void **)factory );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_ForceFeedback_RampForceEffect ))
        IInspectable_QueryInterface( ramp_effect_factory, &IID_IActivationFactory, (void **)factory );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_ForceFeedback_PeriodicForceEffect ))
        IInspectable_QueryInterface( periodic_effect_factory, &IID_IActivationFactory, (void **)factory );
    if (!wcscmp( buffer, RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConditionForceEffect ))
        IInspectable_QueryInterface( condition_effect_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    TRACE( "instance %p, reason %lu, reserved %p.\n", instance, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( instance );
        windows_gaming_input = instance;
        break;
    }
    return TRUE;
}
