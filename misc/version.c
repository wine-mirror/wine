/*
 * Windows and DOS version functions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Patrik Stridvall
 * Copyright 1998 Andreas Mohr
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "process.h"
#include "options.h"
#include "debugtools.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(ver);

typedef enum
{
    WIN20,   /* Windows 2.0 */
    WIN30,   /* Windows 3.0 */
    WIN31,   /* Windows 3.1 */
    WIN95,   /* Windows 95 */
    WIN98,   /* Windows 98 */
 /* insert Windows ME here as WINME if needed */
    NT351,   /* Windows NT 3.51 */
    NT40,    /* Windows NT 4.0 */  
    NT2K,    /* Windows 2000 */
    NB_WINDOWS_VERSIONS
} WINDOWS_VERSION;

typedef struct
{
    LONG             getVersion16; 
    LONG             getVersion32;
    OSVERSIONINFOA getVersionEx;
} VERSION_DATA;

/* FIXME: compare values below with original and fix */
static VERSION_DATA VersionData[NB_WINDOWS_VERSIONS] =
{
    /* WIN20 FIXME: verify values */
    {
	MAKELONG( 0x0002, 0x0303 ), /* assume DOS 3.3 */
	MAKELONG( 0x0003, 0x8000 ),
	{
            sizeof(OSVERSIONINFOA), 2, 0, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3" 
	}
    },
    /* WIN30 FIXME: verify values */
    {
	MAKELONG( 0x0003, 0x0500 ), /* assume DOS 5.00 */
	MAKELONG( 0x0003, 0x8000 ),
	{
            sizeof(OSVERSIONINFOA), 3, 0, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3" 
	}
    },
    /* WIN31 */
    {
	MAKELONG( 0x0a03, 0x0616 ), /* DOS 6.22 */
	MAKELONG( 0x0a03, 0x8000 ),
	{
            sizeof(OSVERSIONINFOA), 3, 10, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3" 
	}
    },
    /* WIN95 */
    {
        0x07005F03,
        0xC0000004,
	{
            sizeof(OSVERSIONINFOA), 4, 0, 0x40003B6,
            VER_PLATFORM_WIN32_WINDOWS, "Win95"
	}
    },
    /* WIN98 */
    {
	0x070A5F03,
	0xC0000A04,
	{
	    sizeof(OSVERSIONINFOA), 4, 10, 0x40A07CE,
	    VER_PLATFORM_WIN32_WINDOWS, "Win98"
	}
    },
    /* NT351 */
    {
        0x05000A03,
        0x04213303,
        {
            sizeof(OSVERSIONINFOA), 3, 51, 0x421,
            VER_PLATFORM_WIN32_NT, "Service Pack 2"
	}
    },
    /* NT40 */
    {
        0x05000A03,
        0x05650004,
        {
            sizeof(OSVERSIONINFOA), 4, 0, 0x565,
            VER_PLATFORM_WIN32_NT, "Service Pack 6"
        }
    },
    /* NT2K FIXME: verify values */
    {
        0x05000A03, /* ? */
        0x08930005, /* better build ? using 2195 (final) for now */
        {
            sizeof(OSVERSIONINFOA), 5, 0, 0x893,
            VER_PLATFORM_WIN32_NT, "FIXME: OS version string not known yet"
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
    "nt351",
    "nt40",
    "win2000,win2k,nt2k,nt2000"
};

/* if one of the following dlls is importing ntdll the windows
version autodetection switches wine to unicode (nt 3.51 or 4.0) */
static char * special_dlls[] =
{
	"COMDLG32",
	"COMCTL32",
	"SHELL32",
	"OLE32",
	"RPCRT4",
	NULL
};

/* the current version has not been autodetected but forced via cmdline */
static BOOL versionForced = FALSE;
static WINDOWS_VERSION defaultWinVersion = WIN31;

/**********************************************************************
 *         VERSION_ParseWinVersion
 */
void VERSION_ParseWinVersion( const char *arg )
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
                defaultWinVersion = (WINDOWS_VERSION)i;
                versionForced = TRUE;
                return;
            }
            pCurr = p+1;
        } while (p);
    }
    MESSAGE("Invalid winver value '%s' specified.\n", arg );
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
void VERSION_ParseDosVersion( const char *arg )
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
        MESSAGE("--dosver: Wrong version format. Use \"--dosver x.xx\"\n");
        ExitProcess(1);
    }
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
    IMAGE_DATA_DIRECTORY *dir = PE_HEADER(hmod)->OptionalHeader.DataDirectory
                                + IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (dir->Size && dir->VirtualAddress)
    {
        IMAGE_IMPORT_DESCRIPTOR *pe_imp = (IMAGE_IMPORT_DESCRIPTOR *)((char *)hmod + dir->VirtualAddress);
        for ( ; pe_imp->Name; pe_imp++)
        {
	    char * name = (char *)hmod + (unsigned int)pe_imp->Name;
	    TRACE("%s\n", name);
	    
	    if (!strncasecmp(name, "ntdll", 5))
	    {
	      if (3 == PE_HEADER(hmod)->OptionalHeader.MajorOperatingSystemVersion)
	        return NT351;
	      else
	        return NT40;
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
 * x.xx/5.00/5.00/4.00	Win98 		system dlls
 * x.xx/4.00/4.00/4.00	NT 4 		most apps
 * 5.12/5.00/5.00/4.00	NT4+IE5		comctl32.dll
 * 5.12/5.00/5.00/4.00	Win98		calc
 * x.xx/5.00/5.00/4.00	win95/win98/NT4	IE5 files
 */
DWORD VERSION_GetLinkedDllVersion(PDB *pdb)
{
	WINE_MODREF *wm;
	DWORD WinVersion = NB_WINDOWS_VERSIONS;
	PIMAGE_OPTIONAL_HEADER ophd;

	if (!pdb->exe_modref)
	{
	  if (!pdb->modref_list)
            return WIN31;

	  /* FIXME: The above condition will never trigger, since all our
	   * standard dlls load their win32 equivalents. We have usually at
	   * this point: kernel32.dll and ntdll.dll.
	   */
	  return WIN95;
	}
	/* First check the native dlls provided. These have to be
	from one windows version */
	for ( wm = pdb->modref_list; wm; wm=wm->next )
	{
	  ophd = &(PE_HEADER(wm->module)->OptionalHeader);

	  TRACE("%s: %02x.%02x/%02x.%02x/%02x.%02x/%02x.%02x\n",
	    wm->modname,
	    ophd->MajorLinkerVersion, ophd->MinorLinkerVersion,
	    ophd->MajorOperatingSystemVersion, ophd->MinorOperatingSystemVersion,
	    ophd->MajorImageVersion, ophd->MinorImageVersion,
	    ophd->MajorSubsystemVersion, ophd->MinorSubsystemVersion);

	  /* test if it is an external (native) dll */
	  if (!(wm->flags & WINE_MODREF_INTERNAL))
	  {
	    int i;
	    for (i = 0; special_dlls[i]; i++)
	    {
	      /* test if it is a special dll */
	      if (!strncasecmp(wm->modname, special_dlls[i], strlen(special_dlls[i]) ))
	      {
	        DWORD DllVersion = VERSION_GetSystemDLLVersion(wm->module);
	        if (WinVersion == NB_WINDOWS_VERSIONS) 
	          WinVersion = DllVersion;
	        else {
	          if (WinVersion != DllVersion) {
	            ERR("You mixed system dlls from different windows versions! Expect a crash! (%s: expected version '%s', but is '%s')\n",
			wm->modname,
			VersionData[WinVersion].getVersionEx.szCSDVersion,
			VersionData[DllVersion].getVersionEx.szCSDVersion);
	            return WIN31; /* this may let the exe exiting */
	          }
	        }
	        break;
	      }
	    }
	  }
	}
	
	if(WinVersion != NB_WINDOWS_VERSIONS) return WinVersion;
	
	/* we are using no external system dlls, look at the exe */
	ophd = &(PE_HEADER(pdb->exe_modref->module)->OptionalHeader);
	
	TRACE("-%s: %02x.%02x/%02x.%02x/%02x.%02x/%02x.%02x\n",
	    pdb->exe_modref->modname,
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

	if (versionForced) /* user has overridden any sensible checks */
	  return defaultWinVersion;

	if (winver == 0xffff) /* to be determined */ {
	  WINDOWS_VERSION retver = VERSION_GetLinkedDllVersion( PROCESS_Current() );

	  if (retver != WIN31) winver = retver;
	  return retver;
	}
	return winver;
}


/***********************************************************************
 *         GetVersion16   (KERNEL.3)
 */
LONG WINAPI GetVersion16(void)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    return VersionData[ver].getVersion16;
}


/***********************************************************************
 *         GetVersion   (KERNEL32.427)
 */
LONG WINAPI GetVersion(void)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    return VersionData[ver].getVersion32;
}


/***********************************************************************
 *         GetVersionEx16   (KERNEL.149)
 */
BOOL16 WINAPI GetVersionEx16(OSVERSIONINFO16 *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFO16))
    {
        WARN("wrong OSVERSIONINFO size from app");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    strcpy( v->szCSDVersion, VersionData[ver].getVersionEx.szCSDVersion );
    return TRUE;
}


/***********************************************************************
 *         GetVersionExA   (KERNEL32.428)
 */
BOOL WINAPI GetVersionExA(OSVERSIONINFOA *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA))
    {
        WARN("wrong OSVERSIONINFO size from app (got: %ld, expected: %d)",
                        v->dwOSVersionInfoSize, sizeof(OSVERSIONINFOA));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    strcpy( v->szCSDVersion, VersionData[ver].getVersionEx.szCSDVersion );
    return TRUE;
}


