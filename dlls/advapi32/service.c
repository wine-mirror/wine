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

static DWORD   start_dwNumServiceArgs;
static LPWSTR *start_lpServiceArgVectors;

static const WCHAR _ServiceStartDataW[]  = {'A','D','V','A','P','I','_','S',
                                            'e','r','v','i','c','e','S','t',
                                            'a','r','t','D','a','t','a',0};

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
 * StartServiceCtrlDispatcherA [ADVAPI32.@]
 */
BOOL WINAPI
StartServiceCtrlDispatcherA( LPSERVICE_TABLE_ENTRYA servent )
{
    LPSERVICE_MAIN_FUNCTIONA fpMain;
    HANDLE wait;
    DWORD  dwNumServiceArgs ;
    LPWSTR *lpArgVecW;
    LPSTR  *lpArgVecA;
    int i;

    TRACE("(%p)\n", servent);
    wait = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, _ServiceStartDataW);
    if(wait == 0)
    {
        ERR("Couldn't find wait semaphore\n");
        ERR("perhaps you need to start services using StartService\n");
        return FALSE;
    }

    dwNumServiceArgs = start_dwNumServiceArgs;
    lpArgVecW        = start_lpServiceArgVectors;

    ReleaseSemaphore(wait, 1, NULL);

    /* Convert the Unicode arg vectors back to ASCII */
    if(dwNumServiceArgs)
        lpArgVecA = (LPSTR*) HeapAlloc( GetProcessHeap(), 0,
                                   dwNumServiceArgs*sizeof(LPSTR) );
    else
        lpArgVecA = NULL;

    for(i=0; i<dwNumServiceArgs; i++)
        lpArgVecA[i]=HEAP_strdupWtoA(GetProcessHeap(), 0, lpArgVecW[i]);

    /* FIXME: should we blindly start all services? */
    while (servent->lpServiceName) {
        TRACE("%s at %p)\n", debugstr_a(servent->lpServiceName),servent);
        fpMain = servent->lpServiceProc;

        /* try to start the service */
        fpMain( dwNumServiceArgs, lpArgVecA);

        servent++;
    }

    if(dwNumServiceArgs)
    {
        /* free arg strings */
        for(i=0; i<dwNumServiceArgs; i++)
            HeapFree(GetProcessHeap(), 0, lpArgVecA[i]);
        HeapFree(GetProcessHeap(), 0, lpArgVecA);
    }

    return TRUE;
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
    HANDLE wait;
    DWORD  dwNumServiceArgs ;
    LPWSTR *lpServiceArgVectors ;

    TRACE("(%p)\n", servent);
    wait = OpenSemaphoreW(SEMAPHORE_ALL_ACCESS, FALSE, _ServiceStartDataW);
    if(wait == 0)
    {
        ERR("Couldn't find wait semaphore\n");
        ERR("perhaps you need to start services using StartService\n");
        return FALSE;
    }

    dwNumServiceArgs    = start_dwNumServiceArgs;
    lpServiceArgVectors = start_lpServiceArgVectors;

    ReleaseSemaphore(wait, 1, NULL);

    /* FIXME: should we blindly start all services? */
    while (servent->lpServiceName) {
        TRACE("%s at %p)\n", debugstr_w(servent->lpServiceName),servent);
        fpMain = servent->lpServiceProc;

        /* try to start the service */
        fpMain( dwNumServiceArgs, lpServiceArgVectors);

        servent++;
    }

    return TRUE;
}

/******************************************************************************
 * LockServiceDatabase  [ADVAPI32.@]
 */
LPVOID WINAPI LockServiceDatabase (SC_HANDLE hSCManager)
{
        FIXME("%p\n",hSCManager);
        return (SC_HANDLE)0xcacacafe;
}

/******************************************************************************
 * UnlockServiceDatabase  [ADVAPI32.@]
 */
