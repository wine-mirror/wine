/*
 * Copyright 2018 Zebediah Figura
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

#include "windows.h"
#include "psapi.h"
#include "verrsrc.h"
#include "dbghelp.h"
#include "wine/test.h"
#include "winternl.h"

static const BOOL is_win64 = sizeof(void*) > sizeof(int);

#if defined(__i386__) || defined(__x86_64__)

static DWORD CALLBACK stack_walk_thread(void *arg)
{
    DWORD count = SuspendThread(GetCurrentThread());
    ok(!count, "got %ld\n", count);
    return 0;
}

static void test_stack_walk(void)
{
    char si_buf[sizeof(SYMBOL_INFO) + 200];
    SYMBOL_INFO *si = (SYMBOL_INFO *)si_buf;
    STACKFRAME64 frame = {{0}}, frame0;
    BOOL found_our_frame = FALSE;
    DWORD machine;
    HANDLE thread;
    DWORD64 disp;
    CONTEXT ctx;
    DWORD count;
    BOOL ret;

    thread = CreateThread(NULL, 0, stack_walk_thread, NULL, 0, NULL);

    /* wait for the thread to suspend itself */
    do
    {
        Sleep(50);
        count = SuspendThread(thread);
        ResumeThread(thread);
    }
    while (!count);

    ctx.ContextFlags = CONTEXT_CONTROL;
    ret = GetThreadContext(thread, &ctx);
    ok(ret, "got error %u\n", ret);

    frame.AddrPC.Mode    = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

#ifdef __i386__
    machine = IMAGE_FILE_MACHINE_I386;

    frame.AddrPC.Segment = ctx.SegCs;
    frame.AddrPC.Offset = ctx.Eip;
    frame.AddrFrame.Segment = ctx.SegSs;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrStack.Segment = ctx.SegSs;
    frame.AddrStack.Offset = ctx.Esp;
#elif defined(__x86_64__)
    machine = IMAGE_FILE_MACHINE_AMD64;

    frame.AddrPC.Segment = ctx.SegCs;
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrFrame.Segment = ctx.SegSs;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrStack.Segment = ctx.SegSs;
    frame.AddrStack.Offset = ctx.Rsp;
#endif
    frame0 = frame;

    /* first invocation just calculates the return address */
    ret = StackWalk64(machine, GetCurrentProcess(), thread, &frame, &ctx, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL);
    ok(ret, "StackWalk64() failed: %lu\n", GetLastError());
    ok(frame.AddrPC.Offset == frame0.AddrPC.Offset, "expected %s, got %s\n",
        wine_dbgstr_longlong(frame0.AddrPC.Offset),
        wine_dbgstr_longlong(frame.AddrPC.Offset));
    ok(frame.AddrStack.Offset == frame0.AddrStack.Offset, "expected %s, got %s\n",
        wine_dbgstr_longlong(frame0.AddrStack.Offset),
        wine_dbgstr_longlong(frame.AddrStack.Offset));
    ok(frame.AddrReturn.Offset && frame.AddrReturn.Offset != frame.AddrPC.Offset,
        "got bad return address %s\n", wine_dbgstr_longlong(frame.AddrReturn.Offset));

    while (frame.AddrReturn.Offset)
    {
        char *addr;

        ret = StackWalk64(machine, GetCurrentProcess(), thread, &frame, &ctx, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL);
        ok(ret, "StackWalk64() failed: %lu\n", GetLastError());

        addr = (void *)(DWORD_PTR)frame.AddrPC.Offset;

        if (!found_our_frame && addr > (char *)stack_walk_thread && addr < (char *)stack_walk_thread + 0x100)
        {
            found_our_frame = TRUE;

            si->SizeOfStruct = sizeof(SYMBOL_INFO);
            si->MaxNameLen = 200;
            if (SymFromAddr(GetCurrentProcess(), frame.AddrPC.Offset, &disp, si))
                ok(!strcmp(si->Name, "stack_walk_thread"), "got wrong name %s\n", si->Name);
        }
    }

    ret = StackWalk64(machine, GetCurrentProcess(), thread, &frame, &ctx, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL);
    ok(!ret, "StackWalk64() should have failed\n");

    ok(found_our_frame, "didn't find stack_walk_thread frame\n");
}

