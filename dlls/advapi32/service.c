/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 2005 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "winsvc.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static const WCHAR szServiceManagerKey[] = { 'S','y','s','t','e','m','\\',
      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
      'S','e','r','v','i','c','e','s','\\',0 };
static const WCHAR  szSCMLock[] = {'A','D','V','A','P','I','_','S','C','M',
                                   'L','O','C','K',0};
static const WCHAR  szServiceShmemNameFmtW[] = {'A','D','V','A','P','I','_',
                                                'S','E','B','_','%','s',0};
static const WCHAR  szServiceDispEventNameFmtW[] = {'A','D','V','A','P','I','_',
                                                    'D','I','S','P','_','%','s',0};
static const WCHAR  szServiceMutexNameFmtW[] = {'A','D','V','A','P','I','_',
                                                'M','U','X','_','%','s',0};
static const WCHAR  szServiceAckEventNameFmtW[] = {'A','D','V','A','P','I','_',
                                                   'A','C','K','_','%','s',0};
static const WCHAR  szWaitServiceStartW[]  = {'A','D','V','A','P','I','_','W',
                                              'a','i','t','S','e','r','v','i',
                                              'c','e','S','t','a','r','t',0};

struct SEB              /* service environment block */
{                       /*   resides in service's shared memory object */
    DWORD control_code;      /* service control code */
    DWORD dispatcher_error;  /* set by dispatcher if it fails to invoke control handler */
    SERVICE_STATUS status;
    DWORD argc;
    /* variable part of SEB contains service arguments */
};

static HANDLE dispatcher_event;  /* this is used by service thread to wakeup
                                  * service control dispatcher when thread terminates */

static struct service_thread_data *service;  /* FIXME: this should be a list */

/******************************************************************************
 * SC_HANDLEs
 */

#define MAX_SERVICE_NAME 256

typedef enum { SC_HTYPE_MANAGER, SC_HTYPE_SERVICE } SC_HANDLE_TYPE;

struct sc_handle;
typedef VOID (*sc_handle_destructor)(struct sc_handle *);

struct sc_handle
{
    SC_HANDLE_TYPE htype;
    DWORD ref_count;
    sc_handle_destructor destroy;
};

struct sc_manager       /* service control manager handle */
{
    struct sc_handle hdr;
    HKEY   hkey;   /* handle to services database in the registry */
};

struct sc_service       /* service handle */
{
    struct sc_handle hdr;
    HKEY   hkey;          /* handle to service entry in the registry (under hkey) */
    struct sc_manager *scm;  /* pointer to SCM handle */
    WCHAR  name[1];
};

static void *sc_handle_alloc(SC_HANDLE_TYPE htype, DWORD size,
                             sc_handle_destructor destroy)
{
    struct sc_handle *hdr;

    hdr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
    if (hdr)
    {
        hdr->htype = htype;
        hdr->ref_count = 1;
        hdr->destroy = destroy;
    }
    TRACE("sc_handle type=%d -> %p\n", htype, hdr);
    return hdr;
}

static void *sc_handle_get_handle_data(SC_HANDLE handle, DWORD htype)
{
    struct sc_handle *hdr = (struct sc_handle *) handle;

    if (!hdr)
        return NULL;
    if (hdr->htype != htype)
        return NULL;
    return hdr;
}

static void sc_handle_free(struct sc_handle* hdr)
{
    if (!hdr)
        return;
    if (--hdr->ref_count)
        return;
    hdr->destroy(hdr);
    HeapFree(GetProcessHeap(), 0, hdr);
}

static void sc_handle_destroy_manager(struct sc_handle *handle)
{
    struct sc_manager *mgr = (struct sc_manager*) handle;

    TRACE("destroying SC Manager %p\n", mgr);
    if (mgr->hkey)
        RegCloseKey(mgr->hkey);
}

static void sc_handle_destroy_service(struct sc_handle *handle)
{
    struct sc_service *svc = (struct sc_service*) handle;
    
    TRACE("destroying service %p\n", svc);
    if (svc->hkey)
        RegCloseKey(svc->hkey);
    svc->hkey = NULL;
    sc_handle_free(&svc->scm->hdr);
    svc->scm = NULL;
}

/******************************************************************************
 * String management functions
 */
static inline LPWSTR SERV_dup( LPCSTR str )
{
    UINT len;
    LPWSTR wstr;

    if( !str )
        return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, len );
    return wstr;
}

static inline LPWSTR SERV_dupmulti( LPCSTR str )
{
    UINT len = 0, n = 0;
    LPWSTR wstr;

    if( !str )
        return NULL;
    do {
        len += MultiByteToWideChar( CP_ACP, 0, &str[n], -1, NULL, 0 );
        n += (strlen( &str[n] ) + 1);
    } while (str[n]);
    len++;
    n++;

    wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, n, wstr, len );
    return wstr;
}

static inline VOID SERV_free( LPWSTR wstr )
{
    HeapFree( GetProcessHeap(), 0, wstr );
}

/******************************************************************************
 * read_scm_lock_data
 *
 * helper function for service control dispatcher
 *
 * SCM database is locked by StartService;
 * open global SCM lock object and read service name
 */
static BOOL read_scm_lock_data( LPWSTR buffer )
{
    HANDLE hLock;
    LPWSTR argptr;

    hLock = OpenFileMappingW( FILE_MAP_ALL_ACCESS, FALSE, szSCMLock );
    if( NULL == hLock )
    {
        SetLastError( ERROR_FAILED_SERVICE_CONTROLLER_CONNECT );
        return FALSE;
    }
    argptr = MapViewOfFile( hLock, FILE_MAP_ALL_ACCESS,
                            0, 0, MAX_SERVICE_NAME * sizeof(WCHAR) );
    if( NULL == argptr )
    {
        CloseHandle( hLock );
        return FALSE;
    }
    strcpyW( buffer, argptr );
    UnmapViewOfFile( argptr );
    CloseHandle( hLock );
    return TRUE;
}

/******************************************************************************
 * open_seb_shmem
 *
 * helper function for service control dispatcher
 */
static struct SEB* open_seb_shmem( LPWSTR service_name, HANDLE* hServiceShmem )
{
    WCHAR object_name[ MAX_PATH ];
    HANDLE hmem;
    struct SEB *ret;

    snprintfW( object_name, MAX_PATH, szServiceShmemNameFmtW, service_name );
    hmem = OpenFileMappingW( FILE_MAP_ALL_ACCESS, FALSE, object_name );
    if( NULL == hmem )
        return NULL;

