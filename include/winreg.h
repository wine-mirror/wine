/*
 * 				Shell Library definitions
 */
#ifndef __WINE_WINREG_H
#define __WINE_WINREG_H

#include "winbase.h"

/* FIXME: should be in security.h or whereever */
#ifndef READ_CONTROL
#define READ_CONTROL		0x00020000
#endif
#ifndef STANDARD_RIGHTS_READ
#define STANDARD_RIGHTS_READ	READ_CONTROL
#endif
#ifndef STANDARD_RIGHTS_WRITE
#define STANDARD_RIGHTS_WRITE	READ_CONTROL	/* FIXME: hmm? */
#endif
#ifndef STANDARD_RIGHTS_ALL
#define STANDARD_RIGHTS_ALL	0x001f0000
#endif
/* ... */


#define SHELL_ERROR_SUCCESS           0L
#define SHELL_ERROR_BADDB             1L
#define SHELL_ERROR_BADKEY            2L
#define SHELL_ERROR_CANTOPEN          3L
#define SHELL_ERROR_CANTREAD          4L
#define SHELL_ERROR_CANTWRITE         5L
#define SHELL_ERROR_OUTOFMEMORY       6L
#define SHELL_ERROR_INVALID_PARAMETER 7L
#define SHELL_ERROR_ACCESS_DENIED     8L

#define REG_NONE		0	/* no type */
#define REG_SZ                  1	/* string type (ASCII) */
#define REG_EXPAND_SZ		2	/* string, includes %ENVVAR% (expanded by caller) (ASCII) */
#define REG_BINARY		3	/* binary format, callerspecific */
/* YES, REG_DWORD == REG_DWORD_LITTLE_ENDIAN */
#define REG_DWORD		4	/* DWORD in little endian format */
#define REG_DWORD_LITTLE_ENDIAN	4	/* DWORD in little endian format */
#define REG_DWORD_BIG_ENDIAN	5	/* DWORD in big endian format  */
#define REG_LINK		6	/* symbolic link (UNICODE) */
#define REG_MULTI_SZ		7	/* multiple strings, delimited by \0, terminated by \0\0 (ASCII) */
#define REG_RESOURCE_LIST	8	/* resource list? huh? */
#define REG_FULL_RESOURCE_DESCRIPTOR	9	/* full resource descriptor? huh? */

#define HKEY_CLASSES_ROOT	0x80000000
#define HKEY_CURRENT_USER	0x80000001
#define HKEY_LOCAL_MACHINE	0x80000002
#define HKEY_USERS		0x80000003
#define HKEY_PERFORMANCE_DATA	0x80000004
#define HKEY_CURRENT_CONFIG	0x80000005
#define HKEY_DYN_DATA		0x80000006

#define	REG_OPTION_RESERVED		0x00000000
#define	REG_OPTION_NON_VOLATILE		0x00000000
#define	REG_OPTION_VOLATILE		0x00000001
#define	REG_OPTION_CREATE_LINK		0x00000002
#define	REG_OPTION_BACKUP_RESTORE	0x00000004 /* FIXME */
#define	REG_OPTION_TAINTED		0x80000000 /* Internal? */

#define REG_CREATED_NEW_KEY	0x00000001
#define REG_OPENED_EXISTING_KEY	0x00000002

/* For RegNotifyChangeKeyValue */
#define REG_NOTIFY_CHANGE_NAME	0x1

#define KEY_QUERY_VALUE         0x00000001
#define KEY_SET_VALUE           0x00000002
#define KEY_CREATE_SUB_KEY      0x00000004
#define KEY_ENUMERATE_SUB_KEYS  0x00000008
#define KEY_NOTIFY              0x00000010
#define KEY_CREATE_LINK         0x00000020

#define KEY_READ                (STANDARD_RIGHTS_READ|	\
				 KEY_QUERY_VALUE|	\
				 KEY_ENUMERATE_SUB_KEYS|\
				 KEY_NOTIFY		\
				)
#define KEY_WRITE               (STANDARD_RIGHTS_WRITE|	\
				 KEY_SET_VALUE|		\
				 KEY_CREATE_SUB_KEY	\
				)
#define KEY_EXECUTE             KEY_READ
#define KEY_ALL_ACCESS          (STANDARD_RIGHTS_ALL|	\
				 KEY_READ|KEY_WRITE|	\
				 KEY_CREATE_LINK	\
				)