#else /* __i386__ || __x86_64__ */

static void test_stack_walk(void)
{
}

#endif /* __i386__ || __x86_64__ */

static void test_search_path(void)
{
    char search_path[128];
    BOOL ret;

    /* The default symbol path is ".[;%_NT_SYMBOL_PATH%][;%_NT_ALT_SYMBOL_PATH%]".
     * We unset both variables earlier so should simply get "." */
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, "."), "Got search path '%s', expected '.'\n", search_path);

    /* Set an arbitrary search path */
    ret = SymSetSearchPath(GetCurrentProcess(), "W:\\");
    ok(ret == TRUE, "ret = %d\n", ret);
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, "W:\\"), "Got search path '%s', expected 'W:\\'\n", search_path);

    /* Setting to NULL resets to the default */
    ret = SymSetSearchPath(GetCurrentProcess(), NULL);
    ok(ret == TRUE, "ret = %d\n", ret);
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, "."), "Got search path '%s', expected '.'\n", search_path);

    /* With _NT_SYMBOL_PATH */
    SetEnvironmentVariableA("_NT_SYMBOL_PATH", "X:\\");
    ret = SymSetSearchPath(GetCurrentProcess(), NULL);
    ok(ret == TRUE, "ret = %d\n", ret);
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, ".;X:\\"), "Got search path '%s', expected '.;X:\\'\n", search_path);

    /* With both _NT_SYMBOL_PATH and _NT_ALT_SYMBOL_PATH */
    SetEnvironmentVariableA("_NT_ALT_SYMBOL_PATH", "Y:\\");
    ret = SymSetSearchPath(GetCurrentProcess(), NULL);
    ok(ret == TRUE, "ret = %d\n", ret);
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, ".;X:\\;Y:\\"), "Got search path '%s', expected '.;X:\\;Y:\\'\n", search_path);

    /* With just _NT_ALT_SYMBOL_PATH */
    SetEnvironmentVariableA("_NT_SYMBOL_PATH", NULL);
    ret = SymSetSearchPath(GetCurrentProcess(), NULL);
    ok(ret == TRUE, "ret = %d\n", ret);
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, ".;Y:\\"), "Got search path '%s', expected '.;Y:\\'\n", search_path);

    /* Restore original search path */
    SetEnvironmentVariableA("_NT_ALT_SYMBOL_PATH", NULL);
    ret = SymSetSearchPath(GetCurrentProcess(), NULL);
    ok(ret == TRUE, "ret = %d\n", ret);
    ret = SymGetSearchPath(GetCurrentProcess(), search_path, ARRAY_SIZE(search_path));
    ok(ret == TRUE, "ret = %d\n", ret);
    ok(!strcmp(search_path, "."), "Got search path '%s', expected '.'\n", search_path);
}

static USHORT get_module_machine(const char* path)
{
    HANDLE hFile, hMap;
    void* mapping;
    IMAGE_NT_HEADERS *nthdr;
    USHORT machine = IMAGE_FILE_MACHINE_UNKNOWN;

    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hMap != NULL)
        {
            mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
            if (mapping != NULL)
            {
                nthdr = RtlImageNtHeader(mapping);
                if (nthdr != NULL) machine = nthdr->FileHeader.Machine;
                UnmapViewOfFile(mapping);
            }
            CloseHandle(hMap);
        }
        CloseHandle(hFile);
    }
    return machine;
}

static BOOL skip_too_old_dbghelp(HANDLE proc, DWORD64 base)
{
    IMAGEHLP_MODULE im0 = {sizeof(im0)};
    BOOL ret;

    /* test if get module info succeeds with oldest structure format */
    ret = SymGetModuleInfo(proc, base, &im0);
    if (ret)
    {
        skip("Too old dbghelp. Skipping module tests.\n");
        ret = SymCleanup(proc);
        ok(ret, "SymCleanup failed: %lu\n", GetLastError());
        return TRUE;
    }
    ok(ret, "SymGetModuleInfo failed: %lu\n", GetLastError());
    return FALSE;
}

