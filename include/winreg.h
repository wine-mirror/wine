/*
 * 				Shell Library definitions
 */
#ifndef __WINE_WINREG_H
#define __WINE_WINREG_H

#include "winbase.h"
#include "winnt.h"

/*
#define SHELL_ERROR_SUCCESS           0L
#define SHELL_ERROR_BADDB             1L
#define SHELL_ERROR_BADKEY            2L
#define SHELL_ERROR_CANTOPEN          3L
#define SHELL_ERROR_CANTREAD          4L
#define SHELL_ERROR_CANTWRITE         5L
#define SHELL_ERROR_OUTOFMEMORY       6L
#define SHELL_ERROR_INVALID_PARAMETER 7L
#define SHELL_ERROR_ACCESS_DENIED     8L
*/

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

#define HEX_REG_NONE                     0x80000000  
#define HEX_REG_SZ                       0x80000001  
#define HEX_REG_EXPAND_SZ                0x80000002  
#define HEX_REG_BINARY                   0x80000003  
#define HEX_REG_DWORD                    0x80000004  
#define HEX_REG_DWORD_LITTLE_ENDIAN      0x80000004  
#define HEX_REG_DWORD_BIG_ENDIAN         0x80000005  
#define HEX_REG_LINK                     0x80000006  
#define HEX_REG_MULTI_SZ                 0x80000007  
#define HEX_REG_RESOURCE_LIST            0x80000008  
#define HEX_REG_FULL_RESOURCE_DESCRIPTOR 0x80000009  

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

/* fixme: move it to basetsd.h */
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
/* end fixme */

/*
 *	registry provider structs
 */
typedef struct value_entA 
{   LPSTR	ve_valuename;
    DWORD	ve_valuelen;
    DWORD_PTR	ve_valueptr;
    DWORD	ve_type;
} VALENTA, *PVALENTA;

typedef struct value_entW {
    LPWSTR	ve_valuename;
    DWORD	ve_valuelen;
    DWORD_PTR	ve_valueptr;
    DWORD	ve_type;
} VALENTW, *PVALENTW;


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

