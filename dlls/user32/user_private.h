/*
 * USER private definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_USER_PRIVATE_H
#define __WINE_USER_PRIVATE_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "hidusage.h"
#include "wine/heap.h"

#define GET_WORD(ptr)  (*(const WORD *)(ptr))
#define GET_DWORD(ptr) (*(const DWORD *)(ptr))
#define GET_LONG(ptr) (*(const LONG *)(ptr))

#define WM_SYSTIMER	    0x0118
#define WM_POPUPSYSTEMMENU  0x0313

#define WINE_MOUSE_HANDLE       ((HANDLE)1)
#define WINE_KEYBOARD_HANDLE    ((HANDLE)2)

struct window_surface;

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

typedef struct tagUSER_DRIVER {
    /* keyboard functions */
    BOOL   (CDECL *pActivateKeyboardLayout)(HKL, UINT);
    void   (CDECL *pBeep)(void);
    INT    (CDECL *pGetKeyNameText)(LONG, LPWSTR, INT);
    UINT   (CDECL *pGetKeyboardLayoutList)(INT, HKL *);
    UINT   (CDECL *pMapVirtualKeyEx)(UINT, UINT, HKL);
    BOOL   (CDECL *pRegisterHotKey)(HWND, UINT, UINT);
    INT    (CDECL *pToUnicodeEx)(UINT, UINT, const BYTE *, LPWSTR, int, UINT, HKL);
    void   (CDECL *pUnregisterHotKey)(HWND, UINT, UINT);
    SHORT  (CDECL *pVkKeyScanEx)(WCHAR, HKL);
    /* cursor/icon functions */
    void   (CDECL *pDestroyCursorIcon)(HCURSOR);
    void   (CDECL *pSetCursor)(HCURSOR);
    BOOL   (CDECL *pGetCursorPos)(LPPOINT);
    BOOL   (CDECL *pSetCursorPos)(INT,INT);
    BOOL   (CDECL *pClipCursor)(LPCRECT);
    /* clipboard functions */
    void   (CDECL *pUpdateClipboard)(void);
    /* display modes */
    LONG   (CDECL *pChangeDisplaySettingsEx)(LPCWSTR,LPDEVMODEW,HWND,DWORD,LPVOID);
    BOOL   (CDECL *pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
    BOOL   (CDECL *pEnumDisplaySettingsEx)(LPCWSTR,DWORD,LPDEVMODEW,DWORD);
    BOOL   (CDECL *pGetMonitorInfo)(HMONITOR,MONITORINFO*);
    /* windowing functions */
    BOOL   (CDECL *pCreateDesktopWindow)(HWND);
    BOOL   (CDECL *pCreateWindow)(HWND);
    void   (CDECL *pDestroyWindow)(HWND);
    void   (CDECL *pFlashWindowEx)(FLASHWINFO*);
    void   (CDECL *pGetDC)(HDC,HWND,HWND,const RECT *,const RECT *,DWORD);
    DWORD  (CDECL *pMsgWaitForMultipleObjectsEx)(DWORD,const HANDLE*,DWORD,DWORD,DWORD);
    void   (CDECL *pReleaseDC)(HWND,HDC);
    BOOL   (CDECL *pScrollDC)(HDC,INT,INT,HRGN);
    void   (CDECL *pSetCapture)(HWND,UINT);
    void   (CDECL *pSetFocus)(HWND);
    void   (CDECL *pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
    void   (CDECL *pSetParent)(HWND,HWND,HWND);
    void   (CDECL *pSetWindowRgn)(HWND,HRGN,BOOL);
    void   (CDECL *pSetWindowIcon)(HWND,UINT,HICON);
    void   (CDECL *pSetWindowStyle)(HWND,INT,STYLESTRUCT*);
    void   (CDECL *pSetWindowText)(HWND,LPCWSTR);
    UINT   (CDECL *pShowWindow)(HWND,INT,RECT*,UINT);
    LRESULT (CDECL *pSysCommand)(HWND,WPARAM,LPARAM);
    BOOL    (CDECL *pUpdateLayeredWindow)(HWND,const UPDATELAYEREDWINDOWINFO *,const RECT *);
    LRESULT (CDECL *pWindowMessage)(HWND,UINT,WPARAM,LPARAM);
    BOOL   (CDECL *pWindowPosChanging)(HWND,HWND,UINT,const RECT *,const RECT *,RECT *,struct window_surface**);
    void   (CDECL *pWindowPosChanged)(HWND,HWND,UINT,const RECT *,const RECT *,const RECT *,const RECT *,struct window_surface*);
    /* system parameters */
    BOOL   (CDECL *pSystemParametersInfo)(UINT,UINT,void*,UINT);
    /* thread management */
    void   (CDECL *pThreadDetach)(void);
} USER_DRIVER;

extern const USER_DRIVER *USER_Driver DECLSPEC_HIDDEN;

extern void USER_unload_driver(void) DECLSPEC_HIDDEN;

extern BOOL CDECL nulldrv_EnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lp ) DECLSPEC_HIDDEN;
extern BOOL CDECL nulldrv_GetMonitorInfo( HMONITOR handle, MONITORINFO *info ) DECLSPEC_HIDDEN;

