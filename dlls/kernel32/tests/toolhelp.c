/*
 * Toolhelp
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "tlhelp32.h"
#include "wine/test.h"
#include "winuser.h"
#include "winternl.h"
#include "winnls.h"

static char     selfname[MAX_PATH];

/* Some functions are only in later versions of kernel32.dll */
static HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
static BOOL (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
static BOOL (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);
static BOOL (WINAPI *pProcess32First)(HANDLE, LPPROCESSENTRY32);
static BOOL (WINAPI *pProcess32Next)(HANDLE, LPPROCESSENTRY32);
static BOOL (WINAPI *pThread32First)(HANDLE, LPTHREADENTRY32);
static BOOL (WINAPI *pThread32Next)(HANDLE, LPTHREADENTRY32);
static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, void *, ULONG, ULONG *);

/* 1 minute should be more than enough */
#define WAIT_TIME       (60 * 1000)
/* Specify the number of simultaneous threads to test */
#define NUM_THREADS 4

static DWORD WINAPI sub_thread(void* pmt)
{
    DWORD w = WaitForSingleObject(pmt, WAIT_TIME);
    return w;
}

/******************************************************************
 *		init
 *
 * generates basic information like:
 *      selfname:       the way to reinvoke ourselves
 * returns:
 *      -1      on error
 *      0       if parent
 *      doesn't return if child
 */
static int     init(void)
{
    int                 argc;
    char**              argv;
    HANDLE              ev1, ev2, ev3, hThread;
    DWORD               w, tid;

    argc = winetest_get_mainargs( &argv );
    strcpy(selfname, argv[0]);

    switch (argc)
    {
    case 2: /* the test program */
        return 0;
    case 4: /* the sub-process */
        ev1 = (HANDLE)(INT_PTR)atoi(argv[2]);
        ev2 = (HANDLE)(INT_PTR)atoi(argv[3]);
        ev3 = CreateEventW(NULL, FALSE, FALSE, NULL);

        if (ev3 == NULL) ExitProcess(WAIT_ABANDONED);
        hThread = CreateThread(NULL, 0, sub_thread, ev3, 0, &tid);
        if (hThread == NULL) ExitProcess(WAIT_ABANDONED);
        if (!LoadLibraryA("shell32.dll")) ExitProcess(WAIT_ABANDONED);

        /* signal init of sub-process is done */
        SetEvent(ev1);
        /* wait for parent to have done all its queries */
        w = WaitForSingleObject(ev2, WAIT_TIME);
        if (w != WAIT_OBJECT_0) ExitProcess(w);
        /* signal sub-thread to terminate */
        SetEvent(ev3);
        w = WaitForSingleObject(hThread, WAIT_TIME);
        if (w != WAIT_OBJECT_0) ExitProcess(w);
        GetExitCodeThread(hThread, &w);
        ExitProcess(w);
    default:
        return -1;
    }
}

static void test_process(DWORD curr_pid, DWORD sub_pcs_pid)
{
    HANDLE              hSnapshot;
    PROCESSENTRY32      pe;
    MODULEENTRY32       me;
    unsigned            found = 0;
    int                 num = 0;
    int			childpos = -1;

    hSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    /* check that this current process is enumerated */
    pe.dwSize = sizeof(pe);
    if (pProcess32First( hSnapshot, &pe ))
    {
        do
        {
            if (pe.th32ProcessID == curr_pid) found++;
            if (pe.th32ProcessID == sub_pcs_pid) { childpos = num; found++; }
            trace("PID=%lx %s\n", pe.th32ProcessID, pe.szExeFile);
            num++;
        } while (pProcess32Next( hSnapshot, &pe ));
    }
    ok(found == 2, "couldn't find self and/or sub-process in process list\n");

    /* check that first really resets the enumeration */
    found = 0;
    if (pProcess32First( hSnapshot, &pe ))
    {
        do
        {
            if (pe.th32ProcessID == curr_pid) found++;
            if (pe.th32ProcessID == sub_pcs_pid) found++;
            trace("PID=%lx %s\n", pe.th32ProcessID, pe.szExeFile);
            num--;
        } while (pProcess32Next( hSnapshot, &pe ));
    }
    ok(found == 2, "couldn't find self and/or sub-process in process list\n");
    ok(!num, "mismatch in counting\n");

    /* one broken program does Process32First() and does not expect anything
     * interesting to be there, especially not the just forked off child */
    ok (childpos !=0, "child is not expected to be at position 0.\n");

    me.dwSize = sizeof(me);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");

    CloseHandle(hSnapshot);
    ok(!pProcess32First( hSnapshot, &pe ), "shouldn't return a process\n");
}

