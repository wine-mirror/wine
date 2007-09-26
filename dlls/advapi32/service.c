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

#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <assert.h>

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
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static const WCHAR szLocalSystem[] = {'L','o','c','a','l','S','y','s','t','e','m',0};
static const WCHAR szServiceManagerKey[] = { 'S','y','s','t','e','m','\\',
      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
      'S','e','r','v','i','c','e','s',0 };
static const WCHAR  szSCMLock[] = {'A','D','V','A','P','I','_','S','C','M',
                                   'L','O','C','K',0};

static const GENERIC_MAPPING scm_generic = {
    (STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS),
    (STANDARD_RIGHTS_WRITE | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_MODIFY_BOOT_CONFIG),
    (STANDARD_RIGHTS_EXECUTE | SC_MANAGER_CONNECT | SC_MANAGER_LOCK),
    SC_MANAGER_ALL_ACCESS
};

static const GENERIC_MAPPING svc_generic = {
    (STANDARD_RIGHTS_READ | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS),
    (STANDARD_RIGHTS_WRITE | SERVICE_CHANGE_CONFIG),
    (STANDARD_RIGHTS_EXECUTE | SERVICE_START | SERVICE_STOP | SERVICE_PAUSE_CONTINUE | SERVICE_USER_DEFINED_CONTROL),
    SERVICE_ALL_ACCESS
};

typedef struct service_start_info_t
{
    DWORD cmd;
    DWORD size;
    WCHAR str[1];
} service_start_info;

#define WINESERV_STARTINFO   1
#define WINESERV_GETSTATUS   2
#define WINESERV_SENDCONTROL 3

typedef struct service_data_t
{
    struct list entry;
    union {
        LPHANDLER_FUNCTION handler;
        LPHANDLER_FUNCTION_EX handler_ex;
    } handler;
    LPVOID context;
    SERVICE_STATUS_PROCESS status;
    HANDLE thread;
    BOOL unicode : 1;
    BOOL extended : 1; /* uses handler_ex instead of handler? */
    union {
        LPSERVICE_MAIN_FUNCTIONA a;
        LPSERVICE_MAIN_FUNCTIONW w;
    } proc;
    LPWSTR args;
    WCHAR name[1];
} service_data;

static CRITICAL_SECTION service_cs;
static CRITICAL_SECTION_DEBUG service_cs_debug =
{
    0, 0, &service_cs,
    { &service_cs_debug.ProcessLocksList, 
      &service_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": service_cs") }
};
static CRITICAL_SECTION service_cs = { &service_cs_debug, -1, 0, 0, 0, 0 };

static struct list service_list = LIST_INIT(service_list);

extern HANDLE __wine_make_process_system(void);

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
    DWORD  dwAccess;
};

struct sc_service       /* service handle */
{
    struct sc_handle hdr;
    HKEY   hkey;          /* handle to service entry in the registry (under hkey) */
    DWORD  dwAccess;
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
 * String management functions (same behaviour as strdup)
 * NOTE: the caller of those functions is responsible for calling HeapFree
 * in order to release the memory allocated by those functions.
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

    wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, n, wstr, len );
    return wstr;
}

/******************************************************************************
 * registry access functions and data
 */
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
static const WCHAR szDependOnService[] = {
       'D','e','p','e','n','d','O','n','S','e','r','v','i','c','e',0};
static const WCHAR szObjectName[] = {
       'O','b','j','e','c','t','N','a','m','e',0};
static const WCHAR szTag[] = {
       'T','a','g',0};

struct reg_value {
    DWORD type;
    DWORD size;
    LPCWSTR name;
    LPCVOID data;
};

static inline void service_set_value( struct reg_value *val,
                        DWORD type, LPCWSTR name, LPCVOID data, DWORD size )
{
    val->name = name;
    val->type = type;
    val->data = data;
    val->size = size;
}

static inline void service_set_dword( struct reg_value *val, 
                                      LPCWSTR name, const DWORD *data )
{
    service_set_value( val, REG_DWORD, name, data, sizeof (DWORD));
}

static inline void service_set_string( struct reg_value *val,
                                       LPCWSTR name, LPCWSTR string )
{
    DWORD len = (lstrlenW(string)+1) * sizeof (WCHAR);
    service_set_value( val, REG_SZ, name, string, len );
}

static inline void service_set_multi_string( struct reg_value *val,
                                             LPCWSTR name, LPCWSTR string )
{
    DWORD len = 0;

    /* determine the length of a double null terminated multi string */
    do {
        len += (lstrlenW( &string[ len ] )+1);
    } while ( string[ len++ ] );

    len *= sizeof (WCHAR);
    service_set_value( val, REG_MULTI_SZ, name, string, len );
}

static inline LONG service_write_values( HKEY hKey,
                                         const struct reg_value *val, int n )
{
    LONG r = ERROR_SUCCESS;
    int i;

    for( i=0; i<n; i++ )
    {
        r = RegSetValueExW(hKey, val[i].name, 0, val[i].type, 
                       (const BYTE*)val[i].data, val[i].size );
        if( r != ERROR_SUCCESS )
            break;
    }
    return r;
}

/******************************************************************************
 * Service IPC functions
 */
static LPWSTR service_get_pipe_name(LPCWSTR service)
{
    static const WCHAR prefix[] = { '\\','\\','.','\\','p','i','p','e','\\',
                   '_','_','w','i','n','e','s','e','r','v','i','c','e','_',0};
    LPWSTR name;
    DWORD len;

    len = sizeof prefix + strlenW(service)*sizeof(WCHAR);
    name = HeapAlloc(GetProcessHeap(), 0, len);
    strcpyW(name, prefix);
    strcatW(name, service);
    return name;
}

static HANDLE service_open_pipe(LPCWSTR service)
{
    LPWSTR szPipe = service_get_pipe_name( service );
    HANDLE handle = INVALID_HANDLE_VALUE;

    do {
        handle = CreateFileW(szPipe, GENERIC_READ|GENERIC_WRITE,
                         0, NULL, OPEN_ALWAYS, 0, NULL);
        if (handle != INVALID_HANDLE_VALUE)
            break;
        if (GetLastError() != ERROR_PIPE_BUSY)
            break;
    } while (WaitNamedPipeW(szPipe, NMPWAIT_WAIT_FOREVER));
    HeapFree(GetProcessHeap(), 0, szPipe);

    return handle;
}

/******************************************************************************
 * service_get_event_handle
 */