static DWORD get_module_size(const char* path)
{
    BOOL ret;
    HANDLE hFile, hMap;
    void* mapping;
    IMAGE_NT_HEADERS *nthdr;
    DWORD size;

    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Couldn't open file %s (%lu)\n", path, GetLastError());
    hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    ok(hMap != NULL, "Couldn't create map (%lu)\n", GetLastError());
    mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    ok(mapping != NULL, "Couldn't map (%lu)\n", GetLastError());
    nthdr = RtlImageNtHeader(mapping);
    ok(nthdr != NULL, "Cannot get NT headers out of %s\n", path);
    size = nthdr ? nthdr->OptionalHeader.SizeOfImage : 0;
    ret = UnmapViewOfFile(mapping);
    ok(ret, "Couldn't unmap (%lu)\n", GetLastError());
    CloseHandle(hMap);
    CloseHandle(hFile);
    return size;
}

static BOOL CALLBACK count_module_cb(const char* name, DWORD64 base, void* usr)
{
    (*(unsigned*)usr)++;
    return TRUE;
}

static unsigned get_module_count(HANDLE proc)
{
    unsigned count = 0;
    BOOL ret;

    ret = SymEnumerateModules64(proc, count_module_cb, &count);
    ok(ret, "SymEnumerateModules64 failed: %lu\n", GetLastError());
    return count;
}

struct nth_module
{
    HANDLE              proc;
    unsigned int        index;
    BOOL                will_fail;
    IMAGEHLP_MODULE64   module;
};

static BOOL CALLBACK nth_module_cb(const char* name, DWORD64 base, void* usr)
{
    struct nth_module* nth = usr;
    BOOL ret;

    if (nth->index--) return TRUE;
    nth->module.SizeOfStruct = sizeof(nth->module);
    ret = SymGetModuleInfo64(nth->proc, base, &nth->module);
    if (nth->will_fail)
    {
        ok(!ret, "SymGetModuleInfo64 should have failed\n");
        nth->module.BaseOfImage = base;
    }
    else
    {
        ok(ret, "SymGetModuleInfo64 failed: %lu\n", GetLastError());
        ok(nth->module.BaseOfImage == base, "Wrong base\n");
    }
    return FALSE;
}

