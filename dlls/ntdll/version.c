/*
 * Windows and DOS version functions
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Patrik Stridvall
 * Copyright 1998, 2003 Andreas Mohr
 * Copyright 1997, 2003 Alexandre Julliard
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "ddk/wdm.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);

typedef enum
{
    WIN20,   /* Windows 2.0 */
    WIN30,   /* Windows 3.0 */
    WIN31,   /* Windows 3.1 */
    WIN95,   /* Windows 95 */
    WIN98,   /* Windows 98 */
    WINME,   /* Windows Me */
    NT351,   /* Windows NT 3.51 */
    NT40,    /* Windows NT 4.0 */
    NT2K,    /* Windows 2000 */
    WINXP,   /* Windows XP */
    WINXP64, /* Windows XP 64-bit */
    WIN2K3,  /* Windows 2003 */
    WINVISTA,/* Windows Vista */
    WIN2K8,  /* Windows 2008 */
    WIN2K8R2,/* Windows 2008 R2 */
    WIN7,    /* Windows 7 */
    WIN8,    /* Windows 8 */
    WIN81,   /* Windows 8.1 */
    WIN10,   /* Windows 10 */
    WIN11,   /* Windows 11 */
    NB_WINDOWS_VERSIONS
} WINDOWS_VERSION;

/* FIXME: compare values below with original and fix.
 * An *excellent* win9x version page (ALL versions !)
 * can be found at www.mdgx.com/ver.htm */
static const RTL_OSVERSIONINFOEXW VersionData[NB_WINDOWS_VERSIONS] =
{
    /* WIN20 FIXME: verify values */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 2, 0, 0, VER_PLATFORM_WIN32s,
        L"Win32s 1.3", 0, 0, 0, 0, 0
    },
    /* WIN30 FIXME: verify values */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 3, 0, 0, VER_PLATFORM_WIN32s,
        L"Win32s 1.3", 0, 0, 0, 0, 0
    },
    /* WIN31 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 3, 10, 0, VER_PLATFORM_WIN32s,
        L"Win32s 1.3", 0, 0, 0, 0, 0
    },
    /* WIN95 */
    {
        /* Win95:       4, 0, 0x40003B6, ""
         * Win95sp1:    4, 0, 0x40003B6, " A " (according to doc)
         * Win95osr2:   4, 0, 0x4000457, " B " (according to doc)
         * Win95osr2.1: 4, 3, 0x40304BC, " B " (according to doc)
         * Win95osr2.5: 4, 3, 0x40304BE, " C " (according to doc)
         * Win95a/b can be discerned via regkey SubVersionNumber
         */
        sizeof(RTL_OSVERSIONINFOEXW), 4, 0, 0x40003B6, VER_PLATFORM_WIN32_WINDOWS,
        L"", 0, 0, 0, 0, 0
    },
    /* WIN98 (second edition) */
    {
        /* Win98:   4, 10, 0x40A07CE, " "   4.10.1998
         * Win98SE: 4, 10, 0x40A08AE, " A " 4.10.2222
         */
        sizeof(RTL_OSVERSIONINFOEXW), 4, 10, 0x40A08AE, VER_PLATFORM_WIN32_WINDOWS,
        L" A ", 0, 0, 0, 0, 0
    },
    /* WINME */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 4, 90, 0x45A0BB8, VER_PLATFORM_WIN32_WINDOWS,
        L" ", 0, 0, 0, 0, 0
    },
    /* NT351 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 3, 51, 1057, VER_PLATFORM_WIN32_NT,
        L"Service Pack 5", 5, 0, 0, VER_NT_WORKSTATION, 0
    },
    /* NT40 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 4, 0, 1381, VER_PLATFORM_WIN32_NT,
        L"Service Pack 6a", 6, 0, 0, VER_NT_WORKSTATION, 0
    },
    /* NT2K */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 0, 2195, VER_PLATFORM_WIN32_NT,
        L"Service Pack 4", 4, 0, 0, VER_NT_WORKSTATION,
        30 /* FIXME: Great, a reserved field with a value! */
    },
    /* WINXP */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 1, 2600, VER_PLATFORM_WIN32_NT,
        L"Service Pack 3", 3, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION,
        30 /* FIXME: Great, a reserved field with a value! */
    },
    /* WINXP64 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 2, 3790, VER_PLATFORM_WIN32_NT,
        L"Service Pack 2", 2, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
    /* WIN2K3 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 2, 3790, VER_PLATFORM_WIN32_NT,
        L"Service Pack 2", 2, 0, VER_SUITE_SINGLEUSERTS, VER_NT_SERVER, 0
    },
    /* WINVISTA */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 0, 6002, VER_PLATFORM_WIN32_NT,
        L"Service Pack 2", 2, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
    /* WIN2K8 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 0, 6002, VER_PLATFORM_WIN32_NT,
        L"Service Pack 2", 2, 0, VER_SUITE_SINGLEUSERTS, VER_NT_SERVER, 0
    },
    /* WIN7 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 1, 7601, VER_PLATFORM_WIN32_NT,
        L"Service Pack 1", 1, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
    /* WIN2K8R2 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 1, 7601, VER_PLATFORM_WIN32_NT,
        L"Service Pack 1", 1, 0, VER_SUITE_SINGLEUSERTS, VER_NT_SERVER, 0
    },
    /* WIN8 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 2, 9200, VER_PLATFORM_WIN32_NT,
        L"", 0, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
    /* WIN81 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 3, 9600, VER_PLATFORM_WIN32_NT,
        L"", 0, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
    /* WIN10 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 10, 0, 19043, VER_PLATFORM_WIN32_NT,
        L"", 0, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
    /* WIN11 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 10, 0, 22000, VER_PLATFORM_WIN32_NT,
        L"", 0, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    },
};

static const struct { WCHAR name[12]; WINDOWS_VERSION ver; } version_names[] =
{
    { L"win20", WIN20 },
    { L"win30", WIN30 },
    { L"win31", WIN31 },
    { L"win95", WIN95 },
    { L"win98", WIN98 },
    { L"winme", WINME },
    { L"nt351", NT351 },
    { L"nt40", NT40 },
    { L"win2000", NT2K },
    { L"win2k", NT2K },
    { L"nt2k", NT2K },
    { L"nt2000", NT2K },
    { L"winxp", WINXP },
    { L"winxp64", WINXP64 },
    { L"win2003", WIN2K3 },
    { L"win2k3", WIN2K3 },
    { L"vista", WINVISTA },
    { L"winvista", WINVISTA },
    { L"win2008", WIN2K8 },
    { L"win2k8", WIN2K8 },
    { L"win2008r2", WIN2K8R2 },
    { L"win2k8r2", WIN2K8R2 },
    { L"win7", WIN7 },
    { L"win8", WIN8 },
    { L"win81", WIN81 },
    { L"win10", WIN10 },
    { L"win11", WIN11 },
};


/* initialized to null so that we crash if we try to retrieve the version too early at startup */
static const RTL_OSVERSIONINFOEXW *current_version;

