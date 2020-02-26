/*
 * Service process to load a kernel driver
 *
 * Copyright 2007 Alexandre Julliard
 * Copyright 2016 Sebastian Lackner
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
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/svcctl.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedevice);

static const WCHAR servicesW[] = {'\\','R','e','g','i','s','t','r','y',
                                  '\\','M','a','c','h','i','n','e',
                                  '\\','S','y','s','t','e','m',
                                  '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
                                  '\\','S','e','r','v','i','c','e','s',
                                  '\\',0};

extern NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event );

static const WCHAR winedeviceW[] = {'w','i','n','e','d','e','v','i','c','e',0};
static SERVICE_STATUS_HANDLE service_handle;
static SC_HANDLE manager_handle;
static HANDLE stop_event;

/* helper function to update service status */
static void set_service_status( SERVICE_STATUS_HANDLE handle, DWORD state, DWORD accepted )
{
    SERVICE_STATUS status;
    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = state;
    status.dwControlsAccepted        = accepted;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = (state == SERVICE_START_PENDING) ? 10000 : 0;
    SetServiceStatus( handle, &status );
}

static DWORD device_handler( DWORD ctrl, const WCHAR *driver_name )
{
    UNICODE_STRING service_name;
    DWORD result = NO_ERROR;
    WCHAR *str;

    if (!(str = heap_alloc( sizeof(servicesW) + lstrlenW(driver_name)*sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    lstrcpyW( str, servicesW );
    lstrcatW( str, driver_name );
    RtlInitUnicodeString( &service_name, str );

    switch (ctrl)
    {
    case SERVICE_CONTROL_START:
        result = RtlNtStatusToDosError(ZwLoadDriver( &service_name ));
        break;

    case SERVICE_CONTROL_STOP:
        result = RtlNtStatusToDosError(ZwUnloadDriver( &service_name ));
        break;

    default:
        FIXME( "got driver ctrl %x for %s\n", ctrl, wine_dbgstr_w(driver_name) );
        break;
    }

    RtlFreeUnicodeString( &service_name );
    return result;
}

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    const WCHAR *service_group = context;

    if (ctrl & SERVICE_CONTROL_FORWARD_FLAG)
    {
        if (!event_data) return ERROR_INVALID_PARAMETER;
        return device_handler( ctrl & ~SERVICE_CONTROL_FORWARD_FLAG, (const WCHAR *)event_data );
    }

    switch (ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        TRACE( "shutting down %s\n", wine_dbgstr_w(service_group) );
        set_service_status( service_handle, SERVICE_STOP_PENDING, 0 );
        SetEvent( stop_event );
        return NO_ERROR;
    default:
        FIXME( "got service ctrl %x for %s\n", ctrl, wine_dbgstr_w(service_group) );
        set_service_status( service_handle, SERVICE_RUNNING,
                            SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN );
        return NO_ERROR;
    }
}

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    const WCHAR *service_group = (argc >= 2) ? argv[1] : argv[0];

    if (!(stop_event = CreateEventW( NULL, TRUE, FALSE, NULL )))
        return;
    if (!(manager_handle = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT )))
        return;
    if (!(service_handle = RegisterServiceCtrlHandlerExW( winedeviceW, service_handler, (void *)service_group )))
        return;

    TRACE( "starting service group %s\n", wine_dbgstr_w(service_group) );
    set_service_status( service_handle, SERVICE_RUNNING,
                        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN );

    wine_ntoskrnl_main_loop( stop_event );

    TRACE( "service group %s stopped\n", wine_dbgstr_w(service_group) );
    set_service_status( service_handle, SERVICE_STOPPED, 0 );
    CloseServiceHandle( manager_handle );
    CloseHandle( stop_event );
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    SERVICE_TABLE_ENTRYW service_table[2];

    service_table[0].lpServiceName = (void *)winedeviceW;
    service_table[0].lpServiceProc = ServiceMain;
    service_table[1].lpServiceName = NULL;
    service_table[1].lpServiceProc = NULL;

    StartServiceCtrlDispatcherW( service_table );
    return 0;
}
