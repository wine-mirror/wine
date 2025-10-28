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

/* avoid including shellscalingapi.h */
typedef enum MONITOR_DPI_TYPE
{
    MDT_EFFECTIVE_DPI   = 0,
    MDT_ANGULAR_DPI     = 1,
    MDT_RAW_DPI         = 2,
    MDT_DEFAULT         = MDT_EFFECTIVE_DPI,
} MONITOR_DPI_TYPE;

typedef NTSTATUS (WINAPI *ntuser_callback)( void *args, ULONG len );
NTSYSAPI NTSTATUS KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len );

struct user_entry
{
    ULONG64 offset;   /* shared user object offset */
    ULONG   tid;      /* owner thread id */
    ULONG   pid;      /* owner process id */
    ULONG64 id;       /* shared user object id */
    union
    {
        struct
        {
            USHORT type;       /* object type (0 if free) */
            USHORT generation; /* generation counter */
        };
        LONG64 uniq;
    };
};

#define MAX_USER_HANDLES ((LAST_USER_HANDLE - FIRST_USER_HANDLE + 1) >> 1)

/* KernelCallbackTable codes, not compatible with Windows.
   All of these functions must live inside user32.dll. Overwatch 2's
   KiUserCallbackDispatcher hook verifies this and prevents the callback from
   running if that check fails. */

#define ALL_USER32_CALLBACKS \
    USER32_CALLBACK_ENTRY(CallDispatchCallback) \
    USER32_CALLBACK_ENTRY(CallEnumDisplayMonitor) \
    USER32_CALLBACK_ENTRY(CallSendAsyncCallback) \
    USER32_CALLBACK_ENTRY(CallWinEventHook) \
    USER32_CALLBACK_ENTRY(CallWinProc) \
    USER32_CALLBACK_ENTRY(CallWindowsHook) \
    USER32_CALLBACK_ENTRY(CopyImage) \
    USER32_CALLBACK_ENTRY(DrawNonClientButton) \
    USER32_CALLBACK_ENTRY(DrawScrollBar) \
    USER32_CALLBACK_ENTRY(DrawText) \
    USER32_CALLBACK_ENTRY(FreeCachedClipboardData) \
    USER32_CALLBACK_ENTRY(ImmProcessKey) \
    USER32_CALLBACK_ENTRY(ImmTranslateMessage) \
    USER32_CALLBACK_ENTRY(InitBuiltinClasses) \
    USER32_CALLBACK_ENTRY(LoadDriver) \
    USER32_CALLBACK_ENTRY(LoadImage) \
    USER32_CALLBACK_ENTRY(LoadSysMenu) \
    USER32_CALLBACK_ENTRY(PostDDEMessage) \
    USER32_CALLBACK_ENTRY(RenderSynthesizedFormat) \
    USER32_CALLBACK_ENTRY(UnpackDDEMessage) \
    USER32_CALLBACK_ENTRY(DragDropEnter) \
    USER32_CALLBACK_ENTRY(DragDropLeave) \
    USER32_CALLBACK_ENTRY(DragDropDrag) \
    USER32_CALLBACK_ENTRY(DragDropDrop) \
    USER32_CALLBACK_ENTRY(DragDropPost)

enum
{
#define USER32_CALLBACK_ENTRY(name) NtUser##name,
    ALL_USER32_CALLBACKS
#undef USER32_CALLBACK_ENTRY
    NtUserCallCount = 256
};

/* NtUserCallDispatchCallback params */
struct dispatch_callback_params
{
    UINT64 callback;
};

static inline NTSTATUS KeUserDispatchCallback( const struct dispatch_callback_params *params, ULONG len,
                                               void **ret_ptr, ULONG *ret_len )
{
    if (!params->callback) return STATUS_ENTRYPOINT_NOT_FOUND;
    return KeUserModeCallback( NtUserCallDispatchCallback, params, len, ret_ptr, ret_len );
}

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
    UINT           dpi_context;       /* DPI awareness context */
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
    UINT dpi_context;
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
struct draw_text_result
{
    int height;
    RECT rect;
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
};

/* NtUserRenderSynthesizedFormat params */
struct render_synthesized_format_params
{
    UINT format;
    UINT from;
};

/* NtUserUnpackDDEMessage params */
struct unpack_dde_message_params
{
    HWND hwnd;
    UINT message;
    WPARAM wparam;
    LPARAM lparam;
    char data[1];
};
struct unpack_dde_message_result
{
    WPARAM wparam;
    LPARAM lparam;
};

/* NtUserDragDropEnter params */
struct format_entry
{
    UINT format;
    UINT size;
    char data[1];
};

/* NtUserDragDropDrag params */
struct drag_drop_drag_params
{
    HWND hwnd;
    POINT point;
    UINT effect;
};

/* NtUserDragDropDrop params */
struct drag_drop_drop_params
{
    HWND hwnd;
};

/* NtUserDragDropPost params */

/* avoid including shlobj.h */
struct drop_files
{
    DWORD pFiles;
    POINT pt;
    BOOL  fNC;
    BOOL  fWide;
};

struct drag_drop_post_params
{
    HWND hwnd;
    UINT drop_size;
    struct drop_files drop;
    BYTE files[];
};

C_ASSERT( sizeof(struct drag_drop_post_params) == offsetof(struct drag_drop_post_params, files[0]) );

