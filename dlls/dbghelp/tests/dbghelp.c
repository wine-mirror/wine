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

#include "windef.h"
#include "verrsrc.h"
#include "dbghelp.h"
#include "wine/test.h"
#include "winternl.h"

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
    BOOL ret;
    HANDLE hFile, hMap;
    void* mapping;
    IMAGE_NT_HEADERS *nthdr;
    USHORT machine;

    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Couldn't open file %s (%lu)\n", path, GetLastError());
    hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    ok(hMap != NULL, "Couldn't create map (%lu)\n", GetLastError());
    mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    ok(mapping != NULL, "Couldn't map (%lu)\n", GetLastError());
    nthdr = RtlImageNtHeader(mapping);
    ok(nthdr != NULL, "Cannot get NT headers out of %s\n", path);
    machine = nthdr ? nthdr->FileHeader.Machine : IMAGE_FILE_MACHINE_UNKNOWN;
    ret = UnmapViewOfFile(mapping);
    ok(ret, "Couldn't unmap (%lu)\n", GetLastError());
    CloseHandle(hMap);
    CloseHandle(hFile);
    return machine;
}

static void test_modules(void)
{
    BOOL ret;
    DWORD attr;
    const DWORD64 base1 = 0x00010000;
    const DWORD64 base2 = 0x08010000;
    IMAGEHLP_MODULEW64 im;

    im.SizeOfStruct = sizeof(im);

    /* can sym load an exec of different bitness even if 32Bit flag not set */

    SymSetOptions(SymGetOptions() & ~SYMOPT_INCLUDE_32BIT_MODULES);
    ret = SymInitialize(GetCurrentProcess(), 0, FALSE);
    ok(ret, "SymInitialize failed: %lu\n", GetLastError());

    /* not always present */
    attr = GetFileAttributesA("C:\\windows\\syswow64\\notepad.exe");
    if (attr != INVALID_FILE_ATTRIBUTES)
    {
        ret = SymLoadModule(GetCurrentProcess(), NULL, "C:\\windows\\syswow64\\notepad.exe", NULL, base2, 0);
        ok(ret, "SymLoadModule failed: %lu\n", GetLastError());
        ret = SymGetModuleInfoW64(GetCurrentProcess(), base2, &im);
        ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
        ok(im.BaseOfImage == base2, "Wrong base address\n");
        ok(im.MachineType == get_module_machine("C:\\windows\\syswow64\\notepad.exe"),
           "Wrong machine %lx\n", im.MachineType);
    }

    ret = SymLoadModule(GetCurrentProcess(), NULL, "C:\\windows\\system32\\notepad.exe", NULL, base1, 0);
    ok(ret, "SymLoadModule failed: %lu\n", GetLastError());
    ret = SymGetModuleInfoW64(GetCurrentProcess(), base1, &im);
    /* we want to access IMAGEHLP_MODULE.MachineType, so ensure that error stems from a too old
     * dbghelp (on Windows), not supporting new enlarged IMAGEHLP_MODULE structures.
     */
    if (broken(!ret && GetLastError() == ERROR_INVALID_PARAMETER))
    {
        IMAGEHLP_MODULE im0 = {sizeof(im0)};
        ret = SymGetModuleInfo(GetCurrentProcess(), base1, &im0);
        ok(ret, "Unexpected error: %lu\n", GetLastError());
        skip("Too old dbghelp. Skipping module tests\n");
        ret = SymCleanup(GetCurrentProcess());
        ok(ret, "SymCleanup failed: %lu\n", GetLastError());
        return;
    }
    ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
    ok(im.BaseOfImage == base1, "Wrong base address\n");
    ok(im.MachineType == get_module_machine("C:\\windows\\system32\\notepad.exe"),
       "Wrong machine %lx\n", im.MachineType);

    /* still can access first module after loading second */
    if (attr != INVALID_FILE_ATTRIBUTES)
    {
        ret = SymGetModuleInfoW64(GetCurrentProcess(), base2, &im);
        ok(ret, "SymGetModuleInfoW64 failed: %lu\n", GetLastError());
        ok(im.BaseOfImage == base2, "Wrong base address\n");
        ok(im.MachineType == get_module_machine("C:\\windows\\syswow64\\notepad.exe"),
           "Wrong machine %lx\n", im.MachineType);
    }

    ret = SymCleanup(GetCurrentProcess());
    ok(ret, "SymCleanup failed: %lu\n", GetLastError());
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

    test_modules();
}