static HANDLE service_get_event_handle(LPCWSTR service)
{
    static const WCHAR prefix[] = { 
           '_','_','w','i','n','e','s','e','r','v','i','c','e','_',0};
    LPWSTR name;
    DWORD len;
    HANDLE handle;

    len = sizeof prefix + strlenW(service)*sizeof(WCHAR);
    name = HeapAlloc(GetProcessHeap(), 0, len);
    strcpyW(name, prefix);
    strcatW(name, service);
    handle = CreateEventW(NULL, TRUE, FALSE, name);
    HeapFree(GetProcessHeap(), 0, name);
    return handle;
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

    if (!argc)
    {
        if (info->unicode)
            info->proc.w(0, NULL);
        else
            info->proc.a(0, NULL);
        return 0;
    }
    
    if (info->unicode)
    {
        LPWSTR *argv, p;

        argv = HeapAlloc(GetProcessHeap(), 0, (argc+1)*sizeof(LPWSTR));
        for (argc=0, p=str; *p; p += strlenW(p) + 1)
            argv[argc++] = p;
        argv[argc] = NULL;

        info->proc.w(argc, argv);
        HeapFree(GetProcessHeap(), 0, argv);
    }
    else
    {
        LPSTR strA, *argv, p;
        DWORD lenA;
        
        lenA = WideCharToMultiByte(CP_ACP,0, str, len, NULL, 0, NULL, NULL);
        strA = HeapAlloc(GetProcessHeap(), 0, lenA);
        WideCharToMultiByte(CP_ACP,0, str, len, strA, lenA, NULL, NULL);

        argv = HeapAlloc(GetProcessHeap(), 0, (argc+1)*sizeof(LPSTR));
        for (argc=0, p=strA; *p; p += strlen(p) + 1)
            argv[argc++] = p;
        argv[argc] = NULL;

        info->proc.a(argc, argv);
        HeapFree(GetProcessHeap(), 0, argv);
        HeapFree(GetProcessHeap(), 0, strA);
    }
    return 0;
}

/******************************************************************************
 * service_handle_start
 */
static BOOL service_handle_start(HANDLE pipe, service_data *service, DWORD count)
{
    DWORD read = 0, result = 0;
    LPWSTR args;
    BOOL r;

    TRACE("%p %p %d\n", pipe, service, count);

    args = HeapAlloc(GetProcessHeap(), 0, count*sizeof(WCHAR));
    r = ReadFile(pipe, args, count*sizeof(WCHAR), &read, NULL);
    if (!r || count!=read/sizeof(WCHAR) || args[count-1])
    {
        ERR("pipe read failed r = %d count = %d read = %d args[n-1]=%s\n",
            r, count, read, debugstr_wn(args, count));
        goto end;
    }

    if (service->thread)
    {
        WARN("service is not stopped\n");
        result = ERROR_SERVICE_ALREADY_RUNNING;
        goto end;
    }

    HeapFree(GetProcessHeap(), 0, service->args);
    service->args = args;
    args = NULL;
    service->thread = CreateThread( NULL, 0, service_thread,
                                    service, 0, NULL );

end:
    HeapFree(GetProcessHeap(), 0, args);
    WriteFile( pipe, &result, sizeof result, &read, NULL );

    return TRUE;
}

/******************************************************************************
 * service_send_start_message
 */
static BOOL service_send_start_message(HANDLE pipe, LPCWSTR *argv, DWORD argc)
{
    DWORD i, len, count, result;
    service_start_info *ssi;
    LPWSTR p;
    BOOL r;

    TRACE("%p %p %d\n", pipe, argv, argc);

    /* calculate how much space do we need to send the startup info */
    len = 1;
    for (i=0; i<argc; i++)
        len += strlenW(argv[i])+1;

    ssi = HeapAlloc(GetProcessHeap(),0,sizeof *ssi + (len-1)*sizeof(WCHAR));
    ssi->cmd = WINESERV_STARTINFO;
    ssi->size = len;

    /* copy service args into a single buffer*/
    p = &ssi->str[0];
    for (i=0; i<argc; i++)
    {
        strcpyW(p, argv[i]);
        p += strlenW(p) + 1;
    }
    *p=0;

    r = WriteFile(pipe, ssi, sizeof *ssi + len*sizeof(WCHAR), &count, NULL);
    if (r)
    {
        r = ReadFile(pipe, &result, sizeof result, &count, NULL);
        if (r && result)
        {
            SetLastError(result);
            r = FALSE;
        }
    }

    HeapFree(GetProcessHeap(),0,ssi);

    return r;
}

/******************************************************************************
 * service_handle_get_status
 */
static BOOL service_handle_get_status(HANDLE pipe, const service_data *service)
{
    DWORD count = 0;
    TRACE("\n");
    return WriteFile(pipe, &service->status, 
                     sizeof service->status, &count, NULL);
}

/******************************************************************************
 * service_get_status
 */
static BOOL service_get_status(HANDLE pipe, LPSERVICE_STATUS_PROCESS status)
{
    DWORD cmd[2], count = 0;
    BOOL r;
    
    cmd[0] = WINESERV_GETSTATUS;
    cmd[1] = 0;
    r = WriteFile( pipe, cmd, sizeof cmd, &count, NULL );
    if (!r || count != sizeof cmd)
    {
        ERR("service protocol error - failed to write pipe!\n");
        return r;
    }
    r = ReadFile( pipe, status, sizeof *status, &count, NULL );
    if (!r || count != sizeof *status)
        ERR("service protocol error - failed to read pipe "
            "r = %d  count = %d!\n", r, count);
    return r;
}

/******************************************************************************
 * service_send_control
 */
static BOOL service_send_control(HANDLE pipe, DWORD dwControl, DWORD *result)
{
    DWORD cmd[2], count = 0;
    BOOL r;
    
    cmd[0] = WINESERV_SENDCONTROL;
    cmd[1] = dwControl;
    r = WriteFile(pipe, cmd, sizeof cmd, &count, NULL);
    if (!r || count != sizeof cmd)
    {
        ERR("service protocol error - failed to write pipe!\n");
        return r;
    }
    r = ReadFile(pipe, result, sizeof *result, &count, NULL);
    if (!r || count != sizeof *result)
        ERR("service protocol error - failed to read pipe "
            "r = %d  count = %d!\n", r, count);
    return r;
}

/******************************************************************************
 * service_accepts_control
 */
