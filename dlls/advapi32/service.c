/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 */

#include "windows.h"
#include "winerror.h"
#include "debug.h"
#include "heap.h"
#include <time.h>

/******************************************************************************
 * EnumServicesStatus32A [ADVAPI32.38]
 */
BOOL32 WINAPI
EnumServicesStatus32A( HANDLE32 hSCManager, DWORD dwServiceType,
                       DWORD dwServiceState, LPVOID lpServices,
                       DWORD cbBufSize, LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned, LPDWORD lpResumeHandle )
{	FIXME (advapi,"%x type=%lx state=%lx %p %lx %p %p %p\n", hSCManager, 
		dwServiceType, dwServiceState, lpServices, cbBufSize,
		pcbBytesNeeded, lpServicesReturned,  lpResumeHandle);
	SetLastError (ERROR_ACCESS_DENIED);
	return 0;
}

/******************************************************************************
 * StartServiceCtrlDispatcher32A [ADVAPI32.196]
 */
BOOL32 WINAPI
StartServiceCtrlDispatcher32A( LPSERVICE_TABLE_ENTRY32A servent )
{	LPSERVICE_TABLE_ENTRY32A ptr = servent;

	while (ptr->lpServiceName)
	{ FIXME(advapi,"%s at %p\n", ptr->lpServiceName, ptr);
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
BOOL32 WINAPI
StartServiceCtrlDispatcher32W( LPSERVICE_TABLE_ENTRY32W servent )
{	LPSERVICE_TABLE_ENTRY32W ptr = servent;
	LPSERVICE_MAIN_FUNCTION32W fpMain;
	
	while (ptr->lpServiceName)
	{ FIXME(advapi,"%s at %p): STUB.\n", debugstr_w(ptr->lpServiceName),ptr);
	  fpMain = ptr->lpServiceProc;
	  fpMain(0,NULL);	/* try to start the service */
	  ptr++;
	}
	return TRUE;
}

/* FIXME: Where do these belong? */
typedef DWORD       SERVICE_STATUS_HANDLE; 
typedef VOID (WINAPI *LPHANDLER_FUNCTION)( DWORD    dwControl);

/*****************************************************************************
 * RegisterServiceCtrlHandlerA [ADVAPI32.176]
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerA( LPSTR lpServiceName,
                             LPHANDLER_FUNCTION lpfHandler )
{	FIXME(advapi,"%s %p\n", lpServiceName, lpfHandler);
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
RegisterServiceCtrlHandlerW( LPWSTR lpServiceName, 
                             LPHANDLER_FUNCTION lpfHandler )
{	FIXME(advapi,"%s %p\n", debugstr_w(lpServiceName), lpfHandler);
	return 0xcacacafe;	
}

/******************************************************************************
 * SetServiceStatus [ADVAPI32.192]
 *
 * PARAMS
 *   hService []
 *   lpStatus []
 */
BOOL32 WINAPI
SetServiceStatus( SERVICE_STATUS_HANDLE hService, LPSERVICE_STATUS lpStatus )
{	FIXME(advapi,"%lx %p\n",hService, lpStatus);
	TRACE(advapi,"\tType:%lx\n",lpStatus->dwServiceType);
	TRACE(advapi,"\tState:%lx\n",lpStatus->dwCurrentState);
	TRACE(advapi,"\tControlAccepted:%lx\n",lpStatus->dwControlsAccepted);
	TRACE(advapi,"\tExitCode:%lx\n",lpStatus->dwWin32ExitCode);
	TRACE(advapi,"\tServiceExitCode:%lx\n",lpStatus->dwServiceSpecificExitCode);
	TRACE(advapi,"\tCheckPoint:%lx\n",lpStatus->dwCheckPoint);
	TRACE(advapi,"\tWaitHint:%lx\n",lpStatus->dwWaitHint);
	return TRUE;
}

/******************************************************************************
 * OpenSCManager32A [ADVAPI32.110]
 */
HANDLE32 WINAPI
OpenSCManager32A( LPCSTR lpMachineName, LPCSTR lpDatabaseName,
                  DWORD dwDesiredAccess )
{   
    LPWSTR lpMachineNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpMachineName);
    LPWSTR lpDatabaseNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpDatabaseName);
    DWORD ret = OpenSCManager32W(lpMachineNameW,lpDatabaseNameW,
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
HANDLE32 WINAPI
OpenSCManager32W( LPCWSTR lpMachineName, LPCWSTR lpDatabaseName,
                  DWORD dwDesiredAccess )
{
    FIXME(advapi,"(%s,%s,0x%08lx): stub\n", debugstr_w(lpMachineName), 
          debugstr_w(lpDatabaseName), dwDesiredAccess);
    return 1;
}


/******************************************************************************
 * AllocateLocallyUniqueId [ADVAPI32.12]
 *
 * PARAMS
 *   lpluid []
 */
BOOL32 WINAPI
AllocateLocallyUniqueId( LPLUID lpluid )
{
	lpluid->LowPart = time(NULL);
	lpluid->HighPart = 0;
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
 * NOTES
 *    hService should be SC_HANDLE
 *
 * RETURNS STD
 */
BOOL32 WINAPI
ControlService( HANDLE32 hService, DWORD dwControl, 
                LPSERVICE_STATUS lpServiceStatus )
{
    FIXME(advapi, "(%d,%ld,%p): stub\n",hService,dwControl,lpServiceStatus);
    return TRUE;
}


/******************************************************************************
 * CloseServiceHandle [ADVAPI32.22]
 * Close handle to service or service control manager
 *
 * PARAMS
 *   hSCObject [I] Handle to service or service control manager database
 *
 * NOTES
 *   hSCObject should be SC_HANDLE
 *
 * RETURNS STD
 */
BOOL32 WINAPI
CloseServiceHandle( HANDLE32 hSCObject )
{
    FIXME(advapi, "(%d): stub\n", hSCObject);
    return TRUE;
}


/******************************************************************************
 * OpenService32A [ADVAPI32.112]
 */
HANDLE32 WINAPI
OpenService32A( HANDLE32 hSCManager, LPCSTR lpServiceName, 
                DWORD dwDesiredAccess )
{
    LPWSTR lpServiceNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpServiceName);
    DWORD ret = OpenService32W( hSCManager, lpServiceNameW, dwDesiredAccess);
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
 * NOTES
 *    The return value should be SC_HANDLE
 *    hSCManager should be SC_HANDLE
 *
 * RETURNS
 *    Success: Handle to the service
 *    Failure: NULL
 */
HANDLE32 WINAPI
OpenService32W(HANDLE32 hSCManager, LPCWSTR lpServiceName,
               DWORD dwDesiredAccess)
{
    FIXME(advapi, "(%d,%p,%ld): stub\n",hSCManager, lpServiceName,
          dwDesiredAccess);
    return 1;
}


/******************************************************************************
 * CreateService32A [ADVAPI32.29]
 */
DWORD WINAPI
CreateService32A( DWORD hSCManager, LPCSTR lpServiceName,
                  LPCSTR lpDisplayName, DWORD dwDesiredAccess, 
                  DWORD dwServiceType, DWORD dwStartType, 
                  DWORD dwErrorControl, LPCSTR lpBinaryPathName,
                  LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, 
                  LPCSTR lpDependencies, LPCSTR lpServiceStartName, 
                  LPCSTR lpPassword )
{
    FIXME(advapi, "(%ld,%s,%s,...): stub\n", 
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
 * NOTES
 *    hService should be SC_HANDLE
 */
BOOL32 WINAPI
DeleteService( HANDLE32 hService )
{
    FIXME(advapi, "(%d): stub\n",hService);
    return TRUE;
}


/******************************************************************************
 * StartService32A [ADVAPI32.195]
 *
 * NOTES
 *    How do we convert lpServiceArgVectors to use the 32W version?
 */
BOOL32 WINAPI
StartService32A( HANDLE32 hService, DWORD dwNumServiceArgs,
                 LPCSTR *lpServiceArgVectors )
{
    FIXME(advapi, "(%d,%ld,%p): stub\n",hService,dwNumServiceArgs,lpServiceArgVectors);
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
 * NOTES
 *   hService should be SC_HANDLE
 */
BOOL32 WINAPI
StartService32W( HANDLE32 hService, DWORD dwNumServiceArgs,
                 LPCWSTR *lpServiceArgVectors )
{
    FIXME(advapi, "(%d,%ld,%p): stub\n",hService,dwNumServiceArgs,
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
 * FIXME
 *   hService should be SC_HANDLE
 *   lpservicestatus should be LPSERVICE_STATUS
 */
BOOL32 WINAPI
QueryServiceStatus( HANDLE32 hService, LPVOID lpservicestatus )
{
	FIXME(advapi,"(%d,%p),stub!\n",hService,lpservicestatus);
	return TRUE;
}

