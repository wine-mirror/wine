/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include <time.h>

#include "windef.h"
#include "winsvc.h"
#include "winerror.h"
#include "winreg.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(advapi)

/******************************************************************************
 * EnumServicesStatus32A [ADVAPI32.38]
 */
BOOL WINAPI
EnumServicesStatusA( SC_HANDLE hSCManager, DWORD dwServiceType,
                       DWORD dwServiceState, LPENUM_SERVICE_STATUSA lpServices,
                       DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned, LPDWORD lpResumeHandle )
{	FIXME("%x type=%lx state=%lx %p %lx %p %p %p\n", hSCManager, 
		dwServiceType, dwServiceState, lpServices, cbBufSize,
		pcbBytesNeeded, lpServicesReturned,  lpResumeHandle);
	SetLastError (ERROR_ACCESS_DENIED);
	return 0;
}

/******************************************************************************
 * StartServiceCtrlDispatcher32A [ADVAPI32.196]
 */
BOOL WINAPI
StartServiceCtrlDispatcherA( LPSERVICE_TABLE_ENTRYA servent )
{	LPSERVICE_TABLE_ENTRYA ptr = servent;

	while (ptr->lpServiceName)
	{ FIXME("%s at %p\n", ptr->lpServiceName, ptr);
	  ptr++;
	}
	return TRUE;
}

/******************************************************************************
 * StartServiceCtrlDispatcher32W [ADVAPI32.197]
 *
 * PARAMS
 *   servent []
 */
BOOL WINAPI
StartServiceCtrlDispatcherW( LPSERVICE_TABLE_ENTRYW servent )
{	LPSERVICE_TABLE_ENTRYW ptr = servent;
	LPSERVICE_MAIN_FUNCTIONW fpMain;
	
	while (ptr->lpServiceName)
	{ FIXME("%s at %p): STUB.\n", debugstr_w(ptr->lpServiceName),ptr);
	  fpMain = ptr->lpServiceProc;
	  fpMain(0,NULL);	/* try to start the service */
	  ptr++;
	}
	return TRUE;
}

/******************************************************************************
 * RegisterServiceCtrlHandlerA [ADVAPI32.176]
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerA( LPCSTR lpServiceName,
                             LPHANDLER_FUNCTION lpfHandler )
{	FIXME("%s %p\n", lpServiceName, lpfHandler);
	return 0xcacacafe;	
}

/******************************************************************************
 * RegisterServiceCtrlHandlerW [ADVAPI32.177]
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
 * SetServiceStatus [ADVAPI32.192]
 *
 * PARAMS
 *   hService []
 *   lpStatus []
 */
BOOL WINAPI
SetServiceStatus( SERVICE_STATUS_HANDLE hService, LPSERVICE_STATUS lpStatus )
{	FIXME("%lx %p\n",hService, lpStatus);
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
 * OpenSCManager32A [ADVAPI32.110]
 */
SC_HANDLE WINAPI
OpenSCManagerA( LPCSTR lpMachineName, LPCSTR lpDatabaseName,
                  DWORD dwDesiredAccess )
{   
    LPWSTR lpMachineNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpMachineName);
    LPWSTR lpDatabaseNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpDatabaseName);
    DWORD ret = OpenSCManagerW(lpMachineNameW,lpDatabaseNameW,
                                 dwDesiredAccess);
    HeapFree(GetProcessHeap(),0,lpDatabaseNameW);
    HeapFree(GetProcessHeap(),0,lpMachineNameW);
    return ret;
}

/******************************************************************************
 * OpenSCManager32W [ADVAPI32.111]
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
    HKEY hKey;
    LONG r;

    TRACE("(%s,%s,0x%08lx)\n", debugstr_w(lpMachineName), 
          debugstr_w(lpDatabaseName), dwDesiredAccess);

    /*
     * FIXME: what is lpDatabaseName?
     * It should be set to "SERVICES_ACTIVE_DATABASE" according to
     * docs, but what if it isn't?
     */

    r = RegConnectRegistryW(lpMachineName,HKEY_LOCAL_MACHINE,&hKey);
    if (r!=ERROR_SUCCESS)
        return 0;

    TRACE("returning %x\n",hKey);

    return hKey;
}


/******************************************************************************
 * AllocateLocallyUniqueId [ADVAPI32.12]
 *
 * PARAMS
 *   lpluid []
 */
BOOL WINAPI
AllocateLocallyUniqueId( PLUID lpluid )
{
	lpluid->s.LowPart = time(NULL);
	lpluid->s.HighPart = 0;
	return TRUE;
}


