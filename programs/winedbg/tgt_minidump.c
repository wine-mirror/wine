/*
 * Wine debugger - minidump handling
 *
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "debugger.h"
#include "wingdi.h"
#include "winuser.h"
#include "tlhelp32.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

static struct be_process_io be_process_minidump_io;

void minidump_write(const char* file, const EXCEPTION_RECORD* rec)
{
    HANDLE                              hFile;
    MINIDUMP_EXCEPTION_INFORMATION      mei;
    EXCEPTION_POINTERS                  ep;

    hFile = CreateFile(file, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return;

    if (rec)
    {
        mei.ThreadId = dbg_curr_thread->tid;
        mei.ExceptionPointers = &ep;
        ep.ExceptionRecord = (EXCEPTION_RECORD*)rec;
        ep.ContextRecord = &dbg_context;
        mei.ClientPointers = FALSE;
    }
    MiniDumpWriteDump(dbg_curr_process->handle, dbg_curr_process->pid,
                      hFile, MiniDumpNormal/*|MiniDumpWithDataSegs*/,
                      rec ? &mei : NULL, NULL, NULL);
    CloseHandle(hFile);
}

#define Wine_ElfModuleListStream        0xFFF0

struct tgt_process_minidump_data
{
    void*       mapping;
    HANDLE      hFile;
    HANDLE      hMap;
};

static inline struct tgt_process_minidump_data* PRIVATE(struct dbg_process* pcs)
{
    return (struct tgt_process_minidump_data*)pcs->pio_data;
}

static BOOL WINAPI tgt_process_minidump_read(HANDLE hProcess, const void* addr, 
                                             void* buffer, SIZE_T len, SIZE_T* rlen)
{
    ULONG               size;
    MINIDUMP_DIRECTORY* dir;
    void*               stream;

    if (!PRIVATE(dbg_curr_process)->mapping) return FALSE;
    if (MiniDumpReadDumpStream(PRIVATE(dbg_curr_process)->mapping,
                               MemoryListStream, &dir, &stream, &size))
    {
        MINIDUMP_MEMORY_LIST*   mml = (MINIDUMP_MEMORY_LIST*)stream;
        MINIDUMP_MEMORY_DESCRIPTOR* mmd = &mml->MemoryRanges[0];
        int                     i;

        for (i = 0; i < mml->NumberOfMemoryRanges; i++, mmd++)
        {
            if (mmd->StartOfMemoryRange <= (DWORD_PTR)addr &&
                (DWORD_PTR)addr < mmd->StartOfMemoryRange + mmd->Memory.DataSize)
            {
                len = min(len, mmd->StartOfMemoryRange + mmd->Memory.DataSize - (DWORD_PTR)addr);
                memcpy(buffer, 
                       (char*)PRIVATE(dbg_curr_process)->mapping + mmd->Memory.Rva + (DWORD_PTR)addr - mmd->StartOfMemoryRange,
                       len);
                if (rlen) *rlen = len;
                return TRUE;
            }
        }
    }
    /* FIXME: this is a dirty hack to let the last frame in a bt to work
     * However, we need to check who's to blame, this code or the current 
     * dbghelp!StackWalk implementation
     */
    if ((DWORD_PTR)addr < 32)
    {
        memset(buffer, 0, len); 
        if (rlen) *rlen = len;
        return TRUE;
    }
    return FALSE;
}

static BOOL WINAPI tgt_process_minidump_write(HANDLE hProcess, void* addr,
                                             const void* buffer, SIZE_T len, SIZE_T* wlen)
{
    return FALSE;
}

BOOL CALLBACK validate_file(PCWSTR name, void* user)
{
    return FALSE; /* get the first file we find !! */
}

static BOOL is_pe_module_embedded(struct tgt_process_minidump_data* data,
                                  MINIDUMP_MODULE* pe_mm)
{
    ULONG                       size;
    MINIDUMP_DIRECTORY*         dir;
    MINIDUMP_MODULE_LIST*       mml;

    if (MiniDumpReadDumpStream(data->mapping, Wine_ElfModuleListStream, &dir,
                               (void**)&mml, &size))
    {
        MINIDUMP_MODULE*        mm;
        unsigned                i;

        for (i = 0, mm = &mml->Modules[0]; i < mml->NumberOfModules; i++, mm++)
        {
            if (mm->BaseOfImage <= pe_mm->BaseOfImage &&
                mm->BaseOfImage + mm->SizeOfImage >= pe_mm->BaseOfImage + pe_mm->SizeOfImage)
                return TRUE;
        }
    }
    return FALSE;
}

