/*
 * Windows and DOS version functions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Patrik Stridvall
 * Copyright 1998,2003 Andreas Mohr
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
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/winbase16.h"
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
    NB_WINDOWS_VERSIONS
} WINDOWS_VERSION;

typedef struct
{
    char             human_readable[32];
    LONG             getVersion16;
    LONG             getVersion32;
    OSVERSIONINFOEXA getVersionEx;
} VERSION_DATA;

/* FIXME: compare values below with original and fix.
 * An *excellent* win9x version page (ALL versions !)
 * can be found at members.aol.com/axcel216/ver.htm */
static VERSION_DATA VersionData[NB_WINDOWS_VERSIONS] =
{
    /* WIN20 FIXME: verify values */
    {
	"Windows 2.0",
	MAKELONG( 0x0002, 0x0303 ), /* assume DOS 3.3 */
	MAKELONG( 0x0002, 0x8000 ),
	{
	    /* yes, sizeof(OSVERSIONINFOA) is correct here
	     * (in case of OSVERSIONINFOEXA application request,
	     * we adapt it dynamically). */
            sizeof(OSVERSIONINFOA), 2, 0, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3",
	    0, 0, 0, 0, 0
	}
    },
    /* WIN30 FIXME: verify values */
    {
	"Windows 3.0",
	MAKELONG( 0x0003, 0x0500 ), /* assume DOS 5.00 */
	MAKELONG( 0x0003, 0x8000 ),
	{
            sizeof(OSVERSIONINFOA), 3, 0, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3",
	    0, 0, 0, 0, 0
	}
    },
    /* WIN31 */
    {
	"Windows 3.1",
	MAKELONG( 0x0a03, 0x0616 ), /* DOS 6.22 */
	MAKELONG( 0x0a03, 0x8000 ),
	{
            sizeof(OSVERSIONINFOA), 3, 10, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3",
	    0, 0, 0, 0, 0
	}
    },
    /* WIN95 */
    {
	"Windows 95",
        0x07005F03,
        0xC0000004,
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
            sizeof(OSVERSIONINFOA), 4, 0, 0x40003B6,
            VER_PLATFORM_WIN32_WINDOWS, "",
	    0, 0, 0, 0, 0
        }
    },
    /* WIN98 (second edition) */
    {
	"Windows 98 SE",
        0x070A5F03,
        0xC0000A04,
        {
            /* Win98:   4, 10, 0x40A07CE, " "   4.10.1998
             * Win98SE: 4, 10, 0x40A08AE, " A " 4.10.2222
             */
            sizeof(OSVERSIONINFOA), 4, 10, 0x40A08AE,
            VER_PLATFORM_WIN32_WINDOWS, " A ",
	    0, 0, 0, 0, 0
        }
    },
    /* WINME */
    {
	"Windows ME",
        0x08005F03,
        0xC0005A04,
        {
            sizeof(OSVERSIONINFOA), 4, 90, 0x45A0BB8,
            VER_PLATFORM_WIN32_WINDOWS, " ",
	    0, 0, 0, 0, 0
        }
    },
    /* NT351 */
    {
	"Windows NT 3.51",
        0x05000A03,
        0x04213303,
        {
            sizeof(OSVERSIONINFOA), 3, 51, 0x421,
            VER_PLATFORM_WIN32_NT, "Service Pack 2",
	    0, 0, 0, 0, 0
        }
    },
    /* NT40 */
    {
	"Windows NT 4.0",
        0x05000A03,
        0x05650004,
        {
            sizeof(OSVERSIONINFOA), 4, 0, 0x565,
            VER_PLATFORM_WIN32_NT, "Service Pack 6",
	    6, 0, 0, VER_NT_WORKSTATION, 0
        }
    },
    /* NT2K */
    {
	"Windows 2000",
        0x05005F03,
        0x08930005,
        {
            sizeof(OSVERSIONINFOA), 5, 0, 0x893,
            VER_PLATFORM_WIN32_NT, "Service Pack 3",
            3, 0, 0, VER_NT_WORKSTATION, 30 /* FIXME: Great, a reserved field with a value! */
        }
    },
    /* WINXP */
    {
	"Windows XP",
        0x05005F03, /* Assuming DOS 5 like the other NT */
        0x0A280105,
        {
            sizeof(OSVERSIONINFOA), 5, 1, 0xA28,
            VER_PLATFORM_WIN32_NT, "Service Pack 1",
            1, 0, VER_SUITE_SINGLEUSERTS, VER_NT_WORKSTATION, 30 /* FIXME: Great, a reserved field with a value! */
        }
    }
};

