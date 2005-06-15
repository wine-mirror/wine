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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winnls.h"
#include "ntstatus.h"
#include "psapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(psapi);

/***********************************************************************
 *           EmptyWorkingSet (PSAPI.@)
 */
BOOL WINAPI EmptyWorkingSet(HANDLE hProcess)
{
    return SetProcessWorkingSetSize(hProcess, 0xFFFFFFFF, 0xFFFFFFFF);
}

/***********************************************************************
 *           EnumDeviceDrivers (PSAPI.@)
 */
BOOL WINAPI EnumDeviceDrivers(LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded)
{
    FIXME("(%p, %ld, %p): stub\n", lpImageBase, cb, lpcbNeeded);

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
 *           EnumProcesses (PSAPI.@)
 */
BOOL WINAPI EnumProcesses(DWORD *lpdwProcessIDs, DWORD cb, DWORD *lpcbUsed)
{
    SYSTEM_PROCESS_INFORMATION *spi;
    NTSTATUS status;
    PVOID pBuf = NULL;
    ULONG nAlloc = 0x8000;

    do {
        if (pBuf != NULL) 
        {
            HeapFree(GetProcessHeap(), 0, pBuf);
            nAlloc *= 2;
        }

        pBuf = HeapAlloc(GetProcessHeap(), 0, nAlloc);
        if (pBuf == NULL)
            return FALSE;

        status = NtQuerySystemInformation(SystemProcessInformation, pBuf,
                                          nAlloc, NULL);
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (status != STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pBuf);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    spi = pBuf;

    for(*lpcbUsed = 0; cb >= sizeof(DWORD); cb -= sizeof(DWORD))
    {
        *lpdwProcessIDs++ = spi->dwProcessID;
        *lpcbUsed += sizeof(DWORD);

        if(spi->dwOffset == 0)
            break;

        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->dwOffset);
    }

    HeapFree(GetProcessHeap(), 0, pBuf);
    return TRUE;
}

/***********************************************************************
 *           EnumProcessModules (PSAPI.@)
 */
BOOL WINAPI EnumProcessModules(HANDLE hProcess, HMODULE *lphModule, 
                               DWORD cb, LPDWORD lpcbNeeded)
{
    HANDLE	hSnapshot;
    DWORD	pid;
    DWORD	count;
    DWORD	countMax;
    int         ret;
    HMODULE     hModule;

    TRACE("(hProcess=%p, %p, %ld, %p)\n",
          hProcess, lphModule, cb, lpcbNeeded );

    if ( lphModule == NULL )
        cb = 0;
    if ( lpcbNeeded != NULL )
        *lpcbNeeded = 0;

    SERVER_START_REQ( get_process_info )
    {
        req->handle = hProcess;
        if ( !wine_server_call_err( req ) )
            pid = (DWORD)reply->pid;
        else
            pid = 0;
    }
    SERVER_END_REQ;

    if ( pid == 0 )
    {
        FIXME("no pid for hProcess %p\n" ,hProcess);
        return FALSE;
    }

    SERVER_START_REQ( create_snapshot )
    {
        req->flags   = SNAP_MODULE;
        req->inherit = FALSE;
        req->pid     = pid;
        wine_server_call_err( req );
        hSnapshot = reply->handle;
    }
    SERVER_END_REQ;
    if ( hSnapshot == 0 )
    {
        FIXME("cannot create snapshot\n");
        return FALSE;
    }
    count = 0;
    countMax = cb / sizeof(HMODULE);
    for (;;)
    {
        SERVER_START_REQ( next_module )
        {
            req->handle = hSnapshot;
            req->reset = (count == 0);
            if ((ret = !wine_server_call_err( req )))
            {
                hModule = (HMODULE)reply->base;
            }
        }
        SERVER_END_REQ;
        if ( !ret ) break;
        TRACE("module 0x%p\n", hModule);
        if ( count < countMax )
            lphModule[count] = hModule;
        count++;
    }
    CloseHandle( hSnapshot );

    if ( lpcbNeeded != NULL )
        *lpcbNeeded = sizeof(HMODULE) * count;

    TRACE("return %lu modules\n", count);

    return TRUE;
}

/***********************************************************************
 *          GetDeviceDriverBaseNameA (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverBaseNameA(LPVOID ImageBase, LPSTR lpBaseName, 
                                      DWORD nSize)
{
    FIXME("(%p, %s, %ld): stub\n",
          ImageBase, debugstr_a(lpBaseName), nSize);

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
    FIXME("(%p, %s, %ld): stub\n",
          ImageBase, debugstr_w(lpBaseName), nSize);

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
    FIXME("(%p, %s, %ld): stub\n",
          ImageBase, debugstr_a(lpFilename), nSize);

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
    FIXME("(%p, %s, %ld): stub\n",
          ImageBase, debugstr_w(lpFilename), nSize);

    if (lpFilename && nSize)
        lpFilename[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetMappedFileNameA (PSAPI.@)
 */
DWORD WINAPI GetMappedFileNameA(HANDLE hProcess, LPVOID lpv, LPSTR lpFilename, 
                                DWORD nSize)
{
    FIXME("(hProcess=%p, %p, %s, %ld): stub\n",
          hProcess, lpv, debugstr_a(lpFilename), nSize);

    if (lpFilename && nSize)
        lpFilename[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetMappedFileNameW (PSAPI.@)
 */
DWORD WINAPI GetMappedFileNameW(HANDLE hProcess, LPVOID lpv, LPWSTR lpFilename, 
                                DWORD nSize)
{
    FIXME("(hProcess=%p, %p, %s, %ld): stub\n",
          hProcess, lpv, debugstr_w(lpFilename), nSize);

    if (lpFilename && nSize)
        lpFilename[0] = '\0';

    return 0;
}

/***********************************************************************
 *           GetModuleBaseNameA (PSAPI.@)
 */
DWORD WINAPI GetModuleBaseNameA(HANDLE hProcess, HMODULE hModule, 
                                LPSTR lpBaseName, DWORD nSize)
{
    WCHAR *lpBaseNameW;
    DWORD buflenW, ret = 0;

    if(!lpBaseName || !nSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    lpBaseNameW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * nSize);
    buflenW = GetModuleBaseNameW(hProcess, hModule, lpBaseNameW, nSize);
    TRACE("%ld, %s\n", buflenW, debugstr_w(lpBaseNameW));
    if (buflenW)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, lpBaseNameW, buflenW,
                                  lpBaseName, nSize, NULL, NULL);
        if (ret < nSize) lpBaseName[ret] = 0;
    }
    HeapFree(GetProcessHeap(), 0, lpBaseNameW);
    return ret;
}

/***********************************************************************
 *           GetModuleBaseNameW (PSAPI.@)
 */
DWORD WINAPI GetModuleBaseNameW(HANDLE hProcess, HMODULE hModule, 
                                LPWSTR lpBaseName, DWORD nSize)
{
    WCHAR  tmp[MAX_PATH];
    WCHAR* ptr;
    int    ptrlen;

    if(!lpBaseName || !nSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!GetModuleFileNameExW(hProcess, hModule, tmp,
                              sizeof(tmp)/sizeof(WCHAR)))
        return 0;
    TRACE("%s\n", debugstr_w(tmp));
    if ((ptr = strrchrW(tmp, '\\')) != NULL) ptr++; else ptr = tmp;
    ptrlen = strlenW(ptr);
    memcpy(lpBaseName, ptr, min(ptrlen+1,nSize) * sizeof(WCHAR));
    return min(ptrlen, nSize);
}

/***********************************************************************
 *           GetModuleFileNameExA (PSAPI.@)
 */
DWORD WINAPI GetModuleFileNameExA(HANDLE hProcess, HMODULE hModule, 
                                  LPSTR lpFileName, DWORD nSize)
{
    WCHAR *ptr;

    TRACE("(hProcess=%p, hModule=%p, %p, %ld)\n",
          hProcess, hModule, lpFileName, nSize);

    if (!lpFileName || !nSize) return 0;

    if ( hProcess == GetCurrentProcess() )
    {
        DWORD len = GetModuleFileNameA( hModule, lpFileName, nSize );
        if (nSize) lpFileName[nSize - 1] = '\0';
        return len;
    }

    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, nSize * sizeof(WCHAR)))) return 0;

    if (!GetModuleFileNameExW(hProcess, hModule, ptr, nSize))
    {
        lpFileName[0] = '\0';
    }
    else
    {
        if (!WideCharToMultiByte( CP_ACP, 0, ptr, -1, lpFileName, nSize, NULL, NULL ))
            lpFileName[nSize - 1] = 0;
    }

    HeapFree(GetProcessHeap(), 0, ptr);
    return strlen(lpFileName);
}

