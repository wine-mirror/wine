/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_WINSVC_H
#define __WINE_WINSVC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* Controls */
#define SERVICE_CONTROL_STOP                  0x00000001
#define SERVICE_CONTROL_PAUSE                 0x00000002
#define SERVICE_CONTROL_CONTINUE              0x00000003
#define SERVICE_CONTROL_INTERROGATE           0x00000004
#define SERVICE_CONTROL_SHUTDOWN              0x00000005
#define SERVICE_CONTROL_PARAMCHANGE           0x00000006
#define SERVICE_CONTROL_NETBINDADD            0x00000007
#define SERVICE_CONTROL_NETBINDREMOVE         0x00000008
#define SERVICE_CONTROL_NETBINDENABLE         0x00000009
#define SERVICE_CONTROL_NETBINDDISABLE        0x0000000A
#define SERVICE_CONTROL_DEVICEEVENT           0x0000000B
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE 0x0000000C
#define SERVICE_CONTROL_POWEREVENT            0x0000000D

/* Service State */
#define SERVICE_STOPPED          0x00000001
#define SERVICE_START_PENDING    0x00000002
#define SERVICE_STOP_PENDING     0x00000003
#define SERVICE_RUNNING          0x00000004
#define SERVICE_CONTINUE_PENDING 0x00000005
#define SERVICE_PAUSE_PENDING    0x00000006
#define SERVICE_PAUSED           0x00000007

/* Controls Accepted */
#define SERVICE_ACCEPT_STOP                  0x00000001
#define SERVICE_ACCEPT_PAUSE_CONTINUE        0x00000002
#define SERVICE_ACCEPT_SHUTDOWN              0x00000004
#define SERVICE_ACCEPT_PARAMCHANGE           0x00000008
#define SERVICE_ACCEPT_NETBINDCHANGE         0x00000010
#define SERVICE_ACCEPT_HARDWAREPROFILECHANGE 0x00000020
#define SERVICE_ACCEPT_POWEREVENT            0x00000040

/* Service Control Manager Object access types */
#define SC_MANAGER_CONNECT            0x0001
#define SC_MANAGER_CREATE_SERVICE     0x0002
#define SC_MANAGER_ENUMERATE_SERVICE  0x0004
#define SC_MANAGER_LOCK               0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS  0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG 0x0020
#define SC_MANAGER_ALL_ACCESS         ( STANDARD_RIGHTS_REQUIRED      | \
                                        SC_MANAGER_CONNECT            | \
                                        SC_MANAGER_CREATE_SERVICE     | \
                                        SC_MANAGER_ENUMERATE_SERVICE  | \
                                        SC_MANAGER_LOCK               | \
                                        SC_MANAGER_QUERY_LOCK_STATUS  | \
                                        SC_MANAGER_MODIFY_BOOT_CONFIG )

#define SERVICE_QUERY_CONFIG         0x0001
#define SERVICE_CHANGE_CONFIG        0x0002
#define SERVICE_QUERY_STATUS         0x0004
#define SERVICE_ENUMERATE_DEPENDENTS 0x0008
#define SERVICE_START                0x0010
#define SERVICE_STOP                 0x0020
#define SERVICE_PAUSE_CONTINUE       0x0040
#define SERVICE_INTERROGATE          0x0080
#define SERVICE_USER_DEFINED_CONTROL 0x0100

#define SERVICE_ALL_ACCESS           ( STANDARD_RIGHTS_REQUIRED     | \
                                       SERVICE_QUERY_CONFIG         | \
                                       SERVICE_CHANGE_CONFIG        | \
                                       SERVICE_QUERY_STATUS         | \
                                       SERVICE_ENUMERATE_DEPENDENTS | \
                                       SERVICE_START                | \
                                       SERVICE_STOP                 | \
                                       SERVICE_PAUSE_CONTINUE       | \
                                       SERVICE_INTERROGATE          | \
                                       SERVICE_USER_DEFINED_CONTROL )



/* Handle types */

typedef HANDLE SC_HANDLE, *LPSC_HANDLE;
typedef DWORD  SERVICE_STATUS_HANDLE;


/* Service status structure */

typedef struct _SERVICE_STATUS {
  DWORD dwServiceType;
  DWORD dwCurrentState;
  DWORD dwControlsAccepted;
  DWORD dwWin32ExitCode;
  DWORD dwServiceSpecificExitCode;
  DWORD dwCheckPoint;
  DWORD dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

typedef enum _SC_STATUS_TYPE {
  SC_STATUS_PROCESS_INFO      = 0
} SC_STATUS_TYPE;

/* Service main function prototype */

typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONA)(DWORD,LPSTR*);
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONW)(DWORD,LPWSTR*);
DECL_WINELIB_TYPE_AW(LPSERVICE_MAIN_FUNCTION)

/* Service start table */

typedef struct _SERVICE_TABLE_ENTRYA {
    LPSTR                    lpServiceName;
    LPSERVICE_MAIN_FUNCTIONA lpServiceProc;
} SERVICE_TABLE_ENTRYA, *LPSERVICE_TABLE_ENTRYA;

typedef struct _SERVICE_TABLE_ENTRYW {
  LPWSTR                   lpServiceName;
  LPSERVICE_MAIN_FUNCTIONW lpServiceProc;
} SERVICE_TABLE_ENTRYW, *LPSERVICE_TABLE_ENTRYW;

