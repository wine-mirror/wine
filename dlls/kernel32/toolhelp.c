/*
 * Misc Toolhelp functions
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 2005 Eric Pouech
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "tlhelp32.h"
#include "winnls.h"
#include "winternl.h"

#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolhelp);

struct snapshot
{
    int         process_count;
    int         process_pos;
    int         process_offset;
    int         thread_count;
    int         thread_pos;
    int         thread_offset;
    int         module_count;
    int         module_pos;
    int         module_offset;
    char        data[1];
};

static WCHAR *fetch_string( HANDLE hProcess, UNICODE_STRING* us)
{
    WCHAR*      local;

    local = HeapAlloc( GetProcessHeap(), 0, us->Length );
    if (local)
    {
        if (!ReadProcessMemory( hProcess, us->Buffer, local, us->Length, NULL))
        {
            HeapFree( GetProcessHeap(), 0, local );
            local = NULL;
        }
    }
    us->Buffer = local;
    return local;
}

static BOOL fetch_module( DWORD process, DWORD flags, LDR_DATA_TABLE_ENTRY **ldr_mod, ULONG *num )
{
    HANDLE                      hProcess;
    PROCESS_BASIC_INFORMATION   pbi;
    PPEB_LDR_DATA               pLdrData;
    PLIST_ENTRY                 head, curr;
    BOOL                        ret = FALSE;

    *num = 0;

    if (!(flags & TH32CS_SNAPMODULE)) return TRUE;

    if (process)
    {
        hProcess = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, process );
        if (!hProcess) return FALSE;
    }
    else
        hProcess = GetCurrentProcess();

    if (set_ntstatus( NtQueryInformationProcess( hProcess, ProcessBasicInformation,
                                                 &pbi, sizeof(pbi), NULL )))
    {
        if (ReadProcessMemory( hProcess, &pbi.PebBaseAddress->LdrData,
                               &pLdrData, sizeof(pLdrData), NULL ) &&
            ReadProcessMemory( hProcess,
                               &pLdrData->InLoadOrderModuleList.Flink,
                               &curr, sizeof(curr), NULL ))
        {
            head = &pLdrData->InLoadOrderModuleList;

            while (curr != head)
            {
                if (!*num)
                    *ldr_mod = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LDR_DATA_TABLE_ENTRY) );
                else
                    *ldr_mod = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, *ldr_mod,
                                            (*num + 1) * sizeof(LDR_DATA_TABLE_ENTRY) );
                if (!*ldr_mod) break;
                if (!ReadProcessMemory( hProcess,
                                        CONTAINING_RECORD(curr, LDR_DATA_TABLE_ENTRY,
                                                          InLoadOrderLinks),
                                        &(*ldr_mod)[*num],
                                        sizeof(LDR_DATA_TABLE_ENTRY), NULL))
                    break;
                curr = (*ldr_mod)[*num].InLoadOrderLinks.Flink;
                /* if we cannot fetch the strings, then just ignore this LDR_DATA_TABLE_ENTRY
                 * and continue loading the other ones in the list
                 */
                if (!fetch_string( hProcess, &(*ldr_mod)[*num].BaseDllName )) continue;
                if (fetch_string( hProcess, &(*ldr_mod)[*num].FullDllName ))
                    (*num)++;
                else
                    HeapFree( GetProcessHeap(), 0, (*ldr_mod)[*num].BaseDllName.Buffer );
            }
            ret = TRUE;
        }
    }

    if (process) CloseHandle( hProcess );
    return ret;
}

