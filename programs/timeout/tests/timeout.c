/*
 * Copyright 2024 Michele Dionisio <michele.dionisio@gmail.com>
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

#include <windows.h>
#include "wine/test.h"

static DWORD run_timeout_stdin(const char *commandline)
{
    PROCESS_INFORMATION process_info = {0};
    STARTUPINFOA startup_info;
    char cmd[4096];
    DWORD exitcode;

    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = NULL;
    startup_info.hStdError = NULL;

    sprintf(cmd, "timeout.exe %s", commandline);

    CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);

    WaitForSingleObject(process_info.hProcess, INFINITE);
    GetExitCodeProcess(process_info.hProcess, &exitcode);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    return exitcode;
}

static void test_basic(void)
{
    DWORD exit_code;

    /* No options */
    exit_code = run_timeout_stdin("");
    ok(exit_code == 1, "Unexpected exit_code %lu\n", exit_code);

    /* /? */
    exit_code = run_timeout_stdin("/?");
    ok(exit_code == 0, "Unexpected exit_code %lu\n", exit_code);

    /* /T 1 /NOBREAK */
    exit_code = run_timeout_stdin("/T 1 /NOBREAK");
    ok(exit_code == 0, "Unexpected exit_code %lu\n", exit_code);

    /* /T 1 */
    exit_code = run_timeout_stdin("/T 1");
    ok(exit_code == 0, "Unexpected exit_code %lu\n", exit_code);

    /* 1 /NOBREAK */
    exit_code = run_timeout_stdin("1 /NOBREAK");
    ok(exit_code == 0, "Unexpected exit_code %lu\n", exit_code);

    /* /T 1ab /NOBREAK */
    exit_code = run_timeout_stdin("/T 1ab /NOBREAK");
    ok(exit_code == 1, "Unexpected exit_code %lu\n", exit_code);

    /* /T -3 /NOBREAK */
    exit_code = run_timeout_stdin("/T -3 /NOBREAK");
    ok(exit_code == 1, "Unexpected exit_code %lu\n", exit_code);

    /* /T 10000000 /NOBREAK */
    exit_code = run_timeout_stdin("/T 10000000 /NOBREAK");
    ok(exit_code == 1, "Unexpected exit_code %lu\n", exit_code);
}

#define run_timeout_ctrlc(a, b) _run_timeout_ctrlc(__FILE__, __LINE__, a, b)
static void _run_timeout_ctrlc(const char *file, int line, const char *option, DWORD exitcode_expected)
{
    PROCESS_INFORMATION process_info = {0};
    STARTUPINFOA startup_info;
    char cmd[4096];
    DWORD status, exitcode;
    DWORD64 tick_count;
    BOOL ret;

    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = NULL;
    startup_info.hStdError = NULL;

    SetConsoleCtrlHandler(NULL, FALSE);
    sprintf(cmd, "timeout.exe /T 12 %s", option);
    ret = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
    ok(ret, "CreateProcessA failed: %lu\n", GetLastError());

    /* wait for process to be started */
    status = WaitForSingleObject(process_info.hProcess, 2000);
    ok_(file, line)(status == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx (expecting WAIT_TIMEOUT)\n", status);

    tick_count = GetTickCount64();

    SetConsoleCtrlHandler(NULL, TRUE);
    ret = GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
    ok_(file, line)(ret, "GenerateConsoleCtrlEvent failed: %lu\n", GetLastError());

    status = WaitForSingleObject(process_info.hProcess, INFINITE);
    ok_(file, line)(status == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx (expecting WAIT_OBJECT_0)\n", status);

    tick_count = GetTickCount64() - tick_count;
    ok_(file, line)(tick_count < 2000, "Process has not been stopped by ctrl-c\n");

    ret = GetExitCodeProcess(process_info.hProcess, &exitcode);
    ok_(file, line)(ret, "GetExitCodeProcess failed\n");

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    ok_(file, line)(exitcode == exitcode_expected, "Expected exitcode %ld, got %lx\n",
                    exitcode_expected, exitcode);
}

static void test_ctrlc(int argc, char** argv)
{
    if (!strcmp(argv[3], "normal"))
        run_timeout_ctrlc("", STATUS_CONTROL_C_EXIT);
    else if (!strcmp(argv[3], "nobreak"))
        run_timeout_ctrlc("/nobreak", 1);
    else
        ok(0, "Unknown test %s.\n", argv[3]);
}

static void test_ctrlc_intermediate(int argc, char** argv, const char* name)
{
    char path_name[MAX_PATH];
    STARTUPINFOA startup;
    PROCESS_INFORMATION info;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(path_name, "%s %s %s %s", argv[0], argv[1], "ctrlc_intermediate", name);
    ok(CreateProcessA(NULL, path_name, NULL, NULL, FALSE,
                      CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
                      NULL, NULL, &startup, &info),
        "CreateProcess failed.\n");
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
}

START_TEST(timeout)
{
    int argc;
    char** argv;
    BOOL ret;

    argc = winetest_get_mainargs(&argv);
    if(argc == 4 && !strcmp(argv[2], "ctrlc_intermediate"))
    {
        test_ctrlc(argc, argv);
        return;
    }

    /* always run on a separate console, so that:
     * - we're sure to have a valid console input handle (no Wine unix console...)
     * - we can send ctrl-c events without interfering with parent (winetest...)
     */
    ret = FreeConsole();
    ok(ret, "FreeConsole() failed\n");
    SetStdHandle(STD_INPUT_HANDLE, NULL); /* will force reallocation of std handle in AllocConsole() */
    SetStdHandle(STD_OUTPUT_HANDLE, NULL); /* will force reallocation of std handle in AllocConsole() */
    SetStdHandle(STD_ERROR_HANDLE, NULL); /* will force reallocation of std handle in AllocConsole() */
    ret = AllocConsole();
    ok(ret, "AllocConsole() failed\n");

    if (broken(run_timeout_stdin("/T 0") == 1)) /* Win7 */
    {
        win_skip("Skipping tests on Windows 7\n");
        return;
    }

    test_basic();
    test_ctrlc_intermediate(argc, argv, "normal");
    test_ctrlc_intermediate(argc, argv, "nobreak");
}