struct received_message_info;

enum user_obj_type
{
    USER_WINDOW = 1,  /* window */
    USER_MENU,        /* menu */
    USER_ACCEL,       /* accelerator */
    USER_ICON,        /* icon or cursor */
    USER_DWP          /* DeferWindowPos structure */
};

struct user_object
{
    HANDLE             handle;
    enum user_obj_type type;
};

#define OBJ_OTHER_PROCESS ((void *)1)  /* returned by get_user_handle_ptr on unknown handles */

HANDLE alloc_user_handle( struct user_object *ptr, enum user_obj_type type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, enum user_obj_type type ) DECLSPEC_HIDDEN;
void release_user_handle_ptr( void *ptr ) DECLSPEC_HIDDEN;
void *free_user_handle( HANDLE handle, enum user_obj_type type ) DECLSPEC_HIDDEN;

/* type of message-sending functions that need special WM_CHAR handling */
enum wm_char_mapping
{
    WMCHAR_MAP_POSTMESSAGE,
    WMCHAR_MAP_SENDMESSAGE,
    WMCHAR_MAP_SENDMESSAGETIMEOUT,
    WMCHAR_MAP_RECVMESSAGE,
    WMCHAR_MAP_DISPATCHMESSAGE,
    WMCHAR_MAP_CALLWINDOWPROC,
    WMCHAR_MAP_COUNT,
    WMCHAR_MAP_NOMAPPING = WMCHAR_MAP_COUNT
};

/* data to store state for A/W mappings of WM_CHAR */
struct wm_char_mapping_data
{
    BYTE lead_byte[WMCHAR_MAP_COUNT];
    MSG  get_msg;
};

/* on windows the buffer capacity is quite large as well, enough to */
/* hold up to 10s of 1kHz mouse rawinput events */
#define RAWINPUT_BUFFER_SIZE (512*1024)

struct rawinput_thread_data
{
    UINT     hw_id;     /* current rawinput message id */
    RAWINPUT buffer[1]; /* rawinput message data buffer */
};

/* this is the structure stored in TEB->Win32ClientInfo */
/* no attempt is made to keep the layout compatible with the Windows one */
struct user_thread_info
{
    HANDLE                        server_queue;           /* Handle to server-side queue */
    DWORD                         wake_mask;              /* Current queue wake mask */
    DWORD                         changed_mask;           /* Current queue changed mask */
    WORD                          recursion_count;        /* SendMessage recursion counter */
    WORD                          message_count;          /* Get/PeekMessage loop counter */
    WORD                          hook_call_depth;        /* Number of recursively called hook procs */
    WORD                          hook_unicode;           /* Is current hook unicode? */
    HHOOK                         hook;                   /* Current hook */
    UINT                          active_hooks;           /* Bitmap of active hooks */
    DPI_AWARENESS                 dpi_awareness;          /* DPI awareness */
    INPUT_MESSAGE_SOURCE          msg_source;             /* Message source for current message */
    struct received_message_info *receive_info;           /* Message being currently received */
    struct wm_char_mapping_data  *wmchar_data;            /* Data for WM_CHAR mappings */
    DWORD                         GetMessageTimeVal;      /* Value for GetMessageTime */
    DWORD                         GetMessagePosVal;       /* Value for GetMessagePos */
    ULONG_PTR                     GetMessageExtraInfoVal; /* Value for GetMessageExtraInfo */
    struct user_key_state_info   *key_state;              /* Cache of global key state */
    HKL                           kbd_layout;             /* Current keyboard layout */
    DWORD                         kbd_layout_id;          /* Current keyboard layout ID */
    HWND                          top_window;             /* Desktop window */
    HWND                          msg_window;             /* HWND_MESSAGE parent window */
    struct rawinput_thread_data  *rawinput;               /* RawInput thread local data / buffer */
};