static void fill_module( struct snapshot* snap, ULONG* offset, ULONG process,
                         LDR_DATA_TABLE_ENTRY* ldr_mod, ULONG num )
{
    MODULEENTRY32W*     mod;
    ULONG               i;
    SIZE_T              l;

    snap->module_count = num;
    snap->module_pos = 0;
    if (!num) return;
    snap->module_offset = *offset;

    mod = (MODULEENTRY32W*)&snap->data[*offset];

    for (i = 0; i < num; i++)
    {
        mod->dwSize = sizeof(MODULEENTRY32W);
        mod->th32ModuleID = 1; /* toolhelp internal id, never used */
        mod->th32ProcessID = process ? process : GetCurrentProcessId();
        mod->GlblcntUsage = 0xFFFF; /* FIXME */
        mod->ProccntUsage = 0xFFFF; /* FIXME */
        mod->modBaseAddr = ldr_mod[i].DllBase;
        mod->modBaseSize = ldr_mod[i].SizeOfImage;
        mod->hModule = ldr_mod[i].DllBase;

        l = min(ldr_mod[i].BaseDllName.Length, sizeof(mod->szModule) - sizeof(WCHAR));
        memcpy(mod->szModule, ldr_mod[i].BaseDllName.Buffer, l);
        mod->szModule[l / sizeof(WCHAR)] = '\0';
        l = min(ldr_mod[i].FullDllName.Length, sizeof(mod->szExePath) - sizeof(WCHAR));
        memcpy(mod->szExePath, ldr_mod[i].FullDllName.Buffer, l);
        mod->szExePath[l / sizeof(WCHAR)] = '\0';

        mod++;
    }

    *offset += num * sizeof(MODULEENTRY32W);
}

static BOOL fetch_process_thread( DWORD flags, SYSTEM_PROCESS_INFORMATION** pspi,
                                  ULONG* num_pcs, ULONG* num_thd)
{
    NTSTATUS                    status;
    ULONG                       size, offset;
    PSYSTEM_PROCESS_INFORMATION spi;

    *num_pcs = *num_thd = 0;
    if (!(flags & (TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD))) return TRUE;

    *pspi = HeapAlloc( GetProcessHeap(), 0, size = 4096 );
    for (;;)
    {
        status = NtQuerySystemInformation( SystemProcessInformation, *pspi,
                                           size, NULL );
        switch (status)
        {
        case STATUS_SUCCESS:
            *num_pcs = *num_thd = offset = 0;
            spi = *pspi;
            do
            {
                spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + offset);
                if (flags & TH32CS_SNAPPROCESS) (*num_pcs)++;
                if (flags & TH32CS_SNAPTHREAD) *num_thd += spi->dwThreadCount;
            } while ((offset = spi->NextEntryOffset));
            return TRUE;
        case STATUS_INFO_LENGTH_MISMATCH:
            *pspi = HeapReAlloc( GetProcessHeap(), 0, *pspi, size *= 2 );
            break;
        default:
            SetLastError( RtlNtStatusToDosError( status ) );
            return FALSE;
        }
    }
}

static void fill_process( struct snapshot* snap, ULONG* offset, 
                          SYSTEM_PROCESS_INFORMATION* spi, ULONG num )
{
    PROCESSENTRY32W*            pcs_entry;
    ULONG                       poff = 0;
    SIZE_T                      l;

    snap->process_count = num;
    snap->process_pos = 0;
    if (!num) return;
    snap->process_offset = *offset;

    pcs_entry = (PROCESSENTRY32W*)&snap->data[*offset];

    do
    {
        spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + poff);

        pcs_entry->dwSize = sizeof(PROCESSENTRY32W);
        pcs_entry->cntUsage = 0; /* MSDN says no longer used, always 0 */
        pcs_entry->th32ProcessID = HandleToUlong(spi->UniqueProcessId);
        pcs_entry->th32DefaultHeapID = 0; /* MSDN says no longer used, always 0 */
        pcs_entry->th32ModuleID = 0; /* MSDN says no longer used, always 0 */
        pcs_entry->cntThreads = spi->dwThreadCount;
        pcs_entry->th32ParentProcessID = HandleToUlong(spi->ParentProcessId);
        pcs_entry->pcPriClassBase = spi->dwBasePriority;
        pcs_entry->dwFlags = 0; /* MSDN says no longer used, always 0 */
        l = min(spi->ProcessName.Length, sizeof(pcs_entry->szExeFile) - sizeof(WCHAR));
        memcpy(pcs_entry->szExeFile, spi->ProcessName.Buffer, l);
        pcs_entry->szExeFile[l / sizeof(WCHAR)] = '\0';
        pcs_entry++;
    } while ((poff = spi->NextEntryOffset));

    *offset += num * sizeof(PROCESSENTRY32W);
}

