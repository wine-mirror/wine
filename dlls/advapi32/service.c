/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 2005 Mike McCormack
 * Copyright 2007 Rolf Kalbermatter
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winsvc.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winternl.h"
#include "lmcons.h"
#include "lmserver.h"

#include "svcctl.h"

#include "advapi32_misc.h"

#include "wine/exception.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(service);

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return heap_alloc(len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    heap_free(ptr);
}

typedef struct service_data_t
{
    LPHANDLER_FUNCTION_EX handler;
    LPVOID context;
    HANDLE thread;
    SC_HANDLE handle;
    SC_HANDLE full_access_handle;
    BOOL unicode : 1;
    union {
        LPSERVICE_MAIN_FUNCTIONA a;
        LPSERVICE_MAIN_FUNCTIONW w;
    } proc;
    LPWSTR args;
    WCHAR name[1];
} service_data;

typedef struct dispatcher_data_t
{
    SC_HANDLE manager;
    HANDLE pipe;
} dispatcher_data;

typedef struct notify_data_t {
    SC_HANDLE service;
    SC_RPC_NOTIFY_PARAMS params;
    SERVICE_NOTIFY_STATUS_CHANGE_PARAMS_2 cparams;
    SC_NOTIFY_RPC_HANDLE notify_handle;
    SERVICE_NOTIFYW *notify_buffer;
    HANDLE calling_thread, ready_evt;
    struct list entry;
} notify_data;

static struct list notify_list = LIST_INIT(notify_list);

static CRITICAL_SECTION service_cs;
static CRITICAL_SECTION_DEBUG service_cs_debug =
{
    0, 0, &service_cs,
    { &service_cs_debug.ProcessLocksList, 
      &service_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": service_cs") }
};
static CRITICAL_SECTION service_cs = { &service_cs_debug, -1, 0, 0, 0, 0 };

static service_data **services;
static unsigned int nb_services;
static HANDLE service_event;
static BOOL stop_service;

extern HANDLE CDECL __wine_make_process_system(void);

/******************************************************************************
 * String management functions (same behaviour as strdup)
 * NOTE: the caller of those functions is responsible for calling HeapFree
 * in order to release the memory allocated by those functions.
 */
LPWSTR SERV_dup( LPCSTR str )
{
    UINT len;
    LPWSTR wstr;

    if( !str )
        return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    wstr = heap_alloc( len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, len );
    return wstr;
}

static inline LPWSTR SERV_dupmulti(LPCSTR str)
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

    wstr = heap_alloc( len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, n, wstr, len );
    return wstr;
}

static inline DWORD multisz_cb(LPCWSTR wmultisz)
{
    const WCHAR *wptr = wmultisz;

    if (wmultisz == NULL)
        return 0;

    while (*wptr)
        wptr += lstrlenW(wptr)+1;
    return (wptr - wmultisz + 1)*sizeof(WCHAR);
}

/******************************************************************************
 * RPC connection with services.exe
 */
static handle_t rpc_wstr_bind(RPC_WSTR str)
{
    WCHAR transport[] = SVCCTL_TRANSPORT;
    WCHAR endpoint[] = SVCCTL_ENDPOINT;
    RPC_WSTR binding_str;
    RPC_STATUS status;
    handle_t rpc_handle;

    status = RpcStringBindingComposeW(NULL, transport, str, endpoint, NULL, &binding_str);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeW failed (%d)\n", (DWORD)status);
        return NULL;
    }

    status = RpcBindingFromStringBindingW(binding_str, &rpc_handle);
    RpcStringFreeW(&binding_str);

    if (status != RPC_S_OK)
    {
        ERR("Couldn't connect to services.exe: error code %u\n", (DWORD)status);
        return NULL;
    }

    return rpc_handle;
}

static handle_t rpc_cstr_bind(RPC_CSTR str)
{
    RPC_CSTR transport = (RPC_CSTR)SVCCTL_TRANSPORTA;
    RPC_CSTR endpoint = (RPC_CSTR)SVCCTL_ENDPOINTA;
    RPC_CSTR binding_str;
    RPC_STATUS status;
    handle_t rpc_handle;

    status = RpcStringBindingComposeA(NULL, transport, str, endpoint, NULL, &binding_str);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeW failed (%d)\n", (DWORD)status);
        return NULL;
    }

    status = RpcBindingFromStringBindingA(binding_str, &rpc_handle);
    RpcStringFreeA(&binding_str);

    if (status != RPC_S_OK)
    {
        ERR("Couldn't connect to services.exe: error code %u\n", (DWORD)status);
        return NULL;
    }

    return rpc_handle;
}

DECLSPEC_HIDDEN handle_t __RPC_USER MACHINE_HANDLEA_bind(MACHINE_HANDLEA MachineName)
{
    return rpc_cstr_bind((RPC_CSTR)MachineName);
}

DECLSPEC_HIDDEN void __RPC_USER MACHINE_HANDLEA_unbind(MACHINE_HANDLEA MachineName, handle_t h)
{
    RpcBindingFree(&h);
}

DECLSPEC_HIDDEN handle_t __RPC_USER MACHINE_HANDLEW_bind(MACHINE_HANDLEW MachineName)
{
    return rpc_wstr_bind((RPC_WSTR)MachineName);
}

DECLSPEC_HIDDEN void __RPC_USER MACHINE_HANDLEW_unbind(MACHINE_HANDLEW MachineName, handle_t h)
{
    RpcBindingFree(&h);
}

DECLSPEC_HIDDEN handle_t __RPC_USER SVCCTL_HANDLEW_bind(SVCCTL_HANDLEW MachineName)
{
    return rpc_wstr_bind((RPC_WSTR)MachineName);
}

DECLSPEC_HIDDEN void __RPC_USER SVCCTL_HANDLEW_unbind(SVCCTL_HANDLEW MachineName, handle_t h)
{
    RpcBindingFree(&h);
}

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}

static DWORD map_exception_code(DWORD exception_code)
{
    switch (exception_code)
    {
    case RPC_X_NULL_REF_POINTER:
        return ERROR_INVALID_ADDRESS;
    case RPC_X_ENUM_VALUE_OUT_OF_RANGE:
    case RPC_X_BYTE_COUNT_TOO_SMALL:
        return ERROR_INVALID_PARAMETER;
    case RPC_S_INVALID_BINDING:
    case RPC_X_SS_IN_NULL_CONTEXT:
        return ERROR_INVALID_HANDLE;
    default:
        return exception_code;
    }
}

/******************************************************************************
 * Service IPC functions
 */
static LPWSTR service_get_pipe_name(void)
{
    static const WCHAR format[] = { '\\','\\','.','\\','p','i','p','e','\\',
        'n','e','t','\\','N','t','C','o','n','t','r','o','l','P','i','p','e','%','u',0};
    static const WCHAR service_current_key_str[] = { 'S','Y','S','T','E','M','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'S','e','r','v','i','c','e','C','u','r','r','e','n','t',0};
    LPWSTR name;
    DWORD len;
    HKEY service_current_key;
    DWORD service_current;
    LONG ret;
    DWORD type;

    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, service_current_key_str, 0,
        KEY_QUERY_VALUE, &service_current_key);
    if (ret != ERROR_SUCCESS)
        return NULL;
    len = sizeof(service_current);
    ret = RegQueryValueExW(service_current_key, NULL, NULL, &type,
        (BYTE *)&service_current, &len);
    RegCloseKey(service_current_key);
    if (ret != ERROR_SUCCESS || type != REG_DWORD)
        return NULL;
    len = ARRAY_SIZE(format) + 10 /* strlenW("4294967295") */;
    name = heap_alloc(len * sizeof(WCHAR));
    if (!name)
        return NULL;
    snprintfW(name, len, format, service_current);
    return name;
}

static HANDLE service_open_pipe(void)
{
    LPWSTR szPipe = service_get_pipe_name();
    HANDLE handle = INVALID_HANDLE_VALUE;

    do {
        handle = CreateFileW(szPipe, GENERIC_READ|GENERIC_WRITE,
                         0, NULL, OPEN_ALWAYS, 0, NULL);
        if (handle != INVALID_HANDLE_VALUE)
            break;
        if (GetLastError() != ERROR_PIPE_BUSY)
            break;
    } while (WaitNamedPipeW(szPipe, NMPWAIT_USE_DEFAULT_WAIT));
    heap_free(szPipe);

    return handle;
}

static service_data *find_service_by_name( const WCHAR *name )
{
    unsigned int i;

    if (nb_services == 1)  /* only one service (FIXME: should depend on OWN_PROCESS etc.) */
        return services[0];
    for (i = 0; i < nb_services; i++)
        if (!strcmpiW( name, services[i]->name )) return services[i];
    return NULL;
}

/******************************************************************************
 * service_thread
 *
 * Call into the main service routine provided by StartServiceCtrlDispatcher.
 */
