/*
 * Unit test suite for PSAPI
 *
 * Copyright (C) 2005 Felix Nawothnig
 * Copyright (C) 2012 Dmitry Timoshkov
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnt.h"
#include "winternl.h"
#include "winnls.h"
#include "winuser.h"
#define PSAPI_VERSION 1
#include "psapi.h"
#include "wine/test.h"

static NTSTATUS (WINAPI *pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtQueryVirtualMemory)(HANDLE, LPCVOID, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, SIZE_T *);
static BOOL  (WINAPI *pIsWow64Process)(HANDLE, BOOL *);
static BOOL  (WINAPI *pWow64DisableWow64FsRedirection)(void **);
static BOOL  (WINAPI *pWow64RevertWow64FsRedirection)(void *);
static BOOL  (WINAPI *pQueryWorkingSetEx)(HANDLE, PVOID, DWORD);

static BOOL wow64;
static char** main_argv;

static BOOL init_func_ptrs(void)
{
    pNtQuerySystemInformation = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    pNtQueryVirtualMemory = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryVirtualMemory");
    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
    pWow64DisableWow64FsRedirection = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64DisableWow64FsRedirection");
    pWow64RevertWow64FsRedirection = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64RevertWow64FsRedirection");
    pQueryWorkingSetEx = (void *)GetProcAddress(GetModuleHandleA("psapi.dll"), "QueryWorkingSetEx");
    return TRUE;
}

static HANDLE hpSR, hpQI, hpVR, hpQV;
static const HANDLE hBad = (HANDLE)0xdeadbeef;

static void test_EnumProcesses(void)
{
    DWORD pid, ret, cbUsed = 0xdeadbeef;

    SetLastError(0xdeadbeef);
    ret = EnumProcesses(NULL, 0, &cbUsed);
    ok(ret == 1, "failed with %ld\n", GetLastError());
    ok(cbUsed == 0, "cbUsed=%ld\n", cbUsed);

    SetLastError(0xdeadbeef);
    ret = EnumProcesses(&pid, 4, &cbUsed);
    ok(ret == 1, "failed with %ld\n", GetLastError());
    ok(cbUsed == 4, "cbUsed=%ld\n", cbUsed);
}

static void test_EnumProcessModules(void)
{
    char buffer[MAX_PATH] = "C:\\windows\\system32\\msinfo32.exe";
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOA si = {0};
    void *cookie;
    HMODULE hMod;
    DWORD ret, cbNeeded = 0xdeadbeef;

    SetLastError(0xdeadbeef);
    EnumProcessModules(NULL, NULL, 0, &cbNeeded);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    EnumProcessModules(hpQI, NULL, 0, &cbNeeded);
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModules(hpQI, &hMod, sizeof(HMODULE), NULL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModules(hpQV, &hMod, sizeof(HMODULE), NULL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %ld\n", GetLastError());
    ok(hMod == GetModuleHandleA(NULL),
       "hMod=%p GetModuleHandleA(NULL)=%p\n", hMod, GetModuleHandleA(NULL));

    SetLastError(0xdeadbeef);
    ret = EnumProcessModules(hpQV, NULL, 0, &cbNeeded);
    ok(ret == 1, "failed with %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumProcessModules(hpQV, NULL, sizeof(HMODULE), &cbNeeded);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModules(hpQV, &hMod, sizeof(HMODULE), &cbNeeded);
    ok(ret == 1, "got %ld, failed with %ld\n", ret, GetLastError());
    ok(hMod == GetModuleHandleA(NULL),
       "hMod=%p GetModuleHandleA(NULL)=%p\n", hMod, GetModuleHandleA(NULL));
    ok(cbNeeded % sizeof(hMod) == 0, "not a multiple of sizeof(HMODULE) cbNeeded=%ld\n", cbNeeded);

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed: %lu\n", GetLastError());

    ret = WaitForInputIdle(pi.hProcess, 5000);
    ok(!ret, "wait timed out\n");

    SetLastError(0xdeadbeef);
    hMod = NULL;
    ret = EnumProcessModules(pi.hProcess, &hMod, sizeof(HMODULE), &cbNeeded);
    ok(ret == 1, "got %ld, error %lu\n", ret, GetLastError());
    ok(!!hMod, "expected non-NULL module\n");
    ok(cbNeeded % sizeof(hMod) == 0, "got %lu\n", cbNeeded);

    TerminateProcess(pi.hProcess, 0);

    if (sizeof(void *) == 8)
    {
        MODULEINFO info;
        char name[MAX_PATH];
        HMODULE hmods[3];

        strcpy(buffer, "C:\\windows\\syswow64\\msinfo32.exe");
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ret)
        {
            ret = WaitForInputIdle(pi.hProcess, 5000);
            ok(!ret, "wait timed out\n");

            SetLastError(0xdeadbeef);
            hmods[0] = NULL;
            ret = EnumProcessModules(pi.hProcess, hmods, sizeof(hmods), &cbNeeded);
            ok(ret == 1, "got %ld, error %lu\n", ret, GetLastError());
            ok(cbNeeded >= sizeof(HMODULE), "expected at least one module\n");
            ok(!!hmods[0], "expected non-NULL module\n");
            ok(cbNeeded % sizeof(hmods[0]) == 0, "got %lu\n", cbNeeded);

            ret = GetModuleBaseNameA(pi.hProcess, hmods[0], name, sizeof(name));
            ok(ret, "got error %lu\n", GetLastError());
            ok(!strcmp(name, "msinfo32.exe"), "got %s\n", name);

            ret = GetModuleFileNameExA(pi.hProcess, hmods[0], name, sizeof(name));
            ok(ret, "got error %lu\n", GetLastError());
            ok(!strcmp(name, buffer), "got %s\n", name);

            ret = GetModuleInformation(pi.hProcess, hmods[0], &info, sizeof(info));
            ok(ret, "got error %lu\n", GetLastError());
            ok(info.lpBaseOfDll == hmods[0], "expected %p, got %p\n", hmods[0], info.lpBaseOfDll);
            ok(info.SizeOfImage, "image size was 0\n");
            ok(info.EntryPoint >= info.lpBaseOfDll, "got entry point %p\n", info.EntryPoint);

            /* "old" Wine wow64 will only return main DLL; while windows & multi-arch Wine Wow64 setup
             * will return main module, ntdll.dll and one of the wow64*.dll.
             */
            todo_wine_if(cbNeeded == sizeof(HMODULE))
            ok(cbNeeded >= 3 * sizeof(HMODULE), "Wrong count of DLLs\n");
            if (cbNeeded >= 3 * sizeof(HMODULE))
            {
                ret = GetModuleBaseNameA(pi.hProcess, hmods[2], name, sizeof(name));
                ok(ret, "got error %lu\n", GetLastError());
                ok(strstr(CharLowerA(name), "wow64") != NULL, "third DLL in wow64 should be one of wow*.dll (%s)\n", name);
            }
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
    else if (wow64)
    {
        pWow64DisableWow64FsRedirection(&cookie);
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        pWow64RevertWow64FsRedirection(cookie);
        ok(ret, "CreateProcess failed: %lu\n", GetLastError());

        ret = WaitForInputIdle(pi.hProcess, 5000);
        ok(!ret, "wait timed out\n");

        SetLastError(0xdeadbeef);
        ret = EnumProcessModules(pi.hProcess, &hMod, sizeof(HMODULE), &cbNeeded);
        ok(!ret, "got %ld\n", ret);
        ok(GetLastError() == ERROR_PARTIAL_COPY, "got error %lu\n", GetLastError());

        TerminateProcess(pi.hProcess, 0);
    }
}

