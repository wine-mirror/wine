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

NTSTATUS WINAPI wow64_NtUserInitializeClientPfnArrays( UINT *args )
{
    FIXME( "\n" );
    return STATUS_NOT_SUPPORTED;
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

NTSTATUS WINAPI wow64_NtUserOpenWindowStation( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );

    struct object_attr64 attr;

    return HandleToUlong( NtUserOpenWindowStation( objattr_32to64( &attr, attr32 ), access ));
}

NTSTATUS WINAPI wow64_NtUserCloseWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserCloseWindowStation( handle );
}

NTSTATUS WINAPI wow64_NtUserGetProcessWindowStation( UINT *args )
{
    return HandleToUlong( NtUserGetProcessWindowStation() );
}

NTSTATUS WINAPI wow64_NtUserSetProcessWindowStation( UINT *args )
{
    HWINSTA handle = get_handle( &args );

    return NtUserSetProcessWindowStation( handle );
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

NTSTATUS WINAPI wow64_NtUserCloseDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserCloseDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserGetThreadDesktop( UINT *args )
{
    DWORD thread = get_ulong( &args );

    return HandleToUlong( NtUserGetThreadDesktop( thread ));
}

NTSTATUS WINAPI wow64_NtUserSetThreadDesktop( UINT *args )
{
    HDESK handle = get_handle( &args );

    return NtUserSetThreadDesktop( handle );
}

NTSTATUS WINAPI wow64_NtUserOpenInputDesktop( UINT *args )
{
    DWORD flags = get_ulong( &args );
    BOOL inherit = get_ulong( &args );
    ACCESS_MASK access = get_ulong( &args );

    return HandleToUlong( NtUserOpenInputDesktop( flags, inherit, access ));
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

NTSTATUS WINAPI wow64_NtUserSetObjectInformation( UINT *args )
{
    HANDLE handle = get_handle( &args );
    INT index = get_ulong( &args );
    void *info = get_ptr( &args );
    DWORD len = get_ulong( &args );

    return NtUserSetObjectInformation( handle, index, info, len );
}

NTSTATUS WINAPI wow64_NtUserGetProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );

    return HandleToUlong( NtUserGetProp( hwnd, str ));
}

NTSTATUS WINAPI wow64_NtUserSetProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );
    HANDLE handle = get_handle( &args );

    return NtUserSetProp( hwnd, str, handle );
}

NTSTATUS WINAPI wow64_NtUserRemoveProp( UINT *args )
{
    HWND hwnd = get_handle( &args );
    const WCHAR *str = get_ptr( &args );

    return HandleToUlong( NtUserRemoveProp( hwnd, str ));
}

