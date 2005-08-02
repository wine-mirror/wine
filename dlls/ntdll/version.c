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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ntstatus.h"
#include "windef.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

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
    NB_WINDOWS_VERSIONS
} WINDOWS_VERSION;

/* FIXME: compare values below with original and fix.
 * An *excellent* win9x version page (ALL versions !)
 * can be found at members.aol.com/axcel216/ver.htm */
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
    "win2003,win2k3"              /* WIN2K3 */
};

/* names to print out in debug traces */
static const char * const debug_names[NB_WINDOWS_VERSIONS] =
{
    "Windows 2.0",       /* WIN20 */
    "Windows 3.0",       /* WIN30 */
    "Windows 3.1",       /* WIN31 */
    "Windows 95",        /* WIN95 */
    "Windows 98",        /* WIN98 */
    "Windows Me",        /* WINME */
    "Windows NT 3.51",   /* NT351 */
    "Windows NT 4.0",    /* NT40 */
    "Windows 2000",      /* NT2K */
    "Windows XP",        /* WINXP */
    "Windows Server 2003"/* WIN2K3 */
};

/* if one of the following dlls is importing ntdll the windows
version autodetection switches wine to unicode (nt 3.51 or 4.0) */
static const WCHAR special_dlls[][16] =
{
    {'c','o','m','d','l','g','3','2','.','d','l','l',0},
    {'c','o','m','c','t','l','3','2','.','d','l','l',0},
    {'s','h','e','l','l','3','2','.','d','l','l',0},
    {'o','l','e','3','2','.','d','l','l',0},
    {'r','p','c','r','t','4','.','d','l','l',0}
};

/* the current version has not been autodetected but forced via cmdline */
static BOOL versionForced = FALSE;
static WINDOWS_VERSION forcedWinVersion; /* init value irrelevant */

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
                forcedWinVersion = i;
                versionForced = TRUE;
                TRACE( "got win version %s\n", WinVersionNames[forcedWinVersion] );
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
 *         VERSION_Init
 */
void VERSION_Init( const WCHAR *appname )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE root, hkey, config_key;
    static const WCHAR configW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e',0};
    static const WCHAR appdefaultsW[] = {'A','p','p','D','e','f','a','u','l','t','s','\\',0};

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
    if (!config_key) return;

    /* open AppDefaults\\appname key */
    if (appname && *appname)
    {
        const WCHAR *p;
        WCHAR appversion[MAX_PATH+20];
        BOOL got_win_ver = FALSE;

        if ((p = strrchrW( appname, '/' ))) appname = p + 1;
        if ((p = strrchrW( appname, '\\' ))) appname = p + 1;

        strcpyW( appversion, appdefaultsW );
        strcatW( appversion, appname );
        TRACE( "getting version from %s\n", debugstr_w(appversion) );
        RtlInitUnicodeString( &nameW, appversion );
        attr.RootDirectory = config_key;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe */
        if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
        {
            got_win_ver = parse_win_version( hkey );
            NtClose( hkey );
        }
        if (got_win_ver) goto done;
    }

    TRACE( "getting default version\n" );
    parse_win_version( config_key );

 done:
    NtClose( config_key );
}

/**********************************************************************
 *         VERSION_GetVersion
 *
 * WARNING !!!
 * Don't call this function too early during the Wine init,
 * as pdb->exe_modref (required by VERSION_GetImageVersion()) might still
 * be NULL in such cases, which causes the winver to ALWAYS be detected
 * as WIN31.
 * And as we cache the winver once it has been determined, this is bad.
 * This can happen much easier than you might think, as this function
 * is called by EVERY GetVersion*() API !
 *
 */
static const RTL_OSVERSIONINFOEXW *VERSION_GetVersion(void)
{
    static WORD winver = WIN98;

    if (versionForced)
        return &VersionData[forcedWinVersion];  /* user has overridden any sensible checks */

    return &VersionData[winver];
}