struct moduleex_snapshot
{
    unsigned list;
    DWORD num_modules;
    HMODULE modules[128];
};

static BOOL test_EnumProcessModulesEx_snapshot(HANDLE proc, struct moduleex_snapshot* mxsnap,
                                               unsigned numsnap)
{
    void* cookie;
    DWORD needed;
    char buffer[MAX_PATH];
    char buffer2[MAX_PATH];
    MODULEINFO info;
    int i, j;
    BOOL ret;

    for (i = 0; i < numsnap; i++)
    {
        winetest_push_context("%d", mxsnap[i].list);
        SetLastError(0xdeadbeef);
        mxsnap[i].modules[0] = (void *)0xdeadbeef;
        ret = EnumProcessModulesEx(proc, mxsnap[i].modules, sizeof(mxsnap[i].modules), &needed, mxsnap[i].list);
        ok(ret, "didn't succeed %lu\n", GetLastError());
        ok(needed % sizeof(HMODULE) == 0, "not a multiple of sizeof(HMODULE) cbNeeded=%ld\n", needed);
        mxsnap[i].num_modules = min(needed, sizeof(mxsnap[i].modules)) / sizeof(HMODULE);
        for (j = 0; j < mxsnap[i].num_modules; j++)
        {
            ret = GetModuleBaseNameA(proc, mxsnap[i].modules[j], buffer, sizeof(buffer));
            ok(ret, "GetModuleBaseName failed: %lu (%u/%lu=%p)\n", GetLastError(), j, mxsnap[i].num_modules, mxsnap[i].modules[j]);
            ret = GetModuleFileNameExA(proc, mxsnap[i].modules[j], buffer, sizeof(buffer));
            ok(ret, "GetModuleFileNameEx failed: %lu (%u/%lu=%p)\n", GetLastError(), j, mxsnap[i].num_modules, mxsnap[i].modules[j]);
            if (wow64)
            {
                pWow64DisableWow64FsRedirection(&cookie);
                ret = GetModuleFileNameExA(proc, mxsnap[i].modules[j], buffer2, sizeof(buffer2));
                ok(ret, "GetModuleFileNameEx failed: %lu (%u/%lu=%p)\n", GetLastError(), j, mxsnap[i].num_modules, mxsnap[i].modules[j]);
                pWow64RevertWow64FsRedirection(cookie);
                ok(!strcmp(buffer, buffer2), "Didn't FS redirection to come into play %s <> %s\n", buffer, buffer2);
            }
            memset(&info, 0, sizeof(info));
            ret = GetModuleInformation(proc, mxsnap[i].modules[j], &info, sizeof(info));
            ok(ret, "GetModuleInformation failed: %lu\n", GetLastError());
            ok(info.lpBaseOfDll == mxsnap[i].modules[j], "expected %p, got %p\n", mxsnap[i].modules[j], info.lpBaseOfDll);
            ok(info.SizeOfImage, "image size was 0\n");
            /* info.EntryPoint to be checked */
        }
        winetest_pop_context();
    }

    if (wow64)
    {
        HMODULE modules[128];
        DWORD num;

        pWow64DisableWow64FsRedirection(&cookie);
        for (i = 0; i < numsnap; i++)
        {
            modules[0] = (void *)0xdeadbeef;
            ret = EnumProcessModulesEx(proc, modules, sizeof(modules), &needed, mxsnap[i].list);
            ok(ret, "didn't succeed %lu\n", GetLastError());
            ok(needed % sizeof(HMODULE) == 0, "not a multiple of sizeof(HMODULE) cbNeeded=%ld\n", needed);
            num = min(needed, sizeof(modules)) / sizeof(HMODULE);
            /* It happens that modules are still loading... so check that msxsnap[i].modules is a subset of modules[].
             * Crossing fingers that no module is unloaded
             */
            ok(num >= mxsnap[i].num_modules, "Mismatch in module count %lu %lu\n", num, mxsnap[i].num_modules);
            num = min(num, mxsnap[i].num_modules);
            for (j = 0; j < num; j++)
                ok(modules[j] == mxsnap[i].modules[j], "Mismatch in modules handle\n");
        }
        pWow64RevertWow64FsRedirection(cookie);
    }
    return ret;
}

static BOOL snapshot_is_empty(const struct moduleex_snapshot* snap)
{
    return snap->num_modules == 0;
}

static BOOL snapshot_contains(const struct moduleex_snapshot* snap, HMODULE mod)
{
    int i;

    for (i = 0; i < snap->num_modules; i++)
        if (snap->modules[i] == mod) return TRUE;
    return FALSE;
}

/* It happens (experienced on Windows) that the considered process still loads modules,
 * meaning that the number of loaded modules can increase between consecutive calls to EnumProcessModulesEx.
 * In order to cope with this, we're testing for modules list being included into the next one (instead of
 * equality)
 */
static BOOL snapshot_is_subset(const struct moduleex_snapshot* subset, const struct moduleex_snapshot* superset)
{
    int i;

    for (i = 0; i < subset->num_modules; i++)
        if (!snapshot_contains(superset, subset->modules[i])) return FALSE;
    return TRUE;
}

static BOOL snapshot_is_equal(const struct moduleex_snapshot* seta, const struct moduleex_snapshot* setb)
{
    return snapshot_is_subset(seta, setb) && seta->num_modules == setb->num_modules;
}

static BOOL snapshot_are_disjoint(const struct moduleex_snapshot* seta, const struct moduleex_snapshot* setb, unsigned start)
{
    int i, j;
    for (i = start; i < seta->num_modules; i++)
        for (j = start; j < setb->num_modules; j++)
            if (seta->modules[i] == setb->modules[j]) return FALSE;
    return TRUE;
}

