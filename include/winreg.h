/*
 * Win32 registry defines (see also winnt.h)
 *
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINREG_H
#define __WINE_WINREG_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define HKEY_CLASSES_ROOT       ((HKEY) 0x80000000)
#define HKEY_CURRENT_USER       ((HKEY) 0x80000001)
#define HKEY_LOCAL_MACHINE      ((HKEY) 0x80000002)
#define HKEY_USERS              ((HKEY) 0x80000003)
#define HKEY_PERFORMANCE_DATA   ((HKEY) 0x80000004)
#define HKEY_CURRENT_CONFIG     ((HKEY) 0x80000005)
#define HKEY_DYN_DATA           ((HKEY) 0x80000006)

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

typedef ACCESS_MASK REGSAM;
typedef LONG LSTATUS;

/*
 * InitiateSystemShutdown() reasons
 */
#include <reason.h>

#define REASON_OTHER            (SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER)
#define REASON_UNKNOWN          SHTDN_REASON_UNKNOWN
#define REASON_LEGACY_API       SHTDN_REASON_LEGACY_API
#define REASON_PLANNED_FLAG     SHTDN_REASON_FLAG_PLANNED

#define MAX_SHUTDOWN_TIMEOUT    (10*365*24*60*60)

/*
 * RegGetValue() restrictions
 */

#define RRF_RT_REG_NONE         (1 << 0)
#define RRF_RT_REG_SZ           (1 << 1)
#define RRF_RT_REG_EXPAND_SZ    (1 << 2)
#define RRF_RT_REG_BINARY       (1 << 3)
#define RRF_RT_REG_DWORD        (1 << 4)
#define RRF_RT_REG_MULTI_SZ     (1 << 5)
#define RRF_RT_REG_QWORD        (1 << 6)
#define RRF_RT_DWORD            (RRF_RT_REG_BINARY | RRF_RT_REG_DWORD)
#define RRF_RT_QWORD            (RRF_RT_REG_BINARY | RRF_RT_REG_QWORD)
#define RRF_RT_ANY              0xffff
#define RRF_NOEXPAND            (1 << 28)
#define RRF_ZEROONFAILURE       (1 << 29)

