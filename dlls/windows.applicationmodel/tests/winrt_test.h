/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_WINRT_TEST_H
#define __WINE_WINRT_TEST_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "wine/test.h"

/* desktop/winrt shared data */
struct winetest_shared_data
{
    BOOL running_under_wine;
    BOOL winetest_report_success;
    BOOL winetest_debug;
    LONG successes;
    LONG failures;
    LONG todo_successes;
    LONG todo_failures;
    LONG skipped;
};

static HANDLE winrt_section;
static HANDLE winrt_okfile;
static HANDLE winrt_event;

#define winrt_test_init() winrt_test_init_( __FILE__, __LINE__ )
#define winrt_test_exit() winrt_test_exit_( __FILE__, __LINE__ )

#ifndef WINE_WINRT_TEST

static BOOL winrt_test_init_( const char *file, int line )
{
    struct winetest_shared_data *data;

    winrt_section = CreateFileMappingW( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*data),
                                        L"winetest_winrt_section" );
    if (!winrt_section && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip_(file, line)( "Skipping WinRT tests: failed to create mapping.\n" );
        return FALSE;
    }
    ok_(file, line)( !!winrt_section, "got error %lu\n", GetLastError() );

    data = MapViewOfFile( winrt_section, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024 );
    ok_(file, line)( !!data, "MapViewOfFile failed, error %lu\n", GetLastError() );
    data->running_under_wine = !strcmp( winetest_platform, "wine" );
    data->winetest_report_success = winetest_report_success;
    data->winetest_debug = winetest_debug;
    UnmapViewOfFile( data );

    winrt_okfile = CreateFileW( L"C:\\Users\\Public\\Documents\\winetest_winrt_okfile", GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL );
    ok_(file, line)( winrt_okfile != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError() );

    winrt_event = CreateEventW( 0, FALSE, FALSE, L"winetest_winrt_event" );
    ok_(file, line)( !!winrt_event, "got error %lu\n", GetLastError() );

    subtest_(file, line)( "application" );
    return TRUE;
}

static void winrt_test_exit_( const char *file, int line )
{
    struct winetest_shared_data *data;
    char buffer[512];
    DWORD size, res;

    res = WaitForSingleObject( winrt_event, 5000 );
    ok_(file, line)( !res, "WaitForSingleObject returned %#lx\n", res );
    CloseHandle( winrt_event );

    SetFilePointer( winrt_okfile, 0, NULL, FILE_BEGIN );
    do
    {
        ReadFile( winrt_okfile, buffer, sizeof(buffer), &size, NULL );
        printf( "%.*s", (int)size, buffer );
    } while (size == sizeof(buffer));
    SetFilePointer( winrt_okfile, 0, NULL, FILE_BEGIN );
    SetEndOfFile( winrt_okfile );
    CloseHandle( winrt_okfile );
    DeleteFileW( L"C:\\Users\\Public\\Documents\\winetest_winrt_okfile" );

    data = MapViewOfFile( winrt_section, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024 );
    ok_(file, line)( !!data, "MapViewOfFile failed, error %lu\n", GetLastError() );
    InterlockedAdd( &winetest_successes, InterlockedExchange( &data->successes, 0 ) );
    winetest_add_failures( InterlockedExchange( &data->failures, 0 ) );
    InterlockedAdd( &winetest_todo_successes, InterlockedExchange( &data->todo_successes, 0 ) );
    winetest_add_failures( InterlockedExchange( &data->todo_failures, 0 ) );
    InterlockedAdd( &winetest_skipped, InterlockedExchange( &data->skipped, 0 ) );
    UnmapViewOfFile( data );
    CloseHandle( winrt_section );
}

#else /* WINE_WINRT_TEST */

static DWORD tls_index;

LONG winetest_successes;
LONG winetest_failures;
LONG winetest_flaky_failures;
LONG winetest_skipped;
LONG winetest_todo_successes;
LONG winetest_todo_failures;
LONG winetest_muted_traces;
LONG winetest_muted_skipped;
LONG winetest_muted_todo_successes;

const char *winetest_platform;
int winetest_platform_is_wine;
int winetest_debug;
int winetest_report_success;
int winetest_color = 0;
int winetest_time = 0;
int winetest_start_time, winetest_last_time;

