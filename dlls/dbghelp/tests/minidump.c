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

static BOOL minidump_write(HANDLE proc, const WCHAR *filename, MINIDUMP_TYPE type, BOOL windows_can_fail)
{
    HANDLE              file;
    BOOL                ret, ret2;

    file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    ok(file != INVALID_HANDLE_VALUE, "Failed to create minidump %ls\n", filename);

    ret = MiniDumpWriteDump(proc, GetProcessId(proc), file, type, NULL, NULL, NULL);
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
    MINIDUMP_HEADER *hdr;
    void *where;
    ULONG size;
    BOOL ret;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(streams_table); i++)
    {
        winetest_push_context("streams_table[%d]", i);
        if (minidump_write(GetCurrentProcess(), L"foo.mdmp", streams_table[i].type, i >= 6))
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
                todo_wine_if(streams_table[i].todo_wine_mask & STREAM2MASK(j))
                if (streams_table[i].streams_mask & STREAM2MASK(j))
                    ok((ret && where) || broken(BASIC_STREAM_BROKEN_MASK & STREAM2MASK(j)), "Expecting stream %d to be present\n", j);
                else
                    ok(!ret, "Not expecting stream %d to be present\n", j);
            }

            minidump_close_for_read(hdr);

            ret = DeleteFileA("foo.mdmp");
            ok(ret, "Couldn't delete file\n");
        }
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
    int i;
    BOOL ret;

    ret = MiniDumpReadDumpStream(data, ThreadListStream, NULL, (void**)&thread_list, NULL);
    ok(ret && thread_list, "Couldn't find thread-list stream\n");
    for (i = 0; i < thread_list->NumberOfThreads; i++)
    {
        const MINIDUMP_THREAD *thread = &thread_list->Threads[i];
        const CONTEXT *ctx;

        ok(thread->SuspendCount == 0, "Unexpected value\n");
        todo_wine
        ok(thread->Stack.StartOfMemoryRange, "Unexpected value\n");
        todo_wine
        ok(thread->Stack.Memory.DataSize, "Unexpected value\n");
        ok(thread->Teb, "Unexpected value\n");
        todo_wine
        ok(thread->ThreadContext.DataSize >= sizeof(CONTEXT), "Unexpected value\n");
        ctx = RVA_TO_ADDR(data, thread->ThreadContext.Rva);
        todo_wine
        ok((ctx->ContextFlags & CONTEXT_ALL) == CONTEXT_ALL, "Unexpected value\n");
    }
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
        minidump_write(GetCurrentProcess(), L"foo.mdmp", process_tests[i].dump_type, FALSE);

        data = minidump_open_for_read("foo.mdmp");

        num_threads = minidump_get_number_of_threads(data);
        ok(num_threads > 0, "Unexpected number of threads\n");

        minidump_check_threads(data);
        md = minidump_get_memory_description(data, (DWORD_PTR)&i);
        todo_wine
        ok(md.kind == MD_STACK, "Couldn't find automatic variable\n");

        md2 = minidump_get_memory_description(data, (DWORD_PTR)NtCurrentTeb()->Tib.StackBase - sizeof(void*));
        todo_wine
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
        todo_wine
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

        todo_wine_if(RtlImageNtHeader(GetModuleHandleW(NULL))->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        ok(walker.num_unwind_info == 0, "unexpected unwind info %u\n", walker.num_unwind_info);
        minidump_check_nostream(data, ExceptionStream);

        minidump_close_for_read(data);

        ret = DeleteFileA("foo.mdmp");
        ok(ret, "Couldn't delete file\n");
        winetest_pop_context();
    }
}

START_TEST(minidump)
{
    test_minidump_contents();
    test_current_process();
}