    ret = MapViewOfFile( hmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    if( NULL == ret )
        CloseHandle( hmem );
    else
        *hServiceShmem = hmem;
    return ret;
}

/******************************************************************************
 * build_arg_vectors
 *
 * helper function for start_new_service
 *
 * Allocate and initialize array of LPWSTRs to arguments in variable part
 * of service environment block.
 * First entry in the array is reserved for service name and not initialized.
 */
static LPWSTR* build_arg_vectors( struct SEB* seb )
{
    LPWSTR *ret;
    LPWSTR argptr;
    DWORD i;

    ret = HeapAlloc( GetProcessHeap(), 0, (1 + seb->argc) * sizeof(LPWSTR) );
    if( NULL == ret )
        return NULL;

    argptr = (LPWSTR) &seb[1];
    for( i = 0; i < seb->argc; i++ )
    {
        ret[ 1 + i ] = argptr;
        argptr += 1 + strlenW( argptr );
    }
    return ret;
}

/******************************************************************************
 * service thread
 */
struct service_thread_data
{
    WCHAR service_name[ MAX_SERVICE_NAME ];
    CHAR service_nameA[ MAX_SERVICE_NAME ];
    LPSERVICE_MAIN_FUNCTIONW service_main;
    DWORD argc;
    LPWSTR *argv;
    HANDLE hServiceShmem;
    struct SEB *seb;
    HANDLE thread_handle;
    HANDLE mutex;            /* provides serialization of control request */
    HANDLE ack_event;        /* control handler completion acknowledgement */
    LPHANDLER_FUNCTION ctrl_handler;
};

static DWORD WINAPI service_thread( LPVOID arg )
{
    struct service_thread_data *data = arg;

    data->service_main( data->argc, data->argv );
    SetEvent( dispatcher_event );
    return 0;
}

/******************************************************************************
 * dispose_service_thread_data
 *
 * helper function for service control dispatcher
 */
static void dispose_service_thread_data( struct service_thread_data* thread_data )
{
    if( thread_data->mutex ) CloseHandle( thread_data->mutex );
    if( thread_data->ack_event ) CloseHandle( thread_data->ack_event );
    HeapFree( GetProcessHeap(), 0, thread_data->argv );
    if( thread_data->seb ) UnmapViewOfFile( thread_data->seb );
    if( thread_data->hServiceShmem ) CloseHandle( thread_data->hServiceShmem );
    HeapFree( GetProcessHeap(), 0, thread_data );
}

/******************************************************************************
 * start_new_service
 *
 * helper function for service control dispatcher
 */
static struct service_thread_data*
start_new_service( LPSERVICE_MAIN_FUNCTIONW service_main, BOOL ascii )
{
    struct service_thread_data *thread_data;
    unsigned int i;
    WCHAR object_name[ MAX_PATH ];

    thread_data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct service_thread_data) );
    if( NULL == thread_data )
        return NULL;

    if( ! read_scm_lock_data( thread_data->service_name ) )
    {
        /* FIXME: Instead of exiting we allow
           service to be executed as ordinary program.
           This behaviour was specially introduced in the patch
           submitted against revision 1.45 and so preserved here.
         */
        FIXME("should fail with ERROR_FAILED_SERVICE_CONTROLLER_CONNECT\n");
        service_main( 0, NULL );
        HeapFree( GetProcessHeap(), 0, thread_data );
        return NULL;
    }

    thread_data->seb = open_seb_shmem( thread_data->service_name, &thread_data->hServiceShmem );
    if( NULL == thread_data->seb )
        goto error;

    thread_data->argv = build_arg_vectors( thread_data->seb );
    if( NULL == thread_data->argv )
        goto error;

    thread_data->argv[0] = thread_data->service_name;
    thread_data->argc = thread_data->seb->argc + 1;

    if( ascii )
    {
        /* Convert the Unicode arg vectors back to ASCII;
         * but we'll need unicode service name (argv[0]) for object names */
        WideCharToMultiByte( CP_ACP, 0, thread_data->argv[0], -1,
                             thread_data->service_nameA, MAX_SERVICE_NAME, NULL, NULL );
        thread_data->argv[0] = (LPWSTR) thread_data->service_nameA;

        for(i=1; i<thread_data->argc; i++)
        {
            LPWSTR src = thread_data->argv[i];
            int len = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
            LPSTR dest = HeapAlloc( GetProcessHeap(), 0, len );
            if( NULL == dest )
                goto error;
            WideCharToMultiByte( CP_ACP, 0, src, -1, dest, len, NULL, NULL );
            /* copy converted string back  */
            memcpy( src, dest, len );
            HeapFree( GetProcessHeap(), 0, dest );
        }
    }

    /* init status according to docs for StartService */
    thread_data->seb->status.dwCurrentState = SERVICE_START_PENDING;
    thread_data->seb->status.dwControlsAccepted = 0;
    thread_data->seb->status.dwCheckPoint = 0;
    thread_data->seb->status.dwWaitHint = 2000;

    /* create service mutex; mutex is initially owned */
    snprintfW( object_name, MAX_PATH, szServiceMutexNameFmtW, thread_data->service_name );
    thread_data->mutex = CreateMutexW( NULL, TRUE, object_name );
    if( NULL == thread_data->mutex )
        goto error;

    if( ERROR_ALREADY_EXISTS == GetLastError() )
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        goto error;
    }

    /* create service event */
    snprintfW( object_name, MAX_PATH, szServiceAckEventNameFmtW, thread_data->service_name );
    thread_data->ack_event = CreateEventW( NULL, FALSE, FALSE, object_name );
    if( NULL == thread_data->ack_event )
        goto error;

    if( ERROR_ALREADY_EXISTS == GetLastError() )
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        goto error;
    }

    /* create service thread in suspended state
     * to avoid race while caller handles return value */
    thread_data->service_main = service_main;
    thread_data->thread_handle = CreateThread( NULL, 0, service_thread,
                                               thread_data, CREATE_SUSPENDED, NULL );
    if( thread_data->thread_handle )
        return thread_data;

error:
    dispose_service_thread_data( thread_data );
    return FALSE;
}

/******************************************************************************
 * service_ctrl_dispatcher
 */
static BOOL service_ctrl_dispatcher( LPSERVICE_TABLE_ENTRYW servent, BOOL ascii )
{
    WCHAR object_name[ MAX_PATH ];
    HANDLE wait;

    /* FIXME: if shared service, find entry by service name */

    /* FIXME: move this into dispatcher loop */
    service = start_new_service( servent->lpServiceProc, ascii );
    if( NULL == service )
        return FALSE;

    ResumeThread( service->thread_handle );

    /* create dispatcher event object */
    /* FIXME: object_name should be based on executable image path because
     * this object is common for all services in the process */
    /* But what if own and shared services have the same executable? */
    snprintfW( object_name, MAX_PATH, szServiceDispEventNameFmtW, service->service_name );
    dispatcher_event = CreateEventW( NULL, FALSE, FALSE, object_name );
    if( NULL == dispatcher_event )
    {
        dispose_service_thread_data( service );
        return FALSE;
    }

    if( ERROR_ALREADY_EXISTS == GetLastError() )
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        CloseHandle( dispatcher_event );
        return FALSE;
    }

    /* ready to accept control requests */
    ReleaseMutex( service->mutex );

    /* signal for StartService */
    wait = OpenSemaphoreW( SEMAPHORE_MODIFY_STATE, FALSE, szWaitServiceStartW );
    if( wait )
    {
        ReleaseSemaphore( wait, 1, NULL );
        CloseHandle( wait );
    }

    /* dispatcher loop */
    for(;;)
    {
        DWORD ret;

        WaitForSingleObject( dispatcher_event, INFINITE );

        /* at first, look for terminated service thread
         * FIXME: threads, if shared service */
        if( !GetExitCodeThread( service->thread_handle, &ret ) )
            ERR("Couldn't get thread exit code\n");
        else if( ret != STILL_ACTIVE )
        {
            CloseHandle( service->thread_handle );
            dispose_service_thread_data( service );
            break;
        }

        /* look for control requests */
        if( service->seb->control_code )
        {
            if( NULL == service->ctrl_handler )
                service->seb->dispatcher_error = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
            else
            {
                service->ctrl_handler( service->seb->control_code );
                service->seb->dispatcher_error = 0;
            }
            service->seb->control_code = 0;
            SetEvent( service->ack_event );
        }

        /* FIXME: if shared service, check SCM lock object;
         * if exists, a new service should be started */
    }

    CloseHandle( dispatcher_event );
    return TRUE;
}

/******************************************************************************
 * StartServiceCtrlDispatcherA [ADVAPI32.@]
 */
BOOL WINAPI
StartServiceCtrlDispatcherA( LPSERVICE_TABLE_ENTRYA servent )
{
    int count, i;
    LPSERVICE_TABLE_ENTRYW ServiceTableW;
    BOOL ret;

    TRACE("(%p)\n", servent);

    /* convert service table to unicode */
    for( count = 0; servent[ count ].lpServiceName; )
        count++;
    ServiceTableW = HeapAlloc( GetProcessHeap(), 0, (count + 1) * sizeof(SERVICE_TABLE_ENTRYW) );
    if( NULL == ServiceTableW )
        return FALSE;

    for( i = 0; i < count; i++ )
    {
        ServiceTableW[ i ].lpServiceName = SERV_dup( servent[ i ].lpServiceName );
        ServiceTableW[ i ].lpServiceProc = (LPSERVICE_MAIN_FUNCTIONW) servent[ i ].lpServiceProc;
    }
    ServiceTableW[ count ].lpServiceName = NULL;
    ServiceTableW[ count ].lpServiceProc = NULL;

    /* start dispatcher */
    ret = service_ctrl_dispatcher( ServiceTableW, TRUE );

    /* free service table */
    for( i = 0; i < count; i++ )
        SERV_free( ServiceTableW[ i ].lpServiceName );
    HeapFree( GetProcessHeap(), 0, ServiceTableW );
    return ret;
}