static void snapshot_check_first_main_module(const struct moduleex_snapshot* snap, HANDLE proc,
                                             const char* filename)
{
    char buffer[MAX_PATH];
    MODULEINFO info;
    const char* modname;
    BOOL ret;

    if (!(modname = strrchr(filename, '\\'))) modname = filename; else modname++;
    winetest_push_context("%d", snap->list);
    ret = GetModuleBaseNameA(proc, snap->modules[0], buffer, sizeof(buffer));
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcasecmp(buffer, modname), "expecting %s but got %s\n", modname, buffer);
    ret = GetModuleFileNameExA(proc, snap->modules[0], buffer, sizeof(buffer));
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcasecmp(filename, buffer), "expecting %s but got %s\n", filename, buffer);

    ret = GetModuleInformation(proc, snap->modules[0], &info, sizeof(info));
    ok(ret, "got error %lu\n", GetLastError());
    ok(info.lpBaseOfDll == snap->modules[0], "expected %p, got %p\n", snap->modules[0], info.lpBaseOfDll);
    ok(info.SizeOfImage, "image size was 0\n");
    ok(info.EntryPoint >= info.lpBaseOfDll, "got entry point %p\n", info.EntryPoint);
    winetest_pop_context();
}

static unsigned int snapshot_count_in_dir(const struct moduleex_snapshot* snap, HANDLE proc, const char* dirname)
{
    unsigned int count = 0;
    char buffer[MAX_PATH];
    size_t dirname_len = strlen(dirname);
    BOOL ret;
    int i;

    for (i = 0; i < snap->num_modules; i++)
    {
        ret = GetModuleFileNameExA(proc, snap->modules[i], buffer, sizeof(buffer));
        ok(ret, "got error %lu\n", GetLastError());
        if (!strncasecmp(buffer, dirname, dirname_len)) count++;
    }
    return count;
}