static BOOL test_modules(void)
{
    BOOL ret;
    char file_system[MAX_PATH];
    char file_wow64[MAX_PATH];
    DWORD64 base;
    const DWORD64 base1 = 0x00010000;
    const DWORD64 base2 = 0x08010000;
    IMAGEHLP_MODULEW64 im;
    USHORT machine_wow, machine2;
    HANDLE dummy = (HANDLE)(ULONG_PTR)0xcafef00d;
    const char* target_dll = "c:\\windows\\system32\\kernel32.dll";
    unsigned count;

    im.SizeOfStruct = sizeof(im);

    /* can sym load an exec of different bitness even if 32Bit flag not set */

    SymSetOptions(SymGetOptions() & ~SYMOPT_INCLUDE_32BIT_MODULES);
    ret = SymInitialize(GetCurrentProcess(), 0, FALSE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());

    GetSystemWow64DirectoryA(file_wow64, MAX_PATH);
    strcat(file_wow64, "\\notepad.exe");

    /* not always present */
    machine_wow = get_module_machine(file_wow64);
    if (machine_wow != IMAGE_FILE_MACHINE_UNKNOWN)
    {
        base = SymLoadModule(GetCurrentProcess(), NULL, file_wow64, NULL, base2, 0);
        ok(base == base2, "SymLoadModule failed: %lu\n", GetLastError());
        ret = SymGetModuleInfoW64(GetCurrentProcess(), base2, &im);
        if (!ret && skip_too_old_dbghelp(GetCurrentProcess(), base2)) return FALSE;
        ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
        ok(im.BaseOfImage == base2, "Wrong base address\n");
        ok(im.MachineType == machine_wow, "Wrong machine %lx (expecting %u)\n", im.MachineType, machine_wow);
    }

    GetSystemDirectoryA(file_system, MAX_PATH);
    strcat(file_system, "\\notepad.exe");

    base = SymLoadModule(GetCurrentProcess(), NULL, file_system, NULL, base1, 0);
    ok(base == base1, "SymLoadModule failed: %lu\n", GetLastError());
    ret = SymGetModuleInfoW64(GetCurrentProcess(), base1, &im);
    if (!ret && skip_too_old_dbghelp(GetCurrentProcess(), base1)) return FALSE;
    ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
    ok(im.BaseOfImage == base1, "Wrong base address\n");
    machine2 = get_module_machine(file_system);
    ok(machine2 != IMAGE_FILE_MACHINE_UNKNOWN, "Unexpected machine %u\n", machine2);
    ok(im.MachineType == machine2, "Wrong machine %lx (expecting %u)\n", im.MachineType, machine2);

    /* still can access first module after loading second */
    if (machine_wow != IMAGE_FILE_MACHINE_UNKNOWN)
    {
        ret = SymGetModuleInfoW64(GetCurrentProcess(), base2, &im);
        ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
        ok(im.BaseOfImage == base2, "Wrong base address\n");
        ok(im.MachineType == machine_wow, "Wrong machine %lx (expecting %u)\n", im.MachineType, machine_wow);
    }

    ret = SymCleanup(GetCurrentProcess());
    ok(ret, "SymCleanup failed: %lu\n", GetLastError());

    ret = SymInitialize(dummy, NULL, FALSE);
    ok(ret, "got error %lu\n", GetLastError());

    count = get_module_count(dummy);
    ok(count == 0, "Unexpected count (%u instead of 0)\n", count);

    /* loading with 0 size succeeds */
    base = SymLoadModuleEx(dummy, NULL, target_dll, NULL, base1, 0, NULL, 0);
    ok(base == base1, "SymLoadModuleEx failed: %lu\n", GetLastError());

    ret = SymUnloadModule64(dummy, base1);
    ok(ret, "SymUnloadModule64 failed: %lu\n", GetLastError());

    count = get_module_count(dummy);
    ok(count == 0, "Unexpected count (%u instead of 0)\n", count);

    ret = SymCleanup(dummy);
    ok(ret, "SymCleanup failed: %lu\n", GetLastError());

    return TRUE;
}

