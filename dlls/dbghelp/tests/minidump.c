/*
 * Copyright 2024 Eric Pouech for CodeWeavers
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

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "dbghelp.h"
#include "wine/test.h"
#include "winternl.h"
#include "winnt.h"
#include "wine/test.h"

static HRESULT (WINAPI *pSetThreadDescription)(HANDLE,PCWSTR);
static const WCHAR *main_thread_name = L"I'm the running thread!";

static unsigned popcount32(ULONG val)
{
    val -= val >> 1 & 0x55555555;
    val = (val & 0x33333333) + (val >> 2 & 0x33333333);
    return ((val + (val >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
}

static inline unsigned popcount64(ULONG64 val)
{
    return popcount32(val >> 32) + popcount32(val);
}

#define minidump_open_for_read(a) _minidump_open_for_read(__LINE__, (a))
static MINIDUMP_HEADER *_minidump_open_for_read(unsigned line, const char *filename)
{
    MINIDUMP_HEADER *hdr;
    HANDLE file, map;
    BOOL ret;

    file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok_(__FILE__, line)(file != INVALID_HANDLE_VALUE, "Couldn't reopen file\n");
    map = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    ok_(__FILE__, line)(map != 0, "Couldn't map file\n");
    hdr = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    ok(hdr != NULL, "Couldn't map file\n");

    ok(hdr && hdr->Signature == MINIDUMP_SIGNATURE, "Unexpected signature\n");

    ret = CloseHandle(map);
    ok_(__FILE__, line)(ret, "Couldn't unmap file\n");
    ret = CloseHandle(file);
    ok_(__FILE__, line)(ret, "Couldn't close file\n");

    return hdr;
}

#define minidump_close_for_read(a) _minidump_close_for_read(__LINE__, (a))
static void _minidump_close_for_read(unsigned line, MINIDUMP_HEADER *data)
{
    BOOL ret;

    ret = UnmapViewOfFile(data);
    ok_(__FILE__, line)(ret, "Couldn't unmap file\n");
}

static BOOL minidump_write(HANDLE proc, const WCHAR *filename, MINIDUMP_TYPE type, BOOL windows_can_fail,
                           MINIDUMP_EXCEPTION_INFORMATION *mei, MINIDUMP_CALLBACK_INFORMATION *cbi)
{
    HANDLE              file;
    BOOL                ret, ret2;

    file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    ok(file != INVALID_HANDLE_VALUE, "Failed to create minidump %ls\n", filename);

    ret = MiniDumpWriteDump(proc, GetProcessId(proc), file, type, mei, NULL, cbi);
    /* using new features that are not supported on old dbghelp versions */
    ok(ret || broken(windows_can_fail), "Couldn't write minidump content\n");

    ret2 = CloseHandle(file);
    ok(ret2, "Couldn't close file\n");

    return ret;
}

typedef DWORD64 stream_mask_t;
#define STREAM2MASK(st) (((stream_mask_t)1) << (st))

#define BASIC_STREAM_MASK \
    (STREAM2MASK(ThreadListStream) | STREAM2MASK(ModuleListStream) | STREAM2MASK(MemoryListStream) | \
     STREAM2MASK(SystemInfoStream) | STREAM2MASK(MiscInfoStream) | STREAM2MASK(SystemMemoryInfoStream) | \
     STREAM2MASK(ProcessVmCountersStream))
