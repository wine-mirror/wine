/*
 * File minidump.c - management of dumps (read & write)
 *
 * Copyright (C) 2004-2005, Eric Pouech
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

#include <time.h>
#include <intrin.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "winternl.h"
#include "psapi.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

/******************************************************************
 *		fetch_process_info
 *
 * reads system wide process information, and gather from it the threads information
 * for process of id 'pid'
 */
static BOOL fetch_process_info(struct dump_context* dc)
{
    ULONG       buf_size = 0x1000;
    NTSTATUS    nts;
    void*       pcs_buffer = NULL;

    if (!(pcs_buffer = HeapAlloc(GetProcessHeap(), 0, buf_size))) return FALSE;
    for (;;)
    {
        nts = NtQuerySystemInformation(SystemProcessInformation,
                                       pcs_buffer, buf_size, NULL);
        if (nts != STATUS_INFO_LENGTH_MISMATCH) break;
        pcs_buffer = HeapReAlloc(GetProcessHeap(), 0, pcs_buffer, buf_size *= 2);
        if (!pcs_buffer) return FALSE;
    }

    if (nts == STATUS_SUCCESS)
    {
        SYSTEM_PROCESS_INFORMATION*     spi = pcs_buffer;
        unsigned                        i;

        for (;;)
        {
            if (HandleToUlong(spi->UniqueProcessId) == dc->pid)
            {
                dc->num_threads = 0;
                dc->threads = HeapAlloc(GetProcessHeap(), 0,
                                        spi->dwThreadCount * sizeof(dc->threads[0]));
                if (!dc->threads) break;
                for (i = 0; i < spi->dwThreadCount; i++)
                {
                    /* don't include current thread */
                    if (HandleToULong(spi->ti[i].ClientId.UniqueThread) == GetCurrentThreadId())
                        continue;
                    dc->threads[dc->num_threads].tid        = HandleToULong(spi->ti[i].ClientId.UniqueThread);
                    dc->threads[dc->num_threads].prio_class = spi->ti[i].dwBasePriority; /* FIXME */
                    dc->threads[dc->num_threads].curr_prio  = spi->ti[i].dwCurrentPriority;
                    dc->num_threads++;
                }
                HeapFree(GetProcessHeap(), 0, pcs_buffer);
                return TRUE;
            }
            if (!spi->NextEntryOffset) break;
            spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + spi->NextEntryOffset);
        }
    }
    HeapFree(GetProcessHeap(), 0, pcs_buffer);
    return FALSE;
}

static void fetch_thread_stack(struct dump_context* dc, const void* teb_addr,
                               const CONTEXT* ctx, MINIDUMP_MEMORY_DESCRIPTOR* mmd)
{
    NT_TIB      tib;
    ADDRESS64   addr;

    if (ReadProcessMemory(dc->process->handle, teb_addr, &tib, sizeof(tib), NULL) &&
        dbghelp_current_cpu &&
        dbghelp_current_cpu->get_addr(NULL /* FIXME */, ctx, cpu_addr_stack, &addr) && addr.Mode == AddrModeFlat)
    {
        if (addr.Offset)
        {
            addr.Offset -= dbghelp_current_cpu->word_size;
            /* make sure stack pointer is within the established range of the stack.  It could have
               been clobbered by whatever caused the original exception. */
            if (addr.Offset < (ULONG_PTR)tib.StackLimit || addr.Offset > (ULONG_PTR)tib.StackBase)
                mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;

            else
                mmd->StartOfMemoryRange = addr.Offset;
        }
        else
            mmd->StartOfMemoryRange = (ULONG_PTR)tib.StackLimit;
        mmd->Memory.DataSize = (ULONG_PTR)tib.StackBase - mmd->StartOfMemoryRange;
    }
}

/******************************************************************
 *		fetch_thread_info
 *
 * fetches some information about thread of id 'tid'
 */
static BOOL fetch_thread_info(struct dump_context* dc, int thd_idx,
                              MINIDUMP_THREAD* mdThd, CONTEXT* ctx)
{
    DWORD                       tid = dc->threads[thd_idx].tid;
    HANDLE                      hThread;
    THREAD_BASIC_INFORMATION    tbi;

    memset(ctx, 0, sizeof(*ctx));

    mdThd->ThreadId = tid;
    mdThd->SuspendCount = 0;
    mdThd->Teb = 0;
    mdThd->Stack.StartOfMemoryRange = 0;
    mdThd->Stack.Memory.DataSize = 0;
    mdThd->Stack.Memory.Rva = 0;
    mdThd->ThreadContext.DataSize = 0;
    mdThd->ThreadContext.Rva = 0;
    mdThd->PriorityClass = dc->threads[thd_idx].prio_class;
    mdThd->Priority = dc->threads[thd_idx].curr_prio;

    if ((hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid)) == NULL)
    {
        FIXME("Couldn't open thread %lu (%lu)\n", tid, GetLastError());
        return FALSE;
    }

    if (NtQueryInformationThread(hThread, ThreadBasicInformation,
                                 &tbi, sizeof(tbi), NULL) == STATUS_SUCCESS)
    {
        mdThd->Teb = (ULONG_PTR)tbi.TebBaseAddress;
        if (tbi.ExitStatus == STILL_ACTIVE)
        {
            mdThd->SuspendCount = SuspendThread(hThread);
            ctx->ContextFlags = CONTEXT_ALL;
            if (!GetThreadContext(hThread, ctx))
                memset(ctx, 0, sizeof(*ctx));
            fetch_thread_stack(dc, tbi.TebBaseAddress, ctx, &mdThd->Stack);
            ResumeThread(hThread);
        }
        else
            mdThd->SuspendCount = (DWORD)-1;
    }
    CloseHandle(hThread);
    return TRUE;
}

