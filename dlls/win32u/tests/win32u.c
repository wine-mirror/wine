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

static void test_class(void)
{
    UNICODE_STRING name;
    WCHAR buf[64];
    WNDCLASSW cls;
    ATOM class;
    HWND hwnd;
    ULONG ret;

    memset( &cls, 0, sizeof(cls) );
    cls.style         = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc   = DefWindowProcW;
    cls.hInstance     = GetModuleHandleW( NULL );
    cls.hbrBackground = GetStockObject( WHITE_BRUSH );
    cls.lpszMenuName  = 0;
    cls.lpszClassName = L"test";

    class = RegisterClassW( &cls );
    ok( class, "RegisterClassW failed: %lu\n", GetLastError() );

    hwnd = CreateWindowW( L"test", L"test name", WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                          CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, NULL, 0 );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = sizeof(buf);
    ret = NtUserGetAtomName( class, &name );
    ok( ret == 4, "NtUserGetAtomName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == sizeof(buf), "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"test" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = 8;
    ret = NtUserGetAtomName( class, &name );
    ok( ret == 3, "NtUserGetAtomName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == 8, "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"tes" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.MaximumLength = 1;
    SetLastError( 0xdeadbeef );
    ret = NtUserGetAtomName( class, &name );
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "NtUserGetAtomName returned %lx %lu\n", ret, GetLastError() );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = sizeof(buf);
    ret = NtUserGetClassName( hwnd, FALSE, &name );
    ok( ret == 4, "NtUserGetClassName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == sizeof(buf), "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"test" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.Length = 0xdead;
    name.MaximumLength = 8;
    ret = NtUserGetClassName( hwnd, FALSE, &name );
    ok( ret == 3, "NtUserGetClassName returned %lu\n", ret );
    ok( name.Length == 0xdead, "Length = %u\n", name.Length );
    ok( name.MaximumLength == 8, "MaximumLength = %u\n", name.MaximumLength );
    ok( !wcscmp( buf, L"tes" ), "buf = %s\n", debugstr_w(buf) );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.MaximumLength = 1;
    SetLastError( 0xdeadbeef );
    ret = NtUserGetClassName( hwnd, FALSE, &name );
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "NtUserGetClassName returned %lx %lu\n", ret, GetLastError() );

    DestroyWindow( hwnd );

    ret = UnregisterClassW( L"test", GetModuleHandleW(NULL) );
    ok( ret, "UnregisterClassW failed: %lu\n", GetLastError() );

    memset( buf, 0xcc, sizeof(buf) );
    name.Buffer = buf;
    name.MaximumLength = sizeof(buf);
    SetLastError( 0xdeadbeef );
    ret = NtUserGetAtomName( class, &name );
    ok( !ret && GetLastError() == ERROR_INVALID_HANDLE,
        "NtUserGetAtomName returned %lx %lu\n", ret, GetLastError() );
    ok( buf[0] == 0xcccc, "buf = %s\n", debugstr_w(buf) );

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
    WCHAR module[MAX_PATH], res_buf[MAX_PATH];
    UNICODE_STRING module_str, res_str;
    BYTE bmp_bits[1024];
    LONG width, height;
    DWORD rate, steps;
    HCURSOR frame;
    HANDLE handle;
    ICONINFO info;
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

    ret = NtUserGetIconInfo( handle, &info, NULL, NULL, NULL, 0 );
    ok( ret, "NtUserGetIconInfo failed: %lu\n", GetLastError() );
    ok( info.fIcon == TRUE, "fIcon = %x\n", info.fIcon );
    ok( info.xHotspot == 8, "xHotspot = %lx\n", info.xHotspot );
    ok( info.yHotspot == 8, "yHotspot = %lx\n", info.yHotspot );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );

    memset( module, 0xcc, sizeof(module) );
    module_str.Buffer = module;
    module_str.Length = 0xdead;
    module_str.MaximumLength = sizeof(module);
    memset( res_buf, 0xcc, sizeof(res_buf) );
    res_str.Buffer = res_buf;
    res_str.Length = 0xdead;
    res_str.MaximumLength = sizeof(res_buf);
    ret = NtUserGetIconInfo( handle, &info, &module_str, &res_str, NULL, 0 );
    ok( ret, "NtUserGetIconInfo failed: %lu\n", GetLastError() );
    ok( info.fIcon == TRUE, "fIcon = %x\n", info.fIcon );
    ok( !module_str.Length, "module_str.Length = %u\n", module_str.Length );
    ok( !res_str.Length, "res_str.Length = %u\n", res_str.Length );
    ok( module_str.Buffer == module, "module_str.Buffer = %p\n", module_str.Buffer );
    ok( !res_str.Buffer, "res_str.Buffer = %p\n", res_str.Buffer );
    ok( module[0] == 0xcccc, "module[0] = %x\n", module[0] );
    ok( res_buf[0] == 0xcccc, "res_buf[0] = %x\n", res_buf[0] );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );

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

    memset( module, 0xcc, sizeof(module) );
    module_str.Buffer = module;
    module_str.Length = 0xdead;
    module_str.MaximumLength = sizeof(module);
    memset( res_buf, 0xcc, sizeof(res_buf) );
    res_str.Buffer = res_buf;
    res_str.Length = 0xdead;
    res_str.MaximumLength = sizeof(res_buf);
    ret = NtUserGetIconInfo( handle, &info, &module_str, &res_str, NULL, 0 );
    ok( ret, "NtUserGetIconInfo failed: %lu\n", GetLastError() );
    ok( info.fIcon == TRUE, "fIcon = %x\n", info.fIcon );
    ok( module_str.Length, "module_str.Length = 0\n" );
    ok( !res_str.Length, "res_str.Length = %u\n", res_str.Length );
    ok( module_str.Buffer == module, "module_str.Buffer = %p\n", module_str.Buffer );
    ok( res_str.Buffer == (WCHAR *)IDI_HAND, "res_str.Buffer = %p\n", res_str.Buffer );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );

    module[module_str.Length] = 0;
    ok( GetModuleHandleW(module) == GetModuleHandleW(L"user32.dll"),
        "GetIconInfoEx wrong module %s\n", wine_dbgstr_w(module) );

    ret = DestroyIcon(handle);
    ok(ret, "Destroy icon failed, error %lu.\n", GetLastError());
}