static BOOL service_accepts_control(const service_data *service, DWORD dwControl)
{
    DWORD a = service->status.dwControlsAccepted;

    switch (dwControl)
    {
    case SERVICE_CONTROL_INTERROGATE:
        return TRUE;
    case SERVICE_CONTROL_STOP:
        if (a&SERVICE_ACCEPT_STOP)
            return TRUE;
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        if (a&SERVICE_ACCEPT_SHUTDOWN)
            return TRUE;
        break;
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
        if (a&SERVICE_ACCEPT_PAUSE_CONTINUE)
            return TRUE;
        break;
    case SERVICE_CONTROL_PARAMCHANGE:
        if (a&SERVICE_ACCEPT_PARAMCHANGE)
            return TRUE;
        break;
    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:
        if (a&SERVICE_ACCEPT_NETBINDCHANGE)
            return TRUE;
    }
    if (!service->extended)
        return FALSE;
    switch (dwControl)
    {
    case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        if (a&SERVICE_ACCEPT_HARDWAREPROFILECHANGE)
            return TRUE;
        break;
    case SERVICE_CONTROL_POWEREVENT:
        if (a&SERVICE_ACCEPT_POWEREVENT)
            return TRUE;
        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
        if (a&SERVICE_ACCEPT_SESSIONCHANGE)
            return TRUE;
        break;
    }
    return FALSE;
}

/******************************************************************************
 * service_handle_control
 */
static BOOL service_handle_control(HANDLE pipe, service_data *service,
                                   DWORD dwControl)
{
    DWORD count, ret = ERROR_INVALID_SERVICE_CONTROL;

    TRACE("received control %d\n", dwControl);
    
    if (service_accepts_control(service, dwControl))
    {
        if (service->extended && service->handler.handler_ex)
        {
            service->handler.handler_ex(dwControl, 0, NULL, service->context);
            ret = ERROR_SUCCESS;
        }
        else if (service->handler.handler)
        {
            service->handler.handler(dwControl);
            ret = ERROR_SUCCESS;
        }
    }
    return WriteFile(pipe, &ret, sizeof ret, &count, NULL);
}

/******************************************************************************
 * service_reap_thread
 */
static DWORD service_reap_thread(service_data *service)
{
    DWORD exitcode = 0;

    if (!service->thread)
        return 0;
    GetExitCodeThread(service->thread, &exitcode);
    if (exitcode!=STILL_ACTIVE)
    {
        CloseHandle(service->thread);
        service->thread = 0;
    }
    return exitcode;
}

/******************************************************************************
 * service_control_dispatcher
 */
static DWORD WINAPI service_control_dispatcher(LPVOID arg)
{
    service_data *service = arg;
    LPWSTR name;
    HANDLE pipe, event;

    TRACE("%p %s\n", service, debugstr_w(service->name));

    /* create a pipe to talk to the rest of the world with */
    name = service_get_pipe_name(service->name);
    pipe = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX,
                  PIPE_TYPE_BYTE|PIPE_WAIT, 1, 256, 256, 10000, NULL );
    HeapFree(GetProcessHeap(), 0, name);

    /* let the process who started us know we've tried to create a pipe */
    event = service_get_event_handle(service->name);
    SetEvent(event);
    CloseHandle(event);

    if (pipe==INVALID_HANDLE_VALUE)
    {
        ERR("failed to create pipe for %s, error = %d\n",
            debugstr_w(service->name), GetLastError());
        return 0;
    }

    /* dispatcher loop */
    while (1)
    {
        BOOL r;
        DWORD count, req[2] = {0,0};

        r = ConnectNamedPipe(pipe, NULL);
        if (!r && GetLastError() != ERROR_PIPE_CONNECTED)
        {
            ERR("pipe connect failed\n");
            break;
        }

        r = ReadFile( pipe, &req, sizeof req, &count, NULL );
        if (!r || count!=sizeof req)
        {
            ERR("pipe read failed\n");
            break;
        }

        service_reap_thread(service);

        /* handle the request */
        switch (req[0])
        {
        case WINESERV_STARTINFO:
            service_handle_start(pipe, service, req[1]);
            break;
        case WINESERV_GETSTATUS:
            service_handle_get_status(pipe, service);
            break;
        case WINESERV_SENDCONTROL:
            service_handle_control(pipe, service, req[1]);
            break;
        default:
            ERR("received invalid command %d length %d\n", req[0], req[1]);
        }

        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
    }

    CloseHandle(pipe);
    return 1;
}

/******************************************************************************
 * service_run_threads
 */
static BOOL service_run_threads(void)
{
    service_data *service;
    DWORD count, n = 0;
    HANDLE *handles;

    EnterCriticalSection( &service_cs );

    count = list_count( &service_list );

    TRACE("Starting %d pipe listener threads. Services running as process %d\n", count, GetCurrentProcessId());

    handles = HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE) * (count + 1));

    handles[n++] = __wine_make_process_system();

    LIST_FOR_EACH_ENTRY( service, &service_list, service_data, entry )
    {
        service->status.dwProcessId = GetCurrentProcessId();
        handles[n++] = CreateThread( NULL, 0, service_control_dispatcher,
                                     service, 0, NULL );
    }
    assert(n == count + 1);

    LeaveCriticalSection( &service_cs );

    /* wait for all the threads to pack up and exit */
    while (n > 1)
    {
        DWORD ret = WaitForMultipleObjects( min(n,MAXIMUM_WAIT_OBJECTS), handles, FALSE, INFINITE );
        if (!ret)  /* system process event */
        {
            TRACE( "last user process exited, shutting down\n" );
            /* FIXME: we should maybe send a shutdown control to running services */
            ExitProcess(0);
        }
        if (ret < MAXIMUM_WAIT_OBJECTS)
        {
            CloseHandle( handles[ret] );
            memmove( &handles[ret], &handles[ret+1], (n - ret - 1) * sizeof(HANDLE) );
            n--;
        }
        else break;
    }

    while (n) CloseHandle( handles[--n] );
    HeapFree(GetProcessHeap(), 0, handles);

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
    DWORD sz, len;
    BOOL ret = TRUE;

    TRACE("%p\n", servent);

    EnterCriticalSection( &service_cs );
    while (servent->lpServiceName)
    {
        LPSTR name = servent->lpServiceName;

        len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
        sz = len*sizeof(WCHAR) + sizeof *info;
        info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz );
        MultiByteToWideChar(CP_ACP, 0, name, -1, info->name, len);
        info->proc.a = servent->lpServiceProc;
        info->unicode = FALSE;
        list_add_head( &service_list, &info->entry );
        servent++;
    }
    LeaveCriticalSection( &service_cs );

    service_run_threads();

    return ret;
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
    DWORD sz, len;
    BOOL ret = TRUE;

    TRACE("%p\n", servent);

    EnterCriticalSection( &service_cs );
    while (servent->lpServiceName)
    {
        LPWSTR name = servent->lpServiceName;

        len = strlenW(name);
        sz = len*sizeof(WCHAR) + sizeof *info;
        info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz );
        strcpyW(info->name, name);
        info->proc.w = servent->lpServiceProc;
        info->unicode = TRUE;
        list_add_head( &service_list, &info->entry );
        servent++;
    }
    LeaveCriticalSection( &service_cs );

    service_run_threads();

    return ret;
}

