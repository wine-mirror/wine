/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
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
#include "heap.h"
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

struct SEB              /* service environment block */
{                       /*   resides in service's shared memory object */
    DWORD argc;
    /* variable part of SEB contains service arguments */
};

/******************************************************************************
 * SC_HANDLEs
 */

#define MAX_SERVICE_NAME 256

typedef enum { SC_HTYPE_MANAGER, SC_HTYPE_SERVICE } SC_HANDLE_TYPE;

struct sc_handle;

struct sc_manager       /* SCM handle */
{
    HKEY hkey_scm_db;   /* handle to services database in the registry */
    LONG ref_count;     /* handle must remain alive until any related service */
                        /* handle exists because DeleteService requires it */
};

struct sc_service       /* service handle */
{
    HKEY hkey;          /* handle to service entry in the registry (under hkey_scm_db) */
    struct sc_handle *sc_manager;  /* pointer to SCM handle */
    WCHAR name[ MAX_SERVICE_NAME ];
};

struct sc_handle
{
    SC_HANDLE_TYPE htype;
    union
    {
        struct sc_manager manager;
        struct sc_service service;
    } u;
};

static struct sc_handle* alloc_sc_handle( SC_HANDLE_TYPE htype )
{
    struct sc_handle *retval;

    retval = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct sc_handle) );
    if( retval != NULL )
    {
        retval->htype = htype;
    }
    TRACE("SC_HANDLE type=%d -> %p\n",htype,retval);
    return retval;
}

static void free_sc_handle( struct sc_handle* handle )
{
    if( NULL == handle )
        return;

    switch( handle->htype )
    {
        case SC_HTYPE_MANAGER:
        {
            if( InterlockedDecrement( &handle->u.manager.ref_count ) )
                /* there are references to this handle */
                return;

            if( handle->u.manager.hkey_scm_db )
                RegCloseKey( handle->u.manager.hkey_scm_db );
            break;
        }

        case SC_HTYPE_SERVICE:
        {
            struct sc_handle *h = handle->u.service.sc_manager;

            if( h )
            {
                /* release SCM handle */
                if( 0 == InterlockedDecrement( &h->u.manager.ref_count ) )
                {
                    /* it's time to destroy SCM handle */
                    if( h->u.manager.hkey_scm_db )
                        RegCloseKey( h->u.manager.hkey_scm_db );
                    
                    TRACE("SC_HANDLE (SCM) %p type=%d\n",h,h->htype);
                    
                    HeapFree( GetProcessHeap(), 0, h );
                }
            }
            if( handle->u.service.hkey )
                RegCloseKey( handle->u.service.hkey );
            break;
        }
    }

    TRACE("SC_HANDLE %p type=%d\n",handle,handle->htype);

    HeapFree( GetProcessHeap(), 0, handle );
}

static void init_service_handle( struct sc_handle* handle,
                                 struct sc_handle* sc_manager,
                                 HKEY hKey, LPCWSTR lpServiceName )
{
    /* init sc_service structure */
    handle->u.service.hkey = hKey;
    lstrcpynW( handle->u.service.name, lpServiceName, MAX_SERVICE_NAME );

    /* add reference to SCM handle */
    InterlockedIncrement( &sc_manager->u.manager.ref_count );
    handle->u.service.sc_manager = sc_manager;
}

/******************************************************************************
 * EnumServicesStatusA [ADVAPI32.@]
 */
