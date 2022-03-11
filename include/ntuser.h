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

#ifndef _NTUSER_
#define _NTUSER_

#include <winuser.h>
#include <wingdi.h>
#include <winternl.h>

/* KernelCallbackTable codes, not compatible with Windows */
enum
{
    /* user32 callbacks */
    NtUserCallEnumDisplayMonitor,
    NtUserCallWinEventHook,
    NtUserCallWindowsHook,
    NtUserLoadDriver,
    /* win16 hooks */
    NtUserCallFreeIcon,
    /* Vulkan support */
    NtUserCallVulkanDebugReportCallback,
    NtUserCallVulkanDebugUtilsCallback,
    NtUserCallCount
};

/* NtUserCallEnumDisplayMonitor params */
struct enum_display_monitor_params
{
    MONITORENUMPROC proc;
    HMONITOR monitor;
    HDC hdc;
    RECT rect;
    LPARAM lparam;
};

/* NtUserCallWinEventHook params */
struct win_event_hook_params
{
    DWORD event;
    HWND hwnd;
    LONG object_id;
    LONG child_id;
    void *handle;
    WINEVENTPROC proc;
    WCHAR module[MAX_PATH];
};

/* NtUserCallWindowsHook params */
struct win_hook_params
{
    void *proc;
    void *handle;
    DWORD pid;
    DWORD tid;
    int id;
    int code;
    WPARAM wparam;
    LPARAM lparam;
    BOOL prev_unicode;
    BOOL next_unicode;
    WCHAR module[MAX_PATH];
};

/* process DPI awareness contexts */
#define NTUSER_DPI_UNAWARE                0x00006010
#define NTUSER_DPI_SYSTEM_AWARE           0x00006011
#define NTUSER_DPI_PER_MONITOR_AWARE      0x00000012
#define NTUSER_DPI_PER_MONITOR_AWARE_V2   0x00000022
#define NTUSER_DPI_PER_UNAWARE_GDISCALED  0x40006010

/* NtUserCallNoParam codes, not compatible with Windows */
enum
{
    NtUserGetDesktopWindow,
    NtUserGetInputState,
    NtUserReleaseCapture,
    /* temporary exports */
    NtUserThreadDetach,
};

/* NtUserCallOneParam codes, not compatible with Windows */
enum
{
    NtUserCreateCursorIcon,
    NtUserGetClipCursor,
    NtUserGetCursorPos,
    NtUserGetIconParam,
    NtUserGetPrimaryMonitorRect,
    NtUserGetSysColor,
    NtUserGetSysColorBrush,
    NtUserGetSysColorPen,
    NtUserGetSystemMetrics,
    NtUserGetVirtualScreenRect,
    NtUserMessageBeep,
    NtUserRealizePalette,
    /* temporary exports */
    NtUserCallHooks,
    NtUserFlushWindowSurfaces,
    NtUserGetDeskPattern,
    NtUserGetWinProcPtr,
    NtUserHandleInternalMessage,
    NtUserIncrementKeyStateCounter,
    NtUserLock,
    NtUserNextThreadWindow,
    NtUserSetCallbacks,
};

/* NtUserCallTwoParam codes, not compatible with Windows */
enum
{
    NtUserGetMonitorInfo,
    NtUserGetSystemMetricsForDpi,
    NtUserMirrorRgn,
    NtUserMonitorFromRect,
    NtUserSetIconParam,
    NtUserUnhookWindowsHook,
    /* temporary exports */
    NtUserAllocHandle,
    NtUserAllocWinProc,
    NtUserFreeHandle,
    NtUserGetHandlePtr,
    NtUserRegisterWindowSurface,
    NtUserSetHandlePtr,
};

/* NtUserCallHwnd codes, not compatible with Windows */
enum
{
    NtUserGetDpiForWindow,
    NtUserGetParent,
    NtUserGetWindowDpiAwarenessContext,
    NtUserGetWindowTextLength,
    NtUserIsWindow,
    NtUserIsWindowUnicode,
    NtUserIsWindowVisible,
    /* temporary exports */
    NtUserCreateDesktopWindow,
};