/******************************************************************
 *		add_module
 *
 * Add a module to a dump context
 */
static BOOL add_module(struct dump_context* dc, const WCHAR* name,
                       DWORD64 base, DWORD size, DWORD timestamp, DWORD checksum,
                       BOOL is_elf)
{
    if (!dc->modules)
    {
        dc->alloc_modules = 32;
        dc->modules = HeapAlloc(GetProcessHeap(), 0,
                                dc->alloc_modules * sizeof(*dc->modules));
    }
    else if(dc->num_modules >= dc->alloc_modules)
    {
        dc->alloc_modules *= 2;
        dc->modules = HeapReAlloc(GetProcessHeap(), 0, dc->modules,
                                  dc->alloc_modules * sizeof(*dc->modules));
    }
    if (!dc->modules)
    {
        dc->alloc_modules = dc->num_modules = 0;
        return FALSE;
    }
    lstrcpynW(dc->modules[dc->num_modules].name, name,
              ARRAY_SIZE(dc->modules[dc->num_modules].name));
    dc->modules[dc->num_modules].base = base;
    dc->modules[dc->num_modules].size = size;
    dc->modules[dc->num_modules].timestamp = timestamp;
    dc->modules[dc->num_modules].checksum = checksum;
    dc->modules[dc->num_modules].is_elf = is_elf;
    dc->num_modules++;

    return TRUE;
}

/******************************************************************
 *		fetch_pe_module_info_cb
 *
 * Callback for accumulating in dump_context a PE modules set
 */
static BOOL WINAPI fetch_pe_module_info_cb(PCWSTR name, DWORD64 base, ULONG size,
                                           PVOID user)
{
    struct dump_context*        dc = user;
    IMAGE_NT_HEADERS            nth;

    if (!validate_addr64(base)) return FALSE;

    if (pe_load_nt_header(dc->process->handle, base, &nth, NULL))
        add_module(user, name, base, size,
                   nth.FileHeader.TimeDateStamp, nth.OptionalHeader.CheckSum,
                   FALSE);
    return TRUE;
}

/******************************************************************
 *		fetch_elf_module_info_cb
 *
 * Callback for accumulating in dump_context an host modules set
 */
static BOOL fetch_host_module_info_cb(const WCHAR* name, ULONG_PTR base,
                                     void* user)
{
    struct dump_context*        dc = user;
    DWORD_PTR                   rbase;
    DWORD                       size, checksum;

    /* FIXME: there's no relevant timestamp on ELF modules */
    if (!dc->process->loader->fetch_file_info(dc->process, name, base, &rbase, &size, &checksum))
        size = checksum = 0;
    add_module(dc, name, base ? base : rbase, size, 0 /* FIXME */, checksum, TRUE);
    return TRUE;
}

static void minidump_add_memory64_block(struct dump_context* dc, ULONG64 base, ULONG64 size)
{
    if (!dc->mem64)
    {
        dc->alloc_mem64 = 32;
        dc->mem64 = HeapAlloc(GetProcessHeap(), 0, dc->alloc_mem64 * sizeof(*dc->mem64));
    }
    else if (dc->num_mem64 >= dc->alloc_mem64)
    {
        dc->alloc_mem64 *= 2;
        dc->mem64 = HeapReAlloc(GetProcessHeap(), 0, dc->mem64,
                                dc->alloc_mem64 * sizeof(*dc->mem64));
    }
    if (dc->mem64)
    {
        dc->mem64[dc->num_mem64].base = base;
        dc->mem64[dc->num_mem64].size = size;
        dc->num_mem64++;
    }
    else dc->num_mem64 = dc->alloc_mem64 = 0;
}

static void fetch_memory64_info(struct dump_context* dc)
{
    ULONG_PTR                   addr;
    MEMORY_BASIC_INFORMATION    mbi;

    addr = 0;
    while (VirtualQueryEx(dc->process->handle, (LPCVOID)addr, &mbi, sizeof(mbi)) != 0)
    {
        /* Memory regions with state MEM_COMMIT will be added to the dump */
        if (mbi.State == MEM_COMMIT)
        {
            minidump_add_memory64_block(dc, (ULONG_PTR)mbi.BaseAddress, mbi.RegionSize);
        }

        if ((addr + mbi.RegionSize) < addr)
            break;

        addr = (ULONG_PTR)mbi.BaseAddress + mbi.RegionSize;
    }
}

static void fetch_modules_info(struct dump_context* dc)
{
    EnumerateLoadedModulesW64(dc->process->handle, fetch_pe_module_info_cb, dc);
    /* Since we include ELF modules in a separate stream from the regular PE ones,
     * we can always include those ELF modules (they don't eat lots of space)
     * And it's always a good idea to have a trace of the loaded ELF modules for
     * a given application in a post mortem debugging condition.
     */
    dc->process->loader->enum_modules(dc->process, fetch_host_module_info_cb, dc);
}

static void fetch_module_versioninfo(LPCWSTR filename, VS_FIXEDFILEINFO* ffi)
{
    DWORD       handle;
    DWORD       sz;

    memset(ffi, 0, sizeof(*ffi));
    if ((sz = GetFileVersionInfoSizeW(filename, &handle)))
    {
        void*   info = HeapAlloc(GetProcessHeap(), 0, sz);
        if (info && GetFileVersionInfoW(filename, handle, sz, info))
        {
            VS_FIXEDFILEINFO*   ptr;
            UINT    len;

            if (VerQueryValueW(info, L"\\", (void*)&ptr, &len))
                memcpy(ffi, ptr, min(len, sizeof(*ffi)));
        }
        HeapFree(GetProcessHeap(), 0, info);
    }
}

