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
#include <imm.h>
#include <immdev.h>
#include <shellapi.h>
#include <winternl.h>

#ifndef W32KAPI
# if defined(_WIN32U_) || defined(WINE_UNIX_LIB)
#  define W32KAPI DECLSPEC_EXPORT
# else
#  define W32KAPI DECLSPEC_IMPORT
# endif
#endif

/* KernelCallbackTable codes, not compatible with Windows */
enum
{
    /* user32 callbacks */
    NtUserCallEnumDisplayMonitor,
    NtUserCallSendAsyncCallback,
    NtUserCallWinEventHook,
    NtUserCallWinProc,
    NtUserCallWindowsHook,
    NtUserCopyImage,
    NtUserDrawNonClientButton,
    NtUserDrawScrollBar,
    NtUserDrawText,
    NtUserFreeCachedClipboardData,
    NtUserImmProcessKey,
    NtUserImmTranslateMessage,
    NtUserInitBuiltinClasses,
    NtUserLoadDriver,
    NtUserLoadImage,
    NtUserLoadSysMenu,
    NtUserPostDDEMessage,
    NtUserRenderSynthesizedFormat,
    NtUserUnpackDDEMessage,
    /* win16 hooks */
    NtUserCallFreeIcon,
    NtUserThunkLock,
    /* Vulkan support */
    NtUserCallVulkanDebugReportCallback,
    NtUserCallVulkanDebugUtilsCallback,
    /* OpenGL support */
    NtUserCallOpenGLDebugMessageCallback,
    /* Driver-specific callbacks */
    NtUserDriverCallbackFirst,
    NtUserDriverCallbackLast = NtUserDriverCallbackFirst + 9,
    NtUserCallCount
};

/* TEB thread info, not compatible with Windows */
struct ntuser_thread_info
{
    UINT64         driver_data;       /* driver-specific data */
    DWORD          message_time;      /* value for GetMessageTime */
    DWORD          message_pos;       /* value for GetMessagePos */
    UINT64         message_extra;     /* value for GetMessageExtraInfo */
    INPUT_MESSAGE_SOURCE msg_source;  /* Message source for current message */
    WORD           recursion_count;   /* SendMessage recursion counter */
    UINT           receive_flags;     /* currently received message flags */
    UINT           top_window;        /* desktop window */
    UINT           msg_window;        /* HWND_MESSAGE parent window */
    DPI_AWARENESS  dpi_awareness;     /* DPI awareness */
    UINT           default_imc;       /* default input context */
    UINT64         client_imm;        /* client IMM thread info */
    UINT64         wmchar_data;       /* client data for WM_CHAR mappings */
};

static inline struct ntuser_thread_info *NtUserGetThreadInfo(void)
{
#ifndef _WIN64
    if (NtCurrentTeb()->GdiBatchCount)
    {
        TEB64 *teb64 = (TEB64 *)(UINT_PTR)NtCurrentTeb()->GdiBatchCount;
        return (struct ntuser_thread_info *)teb64->Win32ClientInfo;
    }
#endif
    return (struct ntuser_thread_info *)NtCurrentTeb()->Win32ClientInfo;
}

/* NtUserCallEnumDisplayMonitor params */
struct enum_display_monitor_params
{
    MONITORENUMPROC proc;
    HMONITOR monitor;
    HDC hdc;
    RECT rect;
    LPARAM lparam;
};

/* NtUserCallSendAsyncCallback params */
struct send_async_params
{
    SENDASYNCPROC callback;
    HWND hwnd;
    UINT msg;
    ULONG_PTR data;
    LRESULT result;
};

/* NtUserCallWinEventHook params */
struct win_event_hook_params
{
    DWORD event;
    HWND hwnd;
    LONG object_id;
    LONG child_id;
    void *handle;
    DWORD tid;
    DWORD time;
    WINEVENTPROC proc;
    WCHAR module[MAX_PATH];
};

/* type of message-sending functions that need special WM_CHAR handling */
enum wm_char_mapping
{
    WMCHAR_MAP_POSTMESSAGE,
    WMCHAR_MAP_SENDMESSAGE,
    WMCHAR_MAP_SENDMESSAGETIMEOUT,
    WMCHAR_MAP_RECVMESSAGE,
    WMCHAR_MAP_DISPATCHMESSAGE,
    WMCHAR_MAP_CALLWINDOWPROC,
    WMCHAR_MAP_ISDIALOGMESSAGE,
    WMCHAR_MAP_COUNT,
    WMCHAR_MAP_NOMAPPING = WMCHAR_MAP_COUNT
};

/* NtUserCallWindowProc params */
struct win_proc_params
{
    WNDPROC func;
    HWND hwnd;
    UINT msg;
    WPARAM wparam;
    LPARAM lparam;
    BOOL ansi;
    BOOL ansi_dst;
    enum wm_char_mapping mapping;
    DPI_AWARENESS_CONTEXT dpi_awareness;
    WNDPROC procA;
    WNDPROC procW;
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
    WCHAR module[1];
};

/* NtUserCopyImage params */
struct copy_image_params
{
    HANDLE hwnd;
    UINT type;
    INT dx;
    INT dy;
    UINT flags;
};

/* NtUserDrawText params */
struct draw_text_params
{
    HDC hdc;
    int count;
    RECT rect;
    UINT flags;
    WCHAR str[1];
};

/* NtUserFreeCachedClipboardData params */
struct free_cached_data_params
{
    UINT format;
    HANDLE handle;
};

/* NtUserImmProcessKey params */
struct imm_process_key_params
{
    HWND hwnd;
    HKL hkl;
    UINT vkey;
    LPARAM key_data;
};

/* NtUserImmTranslateMessage params */
struct imm_translate_message_params
{
    HWND hwnd;
    UINT msg;
    WPARAM wparam;
    LPARAM key_data;
};

/* NtUserLoadImage params */
struct load_image_params
{
    HINSTANCE hinst;
    const WCHAR *name;
    UINT type;
    INT dx;
    INT dy;
    UINT flags;
};

/* NtUserLoadSysMenu params */
struct load_sys_menu_params
{
    BOOL mdi;
};

/* NtUserPostDDEMessage params */
struct post_dde_message_params
{
    HWND hwnd;
    UINT msg;
    WPARAM wparam;
    LPARAM lparam;
    DWORD dest_tid;
    DWORD type;
};

/* NtUserRenderSynthesizedFormat params */
struct render_synthesized_format_params
{
    UINT format;
    UINT from;
};

/* NtUserUnpackDDEMessage params */
struct unpack_dde_message_result
{
    WPARAM wparam;
    LPARAM lparam;
};

struct unpack_dde_message_params
{
    struct unpack_dde_message_result *result;  /* FIXME: Use NtCallbackReturn instead */
    HWND hwnd;
    UINT message;
    WPARAM wparam;
    LPARAM lparam;
    char data[1];
};

/* process DPI awareness contexts */
#define NTUSER_DPI_UNAWARE                0x00006010
#define NTUSER_DPI_SYSTEM_AWARE           0x00006011
#define NTUSER_DPI_PER_MONITOR_AWARE      0x00000012
#define NTUSER_DPI_PER_MONITOR_AWARE_V2   0x00000022
#define NTUSER_DPI_PER_UNAWARE_GDISCALED  0x40006010

