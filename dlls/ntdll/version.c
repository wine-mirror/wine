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
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "module.h"
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
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','6',0},
        6, 0, 0, VER_NT_WORKSTATION, 0
    },
    /* NT2K */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 0, 0x893, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','3',0},
        3, 0, 0, VER_NT_WORKSTATION, 30 /* FIXME: Great, a reserved field with a value! */
    },
    /* WINXP */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 1, 0xA28, VER_PLATFORM_WIN32_NT,
        {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','1',0},
        1, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 30 /* FIXME: Great, a reserved field with a value! */
    },
    /* WIN2K3 */
    {
        sizeof(RTL_OSVERSIONINFOEXW), 5, 2, 0xECE, VER_PLATFORM_WIN32_NT,
        {0},
        0, 0, VER_SUITE_SINGLEUSERTS, VER_NT_SERVER, 0
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
    "Windows 2003"       /* WIN2K3 */
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
static BOOL parse_win_version( HKEY hkey )
{
    static const WCHAR WindowsW[] = {'W','i','n','d','o','w','s',0};

    UNICODE_STRING valueW;
    char tmp[64], buffer[50];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;
    DWORD count, len;
    int i;

    RtlInitUnicodeString( &valueW, WindowsW );
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
    HKEY hkey, config_key;
    static const WCHAR configW[] = {'M','a','c','h','i','n','e','\\',
                                    'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\',
                                    'W','i','n','e','\\',
                                    'C','o','n','f','i','g',0};
    static const WCHAR appdefaultsW[] = {'A','p','p','D','e','f','a','u','l','t','s','\\',0};
    static const WCHAR versionW[] = {'\\','V','e','r','s','i','o','n',0};

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, configW );

    if (NtOpenKey( &config_key, KEY_ALL_ACCESS, &attr )) return;
    attr.RootDirectory = config_key;

    /* open AppDefaults\\appname\\Version key */
    if (appname && *appname)
    {
        const WCHAR *p;
        WCHAR appversion[MAX_PATH+20];
        BOOL got_win_ver = FALSE;

        if ((p = strrchrW( appname, '/' ))) appname = p + 1;
        if ((p = strrchrW( appname, '\\' ))) appname = p + 1;

        strcpyW( appversion, appdefaultsW );
        strcatW( appversion, appname );
        strcatW( appversion, versionW );
        TRACE( "getting version from %s\n", debugstr_w(appversion) );
        RtlInitUnicodeString( &nameW, appversion );

        if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
        {
            got_win_ver = parse_win_version( hkey );
            NtClose( hkey );
        }
        if (got_win_ver) goto done;
    }

    TRACE( "getting default version\n" );
    RtlInitUnicodeString( &nameW, versionW + 1 );
    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        parse_win_version( hkey );
        NtClose( hkey );
    }

 done:
    NtClose( config_key );
}


/**********************************************************************
 *      VERSION_GetSystemDLLVersion
 *
 * This function tries to figure out if a given (native) dll comes from
 * win95/98 or winnt. Since all values in the OptionalHeader are not a
 * usable hint, we test if a dll imports the ntdll.
 * This is at least working for all system dlls like comctl32, comdlg32 and
 * shell32.
 * If you have a better idea to figure this out...
 */