/* streams added in Win8 & Win10... */
#define BASIC_STREAM_BROKEN_MASK (STREAM2MASK(SystemMemoryInfoStream) | STREAM2MASK(ProcessVmCountersStream))
#define BASIC_STREAM_TODO_MASK (STREAM2MASK(SystemMemoryInfoStream) | STREAM2MASK(ProcessVmCountersStream))
static void test_minidump_contents(void)
{
    static const struct minidump_streams
    {
        MINIDUMP_TYPE type;
        stream_mask_t streams_mask;
        stream_mask_t todo_wine_mask;
    }
    streams_table[] =
    {
/* 0 */ {MiniDumpNormal,                BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
        {MiniDumpWithDataSegs,          BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
        {MiniDumpWithProcessThreadData, BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
        {MiniDumpWithThreadInfo,        BASIC_STREAM_MASK | STREAM2MASK(ThreadInfoListStream), BASIC_STREAM_TODO_MASK | STREAM2MASK(ThreadInfoListStream)},
        {MiniDumpWithCodeSegs,          BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
/* 5 */ {MiniDumpWithTokenInformation,  BASIC_STREAM_MASK | STREAM2MASK(TokenStream), BASIC_STREAM_TODO_MASK | STREAM2MASK(TokenStream)},
        {MiniDumpWithModuleHeaders,     BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK}, /* requires win8 at least */
        {MiniDumpWithAvxXStateContext,  BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK}, /* requires win10 at least */
        {MiniDumpWithIptTrace,          BASIC_STREAM_MASK, BASIC_STREAM_TODO_MASK},
    };
    stream_mask_t expected_mask;
    MINIDUMP_HEADER *hdr;
    void *where;
    ULONG size;
    BOOL ret;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(streams_table); i++)
    {
        winetest_push_context("streams_table[%d]", i);

        /* too old for dbghelp in Win7 & 8 */
        if (minidump_write(GetCurrentProcess(), L"foo.mdmp", streams_table[i].type, i >= 6, NULL, NULL))
        {
            hdr = minidump_open_for_read("foo.mdmp");
            /* native keeps (likely padding) some unused streams at the end of directory, but lists them here */
            ok(hdr->NumberOfStreams >= popcount64(streams_table[i].streams_mask), "Unexpected number of streams %u <> %u\n",
               hdr->NumberOfStreams, popcount64(streams_table[i].streams_mask));
            ok(hdr->Flags == streams_table[i].type, "Unexpected flags\n");
            /* start at 3, 0=unused (cf above), 1,2=reserved shall be skipped */
            for (j = 3; j < 25 /* last documented stream */; j++)
            {
                ret = MiniDumpReadDumpStream(hdr, j, NULL, &where, &size);
                expected_mask = streams_table[i].streams_mask;
                if (pSetThreadDescription) expected_mask |= STREAM2MASK(ThreadNamesStream);
                todo_wine_if(streams_table[i].todo_wine_mask & STREAM2MASK(j))
                if (expected_mask & STREAM2MASK(j))
                    ok((ret && where) || broken(BASIC_STREAM_BROKEN_MASK & STREAM2MASK(j)), "Expecting stream %d to be present\n", j);
                else
                    ok(!ret, "Not expecting stream %d to be present\n", j);
            }

            minidump_close_for_read(hdr);

            ret = DeleteFileA("foo.mdmp");
            ok(ret, "Couldn't delete file\n");
        }
        else win_skip("Skipping not supported feature (too old dbghelp version)\n");
        winetest_pop_context();
    }
}

static void minidump_check_nostream(void *data, MINIDUMP_STREAM_TYPE stream_type)
{
    void *stream;
    BOOL ret;

    ret = MiniDumpReadDumpStream(data, stream_type, NULL, (void**)&stream, NULL);
    ok(!ret, "Unexpected stream %u\n", stream_type);
}

static void minidump_check_pid(void *data, DWORD pid)
{
    MINIDUMP_MISC_INFO *misc_info;
    BOOL ret;

    ret = MiniDumpReadDumpStream(data, MiscInfoStream, NULL, (void**)&misc_info, NULL);
    ok(ret && misc_info, "Couldn't find misc-info stream\n");
    ok(misc_info->Flags1 & misc_info->Flags1 & MINIDUMP_MISC1_PROCESS_ID, "No process-id in misc_info\n");
    ok(pid == misc_info->ProcessId, "Unexpected process id\n");
}

static unsigned minidump_get_number_of_threads(void *data)
{
    MINIDUMP_THREAD_LIST *thread_list;
    BOOL ret;

    ret = MiniDumpReadDumpStream(data, ThreadListStream, NULL, (void**)&thread_list, NULL);
    ok(ret && thread_list, "Couldn't find thread-list stream\n");
    return thread_list->NumberOfThreads;
}

static void minidump_check_threads(void *data)
{
    MINIDUMP_THREAD_LIST *thread_list;
    ULONG stream_size;
    int i;
    BOOL ret;

    ret = MiniDumpReadDumpStream(data, ThreadListStream, NULL, (void**)&thread_list, &stream_size);
    ok(ret && thread_list, "Couldn't find thread-list stream\n");
    ok(stream_size == sizeof(thread_list->NumberOfThreads) + thread_list->NumberOfThreads * sizeof(thread_list->Threads[0]),
       "Unexpected size\n");
    for (i = 0; i < thread_list->NumberOfThreads; i++)
    {
        const MINIDUMP_THREAD *thread = &thread_list->Threads[i];
        const CONTEXT *ctx;

        ok(thread->SuspendCount == 0, "Unexpected value\n");
        ok(thread->Stack.StartOfMemoryRange, "Unexpected value %I64x\n", thread->Stack.StartOfMemoryRange);
        ok(thread->Stack.Memory.DataSize, "Unexpected value %x\n", thread->Stack.Memory.DataSize);
        ok(thread->Teb, "Unexpected value\n");
        ok(thread->ThreadContext.DataSize >= sizeof(CONTEXT), "Unexpected value\n");
        ctx = RVA_TO_ADDR(data, thread->ThreadContext.Rva);
        ok((ctx->ContextFlags & CONTEXT_ALL) == CONTEXT_ALL, "Unexpected value\n");
    }
}

static void minidump_check_threads_name(void *data, DWORD tid, const WCHAR* name)
{
    MINIDUMP_THREAD_NAME_LIST *thread_name_list;
    ULONG stream_size;
    int i;
    BOOL ret;

    if (!pSetThreadDescription) return;

    ret = MiniDumpReadDumpStream(data, ThreadNamesStream, NULL, (void**)&thread_name_list, &stream_size);
    ok(ret && thread_name_list, "Couldn't find thread-name-list stream\n");
    ok(stream_size == sizeof(thread_name_list->NumberOfThreadNames) + thread_name_list->NumberOfThreadNames * sizeof(thread_name_list->ThreadNames[0]),
       "Unexpected size\n");
    for (i = 0; i < thread_name_list->NumberOfThreadNames; i++)
    {
        const MINIDUMP_THREAD_NAME *thread_name = &thread_name_list->ThreadNames[i];
        const MINIDUMP_STRING *md_string;

        md_string = RVA_TO_ADDR(data, (ULONG_PTR)thread_name->RvaOfThreadName);
        if (thread_name->RvaOfThreadName == (ULONG_PTR)thread_name->RvaOfThreadName &&
            thread_name->ThreadId == tid &&
            wcslen(name) == md_string->Length / sizeof(WCHAR) &&
            !memcmp(name, md_string->Buffer, md_string->Length))
            break;
    }
    ok(i < thread_name_list->NumberOfThreadNames, "Couldn't find thread %lx %ls\n", tid, name);
}

static void minidump_check_module(void *data, const WCHAR *name, DWORD64 base)
{
    MINIDUMP_MODULE_LIST *module_list;
    size_t namelen = wcslen(name);
    int i;
    BOOL ret;

    ret = MiniDumpReadDumpStream(data, ModuleListStream, NULL, (void**)&module_list, NULL);
    ok(ret && module_list, "Couldn't find module-list stream\n");
    ok(module_list->NumberOfModules > 3, "Unexpected number of modules\n");
    for (i = 0; i < module_list->NumberOfModules; i++)
    {
        MINIDUMP_MODULE *module = &module_list->Modules[i];
        WCHAR *ptr;

        MINIDUMP_STRING *string = RVA_TO_ADDR(data, module->ModuleNameRva);
        for (ptr = string->Buffer + string->Length / sizeof(WCHAR) - 1; ptr >= string->Buffer && *ptr != L'\\'; ptr--) {}
        ptr++;
        if (ptr + namelen == string->Buffer + string->Length / sizeof(WCHAR) &&
            !wcsnicmp(name, ptr, namelen) &&
            module->BaseOfImage == base) break;
    }
    ok(i < module_list->NumberOfModules, "Couldn't find module %ls in minidump\n", name);
}

struct memory_description
{
    /* MD_SECTION, MD_DIRECTORY can be present at the same time */
    /* MD_UNMAPPED: some DLLs are present when creating the minidump, but are unloaded afterwards (native) */
    enum {MD_NONE = 0, MD_UNMAPPED = 1, MD_PE_HEADER = 2, MD_STACK = 3, MD_SECTION = 4, MD_DIRECTORY = 8, MD_UNWIND = 16} kind;
    unsigned id; /* MD_STACK: thread index
                  * MD_DIRECTORY: directory index
                  * MD_UNWIND: function index in function table
                  */
    const char* name; /* MD_SECTION: section name */
};

static struct memory_description minidump_get_memory_description(void *data, DWORD64 addr)
{
    const BYTE *addr_ptr = (void*)(ULONG_PTR)addr;
    MINIDUMP_MODULE_LIST *module_list;
    MINIDUMP_THREAD_LIST *thread_list;
    struct memory_description md = {.kind = MD_NONE};
    BOOL ret;
    int i, j, dir;

    ret = MiniDumpReadDumpStream(data, ModuleListStream, NULL, (void**)&module_list, NULL);
    ok(ret && module_list, "Couldn't find module-list stream\n");
    for (i = 0; i < module_list->NumberOfModules; i++)
    {
        MINIDUMP_MODULE *module = &module_list->Modules[i];
        MINIDUMP_STRING *string = RVA_TO_ADDR(data, module->ModuleNameRva);
        if (module->BaseOfImage <= addr && addr < module->BaseOfImage + module->SizeOfImage)
        {
            HMODULE module_handle;
            WCHAR *module_name;
            size_t module_name_len = string->Length / sizeof(WCHAR);
            IMAGE_NT_HEADERS *nthdr;

            module_name = malloc((module_name_len + 1) * sizeof(WCHAR));
            if (!module_name) continue;
            memcpy(module_name, string->Buffer, module_name_len * sizeof(WCHAR));
            module_name[module_name_len] = L'\0';
            module_handle = GetModuleHandleW(module_name);
            if ((nthdr = RtlImageNtHeader(module_handle)))
            {
                for (dir = 0; dir < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; dir++)
                {
                    ULONG dir_size;
                    const BYTE *dir_start = RtlImageDirectoryEntryToData(module_handle, TRUE, dir, &dir_size);
                    if (dir_start && dir_start <= addr_ptr && addr_ptr < dir_start + dir_size)
                    {
                        md.kind |= MD_DIRECTORY;
                        md.id = dir;
                    }
                    switch (dir)
                    {
                    case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
                        if (nthdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
                        {
                            const IMAGE_AMD64_RUNTIME_FUNCTION_ENTRY *func;
                            for (func = (const void*)dir_start; func < (const IMAGE_AMD64_RUNTIME_FUNCTION_ENTRY *)(dir_start + dir_size); func++)
                            {
                                if (RtlImageRvaToVa(nthdr, module_handle, func->UnwindData, NULL) == addr_ptr)
                                {
                                    md.kind = MD_UNWIND;
                                    md.id = func - (const IMAGE_AMD64_RUNTIME_FUNCTION_ENTRY *)dir_start;
                                }
                            }
                        }
                        break;
                        /* FIXME handle more areas: import/export tables... */
                    }
                }
                if (addr < (DWORD_PTR)(IMAGE_FIRST_SECTION(nthdr) + nthdr->FileHeader.NumberOfSections))
                {
                    md.kind = MD_PE_HEADER;
                }
                for (j = 0; j < nthdr->FileHeader.NumberOfSections; j++)
                {
                    IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nthdr) + j;
                    const BYTE *section_start = (BYTE*)module_handle + section->VirtualAddress;
                    if (section_start <= addr_ptr && addr_ptr < section_start + section->Misc.VirtualSize)
                    {
                        md.kind |= MD_SECTION;
                        md.name = (const char*)section->Name;
                    }
                }
            }
            else md.kind = MD_UNMAPPED;
            free(module_name);
            return md;
        }
    }
    ret = MiniDumpReadDumpStream(data, ThreadListStream, NULL, (void**)&thread_list, NULL);
    ok(ret && thread_list, "Couldn't find thread-list stream\n");
    for (i = 0; i < thread_list->NumberOfThreads; i++)
    {
        MINIDUMP_THREAD *thread = &thread_list->Threads[i];
        if (thread->Stack.StartOfMemoryRange <= addr &&
            addr < thread->Stack.StartOfMemoryRange + thread->Stack.Memory.DataSize)
        {
            md.kind = MD_STACK;
            md.id = i;
            return md;
        }
    }
    return md;
}

/* modules could be load/unloaded when generating the minidump, so count the number of available modules */
static unsigned minidump_get_number_of_available_modules(void *data)
{
    MINIDUMP_MODULE_LIST *module_list;
    unsigned num_modules = 0;
    BOOL ret;
    int i;
    ret = MiniDumpReadDumpStream(data, ModuleListStream, NULL, (void**)&module_list, NULL);
    ok(ret && module_list, "Couldn't find module-list stream\n");
    for (i = 0; i < module_list->NumberOfModules; i++)
    {
        struct memory_description md;
        md = minidump_get_memory_description(data, module_list->Modules[i].BaseOfImage);
        if (md.kind != MD_UNMAPPED)
            num_modules++;
    }
    return num_modules;
}

struct memory_walker
{
    unsigned num_unknown; /* number of unknown memory locations */
    unsigned num_thread_stack; /* number of locations inside a thread stack */
    unsigned num_directories[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; /* number of locations in side the directories' content */
    unsigned num_text; /* number of locations inside .text section */
    unsigned num_unwind_info;
};

static void minidump_walk_memory(void *data, struct memory_walker *walker)
{
    MINIDUMP_MEMORY_LIST *memory_list;
    BOOL ret;
    int i;

    ret = MiniDumpReadDumpStream(data, MemoryListStream, NULL, (void**)&memory_list, NULL);
    ok(ret && memory_list, "Couldn't find memory-list stream\n");
    for (i = 0; i < memory_list->NumberOfMemoryRanges; i++)
    {
        MINIDUMP_MEMORY_DESCRIPTOR *desc = &memory_list->MemoryRanges[i];
        struct memory_description md;
        md = minidump_get_memory_description(data, desc->StartOfMemoryRange);
        switch ((int)md.kind)
        {
        case MD_NONE:
            walker->num_unknown++;
            break;
        case MD_UNMAPPED:
            /* nothing we can do here */
            break;
        case MD_STACK:
            walker->num_thread_stack++;
            break;
        case MD_SECTION | MD_UNWIND:
            walker->num_unwind_info++;
            break;
        case MD_PE_HEADER:
            /* FIXME may change with MiniDumpWithModuleHeaders */
            ok(0, "Unexpected memory block in PE header\n");
            break;
        case MD_SECTION:
        case MD_SECTION | MD_DIRECTORY:
            if (!strcmp(md.name, ".text"))
                walker->num_text++;
            if (md.kind & MD_DIRECTORY)
            {
                ok(md.id < ARRAY_SIZE(walker->num_directories), "Out of bounds index\n");
                walker->num_directories[md.id]++;
            }
            break;
        default:
            ok(0, "Unexpected memory description kind: %x\n", md.kind);
            break;
        }
    }
}

static void test_current_process(void)
{
    static const struct
    {
        MINIDUMP_TYPE dump_type;
    }
    process_tests[] =
    {
        { MiniDumpNormal /* = 0 */ },
        { MiniDumpWithCodeSegs },
        { MiniDumpWithDataSegs },
        { MiniDumpWithThreadInfo },
/* requires more work
   { MiniDumpWithModuleHeaders },
   { MiniDumpWithProcessThreadData },
*/
      };
    struct memory_walker walker;
    struct memory_description md, md2;
    unsigned num_available_modules, num_threads;
    void *data;
    BOOL ret;
    int i;

    for (i = 0; i < ARRAY_SIZE(process_tests); i++)
    {
        winetest_push_context("process_tests[%d]", i);
        minidump_write(GetCurrentProcess(), L"foo.mdmp", process_tests[i].dump_type, FALSE, NULL, NULL);

        data = minidump_open_for_read("foo.mdmp");

        num_threads = minidump_get_number_of_threads(data);
        ok(num_threads > 0, "Unexpected number of threads\n");

        minidump_check_threads(data);
        md = minidump_get_memory_description(data, (DWORD_PTR)&i);
        ok(md.kind == MD_STACK, "Couldn't find automatic variable\n");

        md2 = minidump_get_memory_description(data, (DWORD_PTR)NtCurrentTeb()->Tib.StackBase - sizeof(void*));
        ok(md2.kind == MD_STACK, "Couldn't find stack bottom\n");
        ok(md.id == md2.id, "Should be on same stack\n");

        minidump_check_pid(data, GetCurrentProcessId());
        num_available_modules = minidump_get_number_of_available_modules(data);
#define CHECK_MODULE(s) minidump_check_module(data, s, (DWORD64)(DWORD_PTR)GetModuleHandleW(s))
        CHECK_MODULE(L"ntdll.dll");
        CHECK_MODULE(L"kernelbase.dll");
        CHECK_MODULE(L"kernel32.dll");
        CHECK_MODULE(L"dbghelp.dll");
#undef CHECK_MODULE

        memset(&walker, 0, sizeof(walker));
        minidump_walk_memory(data, &walker);

        ok(walker.num_unknown == 0, "unexpected unknown memory locations\n");
        ok(walker.num_thread_stack == num_threads, "Unexpected number of stacks\n");

        if (sizeof(void*) > 4 && (process_tests[i].dump_type & MiniDumpWithModuleHeaders))
            ok(walker.num_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION] > 0, "expected unwind information\n");
        else
            ok(walker.num_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION] == 0, "unexpected unwind information\n");
        if (process_tests[i].dump_type & MiniDumpWithCodeSegs)
        {
            todo_wine
            ok(walker.num_text >= num_available_modules ||
               /* win7 & 8 report one less */
               broken(walker.num_text + 1 >= num_available_modules), "expected code segments %u %u\n", walker.num_text, num_available_modules);
        }
        else
            /* native embeds some elements in code segment from ntdll */
            ok(walker.num_text < 5, "unexpected code segments %u %u\n", walker.num_text, num_available_modules);

        ok(walker.num_unwind_info == 0, "unexpected unwind info %u\n", walker.num_unwind_info);
        minidump_check_nostream(data, ExceptionStream);

        minidump_check_threads_name(data, GetCurrentThreadId(), main_thread_name);
        minidump_close_for_read(data);

        ret = DeleteFileA("foo.mdmp");
        ok(ret, "Couldn't delete file\n");
        winetest_pop_context();
    }
}

struct cb_info
{
    /* input */
    unsigned module_flags;
    unsigned thread_flags;
    /* output */
    DWORD64 mask_types;
    unsigned num_modules;
    unsigned num_threads;
    unsigned num_include_modules;
    unsigned num_include_threads;
};

static BOOL CALLBACK test_callback_cb(void *pmt, MINIDUMP_CALLBACK_INPUT *input, MINIDUMP_CALLBACK_OUTPUT *output)
{
    struct cb_info *cb = pmt;

    ok(input->CallbackType < sizeof(cb->mask_types) * 8, "Too small mask\n");
    cb->mask_types |= (DWORD64)1u << input->CallbackType;

    if (input->CallbackType == WriteKernelMinidumpCallback || input->CallbackType == IoStartCallback)
    {
        ok(input->ProcessId == 0, "Unexpected pid %lu %lu\n", input->ProcessId, input->CallbackType);
        ok(input->ProcessHandle == NULL, "Unexpected process handle %p %lu\n", input->ProcessHandle, input->CallbackType);
    }
    else if (input->CallbackType == IsProcessSnapshotCallback || input->CallbackType == VmStartCallback)
    {
        ok(input->ProcessId == 0, "Unexpected pid %lu %lu\n", input->ProcessId, input->CallbackType);
        ok(input->ProcessHandle == GetCurrentProcess(), "Unexpected process handle %p %lu\n", input->ProcessHandle, input->CallbackType);
    }
    else
    {
        ok(input->ProcessId == GetCurrentProcessId(), "Unexpected pid %lu %lu\n", input->ProcessId, input->CallbackType);
        ok(input->ProcessHandle == GetCurrentProcess(), "Unexpected process handle %p %lu\n", input->ProcessHandle, input->CallbackType);
    }

    switch (input->CallbackType)
    {
    case ModuleCallback:
        ok(output->ModuleWriteFlags == cb->module_flags, "Unexpected module flags %lx\n", output->ModuleWriteFlags);
        cb->num_modules++;
        break;
    case IncludeModuleCallback:
        ok(output->ModuleWriteFlags == cb->module_flags, "Unexpected module flags %lx\n", output->ModuleWriteFlags);
        cb->num_include_modules++;
        break;
    case ThreadCallback:
    case ThreadExCallback:
        ok(output->ThreadWriteFlags == cb->thread_flags, "Unexpected thread flags %lx\n", output->ThreadWriteFlags);
        cb->num_threads++;
        break;
    case IncludeThreadCallback:
        ok(output->ThreadWriteFlags == cb->thread_flags, "Unexpected thread flags %lx\n", output->ThreadWriteFlags);
        cb->num_include_threads++;
        break;
    case MemoryCallback:
    case RemoveMemoryCallback:
        ok(output->MemoryBase == 0, "Unexpected memory info\n");
        ok(output->MemorySize == 0, "Unexpected memory info\n");
        break;
    case CancelCallback:
        ok(!output->Cancel, "Unexpected value\n");
        ok(!output->CheckCancel, "Unexpected value\n");
        break;
    case WriteKernelMinidumpCallback:
        ok(output->Handle == NULL, "Unexpected value\n");
        break;
        /* case KernelMinidumpStatusCallback: */
        /* case IncludeVmRegionCallback: */
    case IoStartCallback:
        ok(output->Status == E_NOTIMPL, "Unexpected value %lx\n", output->Status);
        /* TODO check the output->Vm* fields */
        break;
        /* case IoWriteAllCallback:
         * case IoFinishCallback:
         */
    case ReadMemoryFailureCallback:
        /* TODO check the rest */
        break;
    case SecondaryFlagsCallback:
        ok(input->SecondaryFlags == 0x00, "Unexpected value %lx\n", input->SecondaryFlags);
        ok(input->SecondaryFlags == output->SecondaryFlags, "Unexpected value %lx\n", output->SecondaryFlags);
        break;
    case IsProcessSnapshotCallback:
        break;
    case VmStartCallback:
        break;
        /*
    case VmQueryCallback:
    case VmPreReadCallback:
    case VmPostReadCallback:
        */
    default:
        ok(0, "Unexpected callback type %lu\n", input->CallbackType);
        break;
    }
    return TRUE;
}

static void test_callback(void)
{
    static const struct
    {
        MINIDUMP_TYPE dump_type;
        unsigned module_flags;
        unsigned thread_flags;
    }
    callback_tests[] =
    {
        {
            MiniDumpNormal /* = 0 */,
            ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord,
            ThreadWriteThread | ThreadWriteStack | ThreadWriteContext | ThreadWriteInstructionWindow,
        },
        {
            MiniDumpWithCodeSegs,
            ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord | ModuleWriteCodeSegs,
            ThreadWriteThread | ThreadWriteStack | ThreadWriteContext | ThreadWriteInstructionWindow,
        },
        {
            MiniDumpWithDataSegs,
            ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord | ModuleWriteDataSeg,
            ThreadWriteThread | ThreadWriteStack | ThreadWriteContext | ThreadWriteInstructionWindow,
        },
        {
            MiniDumpWithThreadInfo,
            ModuleWriteModule | ModuleWriteMiscRecord | ModuleWriteCvRecord,
            ThreadWriteThread | ThreadWriteStack | ThreadWriteContext | ThreadWriteInstructionWindow | ThreadWriteThreadInfo,
        },
    };
#define X(a) ((stream_mask_t)1u << (a))
    static const stream_mask_t mask_types =
        X(ModuleCallback) | X(ThreadCallback) | /* ThreadExCallback */ X(IncludeThreadCallback) |
        X(IncludeModuleCallback) | X(MemoryCallback) |  X(CancelCallback) | X(WriteKernelMinidumpCallback) |
        /* KernelMinidumpStatusCallback) */ X(RemoveMemoryCallback) | /* IncludeVmRegionCallback */ X(IoStartCallback) |
        /* IoWriteAllCallback IoFinishCallback ReadMemoryFailureCallback */ X(SecondaryFlagsCallback) |
        X(IsProcessSnapshotCallback) | X(VmStartCallback) /* VmQueryCallback VmPreReadCallback */
        /* VmPostReadCallback */;
    static const stream_mask_t mask_types_too_old_dbghelp = X(IsProcessSnapshotCallback) | X(VmStartCallback);
#undef X
    struct cb_info cb_info;
    MINIDUMP_CALLBACK_INFORMATION cbi = {.CallbackRoutine = test_callback_cb, .CallbackParam = &cb_info};
    BOOL ret;
    int i;

    for (i = 0; i < ARRAY_SIZE(callback_tests); i++)
    {
        winetest_push_context("callback_tests[%d]", i);

        memset(&cb_info, 0, sizeof(cb_info));
        cb_info.module_flags = callback_tests[i].module_flags;
        cb_info.thread_flags = callback_tests[i].thread_flags;
        minidump_write(GetCurrentProcess(), L"foo.mdmp", callback_tests[i].dump_type, FALSE, NULL, &cbi);

        todo_wine
        ok(cb_info.mask_types == mask_types ||
           broken(cb_info.mask_types == (mask_types & ~mask_types_too_old_dbghelp)),
                  "Unexpected mask for callback types %I64x (%I64x)\n", cb_info.mask_types, mask_types);
        ok(cb_info.num_modules > 5, "Unexpected number of modules\n");
        /* native reports several threads... */
        ok(cb_info.num_threads >= 1, "Unexpected number of threads\n");
        todo_wine
        ok(cb_info.num_modules == cb_info.num_include_modules, "Unexpected number of include modules\n");
        todo_wine
        ok(cb_info.num_threads == cb_info.num_include_threads, "Unexpected number of include threads\n");

        ret = DeleteFileA("foo.mdmp");
        ok(ret, "Couldn't delete file\n");
        winetest_pop_context();
    }
}

static void test_exception(void)
{
    static const struct
    {
        unsigned exception_code;
        unsigned exception_flags;
        unsigned num_args;
        BOOL     with_child;
    }
    exception_tests[] =
    {
        { 0x1234,                       0, 0, FALSE },
        { 0x1234,                       0, 0, TRUE },
        { 0x1234,                       0, 5, FALSE },
        { 0x1234,                       0, 5, TRUE },
        { EXCEPTION_BREAKPOINT,         0, 1, TRUE },
        { EXCEPTION_ACCESS_VIOLATION,   0, 2, TRUE },
    };
    ULONG_PTR args[EXCEPTION_MAXIMUM_PARAMETERS];
    MINIDUMP_EXCEPTION_STREAM *except_info;
    ULONG size;
    void *data;
    BOOL ret;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(args); i++) args[i] = 0x666000 + i;
    for (i = 0; i < ARRAY_SIZE(exception_tests); i++)
    {
        PROCESS_INFORMATION pi;
        MINIDUMP_EXCEPTION_INFORMATION mei;
        EXCEPTION_POINTERS ep;
        DEBUG_EVENT ev;
        /* for local access */
        EXCEPTION_RECORD er;
        CONTEXT ctx;
        CONTEXT *mctx;

        winetest_push_context("test_exceptions[%d]", i);

        if (exception_tests[i].with_child)
        {
            BOOL first_exception = TRUE;
            STARTUPINFOA si;
            char buffer[MAX_PATH];
            char **argv;

            winetest_get_mainargs(&argv);
            snprintf(buffer, ARRAY_SIZE(buffer), "%s minidump exception %x;%x;%u",
                     argv[0], exception_tests[i].exception_code, exception_tests[i].exception_flags,
                     exception_tests[i].num_args);
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
            ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());

            while ((ret = WaitForDebugEvent(&ev, 2000)))
            {
                if (!first_exception && ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) break;
                if (first_exception && ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT &&
                    ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                    first_exception = FALSE;
                ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
            };
            ok(ret, "Couldn't get debug event\n");
            ok(ev.dwThreadId == pi.dwThreadId, "Unexpected value\n");
            mei.ThreadId = ev.dwThreadId;
            mei.ExceptionPointers = &ep;
            mei.ClientPointers = FALSE;
            ep.ExceptionRecord = &ev.u.Exception.ExceptionRecord;
            ep.ContextRecord = &ctx;
            ctx.ContextFlags = CONTEXT_FULL;
            ret = GetThreadContext(pi.hThread, &ctx);
            ok(ret, "Couldn't get thread context\n");
            CloseHandle(pi.hThread);
        }
        else
        {
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = &ep;
            mei.ClientPointers = FALSE;
            ep.ExceptionRecord = &er;
            ep.ContextRecord = &ctx;
            memset(&ctx, 0xA5, sizeof(ctx));
            ctx.ContextFlags = CONTEXT_FULL;
            er.ExceptionCode = exception_tests[i].exception_code;
            er.ExceptionFlags = exception_tests[i].exception_flags;
            er.ExceptionAddress = (void *)(DWORD_PTR)0xdeadbeef;
            er.NumberParameters = exception_tests[i].num_args;
            for (j = 0; j < exception_tests[i].num_args; j++)
                er.ExceptionInformation[j] = args[j];
            pi.hProcess = GetCurrentProcess();
        }
        minidump_write(pi.hProcess, L"foo.mdmp", MiniDumpNormal, FALSE, &mei, NULL);

        data = minidump_open_for_read("foo.mdmp");
        ret = MiniDumpReadDumpStream(data, ExceptionStream, NULL, (void *)&except_info, &size);
        ok(ret, "Couldn't find exception stream\n");
        ok(except_info->ThreadId == mei.ThreadId, "Unexpected value\n");
        ok(except_info->ExceptionRecord.ExceptionCode == exception_tests[i].exception_code, "Unexpected value %x %x\n", except_info->ExceptionRecord.ExceptionCode, exception_tests[i].exception_code);
        /* windows 11 starts adding EXCEPTION_SOFTWARE_ORIGINATE flag */
        ok((except_info->ExceptionRecord.ExceptionFlags & ~EXCEPTION_SOFTWARE_ORIGINATE) == exception_tests[i].exception_flags, "Unexpected value\n");
        /* yes native does a signed conversion to DWORD64 when running on 32bit... */
        ok(except_info->ExceptionRecord.ExceptionAddress == (DWORD_PTR)ep.ExceptionRecord->ExceptionAddress
           || broken(except_info->ExceptionRecord.ExceptionAddress == (LONG_PTR)ep.ExceptionRecord->ExceptionAddress), "Unexpected value\n");
        if (except_info->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
        {
            /* number of parameters depend on machine, wow64... */
            ok(except_info->ExceptionRecord.NumberParameters, "Unexpected value %x\n", except_info->ExceptionRecord.NumberParameters);
        }
        else if (except_info->ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            ok(except_info->ExceptionRecord.NumberParameters == exception_tests[i].num_args, "Unexpected value\n");
        }
        else
        {
            ok(except_info->ExceptionRecord.NumberParameters == exception_tests[i].num_args, "Unexpected value\n");
            for (j = 0; j < exception_tests[i].num_args; j++)
                ok(except_info->ExceptionRecord.ExceptionInformation[j] == args[j], "Unexpected value\n");
        }
        ok(except_info->ThreadContext.Rva, "Unexpected value\n");
        mctx = RVA_TO_ADDR(data, except_info->ThreadContext.Rva);
        ok(!memcmp(mctx, &ctx, sizeof(ctx)), "Unexpected value\n");
        minidump_check_threads(data);
        minidump_close_for_read(data);
        DeleteFileA("foo.mdmp");
        winetest_pop_context();
        if (exception_tests[i].with_child)
        {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hProcess);
        }
    }
}

static void generate_child_exception(const char *arg)
{
    DWORD code, flags;
    unsigned num_args;

    if (sscanf(arg, "%lx;%lx;%u", &code, &flags, &num_args) == 3)
    {
        switch (code)
        {
        case EXCEPTION_BREAKPOINT:
            DbgBreakPoint();
            break;
        case EXCEPTION_ACCESS_VIOLATION:
            {
                /* volatile to silence gcc warning */
                char * volatile crashme = (char *)(DWORD_PTR)0x12;
                *crashme = 2;
            }
            break;
        default:
            {
                DWORD_PTR my_args[EXCEPTION_MAXIMUM_PARAMETERS];
                int i;

                for (i = 0; i < ARRAY_SIZE(my_args); i++)
                    my_args[i] = 0x666000 + i;
                RaiseException(code, flags, num_args, my_args);
            }
            break;
        }
    }
}

START_TEST(minidump)
{
    int argc;
    char **argv;
    argc = winetest_get_mainargs(&argv);
    if (argc == 4 && !strcmp(argv[2], "exception"))
    {
        generate_child_exception(argv[3]);
        return;
    }

    if ((pSetThreadDescription = (void *)GetProcAddress(GetModuleHandleW(L"kernel32"), "SetThreadDescription")))
        pSetThreadDescription(GetCurrentThread(), main_thread_name);

    test_minidump_contents();
    test_current_process();
    test_callback();
    test_exception();
}