/* DPI awareness contexts */
#define MAKE_NTUSER_DPI_CONTEXT( awareness, version, dpi, flags )  ((awareness) | ((version) << 4) | ((dpi) << 8) | (flags))
#define NTUSER_DPI_CONTEXT_GET_AWARENESS( ctx )                    ((ctx) & 0x0f)
#define NTUSER_DPI_CONTEXT_GET_VERSION( ctx )                      (((ctx) & 0xf0) >> 4)
#define NTUSER_DPI_CONTEXT_GET_DPI( ctx )                          ((((ctx) & 0x1ff00) >> 8))
#define NTUSER_DPI_CONTEXT_GET_FLAGS( ctx )                        ((ctx) & 0xfffe0000)
#define NTUSER_DPI_CONTEXT_FLAG_GDISCALED                          0x40000000
#define NTUSER_DPI_CONTEXT_FLAG_PROCESS                            0x80000000
#define NTUSER_DPI_CONTEXT_FLAG_VALID_MASK                         (NTUSER_DPI_CONTEXT_FLAG_PROCESS | NTUSER_DPI_CONTEXT_FLAG_GDISCALED)

#define NTUSER_DPI_CONTEXT_IS_MONITOR_AWARE( ctx )  (NTUSER_DPI_CONTEXT_GET_AWARENESS( ctx ) == DPI_AWARENESS_PER_MONITOR_AWARE)

#define NTUSER_DPI_UNAWARE                MAKE_NTUSER_DPI_CONTEXT( DPI_AWARENESS_UNAWARE, 1, USER_DEFAULT_SCREEN_DPI, 0 )
#define NTUSER_DPI_SYSTEM_AWARE           MAKE_NTUSER_DPI_CONTEXT( DPI_AWARENESS_SYSTEM_AWARE, 1, system_dpi, 0 )
#define NTUSER_DPI_PER_MONITOR_AWARE      MAKE_NTUSER_DPI_CONTEXT( DPI_AWARENESS_PER_MONITOR_AWARE, 1, 0, 0 )
#define NTUSER_DPI_PER_MONITOR_AWARE_V2   MAKE_NTUSER_DPI_CONTEXT( DPI_AWARENESS_PER_MONITOR_AWARE, 2, 0, 0 )
#define NTUSER_DPI_PER_UNAWARE_GDISCALED  MAKE_NTUSER_DPI_CONTEXT( DPI_AWARENESS_UNAWARE, 1, USER_DEFAULT_SCREEN_DPI, NTUSER_DPI_CONTEXT_FLAG_GDISCALED )

/* message spy definitions */
#define SPY_DISPATCHMESSAGE  0x0100
#define SPY_SENDMESSAGE      0x0101
#define SPY_DEFWNDPROC       0x0102

#define SPY_RESULT_OK      0x0001
#define SPY_RESULT_DEFWND  0x0002

/* CreateDesktop wine specific flag */
#define DF_WINE_ROOT_DESKTOP      0x40000000
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
    NtUserDragDropCall        = 0x0307,
    NtUserPostDdeCall         = 0x0308,
    NtUserWintabDriverCall    = 0x0309,
};