static DWORD WINAPI get_id_thread(void* curr_pid)
{
    HANDLE              hSnapshot;
    THREADENTRY32       te;
    HANDLE              ev, threads[NUM_THREADS];
    DWORD               thread_ids[NUM_THREADS];
    DWORD               thread_traversed[NUM_THREADS];
    DWORD               tid, first_tid = 0;
    BOOL                found = FALSE;
    int                 i, matched_idx = -1;
    ULONG               buf_size = 0;
    NTSTATUS            status;
    BYTE*               pcs_buffer = NULL;
    DWORD               pcs_offset = 0;
    SYSTEM_PROCESS_INFORMATION* spi = NULL;

    ev = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(ev != NULL, "Cannot create event\n");

    for (i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = CreateThread(NULL, 0, sub_thread, ev, 0, &tid);
        ok(threads[i] != NULL, "Cannot create thread\n");
        thread_ids[i] = tid;
    }

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    /* Check that this current process is enumerated */
    te.dwSize = sizeof(te);
    ok(pThread32First(hSnapshot, &te), "Thread cannot traverse\n");
    do
    {
        if (found)
        {
            if (te.th32OwnerProcessID != PtrToUlong(curr_pid)) break;

            if (matched_idx >= 0)
            {
                thread_traversed[matched_idx++] = te.th32ThreadID;
                if (matched_idx >= NUM_THREADS) break;
            }
            else if (thread_ids[0] == te.th32ThreadID)
            {
                matched_idx = 0;
                thread_traversed[matched_idx++] = te.th32ThreadID;
            }
        }
        else if (te.th32OwnerProcessID == PtrToUlong(curr_pid))
        {
            found = TRUE;
            first_tid = te.th32ThreadID;
        }
    }
    while (pThread32Next(hSnapshot, &te));

    ok(found, "Couldn't find self and/or sub-process in process list\n");

    /* Check if the thread order is strictly consistent */
    found = FALSE;
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (thread_traversed[i] != thread_ids[i])
        {
            found = TRUE;
            break;
        }
        /* Reset data */
        thread_traversed[i] = 0;
    }
    ok(found == FALSE, "The thread order is not strictly consistent\n");

    /* Determine the order by NtQuerySystemInformation function */

    while ((status = NtQuerySystemInformation(SystemProcessInformation,
            pcs_buffer, buf_size, &buf_size)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        free(pcs_buffer);
        pcs_buffer = malloc(buf_size);
    }
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    found = FALSE;
    matched_idx = -1;

    do
    {
        spi = (SYSTEM_PROCESS_INFORMATION*)&pcs_buffer[pcs_offset];
        if (spi->UniqueProcessId == curr_pid)
        {
            found = TRUE;
            break;
        }
        pcs_offset += spi->NextEntryOffset;
    } while (spi->NextEntryOffset != 0);

    ok(found && spi, "No process found\n");
    for (i = 0; i < spi->dwThreadCount; i++)
    {
        tid = HandleToULong(spi->ti[i].ClientId.UniqueThread);
        if (matched_idx > 0)
        {
            thread_traversed[matched_idx++] = tid;
            if (matched_idx >= NUM_THREADS) break;
        }
        else if (tid == thread_ids[0])
        {
            matched_idx = 0;
            thread_traversed[matched_idx++] = tid;
        }
    }
    free(pcs_buffer);

    ok(matched_idx > 0, "No thread id match found\n");

    found = FALSE;
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (thread_traversed[i] != thread_ids[i])
        {
            found = TRUE;
            break;
        }
    }
    ok(found == FALSE, "wrong order in NtQuerySystemInformation function\n");

    SetEvent(ev);
    WaitForMultipleObjects( NUM_THREADS, threads, TRUE, WAIT_TIME );
    for (i = 0; i < NUM_THREADS; i++)
        CloseHandle(threads[i]);
    CloseHandle(ev);
    CloseHandle(hSnapshot);

    return first_tid;
}