static char wine_version[256];

/*********************************************************************
 *                  wine_get_version
 */
const char * CDECL wine_get_version(void)
{
    return wine_version;
}


/*********************************************************************
 *                  wine_get_build_id
 */
const char * CDECL wine_get_build_id(void)
{
    const char *p = wine_version;
    p += strlen(p) + 1;  /* skip version */
    return p;
}


/*********************************************************************
 *                  wine_get_host_version
 */
void CDECL wine_get_host_version( const char **sysname, const char **release )
{
    const char *p = wine_version;
    p += strlen(p) + 1;  /* skip version */
    p += strlen(p) + 1;  /* skip build id */
    if (sysname) *sysname = p;
    p += strlen(p) + 1;
    if (release) *release = p;
}


/**********************************************************************
 *         get_nt_registry_version
 *
 * Fetch the version information from the NT-style registry keys.
 */
static BOOL get_nt_registry_version( RTL_OSVERSIONINFOEXW *version )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    HANDLE hkey, hkey2;
    char tmp[64];
    DWORD count;
    BOOL ret = FALSE;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;

    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion" );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return FALSE;

    memset( version, 0, sizeof(*version) );

    RtlInitUnicodeString( &valueW, L"CurrentMajorVersionNumber" );
    if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ) &&
        info->Type == REG_DWORD)
    {
        version->dwMajorVersion = *(DWORD *)info->Data;

        RtlInitUnicodeString( &valueW, L"CurrentMinorVersionNumber" );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ) &&
            info->Type == REG_DWORD)
        {
            version->dwMinorVersion = *(DWORD *)info->Data;
        }
        else version->dwMajorVersion = 0;
    }

    if (!version->dwMajorVersion)
    {
        RtlInitUnicodeString( &valueW, L"CurrentVersion" );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
        {
            WCHAR *p, *str = (WCHAR *)info->Data;
            str[info->DataLength / sizeof(WCHAR)] = 0;
            p = wcschr( str, '.' );
            if (p)
            {
                *p++ = 0;
                version->dwMinorVersion = wcstoul( p, NULL, 10 );
            }
            version->dwMajorVersion = wcstoul( str, NULL, 10 );
        }
    }

    if (version->dwMajorVersion)   /* we got the main version, now fetch the other fields */
    {
        ret = TRUE;
        version->dwPlatformId = VER_PLATFORM_WIN32_NT;

        /* get build number */

        RtlInitUnicodeString( &valueW, L"CurrentBuildNumber" );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
        {
            WCHAR *str = (WCHAR *)info->Data;
            str[info->DataLength / sizeof(WCHAR)] = 0;
            version->dwBuildNumber = wcstoul( str, NULL, 10 );
        }

        /* get version description */

        RtlInitUnicodeString( &valueW, L"CSDVersion" );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
        {
            DWORD len = min( info->DataLength, sizeof(version->szCSDVersion) - sizeof(WCHAR) );
            memcpy( version->szCSDVersion, info->Data, len );
            version->szCSDVersion[len / sizeof(WCHAR)] = 0;
        }

        /* get service pack version */

        RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Windows" );
        if (!NtOpenKey( &hkey2, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &valueW, L"CSDVersion" );
            if (!NtQueryValueKey( hkey2, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp), &count ))
            {
                if (info->DataLength >= sizeof(DWORD))
                {
                    DWORD dw = *(DWORD *)info->Data;
                    version->wServicePackMajor = LOWORD(dw) >> 8;
                    version->wServicePackMinor = LOWORD(dw) & 0xff;
                }
            }
            NtClose( hkey2 );
        }

        /* get product type */

        RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ProductOptions" );
        if (!NtOpenKey( &hkey2, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &valueW, L"ProductType" );
            if (!NtQueryValueKey( hkey2, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
            {
                WCHAR *str = (WCHAR *)info->Data;
                str[info->DataLength / sizeof(WCHAR)] = 0;
                if (!wcsicmp( str, L"WinNT" )) version->wProductType = VER_NT_WORKSTATION;
                else if (!wcsicmp( str, L"LanmanNT" )) version->wProductType = VER_NT_DOMAIN_CONTROLLER;
                else if (!wcsicmp( str, L"ServerNT" )) version->wProductType = VER_NT_SERVER;
            }
            NtClose( hkey2 );
        }

        /* FIXME: get wSuiteMask */
    }

    NtClose( hkey );
    return ret;
}


