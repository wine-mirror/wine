/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "tlhelp32.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolhelp);


/***********************************************************************
 *           CreateToolhelp32Snapshot			(KERNEL32.@)
 */
HANDLE WINAPI CreateToolhelp32Snapshot( DWORD flags, DWORD process )
{
    HANDLE ret;

    TRACE("%lx,%lx\n", flags, process );
    if (!(flags & (TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD|TH32CS_SNAPMODULE)))
    {
        FIXME("flags %lx not implemented\n", flags );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return INVALID_HANDLE_VALUE;
    }

    /* Now do the snapshot */
    SERVER_START_REQ( create_snapshot )
    {
        req->flags = 0;
        if (flags & TH32CS_SNAPMODULE)   req->flags |= SNAP_MODULE;
        if (flags & TH32CS_SNAPPROCESS)  req->flags |= SNAP_PROCESS;
        if (flags & TH32CS_SNAPTHREAD)   req->flags |= SNAP_THREAD;
    
        req->inherit = (flags & TH32CS_INHERIT) != 0;
        req->pid     = process;
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    if (!ret) ret = INVALID_HANDLE_VALUE;
    return ret;
}


/***********************************************************************
 *		TOOLHELP_Thread32Next
 *
 * Implementation of Thread32First/Next
 */
static BOOL TOOLHELP_Thread32Next( HANDLE handle, LPTHREADENTRY32 lpte, BOOL first )
{
    BOOL ret;

    if (lpte->dwSize < sizeof(THREADENTRY32))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ERR("Result buffer too small (%ld)\n", lpte->dwSize);
        return FALSE;
    }
    SERVER_START_REQ( next_thread )
    {
        req->handle = handle;
        req->reset = first;
        if ((ret = !wine_server_call_err( req )))
        {
            lpte->cntUsage           = reply->count;
            lpte->th32ThreadID       = (DWORD)reply->tid;
            lpte->th32OwnerProcessID = (DWORD)reply->pid;
            lpte->tpBasePri          = reply->base_pri;
            lpte->tpDeltaPri         = reply->delta_pri;
            lpte->dwFlags            = 0;  /* SDK: "reserved; do not use" */
        }
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *		Thread32First    (KERNEL32.@)
 *
 * Return info about the first thread in a toolhelp32 snapshot
 */
BOOL WINAPI Thread32First(HANDLE hSnapshot, LPTHREADENTRY32 lpte)
{
    return TOOLHELP_Thread32Next(hSnapshot, lpte, TRUE);
}

/***********************************************************************
 *		Thread32Next   (KERNEL32.@)
 *
 * Return info about the "next" thread in a toolhelp32 snapshot
 */
BOOL WINAPI Thread32Next(HANDLE hSnapshot, LPTHREADENTRY32 lpte)
{
    return TOOLHELP_Thread32Next(hSnapshot, lpte, FALSE);
}

/***********************************************************************
 *		TOOLHELP_Process32Next
 *
 * Implementation of Process32First/Next. Note that the ANSI / Unicode
 * version check is a bit of a hack as it relies on the fact that only
 * the last field is actually different.
 */
static BOOL TOOLHELP_Process32Next( HANDLE handle, LPPROCESSENTRY32W lppe, BOOL first, BOOL unicode )
{
    BOOL ret;
    WCHAR exe[MAX_PATH-1];
    DWORD len, sz;

    sz = unicode ? sizeof(PROCESSENTRY32W) : sizeof(PROCESSENTRY32);
    if (lppe->dwSize < sz)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ERR("Result buffer too small (req: %ld, was: %ld)\n", sz, lppe->dwSize);
        return FALSE;
    }

    SERVER_START_REQ( next_process )
    {
        req->handle = handle;
        req->reset = first;
        wine_server_set_reply( req, exe, sizeof(exe) );
        if ((ret = !wine_server_call_err( req )))
        {
            lppe->cntUsage            = reply->count;
            lppe->th32ProcessID       = reply->pid;
            lppe->th32DefaultHeapID   = (DWORD)reply->heap;
            lppe->th32ModuleID        = (DWORD)reply->module;
            lppe->cntThreads          = reply->threads;
            lppe->th32ParentProcessID = reply->ppid;
            switch (reply->priority)
            {
            case PROCESS_PRIOCLASS_IDLE:
                lppe->pcPriClassBase = IDLE_PRIORITY_CLASS; break;
            case PROCESS_PRIOCLASS_BELOW_NORMAL:
                lppe->pcPriClassBase = BELOW_NORMAL_PRIORITY_CLASS; break;
            case PROCESS_PRIOCLASS_NORMAL:
                lppe->pcPriClassBase = NORMAL_PRIORITY_CLASS; break;
            case PROCESS_PRIOCLASS_ABOVE_NORMAL:
                lppe->pcPriClassBase = ABOVE_NORMAL_PRIORITY_CLASS; break;
            case PROCESS_PRIOCLASS_HIGH:
                lppe->pcPriClassBase = HIGH_PRIORITY_CLASS; break;
            case PROCESS_PRIOCLASS_REALTIME:
                lppe->pcPriClassBase = REALTIME_PRIORITY_CLASS; break;
            default:
                FIXME("Unknown NT priority class %d, setting to normal\n", reply->priority);
                lppe->pcPriClassBase = NORMAL_PRIORITY_CLASS;
                break;
            }
            lppe->dwFlags             = -1; /* FIXME */
            if (unicode)
            {
                len = wine_server_reply_size(reply) / sizeof(WCHAR);
                memcpy(lppe->szExeFile, reply, wine_server_reply_size(reply));
                lppe->szExeFile[len] = 0;
            }
            else
            {
                LPPROCESSENTRY32 lppe_a = (LPPROCESSENTRY32) lppe;

                len = WideCharToMultiByte( CP_ACP, 0, exe, wine_server_reply_size(reply) / sizeof(WCHAR),
                                           lppe_a->szExeFile, sizeof(lppe_a->szExeFile)-1, NULL, NULL );
                lppe_a->szExeFile[len] = 0;
            }
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		Process32First    (KERNEL32.@)
 *
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, (LPPROCESSENTRY32W) lppe, TRUE, FALSE /* ANSI */ );
}

/***********************************************************************
 *		Process32Next   (KERNEL32.@)
 *
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, (LPPROCESSENTRY32W) lppe, FALSE, FALSE /* ANSI */ );
}