/******************************************************************
 *		minidump_add_memory_block
 *
 * Add a memory block to be dumped in a minidump
 * If rva is non 0, it's the rva in the minidump where has to be stored
 * also the rva of the memory block when written (this allows us to reference
 * a memory block from outside the list of memory blocks).
 */
void minidump_add_memory_block(struct dump_context* dc, ULONG64 base, ULONG size, ULONG rva)
{
    if (!dc->mem)
    {
        dc->alloc_mem = 32;
        dc->mem = HeapAlloc(GetProcessHeap(), 0, dc->alloc_mem * sizeof(*dc->mem));
    }
    else if (dc->num_mem >= dc->alloc_mem)
    {
        dc->alloc_mem *= 2;
        dc->mem = HeapReAlloc(GetProcessHeap(), 0, dc->mem,
                              dc->alloc_mem * sizeof(*dc->mem));
    }
    if (dc->mem)
    {
        dc->mem[dc->num_mem].base = base;
        dc->mem[dc->num_mem].size = size;
        dc->mem[dc->num_mem].rva  = rva;
        dc->num_mem++;
    }
    else dc->num_mem = dc->alloc_mem = 0;
}

/******************************************************************
 *		writeat
 *
 * Writes a chunk of data at a given position in the minidump
 */
static void writeat(struct dump_context* dc, RVA rva, const void* data, unsigned size)
{
    DWORD       written;

    SetFilePointer(dc->hFile, rva, NULL, FILE_BEGIN);
    WriteFile(dc->hFile, data, size, &written, NULL);
}

/******************************************************************
 *		append
 *
 * writes a new chunk of data to the minidump, increasing the current
 * rva in dc
 */
static void append(struct dump_context* dc, const void* data, unsigned size)
{
    writeat(dc, dc->rva, data, size);
    dc->rva += size;
}

/******************************************************************
 *		dump_exception_info
 *
 * Write in File the exception information from pcs
 */
static  unsigned        dump_exception_info(struct dump_context* dc)
{
    MINIDUMP_EXCEPTION_STREAM   mdExcpt;
    EXCEPTION_RECORD            rec, *prec;
    CONTEXT                     ctx, *pctx;
    DWORD                       i;

    mdExcpt.ThreadId = dc->except_param->ThreadId;
    mdExcpt.__alignment = 0;
    if (dc->except_param->ClientPointers)
    {
        EXCEPTION_POINTERS      ep;

        ReadProcessMemory(dc->process->handle,
                          dc->except_param->ExceptionPointers, &ep, sizeof(ep), NULL);
        ReadProcessMemory(dc->process->handle,
                          ep.ExceptionRecord, &rec, sizeof(rec), NULL);
        ReadProcessMemory(dc->process->handle,
                          ep.ContextRecord, &ctx, sizeof(ctx), NULL);
        prec = &rec;
        pctx = &ctx;
    }
    else
    {
        prec = dc->except_param->ExceptionPointers->ExceptionRecord;
        pctx = dc->except_param->ExceptionPointers->ContextRecord;
    }
    mdExcpt.ExceptionRecord.ExceptionCode = prec->ExceptionCode;
    mdExcpt.ExceptionRecord.ExceptionFlags = prec->ExceptionFlags;
    mdExcpt.ExceptionRecord.ExceptionRecord = (DWORD_PTR)prec->ExceptionRecord;
    mdExcpt.ExceptionRecord.ExceptionAddress = (DWORD_PTR)prec->ExceptionAddress;
    mdExcpt.ExceptionRecord.NumberParameters = prec->NumberParameters;
    mdExcpt.ExceptionRecord.__unusedAlignment = 0;
    for (i = 0; i < mdExcpt.ExceptionRecord.NumberParameters; i++)
        mdExcpt.ExceptionRecord.ExceptionInformation[i] = prec->ExceptionInformation[i];
    mdExcpt.ThreadContext.DataSize = sizeof(*pctx);
    mdExcpt.ThreadContext.Rva = dc->rva + sizeof(mdExcpt);

    append(dc, &mdExcpt, sizeof(mdExcpt));
    append(dc, pctx, sizeof(*pctx));
    return sizeof(mdExcpt);
}

/******************************************************************
 *		dump_modules
 *
 * Write in File the modules from pcs
 */