/******************************************************************************
 * LockServiceDatabase  [ADVAPI32.@]
 */
SC_LOCK WINAPI LockServiceDatabase (SC_HANDLE hSCManager)
{
    HANDLE ret;

    TRACE("%p\n",hSCManager);

    ret = CreateSemaphoreW( NULL, 1, 1, szSCMLock );
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
BOOL WINAPI UnlockServiceDatabase (SC_LOCK ScLock)
{
    TRACE("%p\n",ScLock);

    return CloseHandle( ScLock );
}

/******************************************************************************
 * RegisterServiceCtrlHandlerA [ADVAPI32.@]
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerA( LPCSTR lpServiceName, LPHANDLER_FUNCTION lpfHandler )
{
    LPWSTR lpServiceNameW;
    SERVICE_STATUS_HANDLE ret;

    lpServiceNameW = SERV_dup(lpServiceName);
    ret = RegisterServiceCtrlHandlerW( lpServiceNameW, lpfHandler );
    HeapFree(GetProcessHeap(), 0, lpServiceNameW);
    return ret;
}

/******************************************************************************
 * RegisterServiceCtrlHandlerW [ADVAPI32.@]
 *
 * PARAMS
 *   lpServiceName []
 *   lpfHandler    []
 */
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW( LPCWSTR lpServiceName,
                             LPHANDLER_FUNCTION lpfHandler )
{
    service_data *service;
    SERVICE_STATUS_HANDLE handle = 0;

    EnterCriticalSection( &service_cs );
    LIST_FOR_EACH_ENTRY( service, &service_list, service_data, entry )
    {
        if(!strcmpW(lpServiceName, service->name))
        {
            service->handler.handler = lpfHandler;
            handle = (SERVICE_STATUS_HANDLE)service;
            break;
        }
    }
    LeaveCriticalSection( &service_cs );
    return handle;
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
    service_data *service;
    BOOL r = FALSE;

    TRACE("%p %x %x %x %x %x %x %x\n", hService,
          lpStatus->dwServiceType, lpStatus->dwCurrentState,
          lpStatus->dwControlsAccepted, lpStatus->dwWin32ExitCode,
          lpStatus->dwServiceSpecificExitCode, lpStatus->dwCheckPoint,
          lpStatus->dwWaitHint);

    EnterCriticalSection( &service_cs );
    LIST_FOR_EACH_ENTRY( service, &service_list, service_data, entry )
    {
        if(service == (service_data*)hService)
        {
            memcpy( &service->status, lpStatus, sizeof(SERVICE_STATUS) );
            TRACE("Set service status to %d\n",service->status.dwCurrentState);
            r = TRUE;
            break;
        }
    }
    LeaveCriticalSection( &service_cs );

    return r;
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
    HeapFree(GetProcessHeap(), 0, lpDatabaseNameW);
    HeapFree(GetProcessHeap(), 0, lpMachineNameW);
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
    DWORD new_mask = dwDesiredAccess;

    TRACE("(%s,%s,0x%08x)\n", debugstr_w(lpMachineName),
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

    r = RegCreateKeyW(hReg, szServiceManagerKey, &manager->hkey);
    RegCloseKey( hReg );
    if (r!=ERROR_SUCCESS)
        goto error;

    RtlMapGenericMask(&new_mask, &scm_generic);
    manager->dwAccess = new_mask;
    TRACE("returning %p (access : 0x%08x)\n", manager, manager->dwAccess);

    return (SC_HANDLE) &manager->hdr;

error:
    sc_handle_free( &manager->hdr );
    SetLastError( r);
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
    BOOL ret = FALSE;
    HANDLE handle;

    TRACE("%p %d %p\n", hService, dwControl, lpServiceStatus);

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ret = QueryServiceStatus(hService, lpServiceStatus);
    if (!ret)
    {
        ERR("failed to query service status\n");
        SetLastError(ERROR_SERVICE_NOT_ACTIVE);
        return FALSE;
    }

    switch (lpServiceStatus->dwCurrentState)
    {
    case SERVICE_STOPPED:
        SetLastError(ERROR_SERVICE_NOT_ACTIVE);
        return FALSE;
    case SERVICE_START_PENDING:
        if (dwControl==SERVICE_CONTROL_STOP)
            break;
        /* fall thru */
    case SERVICE_STOP_PENDING:
        SetLastError(ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
        return FALSE;
    }

    handle = service_open_pipe(hsvc->name);
    if (handle!=INVALID_HANDLE_VALUE)
    {
        DWORD result = ERROR_SUCCESS;
        ret = service_send_control(handle, dwControl, &result);
        CloseHandle(handle);
        if (result!=ERROR_SUCCESS)
        {
            SetLastError(result);
            ret = FALSE;
        }
    }

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

    TRACE("%p %s %d\n", hSCManager, debugstr_a(lpServiceName), dwDesiredAccess);

    lpServiceNameW = SERV_dup(lpServiceName);
    ret = OpenServiceW( hSCManager, lpServiceNameW, dwDesiredAccess);
    HeapFree(GetProcessHeap(), 0, lpServiceNameW);
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
    DWORD new_mask = dwDesiredAccess;

    TRACE("%p %s %d\n", hSCManager, debugstr_w(lpServiceName), dwDesiredAccess);

    hscm = sc_handle_get_handle_data( hSCManager, SC_HTYPE_MANAGER );
    if (!hscm)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!lpServiceName)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return NULL;
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
    {
        RegCloseKey(hKey);
        return NULL;
    }
    strcpyW( hsvc->name, lpServiceName );
    hsvc->hkey = hKey;

    RtlMapGenericMask(&new_mask, &svc_generic);
    hsvc->dwAccess = new_mask;

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
    struct reg_value val[10];
    int n = 0;
    DWORD new_mask = dwDesiredAccess;
    DWORD index = 0;
    WCHAR buffer[MAX_PATH];
    BOOL displayname_exists = FALSE;

    TRACE("%p %s %s\n", hSCManager, 
          debugstr_w(lpServiceName), debugstr_w(lpDisplayName));

    hscm = sc_handle_get_handle_data( hSCManager, SC_HTYPE_MANAGER );
    if (!hscm)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }

    if (!lpServiceName || !lpBinaryPathName)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return NULL;
    }

    if (!(hscm->dwAccess & SC_MANAGER_CREATE_SERVICE))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }

    if (!lpServiceName[0])
    {
        SetLastError(ERROR_INVALID_NAME);
        return NULL;
    }

    if (!lpBinaryPathName[0])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* ServiceType can only be one value (except for SERVICE_INTERACTIVE_PROCESS which can be used
     * together with SERVICE_WIN32_OWN_PROCESS or SERVICE_WIN32_SHARE_PROCESS when the service
     * runs under the LocalSystem account)
     */
    switch (dwServiceType)
    {
    case SERVICE_KERNEL_DRIVER:
    case SERVICE_FILE_SYSTEM_DRIVER:
    case SERVICE_WIN32_OWN_PROCESS:
    case SERVICE_WIN32_SHARE_PROCESS:
        /* No problem */
        break;
    case SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS:
    case SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS:
        /* FIXME : Do we need a more thorough check? */
        if (lpServiceStartName)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!lpServiceStartName && (dwServiceType & SERVICE_WIN32))
            lpServiceStartName = szLocalSystem;

    /* StartType can only be a single value (if several values are mixed the result is probably not what was intended) */
    if (dwStartType > SERVICE_DISABLED)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* SERVICE_BOOT_START and SERVICE_SYSTEM_START or only allowed for driver services */
    if (((dwStartType == SERVICE_BOOT_START) || (dwStartType == SERVICE_SYSTEM_START)) &&
        ((dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (dwServiceType & SERVICE_WIN32_SHARE_PROCESS)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Loop through the registry to check if the service already exists and to
     * check if we can use the given displayname.
     * FIXME: Should we use EnumServicesStatusEx?
     */
    len = sizeof(buffer);
    while (RegEnumKeyExW(hscm->hkey, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        HKEY service_key;

        /* The service already exists, so bail out */
        if(!lstrcmpiW(lpServiceName, buffer))
        {
            SetLastError(ERROR_SERVICE_EXISTS);
            return NULL;
        }

        /* The given displayname matches the found servicename. We don't bail out
         * as servicename is checked before a duplicate displayname
         */
        if(!lstrcmpiW(lpDisplayName, buffer))
            displayname_exists = TRUE;

        if (RegOpenKeyExW(hscm->hkey, buffer, 0, KEY_READ, &service_key) == ERROR_SUCCESS)
        {
            WCHAR name[MAX_PATH];
            DWORD size = sizeof(name);

            if (RegQueryValueExW(service_key, szDisplayName, NULL, NULL, (LPBYTE)name, &size) == ERROR_SUCCESS)
            {
                /* The given displayname matches the found displayname */
                if (!lstrcmpiW(lpDisplayName, name))
                    displayname_exists = TRUE;
            }
            RegCloseKey(service_key);
        }
        index++;
        len = sizeof(buffer);
    }

    if (lpDisplayName && displayname_exists)
    {
        SetLastError(ERROR_DUPLICATE_SERVICE_NAME);
        return NULL;
    }

    r = RegCreateKeyExW(hscm->hkey, lpServiceName, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dp);
    if (r!=ERROR_SUCCESS)
    {
        /* FIXME: Should we set an error? */
        return NULL;
    }

    if( lpDisplayName )
        service_set_string( &val[n++], szDisplayName, lpDisplayName );

    service_set_dword( &val[n++], szType, &dwServiceType );
    service_set_dword( &val[n++], szStart, &dwStartType );
    service_set_dword( &val[n++], szError, &dwErrorControl );

    service_set_string( &val[n++], szImagePath, lpBinaryPathName );

    if( lpLoadOrderGroup )
        service_set_string( &val[n++], szGroup, lpLoadOrderGroup );

    /* FIXME: lpDependencies is used to create both DependOnService and DependOnGroup
     * There is no such key as what szDependencies refers to */
    if( lpDependencies )
        service_set_multi_string( &val[n++], szDependencies, lpDependencies );

    if( lpPassword )
        FIXME("Don't know how to add a Password for a service.\n");

    if( lpServiceStartName )
        service_set_string( &val[n++], szObjectName, lpServiceStartName );

    r = service_write_values( hKey, val, n );
    if( r != ERROR_SUCCESS )
        goto error;

    len = strlenW(lpServiceName)+1;
    len = sizeof (struct sc_service) + len*sizeof(WCHAR);
    hsvc = sc_handle_alloc( SC_HTYPE_SERVICE, len, sc_handle_destroy_service );
    if( !hsvc )
        goto error;
    lstrcpyW( hsvc->name, lpServiceName );
    hsvc->hkey = hKey;

    RtlMapGenericMask(&new_mask, &svc_generic);
    hsvc->dwAccess = new_mask;

    hsvc->scm = hscm;
    hscm->hdr.ref_count++;

    return (SC_HANDLE) &hsvc->hdr;
    
error:
    RegCloseKey( hKey );
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

    HeapFree( GetProcessHeap(), 0, lpServiceNameW );
    HeapFree( GetProcessHeap(), 0, lpDisplayNameW );
    HeapFree( GetProcessHeap(), 0, lpBinaryPathNameW );
    HeapFree( GetProcessHeap(), 0, lpLoadOrderGroupW );
    HeapFree( GetProcessHeap(), 0, lpDependenciesW );
    HeapFree( GetProcessHeap(), 0, lpServiceStartNameW );
    HeapFree( GetProcessHeap(), 0, lpPasswordW );

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

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!(hsvc->dwAccess & DELETE))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    /* Close the key to the service */
    RegCloseKey(hsvc->hkey);

    /* Delete the service under the Service Control Manager key */
    RegDeleteTreeW(hsvc->scm->hkey, hsvc->name);

    hsvc->hkey = NULL;

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
        lpwstr = HeapAlloc( GetProcessHeap(), 0,
                                   dwNumServiceArgs*sizeof(LPWSTR) );

    for(i=0; i<dwNumServiceArgs; i++)
        lpwstr[i]=SERV_dup(lpServiceArgVectors[i]);

    r = StartServiceW(hService, dwNumServiceArgs, (LPCWSTR *)lpwstr);

    if (dwNumServiceArgs)
    {
        for(i=0; i<dwNumServiceArgs; i++)
            HeapFree(GetProcessHeap(), 0, lpwstr[i]);
        HeapFree(GetProcessHeap(), 0, lpwstr);
    }

    return r;
}

/******************************************************************************
 * service_start_process    [INTERNAL]
 */
static DWORD service_start_process(struct sc_service *hsvc, LPDWORD ppid)
{
    static const WCHAR _ImagePathW[] = {'I','m','a','g','e','P','a','t','h',0};
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    LPWSTR path = NULL, str;
    DWORD type, size, ret, svc_type;
    HANDLE handles[2];
    BOOL r;

    size = sizeof(svc_type);
    if (RegQueryValueExW(hsvc->hkey, szType, NULL, &type, (LPBYTE)&svc_type, &size) || type != REG_DWORD)
        svc_type = 0;

    if (svc_type == SERVICE_KERNEL_DRIVER)
    {
        static const WCHAR winedeviceW[] = {'\\','w','i','n','e','d','e','v','i','c','e','.','e','x','e',' ',0};
        DWORD len = GetSystemDirectoryW( NULL, 0 ) + sizeof(winedeviceW)/sizeof(WCHAR) + strlenW(hsvc->name);

        if (!(path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
        GetSystemDirectoryW( path, len );
        lstrcatW( path, winedeviceW );
        lstrcatW( path, hsvc->name );
    }
    else
    {
        /* read the executable path from the registry */
        size = 0;
        ret = RegQueryValueExW(hsvc->hkey, _ImagePathW, NULL, &type, NULL, &size);
        if (ret!=ERROR_SUCCESS)
            return FALSE;
        str = HeapAlloc(GetProcessHeap(),0,size);
        ret = RegQueryValueExW(hsvc->hkey, _ImagePathW, NULL, &type, (LPBYTE)str, &size);
        if (ret==ERROR_SUCCESS)
        {
            size = ExpandEnvironmentStringsW(str,NULL,0);
            path = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
            ExpandEnvironmentStringsW(str,path,size);
        }
        HeapFree(GetProcessHeap(),0,str);
        if (!path)
            return FALSE;
    }

    /* wait for the process to start and set an event or terminate */
    handles[0] = service_get_event_handle( hsvc->name );
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    r = CreateProcessW(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (r)
    {
        if (ppid) *ppid = pi.dwProcessId;

        handles[1] = pi.hProcess;
        ret = WaitForMultipleObjectsEx(2, handles, FALSE, 30000, FALSE);
        if(ret != WAIT_OBJECT_0)
        {
            SetLastError(ERROR_IO_PENDING);
            r = FALSE;
        }

        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
    }
    CloseHandle( handles[0] );
    HeapFree(GetProcessHeap(),0,path);
    return r;
}

static BOOL service_wait_for_startup(SC_HANDLE hService)
{
    DWORD i;
    SERVICE_STATUS status;
    BOOL r = FALSE;

    TRACE("%p\n", hService);

    for (i=0; i<30; i++)
    {
        status.dwCurrentState = 0;
        r = QueryServiceStatus(hService, &status);
        if (!r)
            break;
        if (status.dwCurrentState == SERVICE_RUNNING)
        {
            TRACE("Service started successfully\n");
            break;
        }
        r = FALSE;
        Sleep(1000);
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
    struct sc_service *hsvc;
    BOOL r = FALSE;
    SC_LOCK hLock;
    HANDLE handle = INVALID_HANDLE_VALUE;

    TRACE("%p %d %p\n", hService, dwNumServiceArgs, lpServiceArgVectors);

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return r;
    }

    hLock = LockServiceDatabase((SC_HANDLE)hsvc->scm);
    if (!hLock)
        return r;

    handle = service_open_pipe(hsvc->name);
    if (handle==INVALID_HANDLE_VALUE)
    {
        /* start the service process */
        if (service_start_process(hsvc, NULL))
            handle = service_open_pipe(hsvc->name);
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
        r = service_send_start_message(handle, lpServiceArgVectors, dwNumServiceArgs);
        CloseHandle(handle);
    }

    UnlockServiceDatabase( hLock );

    TRACE("returning %d\n", r);

    if (r)
        service_wait_for_startup(hService);

    return r;
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

    TRACE("%p %p\n", hService, lpservicestatus);

    ret = QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&SvcStatusData,
                                sizeof(SERVICE_STATUS_PROCESS), NULL);
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
    struct sc_service *hsvc;
    DWORD size, type, val;
    HANDLE pipe;
    LONG r;
    LPSERVICE_STATUS_PROCESS pSvcStatusData;

    TRACE("%p %d %p %d %p\n", hService, InfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    if (InfoLevel != SC_STATUS_PROCESS_INFO)
    {
        SetLastError( ERROR_INVALID_LEVEL);
        return FALSE;
    }

    pSvcStatusData = (LPSERVICE_STATUS_PROCESS) lpBuffer;
    if (pSvcStatusData == NULL)
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS))
    {
        if( pcbBytesNeeded != NULL)
            *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);

        SetLastError( ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    pipe = service_open_pipe(hsvc->name);
    if (pipe != INVALID_HANDLE_VALUE)
    {
        r = service_get_status(pipe, pSvcStatusData);
        CloseHandle(pipe);
        if (r)
            return TRUE;
    }

    TRACE("Failed to read service status\n");

    /* read the service type from the registry */
    size = sizeof(val);
    r = RegQueryValueExA(hsvc->hkey, "Type", NULL, &type, (LPBYTE)&val, &size);
    if (r != ERROR_SUCCESS || type != REG_DWORD)
        val = 0;

    pSvcStatusData->dwServiceType = val;
    pSvcStatusData->dwCurrentState            = SERVICE_STOPPED;  /* stopped */
    pSvcStatusData->dwControlsAccepted        = 0;
    pSvcStatusData->dwWin32ExitCode           = ERROR_SERVICE_NEVER_STARTED;
    pSvcStatusData->dwServiceSpecificExitCode = 0;
    pSvcStatusData->dwCheckPoint              = 0;
    pSvcStatusData->dwWaitHint                = 0;

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

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, 2 * size )))
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
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/******************************************************************************
 * QueryServiceConfigW [ADVAPI32.@]
 */
BOOL WINAPI 
QueryServiceConfigW( SC_HANDLE hService,
                     LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    WCHAR str_buffer[ MAX_PATH ];
    LONG r;
    DWORD type, val, sz, total, n;
    LPBYTE p;
    HKEY hKey;
    struct sc_service *hsvc;

    TRACE("%p %p %d %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    hKey = hsvc->hkey;

    /* TODO: Check which members are mandatory and what the registry types
     * should be. This should of course also be tested when a service is
     * created.
     */

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
    else
	total += sizeof(WCHAR);

    sz = 0;
    r = RegQueryValueExW( hKey, szDependencies, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_MULTI_SZ ) )
        total += sz;
    else
	total += sizeof(WCHAR);

    sz = 0;
    r = RegQueryValueExW( hKey, szObjectName, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;
    else
	total += sizeof(WCHAR);

    sz = 0;
    r = RegQueryValueExW( hKey, szDisplayName, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;
    else
	total += sizeof(WCHAR);

    *pcbBytesNeeded = total;

    /* if there's not enough memory, return an error */
    if( total > cbBufSize )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    ZeroMemory( lpServiceConfig, total );

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szType, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_DWORD ) )
        lpServiceConfig->dwServiceType = val;

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szStart, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_DWORD ) )
        lpServiceConfig->dwStartType = val;

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szError, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_DWORD ) )
        lpServiceConfig->dwErrorControl = val;

    sz = sizeof val;
    r = RegQueryValueExW( hKey, szTag, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_DWORD ) )
        lpServiceConfig->dwTagId = val;

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
    lpServiceConfig->lpLoadOrderGroup = (LPWSTR) p;
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
    {
        p += sz;
        n -= sz;
    }
    else
    {
	*(WCHAR *) p = 0;
	p += sizeof(WCHAR);
	n -= sizeof(WCHAR);
    }

    sz = n;
    r = RegQueryValueExW( hKey, szDependencies, 0, &type, p, &sz );
    lpServiceConfig->lpDependencies = (LPWSTR) p;
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
    {
        p += sz;
        n -= sz;
    }
    else
    {
	*(WCHAR *) p = 0;
	p += sizeof(WCHAR);
	n -= sizeof(WCHAR);
    }

    sz = n;
    r = RegQueryValueExW( hKey, szObjectName, 0, &type, p, &sz );
    lpServiceConfig->lpServiceStartName = (LPWSTR) p;
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
    {
        p += sz;
        n -= sz;
    }
    else
    {
        *(WCHAR *) p = 0;
        p += sizeof(WCHAR);
        n -= sizeof(WCHAR);
    }

    sz = n;
    r = RegQueryValueExW( hKey, szDisplayName, 0, &type, p, &sz );
    lpServiceConfig->lpDisplayName = (LPWSTR) p;
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
    {
        p += sz;
        n -= sz;
    }
    else
    {
        *(WCHAR *) p = 0;
        p += sizeof(WCHAR);
        n -= sizeof(WCHAR);
    }

    if( n < 0 )
        ERR("Buffer overflow!\n");

    TRACE("Image path           = %s\n", debugstr_w(lpServiceConfig->lpBinaryPathName) );
    TRACE("Group                = %s\n", debugstr_w(lpServiceConfig->lpLoadOrderGroup) );
    TRACE("Dependencies         = %s\n", debugstr_w(lpServiceConfig->lpDependencies) );
    TRACE("Service account name = %s\n", debugstr_w(lpServiceConfig->lpServiceStartName) );
    TRACE("Display name         = %s\n", debugstr_w(lpServiceConfig->lpDisplayName) );

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
    FIXME("%p type=%x state=%x %p %x %p %p %p\n", hSCManager,
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
    FIXME("%p type=%x state=%x %p %x %p %p %p\n", hSCManager,
          dwServiceType, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned,  lpResumeHandle);
    SetLastError (ERROR_ACCESS_DENIED);
    return FALSE;
}

