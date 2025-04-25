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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "psapi.h"
#include "verrsrc.h"
#include "dbghelp.h"
#include "wine/test.h"
#include "winternl.h"

static const BOOL is_win64 = sizeof(void*) > sizeof(int);
static WCHAR system_directory[MAX_PATH];
static WCHAR wow64_directory[MAX_PATH];

static BOOL (*WINAPI pIsWow64Process2)(HANDLE, USHORT*, USHORT*);

struct startup_cb
{
    DWORD pid;
    HWND wnd;
};

static BOOL CALLBACK startup_cb_window(HWND wnd, LPARAM lParam)
{
    struct startup_cb *info = (struct startup_cb*)lParam;
    DWORD pid;

    if (GetWindowThreadProcessId(wnd, &pid) && info->pid == pid && IsWindowVisible(wnd))
    {
        info->wnd = wnd;
        return FALSE;
    }
    return TRUE;
}

static BOOL wait_process_window_visible(HANDLE proc, DWORD pid, DWORD timeout)
{
    DWORD max_tc = GetTickCount() + timeout;
    BOOL ret = WaitForInputIdle(proc, timeout);
    struct startup_cb info = {pid, NULL};

    if (!ret)
    {
        do
        {
            if (EnumWindows(startup_cb_window, (LPARAM)&info))
                Sleep(100);
        } while (!info.wnd && GetTickCount() < max_tc);
    }
    return info.wnd != NULL;
}

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

static BOOL ends_withW(const WCHAR* str, const WCHAR* suffix)
{
    size_t strlen = wcslen(str);
    size_t sfxlen = wcslen(suffix);

    return strlen >= sfxlen && !wcsicmp(str + strlen - sfxlen, suffix);
}

static unsigned get_machine_bitness(USHORT machine)
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:
    case IMAGE_FILE_MACHINE_ARM:
    case IMAGE_FILE_MACHINE_ARMNT:
        return 32;
    case IMAGE_FILE_MACHINE_AMD64:
    case IMAGE_FILE_MACHINE_ARM64:
        return 64;
    default:
        ok(0, "Unsupported machine %x\n", machine);
        return 0;
    }
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

static BOOL CALLBACK count_native_module_cb(const char* name, DWORD64 base, void* usr)
{
    if (strstr(name, ".so") || strchr(name, '<')) (*(unsigned*)usr)++;
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

static unsigned get_native_module_count(HANDLE proc)
{
    static BOOL (*WINAPI pSymSetExtendedOption)(IMAGEHLP_EXTENDED_OPTIONS option, BOOL value);
    unsigned count = 0;
    BOOL old, ret;

    if (!pSymSetExtendedOption)
    {
        pSymSetExtendedOption = (void*)GetProcAddress(GetModuleHandleA("dbghelp.dll"), "SymSetExtendedOption");
        ok(pSymSetExtendedOption != NULL, "SymSetExtendedOption should be present on Wine\n");
    }

    old = pSymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    ret = SymEnumerateModules64(proc, count_native_module_cb, &count);
    ok(ret, "SymEnumerateModules64 failed: %lu\n", GetLastError());
    pSymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, old);

    return count;
}

struct module_present
{
    const WCHAR* module_name;
    BOOL found;
};

static BOOL CALLBACK is_module_present_cb(const WCHAR* name, DWORD64 base, void* usr)
{
    struct module_present* present = usr;
    if (!wcsicmp(name, present->module_name))
    {
        present->found = TRUE;
        return FALSE;
    }
    return TRUE;
}

static BOOL is_module_present(HANDLE proc, const WCHAR* module_name)
{
    struct module_present present = { .module_name = module_name };
    return SymEnumerateModulesW64(proc, is_module_present_cb, &present) && present.found;
}

struct nth_module
{
    HANDLE              proc;
    unsigned int        index;
    BOOL                could_fail;
    IMAGEHLP_MODULE64   module;
};

static BOOL CALLBACK nth_module_cb(const char* name, DWORD64 base, void* usr)
{
    struct nth_module* nth = usr;
    BOOL ret;

    if (nth->index--) return TRUE;
    nth->module.SizeOfStruct = sizeof(nth->module);
    ret = SymGetModuleInfo64(nth->proc, base, &nth->module);
    if (nth->could_fail)
    {
        /* Windows11 succeeds into loading the overlapped module */
        ok(!ret || broken(base == nth->module.BaseOfImage), "SymGetModuleInfo64 should have failed\n");
        nth->module.BaseOfImage = base;
    }
    else
    {
        ok(ret, "SymGetModuleInfo64 failed: %lu\n", GetLastError());
        ok(nth->module.BaseOfImage == base, "Wrong base\n");
    }
    return FALSE;
}

/* wrapper around EnumerateLoadedModuleW64 which sometimes fails for unknown reasons on Win11,
 * with STATUS_INFO_LENGTH_MISMATCH as GetLastError()!
 */
static BOOL wrapper_EnumerateLoadedModulesW64(HANDLE proc, PENUMLOADED_MODULES_CALLBACKW64 cb, void* usr)
{
    BOOL ret;
    int retry;
    int retry_count = !strcmp(winetest_platform, "wine") ? 1 : 5;

    for (retry = retry_count - 1; retry >= 0; retry--)
    {
        ret = EnumerateLoadedModulesW64(proc, cb, usr);
        if (ret || GetLastError() != STATUS_INFO_LENGTH_MISMATCH)
            break;
        Sleep(10);
    }
    if (retry + 1 < retry_count)
        trace("used wrapper retry: ret=%d retry=%d top=%d\n", ret, retry, retry_count);

    return ret;
}

/* wrapper around SymRefreshModuleList which sometimes fails (it's very likely implemented on top
 * of EnumerateLoadedModulesW64 on native too)
 */
static BOOL wrapper_SymRefreshModuleList(HANDLE proc)
{
    BOOL ret;
    int retry;
    int retry_count = !strcmp(winetest_platform, "wine") ? 1 : 5;

    for (retry = retry_count - 1; retry >= 0; retry--)
    {
        ret = SymRefreshModuleList(proc);
        if (ret || (GetLastError() != STATUS_INFO_LENGTH_MISMATCH && GetLastError() == STATUS_PARTIAL_COPY))
            break;
        Sleep(10);
    }
    if (retry + 1 < retry_count)
        trace("used wrapper retry: ret=%d retry=%d top=%d\n", ret, retry, retry_count);

    return ret;
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
    strcat(file_wow64, "\\msinfo32.exe");

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
    strcat(file_system, "\\msinfo32.exe");

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

    ret = SymRefreshModuleList(dummy);
    ok(!ret, "SymRefreshModuleList should have failed\n");
    ok(GetLastError() == STATUS_INVALID_CID, "Unexpected last error %lx\n", GetLastError());

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

struct test_module
{
    DWORD64             base;
    DWORD               size;
    const char*         name;
};

static struct test_module get_test_module(const char* path)
{
    struct test_module tm;
    HANDLE dummy = (HANDLE)(ULONG_PTR)0xcafef00d;
    IMAGEHLP_MODULEW64 im;
    BOOL ret;

    ret = SymInitialize(dummy, NULL, FALSE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());

    tm.base = SymLoadModuleEx(dummy, NULL, path, NULL, 0, 0, NULL, 0);
    ok(tm.base != 0, "SymLoadModuleEx failed: %lu\n", GetLastError());

    im.SizeOfStruct = sizeof(im);
    ret = SymGetModuleInfoW64(dummy, tm.base, &im);
    ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());

    tm.size = im.ImageSize;
    ok(tm.base == im.BaseOfImage, "Unexpected image base\n");
    ok(tm.size == get_module_size(path), "Unexpected size\n");
    tm.name = path;

    ret = SymCleanup(dummy);
    ok(ret, "SymCleanup failed: %lu\n", GetLastError());

    return tm;
}

