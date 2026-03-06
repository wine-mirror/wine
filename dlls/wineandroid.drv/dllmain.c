/*
 * wineandroid.drv entry points
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
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
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);


extern NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event );
static HANDLE stop_event;


static NTSTATUS WINAPI ioctl_callback( DEVICE_OBJECT *device, IRP *irp )
{
    struct ioctl_params params = { .irp = irp, .client_id = HandleToUlong(PsGetCurrentProcessId()) };
    NTSTATUS status = ANDROID_CALL( dispatch_ioctl, &params );
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS CALLBACK init_android_driver( DRIVER_OBJECT *driver, UNICODE_STRING *name )
{
    UNICODE_STRING nameW = RTL_CONSTANT_STRING( L"\\Device\\WineAndroid" );
    DEVICE_OBJECT *device;

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl_callback;

    return IoCreateDevice( driver, 0, &nameW, 0, 0, FALSE, &device );
}

static DWORD CALLBACK device_thread( void *arg )
{
    HANDLE start_event = arg;
    UNICODE_STRING nameW = RTL_CONSTANT_STRING( L"\\Driver\\WineAndroid" );
    NTSTATUS status;
    DWORD ret;

    TRACE( "starting process %lx\n", GetCurrentProcessId() );

    if (ANDROID_CALL( java_init, NULL )) return 0;  /* not running under Java */

    if ((status = IoCreateDriver( &nameW, init_android_driver )))
    {
        FIXME( "failed to create driver error %lx\n", status );
        return status;
    }

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    SetEvent( start_event );

    ret = wine_ntoskrnl_main_loop( stop_event );

    ANDROID_CALL( java_uninit, NULL );
    return ret;
}

static NTSTATUS WINAPI android_start_device(void *param, ULONG size)
{
    HANDLE handles[2];

    handles[0] = CreateEventW( NULL, TRUE, FALSE, NULL );
    handles[1] = CreateThread( NULL, 0, device_thread, handles[0], 0, NULL );
    WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
    CloseHandle( handles[0] );
    return NtCallbackReturn( &handles[1], sizeof(handles[1]), STATUS_SUCCESS );
}


static void CALLBACK register_window_callback( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    struct register_window_params params = { .arg1 = arg1, .arg2 = arg2, .arg3 = arg3 };
    ANDROID_CALL( register_window, &params );
}


/***********************************************************************
 *       dll initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    struct init_params params;

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( inst );
    if (__wine_init_unix_call()) return FALSE;

    params.register_window_callback = register_window_callback;
    params.start_device_callback = (UINT_PTR)android_start_device;
    return !ANDROID_CALL( init, &params );
}