/* NtUserWintabDriverCall codes */
enum
{
    NtUserWintabAttach,
    NtUserWintabInfo,
    NtUserWintabInit,
    NtUserWintabPacket,
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

struct post_dde_message_call_params
{
    void *ptr;
    UINT  size;
    DWORD dest_tid;
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

/* NtUserInitializeClientPfnArrays parameter */
enum ntuser_client_procs
{
    NTUSER_WNDPROC_SCROLLBAR,
    NTUSER_WNDPROC_MESSAGE,
    NTUSER_WNDPROC_MENU,
    NTUSER_WNDPROC_DESKTOP,
    NTUSER_WNDPROC_DEFWND,
    NTUSER_WNDPROC_ICONTITLE,
    NTUSER_WNDPROC_UNKNOWN,
    NTUSER_WNDPROC_BUTTON,
    NTUSER_WNDPROC_COMBO,
    NTUSER_WNDPROC_COMBOLBOX,
    NTUSER_WNDPROC_DIALOG,
    NTUSER_WNDPROC_EDIT,
    NTUSER_WNDPROC_LISTBOX,
    NTUSER_WNDPROC_MDICLIENT,
    NTUSER_WNDPROC_STATIC,
    NTUSER_WNDPROC_IME,
    NTUSER_WNDPROC_GHOST,
    NTUSER_NB_PROCS
};

#define NTUSER_NB_WORKERS 11

/* 64-bit pointers are used on all platforms */
typedef WNDPROC ntuser_client_func_ptr[sizeof(UINT64) / sizeof(void *)];

struct ntuser_client_procs_table
{
    ntuser_client_func_ptr A[NTUSER_NB_PROCS];
    ntuser_client_func_ptr W[NTUSER_NB_PROCS];
    ntuser_client_func_ptr workers[NTUSER_NB_WORKERS];
};
#define ALL_NTUSER_CLIENT_PROCS \
  USER_FUNC( ScrollBarWndProc, NTUSER_WNDPROC_SCROLLBAR ) \
  USER_FUNC( MessageWndProc, NTUSER_WNDPROC_MESSAGE ) \
  USER_FUNC( PopupMenuWndProc, NTUSER_WNDPROC_MENU ) \
  USER_FUNC( DesktopWndProc, NTUSER_WNDPROC_DESKTOP ) \
  USER_FUNC( DefWindowProc, NTUSER_WNDPROC_DEFWND ) \
  USER_FUNC( IconTitleWndProc, NTUSER_WNDPROC_ICONTITLE ) \
  USER_FUNC( ButtonWndProc, NTUSER_WNDPROC_BUTTON ) \
  USER_FUNC( ComboWndProc, NTUSER_WNDPROC_COMBO ) \
  USER_FUNC( ComboLBoxWndProc, NTUSER_WNDPROC_COMBOLBOX ) \
  USER_FUNC( DialogWndProc, NTUSER_WNDPROC_DIALOG ) \
  USER_FUNC( EditWndProc, NTUSER_WNDPROC_EDIT ) \
  USER_FUNC( ListBoxWndProc, NTUSER_WNDPROC_LISTBOX ) \
  USER_FUNC( MDIClientWndProc, NTUSER_WNDPROC_MDICLIENT ) \
  USER_FUNC( StaticWndProc, NTUSER_WNDPROC_STATIC ) \
  USER_FUNC( ImeWndProc, NTUSER_WNDPROC_IME ) \
  USER_FUNC( GhostWndProc, NTUSER_WNDPROC_GHOST )


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
    WM_WINE_IME_NOTIFY,
    WM_WINE_WINDOW_STATE_CHANGED,
    WM_WINE_UPDATEWINDOWSTATE,
    WM_WINE_TRACKMOUSEEVENT,
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
    WINE_IME_POST_UPDATE,  /* for the user drivers */
};

/* NtUserImeDriverCall params */
struct ime_driver_call_params
{
    HIMC himc;
    const BYTE *state;
    COMPOSITIONSTRING *compstr;
    BOOL *key_consumed;
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

/* NtUserDragDropCall calls */
enum wine_drag_drop_call
{
    WINE_DRAG_DROP_ENTER,
    WINE_DRAG_DROP_LEAVE,
    WINE_DRAG_DROP_DRAG,
    WINE_DRAG_DROP_DROP,
    WINE_DRAG_DROP_POST,
};

struct ntuser_property_list
{
    UINT64  data;
    ATOM    atom;
    BOOLEAN string;  /* Wine extension */
};

struct ntuser_name_list
{
    ULONG size;
    ULONG count;
    WCHAR strings[1];
};

#define WM_SYSTIMER  0x0118

/* NtUserQueryWindow info classes */
typedef enum _WINDOWINFOCLASS
{
    WindowProcess,
    WindowProcess2, /* FIXME: same as WindowProcess? */
    WindowThread,
    WindowActiveWindow,
    WindowFocusWindow,
    WindowIsHung,
    WindowClientBase,
    WindowIsForegroundThread,
    WindowDefaultImeWindow,
    WindowDefaultInputContext,
} WINDOWINFOCLASS;

/* NtUserGetThreadState info classes */
typedef enum _USERTHREADSTATECLASS
{
    UserThreadStateFocusWindow,
    UserThreadStateActiveWindow,
    UserThreadStateCaptureWindow,
    UserThreadStateDefaultImeWindow,
    UserThreadStateDefaultInputContext,
    UserThreadStateInputState,
    UserThreadStateCursor,
    UserThreadStateExtraInfo,
    UserThreadStateInSendMessage,
    UserThreadStateMessageTime,
    UserThreadStateIsForeground
} USERTHREADSTATECLASS;

W32KAPI HKL     WINAPI NtUserActivateKeyboardLayout( HKL layout, UINT flags );
W32KAPI BOOL    WINAPI NtUserAddClipboardFormatListener( HWND hwnd );
W32KAPI ULONG   WINAPI NtUserAlterWindowStyle( HWND hwnd, UINT mask, UINT style );
W32KAPI UINT    WINAPI NtUserArrangeIconicWindows( HWND parent );
W32KAPI UINT    WINAPI NtUserAssociateInputContext( HWND hwnd, HIMC ctx, ULONG flags );
W32KAPI BOOL    WINAPI NtUserAttachThreadInput( DWORD from, DWORD to, BOOL attach );
W32KAPI HDWP    WINAPI NtUserBeginDeferWindowPos( INT count );
W32KAPI HDC     WINAPI NtUserBeginPaint( HWND hwnd, PAINTSTRUCT *ps );
W32KAPI NTSTATUS WINAPI NtUserBuildHimcList( UINT thread_id, UINT count, HIMC *buffer, UINT *size );
W32KAPI NTSTATUS WINAPI NtUserBuildHwndList( HDESK desktop, HWND hwnd, BOOL children, BOOL non_immersive,
                                             ULONG thread_id, ULONG count, HWND *buffer, ULONG *size );
W32KAPI NTSTATUS WINAPI NtUserBuildNameList( HWINSTA winsta, ULONG size, struct ntuser_name_list *buffer, ULONG *ret_size );
W32KAPI NTSTATUS WINAPI NtUserBuildPropList( HWND hwnd, ULONG count, struct ntuser_property_list *buffer, ULONG *ret_count );
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
W32KAPI HMENU   WINAPI NtUserCreateMenu(void);
W32KAPI HMENU   WINAPI NtUserCreatePopupMenu(void);
W32KAPI HWND    WINAPI NtUserCreateWindowEx( DWORD ex_style, UNICODE_STRING *class_name,
                                             UNICODE_STRING *version, UNICODE_STRING *window_name,
                                             DWORD style, INT x, INT y, INT cx, INT cy,
                                             HWND parent, HMENU menu, HINSTANCE instance, void *params,
                                             DWORD flags, HINSTANCE client_instance, const WCHAR *class, BOOL ansi );
W32KAPI HWINSTA WINAPI NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK mask, ULONG arg3,
                                                  ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 );
W32KAPI HDWP    WINAPI NtUserDeferWindowPosAndBand( HDWP hdwp, HWND hwnd, HWND after, INT x, INT y,
                                                    INT cx, INT cy, UINT flags, UINT unk1, UINT unk2 );
W32KAPI BOOL    WINAPI NtUserDeleteMenu( HMENU menu, UINT id, UINT flags );
W32KAPI BOOL    WINAPI NtUserDestroyAcceleratorTable( HACCEL handle );
W32KAPI BOOL    WINAPI NtUserDestroyCaret(void);
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
W32KAPI BOOL    WINAPI NtUserDrawMenuBar( HWND hwnd );
W32KAPI DWORD   WINAPI NtUserDrawMenuBarTemp( HWND hwnd, HDC hdc, RECT *rect, HMENU handle, HFONT font );
W32KAPI BOOL    WINAPI NtUserEmptyClipboard(void);
W32KAPI BOOL    WINAPI NtUserEnableMenuItem( HMENU handle, UINT id, UINT flags );
W32KAPI BOOL    WINAPI NtUserEnableMouseInPointer( BOOL );
W32KAPI BOOL    WINAPI NtUserEnableMouseInPointerForThread(void);
W32KAPI BOOL    WINAPI NtUserEnableScrollBar( HWND hwnd, UINT bar, UINT flags );
W32KAPI BOOL    WINAPI NtUserEnableWindow( HWND hwnd, BOOL enable );
W32KAPI BOOL    WINAPI NtUserEndDeferWindowPosEx( HDWP hdwp, BOOL async );
W32KAPI BOOL    WINAPI NtUserEndMenu(void);
W32KAPI BOOL    WINAPI NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps );
W32KAPI UINT    WINAPI NtUserEnumClipboardFormats( UINT format );
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
W32KAPI BOOL    WINAPI NtUserGetClipCursor( RECT *rect );
W32KAPI HANDLE  WINAPI NtUserGetClipboardData( UINT format, struct get_clipboard_params *params );
W32KAPI INT     WINAPI NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen );
W32KAPI HWND    WINAPI NtUserGetClipboardOwner(void);
W32KAPI DWORD   WINAPI NtUserGetClipboardSequenceNumber(void);
W32KAPI HWND    WINAPI NtUserGetClipboardViewer(void);
W32KAPI BOOL    WINAPI NtUserGetCurrentInputMessageSource( INPUT_MESSAGE_SOURCE *source );
W32KAPI HCURSOR WINAPI NtUserGetCursor(void);
W32KAPI HCURSOR WINAPI NtUserGetCursorFrameInfo( HCURSOR hCursor, DWORD istep, DWORD *rate_jiffies,
                                                 DWORD *num_steps );