C_ASSERT( sizeof(struct user_thread_info) <= sizeof(((TEB *)0)->Win32ClientInfo) );

extern INT global_key_state_counter DECLSPEC_HIDDEN;
extern BOOL (WINAPI *imm_register_window)(HWND) DECLSPEC_HIDDEN;
extern void (WINAPI *imm_unregister_window)(HWND) DECLSPEC_HIDDEN;

struct user_key_state_info
{
    UINT                          time;                   /* Time of last key state refresh */
    INT                           counter;                /* Counter to invalidate the key state */
    BYTE                          state[256];             /* State for each key */
};

struct hook_extra_info
{
    HHOOK handle;
    LPARAM lparam;
};

static inline struct user_thread_info *get_user_thread_info(void)
{
    return (struct user_thread_info *)NtCurrentTeb()->Win32ClientInfo;
}

/* check if hwnd is a broadcast magic handle */
static inline BOOL is_broadcast( HWND hwnd )
{
    return (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST);
}

extern HMODULE user32_module DECLSPEC_HIDDEN;

struct dce;
struct tagWND;

struct hardware_msg_data;
extern BOOL rawinput_from_hardware_message(RAWINPUT *rawinput, const struct hardware_msg_data *msg_data);
extern BOOL rawinput_device_get_usages(HANDLE handle, USAGE *usage_page, USAGE *usage);
extern struct rawinput_thread_data *rawinput_thread_data(void);

extern void keyboard_init(void) DECLSPEC_HIDDEN;
extern void create_offscreen_window_surface( const RECT *visible_rect, struct window_surface **surface ) DECLSPEC_HIDDEN;

extern void CLIPBOARD_ReleaseOwner( HWND hwnd ) DECLSPEC_HIDDEN;
extern BOOL FOCUS_MouseActivate( HWND hwnd ) DECLSPEC_HIDDEN;
extern BOOL set_capture_window( HWND hwnd, UINT gui_flags, HWND *prev_ret ) DECLSPEC_HIDDEN;
extern void free_dce( struct dce *dce, HWND hwnd ) DECLSPEC_HIDDEN;
extern void invalidate_dce( struct tagWND *win, const RECT *rect ) DECLSPEC_HIDDEN;
extern HDC get_display_dc(void) DECLSPEC_HIDDEN;
extern void release_display_dc( HDC hdc ) DECLSPEC_HIDDEN;
extern void erase_now( HWND hwnd, UINT rdw_flags ) DECLSPEC_HIDDEN;
extern void move_window_bits( HWND hwnd, struct window_surface *old_surface,
                              struct window_surface *new_surface,
                              const RECT *visible_rect, const RECT *old_visible_rect,
                              const RECT *window_rect, const RECT *valid_rects ) DECLSPEC_HIDDEN;
extern void move_window_bits_parent( HWND hwnd, HWND parent, const RECT *window_rect,
                                     const RECT *valid_rects ) DECLSPEC_HIDDEN;