BOOL        WINAPI AbortSystemShutdownA(LPSTR);
BOOL        WINAPI AbortSystemShutdownW(LPWSTR);
#define     AbortSystemShutdown WINELIB_NAME_AW(AbortSystemShutdown)
BOOL        WINAPI InitiateSystemShutdownA(LPSTR,LPSTR,DWORD,BOOL,BOOL);
BOOL        WINAPI InitiateSystemShutdownW(LPWSTR,LPWSTR,DWORD,BOOL,BOOL);
#define     InitiateSystemShutdown WINELIB_NAME_AW(InitiateSystemShutdown);
BOOL        WINAPI InitiateSystemShutdownExA(LPSTR,LPSTR,DWORD,BOOL,BOOL,DWORD);
BOOL        WINAPI InitiateSystemShutdownExW(LPWSTR,LPWSTR,DWORD,BOOL,BOOL,DWORD);
#define     InitiateSystemShutdownEx WINELIB_NAME_AW(InitiateSystemShutdownEx);
LSTATUS     WINAPI RegCreateKeyExA(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,PHKEY,LPDWORD);
LSTATUS     WINAPI RegCreateKeyExW(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,PHKEY,LPDWORD);
#define     RegCreateKeyEx WINELIB_NAME_AW(RegCreateKeyEx)
LSTATUS     WINAPI RegDisablePredefinedCache(void);
LSTATUS     WINAPI RegSaveKeyA(HKEY,LPCSTR,LPSECURITY_ATTRIBUTES);
LSTATUS     WINAPI RegSaveKeyW(HKEY,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     RegSaveKey WINELIB_NAME_AW(RegSaveKey)
LSTATUS     WINAPI RegSetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
LSTATUS     WINAPI RegConnectRegistryA(LPCSTR,HKEY,PHKEY);
LSTATUS     WINAPI RegConnectRegistryW(LPCWSTR,HKEY,PHKEY);
#define     RegConnectRegistry WINELIB_NAME_AW(RegConnectRegistry)
LSTATUS     WINAPI RegEnumKeyExA(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPSTR,
                                   LPDWORD,LPFILETIME);
LSTATUS     WINAPI RegEnumKeyExW(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPWSTR,
                                   LPDWORD,LPFILETIME);
#define     RegEnumKeyEx WINELIB_NAME_AW(RegEnumKeyEx)
LSTATUS     WINAPI RegGetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,LPDWORD);
LSTATUS     WINAPI RegGetValueA(HKEY,LPCSTR,LPCSTR,DWORD,LPDWORD,PVOID,LPDWORD);
LSTATUS     WINAPI RegGetValueW(HKEY,LPCWSTR,LPCWSTR,DWORD,LPDWORD,PVOID,LPDWORD);
#define     RegGetValue WINELIB_NAME_AW(RegGetValue)
LSTATUS     WINAPI RegLoadKeyA(HKEY,LPCSTR,LPCSTR);
LSTATUS     WINAPI RegLoadKeyW(HKEY,LPCWSTR,LPCWSTR);
#define     RegLoadKey WINELIB_NAME_AW(RegLoadKey)
LSTATUS     WINAPI RegLoadMUIStringA(HKEY,LPCSTR,LPSTR,DWORD,LPDWORD,DWORD,LPCSTR);
LSTATUS     WINAPI RegLoadMUIStringW(HKEY,LPCWSTR,LPWSTR,DWORD,LPDWORD,DWORD,LPCWSTR);
#define     RegLoadMUIString WINELIB_NAME_AW(RegLoadMUIString)
LSTATUS     WINAPI RegNotifyChangeKeyValue(HKEY,BOOL,DWORD,HANDLE,BOOL);
LSTATUS     WINAPI RegOpenCurrentUser(REGSAM,PHKEY);
LSTATUS     WINAPI RegOpenKeyExW(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);
LSTATUS     WINAPI RegOpenKeyExA(HKEY,LPCSTR,DWORD,REGSAM,PHKEY);
#define     RegOpenKeyEx WINELIB_NAME_AW(RegOpenKeyEx)
LSTATUS     WINAPI RegOpenUserClassesRoot(HANDLE,DWORD,REGSAM,PHKEY);
LSTATUS     WINAPI RegQueryInfoKeyW(HKEY,LPWSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
LSTATUS     WINAPI RegQueryInfoKeyA(HKEY,LPSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
#define     RegQueryInfoKey WINELIB_NAME_AW(RegQueryInfoKey)
LSTATUS     WINAPI RegQueryMultipleValuesA(HKEY,PVALENTA,DWORD,LPSTR,LPDWORD);
LSTATUS     WINAPI RegQueryMultipleValuesW(HKEY,PVALENTW,DWORD,LPWSTR,LPDWORD);
#define     RegQueryMultipleValues WINELIB_NAME_AW(RegQueryMultipleValues)
LSTATUS     WINAPI RegReplaceKeyA(HKEY,LPCSTR,LPCSTR,LPCSTR);
LSTATUS     WINAPI RegReplaceKeyW(HKEY,LPCWSTR,LPCWSTR,LPCWSTR);
#define     RegReplaceKey WINELIB_NAME_AW(RegReplaceKey)
LSTATUS     WINAPI RegRestoreKeyA(HKEY,LPCSTR,DWORD);
LSTATUS     WINAPI RegRestoreKeyW(HKEY,LPCWSTR,DWORD);
#define     RegRestoreKey WINELIB_NAME_AW(RegRestoreKey)
LSTATUS     WINAPI RegUnLoadKeyA(HKEY,LPCSTR);
LSTATUS     WINAPI RegUnLoadKeyW(HKEY,LPCWSTR);
#define     RegUnLoadKey WINELIB_NAME_AW(RegUnLoadKey)

/* Declarations for functions that are the same in Win16 and Win32 */

LSTATUS     WINAPI RegCloseKey(HKEY);
LSTATUS     WINAPI RegFlushKey(HKEY);

LSTATUS     WINAPI RegCreateKeyA(HKEY,LPCSTR,PHKEY);
LSTATUS     WINAPI RegCreateKeyW(HKEY,LPCWSTR,PHKEY);
#define     RegCreateKey WINELIB_NAME_AW(RegCreateKey)
LSTATUS     WINAPI RegDeleteKeyA(HKEY,LPCSTR);
LSTATUS     WINAPI RegDeleteKeyW(HKEY,LPCWSTR);
#define     RegDeleteKey WINELIB_NAME_AW(RegDeleteKey)
LSTATUS     WINAPI RegDeleteKeyValueA(HKEY,LPCSTR,LPCSTR);
LSTATUS     WINAPI RegDeleteKeyValueW(HKEY,LPCWSTR,LPCWSTR);
#define     RegDeleteKeyValue WINELIB_NAME_AW(RegDeleteKeyValue)
LSTATUS     WINAPI RegDeleteTreeA(HKEY,LPCSTR);
LSTATUS     WINAPI RegDeleteTreeW(HKEY,LPCWSTR);
#define     RegDeleteTree WINELIB_NAME_AW(RegDeleteTree)
LSTATUS     WINAPI RegDeleteValueA(HKEY,LPCSTR);
LSTATUS     WINAPI RegDeleteValueW(HKEY,LPCWSTR);
#define     RegDeleteValue WINELIB_NAME_AW(RegDeleteValue)
LSTATUS     WINAPI RegEnumKeyA(HKEY,DWORD,LPSTR,DWORD);
LSTATUS     WINAPI RegEnumKeyW(HKEY,DWORD,LPWSTR,DWORD);
#define     RegEnumKey WINELIB_NAME_AW(RegEnumKey)
LSTATUS     WINAPI RegEnumValueA(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
LSTATUS     WINAPI RegEnumValueW(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegEnumValue WINELIB_NAME_AW(RegEnumValue)
LSTATUS     WINAPI RegOpenKeyA(HKEY,LPCSTR,PHKEY);
LSTATUS     WINAPI RegOpenKeyW(HKEY,LPCWSTR,PHKEY);
#define     RegOpenKey WINELIB_NAME_AW(RegOpenKey)
LSTATUS     WINAPI RegQueryValueA(HKEY,LPCSTR,LPSTR,LPLONG);
LSTATUS     WINAPI RegQueryValueW(HKEY,LPCWSTR,LPWSTR,LPLONG);
#define     RegQueryValue WINELIB_NAME_AW(RegQueryValue)
LSTATUS     WINAPI RegQueryValueExA(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
LSTATUS     WINAPI RegQueryValueExW(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegQueryValueEx WINELIB_NAME_AW(RegQueryValueEx)
LSTATUS     WINAPI RegSetValueA(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
LSTATUS     WINAPI RegSetValueW(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     RegSetValue WINELIB_NAME_AW(RegSetValue)
LSTATUS     WINAPI RegSetValueExA(HKEY,LPCSTR,DWORD,DWORD,CONST BYTE*,DWORD);
LSTATUS     WINAPI RegSetValueExW(HKEY,LPCWSTR,DWORD,DWORD,CONST BYTE*,DWORD);
#define     RegSetValueEx WINELIB_NAME_AW(RegSetValueEx)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_WINREG_H */