static const char *WinVersionNames[NB_WINDOWS_VERSIONS] =
{ /* no spaces in here ! */
    "win20",
    "win30",
    "win31",
    "win95",
    "win98",
    "winme",
    "nt351",
    "nt40",
    "win2000,win2k,nt2k,nt2000",
    "winxp"
};

/* if one of the following dlls is importing ntdll the windows
version autodetection switches wine to unicode (nt 3.51 or 4.0) */
static char * special_dlls[] =
{
	"comdlg32.dll",
	"comctl32.dll",
	"shell32.dll",
	"ole32.dll",
	"rpcrt4.dll",
	NULL
};

/* the current version has not been autodetected but forced via cmdline */
static BOOL versionForced = FALSE;
static WINDOWS_VERSION forcedWinVersion = WIN31; /* init value irrelevant */

/**********************************************************************
 *         VERSION_ParseWinVersion
 */
static void VERSION_ParseWinVersion( const char *arg )
{
    int i, len;
    const char *pCurr, *p;
    for (i = 0; i < NB_WINDOWS_VERSIONS; i++)
    {
        pCurr = WinVersionNames[i];
        /* iterate through all winver aliases separated by comma */
        do {
            p = strchr(pCurr, ',');
            len = p ? (int)p - (int)pCurr : strlen(pCurr);
            if ( (!strncmp( pCurr, arg, len )) && (arg[len] == '\0') )
            {
                forcedWinVersion = (WINDOWS_VERSION)i;
                versionForced = TRUE;
                return;
            }
            pCurr = p+1;
        } while (p);
    }
    MESSAGE("Invalid Windows version value '%s' specified in config file.\n", arg );
    MESSAGE("Valid versions are:" );
    for (i = 0; i < NB_WINDOWS_VERSIONS; i++)
    {
        /* only list the first, "official" alias in case of aliases */
        pCurr = WinVersionNames[i];
        p = strchr(pCurr, ',');
        len = (p) ? (int)p - (int)pCurr : strlen(pCurr);

        MESSAGE(" '%.*s'%c", len, pCurr,
                (i == NB_WINDOWS_VERSIONS - 1) ? '\n' : ',' );
    }
    ExitProcess(1);
}


/**********************************************************************
 *         VERSION_ParseDosVersion
 */
static void VERSION_ParseDosVersion( const char *arg )
{
    int hi, lo;
    if (sscanf( arg, "%d.%d", &hi, &lo ) == 2)
    {
        VersionData[WIN31].getVersion16 =
            MAKELONG(LOWORD(VersionData[WIN31].getVersion16),
                     (hi<<8) + lo);
    }
    else
    {
        MESSAGE("Wrong format for DOS version in config file. Use \"x.xx\"\n");
        ExitProcess(1);
    }
}


/**********************************************************************
 *         VERSION_ParseVersion
 *
 * Parse the contents of the Version key.
 */
static void VERSION_ParseVersion( HKEY hkey, BOOL *got_win_ver, BOOL *got_dos_ver )
{
    static const WCHAR WindowsW[] = {'W','i','n','d','o','w','s',0};
    static const WCHAR DosW[] = {'D','O','S',0};

    UNICODE_STRING valueW;
    char tmp[64], buffer[50];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;
    DWORD count, len;

    if (!*got_win_ver)
    {
        RtlInitUnicodeString( &valueW, WindowsW );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp), &count ))
        {
            RtlUnicodeToMultiByteN( buffer, sizeof(buffer)-1, &len,
                                    (WCHAR *)info->Data, info->DataLength );
            buffer[len] = 0;
            VERSION_ParseWinVersion( buffer );
            TRACE( "got win version %s\n", WinVersionNames[forcedWinVersion] );
            *got_win_ver = TRUE;
        }
    }
    if (!*got_dos_ver)
    {
        RtlInitUnicodeString( &valueW, DosW );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp), &count ))
        {
            RtlUnicodeToMultiByteN( buffer, sizeof(buffer)-1, &len,
                                    (WCHAR *)info->Data, info->DataLength );
            buffer[len] = 0;
            VERSION_ParseDosVersion( buffer );
            TRACE( "got dos version %lx\n", VersionData[WIN31].getVersion16 );
            *got_dos_ver = TRUE;
        }
    }
}