/***********************************************************************
 *		Process32FirstW    (KERNEL32.@)
 *
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32FirstW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, lppe, TRUE, TRUE /* Unicode */ );
}

/***********************************************************************
 *		Process32NextW   (KERNEL32.@)
 *
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32NextW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    return TOOLHELP_Process32Next( hSnapshot, lppe, FALSE, TRUE /* Unicode */ );
}


/***********************************************************************
 *		TOOLHELP_Module32NextW
 *
 * Implementation of Module32First/Next
 */
static BOOL TOOLHELP_Module32NextW( HANDLE handle, LPMODULEENTRY32W lpme, BOOL first )
{
    BOOL ret;
    WCHAR exe[MAX_PATH];
    DWORD len;

    if (lpme->dwSize < sizeof (MODULEENTRY32W))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ERR("Result buffer too small (req: %d, was: %ld)\n", sizeof(MODULEENTRY32W), lpme->dwSize);
        return FALSE;
    }
    SERVER_START_REQ( next_module )
    {
        req->handle = handle;
        req->reset = first;
        wine_server_set_reply( req, exe, sizeof(exe) );
        if ((ret = !wine_server_call_err( req )))
        {
            const WCHAR* ptr;
            lpme->th32ModuleID   = 1; /* toolhelp internal id, never used */
            lpme->th32ProcessID  = reply->pid;
            lpme->GlblcntUsage   = 0xFFFF; /* FIXME */
            lpme->ProccntUsage   = 0xFFFF; /* FIXME */
            lpme->modBaseAddr    = reply->base;
            lpme->modBaseSize    = reply->size;
            lpme->hModule        = reply->base;
            len = wine_server_reply_size(reply) / sizeof(WCHAR);
            memcpy(lpme->szExePath, exe, wine_server_reply_size(reply));
            lpme->szExePath[len] = 0;
            if ((ptr = strrchrW(lpme->szExePath, '\\'))) ptr++;
            else ptr = lpme->szExePath;
            lstrcpynW( lpme->szModule, ptr, sizeof(lpme->szModule)/sizeof(lpme->szModule[0]));
        }
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *		TOOLHELP_Module32NextA
 *
 * Implementation of Module32First/Next
 */
static BOOL TOOLHELP_Module32NextA( HANDLE handle, LPMODULEENTRY32 lpme, BOOL first )
{
    BOOL ret;
    MODULEENTRY32W mew;
    
    if (lpme->dwSize < sizeof (MODULEENTRY32))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ERR("Result buffer too small (req: %d, was: %ld)\n", sizeof(MODULEENTRY32), lpme->dwSize);
        return FALSE;
    }
    
    mew.dwSize = sizeof(mew);
    if ((ret = TOOLHELP_Module32NextW( handle, &mew, first )))
    {
        lpme->th32ModuleID  = mew.th32ModuleID;
        lpme->th32ProcessID = mew.th32ProcessID;
        lpme->GlblcntUsage  = mew.GlblcntUsage;
        lpme->ProccntUsage  = mew.ProccntUsage;
        lpme->modBaseAddr   = mew.modBaseAddr;
        lpme->hModule       = mew.hModule;
        WideCharToMultiByte( CP_ACP, 0, mew.szModule, -1, lpme->szModule, sizeof(lpme->szModule), NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, mew.szExePath, -1, lpme->szExePath, sizeof(lpme->szExePath), NULL, NULL );
    }
    return ret;
}

/***********************************************************************
 *		Module32FirstW   (KERNEL32.@)
 *
 * Return info about the "first" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32FirstW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    return TOOLHELP_Module32NextW( hSnapshot, lpme, TRUE );
}

/***********************************************************************
 *		Module32First   (KERNEL32.@)
 *
 * Return info about the "first" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
    return TOOLHELP_Module32NextA( hSnapshot, lpme, TRUE );
}

/***********************************************************************
 *		Module32NextW   (KERNEL32.@)
 *
 * Return info about the "next" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    return TOOLHELP_Module32NextW( hSnapshot, lpme, FALSE );
}

/***********************************************************************
 *		Module32Next   (KERNEL32.@)
 *
 * Return info about the "next" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
    return TOOLHELP_Module32NextA( hSnapshot, lpme, FALSE );
}

/************************************************************************
 *              Heap32ListFirst (KERNEL32.@)
 *
 */
BOOL WINAPI Heap32ListFirst(HANDLE hSnapshot, LPHEAPLIST32 lphl)
{
    FIXME(": stub\n");
    return FALSE;
}

/******************************************************************
 *		Toolhelp32ReadProcessMemory (KERNEL32.@)
 *
 *
 */
BOOL WINAPI Toolhelp32ReadProcessMemory(DWORD pid, const void* base,
                                        void* buf, SIZE_T len, SIZE_T* r)
{
    HANDLE h;
    BOOL   ret = FALSE;

    h = (pid) ? OpenProcess(PROCESS_VM_READ, FALSE, pid) : GetCurrentProcess();
    if (h != NULL)
    {
        ret = ReadProcessMemory(h, base, buf, len, r);
        if (pid) CloseHandle(h);
    }
    return ret;
}