DECL_WINELIB_TYPE_AW(SERVICE_TABLE_ENTRY)
DECL_WINELIB_TYPE_AW(LPSERVICE_TABLE_ENTRY)

/* Service status enumeration structure */

typedef struct _ENUM_SERVICE_STATUSA {
  LPSTR          lpServiceName;
  LPSTR          lpDisplayName;
  SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSA, *LPENUM_SERVICE_STATUSA;

typedef struct _ENUM_SERVICE_STATUSW {
    LPWSTR         lpServiceName;
    LPWSTR         lpDisplayName;
    SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSW, *LPENUM_SERVICE_STATUSW;

DECL_WINELIB_TYPE_AW(ENUM_SERVICE_STATUS)
DECL_WINELIB_TYPE_AW(LPENUM_SERVICE_STATUS)

typedef struct _QUERY_SERVICE_CONFIGA {
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;
    LPSTR   lpBinaryPathName;
    LPSTR   lpLoadOrderGroup;
    DWORD   dwTagId;
    LPSTR   lpDependencies;
    LPSTR   lpServiceStartName;
    LPSTR   lpDisplayName;
} QUERY_SERVICE_CONFIGA, *LPQUERY_SERVICE_CONFIGA;

typedef struct _QUERY_SERVICE_CONFIGW {
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;
    LPWSTR  lpBinaryPathName;
    LPWSTR  lpLoadOrderGroup;
    DWORD   dwTagId;
    LPWSTR  lpDependencies;
    LPWSTR  lpServiceStartName;
    LPWSTR  lpDisplayName;
} QUERY_SERVICE_CONFIGW, *LPQUERY_SERVICE_CONFIGW;

/* Service control handler function prototype */

typedef VOID (WINAPI *LPHANDLER_FUNCTION)(DWORD);

/* API function prototypes */

BOOL        WINAPI CloseServiceHandle(SC_HANDLE);
BOOL        WINAPI ControlService(SC_HANDLE,DWORD,LPSERVICE_STATUS);
SC_HANDLE   WINAPI CreateServiceA(SC_HANDLE,LPCSTR,LPCSTR,DWORD,DWORD,DWORD,DWORD,LPCSTR,
                                  LPCSTR,LPDWORD,LPCSTR,LPCSTR,LPCSTR);
SC_HANDLE   WINAPI CreateServiceW(SC_HANDLE,LPCWSTR,LPCWSTR,DWORD,DWORD,DWORD,DWORD,LPCWSTR,
                                  LPCWSTR,LPDWORD,LPCWSTR,LPCWSTR,LPCWSTR);
#define     CreateService WINELIB_NAME_AW(CreateService)
BOOL        WINAPI DeleteService(SC_HANDLE);
BOOL        WINAPI EnumServicesStatusA(SC_HANDLE,DWORD,DWORD,LPENUM_SERVICE_STATUSA,
                                       DWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL        WINAPI EnumServicesStatusW(SC_HANDLE,DWORD,DWORD,LPENUM_SERVICE_STATUSW,
                                       DWORD,LPDWORD,LPDWORD,LPDWORD);
#define     EnumServicesStatus WINELIB_NAME_AW(EnumServicesStatus)
SC_HANDLE   WINAPI OpenSCManagerA(LPCSTR,LPCSTR,DWORD);
SC_HANDLE   WINAPI OpenSCManagerW(LPCWSTR,LPCWSTR,DWORD);
#define     OpenSCManager WINELIB_NAME_AW(OpenSCManager)
SC_HANDLE   WINAPI OpenServiceA(SC_HANDLE,LPCSTR,DWORD);
SC_HANDLE   WINAPI OpenServiceW(SC_HANDLE,LPCWSTR,DWORD);
#define     OpenService WINELIB_NAME_AW(OpenService)
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerA(LPCSTR,LPHANDLER_FUNCTION);
SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW(LPCWSTR,LPHANDLER_FUNCTION);
#define     RegisterServiceCtrlHandler WINELIB_NAME_AW(RegisterServiceCtrlHandler)
BOOL        WINAPI SetServiceStatus(SERVICE_STATUS_HANDLE,LPSERVICE_STATUS);
BOOL        WINAPI StartServiceA(SC_HANDLE,DWORD,LPCSTR*);
BOOL        WINAPI StartServiceW(SC_HANDLE,DWORD,LPCWSTR*);
#define     StartService WINELIB_NAME_AW(StartService)
BOOL        WINAPI StartServiceCtrlDispatcherA(LPSERVICE_TABLE_ENTRYA);
BOOL        WINAPI StartServiceCtrlDispatcherW(LPSERVICE_TABLE_ENTRYW);
#define     StartServiceCtrlDispatcher WINELIB_NAME_AW(StartServiceCtrlDispatcher)
BOOL        WINAPI QueryServiceStatus(SC_HANDLE,LPSERVICE_STATUS);
BOOL        WINAPI QueryServiceStatusEx(SC_HANDLE,SC_STATUS_TYPE,LPBYTE,DWORD,LPDWORD);
BOOL        WINAPI QueryServiceConfigA(SC_HANDLE,LPQUERY_SERVICE_CONFIGA,DWORD,LPDWORD);
BOOL        WINAPI QueryServiceConfigW(SC_HANDLE,LPQUERY_SERVICE_CONFIGW,DWORD,LPDWORD);
#define     QueryServiceConfig WINELIB_NAME_AW(QueryServiceConfig)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_WINSVC_H) */
