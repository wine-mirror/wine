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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);


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

/***********************************************************************
 *           GetCurrentPackageId       (KERNEL32.@)
 */
LONG WINAPI GetCurrentPackageId(UINT32 *len, BYTE *buffer)
{
    FIXME("(%p %p): stub\n", len, buffer);
    return APPMODEL_ERROR_NO_PACKAGE;
}

/***********************************************************************
 *           GetCurrentPackageFamilyName       (KERNEL32.@)
 */
LONG WINAPI GetCurrentPackageFamilyName(UINT32 *length, PWSTR name)
{
    FIXME("(%p %p): stub\n", length, name);
    return APPMODEL_ERROR_NO_PACKAGE;
}

/***********************************************************************
 *           GetCurrentPackageFullName       (KERNEL32.@)
 */
LONG WINAPI GetCurrentPackageFullName(UINT32 *length, PWSTR name)
{
    FIXME("(%p %p): stub\n", length, name);
    return APPMODEL_ERROR_NO_PACKAGE;
}

/***********************************************************************
 *           GetPackageFullName       (KERNEL32.@)
 */
LONG WINAPI GetPackageFullName(HANDLE process, UINT32 *length, PWSTR name)
{
    FIXME("(%p %p %p): stub\n", process, length, name);
    return APPMODEL_ERROR_NO_PACKAGE;
}