/**********************************************************************
 *         get_win9x_registry_version
 *
 * Fetch the version information from the Win9x-style registry keys.
 */
static BOOL get_win9x_registry_version( RTL_OSVERSIONINFOEXW *version )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    HANDLE hkey;
    char tmp[64];
    DWORD count;
    BOOL ret = FALSE;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;

    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion" );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return FALSE;

    memset( version, 0, sizeof(*version) );

    RtlInitUnicodeString( &valueW, L"VersionNumber" );
    if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
    {
        WCHAR *p, *str = (WCHAR *)info->Data;
        str[info->DataLength / sizeof(WCHAR)] = 0;
        p = wcschr( str, '.' );
        if (p) *p++ = 0;
        version->dwMajorVersion = wcstoul( str, NULL, 10 );
        if (p)
        {
            str = p;
            p = wcschr( str, '.' );
            if (p)
            {
                *p++ = 0;
                version->dwBuildNumber = wcstoul( p, NULL, 10 );
            }
            version->dwMinorVersion = wcstoul( str, NULL, 10 );
        }
        /* build number contains version too on Win9x */
        version->dwBuildNumber |= MAKEWORD( version->dwMinorVersion, version->dwMajorVersion ) << 16;
    }

    if (version->dwMajorVersion)   /* we got the main version, now fetch the other fields */
    {
        ret = TRUE;
        version->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;

        RtlInitUnicodeString( &valueW, L"SubVersionNumber" );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
        {
            DWORD len = min( info->DataLength, sizeof(version->szCSDVersion) - sizeof(WCHAR) );
            memcpy( version->szCSDVersion, info->Data, len );
            version->szCSDVersion[len / sizeof(WCHAR)] = 0;
        }
    }

    NtClose( hkey );
    return ret;
}