static void test_EnumProcessModulesEx(void)
{
    char buffer[MAX_PATH] = "C:\\windows\\system32\\msinfo32.exe";
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOA si = {0};
    void *cookie;
    HMODULE hMod;
    DWORD ret, cbNeeded = 0xdeadbeef;
    struct moduleex_snapshot snap[4] = {{LIST_MODULES_32BIT}, {LIST_MODULES_64BIT}, {LIST_MODULES_DEFAULT}, {LIST_MODULES_ALL}};
    int i;

    SetLastError(0xdeadbeef);
    EnumProcessModulesEx(NULL, NULL, 0, &cbNeeded, LIST_MODULES_ALL);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    EnumProcessModulesEx(hpQI, NULL, 0, &cbNeeded, LIST_MODULES_ALL);
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModulesEx(hpQI, &hMod, sizeof(HMODULE), NULL, LIST_MODULES_ALL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModulesEx(hpQV, &hMod, sizeof(HMODULE), NULL, LIST_MODULES_ALL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %ld\n", GetLastError());
    ok(hMod == GetModuleHandleA(NULL),
       "hMod=%p GetModuleHandleA(NULL)=%p\n", hMod, GetModuleHandleA(NULL));

    SetLastError(0xdeadbeef);
    ret = EnumProcessModulesEx(hpQV, NULL, 0, &cbNeeded, LIST_MODULES_ALL);
    ok(ret == 1, "failed with %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumProcessModulesEx(hpQV, NULL, sizeof(HMODULE), &cbNeeded, LIST_MODULES_ALL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %ld\n", GetLastError());

    winetest_push_context("self");
    if (sizeof(void *) == 8)
    {
        test_EnumProcessModulesEx_snapshot(hpQV, snap, ARRAY_SIZE(snap));
        ok(snapshot_is_empty(&snap[0]), "didn't expect 32bit module\n");
        ok(snapshot_is_equal(&snap[1], &snap[2]), "mismatch in modules count\n");
        ok(snapshot_is_equal(&snap[2], &snap[3]), "mismatch in modules count\n");
        snapshot_check_first_main_module(&snap[1], hpQV, main_argv[0]);
        snapshot_check_first_main_module(&snap[2], hpQV, main_argv[0]);
        snapshot_check_first_main_module(&snap[3], hpQV, main_argv[0]);

        /* in fact, this error is only returned when (list & 3 == 0), otherwise the corresponding
         * list is returned without errors
         */
        SetLastError(0xdeadbeef);
        ret = EnumProcessModulesEx(hpQV, &hMod, sizeof(HMODULE), &cbNeeded, 0x400);
        ok(!ret, "succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected error=ERROR_INVALID_PARAMETER but got %ld\n", GetLastError());
    }
    else if (wow64)
    {
        test_EnumProcessModulesEx_snapshot(hpQV, snap, ARRAY_SIZE(snap));
        ok(snapshot_is_equal(&snap[0], &snap[1]), "mismatch in modules count\n");
        ok(snapshot_is_equal(&snap[1], &snap[2]), "mismatch in modules count\n");
        ok(snapshot_is_equal(&snap[2], &snap[3]), "mismatch in modules count\n");
        snapshot_check_first_main_module(&snap[0], hpQV, main_argv[0]);
        snapshot_check_first_main_module(&snap[1], hpQV, main_argv[0]);
        snapshot_check_first_main_module(&snap[2], hpQV, main_argv[0]);
        snapshot_check_first_main_module(&snap[3], hpQV, main_argv[0]);
    }
    winetest_pop_context();

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed: %lu\n", GetLastError());

    ret = WaitForInputIdle(pi.hProcess, 5000);
    ok(!ret, "wait timed out\n");

    if (sizeof(void *) == 8)
    {
        winetest_push_context("pcs-6464");
        test_EnumProcessModulesEx_snapshot(pi.hProcess, snap, ARRAY_SIZE(snap));
        ok(snapshot_is_empty(&snap[0]), "didn't expect 32bit module\n");
        ok(snapshot_is_subset(&snap[1], &snap[2]), "64bit and default module lists should match\n");
        ok(snapshot_is_subset(&snap[2], &snap[3]), "default and all module lists should match\n");
        snapshot_check_first_main_module(&snap[1], pi.hProcess, buffer);
        snapshot_check_first_main_module(&snap[2], pi.hProcess, buffer);
        snapshot_check_first_main_module(&snap[3], pi.hProcess, buffer);
        winetest_pop_context();

        /* in fact, this error is only returned when (list & 3 == 0), otherwise the corresponding
         * list is returned without errors
         */
        SetLastError(0xdeadbeef);
        ret = EnumProcessModulesEx(hpQV, &hMod, sizeof(HMODULE), &cbNeeded, 0x400);
        ok(!ret, "succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected error=ERROR_INVALID_PARAMETER but got %ld\n", GetLastError());
    }
    else if (wow64)
    {
        winetest_push_context("pcs-3232");
        test_EnumProcessModulesEx_snapshot(pi.hProcess, snap, ARRAY_SIZE(snap));
        /* some windows version return 64bit modules, others don't... */
        /* ok(snapshot_is_empty(&snap[1]), "didn't expect 64bit module\n"); */
        ok(snapshot_is_subset(&snap[1], &snap[3]), "64 and all module lists should match\n");

        ok(snapshot_is_subset(&snap[0], &snap[2]), "32bit and default module lists should match\n");
        ok(snapshot_is_subset(&snap[2], &snap[3]), "default and all module lists should match\n");
        snapshot_check_first_main_module(&snap[0], pi.hProcess, "c:\\windows\\syswow64\\msinfo32.exe");
        snapshot_check_first_main_module(&snap[2], pi.hProcess, "c:\\windows\\syswow64\\msinfo32.exe");
        snapshot_check_first_main_module(&snap[3], pi.hProcess, "c:\\windows\\syswow64\\msinfo32.exe");
        winetest_pop_context();
    }

    TerminateProcess(pi.hProcess, 0);

    if (sizeof(void *) == 8)
    {
        unsigned int count;

        strcpy(buffer, "C:\\windows\\syswow64\\msinfo32.exe");
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ret)
        {
            ret = WaitForInputIdle(pi.hProcess, 5000);
            ok(!ret, "wait timed out\n");

            winetest_push_context("pcs-6432");
            test_EnumProcessModulesEx_snapshot(pi.hProcess, snap, ARRAY_SIZE(snap));
            ok(!snapshot_is_empty(&snap[0]), "expecting 32bit modules\n");
            /* FIXME: this tests fails on Wine "old" wow configuration, but succceeds in "multi-arch" and Windows */
            todo_wine_if(snapshot_is_empty(&snap[1]))
            ok(!snapshot_is_empty(&snap[1]), "expecting 64bit modules\n");
            ok(snapshot_is_subset(&snap[1], &snap[2]), "64bit and default module lists should match\n");
            ok(snapshot_are_disjoint(&snap[0], &snap[1], 0), "32bit and 64bit list should be disjoint\n");
            /* Main module (even 32bit) is present in both 32bit (makes sense) but also default
             * (even if all the other modules are 64bit)
             */
            ok(snapshot_are_disjoint(&snap[0], &snap[2], 1), "32bit and default list should be disjoint\n");
            ok(snapshot_is_subset(&snap[0], &snap[3]), "32bit and all module lists should match\n");
            ok(snapshot_is_subset(&snap[1], &snap[3]), "64bit and all module lists should match\n");
            ok(snapshot_is_subset(&snap[2], &snap[3]), "default and all module list should match\n");
            snapshot_check_first_main_module(&snap[0], pi.hProcess, buffer);
            ok(!snapshot_contains(&snap[1], snap[0].modules[0]), "main module shouldn't be present in 64bit list\n");
            snapshot_check_first_main_module(&snap[2], pi.hProcess, buffer);
            snapshot_check_first_main_module(&snap[3], pi.hProcess, buffer);

            ret = GetSystemWow64DirectoryA(buffer, sizeof(buffer));
            ok(ret, "GetSystemWow64DirectoryA failed: %lu\n", GetLastError());
            count = snapshot_count_in_dir(snap, pi.hProcess, buffer);
            todo_wine
            ok(count <= 1, "Wrong count %u in %s\n", count, buffer); /* msinfo32 can be from system wow64 */
            ret = GetSystemDirectoryA(buffer, sizeof(buffer));
            ok(ret, "GetSystemDirectoryA failed: %lu\n", GetLastError());
            count = snapshot_count_in_dir(snap, pi.hProcess, buffer);
            ok(count > 2, "Wrong count %u in %s\n", count, buffer);

            /* in fact, this error is only returned when (list & 3 == 0), otherwise the corresponding
             * list is returned without errors.
             */
            SetLastError(0xdeadbeef);
            ret = EnumProcessModulesEx(hpQV, &hMod, sizeof(HMODULE), &cbNeeded, 0x400);
            ok(!ret, "succeeded\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected error=ERROR_INVALID_PARAMETER but got %ld\n", GetLastError());

            winetest_pop_context();

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
    else if (wow64)
    {
        pWow64DisableWow64FsRedirection(&cookie);
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        pWow64RevertWow64FsRedirection(cookie);
        ok(ret, "CreateProcess failed: %lu\n", GetLastError());

        ret = WaitForInputIdle(pi.hProcess, 5000);
        ok(!ret, "wait timed out\n");

        winetest_push_context("pcs-3264");
        for (i = 0; i < ARRAY_SIZE(snap); i++)
        {
            SetLastError(0xdeadbeef);
            ret = EnumProcessModulesEx(pi.hProcess, &hMod, sizeof(HMODULE), &cbNeeded, snap[i].list);
            ok(!ret, "succeeded\n");
            ok(GetLastError() == ERROR_PARTIAL_COPY, "expected error=ERROR_PARTIAL_COPY but got %ld\n", GetLastError());
        }
        winetest_pop_context();

        TerminateProcess(pi.hProcess, 0);
    }
}

static void test_GetModuleInformation(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    MODULEINFO info;
    DWORD ret;

    SetLastError(0xdeadbeef);
    GetModuleInformation(NULL, hMod, &info, sizeof(info));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleInformation(hpQI, hMod, &info, sizeof(info));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleInformation(hpQV, hBad, &info, sizeof(info));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleInformation(hpQV, hMod, &info, sizeof(info)-1);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetModuleInformation(hpQV, hMod, &info, sizeof(info));
    ok(ret == 1, "failed with %ld\n", GetLastError());
    ok(info.lpBaseOfDll == hMod, "lpBaseOfDll=%p hMod=%p\n", info.lpBaseOfDll, hMod);
}

static BOOL check_with_margin(SIZE_T perf, SIZE_T sysperf, int margin)
{
    return (perf >= max(sysperf, margin) - margin && perf <= sysperf + margin);
}

static void test_GetPerformanceInfo(void)
{
    PERFORMANCE_INFORMATION info;
    NTSTATUS status;
    DWORD size;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = GetPerformanceInfo(&info, sizeof(info)-1);
    ok(!ret, "GetPerformanceInfo unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BAD_LENGTH, "expected error=ERROR_BAD_LENGTH but got %ld\n", GetLastError());

    if (!pNtQuerySystemInformation)
        win_skip("NtQuerySystemInformation not found, skipping tests\n");
    else
    {
        char performance_buffer[sizeof(SYSTEM_PERFORMANCE_INFORMATION) + 16]; /* larger on w2k8/win7 */
        SYSTEM_PERFORMANCE_INFORMATION *sys_performance_info = (SYSTEM_PERFORMANCE_INFORMATION *)performance_buffer;
        SYSTEM_PROCESS_INFORMATION *sys_process_info = NULL, *spi;
        SYSTEM_BASIC_INFORMATION sys_basic_info;
        DWORD process_count, handle_count, thread_count;

        /* compare with values from SYSTEM_PERFORMANCE_INFORMATION */
        size = 0;
        status = pNtQuerySystemInformation(SystemPerformanceInformation, sys_performance_info, sizeof(performance_buffer), &size);
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
        ok(size >= sizeof(SYSTEM_PERFORMANCE_INFORMATION), "incorrect length %ld\n", size);

        SetLastError(0xdeadbeef);
        ret = GetPerformanceInfo(&info, sizeof(info));
        ok(ret, "GetPerformanceInfo failed with %ld\n", GetLastError());
        ok(info.cb == sizeof(PERFORMANCE_INFORMATION), "got %ld\n", info.cb);

        ok(check_with_margin(info.CommitTotal,          sys_performance_info->TotalCommittedPages,  2048),
           "expected approximately %Id but got %ld\n", info.CommitTotal, sys_performance_info->TotalCommittedPages);

        ok(check_with_margin(info.CommitLimit,          sys_performance_info->TotalCommitLimit,     32),
           "expected approximately %Id but got %ld\n", info.CommitLimit, sys_performance_info->TotalCommitLimit);

        ok(check_with_margin(info.CommitPeak,           sys_performance_info->PeakCommitment,       32),
           "expected approximately %Id but got %ld\n", info.CommitPeak, sys_performance_info->PeakCommitment);

        ok(check_with_margin(info.PhysicalAvailable,    sys_performance_info->AvailablePages,       2048),
           "expected approximately %Id but got %ld\n", info.PhysicalAvailable, sys_performance_info->AvailablePages);

        /* TODO: info.SystemCache not checked yet - to which field(s) does this value correspond to? */

        ok(check_with_margin(info.KernelTotal, sys_performance_info->PagedPoolUsage + sys_performance_info->NonPagedPoolUsage, 256),
           "expected approximately %Id but got %ld\n", info.KernelTotal,
           sys_performance_info->PagedPoolUsage + sys_performance_info->NonPagedPoolUsage);

        ok(check_with_margin(info.KernelPaged,          sys_performance_info->PagedPoolUsage,       256),
           "expected approximately %Id but got %ld\n", info.KernelPaged, sys_performance_info->PagedPoolUsage);

        ok(check_with_margin(info.KernelNonpaged,       sys_performance_info->NonPagedPoolUsage,    16),
           "expected approximately %Id but got %ld\n", info.KernelNonpaged, sys_performance_info->NonPagedPoolUsage);

        /* compare with values from SYSTEM_BASIC_INFORMATION */
        size = 0;
        status = pNtQuerySystemInformation(SystemBasicInformation, &sys_basic_info, sizeof(sys_basic_info), &size);
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
        ok(size >= sizeof(SYSTEM_BASIC_INFORMATION), "incorrect length %ld\n", size);

        ok(info.PhysicalTotal == sys_basic_info.MmNumberOfPhysicalPages,
           "expected info.PhysicalTotal=%lu but got %lu\n",
           sys_basic_info.MmNumberOfPhysicalPages, (ULONG)info.PhysicalTotal);

        ok(info.PageSize == sys_basic_info.PageSize,
           "expected info.PageSize=%lu but got %lu\n",
           sys_basic_info.PageSize, (ULONG)info.PageSize);

        /* compare with values from SYSTEM_PROCESS_INFORMATION */
        size = 0;
        status = pNtQuerySystemInformation(SystemProcessInformation, NULL, 0, &size);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
        ok(size > 0, "incorrect length %ld\n", size);
        while (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            sys_process_info = HeapAlloc(GetProcessHeap(), 0, size);
            ok(sys_process_info != NULL, "failed to allocate memory\n");
            status = pNtQuerySystemInformation(SystemProcessInformation, sys_process_info, size, &size);
            if (status == STATUS_SUCCESS) break;
            HeapFree(GetProcessHeap(), 0, sys_process_info);
        }
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);

        process_count = handle_count = thread_count = 0;
        for (spi = sys_process_info;; spi = (SYSTEM_PROCESS_INFORMATION *)(((char *)spi) + spi->NextEntryOffset))
        {
            process_count++;
            handle_count += spi->HandleCount;
            thread_count += spi->dwThreadCount;
            if (spi->NextEntryOffset == 0) break;
        }
        HeapFree(GetProcessHeap(), 0, sys_process_info);

        ok(check_with_margin(info.HandleCount,  handle_count,  512),
           "expected approximately %ld but got %ld\n", info.HandleCount, handle_count);

        ok(check_with_margin(info.ProcessCount, process_count, 4),
           "expected approximately %ld but got %ld\n", info.ProcessCount, process_count);

        ok(check_with_margin(info.ThreadCount,  thread_count,  4),
           "expected approximately %ld but got %ld\n", info.ThreadCount, thread_count);
    }
}


static void test_GetProcessMemoryInfo(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    DWORD ret;

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(NULL, &pmc, sizeof(pmc));
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(hpSR, &pmc, sizeof(pmc));
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc)-1);
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc));
    ok(ret == 1, "failed with %ld\n", GetLastError());
}