/**********************************************************************
 *         VERSION_Init
 */
static void VERSION_Init(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HKEY hkey, config_key;
    BOOL got_win_ver = FALSE, got_dos_ver = FALSE;
    WCHAR buffer[MAX_PATH], appversion[MAX_PATH+20], *appname, *p;
    static BOOL init_done;
    static const WCHAR configW[] = {'M','a','c','h','i','n','e','\\',
                                    'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\',
                                    'W','i','n','e','\\',
                                    'C','o','n','f','i','g',0};
    static const WCHAR appdefaultsW[] = {'A','p','p','D','e','f','a','u','l','t','s','\\',0};
    static const WCHAR versionW[] = {'\\','V','e','r','s','i','o','n',0};

    if (init_done) return;
    if (!GetModuleFileNameW( 0, buffer, MAX_PATH ))
    {
        WARN( "could not get module file name\n" );
        return;
    }
    init_done = TRUE;
    appname = buffer;
    if ((p = strrchrW( appname, '/' ))) appname = p + 1;
    if ((p = strrchrW( appname, '\\' ))) appname = p + 1;

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

    strcpyW( appversion, appdefaultsW );
    strcatW( appversion, appname );
    strcatW( appversion, versionW );
    RtlInitUnicodeString( &nameW, appversion );

    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        VERSION_ParseVersion( hkey, &got_win_ver, &got_dos_ver );
        NtClose( hkey );
    }

    if (got_win_ver && got_dos_ver) goto done;

    RtlInitUnicodeString( &nameW, versionW + 1 );
    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        VERSION_ParseVersion( hkey, &got_win_ver, &got_dos_ver );
        NtClose( hkey );
    }

 done:
    NtClose( config_key );
}


/**********************************************************************
 *	VERSION_GetSystemDLLVersion
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
		  default:
			  FIXME("Unknown DLL OS version, please report !!\n");
			  return WINXP;
	      }
	    }
        }
    }
    return WIN95;
}
/**********************************************************************
 *	VERSION_GetLinkedDllVersion
 *
 * Some version data (not reliable!):
 * linker/OS/image/subsys
 *
 * x.xx/1.00/0.00/3.10	Win32s		(any version ?)
 * 2.39/1.00/0.00/3.10	Win32s		freecell.exe (any version)
 * 2.50/1.00/4.00/4.00	Win32s 1.30	winhlp32.exe
 * 2.60/3.51/3.51/3.51	NT351SP5	system dlls
 * 2.60/3.51/3.51/4.00	NT351SP5	comctl32 dll
 * 2.xx/1.00/0.00/4.00	Win95 		system files
 * x.xx/4.00/0.00/4.00	Win95		most applications
 * 3.10/4.00/0.00/4.00	Win98		notepad
 * x.xx/5.00/5.00/4.00	Win98 		system dlls (e.g. comctl32.dll)
 * x.xx/4.00/4.00/4.00	NT 4 		most apps
 * 5.12/5.00/5.00/4.00	NT4+IE5		comctl32.dll
 * 5.12/5.00/5.00/4.00	Win98		calc
 * x.xx/5.00/5.00/4.00	win95/win98/NT4	IE5 files
 */
