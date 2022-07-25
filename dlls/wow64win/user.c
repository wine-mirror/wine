/*
 * WoW64 User functions
 *
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"
#include "wow64win_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);

typedef struct
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fType;
    UINT    fState;
    UINT    wID;
    UINT32  hSubMenu;
    UINT32  hbmpChecked;
    UINT32  hbmpUnchecked;
    UINT32  dwItemData;
    UINT32  dwTypeData;
    UINT    cch;
    UINT32  hbmpItem;
} MENUITEMINFOW32;

typedef struct
{
    UINT32  hwnd;
    UINT    message;
    UINT32  wParam;
    UINT32  lParam;
    DWORD   time;
    POINT   pt;
} MSG32;

typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    UINT32 hDevice;
    UINT32 wParam;
} RAWINPUTHEADER32;

typedef struct
{
    USHORT usUsagePage;
    USHORT usUsage;
    DWORD dwFlags;
    UINT32 hwndTarget;
} RAWINPUTDEVICE32;

typedef struct
{
    UINT32 hDevice;
    DWORD dwType;
} RAWINPUTDEVICELIST32;

typedef struct
{
    LONG    dx;
    LONG    dy;
    DWORD   mouseData;
    DWORD   dwFlags;
    DWORD   time;
    ULONG   dwExtraInfo;
} MOUSEINPUT32;

typedef struct
{
    WORD    wVk;
    WORD    wScan;
    DWORD   dwFlags;
    DWORD   time;
    ULONG   dwExtraInfo;
} KEYBDINPUT32;

typedef struct
{
    DWORD type;
    union
    {
        MOUSEINPUT32   mi;
        KEYBDINPUT32   ki;
        HARDWAREINPUT  hi;
    } DUMMYUNIONNAME;
} INPUT32;

typedef struct
{
    UINT32 hdc;
    BOOL   fErase;
    RECT   rcPaint;
    BOOL   fRestore;
    BOOL   fIncUpdate;
    BYTE   rgbReserved[32];
} PAINTSTRUCT32;

typedef struct
{
    UINT   cbSize;
    UINT   style;
    ULONG  lpfnWndProc;
    INT    cbClsExtra;
    INT    cbWndExtra;
    ULONG  hInstance;
    ULONG  hIcon;
    ULONG  hCursor;
    ULONG  hbrBackground;
    ULONG  lpszMenuName;
    ULONG  lpszClassName;
    ULONG  hIconSm;
} WNDCLASSEXW32;

struct client_menu_name32
{
    ULONG nameA;
    ULONG nameW;
    ULONG nameUS;
};

static MSG *msg_32to64( MSG *msg, const MSG32 *msg32 )
{
    if (!msg32) return NULL;

    msg->hwnd    = UlongToHandle( msg32->hwnd );
    msg->message = msg32->message;
    msg->wParam  = msg32->wParam;
    msg->lParam  = msg32->lParam;
    msg->time    = msg32->time;
    msg->pt      = msg32->pt;
    return msg;
}

static MSG32 *msg_64to32( MSG *msg, MSG32 *msg32 )
{
    if (!msg32) return NULL;

    msg32->hwnd    = HandleToUlong( msg->hwnd );
    msg32->message = msg->message;
    msg32->wParam  = msg->wParam;
    msg32->lParam  = msg->lParam;
    msg32->time    = msg->time;
    msg32->pt      = msg->pt;
    return msg32;
}

static struct client_menu_name *client_menu_name_32to64( struct client_menu_name *name,
                                                         const struct client_menu_name32 *name32 )
{
    if (!name32) return NULL;
    name->nameA = UlongToPtr( name32->nameA );
    name->nameW = UlongToPtr( name32->nameW );
    name->nameUS = UlongToPtr( name32->nameUS );
    return name;
}

static struct client_menu_name32 *client_menu_name_64to32( const struct client_menu_name *name,
                                                           struct client_menu_name32 *name32 )
{
    if (name32)
    {
        name32->nameA = PtrToUlong( name->nameA );
        name32->nameW = PtrToUlong( name->nameW );
        name32->nameUS = PtrToUlong( name->nameUS );
    }
    return name32;
}

NTSTATUS WINAPI wow64_NtUserActivateKeyboardLayout( UINT *args )
{
    HKL layout = get_handle( &args );
    UINT flags = get_ulong( &args );

    return HandleToUlong( NtUserActivateKeyboardLayout( layout, flags ));
}

NTSTATUS WINAPI wow64_NtUserAddClipboardFormatListener( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserAddClipboardFormatListener( hwnd );
}

NTSTATUS WINAPI wow64_NtUserAssociateInputContext( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HIMC ctx = get_handle( &args );
    ULONG flags = get_ulong( &args );

    return NtUserAssociateInputContext( hwnd, ctx, flags );
}

NTSTATUS WINAPI wow64_NtUserAttachThreadInput( UINT *args )
{
    DWORD from = get_ulong( &args );
    DWORD to = get_ulong( &args );
    BOOL attach = get_ulong( &args );

    return NtUserAttachThreadInput( from, to, attach );
}

NTSTATUS WINAPI wow64_NtUserBeginPaint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    PAINTSTRUCT32 *ps32 = get_ptr( &args );

    PAINTSTRUCT ps;
    HDC ret;

    ret = NtUserBeginPaint( hwnd, ps32 ? & ps : NULL );
    if (ret && ps32)
    {
        ps32->hdc = HandleToUlong( ps.hdc );
        ps32->fErase  = ps.fErase;
        ps32->rcPaint = ps.rcPaint;
    }
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserBuildHwndList( UINT *args )
{
    HDESK desktop = get_handle( &args );
    ULONG unk2 = get_ulong( &args );
    ULONG unk3 = get_ulong( &args );
    ULONG unk4 = get_ulong( &args );
    ULONG thread_id = get_ulong( &args );
    ULONG count = get_ulong( &args );
    UINT32 *buffer32 = get_ptr( &args );
    ULONG *size = get_ptr( &args );

    HWND *buffer;
    ULONG i;
    NTSTATUS status;

    if (!(buffer = Wow64AllocateTemp( count * sizeof(*buffer) ))) return STATUS_NO_MEMORY;

    if ((status = NtUserBuildHwndList( desktop, unk2, unk3, unk4, thread_id, count, buffer, size )))
        return status;

    for (i = 0; i < *size; i++)
        buffer32[i] = HandleToUlong( buffer[i] );
    return status;
}

NTSTATUS WINAPI wow64_NtUserCallHwnd( UINT *args )
{
    HWND hwnd = get_handle( &args );
    DWORD code = get_ulong( &args );

    return NtUserCallHwnd( hwnd, code );
}

NTSTATUS WINAPI wow64_NtUserCallHwndParam( UINT *args )
{
    HWND hwnd = get_handle( &args );
    DWORD_PTR param = get_ulong( &args );
    DWORD code = get_ulong( &args );

    FIXME( "%p %Ix %lu\n", hwnd, param, code );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserCallMsgFilter( UINT *args )
{
    MSG32 *msg32 = get_ptr( &args );
    INT code = get_ulong( &args );
    MSG msg;
    BOOL ret;

    ret = NtUserCallMsgFilter( msg_32to64( &msg, msg32 ), code );
    msg_64to32( &msg, msg32 );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserCallNextHookEx( UINT *args )
{
    HHOOK hhook = get_handle( &args );
    INT code = get_ulong( &args );
    WPARAM wparam = get_ulong( &args );
    LPARAM lparam = get_ulong( &args );

    return NtUserCallNextHookEx( hhook, code, wparam, lparam );
}

NTSTATUS WINAPI wow64_NtUserCallNoParam( UINT *args )
{
    ULONG code = get_ulong( &args );

    return NtUserCallNoParam( code );
}

NTSTATUS WINAPI wow64_NtUserCallOneParam( UINT *args )
{
    ULONG_PTR arg = get_ulong( &args );
    ULONG code = get_ulong( &args );

    FIXME( "%Ix %lu\n", arg, code );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserCallTwoParam( UINT *args )
{
    ULONG_PTR arg1 = get_ulong( &args );
    ULONG_PTR arg2 = get_ulong( &args );
    ULONG code = get_ulong( &args );

    FIXME( "%Ix %Ix %lu\n", arg1, arg2, code );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserChangeClipboardChain( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HWND next = get_handle( &args );

    return NtUserChangeClipboardChain( hwnd, next );
}

NTSTATUS WINAPI wow64_NtUserChangeDisplaySettings( UINT *args )
{
    UNICODE_STRING32 *devname32 = get_ptr( &args );
    DEVMODEW *devmode = get_ptr( &args );
    HWND hwnd = get_handle( &args );
    DWORD flags = get_ulong( &args );
    void *lparam = get_ptr( &args );

    UNICODE_STRING devname;

    return NtUserChangeDisplaySettings( unicode_str_32to64( &devname, devname32 ),
                                        devmode, hwnd, flags, lparam );
}

NTSTATUS WINAPI wow64_NtUserCheckMenuItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserCheckMenuItem( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserChildWindowFromPointEx( UINT *args )
{
    HWND parent = get_handle( &args );
    LONG x = get_ulong( &args );
    LONG y = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return HandleToUlong( NtUserChildWindowFromPointEx( parent, x, y, flags ));
}

NTSTATUS WINAPI wow64_NtUserClipCursor( UINT *args )
{
    const RECT *rect = get_ptr( &args );

    return NtUserClipCursor( rect );
}

NTSTATUS WINAPI wow64_NtUserCloseClipboard( UINT *args )
{
    return NtUserCloseClipboard();
}

NTSTATUS WINAPI wow64_NtUserCloseDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserCloseDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserCloseWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserCloseWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserCopyAcceleratorTable( UINT *args )
{
    HACCEL src = get_handle( &args );
    ACCEL *dst = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserCopyAcceleratorTable( src, dst, count );
}

NTSTATUS WINAPI wow64_NtUserCountClipboardFormats( UINT *args )
{
    return NtUserCountClipboardFormats();
}

NTSTATUS WINAPI wow64_NtUserCreateAcceleratorTable( UINT *args )
{
    ACCEL *table = get_ptr( &args );
    INT count = get_ulong( &args );

    return HandleToUlong( NtUserCreateAcceleratorTable( table, count ));
}

NTSTATUS WINAPI wow64_NtUserCreateCaret( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HBITMAP bitmap = get_handle( &args );
    int width = get_ulong( &args );
    int height = get_ulong( &args );

    return NtUserCreateCaret( hwnd, bitmap, width, height );
}

NTSTATUS WINAPI wow64_NtUserCreateDesktopEx( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DEVMODEW *devmode = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG heap_size = get_ulong( &args );

    struct object_attr64 attr;
    UNICODE_STRING device;
    HANDLE ret;

    ret = NtUserCreateDesktopEx( objattr_32to64( &attr, attr32 ),
                                 unicode_str_32to64( &device, device32 ),
                                 devmode, flags, access, heap_size );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserCreateInputContext( UINT *args )
{
    UINT_PTR client_ptr = get_ulong( &args );

    return HandleToUlong( NtUserCreateInputContext( client_ptr ));
}

NTSTATUS WINAPI wow64_NtUserCreateWindowEx( UINT *args )
{
    DWORD ex_style = get_ulong( &args );
    UNICODE_STRING32 *class_name32 = get_ptr( &args );
    UNICODE_STRING32 *version32 = get_ptr( &args );
    UNICODE_STRING32 *window_name32 = get_ptr( &args );
    DWORD style = get_ulong( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );
    int width = get_ulong( &args );
    int height = get_ulong( &args );
    HWND parent = get_handle( &args );
    HMENU menu = get_handle( &args );
    HINSTANCE instance = get_handle( &args );
    void *params = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    void *cbtc = get_ptr( &args );
    DWORD unk = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    UNICODE_STRING class_name, version, window_name;
    HWND ret;

    ret = NtUserCreateWindowEx( ex_style,
                                unicode_str_32to64( &class_name, class_name32),
                                unicode_str_32to64( &version, version32 ),
                                unicode_str_32to64( &window_name, window_name32 ),
                                style, x, y, width, height, parent, menu,
                                instance, params, flags, cbtc, unk, ansi );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserCreateWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );
    ULONG arg4 = get_ulong( &args );
    ULONG arg5 = get_ulong( &args );
    ULONG arg6 = get_ulong( &args );
    ULONG arg7 = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserCreateWindowStation( objattr_32to64( &attr, attr32 ), access,
                                                     arg3, arg4, arg5, arg6, arg7 ));
}

NTSTATUS WINAPI wow64_NtUserDeferWindowPosAndBand( UINT *args )
{
    HDWP hdwp = get_handle( &args );
    HWND hwnd = get_handle( &args );
    HWND after = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    INT cx = get_ulong( &args );
    INT cy = get_ulong( &args );
    UINT flags = get_ulong( &args );
    UINT unk1 = get_ulong( &args );
    UINT unk2 = get_ulong( &args );

    HDWP ret = NtUserDeferWindowPosAndBand( hdwp, hwnd, after, x, y, cx, cy, flags, unk1, unk2 );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserDeleteMenu( UINT *args )
{
    HMENU menu = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserDeleteMenu( menu, id, flags );
}

NTSTATUS WINAPI wow64_NtUserDestroyAcceleratorTable( UINT *args )
{
    HACCEL handle = get_handle( &args );

    return NtUserDestroyAcceleratorTable( handle );
}

NTSTATUS WINAPI wow64_NtUserDestroyCursor( UINT *args )
{
    HCURSOR cursor = get_handle( &args );
    ULONG arg = get_ulong( &args );

    return NtUserDestroyCursor( cursor, arg );
}

NTSTATUS WINAPI wow64_NtUserDestroyInputContext( UINT *args )
{
    HIMC handle = get_handle( &args );

    return NtUserDestroyInputContext( handle );
}

NTSTATUS WINAPI wow64_NtUserDestroyMenu( UINT *args )
{
    HMENU handle = get_handle( &args );

    return NtUserDestroyMenu( handle );
}

NTSTATUS WINAPI wow64_NtUserDestroyWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserDestroyWindow( hwnd );
}

NTSTATUS WINAPI wow64_NtUserDisableThreadIme( UINT *args )
{
    DWORD thread_id = get_ulong( &args );

    return NtUserDisableThreadIme( thread_id );
}

NTSTATUS WINAPI wow64_NtUserDispatchMessage( UINT *args )
{
    const MSG32 *msg32 = get_ptr( &args );
    MSG msg;

    return NtUserDispatchMessage( msg_32to64( &msg, msg32 ));
}

NTSTATUS WINAPI wow64_NtUserDragDetect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );

    return NtUserDragDetect( hwnd, x, y );
}

NTSTATUS WINAPI wow64_NtUserDrawIconEx( UINT *args )
{
    HDC hdc = get_handle( &args );
    int x0 = get_ulong( &args );
    int y0 = get_ulong( &args );
    HICON icon = get_handle( &args );
    int width = get_ulong( &args );
    int height = get_ulong( &args );
    UINT istep = get_ulong( &args );
    HBRUSH hbr = get_handle( &args );
    UINT flags = get_ulong( &args );

    return NtUserDrawIconEx( hdc, x0, y0, icon, width, height, istep, hbr, flags );
}

NTSTATUS WINAPI wow64_NtUserEmptyClipboard( UINT *args )
{
    return NtUserEmptyClipboard();
}

NTSTATUS WINAPI wow64_NtUserEnableMenuItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserEnableMenuItem( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserEnableScrollBar( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT bar = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserEnableScrollBar( hwnd, bar, flags );
}

NTSTATUS WINAPI wow64_NtUserEndDeferWindowPosEx( UINT *args )
{
    HDWP hdwp = get_handle( &args );
    BOOL async = get_ulong( &args );

    return NtUserEndDeferWindowPosEx( hdwp, async );
}

NTSTATUS WINAPI wow64_NtUserEndMenu( UINT *args )
{
    return NtUserEndMenu();
}

NTSTATUS WINAPI wow64_NtUserEnumDisplayDevices( UINT *args )
{
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DWORD index = get_ulong( &args );
    DISPLAY_DEVICEW *info = get_ptr( &args );
    DWORD flags = get_ulong( &args );

    UNICODE_STRING device;

    return NtUserEnumDisplayDevices( unicode_str_32to64( &device, device32 ), index, info, flags );
}

NTSTATUS WINAPI wow64_NtUserEnumDisplayMonitors( UINT *args )
{
    HDC hdc = get_handle( &args );
    RECT *rect = get_ptr( &args );
    MONITORENUMPROC proc = get_ptr( &args );
    LPARAM lp = get_ulong( &args );

    return NtUserEnumDisplayMonitors( hdc, rect, proc, lp );
}

NTSTATUS WINAPI wow64_NtUserEnumDisplaySettings( UINT *args )
{
    UNICODE_STRING32 *device32 = get_ptr( &args );
    DWORD mode = get_ulong( &args );
    DEVMODEW *dev_mode = get_ptr( &args );
    DWORD flags = get_ulong( &args );

    UNICODE_STRING device;

    return NtUserEnumDisplaySettings( unicode_str_32to64( &device, device32 ),
                                      mode, dev_mode, flags );
}

NTSTATUS WINAPI wow64_NtUserFindExistingCursorIcon( UINT *args )
{
    UNICODE_STRING32 *module32 = get_ptr( &args );
    UNICODE_STRING32 *res_name32 = get_ptr( &args );
    void *desc = get_ptr( &args );

    UNICODE_STRING module;
    UNICODE_STRING res_name;
    HICON ret;

    ret = NtUserFindExistingCursorIcon( unicode_str_32to64( &module, module32 ),
                                        unicode_str_32to64( &res_name, res_name32 ), desc );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserFindWindowEx( UINT *args )
{
    HWND parent = get_handle( &args );
    HWND child = get_handle( &args );
    UNICODE_STRING32 *class32 = get_ptr( &args );
    UNICODE_STRING32 *title32 = get_ptr( &args );
    ULONG unk = get_ulong( &args );

    UNICODE_STRING class, title;
    HWND ret;

    ret = NtUserFindWindowEx( parent, child, unicode_str_32to64( &class, class32 ),
                              unicode_str_32to64( &title, title32 ), unk );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserFlashWindowEx( UINT *args )
{
    struct
    {
        UINT cbSize;
        ULONG hwnd;
        DWORD dwFlags;
        UINT uCount;
        DWORD dwTimeout;
    } *info32 = get_ptr( &args );

    FLASHWINFO info;

    if (!info32)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (info32->cbSize != sizeof(*info32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize = sizeof(info);
    info.hwnd = UlongToHandle( info32->hwnd );
    info.dwFlags = info32->dwFlags;
    info.uCount = info32->uCount;
    info.dwTimeout = info32->dwTimeout;
    return NtUserFlashWindowEx( &info );
}

NTSTATUS WINAPI wow64_NtUserGetAncestor( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT type = get_ulong( &args );

    return HandleToUlong( NtUserGetAncestor( hwnd, type ));
}

NTSTATUS WINAPI wow64_NtUserGetAsyncKeyState( UINT *args )
{
    INT key = get_ulong( &args );

    return NtUserGetAsyncKeyState( key );
}

NTSTATUS WINAPI wow64_NtUserGetAtomName( UINT *args )
{
    ATOM atom = get_ulong( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUserGetAtomName( atom, unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserGetCaretBlinkTime( UINT *args )
{
    return NtUserGetCaretBlinkTime();
}

NTSTATUS WINAPI wow64_NtUserGetCaretPos( UINT *args )
{
    POINT *pt = get_ptr( &args );

    return NtUserGetCaretPos( pt );
}

NTSTATUS WINAPI wow64_NtUserGetClassInfoEx( UINT *args )
{
    HINSTANCE instance = get_ptr( &args );
    UNICODE_STRING32 *name32 = get_ptr( &args );
    WNDCLASSEXW32 *wc32 = get_ptr( &args );
    struct client_menu_name32 *client_name32 = get_ptr( &args );
    BOOL ansi = get_ulong( &args );

    struct client_menu_name client_name;
    UNICODE_STRING name;
    WNDCLASSEXW wc;

    wc.cbSize = sizeof(wc);
    if (!NtUserGetClassInfoEx( instance, unicode_str_32to64( &name, name32 ), &wc,
                               &client_name, ansi ))
        return FALSE;

    wc32->style = wc.style;
    wc32->lpfnWndProc = PtrToUlong( wc.lpfnWndProc );
    wc32->cbClsExtra = wc.cbClsExtra;
    wc32->cbWndExtra = wc.cbWndExtra;
    wc32->hInstance = HandleToUlong( wc.hInstance );
    wc32->hIcon = HandleToUlong( wc.hIcon );
    wc32->hCursor = HandleToUlong( wc.hCursor );
    wc32->hbrBackground = HandleToUlong( wc.hbrBackground );
    wc32->lpszMenuName = PtrToUlong( wc.lpszMenuName );
    wc32->lpszClassName = PtrToUlong( wc.lpszClassName );
    wc32->hIconSm = HandleToUlong( wc.hIconSm );
    client_menu_name_64to32( &client_name, client_name32 );
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetClassName( UINT *args )
{
    HWND hwnd = get_handle( &args );
    BOOL real = get_ulong( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUserGetClassName( hwnd, real, unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserGetClipboardData( UINT *args )
{
    UINT format = get_ulong( &args );
    struct
    {
        UINT32 data;
        UINT32 size;
        UINT32 data_size;
        UINT   seqno;
        BOOL   data_only;
    } *params32 = get_ptr( &args );

    struct get_clipboard_params params;
    HANDLE ret;

    params.data = UlongToPtr( params32->data );
    params.size = params32->size;

    ret = NtUserGetClipboardData( format, &params );

    params32->data_size = params.data_size;
    params32->seqno     = params.seqno;
    params32->data_only = params.data_only;
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardFormatName( UINT *args )
{
    UINT format = get_ulong( &args );
    WCHAR *buffer = get_ptr( &args );
    INT maxlen = get_ulong( &args );

    return NtUserGetClipboardFormatName( format, buffer, maxlen );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardOwner( UINT *args )
{
    return HandleToUlong( NtUserGetClipboardOwner() );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardSequenceNumber( UINT *args )
{
    return NtUserGetClipboardSequenceNumber();
}

NTSTATUS WINAPI wow64_NtUserGetClipboardViewer( UINT *args )
{
    return HandleToUlong( NtUserGetClipboardViewer() );
}

NTSTATUS WINAPI wow64_NtUserGetCursor( UINT *args )
{
    return HandleToUlong( NtUserGetCursor() );
}

NTSTATUS WINAPI wow64_NtUserGetCursorFrameInfo( UINT *args )
{
    HCURSOR cursor = get_ptr( &args );
    DWORD istep = get_ulong( &args );
    DWORD *rate_jiffies = get_ptr( &args );
    DWORD *num_steps = get_ptr( &args );

    return HandleToUlong( NtUserGetCursorFrameInfo( cursor, istep, rate_jiffies, num_steps ));
}

NTSTATUS WINAPI wow64_NtUserGetCursorInfo( UINT *args )
{
    struct
    {
        DWORD cbSize;
        DWORD flags;
        ULONG hCursor;
        POINT ptScreenPos;
    } *info32 = get_ptr( &args );
    CURSORINFO info;

    if (!info32) return FALSE;
    info.cbSize = sizeof(info);
    if (!NtUserGetCursorInfo( &info )) return FALSE;
    info32->flags = info.flags;
    info32->hCursor = HandleToUlong( info.hCursor );
    info32->ptScreenPos = info.ptScreenPos;
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetDCEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN clip_rgn = get_handle( &args );
    DWORD flags = get_ulong( &args );

    return HandleToUlong( NtUserGetDCEx( hwnd, clip_rgn, flags ));
}

NTSTATUS WINAPI wow64_NtUserGetDisplayConfigBufferSizes( UINT *args )
{
    UINT32 flags = get_ulong( &args );
    UINT32 *num_path_info = get_ptr( &args );
    UINT32 *num_mode_info = get_ptr( &args );

    return NtUserGetDisplayConfigBufferSizes( flags, num_path_info, num_mode_info );
}

NTSTATUS WINAPI wow64_NtUserGetDoubleClickTime( UINT *args )
{
    return NtUserGetDoubleClickTime();
}

NTSTATUS WINAPI wow64_NtUserGetDpiForMonitor( UINT *args )
{
    HMONITOR monitor = get_handle( &args );
    UINT type = get_ulong( &args );
    UINT *x = get_ptr( &args );
    UINT *y = get_ptr( &args );

    return NtUserGetDpiForMonitor( monitor, type, x, y );
}

NTSTATUS WINAPI wow64_NtUserGetForegroundWindow( UINT *args )
{
    return HandleToUlong( NtUserGetForegroundWindow() );
}

NTSTATUS WINAPI wow64_NtUserGetGUIThreadInfo( UINT *args )
{
    DWORD id = get_ulong( &args );
    struct
    {
        DWORD  cbSize;
        DWORD  flags;
        ULONG  hwndActive;
        ULONG  hwndFocus;
        ULONG  hwndCapture;
        ULONG  hwndMenuOwner;
        ULONG  hwndMoveSize;
        ULONG  hwndCaret;
        RECT   rcCaret;
    } *info32 = get_ptr( &args );
    GUITHREADINFO info;

    if (info32->cbSize != sizeof(*info32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize = sizeof(info);
    if (!NtUserGetGUIThreadInfo( id, &info )) return FALSE;
    info32->flags         = info.flags;
    info32->hwndActive    = HandleToUlong( info.hwndActive );
    info32->hwndFocus     = HandleToUlong( info.hwndFocus );
    info32->hwndCapture   = HandleToUlong( info.hwndCapture );
    info32->hwndMenuOwner = HandleToUlong( info.hwndMenuOwner );
    info32->hwndMoveSize  = HandleToUlong( info.hwndMoveSize );
    info32->hwndCaret     = HandleToUlong( info.hwndCaret );
    info32->rcCaret       = info.rcCaret;
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetIconInfo( UINT *args )
{
    HICON icon = get_handle( &args );
    struct
    {
	BOOL   fIcon;
	DWORD  xHotspot;
	DWORD  yHotspot;
	UINT32 hbmMask;
	UINT32 hbmColor;
    } *info32 = get_ptr( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    UNICODE_STRING32 *res_name32 = get_ptr( &args );
    DWORD *bpp = get_ptr( &args );
    LONG unk = get_ulong( &args );

    ICONINFO info;
    UNICODE_STRING module, res_name;

    if (!NtUserGetIconInfo( icon, &info, unicode_str_32to64( &module, module32 ),
                            unicode_str_32to64( &res_name, res_name32 ), bpp, unk ))
        return FALSE;

    info32->fIcon    = info.fIcon;
    info32->xHotspot = info.xHotspot;
    info32->yHotspot = info.yHotspot;
    info32->hbmMask  = HandleToUlong( info.hbmMask );
    info32->hbmColor = HandleToUlong( info.hbmColor );
    if (module32) module32->Length = module.Length;
    if (res_name32) res_name32->Length = res_name.Length;
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetIconSize( UINT *args )
{
    HICON handle = get_handle( &args );
    UINT step = get_ulong( &args );
    LONG *width = get_ptr( &args );
    LONG *height = get_ptr( &args );

    return NtUserGetIconSize( handle, step, width, height );
}

NTSTATUS WINAPI wow64_NtUserGetKeyNameText( UINT *args )
{
    LONG lparam = get_ulong( &args );
    WCHAR *buffer = get_ptr( &args );
    INT size = get_ulong( &args );

    return NtUserGetKeyNameText( lparam, buffer, size );
}

NTSTATUS WINAPI wow64_NtUserGetKeyState( UINT *args )
{
    INT vkey = get_ulong( &args );

    return NtUserGetKeyState( vkey );
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardLayout( UINT *args )
{
    DWORD tid = get_ulong( &args );

    return HandleToUlong( NtUserGetKeyboardLayout( tid ));
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardLayoutList( UINT *args )
{
    INT size = get_ulong( &args );
    UINT32 *layouts32 = get_ptr( &args );
    HKL *layouts = NULL;
    UINT ret, i;

    if (layouts32 && size && !(layouts = Wow64AllocateTemp( size * sizeof(*layouts) )))
        return 0;

    ret = NtUserGetKeyboardLayoutList( size, layouts );
    if (layouts)
        for (i = 0; i < ret; i++) layouts32[i] = HandleToUlong( layouts[i] );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardLayoutName( UINT *args )
{
    WCHAR *name = get_ptr( &args );

    return NtUserGetKeyboardLayoutName( name );
}

NTSTATUS WINAPI wow64_NtUserGetKeyboardState( UINT *args )
{
    BYTE *state = get_ptr( &args );

    return NtUserGetKeyboardState( state );
}

NTSTATUS WINAPI wow64_NtUserGetLayeredWindowAttributes( UINT *args )
{
    HWND hwnd = get_handle( &args );
    COLORREF *key = get_ptr( &args );
    BYTE *alpha = get_ptr( &args );
    DWORD *flags = get_ptr( &args );

    return NtUserGetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

NTSTATUS WINAPI wow64_NtUserGetMenuBarInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    LONG id = get_ulong( &args );
    LONG item = get_ulong( &args );
    struct
    {
        DWORD cbSize;
        RECT  rcBar;
        ULONG hMenu;
        ULONG hwndMenu;
        BOOL  fBarFocused:1;
        BOOL  fFocused:1;
    } *info32 = get_ptr( &args );

    MENUBARINFO info;

    if (info32->cbSize != sizeof(*info32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize = sizeof(info);
    info.rcBar = info32->rcBar;
    info.hMenu = UlongToHandle( info32->hMenu );
    info.hwndMenu = UlongToHandle( info32->hwndMenu );
    info.fBarFocused = info32->fBarFocused;
    info.fFocused = info32->fFocused;
    return NtUserGetMenuBarInfo( hwnd, id, item, &info );
}

NTSTATUS WINAPI wow64_NtUserGetMenuItemRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    RECT *rect = get_ptr( &args );

    return NtUserGetMenuItemRect( hwnd, handle, item, rect );
}

NTSTATUS WINAPI wow64_NtUserGetMessage( UINT *args )
{
    MSG32 *msg32 = get_ptr( &args );
    HWND hwnd = get_handle( &args );
    UINT first = get_ulong( &args );
    UINT last = get_ulong( &args );
    MSG msg;

    if (!NtUserGetMessage( &msg, hwnd, first, last )) return FALSE;
    msg_64to32( &msg, msg32 );
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserGetMouseMovePointsEx( UINT *args )
{
    UINT size = get_ulong( &args );
    MOUSEMOVEPOINT *ptin = get_ptr( &args );
    MOUSEMOVEPOINT *ptout = get_ptr( &args );
    int count = get_ulong( &args );
    DWORD resolution = get_ulong( &args );

    return NtUserGetMouseMovePointsEx( size, ptin, ptout, count, resolution );
}

NTSTATUS WINAPI wow64_NtUserGetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );
    DWORD *needed = get_ptr( &args );

    return NtUserGetObjectInformation( handle, index, info, len, needed );
}

NTSTATUS WINAPI wow64_NtUserGetOpenClipboardWindow( UINT *args )
{
    return HandleToUlong( NtUserGetOpenClipboardWindow() );
}

NTSTATUS WINAPI wow64_NtUserGetPriorityClipboardFormat( UINT *args )
{
    UINT *list = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserGetPriorityClipboardFormat( list, count );
}

NTSTATUS WINAPI wow64_NtUserGetProcessDpiAwarenessContext( UINT *args )
{
    HANDLE process = get_handle( &args );

    return NtUserGetProcessDpiAwarenessContext( process );
}

NTSTATUS WINAPI wow64_NtUserGetProcessWindowStation( UINT *args )
{
    return HandleToUlong( NtUserGetProcessWindowStation() );
}

NTSTATUS WINAPI wow64_NtUserGetProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );

    return HandleToUlong( NtUserGetProp( hwnd, str ));
}

NTSTATUS WINAPI wow64_NtUserGetQueueStatus( UINT *args )
{
    UINT flags = get_ulong( &args );

    return NtUserGetQueueStatus( flags );
}

NTSTATUS WINAPI wow64_NtUserGetRawInputBuffer( UINT *args )
{
    RAWINPUT *data = get_ptr( &args );
    UINT *data_size = get_ptr( &args );
    UINT header_size = get_ulong( &args );

    if (header_size != sizeof(RAWINPUTHEADER32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    /* RAWINPUT has different sizes on 32-bit and 64-bit, but no translation is
     * done. The function actually returns different structures depending on
     * whether it's operating under WoW64 or not. */
    return NtUserGetRawInputBuffer( data, data_size, sizeof(RAWINPUTHEADER) );
}