BOOL WINAPI UnlockServiceDatabase (LPVOID ScLock)
{
        FIXME(": %p\n",ScLock);
	return TRUE;
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
 */
SC_HANDLE WINAPI
OpenSCManagerA( LPCSTR lpMachineName, LPCSTR lpDatabaseName,
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
 * Establishes a connection to the service control manager and opens database
 *
 * NOTES
 *   This should return a SC_HANDLE
 *
 * PARAMS
 *   lpMachineName   [I] Pointer to machine name string
 *   lpDatabaseName  [I] Pointer to database name string
 *   dwDesiredAccess [I] Type of access
 *
 * RETURNS
 *   Success: Handle to service control manager database
 *   Failure: NULL
 */
SC_HANDLE WINAPI
OpenSCManagerW( LPCWSTR lpMachineName, LPCWSTR lpDatabaseName,
                  DWORD dwDesiredAccess )
{
    const WCHAR szKey[] = { 'S','y','s','t','e','m','\\',
      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
      'S','e','r','v','i','c','e','s','\\',0 };
    HKEY hReg, hKey = NULL;
    LONG r;

    TRACE("(%s,%s,0x%08lx)\n", debugstr_w(lpMachineName),
          debugstr_w(lpDatabaseName), dwDesiredAccess);

    /*
     * FIXME: what is lpDatabaseName?
     * It should be set to "SERVICES_ACTIVE_DATABASE" according to
     * docs, but what if it isn't?
     */

    r = RegConnectRegistryW(lpMachineName,HKEY_LOCAL_MACHINE,&hReg);
    if (r==ERROR_SUCCESS)
    {
        r = RegOpenKeyExW(hReg, szKey, 0, dwDesiredAccess, &hKey );
        RegCloseKey( hReg );
    }

    TRACE("returning %p\n", hKey);

    return hKey;
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
 * Sends a control code to a Win32-based service.
 *
 * PARAMS
 *   hService        []
 *   dwControl       []
 *   lpServiceStatus []
 *
 * RETURNS STD
 */
BOOL WINAPI
ControlService( SC_HANDLE hService, DWORD dwControl,
                LPSERVICE_STATUS lpServiceStatus )
{
    FIXME("(%p,%ld,%p): stub\n",hService,dwControl,lpServiceStatus);
    return TRUE;
}


/******************************************************************************
 * CloseServiceHandle [ADVAPI32.@]
 * Close handle to service or service control manager
 *
 * PARAMS
 *   hSCObject [I] Handle to service or service control manager database
 *
 * RETURNS STD
 */
BOOL WINAPI
CloseServiceHandle( SC_HANDLE hSCObject )
{
    TRACE("(%p)\n", hSCObject);

    RegCloseKey(hSCObject);

    return TRUE;
}


/******************************************************************************
 * OpenServiceA [ADVAPI32.@]
 */
SC_HANDLE WINAPI
OpenServiceA( SC_HANDLE hSCManager, LPCSTR lpServiceName,
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
 * Opens a handle to an existing service
 *
 * PARAMS
 *   hSCManager      []
 *   lpServiceName   []
 *   dwDesiredAccess []
 *
 * RETURNS
 *    Success: Handle to the service
 *    Failure: NULL
 */
SC_HANDLE WINAPI
OpenServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName,
               DWORD dwDesiredAccess)
{
    HKEY hKey;
    long r;

    TRACE("(%p,%p,%ld)\n",hSCManager, lpServiceName,
          dwDesiredAccess);

    /* FIXME: dwDesiredAccess may need some processing */
    r = RegOpenKeyExW(hSCManager, lpServiceName, 0, dwDesiredAccess, &hKey );
    if (r!=ERROR_SUCCESS)
        return 0;

    TRACE("returning %p\n",hKey);

    return hKey;
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
    HKEY hKey;
    LONG r;
    DWORD dp;
    const WCHAR szDisplayName[] = { 'D','i','s','p','l','a','y','N','a','m','e', 0 };
    const WCHAR szType[] = {'T','y','p','e',0};
    const WCHAR szStart[] = {'S','t','a','r','t',0};
    const WCHAR szError[] = {'E','r','r','o','r','C','o','n','t','r','o','l', 0};
    const WCHAR szImagePath[] = {'I','m','a','g','e','P','a','t','h',0};
    const WCHAR szGroup[] = {'G','r','o','u','p',0};
    const WCHAR szDependencies[] = { 'D','e','p','e','n','d','e','n','c','i','e','s',0};

    FIXME("%p %s %s\n", hSCManager, 
          debugstr_w(lpServiceName), debugstr_w(lpDisplayName));

    r = RegCreateKeyExW(hSCManager, lpServiceName, 0, NULL,
                       REG_OPTION_NON_VOLATILE, dwDesiredAccess, NULL, &hKey, &dp);
    if (r!=ERROR_SUCCESS)
        return 0;
    if (dp != REG_CREATED_NEW_KEY)
        return 0;

    if(lpDisplayName)
    {
        r = RegSetValueExW(hKey, szDisplayName, 0, REG_SZ, (LPBYTE)lpDisplayName,
                           (strlenW(lpDisplayName)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            return 0;
    }

    r = RegSetValueExW(hKey, szType, 0, REG_DWORD, (LPVOID)&dwServiceType, sizeof (DWORD) );
    if (r!=ERROR_SUCCESS)
        return 0;

    r = RegSetValueExW(hKey, szStart, 0, REG_DWORD, (LPVOID)&dwStartType, sizeof (DWORD) );
    if (r!=ERROR_SUCCESS)
        return 0;

    r = RegSetValueExW(hKey, szError, 0, REG_DWORD,
                           (LPVOID)&dwErrorControl, sizeof (DWORD) );
    if (r!=ERROR_SUCCESS)
        return 0;

    if(lpBinaryPathName)
    {
        r = RegSetValueExW(hKey, szImagePath, 0, REG_SZ, (LPBYTE)lpBinaryPathName,
                           (strlenW(lpBinaryPathName)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            return 0;
    }

    if(lpLoadOrderGroup)
    {
        r = RegSetValueExW(hKey, szGroup, 0, REG_SZ, (LPBYTE)lpLoadOrderGroup,
                           (strlenW(lpLoadOrderGroup)+1)*sizeof(WCHAR) );
        if (r!=ERROR_SUCCESS)
            return 0;
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
            return 0;
    }

    if(lpPassword)
    {
        FIXME("Don't know how to add a Password for a service.\n");
    }

    if(lpServiceStartName)
    {
        FIXME("Don't know how to add a ServiceStartName for a service.\n");
    }

    return hKey;
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
 * PARAMS
 *    hService [I] Handle to service
 *
 * RETURNS STD
 *
 */
BOOL WINAPI
DeleteService( SC_HANDLE hService )
{
    FIXME("(%p): stub\n",hService);
    return TRUE;
}


/******************************************************************************
 * StartServiceA [ADVAPI32.@]
 *
 */
BOOL WINAPI
StartServiceA( SC_HANDLE hService, DWORD dwNumServiceArgs,
                 LPCSTR *lpServiceArgVectors )
{
    LPWSTR *lpwstr=NULL;
    UNICODE_STRING usBuffer;
    int i;

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
 * Starts a service
 *
 * PARAMS
 *   hService            [I] Handle of service
 *   dwNumServiceArgs    [I] Number of arguments
 *   lpServiceArgVectors [I] Address of array of argument string pointers
 *
 * NOTES
 *
 * NT implements this function using an obscure RPC call...
 *
 * Might need to do a "setenv SystemRoot \\WINNT" in your .cshrc
 *   to get things like %SystemRoot%\\System32\\service.exe to load.
 *
 * Will only work for shared address space. How should the service
 *  args be transferred when address spaces are separated?
 *
 * Can only start one service at a time.
 *
 * Has no concept of privilege.
 *
 * RETURNS STD
 *
 */
BOOL WINAPI
StartServiceW( SC_HANDLE hService, DWORD dwNumServiceArgs,
                 LPCWSTR *lpServiceArgVectors )
{
    static const WCHAR  _WaitServiceStartW[]  = {'A','D','V','A','P','I','_','W',
                                                'a','i','t','S','e','r','v','i',
                                                'c','e','S','t','a','r','t',0};
    static const WCHAR  _ImagePathW[]  = {'I','m','a','g','e','P','a','t','h',0};
                                                
    WCHAR path[MAX_PATH],str[MAX_PATH];
    DWORD type,size;
    long r;
    HANDLE data,wait;
    PROCESS_INFORMATION procinfo;
    STARTUPINFOW startupinfo;
    TRACE("(%p,%ld,%p)\n",hService,dwNumServiceArgs,
          lpServiceArgVectors);

    size = sizeof(str);
    r = RegQueryValueExW(hService, _ImagePathW, NULL, &type, (LPVOID)str, &size);
    if (r!=ERROR_SUCCESS)
        return FALSE;
    ExpandEnvironmentStringsW(str,path,sizeof(path));

    TRACE("Starting service %s\n", debugstr_w(path) );

    data = CreateSemaphoreW(NULL,1,1,_ServiceStartDataW);
    if (!data)
    {
        ERR("Couldn't create data semaphore\n");
        return FALSE;
    }
    wait = CreateSemaphoreW(NULL,0,1,_WaitServiceStartW);
    if (!wait)
    {
        ERR("Couldn't create wait semaphore\n");
        return FALSE;
    }

    /*
     * FIXME: lpServiceArgsVectors need to be stored and returned to
     *        the service when it calls StartServiceCtrlDispatcher
     *
     * Chuck these in a global (yuk) so we can pass them to
     * another process - address space separation will break this.
     */

    r = WaitForSingleObject(data,INFINITE);

    if( r == WAIT_FAILED)
        return FALSE;

    FIXME("problematic because of address space separation.\n");
    start_dwNumServiceArgs    = dwNumServiceArgs;
    start_lpServiceArgVectors = (LPWSTR *)lpServiceArgVectors;

    ZeroMemory(&startupinfo,sizeof(STARTUPINFOW));
    startupinfo.cb = sizeof(STARTUPINFOW);

    r = CreateProcessW(path,
                   NULL,
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
        /* ReleaseSemaphore(data, 1, NULL);
        return FALSE; */
    }

    /* docs for StartServiceCtrlDispatcher say this should be 30 sec */
    r = WaitForSingleObject(wait,30000);

    ReleaseSemaphore(data, 1, NULL);

    if( r == WAIT_FAILED)
        return FALSE;

    return TRUE;
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
    LONG r;
    DWORD type, val, size;

    FIXME("(%p,%p) partial\n",hService,lpservicestatus);

    /* read the service type from the registry */
    size = sizeof(val);
    r = RegQueryValueExA(hService, "Type", NULL, &type, (LPBYTE)&val, &size);
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
 * PARAMS
 *   hService       [handle to service]
 *   InfoLevel      [information level]
 *   lpBuffer       [buffer]
 *   cbBufSize      [size of buffer]
 *   pcbBytesNeeded [bytes needed]
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
    FIXME("%p %p %ld %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);
    return FALSE;
}

/******************************************************************************
 * QueryServiceConfigW [ADVAPI32.@]
 */
BOOL WINAPI 
QueryServiceConfigW( SC_HANDLE hService,
                     LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                     DWORD cbBufSize, LPDWORD pcbBytesNeeded)
{
    const WCHAR szDisplayName[] = {
        'D','i','s','p','l','a','y','N','a','m','e', 0 };
    const WCHAR szType[] = {'T','y','p','e',0};
    const WCHAR szStart[] = {'S','t','a','r','t',0};
    const WCHAR szError[] = {
        'E','r','r','o','r','C','o','n','t','r','o','l', 0};
    const WCHAR szImagePath[] = {'I','m','a','g','e','P','a','t','h',0};
    const WCHAR szGroup[] = {'G','r','o','u','p',0};
    const WCHAR szDependencies[] = { 
        'D','e','p','e','n','d','e','n','c','i','e','s',0};
    LONG r;
    DWORD type, val, sz, total, n;
    LPBYTE p;

    TRACE("%p %p %ld %p\n", hService, lpServiceConfig,
           cbBufSize, pcbBytesNeeded);

    /* calculate the size required first */
    total = sizeof (QUERY_SERVICE_CONFIGW);

    sz = 0;
    r = RegQueryValueExW( hService, szImagePath, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hService, szGroup, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hService, szDependencies, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_MULTI_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hService, szStart, 0, &type, NULL, &sz );
    if( ( r == ERROR_SUCCESS ) && ( type == REG_SZ ) )
        total += sz;

    sz = 0;
    r = RegQueryValueExW( hService, szDisplayName, 0, &type, NULL, &sz );
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
    r = RegQueryValueExW( hService, szType, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwServiceType = val;

    sz = sizeof val;
    r = RegQueryValueExW( hService, szStart, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwStartType = val;

    sz = sizeof val;
    r = RegQueryValueExW( hService, szError, 0, &type, (LPBYTE)&val, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_DWORD ) )
        lpServiceConfig->dwErrorControl = val;

    /* now do the strings */
    p = (LPBYTE) &lpServiceConfig[1];
    n = total - sizeof (QUERY_SERVICE_CONFIGW);

    sz = n;
    r = RegQueryValueExW( hService, szImagePath, 0, &type, p, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_SZ ) )
    {
        lpServiceConfig->lpBinaryPathName = (LPWSTR) p;
        p += sz;
        n -= sz;
    }

    sz = n;
    r = RegQueryValueExW( hService, szGroup, 0, &type, p, &sz );
    if( ( r == ERROR_SUCCESS ) || ( type == REG_SZ ) )
    {
        lpServiceConfig->lpLoadOrderGroup = (LPWSTR) p;
        p += sz;
        n -= sz;
    }

    sz = n;
    r = RegQueryValueExW( hService, szDependencies, 0, &type, p, &sz );
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
    FIXME("%p %ld %ld %ld %s %s %p %p %s %s %s\n",
          hService, dwServiceType, dwStartType, dwErrorControl, 
          debugstr_a(lpBinaryPathName), debugstr_a(lpLoadOrderGroup),
          lpdwTagId, lpDependencies, debugstr_a(lpServiceStartName),
          debugstr_a(lpPassword), debugstr_a(lpDisplayName) );
    return TRUE;
}