static void fill_thread( struct snapshot* snap, ULONG* offset, LPVOID info, ULONG num )
{
    THREADENTRY32*              thd_entry;
    SYSTEM_PROCESS_INFORMATION* spi;
    SYSTEM_THREAD_INFORMATION*  sti;
    ULONG                       i, poff = 0;

    snap->thread_count = num;
    snap->thread_pos = 0;
    if (!num) return;
    snap->thread_offset = *offset;

    thd_entry = (THREADENTRY32*)&snap->data[*offset];

    spi = info;
    do
    {
        spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + poff);
        sti = &spi->ti[0];

        for (i = 0; i < spi->dwThreadCount; i++)
        {
            thd_entry->dwSize = sizeof(THREADENTRY32);
            thd_entry->cntUsage = 0; /* MSDN says no longer used, always 0 */
            thd_entry->th32ThreadID = HandleToUlong(sti->ClientId.UniqueThread);
            thd_entry->th32OwnerProcessID = HandleToUlong(sti->ClientId.UniqueProcess);
            thd_entry->tpBasePri = sti->dwBasePriority;
            thd_entry->tpDeltaPri = 0; /* MSDN says no longer used, always 0 */
            thd_entry->dwFlags = 0; /* MSDN says no longer used, always 0" */

            sti++;
            thd_entry++;
      }
    } while ((poff = spi->NextEntryOffset));
    *offset += num * sizeof(THREADENTRY32);
}

/***********************************************************************
 *           CreateToolhelp32Snapshot			(KERNEL32.@)
 */
HANDLE WINAPI CreateToolhelp32Snapshot( DWORD flags, DWORD process )
{
    SYSTEM_PROCESS_INFORMATION* spi = NULL;
    LDR_DATA_TABLE_ENTRY *mod = NULL;
    ULONG               num_pcs, num_thd, num_mod;
    HANDLE              hSnapShot = 0;

    TRACE("%lx,%lx\n", flags, process );
    if (!(flags & (TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD|TH32CS_SNAPMODULE)))
    {
        FIXME("flags %lx not implemented\n", flags );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return INVALID_HANDLE_VALUE;
    }

    if (fetch_module( process, flags, &mod, &num_mod ) &&
        fetch_process_thread( flags, &spi, &num_pcs, &num_thd ))
    {
        ULONG sect_size;
        struct snapshot*snap;
        SECURITY_ATTRIBUTES sa;

        /* create & fill the snapshot section */
        sect_size = sizeof(struct snapshot) - 1; /* for last data[1] */
        if (flags & TH32CS_SNAPMODULE)  sect_size += num_mod * sizeof(MODULEENTRY32W);
        if (flags & TH32CS_SNAPPROCESS) sect_size += num_pcs * sizeof(PROCESSENTRY32W);
        if (flags & TH32CS_SNAPTHREAD)  sect_size += num_thd * sizeof(THREADENTRY32);
        if (flags & TH32CS_SNAPHEAPLIST)FIXME("Unimplemented: heap list snapshot\n");

        sa.bInheritHandle = (flags & TH32CS_INHERIT) != 0;
        sa.lpSecurityDescriptor = NULL;

        hSnapShot = CreateFileMappingW( INVALID_HANDLE_VALUE, &sa,
                                        SEC_COMMIT | PAGE_READWRITE,
                                        0, sect_size, NULL );
        if (hSnapShot && (snap = MapViewOfFile( hSnapShot, FILE_MAP_ALL_ACCESS, 0, 0, 0 )))
        {
            DWORD   offset = 0;

            fill_module( snap, &offset, process, mod, num_mod );
            fill_process( snap, &offset, spi, num_pcs );
            fill_thread( snap, &offset, spi, num_thd );
            UnmapViewOfFile( snap );
        }
    }

    while (num_mod--)
    {
        HeapFree( GetProcessHeap(), 0, mod[num_mod].BaseDllName.Buffer );
        HeapFree( GetProcessHeap(), 0, mod[num_mod].FullDllName.Buffer );
    }
    HeapFree( GetProcessHeap(), 0, mod );
    HeapFree( GetProcessHeap(), 0, spi );

    if (!hSnapShot) return INVALID_HANDLE_VALUE;
    return hSnapShot;
}