/* NtUserCallHwndParam codes, not compatible with Windows */
enum
{
    NtUserGetClassLongA,
    NtUserGetClassLongW,
    NtUserGetClassLongPtrA,
    NtUserGetClassLongPtrW,
    NtUserGetClassWord,
    NtUserGetClientRect,
    NtUserGetWindowInfo,
    NtUserGetWindowLongA,
    NtUserGetWindowLongW,
    NtUserGetWindowLongPtrA,
    NtUserGetWindowLongPtrW,
    NtUserGetWindowPlacement,
    NtUserGetWindowRect,
    NtUserGetWindowRelative,
    NtUserGetWindowThread,
    NtUserGetWindowWord,
    NtUserIsChild,
    NtUserMonitorFromWindow,
    NtUserSetCaptureWindow,
    NtUserSetForegroundWindow,
    /* temporary exports */
    NtUserIsWindowDrawable,
};

/* NtUserMessageCall codes */
enum
{
    FNID_SENDNOTIFYMESSAGE  = 0x02b7,
};

/* color index used to retrieve system 55aa brush */
#define COLOR_55AA_BRUSH  0x100

/* user object types */
#define NTUSER_OBJ_WINDOW   0x01
#define NTUSER_OBJ_MENU     0x02
#define NTUSER_OBJ_ICON     0x03
#define NTUSER_OBJ_WINPOS   0x04
#define NTUSER_OBJ_ACCEL    0x08
#define NTUSER_OBJ_HOOK     0x0f

/* NtUserInitializeClientPfnArrays parameter, not compatible with Windows */
struct user_client_procs
{
    WNDPROC pButtonWndProc;
    WNDPROC pComboWndProc;
    WNDPROC pDefWindowProc;
    WNDPROC pDefDlgProc;
    WNDPROC pEditWndProc;
    WNDPROC pListBoxWndProc;
    WNDPROC pMDIClientWndProc;
    WNDPROC pScrollBarWndProc;
    WNDPROC pStaticWndProc;
    WNDPROC pImeWndProc;
    WNDPROC pDesktopWndProc;
    WNDPROC pIconTitleWndProc;
    WNDPROC pPopupMenuWndProc;
    WNDPROC pMessageWndProc;
};

/* NtUserSetCursorIconData parameter, not compatible with Windows */
struct cursoricon_frame
{
    UINT     width;    /* frame-specific width */
    UINT     height;   /* frame-specific height */
    HBITMAP  color;    /* color bitmap */
    HBITMAP  alpha;    /* pre-multiplied alpha bitmap for 32-bpp icons */
    HBITMAP  mask;     /* mask bitmap (followed by color for 1-bpp icons) */
    POINT    hotspot;
};

struct cursoricon_desc
{
    UINT flags;
    UINT num_steps;
    UINT num_frames;
    UINT delay;
    struct cursoricon_frame *frames;
    DWORD *frame_seq;
    DWORD *frame_rates;
    HRSRC rsrc;
};

struct client_menu_name
{
    char  *nameA;
    WCHAR *nameW;
    UNICODE_STRING *nameUS;
};

/* internal messages codes */
enum wine_internal_message
{
    WM_WINE_DESTROYWINDOW = 0x80000000,
    WM_WINE_SETWINDOWPOS,
    WM_WINE_SHOWWINDOW,
    WM_WINE_SETPARENT,
    WM_WINE_SETWINDOWLONG,
    WM_WINE_SETSTYLE,
    WM_WINE_SETACTIVEWINDOW,
    WM_WINE_KEYBOARD_LL_HOOK,
    WM_WINE_MOUSE_LL_HOOK,
    WM_WINE_CLIPCURSOR,
    WM_WINE_UPDATEWINDOWSTATE,
    WM_WINE_FIRST_DRIVER_MSG = 0x80001000,  /* range of messages reserved for the USER driver */
    WM_WINE_LAST_DRIVER_MSG = 0x80001fff
};


HKL     WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags );
BOOL    WINAPI NtUserAddClipboardFormatListener( HWND hwnd );
BOOL    WINAPI NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach );
NTSTATUS WINAPI NtUserBuildHwndList( HDESK desktop, ULONG unk2, ULONG unk3, ULONG unk4,
                                     ULONG thread_id, ULONG count, HWND *buffer, ULONG *size );