static DWORD VERSION_GetLinkedDllVersion(void)
{
	DWORD WinVersion = NB_WINDOWS_VERSIONS;
	PIMAGE_OPTIONAL_HEADER ophd;
        IMAGE_NT_HEADERS *nt;
        ULONG       count, required;
        SYSTEM_MODULE_INFORMATION*  smi;

	/* First check the native dlls provided. These have to be
	from one windows version */
        smi = (SYSTEM_MODULE_INFORMATION*)&count;
        LdrQueryProcessModuleInformation(smi, sizeof(count), &required);
        smi = RtlAllocateHeap(ntdll_get_process_heap(), 0, required);
        if (smi)
        {
            if (LdrQueryProcessModuleInformation(smi, required, NULL) == STATUS_SUCCESS)
            {
                int i, k;
                for (k = 0; k < smi->ModulesCount; k++)
                {
                    nt = RtlImageNtHeader(smi->Modules[k].ImageBaseAddress);
                    ophd = &nt->OptionalHeader;

                    TRACE("%s: %02x.%02x/%02x.%02x/%02x.%02x/%02x.%02x\n",
                          &smi->Modules[k].Name[smi->Modules[k].NameOffset],
                          ophd->MajorLinkerVersion, ophd->MinorLinkerVersion,
                          ophd->MajorOperatingSystemVersion, ophd->MinorOperatingSystemVersion,
                          ophd->MajorImageVersion, ophd->MinorImageVersion,
                          ophd->MajorSubsystemVersion, ophd->MinorSubsystemVersion);

                    /* test if it is an external (native) dll */
                    if (smi->Modules[k].Flags & LDR_WINE_INTERNAL) continue;

                    for (i = 0; special_dlls[i]; i++)
                    {
                        /* test if it is a special dll */
                        if (!strcasecmp(&smi->Modules[k].Name[smi->Modules[k].NameOffset], special_dlls[i]))
                        {
                            DWORD DllVersion = VERSION_GetSystemDLLVersion(smi->Modules[k].ImageBaseAddress);
                            if (WinVersion == NB_WINDOWS_VERSIONS)
                                WinVersion = DllVersion;
                            else 
                            {
                                if (WinVersion != DllVersion) {
                                    ERR("You mixed system DLLs from different windows versions! Expect a crash! (%s: expected version '%s', but is '%s')\n",
                                        &smi->Modules[k].Name[smi->Modules[k].NameOffset],
                                        VersionData[WinVersion].getVersionEx.szCSDVersion,
                                        VersionData[DllVersion].getVersionEx.szCSDVersion);
                                    return WIN20; /* this may let the exe exiting */
                                }
                            }
                            break;
                        }
                    }
                }
            }
            RtlFreeHeap(ntdll_get_process_heap(), 0, smi);
        }

	if(WinVersion != NB_WINDOWS_VERSIONS) return WinVersion;

	/* we are using no external system dlls, look at the exe */
        nt = RtlImageNtHeader(GetModuleHandleA(NULL));
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
static WINDOWS_VERSION VERSION_GetVersion(void)
{
    static WORD winver = 0xffff;

    if (winver == 0xffff) /* to be determined */
    {
        WINDOWS_VERSION retver;

        VERSION_Init();
        if (versionForced) /* user has overridden any sensible checks */
	    winver = forcedWinVersion;
	else
	{
	    retver = VERSION_GetLinkedDllVersion();

	    /* cache determined value, but do not store in case of WIN31 */
	    if (retver != WIN31) winver = retver;

	    return retver;
	}
    }

    return winver;
}


/***********************************************************************
 *         GetVersion   (KERNEL.3)
 */
LONG WINAPI GetVersion16(void)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    TRACE("<-- %s (%s)\n", VersionData[ver].human_readable, VersionData[ver].getVersionEx.szCSDVersion);
    return VersionData[ver].getVersion16;
}


/***********************************************************************
 *         GetVersion   (KERNEL32.@)
 */
LONG WINAPI GetVersion(void)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    TRACE("<-- %s (%s)\n", VersionData[ver].human_readable, VersionData[ver].getVersionEx.szCSDVersion);
    return VersionData[ver].getVersion32;
}


/***********************************************************************
 *         GetVersionEx   (KERNEL.149)
 */
BOOL16 WINAPI GetVersionEx16(OSVERSIONINFO16 *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    if (v->dwOSVersionInfoSize < sizeof(OSVERSIONINFO16))
    {
        WARN("wrong OSVERSIONINFO size from app\n");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    strcpy( v->szCSDVersion, VersionData[ver].getVersionEx.szCSDVersion );
    TRACE("<-- %s (%s)\n", VersionData[ver].human_readable, VersionData[ver].getVersionEx.szCSDVersion);
    return TRUE;
}


/***********************************************************************
 *         GetVersionExA   (KERNEL32.@)
 */
BOOL WINAPI GetVersionExA(OSVERSIONINFOA *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    LPOSVERSIONINFOEXA vex;

    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA) &&
	v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA))
    {
        WARN("wrong OSVERSIONINFO size from app (got: %ld, expected: %d or %d)\n",
                        v->dwOSVersionInfoSize, sizeof(OSVERSIONINFOA),
			sizeof(OSVERSIONINFOEXA));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    strcpy( v->szCSDVersion, VersionData[ver].getVersionEx.szCSDVersion );
    if(v->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA)) {
	vex = (LPOSVERSIONINFOEXA) v;
	vex->wServicePackMajor = VersionData[ver].getVersionEx.wServicePackMajor;
	vex->wServicePackMinor = VersionData[ver].getVersionEx.wServicePackMinor;
	vex->wSuiteMask = VersionData[ver].getVersionEx.wSuiteMask;
	vex->wProductType = VersionData[ver].getVersionEx.wProductType;
    }
    TRACE("<-- %s (%s)\n", VersionData[ver].human_readable, VersionData[ver].getVersionEx.szCSDVersion);
    return TRUE;
}


