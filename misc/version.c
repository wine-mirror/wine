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
#include "winbase.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "process.h"
#include "options.h"
#include "debug.h"
#include "neexe.h"
#include "winversion.h"

DEFAULT_DEBUG_CHANNEL(ver)

typedef struct
{
    LONG             getVersion16; 
    LONG             getVersion32;
    OSVERSIONINFOA getVersionEx;
} VERSION_DATA;


/* FIXME: compare values below with original and fix */
static VERSION_DATA VersionData[NB_WINDOWS_VERSIONS] =
{
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
            VER_PLATFORM_WIN32_NT, "Service Pack 3"
        }
    }
};

static const char *WinVersionNames[NB_WINDOWS_VERSIONS] =
{
    "win31",
    "win95",
    "nt351",
    "nt40"
};

/* the current version has not been autodetected but forced via cmdline */
static BOOL versionForced = FALSE;
static WINDOWS_VERSION defaultWinVersion = WIN31;


/**********************************************************************
 *         VERSION_ParseWinVersion
 */
void VERSION_ParseWinVersion( const char *arg )
{
    int i;
    for (i = 0; i < NB_WINDOWS_VERSIONS; i++)
    {
        if (!strcmp( WinVersionNames[i], arg ))
        {
            defaultWinVersion = (WINDOWS_VERSION)i;
            versionForced = TRUE;
            return;
        }
    }
    MSG("Invalid winver value '%s' specified.\n", arg );
    MSG("Valid versions are:" );
    for (i = 0; i < NB_WINDOWS_VERSIONS; i++)
        MSG(" '%s'%c", WinVersionNames[i],
	    (i == NB_WINDOWS_VERSIONS - 1) ? '\n' : ',' );
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
        fprintf( stderr, "-dosver: Wrong version format. Use \"-dosver x.xx\"\n");
}


/**********************************************************************
 *         VERSION_GetVersion
 *
 * Some version data:
 * linker/OS/image/subsys	Name			Intended for
 *
 * 2.39/1.00/0.00/3.10		freecell.exe	Win32s (any version)
 * 2.55/1.00/0.00/4.00		acrord32.exe	Win32s, Win95 supported (?)
 * 
 * 2.50/1.00/4.00/4.00		winhlp32.exe	Win32s 1.30
 * 4.20/4.00/1.00/4.00		Asuslm.exe		Win95 (Aaargh !)
 * 5.12/4.00/1.07/4.00      clikfixi.exe    NT 4 (service pack files)
 * 3.10/4.00/4.00/4.00		PLUMBING.EXE	NT
 * ?.??/4.00/97.01/4.00		sse.exe			huh ?? (damn crackerz ;)
 * 6.00/5.00/5.00/4.00						NT 4 driver update (strange numbers)
 * 
 * Common versions:
 * x.xx/1.00/0.00/3.10						Win32s (any version ?)
 * 2.xx/1.00/0.00/4.00						Win95 (Microsoft/system files) 
 * x.xx/4.00/0.00/4.00						Win95 (most applications !)
 * x.xx/4.00/4.00/4.00						NT 4 (most apps)
 * x.xx/5.00/5.00/4.00						NT 4 newer files / NT 5 ??
 */