static enum dbg_start minidump_do_reload(struct tgt_process_minidump_data* data)
{
    ULONG                       size;
    MINIDUMP_DIRECTORY*         dir;
    void*                       stream;
    DWORD                       pid = 1; /* by default */
    HANDLE                      hProc = (HANDLE)0x900DBAAD;
    int                         i;
    MINIDUMP_MODULE_LIST*       mml;
    MINIDUMP_MODULE*            mm;
    MINIDUMP_STRING*            mds;
    char                        exec_name[1024];
    WCHAR                       nameW[1024];
    unsigned                    len;

    /* fetch PID */
    if (MiniDumpReadDumpStream(data->mapping, MiscInfoStream, &dir, &stream, &size))
    {
        MINIDUMP_MISC_INFO* mmi = (MINIDUMP_MISC_INFO*)stream;
        if (mmi->Flags1 & MINIDUMP_MISC1_PROCESS_ID)
            pid = mmi->ProcessId;
    }

    /* fetch executable name (it's normally the first one in module list) */
    strcpy(exec_name, "<minidump-exec>"); /* default */
    if (MiniDumpReadDumpStream(data->mapping, ModuleListStream, &dir, &stream, &size))
    {
        mml = (MINIDUMP_MODULE_LIST*)stream;
        if (mml->NumberOfModules)
        {
            char*               ptr;

            mm = &mml->Modules[0];
            mds = (MINIDUMP_STRING*)((char*)data->mapping + mm->ModuleNameRva);
            len = WideCharToMultiByte(CP_ACP, 0, mds->Buffer,
                                      mds->Length / sizeof(WCHAR),
                                      exec_name, sizeof(exec_name) - 1, NULL, NULL);
            exec_name[len] = 0;
            for (ptr = exec_name + len - 1; ptr >= exec_name; ptr--)
            {
                if (*ptr == '/' || *ptr == '\\')
                {
                    memmove(exec_name, ptr + 1, strlen(ptr + 1) + 1);
                    break;
                }
            }
        }
    }

    if (MiniDumpReadDumpStream(data->mapping, SystemInfoStream, &dir, &stream, &size))
    {
        MINIDUMP_SYSTEM_INFO*   msi = (MINIDUMP_SYSTEM_INFO*)stream;
        const char *str;
        char tmp[128];

        dbg_printf("WineDbg starting on minidump on pid %04x\n", pid);
        switch (msi->ProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_UNKNOWN: 
            str = "Unknown";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            strcpy(tmp, "Intel ");
            switch (msi->ProcessorLevel)
            {
            case 3: str = "80386"; break;
            case 4: str = "80486"; break;
            case 5: str = "Pentium"; break;
            case 6: str = "Pentium Pro/II"; break;
            default: str = "???"; break;
            }
            strcat(tmp, str);
            if (msi->ProcessorLevel == 3 || msi->ProcessorLevel == 4)
            {
                if (HIWORD(msi->ProcessorRevision) == 0xFF)
                    sprintf(tmp + strlen(tmp), "-%c%d",
                            'A' + HIBYTE(LOWORD(msi->ProcessorRevision)),
                            LOBYTE(LOWORD(msi->ProcessorRevision)));
                else
                    sprintf(tmp + strlen(tmp), "-%c%d",
                            'A' + HIWORD(msi->ProcessorRevision),
                            LOWORD(msi->ProcessorRevision));
            }
            else sprintf(tmp + strlen(tmp), "-%d.%d",
                         HIWORD(msi->ProcessorRevision),
                         LOWORD(msi->ProcessorRevision));
            str = tmp;
            break;
        case PROCESSOR_ARCHITECTURE_MIPS:
            str = "Mips";
            break;
        case PROCESSOR_ARCHITECTURE_ALPHA:
            str = "Alpha";
            break;
        case PROCESSOR_ARCHITECTURE_PPC:
            str = "PowerPC";
            break;
        default:
            str = "???";
            break;
        }
        dbg_printf("  %s was running on #%d %s CPU%s",
                   exec_name, msi->u.s.NumberOfProcessors, str, 
                   msi->u.s.NumberOfProcessors < 2 ? "" : "s");
        switch (msi->MajorVersion)
        {
        case 3:
            switch (msi->MinorVersion)
            {
            case 51: str = "NT 3.51"; break;
            default: str = "3-????"; break;
            }
            break;
        case 4:
            switch (msi->MinorVersion)
            {
            case 0: str = (msi->PlatformId == VER_PLATFORM_WIN32_NT) ? "NT 4.0" : "95"; break;
            case 10: str = "98"; break;
            case 90: str = "ME"; break;
            default: str = "5-????"; break;
            }
            break;
        case 5:
            switch (msi->MinorVersion)
            {
            case 0: str = "2000"; break;
            case 1: str = "XP"; break;
            case 2: str = "Server 2003"; break;
            default: str = "5-????"; break;
            }
            break;
        default: str = "???"; break;
        }
        dbg_printf(" on Windows %s (%u)\n", str, msi->BuildNumber);
        /* FIXME CSD: msi->CSDVersionRva */
    }