/* message spy definitions */
#define SPY_DISPATCHMESSAGE  0x0100
#define SPY_SENDMESSAGE      0x0101
#define SPY_DEFWNDPROC       0x0102

#define SPY_RESULT_OK      0x0001
#define SPY_RESULT_DEFWND  0x0002

/* CreateDesktop wine specific flag */
#define DF_WINE_VIRTUAL_DESKTOP   0x80000000

/* NtUserMessageCall codes */
enum
{
    NtUserScrollBarWndProc    = 0x029a,
    NtUserPopupMenuWndProc    = 0x029c,
    NtUserDesktopWindowProc   = 0x029d,
    NtUserDefWindowProc       = 0x029e,
    NtUserCallWindowProc      = 0x02ab,
    NtUserSendMessage         = 0x02b1,
    NtUserSendMessageTimeout  = 0x02b3,
    NtUserSendNotifyMessage   = 0x02b7,
    NtUserSendMessageCallback = 0x02b8,
    /* Wine-specific exports */
    NtUserClipboardWindowProc = 0x0300,
    NtUserGetDispatchParams   = 0x3001,
    NtUserSpyGetMsgName       = 0x3002,
    NtUserSpyEnter            = 0x0303,
    NtUserSpyExit             = 0x0304,
    NtUserImeDriverCall       = 0x0305,
    NtUserSystemTrayCall      = 0x0306,
};

/* NtUserThunkedMenuItemInfo codes */
enum
{
    NtUserSetMenuItemInfo,
    NtUserInsertMenuItem,
    /* Wine extensions */
    NtUserCheckMenuRadioItem,
    NtUserGetMenuDefaultItem,
    NtUserGetMenuItemID,
    NtUserGetMenuItemInfoA,
    NtUserGetMenuItemInfoW,
    NtUserGetMenuState,
    NtUserGetSubMenu,
};

struct send_message_timeout_params
{
    UINT flags;
    UINT timeout;
    DWORD_PTR result;
};

struct send_message_callback_params
{
    SENDASYNCPROC callback;
    ULONG_PTR data;
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
#define NTUSER_OBJ_IMC      0x11

/* NtUserScrollWindowEx flag */
#define SW_NODCCACHE  0x8000

/* NtUserSetScrollInfo flag */
#define SIF_RETURNPREV  0x1000

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

/* NtUserGetClipboardData params, not compatible with Windows */
struct get_clipboard_params
{
    void  *data;
    size_t size;
    size_t data_size;
    UINT   seqno;
    BOOL data_only;
};

/* NtUserSetClipboardData params, not compatible with Windows */
struct set_clipboard_params
{
    void  *data;
    size_t size;
    BOOL   cache_only;
    UINT   seqno;
};

/* NtUserNonClientButton params */
struct draw_non_client_button_params
{
    HWND hwnd;
    HDC hdc;
    enum NONCLIENT_BUTTON_TYPE type;
    RECT rect;
    BOOL down;
    BOOL grayed;
};

/* NtUserDrawScrollBar params */
struct draw_scroll_bar_params
{
    HWND hwnd;
    HDC hdc;
    INT bar;
    UINT hit_test;
    struct SCROLL_TRACKING_INFO tracking_info;
    BOOL arrows;
    BOOL interior;
    RECT rect;
    UINT enable_flags;
    INT arrow_size;
    INT thumb_pos;
    INT thumb_size;
    BOOL vertical;
};

/* NtUserUpdateInputContext param, not compatible with Window */
enum input_context_attr
{
    NtUserInputContextClientPtr,
    NtUserInputContextThreadId,
};

/* NtUserAssociateInputContext result */
enum associate_input_context_result
{
    AICR_OK,
    AICR_FOCUS_CHANGED,
    AICR_FAILED,
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
    WM_WINE_UPDATEWINDOWSTATE,
    WM_WINE_FIRST_DRIVER_MSG = 0x80001000,  /* range of messages reserved for the USER driver */
    WM_WINE_CLIPCURSOR = 0x80001ff0, /* internal driver notification messages */
    WM_WINE_SETCURSOR,
    WM_WINE_LAST_DRIVER_MSG = 0x80001fff
};

/* internal IME message */
#define WM_IME_INTERNAL 0x287
#define IME_INTERNAL_ACTIVATE   0x17
#define IME_INTERNAL_DEACTIVATE 0x18
#define IME_INTERNAL_HKL_ACTIVATE    0x19
#define IME_INTERNAL_HKL_DEACTIVATE  0x20

/* internal WM_IME_NOTIFY wparams, not compatible with Windows */
#define IMN_WINE_SET_OPEN_STATUS  0x000f
#define IMN_WINE_SET_COMP_STRING  0x0010

/* builtin IME driver calls */
enum wine_ime_call
{
    WINE_IME_PROCESS_KEY,
    WINE_IME_TO_ASCII_EX,
};

/* NtUserImeDriverCall params */
struct ime_driver_call_params
{
    HIMC himc;
    const BYTE *state;
    COMPOSITIONSTRING *compstr;
};

/* NtUserSystemTrayCall calls */
enum wine_systray_call
{
    WINE_SYSTRAY_NOTIFY_ICON,
    WINE_SYSTRAY_CLEANUP_ICONS,
    WINE_SYSTRAY_DOCK_INIT,
    WINE_SYSTRAY_DOCK_INSERT,
    WINE_SYSTRAY_DOCK_CLEAR,
    WINE_SYSTRAY_DOCK_REMOVE,
};

#define WM_SYSTIMER  0x0118


W32KAPI HKL     WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags );
W32KAPI BOOL    WINAPI NtUserAddClipboardFormatListener( HWND hwnd );
W32KAPI UINT    WINAPI NtUserAssociateInputContext( HWND hwnd, HIMC ctx, ULONG flags );
W32KAPI BOOL    WINAPI NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach );
W32KAPI HDC     WINAPI NtUserBeginPaint( HWND hwnd, PAINTSTRUCT *ps );
W32KAPI NTSTATUS WINAPI NtUserBuildHimcList( UINT thread_id, UINT count, HIMC *buffer, UINT *size );
W32KAPI NTSTATUS WINAPI NtUserBuildHwndList( HDESK desktop, ULONG unk2, ULONG unk3, ULONG unk4,
                                             ULONG thread_id, ULONG count, HWND *buffer, ULONG *size );