static  unsigned        dump_modules(struct dump_context* dc, BOOL dump_elf)
{
    MINIDUMP_MODULE             mdModule;
    MINIDUMP_MODULE_LIST        mdModuleList;
    char                        tmp[1024];
    MINIDUMP_STRING*            ms = (MINIDUMP_STRING*)tmp;
    ULONG                       i, nmod;
    RVA                         rva_base;
    DWORD                       flags_out;
    unsigned                    sz;

    for (i = nmod = 0; i < dc->num_modules; i++)
    {
        if ((dc->modules[i].is_elf && dump_elf) ||
            (!dc->modules[i].is_elf && !dump_elf))
            nmod++;
    }

    mdModuleList.NumberOfModules = 0;
    /* reserve space for mdModuleList
     * FIXME: since we don't support 0 length arrays, we cannot use the
     * size of mdModuleList
     * FIXME: if we don't ask for all modules in cb, we'll get a hole in the file
     */

    /* the stream size is just the size of the module index.  It does not include the data for the
       names of each module.  *Technically* the names are supposed to go into the common string table
       in the minidump file.  Since each string is referenced by RVA they can all safely be located
       anywhere between streams in the file, so the end of this stream is sufficient. */
    rva_base = dc->rva;
    dc->rva += sz = sizeof(mdModuleList.NumberOfModules) + sizeof(mdModule) * nmod;
    for (i = 0; i < dc->num_modules; i++)
    {
        if ((dc->modules[i].is_elf && !dump_elf) ||
            (!dc->modules[i].is_elf && dump_elf))
            continue;

        flags_out = ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord;
        if (dc->type & MiniDumpWithDataSegs)
            flags_out |= ModuleWriteDataSeg;
        if (dc->type & MiniDumpWithProcessThreadData)
            flags_out |= ModuleWriteTlsData;
        if (dc->type & MiniDumpWithCodeSegs)
            flags_out |= ModuleWriteCodeSegs;
        ms->Length = (lstrlenW(dc->modules[i].name) + 1) * sizeof(WCHAR);
        if (sizeof(ULONG) + ms->Length > sizeof(tmp))
            FIXME("Buffer overflow!!!\n");
        lstrcpyW(ms->Buffer, dc->modules[i].name);

        if (dc->cb)
        {
            MINIDUMP_CALLBACK_INPUT     cbin;
            MINIDUMP_CALLBACK_OUTPUT    cbout;

            cbin.ProcessId = dc->pid;
            cbin.ProcessHandle = dc->process->handle;
            cbin.CallbackType = ModuleCallback;

            cbin.Module.FullPath = ms->Buffer;
            cbin.Module.BaseOfImage = dc->modules[i].base;
            cbin.Module.SizeOfImage = dc->modules[i].size;
            cbin.Module.CheckSum = dc->modules[i].checksum;
            cbin.Module.TimeDateStamp = dc->modules[i].timestamp;
            memset(&cbin.Module.VersionInfo, 0, sizeof(cbin.Module.VersionInfo));
            cbin.Module.CvRecord = NULL;
            cbin.Module.SizeOfCvRecord = 0;
            cbin.Module.MiscRecord = NULL;
            cbin.Module.SizeOfMiscRecord = 0;

            cbout.ModuleWriteFlags = flags_out;
            if (!dc->cb->CallbackRoutine(dc->cb->CallbackParam, &cbin, &cbout))
                continue;
            flags_out &= cbout.ModuleWriteFlags;
        }
        if (flags_out & ModuleWriteModule)
        {
            /* fetch CPU dependent module info (like UNWIND_INFO) */
            dbghelp_current_cpu->fetch_minidump_module(dc, i, flags_out);

            mdModule.BaseOfImage = dc->modules[i].base;
            mdModule.SizeOfImage = dc->modules[i].size;
            mdModule.CheckSum = dc->modules[i].checksum;
            mdModule.TimeDateStamp = dc->modules[i].timestamp;
            mdModule.ModuleNameRva = dc->rva;
            ms->Length -= sizeof(WCHAR);
            append(dc, ms, sizeof(ULONG) + ms->Length + sizeof(WCHAR));
            if (!dump_elf) fetch_module_versioninfo(ms->Buffer, &mdModule.VersionInfo);
            else memset(&mdModule.VersionInfo, 0, sizeof(mdModule.VersionInfo));
            mdModule.CvRecord.DataSize = 0; /* FIXME */
            mdModule.CvRecord.Rva = 0; /* FIXME */
            mdModule.MiscRecord.DataSize = 0; /* FIXME */
            mdModule.MiscRecord.Rva = 0; /* FIXME */
            mdModule.Reserved0 = 0; /* FIXME */
            mdModule.Reserved1 = 0; /* FIXME */
            writeat(dc,
                    rva_base + sizeof(mdModuleList.NumberOfModules) + 
                        mdModuleList.NumberOfModules++ * sizeof(mdModule), 
                    &mdModule, sizeof(mdModule));
        }
    }
    writeat(dc, rva_base, &mdModuleList.NumberOfModules, 
            sizeof(mdModuleList.NumberOfModules));

    return sz;
}


/******************************************************************
 *		dump_system_info
 *
 * Dumps into File the information about the system
 */