    dbg_curr_process = dbg_add_process(&be_process_minidump_io, pid, hProc);
    dbg_curr_pid = pid;
    dbg_curr_process->pio_data = data;
    dbg_set_process_name(dbg_curr_process, exec_name);

    SymInitialize(hProc, NULL, FALSE);

    if (MiniDumpReadDumpStream(data->mapping, ThreadListStream, &dir, &stream, &size))
    {
        MINIDUMP_THREAD_LIST*   mtl = (MINIDUMP_THREAD_LIST*)stream;
        ULONG                   i;

        for (i = 0; i < mtl->NumberOfThreads; i++)
        {
            dbg_add_thread(dbg_curr_process, mtl->Threads[i].ThreadId, NULL,
                           (void*)(DWORD_PTR)mtl->Threads[i].Teb);
        }
    }
    /* first load ELF modules, then do the PE ones */
    if (MiniDumpReadDumpStream(data->mapping, Wine_ElfModuleListStream, &dir,
                               &stream, &size))
    {
        WCHAR   buffer[MAX_PATH];

        mml = (MINIDUMP_MODULE_LIST*)stream;
        for (i = 0, mm = &mml->Modules[0]; i < mml->NumberOfModules; i++, mm++)
        {
            mds = (MINIDUMP_STRING*)((char*)data->mapping + mm->ModuleNameRva);
            memcpy(nameW, mds->Buffer, mds->Length);
            nameW[mds->Length / sizeof(WCHAR)] = 0;
            if (SymFindFileInPathW(hProc, NULL, nameW, (void*)(DWORD_PTR)mm->CheckSum,
                                   0, 0, SSRVOPT_DWORD, buffer, validate_file, NULL))
                SymLoadModuleExW(hProc, NULL, buffer, NULL, mm->BaseOfImage, mm->SizeOfImage,
                                 NULL, 0);
            else
                SymLoadModuleExW(hProc, NULL, nameW, NULL, mm->BaseOfImage, mm->SizeOfImage,
                                 NULL, SLMFLAG_VIRTUAL);
        }
    }
    if (MiniDumpReadDumpStream(data->mapping, ModuleListStream, &dir, &stream, &size))
    {
        WCHAR   buffer[MAX_PATH];

        mml = (MINIDUMP_MODULE_LIST*)stream;
        for (i = 0, mm = &mml->Modules[0]; i < mml->NumberOfModules; i++, mm++)
        {
            mds = (MINIDUMP_STRING*)((char*)data->mapping + mm->ModuleNameRva);
            memcpy(nameW, mds->Buffer, mds->Length);
            nameW[mds->Length / sizeof(WCHAR)] = 0;
            if (SymFindFileInPathW(hProc, NULL, nameW, (void*)(DWORD_PTR)mm->TimeDateStamp,
                                   mm->SizeOfImage, 0, SSRVOPT_DWORD, buffer, validate_file, NULL))
                SymLoadModuleExW(hProc, NULL, buffer, NULL, mm->BaseOfImage, mm->SizeOfImage,
                                 NULL, 0);
            else if (is_pe_module_embedded(data, mm))
                SymLoadModuleExW(hProc, NULL, nameW, NULL, mm->BaseOfImage, mm->SizeOfImage,
                                 NULL, 0);
            else
                SymLoadModuleExW(hProc, NULL, nameW, NULL, mm->BaseOfImage, mm->SizeOfImage,
                                 NULL, SLMFLAG_VIRTUAL);
        }
    }
    if (MiniDumpReadDumpStream(data->mapping, ExceptionStream, &dir, &stream, &size))
    {
        MINIDUMP_EXCEPTION_STREAM*      mes = (MINIDUMP_EXCEPTION_STREAM*)stream;

        if ((dbg_curr_thread = dbg_get_thread(dbg_curr_process, mes->ThreadId)))
        {
            ADDRESS64   addr;

            dbg_curr_tid = mes->ThreadId;
            dbg_curr_thread->in_exception = TRUE;
            dbg_curr_thread->excpt_record.ExceptionCode = mes->ExceptionRecord.ExceptionCode;
            dbg_curr_thread->excpt_record.ExceptionFlags = mes->ExceptionRecord.ExceptionFlags;
            dbg_curr_thread->excpt_record.ExceptionRecord = (void*)(DWORD_PTR)mes->ExceptionRecord.ExceptionRecord;
            dbg_curr_thread->excpt_record.ExceptionAddress = (void*)(DWORD_PTR)mes->ExceptionRecord.ExceptionAddress;
            dbg_curr_thread->excpt_record.NumberParameters = mes->ExceptionRecord.NumberParameters;
            for (i = 0; i < dbg_curr_thread->excpt_record.NumberParameters; i++)
            {
                dbg_curr_thread->excpt_record.ExceptionInformation[i] = mes->ExceptionRecord.ExceptionInformation[i];
            }
            memcpy(&dbg_context, (char*)data->mapping + mes->ThreadContext.Rva,
                   min(sizeof(dbg_context), mes->ThreadContext.DataSize));
            memory_get_current_pc(&addr);
            stack_fetch_frames();
            be_cpu->print_context(dbg_curr_thread->handle, &dbg_context, 0);
            stack_info();
            be_cpu->print_segment_info(dbg_curr_thread->handle, &dbg_context);
            stack_backtrace(mes->ThreadId);
            source_list_from_addr(&addr, 0);
        }
    }
    return start_ok;
}