W32KAPI ULONG_PTR WINAPI NtUserCallHwnd( HWND hwnd, DWORD code );
W32KAPI ULONG_PTR WINAPI NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code );
W32KAPI LRESULT WINAPI NtUserCallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam );
W32KAPI BOOL    WINAPI NtUserCallMsgFilter( MSG *msg, INT code );
W32KAPI ULONG_PTR WINAPI NtUserCallNoParam( ULONG code );
W32KAPI ULONG_PTR WINAPI NtUserCallOneParam( ULONG_PTR arg, ULONG code );
W32KAPI ULONG_PTR WINAPI NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code );
W32KAPI BOOL    WINAPI NtUserChangeClipboardChain( HWND hwnd, HWND next );
W32KAPI LONG    WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                                    DWORD flags, void *lparam );
W32KAPI DWORD   WINAPI NtUserCheckMenuItem( HMENU handle, UINT id, UINT flags );
W32KAPI HWND    WINAPI NtUserChildWindowFromPointEx( HWND parent, LONG x, LONG y, UINT flags );
W32KAPI BOOL    WINAPI NtUserClipCursor( const RECT *rect );
W32KAPI BOOL    WINAPI NtUserCloseClipboard(void);
W32KAPI BOOL    WINAPI NtUserCloseDesktop( HDESK handle );
W32KAPI BOOL    WINAPI NtUserCloseWindowStation( HWINSTA handle );
W32KAPI INT     WINAPI NtUserCopyAcceleratorTable( HACCEL src, ACCEL *dst, INT count );
W32KAPI INT     WINAPI NtUserCountClipboardFormats(void);
W32KAPI HACCEL  WINAPI NtUserCreateAcceleratorTable( ACCEL *table, INT count );
W32KAPI BOOL    WINAPI NtUserCreateCaret( HWND hwnd, HBITMAP bitmap, int width, int height );
W32KAPI HDESK   WINAPI NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                              DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                              ULONG heap_size );
W32KAPI HIMC    WINAPI NtUserCreateInputContext( UINT_PTR client_ptr );
W32KAPI HWND    WINAPI NtUserCreateWindowEx( DWORD ex_style, UNICODE_STRING *class_name,
                                             UNICODE_STRING *version, UNICODE_STRING *window_name,
                                             DWORD style, INT x, INT y, INT cx, INT cy,
                                             HWND parent, HMENU menu, HINSTANCE instance, void *params,
                                             DWORD flags, HINSTANCE client_instance, DWORD unk, BOOL ansi );
W32KAPI HWINSTA WINAPI NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK mask, ULONG arg3,
                                                  ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 );
W32KAPI HDWP    WINAPI NtUserDeferWindowPosAndBand( HDWP hdwp, HWND hwnd, HWND after, INT x, INT y,
                                                    INT cx, INT cy, UINT flags, UINT unk1, UINT unk2 );
W32KAPI BOOL    WINAPI NtUserDeleteMenu( HMENU menu, UINT id, UINT flags );
W32KAPI BOOL    WINAPI NtUserDestroyAcceleratorTable( HACCEL handle );
W32KAPI BOOL    WINAPI NtUserDestroyCursor( HCURSOR cursor, ULONG arg );
W32KAPI BOOL    WINAPI NtUserDestroyInputContext( HIMC handle );
W32KAPI BOOL    WINAPI NtUserDestroyMenu( HMENU menu );
W32KAPI BOOL    WINAPI NtUserDestroyWindow( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserDisableThreadIme( DWORD thread_id );
W32KAPI LRESULT WINAPI NtUserDispatchMessage( const MSG *msg );
W32KAPI NTSTATUS WINAPI NtUserDisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_HEADER *packet );
W32KAPI BOOL    WINAPI NtUserDragDetect( HWND hwnd, int x, int y );
W32KAPI DWORD   WINAPI NtUserDragObject( HWND parent, HWND hwnd, UINT fmt, ULONG_PTR data, HCURSOR cursor );
W32KAPI BOOL    WINAPI NtUserDrawCaptionTemp( HWND hwnd, HDC hdc, const RECT *rect, HFONT font,
                                              HICON icon, const WCHAR *str, UINT flags );
W32KAPI BOOL    WINAPI NtUserDrawIconEx( HDC hdc, INT x0, INT y0, HICON icon, INT width,
                                         INT height, UINT istep, HBRUSH hbr, UINT flags );
W32KAPI DWORD   WINAPI NtUserDrawMenuBarTemp( HWND hwnd, HDC hdc, RECT *rect, HMENU handle, HFONT font );
W32KAPI BOOL    WINAPI NtUserEmptyClipboard(void);
W32KAPI BOOL    WINAPI NtUserEnableMenuItem( HMENU handle, UINT id, UINT flags );
W32KAPI BOOL    WINAPI NtUserEnableMouseInPointer( BOOL );
W32KAPI BOOL    WINAPI NtUserEnableScrollBar( HWND hwnd, UINT bar, UINT flags );
W32KAPI BOOL    WINAPI NtUserEndDeferWindowPosEx( HDWP hdwp, BOOL async );
W32KAPI BOOL    WINAPI NtUserEndMenu(void);
W32KAPI BOOL    WINAPI NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps );
W32KAPI NTSTATUS WINAPI NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                                  DISPLAY_DEVICEW *info, DWORD flags );
W32KAPI BOOL    WINAPI NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lp );
W32KAPI BOOL    WINAPI NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD mode,
                                                  DEVMODEW *dev_mode, DWORD flags );
W32KAPI INT     WINAPI NtUserExcludeUpdateRgn( HDC hdc, HWND hwnd );
W32KAPI HICON   WINAPI NtUserFindExistingCursorIcon( UNICODE_STRING *module, UNICODE_STRING *res_name,
                                                     void *desc );
W32KAPI HWND    WINAPI NtUserFindWindowEx( HWND parent, HWND child, UNICODE_STRING *class,
                                           UNICODE_STRING *title, ULONG unk );
W32KAPI BOOL    WINAPI NtUserFlashWindowEx( FLASHWINFO *info );
W32KAPI HWND    WINAPI NtUserGetAncestor( HWND hwnd, UINT type );
W32KAPI SHORT   WINAPI NtUserGetAsyncKeyState( INT key );
W32KAPI ULONG   WINAPI NtUserGetAtomName( ATOM atom, UNICODE_STRING *name );
W32KAPI UINT    WINAPI NtUserGetCaretBlinkTime(void);
W32KAPI BOOL    WINAPI NtUserGetCaretPos( POINT *point );
W32KAPI ATOM    WINAPI NtUserGetClassInfoEx( HINSTANCE instance, UNICODE_STRING *name, WNDCLASSEXW *wc,
                                             struct client_menu_name *menu_name, BOOL ansi );
W32KAPI INT     WINAPI NtUserGetClassName( HWND hwnd, BOOL real, UNICODE_STRING *name );
W32KAPI HANDLE  WINAPI NtUserGetClipboardData( UINT format, struct get_clipboard_params *params );
W32KAPI INT     WINAPI NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen );
W32KAPI HWND    WINAPI NtUserGetClipboardOwner(void);
W32KAPI DWORD   WINAPI NtUserGetClipboardSequenceNumber(void);
W32KAPI HWND    WINAPI NtUserGetClipboardViewer(void);
W32KAPI HCURSOR WINAPI NtUserGetCursor(void);
W32KAPI HCURSOR WINAPI NtUserGetCursorFrameInfo( HCURSOR hCursor, DWORD istep, DWORD *rate_jiffies,
                                                 DWORD *num_steps );
W32KAPI BOOL    WINAPI NtUserGetCursorInfo( CURSORINFO *info );
W32KAPI HDC     WINAPI NtUserGetDC( HWND hwnd );
W32KAPI HDC     WINAPI NtUserGetDCEx( HWND hwnd, HRGN clip_rgn, DWORD flags );
W32KAPI LONG    WINAPI NtUserGetDisplayConfigBufferSizes( UINT32 flags, UINT32 *num_path_info,
                                                          UINT32 *num_mode_info );
W32KAPI UINT    WINAPI NtUserGetDoubleClickTime(void);
W32KAPI BOOL    WINAPI NtUserGetDpiForMonitor( HMONITOR monitor, UINT type, UINT *x, UINT *y );
W32KAPI HWND    WINAPI NtUserGetForegroundWindow(void);
W32KAPI BOOL    WINAPI NtUserGetGUIThreadInfo( DWORD id, GUITHREADINFO *info );
W32KAPI BOOL    WINAPI NtUserGetIconInfo( HICON icon, ICONINFO *info, UNICODE_STRING *module,
                                          UNICODE_STRING *res_name, DWORD *bpp, LONG unk );
