/*
 * User definitions
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 2022 Jacek Caban
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

#ifndef __WINE_NTUSER_PRIVATE_H
#define __WINE_NTUSER_PRIVATE_H

#include "ntuser.h"
#include "wine/list.h"

struct dce;
struct tagWND;

struct hardware_msg_data;

struct user_callbacks
{
    BOOL (WINAPI *pImmProcessKey)(HWND, HKL, UINT, LPARAM, DWORD);
    BOOL (WINAPI *pImmTranslateMessage)(HWND, UINT, WPARAM, LPARAM);
    NTSTATUS (WINAPI *pNtWaitForMultipleObjects)(ULONG,const HANDLE*,BOOLEAN,BOOLEAN,const LARGE_INTEGER*);
    void (CDECL *draw_nc_scrollbar)( HWND hwnd, HDC hdc, BOOL draw_horizontal, BOOL draw_vertical );
    void (CDECL *free_win_ptr)( struct tagWND *win );
    void (CDECL *notify_ime)( HWND hwnd, UINT param );
    BOOL (CDECL *post_dde_message)( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, DWORD dest_tid,
                                    DWORD type );
    void (WINAPI *set_standard_scroll_painted)( HWND hwnd, INT bar, BOOL visible );
    BOOL (CDECL *unpack_dde_message)( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                                      void **buffer, size_t size );
    BOOL (WINAPI *register_imm)( HWND hwnd );
    void (WINAPI *unregister_imm)( HWND hwnd );
    NTSTATUS (CDECL *try_finally)( NTSTATUS (CDECL *func)( void *), void *arg,
                                   void (CALLBACK *finally_func)( BOOL ));
};

#define WM_SYSTIMER         0x0118
#define WM_POPUPSYSTEMMENU  0x0313

enum system_timer_id
{
    SYSTEM_TIMER_TRACK_MOUSE = 0xfffa,
    SYSTEM_TIMER_CARET = 0xffff,
};

struct rawinput_thread_data
{
    UINT     hw_id;     /* current rawinput message id */
    RAWINPUT buffer[1]; /* rawinput message data buffer */
};

/* on windows the buffer capacity is quite large as well, enough to */
/* hold up to 10s of 1kHz mouse rawinput events */
#define RAWINPUT_BUFFER_SIZE (512 * 1024)

struct user_object
{
    HANDLE       handle;
    unsigned int type;
};

#define OBJ_OTHER_PROCESS ((void *)1)  /* returned by get_user_handle_ptr on unknown handles */

