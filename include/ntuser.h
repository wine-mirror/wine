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
    NtUserGetInputState,
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
    NtUserFreeHandle,
    NtUserGetHandlePtr,
    NtUserRegisterWindowSurface,
    NtUserSetHandlePtr,
};

/* NtUserCallHwnd codes, not compatible with Windows */
enum
{
    NtUserIsWindow,
};

/* NtUserCallHwndParam codes, not compatible with Windows */
enum
{
    NtUserGetWindowThread,
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
DWORD   WINAPI NtUserCallHwnd( HWND hwnd, DWORD code );
DWORD   WINAPI NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code );
LRESULT WINAPI NtUserCallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam );
ULONG_PTR WINAPI NtUserCallNoParam( ULONG code );
ULONG_PTR WINAPI NtUserCallOneParam( ULONG_PTR arg, ULONG code );
ULONG_PTR WINAPI NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code );
LONG    WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                            DWORD flags, void *lparam );
BOOL    WINAPI NtUserClipCursor( const RECT *rect );
BOOL    WINAPI NtUserCloseDesktop( HDESK handle );
BOOL    WINAPI NtUserCloseWindowStation( HWINSTA handle );
INT     WINAPI NtUserCountClipboardFormats(void);
HDESK   WINAPI NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                      DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                      ULONG heap_size );
HWINSTA WINAPI NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK mask, ULONG arg3,
                                          ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 );
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
SHORT   WINAPI NtUserGetAsyncKeyState( INT key );
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
BOOL    WINAPI NtUserIsClipboardFormatAvailable( UINT format );
UINT    WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout );
void    WINAPI NtUserNotifyWinEvent( DWORD event, HWND hwnd, LONG object_id, LONG child_id );
HWINSTA WINAPI NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access );
BOOL    WINAPI NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len );
HDESK   WINAPI NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access );
HDESK   WINAPI NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access );
BOOL    WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk );
BOOL    WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd );
HANDLE  WINAPI NtUserRemoveProp( HWND hwnd, const WCHAR *str );
BOOL    WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                               HRGN ret_update_rgn, RECT *update_rect );
HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE palette, WORD force_background );
HCURSOR WINAPI NtUserSetCursor( HCURSOR cursor );
BOOL    WINAPI NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                        struct cursoricon_desc *desc );
BOOL    WINAPI NtUserSetCursorPos( INT x, INT y );
BOOL    WINAPI NtUserSetKeyboardState( BYTE *state );
BOOL    WINAPI NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown );
BOOL    WINAPI NtUserSetProcessWindowStation( HWINSTA handle );
BOOL    WINAPI NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle );
BOOL    WINAPI NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values );
BOOL    WINAPI NtUserSetThreadDesktop( HDESK handle );
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
BOOL    WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id );
WORD    WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout );

#endif /* _NTUSER_ */