static DWORD WINAPI service_thread(LPVOID arg)
{
    service_data *info = arg;
    LPWSTR str = info->args;
    DWORD argc = 0, len = 0;

    TRACE("%p\n", arg);

    while (str[len])
    {
        len += strlenW(&str[len]) + 1;
        argc++;
    }
    len++;

    if (info->unicode)
    {
        LPWSTR *argv, p;

        argv = heap_alloc((argc+1)*sizeof(LPWSTR));
        for (argc=0, p=str; *p; p += strlenW(p) + 1)
            argv[argc++] = p;
        argv[argc] = NULL;

        info->proc.w(argc, argv);
        heap_free(argv);
    }
    else
    {
        LPSTR strA, *argv, p;
        DWORD lenA;
        
        lenA = WideCharToMultiByte(CP_ACP,0, str, len, NULL, 0, NULL, NULL);
        strA = heap_alloc(lenA);
        WideCharToMultiByte(CP_ACP,0, str, len, strA, lenA, NULL, NULL);

        argv = heap_alloc((argc+1)*sizeof(LPSTR));
        for (argc=0, p=strA; *p; p += strlen(p) + 1)
            argv[argc++] = p;
        argv[argc] = NULL;

        info->proc.a(argc, argv);
        heap_free(argv);
        heap_free(strA);
    }
    return 0;
}

/******************************************************************************
 * service_handle_start
 */
static DWORD service_handle_start(service_data *service, const void *data, DWORD data_size)
{
    DWORD count = data_size / sizeof(WCHAR);

    if (service->thread)
    {
        WARN("service is not stopped\n");
        return ERROR_SERVICE_ALREADY_RUNNING;
    }

    heap_free(service->args);
    service->args = heap_alloc((count + 2) * sizeof(WCHAR));
    if (count) memcpy( service->args, data, count * sizeof(WCHAR) );
    service->args[count++] = 0;
    service->args[count++] = 0;

    service->thread = CreateThread( NULL, 0, service_thread,
                                    service, 0, NULL );
    SetEvent( service_event );  /* notify the main loop */
    return 0;
}

/******************************************************************************
 * service_handle_control
 */
static DWORD service_handle_control(service_data *service, DWORD control, const void *data, DWORD data_size)
{
    DWORD ret = ERROR_INVALID_SERVICE_CONTROL;

    TRACE("%s control %u data %p data_size %u\n", debugstr_w(service->name), control, data, data_size);

    if (control == SERVICE_CONTROL_START)
        ret = service_handle_start(service, data, data_size);
    else if (service->handler)
        ret = service->handler(control, 0, (void *)data, service->context);
    return ret;
}

/******************************************************************************
 * service_control_dispatcher
 */
static DWORD WINAPI service_control_dispatcher(LPVOID arg)
{
    dispatcher_data *disp = arg;

    /* dispatcher loop */
    while (1)
    {
        service_data *service;
        service_start_info info;
        BYTE *data = NULL;
        WCHAR *name;
        BOOL r;
        DWORD data_size = 0, count, result;

        r = ReadFile( disp->pipe, &info, FIELD_OFFSET(service_start_info,data), &count, NULL );
        if (!r)
        {
            if (GetLastError() != ERROR_BROKEN_PIPE)
                ERR( "pipe read failed error %u\n", GetLastError() );
            break;
        }
        if (count != FIELD_OFFSET(service_start_info,data))
        {
            ERR( "partial pipe read %u\n", count );
            break;
        }
        if (count < info.total_size)
        {
            data_size = info.total_size - FIELD_OFFSET(service_start_info,data);
            data = heap_alloc( data_size );
            r = ReadFile( disp->pipe, data, data_size, &count, NULL );
            if (!r)
            {
                if (GetLastError() != ERROR_BROKEN_PIPE)
                    ERR( "pipe read failed error %u\n", GetLastError() );
                heap_free( data );
                break;
            }
            if (count != data_size)
            {
                ERR( "partial pipe read %u/%u\n", count, data_size );
                heap_free( data );
                break;
            }
        }

        EnterCriticalSection( &service_cs );

        /* validate service name */
        name = (WCHAR *)data;
        if (!info.name_size || data_size < info.name_size * sizeof(WCHAR) || name[info.name_size - 1])
        {
            ERR( "got request without valid service name\n" );
            result = ERROR_INVALID_PARAMETER;
            goto done;
        }

        if (info.magic != SERVICE_PROTOCOL_MAGIC)
        {
            ERR( "received invalid request for service %s\n", debugstr_w(name) );
            result = ERROR_INVALID_PARAMETER;
            goto done;
        }

        /* find the service */
        if (!(service = find_service_by_name( name )))
        {
            FIXME( "got request for unknown service %s\n", debugstr_w(name) );
            result = ERROR_INVALID_PARAMETER;
            goto done;
        }

        if (!service->handle)
        {
            if (!(service->handle = OpenServiceW( disp->manager, name, SERVICE_SET_STATUS )) ||
                !(service->full_access_handle = OpenServiceW( disp->manager, name,
                        GENERIC_READ|GENERIC_WRITE )))
                FIXME( "failed to open service %s\n", debugstr_w(name) );
        }

        data_size -= info.name_size * sizeof(WCHAR);
        result = service_handle_control(service, info.control, data_size ?
                                        &data[info.name_size * sizeof(WCHAR)] : NULL, data_size);

    done:
        LeaveCriticalSection( &service_cs );
        WriteFile( disp->pipe, &result, sizeof(result), &count, NULL );
        heap_free( data );
    }

    CloseHandle( disp->pipe );
    CloseServiceHandle( disp->manager );
    heap_free( disp );
    return 1;
}

/* wait for services which accept this type of message to become STOPPED */
static void handle_shutdown_msg(DWORD msg, DWORD accept)
{
    SERVICE_STATUS st;
    SERVICE_PRESHUTDOWN_INFO spi;
    DWORD i, n = 0, sz, timeout = 2000;
    ULONGLONG stop_time;
    BOOL res, done = TRUE;
    SC_HANDLE *wait_handles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SC_HANDLE) * nb_services );

    EnterCriticalSection( &service_cs );
    for (i = 0; i < nb_services; i++)
    {
        res = QueryServiceStatus( services[i]->full_access_handle, &st );
        if (!res || st.dwCurrentState == SERVICE_STOPPED || !(st.dwControlsAccepted & accept))
            continue;

        done = FALSE;

        if (accept == SERVICE_ACCEPT_PRESHUTDOWN)
        {
            res = QueryServiceConfig2W( services[i]->full_access_handle, SERVICE_CONFIG_PRESHUTDOWN_INFO,
                    (LPBYTE)&spi, sizeof(spi), &sz );
            if (res)
            {
                FIXME( "service should be able to delay shutdown\n" );
                timeout = max( spi.dwPreshutdownTimeout, timeout );
            }
        }

        service_handle_control( services[i], msg, NULL, 0 );
        wait_handles[n++] = services[i]->full_access_handle;
    }
    LeaveCriticalSection( &service_cs );

    /* FIXME: these timeouts should be more generous, but we can't currently delay prefix shutdown */
    timeout = min( timeout, 3000 );
    stop_time = GetTickCount64() + timeout;

    while (!done && GetTickCount64() < stop_time)
    {
        done = TRUE;
        for (i = 0; i < n; i++)
        {
            res = QueryServiceStatus( wait_handles[i], &st );
            if (!res || st.dwCurrentState == SERVICE_STOPPED)
                continue;

            done = FALSE;
            Sleep( 100 );
            break;
        }
    }

    HeapFree( GetProcessHeap(), 0, wait_handles );
}

/******************************************************************************
 * service_run_main_thread
 */