/**********************************************************************
 *         parse_win_version
 *
 * Parse the contents of the Version key.
 */
static BOOL parse_win_version( HANDLE hkey )
{
    UNICODE_STRING valueW;
    WCHAR *name, tmp[64];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;
    DWORD i, count;

    RtlInitUnicodeString( &valueW, L"Version" );
    if (NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp) - sizeof(WCHAR), &count ))
        return FALSE;

    name = (WCHAR *)info->Data;
    name[info->DataLength / sizeof(WCHAR)] = 0;

    for (i = 0; i < ARRAY_SIZE(version_names); i++)
    {
        if (wcscmp( version_names[i].name, name )) continue;
        current_version = &VersionData[version_names[i].ver];
        TRACE( "got win version %s\n", debugstr_w(version_names[i].name) );
        return TRUE;
    }

    ERR( "Invalid Windows version value %s specified in config file.\n", debugstr_w(name) );
    return FALSE;
}


/**********************************************************************
 *         version_init
 */
void version_init(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE root, hkey, config_key;
    BOOL got_win_ver = FALSE;
    const WCHAR *p, *appname = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
    WCHAR appversion[MAX_PATH+20];

    NtQuerySystemInformation( SystemWineVersionInformation, wine_version, sizeof(wine_version), NULL );

    current_version = &VersionData[WIN10];

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, root, NULL );
    RtlInitUnicodeString( &nameW, L"Software\\Wine" );

    /* @@ Wine registry key: HKCU\Software\Wine */
    if (NtOpenKey( &config_key, KEY_ALL_ACCESS, &attr )) config_key = 0;
    NtClose( root );
    if (!config_key) goto done;

    /* open AppDefaults\\appname key */

    if ((p = wcsrchr( appname, '/' ))) appname = p + 1;
    if ((p = wcsrchr( appname, '\\' ))) appname = p + 1;

    wcscpy( appversion, L"AppDefaults\\" );
    wcscat( appversion, appname );
    RtlInitUnicodeString( &nameW, appversion );
    attr.RootDirectory = config_key;

    /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe */
    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        TRACE( "getting version from %s\n", debugstr_w(appversion) );
        got_win_ver = parse_win_version( hkey );
        NtClose( hkey );
    }

    if (!got_win_ver)
    {
        TRACE( "getting default version\n" );
        got_win_ver = parse_win_version( config_key );
    }
    NtClose( config_key );

done:
    if (!got_win_ver)
    {
        static RTL_OSVERSIONINFOEXW registry_version;

        TRACE( "getting registry version\n" );
        if (get_nt_registry_version( &registry_version ) ||
            get_win9x_registry_version( &registry_version ))
            current_version = &registry_version;
    }


    NtCurrentTeb()->Peb->OSMajorVersion = current_version->dwMajorVersion;
    NtCurrentTeb()->Peb->OSMinorVersion = current_version->dwMinorVersion;
    NtCurrentTeb()->Peb->OSBuildNumber  = current_version->dwBuildNumber;
    NtCurrentTeb()->Peb->OSPlatformId   = current_version->dwPlatformId;

    TRACE( "got %ld.%ld platform %ld build %lx name %s service pack %d.%d product %d\n",
           current_version->dwMajorVersion, current_version->dwMinorVersion,
           current_version->dwPlatformId, current_version->dwBuildNumber,
           debugstr_w(current_version->szCSDVersion),
           current_version->wServicePackMajor, current_version->wServicePackMinor,
           current_version->wProductType );
}

/***********************************************************************
 *           RtlGetProductInfo    (NTDLL.@)
 *
 * Gives info about the current Windows product type, in a format compatible
 * with the given Windows version
 *
 * Returns TRUE if the input is valid, FALSE otherwise
 */
