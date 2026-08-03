/* Xbox One ERA (kernelx.dll) Wine implementation
 *
 * Based on WinDurango/WinDurango (MIT) and XWine1/SlimEra (MIT)
 * Adapted for Wine/Linux by WineEX project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "winnt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernelx);

/* -----------------------------------------------------------------------
 * Xbox One ERA types
 * ----------------------------------------------------------------------- */

typedef enum _CONSOLE_TYPE
{
    CONSOLE_TYPE_UNKNOWN          = 0,
    CONSOLE_TYPE_XBOX_ONE         = 1,
    CONSOLE_TYPE_XBOX_ONE_S       = 2,
    CONSOLE_TYPE_XBOX_ONE_X       = 3,
    CONSOLE_TYPE_XBOX_ONE_X_DEVKIT= 4,
} CONSOLE_TYPE;

typedef struct _SYSTEMOSVERSIONINFO
{
    BYTE  MajorVersion;
    BYTE  MinorVersion;
    WORD  BuildNumber;
    WORD  Revision;
} SYSTEMOSVERSIONINFO, *LPSYSTEMOSVERSIONINFO;

typedef struct _PROCESSOR_SCHEDULING_STATISTICS
{
    ULONGLONG RunningTime;
    ULONGLONG IdleTime;
    ULONGLONG GlobalTime;
} PROCESSOR_SCHEDULING_STATISTICS, *PPROCESSOR_SCHEDULING_STATISTICS;

typedef struct _TITLEMEMORYSTATUS
{
    DWORD     dwLength;
    DWORD     dwReserved;
    ULONGLONG ullTotalMem;
    ULONGLONG ullAvailMem;
    ULONGLONG ullLegacyUsed;
    ULONGLONG ullLegacyPeak;
    ULONGLONG ullLegacyAvail;
    ULONGLONG ullTitleUsed;
    ULONGLONG ullTitleAvail;
    ULONGLONG ullLegacyPageTableUsed;
    ULONGLONG ullTitlePageTableUsed;
} TITLEMEMORYSTATUS, *LPTITLEMEMORYSTATUS;

typedef struct _TOOLINGMEMORYSTATUS
{
    DWORD     dwLength;
    DWORD     dwReserved;
    ULONGLONG ullTotalMem;
    ULONGLONG ullAvailMem;
    ULONGLONG ulPeakUsage;
    ULONGLONG ullPageTableUsage;
} TOOLINGMEMORYSTATUS, *LPTOOLINGMEMORYSTATUS;

typedef union _XALLOC_ATTRIBUTES
{
    ULONGLONG dwAttributes;
    struct {
        ULONGLONG dwObjectType  : 14;
        ULONGLONG dwPageSize    : 2;
        ULONGLONG dwAllocatorId : 8;
        ULONGLONG dwAlignment   : 5;
        ULONGLONG dwMemoryType  : 4;
        ULONGLONG reserved      : 31;
    } s;
} XALLOC_ATTRIBUTES;

typedef PVOID (WINAPI *PXMEMALLOC_ROUTINE)(SIZE_T dwSize, ULONGLONG dwAttributes);
typedef void  (WINAPI *PXMEMFREE_ROUTINE)(PVOID lpAddress, ULONGLONG dwAttributes);

/* -----------------------------------------------------------------------
 * Internal state
 * ----------------------------------------------------------------------- */

static PXMEMALLOC_ROUTINE xmem_alloc_hook = NULL;
static PXMEMFREE_ROUTINE  xmem_free_hook  = NULL;
static CRITICAL_SECTION   xmem_hook_cs;

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        InitializeCriticalSection(&xmem_hook_cs);
        break;
    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&xmem_hook_cs);
        break;
    }
    return TRUE;
}

/* -----------------------------------------------------------------------
 * Console / OS identity
 * ----------------------------------------------------------------------- */

CONSOLE_TYPE WINAPI GetConsoleType(void)
{
    TRACE("\n");
    return CONSOLE_TYPE_XBOX_ONE;
}

