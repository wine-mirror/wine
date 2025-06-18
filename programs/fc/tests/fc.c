/*
 * Copyright 2024 Hans Leidekker for CodeWeavers
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
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/test.h"

#define MAX_BUF 512
static char tmpfile1[MAX_PATH], tmpfile2[MAX_PATH], stdout_buf[MAX_BUF], stderr_buf[MAX_BUF];
static DWORD stdout_size, stderr_size;

static void read_all_from_handle( HANDLE handle, char *buf, DWORD *size )
{
    char tmp[32];
    DWORD bytes_read;

    memset( buf, 0, MAX_BUF );
    *size = 0;
    for (;;)
    {
        BOOL success = ReadFile( handle, tmp, sizeof(tmp), &bytes_read, NULL );
        if (!success || !bytes_read) break;
        if (*size + bytes_read > MAX_BUF)
        {
            ok( FALSE, "insufficient buffer\n" );
            break;
        }
        memcpy( buf + *size, tmp, bytes_read );
        *size += bytes_read;
    }
}

static void create_temp_file( char *tmpfile, const char *prefix )
{
    char tmpdir[MAX_PATH];
    HANDLE handle;

    GetTempPathA( sizeof(tmpdir), tmpdir );
    GetTempFileNameA( tmpdir, prefix, 0, tmpfile );
    handle = CreateFileA( tmpfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    CloseHandle( handle );
}

static void write_to_file( const char *filename, const char *str )
{
    DWORD dummy;
    HANDLE handle = CreateFileA( filename, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, 0, NULL );
    WriteFile( handle, str, strlen(str), &dummy, NULL );
    CloseHandle( handle );
}

static DWORD run_fc( const char *data1, const char *data2 )
{
    HANDLE parent_stdout_read, parent_stderr_read, child_stdout_write, child_stderr_write;
    SECURITY_ATTRIBUTES security_attrs = {0};
    PROCESS_INFORMATION process_info = {0};
    STARTUPINFOA startup_info = {0};
    char cmd[4096];
    DWORD exitcode;
    BOOL ret;

    write_to_file( tmpfile1, data1 );
    write_to_file( tmpfile2, data2 );

    security_attrs.nLength = sizeof(security_attrs);
    security_attrs.bInheritHandle = TRUE;

    CreatePipe( &parent_stdout_read, &child_stdout_write, &security_attrs, 0 );
    CreatePipe( &parent_stderr_read, &child_stderr_write, &security_attrs, 0 );

    SetHandleInformation( parent_stdout_read, HANDLE_FLAG_INHERIT, 0 );
    SetHandleInformation( parent_stderr_read, HANDLE_FLAG_INHERIT, 0 );

    startup_info.cb = sizeof(startup_info);
    startup_info.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
    startup_info.hStdOutput = child_stdout_write;
    startup_info.hStdError = child_stderr_write;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    sprintf( cmd, "fc.exe %s %s", tmpfile1, tmpfile2 );

    ret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info );
    ok( ret, "got %lu\n", GetLastError() );
    CloseHandle( child_stdout_write );
    CloseHandle( child_stderr_write );

    read_all_from_handle( parent_stdout_read, stdout_buf, &stdout_size );
    read_all_from_handle( parent_stderr_read, stderr_buf, &stderr_size );
    CloseHandle( parent_stdout_read );
    CloseHandle( parent_stderr_read );

    WaitForSingleObject( process_info.hProcess, INFINITE );
    ret = GetExitCodeProcess( process_info.hProcess, &exitcode );
    ok( ret, "got %lu\n", GetLastError() );
    CloseHandle( process_info.hProcess );
    CloseHandle( process_info.hThread );
    return exitcode;
}

static BOOL check_nodiff( const char *output )
{
    const char *ptr = output;

    if (memcmp( ptr, "Comparing files ", sizeof("Comparing files ") - 1 )) return FALSE;
    ptr = strchr( ptr, '\n' ) + 1;
    if (memcmp( ptr, "FC: no differences encountered\r\n", sizeof("FC: no differences encountered\r\n") - 1 ))
        return FALSE;
    ptr = strchr( ptr, '\n' ) + 1;
    if (*ptr++ != '\r' || *ptr++ != '\n' || *ptr) return FALSE;
    return TRUE;
}

static BOOL check_diff( const char *output, const char *str1, const char *str2, const char *str3, const char *str4 )
{
    const char *ptr = output;

    if (memcmp( ptr, "Comparing files ", sizeof("Comparing files ") - 1 )) return FALSE;
    ptr = strchr( ptr, '\n' ) + 1;
    if (memcmp( ptr, "***** ", sizeof("***** ") - 1 )) return FALSE;
    ptr = strchr( ptr, '\n' ) + 1;
    while (*str1)
    {
        if (memcmp( ptr, str1, strlen(str1) )) return FALSE;
        ptr = strchr( ptr, '\n' ) + 1;
        str1 += strlen(str1) + 1;
    }
    if (memcmp( ptr, "***** ", sizeof("***** ") - 1 )) return FALSE;
    ptr = strchr( ptr, '\n' ) + 1;
    while (*str2)
    {
        if (memcmp( ptr, str2, strlen(str2) )) return FALSE;
        ptr = strchr( ptr, '\n' ) + 1;
        str2 += strlen(str2) + 1;
    }
    if (memcmp( ptr, "*****", sizeof("*****") - 1 )) return FALSE;
    ptr = strchr( ptr, '\n' ) + 1;
    if (*ptr++ != '\r' || *ptr++ != '\n') return FALSE;
    if (str3)
    {
        ptr = strchr( ptr, '\n' ) + 1;
        if (memcmp( ptr, "***** ", sizeof("***** ") - 1 )) return FALSE;
        ptr = strchr( ptr, '\n' ) + 1;
        while (*str3)
        {
            if (memcmp( ptr, str3, strlen(str3) )) return FALSE;
            ptr = strchr( ptr, '\n' ) + 1;
            str3 += strlen(str3) + 1;
        }
        if (memcmp( ptr, "*****", sizeof("*****") - 1 )) return FALSE;
        ptr = strchr( ptr, '\n' ) + 1;
        while (*str4)
        {
            if (memcmp( ptr, str4, strlen(str4) )) return FALSE;
            ptr = strchr( ptr, '\n' ) + 1;
            str4 += strlen(str4) + 1;
        }
        if (*ptr++ != '\r' || *ptr++ != '\n') return FALSE;
    }
    if (*ptr) return FALSE;
    return TRUE;
}

static void test_diff_output(void)
{
    struct
    {
        int todo;
        const char *data1;
        const char *data2;
        const char *diff1;
        const char *diff2;
        const char *diff3;
        const char *diff4;
    } tests[] =
    {
        /*  0 */ { 0, "", "" },
        /*  1 */ { 0, "", "apple", "\0", "apple\0" },
        /*  2 */ { 0, "apple", "", "apple\0", "\0" },
        /*  3 */ { 0, "apple", "apple" },
        /*  4 */ { 0, "apple", "orange", "apple\0", "orange\0" },
        /*  5 */ { 0, "apple\nmango", "apple", "mango\0", "\0" },
        /*  6 */ { 0, "apple", "apple\nmango", "\0", "mango\0" },
        /*  7 */ { 0, "apple\nmango", "apple\nkiwi", "apple\0mango\0", "apple\0kiwi\0" },
        /*  8 */ { 0, "apple\nmango", "kiwi\nmango", "apple\0mango\0", "kiwi\0mango\0" },
        /*  9 */ { 0, "apple\nmango\nkiwi", "apple\nlemon\nkiwi", "apple\0mango\0kiwi\0", "apple\0lemon\0kiwi\0" },
        /* 10 */ { 1, "apple\nmango\nkiwi\nlemon", "apple\nlemon\nkiwi\ncherry", "apple\0mango\0kiwi\0lemon\0", "apple\0lemon\0",
                      "kiwi\0cherry\0", "\0" },
        /* 11 */ { 0, "apple\nmango\nkiwi\nlemon", "apple\nmango\nkiwi\ncherry", "kiwi\0lemon\0", "kiwi\0cherry\0" },
        /* 12 */ { 0, "apple\t", "apple", "apple\0", "apple\0" },
        /* 13 */ { 0, "apple\n", "apple" },
        /* 14 */ { 0, "apple", "apple\r\nmango", "\0", "mango\0" },
    };
    UINT i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        BOOL ret;
        DWORD exitcode;

        stdout_size = stderr_size = 0;
        exitcode = run_fc( tests[i].data1, tests[i].data2 );
        ok( stdout_size, "got %ld\n", stdout_size );
        ok( !stderr_size, "got %ld %s\n", stderr_size, stderr_buf );

        if (tests[i].diff1)
        {
            ok( exitcode == 1, "%u: got %lu\n", i, exitcode );
            ret = check_diff( stdout_buf, tests[i].diff1, tests[i].diff2, tests[i].diff3, tests[i].diff4 );
            todo_wine_if (tests[i].todo) ok( ret, "%u: got %d '%s'\n", i, ret, stdout_buf );
            if (!ret) ok( !stderr_size, "got %ld %s\n", stderr_size, stderr_buf );
        }
        else
        {
            ok( exitcode == 0, "%u: got %lu\n", i, exitcode );
            ret = check_nodiff( stdout_buf );
            ok( ret, "%u: got %d '%s'\n", i, ret, stdout_buf );
        }
    }
}

START_TEST(fc)
{
    if (PRIMARYLANGID( GetUserDefaultUILanguage() ) != LANG_ENGLISH)
    {
        skip( "tests require English locale\n" );
        return;
    }

    create_temp_file( tmpfile1, "tst" );
    create_temp_file( tmpfile2, "TST" );

    test_diff_output();

    DeleteFileA( tmpfile1 );
    DeleteFileA( tmpfile2 );
}