static void test_modules_overlap(void)
{
    BOOL ret;
    DWORD64 base;
    const DWORD64 base1 = 0x00010000;
    HANDLE dummy = (HANDLE)(ULONG_PTR)0xcafef00d;
    const char* target1_dll = "c:\\windows\\system32\\kernel32.dll";
    const char* target2_dll = "c:\\windows\\system32\\winmm.dll";
    DWORD imsize = get_module_size(target1_dll);
    int i, j;
    struct test_module
    {
        DWORD64             base;
        DWORD               size;
        const char*         name;
    };
    const struct test
    {
        struct test_module  input;
        DWORD               error_code;
        struct test_module  outputs[2];
    }
    tests[] =
    {
        /* cases where first module is left "untouched" and second not loaded */
        {{base1,              0,          target1_dll}, ERROR_SUCCESS, {{base1, imsize, "kernel32"},{0}}},
        {{base1,              imsize,     target1_dll}, ERROR_SUCCESS, {{base1, imsize, "kernel32"},{0}}},
        {{base1,              imsize / 2, target1_dll}, ERROR_SUCCESS, {{base1, imsize, "kernel32"},{0}}},
        {{base1,              imsize * 2, target1_dll}, ERROR_SUCCESS, {{base1, imsize, "kernel32"},{0}}},

        /* cases where first module is unloaded and replaced by second module */
        {{base1 + imsize / 2, imsize,     target1_dll}, ~0,            {{base1 + imsize / 2, imsize,     "kernel32"},{0}}},
        {{base1 + imsize / 3, imsize / 3, target1_dll}, ~0,            {{base1 + imsize / 3, imsize / 3, "kernel32"},{0}}},
        {{base1 + imsize / 2, imsize,     target2_dll}, ~0,            {{base1 + imsize / 2, imsize,     "winmm"},{0}}},
        {{base1 + imsize / 3, imsize / 3, target2_dll}, ~0,            {{base1 + imsize / 3, imsize / 3, "winmm"},{0}}},

        /* cases where second module is actually loaded */
        {{base1 + imsize,     imsize,     target1_dll}, ~0,            {{base1, imsize, "kernel32"}, {base1 + imsize, imsize, "kernel32"}}},
        {{base1 - imsize / 2, imsize,     target1_dll}, ~0,            {{base1, imsize, "kernel32"}, {base1 - imsize / 2, imsize, NULL}}},
        /* we mark with a NULL modulename the cases where the module is loaded, but isn't visible
         * (SymGetModuleInfo fails in callback) as it's base address is inside the first loaded module.
         */
        {{base1 + imsize,     imsize,     target2_dll}, ~0,            {{base1, imsize, "kernel32"}, {base1 + imsize, imsize, "winmm"}}},
        {{base1 - imsize / 2, imsize,     target2_dll}, ~0,            {{base1, imsize, "kernel32"}, {base1 - imsize / 2, imsize, NULL}}},
    };

    ok(imsize, "Cannot get image size\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ret = SymInitialize(dummy, NULL, FALSE);
        ok(ret, "SymInitialize failed: %lu\n", GetLastError());

        base = SymLoadModuleEx(dummy, NULL, target1_dll, NULL, base1, 0, NULL, 0);
        ok(base == base1, "SymLoadModuleEx failed: %lu\n", GetLastError());
        base = SymLoadModuleEx(dummy, NULL, tests[i].input.name, NULL, tests[i].input.base, tests[i].input.size, NULL, 0);
        if (tests[i].error_code != ~0)
        {
            ok(base == 0, "SymLoadModuleEx should have failed\n");
            ok(GetLastError() == tests[i].error_code, "Wrong error %lu\n", GetLastError());
        }
        else
        {
            ok(base == tests[i].input.base, "SymLoadModuleEx failed: %lu\n", GetLastError());
        }
        for (j = 0; j < ARRAY_SIZE(tests[i].outputs); j++)
        {
            struct nth_module nth = {dummy, j, !tests[i].outputs[j].name, {0}};

            ret = SymEnumerateModules64(dummy, nth_module_cb, &nth);
            ok(ret, "SymEnumerateModules64 failed: %lu\n", GetLastError());
            if (!tests[i].outputs[j].base)
            {
                ok(nth.index != -1, "Got more modules than expected %d, %d\n", nth.index, j);
                break;
            }
            ok(nth.index == -1, "Expecting more modules\n");
            ok(nth.module.BaseOfImage == tests[i].outputs[j].base, "Wrong base\n");
            if (!nth.will_fail)
            {
                ok(nth.module.ImageSize == tests[i].outputs[j].size, "Wrong size\n");
                ok(!strcasecmp(nth.module.ModuleName, tests[i].outputs[j].name), "Wrong name\n");
            }
        }
        ret = SymCleanup(dummy);
        ok(ret, "SymCleanup failed: %lu\n", GetLastError());
    }
}

struct loaded_module_aggregation
{
    HANDLE       proc;
    unsigned int count_32bit;
    unsigned int count_64bit;
    unsigned int count_exe;
    unsigned int count_ntdll;
    unsigned int count_systemdir;
    unsigned int count_wowdir;
};