static LRESULT WINAPI test_message_call_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch (msg)
    {
    case WM_SETTEXT:
        ok( !wcscmp( (const WCHAR *)lparam, L"test" ),
            "lparam = %s\n", wine_dbgstr_w( (const WCHAR *)lparam ));
        return 6;
    case WM_USER:
        ok( wparam == 1, "wparam = %Iu\n", wparam );
        ok( lparam == 2, "lparam = %Iu\n", lparam );
        return 3;
    case WM_USER + 1:
        return lparam;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void WINAPI test_message_callback( HWND hwnd, UINT msg, ULONG_PTR data, LRESULT result )
{
    ok( msg == WM_USER, "msg = %u\n", msg );
    ok( data == 10, "data = %Iu\n", data );
    ok( result == 3, "result = %Iu\n", result );
}

static void test_message_call(void)
{
    const LPARAM large_lparam = (LPARAM)(3 + ((ULONGLONG)1 << 60));
    struct send_message_callback_params callback_params = {
        .callback = test_message_callback,
        .data = 10,
    };
    struct send_message_timeout_params smp;
    WNDCLASSW cls = { 0 };
    LRESULT res;
    HWND hwnd;

    cls.lpfnWndProc = test_message_call_proc;
    cls.lpszClassName = L"TestClass";
    RegisterClassW( &cls );

    hwnd = CreateWindowExW( 0, L"TestClass", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, (void *)0xdeadbeef, NtUserSendMessage, FALSE );
    ok( res == 3, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, (void *)0xdeadbeef, NtUserSendMessage, TRUE );
    ok( res == 3, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)L"test", NULL, NtUserSendMessage, FALSE );
    ok( res == 6, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)"test", NULL, NtUserSendMessage, TRUE );
    ok( res == 6, "res = %Iu\n", res );

    SetLastError( 0xdeadbeef );
    res = NtUserMessageCall( UlongToHandle(0xdeadbeef), WM_USER, 1, 2, 0, NtUserSendMessage, TRUE );
    ok( !res, "res = %Iu\n", res );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetLastError() = %lu\n", GetLastError());

    res = NtUserMessageCall( hwnd, WM_USER + 1, 0, large_lparam, 0, NtUserSendMessage, FALSE );
    ok( res == large_lparam, "res = %Iu\n", res );

    smp.flags = 0;
    smp.timeout = 10;
    smp.result = 0xdeadbeef;
    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, &smp, NtUserSendMessageTimeout, FALSE );
    ok( res == 3, "res = %Iu\n", res );
    ok( smp.result == 1, "smp.result = %Iu\n", smp.result );

    smp.flags = 0;
    smp.timeout = 10;
    smp.result = 0xdeadbeef;
    res = NtUserMessageCall( hwnd, WM_USER + 1, 0, large_lparam,
                             &smp, NtUserSendMessageTimeout, FALSE );
    ok( res == large_lparam, "res = %Iu\n", res );
    ok( smp.result == 1, "smp.result = %Iu\n", smp.result );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, (void *)0xdeadbeef,
                             NtUserSendNotifyMessage, FALSE );
    ok( res == 1, "res = %Iu\n", res );

    res = NtUserMessageCall( hwnd, WM_USER, 1, 2, &callback_params,
                             NtUserSendMessageCallback, FALSE );
    ok( res == 1, "res = %Iu\n", res );

    DestroyWindow( hwnd );
    UnregisterClassW( L"TestClass", NULL );
}