static BOOL next_thread( HANDLE hSnapShot, LPTHREADENTRY32 lpte, BOOL first )
{
    struct snapshot*    snap;
    BOOL                ret = FALSE;

    if (lpte->dwSize < sizeof(THREADENTRY32))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        WARN("Result buffer too small (%ld)\n", lpte->dwSize);
        return FALSE;
    }
    if ((snap = MapViewOfFile( hSnapShot, FILE_MAP_ALL_ACCESS, 0, 0, 0 )))
    {
        if (first) snap->thread_pos = 0;
        if (snap->thread_pos < snap->thread_count)
        {
            LPTHREADENTRY32 te = (THREADENTRY32*)&snap->data[snap->thread_offset];
            *lpte = te[snap->thread_pos++];
            ret = TRUE;
        }
        else SetLastError( ERROR_NO_MORE_FILES );
        UnmapViewOfFile( snap );
    }
    return ret;
}

/***********************************************************************
 *		Thread32First    (KERNEL32.@)
 *
 * Return info about the first thread in a toolhelp32 snapshot
 */
BOOL WINAPI Thread32First( HANDLE hSnapShot, LPTHREADENTRY32 lpte )
{
    return next_thread( hSnapShot, lpte, TRUE );
}

/***********************************************************************
 *		Thread32Next    (KERNEL32.@)
 *
 * Return info about the first thread in a toolhelp32 snapshot
 */
BOOL WINAPI Thread32Next( HANDLE hSnapShot, LPTHREADENTRY32 lpte )
{
    return next_thread( hSnapShot, lpte, FALSE );
}

/***********************************************************************
 *		process_next
 *
 * Implementation of Process32First/Next. Note that the ANSI / Unicode
 * version check is a bit of a hack as it relies on the fact that only
 * the last field is actually different.
 */
static BOOL process_next( HANDLE hSnapShot, LPPROCESSENTRY32W lppe,
                          BOOL first, BOOL unicode )
{
    struct snapshot*    snap;
    BOOL                ret = FALSE;
    DWORD               sz = unicode ? sizeof(PROCESSENTRY32W) : sizeof(PROCESSENTRY32);

    if (lppe->dwSize < sz)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        WARN("Result buffer too small (%ld)\n", lppe->dwSize);
        return FALSE;
    }
    if ((snap = MapViewOfFile( hSnapShot, FILE_MAP_ALL_ACCESS, 0, 0, 0 )))
    {
        if (first) snap->process_pos = 0;
        if (snap->process_pos < snap->process_count)
        {
            LPPROCESSENTRY32W pe = (PROCESSENTRY32W*)&snap->data[snap->process_offset];
            if (unicode)
                *lppe = pe[snap->process_pos];
            else
            {
                lppe->cntUsage = pe[snap->process_pos].cntUsage;
                lppe->th32ProcessID = pe[snap->process_pos].th32ProcessID;
                lppe->th32DefaultHeapID = pe[snap->process_pos].th32DefaultHeapID;
                lppe->th32ModuleID = pe[snap->process_pos].th32ModuleID;
                lppe->cntThreads = pe[snap->process_pos].cntThreads;
                lppe->th32ParentProcessID = pe[snap->process_pos].th32ParentProcessID;
                lppe->pcPriClassBase = pe[snap->process_pos].pcPriClassBase;
                lppe->dwFlags = pe[snap->process_pos].dwFlags;

                WideCharToMultiByte( CP_ACP, 0, pe[snap->process_pos].szExeFile, -1,
                                     (char*)lppe->szExeFile, sizeof(lppe->szExeFile),
                                     0, 0 );
            }
            snap->process_pos++;
            ret = TRUE;
        }
        else SetLastError( ERROR_NO_MORE_FILES );
        UnmapViewOfFile( snap );
    }

    return ret;
}