static BOOL service_run_main_thread(void)
{
    DWORD i, n, ret;
    HANDLE wait_handles[MAXIMUM_WAIT_OBJECTS];
    UINT wait_services[MAXIMUM_WAIT_OBJECTS];
    dispatcher_data *disp = heap_alloc( sizeof(*disp) );

    disp->manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    if (!disp->manager)
    {
        ERR("failed to open service manager error %u\n", GetLastError());
        heap_free( disp );
        return FALSE;
    }

    disp->pipe = service_open_pipe();
    if (disp->pipe == INVALID_HANDLE_VALUE)
    {
        WARN("failed to create control pipe error %u\n", GetLastError());
        CloseServiceHandle( disp->manager );
        heap_free( disp );
        SetLastError( ERROR_FAILED_SERVICE_CONTROLLER_CONNECT );
        return FALSE;
    }

    service_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    stop_service  = FALSE;

    /* FIXME: service_control_dispatcher should be merged into the main thread */
    wait_handles[0] = __wine_make_process_system();
    wait_handles[1] = CreateThread( NULL, 0, service_control_dispatcher, disp, 0, NULL );
    wait_handles[2] = service_event;

    TRACE("Starting %d services running as process %d\n",
          nb_services, GetCurrentProcessId());

    /* wait for all the threads to pack up and exit */
    while (!stop_service)
    {
        EnterCriticalSection( &service_cs );
        for (i = 0, n = 3; i < nb_services && n < MAXIMUM_WAIT_OBJECTS; i++)
        {
            if (!services[i]->thread) continue;
            wait_services[n] = i;
            wait_handles[n++] = services[i]->thread;
        }
        LeaveCriticalSection( &service_cs );

        ret = WaitForMultipleObjects( n, wait_handles, FALSE, INFINITE );
        if (!ret)  /* system process event */
        {
            handle_shutdown_msg(SERVICE_CONTROL_PRESHUTDOWN, SERVICE_ACCEPT_PRESHUTDOWN);
            handle_shutdown_msg(SERVICE_CONTROL_SHUTDOWN, SERVICE_ACCEPT_SHUTDOWN);
            ExitProcess(0);
        }
        else if (ret == 1)
        {
            TRACE( "control dispatcher exited, shutting down\n" );
            /* FIXME: we should maybe send a shutdown control to running services */
            ExitProcess(0);
        }
        else if (ret == 2)
        {
            continue;  /* rebuild the list */
        }
        else if (ret < n)
        {
            i = wait_services[ret];
            EnterCriticalSection( &service_cs );
            CloseHandle( services[i]->thread );
            services[i]->thread = NULL;
            LeaveCriticalSection( &service_cs );
        }
        else return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * StartServiceCtrlDispatcherA [ADVAPI32.@]
 *
 * See StartServiceCtrlDispatcherW.
 */
BOOL WINAPI StartServiceCtrlDispatcherA( const SERVICE_TABLE_ENTRYA *servent )
{
    service_data *info;
    unsigned int i;

    TRACE("%p\n", servent);

    if (nb_services)
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        return FALSE;
    }
    while (servent[nb_services].lpServiceName) nb_services++;
    if (!nb_services)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    services = heap_alloc( nb_services * sizeof(*services) );

    for (i = 0; i < nb_services; i++)
    {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, servent[i].lpServiceName, -1, NULL, 0);
        DWORD sz = FIELD_OFFSET( service_data, name[len] );
        info = heap_alloc_zero( sz );
        MultiByteToWideChar(CP_ACP, 0, servent[i].lpServiceName, -1, info->name, len);
        info->proc.a = servent[i].lpServiceProc;
        info->unicode = FALSE;
        services[i] = info;
    }

    return service_run_main_thread();
}

/******************************************************************************
 * StartServiceCtrlDispatcherW [ADVAPI32.@]
 *
 *  Connects a process containing one or more services to the service control
 * manager.
 *
 * PARAMS
 *   servent [I]  A list of the service names and service procedures
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI StartServiceCtrlDispatcherW( const SERVICE_TABLE_ENTRYW *servent )
{
    service_data *info;
    unsigned int i;

    TRACE("%p\n", servent);

    if (nb_services)
    {
        SetLastError( ERROR_SERVICE_ALREADY_RUNNING );
        return FALSE;
    }
    while (servent[nb_services].lpServiceName) nb_services++;
    if (!nb_services)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    services = heap_alloc( nb_services * sizeof(*services) );

    for (i = 0; i < nb_services; i++)
    {
        DWORD len = strlenW(servent[i].lpServiceName) + 1;
        DWORD sz = FIELD_OFFSET( service_data, name[len] );
        info = heap_alloc_zero( sz );
        strcpyW(info->name, servent[i].lpServiceName);
        info->proc.w = servent[i].lpServiceProc;
        info->unicode = TRUE;
        services[i] = info;
    }

    return service_run_main_thread();
}

/******************************************************************************
 * LockServiceDatabase  [ADVAPI32.@]
 */
SC_LOCK WINAPI LockServiceDatabase (SC_HANDLE hSCManager)
{
    SC_RPC_LOCK hLock = NULL;
    DWORD err;

    TRACE("%p\n",hSCManager);

    __TRY
    {
        err = svcctl_LockServiceDatabase(hSCManager, &hLock);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY
    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return NULL;
    }
    return hLock;
}

/******************************************************************************
 * UnlockServiceDatabase  [ADVAPI32.@]
 */
BOOL WINAPI UnlockServiceDatabase (SC_LOCK ScLock)
{
    DWORD err;
    SC_RPC_LOCK hRpcLock = ScLock;

    TRACE("%p\n",ScLock);

    __TRY
    {
        err = svcctl_UnlockServiceDatabase(&hRpcLock);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY
    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return FALSE;
    }
    return TRUE;
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
    DWORD err;

    TRACE("%p %x %x %x %x %x %x %x\n", hService,
          lpStatus->dwServiceType, lpStatus->dwCurrentState,
          lpStatus->dwControlsAccepted, lpStatus->dwWin32ExitCode,
          lpStatus->dwServiceSpecificExitCode, lpStatus->dwCheckPoint,
          lpStatus->dwWaitHint);

    __TRY
    {
        err = svcctl_SetServiceStatus( hService, lpStatus );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY
    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return FALSE;
    }

    if (lpStatus->dwCurrentState == SERVICE_STOPPED)
    {
        unsigned int i, count = 0;
        EnterCriticalSection( &service_cs );
        for (i = 0; i < nb_services; i++)
        {
            if (services[i]->handle == (SC_HANDLE)hService) continue;
            if (services[i]->thread) count++;
        }
        if (!count)
        {
            stop_service = TRUE;
            SetEvent( service_event );  /* notify the main loop */
        }
        LeaveCriticalSection( &service_cs );
    }

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
    LPWSTR machineW, databaseW;
    SC_HANDLE ret;

    machineW = SERV_dup(lpMachineName);
    databaseW = SERV_dup(lpDatabaseName);
    ret = OpenSCManagerW(machineW, databaseW, dwDesiredAccess);
    heap_free(databaseW);
    heap_free(machineW);
    return ret;
}

/******************************************************************************
 * OpenSCManagerW [ADVAPI32.@]
 *
 * See OpenSCManagerA.
 */
