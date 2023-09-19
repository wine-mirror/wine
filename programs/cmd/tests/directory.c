/*
 * Copyright 2023 Akihiro Sagawa
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

#define MAX_BUFFER 65536

static char stdout_buffer[MAX_BUFFER], stderr_buffer[MAX_BUFFER];
static DWORD stdout_size, stderr_size;
static char work_dir[MAX_PATH];

static void read_all_from_handle(HANDLE handle, char *buffer, DWORD *size)
{
    char bytes[4096];
    DWORD bytes_read;

    memset(buffer, 0, MAX_BUFFER);
    *size = 0;
    for (;;)
    {
        BOOL success = ReadFile(handle, bytes, sizeof(bytes), &bytes_read, NULL);
        if (!success || !bytes_read)
            break;
        if (*size + bytes_read > MAX_BUFFER)
        {
            ok(FALSE, "Insufficient buffer.\n");
            break;
        }
        memcpy(buffer + *size, bytes, bytes_read);
        *size += bytes_read;
    }
}

#define run_dir(a, b) _run_dir(__FILE__, __LINE__, a, b)
static void _run_dir(const char *file, int line, const char *commandline, int exitcode_expected)
{
    HANDLE child_stdout_write, child_stderr_write, parent_stdout_read, parent_stderr_read;
    SECURITY_ATTRIBUTES security_attributes = {0};
    PROCESS_INFORMATION process_info = {0};
    STARTUPINFOA startup_info = {0};
    DWORD exitcode;
    char cmd[256];

    security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attributes.bInheritHandle = TRUE;

    CreatePipe(&parent_stdout_read, &child_stdout_write, &security_attributes, 0);
    CreatePipe(&parent_stderr_read, &child_stderr_write, &security_attributes, 0);
    SetHandleInformation(parent_stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parent_stderr_read, HANDLE_FLAG_INHERIT, 0);

    startup_info.cb = sizeof(STARTUPINFOA);
    startup_info.hStdOutput = child_stdout_write;
    startup_info.hStdError = child_stderr_write;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    sprintf(cmd, "cmd.exe /d /c dir %s", commandline);
    CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
    CloseHandle(child_stdout_write);
    CloseHandle(child_stderr_write);

    read_all_from_handle(parent_stdout_read, stdout_buffer, &stdout_size);
    read_all_from_handle(parent_stderr_read, stderr_buffer, &stderr_size);
    CloseHandle(parent_stdout_read);
    CloseHandle(parent_stderr_read);

    WaitForSingleObject(process_info.hProcess, INFINITE);
    GetExitCodeProcess(process_info.hProcess, &exitcode);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    ok_(file, line)(exitcode == exitcode_expected, "expected exitcode %d, got %ld\n",
                    exitcode_expected, exitcode);
}

static void test_basic(void)
{
    /* no options */
    run_dir("", 0);
    ok(stdout_size > 0, "unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "unexpected stderr buffer size %ld.\n", stderr_size);

    /* if file doesn't exist, cmd.exe prints an error message to stderr. */
    run_dir("nonexistent", 1);
    ok(stdout_size > 0, "unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size > 0, "unexpected stderr buffer size %ld.\n", stderr_size);

    /* unknown option produces an error message to stderr. */
    run_dir("/*", 1);
    ok(stdout_size == 0, "unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size > 0, "unexpected stderr buffer size %ld.\n", stderr_size);

    /* errorlevel for usage is 0. But, cmd.exe's exit code is 1. */
    todo_wine run_dir("/?", 1);
    ok(stdout_size > 0, "unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "unexpected stderr buffer size %ld.\n", stderr_size);
}

START_TEST(directory)
{
    WCHAR curdir[MAX_PATH];
    BOOL ret;

    GetCurrentDirectoryW(ARRAY_SIZE(curdir), curdir);
    GetTempPathA(ARRAY_SIZE(work_dir), work_dir);
    lstrcatA(work_dir, "winetest.dir");
    ret = CreateDirectoryA(work_dir, NULL);
    ok(ret, "Failed to create %s\n", work_dir);
    ret = SetCurrentDirectoryA(work_dir);
    ok(ret, "Failed to set the working directory\n");

    test_basic();

    ret = SetCurrentDirectoryW(curdir);
    ok(ret, "Failed to restore the current directory\n");
    ret = RemoveDirectoryA(work_dir);
    ok(ret, "Failed to remove the working directory\n");
}