static void test_modules_overlap(void)
{
    BOOL ret;
    DWORD64 base[2];
    const DWORD64 base1 = 0x00010000;
    HANDLE dummy = (HANDLE)(ULONG_PTR)0xcafef00d;
    const char* target1_dll = "c:\\windows\\system32\\kernel32.dll";
    const char* target2_dll = "c:\\windows\\system32\\winmm.dll";
    const char* target3_dll = "c:\\windows\\system32\\idontexist.dll";
    char buffer[512];
    IMAGEHLP_SYMBOL64* sym = (void*)buffer;

    int i, j;
    struct test_module target1_dflt = get_test_module(target1_dll);
    DWORD64 base0 = target1_dflt.base;
    DWORD64 imsize0 = target1_dflt.size;
    const struct test
    {
        DWORD64             first_base;
        struct test_module  input;
        DWORD               error_code;
        struct test_module  outputs[2];
    }
    tests[] =
    {
        /* cases where first module is left "untouched" and second not loaded */
/* 0*/  {base1, {base1,               0,           target1_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0,     target1_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0 / 2, target1_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0 * 2, target1_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},

        {base1, {base1,               0,           target2_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
/* 5*/  {base1, {base1,               imsize0,     target2_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0 / 2, target2_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0 * 2, target2_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},

        {base1, {base1,               0,           target3_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0,     target3_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
/*10*/  {base1, {base1,               imsize0 / 2, target3_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},
        {base1, {base1,               imsize0 * 2, target3_dll}, ERROR_SUCCESS, {{base1, imsize0, "kernel32"}, {0}}},

        {base0, {base0,               0,           target1_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0,     target1_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0 / 2, target1_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
/*15*/  {base0, {base0,               imsize0 * 2, target1_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},

        {base0, {base0,               0,           target2_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0,     target2_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0 / 2, target2_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0 * 2, target2_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},

/*20*/  {base0, {base0,               0,           target3_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0,     target3_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0 / 2, target3_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {base0,               imsize0 * 2, target3_dll}, ERROR_SUCCESS, {{base0, imsize0, "kernel32"}, {0}}},

        /* error cases for second module */
        {base0, {0,                   0,           target1_dll}, ERROR_INVALID_ADDRESS, {{base0, imsize0, "kernel32"}, {0}}},
/*25*/  {base0, {0,                   imsize0,     target1_dll}, ERROR_INVALID_ADDRESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {0,                   imsize0 / 2, target1_dll}, ERROR_INVALID_ADDRESS, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {0,                   imsize0 * 2, target1_dll}, ERROR_INVALID_ADDRESS, {{base0, imsize0, "kernel32"}, {0}}},

        {base0, {0,                   0,           target3_dll}, ERROR_NO_MORE_FILES, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {0,                   imsize0,     target3_dll}, ERROR_NO_MORE_FILES, {{base0, imsize0, "kernel32"}, {0}}},
/*30*/  {base0, {0,                   imsize0 / 2, target3_dll}, ERROR_NO_MORE_FILES, {{base0, imsize0, "kernel32"}, {0}}},
        {base0, {0,                   imsize0 * 2, target3_dll}, ERROR_NO_MORE_FILES, {{base0, imsize0, "kernel32"}, {0}}},

        /* cases where first module is unloaded and replaced by second module */
        {base1, {base1 + imsize0 / 2, imsize0,     target1_dll}, ~0,            {{base1 + imsize0 / 2, imsize0,     "kernel32"}, {0}}},
        {base1, {base1 + imsize0 / 3, imsize0 / 3, target1_dll}, ~0,            {{base1 + imsize0 / 3, imsize0 / 3, "kernel32"}, {0}}},
        {base1, {base1 + imsize0 / 2, imsize0,     target2_dll}, ~0,            {{base1 + imsize0 / 2, imsize0,     "winmm"}, {0}}},
/*35*/  {base1, {base1 + imsize0 / 3, imsize0 / 3, target2_dll}, ~0,            {{base1 + imsize0 / 3, imsize0 / 3, "winmm"}, {0}}},

        /* cases where second module is actually loaded */
        {base1, {base1 + imsize0,     imsize0,     target1_dll}, ~0,            {{base1, imsize0, "kernel32"}, {base1 + imsize0, imsize0, "kernel32"}}},
        {base1, {base1 - imsize0 / 2, imsize0,     target1_dll}, ~0,            {{base1, imsize0, "kernel32"}, {base1 - imsize0 / 2, imsize0, NULL}}},
        /* we mark with a NULL modulename the cases where the module is loaded, but isn't visible
         * (SymGetModuleInfo fails in callback) as it's base address is inside the first loaded module.
         */
        {base1, {base1 + imsize0,     imsize0,     target2_dll}, ~0,            {{base1, imsize0, "kernel32"}, {base1 + imsize0, imsize0, "winmm"}}},
        {base1, {base1 - imsize0 / 2, imsize0,     target2_dll}, ~0,            {{base1, imsize0, "kernel32"}, {base1 - imsize0 / 2, imsize0, NULL}}},
    };

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("overlap tests[%d]", i);
        ret = SymInitialize(dummy, NULL, FALSE);
        ok(ret, "SymInitialize failed: %lu\n", GetLastError());

        base[0] = SymLoadModuleEx(dummy, NULL, target1_dll, NULL, tests[i].first_base, 0, NULL, 0);
        ok(base[0] == tests[i].first_base ? tests[i].first_base : base0, "SymLoadModuleEx failed: %lu\n", GetLastError());
        ret = SymAddSymbol(dummy, base[0], "winetest_symbol_virtual", base[0] + (3 * imsize0) / 4, 13, 0);
        ok(ret, "SymAddSymbol failed: %lu\n", GetLastError());

        base[1] = SymLoadModuleEx(dummy, NULL, tests[i].input.name, NULL, tests[i].input.base, tests[i].input.size, NULL, 0);
        if (tests[i].error_code != ~0)
        {
            ok(base[1] == 0, "SymLoadModuleEx should have failed\n");
            ok(GetLastError() == tests[i].error_code ||
               /* Win8 returns this */
               (tests[i].error_code == ERROR_NO_MORE_FILES && broken(GetLastError() == ERROR_INVALID_HANDLE)),
               "Wrong error %lu\n", GetLastError());
        }
        else
        {
            ok(base[1] == tests[i].input.base, "SymLoadModuleEx failed: %lu\n", GetLastError());
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
            if (!nth.could_fail)
            {
                ok(nth.module.ImageSize == tests[i].outputs[j].size, "Wrong size\n");
                ok(!strcasecmp(nth.module.ModuleName, tests[i].outputs[j].name), "Wrong name\n");
            }
        }
        memset(sym, 0, sizeof(*sym));
        sym->SizeOfStruct = sizeof(*sym);
        sym->MaxNameLength = sizeof(buffer) - sizeof(*sym);
        ret = SymGetSymFromName64(dummy, "winetest_symbol_virtual", sym);
        if (tests[i].error_code != ~0 || tests[i].outputs[1].base)
        {
            /* first module is not unloaded, so our added symbol should be present, with right attributes */
            ok(ret, "SymGetSymFromName64 has failed: %lu (%d, %d)\n", GetLastError(), i, j);
            ok(sym->Address == base[0] + (3 * imsize0) / 4, "Unexpected size %lu\n", sym->Size);
            ok(sym->Size == 13, "Unexpected size %lu\n", sym->Size);
            ok(sym->Flags & SYMFLAG_VIRTUAL, "Unexpected flag %lx\n", sym->Flags);
        }
        else
        {
            /* the first module is really unloaded, the our added symbol has disappead */
            ok(!ret, "SymGetSymFromName64 should have failed %lu\n", GetLastError());
        }
        ret = SymCleanup(dummy);
        ok(ret, "SymCleanup failed: %lu\n", GetLastError());
        winetest_pop_context();
    }
}

enum process_kind
{
    PCSKIND_ERROR,
    PCSKIND_64BIT,          /* 64 bit process */
    PCSKIND_32BIT,          /* 32 bit only configuration (Wine, some Win 7...) */
    PCSKIND_WINE_OLD_WOW64, /* Wine "old" wow64 configuration */
    PCSKIND_WOW64,          /* Wine "new" wow64 configuration, and Windows with wow64 support */
};

static enum process_kind get_process_kind_internal(HANDLE process)
{
    USHORT m1, m2;

    if (!pIsWow64Process2) /* only happens on old Win 8 and early win 1064v1507 */
    {
        BOOL is_wow64;

        if (!strcmp(winetest_platform, "wine") || !IsWow64Process(process, &is_wow64))
            return PCSKIND_ERROR;
        if (is_wow64) return PCSKIND_WOW64;
        return is_win64 ? PCSKIND_64BIT : PCSKIND_32BIT;
    }
    if (!pIsWow64Process2(process, &m1, &m2)) return PCSKIND_ERROR;
    if (m1 == IMAGE_FILE_MACHINE_UNKNOWN && get_machine_bitness(m2) == 32) return PCSKIND_32BIT;
    if (m1 == IMAGE_FILE_MACHINE_UNKNOWN && get_machine_bitness(m2) == 64) return PCSKIND_64BIT;
    if (get_machine_bitness(m1) == 32 && get_machine_bitness(m2) == 64)
    {
        enum process_kind pcskind = PCSKIND_WOW64;
        if (!strcmp(winetest_platform, "wine"))
        {
            PROCESS_BASIC_INFORMATION pbi;
            PEB32 peb32;
            const char* peb_addr;

            if (NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL))
                return PCSKIND_ERROR;

            peb_addr = (const char*)pbi.PebBaseAddress;
            if (is_win64) peb_addr += 0x1000;
            if (!ReadProcessMemory(process, peb_addr, &peb32, sizeof(peb32), NULL)) return PCSKIND_ERROR;
            if (*(const DWORD*)((const char*)&peb32 + 0x460 /* CloudFileFlags */))
                pcskind = PCSKIND_WINE_OLD_WOW64;
        }
        return pcskind;
    }
    return PCSKIND_ERROR;
}

static enum process_kind get_process_kind(HANDLE process)
{
    DWORD gle = GetLastError();
    enum process_kind pcskind = get_process_kind_internal(process);
    SetLastError(gle);
    return pcskind;
}

static const char* process_kind2string(enum process_kind pcskind)
{
    static const char* str[] = {"error", "64bit", "32bit", "wine_old_wow64", "wow64"};
    return (pcskind >= ARRAY_SIZE(str)) ? str[0] : str[pcskind];
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
    BOOL ret, wow64;

    memset(&im, 0, sizeof(im));
    im.SizeOfStruct = sizeof(im);

    ret = SymGetModuleInfoW64(aggregation->proc, base, &im);
    if (ret)
        ok(aggregation->count_exe && ends_withW(imagename, L".exe"),
           "%ls shouldn't already be loaded\n", imagename);
    else
    {
        ok(!ret, "Module %ls shouldn't be loaded\n", imagename);
        ret = SymLoadModuleExW(aggregation->proc, NULL, imagename, NULL, base, sz, NULL, 0);
        ok(ret || broken(GetLastError() == ERROR_SUCCESS) /* Win10/64 v1607 return this on bcryptPrimitives.DLL */,
           "SymLoadModuleExW failed on %ls: %lu\n", imagename, GetLastError());
        ret = SymGetModuleInfoW64(aggregation->proc, base, &im);
        ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
    }

    switch (get_machine_bitness(im.MachineType))
    {
    case 32: aggregation->count_32bit++; break;
    case 64: aggregation->count_64bit++; break;
    default: break;
    }
    if (ends_withW(imagename, L".exe"))
        aggregation->count_exe++;
    if (!wcsicmp(im.ModuleName, L"ntdll"))
        aggregation->count_ntdll++;
    if (!wcsnicmp(imagename, system_directory, wcslen(system_directory)))
        aggregation->count_systemdir++;
    if (IsWow64Process(aggregation->proc, &wow64) && wow64 &&
        !wcsnicmp(imagename, wow64_directory, wcslen(wow64_directory)))
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
    enum process_kind pcskind;

    ret = GetSystemDirectoryA(buffer, sizeof(buffer));
    ok(ret, "got error %lu\n", GetLastError());
    strcat(buffer, "\\msinfo32.exe");

    /* testing invalid process handle */
    ret = wrapper_EnumerateLoadedModulesW64((HANDLE)(ULONG_PTR)0xffffffc0, NULL, FALSE);
    ok(!ret, "EnumerateLoadedModulesW64 should have failed\n");
    ok(GetLastError() == STATUS_INVALID_CID, "Unexpected last error %lx\n", GetLastError());

    /* testing with child process of different machines */
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed: %lu\n", GetLastError());

    ret = wait_process_window_visible(pi.hProcess, pi.dwProcessId, 5000);
    ok(ret, "wait timed out\n");

    ret = SymInitialize(pi.hProcess, NULL, FALSE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());
    memset(&aggregation, 0, sizeof(aggregation));
    aggregation.proc = pi.hProcess;

    ret = wrapper_EnumerateLoadedModulesW64(pi.hProcess, aggregate_cb, &aggregation);
    ok(ret, "EnumerateLoadedModulesW64 failed: %lu\n", GetLastError());

    pcskind = get_process_kind(pi.hProcess);
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
        switch (pcskind)
        {
        case PCSKIND_ERROR:
            ok(0, "Unknown process kind\n");
            break;
        case PCSKIND_64BIT:
        case PCSKIND_WOW64:
            todo_wine
            ok(aggregation.count_systemdir > 2 && aggregation.count_wowdir == 1, "Wrong directory aggregation count %u %u\n",
               aggregation.count_systemdir, aggregation.count_wowdir);
            break;
        case PCSKIND_32BIT:
            ok(aggregation.count_systemdir > 2 && aggregation.count_wowdir == 0, "Wrong directory aggregation count %u %u\n",
               aggregation.count_systemdir, aggregation.count_wowdir);
            break;
        case PCSKIND_WINE_OLD_WOW64:
            todo_wine
            ok(aggregation.count_systemdir == 1 && aggregation.count_wowdir > 2, "Wrong directory aggregation count %u %u\n",
               aggregation.count_systemdir, aggregation.count_wowdir);
            break;
        }
    }

    pcskind = get_process_kind(pi.hProcess);

    ret = wrapper_SymRefreshModuleList(pi.hProcess);
    ok(ret || broken(GetLastError() == STATUS_PARTIAL_COPY /* Win11 in some cases */ ||
                             GetLastError() == STATUS_INFO_LENGTH_MISMATCH /* Win11 in some cases */),
       "SymRefreshModuleList failed: %lx\n", GetLastError());

    if (!strcmp(winetest_platform, "wine"))
    {
        unsigned count = get_native_module_count(pi.hProcess);
        todo_wine_if(pcskind == PCSKIND_WOW64)
        ok(count > 0, "Didn't find any native (ELF/Macho) modules\n");
    }

    SymCleanup(pi.hProcess);
    TerminateProcess(pi.hProcess, 0);

    if (is_win64)
    {
        ret = GetSystemWow64DirectoryA(buffer, sizeof(buffer));
        ok(ret, "got error %lu\n", GetLastError());
        strcat(buffer, "\\msinfo32.exe");

        SymSetOptions(SymGetOptions() & ~SYMOPT_INCLUDE_32BIT_MODULES);

        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ret)
        {
            ret = wait_process_window_visible(pi.hProcess, pi.dwProcessId, 5000);
            ok(ret, "wait timed out\n");

            ret = SymInitialize(pi.hProcess, NULL, FALSE);
            ok(ret, "SymInitialize failed: %lu\n", GetLastError());
            memset(&aggregation, 0, sizeof(aggregation));
            aggregation.proc = pi.hProcess;

            ret = wrapper_EnumerateLoadedModulesW64(pi.hProcess, aggregate_cb, &aggregation);
            ok(ret, "EnumerateLoadedModulesW64 failed: %lu\n", GetLastError());

            pcskind = get_process_kind(pi.hProcess);
            switch (pcskind)
            {
            default:
                ok(0, "Unknown process kind\n");
                break;
            case PCSKIND_WINE_OLD_WOW64:
                ok(aggregation.count_32bit == 1 && !aggregation.count_64bit, "Wrong bitness aggregation count %u %u\n",
                   aggregation.count_32bit, aggregation.count_64bit);
                ok(aggregation.count_exe == 1 && aggregation.count_ntdll == 0, "Wrong kind aggregation count %u %u\n",
                   aggregation.count_exe, aggregation.count_ntdll);
                ok(aggregation.count_systemdir == 0 && aggregation.count_wowdir == 1,
                   "Wrong directory aggregation count %u %u\n",
                   aggregation.count_systemdir, aggregation.count_wowdir);
                break;
            case PCSKIND_WOW64:
                ok(aggregation.count_32bit == 1 && aggregation.count_64bit, "Wrong bitness aggregation count %u %u\n",
                   aggregation.count_32bit, aggregation.count_64bit);
                ok(aggregation.count_exe == 1 && aggregation.count_ntdll == 1, "Wrong kind aggregation count %u %u\n",
                   aggregation.count_exe, aggregation.count_ntdll);
                ok(aggregation.count_systemdir > 2 && aggregation.count_64bit == aggregation.count_systemdir && aggregation.count_wowdir == 1,
                   "Wrong directory aggregation count %u %u\n",
                   aggregation.count_systemdir, aggregation.count_wowdir);
            }
            ret = wrapper_SymRefreshModuleList(pi.hProcess);
            ok(ret, "SymRefreshModuleList failed: %lx\n", GetLastError());

            if (!strcmp(winetest_platform, "wine"))
            {
                unsigned count = get_native_module_count(pi.hProcess);
                ok(count > 0, "Didn't find any native (ELF/Macho) modules\n");
            }

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
            ret = wrapper_EnumerateLoadedModulesW64(pi.hProcess, aggregate_cb, &aggregation2);
            ok(ret, "EnumerateLoadedModulesW64 failed: %lu\n", GetLastError());

            pcskind = get_process_kind(pi.hProcess);
            switch (pcskind)
            {
            case PCSKIND_ERROR:
                break;
            case PCSKIND_WINE_OLD_WOW64:
                ok(aggregation2.count_32bit && !aggregation2.count_64bit, "Wrong bitness aggregation count %u %u\n",
                   aggregation2.count_32bit, aggregation2.count_64bit);
                ok(aggregation2.count_exe == 2 && aggregation2.count_ntdll == 1, "Wrong kind aggregation count %u %u\n",
                   aggregation2.count_exe, aggregation2.count_ntdll);
                ok(aggregation2.count_systemdir == 0 && aggregation2.count_32bit == aggregation2.count_wowdir + 1 && aggregation2.count_wowdir > 2,
                   "Wrong directory aggregation count %u %u\n",
                   aggregation2.count_systemdir, aggregation2.count_wowdir);
                break;
            default:
                ok(aggregation2.count_32bit && aggregation2.count_64bit, "Wrong bitness aggregation count %u %u\n",
                   aggregation2.count_32bit, aggregation2.count_64bit);
                ok(aggregation2.count_exe == 2 && aggregation2.count_ntdll == 2, "Wrong kind aggregation count %u %u\n",
                   aggregation2.count_exe, aggregation2.count_ntdll);
                ok(aggregation2.count_systemdir > 2 && aggregation2.count_64bit == aggregation2.count_systemdir && aggregation2.count_wowdir > 2,
                   "Wrong directory aggregation count %u %u\n",
                   aggregation2.count_systemdir, aggregation2.count_wowdir);
                break;
            }

            ret = wrapper_SymRefreshModuleList(pi.hProcess);
            ok(ret, "SymRefreshModuleList failed: %lx\n", GetLastError());

            if (!strcmp(winetest_platform, "wine"))
            {
                unsigned count = get_native_module_count(pi.hProcess);
                ok(count > 0, "Didn't find any native (ELF/Macho) modules\n");
            }

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

static USHORT pcs_get_loaded_module_machine(HANDLE hProc, DWORD64 base)
{
    IMAGE_DOS_HEADER    dos;
    DWORD               signature;
    IMAGE_FILE_HEADER   fh;
    BOOL                ret;

    ret = ReadProcessMemory(hProc, (char*)(DWORD_PTR)base, &dos, sizeof(dos), NULL);
    ok(ret, "ReadProcessMemory failed: %lu\n", GetLastError());
    ok(dos.e_magic == IMAGE_DOS_SIGNATURE, "Unexpected signature %x\n", dos.e_magic);
    ret = ReadProcessMemory(hProc, (char*)(DWORD_PTR)(base + dos.e_lfanew), &signature, sizeof(signature), NULL);
    ok(ret, "ReadProcessMemory failed: %lu\n", GetLastError());
    ok(signature == IMAGE_NT_SIGNATURE, "Unexpected signature %lx\n", signature);
    ret = ReadProcessMemory(hProc, (char*)(DWORD_PTR)(base + dos.e_lfanew + sizeof(signature) ), &fh, sizeof(fh), NULL);
    ok(ret, "ReadProcessMemory failed: %lu\n", GetLastError());
    return fh.Machine;
}

static void aggregation_push(struct loaded_module_aggregation* aggregation, DWORD64 base, const WCHAR* name, USHORT machine)
{
    BOOL wow64;

    switch (get_machine_bitness(machine))
    {
    case 32: aggregation->count_32bit++; break;
    case 64: aggregation->count_64bit++; break;
    default: break;
    }
    if (ends_withW(name, L".exe"))
        aggregation->count_exe++;
    if (ends_withW(name, L"ntdll.dll"))
        aggregation->count_ntdll++;
    if (!wcsnicmp(name, system_directory, wcslen(system_directory)))
        aggregation->count_systemdir++;
    if (IsWow64Process(aggregation->proc, &wow64) && wow64 &&
        !wcsnicmp(name, wow64_directory, wcslen(wow64_directory)))
        aggregation->count_wowdir++;
}

static BOOL CALLBACK aggregation_sym_cb(const WCHAR* name, DWORD64 base, void* usr)
{
    struct loaded_module_aggregation*   aggregation = usr;
    IMAGEHLP_MODULEW64                  module;
    BOOL                                ret;

    module.SizeOfStruct = sizeof(module);
    ret = SymGetModuleInfoW64(aggregation->proc, base, &module);
    ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());

    aggregation_push(aggregation, base, module.LoadedImageName, module.MachineType);

    return TRUE;
}

static BOOL CALLBACK aggregate_enum_cb(PCWSTR imagename, DWORD64 base, ULONG sz, PVOID usr)
{
    struct loaded_module_aggregation*   aggregation = usr;

    aggregation_push(aggregation, base, imagename, pcs_get_loaded_module_machine(aggregation->proc, base));

    return TRUE;
}

static BOOL pcs_fetch_module_name(HANDLE proc, void* str_addr, void* module_base, WCHAR* buffer, unsigned bufsize, BOOL unicode)
{
    BOOL ret = FALSE;

    if (str_addr)
    {
        void* tgt = NULL;
        SIZE_T addr_size;
        SIZE_T r;
        BOOL is_wow64;

        ret = IsWow64Process(proc, &is_wow64);
        ok(ret, "IsWow64Process failed: %lu\n", GetLastError());
        addr_size = is_win64 && is_wow64 ? 4 : sizeof(void*);

        /* note: string is not always present */
        /* assuming little endian CPU */
        ret = ReadProcessMemory(proc, str_addr, &tgt, addr_size, &r) && r == addr_size;
        if (ret)
        {
            if (unicode)
            {
                ret = ReadProcessMemory(proc, tgt, buffer, bufsize, &r);
                if (ret)
                    buffer[r / sizeof(WCHAR)] = '\0';
                /* Win11 starts exposing ntdll from the string indirection in DLL load event.
                 * So we won't fall back to the mapping's filename below, good!
                 * But the sent DLL name for the 32bit ntdll is now "ntdll32.dll" instead of "ntdll.dll".
                 * Replace it by "ntdll.dll" so we won't have to worry about it in the rest of the code.
                 */
                if (broken(!wcscmp(buffer, L"ntdll32.dll")))
                    wcscpy(buffer, L"ntdll.dll");
            }
            else
            {
                char *tmp = malloc(bufsize);
                if (tmp)
                {
                    ret = ReadProcessMemory(proc, tgt, tmp, bufsize, &r);
                    if (ret)
                    {
                        if (!r) tmp[0] = '\0';
                        else if (!memchr(tmp, '\0', r)) tmp[r - 1] = '\0';
                        MultiByteToWideChar(CP_ACP, 0, tmp, -1, buffer, bufsize);
                        buffer[bufsize - 1] = '\0';
                    }
                    free(tmp);
                }
                else ret = FALSE;
            }
        }
    }
    if (!ret)
    {
        WCHAR tmp[128];
        WCHAR drv[3] = {L'A', L':', L'\0'};

        ret = GetMappedFileNameW(proc, module_base, buffer, bufsize);
        /* Win8 returns this error */
        if (!broken(!ret && GetLastError() == ERROR_FILE_INVALID))
        {
            ok(ret, "GetMappedFileNameW failed: %lu\n", GetLastError());
            if (!wcsncmp(buffer, L"\\??\\", 4))
                memmove(buffer, buffer + 4, (wcslen(buffer) + 1 - 4) * sizeof(WCHAR));
            while (drv[0] <= L'Z')
            {
                if (QueryDosDeviceW(drv, tmp, ARRAY_SIZE(tmp)))
                {
                    size_t len = wcslen(tmp);
                    if (len >= 2 && !wcsnicmp(buffer, tmp, len))
                    {
                        memmove(buffer + 2, buffer + len, (wcslen(buffer) + 1 - len) * sizeof(WCHAR));
                        buffer[0] = drv[0];
                        buffer[1] = drv[1];
                        break;
                    }
                }
                drv[0]++;
            }
        }
    }
    return ret;
}

static void test_live_modules_proc(WCHAR* exename, BOOL with_32)
{
    WCHAR buffer[MAX_PATH];
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOW si = {0};
    DEBUG_EVENT de;
    BOOL ret;
    BOOL is_wow64 = FALSE;
    struct loaded_module_aggregation aggregation_event = {};
    struct loaded_module_aggregation aggregation_enum = {};
    struct loaded_module_aggregation aggregation_sym = {};
    DWORD old_options = SymGetOptions();
    enum process_kind pcskind;

    ret = CreateProcessW(NULL, exename, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    if (!ret)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            skip("Skip wow64 test on non compatible platform\n");
        else
            ok(ret, "CreateProcess failed: %lu\n", GetLastError());
        return;
    }

    ret = IsWow64Process(pi.hProcess, &is_wow64);
    ok(ret, "IsWow64Process failed: %lu\n", GetLastError());
    pcskind = get_process_kind(pi.hProcess);
    ok(pcskind != PCSKIND_ERROR, "Unexpected error\n");

    winetest_push_context("[%u/%u enum:%s %s]",
                          is_win64 ? 64 : 32, is_wow64 ? 32 : 64, with_32 ? "+32bit" : "default",
                          process_kind2string(pcskind));

    memset(&aggregation_event, 0, sizeof(aggregation_event));
    aggregation_event.proc = pi.hProcess;
    while (WaitForDebugEvent(&de, 2000))
    {
        switch (de.dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            if (pcs_fetch_module_name(pi.hProcess, de.u.CreateProcessInfo.lpImageName, de.u.CreateProcessInfo.lpBaseOfImage,
                                      buffer, ARRAY_SIZE(buffer), de.u.CreateProcessInfo.fUnicode))
                aggregation_push(&aggregation_event, (ULONG_PTR)de.u.CreateProcessInfo.lpBaseOfImage, buffer,
                                 pcs_get_loaded_module_machine(pi.hProcess, (ULONG_PTR)de.u.CreateProcessInfo.lpBaseOfImage));
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (pcs_fetch_module_name(pi.hProcess, de.u.LoadDll.lpImageName, de.u.LoadDll.lpBaseOfDll, buffer, ARRAY_SIZE(buffer), de.u.LoadDll.fUnicode))
                aggregation_push(&aggregation_event, (ULONG_PTR)de.u.LoadDll.lpBaseOfDll, buffer,
                                 pcs_get_loaded_module_machine(pi.hProcess, (ULONG_PTR)de.u.LoadDll.lpBaseOfDll));
            break;
        case UNLOAD_DLL_DEBUG_EVENT:
            /* FIXME: we should take of updating aggregation_event; as of today, doesn't trigger issue */
            break;
        case EXCEPTION_DEBUG_EVENT:
            break;
        }
        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed: %lu\n", GetLastError());
    }

    if (with_32)
        SymSetOptions(old_options | SYMOPT_INCLUDE_32BIT_MODULES);
    else
        SymSetOptions(old_options & ~SYMOPT_INCLUDE_32BIT_MODULES);

    memset(&aggregation_enum, 0, sizeof(aggregation_enum));
    aggregation_enum.proc = pi.hProcess;
    ret = wrapper_EnumerateLoadedModulesW64(pi.hProcess, aggregate_enum_cb, &aggregation_enum);
    ok(ret, "SymEnumerateModulesW64 failed: %lu\n", GetLastError());

    ret = SymInitialize(pi.hProcess, NULL, TRUE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());

/* .exe ntdll kernel32 kernelbase user32 gdi32 (all are in system or wow64 directory) */
#define MODCOUNT 6
/* when in wow64: wow64 wow64cpu wow64win ntdll (64bit) */
#define MODWOWCOUNT 4
/* .exe is reported twice in enum */
#define XTRAEXE (!!with_32)
/* ntdll is reported twice in enum */
#define XTRANTDLL (is_win64 && is_wow64 && (!!with_32))

    memset(&aggregation_sym, 0, sizeof(aggregation_sym));
    aggregation_sym.proc = pi.hProcess;
    ret = SymEnumerateModulesW64(pi.hProcess, aggregation_sym_cb, &aggregation_sym);
    ok(ret, "SymEnumerateModulesW64 failed: %lu\n", GetLastError());

    if (pcskind == PCSKIND_64BIT) /* 64/64 */
    {
        ok(is_win64, "How come?\n");
        ok(aggregation_event.count_exe == 1,                    "Unexpected event.count_exe %u\n",       aggregation_event.count_exe);
        ok(aggregation_event.count_32bit == 0,                  "Unexpected event.count_32bit %u\n",     aggregation_event.count_32bit);
        ok(aggregation_event.count_64bit >= MODCOUNT,           "Unexpected event.count_64bit %u\n",     aggregation_event.count_64bit);
        ok(aggregation_event.count_systemdir >= MODCOUNT,       "Unexpected event.count_systemdir %u\n", aggregation_event.count_systemdir);
        ok(aggregation_event.count_wowdir == 0,                 "Unexpected event.count_wowdir %u\n",    aggregation_event.count_wowdir);
        ok(aggregation_event.count_ntdll == 1,                  "Unexpected event.count_ntdll %u\n",     aggregation_event.count_ntdll);

        ok(aggregation_enum.count_exe == 1,                     "Unexpected enum.count_exe %u\n",        aggregation_enum.count_exe);
        ok(aggregation_enum.count_32bit == 0,                   "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        ok(aggregation_enum.count_64bit >= MODCOUNT,            "Unexpected enum.count_64bit %u\n",      aggregation_enum.count_64bit);
        ok(aggregation_enum.count_systemdir >= MODCOUNT,        "Unexpected enum.count_systemdir %u\n",  aggregation_enum.count_systemdir);
        ok(aggregation_enum.count_wowdir == 0,                  "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        ok(aggregation_enum.count_ntdll == 1,                   "Unexpected enum.count_ntdll %u\n",      aggregation_enum.count_ntdll);

        ok(aggregation_sym.count_exe == 1,                      "Unexpected sym.count_exe %u\n",         aggregation_sym.count_exe);
        ok(aggregation_sym.count_32bit == 0,                    "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        ok(aggregation_sym.count_64bit >= MODCOUNT,             "Unexpected sym.count_64bit %u\n",       aggregation_sym.count_64bit);
        ok(aggregation_sym.count_systemdir >= MODCOUNT,         "Unexpected sym.count_systemdir %u\n",   aggregation_sym.count_systemdir);
        ok(aggregation_sym.count_wowdir == 0,                   "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        ok(aggregation_sym.count_ntdll == 1,                    "Unexpected sym.count_ntdll %u\n",       aggregation_sym.count_ntdll);
    }
    else if (pcskind == PCSKIND_32BIT) /* 32/32 */
    {
        ok(!is_win64, "How come?\n");
        ok(aggregation_event.count_exe == 1,                    "Unexpected event.count_exe %u\n",       aggregation_event.count_exe);
        ok(aggregation_event.count_32bit >= MODCOUNT,           "Unexpected event.count_32bit %u\n",     aggregation_event.count_32bit);
        ok(aggregation_event.count_64bit == 0,                  "Unexpected event.count_64bit %u\n",     aggregation_event.count_64bit);
        ok(aggregation_event.count_systemdir >= MODCOUNT,       "Unexpected event.count_systemdir %u\n", aggregation_event.count_systemdir);
        ok(aggregation_event.count_wowdir == 0,                 "Unexpected event.count_wowdir %u\n",    aggregation_event.count_wowdir);
        ok(aggregation_event.count_ntdll == 1,                  "Unexpected event.count_ntdll %u\n",     aggregation_event.count_ntdll);

        ok(aggregation_enum.count_exe == 1,                     "Unexpected enum.count_exe %u\n",        aggregation_enum.count_exe);
        ok(aggregation_enum.count_32bit >= MODCOUNT,            "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        ok(aggregation_enum.count_64bit == 0,                   "Unexpected enum.count_64bit %u\n",      aggregation_enum.count_64bit);
        ok(aggregation_enum.count_systemdir >= MODCOUNT,        "Unexpected enum.count_systemdir %u\n",  aggregation_enum.count_systemdir);
        ok(aggregation_enum.count_wowdir == 0,                  "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        ok(aggregation_enum.count_ntdll == 1,                   "Unexpected enum.count_ntdll %u\n",      aggregation_enum.count_ntdll);

        ok(aggregation_sym.count_exe == 1,                      "Unexpected sym.count_exe %u\n",         aggregation_sym.count_exe);
        ok(aggregation_sym.count_32bit >= MODCOUNT,             "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        ok(aggregation_sym.count_64bit == 0,                    "Unexpected sym.count_64bit %u\n",       aggregation_sym.count_64bit);
        ok(aggregation_sym.count_systemdir >= MODCOUNT,         "Unexpected sym.count_systemdir %u\n",   aggregation_sym.count_systemdir);
        ok(aggregation_sym.count_wowdir == 0,                   "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        ok(aggregation_sym.count_ntdll == 1,                    "Unexpected sym.count_ntdll %u\n",       aggregation_sym.count_ntdll);
    }
    else if (is_win64 && pcskind == PCSKIND_WOW64) /* 64/32 */
    {
        ok(aggregation_event.count_exe == 1,                    "Unexpected event.count_exe %u\n",       aggregation_event.count_exe);
        ok(aggregation_event.count_32bit >= MODCOUNT,           "Unexpected event.count_32bit %u\n",     aggregation_event.count_32bit);
        ok(aggregation_event.count_64bit >= MODWOWCOUNT,        "Unexpected event.count_64bit %u\n",     aggregation_event.count_64bit);
        ok(aggregation_event.count_systemdir >= MODWOWCOUNT,    "Unexpected event.count_systemdir %u\n", aggregation_event.count_systemdir);
        ok(aggregation_event.count_wowdir >= MODCOUNT,          "Unexpected event.count_wowdir %u\n",    aggregation_event.count_wowdir);
        ok(aggregation_event.count_ntdll == 2 || broken(aggregation_event.count_ntdll == 3),
                                                 /* yes! with Win 864 and Win1064 v 1507... 2 ntdll:s ain't enuff <g>, 3 are reported! */
                                                 /* in fact the first ntdll is reported twice (at same address) in two consecutive events */
                                                                "Unexpected event.count_ntdll %u\n",     aggregation_event.count_ntdll);

        ok(aggregation_enum.count_exe == 1 + XTRAEXE,           "Unexpected enum.count_exe %u\n",        aggregation_enum.count_exe);
        if (with_32)
            ok(aggregation_enum.count_32bit >= MODCOUNT,        "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        else
            ok(aggregation_enum.count_32bit == 1,               "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        ok(aggregation_enum.count_64bit >= MODWOWCOUNT,         "Unexpected enum.count_64bit %u\n",      aggregation_enum.count_64bit);
        ok(aggregation_enum.count_systemdir >= MODWOWCOUNT,     "Unexpected enum.count_systemdir %u\n",  aggregation_enum.count_systemdir);
        if (with_32)
            ok(aggregation_enum.count_wowdir >= MODCOUNT,       "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        else
            ok(aggregation_enum.count_wowdir == 1,              "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        ok(aggregation_enum.count_ntdll == 1 + XTRANTDLL,       "Unexpected enum.count_ntdll %u\n",      aggregation_enum.count_ntdll);

        ok(aggregation_sym.count_exe == 1,                      "Unexpected sym.count_exe %u\n",         aggregation_sym.count_exe);
        if (with_32)
            ok(aggregation_sym.count_32bit >= MODCOUNT,         "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        else
            ok(aggregation_sym.count_32bit == 1,                "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        ok(aggregation_sym.count_64bit >= MODWOWCOUNT,          "Unexpected sym.count_64bit %u\n",       aggregation_sym.count_64bit);
        ok(aggregation_sym.count_systemdir >= MODWOWCOUNT,      "Unexpected sym.count_systemdir %u\n",   aggregation_sym.count_systemdir);
        if (with_32)
            ok(aggregation_sym.count_wowdir >= MODCOUNT,        "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        else
            ok(aggregation_sym.count_wowdir == 1,               "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        ok(aggregation_sym.count_ntdll == 1 + XTRANTDLL,        "Unexpected sym.count_ntdll %u\n",       aggregation_sym.count_ntdll);
    }
    else if (!is_win64 && pcskind == PCSKIND_WOW64) /* 32/32 */
    {
        ok(aggregation_event.count_exe == 1,                    "Unexpected event.count_exe %u\n",       aggregation_event.count_exe);
        ok(aggregation_event.count_32bit >= MODCOUNT,           "Unexpected event.count_32bit %u\n",     aggregation_event.count_32bit);
        ok(aggregation_event.count_64bit == 0,                  "Unexpected event.count_64bit %u\n",     aggregation_event.count_64bit);
        todo_wine
        ok(aggregation_event.count_systemdir == 0,              "Unexpected event.count_systemdir %u\n", aggregation_event.count_systemdir);
        ok(aggregation_event.count_wowdir >= MODCOUNT,          "Unexpected event.count_wowdir %u\n",    aggregation_event.count_wowdir);
        ok(aggregation_event.count_ntdll == 1,                  "Unexpected event.count_ntdll %u\n",     aggregation_event.count_ntdll);

        ok(aggregation_enum.count_exe == 1 + XTRAEXE,           "Unexpected enum.count_exe %u\n",        aggregation_enum.count_exe);
        ok(aggregation_enum.count_32bit >= MODCOUNT,            "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        ok(aggregation_enum.count_64bit == 0,                   "Unexpected enum.count_64bit %u\n",      aggregation_enum.count_64bit);
        /* yes that's different from event! */
        ok(aggregation_enum.count_systemdir >= MODCOUNT - 1,    "Unexpected enum.count_systemdir %u\n",  aggregation_enum.count_systemdir);
        /* .exe */
        todo_wine
        ok(aggregation_enum.count_wowdir == 1,                  "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        ok(aggregation_enum.count_ntdll == 1,                   "Unexpected enum.count_ntdll %u\n",      aggregation_enum.count_ntdll);

        ok(aggregation_sym.count_exe == 1,                      "Unexpected sym.count_exe %u\n",         aggregation_sym.count_exe);
        ok(aggregation_sym.count_32bit >= MODCOUNT,             "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        ok(aggregation_sym.count_64bit == 0,                    "Unexpected sym.count_64bit %u\n",       aggregation_sym.count_64bit);
        ok(aggregation_sym.count_systemdir >= MODCOUNT - 1,     "Unexpected sym.count_systemdir %u\n",   aggregation_sym.count_systemdir);
        /* .exe */
        todo_wine
        ok(aggregation_sym.count_wowdir == 1,                   "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        ok(aggregation_sym.count_ntdll == 1 + XTRANTDLL,        "Unexpected sym.count_ntdll %u\n",       aggregation_sym.count_ntdll);
    }
    else if (is_win64 && pcskind == PCSKIND_WINE_OLD_WOW64) /* 64/32 */
    {
        ok(aggregation_event.count_exe == 1,                    "Unexpected event.count_exe %u\n",       aggregation_event.count_exe);
        ok(aggregation_event.count_32bit >= MODCOUNT,           "Unexpected event.count_32bit %u\n",     aggregation_event.count_32bit);
        ok(aggregation_event.count_64bit == 0,                  "Unexpected event.count_64bit %u\n",     aggregation_event.count_64bit);
        ok(aggregation_event.count_systemdir >= 1,              "Unexpected event.count_systemdir %u\n", aggregation_event.count_systemdir);
        ok(aggregation_event.count_wowdir >= MODCOUNT - 1,      "Unexpected event.count_wowdir %u\n",    aggregation_event.count_wowdir);
        ok(aggregation_event.count_ntdll == 1,                  "Unexpected event.count_ntdll %u\n",     aggregation_event.count_ntdll);

        ok(aggregation_enum.count_exe == 1 + XTRAEXE,           "Unexpected enum.count_exe %u\n",        aggregation_enum.count_exe);
        if (with_32)
            ok(aggregation_enum.count_32bit >= MODCOUNT,        "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        else
            ok(aggregation_enum.count_32bit <= 1,               "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        ok(aggregation_enum.count_64bit == 0,                   "Unexpected enum.count_64bit %u\n",      aggregation_enum.count_64bit);
        ok(aggregation_enum.count_systemdir == 0,               "Unexpected enum.count_systemdir %u\n",  aggregation_enum.count_systemdir);
        if (with_32)
            ok(aggregation_enum.count_wowdir >= MODCOUNT,       "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        else
            ok(aggregation_enum.count_wowdir <= 1,              "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        ok(aggregation_enum.count_ntdll == XTRANTDLL,           "Unexpected enum.count_ntdll %u\n",      aggregation_enum.count_ntdll);

        ok(aggregation_sym.count_exe == 1,                      "Unexpected sym.count_exe %u\n",         aggregation_sym.count_exe);
        if (with_32)
            ok(aggregation_sym.count_32bit >= MODCOUNT,         "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        else
            ok(aggregation_sym.count_wowdir <= 1,               "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        ok(aggregation_sym.count_64bit == 0,                    "Unexpected sym.count_64bit %u\n",       aggregation_sym.count_64bit);
        ok(aggregation_sym.count_systemdir == 0,                "Unexpected sym.count_systemdir %u\n",   aggregation_sym.count_systemdir);
        if (with_32)
            ok(aggregation_sym.count_wowdir >= MODCOUNT,        "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        else
            ok(aggregation_sym.count_wowdir <= 1,               "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        ok(aggregation_sym.count_ntdll == XTRANTDLL,            "Unexpected sym.count_ntdll %u\n",       aggregation_sym.count_ntdll);
    }
    else if (!is_win64 && pcskind == PCSKIND_WINE_OLD_WOW64) /* 32/32 */
    {
        ok(aggregation_event.count_exe == 1,                    "Unexpected event.count_exe %u\n",       aggregation_event.count_exe);
        ok(aggregation_event.count_32bit >= MODCOUNT,           "Unexpected event.count_32bit %u\n",     aggregation_event.count_32bit);
        ok(aggregation_event.count_64bit == 0,                  "Unexpected event.count_64bit %u\n",     aggregation_event.count_64bit);
        todo_wine
        ok(aggregation_event.count_systemdir == 1,              "Unexpected event.count_systemdir %u\n", aggregation_event.count_systemdir);
        ok(aggregation_event.count_wowdir >= MODCOUNT,          "Unexpected event.count_wowdir %u\n",    aggregation_event.count_wowdir);
        ok(aggregation_event.count_ntdll == 1,                  "Unexpected event.count_ntdll %u\n",     aggregation_event.count_ntdll);

        ok(aggregation_enum.count_exe == 1 + XTRAEXE,           "Unexpected enum.count_exe %u\n",        aggregation_enum.count_exe);
        ok(aggregation_enum.count_32bit >= MODCOUNT,            "Unexpected enum.count_32bit %u\n",      aggregation_enum.count_32bit);
        ok(aggregation_enum.count_64bit == 0,                   "Unexpected enum.count_64bit %u\n",      aggregation_enum.count_64bit);
        ok(aggregation_enum.count_systemdir >= MODCOUNT,        "Unexpected enum.count_systemdir %u\n",  aggregation_enum.count_systemdir);
        /* .exe */
        todo_wine
        ok(aggregation_enum.count_wowdir == 1,                  "Unexpected enum.count_wowdir %u\n",     aggregation_enum.count_wowdir);
        ok(aggregation_enum.count_ntdll == 1 + XTRANTDLL,       "Unexpected enum.count_ntdll %u\n",      aggregation_enum.count_ntdll);

        ok(aggregation_sym.count_exe == 1,                      "Unexpected sym.count_exe %u\n",         aggregation_sym.count_exe);
        ok(aggregation_sym.count_32bit >= MODCOUNT,             "Unexpected sym.count_32bit %u\n",       aggregation_sym.count_32bit);
        ok(aggregation_sym.count_64bit == 0,                    "Unexpected sym.count_64bit %u\n",       aggregation_sym.count_64bit);
        ok(aggregation_sym.count_systemdir >= MODCOUNT,         "Unexpected sym.count_systemdir %u\n",   aggregation_sym.count_systemdir);
        /* .exe */
        todo_wine
        ok(aggregation_sym.count_wowdir == 1,                   "Unexpected sym.count_wowdir %u\n",      aggregation_sym.count_wowdir);
        ok(aggregation_sym.count_ntdll == 1 + XTRANTDLL,        "Unexpected sym.count_ntdll %u\n",       aggregation_sym.count_ntdll);
    }
    else
        ok(0, "Unexpected process kind %u\n", pcskind);

    /* main module is enumerated twice in enum when including 32bit modules */
    ok(aggregation_sym.count_32bit + XTRAEXE == aggregation_enum.count_32bit, "Different sym/enum count32_bit (%u/%u)\n",
       aggregation_sym.count_32bit, aggregation_enum.count_32bit);
    ok(aggregation_sym.count_64bit == aggregation_enum.count_64bit, "Different sym/enum count64_bit (%u/%u)\n",
       aggregation_sym.count_64bit, aggregation_enum.count_64bit);
    ok(aggregation_sym.count_systemdir == aggregation_enum.count_systemdir, "Different sym/enum systemdir (%u/%u)\n",
       aggregation_sym.count_systemdir, aggregation_enum.count_systemdir);
    ok(aggregation_sym.count_wowdir + XTRAEXE == aggregation_enum.count_wowdir, "Different sym/enum wowdir (%u/%u)\n",
       aggregation_sym.count_wowdir, aggregation_enum.count_wowdir);
    ok(aggregation_sym.count_exe + XTRAEXE == aggregation_enum.count_exe, "Different sym/enum exe (%u/%u)\n",
       aggregation_sym.count_exe, aggregation_enum.count_exe);
    ok(aggregation_sym.count_ntdll == aggregation_enum.count_ntdll, "Different sym/enum exe (%u/%u)\n",
       aggregation_sym.count_ntdll, aggregation_enum.count_ntdll);

#undef MODCOUNT
#undef MODWOWCOUNT
#undef XTRAEXE
#undef XTRANTDLL

    TerminateProcess(pi.hProcess, 0);

    winetest_pop_context();
    SymSetOptions(old_options);
}

static void test_live_modules(void)
{
    WCHAR buffer[200];

    wcscpy(buffer, system_directory);
    wcscat(buffer, L"\\msinfo32.exe");

    test_live_modules_proc(buffer, FALSE);

    if (is_win64)
    {
        wcscpy(buffer, wow64_directory);
        wcscat(buffer, L"\\msinfo32.exe");

        test_live_modules_proc(buffer, TRUE);
        test_live_modules_proc(buffer, FALSE);
    }
}

#define test_function_table_main_module(b) _test_function_table_entry(__LINE__, NULL, #b, (DWORD64)(DWORD_PTR)&(b))
#define test_function_table_module(a, b)   _test_function_table_entry(__LINE__, a,    #b, (DWORD64)(DWORD_PTR)GetProcAddress(GetModuleHandleA(a), (b)))
static void *_test_function_table_entry(unsigned lineno, const char *modulename, const char *name, DWORD64 addr)
{
    DWORD64 base_module = (DWORD64)(DWORD_PTR)GetModuleHandleA(modulename);

    if (RtlImageNtHeader(GetModuleHandleW(NULL))->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
    {
        IMAGE_AMD64_RUNTIME_FUNCTION_ENTRY *func;

        func = SymFunctionTableAccess64(GetCurrentProcess(), addr);
        ok_(__FILE__, lineno)(func != NULL, "Couldn't find function table for %s\n", name);
        if (func)
        {
            ok_(__FILE__, lineno)(func->BeginAddress == addr - base_module, "Unexpected start of function\n");
            ok_(__FILE__, lineno)(func->BeginAddress < func->EndAddress, "Unexpected end of function\n");
            ok_(__FILE__, lineno)((func->UnwindData & 1) == 0, "Unexpected chained runtime function\n");
        }

        return func;
    }
    return NULL;
}

static void test_function_tables(void)
{
    void *ptr1, *ptr2;

    SymInitialize(GetCurrentProcess(), NULL, TRUE);
    ptr1 = test_function_table_main_module(test_live_modules);
    ptr2 = test_function_table_main_module(test_function_tables);
    ok(ptr1 == ptr2, "Expecting unique storage area\n");
    ptr2 = test_function_table_module("kernel32.dll", "CreateFileMappingA");
    ok(ptr1 == ptr2, "Expecting unique storage area\n");
    SymCleanup(GetCurrentProcess());
}

static void test_refresh_modules(void)
{
    BOOL ret;
    unsigned count, count_current;
    HMODULE hmod;
    IMAGEHLP_MODULEW64 module_info = { .SizeOfStruct = sizeof(module_info) };

    /* pick a DLL: which isn't already loaded by test, and that will not load other DLLs for deps */
    static const WCHAR* unused_dll = L"psapi";

    ret = SymInitialize(GetCurrentProcess(), 0, TRUE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());

    count = get_module_count(GetCurrentProcess());
    ok(count, "Unexpected module count %u\n", count);

    ret = SymCleanup(GetCurrentProcess());
    ok(ret, "SymCleanup failed: %lu\n", GetLastError());

    ret = SymInitialize(GetCurrentProcess(), 0, FALSE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());

    count_current = get_module_count(GetCurrentProcess());
    ok(!count_current, "Unexpected module count %u\n", count_current);

    ret = wrapper_SymRefreshModuleList(GetCurrentProcess());
    ok(ret, "SymRefreshModuleList failed: %lx\n", GetLastError());

    count_current = get_module_count(GetCurrentProcess());
    ok(count == count_current, "Unexpected module count %u, %u\n", count, count_current);

    hmod = GetModuleHandleW(unused_dll);
    ok(hmod == NULL, "Expecting DLL %ls not be loaded\n", unused_dll);

    hmod = LoadLibraryW(unused_dll);
    ok(hmod != NULL, "LoadLibraryW failed: %lu\n", GetLastError());

    count_current = get_module_count(GetCurrentProcess());
    ok(count == count_current, "Unexpected module count %u, %u\n", count, count_current);
    ret = is_module_present(GetCurrentProcess(), unused_dll);
    ok(!ret, "Couldn't find module %ls\n", unused_dll);

    ret = wrapper_SymRefreshModuleList(GetCurrentProcess());
    ok(ret, "SymRefreshModuleList failed: %lx\n", GetLastError());

    count_current = get_module_count(GetCurrentProcess());
    ok(count + 1 == count_current, "Unexpected module count %u, %u\n", count, count_current);
    ret = is_module_present(GetCurrentProcess(), unused_dll);
    ok(ret, "Couldn't find module %ls\n", unused_dll);

    ret = FreeLibrary(hmod);
    ok(ret, "LoadLibraryW failed: %lu\n", GetLastError());

    count_current = get_module_count(GetCurrentProcess());
    ok(count + 1 == count_current, "Unexpected module count %u, %u\n", count, count_current);

    ret = wrapper_SymRefreshModuleList(GetCurrentProcess());
    ok(ret, "SymRefreshModuleList failed: %lx\n", GetLastError());

    /* SymRefreshModuleList() doesn't remove the unloaded modules... */
    count_current = get_module_count(GetCurrentProcess());
    ok(count + 1 == count_current, "Unexpected module count %u != %u\n", count, count_current);
    ret = is_module_present(GetCurrentProcess(), unused_dll);
    ok(ret, "Couldn't find module %ls\n", unused_dll);

    ret = SymCleanup(GetCurrentProcess());
    ok(ret, "SymCleanup failed: %lu\n", GetLastError());
}

START_TEST(dbghelp)
{
    BOOL ret;

    /* Don't let the user's environment influence our symbol path */
    SetEnvironmentVariableA("_NT_SYMBOL_PATH", NULL);
    SetEnvironmentVariableA("_NT_ALT_SYMBOL_PATH", NULL);

    pIsWow64Process2 = (void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process2");

    ret = SymInitialize(GetCurrentProcess(), NULL, TRUE);
    ok(ret, "got error %lu\n", GetLastError());

    test_stack_walk();
    test_search_path();

    ret = SymCleanup(GetCurrentProcess());
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetSystemDirectoryW(system_directory, ARRAY_SIZE(system_directory));
    ok(ret, "GetSystemDirectoryW failed: %lu\n", GetLastError());
    /* failure happens on a 32bit only wine setup */
    if (!GetSystemWow64DirectoryW(wow64_directory, ARRAY_SIZE(wow64_directory)))
        wow64_directory[0] = L'\0';

    if (test_modules())
    {
        test_modules_overlap();
        test_loaded_modules();
        test_live_modules();
        test_refresh_modules();
    }
    test_function_tables();
}