static void test_main_thread(DWORD curr_pid, DWORD main_tid)
{
    HANDLE              thread;
    DWORD               tid = 0;
    int                 error;

    /* Check that the main thread id is first one in this thread. */
    tid = get_id_thread(ULongToPtr(curr_pid));
    ok(tid == main_tid, "The first thread id returned is not the main thread id\n");

    /* Check that the main thread id is first one in other thread. */
    thread = CreateThread(NULL, 0, get_id_thread, ULongToPtr(curr_pid), 0, NULL);
    error = WaitForSingleObject(thread, WAIT_TIME);
    ok(error == WAIT_OBJECT_0, "Thread did not complete within timelimit\n");

    ok(GetExitCodeThread(thread, &tid), "Could not retrieve exit code\n");
    ok(tid == main_tid, "The first thread id returned is not the main thread id\n");
}

static void test_thread(DWORD curr_pid, DWORD sub_pcs_pid)
{
    HANDLE              hSnapshot;
    THREADENTRY32       te;
    MODULEENTRY32       me;
    unsigned            curr_found = 0;
    unsigned            sub_found = 0;

    hSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    /* check that this current process is enumerated */
    te.dwSize = sizeof(te);
    if (pThread32First( hSnapshot, &te ))
    {
        do
        {
            if (te.th32OwnerProcessID == curr_pid) curr_found++;
            if (te.th32OwnerProcessID == sub_pcs_pid) sub_found++;
            if (winetest_debug > 1)
                trace("PID=%lx TID=%lx %ld\n", te.th32OwnerProcessID, te.th32ThreadID, te.tpBasePri);
        } while (pThread32Next( hSnapshot, &te ));
    }
    ok(curr_found, "couldn't find self in thread list\n");
    ok(sub_found >= 2, "couldn't find sub-process threads in thread list\n");

    /* check that first really resets enumeration */
    curr_found = 0;
    sub_found = 0;
    if (pThread32First( hSnapshot, &te ))
    {
        do
        {
            if (te.th32OwnerProcessID == curr_pid) curr_found++;
            if (te.th32OwnerProcessID == sub_pcs_pid) sub_found++;
            if (winetest_debug > 1)
                trace("PID=%lx TID=%lx %ld\n", te.th32OwnerProcessID, te.th32ThreadID, te.tpBasePri);
        } while (pThread32Next( hSnapshot, &te ));
    }
    ok(curr_found, "couldn't find self in thread list\n");
    ok(sub_found >= 2, "couldn't find sub-process threads in thread list\n");

    me.dwSize = sizeof(me);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");

    CloseHandle(hSnapshot);
    ok(!pThread32First( hSnapshot, &te ), "shouldn't return a thread\n");
}

struct expected_module {
    const char *module;

    /*
     * 0 = C:\windows\system32\
     * 1 = C:\windows\syswow64\
     * 2 = don't check
     */
    int wow64;
};

static struct expected_module curr_expected_modules[] =
{
    {"kernel32_test.exe", 2},
    {"kernel32.dll"},
    {"ntdll.dll"},
};

static struct expected_module sub_expected_modules[] =
{
    {"kernel32_test.exe", 2},
    {"kernel32.dll"},
    {"shell32.dll"},
    {"ntdll.dll"},
};

