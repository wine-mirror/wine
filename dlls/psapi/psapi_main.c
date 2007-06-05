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

typedef struct
{
    HANDLE hProcess;             
    PLIST_ENTRY pHead, pCurrent;
    LDR_MODULE LdrModule;
} MODULE_ITERATOR;

/***********************************************************************
 *           PSAPI_ModuleIteratorInit [internal]
 *
 * Prepares to iterate through the loaded modules of the given process.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL PSAPI_ModuleIteratorInit(MODULE_ITERATOR *iter, HANDLE hProcess)
{
    PROCESS_BASIC_INFORMATION pbi;
    PPEB_LDR_DATA pLdrData;
    NTSTATUS status;

    /* Get address of PEB */
    status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
                                       &pbi, sizeof(pbi), NULL);
    if (status != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    /* Read address of LdrData from PEB */
    if (!ReadProcessMemory(hProcess, &((PPEB)pbi.PebBaseAddress)->LdrData,
                           &pLdrData, sizeof(pLdrData), NULL))
        return FALSE;

    /* Read address of first module from LdrData */
    if (!ReadProcessMemory(hProcess,
                           &pLdrData->InLoadOrderModuleList.Flink,
                           &iter->pCurrent, sizeof(iter->pCurrent), NULL))
        return FALSE;

    iter->pHead = &pLdrData->InLoadOrderModuleList;
    iter->hProcess = hProcess;

    return TRUE;
}

/***********************************************************************
 *           PSAPI_ModuleIteratorNext [internal]
 *
 * Iterates to the next module.
 *
 * RETURNS
 *   1 : Success
 *   0 : No more modules
 *  -1 : Failure
 *
 * NOTES
 *  Every function which uses this routine suffers from a race condition 
 *  when a module is unloaded during the enumeration which can cause the
 *  function to fail. As there is no way to lock the loader of another 
 *  process we can't avoid that.
 */
static INT PSAPI_ModuleIteratorNext(MODULE_ITERATOR *iter)
{
    if (iter->pCurrent == iter->pHead)
        return 0;

    if (!ReadProcessMemory(iter->hProcess, CONTAINING_RECORD(iter->pCurrent,
                           LDR_MODULE, InLoadOrderModuleList),
                           &iter->LdrModule, sizeof(iter->LdrModule), NULL))
         return -1;
    else
         iter->pCurrent = iter->LdrModule.InLoadOrderModuleList.Flink;

    return 1;
}

/***********************************************************************
 *           PSAPI_GetLdrModule [internal]
 *
 * Reads the LDR_MODULE structure of the given module.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */

static BOOL PSAPI_GetLdrModule(HANDLE hProcess, HMODULE hModule, 
                               LDR_MODULE *pLdrModule)
{
    MODULE_ITERATOR iter;
    INT ret;

    if (!PSAPI_ModuleIteratorInit(&iter, hProcess))
        return FALSE;

    while ((ret = PSAPI_ModuleIteratorNext(&iter)) > 0)
        /* When hModule is NULL we return the process image - which will be
         * the first module since our iterator uses InLoadOrderModuleList */
        if (!hModule || hModule == (HMODULE)iter.LdrModule.BaseAddress)
        {
            *pLdrModule = iter.LdrModule;
            return TRUE;
        }

    if (ret == 0)
        SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
}

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

    for (*lpcbUsed = 0; cb >= sizeof(DWORD); cb -= sizeof(DWORD))
    {
        *lpdwProcessIDs++ = spi->dwProcessID;
        *lpcbUsed += sizeof(DWORD);

        if (spi->dwOffset == 0)
            break;

        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->dwOffset);
    }

    HeapFree(GetProcessHeap(), 0, pBuf);
    return TRUE;
}

/***********************************************************************
 *           EnumProcessModules (PSAPI.@)
 *
 * NOTES
 *  Returned list is in load order.
 */