ULONG_PTR WINAPI NtUserCallHwnd( HWND hwnd, DWORD code );
ULONG_PTR WINAPI NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code );
LRESULT WINAPI NtUserCallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam );
ULONG_PTR WINAPI NtUserCallNoParam( ULONG code );
ULONG_PTR WINAPI NtUserCallOneParam( ULONG_PTR arg, ULONG code );
ULONG_PTR WINAPI NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code );
LONG    WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                            DWORD flags, void *lparam );
BOOL    WINAPI NtUserClipCursor( const RECT *rect );
BOOL    WINAPI NtUserCloseDesktop( HDESK handle );
BOOL    WINAPI NtUserCloseWindowStation( HWINSTA handle );
INT     WINAPI NtUserCopyAcceleratorTable( HACCEL src, ACCEL *dst, INT count );
INT     WINAPI NtUserCountClipboardFormats(void);
HACCEL  WINAPI NtUserCreateAcceleratorTable( ACCEL *table, INT count );
HDESK   WINAPI NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                      DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                      ULONG heap_size );
HWINSTA WINAPI NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK mask, ULONG arg3,
                                          ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 );
BOOL    WINAPI NtUserDestroyAcceleratorTable( HACCEL handle );
BOOL    WINAPI NtUserDestroyCursor( HCURSOR cursor, ULONG arg );
BOOL    WINAPI NtUserDrawIconEx( HDC hdc, INT x0, INT y0, HICON icon, INT width,
                                 INT height, UINT istep, HBRUSH hbr, UINT flags );
NTSTATUS WINAPI NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                          DISPLAY_DEVICEW *info, DWORD flags );
BOOL    WINAPI NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lp );
BOOL    WINAPI NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD mode,
                                          DEVMODEW *dev_mode, DWORD flags );
HICON   WINAPI NtUserFindExistingCursorIcon( UNICODE_STRING *module, UNICODE_STRING *res_name,
                                             void *desc );
BOOL    WINAPI NtUserFlashWindowEx( FLASHWINFO *info );
HWND    WINAPI NtUserGetAncestor( HWND hwnd, UINT type );
SHORT   WINAPI NtUserGetAsyncKeyState( INT key );
ULONG   WINAPI NtUserGetAtomName( ATOM atom, UNICODE_STRING *name );
ATOM    WINAPI NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                     struct client_menu_name *menu_name, BOOL ansi );
INT     WINAPI NtUserGetClassName( HWND hwnd, BOOL real, UNICODE_STRING *name );
INT     WINAPI NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen );
HWND    WINAPI NtUserGetClipboardOwner(void);
DWORD   WINAPI NtUserGetClipboardSequenceNumber(void);
HWND    WINAPI NtUserGetClipboardViewer(void);
HCURSOR WINAPI NtUserGetCursor(void);
HCURSOR WINAPI NtUserGetCursorFrameInfo( HCURSOR hCursor, DWORD istep, DWORD *rate_jiffies,
                                         DWORD *num_steps );
BOOL    WINAPI NtUserGetCursorInfo( CURSORINFO *info );
LONG    WINAPI NtUserGetDisplayConfigBufferSizes( UINT32 flags, UINT32 *num_path_info,
                                                  UINT32 *num_mode_info );
UINT    WINAPI NtUserGetDoubleClickTime(void);
BOOL    WINAPI NtUserGetDpiForMonitor( HMONITOR monitor, UINT type, UINT *x, UINT *y );
HWND    WINAPI NtUserGetForegroundWindow(void);
BOOL    WINAPI NtUserGetGUIThreadInfo( DWORD id, GUITHREADINFO *info );
BOOL    WINAPI NtUserGetIconInfo( HICON icon, ICONINFO *info, UNICODE_STRING *module,
                                  UNICODE_STRING *res_name, DWORD *bpp, LONG unk );
BOOL    WINAPI NtUserGetIconSize( HICON handle, UINT step, LONG *width, LONG *height );
INT     WINAPI NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size );
SHORT   WINAPI NtUserGetKeyState( INT vkey );
HKL     WINAPI NtUserGetKeyboardLayout( DWORD thread_id );
UINT    WINAPI NtUserGetKeyboardLayoutList( INT size, HKL *layouts );
BOOL    WINAPI NtUserGetKeyboardLayoutName( WCHAR *name );
BOOL    WINAPI NtUserGetKeyboardState( BYTE *state );
BOOL    WINAPI NtUserGetLayeredWindowAttributes( HWND hwnd, COLORREF *key, BYTE *alpha, DWORD *flags );
int     WINAPI NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                           int count, DWORD resolution );
BOOL    WINAPI NtUserGetObjectInformation( HANDLE handle, INT index, void *info,
                                           DWORD len, DWORD *needed );