NTSTATUS WINAPI wow64_NtUserGetRawInputData( UINT *args )
{
    HRAWINPUT handle = get_handle( &args );
    UINT command = get_ulong( &args );
    void *data = get_ptr( &args );
    UINT *data_size = get_ptr( &args );
    UINT header_size = get_ulong( &args );

    if (header_size != sizeof(RAWINPUTHEADER32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    switch (command)
    {
    case RID_INPUT:
        if (data)
        {
            UINT data_size64, body_size, ret;
            RAWINPUTHEADER32 *data32 = data;
            RAWINPUTHEADER *data64 = NULL;

            data_size64 = *data_size + sizeof(RAWINPUTHEADER);
            if (!(data64 = Wow64AllocateTemp( data_size64 )))
            {
                SetLastError( STATUS_NO_MEMORY );
                return ~0u;
            }

            ret = NtUserGetRawInputData( handle, command, data64, &data_size64, sizeof(RAWINPUTHEADER) );
            if (ret == ~0u) return ret;

            body_size = ret - sizeof(RAWINPUTHEADER);
            if (*data_size < sizeof(RAWINPUTHEADER32) + body_size)
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return ~0u;
            }

            data32->dwType = data64->dwType;
            data32->dwSize = sizeof(RAWINPUTHEADER32) + body_size;
            data32->hDevice = (UINT_PTR)data64->hDevice;
            data32->wParam = data64->wParam;
            memcpy( data32 + 1, data64 + 1, body_size );
            return sizeof(RAWINPUTHEADER32) + body_size;
        }
        else
        {
            UINT data_size64, ret;

            ret = NtUserGetRawInputData( handle, command, NULL, &data_size64, sizeof(RAWINPUTHEADER) );
            if (ret == ~0u) return ret;
            *data_size = data_size64 - sizeof(RAWINPUTHEADER) + sizeof(RAWINPUTHEADER32);
            return 0;
        }

    case RID_HEADER:
    {
        UINT data_size64 = sizeof(RAWINPUTHEADER);
        RAWINPUTHEADER32 *data32 = data;
        RAWINPUTHEADER data64;
        UINT ret;

        if (!data)
        {
            *data_size = sizeof(RAWINPUTHEADER32);
            return 0;
        }

        if (*data_size < sizeof(RAWINPUTHEADER32))
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return ~0u;
        }

        ret = NtUserGetRawInputData( handle, command, &data64, &data_size64, sizeof(RAWINPUTHEADER) );
        if (ret == ~0u) return ret;
        data32->dwType = data64.dwType;
        data32->dwSize = data64.dwSize - sizeof(RAWINPUTHEADER) + sizeof(RAWINPUTHEADER32);
        data32->hDevice = (UINT_PTR)data64.hDevice;
        data32->wParam = data64.wParam;
        return sizeof(RAWINPUTHEADER32);
    }

    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }
}

