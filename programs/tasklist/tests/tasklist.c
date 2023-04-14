/*
 * Copyright 2023 Zhiyi Zhang for CodeWeavers
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
#include <psapi.h>
#include "wine/test.h"

#define MAX_BUFFER 65536

static char stdout_buffer[MAX_BUFFER], stderr_buffer[MAX_BUFFER];
static DWORD stdout_size, stderr_size;

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

#define run_tasklist(a, b) _run_tasklist(__FILE__, __LINE__, a, b)
static void _run_tasklist(const char *file, int line, const char *commandline, int exitcode_expected)
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

    sprintf(cmd, "tasklist.exe %s", commandline);
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
    ok_(file, line)(exitcode == exitcode_expected, "Expected exitcode %d, got %ld\n",
                    exitcode_expected, exitcode);
}

static void test_basic(void)
{
    char *pos;

    /* No options */
    run_tasklist("", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, "\r\n"
                                "Image Name                     PID Session Name        Session#    Mem Usage\r\n"
                                "========================= ======== ================ =========== ============\r\n");
    ok(pos == stdout_buffer, "Got the wrong first line.\n");
    pos = strstr(stdout_buffer, "tasklist.exe");
    ok(!!pos, "Failed to list tasklist.exe.\n");

    /* /? */
    run_tasklist("/?", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
}

static void test_no_header(void)
{
    char *pos;

    /* /nh */
    run_tasklist("/nh", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, "Image Name");
    ok(!pos, "Got header.\n");
    pos = strstr(stdout_buffer, "=");
    ok(!pos, "Got header.\n");
}

static void test_format(void)
{
    char *pos;

    /* /fo */
    run_tasklist("/fo", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size > 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* /fo invalid */
    run_tasklist("/fo invalid", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size > 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* /fo TABLE */
    run_tasklist("/fo TABLE", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, "\r\n"
                                "Image Name                     PID Session Name        Session#    Mem Usage\r\n"
                                "========================= ======== ================ =========== ============\r\n");
    ok(pos == stdout_buffer, "Got the wrong first line.\n");
    pos = strstr(stdout_buffer, "tasklist.exe");
    ok(!!pos, "Failed to list tasklist.exe.\n");

    /* /fo CSV */
    run_tasklist("/fo CSV", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, "\"Image Name\",\"PID\",\"Session Name\",\"Session#\",\"Mem Usage\"");
    ok(pos == stdout_buffer, "Got the wrong first line.\n");
    pos = strstr(stdout_buffer, "\"tasklist.exe\",");
    ok(!!pos, "Failed to list tasklist.exe.\n");

    /* /fo LIST */
    run_tasklist("/fo LIST", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, "Image Name:   tasklist.exe");
    ok(!!pos, "Failed to list tasklist.exe.\n");
}

static void test_filter(void)
{
    char options[256], *pos, basename[64];
    HANDLE current_process;
    DWORD current_pid;

    current_pid = GetCurrentProcessId();
    current_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, current_pid);
    GetModuleBaseNameA(current_process, NULL, basename, ARRAY_SIZE(basename));
    CloseHandle(current_process);

    /* /fi */
    /* no value for fi */
    run_tasklist("/fi", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size > 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* IMAGENAME eq */
    sprintf(options, "/fi \"IMAGENAME eq %s\"", basename);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* IMAGENAME ne */
    sprintf(options, "/fi \"IMAGENAME ne %s\"", basename);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!pos, "Got %s.\n", basename);
    pos = strstr(stdout_buffer, "tasklist.exe");
    ok(!!pos, "Failed to list tasklist.exe.\n");

    /* PID eq */
    sprintf(options, "/fi \"PID eq %ld\"", current_pid);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* PID ne */
    sprintf(options, "/fi \"PID ne %ld\"", current_pid);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!pos, "Got %s.\n", basename);
    pos = strstr(stdout_buffer, "tasklist.exe");
    ok(!!pos, "Failed to list tasklist.exe.\n");

    /* PID gt */
    sprintf(options, "/fi \"PID gt %ld\"", current_pid - 1);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* PID lt */
    sprintf(options, "/fi \"PID lt %ld\"", current_pid + 1);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* PID ge */
    sprintf(options, "/fi \"PID ge %ld\"", current_pid);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* PID le */
    sprintf(options, "/fi \"PID le %ld\"", current_pid);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* IMAGENAME eq + PID eq */
    sprintf(options, "/fi \"IMAGENAME eq %s\" /fi \"PID eq %ld\"", basename, current_pid);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!!pos, "Failed to list %s.\n", basename);

    /* IMAGENAME eq + PID eq with wrong PID */
    sprintf(options, "/fi \"IMAGENAME eq %s\" /fi \"PID eq %ld\"", basename, current_pid + 1);
    run_tasklist(options, 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    pos = strstr(stdout_buffer, basename);
    ok(!pos, "Got %s.\n", basename);
}

START_TEST(tasklist)
{
    if (PRIMARYLANGID(GetUserDefaultUILanguage()) != LANG_ENGLISH)
    {
        skip("Tests only work with English locale.\n");
        return;
    }

    test_basic();
    test_no_header();
    test_format();
    test_filter();
}