/* silence todos and skips above this threshold */
int winetest_mute_threshold = 42;

struct winetest_thread_data *winetest_get_thread_data(void)
{
    struct winetest_thread_data *data;

    if (!(data = TlsGetValue( tls_index )))
    {
        data = calloc( 1, sizeof(*data) );
        data->str_pos = data->strings;
        TlsSetValue( tls_index, data );
    }

    return data;
}

void winetest_print_lock(void)
{
}

void winetest_print_unlock(void)
{
}

int winetest_vprintf( const char *format, va_list args )
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    DWORD written;
    int len;

    len = vsnprintf( data->str_pos, sizeof(data->strings) - (data->str_pos - data->strings), format, args );
    data->str_pos += len;

    if (len && data->str_pos[-1] == '\n')
    {
        WriteFile( winrt_okfile, data->strings, strlen( data->strings ), &written, NULL );
        data->str_pos = data->strings;
    }

    return len;
}

int winetest_get_time(void)
{
    return GetTickCount();
}

static inline BOOL winrt_test_init_( const char *file, int line )
{
    const struct winetest_shared_data *data;

    tls_index = TlsAlloc();

    winrt_okfile = CreateFileW( L"C:\\Users\\Public\\Documents\\winetest_winrt_okfile", GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    if (!winrt_okfile) return FALSE;

    winrt_event = OpenEventW( EVENT_ALL_ACCESS, FALSE, L"winetest_winrt_event" );
    ok_(file, line)( !!winrt_event, "OpenEventW failed, error %lu\n", GetLastError() );

    winrt_section = OpenFileMappingW( FILE_MAP_ALL_ACCESS, FALSE, L"winetest_winrt_section" );
    ok_(file, line)( !!winrt_section, "OpenFileMappingW failed, error %lu\n", GetLastError() );

    data = MapViewOfFile( winrt_section, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024 );
    ok_(file, line)( !!data, "MapViewOfFile failed, error %lu\n", GetLastError() );
    winetest_platform_is_wine = data->running_under_wine;
    winetest_platform = winetest_platform_is_wine ? "wine" : "windows";
    winetest_debug = data->winetest_debug;
    winetest_report_success = data->winetest_report_success;
    UnmapViewOfFile( data );

    return TRUE;
}

static inline void winrt_test_exit_( const char *file, int line )
{
    char test_name[MAX_PATH], *tmp;
    struct winetest_shared_data *data;
    const char *source_file;

    source_file = strrchr( file, '/' );
    if (!source_file) source_file = strrchr( file, '\\' );
    if (!source_file) source_file = file;
    else source_file++;

    strcpy( test_name, source_file );
    if ((tmp = strrchr( test_name, '.' ))) *tmp = 0;

    data = MapViewOfFile( winrt_section, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024 );
    ok_(file, line)( !!data, "MapViewOfFile failed, error %lu\n", GetLastError() );
    InterlockedExchangeAdd( &data->successes, winetest_successes );
    InterlockedExchangeAdd( &data->failures, winetest_failures );
    InterlockedExchangeAdd( &data->todo_successes, winetest_todo_successes );
    InterlockedExchangeAdd( &data->todo_failures, winetest_todo_failures );
    InterlockedExchangeAdd( &data->skipped, winetest_skipped );
    UnmapViewOfFile( data );
    CloseHandle( winrt_section );

    if (winetest_debug)
    {
        winetest_printf( "%04lx:%s: %ld tests executed (%ld marked as todo, 0 as flaky, %ld %s), %ld skipped.\n",
                 (DWORD)(DWORD_PTR)GetCurrentProcessId(), test_name,
                 winetest_successes + winetest_failures + winetest_todo_successes + winetest_todo_failures,
                 winetest_todo_successes, winetest_failures + winetest_todo_failures,
                 (winetest_failures + winetest_todo_failures != 1) ? "failures" : "failure", winetest_skipped );
    }
    CloseHandle( winrt_okfile );

    SetEvent( winrt_event );
    CloseHandle( winrt_event );
}

#endif /* WINE_WINRT_TEST */

#endif /* __WINE_WINRT_TEST_H */
