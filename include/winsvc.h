#ifndef __WINE_WINSVC_H
#define __WINE_WINSVC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include "windef.h"

/* Handle types */

typedef HANDLE SC_HANDLE, *LPSC_HANDLE;

typedef DWORD SERVICE_STATUS_HANDLE;

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

/* Service control handler function prototype */

typedef VOID (WINAPI *LPHANDLER_FUNCTION)(DWORD);

/* API function prototypes */

BOOL        WINAPI CloseServiceHandle(SC_HANDLE);
BOOL        WINAPI ControlService(SC_HANDLE,DWORD,LPSERVICE_STATUS);
SC_HANDLE   WINAPI CreateServiceA(DWORD,LPCSTR,LPCSTR,DWORD,DWORD,DWORD,DWORD,LPCSTR,
                                  LPCSTR,LPDWORD,LPCSTR,LPCSTR,LPCSTR);
SC_HANDLE   WINAPI CreateServiceW(DWORD,LPCWSTR,LPCWSTR,DWORD,DWORD,DWORD,DWORD,LPCWSTR,
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

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_WINSVC_H) */