NTSTATUS WINAPI wow64_NtUserGetRawInputDeviceInfo( UINT *args )
{
    HANDLE handle = get_handle( &args );
    UINT command = get_ulong( &args );
    void *data = get_ptr( &args );
    UINT *data_size = get_ptr( &args );

    return NtUserGetRawInputDeviceInfo( handle, command, data, data_size );
}

NTSTATUS WINAPI wow64_NtUserGetRawInputDeviceList( UINT *args )
{
    RAWINPUTDEVICELIST32 *devices32 = get_ptr( &args );
    UINT *count = get_ptr( &args );
    UINT size = get_ulong( &args );

    if (size != sizeof(RAWINPUTDEVICELIST32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (devices32)
    {
        RAWINPUTDEVICELIST *devices64;
        unsigned int ret, i;

        if (!(devices64 = Wow64AllocateTemp( (*count) * sizeof(*devices64) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return ~0u;
        }

        ret = NtUserGetRawInputDeviceList( devices64, count, sizeof(RAWINPUTDEVICELIST) );
        if (ret == ~0u) return ret;

        for (i = 0; i < *count; ++i)
        {
            devices32[i].hDevice = (UINT_PTR)devices64[i].hDevice;
            devices32[i].dwType  = devices64[i].dwType;
        }
        return ret;
    }
    else
    {
        return NtUserGetRawInputDeviceList( NULL, count, sizeof(RAWINPUTDEVICELIST) );
    }
}

NTSTATUS WINAPI wow64_NtUserRegisterClassExWOW( UINT *args )
{
    const WNDCLASSEXW32 *wc32 = get_ptr( &args );
    UNICODE_STRING32 *name32 = get_ptr( &args );
    UNICODE_STRING32 *version32 = get_ptr( &args );
    struct client_menu_name32 *client_name32 = get_ptr( &args );
    DWORD fnid = get_ulong( &args );
    DWORD flags = get_ulong( &args );
    DWORD *wow = get_ptr( &args );

    struct client_menu_name client_name;
    UNICODE_STRING name, version;
    WNDCLASSEXW wc;

    if (wc32->cbSize != sizeof(*wc32))
    {
         SetLastError( ERROR_INVALID_PARAMETER );
         return 0;
    }

    wc.cbSize = sizeof(wc);
    wc.style = wc32->style;
    wc.lpfnWndProc = UlongToPtr( wc32->lpfnWndProc );
    wc.cbClsExtra = wc32->cbClsExtra;
    wc.cbWndExtra = wc32->cbWndExtra;
    wc.hInstance = UlongToHandle( wc32->hInstance );
    wc.hIcon = UlongToHandle( wc32->hIcon );
    wc.hCursor = UlongToHandle( wc32->hCursor );
    wc.hbrBackground = UlongToHandle( wc32->hbrBackground );
    wc.lpszMenuName = UlongToPtr( wc32->lpszMenuName );
    wc.lpszClassName = UlongToPtr( wc32->lpszClassName );
    wc.hIconSm = UlongToHandle( wc32->hIconSm );

    return NtUserRegisterClassExWOW( &wc,
                                     unicode_str_32to64( &name, name32 ),
                                     unicode_str_32to64( &version, version32 ),
                                     client_menu_name_32to64( &client_name, client_name32 ),
                                     fnid, flags, wow );
}

NTSTATUS WINAPI wow64_NtUserGetRegisteredRawInputDevices( UINT *args )
{
    RAWINPUTDEVICE32 *devices32 = get_ptr( &args );
    UINT *count = get_ptr( &args );
    UINT size = get_ulong( &args );

    if (size != sizeof(RAWINPUTDEVICE32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (devices32)
    {
        RAWINPUTDEVICE *devices64;
        unsigned int ret, i;

        if (!(devices64 = Wow64AllocateTemp( (*count) * sizeof(*devices64) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return ~0u;
        }

        ret = NtUserGetRegisteredRawInputDevices( devices64, count, sizeof(RAWINPUTDEVICE) );
        if (ret == ~0u) return ret;

        for (i = 0; i < *count; ++i)
        {
            devices32[i].usUsagePage = devices64[i].usUsagePage;
            devices32[i].usUsage     = devices64[i].usUsage;
            devices32[i].dwFlags     = devices64[i].dwFlags;
            devices32[i].hwndTarget  = (ULONG_PTR)devices64[i].hwndTarget;
        }
        return ret;
    }
    else
    {
        return NtUserGetRegisteredRawInputDevices( NULL, count, sizeof(RAWINPUTDEVICE) );
    }
}

NTSTATUS WINAPI wow64_NtUserGetScrollBarInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    LONG id = get_ulong( &args );
    SCROLLBARINFO *info = get_ptr( &args );

    return NtUserGetScrollBarInfo( hwnd, id, info );
}

NTSTATUS WINAPI wow64_NtUserGetSystemDpiForProcess( UINT *args )
{
    HANDLE process = get_handle( &args );

    return NtUserGetSystemDpiForProcess( process );
}

NTSTATUS WINAPI wow64_NtUserGetSystemMenu( UINT *args )
{
    HWND hwnd = get_handle( &args );
    BOOL revert = get_ulong( &args );

    return HandleToUlong( NtUserGetSystemMenu( hwnd, revert ));
}

NTSTATUS WINAPI wow64_NtUserGetThreadDesktop( UINT *args )
{
    DWORD thread = get_ulong( &args );

    return HandleToUlong( NtUserGetThreadDesktop( thread ));
}

NTSTATUS WINAPI wow64_NtUserGetTitleBarInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    TITLEBARINFO *info = get_ptr( &args );

    return NtUserGetTitleBarInfo( hwnd, info );
}

NTSTATUS WINAPI wow64_NtUserGetUpdateRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    RECT *rect = get_ptr( &args );
    BOOL erase = get_ulong( &args );

    return NtUserGetUpdateRect( hwnd, rect, erase );
}

NTSTATUS WINAPI wow64_NtUserGetUpdateRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    BOOL erase = get_ulong( &args );

    return NtUserGetUpdateRgn( hwnd, hrgn, erase );
}

NTSTATUS WINAPI wow64_NtUserGetUpdatedClipboardFormats( UINT *args )
{
    UINT *formats = get_ptr( &args );
    UINT size = get_ulong( &args );
    UINT *out_size = get_ptr( &args );

    return NtUserGetUpdatedClipboardFormats( formats, size, out_size );
}

NTSTATUS WINAPI wow64_NtUserGetWindowPlacement( UINT *args )
{
    HWND hwnd = get_handle( &args );
    WINDOWPLACEMENT *placement = get_ptr( &args );

    return NtUserGetWindowPlacement( hwnd, placement );
}

NTSTATUS WINAPI wow64_NtUserGetWindowRgnEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    UINT unk = get_ulong( &args );

    return NtUserGetWindowRgnEx( hwnd, hrgn, unk );
}

NTSTATUS WINAPI wow64_NtUserHideCaret( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserHideCaret( hwnd );
}

NTSTATUS WINAPI wow64_NtUserHiliteMenuItem( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    UINT hilite = get_ulong( &args );

    return NtUserHiliteMenuItem( hwnd, handle, item, hilite );
}

NTSTATUS WINAPI wow64_NtUserInitializeClientPfnArrays( UINT *args )
{
    FIXME( "\n" );
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS WINAPI wow64_NtUserInternalGetWindowIcon( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT type = get_ulong( &args );

    return HandleToUlong( NtUserInternalGetWindowIcon( hwnd, type ));
}

NTSTATUS WINAPI wow64_NtUserInternalGetWindowText( UINT *args )
{
    HWND hwnd = get_handle( &args );
    WCHAR *text = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserInternalGetWindowText( hwnd, text, count );
}

NTSTATUS WINAPI wow64_NtUserInvalidateRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const RECT *rect = get_ptr( &args );
    BOOL erase = get_ulong( &args );

    return NtUserInvalidateRect( hwnd, rect, erase );
}

NTSTATUS WINAPI wow64_NtUserInvalidateRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    BOOL erase = get_ulong( &args );

    return NtUserInvalidateRgn( hwnd, hrgn, erase );
}

NTSTATUS WINAPI wow64_NtUserIsClipboardFormatAvailable( UINT *args )
{
    UINT format = get_ulong( &args );

    return NtUserIsClipboardFormatAvailable( format );
}

NTSTATUS WINAPI wow64_NtUserKillTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );

    return NtUserKillTimer( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserLockWindowUpdate( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserLockWindowUpdate( hwnd );
}

NTSTATUS WINAPI wow64_NtUserMapVirtualKeyEx( UINT *args )
{
    UINT code = get_ulong( &args );
    UINT type = get_ulong( &args );
    HKL layout = get_handle( &args );

    return NtUserMapVirtualKeyEx( code, type, layout );
}

NTSTATUS WINAPI wow64_NtUserMenuItemFromPoint( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );

    return NtUserMenuItemFromPoint( hwnd, handle, x, y );
}

NTSTATUS WINAPI wow64_NtUserMessageCall( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT msg = get_ulong( &args );
    UINT wparam = get_ulong( &args );
    UINT lparam = get_ulong( &args );
    void *result_info = get_ptr( &args );
    UINT type = get_ulong ( &args );
    BOOL ansi = get_ulong( &args );

    FIXME( "%p %x %x %x %p %x %x\n", hwnd, msg, wparam, lparam, result_info, type, ansi );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserMoveWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    INT cx = get_ulong( &args );
    INT cy = get_ulong( &args );
    BOOL repaint = get_ulong( &args );

    return NtUserMoveWindow( hwnd, x, y, cx, cy, repaint );
}

NTSTATUS WINAPI wow64_NtUserMsgWaitForMultipleObjectsEx( UINT *args )
{
    DWORD count = get_ulong( &args );
    const ULONG *handles32 = get_ptr( &args );
    DWORD timeout = get_ulong( &args );
    DWORD mask = get_ulong( &args );
    DWORD flags = get_ulong( &args );

    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    unsigned int i;

    if (count > ARRAYSIZE(handles))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }
    for (i = 0; i < count; i++) handles[i] = UlongToHandle( handles32[i] );

    return NtUserMsgWaitForMultipleObjectsEx( count, handles, timeout, mask, flags );
}