static struct expected_module msinfo32_32_expected_modules[] =
{
    {"msinfo32.exe", 1},
    {"kernel32.dll", 1},
    {"shell32.dll", 1},
    {"ntdll.dll", 1},
    {"ntdll.dll"},
    {"wow64.dll"},
    {"wow64win.dll"},
    {"wow64cpu.dll"},
};

static struct expected_module msinfo32_64_expected_modules[] =
{
    {"msinfo32.exe", 1},
    {"ntdll.dll"},
    {"wow64.dll"},
    {"wow64win.dll"},
    {"wow64cpu.dll"},
};

static char syswow64[MAX_PATH], system32[MAX_PATH];
static int syswow64_len, system32_len;

static HANDLE create_toolhelp_snapshot( DWORD flags, DWORD pid )
{
    HANDLE hSnapshot;
    int retries = winetest_platform_is_wine ? 1 : 5;

    do
    {
        hSnapshot = pCreateToolhelp32Snapshot( flags, pid );
        if (hSnapshot != INVALID_HANDLE_VALUE || GetLastError() != ERROR_BAD_LENGTH)
            break;
    } while (--retries);

    return hSnapshot;
}

static BOOL match_module(const MODULEENTRY32 *module, const struct expected_module *pattern)
{
    if (lstrcmpiA(module->szModule, pattern->module)) return FALSE;
    if (pattern->wow64 == 2) return TRUE;
    if (pattern->wow64 == 1)
    {
        if (lstrlenA(module->szExePath) < syswow64_len) return FALSE;
        return CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, module->szExePath, syswow64_len,
                              syswow64, syswow64_len) == CSTR_EQUAL;
    }
    if (lstrlenA(module->szExePath) < system32_len) return FALSE;
    return CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, module->szExePath, system32_len,
                          system32, system32_len) == CSTR_EQUAL;
}

static const BOOL is_win64 = sizeof(void*) > sizeof(int);

static BOOL is_old_wow_pid(DWORD pid)
{
    PROCESS_BASIC_INFORMATION pbi;
    PPEB_LDR_DATA pLdrData = NULL;
    HANDLE hProcess = OpenProcess( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid );
    NTSTATUS status = NtQueryInformationProcess( hProcess, ProcessBasicInformation, &pbi, sizeof(pbi),
                                                 NULL );
    BOOL ret = FALSE;

    if (status != STATUS_SUCCESS) return FALSE;

    if (!IsWow64Process( hProcess, &ret ) || !ret) return FALSE;

    if (!ReadProcessMemory( hProcess, &pbi.PebBaseAddress->LdrData, &pLdrData, sizeof(pLdrData), NULL ))
    {
        CloseHandle( hProcess );
        return FALSE;
    }

    ret = !pLdrData;
    CloseHandle( hProcess );
    return ret;
}

/* Test to ensure no module is returned when TH32CS_SNAPMODULE32 is set without TH32CS_SNAPMODULE */
static void test_module32_only(DWORD pid)
{
    HANDLE hSnapshot;
    MODULEENTRY32 me;

    hSnapshot = create_toolhelp_snapshot( TH32CS_SNAPMODULE32, pid );
    todo_wine ok(hSnapshot != INVALID_HANDLE_VALUE, "Cannot create snapshot\n");
    if (hSnapshot == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("Cannot create snapshot handle\n");
        return;
    }

    ok(!pModule32First( hSnapshot, &me ), "Got unexpected module entry\n");
    CloseHandle( hSnapshot );
}

