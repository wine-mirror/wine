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
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);


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

static inline BOOL version_compare_values(ULONG left, ULONG right, UCHAR condition)
{
    switch (condition)
    {
        case VER_EQUAL:
            if (left != right) return FALSE;
            break;
        case VER_GREATER:
            if (left <= right) return FALSE;
            break;
        case VER_GREATER_EQUAL:
            if (left < right) return FALSE;
            break;
        case VER_LESS:
            if (left >= right) return FALSE;
            break;
        case VER_LESS_EQUAL:
            if (left > right) return FALSE;
            break;
        default:
            return FALSE;
    }
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
BOOL WINAPI VerifyVersionInfoW( LPOSVERSIONINFOEXW info, DWORD dwTypeMask,
                                DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW ver;

    TRACE("(%p 0x%lx 0x%s)\n", info, dwTypeMask, wine_dbgstr_longlong(dwlConditionMask));

    ver.dwOSVersionInfoSize = sizeof(ver);
    if (!GetVersionExW((OSVERSIONINFOW*)&ver)) return FALSE;

    if (!dwTypeMask || !dwlConditionMask)
    {
        SetLastError(ERROR_BAD_ARGUMENTS);
        return FALSE;
    }

    if (dwTypeMask & VER_PRODUCT_TYPE)
    {
        if (!version_compare_values(ver.wProductType, info->wProductType, dwlConditionMask >> 7*3 & 0x07))
            goto mismatch;
    }
    if (dwTypeMask & VER_SUITENAME)
        switch (dwlConditionMask >> 6*3 & 0x07)
        {
            case VER_AND:
                if ((info->wSuiteMask & ver.wSuiteMask) != info->wSuiteMask)
                    goto mismatch;
                break;
            case VER_OR:
                if (!(info->wSuiteMask & ver.wSuiteMask) && info->wSuiteMask)
                    goto mismatch;
                break;
            default:
                SetLastError(ERROR_BAD_ARGUMENTS);
                return FALSE;
        }
    if (dwTypeMask & VER_PLATFORMID)
    {
        if (!version_compare_values(ver.dwPlatformId, info->dwPlatformId, dwlConditionMask >> 3*3 & 0x07))
            goto mismatch;
    }
    if (dwTypeMask & VER_BUILDNUMBER)
    {
        if (!version_compare_values(ver.dwBuildNumber, info->dwBuildNumber, dwlConditionMask >> 2*3 & 0x07))
            goto mismatch;
    }

    if (dwTypeMask & (VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR))
    {
        unsigned char condition, last_condition = 0;
        BOOL succeeded = TRUE, do_next_check = TRUE;

        if (dwTypeMask & VER_MAJORVERSION)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 1*3 & 0x07);
            succeeded = version_compare_values(ver.dwMajorVersion, info->dwMajorVersion, condition);
            do_next_check = (ver.dwMajorVersion == info->dwMajorVersion) &&
                ((condition >= VER_EQUAL) && (condition <= VER_LESS_EQUAL));
        }
        if ((dwTypeMask & VER_MINORVERSION) && do_next_check)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 0*3 & 0x07);
            succeeded = version_compare_values(ver.dwMinorVersion, info->dwMinorVersion, condition);
            do_next_check = (ver.dwMinorVersion == info->dwMinorVersion) &&
                ((condition >= VER_EQUAL) && (condition <= VER_LESS_EQUAL));
        }
        if ((dwTypeMask & VER_SERVICEPACKMAJOR) && do_next_check)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 5*3 & 0x07);
            succeeded = version_compare_values(ver.wServicePackMajor, info->wServicePackMajor, condition);
            do_next_check = (ver.wServicePackMajor == info->wServicePackMajor) &&
                ((condition >= VER_EQUAL) && (condition <= VER_LESS_EQUAL));
        }
        if ((dwTypeMask & VER_SERVICEPACKMINOR) && do_next_check)
        {
            condition = version_update_condition(&last_condition, dwlConditionMask >> 4*3 & 0x07);
            succeeded = version_compare_values(ver.wServicePackMinor, info->wServicePackMinor, condition);
        }

        if (!succeeded) goto mismatch;
    }

    return TRUE;

mismatch:
    SetLastError(ERROR_OLD_WIN_VERSION);
    return FALSE;
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
