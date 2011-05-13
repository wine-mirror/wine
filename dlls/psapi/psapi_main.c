/*
 *      PSAPI library
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2003 Eric Pouech
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winnls.h"
#include "winternl.h"
#include "psapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(psapi);

/***********************************************************************
 *           EnumDeviceDrivers (PSAPI.@)
 */
BOOL WINAPI EnumDeviceDrivers(LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded)
{
    FIXME("(%p, %d, %p): stub\n", lpImageBase, cb, lpcbNeeded);

    if (lpcbNeeded)
        *lpcbNeeded = 0;

    return TRUE;
}

/***********************************************************************
 *           EnumPageFilesA (PSAPI.@)
 */
BOOL WINAPI EnumPageFilesA( PENUM_PAGE_FILE_CALLBACKA callback, LPVOID context )
{
    FIXME("(%p, %p) stub\n", callback, context );
    return FALSE;
}

/***********************************************************************
 *           EnumPageFilesW (PSAPI.@)
 */
BOOL WINAPI EnumPageFilesW( PENUM_PAGE_FILE_CALLBACKW callback, LPVOID context )
{
    FIXME("(%p, %p) stub\n", callback, context );
    return FALSE;
}

/***********************************************************************
 *          GetDeviceDriverBaseNameA (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverBaseNameA(LPVOID ImageBase, LPSTR lpBaseName, 
                                      DWORD nSize)
{
    FIXME("(%p, %p, %d): stub\n", ImageBase, lpBaseName, nSize);

    if (lpBaseName && nSize)
        lpBaseName[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetDeviceDriverBaseNameW (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverBaseNameW(LPVOID ImageBase, LPWSTR lpBaseName, 
                                      DWORD nSize)
{
    FIXME("(%p, %p, %d): stub\n", ImageBase, lpBaseName, nSize);

    if (lpBaseName && nSize)
        lpBaseName[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetDeviceDriverFileNameA (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverFileNameA(LPVOID ImageBase, LPSTR lpFilename, 
                                      DWORD nSize)
{
    FIXME("(%p, %p, %d): stub\n", ImageBase, lpFilename, nSize);

    if (lpFilename && nSize)
        lpFilename[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetDeviceDriverFileNameW (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverFileNameW(LPVOID ImageBase, LPWSTR lpFilename, 
                                      DWORD nSize)
{
    FIXME("(%p, %p, %d): stub\n", ImageBase, lpFilename, nSize);

    if (lpFilename && nSize)
        lpFilename[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetPerformanceInfo (PSAPI.@)
 */
BOOL WINAPI GetPerformanceInfo( PPERFORMANCE_INFORMATION info, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %d)\n", info, size );

    status = NtQuerySystemInformation( SystemPerformanceInformation, info, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           GetWsChanges (PSAPI.@)
 */
BOOL WINAPI GetWsChanges( HANDLE process, PPSAPI_WS_WATCH_INFORMATION watchinfo, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %d)\n", process, watchinfo, size );

    status = NtQueryInformationProcess( process, ProcessWorkingSetWatch, watchinfo, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           InitializeProcessForWsWatch (PSAPI.@)
 */
BOOL WINAPI InitializeProcessForWsWatch(HANDLE hProcess)
{
    FIXME("(hProcess=%p): stub\n", hProcess);

    return TRUE;
}