static void test_module(DWORD pid, struct expected_module expected[], unsigned num_expected, BOOL module32)
{
    const BOOL          is_old_wow = is_old_wow_pid(pid);
    HANDLE              hSnapshot;
    PROCESSENTRY32      pe;
    THREADENTRY32       te;
    MODULEENTRY32       me;
    DWORD               snapshot_flags = TH32CS_SNAPMODULE | (module32 ? TH32CS_SNAPMODULE32 : 0);
    unsigned            found[32];
    unsigned            i;
    int                 expected_main_exe_count = is_win64 && module32 ? 2 : 1;
    int                 num = 0;

    ok(ARRAY_SIZE(found) >= num_expected, "Internal: bump found[] size\n");

    hSnapshot = create_toolhelp_snapshot( snapshot_flags, pid );
    ok(hSnapshot != INVALID_HANDLE_VALUE, "Cannot create snapshot %#lx\n", GetLastError());
    if (hSnapshot == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("CreateToolhelp32Snapshot doesn't support requested flags\n");
        return;
    }

    for (i = 0; i < num_expected; i++) found[i] = 0;
    me.dwSize = sizeof(me);
    if (pModule32First( hSnapshot, &me ))
    {
        do
        {
            trace("PID=%lx base=%p size=%lx %s %s\n",
                  me.th32ProcessID, me.modBaseAddr, me.modBaseSize, me.szExePath, me.szModule);
            ok(me.th32ProcessID == pid, "wrong returned process id\n");
            for (i = 0; i < num_expected; i++)
                if (match_module(&me, &expected[i])) found[i]++;
            num++;
        } while (pModule32Next( hSnapshot, &me ));
    }
    todo_if(winetest_platform_is_wine && module32 && is_win64)
    ok(found[0] == expected_main_exe_count, "Main exe was found %d time(s)\n", found[0]);
    for (i = 1; i < num_expected; i++)
        todo_if((winetest_platform_is_wine && expected[i].wow64 == 1) || (is_old_wow && !expected[i].wow64))
        ok(found[i] == 1, "Module %s is %s\n",
           expected[i].module, found[i] ? "listed more than once" : "not listed");

    /* check that first really resets the enumeration */
    for (i = 0; i < num_expected; i++) found[i] = 0;
    me.dwSize = sizeof(me);
    if (pModule32First( hSnapshot, &me ))
    {
        do
        {
            trace("PID=%lx base=%p size=%lx %s %s\n",
                  me.th32ProcessID, me.modBaseAddr, me.modBaseSize, me.szExePath, me.szModule);
            for (i = 0; i < num_expected; i++)
                if (match_module(&me, &expected[i])) found[i]++;
            num--;
        } while (pModule32Next( hSnapshot, &me ));
    }
    todo_if(winetest_platform_is_wine && module32 && is_win64)
    ok(found[0] == expected_main_exe_count, "Main exe was found %d time(s)\n", found[0]);
    for (i = 1; i < num_expected; i++)
        todo_if((winetest_platform_is_wine && expected[i].wow64 == 1) || (is_old_wow && !expected[i].wow64))
        ok(found[i] == 1, "Module %s is %s\n",
           expected[i].module, found[i] ? "listed more than once" : "not listed");
    ok(!num, "mismatch in counting\n");

    pe.dwSize = sizeof(pe);
    ok(!pProcess32First( hSnapshot, &pe ), "shouldn't return a process\n");

    te.dwSize = sizeof(te);
    ok(!pThread32First( hSnapshot, &te ), "shouldn't return a thread\n");

    CloseHandle(hSnapshot);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");
}

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