static void test_window_text(void)
{
    WCHAR buf[512];
    LRESULT res;
    int len;
    HWND hwnd;

    hwnd = CreateWindowExW( 0, L"static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL );

    memset( buf, 0xcc, sizeof(buf) );
    len = NtUserInternalGetWindowText( hwnd, buf, ARRAYSIZE(buf) );
    ok( len == 0, "len = %d\n", len );
    ok( !buf[0], "buf = %s\n", wine_dbgstr_w(buf) );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)L"test", 0, NtUserDefWindowProc, FALSE );
    ok( res == 1, "res = %Id\n", res );

    memset( buf, 0xcc, sizeof(buf) );
    len = NtUserInternalGetWindowText( hwnd, buf, ARRAYSIZE(buf) );
    ok( len == 4, "len = %d\n", len );
    ok( !lstrcmpW( buf, L"test" ), "buf = %s\n", wine_dbgstr_w(buf) );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, 0, NtUserDefWindowProc, TRUE );
    ok( res == 4, "res = %Id\n", res );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, 0, NtUserDefWindowProc, FALSE );
    ok( res == 4, "res = %Id\n", res );

    res = NtUserMessageCall( hwnd, WM_SETTEXT, 0, (LPARAM)"TestA", 0, NtUserDefWindowProc, TRUE );
    ok( res == 1, "res = %Id\n", res );

    memset( buf, 0xcc, sizeof(buf) );
    len = NtUserInternalGetWindowText( hwnd, buf, ARRAYSIZE(buf) );
    ok( len == 5, "len = %d\n", len );
    ok( !lstrcmpW( buf, L"TestA" ), "buf = %s\n", wine_dbgstr_w(buf) );

    res = NtUserMessageCall( hwnd, WM_GETTEXTLENGTH, 0, 0, 0, NtUserDefWindowProc, TRUE );
    ok( res == 5, "res = %Id\n", res );

    DestroyWindow( hwnd );
}

#define test_menu_item_id(a, b, c) test_menu_item_id_(a, b, c, __LINE__)
static void test_menu_item_id_( HMENU menu, int pos, int expect, int line )
{
    MENUITEMINFOW item;
    BOOL ret;

    item.cbSize = sizeof(item);
    item.fMask = MIIM_ID;
    ret = GetMenuItemInfoW( menu, pos, TRUE, &item );
    ok_(__FILE__,line)( ret, "GetMenuItemInfoW failed: %lu\n", GetLastError() );
    ok_(__FILE__,line)( item.wID == expect, "got if %d, expected %d\n", item.wID, expect );
}

