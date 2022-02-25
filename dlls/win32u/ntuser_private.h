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

struct user_callbacks
{
    HWND (WINAPI *pGetDesktopWindow)(void);
    BOOL (WINAPI *pGetWindowRect)( HWND hwnd, LPRECT rect );
    BOOL (WINAPI *pIsChild)( HWND, HWND );
    BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    LRESULT (WINAPI *pSendMessageTimeoutW)( HWND, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR );
    HWND (WINAPI *pWindowFromDC)( HDC );
    LRESULT (WINAPI *send_ll_message)( DWORD, DWORD, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR );
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

#define WND_OTHER_PROCESS ((WND *)1)  /* returned by WIN_GetPtr on unknown window handles */
#define WND_DESKTOP       ((WND *)2)  /* returned by WIN_GetPtr on the desktop window */

WND *next_thread_window_ptr( HWND *hwnd );

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

/* cursoricon.c */
HICON alloc_cursoricon_handle( BOOL is_icon ) DECLSPEC_HIDDEN;

/* message.c */
LRESULT handle_internal_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;

/* window.c */
HANDLE alloc_user_handle( struct user_object *ptr, unsigned int type ) DECLSPEC_HIDDEN;
void *free_user_handle( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, unsigned int type ) DECLSPEC_HIDDEN;
void release_user_handle_ptr( void *ptr ) DECLSPEC_HIDDEN;

#endif /* __WINE_NTUSER_PRIVATE_H */