WINDOWS_VERSION VERSION_GetVersion(void)
{
    PIMAGE_NT_HEADERS peheader;	
    PDB *pdb = PROCESS_Current();

    if (versionForced) /* user has overridden any sensible checks */
        return defaultWinVersion;
    if (!pdb->exe_modref)
    {
        /* HACK: if we have loaded a PE image into this address space,
         * we are probably using thunks, so Win95 is our best bet
         */
        if (pdb->modref_list) return WIN95;

        /* FIXME: hmm, do anything else ?
           TDB.version doesn't help here
           as it always holds version 3.10 */
        return WIN31;
    }
    peheader = PE_HEADER(pdb->exe_modref->module);

    TRACE(ver, "%02x.%02x/%02x.%02x/%02x.%02x/%02x.%02x\n",
          peheader->OptionalHeader.MajorLinkerVersion,
          peheader->OptionalHeader.MinorLinkerVersion,
          peheader->OptionalHeader.MajorOperatingSystemVersion,
          peheader->OptionalHeader.MinorOperatingSystemVersion,
          peheader->OptionalHeader.MajorImageVersion,
          peheader->OptionalHeader.MinorImageVersion,
          peheader->OptionalHeader.MajorSubsystemVersion,
          peheader->OptionalHeader.MinorSubsystemVersion);

    if (peheader->OptionalHeader.MajorSubsystemVersion == 4)
    {
	if (peheader->OptionalHeader.MajorOperatingSystemVersion == 4)
        {
            if ((peheader->OptionalHeader.MajorImageVersion == 0) &&
                (peheader->OptionalHeader.SectionAlignment == 4096))
                return WIN95;
	    return NT40;
        }
        if (peheader->OptionalHeader.MajorOperatingSystemVersion == 1) return WIN95;
        if (peheader->OptionalHeader.MajorOperatingSystemVersion == 5)
            return NT40; /* FIXME: this is NT 5, isn't it ? */
    }
    if (peheader->OptionalHeader.MajorSubsystemVersion == 3)
    {
        /* 3.1x versions */
        if (peheader->OptionalHeader.MinorSubsystemVersion <= 11)
        {
            if (peheader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
                return NT351; /* FIXME: NT 3.1 */
            else
		  return WIN31;
        }
        /* NT 3.51 */
        if (peheader->OptionalHeader.MinorSubsystemVersion == 50) return NT351;
        if (peheader->OptionalHeader.MinorSubsystemVersion == 51) return NT351;
    }
    if (peheader->OptionalHeader.MajorSubsystemVersion)
	ERR(ver,"unknown subsystem version: %04x.%04x, please report.\n",
	    peheader->OptionalHeader.MajorSubsystemVersion,
	    peheader->OptionalHeader.MinorSubsystemVersion );
    return defaultWinVersion;
}


/**********************************************************************
 *         VERSION_GetVersionName
 */
char *VERSION_GetVersionName()
{
  WINDOWS_VERSION ver = VERSION_GetVersion();
  switch(ver)
    {
    case WIN31:
      return "Windows 3.1";
    case WIN95:  
      return "Windows 95";
    case NT351:
      return "Windows NT 3.51";
    case NT40:
      return "Windows NT 4.0";
    default:
      FIXME(ver,"Windows version %d not named",ver);
      return "Windows <Unknown>";
    }
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
 *         GetVersion32   (KERNEL32.427)
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
        WARN(ver,"wrong OSVERSIONINFO size from app");
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
 *         GetVersionEx32A   (KERNEL32.428)
 */
BOOL WINAPI GetVersionExA(OSVERSIONINFOA *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();
    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA))
    {
        WARN(ver,"wrong OSVERSIONINFO size from app");
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
 *         GetVersionEx32W   (KERNEL32.429)
 */
BOOL WINAPI GetVersionExW(OSVERSIONINFOW *v)
{
    WINDOWS_VERSION ver = VERSION_GetVersion();

    if (v->dwOSVersionInfoSize!=sizeof(OSVERSIONINFOW))
    {
        WARN(ver,"wrong OSVERSIONINFO size from app");
        return FALSE;
    }
    v->dwMajorVersion = VersionData[ver].getVersionEx.dwMajorVersion;
    v->dwMinorVersion = VersionData[ver].getVersionEx.dwMinorVersion;
    v->dwBuildNumber  = VersionData[ver].getVersionEx.dwBuildNumber;
    v->dwPlatformId   = VersionData[ver].getVersionEx.dwPlatformId;
    lstrcpyAtoW( v->szCSDVersion, VersionData[ver].getVersionEx.szCSDVersion );
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
  result = cpuflags[MIN (si.wProcessorLevel, 4)];

  switch(Options.mode)
  {
  case MODE_STANDARD:
      result |= WF_STANDARD | WF_PMODE | WF_80x87;
      break;

  case MODE_ENHANCED:
      result |= WF_ENHANCED | WF_PMODE | WF_80x87 | WF_PAGING;
      break;

  default:
      ERR(ver, "Unknown mode set? This shouldn't happen. Check GetWinFlags()!\n");
      break;
  }
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
    FIXME(ver, "(%8lx,%d): stub returning 0\n",
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
    FIXME(ver, "(%8lx): stub returning 0\n", (unsigned long)lpwdi);
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

/***********************************************************************
 *           OaBuildVersion           [OLEAUT32.170]
 */
UINT WINAPI OaBuildVersion()
{
    WINDOWS_VERSION ver = VERSION_GetVersion();

    FIXME(ver, "Please report to a.mohr@mailto.de if you get version error messages !\n");
    switch(VersionData[ver].getVersion32)
    {
        case 0x80000a03: /* Win 3.1 */
		return 0x140fd1; /* from Win32s 1.1e */
        case 0xc0000004: /* Win 95 */
		return 0x1e10a9; /* some older version: 0x0a0bd3 */
        case 0x04213303: /* NT 3.51 */
		FIXME(ver, "NT 3.51 version value unknown !\n");
		return 0x1e10a9; /* value borrowed from Win95 */
        case 0x05650004: /* NT 4.0 */
		return 0x141016;
	default:
		return 0x0;
    }
}
/***********************************************************************
 *        VERSION_OsIsUnicode	[internal]
 *
 * NOTES
 *   some functions getting sometimes LPSTR sometimes LPWSTR...
 *
 */
BOOL VERSION_OsIsUnicode(void)
{
    switch(VERSION_GetVersion())
    {
    case NT351:
    case NT40:
        return TRUE;
    default:
        return FALSE;
    }
}