HANDLE    WINAPI OpenSCManagerA(LPCSTR,LPCSTR,DWORD);
HANDLE    WINAPI OpenSCManagerW(LPCWSTR,LPCWSTR,DWORD);
#define     OpenSCManager WINELIB_NAME_AW(OpenSCManager)
HANDLE    WINAPI OpenServiceA(HANDLE,LPCSTR,DWORD);
HANDLE    WINAPI OpenServiceW(HANDLE,LPCWSTR,DWORD);
#define     OpenService WINELIB_NAME_AW(OpenService)
BOOL      WINAPI LookupPrivilegeValueA(LPCSTR,LPCSTR,LPVOID);
BOOL      WINAPI LookupPrivilegeValueW(LPCWSTR,LPCWSTR,LPVOID);
#define     LookupPrivilegeValue WINELIB_NAME_AW(LookupPrivilegeValue)
HANDLE    WINAPI RegisterEventSourceA(LPCSTR,LPCSTR);
HANDLE    WINAPI RegisterEventSourceW(LPCWSTR,LPCWSTR);
#define     RegisterEventSource WINELIB_NAME_AW(RegisterEventSource)
DWORD       WINAPI RegCreateKeyExA(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
DWORD       WINAPI RegCreateKeyExW(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
#define     RegCreateKeyEx WINELIB_NAME_AW(RegCreateKeyEx)
LONG        WINAPI RegSaveKeyA(HKEY,LPCSTR,LPSECURITY_ATTRIBUTES);
LONG        WINAPI RegSaveKeyW(HKEY,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     RegSaveKey WINELIB_NAME_AW(RegSaveKey)
LONG        WINAPI RegSetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
BOOL      WINAPI CloseServiceHandle(HANDLE);
BOOL      WINAPI ControlService(HANDLE,DWORD,LPSERVICE_STATUS);
BOOL      WINAPI DeleteService(HANDLE);
BOOL      WINAPI DeregisterEventSource(HANDLE);
BOOL      WINAPI GetFileSecurityA(LPCSTR,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,DWORD,LPDWORD);
BOOL      WINAPI GetFileSecurityW(LPCWSTR,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,DWORD,LPDWORD);
#define     GetFileSecurity WINELIB_NAME_AW(GetFileSecurity)
BOOL      WINAPI GetUserNameA(LPSTR,LPDWORD);
BOOL      WINAPI GetUserNameW(LPWSTR,LPDWORD);
#define     GetUserName WINELIB_NAME_AW(GetUserName)
BOOL      WINAPI OpenProcessToken(HANDLE,DWORD,HANDLE*);
LONG        WINAPI RegConnectRegistryA(LPCSTR,HKEY,LPHKEY);
LONG        WINAPI RegConnectRegistryW(LPCWSTR,HKEY,LPHKEY);
#define     RegConnectRegistry WINELIB_NAME_AW(RegConnectRegistry)
DWORD       WINAPI RegEnumKeyExA(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPSTR,
                                   LPDWORD,LPFILETIME);
DWORD       WINAPI RegEnumKeyExW(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPWSTR,
                                   LPDWORD,LPFILETIME);
#define     RegEnumKeyEx WINELIB_NAME_AW(RegEnumKeyEx)
LONG        WINAPI RegGetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,LPDWORD);
LONG        WINAPI RegLoadKeyA(HKEY,LPCSTR,LPCSTR);
LONG        WINAPI RegLoadKeyW(HKEY,LPCWSTR,LPCWSTR);
#define     RegLoadKey WINELIB_NAME_AW(RegLoadKey)
LONG        WINAPI RegNotifyChangeKeyValue(HKEY,BOOL,DWORD,HANDLE,BOOL);
DWORD       WINAPI RegOpenKeyExW(HKEY,LPCWSTR,DWORD,REGSAM,LPHKEY);
DWORD       WINAPI RegOpenKeyExA(HKEY,LPCSTR,DWORD,REGSAM,LPHKEY);
#define     RegOpenKeyEx WINELIB_NAME_AW(RegOpenKeyEx)
DWORD       WINAPI RegQueryInfoKeyW(HKEY,LPWSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
DWORD       WINAPI RegQueryInfoKeyA(HKEY,LPSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
#define     RegQueryInfoKey WINELIB_NAME_AW(RegQueryInfoKey)
LONG        WINAPI RegReplaceKeyA(HKEY,LPCSTR,LPCSTR,LPCSTR);
LONG        WINAPI RegReplaceKeyW(HKEY,LPCWSTR,LPCWSTR,LPCWSTR);
#define     RegReplaceKey WINELIB_NAME_AW(RegReplaceKey)
LONG        WINAPI RegRestoreKeyA(HKEY,LPCSTR,DWORD);
LONG        WINAPI RegRestoreKeyW(HKEY,LPCWSTR,DWORD);
#define     RegRestoreKey WINELIB_NAME_AW(RegRestoreKey)
LONG        WINAPI RegUnLoadKeyA(HKEY,LPCSTR);
LONG        WINAPI RegUnLoadKeyW(HKEY,LPCWSTR);
#define     RegUnLoadKey WINELIB_NAME_AW(RegUnLoadKey)
BOOL      WINAPI SetFileSecurityA(LPCSTR,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
BOOL      WINAPI SetFileSecurityW(LPCWSTR,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
#define     SetFileSecurity WINELIB_NAME_AW(SetFileSecurity)
BOOL      WINAPI StartServiceA(HANDLE,DWORD,LPCSTR*);
BOOL      WINAPI StartServiceW(HANDLE,DWORD,LPCWSTR*);
#define     StartService WINELIB_NAME_AW(StartService)

/* Declarations for functions that are the same in Win16 and Win32 */

DWORD       WINAPI RegCloseKey(HKEY);
DWORD       WINAPI RegFlushKey(HKEY);

DWORD       WINAPI RegCreateKeyA(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegCreateKeyW(HKEY,LPCWSTR,LPHKEY);
#define     RegCreateKey WINELIB_NAME_AW(RegCreateKey)
DWORD       WINAPI RegDeleteKeyA(HKEY,LPCSTR);
DWORD       WINAPI RegDeleteKeyW(HKEY,LPWSTR);
#define     RegDeleteKey WINELIB_NAME_AW(RegDeleteKey)
DWORD       WINAPI RegDeleteValueA(HKEY,LPSTR);
DWORD       WINAPI RegDeleteValueW(HKEY,LPWSTR);
#define     RegDeleteValue WINELIB_NAME_AW(RegDeleteValue)
DWORD       WINAPI RegEnumKeyA(HKEY,DWORD,LPSTR,DWORD);
DWORD       WINAPI RegEnumKeyW(HKEY,DWORD,LPWSTR,DWORD);
#define     RegEnumKey WINELIB_NAME_AW(RegEnumKey)
DWORD       WINAPI RegEnumValueA(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegEnumValueW(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegEnumValue WINELIB_NAME_AW(RegEnumValue)
DWORD       WINAPI RegOpenKeyA(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegOpenKeyW(HKEY,LPCWSTR,LPHKEY);
#define     RegOpenKey WINELIB_NAME_AW(RegOpenKey)
DWORD       WINAPI RegQueryValueA(HKEY,LPCSTR,LPSTR,LPLONG);
DWORD       WINAPI RegQueryValueW(HKEY,LPCWSTR,LPWSTR,LPLONG);
#define     RegQueryValue WINELIB_NAME_AW(RegQueryValue)
DWORD       WINAPI RegQueryValueExA(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegQueryValueExW(HKEY,LPWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegQueryValueEx WINELIB_NAME_AW(RegQueryValueEx)
DWORD       WINAPI RegSetValueA(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD       WINAPI RegSetValueW(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     RegSetValue WINELIB_NAME_AW(RegSetValue)
DWORD       WINAPI RegSetValueExA(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
DWORD       WINAPI RegSetValueExW(HKEY,LPWSTR,DWORD,DWORD,LPBYTE,DWORD);
#define     RegSetValueEx WINELIB_NAME_AW(RegSetValueEx)

#endif  /* __WINE_WINREG_H */