static void cleanup(struct tgt_process_minidump_data* data)
{
    if (data->mapping)                          UnmapViewOfFile(data->mapping);
    if (data->hMap)                             CloseHandle(data->hMap);
    if (data->hFile != INVALID_HANDLE_VALUE)    CloseHandle(data->hFile);
    HeapFree(GetProcessHeap(), 0, data);
}

static struct be_process_io be_process_minidump_io;

enum dbg_start minidump_reload(int argc, char* argv[])
{
    struct tgt_process_minidump_data*   data;
    enum dbg_start                      ret = start_error_parse;

    /* try the form <myself> minidump-file */
    if (argc != 1) return start_error_parse;
    
    WINE_TRACE("Processing Minidump file %s\n", argv[0]);

    data = HeapAlloc(GetProcessHeap(), 0, sizeof(struct tgt_process_minidump_data));
    if (!data) return start_error_init;
    data->mapping = NULL;
    data->hMap    = NULL;
    data->hFile   = INVALID_HANDLE_VALUE;

    if ((data->hFile = CreateFileA(argv[0], GENERIC_READ, FILE_SHARE_READ, NULL, 
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE &&
        ((data->hMap = CreateFileMappingA(data->hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != 0) &&
        ((data->mapping = MapViewOfFile(data->hMap, FILE_MAP_READ, 0, 0, 0)) != NULL))
    {
        __TRY
        {
            if (((MINIDUMP_HEADER*)data->mapping)->Signature == MINIDUMP_SIGNATURE)
            {
                ret = minidump_do_reload(data);
            }
        }
        __EXCEPT_PAGE_FAULT
        {
            dbg_printf("Unexpected fault while reading minidump %s\n", argv[0]);
            dbg_curr_pid = 0;
        }
        __ENDTRY;
    }
    if (ret != start_ok) cleanup(data);
    return ret;
}

static BOOL tgt_process_minidump_close_process(struct dbg_process* pcs, BOOL kill)
{
    struct tgt_process_minidump_data*    data = PRIVATE(pcs);

    cleanup(data);
    pcs->pio_data = NULL;
    SymCleanup(pcs->handle);
    dbg_del_process(pcs);
    return TRUE;
}

static struct be_process_io be_process_minidump_io =
{
    tgt_process_minidump_close_process,
    tgt_process_minidump_read,
    tgt_process_minidump_write,
};