BOOLEAN WINAPI RtlGetProductInfo(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion, DWORD dwSpMajorVersion,
                                 DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType)
{
    TRACE("(%ld, %ld, %ld, %ld, %p)\n", dwOSMajorVersion, dwOSMinorVersion,
          dwSpMajorVersion, dwSpMinorVersion, pdwReturnedProductType);

    if (!pdwReturnedProductType)
        return FALSE;

    if (dwOSMajorVersion < 6)
    {
        *pdwReturnedProductType = PRODUCT_UNDEFINED;
        return FALSE;
    }

    if (current_version->wProductType == VER_NT_WORKSTATION)
        *pdwReturnedProductType = PRODUCT_ULTIMATE_N;
    else
        *pdwReturnedProductType = PRODUCT_STANDARD_SERVER;

    return TRUE;
}

/***********************************************************************
 *         RtlGetVersion   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetVersion( RTL_OSVERSIONINFOEXW *info )
{
    info->dwMajorVersion = current_version->dwMajorVersion;
    info->dwMinorVersion = current_version->dwMinorVersion;
    info->dwBuildNumber  = current_version->dwBuildNumber;
    info->dwPlatformId   = current_version->dwPlatformId;
    wcscpy( info->szCSDVersion, current_version->szCSDVersion );
    if(info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        info->wServicePackMajor = current_version->wServicePackMajor;
        info->wServicePackMinor = current_version->wServicePackMinor;
        info->wSuiteMask        = current_version->wSuiteMask;
        info->wProductType      = current_version->wProductType;
    }
    return STATUS_SUCCESS;
}


/******************************************************************************
 *  RtlGetNtVersionNumbers   (NTDLL.@)
 *
 * Get the version numbers of the run time library.
 *
 * PARAMS
 *  major [O] Destination for the Major version
 *  minor [O] Destination for the Minor version
 *  build [O] Destination for the Build version
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 * Introduced in Windows XP (NT5.1)
 */
void WINAPI RtlGetNtVersionNumbers( LPDWORD major, LPDWORD minor, LPDWORD build )
{
    if (major) *major = current_version->dwMajorVersion;
    if (minor) *minor = current_version->dwMinorVersion;
    /* FIXME: Does anybody know the real formula? */
    if (build) *build = (0xF0000000 | current_version->dwBuildNumber);
}


/******************************************************************************
 *  RtlGetNtProductType   (NTDLL.@)
 */
BOOLEAN WINAPI RtlGetNtProductType( LPDWORD type )
{
    if (type) *type = current_version->wProductType;
    return TRUE;
}

static inline UCHAR version_update_condition(UCHAR *last_condition, UCHAR condition)
{
    switch (*last_condition)
    {
        case 0:
            *last_condition = condition;
            break;
        case VER_EQUAL:
            if (condition >= VER_EQUAL && condition <= VER_LESS_EQUAL)
            {
                *last_condition = condition;
                return condition;
            }
            break;
        case VER_GREATER:
        case VER_GREATER_EQUAL:
            if (condition >= VER_EQUAL && condition <= VER_GREATER_EQUAL)
                return condition;
            break;
        case VER_LESS:
        case VER_LESS_EQUAL:
            if (condition == VER_EQUAL || (condition >= VER_LESS && condition <= VER_LESS_EQUAL))
                return condition;
            break;
    }
    if (!condition) *last_condition |= 0x10;
    return *last_condition & 0xf;
}