static  unsigned        dump_system_info(struct dump_context* dc)
{
    MINIDUMP_SYSTEM_INFO        mdSysInfo;
    SYSTEM_INFO                 sysInfo;
    RTL_OSVERSIONINFOEXW        osInfo;
    DWORD                       written;
    ULONG                       slen;
    DWORD                       wine_extra = 0;

    const char *(CDECL *wine_get_build_id)(void);
    void (CDECL *wine_get_host_version)(const char **sysname, const char **release);
    const char* build_id = NULL;
    const char* sys_name = NULL;
    const char* release_name = NULL;

    GetSystemInfo(&sysInfo);
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    RtlGetVersion(&osInfo);

    wine_get_build_id = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_build_id");
    wine_get_host_version = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_host_version");
    if (wine_get_build_id && wine_get_host_version)
    {
        /* cheat minidump system information by adding specific wine information */
        wine_extra = 4 + 4 * sizeof(slen);
        build_id = wine_get_build_id();
        wine_get_host_version(&sys_name, &release_name);
        wine_extra += strlen(build_id) + 1 + strlen(sys_name) + 1 + strlen(release_name) + 1;
    }

    mdSysInfo.ProcessorArchitecture = sysInfo.wProcessorArchitecture;
    mdSysInfo.ProcessorLevel = sysInfo.wProcessorLevel;
    mdSysInfo.ProcessorRevision = sysInfo.wProcessorRevision;
    mdSysInfo.NumberOfProcessors = sysInfo.dwNumberOfProcessors;
    mdSysInfo.ProductType = VER_NT_WORKSTATION; /* FIXME */
    mdSysInfo.MajorVersion = osInfo.dwMajorVersion;
    mdSysInfo.MinorVersion = osInfo.dwMinorVersion;
    mdSysInfo.BuildNumber = osInfo.dwBuildNumber;
    mdSysInfo.PlatformId = osInfo.dwPlatformId;

    mdSysInfo.CSDVersionRva = dc->rva + sizeof(mdSysInfo) + wine_extra;
    mdSysInfo.Reserved1 = 0;
    mdSysInfo.SuiteMask = VER_SUITE_TERMINAL;

#if defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__))
    {
        int regs[4];

        __cpuid(regs, 0);
        mdSysInfo.Cpu.X86CpuInfo.VendorId[0] = regs[1];
        mdSysInfo.Cpu.X86CpuInfo.VendorId[1] = regs[3];
        mdSysInfo.Cpu.X86CpuInfo.VendorId[2] = regs[2];
        __cpuid(regs, 1);
        mdSysInfo.Cpu.X86CpuInfo.VersionInformation = regs[0];
        mdSysInfo.Cpu.X86CpuInfo.FeatureInformation = regs[3];
        mdSysInfo.Cpu.X86CpuInfo.AMDExtendedCpuFeatures = 0;
        if (!memcmp( mdSysInfo.Cpu.X86CpuInfo.VendorId, "AuthenticAMD", 12 ))
        {
            __cpuid(regs, 0x80000001);  /* get vendor features */
            mdSysInfo.Cpu.X86CpuInfo.AMDExtendedCpuFeatures = regs[3];
        }
    }
#else
    mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[0] = 0;
    mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[1] = 0;
    for (unsigned int i = 0; i < sizeof(mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[0]) * 8; i++)
        if (IsProcessorFeaturePresent(i))
            mdSysInfo.Cpu.OtherCpuInfo.ProcessorFeatures[0] |= (ULONG64)1 << i;
#endif
    append(dc, &mdSysInfo, sizeof(mdSysInfo));

    /* write Wine specific system information just behind the structure, and before any string */
    if (wine_extra)
    {
        static const char code[] = {'W','I','N','E'};

        WriteFile(dc->hFile, code, 4, &written, NULL);
        /* number of sub-info, so that we can extend structure if needed */
        slen = 3;
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        /* we store offsets from just after the WINE marker */
        slen = 4 * sizeof(DWORD);
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        slen += strlen(build_id) + 1;
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        slen += strlen(sys_name) + 1;
        WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
        WriteFile(dc->hFile, build_id, strlen(build_id) + 1, &written, NULL);
        WriteFile(dc->hFile, sys_name, strlen(sys_name) + 1, &written, NULL);
        WriteFile(dc->hFile, release_name, strlen(release_name) + 1, &written, NULL);
        dc->rva += wine_extra;
    }

    /* write the service pack version string after this stream.  It is referenced within the
       stream by its RVA in the file. */
    slen = lstrlenW(osInfo.szCSDVersion) * sizeof(WCHAR);
    WriteFile(dc->hFile, &slen, sizeof(slen), &written, NULL);
    WriteFile(dc->hFile, osInfo.szCSDVersion, slen, &written, NULL);
    dc->rva += sizeof(ULONG) + slen;

    return sizeof(mdSysInfo);
}

/******************************************************************
 *		dump_threads
 *
 * Dumps into File the information about running threads
 */
static  unsigned        dump_threads(struct dump_context* dc)
{
    MINIDUMP_THREAD             mdThd;
    MINIDUMP_THREAD_LIST        mdThdList;
    unsigned                    i, sz;
    RVA                         rva_base;
    DWORD                       flags_out;
    CONTEXT                     ctx;

    mdThdList.NumberOfThreads = 0;

    rva_base = dc->rva;
    dc->rva += sz = sizeof(mdThdList.NumberOfThreads) + dc->num_threads * sizeof(mdThd);

    for (i = 0; i < dc->num_threads; i++)
    {
        fetch_thread_info(dc, i, &mdThd, &ctx);

        flags_out = ThreadWriteThread | ThreadWriteStack | ThreadWriteContext |
            ThreadWriteInstructionWindow;
        if (dc->type & MiniDumpWithProcessThreadData)
            flags_out |= ThreadWriteThreadData;
        if (dc->type & MiniDumpWithThreadInfo)
            flags_out |= ThreadWriteThreadInfo;

        if (dc->cb)
        {
            MINIDUMP_CALLBACK_INPUT     cbin;
            MINIDUMP_CALLBACK_OUTPUT    cbout;

            cbin.ProcessId = dc->pid;
            cbin.ProcessHandle = dc->process->handle;
            cbin.CallbackType = ThreadCallback;
            cbin.Thread.ThreadId = dc->threads[i].tid;
            cbin.Thread.ThreadHandle = 0; /* FIXME */
            cbin.Thread.Context = ctx;
            cbin.Thread.SizeOfContext = sizeof(CONTEXT);
            cbin.Thread.StackBase = mdThd.Stack.StartOfMemoryRange;
            cbin.Thread.StackEnd = mdThd.Stack.StartOfMemoryRange +
                mdThd.Stack.Memory.DataSize;

            cbout.ThreadWriteFlags = flags_out;
            if (!dc->cb->CallbackRoutine(dc->cb->CallbackParam, &cbin, &cbout))
                continue;
            flags_out &= cbout.ThreadWriteFlags;
        }
        if (flags_out & ThreadWriteThread)
        {
            if (ctx.ContextFlags && (flags_out & ThreadWriteContext))
            {
                mdThd.ThreadContext.Rva = dc->rva;
                mdThd.ThreadContext.DataSize = sizeof(CONTEXT);
                append(dc, &ctx, sizeof(CONTEXT));
            }
            if (mdThd.Stack.Memory.DataSize && (flags_out & ThreadWriteStack))
            {
                minidump_add_memory_block(dc, mdThd.Stack.StartOfMemoryRange,
                                          mdThd.Stack.Memory.DataSize,
                                          rva_base + sizeof(mdThdList.NumberOfThreads) +
                                          mdThdList.NumberOfThreads * sizeof(mdThd) +
                                          FIELD_OFFSET(MINIDUMP_THREAD, Stack.Memory.Rva));
            }
            writeat(dc, 
                    rva_base + sizeof(mdThdList.NumberOfThreads) +
                        mdThdList.NumberOfThreads * sizeof(mdThd),
                    &mdThd, sizeof(mdThd));
            mdThdList.NumberOfThreads++;
        }
        /* fetch CPU dependent thread info (like 256 bytes around program counter */
        dbghelp_current_cpu->fetch_minidump_thread(dc, i, flags_out, &ctx);
    }
    writeat(dc, rva_base,
            &mdThdList.NumberOfThreads, sizeof(mdThdList.NumberOfThreads));

    return sz;
}