/***********************************************************************
 *         GetVersionExW   (KERNEL32.@)
 */
BOOL WINAPI GetVersionExW(OSVERSIONINFOW *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    LPOSVERSIONINFOEXW vex;

    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW) &&
	v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW))
    {
        WARN("wrong OSVERSIONINFO size from app (got: %ld, expected: %d or %d)\n",
			v->dwOSVersionInfoSize, sizeof(OSVERSIONINFOW),
			sizeof(OSVERSIONINFOEXW));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    MultiByteToWideChar( CP_ACP, 0, VersionData[ver].getVersionEx.szCSDVersion, -1,
                         v->szCSDVersion, sizeof(v->szCSDVersion)/sizeof(WCHAR) );
    if(v->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW)) {
	vex = (LPOSVERSIONINFOEXW) v;
	vex->wServicePackMajor = VersionData[ver].getVersionEx.wServicePackMajor;
	vex->wServicePackMinor = VersionData[ver].getVersionEx.wServicePackMinor;
	vex->wSuiteMask = VersionData[ver].getVersionEx.wSuiteMask;
	vex->wProductType = VersionData[ver].getVersionEx.wProductType;
    }
    TRACE("<-- %s (%s)\n", VersionData[ver].human_readable, VersionData[ver].getVersionEx.szCSDVersion);
    return TRUE;
}


/******************************************************************************
 *        VerifyVersionInfoA   (KERNEL32.@)
 */
BOOL WINAPI VerifyVersionInfoA( LPOSVERSIONINFOEXA lpVersionInfo, DWORD dwTypeMask,
                                DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW verW;

    verW.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    verW.dwMajorVersion = lpVersionInfo->dwMajorVersion;
    verW.dwMinorVersion = lpVersionInfo->dwMinorVersion;
    verW.dwBuildNumber = lpVersionInfo->dwBuildNumber;
    verW.dwPlatformId = lpVersionInfo->dwPlatformId;
    verW.wServicePackMajor = lpVersionInfo->wServicePackMajor;
    verW.wServicePackMinor = lpVersionInfo->wServicePackMinor;
    verW.wSuiteMask = lpVersionInfo->wSuiteMask;
    verW.wProductType = lpVersionInfo->wProductType;
    verW.wReserved = lpVersionInfo->wReserved;

    return VerifyVersionInfoW(&verW, dwTypeMask, dwlConditionMask);
}


/******************************************************************************
 *        VerifyVersionInfoW   (KERNEL32.@)
 */
