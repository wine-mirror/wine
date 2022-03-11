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

struct user_callbacks
{
    HANDLE (WINAPI *pCopyImage)( HANDLE, UINT, INT, INT, UINT );
    BOOL (WINAPI *pPostMessageW)( HWND, UINT, WPARAM, LPARAM );
    BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    UINT (WINAPI *pSendInput)( UINT count, INPUT *inputs, int size );
    LRESULT (WINAPI *pSendMessageTimeoutW)( HWND, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR );
    LRESULT (WINAPI *pSendMessageW)( HWND, UINT, WPARAM, LPARAM );
    BOOL (WINAPI *pSendNotifyMessageW)( HWND, UINT, WPARAM, LPARAM );
    BOOL (WINAPI *pSetWindowPos)( HWND, HWND, INT, INT, INT, INT, UINT );
    DWORD (WINAPI *pWaitForInputIdle)( HANDLE, DWORD );
    HWND (WINAPI *pWindowFromDC)( HDC );
    void (WINAPI *free_dce)( struct dce *dce, HWND hwnd );
    void (CDECL *notify_ime)( HWND hwnd, UINT param );
    void (CDECL *register_builtin_classes)(void);
    LRESULT (WINAPI *send_ll_message)( DWORD, DWORD, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR );
    void (CDECL *set_user_driver)( void *, UINT );
    BOOL (CDECL *set_window_pos)( HWND hwnd, HWND insert_after, UINT swp_flags,
                                  const RECT *window_rect, const RECT *client_rect,
                                  const RECT *valid_rects );
};

struct user_object
{
    HANDLE       handle;
    unsigned int type;
};

#define OBJ_OTHER_PROCESS ((void *)1)  /* returned by get_user_handle_ptr on unknown handles */

HANDLE alloc_user_handle( struct user_object *ptr, unsigned int type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void set_user_handle_ptr( HANDLE handle, struct user_object *ptr ) DECLSPEC_HIDDEN;
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

WND *next_thread_window_ptr( HWND *hwnd );

#define WM_IME_INTERNAL 0x287
#define IME_INTERNAL_ACTIVATE 0x17
#define IME_INTERNAL_DEACTIVATE 0x18

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

typedef struct tagCLASS
{
    struct list      entry;         /* Entry in class list */
    UINT             style;         /* Class style */
    BOOL             local;         /* Local class? */
    WNDPROC          winproc;       /* Window procedure */
    INT              cbClsExtra;    /* Class extra bytes */
    INT              cbWndExtra;    /* Window extra bytes */
    struct client_menu_name menu_name; /* Default menu name */
    struct dce      *dce;           /* Opaque pointer to class DCE */
    UINT_PTR         instance;      /* Module that created the task */
    HICON            hIcon;         /* Default icon */
    HICON            hIconSm;       /* Default small icon */
    HICON            hIconSmIntern; /* Internal small icon, derived from hIcon */
    HCURSOR          hCursor;       /* Default cursor */
    HBRUSH           hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
    WCHAR            name[MAX_ATOM_LEN + 1];
    WCHAR           *basename;      /* Base name for redirected classes, pointer within 'name'. */
} CLASS;

/* class.c */
WNDPROC alloc_winproc( WNDPROC func, BOOL ansi ) DECLSPEC_HIDDEN;
WINDOWPROC *get_winproc_ptr( WNDPROC handle ) DECLSPEC_HIDDEN;
DWORD get_class_long( HWND hwnd, INT offset, BOOL ansi ) DECLSPEC_HIDDEN;
ULONG_PTR get_class_long_ptr( HWND hwnd, INT offset, BOOL ansi ) DECLSPEC_HIDDEN;
WORD get_class_word( HWND hwnd, INT offset ) DECLSPEC_HIDDEN;
ATOM get_int_atom_value( UNICODE_STRING *name ) DECLSPEC_HIDDEN;
WNDPROC get_winproc( WNDPROC proc, BOOL ansi ) DECLSPEC_HIDDEN;

/* cursoricon.c */
HICON alloc_cursoricon_handle( BOOL is_icon ) DECLSPEC_HIDDEN;

/* message.c */
LRESULT handle_internal_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;

/* window.c */
HANDLE alloc_user_handle( struct user_object *ptr, unsigned int type ) DECLSPEC_HIDDEN;
void *free_user_handle( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void release_user_handle_ptr( void *ptr ) DECLSPEC_HIDDEN;

WND *get_win_ptr( HWND hwnd ) DECLSPEC_HIDDEN;
BOOL is_child( HWND parent, HWND child );
BOOL is_window( HWND hwnd ) DECLSPEC_HIDDEN;

#endif /* __WINE_NTUSER_PRIVATE_H */