static DWORD SERV_OpenSCManagerW( LPCWSTR lpMachineName, LPCWSTR lpDatabaseName,
                           DWORD dwDesiredAccess, SC_HANDLE *handle )
{
    DWORD r;

    TRACE("(%s,%s,0x%08x)\n", debugstr_w(lpMachineName),
          debugstr_w(lpDatabaseName), dwDesiredAccess);

    __TRY
    {
        r = svcctl_OpenSCManagerW(lpMachineName, lpDatabaseName, dwDesiredAccess, (SC_RPC_HANDLE *)handle);
    }
    __EXCEPT(rpc_filter)
    {
        r = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (r!=ERROR_SUCCESS)
        *handle = 0;

    TRACE("returning %p\n", *handle);
    return r;
}

SC_HANDLE WINAPI OpenSCManagerW( LPCWSTR lpMachineName, LPCWSTR lpDatabaseName,
                                 DWORD dwDesiredAccess )
{
    SC_HANDLE handle = 0;
    DWORD r;

    r = SERV_OpenSCManagerW(lpMachineName, lpDatabaseName, dwDesiredAccess, &handle);
    if (r!=ERROR_SUCCESS)
        SetLastError(r);
    return handle;
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
    DWORD err;

    TRACE("%p %d %p\n", hService, dwControl, lpServiceStatus);

    __TRY
    {
        err = svcctl_ControlService(hService, dwControl, lpServiceStatus);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY
    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return FALSE;
    }

    return TRUE;
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
    DWORD err;

    TRACE("%p\n", hSCObject);

    __TRY
    {
        err = svcctl_CloseServiceHandle((SC_RPC_HANDLE *)&hSCObject);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return FALSE;
    }
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

    TRACE("%p %s 0x%08x\n", hSCManager, debugstr_a(lpServiceName), dwDesiredAccess);

    lpServiceNameW = SERV_dup(lpServiceName);
    ret = OpenServiceW( hSCManager, lpServiceNameW, dwDesiredAccess);
    heap_free(lpServiceNameW);
    return ret;
}


/******************************************************************************
 * OpenServiceW [ADVAPI32.@]
 *
 * See OpenServiceA.
 */
static DWORD SERV_OpenServiceW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
                         DWORD dwDesiredAccess, SC_HANDLE *handle )
{
    DWORD err;

    TRACE("%p %s 0x%08x\n", hSCManager, debugstr_w(lpServiceName), dwDesiredAccess);

    if (!hSCManager)
        return ERROR_INVALID_HANDLE;

    __TRY
    {
        err = svcctl_OpenServiceW(hSCManager, lpServiceName, dwDesiredAccess, (SC_RPC_HANDLE *)handle);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
        *handle = 0;

    TRACE("returning %p\n", *handle);
    return err;
}

SC_HANDLE WINAPI OpenServiceW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
                               DWORD dwDesiredAccess)
{
    SC_HANDLE handle = 0;
    DWORD err;

    err = SERV_OpenServiceW(hSCManager, lpServiceName, dwDesiredAccess, &handle);
    if (err != ERROR_SUCCESS)
        SetLastError(err);
    return handle;
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
    SC_HANDLE handle = 0;
    DWORD err;
    SIZE_T passwdlen;

    TRACE("%p %s %s\n", hSCManager, 
          debugstr_w(lpServiceName), debugstr_w(lpDisplayName));

    if (!hSCManager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if (lpPassword)
        passwdlen = (strlenW(lpPassword) + 1) * sizeof(WCHAR);
    else
        passwdlen = 0;

    __TRY
    {
        BOOL is_wow64;

        IsWow64Process(GetCurrentProcess(), &is_wow64);

        if (is_wow64)
            err = svcctl_CreateServiceWOW64W(hSCManager, lpServiceName,
                    lpDisplayName, dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
                    lpBinaryPathName, lpLoadOrderGroup, lpdwTagId, (const BYTE*)lpDependencies,
                    multisz_cb(lpDependencies), lpServiceStartName, (const BYTE*)lpPassword, passwdlen,
                    (SC_RPC_HANDLE *)&handle);
        else
            err = svcctl_CreateServiceW(hSCManager, lpServiceName,
                    lpDisplayName, dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
                    lpBinaryPathName, lpLoadOrderGroup, lpdwTagId, (const BYTE*)lpDependencies,
                    multisz_cb(lpDependencies), lpServiceStartName, (const BYTE*)lpPassword, passwdlen,
                    (SC_RPC_HANDLE *)&handle);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        handle = 0;
    }
    return handle;
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

    heap_free( lpServiceNameW );
    heap_free( lpDisplayNameW );
    heap_free( lpBinaryPathNameW );
    heap_free( lpLoadOrderGroupW );
    heap_free( lpDependenciesW );
    heap_free( lpServiceStartNameW );
    heap_free( lpPasswordW );

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
    DWORD err;

    TRACE("%p\n", hService);

    __TRY
    {
        err = svcctl_DeleteService(hService);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY
    if (err != 0)
    {
        SetLastError(err);
        return FALSE;
    }

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
BOOL WINAPI StartServiceA( SC_HANDLE hService, DWORD dwNumServiceArgs,
                           LPCSTR *lpServiceArgVectors )
{
    LPWSTR *lpwstr=NULL;
    unsigned int i;
    BOOL r;

    TRACE("(%p,%d,%p)\n",hService,dwNumServiceArgs,lpServiceArgVectors);

    if (dwNumServiceArgs)
        lpwstr = heap_alloc( dwNumServiceArgs*sizeof(LPWSTR) );

    for(i=0; i<dwNumServiceArgs; i++)
        lpwstr[i]=SERV_dup(lpServiceArgVectors[i]);

    r = StartServiceW(hService, dwNumServiceArgs, (LPCWSTR *)lpwstr);

    if (dwNumServiceArgs)
    {
        for(i=0; i<dwNumServiceArgs; i++)
            heap_free(lpwstr[i]);
        heap_free(lpwstr);
    }

    return r;
}


/******************************************************************************
 * StartServiceW [ADVAPI32.@]
 * 
 * See StartServiceA.
 */
BOOL WINAPI StartServiceW(SC_HANDLE hService, DWORD dwNumServiceArgs,
                          LPCWSTR *lpServiceArgVectors)
{
    DWORD err;

    TRACE("%p %d %p\n", hService, dwNumServiceArgs, lpServiceArgVectors);

    __TRY
    {
        err = svcctl_StartServiceW(hService, dwNumServiceArgs, lpServiceArgVectors);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY
    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * QueryServiceStatus [ADVAPI32.@]
 *
 * PARAMS
 *   hService        [I] Handle to service to get information about
 *   lpservicestatus [O] buffer to receive the status information for the service
 *
 */
BOOL WINAPI QueryServiceStatus(SC_HANDLE hService,
                               LPSERVICE_STATUS lpservicestatus)
{
    SERVICE_STATUS_PROCESS SvcStatusData;
    BOOL ret;
    DWORD dummy;

    TRACE("%p %p\n", hService, lpservicestatus);

    if (!hService)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!lpservicestatus)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    ret = QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&SvcStatusData,
                                sizeof(SERVICE_STATUS_PROCESS), &dummy);
    if (ret) memcpy(lpservicestatus, &SvcStatusData, sizeof(SERVICE_STATUS)) ;
    return ret;
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
    DWORD err;

    TRACE("%p %d %p %d %p\n", hService, InfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    if (InfoLevel != SC_STATUS_PROCESS_INFO)
    {
        err = ERROR_INVALID_LEVEL;
    }
    else if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS))
    {
        *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
        err = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        __TRY
        {
            err = svcctl_QueryServiceStatusEx(hService, InfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);
        }
        __EXCEPT(rpc_filter)
        {
            err = map_exception_code(GetExceptionCode());
        }
        __ENDTRY
    }
    if (err != ERROR_SUCCESS)
    {
        SetLastError(err);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * QueryServiceConfigA [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceConfigA( SC_HANDLE hService, LPQUERY_SERVICE_CONFIGA config,
                                 DWORD size, LPDWORD needed )
{
    DWORD n;
    LPSTR p, buffer;
    BOOL ret;
    QUERY_SERVICE_CONFIGW *configW;

    TRACE("%p %p %d %p\n", hService, config, size, needed);

    if (!(buffer = heap_alloc( 2 * size )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    configW = (QUERY_SERVICE_CONFIGW *)buffer;
    ret = QueryServiceConfigW( hService, configW, 2 * size, needed );
    if (!ret) goto done;

    config->dwServiceType      = configW->dwServiceType;
    config->dwStartType        = configW->dwStartType;
    config->dwErrorControl     = configW->dwErrorControl;
    config->lpBinaryPathName   = NULL;
    config->lpLoadOrderGroup   = NULL;
    config->dwTagId            = configW->dwTagId;
    config->lpDependencies     = NULL;
    config->lpServiceStartName = NULL;
    config->lpDisplayName      = NULL;

    p = (LPSTR)(config + 1);
    n = size - sizeof(*config);
    ret = FALSE;

#define MAP_STR(str) \
    do { \
        if (configW->str) \
        { \
            DWORD sz = WideCharToMultiByte( CP_ACP, 0, configW->str, -1, p, n, NULL, NULL ); \
            if (!sz) goto done; \
            config->str = p; \
            p += sz; \
            n -= sz; \
        } \
    } while (0)

    MAP_STR( lpBinaryPathName );
    MAP_STR( lpLoadOrderGroup );
    MAP_STR( lpDependencies );
    MAP_STR( lpServiceStartName );
    MAP_STR( lpDisplayName );
#undef MAP_STR

    *needed = p - (LPSTR)config;
    ret = TRUE;

done:
    heap_free( buffer );
    return ret;
}

static DWORD move_string_to_buffer(BYTE **buf, LPWSTR *string_ptr)
{
    DWORD cb;

    if (!*string_ptr)
    {
        cb = sizeof(WCHAR);
        memset(*buf, 0, cb);
    }
    else
    {
        cb = (strlenW(*string_ptr) + 1)*sizeof(WCHAR);
        memcpy(*buf, *string_ptr, cb);
        MIDL_user_free(*string_ptr);
    }

    *string_ptr = (LPWSTR)*buf;
    *buf += cb;

    return cb;
}

static DWORD size_string(LPCWSTR string)
{
    return (string ? (strlenW(string) + 1)*sizeof(WCHAR) : sizeof(WCHAR));
}

/******************************************************************************
 * QueryServiceConfigW [ADVAPI32.@]
 */
BOOL WINAPI 
QueryServiceConfigW( SC_HANDLE hService,
                     LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    QUERY_SERVICE_CONFIGW config;
    DWORD total;
    DWORD err;
    BYTE *bufpos;

    TRACE("%p %p %d %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

    memset(&config, 0, sizeof(config));

    __TRY
    {
        err = svcctl_QueryServiceConfigW(hService, &config, cbBufSize, pcbBytesNeeded);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
    {
        TRACE("services.exe: error %u\n", err);
        SetLastError(err);
        return FALSE;
    }

    /* calculate the size required first */
    total = sizeof (QUERY_SERVICE_CONFIGW);
    total += size_string(config.lpBinaryPathName);
    total += size_string(config.lpLoadOrderGroup);
    total += size_string(config.lpDependencies);
    total += size_string(config.lpServiceStartName);
    total += size_string(config.lpDisplayName);

    *pcbBytesNeeded = total;

    /* if there's not enough memory, return an error */
    if( total > cbBufSize )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        MIDL_user_free(config.lpBinaryPathName);
        MIDL_user_free(config.lpLoadOrderGroup);
        MIDL_user_free(config.lpDependencies);
        MIDL_user_free(config.lpServiceStartName);
        MIDL_user_free(config.lpDisplayName);
        return FALSE;
    }

    *lpServiceConfig = config;
    bufpos = ((BYTE *)lpServiceConfig) + sizeof(QUERY_SERVICE_CONFIGW);
    move_string_to_buffer(&bufpos, &lpServiceConfig->lpBinaryPathName);
    move_string_to_buffer(&bufpos, &lpServiceConfig->lpLoadOrderGroup);
    move_string_to_buffer(&bufpos, &lpServiceConfig->lpDependencies);
    move_string_to_buffer(&bufpos, &lpServiceConfig->lpServiceStartName);
    move_string_to_buffer(&bufpos, &lpServiceConfig->lpDisplayName);

    TRACE("Image path           = %s\n", debugstr_w(lpServiceConfig->lpBinaryPathName) );
    TRACE("Group                = %s\n", debugstr_w(lpServiceConfig->lpLoadOrderGroup) );
    TRACE("Dependencies         = %s\n", debugstr_w(lpServiceConfig->lpDependencies) );
    TRACE("Service account name = %s\n", debugstr_w(lpServiceConfig->lpServiceStartName) );
    TRACE("Display name         = %s\n", debugstr_w(lpServiceConfig->lpDisplayName) );

    return TRUE;
}

/******************************************************************************
 * QueryServiceConfig2A [ADVAPI32.@]
 *
 * Note
 *   observed under win2k:
 *   The functions QueryServiceConfig2A and QueryServiceConfig2W return the same
 *   required buffer size (in byte) at least for dwLevel SERVICE_CONFIG_DESCRIPTION
 */
BOOL WINAPI QueryServiceConfig2A(SC_HANDLE hService, DWORD dwLevel, LPBYTE buffer,
                                 DWORD size, LPDWORD needed)
{
    BOOL ret;
    LPBYTE bufferW = NULL;

    TRACE("%p %u %p %u %p\n", hService, dwLevel, buffer, size, needed);

    if(buffer && size)
        bufferW = heap_alloc(size);

    ret = QueryServiceConfig2W(hService, dwLevel, bufferW, size, needed);
    if(!ret) goto cleanup;

    switch(dwLevel) {
        case SERVICE_CONFIG_DESCRIPTION:
            if (buffer && bufferW) {
                LPSERVICE_DESCRIPTIONA configA = (LPSERVICE_DESCRIPTIONA) buffer;
                LPSERVICE_DESCRIPTIONW configW = (LPSERVICE_DESCRIPTIONW) bufferW;
                if (configW->lpDescription && (size > sizeof(SERVICE_DESCRIPTIONA))) {
                    DWORD sz;
                    configA->lpDescription = (LPSTR)(configA + 1);
                    sz = WideCharToMultiByte( CP_ACP, 0, configW->lpDescription, -1,
                             configA->lpDescription, size - sizeof(SERVICE_DESCRIPTIONA), NULL, NULL );
                    if (!sz) {
                        FIXME("WideCharToMultiByte failed for configW->lpDescription\n");
                        ret = FALSE;
                        configA->lpDescription = NULL;
                    }
                }
                else configA->lpDescription = NULL;
            }
            break;
        case SERVICE_CONFIG_PRESHUTDOWN_INFO:
            if (buffer && bufferW && *needed<=size)
                memcpy(buffer, bufferW, *needed);
            break;
        default:
            FIXME("conversation W->A not implemented for level %d\n", dwLevel);
            ret = FALSE;
            break;
    }

cleanup:
    heap_free( bufferW);
    return ret;
}

/******************************************************************************
 * QueryServiceConfig2W [ADVAPI32.@]
 *
 * See QueryServiceConfig2A.
 */
BOOL WINAPI QueryServiceConfig2W(SC_HANDLE hService, DWORD dwLevel, LPBYTE buffer,
                                 DWORD size, LPDWORD needed)
{
    BYTE *bufptr;
    DWORD err;

    TRACE("%p %u %p %u %p\n", hService, dwLevel, buffer, size, needed);

    if (!buffer && size)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    switch (dwLevel)
    {
    case SERVICE_CONFIG_DESCRIPTION:
        if (!(bufptr = heap_alloc( size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        break;

    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
        bufptr = buffer;
        break;

    default:
        FIXME("Level %d not implemented\n", dwLevel);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!needed)
    {
        if (dwLevel == SERVICE_CONFIG_DESCRIPTION) heap_free(bufptr);
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    __TRY
    {
        err = svcctl_QueryServiceConfig2W(hService, dwLevel, bufptr, size, needed);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    switch (dwLevel)
    {
    case SERVICE_CONFIG_DESCRIPTION:
    {
        SERVICE_DESCRIPTIONW *desc = (SERVICE_DESCRIPTIONW *)buffer;
        struct service_description *s = (struct service_description *)bufptr;

        if (err != ERROR_SUCCESS && err != ERROR_INSUFFICIENT_BUFFER)
        {
            heap_free( bufptr );
            SetLastError( err );
            return FALSE;
        }

        /* adjust for potentially larger SERVICE_DESCRIPTIONW structure */
        if (*needed == sizeof(*s)) *needed = sizeof(*desc);
        else
            *needed = *needed - FIELD_OFFSET(struct service_description, description) + sizeof(*desc);

        if (size < *needed)
        {
            heap_free( bufptr );
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        if (desc)
        {
            if (!s->size) desc->lpDescription = NULL;
            else
            {
                desc->lpDescription = (WCHAR *)(desc + 1);
                memcpy( desc->lpDescription, s->description, s->size );
            }
        }
        heap_free( bufptr );
        break;
    }
    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
        if (err != ERROR_SUCCESS)
        {
            SetLastError( err );
            return FALSE;
        }
        break;

    default:
        break;
    }

    return TRUE;
}

/******************************************************************************
 * EnumServicesStatusA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusA( SC_HANDLE hmngr, DWORD type, DWORD state, LPENUM_SERVICE_STATUSA
                     services, DWORD size, LPDWORD needed, LPDWORD returned,
                     LPDWORD resume_handle )
{
    BOOL ret;
    unsigned int i;
    ENUM_SERVICE_STATUSW *servicesW = NULL;
    DWORD sz, n;
    char *p;

    TRACE("%p 0x%x 0x%x %p %u %p %p %p\n", hmngr, type, state, services, size, needed,
          returned, resume_handle);

    if (!hmngr)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (!needed || !returned)
    {
        SetLastError( ERROR_INVALID_ADDRESS );
        return FALSE;
    }

    sz = max( 2 * size, sizeof(*servicesW) );
    if (!(servicesW = heap_alloc( sz )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    ret = EnumServicesStatusW( hmngr, type, state, servicesW, sz, needed, returned, resume_handle );
    if (!ret) goto done;

    p = (char *)services + *returned * sizeof(ENUM_SERVICE_STATUSA);
    n = size - (p - (char *)services);
    ret = FALSE;
    for (i = 0; i < *returned; i++)
    {
        sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpServiceName, -1, p, n, NULL, NULL );
        if (!sz) goto done;
        services[i].lpServiceName = p;
        p += sz;
        n -= sz;
        if (servicesW[i].lpDisplayName)
        {
            sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpDisplayName, -1, p, n, NULL, NULL );
            if (!sz) goto done;
            services[i].lpDisplayName = p;
            p += sz;
            n -= sz;
        }
        else services[i].lpDisplayName = NULL;
        services[i].ServiceStatus = servicesW[i].ServiceStatus;
    }

    ret = TRUE;

done:
    heap_free( servicesW );
    return ret;
}

/******************************************************************************
 * EnumServicesStatusW [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusW( SC_HANDLE hmngr, DWORD type, DWORD state, LPENUM_SERVICE_STATUSW
                     services, DWORD size, LPDWORD needed, LPDWORD returned,
                     LPDWORD resume_handle )
{
    DWORD err, i, offset, buflen, count, total_size = 0;
    struct enum_service_status *entry;
    const WCHAR *str;
    BYTE *buf;

    TRACE("%p 0x%x 0x%x %p %u %p %p %p\n", hmngr, type, state, services, size, needed,
          returned, resume_handle);

    if (!hmngr)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (!needed || !returned)
    {
        SetLastError( ERROR_INVALID_ADDRESS );
        return FALSE;
    }

    /* make sure we pass a valid pointer */
    buflen = max( size, sizeof(*services) );
    if (!(buf = heap_alloc( buflen )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    __TRY
    {
        err = svcctl_EnumServicesStatusW( hmngr, type, state, buf, buflen, needed, &count, resume_handle );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    *returned = 0;
    if (err != ERROR_SUCCESS)
    {
        /* double the needed size to fit the potentially larger ENUM_SERVICE_STATUSW */
        if (err == ERROR_MORE_DATA) *needed *= 2;
        heap_free( buf );
        SetLastError( err );
        return FALSE;
    }

    entry = (struct enum_service_status *)buf;
    for (i = 0; i < count; i++)
    {
        total_size += sizeof(*services);
        if (entry->service_name)
        {
            str = (const WCHAR *)(buf + entry->service_name);
            total_size += (strlenW( str ) + 1) * sizeof(WCHAR);
        }
        if (entry->display_name)
        {
            str = (const WCHAR *)(buf + entry->display_name);
            total_size += (strlenW( str ) + 1) * sizeof(WCHAR);
        }
        entry++;
    }

    if (total_size > size)
    {
        heap_free( buf );
        *needed = total_size;
        SetLastError( ERROR_MORE_DATA );
        return FALSE;
    }

    offset = count * sizeof(*services);
    entry = (struct enum_service_status *)buf;
    for (i = 0; i < count; i++)
    {
        DWORD str_size;
        str = (const WCHAR *)(buf + entry->service_name);
        str_size = (strlenW( str ) + 1) * sizeof(WCHAR);
        services[i].lpServiceName = (WCHAR *)((char *)services + offset);
        memcpy( services[i].lpServiceName, str, str_size );
        offset += str_size;

        if (!entry->display_name) services[i].lpDisplayName = NULL;
        else
        {
            str = (const WCHAR *)(buf + entry->display_name);
            str_size = (strlenW( str ) + 1) * sizeof(WCHAR);
            services[i].lpDisplayName = (WCHAR *)((char *)services + offset);
            memcpy( services[i].lpDisplayName, str, str_size );
            offset += str_size;
        }
        services[i].ServiceStatus = entry->service_status;
        entry++;
    }

    heap_free( buf );
    *needed = 0;
    *returned = count;
    return TRUE;
}

/******************************************************************************
 * EnumServicesStatusExA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusExA( SC_HANDLE hmngr, SC_ENUM_TYPE level, DWORD type, DWORD state,
                       LPBYTE buffer, DWORD size, LPDWORD needed, LPDWORD returned,
                       LPDWORD resume_handle, LPCSTR group )
{
    BOOL ret;
    unsigned int i;
    ENUM_SERVICE_STATUS_PROCESSA *services = (ENUM_SERVICE_STATUS_PROCESSA *)buffer;
    ENUM_SERVICE_STATUS_PROCESSW *servicesW = NULL;
    WCHAR *groupW = NULL;
    DWORD sz, n;
    char *p;

    TRACE("%p %u 0x%x 0x%x %p %u %p %p %p %s\n", hmngr, level, type, state, buffer,
          size, needed, returned, resume_handle, debugstr_a(group));

    sz = max( 2 * size, sizeof(*servicesW) );
    if (!(servicesW = heap_alloc( sz )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    if (group)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, group, -1, NULL, 0 );
        if (!(groupW = heap_alloc( len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            heap_free( servicesW );
            return FALSE;
        }
        MultiByteToWideChar( CP_ACP, 0, group, -1, groupW, len * sizeof(WCHAR) );
    }

    ret = EnumServicesStatusExW( hmngr, level, type, state, (BYTE *)servicesW, sz,
                                 needed, returned, resume_handle, groupW );
    if (!ret) goto done;

    p = (char *)services + *returned * sizeof(ENUM_SERVICE_STATUS_PROCESSA);
    n = size - (p - (char *)services);
    ret = FALSE;
    for (i = 0; i < *returned; i++)
    {
        sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpServiceName, -1, p, n, NULL, NULL );
        if (!sz) goto done;
        services[i].lpServiceName = p;
        p += sz;
        n -= sz;
        if (servicesW[i].lpDisplayName)
        {
            sz = WideCharToMultiByte( CP_ACP, 0, servicesW[i].lpDisplayName, -1, p, n, NULL, NULL );
            if (!sz) goto done;
            services[i].lpDisplayName = p;
            p += sz;
            n -= sz;
        }
        else services[i].lpDisplayName = NULL;
        services[i].ServiceStatusProcess = servicesW[i].ServiceStatusProcess;
    }

    ret = TRUE;

done:
    heap_free( servicesW );
    heap_free( groupW );
    return ret;
}

/******************************************************************************
 * EnumServicesStatusExW [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusExW( SC_HANDLE hmngr, SC_ENUM_TYPE level, DWORD type, DWORD state,
                       LPBYTE buffer, DWORD size, LPDWORD needed, LPDWORD returned,
                       LPDWORD resume_handle, LPCWSTR group )
{
    DWORD err, i, offset, buflen, count, total_size = 0;
    ENUM_SERVICE_STATUS_PROCESSW *services = (ENUM_SERVICE_STATUS_PROCESSW *)buffer;
    struct enum_service_status_process *entry;
    const WCHAR *str;
    BYTE *buf;

    TRACE("%p %u 0x%x 0x%x %p %u %p %p %p %s\n", hmngr, level, type, state, buffer,
          size, needed, returned, resume_handle, debugstr_w(group));

    if (level != SC_ENUM_PROCESS_INFO)
    {
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }
    if (!hmngr)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (!needed || !returned)
    {
        SetLastError( ERROR_INVALID_ADDRESS );
        return FALSE;
    }

    /* make sure we pass a valid pointer */
    buflen = max( size, sizeof(*services) );
    if (!(buf = heap_alloc( buflen )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    __TRY
    {
        err = svcctl_EnumServicesStatusExW( hmngr, SC_ENUM_PROCESS_INFO, type, state, buf, buflen, needed,
                                            &count, resume_handle, group );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code( GetExceptionCode() );
    }
    __ENDTRY

    *returned = 0;
    if (err != ERROR_SUCCESS)
    {
        /* double the needed size to fit the potentially larger ENUM_SERVICE_STATUS_PROCESSW */
        if (err == ERROR_MORE_DATA) *needed *= 2;
        heap_free( buf );
        SetLastError( err );
        return FALSE;
    }

    entry = (struct enum_service_status_process *)buf;
    for (i = 0; i < count; i++)
    {
        total_size += sizeof(*services);
        if (entry->service_name)
        {
            str = (const WCHAR *)(buf + entry->service_name);
            total_size += (strlenW( str ) + 1) * sizeof(WCHAR);
        }
        if (entry->display_name)
        {
            str = (const WCHAR *)(buf + entry->display_name);
            total_size += (strlenW( str ) + 1) * sizeof(WCHAR);
        }
        entry++;
    }

    if (total_size > size)
    {
        heap_free( buf );
        *needed = total_size;
        SetLastError( ERROR_MORE_DATA );
        return FALSE;
    }

    offset = count * sizeof(*services);
    entry = (struct enum_service_status_process *)buf;
    for (i = 0; i < count; i++)
    {
        DWORD str_size;
        str = (const WCHAR *)(buf + entry->service_name);
        str_size = (strlenW( str ) + 1) * sizeof(WCHAR);
        services[i].lpServiceName = (WCHAR *)((char *)services + offset);
        memcpy( services[i].lpServiceName, str, str_size );
        offset += str_size;

        if (!entry->display_name) services[i].lpDisplayName = NULL;
        else
        {
            str = (const WCHAR *)(buf + entry->display_name);
            str_size = (strlenW( str ) + 1) * sizeof(WCHAR);
            services[i].lpDisplayName = (WCHAR *)((char *)services + offset);
            memcpy( services[i].lpDisplayName, str, str_size );
            offset += str_size;
        }
        services[i].ServiceStatusProcess = entry->service_status_process;
        entry++;
    }

    heap_free( buf );
    *needed = 0;
    *returned = count;
    return TRUE;
}

/******************************************************************************
 * GetServiceKeyNameA [ADVAPI32.@]
 */
BOOL WINAPI GetServiceKeyNameA( SC_HANDLE hSCManager, LPCSTR lpDisplayName,
                                LPSTR lpServiceName, LPDWORD lpcchBuffer )
{
    LPWSTR lpDisplayNameW, lpServiceNameW;
    DWORD sizeW;
    BOOL ret = FALSE;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_a(lpDisplayName), lpServiceName, lpcchBuffer);

    lpDisplayNameW = SERV_dup(lpDisplayName);
    if (lpServiceName)
        lpServiceNameW = heap_alloc(*lpcchBuffer * sizeof(WCHAR));
    else
        lpServiceNameW = NULL;

    sizeW = *lpcchBuffer;
    if (!GetServiceKeyNameW(hSCManager, lpDisplayNameW, lpServiceNameW, &sizeW))
    {
        if (lpServiceName && *lpcchBuffer)
            lpServiceName[0] = 0;
        *lpcchBuffer = sizeW*2;  /* we can only provide an upper estimation of string length */
        goto cleanup;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpServiceNameW, (sizeW + 1), lpServiceName,
                        *lpcchBuffer, NULL, NULL ))
    {
        if (*lpcchBuffer && lpServiceName)
            lpServiceName[0] = 0;
        *lpcchBuffer = WideCharToMultiByte(CP_ACP, 0, lpServiceNameW, -1, NULL, 0, NULL, NULL);
        goto cleanup;
    }

    /* lpcchBuffer not updated - same as in GetServiceDisplayNameA */
    ret = TRUE;

cleanup:
    heap_free(lpServiceNameW);
    heap_free(lpDisplayNameW);
    return ret;
}

/******************************************************************************
 * GetServiceKeyNameW [ADVAPI32.@]
 */
BOOL WINAPI GetServiceKeyNameW( SC_HANDLE hSCManager, LPCWSTR lpDisplayName,
                                LPWSTR lpServiceName, LPDWORD lpcchBuffer )
{
    DWORD err;
    WCHAR buffer[2];
    DWORD size;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_w(lpDisplayName), lpServiceName, lpcchBuffer);

    if (!hSCManager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    /* provide a buffer if the caller didn't */
    if (!lpServiceName || *lpcchBuffer < 2)
    {
        lpServiceName = buffer;
        /* A size of 1 would be enough, but tests show that Windows returns 2,
         * probably because of a WCHAR/bytes mismatch in their code.
         */
        *lpcchBuffer = 2;
    }

    /* RPC call takes size excluding nul-terminator, whereas *lpcchBuffer
     * includes the nul-terminator on input. */
    size = *lpcchBuffer - 1;

    __TRY
    {
        err = svcctl_GetServiceKeyNameW(hSCManager, lpDisplayName, lpServiceName,
                                        &size);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    /* The value of *lpcchBuffer excludes nul-terminator on output. */
    if (err == ERROR_SUCCESS || err == ERROR_INSUFFICIENT_BUFFER)
        *lpcchBuffer = size;

    if (err)
        SetLastError(err);
    return err == ERROR_SUCCESS;
}

/******************************************************************************
 * QueryServiceLockStatusA [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceLockStatusA( SC_HANDLE hSCManager,
                                     LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
                                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    FIXME("%p %p %08x %p\n", hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

    return FALSE;
}

/******************************************************************************
 * QueryServiceLockStatusW [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceLockStatusW( SC_HANDLE hSCManager,
                                     LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
                                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    FIXME("%p %p %08x %p\n", hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

    return FALSE;
}

/******************************************************************************
 * GetServiceDisplayNameA  [ADVAPI32.@]
 */
BOOL WINAPI GetServiceDisplayNameA( SC_HANDLE hSCManager, LPCSTR lpServiceName,
  LPSTR lpDisplayName, LPDWORD lpcchBuffer)
{
    LPWSTR lpServiceNameW, lpDisplayNameW;
    DWORD sizeW;
    BOOL ret = FALSE;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_a(lpServiceName), lpDisplayName, lpcchBuffer);

    lpServiceNameW = SERV_dup(lpServiceName);
    if (lpDisplayName)
        lpDisplayNameW = heap_alloc(*lpcchBuffer * sizeof(WCHAR));
    else
        lpDisplayNameW = NULL;

    sizeW = *lpcchBuffer;
    if (!GetServiceDisplayNameW(hSCManager, lpServiceNameW, lpDisplayNameW, &sizeW))
    {
        if (lpDisplayName && *lpcchBuffer)
            lpDisplayName[0] = 0;
        *lpcchBuffer = sizeW*2;  /* we can only provide an upper estimation of string length */
        goto cleanup;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, (sizeW + 1), lpDisplayName,
                        *lpcchBuffer, NULL, NULL ))
    {
        if (*lpcchBuffer && lpDisplayName)
            lpDisplayName[0] = 0;
        *lpcchBuffer = WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, -1, NULL, 0, NULL, NULL);
        goto cleanup;
    }

    /* probably due to a bug GetServiceDisplayNameA doesn't modify lpcchBuffer on success.
     * (but if the function succeeded it means that is a good upper estimation of the size) */
    ret = TRUE;

cleanup:
    heap_free(lpDisplayNameW);
    heap_free(lpServiceNameW);
    return ret;
}

/******************************************************************************
 * GetServiceDisplayNameW  [ADVAPI32.@]
 */
BOOL WINAPI GetServiceDisplayNameW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
  LPWSTR lpDisplayName, LPDWORD lpcchBuffer)
{
    DWORD err;
    DWORD size;
    WCHAR buffer[2];

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_w(lpServiceName), lpDisplayName, lpcchBuffer);

    if (!hSCManager)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    /* provide a buffer if the caller didn't */
    if (!lpDisplayName || *lpcchBuffer < 2)
    {
        lpDisplayName = buffer;
        /* A size of 1 would be enough, but tests show that Windows returns 2,
         * probably because of a WCHAR/bytes mismatch in their code.
         */
        *lpcchBuffer = 2;
    }

    /* RPC call takes size excluding nul-terminator, whereas *lpcchBuffer
     * includes the nul-terminator on input. */
    size = *lpcchBuffer - 1;

    __TRY
    {
        err = svcctl_GetServiceDisplayNameW(hSCManager, lpServiceName, lpDisplayName,
                                            &size);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    /* The value of *lpcchBuffer excludes nul-terminator on output. */
    if (err == ERROR_SUCCESS || err == ERROR_INSUFFICIENT_BUFFER)
        *lpcchBuffer = size;

    if (err)
        SetLastError(err);
    return err == ERROR_SUCCESS;
}

/******************************************************************************
 * ChangeServiceConfigW  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfigW( SC_HANDLE hService, DWORD dwServiceType,
  DWORD dwStartType, DWORD dwErrorControl, LPCWSTR lpBinaryPathName,
  LPCWSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCWSTR lpDependencies,
  LPCWSTR lpServiceStartName, LPCWSTR lpPassword, LPCWSTR lpDisplayName)
{
    DWORD cb_pwd;
    DWORD err;

    TRACE("%p %d %d %d %s %s %p %p %s %s %s\n",
          hService, dwServiceType, dwStartType, dwErrorControl, 
          debugstr_w(lpBinaryPathName), debugstr_w(lpLoadOrderGroup),
          lpdwTagId, lpDependencies, debugstr_w(lpServiceStartName),
          debugstr_w(lpPassword), debugstr_w(lpDisplayName) );

    cb_pwd = lpPassword ? (strlenW(lpPassword) + 1)*sizeof(WCHAR) : 0;

    __TRY
    {
        err = svcctl_ChangeServiceConfigW(hService, dwServiceType,
                dwStartType, dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, lpdwTagId,
                (const BYTE *)lpDependencies, multisz_cb(lpDependencies), lpServiceStartName,
                (const BYTE *)lpPassword, cb_pwd, lpDisplayName);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
        SetLastError(err);

    return err == ERROR_SUCCESS;
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

    TRACE("%p %d %d %d %s %s %p %p %s %s %s\n",
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

    heap_free( wBinaryPathName );
    heap_free( wLoadOrderGroup );
    heap_free( wDependencies );
    heap_free( wServiceStartName );
    heap_free( wPassword );
    heap_free( wDisplayName );

    return r;
}

/******************************************************************************
 * ChangeServiceConfig2A  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfig2A( SC_HANDLE hService, DWORD dwInfoLevel, 
    LPVOID lpInfo)
{
    BOOL r = FALSE;

    TRACE("%p %d %p\n",hService, dwInfoLevel, lpInfo);

    if (dwInfoLevel == SERVICE_CONFIG_DESCRIPTION)
    {
        LPSERVICE_DESCRIPTIONA sd = lpInfo;
        SERVICE_DESCRIPTIONW sdw;

        sdw.lpDescription = SERV_dup( sd->lpDescription );

        r = ChangeServiceConfig2W( hService, dwInfoLevel, &sdw );

        heap_free( sdw.lpDescription );
    }
    else if (dwInfoLevel == SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        LPSERVICE_FAILURE_ACTIONSA fa = lpInfo;
        SERVICE_FAILURE_ACTIONSW faw;

        faw.dwResetPeriod = fa->dwResetPeriod;
        faw.lpRebootMsg = SERV_dup( fa->lpRebootMsg );
        faw.lpCommand = SERV_dup( fa->lpCommand );
        faw.cActions = fa->cActions;
        faw.lpsaActions = fa->lpsaActions;

        r = ChangeServiceConfig2W( hService, dwInfoLevel, &faw );

        heap_free( faw.lpRebootMsg );
        heap_free( faw.lpCommand );
    }
    else if (dwInfoLevel == SERVICE_CONFIG_PRESHUTDOWN_INFO)
    {
        r = ChangeServiceConfig2W( hService, dwInfoLevel, lpInfo);
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
    SERVICE_RPC_REQUIRED_PRIVILEGES_INFO rpc_privinfo;
    DWORD err;

    __TRY
    {
        SC_RPC_CONFIG_INFOW info;

        info.dwInfoLevel = dwInfoLevel;
        if (dwInfoLevel == SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO)
        {
            SERVICE_REQUIRED_PRIVILEGES_INFOW *privinfo = lpInfo;
            WCHAR *p;

            for (p = privinfo->pmszRequiredPrivileges; *p; p += strlenW(p) + 1);
            rpc_privinfo.cbRequiredPrivileges =
                (p - privinfo->pmszRequiredPrivileges + 1) * sizeof(WCHAR);
            rpc_privinfo.pRequiredPrivileges = (BYTE *)privinfo->pmszRequiredPrivileges;
            info.u.privinfo = &rpc_privinfo;
        }
        else
            info.u.descr = lpInfo;
        err = svcctl_ChangeServiceConfig2W( hService, info );
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
        SetLastError(err);

    return err == ERROR_SUCCESS;
}

static NTSTATUS SERV_QueryServiceObjectSecurity(SC_HANDLE hService,
       SECURITY_INFORMATION dwSecurityInformation,
       PSECURITY_DESCRIPTOR lpSecurityDescriptor,
       DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    SECURITY_DESCRIPTOR descriptor;
    NTSTATUS status;
    DWORD size;
    ACL acl;

    FIXME("%p %d %p %u %p - semi-stub\n", hService, dwSecurityInformation,
          lpSecurityDescriptor, cbBufSize, pcbBytesNeeded);

    if (dwSecurityInformation != DACL_SECURITY_INFORMATION)
        FIXME("information %d not supported\n", dwSecurityInformation);

    InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION);

    InitializeAcl(&acl, sizeof(ACL), ACL_REVISION);
    SetSecurityDescriptorDacl(&descriptor, TRUE, &acl, TRUE);

    size = cbBufSize;
    status = RtlMakeSelfRelativeSD(&descriptor, lpSecurityDescriptor, &size);
    *pcbBytesNeeded = size;
    return status;
}

/******************************************************************************
 * QueryServiceObjectSecurity [ADVAPI32.@]
 */
BOOL WINAPI QueryServiceObjectSecurity(SC_HANDLE hService,
       SECURITY_INFORMATION dwSecurityInformation,
       PSECURITY_DESCRIPTOR lpSecurityDescriptor,
       DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    NTSTATUS status = SERV_QueryServiceObjectSecurity(hService, dwSecurityInformation, lpSecurityDescriptor,
                                                      cbBufSize, pcbBytesNeeded);
    if (status != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * SetServiceObjectSecurity [ADVAPI32.@]
 *
 * NOTES
 *  - SetSecurityInfo should be updated to call this function once it's implemented.
 */
BOOL WINAPI SetServiceObjectSecurity(SC_HANDLE hService,
       SECURITY_INFORMATION dwSecurityInformation,
       PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    FIXME("%p %d %p\n", hService, dwSecurityInformation, lpSecurityDescriptor);
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
    FIXME("%p %08x %x %x\n", hServiceStatus, dwServiceBits,
          bSetBitsOn, bUpdateImmediately);
    return TRUE;
}

/* thunk for calling the RegisterServiceCtrlHandler handler function */
static DWORD WINAPI ctrl_handler_thunk( DWORD control, DWORD type, void *data, void *context )
{
    LPHANDLER_FUNCTION func = context;

    func( control );
    return ERROR_SUCCESS;
}

/******************************************************************************
 * RegisterServiceCtrlHandlerA [ADVAPI32.@]
 */
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerA( LPCSTR name, LPHANDLER_FUNCTION handler )
{
    return RegisterServiceCtrlHandlerExA( name, ctrl_handler_thunk, handler );
}

/******************************************************************************
 * RegisterServiceCtrlHandlerW [ADVAPI32.@]
 */
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW( LPCWSTR name, LPHANDLER_FUNCTION handler )
{
    return RegisterServiceCtrlHandlerExW( name, ctrl_handler_thunk, handler );
}

/******************************************************************************
 * RegisterServiceCtrlHandlerExA [ADVAPI32.@]
 */
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExA( LPCSTR name, LPHANDLER_FUNCTION_EX handler, LPVOID context )
{
    LPWSTR nameW;
    SERVICE_STATUS_HANDLE ret;

    nameW = SERV_dup(name);
    ret = RegisterServiceCtrlHandlerExW( nameW, handler, context );
    heap_free( nameW );
    return ret;
}

/******************************************************************************
 * RegisterServiceCtrlHandlerExW [ADVAPI32.@]
 */
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExW( LPCWSTR lpServiceName,
        LPHANDLER_FUNCTION_EX lpHandlerProc, LPVOID lpContext )
{
    service_data *service;
    SC_HANDLE hService = 0;

    TRACE("%s %p %p\n", debugstr_w(lpServiceName), lpHandlerProc, lpContext);

    EnterCriticalSection( &service_cs );
    if ((service = find_service_by_name( lpServiceName )))
    {
        service->handler = lpHandlerProc;
        service->context = lpContext;
        hService = service->handle;
    }
    LeaveCriticalSection( &service_cs );

    if (!hService) SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
    return (SERVICE_STATUS_HANDLE)hService;
}

/******************************************************************************
 * EnumDependentServicesA [ADVAPI32.@]
 */
BOOL WINAPI EnumDependentServicesA( SC_HANDLE hService, DWORD dwServiceState,
                                    LPENUM_SERVICE_STATUSA lpServices, DWORD cbBufSize,
        LPDWORD pcbBytesNeeded, LPDWORD lpServicesReturned )
{
    FIXME("%p 0x%08x %p 0x%08x %p %p - stub\n", hService, dwServiceState,
          lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned);

    *lpServicesReturned = 0;
    return TRUE;
}

/******************************************************************************
 * EnumDependentServicesW [ADVAPI32.@]
 */
BOOL WINAPI EnumDependentServicesW( SC_HANDLE hService, DWORD dwServiceState,
                                    LPENUM_SERVICE_STATUSW lpServices, DWORD cbBufSize,
                                    LPDWORD pcbBytesNeeded, LPDWORD lpServicesReturned )
{
    FIXME("%p 0x%08x %p 0x%08x %p %p - stub\n", hService, dwServiceState,
          lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned);

    *lpServicesReturned = 0;
    return TRUE;
}

static DWORD WINAPI notify_thread(void *user)
{
    DWORD err;
    notify_data *data = user;
    SC_RPC_NOTIFY_PARAMS_LIST *list = NULL;
    SERVICE_NOTIFY_STATUS_CHANGE_PARAMS_2 *cparams;
    BOOL dummy;

    __TRY
    {
        /* GetNotifyResults blocks until there is an event */
        err = svcctl_GetNotifyResults(data->notify_handle, &list);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    EnterCriticalSection( &service_cs );

    list_remove(&data->entry);

    LeaveCriticalSection( &service_cs );

    if (err == ERROR_SUCCESS && list)
    {
        cparams = list->NotifyParamsArray[0].u.params;

        data->notify_buffer->dwNotificationStatus = cparams->dwNotificationStatus;
        memcpy(&data->notify_buffer->ServiceStatus, &cparams->ServiceStatus,
                sizeof(SERVICE_STATUS_PROCESS));
        data->notify_buffer->dwNotificationTriggered = cparams->dwNotificationTriggered;
        data->notify_buffer->pszServiceNames = NULL;

        QueueUserAPC((PAPCFUNC)data->notify_buffer->pfnNotifyCallback,
                data->calling_thread, (ULONG_PTR)data->notify_buffer);

        HeapFree(GetProcessHeap(), 0, list);
    }
    else
        WARN("GetNotifyResults server call failed: %u\n", err);


    __TRY
    {
        err = svcctl_CloseNotifyHandle(&data->notify_handle, &dummy);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
        WARN("CloseNotifyHandle server call failed: %u\n", err);

    CloseHandle(data->calling_thread);
    HeapFree(GetProcessHeap(), 0, data);

    return 0;
}

/******************************************************************************
 * NotifyServiceStatusChangeW [ADVAPI32.@]
 */
DWORD WINAPI NotifyServiceStatusChangeW(SC_HANDLE hService, DWORD dwNotifyMask,
        SERVICE_NOTIFYW *pNotifyBuffer)
{
    DWORD err;
    BOOL b_dummy = FALSE;
    GUID g_dummy = {0};
    notify_data *data;

    TRACE("%p 0x%x %p\n", hService, dwNotifyMask, pNotifyBuffer);

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
    if (!data)
        return ERROR_NOT_ENOUGH_MEMORY;

    data->service = hService;
    data->notify_buffer = pNotifyBuffer;
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
            GetCurrentProcess(), &data->calling_thread, 0, FALSE,
            DUPLICATE_SAME_ACCESS))
    {
        ERR("DuplicateHandle failed: %u\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, data);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    data->params.dwInfoLevel = 2;
    data->params.u.params = &data->cparams;

    data->cparams.dwNotifyMask = dwNotifyMask;

    EnterCriticalSection( &service_cs );

    __TRY
    {
        err = svcctl_NotifyServiceStatusChange(hService, data->params,
                &g_dummy, &g_dummy, &b_dummy, &data->notify_handle);
    }
    __EXCEPT(rpc_filter)
    {
        err = map_exception_code(GetExceptionCode());
    }
    __ENDTRY

    if (err != ERROR_SUCCESS)
    {
        WARN("NotifyServiceStatusChange server call failed: %u\n", err);
        LeaveCriticalSection( &service_cs );
        CloseHandle(data->calling_thread);
        CloseHandle(data->ready_evt);
        HeapFree(GetProcessHeap(), 0, data);
        return err;
    }

    CloseHandle(CreateThread(NULL, 0, &notify_thread, data, 0, NULL));

    list_add_tail(&notify_list, &data->entry);

    LeaveCriticalSection( &service_cs );

    return ERROR_SUCCESS;
}