BOOL WINAPI VerifyVersionInfoW( LPOSVERSIONINFOEXW lpVersionInfo, DWORD dwTypeMask,
                                DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW ver;
    BOOL res, error_set;

    FIXME("(%p,%lu,%llx): Not all cases correctly implemented yet\n", lpVersionInfo, dwTypeMask, dwlConditionMask);
    /* FIXME:
	- Check the following special case on Windows (various versions):
	  o lp->wSuiteMask == 0 and ver.wSuiteMask != 0 and VER_AND/VER_OR
	  o lp->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)
	- MSDN talks about some tests being impossible. Check what really happens.
     */

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    if(!GetVersionExW((LPOSVERSIONINFOW) &ver))
	return FALSE;

    res = TRUE;
    error_set = FALSE;
    if(!(dwTypeMask && dwlConditionMask)) {
	res = FALSE;
	SetLastError(ERROR_BAD_ARGUMENTS);
	error_set = TRUE;
    }
    if(dwTypeMask & VER_PRODUCT_TYPE)
	switch(dwlConditionMask >> 7*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.wProductType != lpVersionInfo->wProductType)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.wProductType <= lpVersionInfo->wProductType)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.wProductType < lpVersionInfo->wProductType)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.wProductType >= lpVersionInfo->wProductType)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.wProductType > lpVersionInfo->wProductType)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_SUITENAME && res)
	switch(dwlConditionMask >> 6*3 & 0x07) {
	    case VER_AND:
		if((lpVersionInfo->wSuiteMask & ver.wSuiteMask) != lpVersionInfo->wSuiteMask)
		    res = FALSE;
		break;
	    case VER_OR:
		if(!(lpVersionInfo->wSuiteMask & ver.wSuiteMask) && lpVersionInfo->wSuiteMask)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_PLATFORMID && res)
	switch(dwlConditionMask >> 3*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.dwPlatformId != lpVersionInfo->dwPlatformId)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.dwPlatformId <= lpVersionInfo->dwPlatformId)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.dwPlatformId < lpVersionInfo->dwPlatformId)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.dwPlatformId >= lpVersionInfo->dwPlatformId)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.dwPlatformId > lpVersionInfo->dwPlatformId)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_BUILDNUMBER && res)
	switch(dwlConditionMask >> 2*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.dwBuildNumber != lpVersionInfo->dwBuildNumber)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.dwBuildNumber <= lpVersionInfo->dwBuildNumber)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.dwBuildNumber < lpVersionInfo->dwBuildNumber)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.dwBuildNumber >= lpVersionInfo->dwBuildNumber)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.dwBuildNumber > lpVersionInfo->dwBuildNumber)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_MAJORVERSION && res)
	switch(dwlConditionMask >> 1*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.dwMajorVersion != lpVersionInfo->dwMajorVersion)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.dwMajorVersion <= lpVersionInfo->dwMajorVersion)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.dwMajorVersion < lpVersionInfo->dwMajorVersion)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.dwMajorVersion >= lpVersionInfo->dwMajorVersion)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.dwMajorVersion > lpVersionInfo->dwMajorVersion)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_MINORVERSION && res)
	switch(dwlConditionMask >> 0*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.dwMinorVersion != lpVersionInfo->dwMinorVersion)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.dwMinorVersion <= lpVersionInfo->dwMinorVersion)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.dwMinorVersion < lpVersionInfo->dwMinorVersion)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.dwMinorVersion >= lpVersionInfo->dwMinorVersion)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.dwMinorVersion > lpVersionInfo->dwMinorVersion)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_SERVICEPACKMAJOR && res)
	switch(dwlConditionMask >> 5*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.wServicePackMajor != lpVersionInfo->wServicePackMajor)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.wServicePackMajor <= lpVersionInfo->wServicePackMajor)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.wServicePackMajor < lpVersionInfo->wServicePackMajor)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.wServicePackMajor >= lpVersionInfo->wServicePackMajor)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.wServicePackMajor > lpVersionInfo->wServicePackMajor)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}
    if(dwTypeMask & VER_SERVICEPACKMINOR && res)
	switch(dwlConditionMask >> 4*3 & 0x07) {
	    case VER_EQUAL:
		if(ver.wServicePackMinor != lpVersionInfo->wServicePackMinor)
		    res = FALSE;
		break;
	    case VER_GREATER:
		if(ver.wServicePackMinor <= lpVersionInfo->wServicePackMinor)
		    res = FALSE;
		break;
	    case VER_GREATER_EQUAL:
		if(ver.wServicePackMinor < lpVersionInfo->wServicePackMinor)
		    res = FALSE;
		break;
	    case VER_LESS:
		if(ver.wServicePackMinor >= lpVersionInfo->wServicePackMinor)
		    res = FALSE;
		break;
	    case VER_LESS_EQUAL:
		if(ver.wServicePackMinor > lpVersionInfo->wServicePackMinor)
		    res = FALSE;
		break;
	    default:
	        res = FALSE;
		SetLastError(ERROR_BAD_ARGUMENTS);
		error_set = TRUE;
	}

    if(!(res || error_set))
	SetLastError(ERROR_OLD_WIN_VERSION);
    return res;
}


/***********************************************************************
 *	    GetWinFlags   (KERNEL.132)
 */