HANDLE alloc_user_handle( struct user_object *ptr, unsigned int type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void release_user_handle_ptr( void *ptr ) DECLSPEC_HIDDEN;
void *free_user_handle( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;

typedef struct tagWND
{
    struct user_object obj;           /* object header */
    HWND               parent;        /* Window parent */
    HWND               owner;         /* Window owner */
    struct tagCLASS   *class;         /* Window class */
    struct dce        *dce;           /* DCE pointer */
    WNDPROC            winproc;       /* Window procedure */
    DWORD              tid;           /* Owner thread id */
    HINSTANCE          hInstance;     /* Window hInstance (from CreateWindow) */
    RECT               client_rect;   /* Client area rel. to parent client area */
    RECT               window_rect;   /* Whole window rel. to parent client area */
    RECT               visible_rect;  /* Visible part of the whole rect, rel. to parent client area */
    RECT               normal_rect;   /* Normal window rect saved when maximized/minimized */
    POINT              min_pos;       /* Position for minimized window */
    POINT              max_pos;       /* Position for maximized window */
    WCHAR             *text;          /* Window text */
    void              *pScroll;       /* Scroll-bar info */
    DWORD              dwStyle;       /* Window style (from CreateWindow) */
    DWORD              dwExStyle;     /* Extended style (from CreateWindowEx) */
    UINT_PTR           wIDmenu;       /* ID or hmenu (from CreateWindow) */
    DWORD              helpContext;   /* Help context ID */
    UINT               flags;         /* Misc. flags (see below) */
    HMENU              hSysMenu;      /* window's copy of System Menu */
    HICON              hIcon;         /* window's icon */
    HICON              hIconSmall;    /* window's small icon */
    HICON              hIconSmall2;   /* window's secondary small icon, derived from hIcon */
    UINT               dpi;           /* window DPI */
    DPI_AWARENESS      dpi_awareness; /* DPI awareness */
    struct window_surface *surface;   /* Window surface if any */
    struct tagDIALOGINFO *dlgInfo;    /* Dialog additional info (dialogs only) */
    int                pixel_format;  /* Pixel format set by the graphics driver */
    int                cbWndExtra;    /* class cbWndExtra at window creation */
    DWORD_PTR          userdata;      /* User private data */
    DWORD              wExtra[1];     /* Window extra bytes */
} WND;

/* WND flags values */
#define WIN_RESTORE_MAX           0x0001 /* Maximize when restoring */
#define WIN_NEED_SIZE             0x0002 /* Internal WM_SIZE is needed */
#define WIN_NCACTIVATED           0x0004 /* last WM_NCACTIVATE was positive */
#define WIN_ISMDICLIENT           0x0008 /* Window is an MDIClient */
#define WIN_ISUNICODE             0x0010 /* Window is Unicode */
#define WIN_NEEDS_SHOW_OWNEDPOPUP 0x0020 /* WM_SHOWWINDOW:SC_SHOW must be sent in the next ShowOwnedPopup call */
#define WIN_CHILDREN_MOVED        0x0040 /* children may have moved, ignore stored positions */
#define WIN_HAS_IME_WIN           0x0080 /* the window has been registered with imm32 */

#define WND_OTHER_PROCESS ((WND *)1)  /* returned by WIN_GetPtr on unknown window handles */
#define WND_DESKTOP       ((WND *)2)  /* returned by WIN_GetPtr on the desktop window */

/* check if hwnd is a broadcast magic handle */
static inline BOOL is_broadcast( HWND hwnd )
{
    return hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST;
}

#define WM_IME_INTERNAL 0x287
#define IME_INTERNAL_ACTIVATE 0x17
#define IME_INTERNAL_DEACTIVATE 0x18

/* this is the structure stored in TEB->Win32ClientInfo */
/* no attempt is made to keep the layout compatible with the Windows one */
struct user_thread_info
{
    struct ntuser_thread_info     client_info;            /* Data shared with client */
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
    struct user_key_state_info   *key_state;              /* Cache of global key state */
    HKL                           kbd_layout;             /* Current keyboard layout */
    DWORD                         kbd_layout_id;          /* Current keyboard layout ID */
    struct rawinput_thread_data  *rawinput;               /* RawInput thread local data / buffer */
    UINT                          spy_indent;             /* Current spy indent */
};

C_ASSERT( sizeof(struct user_thread_info) <= sizeof(((TEB *)0)->Win32ClientInfo) );

struct user_key_state_info
{
    UINT  time;          /* Time of last key state refresh */
    INT   counter;       /* Counter to invalidate the key state */
    BYTE  state[256];    /* State for each key */
};

struct hook_extra_info
{
    HHOOK handle;
    LPARAM lparam;
};

enum builtin_winprocs
{
    /* dual A/W procs */
    WINPROC_BUTTON = 0,
    WINPROC_COMBO,
    WINPROC_DEFWND,
    WINPROC_DIALOG,
    WINPROC_EDIT,
    WINPROC_LISTBOX,
    WINPROC_MDICLIENT,
    WINPROC_SCROLLBAR,
    WINPROC_STATIC,
    WINPROC_IME,
    /* unicode-only procs */
    WINPROC_DESKTOP,
    WINPROC_ICONTITLE,
    WINPROC_MENU,
    WINPROC_MESSAGE,
    NB_BUILTIN_WINPROCS,
    NB_BUILTIN_AW_WINPROCS = WINPROC_DESKTOP
};

/* FIXME: make it private to class.c */
typedef struct tagWINDOWPROC
{
    WNDPROC        procA;    /* ANSI window proc */
    WNDPROC        procW;    /* Unicode window proc */
} WINDOWPROC;

#define WINPROC_HANDLE (~0u >> 16)
#define BUILTIN_WINPROC(index) ((WNDPROC)(ULONG_PTR)((index) | (WINPROC_HANDLE << 16)))

#define MAX_ATOM_LEN 255

/* Built-in class names (see _Undocumented_Windows_ p.418) */
#define POPUPMENU_CLASS_ATOM MAKEINTATOM(32768)  /* PopupMenu */
#define DESKTOP_CLASS_ATOM   MAKEINTATOM(32769)  /* Desktop */
#define DIALOG_CLASS_ATOM    MAKEINTATOM(32770)  /* Dialog */
#define WINSWITCH_CLASS_ATOM MAKEINTATOM(32771)  /* WinSwitch */
#define ICONTITLE_CLASS_ATOM MAKEINTATOM(32772)  /* IconTitle */

/* message spy definitions */

#define SPY_DISPATCHMESSAGE       0x0100
#define SPY_SENDMESSAGE           0x0101
#define SPY_DEFWNDPROC            0x0102

#define SPY_RESULT_OK             0x0001
#define SPY_RESULT_DEFWND         0x0002

/* info about the message currently being received by the current thread */
struct received_message_info
{
    UINT  type;
    MSG   msg;
    UINT  flags;  /* InSendMessageEx return flags */
    struct received_message_info *prev;
};

extern const char *debugstr_msg_name( UINT msg, HWND hwnd ) DECLSPEC_HIDDEN;
extern const char *debugstr_vkey_name( WPARAM wParam ) DECLSPEC_HIDDEN;
extern void spy_enter_message( INT flag, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;
extern void spy_exit_message( INT flag, HWND hwnd, UINT msg,
                              LRESULT lreturn, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;

/* class.c */
extern HINSTANCE user32_module DECLSPEC_HIDDEN;
WNDPROC alloc_winproc( WNDPROC func, BOOL ansi ) DECLSPEC_HIDDEN;
WINDOWPROC *get_winproc_ptr( WNDPROC handle ) DECLSPEC_HIDDEN;
BOOL is_winproc_unicode( WNDPROC proc, BOOL def_val ) DECLSPEC_HIDDEN;
DWORD get_class_long( HWND hwnd, INT offset, BOOL ansi ) DECLSPEC_HIDDEN;
WNDPROC get_class_winproc( struct tagCLASS *class ) DECLSPEC_HIDDEN;
ULONG_PTR get_class_long_ptr( HWND hwnd, INT offset, BOOL ansi ) DECLSPEC_HIDDEN;
WORD get_class_word( HWND hwnd, INT offset ) DECLSPEC_HIDDEN;
ATOM get_int_atom_value( UNICODE_STRING *name ) DECLSPEC_HIDDEN;
WNDPROC get_winproc( WNDPROC proc, BOOL ansi ) DECLSPEC_HIDDEN;
void get_winproc_params( struct win_proc_params *params ) DECLSPEC_HIDDEN;
struct dce *get_class_dce( struct tagCLASS *class ) DECLSPEC_HIDDEN;
struct dce *set_class_dce( struct tagCLASS *class, struct dce *dce ) DECLSPEC_HIDDEN;
extern void register_builtin_classes(void) DECLSPEC_HIDDEN;

/* cursoricon.c */
HICON alloc_cursoricon_handle( BOOL is_icon ) DECLSPEC_HIDDEN;

/* dce.c */
extern void free_dce( struct dce *dce, HWND hwnd ) DECLSPEC_HIDDEN;
extern void invalidate_dce( WND *win, const RECT *extra_rect ) DECLSPEC_HIDDEN;

/* window.c */
HANDLE alloc_user_handle( struct user_object *ptr, unsigned int type ) DECLSPEC_HIDDEN;
void *free_user_handle( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void release_user_handle_ptr( void *ptr ) DECLSPEC_HIDDEN;
UINT win_set_flags( HWND hwnd, UINT set_mask, UINT clear_mask ) DECLSPEC_HIDDEN;

static inline UINT win_get_flags( HWND hwnd )
{
    return win_set_flags( hwnd, 0, 0 );
}

WND *get_win_ptr( HWND hwnd ) DECLSPEC_HIDDEN;
BOOL is_child( HWND parent, HWND child );
BOOL is_window( HWND hwnd ) DECLSPEC_HIDDEN;

#endif /* __WINE_NTUSER_PRIVATE_H */