NTSTATUS WINAPI wow64_NtUserNotifyWinEvent( UINT *args )
{
    DWORD event = get_ulong( &args );
    HWND hwnd = get_handle( &args );
    LONG object_id = get_ulong( &args );
    LONG child_id = get_ulong( &args );

    NtUserNotifyWinEvent( event, hwnd, object_id, child_id );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserOpenClipboard( UINT *args )
{
    HWND hwnd = get_handle( &args );
    ULONG unk = get_ulong( &args );

    return NtUserOpenClipboard( hwnd, unk );
}

NTSTATUS WINAPI wow64_NtUserOpenDesktop( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    DWORD flags = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE ret;

    ret = NtUserOpenDesktop( objattr_32to64( &attr, attr32 ), flags, access );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserOpenInputDesktop( UINT *args )
{
    DWORD flags = get_ulong( &args );
    BOOL inherit = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    return HandleToUlong( NtUserOpenInputDesktop( flags, inherit, access ));
}

NTSTATUS WINAPI wow64_NtUserOpenWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserOpenWindowStation( objattr_32to64( &attr, attr32 ), access ));
}

NTSTATUS WINAPI wow64_NtUserPeekMessage( UINT *args )
{
    MSG32 *msg32 = get_ptr( &args );
    HWND hwnd = get_handle( &args );
    UINT first = get_ulong( &args );
    UINT last = get_ulong( &args );
    UINT flags = get_ulong( &args );
    MSG msg;

    if (!NtUserPeekMessage( &msg, hwnd, first, last, flags )) return FALSE;
    msg_64to32( &msg, msg32 );
    return TRUE;
}