W32KAPI BOOL    WINAPI NtUserGetIconSize( HICON handle, UINT step, LONG *width, LONG *height );
W32KAPI UINT    WINAPI NtUserGetInternalWindowPos( HWND hwnd, RECT *rect, POINT *pt );
W32KAPI INT     WINAPI NtUserGetKeyNameText( LONG lparam, WCHAR *buffer, INT size );
W32KAPI SHORT   WINAPI NtUserGetKeyState( INT vkey );
W32KAPI HKL     WINAPI NtUserGetKeyboardLayout( DWORD thread_id );
W32KAPI UINT    WINAPI NtUserGetKeyboardLayoutList( INT size, HKL *layouts );
W32KAPI BOOL    WINAPI NtUserGetKeyboardLayoutName( WCHAR *name );
W32KAPI BOOL    WINAPI NtUserGetKeyboardState( BYTE *state );
W32KAPI BOOL    WINAPI NtUserGetLayeredWindowAttributes( HWND hwnd, COLORREF *key, BYTE *alpha, DWORD *flags );
W32KAPI BOOL    WINAPI NtUserGetMenuBarInfo( HWND hwnd, LONG id, LONG item, MENUBARINFO *info );
W32KAPI BOOL    WINAPI NtUserGetMenuItemRect( HWND hwnd, HMENU menu, UINT item, RECT *rect );
W32KAPI BOOL    WINAPI NtUserGetMessage( MSG *msg, HWND hwnd, UINT first, UINT last );
W32KAPI int     WINAPI NtUserGetMouseMovePointsEx( UINT size, MOUSEMOVEPOINT *ptin, MOUSEMOVEPOINT *ptout,
                                                   int count, DWORD resolution );
W32KAPI BOOL    WINAPI NtUserGetObjectInformation( HANDLE handle, INT index, void *info,
                                                   DWORD len, DWORD *needed );
W32KAPI HWND    WINAPI NtUserGetOpenClipboardWindow(void);
W32KAPI BOOL    WINAPI NtUserGetPointerInfoList( UINT32 id, POINTER_INPUT_TYPE type, UINT_PTR, UINT_PTR, SIZE_T size,
                                                 UINT32 *entry_count, UINT32 *pointer_count, void *pointer_info );
W32KAPI INT     WINAPI NtUserGetPriorityClipboardFormat( UINT *list, INT count );
W32KAPI HWINSTA WINAPI NtUserGetProcessWindowStation(void);
W32KAPI HANDLE  WINAPI NtUserGetProp( HWND hwnd, const WCHAR *str );
W32KAPI ULONG   WINAPI NtUserGetProcessDpiAwarenessContext( HANDLE process );
W32KAPI DWORD   WINAPI NtUserGetQueueStatus( UINT flags );
W32KAPI UINT    WINAPI NtUserGetRawInputBuffer( RAWINPUT *data, UINT *data_size, UINT header_size );
W32KAPI UINT    WINAPI NtUserGetRawInputData( HRAWINPUT rawinput, UINT command, void *data, UINT *data_size, UINT header_size );
W32KAPI UINT    WINAPI NtUserGetRawInputDeviceInfo( HANDLE handle, UINT command, void *data, UINT *data_size );
W32KAPI UINT    WINAPI NtUserGetRawInputDeviceList( RAWINPUTDEVICELIST *devices, UINT *device_count, UINT size );
W32KAPI UINT    WINAPI NtUserGetRegisteredRawInputDevices( RAWINPUTDEVICE *devices, UINT *device_count, UINT size );
W32KAPI BOOL    WINAPI NtUserGetScrollBarInfo( HWND hwnd, LONG id, SCROLLBARINFO *info );
W32KAPI ULONG   WINAPI NtUserGetSystemDpiForProcess( HANDLE process );
W32KAPI HMENU   WINAPI NtUserGetSystemMenu( HWND hwnd, BOOL revert );
W32KAPI HDESK   WINAPI NtUserGetThreadDesktop( DWORD thread );
W32KAPI BOOL    WINAPI NtUserGetTitleBarInfo( HWND hwnd, TITLEBARINFO *info );
W32KAPI INT     WINAPI NtUserGetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase );
W32KAPI BOOL    WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size );
W32KAPI BOOL    WINAPI NtUserGetUpdateRect( HWND hwnd, RECT *rect, BOOL erase );
W32KAPI HDC     WINAPI NtUserGetWindowDC( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserGetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *placement );
W32KAPI int     WINAPI NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk );
W32KAPI BOOL    WINAPI NtUserHideCaret( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserHiliteMenuItem( HWND hwnd, HMENU handle, UINT item, UINT hilite );
W32KAPI NTSTATUS WINAPI NtUserInitializeClientPfnArrays( const struct user_client_procs *client_procsA,
                                                         const struct user_client_procs *client_procsW,
                                                         const void *client_workers, HINSTANCE user_module );
W32KAPI HICON   WINAPI NtUserInternalGetWindowIcon( HWND hwnd, UINT type );
W32KAPI INT     WINAPI NtUserInternalGetWindowText( HWND hwnd, WCHAR *text, INT count );
W32KAPI BOOL    WINAPI NtUserIsClipboardFormatAvailable( UINT format );
W32KAPI BOOL    WINAPI NtUserIsMouseInPointerEnabled(void);
W32KAPI BOOL    WINAPI NtUserInvalidateRect( HWND hwnd, const RECT *rect, BOOL erase );
W32KAPI BOOL    WINAPI NtUserInvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase );
W32KAPI BOOL    WINAPI NtUserKillTimer( HWND hwnd, UINT_PTR id );
W32KAPI BOOL    WINAPI NtUserLockWindowUpdate( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserLogicalToPerMonitorDPIPhysicalPoint( HWND hwnd, POINT *pt );
W32KAPI UINT    WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout );
W32KAPI INT     WINAPI NtUserMenuItemFromPoint( HWND hwnd, HMENU handle, int x, int y );
W32KAPI LRESULT WINAPI NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                          void *result_info, DWORD type, BOOL ansi );
W32KAPI BOOL    WINAPI NtUserMoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint );
W32KAPI DWORD   WINAPI NtUserMsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                          DWORD timeout, DWORD mask, DWORD flags );
W32KAPI void    WINAPI NtUserNotifyIMEStatus( HWND hwnd, UINT status );
W32KAPI void    WINAPI NtUserNotifyWinEvent( DWORD event, HWND hwnd, LONG object_id, LONG child_id );
W32KAPI HWINSTA WINAPI NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access );
W32KAPI BOOL    WINAPI NtUserOpenClipboard( HWND hwnd, ULONG unk );
W32KAPI HDESK   WINAPI NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access );
W32KAPI HDESK   WINAPI NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access );
W32KAPI BOOL    WINAPI NtUserPeekMessage( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags );
W32KAPI BOOL    WINAPI NtUserPerMonitorDPIPhysicalToLogicalPoint( HWND hwnd, POINT *pt );
W32KAPI BOOL    WINAPI NtUserPostMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
W32KAPI BOOL    WINAPI NtUserPostThreadMessage( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam );
W32KAPI BOOL    WINAPI NtUserPrintWindow( HWND hwnd, HDC hdc, UINT flags );
W32KAPI LONG    WINAPI NtUserQueryDisplayConfig( UINT32 flags, UINT32 *paths_count, DISPLAYCONFIG_PATH_INFO *paths,
                                                 UINT32 *modes_count, DISPLAYCONFIG_MODE_INFO *modes,
                                                 DISPLAYCONFIG_TOPOLOGY_ID *topology_id);
