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
#include "wine/debug.h"
#include "winnls.h"
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
 *           EnumProcesses (PSAPI.@)
 */
BOOL WINAPI EnumProcesses(DWORD *lpidProcess, DWORD cb, DWORD *lpcbNeeded)
{
    HANDLE	hSnapshot;
    DWORD	count;
    DWORD	countMax;
    DWORD       pid;
    int         ret;

    TRACE("(%p, %ld, %p)\n", lpidProcess,cb, lpcbNeeded);

    if ( lpidProcess == NULL )
        cb = 0;
    if ( lpcbNeeded != NULL )
        *lpcbNeeded = 0;

    SERVER_START_REQ( create_snapshot )
    {
        req->flags   = SNAP_PROCESS;
        req->inherit = FALSE;
        req->pid     = 0;
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
    countMax = cb / sizeof(DWORD);
    for (;;)
    {
        SERVER_START_REQ( next_process )
        {
            req->handle = hSnapshot;
            req->reset = (count == 0);
            if ((ret = !wine_server_call_err( req )))
                pid = reply->pid;
        }
        SERVER_END_REQ;
        if (!ret) break;
        TRACE("process 0x%08lx\n", pid);
        if ( count < countMax )
            lpidProcess[count] = pid;
        count++;
    }
    CloseHandle( hSnapshot );

    if ( lpcbNeeded != NULL )
        *lpcbNeeded = sizeof(DWORD) * count;

    TRACE("return %lu processes\n", count);

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
    char        tmp[MAX_PATH];
    char*       ptr;

    if (!GetModuleFileNameExA(hProcess, hModule, tmp, sizeof(tmp)))
        return 0;
    if ((ptr = strrchr(tmp, '\\')) != NULL) ptr++; else ptr = tmp;
    strncpy(lpBaseName, ptr, nSize);
    lpBaseName[nSize - 1] = '\0';
    return strlen(lpBaseName);
}

/***********************************************************************
 *           GetModuleBaseNameW (PSAPI.@)
 */
DWORD WINAPI GetModuleBaseNameW(HANDLE hProcess, HMODULE hModule, 
                                LPWSTR lpBaseName, DWORD nSize)
{
    char*       ptr;
    DWORD       len;

    TRACE("(hProcess=%p, hModule=%p, %p, %ld)\n",
          hProcess, hModule, lpBaseName, nSize);
   
    if (!lpBaseName || !nSize) return 0;

    ptr = HeapAlloc(GetProcessHeap(), 0, nSize / 2);
    if (!ptr) return 0;

    len = GetModuleBaseNameA(hProcess, hModule, ptr, nSize / 2);
    if (len == 0)
    {
        lpBaseName[0] = '\0';
    }
    else
    {
        if (!MultiByteToWideChar( CP_ACP, 0, ptr, -1, lpBaseName, nSize / 2 ))
            lpBaseName[nSize / 2 - 1] = 0;
    }

    HeapFree(GetProcessHeap(), 0, ptr);
    return len;
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
        return GetModuleFileNameA( hModule, lpFileName, nSize );

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
        return GetModuleFileNameW( hModule, lpFileName, nSize );

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
 *           GetProcessMemoryInfo (PSAPI.@)
 */
BOOL WINAPI GetProcessMemoryInfo(HANDLE Process, 
                                 PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb)
{
    FIXME("(hProcess=%p, %p, %ld): stub\n",
          Process, ppsmemCounters, cb);
    
    memset(ppsmemCounters, 0, cb);

    return TRUE;
}

/***********************************************************************
 *           GetWsChanges (PSAPI.@)
 */
BOOL WINAPI GetWsChanges(HANDLE hProcess, 
                         PPSAPI_WS_WATCH_INFORMATION lpWatchInfo, DWORD cb)
{
    FIXME("(hProcess=%p, %p, %ld): stub\n",
          hProcess, lpWatchInfo, cb);

    memset(lpWatchInfo, 0, cb);

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
 *           QueryWorkingSet (PSAPI.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
 */
BOOL WINAPI QueryWorkingSet(HANDLE hProcess, LPVOID pv, DWORD cb)
{
    FIXME("(hProcess=%p, %p, %ld)\n", hProcess, pv, cb);

    if (pv && cb)
        ((DWORD *) pv)[0] = 0; /* Empty WorkingSet */

    return TRUE;
}