HWND    WINAPI NtUserGetOpenClipboardWindow(void);
INT     WINAPI NtUserGetPriorityClipboardFormat( UINT *list, INT count );
HWINSTA WINAPI NtUserGetProcessWindowStation(void);
HANDLE  WINAPI NtUserGetProp( HWND hwnd, const WCHAR *str );
ULONG   WINAPI NtUserGetProcessDpiAwarenessContext( HANDLE process );
DWORD   WINAPI NtUserGetQueueStatus( UINT flags );
ULONG   WINAPI NtUserGetSystemDpiForProcess( HANDLE process );
HDESK   WINAPI NtUserGetThreadDesktop( DWORD thread );
BOOL    WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size );
int     WINAPI NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk );
NTSTATUS WINAPI NtUserInitializeClientPfnArrays( const struct user_client_procs *client_procsA,
                                                 const struct user_client_procs *client_procsW,
                                                 const void *client_workers, HINSTANCE user_module );
INT     WINAPI NtUserInternalGetWindowText( HWND hwnd, WCHAR *text, INT count );
BOOL    WINAPI NtUserIsClipboardFormatAvailable( UINT format );
UINT    WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout );
BOOL    WINAPI NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  ULONG_PTR result_info, DWORD type, BOOL ansi );
BOOL    WINAPI NtUserMoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint );
void    WINAPI NtUserNotifyWinEvent( DWORD event, HWND hwnd, LONG object_id, LONG child_id );
HWINSTA WINAPI NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access );
BOOL    WINAPI NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len );
HDESK   WINAPI NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access );
HDESK   WINAPI NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access );
ATOM    WINAPI NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                         struct client_menu_name *client_menu_name, DWORD fnid, DWORD flags,
                                         DWORD *wow );
BOOL    WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk );
BOOL    WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd );
HANDLE  WINAPI NtUserRemoveProp( HWND hwnd, const WCHAR *str );
BOOL    WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                               HRGN ret_update_rgn, RECT *update_rect );
HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE palette, WORD force_background );
HWND     WINAPI NtUserSetActiveWindow( HWND hwnd );
HWND     WINAPI NtUserSetCapture( HWND hwnd );
DWORD    WINAPI NtUserSetClassLong( HWND hwnd, INT offset, LONG newval, BOOL ansi );
ULONG_PTR WINAPI NtUserSetClassLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi );
WORD    WINAPI NtUserSetClassWord( HWND hwnd, INT offset, WORD newval );
HCURSOR WINAPI NtUserSetCursor( HCURSOR cursor );
BOOL    WINAPI NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                        struct cursoricon_desc *desc );
BOOL    WINAPI NtUserSetCursorPos( INT x, INT y );
HWND    WINAPI NtUserSetFocus( HWND hwnd );
BOOL    WINAPI NtUserSetKeyboardState( BYTE *state );
BOOL    WINAPI NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags );
BOOL    WINAPI NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown );
BOOL    WINAPI NtUserSetProcessWindowStation( HWINSTA handle );
BOOL    WINAPI NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle );
BOOL    WINAPI NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values );
BOOL    WINAPI NtUserSetThreadDesktop( HDESK handle );
BOOL    WINAPI NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags );
int     WINAPI NtUserSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw );
HHOOK   WINAPI NtUserSetWindowsHookEx( HINSTANCE inst, UNICODE_STRING *module, DWORD tid, INT id,
                                       HOOKPROC proc, BOOL ansi );
HWINEVENTHOOK WINAPI NtUserSetWinEventHook( DWORD event_min, DWORD event_max, HMODULE inst,
                                            UNICODE_STRING *module, WINEVENTPROC proc,
                                            DWORD pid, DWORD tid, DWORD flags );
INT     WINAPI NtUserShowCursor( BOOL show );
BOOL    WINAPI NtUserSystemParametersInfo( UINT action, UINT val, void *ptr, UINT winini );
BOOL    WINAPI NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi );
INT     WINAPI NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                                  WCHAR *str, int size, UINT flags, HKL layout );
BOOL    WINAPI NtUserUnhookWinEvent( HWINEVENTHOOK hEventHook );
BOOL    WINAPI NtUserUnhookWindowsHookEx( HHOOK handle );
BOOL    WINAPI NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                      struct client_menu_name *client_menu_name );
BOOL    WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id );
BOOL    WINAPI NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                          HDC hdc_src, const POINT *pts_src, COLORREF key,
                                          const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty );
WORD    WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout );
DWORD   WINAPI NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow );
HWND    WINAPI NtUserWindowFromPoint( LONG x, LONG y );

#endif /* _NTUSER_ */
