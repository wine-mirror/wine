/*
 * Windows version functions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1997 Marcus Meissner
 */

#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "process.h"
#include "options.h"
#include "debug.h"

typedef enum
{
    WIN31, /* Windows 3.1 */
    WIN95, /* Windows 95 */
    NT351, /* Windows NT 3.51 */
    NT40,  /* Windows NT 4.0 */
    NB_VERSIONS
} VERSION;

typedef struct
{
    LONG             getVersion16; 
    LONG             getVersion32;
    OSVERSIONINFO32A getVersionEx;
} VERSION_DATA;


/* FIXME: compare values below with original and fix */
static const VERSION_DATA VersionData[NB_VERSIONS] =
{
    /* WIN31 */
    {
	MAKELONG( 0x0a03, 0x0616 ), /* DOS 6.22 */
	MAKELONG( 0x0a03, 0x8000 ),
	{
            sizeof(OSVERSIONINFO32A), 3, 10, 0,
            VER_PLATFORM_WIN32s, "Win32s 1.3" 
	}
    },
    /* WIN95 */
    {
        0x07005F03,
        0xC0000004,
	{
            sizeof(OSVERSIONINFO32A), 4, 0, 0x40003B6,
            VER_PLATFORM_WIN32_WINDOWS, "Win95"
	}
    },
    /* NT351 */
    {
        0x05000A03,
        0x04213303,
        {
            sizeof(OSVERSIONINFO32A), 3, 51, 0x421,
            VER_PLATFORM_WIN32_NT, "Service Pack 2"
	}
    },
    /* NT40 */
    {
        0x05000A03,
        0x05650004,
        {
            sizeof(OSVERSIONINFO32A), 4, 0, 0x565,
            VER_PLATFORM_WIN32_NT, "Service Pack 3"
        }
    }
};

static const char *VersionNames[NB_VERSIONS] =
{
    "win31",
    "win95",
    "nt351",
    "nt40"
};

/* the current version has not been autodetected but forced via cmdline */
static BOOL32 versionForced = FALSE;
static VERSION defaultVersion = WIN31;


/**********************************************************************
 *         VERSION_ParseVersion
 */
void VERSION_ParseVersion( char *arg )
{
    int i;
    for (i = 0; i < NB_VERSIONS; i++)
    {
        if (!strcmp( VersionNames[i], arg ))
        {
            defaultVersion = (VERSION)i;
            versionForced = TRUE;
            return;
        }
    }
    MSG("Invalid winver value '%s' specified.\n", arg );
    MSG("Valid versions are:" );
    for (i = 0; i < NB_VERSIONS; i++)
        MSG(" '%s'%c", VersionNames[i],
	    (i == NB_VERSIONS - 1) ? '\n' : ',' );
}


/**********************************************************************
 *         VERSION_get_version
 */
static VERSION VERSION_GetVersion(void)
{
    LPIMAGE_NT_HEADERS peheader;	

    if (versionForced) /* user has overridden any sensible checks */
        return defaultVersion;
    if (!PROCESS_Current()->exe_modref)
    {
        /* HACK: if we have loaded a PE image into this address space,
         * we are probably using thunks, so Win95 is our best bet
         */
        if (PROCESS_Current()->modref_list) return WIN95;
        return WIN31; /* FIXME: hmm, look at DDB.version ? */
    }
    peheader = PE_HEADER(PROCESS_Current()->exe_modref->module);
    if (peheader->OptionalHeader.MajorSubsystemVersion == 4)
        /* FIXME: NT4 has the same majorversion; add a check here for it. */
        return WIN95;
    if (peheader->OptionalHeader.MajorSubsystemVersion == 3)
    {
        /* Win3.10 */
        if (peheader->OptionalHeader.MinorSubsystemVersion <= 11) return WIN31;
        /* NT 3.51 */
        if (peheader->OptionalHeader.MinorSubsystemVersion == 50) return NT351;
        if (peheader->OptionalHeader.MinorSubsystemVersion == 51) return NT351;
    }
    ERR(ver,"unknown subsystem version: %04x.%04x, please report.\n",
	peheader->OptionalHeader.MajorSubsystemVersion,
	peheader->OptionalHeader.MinorSubsystemVersion );
    return defaultVersion;
}


/***********************************************************************
 *         GetVersion16   (KERNEL.3)
 */
LONG WINAPI GetVersion16(void)
{
    VERSION ver = VERSION_GetVersion();
    return VersionData[ver].getVersion16;
}


/***********************************************************************
 *         GetVersion32   (KERNEL32.427)
 */
LONG WINAPI GetVersion32(void)
{
    VERSION ver = VERSION_GetVersion();
    return VersionData[ver].getVersion32;
}


/***********************************************************************
 *         GetVersionEx32A   (KERNEL32.428)
 */
BOOL32 WINAPI GetVersionEx32A(OSVERSIONINFO32A *v)
{
    VERSION ver = VERSION_GetVersion();
    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFO32A))
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
BOOL32 WINAPI GetVersionEx32W(OSVERSIONINFO32W *v)
{
    VERSION ver = VERSION_GetVersion();
    if (v->dwOSVersionInfoSize!=sizeof(OSVERSIONINFO32W))
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
DWORD WINAPI GetWinFlags(void)
{
  static const long cpuflags[5] =
    { WF_CPU086, WF_CPU186, WF_CPU286, WF_CPU386, WF_CPU486 };
  SYSTEM_INFO si;
  OSVERSIONINFO32A ovi;
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
  GetVersionEx32A(&ovi);
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
      result |= WF_WIN32WOW; /* undocumented WF_WINNT */
  return result;
}


/***********************************************************************
 *	    GetWinDebugInfo   (KERNEL.355)
 */
BOOL16 WINAPI GetWinDebugInfo(WINDEBUGINFO *lpwdi, UINT16 flags)
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
BOOL16 WINAPI SetWinDebugInfo(WINDEBUGINFO *lpwdi)
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
BOOL16 WINAPI DiagQuery()
{
	/* perhaps implement a Wine "/b" command line flag sometime ? */
	return FALSE;
}

/***********************************************************************
 *           DiagOutput                         (KERNEL.340)
 *
 * writes a debug string into <windir>\bootlog.txt
 */
void WINAPI DiagOutput(LPCSTR str)
{
        /* FIXME */
	DPRINTF("DIAGOUTPUT:%s\n",str);
}