void WINAPI GetSystemOSVersion(LPSYSTEMOSVERSIONINFO lpVersionInfo)
{
    TRACE("%p\n", lpVersionInfo);
    /* Report Xbox One OS version 10.0.10586.1000 (ERA era baseline) */
    lpVersionInfo->MajorVersion = 10;
    lpVersionInfo->MinorVersion = 0;
    lpVersionInfo->BuildNumber  = 10586;
    lpVersionInfo->Revision     = 1000;
}

/* -----------------------------------------------------------------------
 * Processor / scheduling statistics
 * ----------------------------------------------------------------------- */

void WINAPI QueryProcessorSchedulingStatistics(PPROCESSOR_SCHEDULING_STATISTICS lpStats)
{
    LARGE_INTEGER freq, counter;
    FILETIME idle, kernel, user;

    TRACE("%p\n", lpStats);

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    lpStats->GlobalTime = (freq.QuadPart > 0)
        ? counter.QuadPart / (freq.QuadPart / 10000000ULL) : 0;

    if (GetSystemTimes(&idle, &kernel, &user))
    {
        ULARGE_INTEGER i64, k64, u64;
        i64.LowPart = idle.dwLowDateTime;   i64.HighPart = idle.dwHighDateTime;
        k64.LowPart = kernel.dwLowDateTime; k64.HighPart = kernel.dwHighDateTime;
        u64.LowPart = user.dwLowDateTime;   u64.HighPart = user.dwHighDateTime;
        lpStats->RunningTime = (k64.QuadPart - i64.QuadPart) + u64.QuadPart;
        lpStats->IdleTime    = i64.QuadPart;
    }
    else
    {
        lpStats->RunningTime = 0;
        lpStats->IdleTime    = 0;
    }
}

/* -----------------------------------------------------------------------
 * Thread pool / thread naming
 * ----------------------------------------------------------------------- */