/******************************************************************************
 * ControlService [ADVAPI32.23]
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
    FIXME("(%d,%ld,%p): stub\n",hService,dwControl,lpServiceStatus);
    return TRUE;
}


/******************************************************************************
 * CloseServiceHandle [ADVAPI32.22]
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
    TRACE("(%x)\n", hSCObject);

    RegCloseKey(hSCObject);
    
    return TRUE;
}


/******************************************************************************
 * OpenService32A [ADVAPI32.112]
 */
SC_HANDLE WINAPI
OpenServiceA( SC_HANDLE hSCManager, LPCSTR lpServiceName, 
                DWORD dwDesiredAccess )
{
    LPWSTR lpServiceNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpServiceName);
    DWORD ret;

    if(lpServiceName)
        TRACE("Request for service %s\n",lpServiceName);
    else
        return FALSE;
    ret = OpenServiceW( hSCManager, lpServiceNameW, dwDesiredAccess);
    HeapFree(GetProcessHeap(),0,lpServiceNameW);
    return ret;
}


/******************************************************************************
 * OpenService32W [ADVAPI32.113]
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
    const char *str = "System\\CurrentControlSet\\Services\\";
    WCHAR lpServiceKey[80]; /* FIXME: this should be dynamically allocated */
    HKEY hKey;
    long r;

    TRACE("(%d,%p,%ld)\n",hSCManager, lpServiceName,
          dwDesiredAccess);

    lstrcpyAtoW(lpServiceKey,str);
    lstrcatW(lpServiceKey,lpServiceName);

    TRACE("Opening reg key %s\n", debugstr_w(lpServiceKey));

    /* FIXME: dwDesiredAccess may need some processing */
    r = RegOpenKeyExW(hSCManager, lpServiceKey, 0, dwDesiredAccess, &hKey );
    if (r!=ERROR_SUCCESS)
        return 0;

    TRACE("returning %x\n",hKey);

    return hKey;
}


/******************************************************************************
 * CreateService32A [ADVAPI32.29]
 */
SC_HANDLE WINAPI
CreateServiceA( DWORD hSCManager, LPCSTR lpServiceName,
                  LPCSTR lpDisplayName, DWORD dwDesiredAccess, 
                  DWORD dwServiceType, DWORD dwStartType, 
                  DWORD dwErrorControl, LPCSTR lpBinaryPathName,
                  LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, 
                  LPCSTR lpDependencies, LPCSTR lpServiceStartName, 
                  LPCSTR lpPassword )
{
    FIXME("(%ld,%s,%s,...): stub\n", 
          hSCManager, debugstr_a(lpServiceName), debugstr_a(lpDisplayName));
    return 1;
}


/******************************************************************************
 * DeleteService [ADVAPI32.31]
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
    FIXME("(%d): stub\n",hService);
    return TRUE;
}


/******************************************************************************
 * StartService32A [ADVAPI32.195]
 *
 * NOTES
 *    How do we convert lpServiceArgVectors to use the 32W version?
 */
BOOL WINAPI
StartServiceA( SC_HANDLE hService, DWORD dwNumServiceArgs,
                 LPCSTR *lpServiceArgVectors )
{
    FIXME("(%d,%ld,%p): stub\n",hService,dwNumServiceArgs,lpServiceArgVectors);
    return TRUE;
}


/******************************************************************************
 * StartService32W [ADVAPI32.198]
 * Starts a service
 *
 * PARAMS
 *   hService            [I] Handle of service
 *   dwNumServiceArgs    [I] Number of arguments
 *   lpServiceArgVectors [I] Address of array of argument string pointers
 *
 * RETURNS STD
 *
 */
BOOL WINAPI
StartServiceW( SC_HANDLE hService, DWORD dwNumServiceArgs,
                 LPCWSTR *lpServiceArgVectors )
{
    FIXME("(%d,%ld,%p): stub\n",hService,dwNumServiceArgs,
          lpServiceArgVectors);
    return TRUE;
}

/******************************************************************************
 * QueryServiceStatus [ADVAPI32.123]
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

    FIXME("(%x,%p) partial\n",hService,lpservicestatus);

    /* read the service type from the registry */
    size = sizeof val;
    r = RegQueryValueExA(hService, "Type", NULL, &type, (LPBYTE)&val, &size);
    if(type!=REG_DWORD)
    {
        ERR("invalid Type\n");
        return FALSE;
    }
    lpservicestatus->dwServiceType = val;

    /* FIXME: how are these determined or read from the registry? */
    lpservicestatus->dwCurrentState            = 0 /*SERVICE_STOPPED*/;
    lpservicestatus->dwControlsAccepted        = 0;
    lpservicestatus->dwWin32ExitCode           = NO_ERROR;
    lpservicestatus->dwServiceSpecificExitCode = 0;
    lpservicestatus->dwCheckPoint              = 0;
    lpservicestatus->dwWaitHint                = 0;

    return TRUE;
}