/******************************************************************************
 * EnumServicesStatusExA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusExA(SC_HANDLE hSCManager, SC_ENUM_TYPE InfoLevel, DWORD dwServiceType,
                      DWORD dwServiceState, LPBYTE lpServices, DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                      LPDWORD lpServicesReturned, LPDWORD lpResumeHandle, LPCSTR pszGroupName)
{
    FIXME("%p level=%d type=%x state=%x %p %x %p %p %p %s\n", hSCManager, InfoLevel,
          dwServiceType, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned,  lpResumeHandle, debugstr_a(pszGroupName));
    SetLastError (ERROR_ACCESS_DENIED);
    return FALSE;
}

/******************************************************************************
 * EnumServicesStatusExW [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusExW(SC_HANDLE hSCManager, SC_ENUM_TYPE InfoLevel, DWORD dwServiceType,
                      DWORD dwServiceState, LPBYTE lpServices, DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                      LPDWORD lpServicesReturned, LPDWORD lpResumeHandle, LPCWSTR pszGroupName)
{
    FIXME("%p level=%d type=%x state=%x %p %x %p %p %p %s\n", hSCManager, InfoLevel,
          dwServiceType, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned,  lpResumeHandle, debugstr_w(pszGroupName));
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
    LPWSTR lpServiceNameW, lpDisplayNameW = NULL;
    DWORD size, sizeW, GLE;
    BOOL ret;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_a(lpServiceName), lpDisplayName, lpcchBuffer);

    lpServiceNameW = SERV_dup(lpServiceName);
    lpDisplayNameW = HeapAlloc(GetProcessHeap(), 0, *lpcchBuffer * sizeof(WCHAR));

    size = sizeW = *lpcchBuffer;
    ret = GetServiceDisplayNameW(hSCManager, lpServiceNameW,
                                 lpDisplayName ? lpDisplayNameW : NULL,
                                 &sizeW);
    /* Last error will be set by GetServiceDisplayNameW and must be preserved */
    GLE = GetLastError();

    if (!lpDisplayName && *lpcchBuffer && !ret && (GLE == ERROR_INSUFFICIENT_BUFFER))
    {
        /* Request for buffersize.
         *
         * Only set the size for ERROR_INSUFFICIENT_BUFFER
         */
        size = sizeW * 2;
    }
    else if (lpDisplayName && *lpcchBuffer && !ret)
    {
        /* Request for displayname.
         *
         * size only has to be set if this fails
         */
        size = sizeW * 2;
    }

    WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, (sizeW + 1), lpDisplayName,
                        *lpcchBuffer, NULL, NULL );

    *lpcchBuffer = size;

    HeapFree(GetProcessHeap(), 0, lpDisplayNameW);
    HeapFree(GetProcessHeap(), 0, lpServiceNameW);

    SetLastError(GLE);
    return ret;
}