W32KAPI BOOL    WINAPI NtUserGetCursorInfo( CURSORINFO *info );
W32KAPI BOOL    WINAPI NtUserGetCursorPos( POINT *pt );
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
W32KAPI BOOL    WINAPI NtUserGetProcessDefaultLayout( ULONG *layout );
W32KAPI ULONG   WINAPI NtUserGetProcessDpiAwarenessContext( HANDLE process );
W32KAPI HWINSTA WINAPI NtUserGetProcessWindowStation(void);
W32KAPI HANDLE  WINAPI NtUserGetProp( HWND hwnd, const WCHAR *str );
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
W32KAPI ULONG_PTR WINAPI NtUserGetThreadState( USERTHREADSTATECLASS cls );
W32KAPI BOOL    WINAPI NtUserGetTitleBarInfo( HWND hwnd, TITLEBARINFO *info );
W32KAPI INT     WINAPI NtUserGetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase );
W32KAPI BOOL    WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size );
W32KAPI BOOL    WINAPI NtUserGetUpdateRect( HWND hwnd, RECT *rect, BOOL erase );
W32KAPI DWORD   WINAPI NtUserGetWindowContextHelpId( HWND hwnd );
W32KAPI HDC     WINAPI NtUserGetWindowDC( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserGetWindowDisplayAffinity( HWND hwnd, DWORD *affinity );
W32KAPI BOOL    WINAPI NtUserGetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *placement );
W32KAPI int     WINAPI NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk );
W32KAPI BOOL    WINAPI NtUserHideCaret( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserHiliteMenuItem( HWND hwnd, HMENU handle, UINT item, UINT hilite );
W32KAPI NTSTATUS WINAPI NtUserInitializeClientPfnArrays( const ntuser_client_func_ptr *client_procsA,
                                                         const ntuser_client_func_ptr *client_procsW,
                                                         const ntuser_client_func_ptr *client_workers, HINSTANCE user_module );
W32KAPI HICON   WINAPI NtUserInternalGetWindowIcon( HWND hwnd, UINT type );
W32KAPI INT     WINAPI NtUserInternalGetWindowText( HWND hwnd, WCHAR *text, INT count );
W32KAPI BOOL    WINAPI NtUserIsChildWindowDpiMessageEnabled( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserIsClipboardFormatAvailable( UINT format );
W32KAPI BOOL    WINAPI NtUserIsMouseInPointerEnabled(void);
W32KAPI BOOL    WINAPI NtUserInvalidateRect( HWND hwnd, const RECT *rect, BOOL erase );
W32KAPI BOOL    WINAPI NtUserInvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase );
W32KAPI BOOL    WINAPI NtUserKillSystemTimer( HWND hwnd, UINT_PTR id );
W32KAPI BOOL    WINAPI NtUserKillTimer( HWND hwnd, UINT_PTR id );
W32KAPI BOOL    WINAPI NtUserLockWindowUpdate( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserLogicalToPerMonitorDPIPhysicalPoint( HWND hwnd, POINT *pt );
W32KAPI UINT    WINAPI NtUserMapVirtualKeyEx( UINT code, UINT type, HKL layout );
W32KAPI INT     WINAPI NtUserMenuItemFromPoint( HWND hwnd, HMENU handle, int x, int y );
W32KAPI BOOL    WINAPI NtUserMessageBeep( UINT type );
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
W32KAPI BOOL    WINAPI NtUserPostQuitMessage( INT exit_code );
W32KAPI BOOL    WINAPI NtUserPostThreadMessage( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam );
W32KAPI BOOL    WINAPI NtUserPrintWindow( HWND hwnd, HDC hdc, UINT flags );
W32KAPI LONG    WINAPI NtUserQueryDisplayConfig( UINT32 flags, UINT32 *paths_count, DISPLAYCONFIG_PATH_INFO *paths,
                                                 UINT32 *modes_count, DISPLAYCONFIG_MODE_INFO *modes,
                                                 DISPLAYCONFIG_TOPOLOGY_ID *topology_id);
W32KAPI UINT_PTR WINAPI NtUserQueryInputContext( HIMC handle, UINT attr );
W32KAPI HANDLE  WINAPI NtUserQueryWindow( HWND hwnd, WINDOWINFOCLASS cls );
W32KAPI HWND    WINAPI NtUserRealChildWindowFromPoint( HWND parent, LONG x, LONG y );
W32KAPI UINT    WINAPI NtUserRealizePalette( HDC hdc );
W32KAPI BOOL    WINAPI NtUserRedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags );
W32KAPI ATOM    WINAPI NtUserRegisterClassExWOW( const WNDCLASSEXW *wc, UNICODE_STRING *name, UNICODE_STRING *version,
                                                 struct client_menu_name *client_menu_name, DWORD fnid, DWORD flags,
                                                 DWORD *wow );
W32KAPI BOOL    WINAPI NtUserRegisterHotKey( HWND hwnd, INT id, UINT modifiers, UINT vk );
W32KAPI BOOL    WINAPI NtUserRegisterRawInputDevices( const RAWINPUTDEVICE *devices, UINT device_count, UINT size );
W32KAPI BOOL    WINAPI NtUserRegisterTouchPadCapable( BOOL capable );
W32KAPI ATOM    WINAPI NtUserRegisterWindowMessage( UNICODE_STRING *name );
W32KAPI BOOL    WINAPI NtUserReleaseCapture(void);
W32KAPI INT     WINAPI NtUserReleaseDC( HWND hwnd, HDC hdc );
W32KAPI BOOL    WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserRemoveMenu( HMENU menu, UINT id, UINT flags );
W32KAPI HANDLE  WINAPI NtUserRemoveProp( HWND hwnd, const WCHAR *str );
W32KAPI BOOL    WINAPI NtUserReplyMessage( LRESULT result );
W32KAPI INT     WINAPI NtUserScheduleDispatchNotification( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserScrollDC( HDC hdc, INT dx, INT dy, const RECT *scroll, const RECT *clip,
                                       HRGN ret_update_rgn, RECT *update_rect );
W32KAPI INT     WINAPI NtUserScrollWindowEx( HWND hwnd, INT dx, INT dy, const RECT *rect,
                                             const RECT *clip_rect, HRGN update_rgn,
                                             RECT *update_rect, UINT flags );
W32KAPI HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE palette, WORD force_background );
W32KAPI UINT     WINAPI NtUserSendInput( UINT count, INPUT *inputs, int size );
W32KAPI HWND     WINAPI NtUserSetActiveWindow( HWND hwnd );
W32KAPI BOOL     WINAPI NtUserSetAdditionalForegroundBoostProcesses( HWND hwnd, DWORD count, HANDLE *handles );
W32KAPI HWND     WINAPI NtUserSetCapture( HWND hwnd );
W32KAPI BOOL     WINAPI NtUserSetCaretBlinkTime( unsigned int time );
W32KAPI BOOL     WINAPI NtUserSetCaretPos( INT x, INT y );
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
W32KAPI BOOL    WINAPI NtUserSetForegroundWindow( HWND hwnd );
W32KAPI void    WINAPI NtUserSetInternalWindowPos( HWND hwnd, UINT cmd, RECT *rect, POINT *pt );
W32KAPI BOOL    WINAPI NtUserSetKeyboardState( BYTE *state );
W32KAPI BOOL    WINAPI NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags );
W32KAPI BOOL    WINAPI NtUserSetMenu( HWND hwnd, HMENU menu );
W32KAPI BOOL    WINAPI NtUserSetMenuContextHelpId( HMENU handle, DWORD id );
W32KAPI BOOL    WINAPI NtUserSetMenuDefaultItem( HMENU handle, UINT item, UINT bypos );
W32KAPI BOOL    WINAPI NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len );
W32KAPI HWND    WINAPI NtUserSetParent( HWND hwnd, HWND parent );
W32KAPI BOOL    WINAPI NtUserSetProcessDefaultLayout( ULONG layout );
W32KAPI BOOL    WINAPI NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown );
W32KAPI BOOL    WINAPI NtUserSetProcessWindowStation( HWINSTA handle );
W32KAPI HWND    WINAPI NtUserSetProgmanWindow( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle );
W32KAPI INT     WINAPI NtUserSetScrollInfo( HWND hwnd, INT bar, const SCROLLINFO *info, BOOL redraw );
W32KAPI BOOL    WINAPI NtUserSetShellWindowEx( HWND shell, HWND list_view );
W32KAPI BOOL    WINAPI NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values );
W32KAPI BOOL    WINAPI NtUserSetSystemMenu( HWND hwnd, HMENU menu );
W32KAPI UINT_PTR WINAPI NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout );
W32KAPI HWND    WINAPI NtUserSetTaskmanWindow( HWND hwnd );
W32KAPI BOOL    WINAPI NtUserSetThreadDesktop( HDESK handle );
W32KAPI UINT_PTR WINAPI NtUserSetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc, ULONG tolerance );
W32KAPI BOOL    WINAPI NtUserSetWindowContextHelpId( HWND hwnd, DWORD id );
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
W32KAPI BOOL    WINAPI NtUserShowOwnedPopups( HWND owner, BOOL show );
W32KAPI BOOL    WINAPI NtUserShowScrollBar( HWND hwnd, INT bar, BOOL show );
W32KAPI BOOL    WINAPI NtUserShowWindow( HWND hwnd, INT cmd );
W32KAPI BOOL    WINAPI NtUserShowWindowAsync( HWND hwnd, INT cmd );
W32KAPI BOOL    WINAPI NtUserSystemParametersInfo( UINT action, UINT val, void *ptr, UINT winini );
W32KAPI BOOL    WINAPI NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi );
W32KAPI BOOL    WINAPI NtUserSwitchDesktop( HDESK desktop );
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
W32KAPI BOOL    WINAPI NtUserUnhookWindowsHook( INT id, HOOKPROC proc );
W32KAPI BOOL    WINAPI NtUserUnhookWindowsHookEx( HHOOK handle );
W32KAPI BOOL    WINAPI NtUserUnregisterClass( UNICODE_STRING *name, HINSTANCE instance,
                                              struct client_menu_name *client_menu_name );