static BOOL nt_get_mapped_file_name(HANDLE process, LPVOID addr, LPWSTR name, DWORD len)
{
    MEMORY_SECTION_NAME *section_name;
    WCHAR *buf;
    SIZE_T buf_len, ret_len;
    NTSTATUS status;

    if (!pNtQueryVirtualMemory) return FALSE;

    buf_len = len * sizeof(WCHAR) + sizeof(MEMORY_SECTION_NAME);
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buf_len);

    ret_len = 0xdeadbeef;
    status = pNtQueryVirtualMemory(process, addr, MemoryMappedFilenameInformation, buf, buf_len, &ret_len);
    ok(!status, "NtQueryVirtualMemory error %lx\n", status);

    section_name = (MEMORY_SECTION_NAME *)buf;
    ok(ret_len == section_name->SectionFileName.MaximumLength + sizeof(*section_name), "got %Iu, %u\n",
       ret_len, section_name->SectionFileName.MaximumLength);
    ok((char *)section_name->SectionFileName.Buffer == (char *)section_name + sizeof(*section_name), "got %p, %p\n",
       section_name, section_name->SectionFileName.Buffer);
    ok(section_name->SectionFileName.MaximumLength == section_name->SectionFileName.Length + sizeof(WCHAR), "got %u, %u\n",
       section_name->SectionFileName.MaximumLength, section_name->SectionFileName.Length);
    ok(section_name->SectionFileName.Length == lstrlenW(section_name->SectionFileName.Buffer) * sizeof(WCHAR), "got %u, %u\n",
       section_name->SectionFileName.Length, lstrlenW(section_name->SectionFileName.Buffer));

    memcpy(name, section_name->SectionFileName.Buffer, section_name->SectionFileName.MaximumLength);
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

