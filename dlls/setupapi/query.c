/*
 * setupapi query functions
 *
 * Copyright 2006 James Hawkins
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winver.h"
#include "setupapi.h"
#include "advpub.h"
#include "winnls.h"
#include "wine/debug.h"
#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *      SetupQueryInfFileInformationA    (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfFileInformationA(PSP_INF_INFORMATION InfInformation,
                                          UINT InfIndex, PSTR ReturnBuffer,
                                          DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    LPWSTR filenameW;
    DWORD size;
    BOOL ret;

    ret = SetupQueryInfFileInformationW(InfInformation, InfIndex, NULL, 0, &size);
    if (!ret)
        return FALSE;

    filenameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));

    ret = SetupQueryInfFileInformationW(InfInformation, InfIndex,
                                        filenameW, size, &size);
    if (!ret)
    {
        HeapFree(GetProcessHeap(), 0, filenameW);
        return FALSE;
    }

    if (RequiredSize)
        *RequiredSize = size;

    if (!ReturnBuffer)
        return TRUE;

    if (size > ReturnBufferSize)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    WideCharToMultiByte(CP_ACP, 0, filenameW, -1, ReturnBuffer, size, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, filenameW);

    return ret;
}

/***********************************************************************
 *      SetupQueryInfFileInformationW    (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfFileInformationW(PSP_INF_INFORMATION InfInformation,
                                          UINT InfIndex, PWSTR ReturnBuffer,
                                          DWORD ReturnBufferSize, PDWORD RequiredSize) 
{
    DWORD len;
    LPWSTR ptr;

    TRACE("(%p, %u, %p, %ld, %p) Stub!\n", InfInformation, InfIndex,
          ReturnBuffer, ReturnBufferSize, RequiredSize);

    if (!InfInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (InfIndex != 0)
        FIXME("Appended INF files are not handled\n");

    ptr = (LPWSTR)&InfInformation->VersionData[0];
    len = lstrlenW(ptr);

    if (RequiredSize)
        *RequiredSize = len + 1;

    if (!ReturnBuffer)
        return TRUE;

    if (ReturnBufferSize < len)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    lstrcpyW(ReturnBuffer, ptr);
    return TRUE;
}