/******************************************************************************
 * StartServiceCtrlDispatcherW [ADVAPI32.@]
 *
 * PARAMS
 *   servent []
 */
BOOL WINAPI
StartServiceCtrlDispatcherW( LPSERVICE_TABLE_ENTRYW servent )
{
    TRACE("(%p)\n", servent);
    return service_ctrl_dispatcher( servent, FALSE );
}

/******************************************************************************
 * LockServiceDatabase  [ADVAPI32.@]
 */
LPVOID WINAPI LockServiceDatabase (SC_HANDLE hSCManager)
{
    HANDLE ret;

    TRACE("%p\n",hSCManager);

    ret = CreateFileMappingW( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                              0, MAX_SERVICE_NAME * sizeof(WCHAR), szSCMLock );
    if( ret && GetLastError() == ERROR_ALREADY_EXISTS )
    {
        CloseHandle( ret );
        ret = NULL;
        SetLastError( ERROR_SERVICE_DATABASE_LOCKED );
    }

    TRACE("returning %p\n", ret);

    return ret;
}

/******************************************************************************
 * UnlockServiceDatabase  [ADVAPI32.@]
 */
BOOL WINAPI UnlockServiceDatabase (LPVOID ScLock)
{
    TRACE("%p\n",ScLock);

    return CloseHandle( (HANDLE) ScLock );
}

/******************************************************************************
 * RegisterServiceCtrlHandlerA [ADVAPI32.@]
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerA( LPCSTR lpServiceName,
                             LPHANDLER_FUNCTION lpfHandler )
{
    LPWSTR lpServiceNameW;
    SERVICE_STATUS_HANDLE ret;

    lpServiceNameW = SERV_dup(lpServiceName);
    ret = RegisterServiceCtrlHandlerW( lpServiceNameW, lpfHandler );
    SERV_free(lpServiceNameW);
    return ret;
}

/******************************************************************************
 * RegisterServiceCtrlHandlerW [ADVAPI32.@]
 *
 * PARAMS
 *   lpServiceName []
 *   lpfHandler    []
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerW( LPCWSTR lpServiceName,
                             LPHANDLER_FUNCTION lpfHandler )
{
    /* FIXME: find service thread data by service name */

    service->ctrl_handler = lpfHandler;
    return 0xcacacafe;
}

/******************************************************************************
 * SetServiceStatus [ADVAPI32.@]
 *
 * PARAMS
 *   hService []
 *   lpStatus []
 */
BOOL WINAPI
SetServiceStatus( SERVICE_STATUS_HANDLE hService, LPSERVICE_STATUS lpStatus )
{
    DWORD r;

    TRACE("\tType:%lx\n",lpStatus->dwServiceType);
    TRACE("\tState:%lx\n",lpStatus->dwCurrentState);
    TRACE("\tControlAccepted:%lx\n",lpStatus->dwControlsAccepted);
    TRACE("\tExitCode:%lx\n",lpStatus->dwWin32ExitCode);
    TRACE("\tServiceExitCode:%lx\n",lpStatus->dwServiceSpecificExitCode);
    TRACE("\tCheckPoint:%lx\n",lpStatus->dwCheckPoint);
    TRACE("\tWaitHint:%lx\n",lpStatus->dwWaitHint);

    /* FIXME: find service thread data by status handle */

    /* acquire mutex; note that mutex may already be owned
     * when service handles control request
     */
    r = WaitForSingleObject( service->mutex, 0 );
    memcpy( &service->seb->status, lpStatus, sizeof(SERVICE_STATUS) );
    if( WAIT_OBJECT_0 == r || WAIT_ABANDONED == r )
        ReleaseMutex( service->mutex );
    return TRUE;
}

/******************************************************************************
 * OpenSCManagerA [ADVAPI32.@]
 *
 * Establish a connection to the service control manager and open its database.
 *
 * PARAMS
 *   lpMachineName   [I] Pointer to machine name string
 *   lpDatabaseName  [I] Pointer to database name string
 *   dwDesiredAccess [I] Type of access
 *
 * RETURNS
 *   Success: A Handle to the service control manager database
 *   Failure: NULL
 */
SC_HANDLE WINAPI OpenSCManagerA( LPCSTR lpMachineName, LPCSTR lpDatabaseName,
                                 DWORD dwDesiredAccess )
{
    LPWSTR lpMachineNameW, lpDatabaseNameW;
    SC_HANDLE ret;

    lpMachineNameW = SERV_dup(lpMachineName);
    lpDatabaseNameW = SERV_dup(lpDatabaseName);
    ret = OpenSCManagerW(lpMachineNameW, lpDatabaseNameW, dwDesiredAccess);
    SERV_free(lpDatabaseNameW);
    SERV_free(lpMachineNameW);
    return ret;
}

/******************************************************************************
 * OpenSCManagerW [ADVAPI32.@]
 *
 * See OpenSCManagerA.
 */
SC_HANDLE WINAPI OpenSCManagerW( LPCWSTR lpMachineName, LPCWSTR lpDatabaseName,
                                 DWORD dwDesiredAccess )
{
    struct sc_manager *manager;
    HKEY hReg;
    LONG r;

    TRACE("(%s,%s,0x%08lx)\n", debugstr_w(lpMachineName),
          debugstr_w(lpDatabaseName), dwDesiredAccess);

    if( lpDatabaseName && lpDatabaseName[0] )
    {
        if( strcmpiW( lpDatabaseName, SERVICES_ACTIVE_DATABASEW ) == 0 )
        {
            /* noop, all right */
        }
        else if( strcmpiW( lpDatabaseName, SERVICES_FAILED_DATABASEW ) == 0 )
        {
            SetLastError( ERROR_DATABASE_DOES_NOT_EXIST );
            return NULL;
        }
        else
        {
            SetLastError( ERROR_INVALID_NAME );
            return NULL;
        }
    }

    manager = sc_handle_alloc( SC_HTYPE_MANAGER, sizeof (struct sc_manager),
                               sc_handle_destroy_manager );
    if (!manager)
         return NULL;

    r = RegConnectRegistryW(lpMachineName,HKEY_LOCAL_MACHINE,&hReg);
    if (r!=ERROR_SUCCESS)
        goto error;

    r = RegOpenKeyExW(hReg, szServiceManagerKey,
                      0, KEY_ALL_ACCESS, &manager->hkey);
    RegCloseKey( hReg );
    if (r!=ERROR_SUCCESS)
        goto error;

    TRACE("returning %p\n", manager);

    return (SC_HANDLE) &manager->hdr;

error:
    sc_handle_free( &manager->hdr );
    return NULL;
}

/******************************************************************************
 * ControlService [ADVAPI32.@]
 *
 * Send a control code to a service.
 *
 * PARAMS
 *   hService        [I] Handle of the service control manager database
 *   dwControl       [I] Control code to send (SERVICE_CONTROL_* flags from "winsvc.h")
 *   lpServiceStatus [O] Destination for the status of the service, if available
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 *
 * BUGS
 *   Unlike M$' implementation, control requests are not serialized and may be
 *   processed asynchronously.
 */