BOOL WINAPI EnumProcessModules(HANDLE hProcess, HMODULE *lphModule,
                               DWORD cb, LPDWORD lpcbNeeded)
{
    MODULE_ITERATOR iter;
    INT ret;

    if (!PSAPI_ModuleIteratorInit(&iter, hProcess))
        return FALSE;

    *lpcbNeeded = 0;
        
    while ((ret = PSAPI_ModuleIteratorNext(&iter)) > 0)
    {
        if (cb >= sizeof(HMODULE))
        {
            *lphModule++ = (HMODULE)iter.LdrModule.BaseAddress;
            cb -= sizeof(HMODULE);
        }
        *lpcbNeeded += sizeof(HMODULE);
    }

    return (ret == 0);
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
 *           GetMappedFileNameA (PSAPI.@)
 */
DWORD WINAPI GetMappedFileNameA(HANDLE hProcess, LPVOID lpv, LPSTR lpFilename, 
                                DWORD nSize)
{
    FIXME("(%p, %p, %p, %d): stub\n", hProcess, lpv, lpFilename, nSize);

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
    FIXME("(%p, %p, %p, %d): stub\n", hProcess, lpv, lpFilename, nSize);

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
    TRACE("%d, %s\n", buflenW, debugstr_w(lpBaseNameW));
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
    LDR_MODULE LdrModule;

    if (!PSAPI_GetLdrModule(hProcess, hModule, &LdrModule))
        return 0;

    nSize = min(LdrModule.BaseDllName.Length / sizeof(WCHAR), nSize);
    if (!ReadProcessMemory(hProcess, LdrModule.BaseDllName.Buffer,
                           lpBaseName, nSize * sizeof(WCHAR), NULL))
        return 0;

    lpBaseName[nSize] = 0;
    return nSize;
}

/***********************************************************************
 *           GetModuleFileNameExA (PSAPI.@)
 */
DWORD WINAPI GetModuleFileNameExA(HANDLE hProcess, HMODULE hModule, 
                                  LPSTR lpFileName, DWORD nSize)
{
    WCHAR *ptr;

    TRACE("(hProcess=%p, hModule=%p, %p, %d)\n",
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
    LDR_MODULE LdrModule;
    
    if(!PSAPI_GetLdrModule(hProcess, hModule, &LdrModule))
        return 0;
        
    nSize = min(LdrModule.FullDllName.Length / sizeof(WCHAR), nSize);
    if (!ReadProcessMemory(hProcess, LdrModule.FullDllName.Buffer,
                           lpFileName, nSize * sizeof(WCHAR), NULL))
        return 0;

    lpFileName[nSize] = 0;
    return nSize;
}

/***********************************************************************
 *           GetModuleInformation (PSAPI.@)
 */
BOOL WINAPI GetModuleInformation(HANDLE hProcess, HMODULE hModule, 
                                 LPMODULEINFO lpmodinfo, DWORD cb)
{
    LDR_MODULE LdrModule;

    if (cb < sizeof(MODULEINFO)) 
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!PSAPI_GetLdrModule(hProcess, hModule, &LdrModule))
        return FALSE;

    lpmodinfo->lpBaseOfDll = LdrModule.BaseAddress;
    lpmodinfo->SizeOfImage = LdrModule.SizeOfImage;
    lpmodinfo->EntryPoint  = LdrModule.EntryPoint;
    return TRUE;
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
 *           GetProcessImageFileNameA (PSAPI.@)
 */
DWORD WINAPI GetProcessImageFileNameA( HANDLE process, LPSTR file, DWORD size )
{
    FIXME("(%p, %p, %d) stub\n", process, file, size );
    return 0;
}

/***********************************************************************
 *           GetProcessImageFileNameW (PSAPI.@)
 */
DWORD WINAPI GetProcessImageFileNameW( HANDLE process, LPWSTR file, DWORD size )
{
    FIXME("(%p, %p, %d) stub\n", process, file, size );
    return 0;
}

/***********************************************************************
 *           GetProcessMemoryInfo (PSAPI.@)
 *
 * Retrieve memory usage information for a given process
 *
 */
BOOL WINAPI GetProcessMemoryInfo(HANDLE hProcess, 
                                 PPROCESS_MEMORY_COUNTERS pmc, DWORD cb)
{
    NTSTATUS status;
    VM_COUNTERS vmc;

    if (cb < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    status = NtQueryInformationProcess(hProcess, ProcessVmCounters, 
                                       &vmc, sizeof(vmc), NULL);

    if (status)
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    pmc->PageFaultCount = vmc.PageFaultCount;
    pmc->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    pmc->WorkingSetSize = vmc.WorkingSetSize;
    pmc->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    pmc->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    pmc->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    pmc->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    pmc->PagefileUsage = vmc.PagefileUsage;
    pmc->PeakPagefileUsage = vmc.PeakPagefileUsage;

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

/***********************************************************************
 *           QueryWorkingSet (PSAPI.@)
 */
BOOL WINAPI QueryWorkingSet( HANDLE process, LPVOID buffer, DWORD size )
{
    NTSTATUS status;

    TRACE( "(%p, %p, %d)\n", process, buffer, size );

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

    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    status = NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer,  size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}