static void test_GetMappedFileName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szMapPath[MAX_PATH], szModPath[MAX_PATH], *szMapBaseName;
    DWORD ret;
    char *base;
    char temp_path[MAX_PATH], file_name[MAX_PATH], map_name[MAX_PATH], device_name[MAX_PATH], drive[3];
    WCHAR map_nameW[MAX_PATH], nt_map_name[MAX_PATH];
    HANDLE hfile, hmap;

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(NULL, hMod, szMapPath, sizeof(szMapPath));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(hpSR, hMod, szMapPath, sizeof(szMapPath));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = GetMappedFileNameA(hpQI, hMod, szMapPath, sizeof(szMapPath));
    ok( ret || broken(GetLastError() == ERROR_UNEXP_NET_ERR), /* win2k */
        "GetMappedFileNameA failed with error %lu\n", GetLastError() );
    if (ret)
    {
        ok(ret == strlen(szMapPath), "szMapPath=\"%s\" ret=%ld\n", szMapPath, ret);
        ok(szMapPath[0] == '\\', "szMapPath=\"%s\"\n", szMapPath);
        szMapBaseName = strrchr(szMapPath, '\\'); /* That's close enough for us */
        ok(szMapBaseName && *szMapBaseName, "szMapPath=\"%s\"\n", szMapPath);
        if (szMapBaseName)
        {
            GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
            ok(!strcmp(strrchr(szModPath, '\\'), szMapBaseName),
               "szModPath=\"%s\" szMapBaseName=\"%s\"\n", szModPath, szMapBaseName);
        }
    }

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "map", 0, file_name);

    drive[0] = file_name[0];
    drive[1] = ':';
    drive[2] = 0;
    SetLastError(0xdeadbeef);
    ret = QueryDosDeviceA(drive, device_name, sizeof(device_name));
    ok(ret, "QueryDosDeviceA error %ld\n", GetLastError());
    trace("%s -> %s\n", drive, device_name);

    SetLastError(0xdeadbeef);
    hfile = CreateFileA(file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFileA(%s) error %ld\n", file_name, GetLastError());
    SetFilePointer(hfile, 0x4000, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingA(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmap != 0, "CreateFileMappingA error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    base = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
    ok(base != NULL, "MapViewOfFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, 0);
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "wrong error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, 0, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, 1);
    ok(ret == 1, "GetMappedFileName error %ld\n", GetLastError());
    ok(!map_name[0] || broken(map_name[0] == device_name[0]) /* before win2k */, "expected 0, got %c\n", map_name[0]);

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, sizeof(map_name));
    ok(ret, "GetMappedFileName error %ld\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    todo_wine
    ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameW(GetCurrentProcess(), base, map_nameW, ARRAY_SIZE(map_nameW));
    ok(ret, "GetMappedFileNameW error %ld\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    if (nt_get_mapped_file_name(GetCurrentProcess(), base, nt_map_name, ARRAY_SIZE(nt_map_name)))
    {
        ok(memcmp(map_nameW, nt_map_name, lstrlenW(map_nameW)) == 0, "map name does not start with a device name: %s\n", map_name);
        WideCharToMultiByte(CP_ACP, 0, map_nameW, -1, map_name, MAX_PATH, NULL, NULL);
        todo_wine
        ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);
    }

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base + 0x2000, map_name, sizeof(map_name));
    ok(ret, "GetMappedFileName error %ld\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    todo_wine
    ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base + 0x4000, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_UNEXP_NET_ERR, "expected ERROR_UNEXP_NET_ERR, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), NULL, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_UNEXP_NET_ERR, "expected ERROR_UNEXP_NET_ERR, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(0, base, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    UnmapViewOfFile(base);
    CloseHandle(hmap);
    CloseHandle(hfile);
    DeleteFileA(file_name);

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY | SEC_COMMIT, 0, 4096, NULL);
    ok(hmap != 0, "CreateFileMappingA error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    base = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
    ok(base != NULL, "MapViewOfFile error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_FILE_INVALID, "expected ERROR_FILE_INVALID, got %ld\n", GetLastError());

    UnmapViewOfFile(base);
    CloseHandle(hmap);
}

static void test_GetProcessImageFileName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szImgPath[MAX_PATH], szMapPath[MAX_PATH];
    WCHAR szImgPathW[MAX_PATH];
    DWORD ret, ret1;

    /* This function is available on WinXP+ only */
    SetLastError(0xdeadbeef);
    if(!GetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath)))
    {
        if(GetLastError() == ERROR_INVALID_FUNCTION) {
	    win_skip("GetProcessImageFileName not implemented\n");
            return;
        }
        ok(0, "failed with %ld\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameA(NULL, szImgPath, sizeof(szImgPath));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameA(hpSR, szImgPath, sizeof(szImgPath));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameA(hpQI, szImgPath, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %ld\n", GetLastError());

    ret = GetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath));
    ret1 = GetMappedFileNameA(hpQV, hMod, szMapPath, sizeof(szMapPath));
    if(ret && ret1)
    {
        /* Windows returns 2*strlen-1 */
        ok(ret >= strlen(szImgPath), "szImgPath=\"%s\" ret=%ld\n", szImgPath, ret);
        todo_wine ok(!strcmp(szImgPath, szMapPath), "szImgPath=\"%s\" szMapPath=\"%s\"\n", szImgPath, szMapPath);
    }

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(NULL, szImgPathW, ARRAY_SIZE(szImgPathW));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    /* no information about correct buffer size returned: */
    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(hpQI, szImgPathW, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(hpQI, NULL, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %ld\n", GetLastError());

    /* correct call */
    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = GetProcessImageFileNameW(hpQI, szImgPathW, ARRAY_SIZE(szImgPathW));
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    ok(lstrlenW(szImgPathW) == ret, "Expected length to be %ld, got %d\n", ret, lstrlenW(szImgPathW));

    /* boundary values of 'size' */
    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(hpQI, szImgPathW, ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %ld\n", GetLastError());

    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = GetProcessImageFileNameW(hpQI, szImgPathW, ret + 1);
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    ok(lstrlenW(szImgPathW) == ret, "Expected length to be %ld, got %d\n", ret, lstrlenW(szImgPathW));
}

static void test_GetModuleFileNameEx(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char szModExPath[MAX_PATH+1], szModPath[MAX_PATH+1];
    WCHAR buffer[MAX_PATH], buffer2[MAX_PATH];
    DWORD ret, size, size2;

    SetLastError(0xdeadbeef);
    ret = GetModuleFileNameExA(NULL, hMod, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetModuleFileNameExA(hpQI, hMod, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetModuleFileNameExA(hpQV, hBad, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    ret = GetModuleFileNameExA(hpQV, NULL, szModExPath, sizeof(szModExPath));
    if(!ret)
            return;
    ok(ret == strlen(szModExPath), "szModExPath=\"%s\" ret=%ld\n", szModExPath, ret);
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strncmp(szModExPath, szModPath, MAX_PATH), 
       "szModExPath=\"%s\" szModPath=\"%s\"\n", szModExPath, szModPath);

    SetLastError(0xdeadbeef);
    memset( szModExPath, 0xcc, sizeof(szModExPath) );
    ret = GetModuleFileNameExA(hpQV, NULL, szModExPath, 4 );
    ok( ret == 4 || ret == strlen(szModExPath), "wrong length %lu\n", ret );
    ok( broken(szModExPath[3]) /*w2kpro*/ || strlen(szModExPath) == 3,
        "szModExPath=\"%s\" ret=%ld\n", szModExPath, ret );
    ok(GetLastError() == 0xdeadbeef, "got error %ld\n", GetLastError());

    if (0) /* crashes on Windows 10 */
    {
        SetLastError(0xdeadbeef);
        ret = GetModuleFileNameExA(hpQV, NULL, szModExPath, 0 );
        ok( ret == 0, "wrong length %lu\n", ret );
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %ld\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    memset( buffer, 0xcc, sizeof(buffer) );
    ret = GetModuleFileNameExW(hpQV, NULL, buffer, 4 );
    ok( ret == 4 || ret == lstrlenW(buffer), "wrong length %lu\n", ret );
    ok( broken(buffer[3]) /*w2kpro*/ || lstrlenW(buffer) == 3,
        "buffer=%s ret=%ld\n", wine_dbgstr_w(buffer), ret );
    ok(GetLastError() == 0xdeadbeef, "got error %ld\n", GetLastError());

    if (0) /* crashes on Windows 10 */
    {
        SetLastError(0xdeadbeef);
        buffer[0] = 0xcc;
        ret = GetModuleFileNameExW(hpQV, NULL, buffer, 0 );
        ok( ret == 0, "wrong length %lu\n", ret );
        ok(GetLastError() == 0xdeadbeef, "got error %ld\n", GetLastError());
        ok( buffer[0] == 0xcc, "buffer modified %s\n", wine_dbgstr_w(buffer) );
    }

    /* Verify that retrieving the main process image file name works on a suspended process,
     * where the loader has not yet had a chance to initialize the PEB. */
    wcscpy(buffer, L"C:\\windows\\system32\\msinfo32.exe");
    ret = CreateProcessW(NULL, buffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcessW failed: %lu\n", GetLastError());

    size = GetModuleFileNameExW(pi.hProcess, NULL, buffer, ARRAYSIZE(buffer));
    ok(size ||
       broken(GetLastError() == ERROR_INVALID_HANDLE), /* < Win10 */
       "GetModuleFileNameExW failed: %lu\n", GetLastError());
    if (GetLastError() == ERROR_INVALID_HANDLE)
        goto cleanup;
    ok(size == wcslen(buffer), "unexpected size %lu\n", size);

    size2 = ARRAYSIZE(buffer2);
    ret = QueryFullProcessImageNameW(pi.hProcess, 0, buffer2, &size2);
    ok(size == size2, "got size %lu, expected %lu\n", size, size2);
    ok(!wcscmp(buffer, buffer2), "unexpected image name %s, expected %s\n", debugstr_w(buffer), debugstr_w(buffer2));

cleanup:
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

static void test_GetModuleBaseName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szModPath[MAX_PATH], szModBaseName[MAX_PATH];
    DWORD ret;

    SetLastError(0xdeadbeef);
    GetModuleBaseNameA(NULL, hMod, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleBaseNameA(hpQI, hMod, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleBaseNameA(hpQV, hBad, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    ret = GetModuleBaseNameA(hpQV, NULL, szModBaseName, sizeof(szModBaseName));
    if(!ret)
        return;
    ok(ret == strlen(szModBaseName), "szModBaseName=\"%s\" ret=%ld\n", szModBaseName, ret);
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strcmp(strrchr(szModPath, '\\') + 1, szModBaseName),
       "szModPath=\"%s\" szModBaseName=\"%s\"\n", szModPath, szModBaseName);
}

static void test_ws_functions(void)
{
    PSAPI_WS_WATCH_INFORMATION wswi[4096];
    HANDLE ws_handle;
    char *addr;
    unsigned int i;
    BOOL ret;

    ws_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA |
        PROCESS_SET_INFORMATION, FALSE, GetCurrentProcessId());
    ok(!!ws_handle, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    EmptyWorkingSet(NULL);
    todo_wine ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    EmptyWorkingSet(hpSR);
    todo_wine ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EmptyWorkingSet(ws_handle);
    ok(ret == 1, "failed with %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = InitializeProcessForWsWatch( NULL );
    todo_wine ok( !ret, "InitializeProcessForWsWatch succeeded\n" );
    if (!ret)
    {
        if (GetLastError() == ERROR_INVALID_FUNCTION)  /* not supported on xp in wow64 mode */
        {
            trace( "InitializeProcessForWsWatch not supported\n" );
            return;
        }
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
    }
    SetLastError(0xdeadbeef);
    ret = InitializeProcessForWsWatch(ws_handle);
    ok(ret == 1, "failed with %ld\n", GetLastError());
    
    addr = VirtualAlloc(NULL, 1, MEM_COMMIT, PAGE_READWRITE);
    if(!addr)
        return;

    *addr = 0; /* make sure it's paged in (needed on wow64) */
    if(!VirtualLock(addr, 1))
    {
        trace("locking failed (error=%ld) - skipping test\n", GetLastError());
        goto free_page;
    }

    SetLastError(0xdeadbeef);
    ret = GetWsChanges(hpQI, wswi, sizeof(wswi));
    todo_wine ok(ret == 1, "failed with %ld\n", GetLastError());
    if(ret == 1)
    {
        for(i = 0; wswi[i].FaultingVa; i++)
	    if(((ULONG_PTR)wswi[i].FaultingVa & ~0xfffL) == (ULONG_PTR)addr)
	    {
	        todo_wine ok(ret == 1, "GetWsChanges found our page\n");
		goto free_page;
	    }

	todo_wine ok(0, "GetWsChanges didn't find our page\n");
    }
    
free_page:
    VirtualFree(addr, 0, MEM_RELEASE);
}

static void check_working_set_info(PSAPI_WORKING_SET_EX_INFORMATION *info, const char *desc, DWORD expected_valid,
                                   DWORD expected_protection, DWORD expected_shared, BOOL todo)
{
    todo_wine_if(todo)
    ok(info->VirtualAttributes.Valid == expected_valid, "%s expected Valid=%lu but got %u\n",
        desc, expected_valid, info->VirtualAttributes.Valid);

    todo_wine_if(todo)
    ok(info->VirtualAttributes.Win32Protection == expected_protection, "%s expected Win32Protection=%lu but got %u\n",
        desc, expected_protection, info->VirtualAttributes.Win32Protection);

    ok(info->VirtualAttributes.Node == 0, "%s expected Node=0 but got %u\n",
        desc, info->VirtualAttributes.Node);
    ok(info->VirtualAttributes.LargePage == 0, "%s expected LargePage=0 but got %u\n",
        desc, info->VirtualAttributes.LargePage);

    ok(info->VirtualAttributes.Shared == expected_shared || broken(!info->VirtualAttributes.Valid) /* w2003 */,
        "%s expected Shared=%lu but got %u\n", desc, expected_shared, info->VirtualAttributes.Shared);
    if (info->VirtualAttributes.Valid && info->VirtualAttributes.Shared)
        ok(info->VirtualAttributes.ShareCount > 0, "%s expected ShareCount > 0 but got %u\n",
            desc, info->VirtualAttributes.ShareCount);
    else
        ok(info->VirtualAttributes.ShareCount == 0, "%s expected ShareCount == 0 but got %u\n",
            desc, info->VirtualAttributes.ShareCount);
}

static void check_QueryWorkingSetEx(PVOID addr, const char *desc, DWORD expected_valid,
                                    DWORD expected_protection, DWORD expected_shared, BOOL todo)
{
    PSAPI_WORKING_SET_EX_INFORMATION info;
    BOOL ret;

    memset(&info, 0x41, sizeof(info));
    info.VirtualAddress = addr;
    ret = pQueryWorkingSetEx(GetCurrentProcess(), &info, sizeof(info));
    ok(ret, "QueryWorkingSetEx failed with %ld\n", GetLastError());

    check_working_set_info(&info, desc, expected_valid, expected_protection, expected_shared, todo);
}

static void test_QueryWorkingSetEx(void)
{
    PSAPI_WORKING_SET_EX_INFORMATION info[4];
    char *addr, *addr2;
    NTSTATUS status;
    SIZE_T size;
    DWORD prot;
    BOOL ret;

    if (pQueryWorkingSetEx == NULL)
    {
        win_skip("QueryWorkingSetEx not found, skipping tests\n");
        return;
    }

    size = 0xdeadbeef;
    memset(info, 0, sizeof(info));
    status = pNtQueryVirtualMemory(GetCurrentProcess(), NULL, MemoryWorkingSetExInformation, info, 0, &size);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx.\n", status);
    ok(size == 0xdeadbeef, "got %Iu.\n", size);

    memset(&info, 0, sizeof(info));
    ret = pQueryWorkingSetEx(GetCurrentProcess(), info, 0);
    ok(!ret && GetLastError() == ERROR_BAD_LENGTH, "got ret %d, err %lu.\n", ret, GetLastError());

    size = 0xdeadbeef;
    memset(info, 0, sizeof(info));
    status = pNtQueryVirtualMemory(GetCurrentProcess(), NULL, MemoryWorkingSetExInformation, info,
            sizeof(*info) + sizeof(*info) / 2, &size);
    ok(!status, "got %#lx.\n", status);
    ok(!info->VirtualAttributes.Valid, "got %d.\n", info->VirtualAttributes.Valid);
    ok(size == sizeof(*info) /* wow64 */ || size == sizeof(*info) + sizeof(*info) / 2 /* win64 */,
            "got %Iu, sizeof(info) %Iu.\n", size, sizeof(info));

    addr = (void *)GetModuleHandleA(NULL);
    check_QueryWorkingSetEx(addr, "exe", 1, PAGE_READONLY, 1, FALSE);

    ret = VirtualProtect(addr, 0x1000, PAGE_NOACCESS, &prot);
    ok(ret, "VirtualProtect failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "exe,noaccess", 0, 0, 1, FALSE);

    ret = VirtualProtect(addr, 0x1000, prot, &prot);
    ok(ret, "VirtualProtect failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "exe,readonly1", 0, 0, 1, TRUE);

    *(volatile char *)addr;
    ok(ret, "VirtualProtect failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "exe,readonly2", 1, PAGE_READONLY, 1, FALSE);

    addr = VirtualAlloc(NULL, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ok(addr != NULL, "VirtualAlloc failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "valloc", 0, 0, 0, FALSE);

    *(volatile char *)addr;
    check_QueryWorkingSetEx(addr, "valloc,read", 1, PAGE_READWRITE, 0, FALSE);

    *(volatile char *)addr = 0x42;
    check_QueryWorkingSetEx(addr, "valloc,write", 1, PAGE_READWRITE, 0, FALSE);

    ret = VirtualProtect(addr, 0x1000, PAGE_NOACCESS, &prot);
    ok(ret, "VirtualProtect failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "valloc,noaccess", 0, 0, 0, FALSE);

    ret = VirtualProtect(addr, 0x1000, prot, &prot);
    ok(ret, "VirtualProtect failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "valloc,readwrite1", 0, 0, 0, TRUE);

    *(volatile char *)addr;
    check_QueryWorkingSetEx(addr, "valloc,readwrite2", 1, PAGE_READWRITE, 0, FALSE);

    addr2 = VirtualAlloc(NULL, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ok(!!addr2, "VirtualAlloc failed with %ld\n", GetLastError());
    *(addr2 + 0x1000) = 1;

    info[1].VirtualAddress = addr;
    info[0].VirtualAddress = addr + 0x1000;
    info[3].VirtualAddress = addr2;
    info[2].VirtualAddress = addr2 + 0x1000;
    ret = pQueryWorkingSetEx(GetCurrentProcess(), info, sizeof(info));
    ok(ret, "got error %lu\n", GetLastError());
    check_working_set_info(&info[1], "[1] range[1] valid", 1, PAGE_READWRITE, 0, FALSE);
    check_working_set_info(&info[0], "[1] range[0] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[3], "[1] range[3] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[2], "[1] range[2] valid", 1, PAGE_READWRITE, 0, FALSE);

    ret = VirtualFree(addr, 0, MEM_RELEASE);
    ok(ret, "VirtualFree failed with %ld\n", GetLastError());
    check_QueryWorkingSetEx(addr, "valloc,free", FALSE, 0, 0, FALSE);

    ret = pQueryWorkingSetEx(GetCurrentProcess(), info, sizeof(info));
    ok(ret, "got error %lu\n", GetLastError());
    check_working_set_info(&info[1], "[2] range[1] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[0], "[2] range[0] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[3], "[2] range[3] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[2], "[2] range[2] valid", 1, PAGE_READWRITE, 0, FALSE);

    VirtualFree(addr2, 0, MEM_RELEASE);
    ret = pQueryWorkingSetEx(GetCurrentProcess(), info, sizeof(info));
    ok(ret, "got error %lu\n", GetLastError());
    check_working_set_info(&info[1], "[3] range[1] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[0], "[3] range[0] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[3], "[3] range[3] invalid", 0, 0, 0, FALSE);
    check_working_set_info(&info[2], "[3] range[2] invalid", 0, 0, 0, FALSE);
}

START_TEST(psapi_main)
{
    DWORD pid = GetCurrentProcessId();

    winetest_get_mainargs(&main_argv);
    init_func_ptrs();

    if (pIsWow64Process)
        IsWow64Process(GetCurrentProcess(), &wow64);

    hpSR = OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, pid);
    ok(!!hpSR, "got error %lu\n", GetLastError());
    hpQI = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    ok(!!hpQI, "got error %lu\n", GetLastError());
    hpVR = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    ok(!!hpVR, "got error %lu\n", GetLastError());
    hpQV = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    ok(!!hpQV, "got error %lu\n", GetLastError());

    test_EnumProcesses();
    test_EnumProcessModules();
    test_EnumProcessModulesEx();
    test_GetModuleInformation();
    test_GetPerformanceInfo();
    test_GetProcessMemoryInfo();
    test_GetMappedFileName();
    test_GetProcessImageFileName();
    test_GetModuleFileNameEx();
    test_GetModuleBaseName();
    test_QueryWorkingSetEx();
    test_ws_functions();

    CloseHandle(hpSR);
    CloseHandle(hpQI);
    CloseHandle(hpVR);
    CloseHandle(hpQV);
}