static BOOL CALLBACK aggregate_cb(PCWSTR imagename, DWORD64 base, ULONG sz, PVOID usr)
{
    struct loaded_module_aggregation* aggregation = usr;
    IMAGEHLP_MODULEW64 im;
    size_t image_len;
    BOOL ret, wow64;
    WCHAR buffer[MAX_PATH];

    memset(&im, 0, sizeof(im));
    im.SizeOfStruct = sizeof(im);

    image_len = wcslen(imagename);

    ret = SymGetModuleInfoW64(aggregation->proc, base, &im);
    if (ret)
        ok(aggregation->count_exe && image_len >= 4 && !wcscmp(imagename + image_len - 4, L".exe"),
           "%ls shouldn't already be loaded\n", imagename);
    else
    {
        ok(!ret, "Module %ls shouldn't be loaded\n", imagename);
        ret = SymLoadModuleExW(aggregation->proc, NULL, imagename, NULL, base, sz, NULL, 0);
        ok(ret, "SymLoadModuleExW failed on %ls: %lu\n", imagename, GetLastError());
        ret = SymGetModuleInfoW64(aggregation->proc, base, &im);
        ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
    }

    switch (im.MachineType)
    {
    case IMAGE_FILE_MACHINE_UNKNOWN:
        break;
    case IMAGE_FILE_MACHINE_I386:
    case IMAGE_FILE_MACHINE_ARM:
    case IMAGE_FILE_MACHINE_ARMNT:
        aggregation->count_32bit++;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
    case IMAGE_FILE_MACHINE_ARM64:
        aggregation->count_64bit++;
        break;
    default:
        ok(0, "Unsupported machine %lx\n", im.MachineType);
        break;
    }
    if (image_len >= 4 && !wcsicmp(imagename + image_len - 4, L".exe"))
        aggregation->count_exe++;
    if (!wcsicmp(im.ModuleName, L"ntdll"))
        aggregation->count_ntdll++;
    if (GetSystemDirectoryW(buffer, ARRAY_SIZE(buffer)) &&
        !wcsnicmp(imagename, buffer, wcslen(buffer)))
        aggregation->count_systemdir++;
    if (is_win64 && IsWow64Process(aggregation->proc, &wow64) && wow64 &&
        GetSystemWow64DirectoryW(buffer, ARRAY_SIZE(buffer)) &&
        !wcsnicmp(imagename, buffer, wcslen(buffer)))
        aggregation->count_wowdir++;

    return TRUE;
}