/***********************************************************************
 *           GetModuleFileNameExW (PSAPI.@)
 */
DWORD WINAPI GetModuleFileNameExW(HANDLE hProcess, HMODULE hModule, 
                                  LPWSTR lpFileName, DWORD nSize)
{
    DWORD len = 0;

    TRACE("(hProcess=%p, hModule=%p, %p, %ld)\n",
          hProcess, hModule, lpFileName, nSize);

    if (!lpFileName || !nSize) return 0;

    if ( hProcess == GetCurrentProcess() )
    {
        DWORD len = GetModuleFileNameW( hModule, lpFileName, nSize );
        if (nSize) lpFileName[nSize - 1] = '\0';
        TRACE("return (cur) %s (%lu)\n", debugstr_w(lpFileName), len);
        return len;
    }

    lpFileName[0] = 0;

    SERVER_START_REQ( get_dll_info )
    {
        req->handle       = hProcess;
        req->base_address = hModule;
        wine_server_set_reply( req, lpFileName, (nSize - 1) * sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            len = wine_server_reply_size(reply) / sizeof(WCHAR);
            lpFileName[len] = 0;
        }
    }
    SERVER_END_REQ;

    TRACE("return %s (%lu)\n", debugstr_w(lpFileName), len);

    return len;
}

/***********************************************************************
 *           GetModuleInformation (PSAPI.@)
 */