BOOL WINAPI ControlService( SC_HANDLE hService, DWORD dwControl,
                            LPSERVICE_STATUS lpServiceStatus )
{
    struct sc_service *hsvc;
    WCHAR object_name[ MAX_PATH ];
    HANDLE mutex = NULL, shmem = NULL;
    HANDLE disp_event = NULL, ack_event = NULL;
    struct SEB *seb = NULL;
    DWORD  r;
    BOOL ret = FALSE, mutex_owned = FALSE;

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    TRACE("%p(%s) %ld %p\n", hService, debugstr_w(hsvc->name),
          dwControl, lpServiceStatus);

    /* open and hold mutex */
    snprintfW( object_name, MAX_PATH, szServiceMutexNameFmtW, hsvc->name );
    mutex = OpenMutexW( MUTEX_ALL_ACCESS, FALSE, object_name );
    if( NULL == mutex )
    {
        SetLastError( ERROR_SERVICE_NOT_ACTIVE );
        return FALSE;
    }

    r = WaitForSingleObject( mutex, 30000 );
    if( WAIT_FAILED == r )
        goto done;

    if( WAIT_TIMEOUT == r )
    {
        SetLastError( ERROR_SERVICE_REQUEST_TIMEOUT );
        goto done;
    }
    mutex_owned = TRUE;

    /* open event objects */
    snprintfW( object_name, MAX_PATH, szServiceDispEventNameFmtW, hsvc->name );
    disp_event = OpenEventW( EVENT_ALL_ACCESS, FALSE, object_name );
    if( NULL == disp_event )
        goto done;

    snprintfW( object_name, MAX_PATH, szServiceAckEventNameFmtW, hsvc->name );
    ack_event = OpenEventW( EVENT_ALL_ACCESS, FALSE, object_name );
    if( NULL == ack_event )
        goto done;

    /* get service environment block */
    seb = open_seb_shmem( hsvc->name, &shmem );
    if( NULL == seb )
        goto done;

    /* send request */
    /* FIXME: check dwControl against controls accepted */
    seb->control_code = dwControl;
    SetEvent( disp_event );

    /* wait for acknowledgement */
    r = WaitForSingleObject( ack_event, 30000 );
    if( WAIT_FAILED == r )
        goto done;

    if( WAIT_TIMEOUT == r )
    {
        SetLastError( ERROR_SERVICE_REQUEST_TIMEOUT );
        goto done;
    }

    if( seb->dispatcher_error )
    {
        SetLastError( seb->dispatcher_error );
        goto done;
    }

    /* get status */
    if( lpServiceStatus )
        memcpy( lpServiceStatus, &seb->status, sizeof(SERVICE_STATUS) );
    ret = TRUE;

done:
    if( seb ) UnmapViewOfFile( seb );
    if( shmem ) CloseHandle( shmem );
    if( ack_event ) CloseHandle( ack_event );
    if( disp_event ) CloseHandle( disp_event );
    if( mutex_owned ) ReleaseMutex( mutex );
    if( mutex ) CloseHandle( mutex );
    return ret;
}

/******************************************************************************
 * CloseServiceHandle [ADVAPI32.@]
 * 
 * Close a handle to a service or the service control manager database.
 *
 * PARAMS
 *   hSCObject [I] Handle to service or service control manager database
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI
CloseServiceHandle( SC_HANDLE hSCObject )
{
    TRACE("%p\n", hSCObject);

    sc_handle_free( (struct sc_handle*) hSCObject );

    return TRUE;
}


/******************************************************************************
 * OpenServiceA [ADVAPI32.@]
 *
 * Open a handle to a service.
 *
 * PARAMS
 *   hSCManager      [I] Handle of the service control manager database
 *   lpServiceName   [I] Name of the service to open
 *   dwDesiredAccess [I] Access required to the service
 *
 * RETURNS
 *    Success: Handle to the service
 *    Failure: NULL
 */
SC_HANDLE WINAPI OpenServiceA( SC_HANDLE hSCManager, LPCSTR lpServiceName,
                               DWORD dwDesiredAccess )
{
    LPWSTR lpServiceNameW;
    SC_HANDLE ret;

    TRACE("%p %s %ld\n", hSCManager, debugstr_a(lpServiceName), dwDesiredAccess);

    lpServiceNameW = SERV_dup(lpServiceName);
    ret = OpenServiceW( hSCManager, lpServiceNameW, dwDesiredAccess);
    SERV_free(lpServiceNameW);
    return ret;
}


/******************************************************************************
 * OpenServiceW [ADVAPI32.@]
 *
 * See OpenServiceA.
 */
SC_HANDLE WINAPI OpenServiceW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
                               DWORD dwDesiredAccess)
{
    struct sc_manager *hscm;
    struct sc_service *hsvc;
    HKEY hKey;
    long r;
    DWORD len;

    TRACE("%p %s %ld\n", hSCManager, debugstr_w(lpServiceName), dwDesiredAccess);

    if (!lpServiceName)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return NULL;
    }

    hscm = sc_handle_get_handle_data( hSCManager, SC_HTYPE_MANAGER );
    if (!hscm)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    r = RegOpenKeyExW( hscm->hkey, lpServiceName, 0, KEY_ALL_ACCESS, &hKey );
    if (r!=ERROR_SUCCESS)
    {
        SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
        return NULL;
    }
    
    len = strlenW(lpServiceName)+1;
    hsvc = sc_handle_alloc( SC_HTYPE_SERVICE,
                            sizeof (struct sc_service) + len*sizeof(WCHAR),
                            sc_handle_destroy_service );
    if (!hsvc)
        return NULL;
    strcpyW( hsvc->name, lpServiceName );
    hsvc->hkey = hKey;

    /* add reference to SCM handle */
    hscm->hdr.ref_count++;
    hsvc->scm = hscm;

    TRACE("returning %p\n",hsvc);

    return (SC_HANDLE) &hsvc->hdr;
}

/******************************************************************************
 * CreateServiceW [ADVAPI32.@]
 */
