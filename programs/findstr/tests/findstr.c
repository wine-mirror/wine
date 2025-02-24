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

static void write_to_handle(HANDLE handle, const char *str, int len)
{
    DWORD dummy;

    WriteFile(handle, str, len, &dummy, NULL);
}

#define run_find_stdin(a, b, c) _run_find_stdin(__FILE__, __LINE__, a, b, c)
static void _run_find_stdin(const char *file, int line, const char *commandline, const char *input,
                            int exitcode_expected)
{
    HANDLE parent_stdin_write, parent_stdout_read, parent_stderr_read;
    HANDLE child_stdin_read, child_stdout_write, child_stderr_write;
    SECURITY_ATTRIBUTES security_attributes = {0};
    PROCESS_INFORMATION process_info = {0};
    STARTUPINFOA startup_info = {0};
    char cmd[4096];
    DWORD exitcode;

    security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attributes.bInheritHandle = TRUE;

    CreatePipe(&child_stdin_read, &parent_stdin_write, &security_attributes, 0);
    CreatePipe(&parent_stdout_read, &child_stdout_write, &security_attributes, 0);
    CreatePipe(&parent_stderr_read, &child_stderr_write, &security_attributes, 0);

    SetHandleInformation(parent_stdin_write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parent_stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parent_stderr_read, HANDLE_FLAG_INHERIT, 0);

    startup_info.cb = sizeof(STARTUPINFOA);
    startup_info.hStdInput = child_stdin_read;
    startup_info.hStdOutput = child_stdout_write;
    startup_info.hStdError = child_stderr_write;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    sprintf(cmd, "findstr.exe %s", commandline);

    CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
    CloseHandle(child_stdin_read);
    CloseHandle(child_stdout_write);
    CloseHandle(child_stderr_write);

    write_to_handle(parent_stdin_write, input, lstrlenA(input));
    CloseHandle(parent_stdin_write);

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

#define run_find_file(a, b, c)  _run_find_file(__FILE__, __LINE__, a, b, c)
static void _run_find_file(const char *file, int line, const char *commandline, const char *input,
                           int exitcode_expected)
{
    char path_temp_file[MAX_PATH], path_temp_dir[MAX_PATH], commandline_new[MAX_PATH];
    HANDLE handle_file;

    GetTempPathA(ARRAY_SIZE(path_temp_dir), path_temp_dir);
    GetTempFileNameA(path_temp_dir, "", 0, path_temp_file);
    handle_file = CreateFileA(path_temp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    write_to_handle(handle_file, input, lstrlenA(input));
    CloseHandle(handle_file);

    sprintf(commandline_new, "%s %s", commandline, path_temp_file);
    _run_find_stdin(file, line, commandline_new, "", exitcode_expected);

    DeleteFileA(path_temp_file);
}

static void test_basic(void)
{
    int ret;

    /* No options */
    run_find_stdin("", "", 2);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size > 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stderr_buffer, "FINDSTR: Bad command line\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* /? */
    run_find_stdin("/?", "", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* find string in stdin */
    run_find_stdin("abc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin, LF */
    run_find_stdin("abc", "abc\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin, CR/LF */
    run_find_stdin("abc", "abc\r\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin fails */
    run_find_stdin("abc", "cba", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* find string in file */
    run_find_file("abc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file fails */
    run_find_file("abc", "cba", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* find string in stdin with space separator */
    run_find_stdin("\"abc cba\"", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin with /C: */
    run_find_stdin("/C:abc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin with /C:"abc" */
    run_find_stdin("/C:\"abc\"", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin with /C:"abc cba" fails */
    run_find_stdin("/C:\"abc cba\"", "abc", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* find string in stdin with /C: fails */
    run_find_stdin("/C:abc", "cba", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* find string in file, case insensitive */
    run_find_file("/I aBc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file, regular expression */
    run_find_file("/R abc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file, regular expression, LF */
    run_find_file("/R abc", "abc\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file, regular expression, CR/LF */
    run_find_file("/R abc", "abc\r\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin, regular expression */
    run_find_stdin("/R abc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin, regular expression, LF */
    run_find_stdin("/R abc", "abc\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in stdin, regular expression, CR/LF */
    run_find_stdin("/R abc", "abc\r\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file with /C:, regular expression */
    run_find_file("/R /C:^abc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file, regular expression, case insensitive */
    run_find_file("/I /R /C:.Bc", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc");
    ok(!ret, "Got the wrong result.\n");

    /* find string in file, regular expression, escape */
    run_find_file("/R /C:\\.bc", "abc", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    /* $ doesn't match if there's no newline */
    run_find_file("/R /C:abc$", "abc", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);

    run_find_file("/R /C:abc$", "abc\r\n", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc\r\n");
    ok(!ret, "Got the wrong result.\n");

    /* escaped . before * */
    run_find_file("/R /C:\\.*", "...", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "...");
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);

    run_find_file("/R /C:\\.*", "abc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "abc");
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);

    /* ^ after first character */
    run_find_file("/R /C:a^bc", "a^bc", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "a^bc");
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);

    /* $ before last character */
    run_find_file("/R /C:ab$c", "ab$c", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "ab$c");
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);

    run_find_file("/R .", "a", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "a");
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);

    run_find_file(".", "a", 0);
    ok(stdout_size > 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ret = strcmp(stdout_buffer, "a");
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);

    run_find_file("/L .", "a", 1);
    ok(stdout_size == 0, "Unexpected stdout buffer size %ld.\n", stdout_size);
    ok(stderr_size == 0, "Unexpected stderr buffer size %ld.\n", stderr_size);
    ok(!ret, "Got the wrong result. '%s'\n", stdout_buffer);
}

START_TEST(findstr)
{
    if (PRIMARYLANGID(GetUserDefaultUILanguage()) != LANG_ENGLISH)
    {
        skip("Tests only work with English locale.\n");
        return;
    }

    test_basic();
}