/******************************************************************
 *		dump_threads_names
 *
 * Dumps into File the information about threads's name
 */
static  unsigned        dump_threads_names(struct dump_context* dc)
{
    MINIDUMP_THREAD_NAME        md_thread_name;
    MINIDUMP_THREAD_NAME_LIST   md_thread_name_list;
    unsigned                    i, sz;
    RVA                         rva_base;

    /* FIXME this could be optimized
     * (we use dc->num_threads disk space, could be optimized to the number of threads with name)
     */

    rva_base = dc->rva;
    dc->rva += sz = offsetof(MINIDUMP_THREAD_NAME_LIST, ThreadNames[dc->num_threads]);

    md_thread_name_list.NumberOfThreadNames = 0;

    for (i = 0; i < dc->num_threads; i++)
    {
        HANDLE  thread;
        WCHAR  *thread_name;

        if ((thread = OpenThread(THREAD_ALL_ACCESS, FALSE, dc->threads[i].tid)) != NULL)
        {
            if (GetThreadDescription(thread, &thread_name))
            {
                MINIDUMP_STRING md_string;

                md_thread_name.ThreadId = dc->threads[i].tid;
                md_thread_name.RvaOfThreadName = dc->rva;

                md_string.Length = wcslen(thread_name) * sizeof(WCHAR);
                append(dc, &md_string.Length, sizeof(md_string.Length));
                append(dc, thread_name, md_string.Length);

                writeat(dc,
                        rva_base + offsetof(MINIDUMP_THREAD_NAME_LIST, ThreadNames[md_thread_name_list.NumberOfThreadNames]),
                        &md_thread_name, sizeof(md_thread_name));
                md_thread_name_list.NumberOfThreadNames++;
                LocalFree(thread_name);
            }
            CloseHandle(thread);
        }
    }
    if (!md_thread_name_list.NumberOfThreadNames) return 0;

    writeat(dc, rva_base, &md_thread_name_list.NumberOfThreadNames, sizeof(md_thread_name_list.NumberOfThreadNames));
    return sz;
}

/******************************************************************
 *		dump_memory_info
 *
 * dumps information about the memory of the process (stack of the threads)
 */
static unsigned         dump_memory_info(struct dump_context* dc)
{
    MINIDUMP_MEMORY_LIST        mdMemList;
    MINIDUMP_MEMORY_DESCRIPTOR  mdMem;
    DWORD                       written;
    unsigned                    i, pos, len, sz;
    RVA                         rva_base;
    char                        tmp[1024];

    mdMemList.NumberOfMemoryRanges = dc->num_mem;
    append(dc, &mdMemList.NumberOfMemoryRanges,
           sizeof(mdMemList.NumberOfMemoryRanges));
    rva_base = dc->rva;
    sz = mdMemList.NumberOfMemoryRanges * sizeof(mdMem);
    dc->rva += sz;
    sz += sizeof(mdMemList.NumberOfMemoryRanges);

    for (i = 0; i < dc->num_mem; i++)
    {
        mdMem.StartOfMemoryRange = dc->mem[i].base;
        mdMem.Memory.Rva = dc->rva;
        mdMem.Memory.DataSize = dc->mem[i].size;
        SetFilePointer(dc->hFile, dc->rva, NULL, FILE_BEGIN);
        for (pos = 0; pos < dc->mem[i].size; pos += sizeof(tmp))
        {
            len = min(dc->mem[i].size - pos, sizeof(tmp));
            if (read_process_memory(dc->process, dc->mem[i].base + pos, tmp, len))
                WriteFile(dc->hFile, tmp, len, &written, NULL);
        }
        dc->rva += mdMem.Memory.DataSize;
        writeat(dc, rva_base + i * sizeof(mdMem), &mdMem, sizeof(mdMem));
        if (dc->mem[i].rva)
        {
            writeat(dc, dc->mem[i].rva, &mdMem.Memory.Rva, sizeof(mdMem.Memory.Rva));
        }
    }

    return sz;
}

/******************************************************************
 *		dump_memory64_info
 *
 * dumps information about the memory of the process (virtual memory)
 */