BOOL WINAPI GetModuleInformation(HANDLE hProcess, HMODULE hModule, 
                                 LPMODULEINFO lpmodinfo, DWORD cb)
{
    BOOL ret = FALSE;

    TRACE("(hProcess=%p, hModule=%p, %p, %ld)\n",
          hProcess, hModule, lpmodinfo, cb);

    if (cb < sizeof(MODULEINFO)) return FALSE;

    SERVER_START_REQ( get_dll_info )
    {
        req->handle       = hProcess;
        req->base_address = (void*)hModule;
        if (!wine_server_call_err( req ))
        {
            ret = TRUE;
            lpmodinfo->lpBaseOfDll = (void*)hModule;
            lpmodinfo->SizeOfImage = reply->size;
            lpmodinfo->EntryPoint  = reply->entry_point;
        }
    }
    SERVER_END_REQ;

    return TRUE;
}

/***********************************************************************
 *           GetPerformanceInfo (PSAPI.@)
 */
BOOL WINAPI GetPerformanceInfo( PPERFORMANCE_INFORMATION info, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %ld)\n", info, size );

    status = NtQueryInformationProcess( GetCurrentProcess(), SystemPerformanceInformation, info, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           GetProcessImageFileNameA (PSAPI.@)
 */
DWORD WINAPI GetProcessImageFileNameA( HANDLE process, LPSTR file, DWORD size )
{
    FIXME("(%p, %p, %ld) stub\n", process, file, size );
    return 0;
}

/***********************************************************************
 *           GetProcessImageFileNameW (PSAPI.@)
 */
DWORD WINAPI GetProcessImageFileNameW( HANDLE process, LPWSTR file, DWORD size )
{
    FIXME("(%p, %p, %ld) stub\n", process, file, size );
    return 0;
}

/***********************************************************************
 *           GetProcessMemoryInfo (PSAPI.@)
 *
 * Retrieve memory usage information for a given process
 *
 */
BOOL WINAPI GetProcessMemoryInfo( HANDLE process, PPROCESS_MEMORY_COUNTERS counters, DWORD size )
{
    NTSTATUS status;
    VM_COUNTERS vmc;

    TRACE( "(%p, %p, %ld)\n", process, counters, size );

    status = NtQueryInformationProcess( process, ProcessVmCounters, &vmc, sizeof(vmc), NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }

    /* FIXME: check size */

    counters->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    counters->PageFaultCount = vmc.PageFaultCount;
    counters->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    counters->WorkingSetSize = vmc.WorkingSetSize;
    counters->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    counters->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    counters->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    counters->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    counters->PagefileUsage = vmc.PagefileUsage;
    counters->PeakPagefileUsage = vmc.PeakPagefileUsage;

    return TRUE;
}

/***********************************************************************
 *           GetWsChanges (PSAPI.@)
 */
BOOL WINAPI GetWsChanges( HANDLE process, PPSAPI_WS_WATCH_INFORMATION watchinfo, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %ld)\n", process, watchinfo, size );

    status = NtQueryVirtualMemory( process, NULL, ProcessWorkingSetWatch, watchinfo, size, NULL );

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

/***********************************************************************
 *           QueryWorkingSet (PSAPI.@)
 */
BOOL WINAPI QueryWorkingSet( HANDLE process, LPVOID buffer, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %ld)\n", process, buffer, size );

    status = NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *           QueryWorkingSetEx (PSAPI.@)
 */
BOOL WINAPI QueryWorkingSetEx( HANDLE process, LPVOID buffer, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %ld)\n", process, buffer, size );

    status = NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer,  size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}