DWORD WINAPI GetWinFlags16(void)
{
  static const long cpuflags[5] =
    { WF_CPU086, WF_CPU186, WF_CPU286, WF_CPU386, WF_CPU486 };
  SYSTEM_INFO si;
  OSVERSIONINFOA ovi;
  DWORD result;

  GetSystemInfo(&si);

  /* There doesn't seem to be any Pentium flag.  */
  result = cpuflags[min(si.wProcessorLevel, 4)] | WF_ENHANCED | WF_PMODE | WF_80x87 | WF_PAGING;
  if (si.wProcessorLevel >= 4) result |= WF_HASCPUID;
  ovi.dwOSVersionInfoSize = sizeof(ovi);
  GetVersionExA(&ovi);
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
      result |= WF_WIN32WOW; /* undocumented WF_WINNT */
  return result;
}


#if 0
/* Not used at this time. This is here for documentation only */

/* WINDEBUGINFO flags values */
#define WDI_OPTIONS         0x0001
#define WDI_FILTER          0x0002
#define WDI_ALLOCBREAK      0x0004

/* dwOptions values */
#define DBO_CHECKHEAP       0x0001
#define DBO_BUFFERFILL      0x0004
#define DBO_DISABLEGPTRAPPING 0x0010
#define DBO_CHECKFREE       0x0020

#define DBO_SILENT          0x8000

#define DBO_TRACEBREAK      0x2000
#define DBO_WARNINGBREAK    0x1000
#define DBO_NOERRORBREAK    0x0800
#define DBO_NOFATALBREAK    0x0400
#define DBO_INT3BREAK       0x0100

/* DebugOutput flags values */
#define DBF_TRACE           0x0000
#define DBF_WARNING         0x4000
#define DBF_ERROR           0x8000
#define DBF_FATAL           0xc000

/* dwFilter values */
#define DBF_KERNEL          0x1000
#define DBF_KRN_MEMMAN      0x0001
#define DBF_KRN_LOADMODULE  0x0002
#define DBF_KRN_SEGMENTLOAD 0x0004
#define DBF_USER            0x0800
#define DBF_GDI             0x0400
#define DBF_MMSYSTEM        0x0040
#define DBF_PENWIN          0x0020
#define DBF_APPLICATION     0x0008
#define DBF_DRIVER          0x0010

#endif /* NOLOGERROR */


/***********************************************************************
 *	    GetWinDebugInfo   (KERNEL.355)
 */
BOOL16 WINAPI GetWinDebugInfo16(WINDEBUGINFO16 *lpwdi, UINT16 flags)
{
    FIXME("(%8lx,%d): stub returning 0\n",
	  (unsigned long)lpwdi, flags);
    /* 0 means not in debugging mode/version */
    /* Can this type of debugging be used in wine ? */
    /* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
    return 0;
}


/***********************************************************************
 *	    SetWinDebugInfo   (KERNEL.356)
 */
BOOL16 WINAPI SetWinDebugInfo16(WINDEBUGINFO16 *lpwdi)
{
    FIXME("(%8lx): stub returning 0\n", (unsigned long)lpwdi);
    /* 0 means not in debugging mode/version */
    /* Can this type of debugging be used in wine ? */
    /* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
    return 0;
}


/***********************************************************************
 *           K329                    (KERNEL.329)
 *
 * TODO:
 * Should fill lpBuffer only if DBO_BUFFERFILL has been set by SetWinDebugInfo()
 */
void WINAPI DebugFillBuffer(LPSTR lpBuffer, WORD wBytes)
{
	memset(lpBuffer, DBGFILL_BUFFER, wBytes);
}

/***********************************************************************
 *           DiagQuery                          (KERNEL.339)
 *
 * returns TRUE if Win called with "/b" (bootlog.txt)
 */
BOOL16 WINAPI DiagQuery16(void)
{
	/* perhaps implement a Wine "/b" command line flag sometime ? */
	return FALSE;
}

/***********************************************************************
 *           DiagOutput                         (KERNEL.340)
 *
 * writes a debug string into <windir>\bootlog.txt
 */
void WINAPI DiagOutput16(LPCSTR str)
{
        /* FIXME */
	DPRINTF("DIAGOUTPUT:%s\n", debugstr_a(str));
}
