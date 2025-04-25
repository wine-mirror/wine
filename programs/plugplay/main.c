/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <dbt.h>
#include "winsvc.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "plugplay.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

static WCHAR plugplayW[] = L"PlugPlay";

static SERVICE_STATUS_HANDLE service_handle;
static HANDLE stop_event;

void  __RPC_FAR * __RPC_USER MIDL_user_allocate( SIZE_T len )
{
    return malloc( len );
}

void __RPC_USER MIDL_user_free( void __RPC_FAR *ptr )
{
    free( ptr );
}

static CRITICAL_SECTION plugplay_cs;
static CRITICAL_SECTION_DEBUG plugplay_cs_debug =
{
    0, 0, &plugplay_cs,
    { &plugplay_cs_debug.ProcessLocksList, &plugplay_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": plugplay_cs") }
};
static CRITICAL_SECTION plugplay_cs = { &plugplay_cs_debug, -1, 0, 0, 0, 0 };

static struct list listener_list = LIST_INIT(listener_list);

struct listener
{
    struct list entry;
    struct list events;
    CONDITION_VARIABLE cv;
};

struct event
{
    struct list entry;
    DWORD code;
    BYTE *data;
    WCHAR *path;
    unsigned int size;
};


static void destroy_listener( struct listener *listener )
{
    struct event *event, *next;

    EnterCriticalSection( &plugplay_cs );
    list_remove( &listener->entry );
    LeaveCriticalSection( &plugplay_cs );

    LIST_FOR_EACH_ENTRY_SAFE(event, next, &listener->events, struct event, entry)
    {
        MIDL_user_free( event->data );
        list_remove( &event->entry );
        free( event );
    }
    free( listener );
}

void __RPC_USER plugplay_rpc_handle_rundown( plugplay_rpc_handle handle )
{
    destroy_listener( handle );
}

plugplay_rpc_handle __cdecl plugplay_register_listener(void)
{
    struct listener *listener;

    if (!(listener = calloc( 1, sizeof(*listener) )))
        return NULL;

    list_init( &listener->events );
    InitializeConditionVariable( &listener->cv );

    EnterCriticalSection( &plugplay_cs );
    list_add_tail( &listener_list, &listener->entry );
    LeaveCriticalSection( &plugplay_cs );

    return listener;
}

DWORD __cdecl plugplay_get_event( plugplay_rpc_handle handle, WCHAR **path, BYTE **data, unsigned int *size )
{
    struct listener *listener = handle;
    struct event *event;
    struct list *entry;
    DWORD ret;

    EnterCriticalSection( &plugplay_cs );

    while (!(entry = list_head( &listener->events )))
        SleepConditionVariableCS( &listener->cv, &plugplay_cs, INFINITE );

    event = LIST_ENTRY(entry, struct event, entry);
    list_remove( &event->entry );

    LeaveCriticalSection( &plugplay_cs );

    ret = event->code;
    *path = event->path;
    *data = event->data;
    *size = event->size;
    free( event );
    return ret;
}

void __cdecl plugplay_unregister_listener( plugplay_rpc_handle handle )
{
    destroy_listener( handle );
}

void __cdecl plugplay_send_event( const WCHAR *path, DWORD code, const BYTE *data, unsigned int size )
{
    struct listener *listener;
    struct event *event;
    const DEV_BROADCAST_HDR *header = (const DEV_BROADCAST_HDR *)data;

    if (header->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
    {
        BroadcastSystemMessageW( 0, NULL, WM_DEVICECHANGE, code, (LPARAM)data );
        BroadcastSystemMessageW( 0, NULL, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0 );
    }

    EnterCriticalSection( &plugplay_cs );

    LIST_FOR_EACH_ENTRY(listener, &listener_list, struct listener, entry)
    {
        if (!(event = malloc( sizeof(*event) )))
            break;

        if (!(event->data = malloc( size )))
        {
            free( event );
            break;
        }

        event->path = wcsdup( path );
        event->code = code;
        memcpy( event->data, data, size );
        event->size = size;
        list_add_tail( &listener->events, &event->entry );
        WakeConditionVariable( &listener->cv );
    }

    LeaveCriticalSection( &plugplay_cs );
}

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        WINE_TRACE( "shutting down\n" );
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        SetEvent( stop_event );
        return NO_ERROR;
    default:
        WINE_FIXME( "got service ctrl %lx\n", ctrl );
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI ServiceMain( DWORD argc, LPWSTR *argv )
{
    unsigned char endpoint[] = "\\pipe\\wine_plugplay";
    unsigned char protseq[] = "ncacn_np";
    SERVICE_STATUS status;
    RPC_STATUS err;

    WINE_TRACE( "starting service\n" );

    if ((err = RpcServerUseProtseqEpA( protseq, 0, endpoint, NULL )))
    {
        ERR("RpcServerUseProtseqEp() failed, error %lu\n", err);
        return;
    }
    if ((err = RpcServerRegisterIf( plugplay_v0_0_s_ifspec, NULL, NULL )))
    {
        ERR("RpcServerRegisterIf() failed, error %lu\n", err);
        return;
    }
    if ((err = RpcServerListen( 1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE )))
    {
        ERR("RpcServerListen() failed, error %lu\n", err);
        return;
    }

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );

    service_handle = RegisterServiceCtrlHandlerExW( plugplayW, service_handler, NULL );
    if (!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    SetServiceStatus( service_handle, &status );

    WaitForSingleObject( stop_event, INFINITE );

    RpcMgmtStopServerListening( NULL );
    RpcServerUnregisterIf( plugplay_v0_0_s_ifspec, NULL, TRUE );
    RpcMgmtWaitServerListen();

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus( service_handle, &status );
    WINE_TRACE( "service stopped\n" );
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    static const SERVICE_TABLE_ENTRYW service_table[] =
    {
        { plugplayW, ServiceMain },
        { NULL, NULL }
    };

    StartServiceCtrlDispatcherW( service_table );
    return 0;
}