static unsigned         dump_memory64_info(struct dump_context* dc)
{
    MINIDUMP_MEMORY64_LIST          mdMem64List;
    MINIDUMP_MEMORY_DESCRIPTOR64    mdMem64;
    DWORD                           written;
    unsigned                        i, len, sz;
    RVA                             rva_base;
    char                            tmp[1024];
    ULONG64                         pos;
    LARGE_INTEGER                   filepos;

    sz = sizeof(mdMem64List.NumberOfMemoryRanges) +
            sizeof(mdMem64List.BaseRva) +
            dc->num_mem64 * sizeof(mdMem64);

    mdMem64List.NumberOfMemoryRanges = dc->num_mem64;
    mdMem64List.BaseRva = dc->rva + sz;

    append(dc, &mdMem64List.NumberOfMemoryRanges,
           sizeof(mdMem64List.NumberOfMemoryRanges));
    append(dc, &mdMem64List.BaseRva,
           sizeof(mdMem64List.BaseRva));

    rva_base = dc->rva;
    dc->rva += dc->num_mem64 * sizeof(mdMem64);

    /* dc->rva is not updated past this point. The end of the dump
     * is just the full memory data. */
    filepos.QuadPart = dc->rva;
    for (i = 0; i < dc->num_mem64; i++)
    {
        mdMem64.StartOfMemoryRange = dc->mem64[i].base;
        mdMem64.DataSize = dc->mem64[i].size;
        SetFilePointerEx(dc->hFile, filepos, NULL, FILE_BEGIN);
        for (pos = 0; pos < dc->mem64[i].size; pos += sizeof(tmp))
        {
            len = min(dc->mem64[i].size - pos, sizeof(tmp));
            if (read_process_memory(dc->process, dc->mem64[i].base + pos, tmp, len))
                WriteFile(dc->hFile, tmp, len, &written, NULL);
        }
        filepos.QuadPart += mdMem64.DataSize;
        writeat(dc, rva_base + i * sizeof(mdMem64), &mdMem64, sizeof(mdMem64));
    }

    return sz;
}

static unsigned         dump_misc_info(struct dump_context* dc)
{
    MINIDUMP_MISC_INFO  mmi;

    mmi.SizeOfInfo = sizeof(mmi);
    mmi.Flags1 = MINIDUMP_MISC1_PROCESS_ID;
    mmi.ProcessId = dc->pid;
    /* FIXME: create/user/kernel time */
    mmi.ProcessCreateTime = 0;
    mmi.ProcessKernelTime = 0;
    mmi.ProcessUserTime = 0;

    append(dc, &mmi, sizeof(mmi));
    return sizeof(mmi);
}

static DWORD CALLBACK write_minidump(void *_args)
{
    struct dump_context *dc = _args;
    static const MINIDUMP_DIRECTORY emptyDir = {UnusedStream, {0, 0}};
    MINIDUMP_HEADER     mdHead;
    MINIDUMP_DIRECTORY  mdDir;
    DWORD               i, nStreams, idx_stream;

    if (!fetch_process_info(dc)) return FALSE;
    fetch_modules_info(dc);

    /* 1) init */
    nStreams = 7 + (dc->except_param ? 1 : 0) +
        (dc->user_stream ? dc->user_stream->UserStreamCount : 0);

    /* pad the directory size to a multiple of 4 for alignment purposes */
    nStreams = (nStreams + 3) & ~3;

    /* 2) write header */
    mdHead.Signature = MINIDUMP_SIGNATURE;
    mdHead.Version = MINIDUMP_VERSION;  /* NOTE: native puts in an 'implementation specific' value in the high order word of this member */
    mdHead.NumberOfStreams = nStreams;
    mdHead.CheckSum = 0;                /* native sets a 0 checksum in its files */
    mdHead.StreamDirectoryRva = sizeof(mdHead);
    mdHead.TimeDateStamp = time(NULL);
    mdHead.Flags = dc->type;
    append(dc, &mdHead, sizeof(mdHead));

    /* 3) write stream directories */
    dc->rva += nStreams * sizeof(mdDir);
    idx_stream = 0;

    /* 3.1) write data stream directories */

    /* must be first in minidump */
    mdDir.StreamType = SystemInfoStream;
    mdDir.Location.Rva = dc->rva;
    mdDir.Location.DataSize = dump_system_info(dc);
    writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = ThreadListStream;
    mdDir.Location.Rva = dc->rva;
    mdDir.Location.DataSize = dump_threads(dc);
    writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = ThreadNamesStream;
    mdDir.Location.Rva = dc->rva;
    if ((mdDir.Location.DataSize = dump_threads_names(dc)))
        writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                &mdDir, sizeof(mdDir));

    mdDir.StreamType = ModuleListStream;
    mdDir.Location.Rva = dc->rva;
    mdDir.Location.DataSize = dump_modules(dc, FALSE);
    writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    mdDir.StreamType = 0xfff0; /* FIXME: this is part of MS reserved streams */
    mdDir.Location.Rva = dc->rva;
    mdDir.Location.DataSize = dump_modules(dc, TRUE);
    writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));


    if (!(dc->type & MiniDumpWithFullMemory))
    {
        mdDir.StreamType = MemoryListStream;
        mdDir.Location.Rva = dc->rva;
        mdDir.Location.DataSize = dump_memory_info(dc);
        writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                &mdDir, sizeof(mdDir));
    }

    mdDir.StreamType = MiscInfoStream;
    mdDir.Location.Rva = dc->rva;
    mdDir.Location.DataSize = dump_misc_info(dc);
    writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
            &mdDir, sizeof(mdDir));

    /* 3.2) write exception information (if any) */
    if (dc->except_param)
    {
        mdDir.StreamType = ExceptionStream;
        mdDir.Location.Rva = dc->rva;
        mdDir.Location.DataSize = dump_exception_info(dc);
        writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                &mdDir, sizeof(mdDir));
    }

    /* 3.3) write user defined streams (if any) */
    if (dc->user_stream)
    {
        for (i = 0; i < dc->user_stream->UserStreamCount; i++)
        {
            mdDir.StreamType = dc->user_stream->UserStreamArray[i].Type;
            mdDir.Location.DataSize = dc->user_stream->UserStreamArray[i].BufferSize;
            mdDir.Location.Rva = dc->rva;
            writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                    &mdDir, sizeof(mdDir));
            append(dc, dc->user_stream->UserStreamArray[i].Buffer,
                   dc->user_stream->UserStreamArray[i].BufferSize);
        }
    }

    /* 3.4) write full memory (if requested) */
    if (dc->type & MiniDumpWithFullMemory)
    {
        fetch_memory64_info(dc);

        mdDir.StreamType = Memory64ListStream;
        mdDir.Location.Rva = dc->rva;
        mdDir.Location.DataSize = dump_memory64_info(dc);
        writeat(dc, mdHead.StreamDirectoryRva + idx_stream++ * sizeof(mdDir),
                &mdDir, sizeof(mdDir));
    }

    /* fill the remaining directory entries with 0's (unused stream types) */
    /* NOTE: this should always come last in the dump! */
    for (i = idx_stream; i < nStreams; i++)
        writeat(dc, mdHead.StreamDirectoryRva + i * sizeof(emptyDir), &emptyDir, sizeof(emptyDir));

    return TRUE;
}