W32KAPI BOOL    WINAPI NtUserUnregisterHotKey( HWND hwnd, INT id );
W32KAPI BOOL    WINAPI NtUserUpdateInputContext( HIMC handle, UINT attr, UINT_PTR value );
W32KAPI BOOL    WINAPI NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                                  HDC hdc_src, const POINT *pts_src, COLORREF key,
                                                  const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty );
W32KAPI BOOL    WINAPI NtUserValidateRect( HWND hwnd, const RECT *rect );
W32KAPI BOOL    WINAPI NtUserValidateRgn( HWND hwnd, HRGN hrgn );
W32KAPI WORD    WINAPI NtUserVkKeyScanEx( WCHAR chr, HKL layout );
W32KAPI DWORD   WINAPI NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow );
W32KAPI BOOL    WINAPI NtUserWaitMessage(void);
W32KAPI HWND    WINAPI NtUserWindowFromDC( HDC hdc );
W32KAPI HWND    WINAPI NtUserWindowFromPoint( LONG x, LONG y );

/* NtUserCallNoParam codes, not compatible with Windows */
enum
{
    NtUserCallNoParam_GetDesktopWindow,
    NtUserCallNoParam_GetDialogBaseUnits,
    NtUserCallNoParam_GetLastInputTime,
    NtUserCallNoParam_GetProgmanWindow,
    NtUserCallNoParam_GetShellWindow,
    NtUserCallNoParam_GetTaskmanWindow,
    NtUserCallNoParam_DisplayModeChanged,
    /* temporary exports */
    NtUserExitingThread,
    NtUserThreadDetach,
};