/******************************************************************************
 * GetServiceDisplayNameW  [ADVAPI32.@]
 */
BOOL WINAPI GetServiceDisplayNameW( SC_HANDLE hSCManager, LPCWSTR lpServiceName,
  LPWSTR lpDisplayName, LPDWORD lpcchBuffer)
{
    struct sc_manager *hscm;
    DWORD type, size;
    LONG ret;

    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_w(lpServiceName), lpDisplayName, lpcchBuffer);

    hscm = sc_handle_get_handle_data(hSCManager, SC_HTYPE_MANAGER);
    if (!hscm)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpServiceName)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    size = *lpcchBuffer * sizeof(WCHAR);
    ret = RegGetValueW(hscm->hkey, lpServiceName, szDisplayName, RRF_RT_REG_SZ, &type, lpDisplayName, &size);
    if (!ret && !lpDisplayName && size)
        ret = ERROR_MORE_DATA;

    if (ret)
    {
        if (lpDisplayName && *lpcchBuffer) *lpDisplayName = 0;

        if (ret == ERROR_MORE_DATA)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            *lpcchBuffer = (size / sizeof(WCHAR)) - 1;
        }
        else if (ret == ERROR_FILE_NOT_FOUND)
        {
            HKEY hkey;

            if (!RegOpenKeyW(hscm->hkey, lpServiceName, &hkey))
            {
                UINT len = lstrlenW(lpServiceName);
                BOOL r = FALSE;

                if ((*lpcchBuffer <= len) || (!lpDisplayName && *lpcchBuffer))
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                else if (lpDisplayName && *lpcchBuffer)
                {
                    /* No displayname, but the service exists and the buffer
                     * is big enough. We should return the servicename.
                     */
                    lstrcpyW(lpDisplayName, lpServiceName);
                    r = TRUE;
                }

                *lpcchBuffer = len;
                RegCloseKey(hkey);
                return r;
            }
            else
                SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);
        }
        else
            SetLastError(ret);
        return FALSE;
    }

    /* Always return the correct needed size on success */
    *lpcchBuffer = (size / sizeof(WCHAR)) - 1;

    return TRUE;
}