/***********************************************************************
 *         RtlGetVersion   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetVersion( RTL_OSVERSIONINFOEXW *info )
{
    const RTL_OSVERSIONINFOEXW * const current = VERSION_GetVersion();

    info->dwMajorVersion = current->dwMajorVersion;
    info->dwMinorVersion = current->dwMinorVersion;
    info->dwBuildNumber  = current->dwBuildNumber;
    info->dwPlatformId   = current->dwPlatformId;
    strcpyW( info->szCSDVersion, current->szCSDVersion );
    if(info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        info->wServicePackMajor = current->wServicePackMajor;
        info->wServicePackMinor = current->wServicePackMinor;
        info->wSuiteMask        = current->wSuiteMask;
        info->wProductType      = current->wProductType;
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
    const RTL_OSVERSIONINFOEXW * const current = VERSION_GetVersion();

    if (major) *major = current->dwMajorVersion;
    if (minor) *minor = current->dwMinorVersion;
    /* FIXME: Does anybody know the real formula? */
    if (build) *build = (0xF0000000 | current->dwBuildNumber);
}


/******************************************************************************
 *        VerifyVersionInfoW   (KERNEL32.@)
 */
NTSTATUS WINAPI RtlVerifyVersionInfo( const RTL_OSVERSIONINFOEXW *info,
                                      DWORD dwTypeMask, DWORDLONG dwlConditionMask )
{
    RTL_OSVERSIONINFOEXW ver;
    NTSTATUS status;

    FIXME("(%p,%lu,%llx): Not all cases correctly implemented yet\n",
          info, dwTypeMask, dwlConditionMask);

    /* FIXME:
        - Check the following special case on Windows (various versions):
          o lp->wSuiteMask == 0 and ver.wSuiteMask != 0 and VER_AND/VER_OR
          o lp->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)
        - MSDN talks about some tests being impossible. Check what really happens.
     */

    ver.dwOSVersionInfoSize = sizeof(ver);
    if ((status = RtlGetVersion( &ver )) != STATUS_SUCCESS) return status;

    if(!(dwTypeMask && dwlConditionMask)) return STATUS_INVALID_PARAMETER;

    if(dwTypeMask & VER_PRODUCT_TYPE)
        switch(dwlConditionMask >> 7*3 & 0x07) {
            case VER_EQUAL:
                if(ver.wProductType != info->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.wProductType <= info->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.wProductType < info->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.wProductType >= info->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.wProductType > info->wProductType) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
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
        switch(dwlConditionMask >> 3*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwPlatformId != info->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwPlatformId <= info->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwPlatformId < info->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwPlatformId >= info->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwPlatformId > info->dwPlatformId) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(dwTypeMask & VER_BUILDNUMBER)
        switch(dwlConditionMask >> 2*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwBuildNumber != info->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwBuildNumber <= info->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwBuildNumber < info->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwBuildNumber >= info->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwBuildNumber > info->dwBuildNumber) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(dwTypeMask & VER_MAJORVERSION)
        switch(dwlConditionMask >> 1*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwMajorVersion != info->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwMajorVersion <= info->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwMajorVersion < info->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwMajorVersion >= info->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwMajorVersion > info->dwMajorVersion) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(dwTypeMask & VER_MINORVERSION)
        switch(dwlConditionMask >> 0*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.dwMinorVersion != info->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.dwMinorVersion <= info->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.dwMinorVersion < info->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.dwMinorVersion >= info->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.dwMinorVersion > info->dwMinorVersion) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(dwTypeMask & VER_SERVICEPACKMAJOR)
        switch(dwlConditionMask >> 5*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.wServicePackMajor != info->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.wServicePackMajor <= info->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.wServicePackMajor < info->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.wServicePackMajor >= info->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.wServicePackMajor > info->wServicePackMajor) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    if(dwTypeMask & VER_SERVICEPACKMINOR)
        switch(dwlConditionMask >> 4*3 & 0x07)
        {
            case VER_EQUAL:
                if(ver.wServicePackMinor != info->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER:
                if(ver.wServicePackMinor <= info->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_GREATER_EQUAL:
                if(ver.wServicePackMinor < info->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS:
                if(ver.wServicePackMinor >= info->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            case VER_LESS_EQUAL:
                if(ver.wServicePackMinor > info->wServicePackMinor) return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }

    return STATUS_SUCCESS;
}
