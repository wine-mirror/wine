/*
 * Copyright 2026 Paul Gofman for CodeWeavers
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

static HANDLE con_out, orig_stderr;
static FILE *child_log_file;

#define ok_child(cond, format, ...) do { \
            ok( cond, format, __VA_ARGS__ ); \
            if (!(cond)) fprintf( child_log_file, "%u:"format, __LINE__, __VA_ARGS__ ); \
        } while (0)

static void test_console_mode_change_grandchild( const char *exec_type, const char *log_fn )
{
    HANDLE con = GetStdHandle( STD_OUTPUT_HANDLE );
    DWORD mode;
    BOOL bret;

    if (log_fn)
    {
        child_log_file = fopen( log_fn, "w" );
        ok( !!child_log_file, "got NULL.\n" );
    }
    else
    {
        child_log_file = stderr;
    }

    bret = GetConsoleMode( con, &mode );
    ok_child( bret, "got error %lu.\n", GetLastError() );
    if (exec_type && !strcmp( exec_type, "startcmd" ))
    {
        ok_child( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
                  || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
                  "got %#lx.\n", mode );
    }
    else
    {
        /* Looks like before Win10 the console mode is not restored for external command and
         * it is either internal cmd default or ENABLE_WRAP_AT_EOL_OUTPUT wheb left by previous
         * child invocation within the same command. */
        todo_wine_if( exec_type && !strcmp( exec_type, "todo" ))
        ok_child( mode == ENABLE_PROCESSED_OUTPUT ||
                  broken(mode == ENABLE_WRAP_AT_EOL_OUTPUT
                  || mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)) /* before Win10 */,
                  "got %#lx.\n", mode );
    }

    bret = SetConsoleMode( con, ENABLE_WRAP_AT_EOL_OUTPUT );
    ok_child( bret, "got error %lu.\n", GetLastError() );
    fprintf( child_log_file, "test error count: %ld\n", winetest_get_failures() );
}

static void test_console_mode_change(int argc, char *argv[])
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    unsigned int error_count;
    DWORD old_mode, mode;
    HANDLE con_out_dup;
    char cmd[MAX_PATH];
    char s[1024];
    DWORD ret;
    BOOL bret;
    FILE *f;

    bret = GetConsoleMode( con_out, &old_mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    ok( old_mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)
        || old_mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING),
        "got %#lx.\n", old_mode );

    bret = DuplicateHandle( GetCurrentProcess(), con_out, GetCurrentProcess(), &con_out_dup, 0, TRUE, DUPLICATE_SAME_ACCESS );
    ok( bret, "got error %lu.\n", GetLastError() );

    SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
    si.hStdError = orig_stderr;
    si.hStdOutput = con_out_dup;
    strcpy( cmd, "cmd.exe /c nonexistent" );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    ret = WaitForSingleObject( info.hProcess, 5000 );
    ok( ret == WAIT_OBJECT_0, "got %lu.\n", ret );
    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
    si.hStdError = orig_stderr;
    si.hStdOutput = con_out_dup;
    strcpy( cmd, "cmd.exe /c cls" );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    ret = WaitForSingleObject( info.hProcess, 5000 );
    ok( ret == WAIT_OBJECT_0, "got %lu.\n", ret );
    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    bret = SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    ok( bret, "got error %lu.\n", GetLastError() );
    DeleteFileA( "grandchild.out" );
    sprintf( cmd, "cmd.exe /c %s %s grandchild 2>grandchild.out", argv[0], argv[1] );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    wait_child_process( &info );
    if ((f = fopen( "grandchild.out", "r" )))
    {
        while (fgets( s, sizeof(s), f ))
            trace("grandchild: %s", s);
        fclose(f);
    }
    bret = DeleteFileA( "grandchild.out" );
    ok( bret, "got error %lu.\n", GetLastError() );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    todo_wine
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    bret = SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    ok( bret, "got error %lu.\n", GetLastError() );
    sprintf( cmd, "cmd.exe /c %s %s grandchild cmd grandchild.out", argv[0], argv[1] );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    wait_child_process( &info );
    if ((f = fopen( "grandchild.out", "r" )))
    {
        while (fgets( s, sizeof(s), f ))
            trace( "grandchild no redirect: %s", s );
        fclose( f );
    }
    bret = DeleteFileA( "grandchild.out" );
    ok( bret, "got error %lu.\n", GetLastError() );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    todo_wine
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    bret = SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    ok( bret, "got error %lu.\n", GetLastError() );
    DeleteFileA( "grandchild2.out" );
    sprintf( cmd, "cmd.exe /c \"start /b /wait %s %s grandchild startcmd\" 2>>grandchild.out", argv[0], argv[1] );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    wait_child_process( &info );
    if ((f = fopen( "grandchild.out", "r" )))
    {
        while (fgets( s, sizeof(s), f ))
        {
            trace("grandchild start: %s", s);
            if (sscanf( s, "test error count: %u.\n", &error_count ))
                ok( !error_count, "got %u errors in grandchild.\n", error_count);
        }
        fclose( f );
    }
    DeleteFileA( "grandchild.out" );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    todo_wine
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    bret = SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    ok( bret, "got error %lu.\n", GetLastError() );
    DeleteFileA( "grandchild.out" );
    sprintf( cmd, "cmd.exe /c %s %s grandchild 2>>grandchild.out & %s %s grandchild 2>>grandchild.out",
             argv[0], argv[1], argv[0], argv[1] );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    wait_child_process( &info );
    if ((f = fopen( "grandchild.out", "r" )))
    {
        while (fgets(s, sizeof(s), f))
            trace( "grandchild &: %s", s );
        fclose( f );
    }
    DeleteFileA( "grandchild.out" );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    todo_wine
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    bret = SetConsoleMode( con_out, ENABLE_PROCESSED_OUTPUT );
    ok( bret, "got error %lu.\n", GetLastError() );
    strcpy( cmd, "cmd.exe /c echo text\r\n" );
    bret = CreateProcessA( NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info );
    ok( bret, "got error %lu.\n", GetLastError() );
    wait_child_process( &info );
    bret = GetConsoleMode( con_out, &mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    ok( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || broken( mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) ) /* before Win10 */,
        "got %#lx.\n", mode );

    bret = SetConsoleMode( con_out, old_mode );
    ok( bret, "got error %lu.\n", GetLastError() );
    CloseHandle( con_out_dup );
}

START_TEST(console)
{
    char **argv;
    int argc;
    BOOL bret;

    argc = winetest_get_mainargs(&argv);
    if (argc > 2 && !strcmp(argv[2], "grandchild"))
    {
        test_console_mode_change_grandchild( argc > 3 ? argv[3] : NULL, argc > 4 ? argv[4] : NULL );
        return;
    }

    /* Make sure console is functioonal. All the tests which need original console should go before. */
    orig_stderr = GetStdHandle( STD_ERROR_HANDLE );
    FreeConsole();
    bret = AllocConsole();
    ok( bret, "got error %lu.\n", GetLastError() );
    con_out = CreateFileA( "CONOUT$", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( con_out != INVALID_HANDLE_VALUE, "got error %ld.\n", GetLastError() );
    /* disable winetest ANSI escape of errors (it tempers with console output mode) */
    SetEnvironmentVariableA( "WINETEST_COLOR", NULL );
    winetest_color = 0;

    test_console_mode_change( argc, argv );
}