NTSTATUS WINAPI wow64_NtUserPostMessage( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT msg = get_ulong( &args );
    WPARAM wparam = get_ulong( &args );
    LPARAM lparam = get_ulong( &args );

    return NtUserPostMessage( hwnd, msg, wparam, lparam );
}

NTSTATUS WINAPI wow64_NtUserPostThreadMessage( UINT *args )
{
    DWORD thread = get_ulong( &args );
    UINT msg = get_ulong( &args );
    WPARAM wparam = get_ulong( &args );
    LPARAM lparam = get_ulong( &args );

    return NtUserPostThreadMessage( thread, msg, wparam, lparam );
}

NTSTATUS WINAPI wow64_NtUserQueryInputContext( UINT *args )
{
    HIMC handle = get_handle( &args );
    UINT attr = get_ulong( &args );

    return NtUserQueryInputContext( handle, attr );
}

NTSTATUS WINAPI wow64_NtUserRedrawWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const RECT *rect = get_ptr( &args );
    HRGN hrgn = get_handle( &args );
    UINT flags = get_ulong( &args );

    return NtUserRedrawWindow( hwnd, rect, hrgn, flags );
}

NTSTATUS WINAPI wow64_NtUserRegisterHotKey( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT id = get_ulong( &args );
    UINT modifiers = get_ulong( &args );
    UINT vk = get_ulong( &args );

    return NtUserRegisterHotKey( hwnd, id, modifiers, vk );
}