W32KAPI UINT_PTR WINAPI NtUserQueryInputContext( HIMC handle, UINT attr );
W32KAPI HWND    WINAPI NtUserRealChildWindowFromPoint( HWND parent, LONG x, LONG y );
W32KAPI BOOL    WINAPI NtUserRedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags );
W32KAPI ATOM    WINAPI NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                                 struct client_menu_name *client_menu_name, DWORD fnid, DWORD flags,
                                                 DWORD *wow );
W32KAPI BOOL    WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk );
W32KAPI BOOL    WINAPI NtUserRegisterRawInputDevices( const RAWINPUTDEVICE *devices, UINT device_count, UINT size );
W32KAPI INT     WINAPI NtUserReleaseDC( HWND hwnd, HDC hdc );
W32KAPI BOOL    WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserRemoveMenu( HMENU menu, UINT id, UINT flags );
W32KAPI HANDLE  WINAPI NtUserRemoveProp( HWND hwnd, const WCHAR *str );
W32KAPI BOOL    WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                       HRGN ret_update_rgn, RECT *update_rect );
W32KAPI INT     WINAPI NtUserScrollWindowEx( HWND hwnd, INT dx, INT dy, const RECT *rect,
                                             const RECT *clip_rect, HRGN update_rgn,
                                             RECT *update_rect, UINT flags );
W32KAPI HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE palette, WORD force_background );
W32KAPI UINT     WINAPI NtUserSendInput( UINT count, INPUT *inputs, int size );
W32KAPI HWND     WINAPI NtUserSetActiveWindow( HWND hwnd );
W32KAPI HWND     WINAPI NtUserSetCapture( HWND hwnd );
W32KAPI DWORD    WINAPI NtUserSetClassLong( HWND hwnd, INT offset, LONG newval, BOOL ansi );
W32KAPI ULONG_PTR WINAPI NtUserSetClassLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi );
W32KAPI WORD    WINAPI NtUserSetClassWord( HWND hwnd, INT offset, WORD newval );
W32KAPI NTSTATUS WINAPI NtUserSetClipboardData( UINT format, HANDLE handle, struct set_clipboard_params *params );
W32KAPI HWND    WINAPI NtUserSetClipboardViewer( HWND hwnd );
W32KAPI HCURSOR WINAPI NtUserSetCursor( HCURSOR cursor );
W32KAPI BOOL    WINAPI NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                                struct cursoricon_desc *desc );
W32KAPI BOOL    WINAPI NtUserSetCursorPos( INT x, INT y );
W32KAPI HWND    WINAPI NtUserSetFocus( HWND hwnd );
W32KAPI void    WINAPI NtUserSetInternalWindowPos( HWND hwnd, UINT cmd, RECT *rect, POINT *pt );
W32KAPI BOOL    WINAPI NtUserSetKeyboardState( BYTE *state );
W32KAPI BOOL    WINAPI NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags );
W32KAPI BOOL    WINAPI NtUserSetMenu( HWND hwnd, HMENU menu );
W32KAPI BOOL    WINAPI NtUserSetMenuContextHelpId( HMENU handle, DWORD id );
W32KAPI BOOL    WINAPI NtUserSetMenuDefaultItem( HMENU handle, UINT item, UINT bypos );
W32KAPI BOOL    WINAPI NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len );
W32KAPI HWND    WINAPI NtUserSetParent( HWND hwnd, HWND parent );
W32KAPI BOOL    WINAPI NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown );
W32KAPI BOOL    WINAPI NtUserSetProcessWindowStation( HWINSTA handle );
W32KAPI BOOL    WINAPI NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle );
W32KAPI INT     WINAPI NtUserSetScrollInfo( HWND hwnd, INT bar, const SCROLLINFO *info, BOOL redraw );
W32KAPI BOOL    WINAPI NtUserSetShellWindowEx( HWND shell, HWND list_view );
W32KAPI BOOL    WINAPI NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values );
W32KAPI BOOL    WINAPI NtUserSetSystemMenu( HWND hwnd, HMENU menu );
W32KAPI UINT_PTR WINAPI NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout );
W32KAPI BOOL    WINAPI NtUserSetThreadDesktop( HDESK handle );
W32KAPI UINT_PTR WINAPI NtUserSetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc, ULONG tolerance );
W32KAPI LONG    WINAPI NtUserSetWindowLong( HWND hwnd, INT offset, LONG newval, BOOL ansi );
W32KAPI LONG_PTR WINAPI NtUserSetWindowLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi );
W32KAPI BOOL    WINAPI NtUserSetWindowPlacement( HWND hwnd, const WINDOWPLACEMENT *wpl );
W32KAPI BOOL    WINAPI NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags );
W32KAPI int     WINAPI NtUserSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw );
W32KAPI WORD    WINAPI NtUserSetWindowWord( HWND hwnd, INT offset, WORD newval );
W32KAPI HHOOK   WINAPI NtUserSetWindowsHookEx( HINSTANCE inst, UNICODE_STRING *module, DWORD tid, INT id,
                                               HOOKPROC proc, BOOL ansi );
W32KAPI HWINEVENTHOOK WINAPI NtUserSetWinEventHook( DWORD event_min, DWORD event_max, HMODULE inst,
                                                    UNICODE_STRING *module, WINEVENTPROC proc,
                                                    DWORD pid, DWORD tid, DWORD flags );
W32KAPI BOOL    WINAPI NtUserShowCaret( HWND hwnd );
W32KAPI INT     WINAPI NtUserShowCursor( BOOL show );
W32KAPI BOOL    WINAPI NtUserShowScrollBar( HWND hwnd, INT bar, BOOL show );
W32KAPI BOOL    WINAPI NtUserShowWindow( HWND hwnd, INT cmd );
W32KAPI BOOL    WINAPI NtUserShowWindowAsync( HWND hwnd, INT cmd );
W32KAPI BOOL    WINAPI NtUserSystemParametersInfo( UINT action, UINT val, void *ptr, UINT winini );
W32KAPI BOOL    WINAPI NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi );
W32KAPI BOOL    WINAPI NtUserThunkedMenuInfo( HMENU menu, const MENUINFO *info );
W32KAPI UINT    WINAPI NtUserThunkedMenuItemInfo( HMENU menu, UINT pos, UINT flags, UINT method,
                                                  MENUITEMINFOW *info, UNICODE_STRING *str );
W32KAPI INT     WINAPI NtUserToUnicodeEx( UINT virt, UINT scan, const BYTE *state,
                                          WCHAR *str, int size, UINT flags, HKL layout );