BOOL WINAPI
EnumServicesStatusA( SC_HANDLE hSCManager, DWORD dwServiceType,
                     DWORD dwServiceState, LPENUM_SERVICE_STATUSA lpServices,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                     LPDWORD lpServicesReturned, LPDWORD lpResumeHandle )
{	FIXME("%p type=%lx state=%lx %p %lx %p %p %p\n", hSCManager,
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
{	FIXME("%p type=%lx state=%lx %p %lx %p %p %p\n", hSCManager,
		dwServiceType, dwServiceState, lpServices, cbBufSize,
		pcbBytesNeeded, lpServicesReturned,  lpResumeHandle);
	SetLastError (ERROR_ACCESS_DENIED);
	return FALSE;
}

/******************************************************************************
 * read_scm_lock_data
 *
 * helper function for StartServiceCtrlDispatcherA/W
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
 * helper function for StartServiceCtrlDispatcherA/W
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
 * helper function for StartServiceCtrlDispatcherA/W
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
 * StartServiceCtrlDispatcherA [ADVAPI32.@]
 */
BOOL WINAPI
StartServiceCtrlDispatcherA( LPSERVICE_TABLE_ENTRYA servent )
{
    LPSERVICE_MAIN_FUNCTIONA fpMain;
    WCHAR service_name[ MAX_SERVICE_NAME ];
    HANDLE hServiceShmem = NULL;
    struct SEB *seb = NULL;
    DWORD  dwNumServiceArgs = 0;
    LPWSTR *lpArgVecW = NULL;
    LPSTR  *lpArgVecA = NULL;
    unsigned int i;
    BOOL ret = FALSE;

    TRACE("(%p)\n", servent);

    if( ! read_scm_lock_data( service_name ) )
    {
        /* FIXME: Instead of exiting we allow
           service to be executed as ordinary program.
           This behaviour was specially introduced in the patch
           submitted against revision 1.45 and so preserved here.
         */
        FIXME("should fail with ERROR_FAILED_SERVICE_CONTROLLER_CONNECT\n");
        servent->lpServiceProc( 0, NULL );
        return TRUE;
    }

    seb = open_seb_shmem( service_name, &hServiceShmem );
    if( NULL == seb )
        return FALSE;

    lpArgVecW = build_arg_vectors( seb );
    if( NULL == lpArgVecW )
        goto done;

    lpArgVecW[0] = service_name;
    dwNumServiceArgs = seb->argc + 1;

    /* Convert the Unicode arg vectors back to ASCII */
    lpArgVecA = (LPSTR*) HeapAlloc( GetProcessHeap(), 0,
                                    dwNumServiceArgs*sizeof(LPSTR) );
    for(i=0; i<dwNumServiceArgs; i++)
        lpArgVecA[i]=HEAP_strdupWtoA(GetProcessHeap(), 0, lpArgVecW[i]);

    /* FIXME: find service entry by name if SERVICE_WIN32_SHARE_PROCESS */
    TRACE("%s at %p)\n", debugstr_a(servent->lpServiceName),servent);
    fpMain = servent->lpServiceProc;

    /* try to start the service */
    fpMain( dwNumServiceArgs, lpArgVecA);

done:
    if(dwNumServiceArgs)
    {
        /* free arg strings */
        for(i=0; i<dwNumServiceArgs; i++)
            HeapFree(GetProcessHeap(), 0, lpArgVecA[i]);
        HeapFree(GetProcessHeap(), 0, lpArgVecA);
    }

    if( lpArgVecW ) HeapFree( GetProcessHeap(), 0, lpArgVecW );
    if( seb ) UnmapViewOfFile( seb );
    if( hServiceShmem ) CloseHandle( hServiceShmem );
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
    LPSERVICE_MAIN_FUNCTIONW fpMain;
    WCHAR service_name[ MAX_SERVICE_NAME ];
    HANDLE hServiceShmem = NULL;
    struct SEB *seb;
    DWORD  dwNumServiceArgs ;
    LPWSTR *lpServiceArgVectors ;

    TRACE("(%p)\n", servent);

    if( ! read_scm_lock_data( service_name ) )
        return FALSE;

    seb = open_seb_shmem( service_name, &hServiceShmem );
    if( NULL == seb )
        return FALSE;

    lpServiceArgVectors = build_arg_vectors( seb );
    if( NULL == lpServiceArgVectors )
    {
        UnmapViewOfFile( seb );
        CloseHandle( hServiceShmem );
        return FALSE;
    }
    lpServiceArgVectors[0] = service_name;
    dwNumServiceArgs = seb->argc + 1;

    /* FIXME: find service entry by name if SERVICE_WIN32_SHARE_PROCESS */
    TRACE("%s at %p)\n", debugstr_w(servent->lpServiceName),servent);
    fpMain = servent->lpServiceProc;

    /* try to start the service */
    fpMain( dwNumServiceArgs, lpServiceArgVectors);

    HeapFree( GetProcessHeap(), 0, lpServiceArgVectors );
    UnmapViewOfFile( seb );
    CloseHandle( hServiceShmem );
    return TRUE;
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
{	FIXME("%s %p\n", lpServiceName, lpfHandler);
	return 0xcacacafe;
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
{	FIXME("%s %p\n", debugstr_w(lpServiceName), lpfHandler);
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
{	FIXME("0x%lx %p\n",hService, lpStatus);
	TRACE("\tType:%lx\n",lpStatus->dwServiceType);
	TRACE("\tState:%lx\n",lpStatus->dwCurrentState);
	TRACE("\tControlAccepted:%lx\n",lpStatus->dwControlsAccepted);
	TRACE("\tExitCode:%lx\n",lpStatus->dwWin32ExitCode);
	TRACE("\tServiceExitCode:%lx\n",lpStatus->dwServiceSpecificExitCode);
	TRACE("\tCheckPoint:%lx\n",lpStatus->dwCheckPoint);
	TRACE("\tWaitHint:%lx\n",lpStatus->dwWaitHint);
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
    UNICODE_STRING lpMachineNameW;
    UNICODE_STRING lpDatabaseNameW;
    SC_HANDLE ret;

    RtlCreateUnicodeStringFromAsciiz (&lpMachineNameW,lpMachineName);
    RtlCreateUnicodeStringFromAsciiz (&lpDatabaseNameW,lpDatabaseName);
    ret = OpenSCManagerW(lpMachineNameW.Buffer,lpDatabaseNameW.Buffer, dwDesiredAccess);
    RtlFreeUnicodeString(&lpDatabaseNameW);
    RtlFreeUnicodeString(&lpMachineNameW);
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
    struct sc_handle *retval;
    HKEY hReg;
    LONG r;

    TRACE("(%s,%s,0x%08lx)\n", debugstr_w(lpMachineName),
          debugstr_w(lpDatabaseName), dwDesiredAccess);

    /*
     * FIXME: what is lpDatabaseName?
     * It should be set to "SERVICES_ACTIVE_DATABASE" according to
     * docs, but what if it isn't?
     */

    retval = alloc_sc_handle( SC_HTYPE_MANAGER );
    if( NULL == retval ) return NULL;

    retval->u.manager.ref_count = 1;

    r = RegConnectRegistryW(lpMachineName,HKEY_LOCAL_MACHINE,&hReg);
    if (r!=ERROR_SUCCESS)
        goto error;

    r = RegOpenKeyExW(hReg, szServiceManagerKey,
                      0, KEY_ALL_ACCESS, &retval->u.manager.hkey_scm_db);
    RegCloseKey( hReg );
    if (r!=ERROR_SUCCESS)
        goto error;

    TRACE("returning %p\n", retval);

    return (SC_HANDLE) retval;

error:
    free_sc_handle( retval );
    return NULL;
}


/******************************************************************************
 * AllocateLocallyUniqueId [ADVAPI32.@]
 *
 * PARAMS
 *   lpluid []
 */
BOOL WINAPI
AllocateLocallyUniqueId( PLUID lpluid )
{
	lpluid->LowPart = time(NULL);
	lpluid->HighPart = 0;
	return TRUE;
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
 */
BOOL WINAPI ControlService( SC_HANDLE hService, DWORD dwControl,
                            LPSERVICE_STATUS lpServiceStatus )
{
    FIXME("(%p,%ld,%p): stub\n",hService,dwControl,lpServiceStatus);
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
    TRACE("(%p)\n", hSCObject);

    free_sc_handle( (struct sc_handle*) hSCObject );

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
    UNICODE_STRING lpServiceNameW;
    SC_HANDLE ret;
    RtlCreateUnicodeStringFromAsciiz (&lpServiceNameW,lpServiceName);
    if(lpServiceName)
        TRACE("Request for service %s\n",lpServiceName);
    else
        return FALSE;
    ret = OpenServiceW( hSCManager, lpServiceNameW.Buffer, dwDesiredAccess);
    RtlFreeUnicodeString(&lpServiceNameW);
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
    struct sc_handle *hscm = hSCManager;
    struct sc_handle *retval;
    HKEY hKey;
    long r;

    TRACE("(%p,%p,%ld)\n",hSCManager, lpServiceName,
          dwDesiredAccess);

    retval = alloc_sc_handle( SC_HTYPE_SERVICE );
    if( NULL == retval )
        return NULL;

    r = RegOpenKeyExW( hscm->u.manager.hkey_scm_db,
                       lpServiceName, 0, KEY_ALL_ACCESS, &hKey );
    if (r!=ERROR_SUCCESS)
    {
        free_sc_handle( retval );
        SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
        return NULL;
    }
    
    init_service_handle( retval, hscm, hKey, lpServiceName );

    TRACE("returning %p\n",retval);

    return (SC_HANDLE) retval;
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
    struct sc_handle *hscm = hSCManager;
    struct sc_handle *retval;
    HKEY hKey;
    LONG r;
    DWORD dp;
    static const WCHAR szDisplayName[] = { 'D','i','s','p','l','a','y','N','a','m','e', 0 };
    static const WCHAR szType[] = {'T','y','p','e',0};
    static const WCHAR szStart[] = {'S','t','a','r','t',0};
    static const WCHAR szError[] = {'E','r','r','o','r','C','o','n','t','r','o','l', 0};
    static const WCHAR szImagePath[] = {'I','m','a','g','e','P','a','t','h',0};
    static const WCHAR szGroup[] = {'G','r','o','u','p',0};
    static const WCHAR szDependencies[] = { 'D','e','p','e','n','d','e','n','c','i','e','s',0};

    FIXME("%p %s %s\n", hSCManager, 
          debugstr_w(lpServiceName), debugstr_w(lpDisplayName));

    retval = alloc_sc_handle( SC_HTYPE_SERVICE );
    if( NULL == retval )
        return NULL;

    r = RegCreateKeyExW(hscm->u.manager.hkey_scm_db, lpServiceName, 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dp);
    if (r!=ERROR_SUCCESS)
        goto error;

    init_service_handle( retval, hscm, hKey, lpServiceName );

    if (dp != REG_CREATED_NEW_KEY)
        goto error;

    if(lpDisplayName)
    {
        r = RegSetValueExW(hKey, szDisplayName, 0, REG_SZ, (LPBYTE)lpDisplayName,
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
        r = RegSetValueExW(hKey, szImagePath, 0, REG_SZ, (LPBYTE)lpBinaryPathName,
                           (strlenW(lpBinaryPathName)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            goto error;
    }

    if(lpLoadOrderGroup)
    {
        r = RegSetValueExW(hKey, szGroup, 0, REG_SZ, (LPBYTE)lpLoadOrderGroup,
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
                           (LPBYTE)lpDependencies, len );
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

    return (SC_HANDLE) retval;
    
error:
    free_sc_handle( retval );
    return NULL;
}


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
    struct sc_handle *hsvc = hService;
    HKEY hKey = hsvc->u.service.hkey;
    WCHAR valname[MAX_PATH+1];
    INT index = 0;
    LONG rc;
    DWORD size;

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
    hsvc->u.service.hkey = NULL;

    /* delete the key */
    RegDeleteKeyW(hsvc->u.service.sc_manager->u.manager.hkey_scm_db,
                  hsvc->u.service.name);

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
    UNICODE_STRING usBuffer;
    unsigned int i;

    TRACE("(%p,%ld,%p)\n",hService,dwNumServiceArgs,lpServiceArgVectors);

    if(dwNumServiceArgs)
        lpwstr = (LPWSTR*) HeapAlloc( GetProcessHeap(), 0,
                                   dwNumServiceArgs*sizeof(LPWSTR) );
    else
        lpwstr = NULL;

    for(i=0; i<dwNumServiceArgs; i++)
    {
        RtlCreateUnicodeStringFromAsciiz (&usBuffer,lpServiceArgVectors[i]);
        lpwstr[i]=usBuffer.Buffer;
    }

    StartServiceW(hService, dwNumServiceArgs, (LPCWSTR *)lpwstr);

    if(dwNumServiceArgs)
    {
        for(i=0; i<dwNumServiceArgs; i++)
            HeapFree(GetProcessHeap(), 0, lpwstr[i]);
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
    static const WCHAR  _WaitServiceStartW[]  = {'A','D','V','A','P','I','_','W',
                                                'a','i','t','S','e','r','v','i',
                                                'c','e','S','t','a','r','t',0};
    static const WCHAR  _ImagePathW[]  = {'I','m','a','g','e','P','a','t','h',0};
                                                
    struct sc_handle *hsvc = hService;
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

    TRACE("(%p,%ld,%p)\n",hService,dwNumServiceArgs,
          lpServiceArgVectors);

    size = sizeof(str);
    r = RegQueryValueExW(hsvc->u.service.hkey, _ImagePathW, NULL, &type, (LPVOID)str, &size);
    if (r!=ERROR_SUCCESS)
        return FALSE;
    ExpandEnvironmentStringsW(str,path,sizeof(path));

    TRACE("Starting service %s\n", debugstr_w(path) );

    hLock = LockServiceDatabase( hsvc->u.service.sc_manager );
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
    strcpyW( shmem_lock, hsvc->u.service.name );

    /* create service environment block */
    size = sizeof(struct SEB);
    for( i = 0; i < dwNumServiceArgs; i++ )
        size += sizeof(WCHAR) * (1 + strlenW( lpServiceArgVectors[ i ] ));

    snprintfW( str, MAX_PATH, szServiceShmemNameFmtW, hsvc->u.service.name );
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

    wait = CreateSemaphoreW(NULL,0,1,_WaitServiceStartW);
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
    struct sc_handle *hsvc = hService;
    LONG r;
    DWORD type, val, size;

    FIXME("(%p,%p) partial\n",hService,lpservicestatus);

    /* read the service type from the registry */
    size = sizeof(val);
    r = RegQueryValueExA(hsvc->u.service.hkey, "Type", NULL, &type, (LPBYTE)&val, &size);
    if(type!=REG_DWORD)
    {
        ERR("invalid Type\n");
        return FALSE;
    }
    lpservicestatus->dwServiceType = val;
    /* FIXME: how are these determined or read from the registry? */
    /* SERVICE: unavailable=0, stopped=1, starting=2, running=3? */;
    lpservicestatus->dwCurrentState            = 1;
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
    HKEY hKey = ((struct sc_handle*) hService)->u.service.hkey;
    CHAR str_buffer[ MAX_PATH ];
    LONG r;
    DWORD type, val, sz, total, n;
    LPBYTE p;

    TRACE("%p %p %ld %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

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
    HKEY hKey = ((struct sc_handle*) hService)->u.service.hkey;
    WCHAR str_buffer[ MAX_PATH ];
    LONG r;
    DWORD type, val, sz, total, n;
    LPBYTE p;

    TRACE("%p %p %ld %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

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
    HKEY hKey = ((struct sc_handle*) hService)->u.service.hkey;

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
    FIXME("%p %ld %p %lu %p\n", hService, dwSecurityInformation,
          lpSecurityDescriptor, cbBufSize, pcbBytesNeeded);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