static DWORD VERSION_GetSystemDLLVersion( HMODULE hmod )
{
    DWORD size;
    IMAGE_IMPORT_DESCRIPTOR *pe_imp;

    if ((pe_imp = RtlImageDirectoryEntryToData( hmod, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
    {
        for ( ; pe_imp->Name; pe_imp++)
        {
            char * name = (char *)hmod + (unsigned int)pe_imp->Name;
            TRACE("%s\n", name);

            if (!strncasecmp(name, "ntdll", 5))
            {
              switch(RtlImageNtHeader(hmod)->OptionalHeader.MajorOperatingSystemVersion) {
                  case 3:
                          MESSAGE("WARNING: very old native DLL (NT 3.x) used, might cause instability.\n");
                          return NT351;
                  case 4: return NT40;
                  case 5: return NT2K;
                  case 6: return WINXP;
                  case 7: return WIN2K3; /* FIXME: Not sure, should be verified with a Win2K3 dll */
                  default:
                          FIXME("Unknown DLL OS version, please report !!\n");
                          return WIN2K3;
              }
            }
        }
    }
    return WIN95;
}


/**********************************************************************
 *      VERSION_GetLinkedDllVersion
 *
 * Some version data (not reliable!):
 * linker/OS/image/subsys
 *
 * x.xx/1.00/0.00/3.10  Win32s          (any version ?)
 * 2.39/1.00/0.00/3.10  Win32s          freecell.exe (any version)
 * 2.50/1.00/4.00/4.00  Win32s 1.30     winhlp32.exe
 * 2.60/3.51/3.51/3.51  NT351SP5        system dlls
 * 2.60/3.51/3.51/4.00  NT351SP5        comctl32 dll
 * 2.xx/1.00/0.00/4.00  Win95           system files
 * x.xx/4.00/0.00/4.00  Win95           most applications
 * 3.10/4.00/0.00/4.00  Win98           notepad
 * x.xx/5.00/5.00/4.00  Win98           system dlls (e.g. comctl32.dll)
 * x.xx/4.00/4.00/4.00  NT 4            most apps
 * 5.12/5.00/5.00/4.00  NT4+IE5         comctl32.dll
 * 5.12/5.00/5.00/4.00  Win98           calc
 * x.xx/5.00/5.00/4.00  win95/win98/NT4 IE5 files
 */
static DWORD VERSION_GetLinkedDllVersion(void)
{
    WINDOWS_VERSION WinVersion = NB_WINDOWS_VERSIONS;
    PIMAGE_OPTIONAL_HEADER ophd;
    IMAGE_NT_HEADERS *nt;
    const WCHAR *name;
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;
    int i;

    /* First check the native dlls provided. These have to be
       from one windows version */

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        if (mod->Flags & LDR_WINE_INTERNAL) continue;
        nt = RtlImageNtHeader(mod->BaseAddress);
        ophd = &nt->OptionalHeader;
        name = strrchrW( mod->FullDllName.Buffer, '\\' );
        if (name) name++;
        else name = mod->FullDllName.Buffer;

        TRACE("%s: %02x.%02x/%02x.%02x/%02x.%02x/%02x.%02x\n",
              debugstr_w(name), ophd->MajorLinkerVersion, ophd->MinorLinkerVersion,
              ophd->MajorOperatingSystemVersion, ophd->MinorOperatingSystemVersion,
              ophd->MajorImageVersion, ophd->MinorImageVersion,
              ophd->MajorSubsystemVersion, ophd->MinorSubsystemVersion);

        for (i = 0; i < sizeof(special_dlls)/sizeof(special_dlls[0]); i++)
        {
            /* test if it is a special dll */
            if (!strcmpiW(name, special_dlls[i]))
            {
                DWORD DllVersion = VERSION_GetSystemDLLVersion(mod->BaseAddress);
                if (WinVersion == NB_WINDOWS_VERSIONS) WinVersion = DllVersion;
                else if (WinVersion != DllVersion)
                {
                    ERR("You mixed system DLLs from different windows versions! Expect a crash! (%s: expected version %s, but is %s)\n",
                        debugstr_w(name),
                        debugstr_w(VersionData[WinVersion].szCSDVersion),
                        debugstr_w(VersionData[DllVersion].szCSDVersion));
                    return WIN20; /* this may let the exe exit */
                }
                break;
            }
        }
    }

    if(WinVersion != NB_WINDOWS_VERSIONS) return WinVersion;

    /* we are using no external system dlls, look at the exe */
    nt = RtlImageNtHeader(NtCurrentTeb()->Peb->ImageBaseAddress);
    ophd = &nt->OptionalHeader;

    TRACE("%02x.%02x/%02x.%02x/%02x.%02x/%02x.%02x\n",
          ophd->MajorLinkerVersion, ophd->MinorLinkerVersion,
          ophd->MajorOperatingSystemVersion, ophd->MinorOperatingSystemVersion,
          ophd->MajorImageVersion, ophd->MinorImageVersion,
          ophd->MajorSubsystemVersion, ophd->MinorSubsystemVersion);

    /* special nt 3.51 */
    if (3 == ophd->MajorOperatingSystemVersion && 51 == ophd->MinorOperatingSystemVersion)
    {
        return NT351;
    }

    /* the MajorSubsystemVersion is the only usable sign */
    if (ophd->MajorSubsystemVersion < 4)
    {
        if ( ophd->MajorOperatingSystemVersion == 1
             && ophd->MinorOperatingSystemVersion == 0)
        {
            return WIN31; /* win32s */
        }

        if (ophd->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            return NT351; /* FIXME: NT 3.1, not tested */
        else
            return WIN95;
    }

    return WIN95;
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
    static WORD winver = 0xffff;

    if (versionForced)
        return &VersionData[forcedWinVersion];  /* user has overridden any sensible checks */

    if (winver == 0xffff) /* to be determined */
    {
        WINDOWS_VERSION retver = VERSION_GetLinkedDllVersion();

        /* cache determined value, but do not store in case of WIN31 */
        if (retver != WIN31) winver = retver;
        return &VersionData[retver];
    }
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
    TRACE("<-- %s (%s)\n", debug_names[current - VersionData], debugstr_w(current->szCSDVersion) );
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