SC_HANDLE WINAPI
CreateServiceW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
                  LPCWSTR lpDisplayName, DWORD dwDesiredAccess,
                  DWORD dwServiceType, DWORD dwStartType,
                  DWORD dwErrorControl, LPCWSTR lpBinaryPathName,
                  LPCWSTR lpLoadOrderGroup, LPDWORD lpdwTagId,
                  LPCWSTR lpDependencies, LPCWSTR lpServiceStartName,
                  LPCWSTR lpPassword )
{
    struct sc_manager *hscm;
    struct sc_service *hsvc = NULL;
    HKEY hKey;
    LONG r;
    DWORD dp, len;
    static const WCHAR szDisplayName[] = { 'D','i','s','p','l','a','y','N','a','m','e', 0 };
    static const WCHAR szType[] = {'T','y','p','e',0};
    static const WCHAR szStart[] = {'S','t','a','r','t',0};
    static const WCHAR szError[] = {'E','r','r','o','r','C','o','n','t','r','o','l', 0};
    static const WCHAR szImagePath[] = {'I','m','a','g','e','P','a','t','h',0};
    static const WCHAR szGroup[] = {'G','r','o','u','p',0};
    static const WCHAR szDependencies[] = { 'D','e','p','e','n','d','e','n','c','i','e','s',0};

    FIXME("%p %s %s\n", hSCManager, 
          debugstr_w(lpServiceName), debugstr_w(lpDisplayName));

    hscm = sc_handle_get_handle_data( hSCManager, SC_HTYPE_MANAGER );
    if (!hscm)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }

    r = RegCreateKeyExW(hscm->hkey, lpServiceName, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dp);
    if (r!=ERROR_SUCCESS)
        goto error;

    if (dp != REG_CREATED_NEW_KEY)
    {
        SetLastError(ERROR_SERVICE_EXISTS);
        goto error;
    }

    if(lpDisplayName)
    {
        r = RegSetValueExW(hKey, szDisplayName, 0, REG_SZ, (const BYTE*)lpDisplayName,
                           (strlenW(lpDisplayName)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            goto error;
    }

    r = RegSetValueExW(hKey, szType, 0, REG_DWORD, (LPVOID)&dwServiceType, sizeof (DWORD) );
    if (r!=ERROR_SUCCESS)
        goto error;

    r = RegSetValueExW(hKey, szStart, 0, REG_DWORD, (LPVOID)&dwStartType, sizeof (DWORD) );
    if (r!=ERROR_SUCCESS)
        goto error;

    r = RegSetValueExW(hKey, szError, 0, REG_DWORD,
                           (LPVOID)&dwErrorControl, sizeof (DWORD) );
    if (r!=ERROR_SUCCESS)
        goto error;

    if(lpBinaryPathName)
    {
        r = RegSetValueExW(hKey, szImagePath, 0, REG_SZ, (const BYTE*)lpBinaryPathName,
                           (strlenW(lpBinaryPathName)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            goto error;
    }

    if(lpLoadOrderGroup)
    {
        r = RegSetValueExW(hKey, szGroup, 0, REG_SZ, (const BYTE*)lpLoadOrderGroup,
                           (strlenW(lpLoadOrderGroup)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            goto error;
    }

    if(lpDependencies)
    {
        DWORD len = 0;

        /* determine the length of a double null terminated multi string */
        do {
            len += (strlenW(&lpDependencies[len])+1);
        } while (lpDependencies[len++]);

        r = RegSetValueExW(hKey, szDependencies, 0, REG_MULTI_SZ,
                           (const BYTE*)lpDependencies, len );
        if (r!=ERROR_SUCCESS)
            goto error;
    }

    if(lpPassword)
    {
        FIXME("Don't know how to add a Password for a service.\n");
    }

    if(lpServiceStartName)
    {
        FIXME("Don't know how to add a ServiceStartName for a service.\n");
    }

    len = strlenW(lpServiceName)+1;
    hsvc = sc_handle_alloc( SC_HTYPE_SERVICE,
                            sizeof (struct sc_service) + len*sizeof(WCHAR),
                            sc_handle_destroy_service );
    if (!hsvc)
        return NULL;
    strcpyW( hsvc->name, lpServiceName );
    hsvc->hkey = hKey;
    hsvc->scm = hscm;
    hscm->hdr.ref_count++;

    return (SC_HANDLE) &hsvc->hdr;
    
error:
    if (hsvc)
        sc_handle_free( &hsvc->hdr );
    return NULL;
}


/******************************************************************************
 * CreateServiceA [ADVAPI32.@]
 */
SC_HANDLE WINAPI
CreateServiceA( SC_HANDLE hSCManager, LPCSTR lpServiceName,
                  LPCSTR lpDisplayName, DWORD dwDesiredAccess,
                  DWORD dwServiceType, DWORD dwStartType,
                  DWORD dwErrorControl, LPCSTR lpBinaryPathName,
                  LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId,
                  LPCSTR lpDependencies, LPCSTR lpServiceStartName,
                  LPCSTR lpPassword )
{
    LPWSTR lpServiceNameW, lpDisplayNameW, lpBinaryPathNameW,
        lpLoadOrderGroupW, lpDependenciesW, lpServiceStartNameW, lpPasswordW;
    SC_HANDLE r;

    TRACE("%p %s %s\n", hSCManager,
          debugstr_a(lpServiceName), debugstr_a(lpDisplayName));

    lpServiceNameW = SERV_dup( lpServiceName );
    lpDisplayNameW = SERV_dup( lpDisplayName );
    lpBinaryPathNameW = SERV_dup( lpBinaryPathName );
    lpLoadOrderGroupW = SERV_dup( lpLoadOrderGroup );
    lpDependenciesW = SERV_dupmulti( lpDependencies );
    lpServiceStartNameW = SERV_dup( lpServiceStartName );
    lpPasswordW = SERV_dup( lpPassword );

    r = CreateServiceW( hSCManager, lpServiceNameW, lpDisplayNameW,
            dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
            lpBinaryPathNameW, lpLoadOrderGroupW, lpdwTagId,
            lpDependenciesW, lpServiceStartNameW, lpPasswordW );

    SERV_free( lpServiceNameW );
    SERV_free( lpDisplayNameW );
    SERV_free( lpBinaryPathNameW );
    SERV_free( lpLoadOrderGroupW );
    SERV_free( lpDependenciesW );
    SERV_free( lpServiceStartNameW );
    SERV_free( lpPasswordW );

    return r;
}


/******************************************************************************
 * DeleteService [ADVAPI32.@]
 *
 * Delete a service from the service control manager database.
 *
 * PARAMS
 *    hService [I] Handle of the service to delete
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI DeleteService( SC_HANDLE hService )
{
    struct sc_service *hsvc;
    HKEY hKey;
    WCHAR valname[MAX_PATH+1];
    INT index = 0;
    LONG rc;
    DWORD size;

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    hKey = hsvc->hkey;

    size = MAX_PATH+1; 
    /* Clean out the values */
    rc = RegEnumValueW(hKey, index, valname,&size,0,0,0,0);
    while (rc == ERROR_SUCCESS)
    {
        RegDeleteValueW(hKey,valname);
        index++;
        size = MAX_PATH+1; 
        rc = RegEnumValueW(hKey, index, valname, &size,0,0,0,0);
    }

    RegCloseKey(hKey);
    hsvc->hkey = NULL;

    /* delete the key */
    RegDeleteKeyW(hsvc->scm->hkey, hsvc->name);

    return TRUE;
}


/******************************************************************************
 * StartServiceA [ADVAPI32.@]
 *
 * Start a service
 *
 * PARAMS
 *   hService            [I] Handle of service
 *   dwNumServiceArgs    [I] Number of arguments
 *   lpServiceArgVectors [I] Address of array of argument strings
 *
 * NOTES
 *  - NT implements this function using an obscure RPC call.
 *  - You might need to do a "setenv SystemRoot \\WINNT" in your .cshrc
 *    to get things like "%SystemRoot%\\System32\\service.exe" to load.
 *  - This will only work for shared address space. How should the service
 *    args be transferred when address spaces are separated?
 *  - Can only start one service at a time.
 *  - Has no concept of privilege.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE
 */
BOOL WINAPI
StartServiceA( SC_HANDLE hService, DWORD dwNumServiceArgs,
                 LPCSTR *lpServiceArgVectors )
{
    LPWSTR *lpwstr=NULL;
    unsigned int i;

    TRACE("(%p,%ld,%p)\n",hService,dwNumServiceArgs,lpServiceArgVectors);

    if(dwNumServiceArgs)
        lpwstr = HeapAlloc( GetProcessHeap(), 0,
                                   dwNumServiceArgs*sizeof(LPWSTR) );

    for(i=0; i<dwNumServiceArgs; i++)
        lpwstr[i]=SERV_dup(lpServiceArgVectors[i]);

    StartServiceW(hService, dwNumServiceArgs, (LPCWSTR *)lpwstr);

    if(dwNumServiceArgs)
    {
        for(i=0; i<dwNumServiceArgs; i++)
            SERV_free(lpwstr[i]);
        HeapFree(GetProcessHeap(), 0, lpwstr);
    }

    return TRUE;
}


/******************************************************************************
 * StartServiceW [ADVAPI32.@]
 * 
 * See StartServiceA.
 */
BOOL WINAPI
StartServiceW( SC_HANDLE hService, DWORD dwNumServiceArgs,
                 LPCWSTR *lpServiceArgVectors )
{
    static const WCHAR  _ImagePathW[]  = {'I','m','a','g','e','P','a','t','h',0};
                                                
    struct sc_service *hsvc;
    WCHAR path[MAX_PATH],str[MAX_PATH];
    DWORD type,size;
    DWORD i;
    long r;
    HANDLE hLock;
    HANDLE hServiceShmem = NULL;
    HANDLE wait = NULL;
    LPWSTR shmem_lock = NULL;
    struct SEB *seb = NULL;
    LPWSTR argptr;
    PROCESS_INFORMATION procinfo;
    STARTUPINFOW startupinfo;
    BOOL ret = FALSE;

    TRACE("%p %ld %p\n",hService,dwNumServiceArgs,
          lpServiceArgVectors);

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    size = sizeof(str);
    r = RegQueryValueExW(hsvc->hkey, _ImagePathW, NULL, &type, (LPVOID)str, &size);
    if (r!=ERROR_SUCCESS)
        return FALSE;
    ExpandEnvironmentStringsW(str,path,sizeof(path));

    TRACE("Starting service %s\n", debugstr_w(path) );

    hLock = LockServiceDatabase( (SC_HANDLE) &hsvc->scm->hdr );
    if( NULL == hLock )
        return FALSE;

    /*
     * FIXME: start dependent services
     */

    /* pass argv[0] (service name) to the service via global SCM lock object */
    shmem_lock = MapViewOfFile( hLock, FILE_MAP_ALL_ACCESS,
                                0, 0, MAX_SERVICE_NAME * sizeof(WCHAR) );
    if( NULL == shmem_lock )
    {
        ERR("Couldn't map shared memory\n");
        goto done;
    }
    strcpyW( shmem_lock, hsvc->name );

    /* create service environment block */
    size = sizeof(struct SEB);
    for( i = 0; i < dwNumServiceArgs; i++ )
        size += sizeof(WCHAR) * (1 + strlenW( lpServiceArgVectors[ i ] ));

    snprintfW( str, MAX_PATH, szServiceShmemNameFmtW, hsvc->name );
    hServiceShmem = CreateFileMappingW( INVALID_HANDLE_VALUE,
                                        NULL, PAGE_READWRITE, 0, size, str );
    if( NULL == hServiceShmem )
    {
        ERR("Couldn't create shared memory object\n");
        goto done;
    }
    if( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        goto done;
    }
    seb = MapViewOfFile( hServiceShmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    if( NULL == seb )
    {
        ERR("Couldn't map shared memory\n");
        goto done;
    }

    /* copy service args to SEB */
    seb->argc = dwNumServiceArgs;
    argptr = (LPWSTR) &seb[1];
    for( i = 0; i < dwNumServiceArgs; i++ )
    {
        strcpyW( argptr, lpServiceArgVectors[ i ] );
        argptr += 1 + strlenW( argptr );
    }

    wait = CreateSemaphoreW(NULL,0,1,szWaitServiceStartW);
    if (!wait)
    {
        ERR("Couldn't create wait semaphore\n");
        goto done;
    }

    ZeroMemory(&startupinfo,sizeof(STARTUPINFOW));
    startupinfo.cb = sizeof(STARTUPINFOW);

    r = CreateProcessW(NULL,
                   path,
                   NULL,  /* process security attribs */
                   NULL,  /* thread security attribs */
                   FALSE, /* inherit handles */
                   0,     /* creation flags */
                   NULL,  /* environment */
                   NULL,  /* current directory */
                   &startupinfo,  /* startup info */
                   &procinfo); /* process info */

    if(r == FALSE)
    {
        ERR("Couldn't start process\n");
        goto done;
    }
    CloseHandle( procinfo.hThread );

    /* docs for StartServiceCtrlDispatcher say this should be 30 sec */
    r = WaitForSingleObject(wait,30000);
    if( WAIT_FAILED == r )
    {
        CloseHandle( procinfo.hProcess );
        goto done;
    }
    if( WAIT_TIMEOUT == r )
    {
        TerminateProcess( procinfo.hProcess, 1 );
        CloseHandle( procinfo.hProcess );
        SetLastError( ERROR_SERVICE_REQUEST_TIMEOUT );
        goto done;
    }

    /* allright */
    CloseHandle( procinfo.hProcess );
    ret = TRUE;

done:
    if( wait ) CloseHandle( wait );
    if( seb != NULL ) UnmapViewOfFile( seb );
    if( hServiceShmem != NULL ) CloseHandle( hServiceShmem );
    if( shmem_lock != NULL ) UnmapViewOfFile( shmem_lock );
    UnlockServiceDatabase( hLock );
    return ret;
}

/******************************************************************************
 * QueryServiceStatus [ADVAPI32.@]
 *
 * PARAMS
 *   hService        []
 *   lpservicestatus []
 *
 */
BOOL WINAPI
QueryServiceStatus( SC_HANDLE hService, LPSERVICE_STATUS lpservicestatus )
{
    struct sc_service *hsvc;
    LONG r;
    DWORD type, val, size;
    WCHAR object_name[ MAX_PATH ];
    HANDLE mutex, shmem = NULL;
    struct SEB *seb = NULL;
    BOOL ret = FALSE, mutex_owned = FALSE;

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    /* try to open service mutex */
    snprintfW( object_name, MAX_PATH, szServiceMutexNameFmtW, hsvc->name );
    mutex = OpenMutexW( MUTEX_ALL_ACCESS, FALSE, object_name );
    if( NULL == mutex )
        goto stopped;

    /* hold mutex */
    r = WaitForSingleObject( mutex, 30000 );
    if( WAIT_FAILED == r )
        goto done;

    if( WAIT_TIMEOUT == r )
    {
        SetLastError( ERROR_SERVICE_REQUEST_TIMEOUT );
        goto done;
    }
    mutex_owned = TRUE;

    /* get service environment block */
    seb = open_seb_shmem( hsvc->name, &shmem );
    if( NULL == seb )
        goto done;
 
    /* get status */
    memcpy( lpservicestatus, &seb->status, sizeof(SERVICE_STATUS) );
    ret = TRUE;

done:
    if( seb ) UnmapViewOfFile( seb );
    if( shmem ) CloseHandle( shmem );
    if( mutex_owned ) ReleaseMutex( mutex );
    CloseHandle( mutex );
    return ret;
 
stopped:
    /* service stopped */
    /* read the service type from the registry */
    size = sizeof(val);
    r = RegQueryValueExA(hsvc->hkey, "Type", NULL, &type, (LPBYTE)&val, &size);
    if(type!=REG_DWORD)
    {
        ERR("invalid Type\n");
        return FALSE;
    }
    lpservicestatus->dwServiceType = val;
    lpservicestatus->dwCurrentState            = 1;  /* stopped */
    lpservicestatus->dwControlsAccepted        = 0;
    lpservicestatus->dwWin32ExitCode           = NO_ERROR;
    lpservicestatus->dwServiceSpecificExitCode = 0;
    lpservicestatus->dwCheckPoint              = 0;
    lpservicestatus->dwWaitHint                = 0;

    return TRUE;
}

/******************************************************************************
 * QueryServiceStatusEx [ADVAPI32.@]
 *
 * Get information about a service.
 *
 * PARAMS
 *   hService       [I] Handle to service to get information about
 *   InfoLevel      [I] Level of information to get
 *   lpBuffer       [O] Destination for requested information
 *   cbBufSize      [I] Size of lpBuffer in bytes
 *   pcbBytesNeeded [O] Destination for number of bytes needed, if cbBufSize is too small
 *
 * RETURNS
 *  Success: TRUE
 *  FAILURE: FALSE
 */
BOOL WINAPI QueryServiceStatusEx(SC_HANDLE hService, SC_STATUS_TYPE InfoLevel,
                        LPBYTE lpBuffer, DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 * QueryServiceConfigA [ADVAPI32.@]
 */
BOOL WINAPI 
QueryServiceConfigA( SC_HANDLE hService,
                     LPQUERY_SERVICE_CONFIGA lpServiceConfig,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    static const CHAR szDisplayName[] = "DisplayName";
    static const CHAR szType[] = "Type";
    static const CHAR szStart[] = "Start";
    static const CHAR szError[] = "ErrorControl";
    static const CHAR szImagePath[] = "ImagePath";
    static const CHAR szGroup[] = "Group";
    static const CHAR szDependencies[] = "Dependencies";
    struct sc_service *hsvc;
    HKEY hKey;
    CHAR str_buffer[ MAX_PATH ];
    LONG r;
    DWORD type, val, sz, total, n;
    LPBYTE p;

    TRACE("%p %p %ld %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    hKey = hsvc->hkey;

    /* calculate the size required first */
    total = sizeof (QUERY_SERVICE_CONFIGA);

    sz = sizeof(str_buffer);
    r = RegQueryValueExA( hKey, szImagePath, 0, &type, str_buffer, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ || type == REG_EXPAND_SZ ) )
    {
        sz = ExpandEnvironmentStringsA(str_buffer,NULL,0);
        if( 0 == sz ) return FALSE;

        total += sz;
    }
    else
    {
        /* FIXME: set last error */
        return FALSE;
    }

    sz = 0;
    r = RegQueryValueExA( hKey, szGroup, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExA( hKey, szDependencies, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_MULTI_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExA( hKey, szStart, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExA( hKey, szDisplayName, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    /* if there's not enough memory, return an error */
    if( total > *pcbBytesNeeded )
    {
        *pcbBytesNeeded = total;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *pcbBytesNeeded = total;
    ZeroMemory( lpServiceConfig, total );

    sz = sizeof val;
    r = RegQueryValueExA( hKey, szType, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwServiceType = val;

    sz = sizeof val;
    r = RegQueryValueExA( hKey, szStart, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwStartType = val;

    sz = sizeof val;
    r = RegQueryValueExA( hKey, szError, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwErrorControl = val;

    /* now do the strings */
    p = (LPBYTE) &lpServiceConfig[1];
    n = total - sizeof (QUERY_SERVICE_CONFIGA);

    sz = sizeof(str_buffer);
    r = RegQueryValueExA( hKey, szImagePath, 0, &type, str_buffer, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ || type == REG_EXPAND_SZ ) )
    {
        sz = ExpandEnvironmentStringsA(str_buffer, p, n);
        if( 0 == sz || sz > n ) return FALSE;

        lpServiceConfig->lpBinaryPathName = (LPSTR) p;
        p += sz;
        n -= sz;
    }
    else
    {
        /* FIXME: set last error */
        return FALSE;
    }

    sz = n;
    r = RegQueryValueExA( hKey, szGroup, 0, &type, p, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_SZ ) )
    {
        lpServiceConfig->lpLoadOrderGroup = (LPSTR) p;
        p += sz;
        n -= sz;
    }

    sz = n;
    r = RegQueryValueExA( hKey, szDependencies, 0, &type, p, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_SZ ) )
    {
        lpServiceConfig->lpDependencies = (LPSTR) p;
        p += sz;
        n -= sz;
    }

    if( n < 0 )
        ERR("Buffer overflow!\n");

    TRACE("Image path = %s\n", lpServiceConfig->lpBinaryPathName );
    TRACE("Group      = %s\n", lpServiceConfig->lpLoadOrderGroup );

    return TRUE;
}

/******************************************************************************
 * QueryServiceConfigW [ADVAPI32.@]
 */
BOOL WINAPI 
QueryServiceConfigW( SC_HANDLE hService,
                     LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    static const WCHAR szDisplayName[] = {
        'D','i','s','p','l','a','y','N','a','m','e', 0 };
    static const WCHAR szType[] = {'T','y','p','e',0};
    static const WCHAR szStart[] = {'S','t','a','r','t',0};
    static const WCHAR szError[] = {
        'E','r','r','o','r','C','o','n','t','r','o','l', 0};
    static const WCHAR szImagePath[] = {'I','m','a','g','e','P','a','t','h',0};
    static const WCHAR szGroup[] = {'G','r','o','u','p',0};
    static const WCHAR szDependencies[] = {
        'D','e','p','e','n','d','e','n','c','i','e','s',0};
    WCHAR str_buffer[ MAX_PATH ];
    LONG r;
    DWORD type, val, sz, total, n;
    LPBYTE p;
    HKEY hKey;
    struct sc_service *hsvc;

    TRACE("%p %p %ld %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    hKey = hsvc->hkey;

    /* calculate the size required first */
    total = sizeof (QUERY_SERVICE_CONFIGW);

    sz = sizeof(str_buffer);
    r = RegQueryValueExW( hKey, szImagePath, 0, &type, (LPBYTE) str_buffer, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ || type == REG_EXPAND_SZ ) )
    {
        sz = ExpandEnvironmentStringsW(str_buffer,NULL,0);
        if( 0 == sz ) return FALSE;

        total += sizeof(WCHAR) * sz;
    }
    else
    {
       /* FIXME: set last error */
       return FALSE;
    }

    sz = 0;
    r = RegQueryValueExW( hKey, szGroup, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hKey, szDependencies, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_MULTI_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hKey, szStart, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hKey, szDisplayName, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    /* if there's not enough memory, return an error */
    if( total > *pcbBytesNeeded )
    {
        *pcbBytesNeeded = total;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *pcbBytesNeeded = total;
    ZeroMemory( lpServiceConfig, total );

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szType, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwServiceType = val;

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szStart, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwStartType = val;

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szError, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwErrorControl = val;

    /* now do the strings */
    p = (LPBYTE) &lpServiceConfig[1];
    n = total - sizeof (QUERY_SERVICE_CONFIGW);

    sz = sizeof(str_buffer);
    r = RegQueryValueExW( hKey, szImagePath, 0, &type, (LPBYTE) str_buffer, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ || type == REG_EXPAND_SZ ) )
    {
        sz = ExpandEnvironmentStringsW(str_buffer, (LPWSTR) p, n);
        sz *= sizeof(WCHAR);
        if( 0 == sz || sz > n ) return FALSE;

        lpServiceConfig->lpBinaryPathName = (LPWSTR) p;
        p += sz;
        n -= sz;
    }
    else
    {
       /* FIXME: set last error */
       return FALSE;
    }

    sz = n;
    r = RegQueryValueExW( hKey, szGroup, 0, &type, p, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_SZ ) )
    {
        lpServiceConfig->lpLoadOrderGroup = (LPWSTR) p;
        p += sz;
        n -= sz;
    }

    sz = n;
    r = RegQueryValueExW( hKey, szDependencies, 0, &type, p, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_SZ ) )
    {
        lpServiceConfig->lpDependencies = (LPWSTR) p;
        p += sz;
        n -= sz;
    }

    if( n < 0 )
        ERR("Buffer overflow!\n");

    TRACE("Image path = %s\n", debugstr_w(lpServiceConfig->lpBinaryPathName) );
    TRACE("Group      = %s\n", debugstr_w(lpServiceConfig->lpLoadOrderGroup) );

    return TRUE;
}

/******************************************************************************
 * EnumServicesStatusA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusA( SC_HANDLE hSCManager, DWORD dwServiceType,
                     DWORD dwServiceState, LPENUM_SERVICE_STATUSA lpServices,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                     LPDWORD lpServicesReturned, LPDWORD lpResumeHandle )
{
    FIXME("%p type=%lx state=%lx %p %lx %p %p %p\n", hSCManager,
          dwServiceType, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned,  lpResumeHandle);
    SetLastError (ERROR_ACCESS_DENIED);
    return FALSE;
}

/******************************************************************************
 * EnumServicesStatusW [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusW( SC_HANDLE hSCManager, DWORD dwServiceType,
                     DWORD dwServiceState, LPENUM_SERVICE_STATUSW lpServices,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                     LPDWORD lpServicesReturned, LPDWORD lpResumeHandle )
{
    FIXME("%p type=%lx state=%lx %p %lx %p %p %p\n", hSCManager,
          dwServiceType, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned,  lpResumeHandle);
    SetLastError (ERROR_ACCESS_DENIED);
    return FALSE;
}

/******************************************************************************
 * GetServiceKeyNameA [ADVAPI32.@]
 */
BOOL WINAPI GetServiceKeyNameA( SC_HANDLE hSCManager, LPCSTR lpDisplayName,
                                LPSTR lpServiceName, LPDWORD lpcchBuffer )
{
    FIXME("%p %s %p %p\n", hSCManager, debugstr_a(lpDisplayName), lpServiceName, lpcchBuffer);
    return FALSE;
}

/******************************************************************************
 * GetServiceKeyNameW [ADVAPI32.@]
 */
BOOL WINAPI GetServiceKeyNameW( SC_HANDLE hSCManager, LPCWSTR lpDisplayName,
                                LPWSTR lpServiceName, LPDWORD lpcchBuffer )
{
    FIXME("%p %s %p %p\n", hSCManager, debugstr_w(lpDisplayName), lpServiceName, lpcchBuffer);
    return FALSE;
}

/******************************************************************************
 * QueryServiceLockStatusA [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceLockStatusA( SC_HANDLE hSCManager,
                                     LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
                                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    FIXME("%p %p %08lx %p\n", hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

    return FALSE;
}

/******************************************************************************
 * QueryServiceLockStatusW [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceLockStatusW( SC_HANDLE hSCManager,
                                     LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
                                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    FIXME("%p %p %08lx %p\n", hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

    return FALSE;
}

/******************************************************************************
 * GetServiceDisplayNameA  [ADVAPI32.@]
 */
BOOL WINAPI GetServiceDisplayNameA( SC_HANDLE hSCManager, LPCSTR lpServiceName,
  LPSTR lpDisplayName, LPDWORD lpcchBuffer)
{
    FIXME("%p %s %p %p\n", hSCManager,
          debugstr_a(lpServiceName), lpDisplayName, lpcchBuffer);
    return FALSE;
}

/******************************************************************************
 * GetServiceDisplayNameW  [ADVAPI32.@]
 */
BOOL WINAPI GetServiceDisplayNameW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
  LPWSTR lpDisplayName, LPDWORD lpcchBuffer)
{
    FIXME("%p %s %p %p\n", hSCManager,
          debugstr_w(lpServiceName), lpDisplayName, lpcchBuffer);
    return FALSE;
}

/******************************************************************************
 * ChangeServiceConfigW  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfigW( SC_HANDLE hService, DWORD dwServiceType,
  DWORD dwStartType, DWORD dwErrorControl, LPCWSTR lpBinaryPathName,
  LPCWSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCWSTR lpDependencies,
  LPCWSTR lpServiceStartName, LPCWSTR lpPassword, LPCWSTR lpDisplayName)
{
    FIXME("%p %ld %ld %ld %s %s %p %p %s %s %s\n",
          hService, dwServiceType, dwStartType, dwErrorControl, 
          debugstr_w(lpBinaryPathName), debugstr_w(lpLoadOrderGroup),
          lpdwTagId, lpDependencies, debugstr_w(lpServiceStartName),
          debugstr_w(lpPassword), debugstr_w(lpDisplayName) );
    return TRUE;
}

/******************************************************************************
 * ChangeServiceConfigA  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfigA( SC_HANDLE hService, DWORD dwServiceType,
  DWORD dwStartType, DWORD dwErrorControl, LPCSTR lpBinaryPathName,
  LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCSTR lpDependencies,
  LPCSTR lpServiceStartName, LPCSTR lpPassword, LPCSTR lpDisplayName)
{
    LPWSTR wBinaryPathName, wLoadOrderGroup, wDependencies;
    LPWSTR wServiceStartName, wPassword, wDisplayName;
    BOOL r;

    TRACE("%p %ld %ld %ld %s %s %p %p %s %s %s\n",
          hService, dwServiceType, dwStartType, dwErrorControl, 
          debugstr_a(lpBinaryPathName), debugstr_a(lpLoadOrderGroup),
          lpdwTagId, lpDependencies, debugstr_a(lpServiceStartName),
          debugstr_a(lpPassword), debugstr_a(lpDisplayName) );

    wBinaryPathName = SERV_dup( lpBinaryPathName );
    wLoadOrderGroup = SERV_dup( lpLoadOrderGroup );
    wDependencies = SERV_dupmulti( lpDependencies );
    wServiceStartName = SERV_dup( lpServiceStartName );
    wPassword = SERV_dup( lpPassword );
    wDisplayName = SERV_dup( lpDisplayName );

    r = ChangeServiceConfigW( hService, dwServiceType,
            dwStartType, dwErrorControl, wBinaryPathName,
            wLoadOrderGroup, lpdwTagId, wDependencies,
            wServiceStartName, wPassword, wDisplayName);

    SERV_free( wBinaryPathName );
    SERV_free( wLoadOrderGroup );
    SERV_free( wDependencies );
    SERV_free( wServiceStartName );
    SERV_free( wPassword );
    SERV_free( wDisplayName );

    return r;
}

/******************************************************************************
 * ChangeServiceConfig2A  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfig2A( SC_HANDLE hService, DWORD dwInfoLevel, 
    LPVOID lpInfo)
{
    BOOL r = FALSE;

    TRACE("%p %ld %p\n",hService, dwInfoLevel, lpInfo);

    if (dwInfoLevel == SERVICE_CONFIG_DESCRIPTION)
    {
        LPSERVICE_DESCRIPTIONA sd = (LPSERVICE_DESCRIPTIONA) lpInfo;
        SERVICE_DESCRIPTIONW sdw;

        sdw.lpDescription = SERV_dup( sd->lpDescription );

        r = ChangeServiceConfig2W( hService, dwInfoLevel, &sdw );

        SERV_free( sdw.lpDescription );
    }
    else if (dwInfoLevel == SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        LPSERVICE_FAILURE_ACTIONSA fa = (LPSERVICE_FAILURE_ACTIONSA) lpInfo;
        SERVICE_FAILURE_ACTIONSW faw;

        faw.dwResetPeriod = fa->dwResetPeriod;
        faw.lpRebootMsg = SERV_dup( fa->lpRebootMsg );
        faw.lpCommand = SERV_dup( fa->lpCommand );
        faw.cActions = fa->cActions;
        faw.lpsaActions = fa->lpsaActions;

        r = ChangeServiceConfig2W( hService, dwInfoLevel, &faw );

        SERV_free( faw.lpRebootMsg );
        SERV_free( faw.lpCommand );
    }
    else
        SetLastError( ERROR_INVALID_PARAMETER );

    return r;
}

/******************************************************************************
 * ChangeServiceConfig2W  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfig2W( SC_HANDLE hService, DWORD dwInfoLevel, 
    LPVOID lpInfo)
{
    HKEY hKey;
    struct sc_service *hsvc;

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    hKey = hsvc->hkey;

    if (dwInfoLevel == SERVICE_CONFIG_DESCRIPTION)
    {
        static const WCHAR szDescription[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
        LPSERVICE_DESCRIPTIONW sd = (LPSERVICE_DESCRIPTIONW)lpInfo;
        if (sd->lpDescription)
        {
            TRACE("Setting Description to %s\n",debugstr_w(sd->lpDescription));
            if (sd->lpDescription[0] == 0)
                RegDeleteValueW(hKey,szDescription);
            else
                RegSetValueExW(hKey, szDescription, 0, REG_SZ,
                                        (LPVOID)sd->lpDescription,
                                 sizeof(WCHAR)*(strlenW(sd->lpDescription)+1));
        }
    }
    else   
        FIXME("STUB: %p %ld %p\n",hService, dwInfoLevel, lpInfo);
    return TRUE;
}

/******************************************************************************
 * QueryServiceObjectSecurity [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceObjectSecurity(SC_HANDLE hService,
       SECURITY_INFORMATION dwSecurityInformation,
       PSECURITY_DESCRIPTOR lpSecurityDescriptor,
       DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    PACL pACL = NULL;

    FIXME("%p %ld %p %lu %p\n", hService, dwSecurityInformation,
          lpSecurityDescriptor, cbBufSize, pcbBytesNeeded);

    InitializeSecurityDescriptor(lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    pACL = HeapAlloc( GetProcessHeap(), 0, sizeof(ACL) );
    InitializeAcl(pACL, sizeof(ACL), ACL_REVISION);
    SetSecurityDescriptorDacl(lpSecurityDescriptor, TRUE, pACL, TRUE);
    return TRUE;
}

/******************************************************************************
 * SetServiceObjectSecurity [ADVAPI32.@]
 */
BOOL WINAPI SetServiceObjectSecurity(SC_HANDLE hService,
       SECURITY_INFORMATION dwSecurityInformation,
       PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    FIXME("%p %ld %p\n", hService, dwSecurityInformation, lpSecurityDescriptor);
    return TRUE;
}

/******************************************************************************
 * SetServiceBits [ADVAPI32.@]
 */
BOOL WINAPI SetServiceBits( SERVICE_STATUS_HANDLE hServiceStatus,
        DWORD dwServiceBits,
        BOOL bSetBitsOn,
        BOOL bUpdateImmediately)
{
    FIXME("%08lx %08lx %x %x\n", hServiceStatus, dwServiceBits,
          bSetBitsOn, bUpdateImmediately);
    return TRUE;
}