static inline NTSTATUS version_compare_values(ULONG left, ULONG right, UCHAR condition)
{
    switch (condition) {
        case VER_EQUAL:
            if (left != right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_GREATER:
            if (left <= right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_GREATER_EQUAL:
            if (left < right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_LESS:
            if (left >= right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_LESS_EQUAL:
            if (left > right) return STATUS_REVISION_MISMATCH;
            break;
        default:
            return STATUS_REVISION_MISMATCH;
    }
    return STATUS_SUCCESS;
}

/******************************************************************************
 *        RtlVerifyVersionInfo   (NTDLL.@)
 */
NTSTATUS WINAPI RtlVerifyVersionInfo( const RTL_OSVERSIONINFOEXW *info,
                                      DWORD dwTypeMask, DWORDLONG dwlConditionMask )
{
    RTL_OSVERSIONINFOEXW ver;
    NTSTATUS status;

    TRACE("(%p,0x%lx,0x%s)\n", info, dwTypeMask, wine_dbgstr_longlong(dwlConditionMask));

    ver.dwOSVersionInfoSize = sizeof(ver);
    if ((status = RtlGetVersion( &ver )) != STATUS_SUCCESS) return status;

    if(!(dwTypeMask && dwlConditionMask)) return STATUS_INVALID_PARAMETER;

    if(dwTypeMask & VER_PRODUCT_TYPE)
    {
        status = version_compare_values(ver.wProductType, info->wProductType, dwlConditionMask >> 7*3 & 0x07);
        if (status != STATUS_SUCCESS)
            return status;
    }
    if(dwTypeMask & VER_SUITENAME)
        switch(dwlConditionMask >> 6*3 & 0x07)
        {
            case VER_AND:
                if((info->wSuiteMask & ver.wSuiteMask) != info->wSuiteMask)
                    return STATUS_REVISION_MISMATCH;
                break;
            case VER_OR:
                if(!(info->wSuiteMask & ver.wSuiteMask) && info->wSuiteMask)
                    return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(dwTypeMask & VER_PLATFORMID)
    {
        status = version_compare_values(ver.dwPlatformId, info->dwPlatformId, dwlConditionMask >> 3*3 & 0x07);
        if (status != STATUS_SUCCESS)
            return status;
    }
    if(dwTypeMask & VER_BUILDNUMBER)
    {
        status = version_compare_values(ver.dwBuildNumber, info->dwBuildNumber, dwlConditionMask >> 2*3 & 0x07);
        if (status != STATUS_SUCCESS)
            return status;
    }

    if(dwTypeMask & (VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR|VER_SERVICEPACKMINOR))
    {
        unsigned char condition, last_condition = 0;
        BOOLEAN do_next_check = TRUE;

        if(dwTypeMask & VER_MAJORVERSION)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 1*3 & 0x07);
            status = version_compare_values(ver.dwMajorVersion, info->dwMajorVersion, condition);
            do_next_check = (ver.dwMajorVersion == info->dwMajorVersion) &&
                ((condition >= VER_EQUAL) && (condition <= VER_LESS_EQUAL));
        }
        if((dwTypeMask & VER_MINORVERSION) && do_next_check)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 0*3 & 0x07);
            status = version_compare_values(ver.dwMinorVersion, info->dwMinorVersion, condition);
            do_next_check = (ver.dwMinorVersion == info->dwMinorVersion) &&
                ((condition >= VER_EQUAL) && (condition <= VER_LESS_EQUAL));
        }
        if((dwTypeMask & VER_SERVICEPACKMAJOR) && do_next_check)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 5*3 & 0x07);
            status = version_compare_values(ver.wServicePackMajor, info->wServicePackMajor, condition);
            do_next_check = (ver.wServicePackMajor == info->wServicePackMajor) &&
                ((condition >= VER_EQUAL) && (condition <= VER_LESS_EQUAL));
        }
        if((dwTypeMask & VER_SERVICEPACKMINOR) && do_next_check)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 4*3 & 0x07);
            status = version_compare_values(ver.wServicePackMinor, info->wServicePackMinor, condition);
        }

        if (status != STATUS_SUCCESS)
            return status;
    }

    return STATUS_SUCCESS;
}


/******************************************************************************
 *        VerSetConditionMask   (NTDLL.@)
 */
ULONGLONG WINAPI VerSetConditionMask( ULONGLONG condition_mask, DWORD type_mask, BYTE condition )
{
    condition &= 0x07;
    if (type_mask & VER_PRODUCT_TYPE) condition_mask |= condition << 7*3;
    else if (type_mask & VER_SUITENAME) condition_mask |= condition << 6*3;
    else if (type_mask & VER_SERVICEPACKMAJOR) condition_mask |= condition << 5*3;
    else if (type_mask & VER_SERVICEPACKMINOR) condition_mask |= condition << 4*3;
    else if (type_mask & VER_PLATFORMID) condition_mask |= condition << 3*3;
    else if (type_mask & VER_BUILDNUMBER) condition_mask |= condition << 2*3;
    else if (type_mask & VER_MAJORVERSION) condition_mask |= condition << 1*3;
    else if (type_mask & VER_MINORVERSION) condition_mask |= condition << 0*3;
    return condition_mask;
}
