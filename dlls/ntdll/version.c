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

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "wine/unicode.h"
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
    WIN2K3,  /* Windows 2003 */
    WINVISTA,/* Windows Vista */
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
        {'W','i','n','3','2','s',' ','1','.','3',0},
        0, 0, 0, 0, 0
    },
    /* WIN30 FIXME: verify values */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 3, 0, 0, VER_PLATFORM_WIN32s,
        {'W','i','n','3','2','s',' ','1','.','3',0},
        0, 0, 0, 0, 0
    },
    /* WIN31 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 3, 10, 0, VER_PLATFORM_WIN32s,
        {'W','i','n','3','2','s',' ','1','.','3',0},
        0, 0, 0, 0, 0
    },
    /* WIN95 */
    {
        /* Win95:       4, 0, 0x40003B6, ""
         * Win95sp1:    4, 0, 0x40003B6, " A " (according to doc)
         * Win95osr2:   4, 0, 0x4000457, " B " (according to doc)
         * Win95osr2.1: 4, 3, 0x40304BC, " B " (according to doc)
         * Win95osr2.5: 4, 3, 0x40304BE, " C " (according to doc)
         * Win95a/b can be discerned via regkey SubVersionNumber
         * See also:
         * http://support.microsoft.com/support/kb/articles/q158/2/38.asp
         */
        sizeof(RTL_OSVERSIONINFOEXW), 4, 0, 0x40003B6, VER_PLATFORM_WIN32_WINDOWS,
        {0},
        0, 0, 0, 0, 0
    },
    /* WIN98 (second edition) */
    {
        /* Win98:   4, 10, 0x40A07CE, " "   4.10.1998
         * Win98SE: 4, 10, 0x40A08AE, " A " 4.10.2222
         */
        sizeof(RTL_OSVERSIONINFOEXW), 4, 10, 0x40A08AE, VER_PLATFORM_WIN32_WINDOWS,
        {' ','A',' ',0},
        0, 0, 0, 0, 0
    },
    /* WINME */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 4, 90, 0x45A0BB8, VER_PLATFORM_WIN32_WINDOWS,
        {' ',0},
        0, 0, 0, 0, 0
    },
    /* NT351 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 3, 51, 0x421, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','2',0},
        0, 0, 0, 0, 0
    },
    /* NT40 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 4, 0, 0x565, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','6','a',0},
        6, 0, 0, VER_NT_WORKSTATION, 0
    },
    /* NT2K */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 0, 0x893, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','4',0},
        4, 0, 0, VER_NT_WORKSTATION, 30 /* FIXME: Great, a reserved field with a value! */
    },
    /* WINXP */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 1, 0xA28, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','2',0},
        2, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 30 /* FIXME: Great, a reserved field with a value! */
    },
    /* WIN2K3 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 2, 0xECE, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','1',0},
        1, 0, VER_SUITE_SINGLEUSERTS, VER_NT_SERVER, 0
    },
    /* WINVISTA */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 6, 0, 0x1770, VER_PLATFORM_WIN32_NT,
        {' ',0},
        0, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 0
    }
};

static const char * const WinVersionNames[NB_WINDOWS_VERSIONS] =
{ /* no spaces in here ! */
    "win20",                      /* WIN20 */
    "win30",                      /* WIN30 */
    "win31",                      /* WIN31 */
    "win95",                      /* WIN95 */
    "win98",                      /* WIN98 */
    "winme",                      /* WINME */
    "nt351",                      /* NT351 */
    "nt40",                       /* NT40 */
    "win2000,win2k,nt2k,nt2000",  /* NT2K */
    "winxp",                      /* WINXP */
    "win2003,win2k3",             /* WIN2K3 */
    "vista,winvista"              /* WINVISTA*/
};


/* initialized to null so that we crash if we try to retrieve the version too early at startup */
static const RTL_OSVERSIONINFOEXW *current_version;


/**********************************************************************
 *         get_nt_registry_version
 *
 * Fetch the version information from the NT-style registry keys.
 */