START_TEST(toolhelp)
{
    DWORD               pid = GetCurrentProcessId();
    int                 r;
    char                *p, module[MAX_PATH];
    char                buffer[MAX_PATH + 21];
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION	info, info32 = {0};
    STARTUPINFOA	startup, startup32 = {0};
    HANDLE              ev1, ev2;
    DWORD               w;
    HANDLE              hkernel32 = GetModuleHandleA("kernel32");
    HANDLE              hntdll = GetModuleHandleA("ntdll.dll");
    BOOL                ret;

    if (is_win64)
    {
        syswow64_len = GetSystemWow64DirectoryA(syswow64, ARRAY_SIZE(syswow64));
        ok(syswow64_len, "Can't GetSystemWow64DirectoryA, %#lx\n", GetLastError());
    }
    system32_len = GetSystemDirectoryA(system32, ARRAY_SIZE(system32));
    ok(system32_len, "Can't GetSystemDirectoryA, %#lx\n", GetLastError());

    pCreateToolhelp32Snapshot = (VOID *) GetProcAddress(hkernel32, "CreateToolhelp32Snapshot");
    pModule32First = (VOID *) GetProcAddress(hkernel32, "Module32First");
    pModule32Next = (VOID *) GetProcAddress(hkernel32, "Module32Next");
    pProcess32First = (VOID *) GetProcAddress(hkernel32, "Process32First");
    pProcess32Next = (VOID *) GetProcAddress(hkernel32, "Process32Next");
    pThread32First = (VOID *) GetProcAddress(hkernel32, "Thread32First");
    pThread32Next = (VOID *) GetProcAddress(hkernel32, "Thread32Next");
    pNtQuerySystemInformation = (VOID *) GetProcAddress(hntdll, "NtQuerySystemInformation");

    if (!pCreateToolhelp32Snapshot || 
        !pModule32First || !pModule32Next ||
        !pProcess32First || !pProcess32Next ||
        !pThread32First || !pThread32Next ||
        !pNtQuerySystemInformation)
    {
        win_skip("Needed functions are not available, most likely running on Windows NT\n");
        return;
    }

    r = init();
    ok(r == 0, "Basic init of sub-process test\n");
    if (r != 0) return;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    ev1 = CreateEventW(&sa, FALSE, FALSE, NULL);
    ev2 = CreateEventW(&sa, FALSE, FALSE, NULL);
    ok (ev1 != NULL && ev2 != NULL, "Couldn't create events\n");
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(buffer, "%s toolhelp %Iu %Iu", selfname, (DWORD_PTR)ev1, (DWORD_PTR)ev2);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to be initialized */
    w = WaitForSingleObject(ev1, WAIT_TIME);
    ok(w == WAIT_OBJECT_0, "Failed to wait on sub-process startup\n");

    GetModuleFileNameA( 0, module, sizeof(module) );
    if (!(p = strrchr( module, '\\' ))) p = module;
    else p++;
    curr_expected_modules[0].module = p;
    sub_expected_modules[0].module = p;

    test_process(pid, info.dwProcessId);
    test_thread(pid, info.dwProcessId);
    test_main_thread(pid, GetCurrentThreadId());
    test_module(pid, curr_expected_modules, ARRAY_SIZE(curr_expected_modules), FALSE);
    test_module(info.dwProcessId, sub_expected_modules, ARRAY_SIZE(sub_expected_modules), FALSE);
    test_module32_only(pid);
    test_module32_only(info.dwProcessId);
    if (is_win64)
    {
        lstrcpyA(buffer, syswow64);
        strcat(buffer, "\\msinfo32.exe");
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup32, &info32);
        if (ret)
        {
            ret = wait_process_window_visible(info32.hProcess, info32.dwProcessId, 5000);
            ok(ret, "wait timed out\n");

            trace("testing 32bit\n");
            test_module(info32.dwProcessId, msinfo32_32_expected_modules,
                ARRAY_SIZE(msinfo32_32_expected_modules), TRUE);
            test_module(info32.dwProcessId, msinfo32_64_expected_modules,
                ARRAY_SIZE(msinfo32_64_expected_modules), FALSE);
            test_module32_only(pid);
            TerminateProcess(info32.hProcess, 0);
        }
        else
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                skip("Skip wow64 test on non compatible platform\n");
            else
                ok(ret, "wow64 CreateProcess failed: %#lx\n", GetLastError());
        }
    }
    else
    {
        /* what happens if TH32CS_SNAPMODULE32 is set on 32bit? */
        test_module(info.dwProcessId, sub_expected_modules, ARRAY_SIZE(sub_expected_modules), TRUE);
    }

    SetEvent(ev2);
    wait_child_process(&info);
}