/* Used by: ControlService */
typedef struct _SERVICE_STATUS {
    DWORD dwServiceType;
    DWORD dwCurrentState;
    DWORD dwControlsAccepted;
    DWORD dwWin32ExitCode;
    DWORD dwServiceSpecificExitCode;
    DWORD dwCheckPoint;
    DWORD dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

HANDLE32    WINAPI OpenSCManager32A(LPCSTR,LPCSTR,DWORD);
HANDLE32    WINAPI OpenSCManager32W(LPCWSTR,LPCWSTR,DWORD);
#define     OpenSCManager WINELIB_NAME_AW(OpenSCManager)
HANDLE32    WINAPI OpenService32A(HANDLE32,LPCSTR,DWORD);
HANDLE32    WINAPI OpenService32W(HANDLE32,LPCWSTR,DWORD);
#define     OpenService WINELIB_NAME_AW(OpenService)
BOOL32      WINAPI LookupPrivilegeValue32A(LPCSTR,LPCSTR,LPVOID);
BOOL32      WINAPI LookupPrivilegeValue32W(LPCWSTR,LPCWSTR,LPVOID);
#define     LookupPrivilegeValue WINELIB_NAME_AW(LookupPrivilegeValue)
HANDLE32    WINAPI RegisterEventSource32A(LPCSTR,LPCSTR);
HANDLE32    WINAPI RegisterEventSource32W(LPCWSTR,LPCWSTR);
#define     RegisterEventSource WINELIB_NAME_AW(RegisterEventSource)
DWORD       WINAPI RegCreateKeyEx32A(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
DWORD       WINAPI RegCreateKeyEx32W(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
#define     RegCreateKeyEx WINELIB_NAME_AW(RegCreateKeyEx)
LONG        WINAPI RegSaveKey32A(HKEY,LPCSTR,LPSECURITY_ATTRIBUTES);
LONG        WINAPI RegSaveKey32W(HKEY,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     RegSaveKey WINELIB_NAME_AW(RegSaveKey)
LONG        WINAPI RegSetKeySecurity(HKEY,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR);
BOOL32      WINAPI CloseServiceHandle(HANDLE32);
BOOL32      WINAPI ControlService(HANDLE32,DWORD,LPSERVICE_STATUS);
BOOL32      WINAPI DeleteService(HANDLE32);
BOOL32      WINAPI DeregisterEventSource(HANDLE32);
BOOL32      WINAPI GetFileSecurity32A(LPCSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR,DWORD,LPDWORD);
BOOL32      WINAPI GetFileSecurity32W(LPCWSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR,DWORD,LPDWORD);
#define     GetFileSecurity WINELIB_NAME_AW(GetFileSecurity)
BOOL32      WINAPI GetUserName32A(LPSTR,LPDWORD);
BOOL32      WINAPI GetUserName32W(LPWSTR,LPDWORD);
#define     GetUserName WINELIB_NAME_AW(GetUserName)
BOOL32      WINAPI OpenProcessToken(HANDLE32,DWORD,HANDLE32*);
LONG        WINAPI RegConnectRegistry32A(LPCSTR,HKEY,LPHKEY);
LONG        WINAPI RegConnectRegistry32W(LPCWSTR,HKEY,LPHKEY);
#define     RegConnectRegistry WINELIB_NAME_AW(RegConnectRegistry)
DWORD       WINAPI RegEnumKeyEx32A(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPSTR,
                                   LPDWORD,LPFILETIME);
DWORD       WINAPI RegEnumKeyEx32W(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPWSTR,
                                   LPDWORD,LPFILETIME);
#define     RegEnumKeyEx WINELIB_NAME_AW(RegEnumKeyEx)
LONG        WINAPI RegGetKeySecurity(HKEY,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR,LPDWORD);
LONG        WINAPI RegLoadKey32A(HKEY,LPCSTR,LPCSTR);
LONG        WINAPI RegLoadKey32W(HKEY,LPCWSTR,LPCWSTR);
#define     RegLoadKey WINELIB_NAME_AW(RegLoadKey)
LONG        WINAPI RegNotifyChangeKeyValue(HKEY,BOOL32,DWORD,HANDLE32,BOOL32);
DWORD       WINAPI RegOpenKeyEx32W(HKEY,LPCWSTR,DWORD,REGSAM,LPHKEY);
DWORD       WINAPI RegOpenKeyEx32A(HKEY,LPCSTR,DWORD,REGSAM,LPHKEY);
#define     RegOpenKeyEx WINELIB_NAME_AW(RegOpenKeyEx)
DWORD       WINAPI RegQueryInfoKey32W(HKEY,LPWSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
DWORD       WINAPI RegQueryInfoKey32A(HKEY,LPSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
#define     RegQueryInfoKey WINELIB_NAME_AW(RegQueryInfoKey)
LONG        WINAPI RegReplaceKey32A(HKEY,LPCSTR,LPCSTR,LPCSTR);
LONG        WINAPI RegReplaceKey32W(HKEY,LPCWSTR,LPCWSTR,LPCWSTR);
#define     RegReplaceKey WINELIB_NAME_AW(RegReplaceKey)
LONG        WINAPI RegRestoreKey32A(HKEY,LPCSTR,DWORD);
LONG        WINAPI RegRestoreKey32W(HKEY,LPCWSTR,DWORD);
#define     RegRestoreKey WINELIB_NAME_AW(RegRestoreKey)
LONG        WINAPI RegUnLoadKey32A(HKEY,LPCSTR);
LONG        WINAPI RegUnLoadKey32W(HKEY,LPCWSTR);
#define     RegUnLoadKey WINELIB_NAME_AW(RegUnLoadKey)
BOOL32      WINAPI SetFileSecurity32A(LPCSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR);
BOOL32      WINAPI SetFileSecurity32W(LPCWSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR);
#define     SetFileSecurity WINELIB_NAME_AW(SetFileSecurity)
BOOL32      WINAPI StartService32A(HANDLE32,DWORD,LPCSTR*);
BOOL32      WINAPI StartService32W(HANDLE32,DWORD,LPCWSTR*);
#define     StartService WINELIB_NAME_AW(StartService)

/* Declarations for functions that are the same in Win16 and Win32 */

DWORD       WINAPI RegCloseKey(HKEY);
DWORD       WINAPI RegFlushKey(HKEY);

DWORD       WINAPI RegCreateKey32A(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegCreateKey32W(HKEY,LPCWSTR,LPHKEY);
#define     RegCreateKey WINELIB_NAME_AW(RegCreateKey)
DWORD       WINAPI RegDeleteKey32A(HKEY,LPCSTR);
DWORD       WINAPI RegDeleteKey32W(HKEY,LPWSTR);
#define     RegDeleteKey WINELIB_NAME_AW(RegDeleteKey)
DWORD       WINAPI RegDeleteValue32A(HKEY,LPSTR);
DWORD       WINAPI RegDeleteValue32W(HKEY,LPWSTR);
#define     RegDeleteValue WINELIB_NAME_AW(RegDeleteValue)
DWORD       WINAPI RegEnumKey32A(HKEY,DWORD,LPSTR,DWORD);
DWORD       WINAPI RegEnumKey32W(HKEY,DWORD,LPWSTR,DWORD);
#define     RegEnumKey WINELIB_NAME_AW(RegEnumKey)
DWORD       WINAPI RegEnumValue32A(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegEnumValue32W(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegEnumValue WINELIB_NAME_AW(RegEnumValue)
DWORD       WINAPI RegOpenKey32A(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegOpenKey32W(HKEY,LPCWSTR,LPHKEY);
#define     RegOpenKey WINELIB_NAME_AW(RegOpenKey)
DWORD       WINAPI RegQueryValue32A(HKEY,LPSTR,LPSTR,LPDWORD);
DWORD       WINAPI RegQueryValue32W(HKEY,LPWSTR,LPWSTR,LPDWORD);
#define     RegQueryValue WINELIB_NAME_AW(RegQueryValue)
DWORD       WINAPI RegQueryValueEx32A(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegQueryValueEx32W(HKEY,LPWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegQueryValueEx WINELIB_NAME_AW(RegQueryValueEx)
DWORD       WINAPI RegSetValue32A(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD       WINAPI RegSetValue32W(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     RegSetValue WINELIB_NAME_AW(RegSetValue)
DWORD       WINAPI RegSetValueEx32A(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
DWORD       WINAPI RegSetValueEx32W(HKEY,LPWSTR,DWORD,DWORD,LPBYTE,DWORD);
#define     RegSetValueEx WINELIB_NAME_AW(RegSetValueEx)

void SHELL_Init(void);
void SHELL_SaveRegistry(void);
void SHELL_LoadRegistry(void);



#endif  /* __WINE_WINREG_H */
