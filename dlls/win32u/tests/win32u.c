/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "wine/test.h"

#include "winbase.h"
#include "ntuser.h"


static void test_NtUserEnumDisplayDevices(void)
{
    NTSTATUS ret;
    DISPLAY_DEVICEW info = { sizeof(DISPLAY_DEVICEW) };

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, &info, 0 );
    ok( !ret && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
                  "NtUserEnumDisplayDevices returned %#lx %lu\n", ret,
                  GetLastError() );

    info.cb = 0;

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, &info, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 0, NULL, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserEnumDisplayDevices( NULL, 12345, NULL, 0 );
    ok( ret == STATUS_UNSUCCESSFUL && GetLastError() == 0xdeadbeef,
        "NtUserEnumDisplayDevices returned %#lx %lu\n", ret, GetLastError() );
}

static void test_NtUserCloseWindowStation(void)
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = NtUserCloseWindowStation( 0 );
    ok( !ret && GetLastError() == ERROR_INVALID_HANDLE,
        "NtUserCloseWindowStation returned %x %lu\n", ret, GetLastError() );
}

static void test_window_props(void)
{
    HANDLE prop;
    ATOM atom;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    atom = GlobalAddAtomW( L"test" );

    ret = NtUserSetProp( hwnd, UlongToPtr(atom), UlongToHandle(0xdeadbeef) );
    ok( ret, "NtUserSetProp failed: %lu\n", GetLastError() );

    prop = GetPropW( hwnd, L"test" );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = NtUserGetProp( hwnd, UlongToPtr(atom) );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = NtUserRemoveProp( hwnd, UlongToPtr(atom) );
    ok( prop == UlongToHandle(0xdeadbeef), "prop = %p\n", prop );

    prop = GetPropW(hwnd, L"test");
    ok(!prop, "prop = %p\n", prop);

    GlobalDeleteAtom( atom );
    DestroyWindow( hwnd );
}

static BOOL WINAPI count_win( HWND hwnd, LPARAM lparam )
{
    ULONG *cnt = (ULONG *)lparam;
    (*cnt)++;
    return TRUE;
}

static void test_NtUserBuildHwndList(void)
{
    ULONG size, desktop_windows_cnt;
    HWND buf[512], hwnd;
    NTSTATUS status;

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 1, "size = %lu\n", size );
    ok( buf[0] == HWND_BOTTOM, "buf[0] = %p\n", buf[0] );

    hwnd = CreateWindowExA( 0, "static", NULL, WS_POPUP, 0,0,0,0,GetDesktopWindow(),0,0, NULL );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );
    ok( buf[0] == hwnd, "buf[0] = %p\n", buf[0] );
    ok( buf[2] == HWND_BOTTOM, "buf[0] = %p\n", buf[2] );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 3, buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 2, buf, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 0, GetCurrentThreadId(), 1, buf, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 3, "size = %lu\n", size );

    desktop_windows_cnt = 0;
    EnumDesktopWindows( 0, count_win, (LPARAM)&desktop_windows_cnt );

    size = 0;
    status = NtUserBuildHwndList( 0, 0, 0, 1, 0, ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == desktop_windows_cnt + 1, "size = %lu, expected %lu\n", size, desktop_windows_cnt + 1 );

    desktop_windows_cnt = 0;
    EnumDesktopWindows( GetThreadDesktop( GetCurrentThreadId() ), count_win, (LPARAM)&desktop_windows_cnt );

    size = 0;
    status = NtUserBuildHwndList( GetThreadDesktop(GetCurrentThreadId()), 0, 0, 1, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == desktop_windows_cnt + 1, "size = %lu, expected %lu\n", size, desktop_windows_cnt + 1 );

    size = 0;
    status = NtUserBuildHwndList( GetThreadDesktop(GetCurrentThreadId()), 0, 0, 0, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( !status, "NtUserBuildHwndList failed: %#lx\n", status );
    todo_wine
    ok( size > desktop_windows_cnt + 1, "size = %lu, expected %lu\n", size, desktop_windows_cnt + 1 );

    size = 0xdeadbeef;
    status = NtUserBuildHwndList( UlongToHandle(0xdeadbeef), 0, 0, 0, 0,
                                  ARRAYSIZE(buf), buf, &size );
    ok( status == STATUS_INVALID_HANDLE, "NtUserBuildHwndList failed: %#lx\n", status );
    ok( size == 0xdeadbeef, "size = %lu\n", size );

    DestroyWindow( hwnd );
}

static void test_cursoricon(void)
{
    BYTE bmp_bits[1024];
    LONG width, height;
    DWORD rate, steps;
    HCURSOR frame;
    HANDLE handle;
    unsigned int i;
    BOOL ret;

    for (i = 0; i < sizeof(bmp_bits); ++i)
        bmp_bits[i] = 111 * i;

    handle = CreateIcon( 0, 16, 16, 1, 1, bmp_bits, &bmp_bits[16 * 16 / 8] );
    ok(handle != 0, "CreateIcon failed\n");

    ret = NtUserGetIconSize( handle, 0, &width, &height );
    ok( ret, "NtUserGetIconSize failed: %lu\n", GetLastError() );
    ok( width == 16, "width = %ld\n", width );
    ok( height == 32, "height = %ld\n", height );

    ret = NtUserGetIconSize( handle, 6, &width, &height );
    ok( ret, "NtUserGetIconSize failed: %lu\n", GetLastError() );
    ok( width == 16, "width = %ld\n", width );
    ok( height == 32, "height = %ld\n", height );

    frame = NtUserGetCursorFrameInfo( handle, 0, &rate, &steps );
    ok( frame != NULL, "NtUserGetCursorFrameInfo failed: %lu\n", GetLastError() );
    ok( frame == handle, "frame != handle\n" );
    ok( rate == 0, "rate = %lu\n", rate );
    ok( steps == 1, "steps = %lu\n", steps );

    ret = NtUserDestroyCursor( handle, 0 );
    ok( ret, "NtUserDestroyIcon failed: %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = NtUserGetIconSize( handle, 0, &width, &height );
    ok( !ret && GetLastError() == ERROR_INVALID_CURSOR_HANDLE,
        "NtUserGetIconSize returned %x %lu\n", ret, GetLastError() );

    /* Test a system icon */
    handle = LoadIconA( 0, (LPCSTR)IDI_HAND );
    ok( handle != NULL, "LoadIcon icon failed, error %lu\n", GetLastError() );

    ret = NtUserGetIconSize( handle, 0, &width, &height );
    ok( width == 32, "width = %ld\n", width );
    ok( height == 64, "height = %ld\n", height );
    ok( ret, "NtUserGetIconSize failed: %lu\n", GetLastError() );

    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());
}

START_TEST(win32u)
{
    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    test_NtUserEnumDisplayDevices();
    test_window_props();
    test_NtUserBuildHwndList();
    test_cursoricon();

    test_NtUserCloseWindowStation();
}