static void test_loaded_modules(void)
{
    BOOL ret;
    char buffer[200];
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOA si = {0};
    struct loaded_module_aggregation aggregation = {0};

    ret = GetSystemDirectoryA(buffer, sizeof(buffer));
    ok(ret, "got error %lu\n", GetLastError());
    strcat(buffer, "\\notepad.exe");

    /* testing with child process of different machines */
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed: %lu\n", GetLastError());

    ret = WaitForInputIdle(pi.hProcess, 5000);
    ok(!ret, "wait timed out\n");

    ret = SymInitialize(pi.hProcess, NULL, FALSE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());
    memset(&aggregation, 0, sizeof(aggregation));
    aggregation.proc = pi.hProcess;

    ret = EnumerateLoadedModulesW64(pi.hProcess, aggregate_cb, &aggregation);
    ok(ret, "EnumerateLoadedModulesW64 failed: %lu\n", GetLastError());

    if (is_win64)
    {
        ok(!aggregation.count_32bit && aggregation.count_64bit, "Wrong bitness aggregation count %u %u\n",
           aggregation.count_32bit, aggregation.count_64bit);
        ok(aggregation.count_exe == 1 && aggregation.count_ntdll == 1, "Wrong kind aggregation count %u %u\n",
           aggregation.count_exe, aggregation.count_ntdll);
        ok(aggregation.count_systemdir > 2 && !aggregation.count_wowdir, "Wrong directory aggregation count %u %u\n",
           aggregation.count_systemdir, aggregation.count_wowdir);
    }
    else
    {
        BOOL is_wow64;
        ret = IsWow64Process(pi.hProcess, &is_wow64);
        ok(ret, "IsWow64Process failed: %lu\n", GetLastError());

        ok(aggregation.count_32bit && !aggregation.count_64bit, "Wrong bitness aggregation count %u %u\n",
           aggregation.count_32bit, aggregation.count_64bit);
        ok(aggregation.count_exe == 1 && aggregation.count_ntdll == 1, "Wrong kind aggregation count %u %u\n",
           aggregation.count_exe, aggregation.count_ntdll);
        todo_wine_if(is_wow64)
        ok(aggregation.count_systemdir > 2 && !aggregation.count_wowdir, "Wrong directory aggregation count %u %u\n",
           aggregation.count_systemdir, aggregation.count_wowdir);
    }

    SymCleanup(pi.hProcess);
    TerminateProcess(pi.hProcess, 0);

    if (is_win64)
    {
        ret = GetSystemWow64DirectoryA(buffer, sizeof(buffer));
        ok(ret, "got error %lu\n", GetLastError());
        strcat(buffer, "\\notepad.exe");

        SymSetOptions(SymGetOptions() & ~SYMOPT_INCLUDE_32BIT_MODULES);

        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ret)
        {
            ret = WaitForInputIdle(pi.hProcess, 5000);
            ok(!ret, "wait timed out\n");

            ret = SymInitialize(pi.hProcess, NULL, FALSE);
            ok(ret, "SymInitialize failed: %lu\n", GetLastError());
            memset(&aggregation, 0, sizeof(aggregation));
            aggregation.proc = pi.hProcess;

            ret = EnumerateLoadedModulesW64(pi.hProcess, aggregate_cb, &aggregation);
            ok(ret, "EnumerateLoadedModulesW64 failed: %lu\n", GetLastError());

            todo_wine
            ok(aggregation.count_32bit == 1 && aggregation.count_64bit, "Wrong bitness aggregation count %u %u\n",
               aggregation.count_32bit, aggregation.count_64bit);
            ok(aggregation.count_exe == 1 && aggregation.count_ntdll == 1, "Wrong kind aggregation count %u %u\n",
               aggregation.count_exe, aggregation.count_ntdll);
            todo_wine
            ok(aggregation.count_systemdir > 2 && aggregation.count_64bit == aggregation.count_systemdir && aggregation.count_wowdir == 1,
               "Wrong directory aggregation count %u %u\n",
               aggregation.count_systemdir, aggregation.count_wowdir);

            SymCleanup(pi.hProcess);
            TerminateProcess(pi.hProcess, 0);
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                skip("Skip wow64 test on non compatible platform\n");
            else
                ok(ret, "CreateProcess failed: %lu\n", GetLastError());
        }

        SymSetOptions(SymGetOptions() | SYMOPT_INCLUDE_32BIT_MODULES);

        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ret)
        {
            struct loaded_module_aggregation aggregation2 = {0};

            ret = WaitForInputIdle(pi.hProcess, 5000);
            ok(!ret, "wait timed out\n");

            ret = SymInitialize(pi.hProcess, NULL, FALSE);
            ok(ret, "SymInitialize failed: %lu\n", GetLastError());
            memset(&aggregation2, 0, sizeof(aggregation2));
            aggregation2.proc = pi.hProcess;
            ret = EnumerateLoadedModulesW64(pi.hProcess, aggregate_cb, &aggregation2);
            ok(ret, "EnumerateLoadedModulesW64 failed: %lu\n", GetLastError());

            ok(aggregation2.count_32bit && aggregation2.count_64bit, "Wrong bitness aggregation count %u %u\n",
               aggregation2.count_32bit, aggregation2.count_64bit);
            todo_wine
            ok(aggregation2.count_exe == 2 && aggregation2.count_ntdll == 2, "Wrong kind aggregation count %u %u\n",
               aggregation2.count_exe, aggregation2.count_ntdll);
            todo_wine
            ok(aggregation2.count_systemdir > 2 && aggregation2.count_64bit == aggregation2.count_systemdir && aggregation2.count_wowdir > 2,
               "Wrong directory aggregation count %u %u\n",
               aggregation2.count_systemdir, aggregation2.count_wowdir);

            SymCleanup(pi.hProcess);
            TerminateProcess(pi.hProcess, 0);
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                skip("Skip wow64 test on non compatible platform\n");
            else
                ok(ret, "CreateProcess failed: %lu\n", GetLastError());
        }
    }
}

START_TEST(dbghelp)
{
    BOOL ret;

    /* Don't let the user's environment influence our symbol path */
    SetEnvironmentVariableA("_NT_SYMBOL_PATH", NULL);
    SetEnvironmentVariableA("_NT_ALT_SYMBOL_PATH", NULL);

    ret = SymInitialize(GetCurrentProcess(), NULL, TRUE);
    ok(ret, "got error %lu\n", GetLastError());

    test_stack_walk();
    test_search_path();

    ret = SymCleanup(GetCurrentProcess());
    ok(ret, "got error %lu\n", GetLastError());

    if (test_modules())
    {
        test_modules_overlap();
        test_loaded_modules();
    }
}
