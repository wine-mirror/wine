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
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);


/***********************************************************************
 *         GetVersion   (KERNEL.3)
 */
DWORD WINAPI GetVersion16(void)
{
    static WORD dosver, winver;

    if (!dosver)  /* not determined yet */
    {
        RTL_OSVERSIONINFOEXW info;

        info.dwOSVersionInfoSize = sizeof(info);
        if (RtlGetVersion( &info ) != STATUS_SUCCESS) return 0;

        if (info.dwMajorVersion <= 3)
            winver = MAKEWORD( info.dwMajorVersion, info.dwMinorVersion );
        else
            winver = MAKEWORD( 3, 95 );

        switch(info.dwPlatformId)
        {
        case VER_PLATFORM_WIN32s:
            switch(MAKELONG( info.dwMinorVersion, info.dwMajorVersion ))
            {
            case 0x0200:
                dosver = 0x0303;  /* DOS 3.3 for Windows 2.0 */
                break;
            case 0x0300:
                dosver = 0x0500;  /* DOS 5.0 for Windows 3.0 */
                break;
            default:
                dosver = 0x0616;  /* DOS 6.22 for Windows 3.1 and later */
                break;
            }
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            /* DOS 8.0 for WinME, 7.0 for Win95/98 */
            if (info.dwMinorVersion >= 90) dosver = 0x0800;
            else dosver = 0x0700;
            break;
        case VER_PLATFORM_WIN32_NT:
            dosver = 0x0500;  /* always DOS 5.0 for NT */
            break;
        }
        TRACE( "DOS %d.%02d Win %d.%02d\n",
               HIBYTE(dosver), LOBYTE(dosver), LOBYTE(winver), HIBYTE(winver) );
    }
    return MAKELONG( winver, dosver );
}


/***********************************************************************
 *         GetVersion   (KERNEL32.@)
 *
 * Win31   0x80000a03
 * Win95   0xc0000004
 * Win98   0xc0000a04
 * WinME   0xc0005a04
 * NT351   0x04213303
 * NT4     0x05650004
 * Win2000 0x08930005
 * WinXP   0x0a280105
 */
DWORD WINAPI GetVersion(void)
{
    DWORD result = MAKELONG( MAKEWORD( NtCurrentTeb()->Peb->OSMajorVersion,
                                       NtCurrentTeb()->Peb->OSMinorVersion ),
                             (NtCurrentTeb()->Peb->OSPlatformId ^ 2) << 14 );
    if (NtCurrentTeb()->Peb->OSPlatformId == VER_PLATFORM_WIN32_NT)
        result |= LOWORD(NtCurrentTeb()->Peb->OSBuildNumber) << 16;
    return result;
}


/***********************************************************************
 *         GetVersionEx   (KERNEL.149)
 */
BOOL16 WINAPI GetVersionEx16(OSVERSIONINFO16 *v)
{
    OSVERSIONINFOA info;

    if (v->dwOSVersionInfoSize < sizeof(OSVERSIONINFO16))
    {
        WARN("wrong OSVERSIONINFO size from app\n");
        return FALSE;
    }

    info.dwOSVersionInfoSize = sizeof(info);
    if (!GetVersionExA( &info )) return FALSE;

    v->dwMajorVersion = info.dwMajorVersion;
    v->dwMinorVersion = info.dwMinorVersion;
    v->dwBuildNumber  = info.dwBuildNumber;
    v->dwPlatformId   = info.dwPlatformId;
    strcpy( v->szCSDVersion, info.szCSDVersion );
    return TRUE;
}


/***********************************************************************
 *         GetVersionExA   (KERNEL32.@)
 */
BOOL WINAPI GetVersionExA(OSVERSIONINFOA *v)
{
    RTL_OSVERSIONINFOEXW infoW;

    if (v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA) &&
        v->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA))
    {
        WARN("wrong OSVERSIONINFO size from app (got: %d)\n",
                        v->dwOSVersionInfoSize );
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    infoW.dwOSVersionInfoSize = sizeof(infoW);
    if (RtlGetVersion( &infoW ) != STATUS_SUCCESS) return FALSE;

    v->dwMajorVersion = infoW.dwMajorVersion;
    v->dwMinorVersion = infoW.dwMinorVersion;
    v->dwBuildNumber  = infoW.dwBuildNumber;
    v->dwPlatformId   = infoW.dwPlatformId;
    WideCharToMultiByte( CP_ACP, 0, infoW.szCSDVersion, -1,
                         v->szCSDVersion, sizeof(v->szCSDVersion), NULL, NULL );

    if(v->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA))
    {
        LPOSVERSIONINFOEXA vex = (LPOSVERSIONINFOEXA) v;
        vex->wServicePackMajor = infoW.wServicePackMajor;
        vex->wServicePackMinor = infoW.wServicePackMinor;
        vex->wSuiteMask        = infoW.wSuiteMask;
        vex->wProductType      = infoW.wProductType;
    }
    return TRUE;
}


/***********************************************************************
 *         GetVersionExW   (KERNEL32.@)
 */
BOOL WINAPI GetVersionExW( OSVERSIONINFOW *info )
{
    if (info->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW) &&
        info->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW))
    {
        WARN("wrong OSVERSIONINFO size from app (got: %d)\n",
                        info->dwOSVersionInfoSize);
        return FALSE;
    }
    return (RtlGetVersion( (RTL_OSVERSIONINFOEXW *)info ) == STATUS_SUCCESS);
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
    switch(RtlVerifyVersionInfo( lpVersionInfo, dwTypeMask, dwlConditionMask ))
    {
    case STATUS_INVALID_PARAMETER:
        SetLastError( ERROR_BAD_ARGUMENTS );
        return FALSE;
    case STATUS_REVISION_MISMATCH:
        SetLastError( ERROR_OLD_WIN_VERSION );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *          GetWinFlags   (KERNEL.132)
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
 *          GetWinDebugInfo   (KERNEL.355)
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
 *          SetWinDebugInfo   (KERNEL.356)
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
        TRACE("DIAGOUTPUT:%s\n", debugstr_a(str));
}

/***********************************************************************
 *           TermsrvAppInstallMode       (KERNEL32.@)
 *
 * Find out whether the terminal server is in INSTALL or EXECUTE mode.
 */
BOOL WINAPI TermsrvAppInstallMode(void)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           SetTermsrvAppInstallMode       (KERNEL32.@)
 *
 * This function is said to switch between the INSTALL (TRUE) or
 * EXECUTE (FALSE) terminal server modes.
 *
 * This function always returns zero on WinXP Home so it's probably
 * safe to return that value in most cases. However, if a terminal
 * server is running it will probably return something else.
 */
DWORD WINAPI SetTermsrvAppInstallMode(BOOL bInstallMode)
{
    FIXME("(%d): stub\n", bInstallMode);
    return 0;
}