/***********************************************************************
 *         GetVersionExW   (KERNEL32.429)
 */
BOOL WINAPI GetVersionExW(OSVERSIONINFOW *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();

    if (v->dwOSVersionInfoSize!=sizeof(OSVERSIONINFOW))
    {
        WARN("wrong OSVERSIONINFO size from app (got: %ld, expected: %d)",
			v->dwOSVersionInfoSize, sizeof(OSVERSIONINFOW));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    MultiByteToWideChar( CP_ACP, 0, VersionData[ver].getVersionEx.szCSDVersion, -1,
                         v->szCSDVersion, sizeof(v->szCSDVersion)/sizeof(WCHAR) );
    return TRUE;
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


/***********************************************************************
 *	    GetWinDebugInfo   (KERNEL.355)
 */
BOOL16 WINAPI GetWinDebugInfo16(WINDEBUGINFO *lpwdi, UINT16 flags)
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
BOOL16 WINAPI SetWinDebugInfo16(WINDEBUGINFO *lpwdi)
{
    FIXME("(%8lx): stub returning 0\n", (unsigned long)lpwdi);
    /* 0 means not in debugging mode/version */
    /* Can this type of debugging be used in wine ? */
    /* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
    return 0;
}


/***********************************************************************
 *           DebugFillBuffer                    (KERNEL.329)
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
BOOL16 WINAPI DiagQuery16()
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