/******************************************************************************
 * ChangeServiceConfigW  [ADVAPI32.@]
 */
BOOL WINAPI ChangeServiceConfigW( SC_HANDLE hService, DWORD dwServiceType,
  DWORD dwStartType, DWORD dwErrorControl, LPCWSTR lpBinaryPathName,
  LPCWSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCWSTR lpDependencies,
  LPCWSTR lpServiceStartName, LPCWSTR lpPassword, LPCWSTR lpDisplayName)
{
    struct reg_value val[10];
    struct sc_service *hsvc;
    DWORD r = ERROR_SUCCESS;
    HKEY hKey;
    int n = 0;

    TRACE("%p %d %d %d %s %s %p %p %s %s %s\n",
          hService, dwServiceType, dwStartType, dwErrorControl, 
          debugstr_w(lpBinaryPathName), debugstr_w(lpLoadOrderGroup),
          lpdwTagId, lpDependencies, debugstr_w(lpServiceStartName),
          debugstr_w(lpPassword), debugstr_w(lpDisplayName) );

    hsvc = sc_handle_get_handle_data(hService, SC_HTYPE_SERVICE);
    if (!hsvc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    hKey = hsvc->hkey;

    if( dwServiceType != SERVICE_NO_CHANGE )
        service_set_dword( &val[n++], szType, &dwServiceType );

    if( dwStartType != SERVICE_NO_CHANGE )
        service_set_dword( &val[n++], szStart, &dwStartType );

    if( dwErrorControl != SERVICE_NO_CHANGE )
        service_set_dword( &val[n++], szError, &dwErrorControl );

    if( lpBinaryPathName )
        service_set_string( &val[n++], szImagePath, lpBinaryPathName );

    if( lpLoadOrderGroup )
        service_set_string( &val[n++], szGroup, lpLoadOrderGroup );

    /* FIXME: lpDependencies is used to create/change both DependOnService and DependOnGroup
     * There is no such key as what szDependencies refers to */
    if( lpDependencies )
        service_set_multi_string( &val[n++], szDependencies, lpDependencies );

    if( lpPassword )
        FIXME("ignoring password\n");

    if( lpServiceStartName )
        service_set_string( &val[n++], szObjectName, lpServiceStartName );

    r = service_write_values( hsvc->hkey, val, n );

    return (r == ERROR_SUCCESS) ? TRUE : FALSE ;
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

    HeapFree( GetProcessHeap(), 0, wBinaryPathName );
    HeapFree( GetProcessHeap(), 0, wLoadOrderGroup );
    HeapFree( GetProcessHeap(), 0, wDependencies );
    HeapFree( GetProcessHeap(), 0, wServiceStartName );
    HeapFree( GetProcessHeap(), 0, wPassword );
    HeapFree( GetProcessHeap(), 0, wDisplayName );

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
        LPSERVICE_DESCRIPTIONA sd = (LPSERVICE_DESCRIPTIONA) lpInfo;
        SERVICE_DESCRIPTIONW sdw;

        sdw.lpDescription = SERV_dup( sd->lpDescription );

        r = ChangeServiceConfig2W( hService, dwInfoLevel, &sdw );

        HeapFree( GetProcessHeap(), 0, sdw.lpDescription );
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

        HeapFree( GetProcessHeap(), 0, faw.lpRebootMsg );
        HeapFree( GetProcessHeap(), 0, faw.lpCommand );
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
        FIXME("STUB: %p %d %p\n",hService, dwInfoLevel, lpInfo);
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
    SECURITY_DESCRIPTOR descriptor;
    DWORD size;
    BOOL succ;
    ACL acl;

    FIXME("%p %d %p %u %p - semi-stub\n", hService, dwSecurityInformation,
          lpSecurityDescriptor, cbBufSize, pcbBytesNeeded);

    if (dwSecurityInformation != DACL_SECURITY_INFORMATION)
        FIXME("information %d not supported\n", dwSecurityInformation);

    InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION);

    InitializeAcl(&acl, sizeof(ACL), ACL_REVISION);
    SetSecurityDescriptorDacl(&descriptor, TRUE, &acl, TRUE);

    size = cbBufSize;
    succ = MakeSelfRelativeSD(&descriptor, lpSecurityDescriptor, &size);
    *pcbBytesNeeded = size;
    return succ;
}

/******************************************************************************
 * SetServiceObjectSecurity [ADVAPI32.@]
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

SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExA( LPCSTR lpServiceName,
        LPHANDLER_FUNCTION_EX lpHandlerProc, LPVOID lpContext )
{
    FIXME("%s %p %p\n", debugstr_a(lpServiceName), lpHandlerProc, lpContext);
    return 0;
}

SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExW( LPCWSTR lpServiceName,
        LPHANDLER_FUNCTION_EX lpHandlerProc, LPVOID lpContext )
{
    service_data *service;
    SERVICE_STATUS_HANDLE handle = 0;

    TRACE("%s %p %p\n", debugstr_w(lpServiceName), lpHandlerProc, lpContext);

    EnterCriticalSection( &service_cs );
    LIST_FOR_EACH_ENTRY( service, &service_list, service_data, entry )
    {
        if(!strcmpW(lpServiceName, service->name))
        {
            service->handler.handler_ex = lpHandlerProc;
            service->context = lpContext;
            service->extended = TRUE;
            handle = (SERVICE_STATUS_HANDLE)service;
            break;
        }
    }
    LeaveCriticalSection( &service_cs );

    return handle;
}