NTSTATUS WINAPI wow64_NtUserGetAtomName( UINT *args )
{
    ATOM atom = get_ulong( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUserGetAtomName( atom, unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserGetClassName( UINT *args )
{
    HWND hwnd = get_handle( &args );
    BOOL real = get_ulong( &args );
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUserGetClassName( hwnd, real, unicode_str_32to64( &str, str32 ));
}

NTSTATUS WINAPI wow64_NtUserGetAncestor( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT type = get_ulong( &args );

    return HandleToUlong( NtUserGetAncestor( hwnd, type ));
}

NTSTATUS WINAPI wow64_NtUserGetWindowRgnEx( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HRGN hrgn = get_handle( &args );
    UINT unk = get_ulong( &args );

    return NtUserGetWindowRgnEx( hwnd, hrgn, unk );
}

NTSTATUS WINAPI wow64_NtUserWindowFromDC( UINT *args )
{
    HDC hdc = get_handle( &args );

    return HandleToUlong( NtUserWindowFromDC( hdc ));
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

NTSTATUS WINAPI wow64_NtUserInternalGetWindowText( UINT *args )
{
    HWND hwnd = get_handle( &args );
    WCHAR *text = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserInternalGetWindowText( hwnd, text, count );
}

NTSTATUS WINAPI wow64_NtUserGetLayeredWindowAttributes( UINT *args )
{
    HWND hwnd = get_handle( &args );
    COLORREF *key = get_ptr( &args );
    BYTE *alpha = get_ptr( &args );
    DWORD *flags = get_ptr( &args );

    return NtUserGetLayeredWindowAttributes( hwnd, key, alpha, flags );
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

NTSTATUS WINAPI wow64_NtUserGetOpenClipboardWindow( UINT *args )
{
    return HandleToUlong( NtUserGetOpenClipboardWindow() );
}

NTSTATUS WINAPI wow64_NtUserGetClipboardSequenceNumber( UINT *args )
{
    return NtUserGetClipboardSequenceNumber();
}

NTSTATUS WINAPI wow64_NtUserGetClipboardViewer( UINT *args )
{
    return HandleToUlong( NtUserGetClipboardViewer() );
}

NTSTATUS WINAPI wow64_NtUserAddClipboardFormatListener( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserAddClipboardFormatListener( hwnd );
}

NTSTATUS WINAPI wow64_NtUserRemoveClipboardFormatListener( UINT *args )
{
    HWND hwnd = get_handle( &args );

    return NtUserRemoveClipboardFormatListener( hwnd );
}

NTSTATUS WINAPI wow64_NtUserGetCursor( UINT *args )
{
    return HandleToUlong( NtUserGetCursor() );
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

NTSTATUS WINAPI wow64_NtUserGetIconSize( UINT *args )
{
    HICON handle = get_handle( &args );
    UINT step = get_ulong( &args );
    LONG *width = get_ptr( &args );
    LONG *height = get_ptr( &args );

    return NtUserGetIconSize( handle, step, width, height );
}

NTSTATUS WINAPI wow64_NtUserGetCursorFrameInfo( UINT *args )
{
    HCURSOR cursor = get_ptr( &args );
    DWORD istep = get_ulong( &args );
    DWORD *rate_jiffies = get_ptr( &args );
    DWORD *num_steps = get_ptr( &args );

    return HandleToUlong( NtUserGetCursorFrameInfo( cursor, istep, rate_jiffies, num_steps ));
}

NTSTATUS WINAPI wow64_NtUserAttachThreadInput( UINT *args )
{
    DWORD from = get_ulong( &args );
    DWORD to = get_ulong( &args );
    BOOL attach = get_ulong( &args );

    return NtUserAttachThreadInput( from, to, attach );
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

NTSTATUS WINAPI wow64_NtUserSetKeyboardState( UINT *args )
{
    BYTE *state = get_ptr( &args );

    return NtUserSetKeyboardState( state );
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

NTSTATUS WINAPI wow64_NtUserChildWindowFromPointEx( UINT *args )
{
    HWND parent = get_handle( &args );
    LONG x = get_ulong( &args );
    LONG y = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return HandleToUlong( NtUserChildWindowFromPointEx( parent, x, y, flags ));
}

NTSTATUS WINAPI wow64_NtUserSetProcessDpiAwarenessContext( UINT *args )
{
    ULONG awareness = get_ulong( &args );
    ULONG unknown = get_ulong( &args );

    return NtUserSetProcessDpiAwarenessContext( awareness, unknown );
}

NTSTATUS WINAPI wow64_NtUserGetProcessDpiAwarenessContext( UINT *args )
{
    HANDLE process = get_handle( &args );

    return NtUserGetProcessDpiAwarenessContext( process );
}

NTSTATUS WINAPI wow64_NtUserGetSystemDpiForProcess( UINT *args )
{
    HANDLE process = get_handle( &args );

    return NtUserGetSystemDpiForProcess( process );
}

NTSTATUS WINAPI wow64_NtUserGetDpiForMonitor( UINT *args )
{
    HMONITOR monitor = get_handle( &args );
    UINT type = get_ulong( &args );
    UINT *x = get_ptr( &args );
    UINT *y = get_ptr( &args );

    return NtUserGetDpiForMonitor( monitor, type, x, y );
}

NTSTATUS WINAPI wow64_NtUserGetDoubleClickTime( UINT *args )
{
    return NtUserGetDoubleClickTime();
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

NTSTATUS WINAPI wow64_NtUserUnhookWinEvent( UINT *args )
{
    HWINEVENTHOOK handle = get_handle( &args );

    return NtUserUnhookWinEvent( handle );
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

NTSTATUS WINAPI wow64_NtUserUnhookWindowsHookEx( UINT *args )
{
    HHOOK handle = get_handle( &args );

    return NtUserUnhookWindowsHookEx( handle );
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

NTSTATUS WINAPI wow64_NtUserSetSystemTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );
    UINT timeout = get_ulong( &args );

    return NtUserSetSystemTimer( hwnd, id, timeout );
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

NTSTATUS WINAPI wow64_NtUserKillTimer( UINT *args )
{
    HWND hwnd = get_handle( &args );
    UINT_PTR id = get_ulong( &args );

    return NtUserKillTimer( hwnd, id );
}

NTSTATUS WINAPI wow64_NtUserCopyAcceleratorTable( UINT *args )
{
    HACCEL src = get_handle( &args );
    ACCEL *dst = get_ptr( &args );
    INT count = get_ulong( &args );

    return NtUserCopyAcceleratorTable( src, dst, count );
}

NTSTATUS WINAPI wow64_NtUserCreateAcceleratorTable( UINT *args )
{
    ACCEL *table = get_ptr( &args );
    INT count = get_ulong( &args );

    return HandleToUlong( NtUserCreateAcceleratorTable( table, count ));
}

NTSTATUS WINAPI wow64_NtUserDestroyAcceleratorTable( UINT *args )
{
    HACCEL handle = get_handle( &args );

    return NtUserDestroyAcceleratorTable( handle );
}

NTSTATUS WINAPI wow64_NtUserCheckMenuItem( UINT *args )
{
    HMENU handle = get_handle( &args );
    UINT id = get_ulong( &args );
    UINT flags = get_ulong( &args );

    return NtUserCheckMenuItem( handle, id, flags );
}

NTSTATUS WINAPI wow64_NtUserGetMenuItemRect( UINT *args )
{
    HWND hwnd = get_handle( &args );
    HMENU handle = get_handle( &args );
    UINT item = get_ulong( &args );
    RECT *rect = get_ptr( &args );

    return NtUserGetMenuItemRect( hwnd, handle, item, rect );
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