/******************************************************************
 *		MiniDumpWriteDump (DEBUGHLP.@)
 *
 */
BOOL WINAPI MiniDumpWriteDump(HANDLE hProcess, DWORD pid, HANDLE hFile,
                              MINIDUMP_TYPE DumpType,
                              PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                              PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                              PMINIDUMP_CALLBACK_INFORMATION CallbackParam)
{
    struct dump_context dc;
    BOOL                sym_initialized = FALSE;
    BOOL                ret = FALSE;

    TRACE("(%p, %lu, %p, %u, %p, %p, %p)\n",
          hProcess, pid, hFile, DumpType, ExceptionParam, UserStreamParam, CallbackParam);

    if (!(dc.process = process_find_by_handle(hProcess)))
    {
        if (!(sym_initialized = SymInitializeW(hProcess, NULL, TRUE)))
        {
            WARN("failed to initialize process\n");
            return FALSE;
        }
        dc.process = process_find_by_handle(hProcess);
    }

    if (DumpType & MiniDumpWithDataSegs)
        FIXME("NIY MiniDumpWithDataSegs\n");
    if (DumpType & MiniDumpWithHandleData)
        FIXME("NIY MiniDumpWithHandleData\n");
    if (DumpType & MiniDumpFilterMemory)
        FIXME("NIY MiniDumpFilterMemory\n");
    if (DumpType & MiniDumpScanMemory)
        FIXME("NIY MiniDumpScanMemory\n");

    dc.hFile = hFile;
    dc.pid = pid;
    dc.modules = NULL;
    dc.num_modules = 0;
    dc.alloc_modules = 0;
    dc.threads = NULL;
    dc.num_threads = 0;
    dc.cb = CallbackParam;
    dc.type = DumpType;
    dc.mem = NULL;
    dc.num_mem = 0;
    dc.alloc_mem = 0;
    dc.mem64 = NULL;
    dc.num_mem64 = 0;
    dc.alloc_mem64 = 0;
    dc.rva = 0;
    dc.except_param = ExceptionParam;
    dc.user_stream = UserStreamParam;

    /* have a dedicated thread for fetching info on self */
    if (dc.pid != GetCurrentProcessId())
        ret = write_minidump(&dc);
    else
    {
        DWORD  exit_code;
        HANDLE h = CreateThread(NULL, 0, write_minidump, &dc, 0, NULL);
        if (h)
        {
            if (WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0 && GetExitCodeThread(h, &exit_code))
                ret = exit_code;
            else
                TerminateThread(h, 0);
            CloseHandle(h);
        }
    }

    if (sym_initialized)
        SymCleanup(hProcess);

    HeapFree(GetProcessHeap(), 0, dc.mem);
    HeapFree(GetProcessHeap(), 0, dc.mem64);
    HeapFree(GetProcessHeap(), 0, dc.modules);
    HeapFree(GetProcessHeap(), 0, dc.threads);

    return ret;
}

/******************************************************************
 *		MiniDumpReadDumpStream (DEBUGHLP.@)
 *
 *
 */
BOOL WINAPI MiniDumpReadDumpStream(PVOID base, ULONG str_idx,
                                   PMINIDUMP_DIRECTORY* pdir,
                                   PVOID* stream, ULONG* size)
{
    MINIDUMP_HEADER*    mdHead = base;

    if (mdHead->Signature == MINIDUMP_SIGNATURE)
    {
        MINIDUMP_DIRECTORY* dir;
        DWORD               i;

        dir = (MINIDUMP_DIRECTORY*)((char*)base + mdHead->StreamDirectoryRva);
        for (i = 0; i < mdHead->NumberOfStreams; i++, dir++)
        {
            if (dir->StreamType == str_idx)
            {
                if (pdir) *pdir = dir;
                if (stream) *stream = (char*)base + dir->Location.Rva;
                if (size) *size = dir->Location.DataSize;
                return TRUE;
            }
        }
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}