NTSTATUS WINAPI wow64_NtUserRegisterRawInputDevices( UINT *args )
{
    const RAWINPUTDEVICE32 *devices32 = get_ptr( &args );
    UINT count = get_ulong( &args );
    UINT size = get_ulong( &args );

    RAWINPUTDEVICE *devices64;
    unsigned int i;

    if (size != sizeof(RAWINPUTDEVICE32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(devices64 = Wow64AllocateTemp( count * sizeof(*devices64) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    for (i = 0; i < count; ++i)
    {
        devices64[i].usUsagePage = devices32[i].usUsagePage;
        devices64[i].usUsage = devices32[i].usUsage;
        devices64[i].dwFlags = devices32[i].dwFlags;
        devices64[i].hwndTarget = UlongToPtr( devices32[i].hwndTarget );
    }

    return NtUserRegisterRawInputDevices( devices64, count, sizeof(*devices64) );
}

NTSTATUS WINAPI wow64_NtUserRemoveClipboardFormatListener( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserRemoveClipboardFormatListener( hwnd );
}

NTSTATUS WINAPI wow64_NtUserRemoveMenu( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserRemoveMenu( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserRemoveProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );

    return HandleToUlong( NtUserRemoveProp( hwnd, str ));
}

NTSTATUS WINAPI wow64_NtUserScrollWindowEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT dx = get_ulong( &args );
    INT dy = get_ulong( &args );
    const RECT *rect = get_ptr( &args );
    const RECT *clip_rect = get_ptr( &args );
    HRGN update_rgn = get_handle( &args );
    RECT *update_rect = get_ptr( &args );
    UINT flags = get_ulong( &args );

    return NtUserScrollWindowEx( hwnd, dx, dy, rect, clip_rect, update_rgn, update_rect, flags );
}

NTSTATUS WINAPI wow64_NtUserSendInput( UINT *args )
{
    UINT count = get_ulong( &args );
    INPUT32 *inputs32 = get_ptr( &args );
    int size = get_ulong( &args );

    INPUT *inputs = NULL;
    unsigned int i;

    if (size != sizeof(*inputs32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (count && !(inputs = Wow64AllocateTemp( count * sizeof(*inputs) )))
        return 0;

    for (i = 0; i < count; i++)
    {
        inputs[i].type = inputs32[i].type;
        switch (inputs[i].type)
        {
        case INPUT_MOUSE:
            inputs[i].mi.dx          = inputs32[i].mi.dx;
            inputs[i].mi.dy          = inputs32[i].mi.dy;
            inputs[i].mi.mouseData   = inputs32[i].mi.mouseData;
            inputs[i].mi.dwFlags     = inputs32[i].mi.dwFlags;
            inputs[i].mi.time        = inputs32[i].mi.time;
            inputs[i].mi.dwExtraInfo = inputs32[i].mi.dwExtraInfo;
            break;
        case INPUT_KEYBOARD:
            inputs[i].ki.wVk         = inputs32[i].ki.wVk;
            inputs[i].ki.wScan       = inputs32[i].ki.wScan;
            inputs[i].ki.dwFlags     = inputs32[i].ki.dwFlags;
            inputs[i].ki.time        = inputs32[i].ki.time;
            inputs[i].ki.dwExtraInfo = inputs32[i].ki.dwExtraInfo;
            break;
        case INPUT_HARDWARE:
            inputs[i].hi = inputs32[i].hi;
            break;
        }
    }

    return NtUserSendInput( count, inputs, sizeof(*inputs) );
}

NTSTATUS WINAPI wow64_NtUserSetActiveWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetActiveWindow( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetCapture( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetCapture( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetClassLong( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    return NtUserSetClassLong( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetClassLongPtr( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG_PTR newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    return NtUserSetClassLongPtr( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetClassWord( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    WORD newval = get_ulong( &args );

    return NtUserSetClassWord( hwnd, offset, newval );
}

NTSTATUS WINAPI wow64_NtUserSetClipboardData( UINT *args )
{
    UINT format = get_ulong( &args );
    HANDLE handle = get_handle( &args );
    struct
    {
        UINT32 data;
        UINT32 size;
        BOOL   cache_only;
        UINT   seqno;
    } *params32 = get_ptr( &args );

    struct set_clipboard_params params;
    params.data       = UlongToPtr( params32->data );
    params.size       = params32->size;
    params.cache_only = params32->cache_only;
    params.seqno      = params32->seqno;

    return NtUserSetClipboardData( format, handle, &params );
}

NTSTATUS WINAPI wow64_NtUserSetClipboardViewer( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetClipboardViewer( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetCursor( UINT *args )
{
    HCURSOR cursor = get_handle( &args );

    return HandleToUlong( NtUserSetCursor( cursor ));
}

NTSTATUS WINAPI wow64_NtUserSetCursorIconData( UINT *args )
{
    HCURSOR cursor = get_handle( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    UNICODE_STRING32 *res_name32 = get_ptr( &args );
    void *desc = get_ptr( &args );

    FIXME( "%p %p %p %p\n", cursor, module32, res_name32, desc );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserSetCursorPos( UINT *args )
{
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );

    return NtUserSetCursorPos( x, y );
}

NTSTATUS WINAPI wow64_NtUserSetFocus( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return HandleToUlong( NtUserSetFocus( hwnd ));
}

NTSTATUS WINAPI wow64_NtUserSetInternalWindowPos( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT cmd = get_ulong( &args );
    RECT *rect = get_ptr( &args );
    POINT *pt = get_ptr( &args );

    NtUserSetInternalWindowPos( hwnd, cmd, rect, pt );
    return 0;
}

NTSTATUS WINAPI wow64_NtUserSetKeyboardState( UINT *args )
{
    BYTE *state = get_ptr( &args );

    return NtUserSetKeyboardState( state );
}

NTSTATUS WINAPI wow64_NtUserSetLayeredWindowAttributes( UINT *args )
{
    HWND hwnd = get_handle( &args );
    COLORREF key = get_ulong( &args );
    BYTE alpha = get_ulong( &args );
    DWORD flags = get_ulong( &args );

    return NtUserSetLayeredWindowAttributes( hwnd, key, alpha, flags );
}

NTSTATUS WINAPI wow64_NtUserSetMenu( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU menu = get_handle( &args );

    return NtUserSetMenu( hwnd, menu );
}

NTSTATUS WINAPI wow64_NtUserSetMenuContextHelpId( UINT *args )
{
    HMENU menu = get_handle( &args );
    DWORD id = get_ulong( &args );

    return NtUserSetMenuContextHelpId( menu, id );
}

NTSTATUS WINAPI wow64_NtUserSetMenuDefaultItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    UINT bypos = get_ulong( &args );

    return NtUserSetMenuDefaultItem( handle, item, bypos );
}

NTSTATUS WINAPI wow64_NtUserSetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );

    return NtUserSetObjectInformation( handle, index, info, len );
}

NTSTATUS WINAPI wow64_NtUserSetParent( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HWND parent = get_handle( &args );

    return HandleToUlong( NtUserSetParent( hwnd, parent ));
}

NTSTATUS WINAPI wow64_NtUserSetProcessDpiAwarenessContext( UINT *args )
{
    ULONG awareness = get_ulong( &args );
    ULONG unknown = get_ulong( &args );

    return NtUserSetProcessDpiAwarenessContext( awareness, unknown );
}

NTSTATUS WINAPI wow64_NtUserSetProcessWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserSetProcessWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserSetProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );
    HANDLE handle = get_handle( &args );

    return NtUserSetProp( hwnd, str, handle );
}

NTSTATUS WINAPI wow64_NtUserSetScrollInfo( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT bar = get_ulong( &args );
    const SCROLLINFO *info = get_ptr( &args );
    BOOL redraw = get_ulong( &args );

    return NtUserSetScrollInfo( hwnd, bar, info, redraw );
}

NTSTATUS WINAPI wow64_NtUserSetSysColors( UINT *args )
{
    INT count = get_ulong( &args );
    const INT *colors = get_ptr( &args );
    const COLORREF *values = get_ptr( &args );

    return NtUserSetSysColors( count, colors, values );
}

NTSTATUS WINAPI wow64_NtUserSetSystemMenu( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU menu = get_handle( &args );

    return NtUserSetSystemMenu( hwnd, menu );
}

NTSTATUS WINAPI wow64_NtUserSetSystemTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );
    UINT timeout = get_ulong( &args );

    return NtUserSetSystemTimer( hwnd, id, timeout );
}

NTSTATUS WINAPI wow64_NtUserSetThreadDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserSetThreadDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserSetTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );
    UINT timeout = get_ulong( &args );
    TIMERPROC proc = get_ptr( &args );
    ULONG tolerance = get_ulong( &args );

    return NtUserSetTimer( hwnd, id, timeout, proc, tolerance );
}

NTSTATUS WINAPI wow64_NtUserSetWinEventHook( UINT *args )
{
    DWORD event_min = get_ulong( &args );
    DWORD event_max = get_ulong( &args );
    HMODULE inst = get_handle( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    WINEVENTPROC proc = get_ptr(&args );
    DWORD pid = get_ulong( &args );
    DWORD tid = get_ulong( &args );
    DWORD flags = get_ulong( &args );
    UNICODE_STRING module;
    HWINEVENTHOOK ret;

    ret = NtUserSetWinEventHook( event_min, event_max, inst,
                                 unicode_str_32to64( &module, module32 ),
                                 proc, pid, tid, flags );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserSetWindowLong( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    return NtUserSetWindowLong( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetWindowLongPtr( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    LONG_PTR newval = get_ulong( &args );
    BOOL ansi = get_ulong( &args );

    return NtUserSetWindowLongPtr( hwnd, offset, newval, ansi );
}

NTSTATUS WINAPI wow64_NtUserSetWindowPlacement( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WINDOWPLACEMENT *wpl = get_ptr( &args );

    return NtUserSetWindowPlacement( hwnd, wpl );
}

NTSTATUS WINAPI wow64_NtUserSetWindowPos( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HWND after = get_handle( &args );
    INT x = get_ulong( &args );
    INT y = get_ulong( &args );
    INT cx = get_ulong( &args );
    INT cy = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserSetWindowPos( hwnd, after, x, y, cx, cy, flags );
}

NTSTATUS WINAPI wow64_NtUserSetWindowRgn( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    BOOL redraw = get_ulong( &args );

    return NtUserSetWindowRgn( hwnd, hrgn, redraw );
}

NTSTATUS WINAPI wow64_NtUserSetWindowWord( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT offset = get_ulong( &args );
    WORD newval = get_ulong( &args );

    return NtUserSetWindowWord( hwnd, offset, newval );
}

NTSTATUS WINAPI wow64_NtUserSetWindowsHookEx( UINT *args )
{
    HINSTANCE inst = get_handle( &args );
    UNICODE_STRING32 *module32 = get_ptr( &args );
    DWORD tid = get_ulong( &args );
    INT id = get_ulong( &args );
    HOOKPROC proc = get_ptr( &args );
    BOOL ansi = get_ulong( &args );
    UNICODE_STRING module;
    HHOOK ret;

    ret = NtUserSetWindowsHookEx( inst, unicode_str_32to64( &module, module32 ),
                                  tid, id, proc, ansi );
    return HandleToUlong( ret );
}

NTSTATUS WINAPI wow64_NtUserShowCaret( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserShowCaret( hwnd );
}

NTSTATUS WINAPI wow64_NtUserShowCursor( UINT *args )
{
    BOOL show = get_ulong( &args );

    return NtUserShowCursor( show );
}

NTSTATUS WINAPI wow64_NtUserShowScrollBar( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT bar = get_ulong( &args );
    BOOL show = get_ulong( &args );

    return NtUserShowScrollBar( hwnd, bar, show );
}

NTSTATUS WINAPI wow64_NtUserShowWindow( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT cmd = get_ulong( &args );

    return NtUserShowWindow( hwnd, cmd );
}

NTSTATUS WINAPI wow64_NtUserShowWindowAsync( UINT *args )
{
    HWND hwnd = get_handle( &args );
    INT cmd = get_ulong( &args );

    return NtUserShowWindowAsync( hwnd, cmd );
}

NTSTATUS WINAPI wow64_NtUserSystemParametersInfo( UINT *args )
{
    UINT action = get_ulong( &args );
    UINT val = get_ulong( &args );
    void *ptr = get_ptr( &args );
    UINT winini = get_ulong( &args );

    switch (action)
    {
    case SPI_GETSERIALKEYS:
        if (ptr)
        {
            struct
            {
                UINT  cbSize;
                DWORD dwFlags;
                ULONG lpszActivePort;
                ULONG lpszPort;
                UINT  iBaudRate;
                UINT  iPortState;
                UINT  iActive;
            } *keys32 = ptr;
            SERIALKEYSW keys;

            if (keys32->cbSize != sizeof(*keys32)) return FALSE;
            keys.cbSize = sizeof(keys);
            if (!NtUserSystemParametersInfo( action, val, &keys, winini )) return FALSE;
            keys32->dwFlags = keys.dwFlags;
            keys32->lpszActivePort = PtrToUlong( keys.lpszActivePort );
            keys32->lpszPort = PtrToUlong( keys.lpszPort );
            keys32->iBaudRate = keys.iBaudRate;
            keys32->iPortState = keys.iPortState;
            keys32->iActive = keys.iActive;
            return TRUE;
        }
        break;

    case SPI_GETSOUNDSENTRY:
        if (ptr)
        {
            struct
            {
                UINT  cbSize;
                DWORD dwFlags;
                DWORD iFSTextEffect;
                DWORD iFSTextEffectMSec;
                DWORD iFSTextEffectColorBits;
                DWORD iFSGrafEffect;
                DWORD iFSGrafEffectMSec;
                DWORD iFSGrafEffectColor;
                DWORD iWindowsEffect;
                DWORD iWindowsEffectMSec;
                ULONG lpszWindowsEffectDLL;
                DWORD iWindowsEffectOrdinal;
            } *entry32 = ptr;
            SOUNDSENTRYW entry;

            if (entry32->cbSize != sizeof(*entry32)) return FALSE;
            entry.cbSize = sizeof(entry);
            if (!NtUserSystemParametersInfo( action, val, &entry, winini )) return FALSE;
            entry32->dwFlags = entry.dwFlags;
            entry32->iFSTextEffect = entry.iFSTextEffect;
            entry32->iFSTextEffectMSec = entry.iFSTextEffectMSec;
            entry32->iFSTextEffectColorBits = entry.iFSTextEffectColorBits;
            entry32->iFSGrafEffect = entry.iFSGrafEffect;
            entry32->iFSGrafEffectMSec = entry.iFSGrafEffectMSec;
            entry32->iFSGrafEffectColor = entry.iFSGrafEffectColor;
            entry32->iWindowsEffect = entry.iWindowsEffect;
            entry32->iWindowsEffectMSec = entry.iWindowsEffectMSec;
            entry32->lpszWindowsEffectDLL = PtrToUlong( entry.lpszWindowsEffectDLL );
            entry32->iWindowsEffectOrdinal = entry.iWindowsEffectOrdinal;
            return TRUE;
        }
        break;

    case SPI_GETHIGHCONTRAST:
        if (ptr)
        {
            struct
            {
                UINT  cbSize;
                DWORD dwFlags;
                ULONG lpszDefaultScheme;
            } *info32 = ptr;
            HIGHCONTRASTW info;

            if (info32->cbSize != sizeof(*info32)) return FALSE;
            info.cbSize = sizeof(info);
            if (!NtUserSystemParametersInfo( action, val, &info, winini )) return FALSE;
            info32->dwFlags = info.dwFlags;
            info32->lpszDefaultScheme = PtrToUlong( info.lpszDefaultScheme );
            return TRUE;
        }
        break;
    }

    return NtUserSystemParametersInfo( action, val, ptr, winini );
}

NTSTATUS WINAPI wow64_NtUserSystemParametersInfoForDpi( UINT *args )
{
    UINT action = get_ulong( &args );
    UINT val = get_ulong( &args );
    void *ptr = get_ptr( &args );
    UINT winini = get_ulong( &args );
    UINT dpi = get_ulong( &args );

    return NtUserSystemParametersInfoForDpi( action, val, ptr, winini, dpi );
}

NTSTATUS WINAPI wow64_NtUserThunkedMenuInfo( UINT *args )
{
    HMENU menu = get_handle( &args );
    const struct
    {
        DWORD cbSize;
        DWORD fMask;
        DWORD dwStyle;
        UINT  cyMax;
        ULONG hbrBack;
        DWORD dwContextHelpID;
        ULONG dwMenuData;
    } *info32 = get_ptr( &args );
    MENUINFO info;

    if (info32)
    {
        info.cbSize = sizeof(info);
        info.fMask = info32->fMask;
        info.dwStyle = info32->dwStyle;
        info.cyMax = info32->cyMax;
        info.hbrBack = UlongToHandle( info32->hbrBack );
        info.dwContextHelpID = info32->dwContextHelpID;
        info.dwMenuData = info32->dwMenuData;
    }

    return NtUserThunkedMenuInfo( menu, info32 ? &info : NULL );
}

NTSTATUS WINAPI wow64_NtUserThunkedMenuItemInfo( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT pos = get_ulong( &args );
    UINT flags = get_ulong( &args );
    UINT method = get_ulong( &args );
    MENUITEMINFOW32 *info32 = get_ptr( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );
    MENUITEMINFOW info = { sizeof(info) }, *info_ptr;
    UNICODE_STRING str;

    if (info32)
    {
        info.cbSize = sizeof(info);
        info.fMask = info32->fMask;
        switch (method)
        {
        case NtUserSetMenuItemInfo:
        case NtUserInsertMenuItem:
            info.fType = info32->fType;
            info.fState = info32->fState;
            info.wID = info32->wID;
            info.hSubMenu = UlongToHandle( info32->hSubMenu );
            info.hbmpChecked = UlongToHandle( info32->hbmpUnchecked );
            info.dwItemData = info32->dwItemData;
            info.dwTypeData = UlongToPtr( info32->dwTypeData );
            info.cch = info32->cch;
            info.hbmpItem = UlongToHandle( info32->hbmpItem );
            break;
        }
        info_ptr = &info;
    }
    else info_ptr = NULL;

    return NtUserThunkedMenuItemInfo( handle, pos, flags, method, info_ptr,
                                      unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserToUnicodeEx( UINT *args )
{
    UINT virt = get_ulong( &args );
    UINT scan = get_ulong( &args );
    const BYTE *state = get_ptr( &args );
    WCHAR *str = get_ptr( &args );
    int size = get_ulong( &args );
    UINT flags = get_ulong( &args );
    HKL layout = get_handle( &args );

    return NtUserToUnicodeEx( virt, scan, state, str, size, flags, layout );
}

NTSTATUS WINAPI wow64_NtUserTrackMouseEvent( UINT *args )
{
    struct
    {
        DWORD  cbSize;
        DWORD  dwFlags;
        UINT32 hwndTrack;
        DWORD  dwHoverTime;
    } *info32 = get_ptr( &args );
    TRACKMOUSEEVENT info;

    if (info32->cbSize != sizeof(*info32))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    info.cbSize      = sizeof(info);
    info.dwFlags     = info32->dwFlags;
    info.hwndTrack   = UlongToHandle( info32->hwndTrack );
    info.dwHoverTime = info32->dwHoverTime;
    return NtUserTrackMouseEvent( &info );
}

NTSTATUS WINAPI wow64_NtUserTrackPopupMenuEx( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT flags = get_ulong( &args );
    int x = get_ulong( &args );
    int y = get_ulong( &args );
    HWND hwnd = get_handle( &args );
    TPMPARAMS *params = get_ptr( &args );

    return NtUserTrackPopupMenuEx( handle, flags, x, y, hwnd, params );
}

NTSTATUS WINAPI wow64_NtUserTranslateAccelerator( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HACCEL accel = get_handle( &args );
    MSG32 *msg32 = get_ptr( &args );

    MSG msg;

    return NtUserTranslateAccelerator( hwnd, accel, msg_32to64( &msg, msg32 ));
}

NTSTATUS WINAPI wow64_NtUserTranslateMessage( UINT *args )
{
    const MSG32 *msg32 = get_ptr( &args );
    UINT flags = get_ulong( &args );
    MSG msg;

    return NtUserTranslateMessage( msg_32to64( &msg, msg32 ), flags );
}

NTSTATUS WINAPI wow64_NtUserUnhookWinEvent( UINT *args )
{
    HWINEVENTHOOK handle = get_handle( &args );

    return NtUserUnhookWinEvent( handle );
}

NTSTATUS WINAPI wow64_NtUserUnhookWindowsHookEx( UINT *args )
{
    HHOOK handle = get_handle( &args );

    return NtUserUnhookWindowsHookEx( handle );
}

NTSTATUS WINAPI wow64_NtUserUnregisterClass( UINT *args )
{
    UNICODE_STRING32 *name32 = get_ptr( &args );
    HINSTANCE instance = get_ptr( &args );
    struct client_menu_name32 *menu_name32 = get_ptr( &args );

    UNICODE_STRING name;
    struct client_menu_name menu_name;
    BOOL ret;

    ret = NtUserUnregisterClass( unicode_str_32to64( &name, name32 ), instance, &menu_name );
    if (ret) client_menu_name_64to32( &menu_name, menu_name32 );
    return ret;
}

NTSTATUS WINAPI wow64_NtUserUnregisterHotKey( UINT *args )
{
    HWND hwnd = get_handle( &args );
    int id = get_ulong( &args );

    return NtUserUnregisterHotKey( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserUpdateInputContext( UINT *args )
{
    HIMC handle = get_handle( &args );
    UINT attr = get_ulong( &args );
    UINT_PTR value = get_ulong( &args );

    return NtUserUpdateInputContext( handle, attr, value );
}

NTSTATUS WINAPI wow64_NtUserVkKeyScanEx( UINT *args )
{
    WCHAR chr = get_ulong( &args );
    HKL layout = get_handle( &args );

    return NtUserVkKeyScanEx( chr, layout );
}

NTSTATUS WINAPI wow64_NtUserWaitForInputIdle( UINT *args )
{
    HANDLE process = get_handle( &args );
    DWORD timeout = get_ulong( &args );
    BOOL wow = get_ulong( &args );

    return NtUserWaitForInputIdle( process, timeout, wow );
}

NTSTATUS WINAPI wow64_NtUserWindowFromDC( UINT *args )
{
    HDC hdc = get_handle( &args );

    return HandleToUlong( NtUserWindowFromDC( hdc ));
}

NTSTATUS WINAPI wow64_NtUserWindowFromPoint( UINT *args )
{
    LONG x = get_ulong( &args );
    LONG y = get_ulong( &args );

    return HandleToUlong( NtUserWindowFromPoint( x, y ));
}