static inline HWND NtUserGetDesktopWindow(void)
{
    return UlongToHandle( NtUserCallNoParam( NtUserCallNoParam_GetDesktopWindow ));
}

static inline DWORD NtUserGetDialogBaseUnits(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_GetDialogBaseUnits );
};

static inline DWORD NtUserGetLastInputTime(void)
{
    return NtUserCallNoParam( NtUserCallNoParam_GetLastInputTime );
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

/* NtUserCallOneParam codes, not compatible with Windows */
enum
{
    NtUserCallOneParam_CreateCursorIcon,
    NtUserCallOneParam_EnableDC,
    NtUserCallOneParam_EnableThunkLock,
    NtUserCallOneParam_GetIconParam,
    NtUserCallOneParam_GetMenuItemCount,
    NtUserCallOneParam_GetPrimaryMonitorRect,
    NtUserCallOneParam_GetSysColor,
    NtUserCallOneParam_GetSysColorBrush,
    NtUserCallOneParam_GetSysColorPen,
    NtUserCallOneParam_GetSystemMetrics,
    NtUserCallOneParam_GetVirtualScreenRect,
    NtUserCallOneParam_SetKeyboardAutoRepeat,
    NtUserCallOneParam_SetThreadDpiAwarenessContext,
    NtUserCallOneParam_D3DKMTOpenAdapterFromGdiDisplayName,
    NtUserCallOneParam_GetAsyncKeyboardState,
    /* temporary exports */
    NtUserGetDeskPattern,
};

static inline HICON NtUserCreateCursorIcon( BOOL is_icon )
{
    return UlongToHandle( NtUserCallOneParam( is_icon, NtUserCallOneParam_CreateCursorIcon ));
}

static inline WORD NtUserEnableDC( HDC hdc )
{
    return NtUserCallOneParam( HandleToUlong(hdc), NtUserCallOneParam_EnableDC );
}

struct thunk_lock_params
{
    struct dispatch_callback_params dispatch;
    BOOL restore;
    DWORD locks;
};

static inline void NtUserEnableThunkLock( ntuser_callback thunk_lock_callback )
{
    NtUserCallOneParam( (UINT_PTR)thunk_lock_callback, NtUserCallOneParam_EnableThunkLock );
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

static inline UINT NtUserSetThreadDpiAwarenessContext( UINT context )
{
    return NtUserCallOneParam( context, NtUserCallOneParam_SetThreadDpiAwarenessContext );
}

typedef struct _D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME;

static inline NTSTATUS NtUserD3DKMTOpenAdapterFromGdiDisplayName( D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc )
{
    return NtUserCallOneParam( (UINT_PTR)desc, NtUserCallOneParam_D3DKMTOpenAdapterFromGdiDisplayName );
}

static inline BOOL NtUserGetAsyncKeyboardState( BYTE state[256] )
{
    return NtUserCallOneParam( (UINT_PTR)state, NtUserCallOneParam_GetAsyncKeyboardState );
}

/* NtUserCallTwoParam codes, not compatible with Windows */
enum
{
    NtUserCallTwoParam_GetDialogProc,
    NtUserCallTwoParam_GetMenuInfo,
    NtUserCallTwoParam_GetMonitorInfo,
    NtUserCallTwoParam_GetSystemMetricsForDpi,
    NtUserCallTwoParam_MonitorFromRect,
    NtUserCallTwoParam_SetIconParam,
    NtUserCallTwoParam_SetIMECompositionRect,
    NtUserCallTwoParam_AdjustWindowRect,
    NtUserCallTwoParam_GetVirtualScreenRect,
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

struct free_icon_params
{
    struct dispatch_callback_params dispatch;
    UINT64 param;
};

static inline UINT_PTR NtUserSetIconParam( HICON icon, ULONG_PTR param, ntuser_callback callback )
{
    struct free_icon_params params = {.dispatch = {.callback = (UINT_PTR)callback}, .param = param};
    return NtUserCallTwoParam( HandleToUlong(icon), (UINT_PTR)&params, NtUserCallTwoParam_SetIconParam );
}

struct adjust_window_rect_params
{
    DWORD style;
    DWORD ex_style;
    BOOL menu;
    UINT dpi;
};

static inline BOOL NtUserAdjustWindowRect( RECT *rect, DWORD style, BOOL menu, DWORD ex_style, UINT dpi )
{
    struct adjust_window_rect_params params =
    {
        .style = style,
        .ex_style = ex_style,
        .menu = menu,
        .dpi = dpi,
    };
    return NtUserCallTwoParam( (ULONG_PTR)rect, (ULONG_PTR)&params, NtUserCallTwoParam_AdjustWindowRect );
}

static inline RECT NtUserGetVirtualScreenRect( MONITOR_DPI_TYPE type )
{
    RECT virtual;
    NtUserCallTwoParam( (UINT_PTR)&virtual, type, NtUserCallTwoParam_GetVirtualScreenRect );
    return virtual;
}

/* NtUserCallHwnd codes, not compatible with Windows */
enum
{
    NtUserCallHwnd_ActivateOtherWindow,
    NtUserCallHwnd_GetDialogInfo,
    NtUserCallHwnd_GetDpiForWindow,
    NtUserCallHwnd_GetLastActivePopup,
    NtUserCallHwnd_GetMDIClientInfo,
    NtUserCallHwnd_GetParent,
    NtUserCallHwnd_GetWindowDpiAwarenessContext,
    NtUserCallHwnd_GetWindowInputContext,
    NtUserCallHwnd_GetWindowSysSubMenu,
    NtUserCallHwnd_GetWindowTextLength,
    NtUserCallHwnd_IsWindow,
    NtUserCallHwnd_IsWindowEnabled,
    NtUserCallHwnd_IsWindowUnicode,
    NtUserCallHwnd_IsWindowVisible,
    /* temporary exports */
    NtUserGetFullWindowHandle,
    NtUserIsCurrentProcessWindow,
    NtUserIsCurrentThreadWindow,
};

static inline void NtUserActivateOtherWindow( HWND hwnd )
{
    NtUserCallHwnd( hwnd, NtUserCallHwnd_ActivateOtherWindow );
}

static inline void *NtUserGetDialogInfo( HWND hwnd )
{
    return (void *)NtUserCallHwnd( hwnd, NtUserCallHwnd_GetDialogInfo );
}

static inline UINT NtUserGetDpiForWindow( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_GetDpiForWindow );
}

static inline HWND NtUserGetLastActivePopup( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_GetLastActivePopup ));
}