static BOOL get_nt_registry_version( RTL_OSVERSIONINFOEXW *version )
{
    static const WCHAR version_keyW[] = {'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'W','i','n','d','o','w','s',' ','N','T','\\',
                                         'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    static const WCHAR service_pack_keyW[] = {'M','a','c','h','i','n','e','\\',
                                              'S','y','s','t','e','m','\\',
                                              'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                              'C','o','n','t','r','o','l','\\',
                                              'W','i','n','d','o','w','s',0};
    static const WCHAR product_keyW[] = {'M','a','c','h','i','n','e','\\',
                                         'S','y','s','t','e','m','\\',
                                         'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                         'C','o','n','t','r','o','l','\\',
                                         'P','r','o','d','u','c','t','O','p','t','i','o','n','s',0};
    static const WCHAR CurrentBuildNumberW[] = {'C','u','r','r','e','n','t','B','u','i','l','d','N','u','m','b','e','r',0};
    static const WCHAR CSDVersionW[] = {'C','S','D','V','e','r','s','i','o','n',0};
    static const WCHAR CurrentVersionW[] = {'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    static const WCHAR ProductTypeW[] = {'P','r','o','d','u','c','t','T','y','p','e',0};
    static const WCHAR WinNTW[]    = {'W','i','n','N','T',0};
    static const WCHAR ServerNTW[] = {'S','e','r','v','e','r','N','T',0};
    static const WCHAR LanmanNTW[] = {'L','a','n','m','a','n','N','T',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    HANDLE hkey, hkey2;
    char tmp[64];
    DWORD count;
    BOOL ret = FALSE;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, version_keyW );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return FALSE;

    memset( version, 0, sizeof(*version) );

    RtlInitUnicodeString( &valueW, CurrentVersionW );
    if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
    {
        WCHAR *p, *str = (WCHAR *)info->Data;
        str[info->DataLength / sizeof(WCHAR)] = 0;
        p = strchrW( str, '.' );
        if (p)
        {
            *p++ = 0;
            version->dwMinorVersion = atoiW( p );
        }
        version->dwMajorVersion = atoiW( str );
    }

    if (version->dwMajorVersion)   /* we got the main version, now fetch the other fields */
    {
        ret = TRUE;
        version->dwPlatformId = VER_PLATFORM_WIN32_NT;

        /* get build number */

        RtlInitUnicodeString( &valueW, CurrentBuildNumberW );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
        {
            WCHAR *str = (WCHAR *)info->Data;
            str[info->DataLength / sizeof(WCHAR)] = 0;
            version->dwBuildNumber = atoiW( str );
        }

        /* get version description */

        RtlInitUnicodeString( &valueW, CSDVersionW );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
        {
            DWORD len = min( info->DataLength, sizeof(version->szCSDVersion) - sizeof(WCHAR) );
            memcpy( version->szCSDVersion, info->Data, len );
            version->szCSDVersion[len / sizeof(WCHAR)] = 0;
        }

        /* get service pack version */

        RtlInitUnicodeString( &nameW, service_pack_keyW );
        if (!NtOpenKey( &hkey2, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &valueW, CSDVersionW );
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

        RtlInitUnicodeString( &nameW, product_keyW );
        if (!NtOpenKey( &hkey2, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &valueW, ProductTypeW );
            if (!NtQueryValueKey( hkey2, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
            {
                WCHAR *str = (WCHAR *)info->Data;
                str[info->DataLength / sizeof(WCHAR)] = 0;
                if (!strcmpiW( str, WinNTW )) version->wProductType = VER_NT_WORKSTATION;
                else if (!strcmpiW( str, LanmanNTW )) version->wProductType = VER_NT_DOMAIN_CONTROLLER;
                else if (!strcmpiW( str, ServerNTW )) version->wProductType = VER_NT_SERVER;
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
    static const WCHAR version_keyW[] = {'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'W','i','n','d','o','w','s','\\',
                                         'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    static const WCHAR VersionNumberW[] = {'V','e','r','s','i','o','n','N','u','m','b','e','r',0};
    static const WCHAR SubVersionNumberW[] = {'S','u','b','V','e','r','s','i','o','n','N','u','m','b','e','r',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    HANDLE hkey;
    char tmp[64];
    DWORD count;
    BOOL ret = FALSE;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, version_keyW );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return FALSE;

    memset( version, 0, sizeof(*version) );

    RtlInitUnicodeString( &valueW, VersionNumberW );
    if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp)-1, &count ))
    {
        WCHAR *p, *str = (WCHAR *)info->Data;
        str[info->DataLength / sizeof(WCHAR)] = 0;
        p = strchrW( str, '.' );
        if (p) *p++ = 0;
        version->dwMajorVersion = atoiW( str );
        if (p)
        {
            str = p;
            p = strchrW( str, '.' );
            if (p)
            {
                *p++ = 0;
                version->dwBuildNumber = atoiW( p );
            }
            version->dwMinorVersion = atoiW( str );
        }
        /* build number contains version too on Win9x */
        version->dwBuildNumber |= MAKEWORD( version->dwMinorVersion, version->dwMajorVersion ) << 16;
    }

    if (version->dwMajorVersion)   /* we got the main version, now fetch the other fields */
    {
        ret = TRUE;
        version->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;

        RtlInitUnicodeString( &valueW, SubVersionNumberW );
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
    static const WCHAR VersionW[] = {'V','e','r','s','i','o','n',0};

    UNICODE_STRING valueW;
    char tmp[64], buffer[50];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;
    DWORD count, len;
    int i;

    RtlInitUnicodeString( &valueW, VersionW );
    if (NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp), &count ))
        return FALSE;

    RtlUnicodeToMultiByteN( buffer, sizeof(buffer)-1, &len, (WCHAR *)info->Data, info->DataLength );
    buffer[len] = 0;

    for (i = 0; i < NB_WINDOWS_VERSIONS; i++)
    {
        const char *p, *pCurr = WinVersionNames[i];
        /* iterate through all winver aliases separated by comma */
        do {
            p = strchr(pCurr, ',');
            len = p ? p - pCurr : strlen(pCurr);
            if ( (!strncmp( pCurr, buffer, len )) && (buffer[len] == 0) )
            {
                current_version = &VersionData[i];
                TRACE( "got win version %s\n", WinVersionNames[i] );
                return TRUE;
            }
            pCurr = p+1;
        } while (p);
    }

    MESSAGE("Invalid Windows version value '%s' specified in config file.\n", buffer );
    MESSAGE("Valid versions are:" );
    for (i = 0; i < NB_WINDOWS_VERSIONS; i++)
    {
        /* only list the first, "official" alias in case of aliases */
        const char *pCurr = WinVersionNames[i];
        const char *p = strchr(pCurr, ',');
        len = (p) ? p - pCurr : strlen(pCurr);

        MESSAGE(" '%.*s'%c", (int)len, pCurr, (i == NB_WINDOWS_VERSIONS - 1) ? '\n' : ',' );
    }
    return FALSE;
}


/**********************************************************************
 *         version_init
 */
void version_init( const WCHAR *appname )
{
    static const WCHAR configW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e',0};
    static const WCHAR appdefaultsW[] = {'A','p','p','D','e','f','a','u','l','t','s','\\',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE root, hkey, config_key;
    BOOL got_win_ver = FALSE;

    current_version = &VersionData[NT2K];  /* default if nothing else is specified */

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, configW );

    /* @@ Wine registry key: HKCU\Software\Wine */
    if (NtOpenKey( &config_key, KEY_ALL_ACCESS, &attr )) config_key = 0;
    NtClose( root );
    if (!config_key) goto done;

    /* open AppDefaults\\appname key */
    if (appname && *appname)
    {
        const WCHAR *p;
        WCHAR appversion[MAX_PATH+20];

        if ((p = strrchrW( appname, '/' ))) appname = p + 1;
        if ((p = strrchrW( appname, '\\' ))) appname = p + 1;

        strcpyW( appversion, appdefaultsW );
        strcatW( appversion, appname );
        RtlInitUnicodeString( &nameW, appversion );
        attr.RootDirectory = config_key;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe */
        if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
        {
            TRACE( "getting version from %s\n", debugstr_w(appversion) );
            got_win_ver = parse_win_version( hkey );
            NtClose( hkey );
        }
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

    user_shared_data->NtProductType      = current_version->wProductType;
    user_shared_data->ProductTypeIsValid = TRUE;
    user_shared_data->MajorNtVersion     = current_version->dwMajorVersion;
    user_shared_data->MinorNtVersion     = current_version->dwMinorVersion;
    user_shared_data->MinorNtVersion     = current_version->dwMinorVersion;
    user_shared_data->SuiteMask          = current_version->wSuiteMask;

    TRACE( "got %d.%d plaform %d build %x name %s service pack %d.%d product %d\n",
           current_version->dwMajorVersion, current_version->dwMinorVersion,
           current_version->dwPlatformId, current_version->dwBuildNumber,
           debugstr_w(current_version->szCSDVersion),
           current_version->wServicePackMajor, current_version->wServicePackMinor,
           current_version->wProductType );
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
    strcpyW( info->szCSDVersion, current_version->szCSDVersion );
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

    TRACE("(%p,0x%x,0x%s)\n", info, dwTypeMask, wine_dbgstr_longlong(dwlConditionMask));

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
        unsigned char condition = 0;
        BOOLEAN do_next_check = TRUE;

        if(dwTypeMask & VER_MAJORVERSION)
            condition = dwlConditionMask >> 1*3 & 0x07;
        else if(dwTypeMask & VER_MINORVERSION)
            condition = dwlConditionMask >> 0*3 & 0x07;
        else if(dwTypeMask & VER_SERVICEPACKMAJOR)
            condition = dwlConditionMask >> 5*3 & 0x07;
        else if(dwTypeMask & VER_SERVICEPACKMINOR)
            condition = dwlConditionMask >> 4*3 & 0x07;

        if(dwTypeMask & VER_MAJORVERSION)
        {
            status = version_compare_values(ver.dwMajorVersion, info->dwMajorVersion, condition);
            do_next_check = (ver.dwMajorVersion == info->dwMajorVersion) &&
                ((condition != VER_EQUAL) || (status == STATUS_SUCCESS));
        }
        if((dwTypeMask & VER_MINORVERSION) && do_next_check)
        {
            status = version_compare_values(ver.dwMinorVersion, info->dwMinorVersion, condition);
            do_next_check = (ver.dwMinorVersion == info->dwMinorVersion) &&
                ((condition != VER_EQUAL) || (status == STATUS_SUCCESS));
        }
        if((dwTypeMask & VER_SERVICEPACKMAJOR) && do_next_check)
        {
            status = version_compare_values(ver.wServicePackMajor, info->wServicePackMajor, condition);
            do_next_check = (ver.wServicePackMajor == info->wServicePackMajor) &&
                ((condition != VER_EQUAL) || (status == STATUS_SUCCESS));
        }
        if((dwTypeMask & VER_SERVICEPACKMINOR) && do_next_check)
        {
            status = version_compare_values(ver.wServicePackMinor, info->wServicePackMinor, condition);
        }

        if (status != STATUS_SUCCESS)
            return status;
    }

    return STATUS_SUCCESS;
}