static void test_menu(void)
{
    MENUITEMINFOW item;
    HMENU menu;
    int count;
    BOOL ret;

    menu = CreateMenu();

    memset( &item, 0, sizeof(item) );
    item.cbSize = sizeof(item);
    item.fMask = MIIM_ID;
    item.wID = 10;
    ret = NtUserThunkedMenuItemInfo( menu, 0, MF_BYPOSITION, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 1, "count = %d\n", count );

    item.wID = 20;
    ret = NtUserThunkedMenuItemInfo( menu, 1, MF_BYPOSITION, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 2, "count = %d\n", count );
    test_menu_item_id( menu, 0, 10 );
    test_menu_item_id( menu, 1, 20 );

    item.wID = 30;
    ret = NtUserThunkedMenuItemInfo( menu, 1, MF_BYPOSITION, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 3, "count = %d\n", count );
    test_menu_item_id( menu, 0, 10 );
    test_menu_item_id( menu, 1, 30 );
    test_menu_item_id( menu, 2, 20 );

    item.wID = 50;
    ret = NtUserThunkedMenuItemInfo( menu, 10, 0, NtUserInsertMenuItem, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 4, "count = %d\n", count );
    test_menu_item_id( menu, 0, 50 );
    test_menu_item_id( menu, 1, 10 );
    test_menu_item_id( menu, 2, 30 );
    test_menu_item_id( menu, 3, 20 );

    item.wID = 60;
    ret = NtUserThunkedMenuItemInfo( menu, 1, MF_BYPOSITION, NtUserSetMenuItemInfo, &item, NULL );
    ok( ret, "InsertMenuItemW failed: %lu\n", GetLastError() );

    count = GetMenuItemCount( menu );
    ok( count == 4, "count = %d\n", count );
    test_menu_item_id( menu, 1, 60 );

    ret = NtUserDestroyMenu( menu );
    ok( ret, "NtUserDestroyMenu failed: %lu\n", GetLastError() );
}

static MSG *msg_ptr;

static LRESULT WINAPI hook_proc( INT code, WPARAM wparam, LPARAM lparam )
{
    msg_ptr = (MSG *)lparam;
    ok( code == 100, "code = %d\n", code );
    ok( msg_ptr->time == 1, "time = %lx\n", msg_ptr->time );
    ok( msg_ptr->wParam == 10, "wParam = %Ix\n", msg_ptr->wParam );
    ok( msg_ptr->lParam == 20, "lParam = %Ix\n", msg_ptr->lParam );
    msg_ptr->time = 3;
    msg_ptr->wParam = 1;
    msg_ptr->lParam = 2;
    return CallNextHookEx( NULL, code, wparam, lparam );
}

static void test_message_filter(void)
{
    HHOOK hook;
    MSG msg;
    BOOL ret;

    hook = SetWindowsHookExW( WH_MSGFILTER, hook_proc, NULL, GetCurrentThreadId() );
    ok( hook != NULL, "SetWindowsHookExW failed\n");

    memset( &msg, 0, sizeof(msg) );
    msg.time = 1;
    msg.wParam = 10;
    msg.lParam = 20;
    ret = NtUserCallMsgFilter( &msg, 100 );
    ok( !ret, "CallMsgFilterW returned: %x\n", ret );
    todo_wine
    ok( msg_ptr != &msg, "our ptr was passed directly to hook\n" );

    if (sizeof(void *) == 8) /* on some Windows versions, msg is not modified on wow64 */
    {
        ok( msg.time == 3, "time = %lx\n", msg.time );
        ok( msg.wParam == 1, "wParam = %Ix\n", msg.wParam );
        ok( msg.lParam == 2, "lParam = %Ix\n", msg.lParam );
    }

    ret = NtUserUnhookWindowsHookEx( hook );
    ok( ret, "NtUserUnhookWindowsHook failed: %lu\n", GetLastError() );
}

START_TEST(win32u)
{
    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    test_NtUserEnumDisplayDevices();
    test_window_props();
    test_class();
    test_NtUserBuildHwndList();
    test_cursoricon();
    test_message_call();
    test_window_text();
    test_menu();
    test_message_filter();

    test_NtUserCloseWindowStation();
}