W32KAPI BOOL    WINAPI NtUserTrackMouseEvent( TRACKMOUSEEVENT *info );
W32KAPI BOOL    WINAPI NtUserTrackPopupMenuEx( HMENU handle, UINT flags, INT x, INT y, HWND hwnd, TPMPARAMS *params );
W32KAPI INT     WINAPI NtUserTranslateAccelerator( HWND hwnd, HACCEL accel, MSG *msg );
W32KAPI BOOL    WINAPI NtUserTranslateMessage( const MSG *msg, UINT flags );
W32KAPI BOOL    WINAPI NtUserUnhookWinEvent( HWINEVENTHOOK hEventHook );
W32KAPI BOOL    WINAPI NtUserUnhookWindowsHookEx( HHOOK handle );
W32KAPI BOOL    WINAPI NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                              struct client_menu_name *client_menu_name );
W32KAPI BOOL    WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id );
W32KAPI BOOL    WINAPI NtUserUpdateInputContext( HIMC handle, UINT attr, UINT_PTR value );
W32KAPI BOOL    WINAPI NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                                  HDC hdc_src, const POINT *pts_src, COLORREF key,
                                                  const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty );
W32KAPI BOOL    WINAPI NtUserValidateRect( HWND hwnd, const RECT *rect );
W32KAPI WORD    WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout );
W32KAPI DWORD   WINAPI NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow );
W32KAPI BOOL    WINAPI NtUserWaitMessage(void);
W32KAPI HWND    WINAPI NtUserWindowFromDC( HDC hdc );
W32KAPI HWND    WINAPI NtUserWindowFromPoint( LONG x, LONG y );

/* NtUserCallNoParam codes, not compatible with Windows */
enum
{
    NtUserCallNoParam_DestroyCaret,
    NtUserCallNoParam_GetDesktopWindow,
    NtUserCallNoParam_GetDialogBaseUnits,
    NtUserCallNoParam_GetInputState,
    NtUserCallNoParam_GetProcessDefaultLayout,
    NtUserCallNoParam_GetProgmanWindow,
    NtUserCallNoParam_GetShellWindow,
    NtUserCallNoParam_GetTaskmanWindow,
    NtUserCallNoParam_ReleaseCapture,
    /* temporary exports */
    NtUserExitingThread,
    NtUserThreadDetach,
};

static inline BOOL NtUserDestroyCaret(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_DestroyCaret );
}

static inline HWND NtUserGetDesktopWindow(void)
{
    return UlongToHandle( NtUserCallNoParam( NtUserCallNoParam_GetDesktopWindow ));
}

static inline DWORD NtUserGetDialogBaseUnits(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_GetDialogBaseUnits );
};

static inline BOOL NtUserGetInputState(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_GetInputState );
}

static inline DWORD NtUserGetProcessDefaultLayout(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_GetProcessDefaultLayout );
}

static inline HWND NtUserGetProgmanWindow(void)
{
    return UlongToHandle( NtUserCallNoParam( NtUserCallNoParam_GetProgmanWindow ));
}

static inline HWND NtUserGetShellWindow(void)
{
    return UlongToHandle( NtUserCallNoParam( NtUserCallNoParam_GetShellWindow ));
}

static inline HWND NtUserGetTaskmanWindow(void)
{
    return UlongToHandle( NtUserCallNoParam( NtUserCallNoParam_GetTaskmanWindow ));
}

static inline BOOL NtUserReleaseCapture(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_ReleaseCapture );
}

/* NtUserCallOneParam codes, not compatible with Windows */
enum
{
    NtUserCallOneParam_BeginDeferWindowPos,
    NtUserCallOneParam_CreateCursorIcon,
    NtUserCallOneParam_CreateMenu,
    NtUserCallOneParam_EnableDC,
    NtUserCallOneParam_EnableThunkLock,
    NtUserCallOneParam_EnumClipboardFormats,
    NtUserCallOneParam_GetClipCursor,
    NtUserCallOneParam_GetCursorPos,
    NtUserCallOneParam_GetIconParam,
    NtUserCallOneParam_GetMenuItemCount,
    NtUserCallOneParam_GetPrimaryMonitorRect,
    NtUserCallOneParam_GetSysColor,
    NtUserCallOneParam_GetSysColorBrush,
    NtUserCallOneParam_GetSysColorPen,
    NtUserCallOneParam_GetSystemMetrics,
    NtUserCallOneParam_GetVirtualScreenRect,
    NtUserCallOneParam_IsWindowRectFullScreen,
    NtUserCallOneParam_MessageBeep,
    NtUserCallOneParam_RealizePalette,
    NtUserCallOneParam_ReplyMessage,
    NtUserCallOneParam_SetCaretBlinkTime,
    NtUserCallOneParam_SetProcessDefaultLayout,
    NtUserCallOneParam_SetKeyboardAutoRepeat,
    /* temporary exports */
    NtUserGetDeskPattern,
};

static inline HDWP NtUserBeginDeferWindowPos( INT count )
{
    return UlongToHandle( NtUserCallOneParam( count, NtUserCallOneParam_BeginDeferWindowPos ));
}

static inline HICON NtUserCreateCursorIcon( BOOL is_icon )
{
    return UlongToHandle( NtUserCallOneParam( is_icon, NtUserCallOneParam_CreateCursorIcon ));
}

static inline HMENU NtUserCreateMenu( BOOL is_popup )
{
    return UlongToHandle( NtUserCallOneParam( is_popup, NtUserCallOneParam_CreateMenu ));
}

static inline WORD NtUserEnableDC( HDC hdc )
{
    return NtUserCallOneParam( HandleToUlong(hdc), NtUserCallOneParam_EnableDC );
}

static inline void NtUserEnableThunkLock( BOOL enable )
{
    NtUserCallOneParam( enable, NtUserCallOneParam_EnableThunkLock );
}

static inline UINT NtUserEnumClipboardFormats( UINT format )
{
    return NtUserCallOneParam( format, NtUserCallOneParam_EnumClipboardFormats );
}

static inline BOOL NtUserGetClipCursor( RECT *rect )
{
    return NtUserCallOneParam( (UINT_PTR)rect, NtUserCallOneParam_GetClipCursor );
}

static inline BOOL NtUserGetCursorPos( POINT *pt )
{
    return NtUserCallOneParam( (UINT_PTR)pt, NtUserCallOneParam_GetCursorPos );
}

static inline UINT_PTR NtUserGetIconParam( HICON icon )
{
    return NtUserCallOneParam( HandleToUlong(icon), NtUserCallOneParam_GetIconParam );
}

static inline UINT_PTR NtUserGetMenuItemCount( HMENU menu )
{
    return NtUserCallOneParam( HandleToUlong(menu), NtUserCallOneParam_GetMenuItemCount );
}

static inline RECT NtUserGetPrimaryMonitorRect(void)
{
    RECT primary;
    NtUserCallOneParam( (UINT_PTR)&primary, NtUserCallOneParam_GetPrimaryMonitorRect );
    return primary;
}

static inline BOOL NtUserSetCaretBlinkTime( unsigned int time )
{
    return NtUserCallOneParam( time, NtUserCallOneParam_SetCaretBlinkTime );
}

static inline COLORREF NtUserGetSysColor( INT index )
{
    return NtUserCallOneParam( index, NtUserCallOneParam_GetSysColor );
}

static inline HBRUSH NtUserGetSysColorBrush( INT index )
{
    return UlongToHandle( NtUserCallOneParam( index, NtUserCallOneParam_GetSysColorBrush ));
}