BOOL WINAPI SetThreadpoolAffinityMask(void *pool, DWORD_PTR mask)
{
    FIXME("(%p, %Iu) stub\n", pool, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetThreadName(HANDLE hThread, PCWSTR name)
{
    HRESULT hr;
    TRACE("(%p, %s)\n", hThread, debugstr_w(name));
    hr = SetThreadDescription(hThread, name);
    SetLastError(SUCCEEDED(hr) ? ERROR_SUCCESS : hr & 0xFFFF);
    return SUCCEEDED(hr);
}

BOOL WINAPI GetThreadName(HANDLE hThread, PWSTR buf, SIZE_T len, PSIZE_T pRet)
{
    PWSTR desc = NULL;
    int n;

    TRACE("(%p, %p, %Iu, %p)\n", hThread, buf, len, pRet);

    if (!pRet) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }

    if (FAILED(GetThreadDescription(hThread, &desc)))
    {
        *pRet = 0;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    n = lstrlenW(desc);
    *pRet = n;
    if (!buf || (SIZE_T)n >= len)
    {
        LocalFree(desc);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    memcpy(buf, desc, (n + 1) * sizeof(WCHAR));
    LocalFree(desc);
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

/* -----------------------------------------------------------------------
 * Memory status
 * ----------------------------------------------------------------------- */

BOOL WINAPI TitleMemoryStatus(LPTITLEMEMORYSTATUS lpBuf)
{
    MEMORYSTATUSEX ms;

    TRACE("%p\n", lpBuf);

    if (lpBuf->dwLength != 64)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);

    lpBuf->ullTotalMem    = ms.ullTotalPhys;
    lpBuf->ullAvailMem    = ms.ullAvailPhys;
    lpBuf->ullLegacyUsed  = ms.ullTotalPhys - ms.ullAvailPhys;
    lpBuf->ullLegacyAvail = ms.ullAvailPhys;
    lpBuf->ullLegacyPeak  = ms.ullTotalPhys;
    lpBuf->ullTitleUsed   = ms.ullTotalPhys - ms.ullAvailPhys;
    lpBuf->ullTitleAvail  = ms.ullAvailPhys;
    lpBuf->ullLegacyPageTableUsed = 0;
    lpBuf->ullTitlePageTableUsed  = 0;

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI JobTitleMemoryStatus(LPTITLEMEMORYSTATUS lpBuf)
{
    FIXME("%p stub\n", lpBuf);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI ToolingMemoryStatus(LPTOOLINGMEMORYSTATUS lpBuf)
{
    FIXME("%p stub\n", lpBuf);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/* -----------------------------------------------------------------------
 * Physical / mappable pages (Xbox One ESRAM / title memory)
 * These are Xbox One hardware features; stub them on Linux/Wine.
 * ----------------------------------------------------------------------- */

BOOL WINAPI AllocateTitlePhysicalPages(HANDLE hProc, DWORD type,
        PULONG_PTR pCount, PULONG_PTR pageArray)
{
    FIXME("(%p, %08lx, %p, %p) stub\n", hProc, type, pCount, pageArray);
    if (pCount) *pCount = 0;
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL WINAPI FreeTitlePhysicalPages(HANDLE hProc, ULONG_PTR count,
        PULONG_PTR pageArray)
{
    FIXME("(%p, %Iu, %p) stub\n", hProc, count, pageArray);
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

PVOID WINAPI MapTitlePhysicalPages(PVOID addr, ULONG_PTR count, DWORD type,
        DWORD prot, PULONG_PTR pageArray)
{
    FIXME("(%p, %Iu, %08lx, %08lx, %p) stub\n", addr, count, type, prot, pageArray);
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

HRESULT WINAPI MapTitleEsramPages(PVOID addr, UINT count, DWORD type,
        const UINT *pageArray)
{
    FIXME("(%p, %u, %08lx, %p) stub\n", addr, count, type, pageArray);
    return E_NOTIMPL;
}

/* -----------------------------------------------------------------------
 * ERA virtual memory wrappers
 * Strip Xbox-specific flags (MEM_GRAPHICS=0x10000000, MEM_TITLE=0x40000000)
 * and forward to standard VirtualAlloc*.
 * ----------------------------------------------------------------------- */

#define MEM_GRAPHICS 0x10000000
#define MEM_TITLE    0x40000000
#define ERA_MEM_MASK (~(DWORD)(MEM_GRAPHICS | MEM_TITLE))

LPVOID WINAPI EraVirtualAllocEx(HANDLE hProc, LPVOID addr, SIZE_T size,
        DWORD type, DWORD prot)
{
    TRACE("(%p, %p, %Iu, %08lx, %08lx)\n", hProc, addr, size, type, prot);
    return VirtualAllocEx(hProc, addr, size, type & ERA_MEM_MASK, prot);
}

LPVOID WINAPI EraVirtualAlloc(LPVOID addr, SIZE_T size, DWORD type, DWORD prot)
{
    TRACE("(%p, %Iu, %08lx, %08lx)\n", addr, size, type, prot);
    return VirtualAlloc(addr, size, type & ERA_MEM_MASK, prot);
}

BOOL WINAPI EraVirtualFreeEx(HANDLE hProc, LPVOID addr, SIZE_T size, DWORD freeType)
{
    TRACE("(%p, %p, %Iu, %08lx)\n", hProc, addr, size, freeType);
    return VirtualFreeEx(hProc, addr, size, freeType);
}

BOOL WINAPI EraVirtualFree(LPVOID addr, SIZE_T size, DWORD freeType)
{
    TRACE("(%p, %Iu, %08lx)\n", addr, size, freeType);
    return VirtualFree(addr, size, freeType);
}

SIZE_T WINAPI EraVirtualQueryEx(HANDLE hProc, LPCVOID addr,
        PMEMORY_BASIC_INFORMATION buf, SIZE_T len)
{
    return VirtualQueryEx(hProc, addr, buf, len);
}

SIZE_T WINAPI EraVirtualQuery(LPCVOID addr, PMEMORY_BASIC_INFORMATION buf, SIZE_T len)
{
    return VirtualQuery(addr, buf, len);
}

BOOL WINAPI EraVirtualProtectEx(HANDLE hProc, LPVOID addr, SIZE_T size,
        DWORD newProt, PDWORD oldProt)
{
    return VirtualProtectEx(hProc, addr, size, newProt, oldProt);
}

BOOL WINAPI EraVirtualProtect(LPVOID addr, SIZE_T size, DWORD newProt, PDWORD oldProt)
{
    return VirtualProtect(addr, size, newProt, oldProt);
}

/* -----------------------------------------------------------------------
 * XMem allocation
 * ----------------------------------------------------------------------- */

PVOID WINAPI XMemAllocDefault(SIZE_T size, ULONGLONG attrs)
{
    TRACE("(%Iu, %I64x)\n", size, attrs);
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void WINAPI XMemFreeDefault(PVOID ptr, ULONGLONG attrs)
{
    TRACE("(%p, %I64x)\n", ptr, attrs);
    HeapFree(GetProcessHeap(), 0, ptr);
}

PVOID WINAPI XMemAlloc(SIZE_T size, ULONGLONG attrs)
{
    PVOID ret;
    EnterCriticalSection(&xmem_hook_cs);
    if (xmem_alloc_hook)
        ret = xmem_alloc_hook(size, attrs);
    else
        ret = XMemAllocDefault(size, attrs);
    LeaveCriticalSection(&xmem_hook_cs);
    return ret;
}

void WINAPI XMemFree(PVOID ptr, ULONGLONG attrs)
{
    EnterCriticalSection(&xmem_hook_cs);
    if (xmem_free_hook)
        xmem_free_hook(ptr, attrs);
    else
        XMemFreeDefault(ptr, attrs);
    LeaveCriticalSection(&xmem_hook_cs);
}

void WINAPI XMemSetAllocationHooks(PXMEMALLOC_ROUTINE allocHook,
        PXMEMFREE_ROUTINE freeHook)
{
    TRACE("(%p, %p)\n", allocHook, freeHook);
    EnterCriticalSection(&xmem_hook_cs);
    xmem_alloc_hook = allocHook;
    xmem_free_hook  = freeHook;
    LeaveCriticalSection(&xmem_hook_cs);
}

void WINAPI XMemCheckDefaultHeaps(void)
{
    TRACE("\n");
}

BOOL WINAPI XMemSetAllocationHysteresis(void)
{
    FIXME("stub\n");
    return FALSE;
}

SIZE_T WINAPI XMemGetAllocationHysteresis(void)
{
    FIXME("stub\n");
    return 0;
}

BOOL WINAPI XMemPreallocateFreeSpace(void)
{
    FIXME("stub\n");
    return FALSE;
}

BOOL WINAPI XMemGetAllocationStatistics(void)
{
    FIXME("stub\n");
    return FALSE;
}

HANDLE WINAPI XMemGetAuxiliaryTitleMemory(void)
{
    FIXME("stub\n");
    return NULL;
}

void WINAPI XMemReleaseAuxiliaryTitleMemory(HANDLE h)
{
    FIXME("(%p) stub\n", h);
}

/* -----------------------------------------------------------------------
 * ERA file I/O wrappers (pass-through on Wine/Linux)
 * ----------------------------------------------------------------------- */

HANDLE WINAPI EraCreateFileW(LPCWSTR path, DWORD access, DWORD share,
        LPSECURITY_ATTRIBUTES sa, DWORD disp, DWORD flags, HANDLE tmpl)
{
    TRACE("(%s)\n", debugstr_w(path));
    return CreateFileW(path, access, share, sa, disp, flags, tmpl);
}

HANDLE WINAPI EraCreateFileA(LPCSTR path, DWORD access, DWORD share,
        LPSECURITY_ATTRIBUTES sa, DWORD disp, DWORD flags, HANDLE tmpl)
{
    TRACE("(%s)\n", debugstr_a(path));
    return CreateFileA(path, access, share, sa, disp, flags, tmpl);
}

HANDLE WINAPI EraCreateFile2(LPCWSTR path, DWORD access, DWORD share,
        DWORD disp, void *pExParams)
{
    TRACE("(%s)\n", debugstr_w(path));
    return CreateFile2(path, access, share, disp, pExParams);
}

BOOL WINAPI EraCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sa)
{
    TRACE("(%s)\n", debugstr_a(path));
    return CreateDirectoryA(path, sa);
}

BOOL WINAPI EraCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sa)
{
    TRACE("(%s)\n", debugstr_w(path));
    return CreateDirectoryW(path, sa);
}

HMODULE WINAPI EraLoadLibraryExA(LPCSTR name, HANDLE hFile, DWORD flags)
{
    TRACE("(%s)\n", debugstr_a(name));
    return LoadLibraryExA(name, hFile, flags);
}

HMODULE WINAPI EraLoadLibraryW(LPCWSTR name)
{
    TRACE("(%s)\n", debugstr_w(name));
    return LoadLibraryW(name);
}

HMODULE WINAPI EraLoadLibraryExW(LPCWSTR name, HANDLE hFile, DWORD flags)
{
    TRACE("(%s)\n", debugstr_w(name));
    return LoadLibraryExW(name, hFile, flags);
}

DWORD WINAPI EraGetFileAttributesW(LPCWSTR path)
{
    return GetFileAttributesW(path);
}

BOOL WINAPI EraGetFileAttributesExW(LPCWSTR path, GET_FILEEX_INFO_LEVELS level,
        LPVOID info)
{
    return GetFileAttributesExW(path, level, info);
}

DWORD WINAPI EraGetFileAttributesA(LPCSTR path)
{
    return GetFileAttributesA(path);
}

HANDLE WINAPI EraFindFirstFileW(LPCWSTR path, LPWIN32_FIND_DATAW data)
{
    return FindFirstFileW(path, data);
}

HANDLE WINAPI EraFindFirstFileA(LPCSTR path, LPWIN32_FIND_DATAA data)
{
    return FindFirstFileA(path, data);
}

BOOL WINAPI EraFindNextFileW(HANDLE h, LPWIN32_FIND_DATAW data)
{
    return FindNextFileW(h, data);
}

BOOL WINAPI EraFindNextFileA(HANDLE h, LPWIN32_FIND_DATAA data)
{
    return FindNextFileA(h, data);
}

BOOL WINAPI EraDeleteFileW(LPCWSTR path)
{
    return DeleteFileW(path);
}

BOOL WINAPI EraSetFileAttributesA(LPCSTR path, DWORD attrs)
{
    return SetFileAttributesA(path, attrs);
}

BOOL WINAPI EraGetFileInformationByHandleEx(HANDLE h,
        FILE_INFO_BY_HANDLE_CLASS cls, LPVOID info, DWORD len)
{
    return GetFileInformationByHandleEx(h, cls, info, len);
}

BOOL WINAPI EraReadFile(HANDLE h, LPVOID buf, DWORD toRead,
        LPDWORD pRead, LPOVERLAPPED ov)
{
    return ReadFile(h, buf, toRead, pRead, ov);
}

BOOL WINAPI EraWriteFile(HANDLE h, LPCVOID buf, DWORD toWrite,
        LPDWORD pWritten, LPOVERLAPPED ov)
{
    return WriteFile(h, buf, toWrite, pWritten, ov);
}

/* -----------------------------------------------------------------------
 * Misc
 * ----------------------------------------------------------------------- */

void WINAPI NtEnable32BitProcess(HANDLE hProc, UINT flags, LPVOID addr, UINT16 unk)
{
    FIXME("(%p, %u, %p, %u) stub\n", hProc, flags, addr, unk);
}

FARPROC WINAPI EraGetProcAddress(HMODULE hMod, LPCSTR name)
{
    TRACE("(%p, %s)\n", hMod, debugstr_a(name));
    return GetProcAddress(hMod, name);
}

/* GetVersionExW is exported from kernelx.dll on Xbox to allow version spoofing */
BOOL WINAPI EraGetVersionExW(OSVERSIONINFOW *info)
{
    TRACE("%p\n", info);
    if (info->dwOSVersionInfoSize < sizeof(OSVERSIONINFOW))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    info->dwMajorVersion = 10;
    info->dwMinorVersion = 0;
    info->dwBuildNumber  = 10586;
    info->dwPlatformId   = VER_PLATFORM_WIN32_NT;
    lstrcpyW(info->szCSDVersion, L"");
    return TRUE;
}