extern void update_window_state( HWND hwnd ) DECLSPEC_HIDDEN;
extern void wait_graphics_driver_ready(void) DECLSPEC_HIDDEN;
extern void *get_hook_proc( void *proc, const WCHAR *module, HMODULE *free_module ) DECLSPEC_HIDDEN;
extern RECT get_virtual_screen_rect(void) DECLSPEC_HIDDEN;
extern RECT get_primary_monitor_rect(void) DECLSPEC_HIDDEN;
extern LRESULT call_current_hook( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;
extern DWORD get_input_codepage( void ) DECLSPEC_HIDDEN;
extern BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;
extern NTSTATUS send_hardware_message( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput, UINT flags ) DECLSPEC_HIDDEN;
extern LRESULT MSG_SendInternalMessageTimeout( DWORD dest_pid, DWORD dest_tid,
                                               UINT msg, WPARAM wparam, LPARAM lparam,
                                               UINT flags, UINT timeout, PDWORD_PTR res_ptr ) DECLSPEC_HIDDEN;
extern HPEN SYSCOLOR_GetPen( INT index ) DECLSPEC_HIDDEN;
extern HBRUSH SYSCOLOR_Get55AABrush(void) DECLSPEC_HIDDEN;
extern void SYSPARAMS_Init(void) DECLSPEC_HIDDEN;
extern void USER_CheckNotLock(void) DECLSPEC_HIDDEN;
extern BOOL USER_IsExitingThread( DWORD tid ) DECLSPEC_HIDDEN;

extern BOOL USER_SetWindowPos( WINDOWPOS * winpos, int parent_x, int parent_y ) DECLSPEC_HIDDEN;

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );

extern WNDPROC WINPROC_GetProc( WNDPROC proc, BOOL unicode ) DECLSPEC_HIDDEN;
extern WNDPROC WINPROC_AllocProc( WNDPROC func, BOOL unicode ) DECLSPEC_HIDDEN;
extern BOOL WINPROC_IsUnicode( WNDPROC proc, BOOL def_val ) DECLSPEC_HIDDEN;

extern LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg,
                                     enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;

extern INT_PTR WINPROC_CallDlgProcA( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern INT_PTR WINPROC_CallDlgProcW( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern BOOL WINPROC_call_window( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                 LRESULT *result, BOOL unicode, enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;

extern const WCHAR *CLASS_GetVersionedName(const WCHAR *classname, UINT *basename_offset,
        WCHAR *combined, BOOL register_class) DECLSPEC_HIDDEN;

/* message spy definitions */

#define SPY_DISPATCHMESSAGE       0x0100
#define SPY_SENDMESSAGE           0x0101
#define SPY_DEFWNDPROC            0x0102

#define SPY_RESULT_OK             0x0001
#define SPY_RESULT_DEFWND         0x0002

extern const char *SPY_GetClassLongOffsetName( INT offset ) DECLSPEC_HIDDEN;
extern const char *SPY_GetMsgName( UINT msg, HWND hWnd ) DECLSPEC_HIDDEN;
extern const char *SPY_GetVKeyName(WPARAM wParam) DECLSPEC_HIDDEN;
extern void SPY_EnterMessage( INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern void SPY_ExitMessage( INT iFlag, HWND hwnd, UINT msg,
                             LRESULT lReturn, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;

#include "pshpack1.h"

typedef struct
{
    BYTE   bWidth;
    BYTE   bHeight;
    BYTE   bColorCount;
    BYTE   bReserved;
} ICONRESDIR;

typedef struct
{
    WORD   wWidth;
    WORD   wHeight;
} CURSORDIR;

typedef struct
{   union
    { ICONRESDIR icon;
      CURSORDIR  cursor;
    } ResInfo;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1];
} CURSORICONDIR;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse ) DECLSPEC_HIDDEN;
extern BOOL get_icon_size( HICON handle, SIZE *size ) DECLSPEC_HIDDEN;

struct png_funcs
{
    BOOL (CDECL *get_png_info)(const void *png_data, DWORD size, int *width, int *height, int *bpp);
    BITMAPINFO * (CDECL *load_png)(const char *png_data, DWORD *size);
};

/* Mingw's assert() imports MessageBoxA and gets confused by user32 exporting it */
#ifdef __MINGW32__
#undef assert
#define assert(expr) ((void)0)
#endif

extern struct user_api_hook *user_api DECLSPEC_HIDDEN;
LRESULT WINAPI USER_DefDlgProc(HWND, UINT, WPARAM, LPARAM, BOOL) DECLSPEC_HIDDEN;
LRESULT WINAPI USER_ScrollBarProc(HWND, UINT, WPARAM, LPARAM, BOOL) DECLSPEC_HIDDEN;
void WINAPI USER_ScrollBarDraw(HWND, HDC, INT, enum SCROLL_HITTEST,
                               const struct SCROLL_TRACKING_INFO *, BOOL, BOOL, RECT *, INT, INT,
                               INT, BOOL) DECLSPEC_HIDDEN;

#endif /* __WINE_USER_PRIVATE_H */