static inline void *NtUserGetMDIClientInfo( HWND hwnd )
{
    return (void *)NtUserCallHwnd( hwnd, NtUserCallHwnd_GetMDIClientInfo );
}

static inline HWND NtUserGetParent( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserCallHwnd_GetParent ));
}

static inline UINT NtUserGetWindowDpiAwarenessContext( HWND hwnd )
{
    return NtUserCallHwnd( hwnd, NtUserCallHwnd_GetWindowDpiAwarenessContext );
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

/* NtUserCallHwndParam codes, not compatible with Windows */
enum
{
    NtUserCallHwndParam_ClientToScreen,
    NtUserCallHwndParam_GetChildRect,
    NtUserCallHwndParam_GetClassLongA,
    NtUserCallHwndParam_GetClassLongW,
    NtUserCallHwndParam_GetClassLongPtrA,
    NtUserCallHwndParam_GetClassLongPtrW,
    NtUserCallHwndParam_GetClassWord,
    NtUserCallHwndParam_GetScrollInfo,
    NtUserCallHwndParam_GetWindowInfo,
    NtUserCallHwndParam_GetWindowLongA,
    NtUserCallHwndParam_GetWindowLongW,
    NtUserCallHwndParam_GetWindowLongPtrA,
    NtUserCallHwndParam_GetWindowLongPtrW,
    NtUserCallHwndParam_GetWindowRects,
    NtUserCallHwndParam_GetWindowRelative,
    NtUserCallHwndParam_GetWindowThread,
    NtUserCallHwndParam_GetWindowWord,
    NtUserCallHwndParam_IsChild,
    NtUserCallHwndParam_MapWindowPoints,
    NtUserCallHwndParam_MirrorRgn,
    NtUserCallHwndParam_MonitorFromWindow,
    NtUserCallHwndParam_ScreenToClient,
    NtUserCallHwndParam_SetDialogInfo,
    NtUserCallHwndParam_SetMDIClientInfo,
    NtUserCallHwndParam_SendHardwareInput,
    NtUserCallHwndParam_ExposeWindowSurface,
    NtUserCallHwndParam_GetWinMonitorDpi,
    NtUserCallHwndParam_SetRawWindowPos,
};

struct get_window_rects_params
{
    RECT *rect;
    BOOL client;
    UINT dpi;
};

static inline BOOL NtUserClientToScreen( HWND hwnd, POINT *pt )
{
    return NtUserCallHwndParam( hwnd, (UINT_PTR)pt, NtUserCallHwndParam_ClientToScreen );
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

static inline BOOL NtUserGetClientRect( HWND hwnd, RECT *rect, UINT dpi )
{
    struct get_window_rects_params params = {.rect = rect, .client = TRUE, .dpi = dpi};
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, NtUserCallHwndParam_GetWindowRects );
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

static inline BOOL NtUserGetWindowRect( HWND hwnd, RECT *rect, UINT dpi )
{
    struct get_window_rects_params params = {.rect = rect, .client = FALSE, .dpi = dpi};
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, NtUserCallHwndParam_GetWindowRects );
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

struct map_window_points_params
{
    HWND hwnd_to;
    POINT *points;
    UINT count;
    UINT dpi;
};

static inline int NtUserMapWindowPoints( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count, UINT dpi )
{
    struct map_window_points_params params;
    params.hwnd_to = hwnd_to;
    params.points = points;
    params.count = count;
    params.dpi = dpi;
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

struct hid_input
{
    UINT device;
    UINT usage;
    UINT count;
    UINT length;
};

struct hid_packet
{
    struct hid_input head;
    BYTE data[];
};

C_ASSERT(sizeof(struct hid_packet) == offsetof(struct hid_packet, data[0]));

struct send_hardware_input_params
{
    UINT flags;
    const INPUT *input;
    LPARAM lparam;  /* struct hid_packet pointer for WM_INPUT* messages */
};

static inline BOOL NtUserSendHardwareInput( HWND hwnd, UINT flags, const INPUT *input, LPARAM lparam )
{
    struct send_hardware_input_params params = {.flags = flags, .input = input, .lparam = lparam};
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, NtUserCallHwndParam_SendHardwareInput );
}

struct expose_window_surface_params
{
    UINT flags;
    BOOL whole;
    RECT rect;
    UINT dpi;
};

static inline BOOL NtUserExposeWindowSurface( HWND hwnd, UINT flags, const RECT *rect, UINT dpi )
{
    struct expose_window_surface_params params = {.flags = flags, .whole = !rect, .dpi = dpi};
    if (rect) params.rect = *rect;
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, NtUserCallHwndParam_ExposeWindowSurface );
}

static inline UINT NtUserGetWinMonitorDpi( HWND hwnd, MONITOR_DPI_TYPE type )
{
    return NtUserCallHwndParam( hwnd, type, NtUserCallHwndParam_GetWinMonitorDpi );
}

struct set_raw_window_pos_params
{
    RECT rect;
    UINT flags;
    BOOL internal;
};

static inline BOOL NtUserSetRawWindowPos( HWND hwnd, RECT rect, UINT flags, BOOL internal )
{
    struct set_raw_window_pos_params params = {.rect = rect, .flags = flags, .internal = internal};
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&params, NtUserCallHwndParam_SetRawWindowPos );
}

#endif /* _NTUSER_ */