static inline HPEN NtUserGetSysColorPen( INT index )
{
    return UlongToHandle( NtUserCallOneParam( index, NtUserCallOneParam_GetSysColorPen ));
}

static inline INT NtUserGetSystemMetrics( INT index )
{
    return NtUserCallOneParam( index, NtUserCallOneParam_GetSystemMetrics );
}

static inline RECT NtUserGetVirtualScreenRect(void)
{
    RECT virtual;
    NtUserCallOneParam( (UINT_PTR)&virtual, NtUserCallOneParam_GetVirtualScreenRect );
    return virtual;
}

static inline BOOL NtUserIsWindowRectFullScreen( const RECT *rect )
{
    return NtUserCallOneParam( (UINT_PTR)rect, NtUserCallOneParam_IsWindowRectFullScreen );
}

static inline BOOL NtUserMessageBeep( UINT i )
{
    return NtUserCallOneParam( i, NtUserCallOneParam_MessageBeep );
}

static inline UINT NtUserRealizePalette( HDC hdc )
{
    return NtUserCallOneParam( HandleToUlong(hdc), NtUserCallOneParam_RealizePalette );
}

static inline BOOL NtUserReplyMessage( LRESULT result )
{
    return NtUserCallOneParam( result, NtUserCallOneParam_ReplyMessage );
}

static inline UINT NtUserSetProcessDefaultLayout( DWORD layout )
{
    return NtUserCallOneParam( layout, NtUserCallOneParam_SetProcessDefaultLayout );
}

/* NtUserCallTwoParam codes, not compatible with Windows */
enum
{
    NtUserCallTwoParam_GetDialogProc,
    NtUserCallTwoParam_GetMenuInfo,
    NtUserCallTwoParam_GetMonitorInfo,
    NtUserCallTwoParam_GetSystemMetricsForDpi,
    NtUserCallTwoParam_MonitorFromRect,
    NtUserCallTwoParam_SetCaretPos,
    NtUserCallTwoParam_SetIconParam,
    NtUserCallTwoParam_UnhookWindowsHook,
    /* temporary exports */
    NtUserAllocWinProc,
};

static inline DLGPROC NtUserGetDialogProc( DLGPROC proc, BOOL ansi )
{
    return (DLGPROC)NtUserCallTwoParam( (UINT_PTR)proc, ansi, NtUserCallTwoParam_GetDialogProc );
}

static inline BOOL NtUserGetMenuInfo( HMENU menu, MENUINFO *info )
{
    return NtUserCallTwoParam( HandleToUlong(menu), (ULONG_PTR)info,
                               NtUserCallTwoParam_GetMenuInfo );
}

static inline BOOL NtUserGetMonitorInfo( HMONITOR monitor, MONITORINFO *info )
{
    return NtUserCallTwoParam( HandleToUlong(monitor), (ULONG_PTR)info,
                               NtUserCallTwoParam_GetMonitorInfo );
}

static inline INT NtUserGetSystemMetricsForDpi( INT index, UINT dpi )
{
    return NtUserCallTwoParam( index, dpi, NtUserCallTwoParam_GetSystemMetricsForDpi );
}

static inline HMONITOR NtUserMonitorFromRect( const RECT *rect, DWORD flags )
{
    ULONG ret = NtUserCallTwoParam( (LONG_PTR)rect, flags, NtUserCallTwoParam_MonitorFromRect );
    return UlongToHandle( ret );
}

static inline BOOL NtUserSetCaretPos( int x, int y )
{
    return NtUserCallTwoParam( x, y, NtUserCallTwoParam_SetCaretPos );
}

static inline UINT_PTR NtUserSetIconParam( HICON icon, ULONG_PTR param )
{
    return NtUserCallTwoParam( HandleToUlong(icon), param, NtUserCallTwoParam_SetIconParam );
}

static inline BOOL NtUserUnhookWindowsHook( INT id, HOOKPROC proc )
{
    return NtUserCallTwoParam( id, (UINT_PTR)proc, NtUserCallTwoParam_UnhookWindowsHook );
}

/* NtUserCallHwnd codes, not compatible with Windows */
enum
{
    NtUserCallHwnd_ActivateOtherWindow,
    NtUserCallHwnd_ArrangeIconicWindows,
    NtUserCallHwnd_DrawMenuBar,
    NtUserCallHwnd_GetDefaultImeWindow,
    NtUserCallHwnd_GetDialogInfo,
    NtUserCallHwnd_GetDpiForWindow,
    NtUserCallHwnd_GetMDIClientInfo,
    NtUserCallHwnd_GetParent,
    NtUserCallHwnd_GetWindowContextHelpId,
    NtUserCallHwnd_GetWindowDpiAwarenessContext,
    NtUserCallHwnd_GetWindowInputContext,
    NtUserCallHwnd_GetWindowSysSubMenu,
    NtUserCallHwnd_GetWindowTextLength,
    NtUserCallHwnd_IsWindow,
    NtUserCallHwnd_IsWindowEnabled,
    NtUserCallHwnd_IsWindowUnicode,
    NtUserCallHwnd_IsWindowVisible,
    NtUserCallHwnd_SetForegroundWindow,
    NtUserCallHwnd_SetProgmanWindow,
    NtUserCallHwnd_SetTaskmanWindow,
    /* temporary exports */
    NtUserGetFullWindowHandle,
    NtUserIsCurrentProcessWindow,
    NtUserIsCurrentThreadWindow,
};

static inline void NtUserActivateOtherWindow( HWND hwnd )
{
    NtUserCallHwnd( hwnd, NtUserCallHwnd_ActivateOtherWindow );
}

static inline UINT NtUserArrangeIconicWindows( HWND parent )
{
    return NtUserCallHwnd( parent, NtUserCallHwnd_ArrangeIconicWindows );
}

static inline BOOL NtUserDrawMenuBar( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_DrawMenuBar );
}

static inline DWORD NtUserGetWindowContextHelpId( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_GetWindowContextHelpId );
}

static inline HWND NtUserGetDefaultImeWindow( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_GetDefaultImeWindow ));
}

static inline void *NtUserGetDialogInfo( HWND hwnd )
{
    return (void *)NtUserCallHwnd( hwnd, NtUserCallHwnd_GetDialogInfo );
}

static inline UINT NtUserGetDpiForWindow( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_GetDpiForWindow );
}

static inline void *NtUserGetMDIClientInfo( HWND hwnd )
{
    return (void *)NtUserCallHwnd( hwnd, NtUserCallHwnd_GetMDIClientInfo );
}

static inline HWND NtUserGetParent( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_GetParent ));
}

static inline DPI_AWARENESS_CONTEXT NtUserGetWindowDpiAwarenessContext( HWND hwnd )
{
    return (DPI_AWARENESS_CONTEXT)NtUserCallHwnd( hwnd,
                                                  NtUserCallHwnd_GetWindowDpiAwarenessContext );
}

static inline HIMC NtUserGetWindowInputContext( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_GetWindowInputContext ));
}

static inline HMENU NtUserGetWindowSysSubMenu( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_GetWindowSysSubMenu ));
}

static inline INT NtUserGetWindowTextLength( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_GetWindowTextLength );
}

static inline BOOL NtUserIsWindow( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_IsWindow );
}

static inline BOOL NtUserIsWindowEnabled( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_IsWindowEnabled );
}

static inline BOOL NtUserIsWindowUnicode( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_IsWindowUnicode );
}