/***********************************************************************
 *		Process32First    (KERNEL32.@)
 *
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    return process_next( hSnapshot, (PROCESSENTRY32W*)lppe, TRUE, FALSE /* ANSI */ );
}

/***********************************************************************
 *		Process32Next   (KERNEL32.@)
 *
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe)
{
    return process_next( hSnapshot, (PROCESSENTRY32W*)lppe, FALSE, FALSE /* ANSI */ );
}

/***********************************************************************
 *		Process32FirstW    (KERNEL32.@)
 *
 * Return info about the first process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32FirstW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    return process_next( hSnapshot, lppe, TRUE, TRUE /* Unicode */ );
}

/***********************************************************************
 *		Process32NextW   (KERNEL32.@)
 *
 * Return info about the "next" process in a toolhelp32 snapshot
 */
BOOL WINAPI Process32NextW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe)
{
    return process_next( hSnapshot, lppe, FALSE, TRUE /* Unicode */ );
}

/***********************************************************************
 *		module_nextW
 *
 * Implementation of Module32{First|Next}W
 */
static BOOL module_nextW( HANDLE hSnapShot, LPMODULEENTRY32W lpme, BOOL first )
{
    struct snapshot*    snap;
    BOOL                ret = FALSE;

    if (lpme->dwSize < sizeof (MODULEENTRY32W))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        WARN("Result buffer too small (was: %ld)\n", lpme->dwSize);
        return FALSE;
    }
    if ((snap = MapViewOfFile( hSnapShot, FILE_MAP_ALL_ACCESS, 0, 0, 0 )))
    {
        if (first) snap->module_pos = 0;
        if (snap->module_pos < snap->module_count)
        {
            LPMODULEENTRY32W pe = (MODULEENTRY32W*)&snap->data[snap->module_offset];
            *lpme = pe[snap->module_pos++];
            ret = TRUE;
        }
        else SetLastError( ERROR_NO_MORE_FILES );
        UnmapViewOfFile( snap );
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
    return module_nextW( hSnapshot, lpme, TRUE );
}

/***********************************************************************
 *		Module32NextW   (KERNEL32.@)
 *
 * Return info about the "next" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    return module_nextW( hSnapshot, lpme, FALSE );
}

/***********************************************************************
 *		module_nextA
 *
 * Implementation of Module32{First|Next}A
 */
static BOOL module_nextA( HANDLE handle, LPMODULEENTRY32 lpme, BOOL first )
{
    BOOL ret;
    MODULEENTRY32W mew;

    if (lpme->dwSize < sizeof(MODULEENTRY32))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        WARN("Result buffer too small (was: %ld)\n", lpme->dwSize);
        return FALSE;
    }

    mew.dwSize = sizeof(mew);
    if ((ret = module_nextW( handle, &mew, first )))
    {
        lpme->th32ModuleID  = mew.th32ModuleID;
        lpme->th32ProcessID = mew.th32ProcessID;
        lpme->GlblcntUsage  = mew.GlblcntUsage;
        lpme->ProccntUsage  = mew.ProccntUsage;
        lpme->modBaseAddr   = mew.modBaseAddr;
        lpme->modBaseSize   = mew.modBaseSize;
        lpme->hModule       = mew.hModule;
        WideCharToMultiByte( CP_ACP, 0, mew.szModule, -1, lpme->szModule, sizeof(lpme->szModule), NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, mew.szExePath, -1, lpme->szExePath, sizeof(lpme->szExePath), NULL, NULL );
    }
    return ret;
}

/***********************************************************************
 *		Module32First   (KERNEL32.@)
 *
 * Return info about the "first" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32First(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
    return module_nextA( hSnapshot, lpme, TRUE );
}

/***********************************************************************
 *		Module32Next   (KERNEL32.@)
 *
 * Return info about the "next" module in a toolhelp32 snapshot
 */
BOOL WINAPI Module32Next(HANDLE hSnapshot, LPMODULEENTRY32 lpme)
{
    return module_nextA( hSnapshot, lpme, FALSE );
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