static inline BOOL NtUserIsWindowVisible( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_IsWindowVisible );
}

static inline BOOL NtUserSetForegroundWindow( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_SetForegroundWindow );
}

static inline HWND NtUserSetProgmanWindow( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_SetProgmanWindow ));
}

static inline HWND NtUserSetTaskmanWindow( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_SetTaskmanWindow ));
}

/* NtUserCallHwndParam codes, not compatible with Windows */
enum
{
    NtUserCallHwndParam_ClientToScreen,
    NtUserCallHwndParam_EnableWindow,
    NtUserCallHwndParam_GetChildRect,
    NtUserCallHwndParam_GetClassLongA,
    NtUserCallHwndParam_GetClassLongW,
    NtUserCallHwndParam_GetClassLongPtrA,
    NtUserCallHwndParam_GetClassLongPtrW,
    NtUserCallHwndParam_GetClassWord,
    NtUserCallHwndParam_GetClientRect,
    NtUserCallHwndParam_GetScrollInfo,
    NtUserCallHwndParam_GetWindowInfo,
    NtUserCallHwndParam_GetWindowLongA,
    NtUserCallHwndParam_GetWindowLongW,
    NtUserCallHwndParam_GetWindowLongPtrA,
    NtUserCallHwndParam_GetWindowLongPtrW,
    NtUserCallHwndParam_GetWindowRect,
    NtUserCallHwndParam_GetWindowRelative,
    NtUserCallHwndParam_GetWindowThread,
    NtUserCallHwndParam_GetWindowWord,
    NtUserCallHwndParam_IsChild,
    NtUserCallHwndParam_KillSystemTimer,
    NtUserCallHwndParam_MapWindowPoints,
    NtUserCallHwndParam_MirrorRgn,
    NtUserCallHwndParam_MonitorFromWindow,
    NtUserCallHwndParam_ScreenToClient,
    NtUserCallHwndParam_SetDialogInfo,
    NtUserCallHwndParam_SetMDIClientInfo,
    NtUserCallHwndParam_SetWindowContextHelpId,
    NtUserCallHwndParam_ShowOwnedPopups,
    /* temporary exports */
    NtUserSetWindowStyle,
};

static inline BOOL NtUserClientToScreen( HWND hwnd, POINT *pt )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)pt, NtUserCallHwndParam_ClientToScreen );
}

static inline BOOL NtUserEnableWindow( HWND hwnd, BOOL enable )
{
    return NtUserCallHwndParam( hwnd, enable, NtUserCallHwndParam_EnableWindow );
}

static inline BOOL NtUserGetChildRect( HWND hwnd, RECT *rect )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)rect, NtUserCallHwndParam_GetChildRect );
}

static inline DWORD NtUserGetClassLongA( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetClassLongA );
}

static inline ULONG_PTR NtUserGetClassLongPtrA( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetClassLongPtrA );
}

static inline ULONG_PTR NtUserGetClassLongPtrW( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetClassLongPtrW );
}

static inline DWORD NtUserGetClassLongW( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetClassLongW );
}

static inline WORD NtUserGetClassWord( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetClassWord );
}

static inline BOOL NtUserGetClientRect( HWND hwnd, RECT *rect )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)rect, NtUserCallHwndParam_GetClientRect );
}

struct get_scroll_info_params
{
    int bar;
    SCROLLINFO *info;
};

static inline BOOL NtUserGetScrollInfo( HWND hwnd, INT bar, SCROLLINFO *info )
{
    struct get_scroll_info_params params = { .bar = bar, .info = info };
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, NtUserCallHwndParam_GetScrollInfo );
}

static inline BOOL NtUserGetWindowInfo( HWND hwnd, WINDOWINFO *info )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)info, NtUserCallHwndParam_GetWindowInfo );
}

static inline LONG NtUserGetWindowLongA( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetWindowLongA );
}

static inline LONG_PTR NtUserGetWindowLongPtrA( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetWindowLongPtrA );
}

static inline LONG_PTR NtUserGetWindowLongPtrW( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetWindowLongPtrW );
}

static inline LONG NtUserGetWindowLongW( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetWindowLongW );
}

static inline BOOL NtUserGetWindowRect( HWND hwnd, RECT *rect )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)rect, NtUserCallHwndParam_GetWindowRect );
}

static inline HWND NtUserGetWindowRelative( HWND hwnd, UINT rel )
{
    return UlongToHandle( NtUserCallHwndParam( hwnd, rel,
                                               NtUserCallHwndParam_GetWindowRelative ));
}

static inline DWORD NtUserGetWindowThread( HWND hwnd, DWORD *process )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)process, NtUserCallHwndParam_GetWindowThread );
}

static inline WORD NtUserGetWindowWord( HWND hwnd, INT offset )
{
    return NtUserCallHwndParam( hwnd, offset, NtUserCallHwndParam_GetWindowWord );
}

static inline BOOL NtUserIsChild( HWND parent, HWND child )
{
    return NtUserCallHwndParam( parent, HandleToUlong(child), NtUserCallHwndParam_IsChild );
}

static inline BOOL NtUserKillSystemTimer( HWND hwnd, UINT_PTR id )
{
    return NtUserCallHwndParam( hwnd, id, NtUserCallHwndParam_KillSystemTimer );
}

struct map_window_points_params
{
    HWND hwnd_to;
    POINT *points;
    UINT count;
};

static inline int NtUserMapWindowPoints( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count )
{
    struct map_window_points_params params;
    params.hwnd_to = hwnd_to;
    params.points = points;
    params.count = count;
    return NtUserCallHwndParam( hwnd_from, (UINT_PTR)&params,
                                NtUserCallHwndParam_MapWindowPoints );
}

static inline BOOL NtUserMirrorRgn( HWND hwnd, HRGN hrgn )
{
    return NtUserCallHwndParam( hwnd, HandleToUlong(hrgn), NtUserCallHwndParam_MirrorRgn );
}

static inline HMONITOR NtUserMonitorFromWindow( HWND hwnd, DWORD flags )
{
    ULONG ret = NtUserCallHwndParam(  hwnd, flags, NtUserCallHwndParam_MonitorFromWindow );
    return UlongToHandle( ret );
}

static inline BOOL NtUserScreenToClient( HWND hwnd, POINT *pt )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)pt, NtUserCallHwndParam_ScreenToClient );
}

static inline void NtUserSetDialogInfo( HWND hwnd, void *info )
{
    NtUserCallHwndParam( hwnd, (UINT_PTR)info, NtUserCallHwndParam_SetDialogInfo );
}

static inline void NtUserSetMDIClientInfo( HWND hwnd, void *info )
{
    NtUserCallHwndParam( hwnd, (UINT_PTR)info, NtUserCallHwndParam_SetMDIClientInfo );
}

static inline BOOL NtUserSetWindowContextHelpId( HWND hwnd, DWORD id )
{
    return NtUserCallHwndParam( hwnd, id, NtUserCallHwndParam_SetWindowContextHelpId );
}

static inline BOOL NtUserShowOwnedPopups( HWND hwnd, BOOL show )
{
    return NtUserCallHwndParam( hwnd, show, NtUserCallHwndParam_ShowOwnedPopups );
}

/* Wine extensions */
W32KAPI BOOL WINAPI __wine_send_input( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput );

#endif /* _NTUSER_ */
