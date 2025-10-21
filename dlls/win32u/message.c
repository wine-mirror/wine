/*
 * Window messaging support
 *
 * Copyright 2001 Alexandre Julliard
 * Copyright 2008 Maarten Lankhorst
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


#if 0
#pragma makedep unix
#endif

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "ddk/wdm.h"
#include "win32u_private.h"
#include "ntuser_private.h"
#include "winnls.h"
#include "hidusage.h"
#include "dbt.h"
#include "dde.h"
#include "immdev.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(key);
WINE_DECLARE_DEBUG_CHANNEL(relay);

#define QS_DRIVER       0x80000000
#define QS_HARDWARE     0x40000000
#define QS_INTERNAL     (QS_DRIVER | QS_HARDWARE)

static const struct _KUSER_SHARED_DATA *user_shared_data = (struct _KUSER_SHARED_DATA *)0x7ffe0000;

static LONG atomic_load_long( const volatile LONG *ptr )
{
#if defined(__i386__) || defined(__x86_64__)
    return *ptr;
#else
    return __atomic_load_n( ptr, __ATOMIC_SEQ_CST );
#endif
}

static ULONG atomic_load_ulong( const volatile ULONG *ptr )
{
#if defined(__i386__) || defined(__x86_64__)
    return *ptr;
#else
    return __atomic_load_n( ptr, __ATOMIC_SEQ_CST );
#endif
}

static UINT64 get_tick_count(void)
{
    ULONG high, low;

    do
    {
        high = atomic_load_long( &user_shared_data->TickCount.High1Time );
        low = atomic_load_ulong( &user_shared_data->TickCount.LowPart );
    }
    while (high != atomic_load_long( &user_shared_data->TickCount.High2Time ));
    /* note: we ignore TickCountMultiplier */
    return (UINT64)high << 32 | low;
}

#define MAX_WINPROC_RECURSION  64

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

#define MAX_PACK_COUNT 4

struct peek_message_filter
{
    HWND hwnd;
    UINT first;
    UINT last;
    UINT mask;
    UINT flags;
    BOOL internal;
};

/* info about the message currently being received by the current thread */
struct received_message_info
{
    UINT  type;
    MSG   msg;
    UINT  flags;  /* InSendMessageEx return flags */
    struct received_message_info *prev;
};

struct packed_hook_extra_info
{
    user_handle_t handle;
    DWORD         __pad;
    ULONGLONG     lparam;
};

/* the various structures that can be sent in messages, in platform-independent layout */
struct packed_CREATESTRUCTW
{
    ULONGLONG     lpCreateParams;
    ULONGLONG     hInstance;
    user_handle_t hMenu;
    DWORD         __pad1;
    user_handle_t hwndParent;
    DWORD         __pad2;
    INT           cy;
    INT           cx;
    INT           y;
    INT           x;
    LONG          style;
    ULONGLONG     lpszName;
    ULONGLONG     lpszClass;
    DWORD         dwExStyle;
    DWORD         __pad3;
};

struct packed_DRAWITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    UINT          itemID;
    UINT          itemAction;
    UINT          itemState;
    user_handle_t hwndItem;
    DWORD         __pad1;
    user_handle_t hDC;
    DWORD         __pad2;
    RECT          rcItem;
    ULONGLONG     itemData;
};

struct packed_MEASUREITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    UINT          itemID;
    UINT          itemWidth;
    UINT          itemHeight;
    ULONGLONG     itemData;
};

struct packed_DELETEITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    UINT          itemID;
    user_handle_t hwndItem;
    DWORD         __pad;
    ULONGLONG     itemData;
};

struct packed_COMPAREITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    user_handle_t hwndItem;
    DWORD         __pad1;
    UINT          itemID1;
    ULONGLONG     itemData1;
    UINT          itemID2;
    ULONGLONG     itemData2;
    DWORD         dwLocaleId;
    DWORD         __pad2;
};

struct packed_WINDOWPOS
{
    UINT          hwnd;
    DWORD         __pad1;
    user_handle_t hwndInsertAfter;
    DWORD         __pad2;
    INT           x;
    INT           y;
    INT           cx;
    INT           cy;
    UINT          flags;
    DWORD         __pad3;
};

struct packed_COPYDATASTRUCT
{
    ULONGLONG     dwData;
    DWORD         cbData;
    ULONGLONG     lpData;
};

struct packed_HELPINFO
{
    UINT          cbSize;
    INT           iContextType;
    INT           iCtrlId;
    user_handle_t hItemHandle;
    DWORD         __pad;
    ULONGLONG     dwContextId;
    POINT         MousePos;
};

struct packed_NCCALCSIZE_PARAMS
{
    RECT          rgrc[3];
    ULONGLONG     __pad1;
    user_handle_t hwnd;
    DWORD         __pad2;
    user_handle_t hwndInsertAfter;
    DWORD         __pad3;
    INT           x;
    INT           y;
    INT           cx;
    INT           cy;
    UINT          flags;
    DWORD         __pad4;
};

struct packed_MSG
{
    user_handle_t hwnd;
    DWORD         __pad1;
    UINT          message;
    ULONGLONG     wParam;
    ULONGLONG     lParam;
    DWORD         time;
    POINT         pt;
    DWORD         __pad2;
};

struct packed_MDINEXTMENU
{
    user_handle_t hmenuIn;
    DWORD         __pad1;
    user_handle_t hmenuNext;
    DWORD         __pad2;
    user_handle_t hwndNext;
    DWORD         __pad3;
};

struct packed_MDICREATESTRUCTW
{
    ULONGLONG     szClass;
    ULONGLONG     szTitle;
    ULONGLONG     hOwner;
    INT           x;
    INT           y;
    INT           cx;
    INT           cy;
    DWORD         style;
    ULONGLONG     lParam;
};

struct packed_COMBOBOXINFO
{
    DWORD cbSize;
    RECT rcItem;
    RECT rcButton;
    DWORD stateButton;
    ULONGLONG hwndCombo;
    ULONGLONG hwndItem;
    ULONGLONG hwndList;
};

/* the structures are unpacked on top of the packed ones, so make sure they fit */
C_ASSERT( sizeof(struct packed_CREATESTRUCTW) >= sizeof(CREATESTRUCTW) );
C_ASSERT( sizeof(struct packed_DRAWITEMSTRUCT) >= sizeof(DRAWITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_MEASUREITEMSTRUCT) >= sizeof(MEASUREITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_DELETEITEMSTRUCT) >= sizeof(DELETEITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_COMPAREITEMSTRUCT) >= sizeof(COMPAREITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_WINDOWPOS) >= sizeof(WINDOWPOS) );
C_ASSERT( sizeof(struct packed_COPYDATASTRUCT) >= sizeof(COPYDATASTRUCT) );
C_ASSERT( sizeof(struct packed_HELPINFO) >= sizeof(HELPINFO) );
C_ASSERT( sizeof(struct packed_NCCALCSIZE_PARAMS) >= sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) );
C_ASSERT( sizeof(struct packed_MSG) >= sizeof(MSG) );
C_ASSERT( sizeof(struct packed_MDINEXTMENU) >= sizeof(MDINEXTMENU) );
C_ASSERT( sizeof(struct packed_MDICREATESTRUCTW) >= sizeof(MDICREATESTRUCTW) );
C_ASSERT( sizeof(struct packed_COMBOBOXINFO) >= sizeof(COMBOBOXINFO) );
C_ASSERT( sizeof(struct packed_hook_extra_info) >= sizeof(struct hook_extra_info) );

union packed_structs
{
    struct packed_CREATESTRUCTW cs;
    struct packed_DRAWITEMSTRUCT dis;
    struct packed_MEASUREITEMSTRUCT mis;
    struct packed_DELETEITEMSTRUCT dls;
    struct packed_COMPAREITEMSTRUCT cis;
    struct packed_WINDOWPOS wp;
    struct packed_COPYDATASTRUCT cds;
    struct packed_HELPINFO hi;
    struct packed_NCCALCSIZE_PARAMS ncp;
    struct packed_MSG msg;
    struct packed_MDINEXTMENU mnm;
    struct packed_MDICREATESTRUCTW mcs;
    struct packed_COMBOBOXINFO cbi;
    struct packed_hook_extra_info hook;
};

/* description of the data fields that need to be packed along with a sent message */
struct packed_message
{
    union packed_structs ps;
    int                  count;
    const void          *data[MAX_PACK_COUNT];
    size_t               size[MAX_PACK_COUNT];
};

/* structure to group all parameters for sent messages of the various kinds */
struct send_message_info
{
    enum message_type type;
    DWORD             dest_tid;
    HWND              hwnd;
    UINT              msg;
    WPARAM            wparam;
    LPARAM            lparam;
    UINT              flags;      /* flags for SendMessageTimeout */
    UINT              timeout;    /* timeout for SendMessageTimeout */
    SENDASYNCPROC     callback;   /* callback function for SendMessageCallback */
    ULONG_PTR         data;       /* callback data */
    enum wm_char_mapping wm_char;
    struct win_proc_params *params;
};

static const INPUT_MESSAGE_SOURCE msg_source_unavailable = { IMDT_UNAVAILABLE, IMO_UNAVAILABLE };

/* flag for messages that contain pointers */
/* 32 messages per entry, messages 0..31 map to bits 0..31 */

#define SET(msg) (1 << ((msg) & 31))

static const unsigned int message_pointer_flags[] =
{
    /* 0x00 - 0x1f */
    SET(WM_CREATE) | SET(WM_SETTEXT) | SET(WM_GETTEXT) |
    SET(WM_WININICHANGE) | SET(WM_DEVMODECHANGE),
    /* 0x20 - 0x3f */
    SET(WM_GETMINMAXINFO) | SET(WM_DRAWITEM) | SET(WM_MEASUREITEM) | SET(WM_DELETEITEM) |
    SET(WM_COMPAREITEM),
    /* 0x40 - 0x5f */
    SET(WM_WINDOWPOSCHANGING) | SET(WM_WINDOWPOSCHANGED) | SET(WM_COPYDATA) | SET(WM_HELP),
    /* 0x60 - 0x7f */
    SET(WM_STYLECHANGING) | SET(WM_STYLECHANGED),
    /* 0x80 - 0x9f */
    SET(WM_NCCREATE) | SET(WM_NCCALCSIZE) | SET(WM_GETDLGCODE),
    /* 0xa0 - 0xbf */
    SET(EM_GETSEL) | SET(EM_GETRECT) | SET(EM_SETRECT) | SET(EM_SETRECTNP),
    /* 0xc0 - 0xdf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETTABSTOPS),
    /* 0xe0 - 0xff */
    SET(SBM_GETRANGE) | SET(SBM_SETSCROLLINFO) | SET(SBM_GETSCROLLINFO) | SET(SBM_GETSCROLLBARINFO),
    /* 0x100 - 0x11f */
    0,
    /* 0x120 - 0x13f */
    0,
    /* 0x140 - 0x15f */
    SET(CB_GETEDITSEL) | SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) |
    SET(CB_INSERTSTRING) | SET(CB_FINDSTRING) | SET(CB_SELECTSTRING) |
    SET(CB_GETDROPPEDCONTROLRECT) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x17f */
    SET(CB_GETCOMBOBOXINFO),
    /* 0x180 - 0x19f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_SELECTSTRING) |
    SET(LB_DIR) | SET(LB_FINDSTRING) |
    SET(LB_GETSELITEMS) | SET(LB_SETTABSTOPS) | SET(LB_ADDFILE) | SET(LB_GETITEMRECT),
    /* 0x1a0 - 0x1bf */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1c0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ff */
    0,
    /* 0x200 - 0x21f */
    SET(WM_NEXTMENU) | SET(WM_SIZING) | SET(WM_MOVING) | SET(WM_DEVICECHANGE),
    /* 0x220 - 0x23f */
    SET(WM_MDICREATE) | SET(WM_MDIGETACTIVE) | SET(WM_DROPOBJECT) |
    SET(WM_QUERYDROPOBJECT) | SET(WM_DRAGLOOP) | SET(WM_DRAGSELECT) | SET(WM_DRAGMOVE),
    /* 0x240 - 0x25f */
    0,
    /* 0x260 - 0x27f */
    0,
    /* 0x280 - 0x29f */
    0,
    /* 0x2a0 - 0x2bf */
    0,
    /* 0x2c0 - 0x2df */
    0,
    /* 0x2e0 - 0x2ff */
    0,
    /* 0x300 - 0x31f */
    SET(WM_ASKCBFORMATNAME)
};

/* check whether a given message type includes pointers */
static inline BOOL is_pointer_message( UINT message, WPARAM wparam )
{
    if (message >= 8*sizeof(message_pointer_flags)) return FALSE;
    if (message == WM_DEVICECHANGE && !(wparam & 0x8000)) return FALSE;
    return (message_pointer_flags[message / 32] & SET(message)) != 0;
}

#undef SET

static BOOL init_win_proc_params( struct win_proc_params *params, HWND hwnd, UINT msg,
                                  WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    if (!params->func) return FALSE;

    user_check_not_lock();

    params->hwnd = get_full_window_handle( hwnd );
    params->msg = msg;
    params->wparam = wparam;
    params->lparam = lparam;
    params->ansi = params->ansi_dst = ansi;
    params->mapping = WMCHAR_MAP_CALLWINDOWPROC;
    params->dpi_context = get_window_dpi_awareness_context( params->hwnd );
    get_winproc_params( params, TRUE );
    return TRUE;
}

static BOOL init_window_call_params( struct win_proc_params *params, HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, BOOL ansi, enum wm_char_mapping mapping )
{
    BOOL is_dialog;
    WND *win;

    user_check_not_lock();

    if (!is_current_thread_window( hwnd )) return FALSE;
    if (!(win = get_win_ptr( hwnd ))) return FALSE;
    if (win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;
    params->func = win->winproc;
    params->ansi_dst = !(win->flags & WIN_ISUNICODE);
    is_dialog = win->dlgInfo != NULL;
    release_win_ptr( win );

    params->hwnd = get_full_window_handle( hwnd );
    params->msg = msg;
    params->wparam = wParam;
    params->lparam = lParam;
    params->ansi = ansi;
    params->mapping = mapping;
    params->dpi_context = get_window_dpi_awareness_context( params->hwnd );
    get_winproc_params( params, !is_dialog );
    return TRUE;
}

static LRESULT dispatch_win_proc_params( struct win_proc_params *params, size_t size,
                                         void **client_ret, size_t *client_ret_size )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    LRESULT result = 0;
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status;

    if (thread_info->recursion_count > MAX_WINPROC_RECURSION) return 0;
    thread_info->recursion_count++;
    status = KeUserModeCallback( NtUserCallWinProc, params, size, &ret_ptr, &ret_len );
    thread_info->recursion_count--;

    if (status) return result;

    if (ret_len >= sizeof(result))
    {
        result = *(LRESULT *)ret_ptr;
        if (client_ret)
        {
            *client_ret = (LRESULT *)ret_ptr + 1;
            *client_ret_size = ret_len - sizeof(result);
        }
    }

    return result;
}

/* add a data field to a packed message */
static inline void push_data( struct packed_message *data, const void *ptr, size_t size )
{
    data->data[data->count] = ptr;
    data->size[data->count] = size;
    data->count++;
}

/* pack a pointer into a 32/64 portable format */
static inline ULONGLONG pack_ptr( const void *ptr )
{
    return (ULONG_PTR)ptr;
}

/* unpack a potentially 64-bit pointer, returning 0 when truncated */
static inline void *unpack_ptr( ULONGLONG ptr64 )
{
    if ((ULONG_PTR)ptr64 != ptr64) return 0;
    return (void *)(ULONG_PTR)ptr64;
}

/* add a string to a packed message */
static inline void push_string( struct packed_message *data, LPCWSTR str )
{
    push_data( data, str, (lstrlenW(str) + 1) * sizeof(WCHAR) );
}

/* make sure that there is space for 'size' bytes in buffer, growing it if needed */
static inline void *get_buffer_space( void **buffer, size_t size, size_t *buffer_size )
{
    if (*buffer_size < size)
    {
        void *new;

        if (!(new = realloc( *buffer, size ))) return NULL;
        *buffer = new;
        *buffer_size = size;
    }
    return *buffer;
}

/* check whether a combobox expects strings or ids in CB_ADDSTRING/CB_INSERTSTRING */
static inline BOOL combobox_has_strings( HWND hwnd )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
}

/* check whether a listbox expects strings or ids in LB_ADDSTRING/LB_INSERTSTRING */
static inline BOOL listbox_has_strings( HWND hwnd )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));
}

/* check whether message is in the range of keyboard messages */
static inline BOOL is_keyboard_message( UINT message )
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST);
}

/* check whether message is in the range of mouse messages */
static inline BOOL is_mouse_message( UINT message )
{
    return ((message >= WM_NCMOUSEFIRST && message <= WM_NCMOUSELAST) ||
            (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST));
}

/* check whether message matches the specified hwnd filter */
static inline BOOL check_hwnd_filter( const MSG *msg, HWND hwnd_filter )
{
    if (!hwnd_filter || hwnd_filter == get_desktop_window()) return TRUE;
    return (msg->hwnd == hwnd_filter || is_child( hwnd_filter, msg->hwnd ));
}

/***********************************************************************
 *           unpack_message
 *
 * Unpack a message received from another process.
 */
static BOOL unpack_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                            void **buffer, size_t size, size_t *buffer_size )
{
    size_t minsize = 0;
    union packed_structs *ps = *buffer;

    switch(message)
    {
     case WM_NCCREATE:
     case WM_CREATE:
     {
         CREATESTRUCTW cs;
         WCHAR *str = (WCHAR *)(&ps->cs + 1);
         if (size < sizeof(ps->cs)) return FALSE;
         size -= sizeof(ps->cs);
         cs.lpCreateParams = unpack_ptr( ps->cs.lpCreateParams );
         cs.hInstance      = unpack_ptr( ps->cs.hInstance );
         cs.hMenu          = wine_server_ptr_handle( ps->cs.hMenu );
         cs.hwndParent     = wine_server_ptr_handle( ps->cs.hwndParent );
         cs.cy             = ps->cs.cy;
         cs.cx             = ps->cs.cx;
         cs.y              = ps->cs.y;
         cs.x              = ps->cs.x;
         cs.style          = ps->cs.style;
         cs.dwExStyle      = ps->cs.dwExStyle;
         cs.lpszName       = unpack_ptr( ps->cs.lpszName );
         cs.lpszClass      = unpack_ptr( ps->cs.lpszClass );
         if (ps->cs.lpszName >> 16)
         {
             cs.lpszName = str;
             size -= (lstrlenW(str) + 1) * sizeof(WCHAR);
             str += lstrlenW(str) + 1;
         }
         if (ps->cs.lpszClass >> 16)
         {
             cs.lpszClass = str;
         }
         memcpy( *buffer, &cs, sizeof(cs) );
         break;
    }
    case WM_NCCALCSIZE:
        if (!*wparam) minsize = sizeof(RECT);
        else
        {
            NCCALCSIZE_PARAMS ncp;
            WINDOWPOS wp;
            if (size < sizeof(ps->ncp)) return FALSE;
            ncp.rgrc[0]        = ps->ncp.rgrc[0];
            ncp.rgrc[1]        = ps->ncp.rgrc[1];
            ncp.rgrc[2]        = ps->ncp.rgrc[2];
            wp.hwnd            = wine_server_ptr_handle( ps->ncp.hwnd );
            wp.hwndInsertAfter = wine_server_ptr_handle( ps->ncp.hwndInsertAfter );
            wp.x               = ps->ncp.x;
            wp.y               = ps->ncp.y;
            wp.cx              = ps->ncp.cx;
            wp.cy              = ps->ncp.cy;
            wp.flags           = ps->ncp.flags;
            ncp.lppos = (WINDOWPOS *)((NCCALCSIZE_PARAMS *)&ps->ncp + 1);
            memcpy( &ps->ncp, &ncp, sizeof(ncp) );
            *ncp.lppos = wp;
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        if (!get_buffer_space( buffer, (*wparam * sizeof(WCHAR)), buffer_size )) return FALSE;
        break;
    case WM_WININICHANGE:
        if (!*lparam) return TRUE;
        /* fall through */
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        break;
    case WM_GETMINMAXINFO:
        minsize = sizeof(MINMAXINFO);
        break;
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT dis;
        if (size < sizeof(ps->dis)) return FALSE;
        dis.CtlType    = ps->dis.CtlType;
        dis.CtlID      = ps->dis.CtlID;
        dis.itemID     = ps->dis.itemID;
        dis.itemAction = ps->dis.itemAction;
        dis.itemState  = ps->dis.itemState;
        dis.hwndItem   = wine_server_ptr_handle( ps->dis.hwndItem );
        dis.hDC        = wine_server_ptr_handle( ps->dis.hDC );
        dis.rcItem     = ps->dis.rcItem;
        dis.itemData   = (ULONG_PTR)unpack_ptr( ps->dis.itemData );
        memcpy( *buffer, &dis, sizeof(dis) );
        break;
    }
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT mis;
        if (size < sizeof(ps->mis)) return FALSE;
        mis.CtlType    = ps->mis.CtlType;
        mis.CtlID      = ps->mis.CtlID;
        mis.itemID     = ps->mis.itemID;
        mis.itemWidth  = ps->mis.itemWidth;
        mis.itemHeight = ps->mis.itemHeight;
        mis.itemData   = (ULONG_PTR)unpack_ptr( ps->mis.itemData );
        memcpy( *buffer, &mis, sizeof(mis) );
        break;
    }
    case WM_DELETEITEM:
    {
        DELETEITEMSTRUCT dls;
        if (size < sizeof(ps->dls)) return FALSE;
        dls.CtlType    = ps->dls.CtlType;
        dls.CtlID      = ps->dls.CtlID;
        dls.itemID     = ps->dls.itemID;
        dls.hwndItem   = wine_server_ptr_handle( ps->dls.hwndItem );
        dls.itemData   = (ULONG_PTR)unpack_ptr( ps->dls.itemData );
        memcpy( *buffer, &dls, sizeof(dls) );
        break;
    }
    case WM_COMPAREITEM:
    {
        COMPAREITEMSTRUCT cis;
        if (size < sizeof(ps->cis)) return FALSE;
        cis.CtlType    = ps->cis.CtlType;
        cis.CtlID      = ps->cis.CtlID;
        cis.hwndItem   = wine_server_ptr_handle( ps->cis.hwndItem );
        cis.itemID1    = ps->cis.itemID1;
        cis.itemData1  = (ULONG_PTR)unpack_ptr( ps->cis.itemData1 );
        cis.itemID2    = ps->cis.itemID2;
        cis.itemData2  = (ULONG_PTR)unpack_ptr( ps->cis.itemData2 );
        cis.dwLocaleId = ps->cis.dwLocaleId;
        memcpy( *buffer, &cis, sizeof(cis) );
        break;
    }
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS wp;
        if (size < sizeof(ps->wp)) return FALSE;
        wp.hwnd            = wine_server_ptr_handle( ps->wp.hwnd );
        wp.hwndInsertAfter = wine_server_ptr_handle( ps->wp.hwndInsertAfter );
        wp.x               = ps->wp.x;
        wp.y               = ps->wp.y;
        wp.cx              = ps->wp.cx;
        wp.cy              = ps->wp.cy;
        wp.flags           = ps->wp.flags;
        memcpy( *buffer, &wp, sizeof(wp) );
        break;
    }
    case WM_COPYDATA:
    {
        COPYDATASTRUCT cds;
        if (size < sizeof(ps->cds)) return FALSE;
        cds.dwData = (ULONG_PTR)unpack_ptr( ps->cds.dwData );
        if (ps->cds.lpData)
        {
            cds.cbData = ps->cds.cbData;
            cds.lpData = &ps->cds + 1;
            minsize = sizeof(ps->cds) + cds.cbData;
        }
        else
        {
            cds.cbData = 0;
            cds.lpData = 0;
        }
        memcpy( &ps->cds, &cds, sizeof(cds) );
        break;
    }
    case WM_HELP:
    {
        HELPINFO hi;
        if (size < sizeof(ps->hi)) return FALSE;
        hi.cbSize       = sizeof(hi);
        hi.iContextType = ps->hi.iContextType;
        hi.iCtrlId      = ps->hi.iCtrlId;
        hi.hItemHandle  = wine_server_ptr_handle( ps->hi.hItemHandle );
        hi.dwContextId  = (ULONG_PTR)unpack_ptr( ps->hi.dwContextId );
        hi.MousePos     = ps->hi.MousePos;
        memcpy( &ps->hi, &hi, sizeof(hi) );
        break;
    }
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        minsize = sizeof(STYLESTRUCT);
        break;
    case WM_GETDLGCODE:
        if (*lparam)
        {
            MSG msg;
            if (size < sizeof(ps->msg)) return FALSE;
            msg.hwnd    = wine_server_ptr_handle( ps->msg.hwnd );
            msg.message = ps->msg.message;
            msg.wParam  = (ULONG_PTR)unpack_ptr( ps->msg.wParam );
            msg.lParam  = (ULONG_PTR)unpack_ptr( ps->msg.lParam );
            msg.time    = ps->msg.time;
            msg.pt      = ps->msg.pt;
            memcpy( &ps->msg, &msg, sizeof(msg) );
            break;
        }
        return TRUE;
    case SBM_SETSCROLLINFO:
        minsize = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLINFO), buffer_size )) return FALSE;
        break;
    case SBM_GETSCROLLBARINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLBARINFO), buffer_size )) return FALSE;
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (*wparam || *lparam)
        {
            if (!get_buffer_space( buffer, 2 * sizeof(DWORD), buffer_size )) return FALSE;
            if (*wparam) *wparam = (WPARAM)*buffer;
            if (*lparam) *lparam = (LPARAM)((DWORD *)*buffer + 1);
        }
        return TRUE;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        if (!get_buffer_space( buffer, sizeof(RECT), buffer_size )) return FALSE;
        break;
    case EM_SETRECT:
    case EM_SETRECTNP:
        minsize = sizeof(RECT);
        break;
    case EM_GETLINE:
    {
        WORD *len_ptr, len;
        if (size < sizeof(WORD)) return FALSE;
        len = *(WORD *)*buffer;
        if (!get_buffer_space( buffer, (len + 1) * sizeof(WCHAR), buffer_size )) return FALSE;
        len_ptr = *buffer;
        len_ptr[0] = len_ptr[1] = len;
        *lparam = (LPARAM)(len_ptr + 1);
        return TRUE;
    }
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        if (!*wparam) return TRUE;
        minsize = *wparam * sizeof(UINT);
        break;
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (!*buffer) return TRUE;
        break;
    case CB_GETLBTEXT:
    {
        if (combobox_has_strings( hwnd ))
            size = (send_message( hwnd, CB_GETLBTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        else
            size = sizeof(ULONG_PTR);
        if (!get_buffer_space( buffer, size, buffer_size )) return FALSE;
        break;
    }
    case LB_GETTEXT:
    {
        if (listbox_has_strings( hwnd ))
            size = (send_message( hwnd, LB_GETTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        else
            size = sizeof(ULONG_PTR);
        if (!get_buffer_space( buffer, size, buffer_size )) return FALSE;
        break;
    }
    case LB_GETSELITEMS:
        if (!get_buffer_space( buffer, *wparam * sizeof(UINT), buffer_size )) return FALSE;
        break;
    case WM_NEXTMENU:
    {
        MDINEXTMENU mnm;
        if (size < sizeof(ps->mnm)) return FALSE;
        mnm.hmenuIn   = wine_server_ptr_handle( ps->mnm.hmenuIn );
        mnm.hmenuNext = wine_server_ptr_handle( ps->mnm.hmenuNext );
        mnm.hwndNext  = wine_server_ptr_handle( ps->mnm.hwndNext );
        memcpy( *buffer, &mnm, sizeof(mnm) );
        break;
    }
    case WM_SIZING:
    case WM_MOVING:
        minsize = sizeof(RECT);
        if (!get_buffer_space( buffer, sizeof(RECT), buffer_size )) return FALSE;
        break;
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW mcs;
        WCHAR *str = (WCHAR *)(&ps->mcs + 1);
        if (size < sizeof(ps->mcs)) return FALSE;
        size -= sizeof(ps->mcs);

        mcs.szClass = unpack_ptr( ps->mcs.szClass );
        mcs.szTitle = unpack_ptr( ps->mcs.szTitle );
        mcs.hOwner  = unpack_ptr( ps->mcs.hOwner );
        mcs.x       = ps->mcs.x;
        mcs.y       = ps->mcs.y;
        mcs.cx      = ps->mcs.cx;
        mcs.cy      = ps->mcs.cy;
        mcs.style   = ps->mcs.style;
        mcs.lParam  = (LPARAM)unpack_ptr( ps->mcs.lParam );
        if (ps->mcs.szClass >> 16)
        {
            mcs.szClass = str;
            size -= (lstrlenW(str) + 1) * sizeof(WCHAR);
            str += lstrlenW(str) + 1;
        }
        if (ps->mcs.szTitle >> 16)
        {
            mcs.szTitle = str;
        }
        memcpy( *buffer, &mcs, sizeof(mcs) );
        break;
    }
    case WM_WINE_SETWINDOWPOS:
    {
        WINDOWPOS wp;
        if (size < sizeof(ps->wp)) return FALSE;
        wp.hwnd            = wine_server_ptr_handle( ps->wp.hwnd );
        wp.hwndInsertAfter = wine_server_ptr_handle( ps->wp.hwndInsertAfter );
        wp.x               = ps->wp.x;
        wp.y               = ps->wp.y;
        wp.cx              = ps->wp.cx;
        wp.cy              = ps->wp.cy;
        wp.flags           = ps->wp.flags;
        memcpy( ps, &wp, sizeof(wp) );
        break;
    }
    case WM_WINE_KEYBOARD_LL_HOOK:
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info h_extra;
        minsize = sizeof(ps->hook) +
                  (message == WM_WINE_KEYBOARD_LL_HOOK ? sizeof(KBDLLHOOKSTRUCT)
                                                       : sizeof(MSLLHOOKSTRUCT));
        if (size < minsize) return FALSE;
        h_extra.handle = wine_server_ptr_handle( ps->hook.handle );
        h_extra.lparam = (LPARAM)(&ps->hook + 1);
        memcpy( &ps->hook, &h_extra, sizeof(h_extra) );
        break;
    }
    case CB_GETCOMBOBOXINFO:
    {
        COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
        memcpy( ps, &cbi, sizeof(cbi) );
        break;
    }
    case WM_MDIGETACTIVE:
        if (!*lparam) return TRUE;
        if (!get_buffer_space( buffer, sizeof(BOOL), buffer_size )) return FALSE;
        break;
    case WM_DEVICECHANGE:
        if (!(*wparam & 0x8000)) return TRUE;
        minsize = sizeof(DEV_BROADCAST_HDR);
        break;
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        return FALSE;
    case WM_NCPAINT:
        if (*wparam <= 1) return TRUE;
        FIXME( "WM_NCPAINT hdc unpacking not supported\n" );
        return FALSE;
    case WM_PAINT:
        if (!*wparam) return TRUE;
        /* fall through */

    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_PRINT:
    case WM_PRINTCLIENT:
    /* these contain an HGLOBAL */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    /* these contain HICON */
    case WM_GETICON:
    case WM_SETICON:
    case WM_QUERYDRAGICON:
    case WM_QUERYPARKICON:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
        FIXME( "msg %x (%s) not supported yet\n", message, debugstr_msg_name( message, hwnd ));
        return FALSE;

    default:
        return TRUE;  /* message doesn't need any unpacking */
    }

    /* default exit for most messages: check minsize and store buffer in lparam */
    if (size < minsize) return FALSE;
    *lparam = (LPARAM)*buffer;
    return TRUE;
}

/***********************************************************************
 *           pack_message
 *
 * Pack a message for sending to another process.
 * Return the size of the data we expect in the message reply.
 * Set data->count to -1 if there is an error.
 */
static size_t pack_message( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                            struct packed_message *data )
{
    data->count = 0;
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        data->ps.cs.lpCreateParams = pack_ptr( cs->lpCreateParams );
        data->ps.cs.hInstance      = pack_ptr( cs->hInstance );
        data->ps.cs.hMenu          = wine_server_user_handle( cs->hMenu );
        data->ps.cs.hwndParent     = wine_server_user_handle( cs->hwndParent );
        data->ps.cs.cy             = cs->cy;
        data->ps.cs.cx             = cs->cx;
        data->ps.cs.y              = cs->y;
        data->ps.cs.x              = cs->x;
        data->ps.cs.style          = cs->style;
        data->ps.cs.dwExStyle      = cs->dwExStyle;
        data->ps.cs.lpszName       = pack_ptr( cs->lpszName );
        data->ps.cs.lpszClass      = pack_ptr( cs->lpszClass );
        push_data( data, &data->ps.cs, sizeof(data->ps.cs) );
        if (!IS_INTRESOURCE(cs->lpszName)) push_string( data, cs->lpszName );
        if (!IS_INTRESOURCE(cs->lpszClass)) push_string( data, cs->lpszClass );
        return sizeof(data->ps.cs);
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        return wparam * sizeof(WCHAR);
    case WM_WININICHANGE:
        if (lparam) push_string(data, (LPWSTR)lparam );
        return 0;
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        push_string( data, (LPWSTR)lparam );
        return 0;
    case WM_GETMINMAXINFO:
        push_data( data, (MINMAXINFO *)lparam, sizeof(MINMAXINFO) );
        return sizeof(MINMAXINFO);
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lparam;
        data->ps.dis.CtlType    = dis->CtlType;
        data->ps.dis.CtlID      = dis->CtlID;
        data->ps.dis.itemID     = dis->itemID;
        data->ps.dis.itemAction = dis->itemAction;
        data->ps.dis.itemState  = dis->itemState;
        data->ps.dis.hwndItem   = wine_server_user_handle( dis->hwndItem );
        data->ps.dis.hDC        = wine_server_user_handle( dis->hDC );  /* FIXME */
        data->ps.dis.rcItem     = dis->rcItem;
        data->ps.dis.itemData   = dis->itemData;
        push_data( data, &data->ps.dis, sizeof(data->ps.dis) );
        return 0;
    }
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lparam;
        data->ps.mis.CtlType    = mis->CtlType;
        data->ps.mis.CtlID      = mis->CtlID;
        data->ps.mis.itemID     = mis->itemID;
        data->ps.mis.itemWidth  = mis->itemWidth;
        data->ps.mis.itemHeight = mis->itemHeight;
        data->ps.mis.itemData   = mis->itemData;
        push_data( data, &data->ps.mis, sizeof(data->ps.mis) );
        return sizeof(data->ps.mis);
    }
    case WM_DELETEITEM:
    {
        DELETEITEMSTRUCT *dls = (DELETEITEMSTRUCT *)lparam;
        data->ps.dls.CtlType    = dls->CtlType;
        data->ps.dls.CtlID      = dls->CtlID;
        data->ps.dls.itemID     = dls->itemID;
        data->ps.dls.hwndItem   = wine_server_user_handle( dls->hwndItem );
        data->ps.dls.itemData   = dls->itemData;
        push_data( data, &data->ps.dls, sizeof(data->ps.dls) );
        return 0;
    }
    case WM_COMPAREITEM:
    {
        COMPAREITEMSTRUCT *cis = (COMPAREITEMSTRUCT *)lparam;
        data->ps.cis.CtlType    = cis->CtlType;
        data->ps.cis.CtlID      = cis->CtlID;
        data->ps.cis.hwndItem   = wine_server_user_handle( cis->hwndItem );
        data->ps.cis.itemID1    = cis->itemID1;
        data->ps.cis.itemData1  = cis->itemData1;
        data->ps.cis.itemID2    = cis->itemID2;
        data->ps.cis.itemData2  = cis->itemData2;
        data->ps.cis.dwLocaleId = cis->dwLocaleId;
        push_data( data, &data->ps.cis, sizeof(data->ps.cis) );
        return 0;
    }
    case WM_WINE_SETWINDOWPOS:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS *wp = (WINDOWPOS *)lparam;
        data->ps.wp.hwnd            = wine_server_user_handle( wp->hwnd );
        data->ps.wp.hwndInsertAfter = wine_server_user_handle( wp->hwndInsertAfter );
        data->ps.wp.x               = wp->x;
        data->ps.wp.y               = wp->y;
        data->ps.wp.cx              = wp->cx;
        data->ps.wp.cy              = wp->cy;
        data->ps.wp.flags           = wp->flags;
        push_data( data, &data->ps.wp, sizeof(data->ps.wp) );
        return sizeof(data->ps.wp);
    }
    case WM_COPYDATA:
    {
        COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lparam;
        data->ps.cds.cbData = cds->cbData;
        data->ps.cds.dwData = cds->dwData;
        data->ps.cds.lpData = pack_ptr( cds->lpData );
        push_data( data, &data->ps.cds, sizeof(data->ps.cds) );
        if (cds->lpData) push_data( data, cds->lpData, cds->cbData );
        return 0;
    }
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        data->count = -1;
        return 0;
    case WM_HELP:
    {
        HELPINFO *hi = (HELPINFO *)lparam;
        data->ps.hi.iContextType = hi->iContextType;
        data->ps.hi.iCtrlId      = hi->iCtrlId;
        data->ps.hi.hItemHandle  = wine_server_user_handle( hi->hItemHandle );
        data->ps.hi.dwContextId  = hi->dwContextId;
        data->ps.hi.MousePos     = hi->MousePos;
        push_data( data, &data->ps.hi, sizeof(data->ps.hi) );
        return 0;
    }
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        push_data( data, (STYLESTRUCT *)lparam, sizeof(STYLESTRUCT) );
        return 0;
    case WM_NCCALCSIZE:
        if (!wparam)
        {
            push_data( data, (RECT *)lparam, sizeof(RECT) );
            return sizeof(RECT);
        }
        else
        {
            NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS *)lparam;
            data->ps.ncp.rgrc[0]         = ncp->rgrc[0];
            data->ps.ncp.rgrc[1]         = ncp->rgrc[1];
            data->ps.ncp.rgrc[2]         = ncp->rgrc[2];
            data->ps.ncp.hwnd            = wine_server_user_handle( ncp->lppos->hwnd );
            data->ps.ncp.hwndInsertAfter = wine_server_user_handle( ncp->lppos->hwndInsertAfter );
            data->ps.ncp.x               = ncp->lppos->x;
            data->ps.ncp.y               = ncp->lppos->y;
            data->ps.ncp.cx              = ncp->lppos->cx;
            data->ps.ncp.cy              = ncp->lppos->cy;
            data->ps.ncp.flags           = ncp->lppos->flags;
            push_data( data, &data->ps.ncp, sizeof(data->ps.ncp) );
            return sizeof(data->ps.ncp);
        }
    case WM_GETDLGCODE:
        if (lparam)
        {
            MSG *msg = (MSG *)lparam;
            data->ps.msg.hwnd    = wine_server_user_handle( msg->hwnd );
            data->ps.msg.message = msg->message;
            data->ps.msg.wParam  = msg->wParam;
            data->ps.msg.lParam  = msg->lParam;
            data->ps.msg.time    = msg->time;
            data->ps.msg.pt      = msg->pt;
            push_data( data, &data->ps.msg, sizeof(data->ps.msg) );
            return sizeof(data->ps.msg);
        }
        return 0;
    case SBM_SETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        return 0;
    case SBM_GETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        return sizeof(SCROLLINFO);
    case SBM_GETSCROLLBARINFO:
    {
        const SCROLLBARINFO *info = (const SCROLLBARINFO *)lparam;
        size_t size = min( info->cbSize, sizeof(SCROLLBARINFO) );
        push_data( data, info, size );
        return size;
    }
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
    {
        size_t size = 0;
        if (wparam) size += sizeof(DWORD);
        if (lparam) size += sizeof(DWORD);
        return size;
    }
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        return sizeof(RECT);
    case EM_SETRECT:
    case EM_SETRECTNP:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        return 0;
    case EM_GETLINE:
    {
        WORD *pw = (WORD *)lparam;
        push_data( data, pw, sizeof(*pw) );
        return *pw * sizeof(WCHAR);
    }
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        if (wparam) push_data( data, (UINT *)lparam, sizeof(UINT) * wparam );
        return 0;
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (combobox_has_strings( hwnd )) push_string( data, (LPWSTR)lparam );
        return 0;
    case CB_GETLBTEXT:
        if (!combobox_has_strings( hwnd )) return sizeof(ULONG_PTR);
        return (send_message( hwnd, CB_GETLBTEXTLEN, wparam, 0 ) + 1) * sizeof(WCHAR);
    case CB_GETCOMBOBOXINFO:
        return sizeof(data->ps.cbi);
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (listbox_has_strings( hwnd )) push_string( data, (LPWSTR)lparam );
        return 0;
    case LB_GETTEXT:
        if (!listbox_has_strings( hwnd )) return sizeof(ULONG_PTR);
        return (send_message( hwnd, LB_GETTEXTLEN, wparam, 0 ) + 1) * sizeof(WCHAR);
    case LB_GETSELITEMS:
        return wparam * sizeof(UINT);
    case WM_NEXTMENU:
    {
        MDINEXTMENU *mnm = (MDINEXTMENU *)lparam;
        data->ps.mnm.hmenuIn   = wine_server_user_handle( mnm->hmenuIn );
        data->ps.mnm.hmenuNext = wine_server_user_handle( mnm->hmenuNext );
        data->ps.mnm.hwndNext  = wine_server_user_handle( mnm->hwndNext );
        push_data( data, &data->ps.mnm, sizeof(data->ps.mnm) );
        return sizeof(data->ps.mnm);
    }
    case WM_SIZING:
    case WM_MOVING:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        return sizeof(RECT);
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *mcs = (MDICREATESTRUCTW *)lparam;
        data->ps.mcs.szClass = pack_ptr( mcs->szClass );
        data->ps.mcs.szTitle = pack_ptr( mcs->szTitle );
        data->ps.mcs.hOwner  = pack_ptr( mcs->hOwner );
        data->ps.mcs.x       = mcs->x;
        data->ps.mcs.y       = mcs->y;
        data->ps.mcs.cx      = mcs->cx;
        data->ps.mcs.cy      = mcs->cy;
        data->ps.mcs.style   = mcs->style;
        data->ps.mcs.lParam  = mcs->lParam;
        push_data( data, &data->ps.mcs, sizeof(data->ps.mcs) );
        if (!IS_INTRESOURCE(mcs->szClass)) push_string( data, mcs->szClass );
        if (!IS_INTRESOURCE(mcs->szTitle)) push_string( data, mcs->szTitle );
        return sizeof(data->ps.mcs);
    }
    case WM_MDIGETACTIVE:
        if (lparam) return sizeof(BOOL);
        return 0;
    case WM_DEVICECHANGE:
    {
        DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)lparam;
        if ((wparam & 0x8000) && header) push_data( data, header, header->dbch_size );
        return 0;
    }
    case WM_WINE_KEYBOARD_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;
        data->ps.hook.handle = wine_server_user_handle( h_extra->handle );
        push_data( data, &data->ps.hook, sizeof(data->ps.hook) );
        push_data( data, (LPVOID)h_extra->lparam, sizeof(KBDLLHOOKSTRUCT) );
        return 0;
    }
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;
        data->ps.hook.handle = wine_server_user_handle( h_extra->handle );
        push_data( data, &data->ps.hook, sizeof(data->ps.hook) );
        push_data( data, (LPVOID)h_extra->lparam, sizeof(MSLLHOOKSTRUCT) );
        return 0;
    }
    case WM_NCPAINT:
        if (wparam <= 1) return 0;
        FIXME( "WM_NCPAINT hdc packing not supported yet\n" );
        data->count = -1;
        return 0;
    case WM_PAINT:
        if (!wparam) return 0;
        /* fall through */

    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_PRINT:
    case WM_PRINTCLIENT:
    /* these contain an HGLOBAL */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    /* these contain HICON */
    case WM_GETICON:
    case WM_SETICON:
    case WM_QUERYDRAGICON:
    case WM_QUERYPARKICON:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
        FIXME( "msg %x (%s) not supported yet\n", message, debugstr_msg_name(message, hwnd) );
        data->count = -1;
        return 0;
    }
    return 0;
}

/***********************************************************************
 *           pack_reply
 *
 * Pack a reply to a message for sending to another process.
 */
static void pack_reply( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                        LRESULT res, struct packed_message *data )
{
    data->count = 0;
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        data->ps.cs.lpCreateParams = (ULONG_PTR)cs->lpCreateParams;
        data->ps.cs.hInstance      = (ULONG_PTR)cs->hInstance;
        data->ps.cs.hMenu          = wine_server_user_handle( cs->hMenu );
        data->ps.cs.hwndParent     = wine_server_user_handle( cs->hwndParent );
        data->ps.cs.cy             = cs->cy;
        data->ps.cs.cx             = cs->cx;
        data->ps.cs.y              = cs->y;
        data->ps.cs.x              = cs->x;
        data->ps.cs.style          = cs->style;
        data->ps.cs.dwExStyle      = cs->dwExStyle;
        data->ps.cs.lpszName       = (ULONG_PTR)cs->lpszName;
        data->ps.cs.lpszClass      = (ULONG_PTR)cs->lpszClass;
        push_data( data, &data->ps.cs, sizeof(data->ps.cs) );
        break;
    }
    case WM_GETTEXT:
    case CB_GETLBTEXT:
    case LB_GETTEXT:
        push_data( data, (WCHAR *)lparam, (res + 1) * sizeof(WCHAR) );
        break;
    case CB_GETCOMBOBOXINFO:
    {
        COMBOBOXINFO *cbi = (COMBOBOXINFO *)lparam;
        data->ps.cbi.rcItem         = cbi->rcItem;
        data->ps.cbi.rcButton       = cbi->rcButton;
        data->ps.cbi.stateButton    = cbi->stateButton;
        data->ps.cbi.hwndCombo      = wine_server_user_handle( cbi->hwndCombo );
        data->ps.cbi.hwndItem       = wine_server_user_handle( cbi->hwndItem );
        data->ps.cbi.hwndList       = wine_server_user_handle( cbi->hwndList );
        push_data( data, &data->ps.cbi, sizeof(data->ps.cbi) );
        break;
    }
    case WM_GETMINMAXINFO:
        push_data( data, (MINMAXINFO *)lparam, sizeof(MINMAXINFO) );
        break;
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lparam;
        data->ps.mis.CtlType    = mis->CtlType;
        data->ps.mis.CtlID      = mis->CtlID;
        data->ps.mis.itemID     = mis->itemID;
        data->ps.mis.itemWidth  = mis->itemWidth;
        data->ps.mis.itemHeight = mis->itemHeight;
        data->ps.mis.itemData   = mis->itemData;
        push_data( data, &data->ps.mis, sizeof(data->ps.mis) );
        break;
    }
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS *wp = (WINDOWPOS *)lparam;
        data->ps.wp.hwnd            = wine_server_user_handle( wp->hwnd );
        data->ps.wp.hwndInsertAfter = wine_server_user_handle( wp->hwndInsertAfter );
        data->ps.wp.x               = wp->x;
        data->ps.wp.y               = wp->y;
        data->ps.wp.cx              = wp->cx;
        data->ps.wp.cy              = wp->cy;
        data->ps.wp.flags           = wp->flags;
        push_data( data, &data->ps.wp, sizeof(data->ps.wp) );
        break;
    }
    case SBM_GETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        break;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case WM_SIZING:
    case WM_MOVING:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        break;
    case EM_GETLINE:
    {
        WORD *ptr = (WORD *)lparam;
        push_data( data, ptr, ptr[-1] * sizeof(WCHAR) );
        break;
    }
    case LB_GETSELITEMS:
        push_data( data, (UINT *)lparam, wparam * sizeof(UINT) );
        break;
    case WM_MDIGETACTIVE:
        if (lparam) push_data( data, (BOOL *)lparam, sizeof(BOOL) );
        break;
    case WM_NCCALCSIZE:
        if (!wparam)
            push_data( data, (RECT *)lparam, sizeof(RECT) );
        else
        {
            NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS *)lparam;
            data->ps.ncp.rgrc[0]         = ncp->rgrc[0];
            data->ps.ncp.rgrc[1]         = ncp->rgrc[1];
            data->ps.ncp.rgrc[2]         = ncp->rgrc[2];
            data->ps.ncp.hwnd            = wine_server_user_handle( ncp->lppos->hwnd );
            data->ps.ncp.hwndInsertAfter = wine_server_user_handle( ncp->lppos->hwndInsertAfter );
            data->ps.ncp.x               = ncp->lppos->x;
            data->ps.ncp.y               = ncp->lppos->y;
            data->ps.ncp.cx              = ncp->lppos->cx;
            data->ps.ncp.cy              = ncp->lppos->cy;
            data->ps.ncp.flags           = ncp->lppos->flags;
            push_data( data, &data->ps.ncp, sizeof(data->ps.ncp) );
        }
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam) push_data( data, (DWORD *)wparam, sizeof(DWORD) );
        if (lparam) push_data( data, (DWORD *)lparam, sizeof(DWORD) );
        break;
    case WM_NEXTMENU:
    {
        MDINEXTMENU *mnm = (MDINEXTMENU *)lparam;
        data->ps.mnm.hmenuIn   = wine_server_user_handle( mnm->hmenuIn );
        data->ps.mnm.hmenuNext = wine_server_user_handle( mnm->hmenuNext );
        data->ps.mnm.hwndNext  = wine_server_user_handle( mnm->hwndNext );
        push_data( data, &data->ps.mnm, sizeof(data->ps.mnm) );
        break;
    }
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *mcs = (MDICREATESTRUCTW *)lparam;
        data->ps.mcs.szClass = pack_ptr( mcs->szClass );
        data->ps.mcs.szTitle = pack_ptr( mcs->szTitle );
        data->ps.mcs.hOwner  = pack_ptr( mcs->hOwner );
        data->ps.mcs.x       = mcs->x;
        data->ps.mcs.y       = mcs->y;
        data->ps.mcs.cx      = mcs->cx;
        data->ps.mcs.cy      = mcs->cy;
        data->ps.mcs.style   = mcs->style;
        data->ps.mcs.lParam  = mcs->lParam;
        push_data( data, &data->ps.mcs, sizeof(data->ps.mcs) );
        break;
    }
    case WM_ASKCBFORMATNAME:
        push_data( data, (WCHAR *)lparam, (lstrlenW((WCHAR *)lparam) + 1) * sizeof(WCHAR) );
        break;
    }
}

/***********************************************************************
 *           unpack_reply
 *
 * Unpack a message reply received from another process.
 */
static void unpack_reply( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                          void *buffer, size_t size )
{
    union packed_structs *ps = buffer;

    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        if (size >= sizeof(ps->cs))
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
            cs->lpCreateParams = unpack_ptr( ps->cs.lpCreateParams );
            cs->hInstance      = unpack_ptr( ps->cs.hInstance );
            cs->hMenu          = wine_server_ptr_handle( ps->cs.hMenu );
            cs->hwndParent     = wine_server_ptr_handle( ps->cs.hwndParent );
            cs->cy             = ps->cs.cy;
            cs->cx             = ps->cs.cx;
            cs->y              = ps->cs.y;
            cs->x              = ps->cs.x;
            cs->style          = ps->cs.style;
            cs->dwExStyle      = ps->cs.dwExStyle;
            /* don't allow changing name and class pointers */
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        memcpy( (WCHAR *)lparam, buffer, min( wparam*sizeof(WCHAR), size ));
        break;
    case WM_GETMINMAXINFO:
        memcpy( (MINMAXINFO *)lparam, buffer, min( sizeof(MINMAXINFO), size ));
        break;
    case WM_MEASUREITEM:
        if (size >= sizeof(ps->mis))
        {
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lparam;
            mis->CtlType    = ps->mis.CtlType;
            mis->CtlID      = ps->mis.CtlID;
            mis->itemID     = ps->mis.itemID;
            mis->itemWidth  = ps->mis.itemWidth;
            mis->itemHeight = ps->mis.itemHeight;
            mis->itemData   = (ULONG_PTR)unpack_ptr( ps->mis.itemData );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        if (size >= sizeof(ps->wp))
        {
            WINDOWPOS *wp = (WINDOWPOS *)lparam;
            wp->hwnd            = wine_server_ptr_handle( ps->wp.hwnd );
            wp->hwndInsertAfter = wine_server_ptr_handle( ps->wp.hwndInsertAfter );
            wp->x               = ps->wp.x;
            wp->y               = ps->wp.y;
            wp->cx              = ps->wp.cx;
            wp->cy              = ps->wp.cy;
            wp->flags           = ps->wp.flags;
        }
        break;
    case SBM_GETSCROLLINFO:
        memcpy( (SCROLLINFO *)lparam, buffer, min( sizeof(SCROLLINFO), size ));
        break;
    case SBM_GETSCROLLBARINFO:
        memcpy( (SCROLLBARINFO *)lparam, buffer, min( sizeof(SCROLLBARINFO), size ));
        break;
    case EM_GETRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case LB_GETITEMRECT:
    case WM_SIZING:
    case WM_MOVING:
        memcpy( (RECT *)lparam, buffer, min( sizeof(RECT), size ));
        break;
    case EM_GETLINE:
        size = min( size, (size_t)*(WORD *)lparam );
        memcpy( (WCHAR *)lparam, buffer, size );
        break;
    case LB_GETSELITEMS:
        memcpy( (UINT *)lparam, buffer, min( wparam*sizeof(UINT), size ));
        break;
    case LB_GETTEXT:
    case CB_GETLBTEXT:
        memcpy( (WCHAR *)lparam, buffer, size );
        break;
    case CB_GETCOMBOBOXINFO:
        if (lparam && size >= sizeof(ps->cbi) &&
            ((COMBOBOXINFO*)lparam)->cbSize == sizeof(COMBOBOXINFO))
        {
            COMBOBOXINFO *cbi = (COMBOBOXINFO *)lparam;
            cbi->rcItem         = ps->cbi.rcItem;
            cbi->rcButton       = ps->cbi.rcButton;
            cbi->stateButton    = ps->cbi.stateButton;
            cbi->hwndCombo      = wine_server_ptr_handle( ps->cbi.hwndCombo );
            cbi->hwndItem       = wine_server_ptr_handle( ps->cbi.hwndItem );
            cbi->hwndList       = wine_server_ptr_handle( ps->cbi.hwndList );
        }
        break;
    case WM_NEXTMENU:
        if (size >= sizeof(ps->mnm))
        {
            MDINEXTMENU *mnm = (MDINEXTMENU *)lparam;
            mnm->hmenuIn   = wine_server_ptr_handle( ps->mnm.hmenuIn );
            mnm->hmenuNext = wine_server_ptr_handle( ps->mnm.hmenuNext );
            mnm->hwndNext  = wine_server_ptr_handle( ps->mnm.hwndNext );
        }
        break;
    case WM_MDIGETACTIVE:
        if (lparam) memcpy( (BOOL *)lparam, buffer, min( sizeof(BOOL), size ));
        break;
    case WM_NCCALCSIZE:
        if (!wparam)
            memcpy( (RECT *)lparam, buffer, min( sizeof(RECT), size ));
        else if (size >= sizeof(ps->ncp))
        {
            NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS *)lparam;
            ncp->rgrc[0]                = ps->ncp.rgrc[0];
            ncp->rgrc[1]                = ps->ncp.rgrc[1];
            ncp->rgrc[2]                = ps->ncp.rgrc[2];
            ncp->lppos->hwnd            = wine_server_ptr_handle( ps->ncp.hwnd );
            ncp->lppos->hwndInsertAfter = wine_server_ptr_handle( ps->ncp.hwndInsertAfter );
            ncp->lppos->x               = ps->ncp.x;
            ncp->lppos->y               = ps->ncp.y;
            ncp->lppos->cx              = ps->ncp.cx;
            ncp->lppos->cy              = ps->ncp.cy;
            ncp->lppos->flags           = ps->ncp.flags;
        }
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam)
        {
            memcpy( (DWORD *)wparam, buffer, min( sizeof(DWORD), size ));
            if (size <= sizeof(DWORD)) break;
            size -= sizeof(DWORD);
            buffer = (DWORD *)buffer + 1;
        }
        if (lparam) memcpy( (DWORD *)lparam, buffer, min( sizeof(DWORD), size ));
        break;
    case WM_MDICREATE:
        if (size >= sizeof(ps->mcs))
        {
            MDICREATESTRUCTW *mcs = (MDICREATESTRUCTW *)lparam;
            mcs->hOwner  = unpack_ptr( ps->mcs.hOwner );
            mcs->x       = ps->mcs.x;
            mcs->y       = ps->mcs.y;
            mcs->cx      = ps->mcs.cx;
            mcs->cy      = ps->mcs.cy;
            mcs->style   = ps->mcs.style;
            mcs->lParam  = (LPARAM)unpack_ptr( ps->mcs.lParam );
            /* don't allow changing class and title pointers */
        }
        break;
    default:
        ERR( "should not happen: unexpected message %x\n", message );
        break;
    }
}

static size_t char_size( BOOL ansi )
{
    return ansi ? sizeof(char) : sizeof(WCHAR);
}

static size_t string_size( const void *str, BOOL ansi )
{
    return ansi ? strlen( str ) + 1 : (wcslen( str ) + 1) * sizeof(WCHAR);
}

static size_t copy_string( void *ptr, const void *str, BOOL ansi )
{
    size_t size = string_size( str, ansi );
    memcpy( ptr, str, size );
    return size;
}

/***********************************************************************
 *           user_message_size
 *
 * Calculate size of packed message buffer.
 */
size_t user_message_size( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                          BOOL other_process, BOOL ansi, size_t *reply_size )
{
    const void *lparam_ptr = (const void *)lparam;
    size_t size = 0;

    /* Windows provices space for at least 2048 bytes for string getters, which
     * mitigates problems with buffer overflows. */
    static const size_t min_string_buffer_size = 2048;

    switch (message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        const CREATESTRUCTW *cs = lparam_ptr;
        size = sizeof(*cs);
        if (!IS_INTRESOURCE(cs->lpszName))  size += string_size( cs->lpszName, ansi );
        if (!IS_INTRESOURCE(cs->lpszClass)) size += string_size( cs->lpszClass, ansi );
        break;
    }
    case WM_NCCALCSIZE:
        size = wparam ? sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) : sizeof(RECT);
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        *reply_size = wparam * char_size( ansi );
        return max( *reply_size, min_string_buffer_size );
    case WM_WININICHANGE:
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (other_process && lparam) size = string_size( lparam_ptr, ansi );
        break;
    case WM_GETMINMAXINFO:
        size = sizeof(MINMAXINFO);
        break;
    case WM_DRAWITEM:
        size = sizeof(DRAWITEMSTRUCT);
        break;
    case WM_MEASUREITEM:
        size = sizeof(MEASUREITEMSTRUCT);
        break;
    case WM_DELETEITEM:
        size = sizeof(DELETEITEMSTRUCT);
        break;
    case WM_COMPAREITEM:
        size = sizeof(COMPAREITEMSTRUCT);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        size = sizeof(WINDOWPOS);
        break;
    case WM_COPYDATA:
    {
        const COPYDATASTRUCT *cds = lparam_ptr;
        /* If cbData <= 2048 bytes, pack the data at the end of the message. Otherwise, pack the data
         * in an extra user buffer to avoid potential stack overflows when calling KeUserModeCallback()
         * because cbData can be very large. Manual tests of KiUserCallbackDispatcher() when receiving
         * WM_COPYDATA messages show that Windows does similar things */
        size = cds->cbData <= 2048 ? sizeof(*cds) + cds->cbData : sizeof(*cds);
        break;
    }
    case WM_HELP:
        size = sizeof(HELPINFO);
        break;
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        size = sizeof(STYLESTRUCT);
        break;
    case WM_GETDLGCODE:
        if (lparam) size = sizeof(MSG);
        break;
    case SBM_SETSCROLLINFO:
    case SBM_GETSCROLLINFO:
        size = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLBARINFO:
        size = sizeof(SCROLLBARINFO);
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        size = 2 * sizeof(DWORD);
        break;
    case WM_SIZING:
    case WM_MOVING:
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case EM_SETRECT:
    case EM_SETRECTNP:
        size = sizeof(RECT);
        break;
    case EM_GETLINE:
        size = max( *(WORD *)lparam * char_size( ansi ), sizeof(WORD) );
        break;
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        size = wparam * sizeof(UINT);
        break;
    case CB_GETLBTEXT:
    {
        size_t len = send_message_timeout( hwnd, CB_GETLBTEXTLEN, wparam, 0, SMTO_NORMAL, 0, ansi );
        *reply_size = (len + 1) * char_size( ansi );
        return max( *reply_size, min_string_buffer_size );
    }
    case LB_GETTEXT:
    {
        size_t len = send_message_timeout( hwnd, LB_GETTEXTLEN, wparam, 0, SMTO_NORMAL, 0, ansi );
        *reply_size = (len + 1) * char_size( ansi );
        return max( *reply_size, min_string_buffer_size );
    }
    case LB_GETSELITEMS:
        size = wparam * sizeof(UINT);
        break;
    case WM_NEXTMENU:
        size = sizeof(MDINEXTMENU);
        break;
    case WM_MDICREATE:
    {
        const MDICREATESTRUCTW *mcs = lparam_ptr;
        size = sizeof(*mcs);
        if (!IS_INTRESOURCE(mcs->szClass)) size += string_size( mcs->szClass, ansi );
        if (!IS_INTRESOURCE(mcs->szTitle)) size += string_size( mcs->szTitle, ansi );
        break;
    }
    case CB_GETCOMBOBOXINFO:
        size = sizeof(COMBOBOXINFO);
        break;
    case WM_MDIGETACTIVE:
        if (lparam) size = sizeof(BOOL);
        break;
    case WM_DEVICECHANGE:
        if ((wparam & 0x8000) && lparam)
        {
            const DEV_BROADCAST_HDR *header = lparam_ptr;
            size = header->dbch_size;
        }
        break;
    }

    return *reply_size = size;
}

/***********************************************************************
 *           pack_user_message
 *
 * Copy message to a buffer for passing to client.
 *
 * ret_extra_buffer returns an extra user buffer allocated to store large message data, for example,
 * for WM_COPYDATA. Call NtFreeVirtualMemory() for *ret_extra_buffer when done using it if it's not NULL.
 */
void pack_user_message( void *buffer, size_t size, UINT message,
                        WPARAM wparam, LPARAM lparam, BOOL ansi, void **ret_extra_buffer )
{
    const void *lparam_ptr = (const void *)lparam;
    void const *inline_ptr = (void *)0xffffffff;

    if (!size) return;

    *ret_extra_buffer = NULL;

    switch (message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = buffer;
        char *ptr = (char *)(cs + 1);

        memcpy( cs, lparam_ptr, sizeof(*cs) );
        if (!IS_INTRESOURCE(cs->lpszName))
        {
            ptr += copy_string( ptr, cs->lpszName, ansi );
            cs->lpszName  = inline_ptr;
        }
        if (!IS_INTRESOURCE(cs->lpszClass))
        {
            copy_string( ptr, cs->lpszClass, ansi );
            cs->lpszClass = inline_ptr;
        }
        return;
    }
    case WM_NCCALCSIZE:
        if (wparam)
        {
            const NCCALCSIZE_PARAMS *ncp = lparam_ptr;
            memcpy( (char *)buffer + sizeof(*ncp), ncp->lppos, sizeof(*ncp->lppos) );
            size = sizeof(*ncp);
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        if (wparam) memset( buffer, 0, char_size( ansi ));
        return;
    case WM_COPYDATA:
    {
        const COPYDATASTRUCT *cds = lparam_ptr;

        if (!cds->lpData)
        {
            size = sizeof(*cds);
        }
        else
        {
            /* If cbData <= 2048 bytes, pack the data at the end of the message. Otherwise, pack the data
             * in an extra user buffer to avoid potential stack overflow when calling KeUserModeCallback()
             * because cbData can be very large. Manual tests of KiUserCallbackDispatcher() when receiving
             * WM_COPYDATA messages show that Windows does similar things */
            if (cds->cbData <= 2048)
            {
                memcpy( (char *)buffer + sizeof(*cds), cds->lpData, cds->cbData );
                size = sizeof(*cds);
            }
            else
            {
                COPYDATASTRUCT *tmp_cds = buffer;
                SIZE_T extra_buffer_size;
                unsigned int status;

                memcpy( tmp_cds, cds, sizeof(*cds) );

                extra_buffer_size = cds->cbData;
                status = NtAllocateVirtualMemory( GetCurrentProcess(), ret_extra_buffer, zero_bits,
                                                  &extra_buffer_size, MEM_RESERVE | MEM_COMMIT,
                                                  PAGE_READWRITE );
                if (!status)
                {
                    memcpy( *ret_extra_buffer, cds->lpData, cds->cbData );
                    tmp_cds->lpData = *ret_extra_buffer;
                }
                else
                {
                    tmp_cds->lpData = NULL;
                    tmp_cds->cbData = 0;
                }
                return;
            }
        }
        break;
    }
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
    case EM_GETRECT:
    case CB_GETDROPPEDCONTROLRECT:
        return;
    case EM_GETLINE:
        size = sizeof(WORD);
        break;
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *mcs = buffer;
        char *ptr = (char *)(mcs + 1);

        memcpy( buffer, lparam_ptr, sizeof(*mcs) );
        if (!IS_INTRESOURCE(mcs->szClass))
        {
            ptr += copy_string( ptr, mcs->szClass, ansi );
            mcs->szClass = inline_ptr;
        }
        if (!IS_INTRESOURCE(mcs->szTitle))
        {
            copy_string( ptr, mcs->szTitle, ansi );
            mcs->szTitle = inline_ptr;
        }
        return;
    }
    case CB_GETCOMBOBOXINFO:
        if (sizeof(void *) == 4)
        {
            COMBOBOXINFO *cbi = buffer;
            memcpy( cbi, lparam_ptr, sizeof(*cbi) );
            cbi->cbSize = sizeof(*cbi);
            return;
        }
        break;
    case CB_GETLBTEXT:
    case LB_GETTEXT:
        if (size) memset( buffer, 0, size );
        return;
    }

    if (size) memcpy( buffer, lparam_ptr, size );
}

/***********************************************************************
 *           copy_user_result
 *
 * Copy a message result received from client.
 */
static void copy_user_result( void *buffer, size_t size, LRESULT result, UINT message,
                              WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    void *lparam_ptr = (void *)lparam;
    size_t copy_size = 0;

    if (!size) return;

    switch (message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        if (size >= sizeof(CREATESTRUCTW))
        {
            CREATESTRUCTW *dst = lparam_ptr;
            const CREATESTRUCTW *src = buffer;
            dst->lpCreateParams = src->lpCreateParams;
            dst->hInstance      = src->hInstance;
            dst->hMenu          = src->hMenu;
            dst->hwndParent     = src->hwndParent;
            dst->cy             = src->cy;
            dst->cx             = src->cx;
            dst->y              = src->y;
            dst->x              = src->x;
            dst->style          = src->style;
            dst->dwExStyle      = src->dwExStyle;
            /* don't allow changing name and class pointers */
        }
        return;
    case WM_NCCALCSIZE:
        if (wparam)
        {
            NCCALCSIZE_PARAMS *dst = lparam_ptr;
            const NCCALCSIZE_PARAMS *src = buffer;
            const WINDOWPOS *winpos = (const WINDOWPOS *)(src + 1);
            dst->rgrc[0] = src->rgrc[0];
            dst->rgrc[1] = src->rgrc[1];
            dst->rgrc[2] = src->rgrc[2];
            *dst->lppos = *winpos;
            return;
        }
        copy_size = sizeof(RECT);
        break;
    case WM_GETTEXT:
        if (!result) memset( buffer, 0, char_size( ansi ));
        copy_size = string_size( buffer, ansi );
        break;
    case CB_GETLBTEXT:
    case LB_GETTEXT:
        copy_size = size;
        break;
    case WM_ASKCBFORMATNAME:
        copy_size = string_size( buffer, ansi );
        break;
    case WM_GETMINMAXINFO:
        copy_size = sizeof(MINMAXINFO);
        break;
    case WM_MEASUREITEM:
        copy_size = sizeof(MEASUREITEMSTRUCT);
        break;
    case WM_WINDOWPOSCHANGING:
        copy_size = sizeof(WINDOWPOS);
        break;
    case WM_STYLECHANGING:
        copy_size = sizeof(STYLESTRUCT);
        break;
    case SBM_SETSCROLLINFO:
    case SBM_GETSCROLLINFO:
        copy_size = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLBARINFO:
        copy_size = sizeof(SCROLLBARINFO);
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        {
            DWORD *ptr = buffer;
            if (wparam) *(DWORD *)wparam = ptr[0];
            if (lparam) *(DWORD *)lparam = ptr[1];
            break;
        }
    case WM_SIZING:
    case WM_MOVING:
    case EM_GETRECT:
    case EM_SETRECT:
    case EM_SETRECTNP:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        copy_size = sizeof(RECT);
        break;
    case EM_GETLINE:
        copy_size = string_size( buffer, ansi );
        break;
    case LB_GETSELITEMS:
        copy_size = wparam * sizeof(UINT);
        break;
    case WM_NEXTMENU:
        copy_size = sizeof(MDINEXTMENU);
        break;
    case CB_GETCOMBOBOXINFO:
        if (sizeof(void *) == 4)
        {
            COMBOBOXINFO *cbi = lparam_ptr;
            memcpy( cbi, buffer, size );
            cbi->cbSize = sizeof(*cbi);
            return;
        }
        copy_size = sizeof(COMBOBOXINFO);
        break;
    case WM_MDIGETACTIVE:
        if (lparam) copy_size = sizeof(BOOL);
        break;
    default:
        return;
    }

    if (copy_size > size) copy_size = size;
    if (copy_size) memcpy( lparam_ptr, buffer, copy_size );
}

/***********************************************************************
 *           reply_message
 *
 * Send a reply to a sent message.
 */
static void reply_message( struct received_message_info *info, LRESULT result, MSG *msg )
{
    struct packed_message data;
    int i, replied = info->flags & ISMEX_REPLIED;
    BOOL remove = msg != NULL;

    if (info->flags & ISMEX_NOTIFY) return;  /* notify messages don't get replies */
    if (!remove && replied) return;  /* replied already */

    memset( &data, 0, sizeof(data) );
    info->flags |= ISMEX_REPLIED;
    if (info == get_user_thread_info()->receive_info)
        NtUserGetThreadInfo()->receive_flags = info->flags;

    if (info->type == MSG_OTHER_PROCESS && !replied)
    {
        if (!msg) msg = &info->msg;
        pack_reply( msg->hwnd, msg->message, msg->wParam, msg->lParam, result, &data );
    }

    SERVER_START_REQ( reply_message )
    {
        req->result = result;
        req->remove = remove;
        for (i = 0; i < data.count; i++) wine_server_add_data( req, data.data[i], data.size[i] );
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

/***********************************************************************
 *           NtUserReplyMessage  (win32u.@)
 *
 * Send a reply to a sent message and update thread receive info.
 */
BOOL WINAPI NtUserReplyMessage( LRESULT result )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct received_message_info *info = thread_info->receive_info;

    if (!info) return FALSE;
    reply_message( info, result, NULL );
    return TRUE;
}

/***********************************************************************
 *           reply_winproc_result
 *
 * Send a reply to a sent message and update thread receive info.
 */
static BOOL reply_winproc_result( LRESULT result, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct received_message_info *info = thread_info->receive_info;
    MSG msg;

    if (!info) return FALSE;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = wparam;
    msg.lParam = lparam;
    reply_message( info, result, &msg );

    thread_info->receive_info = info->prev;
    thread_info->client_info.receive_flags = info->prev ? info->prev->flags : ISMEX_NOSEND;
    return TRUE;
}

/***********************************************************************
 *           handle_internal_message
 *
 * Handle an internal Wine message instead of calling the window proc.
 */
static LRESULT handle_internal_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch(msg)
    {
    case WM_WINE_DESTROYWINDOW:
        return destroy_window( hwnd );
    case WM_WINE_SETWINDOWPOS:
        if (is_desktop_window( hwnd )) return 0;
        return set_window_pos( (WINDOWPOS *)lparam, 0, 0 );
    case WM_WINE_SHOWWINDOW:
        if (is_desktop_window( hwnd )) return 0;
        return NtUserShowWindow( hwnd, wparam );
    case WM_WINE_SETPARENT:
        if (is_desktop_window( hwnd )) return 0;
        return HandleToUlong( NtUserSetParent( hwnd, UlongToHandle(wparam) ));
    case WM_WINE_SETWINDOWLONG:
        return set_window_long( hwnd, (short)LOWORD(wparam), HIWORD(wparam), lparam, FALSE );
    case WM_WINE_SETSTYLE:
        if (is_desktop_window( hwnd )) return 0;
        return set_window_style_bits( hwnd, wparam, lparam );
    case WM_WINE_SETACTIVEWINDOW:
    {
        HWND prev;

        if (!wparam && NtUserGetForegroundWindow() == hwnd) return 0;
        if (!set_active_window( (HWND)wparam, &prev, FALSE, TRUE, lparam )) return 0;
        return (LRESULT)prev;
    }
    case WM_WINE_KEYBOARD_LL_HOOK:
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;

        return call_current_hook( h_extra->handle, HC_ACTION, wparam, h_extra->lparam );
    }
    case WM_WINE_CLIPCURSOR:
        /* non-hardware message, posted on display mode change to trigger fullscreen
           clipping or to the desktop window to forcefully release the cursor grabs */
        if (wparam & SET_CURSOR_FSCLIP) return clip_fullscreen_window( hwnd, FALSE );
        return process_wine_clipcursor( hwnd, wparam, lparam );
    case WM_WINE_SETCURSOR:
        FIXME( "Unexpected non-hardware WM_WINE_SETCURSOR message\n" );
        return FALSE;
    case WM_WINE_IME_NOTIFY:
    {
        HWND ime_hwnd = get_default_ime_window( hwnd );
        if (!ime_hwnd || ime_hwnd == NtUserGetParent( hwnd )) return 0;
        return send_message( ime_hwnd, WM_IME_NOTIFY, wparam, lparam );
    }
    case WM_WINE_WINDOW_STATE_CHANGED:
    {
        UINT state_cmd, config_cmd;
        RECT window_rect;
        HWND foreground;

        if (!user_driver->pGetWindowStateUpdates( hwnd, &state_cmd, &config_cmd, &window_rect, &foreground )) return 0;
        if (foreground) NtUserSetForegroundWindow( foreground );
        if (state_cmd)
        {
            if (LOWORD(state_cmd) == SC_RESTORE && HIWORD(state_cmd)) NtUserSetActiveWindow( hwnd );
            send_message( hwnd, WM_SYSCOMMAND, LOWORD(state_cmd), 0 );

            /* state change might have changed the window config already, check again */
            user_driver->pGetWindowStateUpdates( hwnd, &state_cmd, &config_cmd, &window_rect, &foreground );
            if (foreground) NtUserSetForegroundWindow( foreground );
            if (state_cmd) WARN( "window %p state needs another update, ignoring\n", hwnd );
        }
        if (config_cmd)
        {
            if (LOWORD(config_cmd) == SC_MOVE) NtUserSetRawWindowPos( hwnd, window_rect, HIWORD(config_cmd), FALSE );
            else send_message( hwnd, WM_SYSCOMMAND, LOWORD(config_cmd), 0 );
        }
        return 0;
    }
    case WM_WINE_UPDATEWINDOWSTATE:
        update_window_state( hwnd );
        return 0;
    case WM_WINE_TRACKMOUSEEVENT:
    {
        TRACKMOUSEEVENT info;

        info.cbSize = sizeof(info);
        info.hwndTrack = hwnd;
        info.dwFlags = wparam;
        info.dwHoverTime = lparam;
        return NtUserTrackMouseEvent( &info );
    }
    default:
        if (msg >= WM_WINE_FIRST_DRIVER_MSG && msg <= WM_WINE_LAST_DRIVER_MSG)
            return user_driver->pWindowMessage( hwnd, msg, wparam, lparam );
        FIXME( "unknown internal message %x\n", msg );
        return 0;
    }
}

/**********************************************************************
 *           NtUserGetGUIThreadInfo  (win32u.@)
 */
BOOL WINAPI NtUserGetGUIThreadInfo( DWORD id, GUITHREADINFO *info )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const input_shm_t *input_shm;
    NTSTATUS status;

    if (info->cbSize != sizeof(*info))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    while ((status = get_shared_input( id, &lock, &input_shm )) == STATUS_PENDING)
    {
        info->flags          = 0;
        info->hwndActive     = wine_server_ptr_handle( input_shm->active );
        info->hwndFocus      = wine_server_ptr_handle( input_shm->focus );
        info->hwndCapture    = wine_server_ptr_handle( input_shm->capture );
        info->hwndMenuOwner  = wine_server_ptr_handle( input_shm->menu_owner );
        info->hwndMoveSize   = wine_server_ptr_handle( input_shm->move_size );
        info->hwndCaret      = wine_server_ptr_handle( input_shm->caret );
        info->rcCaret        = wine_server_get_rect( input_shm->caret_rect );
        if (input_shm->menu_owner) info->flags |= GUI_INMENUMODE;
        if (input_shm->move_size) info->flags |= GUI_INMOVESIZE;
        if (input_shm->caret) info->flags |= GUI_CARETBLINKING;
    }

    if (status)
    {
        info->flags = 0;
        info->hwndActive = 0;
        info->hwndFocus = 0;
        info->hwndCapture = 0;
        info->hwndMenuOwner = 0;
        info->hwndMoveSize = 0;
        info->hwndCaret = 0;
        memset( &info->rcCaret, 0, sizeof(info->rcCaret) );
    }

    return TRUE;
}

/***********************************************************************
 *           call_window_proc
 *
 * Call a window procedure and the corresponding hooks.
 */
static LRESULT call_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                 enum message_type type, BOOL same_thread,
                                 enum wm_char_mapping mapping, BOOL ansi_dst )
{
    struct win_proc_params p, *params = &p;
    BOOL ansi = ansi_dst && type == MSG_ASCII;
    size_t packed_size = 0, offset = sizeof(*params), reply_size;
    void *ret_ptr, *extra_buffer = NULL;
    SIZE_T extra_buffer_size = 0;
    LRESULT result = 0;
    CWPSTRUCT cwp;
    CWPRETSTRUCT cwpret;
    size_t ret_len = 0;

    if (msg & 0x80000000)
        return handle_internal_message( hwnd, msg, wparam, lparam );

    if (!is_current_thread_window( hwnd )) return 0;

    packed_size = user_message_size( hwnd, msg, wparam, lparam, type == MSG_OTHER_PROCESS, ansi, &reply_size );

    /* first the WH_CALLWNDPROC hook */
    cwp.lParam  = lparam;
    cwp.wParam  = wparam;
    cwp.message = msg;
    cwp.hwnd    = hwnd = get_full_window_handle( hwnd );
    call_message_hooks( WH_CALLWNDPROC, HC_ACTION, same_thread, (LPARAM)&cwp, sizeof(cwp),
                        packed_size, ansi );

    if (packed_size)
    {
        offset = (offset + 15) & ~15;
        if (!(params = malloc( offset + packed_size ))) return 0;
    }

    if (!init_window_call_params( params, hwnd, msg, wparam, lparam, ansi_dst, mapping ))
    {
        if (params != &p) free( params );
        return 0;
    }

    if (type == MSG_OTHER_PROCESS) params->ansi = FALSE;
    if (packed_size)
        pack_user_message( (char *)params + offset, packed_size, msg, wparam, lparam, ansi, &extra_buffer );

    result = dispatch_win_proc_params( params, offset + packed_size, &ret_ptr, &ret_len );
    if (params != &p) free( params );
    if (extra_buffer)
        NtFreeVirtualMemory( GetCurrentProcess(), &extra_buffer, &extra_buffer_size, MEM_RELEASE );

    copy_user_result( ret_ptr, min( ret_len, reply_size ), result, msg, wparam, lparam, ansi );

    /* and finally the WH_CALLWNDPROCRET hook */
    cwpret.lResult = result;
    cwpret.lParam  = lparam;
    cwpret.wParam  = wparam;
    cwpret.message = msg;
    cwpret.hwnd    = hwnd;
    call_message_hooks( WH_CALLWNDPROCRET, HC_ACTION, same_thread, (LPARAM)&cwpret, sizeof(cwpret),
                        packed_size, ansi );
    return result;
}

/***********************************************************************
 *           call_sendmsg_callback
 *
 * Call the callback function of SendMessageCallback.
 */
static inline void call_sendmsg_callback( SENDASYNCPROC callback, HWND hwnd, UINT msg,
                                          ULONG_PTR data, LRESULT result )
{
    struct send_async_params params;
    void *ret_ptr;
    ULONG ret_len;

    if (!callback) return;

    params.callback = callback;
    params.hwnd     = hwnd;
    params.msg      = msg;
    params.data     = data;
    params.result   = result;

    TRACE_(relay)( "\1Call message callback %p (hwnd=%p,msg=%s,data=%08lx,result=%08lx)\n",
                   callback, hwnd, debugstr_msg_name( msg, hwnd ), data, result );

    KeUserModeCallback( NtUserCallSendAsyncCallback, &params, sizeof(params), &ret_ptr, &ret_len );

    TRACE_(relay)( "\1Ret  message callback %p (hwnd=%p,msg=%s,data=%08lx,result=%08lx)\n",
                   callback, hwnd, debugstr_msg_name( msg, hwnd ), data, result );
}

/***********************************************************************
 *          accept_hardware_message
 *
 * Tell the server we have passed the message to the app
 * (even though we may end up dropping it later on)
 */
static void accept_hardware_message( UINT hw_id )
{
    SERVER_START_REQ( accept_hardware_message )
    {
        req->hw_id = hw_id;
        if (wine_server_call( req ))
            FIXME("Failed to reply to MSG_HARDWARE message. Message may not be removed from queue.\n");
    }
    SERVER_END_REQ;
}

/***********************************************************************
 *           send_parent_notify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void send_parent_notify( HWND hwnd, WORD event, WORD idChild, POINT pt )
{
    /* pt has to be in the client coordinates of the parent window */
    map_window_points( 0, hwnd, &pt, 1, get_thread_dpi() );
    for (;;)
    {
        HWND parent;

        if (!(get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)) break;
        if (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY) break;
        if (!(parent = get_parent( hwnd ))) break;
        if (parent == get_desktop_window()) break;
        map_window_points( hwnd, parent, &pt, 1, get_thread_dpi() );
        hwnd = parent;
        send_message( hwnd, WM_PARENTNOTIFY,
                      MAKEWPARAM( event, idChild ), MAKELPARAM( pt.x, pt.y ) );
    }
}

/***********************************************************************
 *          process_pointer_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_pointer_message( MSG *msg, UINT hw_id, const struct hardware_msg_data *msg_data )
{
    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );
    return TRUE;
}

/***********************************************************************
 *          process_keyboard_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_keyboard_message( MSG *msg, UINT hw_id, HWND hwnd_filter,
                                      UINT first, UINT last, BOOL remove )
{
    EVENTMSG event;

    if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN ||
        msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
        switch (msg->wParam)
        {
            case VK_LSHIFT: case VK_RSHIFT:
                msg->wParam = VK_SHIFT;
                break;
            case VK_LCONTROL: case VK_RCONTROL:
                msg->wParam = VK_CONTROL;
                break;
            case VK_LMENU: case VK_RMENU:
                msg->wParam = VK_MENU;
                break;
        }

    /* FIXME: is this really the right place for this hook? */
    event.message = msg->message;
    event.hwnd    = msg->hwnd;
    event.time    = msg->time;
    event.paramL  = (msg->wParam & 0xFF) | (HIWORD(msg->lParam) << 8);
    event.paramH  = msg->lParam & 0x7FFF;
    if (HIWORD(msg->lParam) & 0x0100) event.paramH |= 0x8000; /* special_key - bit */
    call_hooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event, sizeof(event) );

    /* check message filters */
    if (msg->message < first || msg->message > last) return FALSE;
    if (!check_hwnd_filter( msg, hwnd_filter )) return FALSE;

    if (remove)
    {
        if((msg->message == WM_KEYDOWN) &&
           (msg->hwnd != get_desktop_window()))
        {
            /* Handle F1 key by sending out WM_HELP message */
            if (msg->wParam == VK_F1)
            {
                NtUserPostMessage( msg->hwnd, WM_KEYF1, 0, 0 );
            }
            else if(msg->wParam >= VK_BROWSER_BACK &&
                    msg->wParam <= VK_LAUNCH_APP2)
            {
                /* FIXME: Process keystate */
                send_message( msg->hwnd, WM_APPCOMMAND, (WPARAM)msg->hwnd,
                              MAKELPARAM(0, (FAPPCOMMAND_KEY | (msg->wParam - VK_BROWSER_BACK + 1))) );
            }
        }
        else if (msg->message == WM_KEYUP)
        {
            /* Handle VK_APPS key by posting a WM_CONTEXTMENU message */
            if (msg->wParam == VK_APPS && !is_menu_active())
                NtUserPostMessage( msg->hwnd, WM_CONTEXTMENU, (WPARAM)msg->hwnd, -1 );
        }
    }

    /* if remove is TRUE, remove message first before calling hooks to avoid recursive hook calls */
    if (remove) accept_hardware_message( hw_id );
    if (call_hooks( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
                    LOWORD(msg->wParam), msg->lParam, 0 ))
    {
        /* if the message has not been removed, remove it */
        if (!remove) accept_hardware_message( hw_id );
        /* skip this message */
        call_hooks( WH_CBT, HCBT_KEYSKIPPED, LOWORD(msg->wParam), msg->lParam, 0 );
        return FALSE;
    }
    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );

    if (remove && (msg->message == WM_KEYDOWN || msg->message == WM_KEYUP))
        if (ImmProcessKey( msg->hwnd, NtUserGetKeyboardLayout(0), msg->wParam, msg->lParam, 0 ))
            msg->wParam = VK_PROCESSKEY;

    return TRUE;
}

/***********************************************************************
 *          process_mouse_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_mouse_message( MSG *msg, UINT hw_id, ULONG_PTR extra_info, HWND hwnd_filter,
                                   UINT first, UINT last, BOOL remove )
{
    static MSG clk_msg;

    POINT pt;
    UINT message;
    INT hittest;
    EVENTMSG event;
    GUITHREADINFO info;
    MOUSEHOOKSTRUCTEX hook;
    BOOL eat_msg;
    WPARAM wparam;

    /* find the window to dispatch this mouse message to */

    info.cbSize = sizeof(info);
    NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info );
    if (info.hwndCapture)
    {
        hittest = HTCLIENT;
        msg->hwnd = info.hwndCapture;
    }
    else
    {
        HWND orig = msg->hwnd;

        msg->hwnd = window_from_point( msg->hwnd, msg->pt, &hittest, TRUE );
        if (!msg->hwnd) /* As a heuristic, try the next window if it's the owner of orig */
        {
            HWND next = get_window_relative( orig, GW_HWNDNEXT );

            if (next && get_window_relative( orig, GW_OWNER ) == next &&
                is_current_thread_window( next ))
                msg->hwnd = window_from_point( next, msg->pt, &hittest, TRUE );
        }
    }

    if (!msg->hwnd || !is_current_thread_window( msg->hwnd ))
    {
        accept_hardware_message( hw_id );
        return FALSE;
    }
    update_current_mouse_window( msg->hwnd, hittest, msg->pt );

    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );
    set_thread_dpi_awareness_context( get_window_dpi_awareness_context( msg->hwnd ));

    /* FIXME: is this really the right place for this hook? */
    event.message = msg->message;
    event.time    = msg->time;
    event.hwnd    = msg->hwnd;
    event.paramL  = msg->pt.x;
    event.paramH  = msg->pt.y;
    call_hooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event, sizeof(event) );

    if (!check_hwnd_filter( msg, hwnd_filter )) return FALSE;

    pt = msg->pt;
    message = msg->message;
    wparam = msg->wParam;
    /* Note: windows has no concept of a non-client wheel message */
    if (message != WM_MOUSEWHEEL)
    {
        if (hittest != HTCLIENT)
        {
            message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
            wparam = hittest;
        }
        else
        {
            /* coordinates don't get translated while tracking a menu */
            /* FIXME: should differentiate popups and top-level menus */
            if (!(info.flags & GUI_INMENUMODE))
                screen_to_client( msg->hwnd, &pt );
        }
    }
    msg->lParam = MAKELONG( pt.x, pt.y );

    /* translate double clicks */

    if (msg->message == WM_LBUTTONDOWN ||
        msg->message == WM_RBUTTONDOWN ||
        msg->message == WM_MBUTTONDOWN ||
        msg->message == WM_XBUTTONDOWN)
    {
        BOOL update = remove;

        /* translate double clicks -
	 * note that ...MOUSEMOVEs can slip in between
	 * ...BUTTONDOWN and ...BUTTONDBLCLK messages */

        if ((info.flags & (GUI_INMENUMODE|GUI_INMOVESIZE)) ||
            hittest != HTCLIENT ||
            (get_class_long( msg->hwnd, GCL_STYLE, FALSE ) & CS_DBLCLKS))
        {
           if ((msg->message == clk_msg.message) &&
               (msg->hwnd == clk_msg.hwnd) &&
               (msg->wParam == clk_msg.wParam) &&
               (msg->time - clk_msg.time < NtUserGetDoubleClickTime()) &&
               (abs(msg->pt.x - clk_msg.pt.x) < get_system_metrics( SM_CXDOUBLECLK ) / 2) &&
               (abs(msg->pt.y - clk_msg.pt.y) < get_system_metrics( SM_CYDOUBLECLK ) / 2))
           {
               message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
               if (update)
               {
                   clk_msg.message = 0;  /* clear the double click conditions */
                   update = FALSE;
               }
           }
        }
        if (message < first || message > last) return FALSE;
        /* update static double click conditions */
        if (update) clk_msg = *msg;
    }
    else
    {
        if (message < first || message > last) return FALSE;
    }
    msg->wParam = wparam;

    /* message is accepted now (but may still get dropped) */

    hook.pt           = msg->pt;
    hook.hwnd         = msg->hwnd;
    hook.wHitTestCode = hittest;
    hook.dwExtraInfo  = extra_info;
    hook.mouseData    = msg->wParam;
    if (call_hooks( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE, message, (LPARAM)&hook, sizeof(hook) ))
    {
        hook.pt           = msg->pt;
        hook.hwnd         = msg->hwnd;
        hook.wHitTestCode = hittest;
        hook.dwExtraInfo  = extra_info;
        hook.mouseData    = msg->wParam;
        accept_hardware_message( hw_id );
        call_hooks( WH_CBT, HCBT_CLICKSKIPPED, message, (LPARAM)&hook, sizeof(hook) );
        return FALSE;
    }

    if ((hittest == HTERROR) || (hittest == HTNOWHERE))
    {
        send_message( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd, MAKELONG( hittest, msg->message ));
        accept_hardware_message( hw_id );
        return FALSE;
    }

    if (remove) accept_hardware_message( hw_id );

    if (!remove || info.hwndCapture)
    {
        msg->message = message;
        return TRUE;
    }

    eat_msg = FALSE;

    if (msg->message == WM_LBUTTONDOWN ||
        msg->message == WM_RBUTTONDOWN ||
        msg->message == WM_MBUTTONDOWN ||
        msg->message == WM_XBUTTONDOWN)
    {
        /* Send the WM_PARENTNOTIFY,
         * note that even for double/nonclient clicks
         * notification message is still WM_L/M/RBUTTONDOWN.
         */
        send_parent_notify( msg->hwnd, msg->message, 0, msg->pt );

        /* Activate the window if needed */

        if (msg->hwnd != info.hwndActive)
        {
            HWND hwndTop = NtUserGetAncestor( msg->hwnd, GA_ROOT );

            if ((get_window_long( hwndTop, GWL_STYLE ) & (WS_POPUP|WS_CHILD)) != WS_CHILD)
            {
                UINT ret = send_message( msg->hwnd, WM_MOUSEACTIVATE, (WPARAM)hwndTop,
                                         MAKELONG( hittest, msg->message ) );
                switch(ret)
                {
                case MA_NOACTIVATEANDEAT:
                    eat_msg = TRUE;
                    /* fall through */
                case MA_NOACTIVATE:
                    break;
                case MA_ACTIVATEANDEAT:
                    eat_msg = TRUE;
                    /* fall through */
                case MA_ACTIVATE:
                case 0:
                    if (!set_foreground_window( hwndTop, TRUE )) eat_msg = TRUE;
                    break;
                default:
                    WARN( "unknown WM_MOUSEACTIVATE code %d\n", ret );
                    break;
                }
            }
        }
    }

    /* send the WM_SETCURSOR message */

    /* Windows sends the normal mouse message as the message parameter
       in the WM_SETCURSOR message even if it's non-client mouse message */
    send_message( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd, MAKELONG( hittest, msg->message ));

    msg->message = message;
    return !eat_msg;
}

/***********************************************************************
 *           process_hardware_message
 *
 * Process a hardware message; return TRUE if message should be passed on to the app
 */
static BOOL process_hardware_message( MSG *msg, UINT hw_id, const struct hardware_msg_data *msg_data,
                                      HWND hwnd_filter, UINT first, UINT last, BOOL remove )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    RECT rect = {msg->pt.x, msg->pt.y, msg->pt.x, msg->pt.y};
    UINT context;
    BOOL ret = FALSE;

    thread_info->msg_source.deviceType = msg_data->source.device;
    thread_info->msg_source.originId   = msg_data->source.origin;

    /* hardware messages are always in raw physical coords */
    context = set_thread_dpi_awareness_context( NTUSER_DPI_PER_MONITOR_AWARE );
    rect = map_rect_raw_to_virt( rect, get_thread_dpi() );
    msg->pt.x = rect.left;
    msg->pt.y = rect.top;

    if (msg->message == WM_INPUT || msg->message == WM_INPUT_DEVICE_CHANGE)
        ret = process_rawinput_message( msg, hw_id, msg_data );
    else if (is_keyboard_message( msg->message ))
        ret = process_keyboard_message( msg, hw_id, hwnd_filter, first, last, remove );
    else if (is_mouse_message( msg->message ))
        ret = process_mouse_message( msg, hw_id, msg_data->info, hwnd_filter, first, last, remove );
    else if (msg->message >= WM_POINTERUPDATE && msg->message <= WM_POINTERLEAVE)
        ret = process_pointer_message( msg, hw_id, msg_data );
    else if (msg->message == WM_WINE_CLIPCURSOR)
        process_wine_clipcursor( msg->hwnd, msg->wParam, msg->lParam );
    else if (msg->message == WM_WINE_SETCURSOR)
        process_wine_setcursor( msg->hwnd, (HWND)msg->wParam, (HCURSOR)msg->lParam );
    else
        ERR( "unknown message type %x\n", msg->message );
    set_thread_dpi_awareness_context( context );
    return ret;
}

/***********************************************************************
 *           check_queue_bits
 *
 * returns TRUE and the queue wake bits and changed bits if we can skip a server request
 * returns FALSE if we need to make a server request to update the queue masks or bits
 */
static BOOL check_queue_bits( UINT wake_mask, UINT changed_mask, UINT signal_bits, UINT clear_bits,
                              UINT *wake_bits, UINT *changed_bits, BOOL internal )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const queue_shm_t *queue_shm;
    BOOL skip = FALSE;
    UINT status;

    while ((status = get_shared_queue( &lock, &queue_shm )) == STATUS_PENDING)
    {
        if (internal) skip = !(queue_shm->internal_bits & QS_HARDWARE);
        /* if the masks need an update */
        else if (queue_shm->wake_mask != wake_mask) skip = FALSE;
        else if (queue_shm->changed_mask != changed_mask) skip = FALSE;
        /* or if some bits need to be cleared, or queue is signaled */
        else if (queue_shm->wake_bits & signal_bits) skip = FALSE;
        else if (queue_shm->changed_bits & clear_bits) skip = FALSE;
        else
        {
            *wake_bits = queue_shm->wake_bits;
            *changed_bits = queue_shm->changed_bits;
            skip = get_tick_count() - (UINT64)queue_shm->access_time / 10000 < 3000; /* avoid hung queue */
        }
    }

    if (status) return FALSE;
    return skip;
}

/***********************************************************************
 *           peek_message
 *
 * Peek for a message matching the given parameters. Return 0 if none are
 * available; -1 on error.
 * All pending sent messages are processed before returning.
 */
static int peek_message( MSG *msg, const struct peek_message_filter *filter )
{
    LRESULT result;
    HWND hwnd = filter->hwnd;
    UINT first = filter->first, last = filter->last, flags = filter->flags;
    struct user_thread_info *thread_info = get_user_thread_info();
    INPUT_MESSAGE_SOURCE prev_source = thread_info->client_info.msg_source;
    HANDLE idle_event = thread_info->idle_event;
    struct received_message_info info;
    unsigned int hw_id = 0;  /* id of previous hardware message */
    unsigned char buffer_init[1024];
    size_t buffer_size = sizeof(buffer_init);
    void *buffer = buffer_init;

    if (!first && !last) last = ~0;
    if (hwnd == HWND_BROADCAST) hwnd = HWND_TOPMOST;

    for (;;)
    {
        NTSTATUS res;
        size_t size = 0;
        const union message_data *msg_data = buffer;
        UINT wake_mask, signal_bits, wake_bits, changed_bits, clear_bits = 0;

        /* use the same logic as in server/queue.c get_message */
        if (!(signal_bits = flags >> 16)) signal_bits = QS_ALLINPUT;

        if (signal_bits & QS_POSTMESSAGE)
        {
            clear_bits |= QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER;
            if (first == 0 && last == ~0U) clear_bits |= QS_ALLPOSTMESSAGE;
        }
        if (signal_bits & QS_INPUT) clear_bits |= QS_INPUT;
        if (signal_bits & QS_PAINT) clear_bits |= QS_PAINT;

        /* if filter includes QS_RAWINPUT we have to translate hardware messages */
        if (signal_bits & QS_RAWINPUT) signal_bits |= QS_KEY | QS_MOUSEMOVE | QS_MOUSEBUTTON;

        thread_info->client_info.msg_source = prev_source;
        wake_mask = filter->mask & (QS_SENDMESSAGE | QS_SMRESULT);

        if (check_queue_bits( wake_mask, filter->mask, wake_mask | signal_bits, filter->mask | clear_bits,
                              &wake_bits, &changed_bits, filter->internal ))
            res = STATUS_PENDING;
        else SERVER_START_REQ( get_message )
        {
            req->internal  = filter->internal;
            req->flags     = flags;
            req->get_win   = wine_server_user_handle( hwnd );
            req->get_first = first;
            req->get_last  = last;
            req->hw_id     = hw_id;
            req->wake_mask = wake_mask;
            req->changed_mask = filter->mask;
            wine_server_set_reply( req, buffer, buffer_size );
            if (!(res = wine_server_call( req )))
            {
                size = wine_server_reply_size( reply );
                info.type        = reply->type;
                info.msg.hwnd    = wine_server_ptr_handle( reply->win );
                info.msg.message = reply->msg;
                info.msg.wParam  = reply->wparam;
                info.msg.lParam  = reply->lparam;
                info.msg.time    = reply->time;
                info.msg.pt.x    = reply->x;
                info.msg.pt.y    = reply->y;
                hw_id            = 0;
            }
            else buffer_size = reply->total;
        }
        SERVER_END_REQ;

        if (res)
        {
            if (buffer != buffer_init) free( buffer );
            if (res == STATUS_PENDING)
            {
                if (hwnd == (HWND)-1 && idle_event) NtSetEvent( idle_event, NULL );
                return 0;
            }
            if (res != STATUS_BUFFER_OVERFLOW)
            {
                RtlSetLastWin32Error( RtlNtStatusToDosError(res) );
                return -1;
            }
            if (!(buffer = malloc( buffer_size ))) return -1;
            continue;
        }

        TRACE( "got type %d msg %x (%s) hwnd %p wp %lx lp %lx\n",
               info.type, info.msg.message,
               (info.type == MSG_WINEVENT) ? "MSG_WINEVENT" : debugstr_msg_name(info.msg.message, info.msg.hwnd),
               info.msg.hwnd, (long)info.msg.wParam, info.msg.lParam );

        switch(info.type)
        {
        case MSG_ASCII:
        case MSG_UNICODE:
            info.flags = ISMEX_SEND;
            break;
        case MSG_NOTIFY:
            info.flags = ISMEX_NOTIFY;
            /* unpack_message may have to reallocate */
            if (buffer == buffer_init)
            {
                buffer = malloc( buffer_size );
                memcpy( buffer, buffer_init, buffer_size );
            }
            if (!unpack_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                 &info.msg.lParam, &buffer, size, &buffer_size ))
                continue;
            break;
        case MSG_CALLBACK:
            info.flags = ISMEX_CALLBACK;
            break;
        case MSG_CALLBACK_RESULT:
            if (size >= sizeof(msg_data->callback))
                call_sendmsg_callback( wine_server_get_ptr(msg_data->callback.callback),
                                       info.msg.hwnd, info.msg.message,
                                       msg_data->callback.data, msg_data->callback.result );
            continue;
        case MSG_WINEVENT:
            if (size >= sizeof(msg_data->winevent))
            {
                struct win_event_hook_params params;
                void *ret_ptr;
                ULONG ret_len;

                params.proc = wine_server_get_ptr( msg_data->winevent.hook_proc );
                size -= sizeof(msg_data->winevent);
                if (size)
                {
                    size = min( size, sizeof(params.module) - sizeof(WCHAR) );
                    memcpy( params.module, &msg_data->winevent + 1, size );
                }
                params.module[size / sizeof(WCHAR)] = 0;
                size = FIELD_OFFSET( struct win_event_hook_params, module[size / sizeof(WCHAR) + 1] );

                params.handle    = wine_server_ptr_handle( msg_data->winevent.hook );
                params.event     = info.msg.message;
                params.hwnd      = info.msg.hwnd;
                params.object_id = info.msg.wParam;
                params.child_id  = info.msg.lParam;
                params.tid       = msg_data->winevent.tid;
                params.time      = info.msg.time;

                KeUserModeCallback( NtUserCallWinEventHook, &params, size, &ret_ptr, &ret_len );
            }
            continue;
        case MSG_HOOK_LL:
            info.flags = ISMEX_SEND;
            result = 0;
            if (info.msg.message == WH_KEYBOARD_LL && size >= sizeof(msg_data->hardware))
            {
                KBDLLHOOKSTRUCT hook;

                hook.vkCode      = LOWORD( info.msg.lParam );
                hook.scanCode    = HIWORD( info.msg.lParam );
                hook.flags       = msg_data->hardware.flags;
                hook.time        = info.msg.time;
                hook.dwExtraInfo = msg_data->hardware.info;
                TRACE( "calling keyboard LL hook vk %x scan %x flags %x time %u info %lx\n",
                       hook.vkCode, hook.scanCode, hook.flags,
                       hook.time, hook.dwExtraInfo );
                result = call_hooks( WH_KEYBOARD_LL, HC_ACTION, info.msg.wParam,
                                     (LPARAM)&hook, sizeof(hook) );
            }
            else if (info.msg.message == WH_MOUSE_LL && size >= sizeof(msg_data->hardware))
            {
                RECT rect = {info.msg.pt.x, info.msg.pt.y, info.msg.pt.x, info.msg.pt.y};
                MSLLHOOKSTRUCT hook;

                rect = map_rect_raw_to_virt( rect, 0 );
                info.msg.pt.x = rect.left;
                info.msg.pt.y = rect.top;

                hook.pt          = info.msg.pt;
                hook.mouseData   = info.msg.lParam;
                hook.flags       = msg_data->hardware.flags;
                hook.time        = info.msg.time;
                hook.dwExtraInfo = msg_data->hardware.info;
                TRACE( "calling mouse LL hook pos %d,%d data %x flags %x time %u info %lx\n",
                       hook.pt.x, hook.pt.y, hook.mouseData, hook.flags,
                       hook.time, hook.dwExtraInfo );
                result = call_hooks( WH_MOUSE_LL, HC_ACTION, info.msg.wParam,
                                     (LPARAM)&hook, sizeof(hook) );
            }
            reply_message( &info, result, &info.msg );
            continue;
        case MSG_OTHER_PROCESS:
            info.flags = ISMEX_SEND;
            /* unpack_message may have to reallocate */
            if (buffer == buffer_init)
            {
                buffer = malloc( buffer_size );
                memcpy( buffer, buffer_init, buffer_size );
            }
            if (!unpack_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                 &info.msg.lParam, &buffer, size, &buffer_size ))
            {
                /* ignore it */
                reply_message( &info, 0, &info.msg );
                continue;
            }
            break;
        case MSG_HARDWARE:
            if (size >= sizeof(msg_data->hardware))
            {
                hw_id = msg_data->hardware.hw_id;
                if (!process_hardware_message( &info.msg, hw_id, &msg_data->hardware,
                                               hwnd, first, last, flags & PM_REMOVE ))
                {
                    TRACE("dropping msg %x\n", info.msg.message );
                    continue;  /* ignore it */
                }
                *msg = info.msg;
                thread_info->client_info.message_pos   = MAKELONG( info.msg.pt.x, info.msg.pt.y );
                thread_info->client_info.message_time  = info.msg.time;
                thread_info->client_info.message_extra = msg_data->hardware.info;
                if (buffer != buffer_init) free( buffer );
                call_hooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, sizeof(*msg) );
                return 1;
            }
            continue;
        case MSG_POSTED:
            if (info.msg.message & 0x80000000)  /* internal message */
            {
                if (flags & PM_REMOVE)
                {
                    handle_internal_message( info.msg.hwnd, info.msg.message,
                                             info.msg.wParam, info.msg.lParam );
                    /* if this is a nested call return right away */
                    if (first == info.msg.message && last == info.msg.message)
                    {
                        if (buffer != buffer_init) free( buffer );
                        return 0;
                    }
                }
                else
                {
                    struct peek_message_filter new_filter =
                    {
                        .hwnd = info.msg.hwnd,
                        .flags = flags | PM_REMOVE,
                        .first = info.msg.message,
                        .last = info.msg.message,
                        .mask = filter->mask,
                        .internal = filter->internal,
                    };
                    peek_message( msg, &new_filter );
                }
                continue;
            }
            if (info.msg.message >= WM_DDE_FIRST && info.msg.message <= WM_DDE_LAST)
            {
                struct unpack_dde_message_result *result;
                struct unpack_dde_message_params *params;
                NTSTATUS status;
                ULONG len;

                len = FIELD_OFFSET( struct unpack_dde_message_params, data[size] );
                if (!(params = malloc( len )))
                    continue;
                params->hwnd    = info.msg.hwnd;
                params->message = info.msg.message;
                params->wparam  = info.msg.wParam;
                params->lparam  = info.msg.lParam;
                if (size) memcpy( params->data, buffer, size );
                status = KeUserModeCallback( NtUserUnpackDDEMessage, params, len, (void **)&result, &len );
                free( params );
                if (status) continue; /* ignore it */
                if (len == sizeof(*result))
                {
                    info.msg.wParam = result->wparam;
                    info.msg.lParam = result->lparam;
                }
            }
            if (info.msg.message == WM_TIMER || info.msg.message == WM_SYSTIMER)
            {
                if (!(flags & PM_NOYIELD) && idle_event) NtSetEvent( idle_event, NULL );
            }
            *msg = info.msg;
            msg->pt = point_phys_to_win_dpi( info.msg.hwnd, info.msg.pt );
            thread_info->client_info.message_pos   = MAKELONG( msg->pt.x, msg->pt.y );
            thread_info->client_info.message_time  = info.msg.time;
            thread_info->client_info.message_extra = 0;
            thread_info->client_info.msg_source = msg_source_unavailable;
            if (buffer != buffer_init) free( buffer );
            call_hooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, sizeof(*msg) );
            return 1;
        }

        /* if we get here, we have a sent message; call the window procedure */
        info.prev = thread_info->receive_info;
        thread_info->receive_info = &info;
        thread_info->client_info.msg_source = msg_source_unavailable;
        thread_info->client_info.receive_flags = info.flags;
        result = call_window_proc( info.msg.hwnd, info.msg.message, info.msg.wParam,
                                   info.msg.lParam, info.type, FALSE, WMCHAR_MAP_RECVMESSAGE,
                                   info.type == MSG_ASCII );
        if (thread_info->receive_info == &info)
            reply_winproc_result( result, info.msg.hwnd, info.msg.message,
                                  info.msg.wParam, info.msg.lParam );

        /* if some PM_QS* flags were specified, only handle sent messages from now on */
        if (HIWORD(flags) && !filter->mask) flags = PM_QS_SENDMESSAGE | LOWORD(flags);
    }
}

/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages.
 */
static void process_sent_messages(void)
{
    struct peek_message_filter filter = {.flags = PM_REMOVE | PM_QS_SENDMESSAGE};
    MSG msg;
    peek_message( &msg, &filter );
}

/***********************************************************************
 *           get_server_queue_handle
 *
 * Get a handle to the server message queue for the current thread.
 */
static HANDLE get_server_queue_handle(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    HANDLE ret;

    if (!(ret = thread_info->server_queue))
    {
        SERVER_START_REQ( get_msg_queue_handle )
        {
            wine_server_call( req );
            thread_info->server_queue = wine_server_ptr_handle( reply->handle );
            thread_info->idle_event = wine_server_ptr_handle( reply->idle_event );
        }
        SERVER_END_REQ;
        if (!(ret = thread_info->server_queue)) ERR( "Cannot get server thread queue\n" );
    }
    return ret;
}

static BOOL is_queue_signaled(void)
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const queue_shm_t *queue_shm;
    BOOL signaled = FALSE;
    UINT status;

    while ((status = get_shared_queue( &lock, &queue_shm )) == STATUS_PENDING)
        signaled = (queue_shm->wake_bits & queue_shm->wake_mask) ||
                   (queue_shm->changed_bits & queue_shm->changed_mask);
    if (status) return FALSE;

    return signaled;
}

static BOOL check_queue_masks( UINT wake_mask, UINT changed_mask )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const queue_shm_t *queue_shm;
    BOOL skip = FALSE;
    UINT status;

    while ((status = get_shared_queue( &lock, &queue_shm )) == STATUS_PENDING)
    {
        if (queue_shm->wake_mask != wake_mask || queue_shm->changed_mask != changed_mask) skip = FALSE;
        else skip = get_tick_count() - (UINT64)queue_shm->access_time / 10000 < 3000; /* avoid hung queue */
    }

    if (status) return FALSE;
    return skip;
}

static BOOL check_internal_bits( UINT mask )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const queue_shm_t *queue_shm;
    BOOL signaled = FALSE;
    UINT status;

    while ((status = get_shared_queue( &lock, &queue_shm )) == STATUS_PENDING)
        signaled = queue_shm->internal_bits & mask;
    if (status) return FALSE;

    return signaled;
}

static BOOL process_driver_events( UINT events_mask, UINT wake_mask, UINT changed_mask )
{
    BOOL drained = FALSE;

    if (check_internal_bits( QS_DRIVER )) drained = user_driver->pProcessEvents( events_mask );

    if (drained || !check_queue_masks( wake_mask, changed_mask ))
    {
        SERVER_START_REQ( set_queue_mask )
        {
            req->poll_events = drained;
            req->wake_mask = wake_mask;
            req->changed_mask = changed_mask;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    /* process every pending internal hardware messages */
    if (check_internal_bits( QS_HARDWARE ))
    {
        struct peek_message_filter filter = {.internal = TRUE};
        MSG msg;
        peek_message( &msg, &filter );
    }

    return is_queue_signaled();
}

void check_for_events( UINT flags )
{
    if (!process_driver_events( flags, 0, 0 ) && !(flags & QS_PAINT)) flush_window_surfaces( TRUE );
}

/* monotonic timer tick for throttling driver event checks */
static inline LONGLONG get_driver_check_time(void)
{
    LARGE_INTEGER counter, freq;
    NtQueryPerformanceCounter( &counter, &freq );
    return counter.QuadPart * 8000 / freq.QuadPart; /* 8kHz */
}

/* check for driver events if we detect that the app is not properly consuming messages */
static inline void check_for_driver_events(void)
{
    if (get_user_thread_info()->last_driver_time != get_driver_check_time())
    {
        flush_window_surfaces( FALSE );
        process_driver_events( QS_ALLINPUT, 0, 0 );
        get_user_thread_info()->last_driver_time = get_driver_check_time();
    }
}

/* helper for kernel32->ntdll timeout format conversion */
static inline LARGE_INTEGER *get_nt_timeout( LARGE_INTEGER *time, DWORD timeout )
{
    if (timeout == INFINITE) return NULL;
    time->QuadPart = (ULONGLONG)timeout * -10000;
    return time;
}

/* wait for message or signaled handle */
static DWORD wait_message( DWORD count, const HANDLE *handles, DWORD timeout, DWORD wake_mask, DWORD changed_mask, DWORD flags )
{
    struct thunk_lock_params params = {.dispatch.callback = thunk_lock_callback};
    WAIT_TYPE type = flags & MWMO_WAITALL ? WaitAll : WaitAny;
    LARGE_INTEGER time, now, *abs;
    void *ret_ptr;
    ULONG ret_len;
    HANDLE event;
    DWORD ret;

    if ((abs = get_nt_timeout( &time, timeout )))
    {
        NtQuerySystemTime( &now );
        abs->QuadPart = now.QuadPart - abs->QuadPart;
    }

    if (!KeUserDispatchCallback( &params.dispatch, sizeof(params), &ret_ptr, &ret_len ) &&
        ret_len == sizeof(params.locks))
    {
        params.locks = *(DWORD *)ret_ptr;
        params.restore = TRUE;
    }

    process_driver_events( QS_ALLINPUT, wake_mask, changed_mask );
    if (!(changed_mask & QS_SMRESULT) && (event = get_user_thread_info()->idle_event)) NtSetEvent( event, NULL );

    do ret = NtWaitForMultipleObjects( count, handles, type, !!(flags & MWMO_ALERTABLE), abs );
    while (ret == count - 1 && !process_driver_events( QS_ALLINPUT, wake_mask, changed_mask ));

    if (HIWORD(ret)) /* is it an error code? */
    {
        RtlSetLastWin32Error( RtlNtStatusToDosError(ret) );
        ret = WAIT_FAILED;
    }

    if (ret == WAIT_TIMEOUT && !count && !timeout) NtYieldExecution();
    if (ret == count - 1) get_user_thread_info()->last_driver_time = get_driver_check_time();

    KeUserDispatchCallback( &params.dispatch, sizeof(params), &ret_ptr, &ret_len );

    return ret;
}

/***********************************************************************
 *           wait_objects
 *
 * Wait for multiple objects including the server queue, with specific queue masks.
 */
static DWORD wait_objects( DWORD count, const HANDLE *handles, DWORD timeout,
                           DWORD wake_mask, DWORD changed_mask, DWORD flags )
{
    assert( count );  /* we must have at least the server queue */

    flush_window_surfaces( TRUE );

    return wait_message( count, handles, timeout, wake_mask, changed_mask, flags );
}

static HANDLE normalize_std_handle( HANDLE handle )
{
    if (handle == (HANDLE)STD_INPUT_HANDLE)
        return NtCurrentTeb()->Peb->ProcessParameters->hStdInput;
    if (handle == (HANDLE)STD_OUTPUT_HANDLE)
        return NtCurrentTeb()->Peb->ProcessParameters->hStdOutput;
    if (handle == (HANDLE)STD_ERROR_HANDLE)
        return NtCurrentTeb()->Peb->ProcessParameters->hStdError;

    return handle;
}

/***********************************************************************
 *           NtUserMsgWaitForMultipleObjectsEx   (win32u.@)
 */
DWORD WINAPI NtUserMsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                DWORD timeout, DWORD mask, DWORD flags )
{
    HANDLE wait_handles[MAXIMUM_WAIT_OBJECTS];
    DWORD i;

    if (count > MAXIMUM_WAIT_OBJECTS-1)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    /* add the queue to the handle list */
    for (i = 0; i < count; i++) wait_handles[i] = normalize_std_handle( handles[i] );
    wait_handles[count] = get_server_queue_handle();

    return wait_objects( count+1, wait_handles, timeout,
                         (flags & MWMO_INPUTAVAILABLE) ? mask : 0, mask, flags );
}

/***********************************************************************
 *           NtUserWaitForInputIdle (win32u.@)
 */
DWORD WINAPI NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow )
{
    DWORD start_time, elapsed, ret;
    HANDLE handles[2];

    handles[0] = process;
    SERVER_START_REQ( get_process_idle_event )
    {
        req->handle = wine_server_obj_handle( process );
        wine_server_call_err( req );
        handles[1] = wine_server_ptr_handle( reply->event );
    }
    SERVER_END_REQ;
    if (!handles[1]) return WAIT_FAILED;  /* no event to wait on */

    start_time = NtGetTickCount();
    elapsed = 0;

    TRACE("waiting for %p\n", handles[1] );

    for (;;)
    {
        ret = NtUserMsgWaitForMultipleObjectsEx( 2, handles, timeout - elapsed, QS_SENDMESSAGE, 0 );
        switch (ret)
        {
        case WAIT_OBJECT_0:
            return 0;
        case WAIT_OBJECT_0+2:
            process_sent_messages();
            break;
        case WAIT_TIMEOUT:
        case WAIT_FAILED:
            TRACE("timeout or error\n");
            return ret;
        default:
            TRACE("finished\n");
            return 0;
        }
        if (timeout != INFINITE)
        {
            elapsed = NtGetTickCount() - start_time;
            if (elapsed > timeout)
                break;
        }
    }

    return WAIT_TIMEOUT;
}

/***********************************************************************
 *           NtUserWaitMessage (win32u.@)
 */
BOOL WINAPI NtUserWaitMessage(void)
{
    return NtUserMsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, 0 ) != WAIT_FAILED;
}

/***********************************************************************
 *           NtUserPeekMessage  (win32u.@)
 */
BOOL WINAPI NtUserPeekMessage( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    struct peek_message_filter filter = {.hwnd = hwnd, .first = first, .last = last, .flags = flags};
    MSG msg;
    int ret;

    user_check_not_lock();
    check_for_driver_events();

    if ((ret = peek_message( &msg, &filter )) <= 0)
    {
        if (!ret)
        {
            struct thunk_lock_params params = {.dispatch.callback = thunk_lock_callback};
            void *ret_ptr;
            ULONG ret_len;

            flush_window_surfaces( TRUE );

            if (!KeUserDispatchCallback( &params.dispatch, sizeof(params), &ret_ptr, &ret_len ) &&
                ret_len == sizeof(params.locks))
            {
                params.locks = *(DWORD *)ret_ptr;
                params.restore = TRUE;
            }
            NtYieldExecution();
            KeUserDispatchCallback( &params.dispatch, sizeof(params), &ret_ptr, &ret_len );
        }
        return FALSE;
    }

    check_for_driver_events();

    /* copy back our internal safe copy of message data to msg_out.
     * msg_out is a variable from the *program*, so it can't be used
     * internally as it can get "corrupted" by our use of SendMessage()
     * (back to the program) inside the message handling itself. */
    if (!msg_out)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }
    *msg_out = msg;
    return TRUE;
}

/***********************************************************************
 *           NtUserGetMessage  (win32u.@)
 */
BOOL WINAPI NtUserGetMessage( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    struct peek_message_filter filter = {.hwnd = hwnd, .first = first, .last = last};
    HANDLE server_queue = get_server_queue_handle();
    unsigned int mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    int ret;

    user_check_not_lock();
    check_for_driver_events();

    if (first || last)
    {
        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask = QS_ALLINPUT;

    filter.mask = mask;
    filter.flags = PM_REMOVE | (mask << 16);
    while (!(ret = peek_message( msg, &filter )))
    {
        wait_objects( 1, &server_queue, INFINITE, mask & (QS_SENDMESSAGE | QS_SMRESULT), mask, 0 );
    }
    if (ret < 0) return -1;

    check_for_driver_events();

    return msg->message != WM_QUIT;
}

/***********************************************************************
 *           put_message_in_queue
 *
 * Put a sent message into the destination queue.
 * For inter-process message, reply_size is set to expected size of reply data.
 */
static BOOL put_message_in_queue( const struct send_message_info *info, size_t *reply_size )
{
    struct packed_message data;
    union message_data msg_data;
    unsigned int res;
    int i;
    timeout_t timeout = TIMEOUT_INFINITE;

    /* Check for INFINITE timeout for compatibility with Win9x,
     * although Windows >= NT does not do so
     */
    if (info->type != MSG_NOTIFY &&
        info->type != MSG_CALLBACK &&
        info->type != MSG_POSTED &&
        info->timeout &&
        info->timeout != INFINITE)
    {
        /* timeout is signed despite the prototype */
        timeout = (timeout_t)max( 0, (int)info->timeout ) * -10000;
    }

    memset( &data, 0, sizeof(data) );
    if (info->type == MSG_OTHER_PROCESS || info->type == MSG_NOTIFY)
    {
        *reply_size = pack_message( info->hwnd, info->msg, info->wparam, info->lparam, &data );
        if (data.count == -1)
        {
            WARN( "cannot pack message %x\n", info->msg );
            return FALSE;
        }
    }
    else if (info->type == MSG_CALLBACK)
    {
        msg_data.callback.callback = wine_server_client_ptr( info->callback );
        msg_data.callback.data     = info->data;
        msg_data.callback.result   = 0;
        data.data[0] = &msg_data;
        data.size[0] = sizeof(msg_data.callback);
        data.count = 1;
    }
    else if (info->type == MSG_POSTED && info->msg >= WM_DDE_FIRST && info->msg <= WM_DDE_LAST)
    {
        struct post_dde_message_params params;
        void *ret_ptr;
        ULONG ret_len;

        params.hwnd     = info->hwnd;
        params.msg      = info->msg;
        params.wparam   = info->wparam;
        params.lparam   = info->lparam;
        params.dest_tid = info->dest_tid;
        res = KeUserModeCallback( NtUserPostDDEMessage, &params, sizeof(params), &ret_ptr, &ret_len );
        goto done;
    }

    SERVER_START_REQ( send_message )
    {
        req->id      = info->dest_tid;
        req->type    = info->type;
        req->flags   = 0;
        req->win     = wine_server_user_handle( info->hwnd );
        req->msg     = info->msg;
        req->wparam  = info->wparam;
        req->lparam  = info->lparam;
        req->timeout = timeout;

        if (info->flags & SMTO_ABORTIFHUNG) req->flags |= SEND_MSG_ABORT_IF_HUNG;
        for (i = 0; i < data.count; i++) wine_server_add_data( req, data.data[i], data.size[i] );
        res = wine_server_call( req );
    }
    SERVER_END_REQ;

done:
    if (res == STATUS_INVALID_PARAMETER) res = STATUS_NO_LDT;
    if (res) RtlSetLastWin32Error( RtlNtStatusToDosError(res) );
    return !res;
}

/***********************************************************************
 *           post_dde_message_call
 */
static NTSTATUS post_dde_message_call( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                       const struct post_dde_message_call_params *params )
{
    NTSTATUS status;

    SERVER_START_REQ( send_message )
    {
        req->id      = params->dest_tid;
        req->type    = MSG_POSTED;
        req->flags   = 0;
        req->win     = wine_server_user_handle( hwnd );
        req->msg     = msg;
        req->wparam  = wparam;
        req->lparam  = lparam;
        req->timeout = TIMEOUT_INFINITE;
        wine_server_add_data( req, params->ptr, params->size );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}


/***********************************************************************
 *           wait_message_reply
 *
 * Wait until a sent message gets replied to.
 */
static void wait_message_reply( UINT flags )
{
    HANDLE server_queue = get_server_queue_handle();
    unsigned int wake_mask = QS_SMRESULT | ((flags & SMTO_BLOCK) ? 0 : QS_SENDMESSAGE);

    for (;;)
    {
        UINT wake_bits, changed_bits;

        if (check_queue_bits( wake_mask, wake_mask, wake_mask, wake_mask,
                              &wake_bits, &changed_bits, FALSE ))
            wake_bits = wake_bits & wake_mask;
        else SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = wake_mask;
            req->changed_mask = wake_mask;
            wine_server_call( req );
            wake_bits = reply->wake_bits & wake_mask;
        }
        SERVER_END_REQ;

        if (wake_bits & QS_SMRESULT) return;  /* got a result */
        if (wake_bits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            process_sent_messages();
            continue;
        }

        wait_message( 1, &server_queue, INFINITE, wake_mask, wake_mask, 0 );
    }
}

/***********************************************************************
 *           retrieve_reply
 *
 * Retrieve a message reply from the server.
 */
static LRESULT retrieve_reply( const struct send_message_info *info,
                               size_t reply_size, LRESULT *result )
{
    unsigned int status;
    void *reply_data = NULL;

    if (reply_size)
    {
        if (!(reply_data = malloc( reply_size )))
        {
            WARN( "no memory for reply, will be truncated\n" );
            reply_size = 0;
        }
    }
    SERVER_START_REQ( get_message_reply )
    {
        req->cancel = 1;
        if (reply_size) wine_server_set_reply( req, reply_data, reply_size );
        if (!(status = wine_server_call( req ))) *result = reply->result;
        reply_size = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;
    if (!status && reply_size)
        unpack_reply( info->hwnd, info->msg, info->wparam, info->lparam, reply_data, reply_size );

    free( reply_data );

    TRACE( "hwnd %p msg %x (%s) wp %lx lp %lx got reply %lx (err=%d)\n",
           info->hwnd, info->msg, debugstr_msg_name(info->msg, info->hwnd), (long)info->wparam,
           info->lparam, *result, status );

    /* MSDN states that last error is 0 on timeout, but at least NT4 returns ERROR_TIMEOUT */
    if (status) RtlSetLastWin32Error( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 *           send_inter_thread_message
 */
static LRESULT send_inter_thread_message( const struct send_message_info *info, LRESULT *res_ptr )
{
    size_t reply_size = 0;

    TRACE( "hwnd %p msg %x (%s) wp %lx lp %lx\n",
           info->hwnd, info->msg, debugstr_msg_name(info->msg, info->hwnd),
           (long)info->wparam, info->lparam );

    user_check_not_lock();

    if (!put_message_in_queue( info, &reply_size )) return 0;

    /* there's no reply to wait for on notify/callback messages */
    if (info->type == MSG_NOTIFY || info->type == MSG_CALLBACK) return 1;

    wait_message_reply( info->flags );
    return retrieve_reply( info, reply_size, res_ptr );
}

static LRESULT send_inter_thread_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                           LRESULT *result, void *arg )
{
    struct send_message_info *info = arg;
    info->hwnd   = hwnd;
    info->msg    = msg;
    info->wparam = wp;
    info->lparam = lp;
    return send_inter_thread_message( info, result );
}

/***********************************************************************
 *           send_internal_message_timeout
 *
 * Same as SendMessageTimeoutW but sends the message to a specific thread
 * without requiring a window handle. Only works for internal Wine messages.
 */
LRESULT send_internal_message_timeout( DWORD dest_pid, DWORD dest_tid,
                                       UINT msg, WPARAM wparam, LPARAM lparam,
                                       UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    LRESULT ret, result = 0;

    assert( msg & 0x80000000 );  /* must be an internal Wine message */

    if (is_exiting_thread( dest_tid )) return 0;

    if (dest_tid == GetCurrentThreadId())
    {
        result = handle_internal_message( 0, msg, wparam, lparam );
        ret = 1;
    }
    else
    {
        struct send_message_info info;

        info.type     = dest_pid == GetCurrentProcessId() ? MSG_UNICODE : MSG_OTHER_PROCESS;
        info.dest_tid = dest_tid;
        info.hwnd     = 0;
        info.msg      = msg;
        info.wparam   = wparam;
        info.lparam   = lparam;
        info.flags    = flags;
        info.timeout  = timeout;
        info.params   = NULL;

        ret = send_inter_thread_message( &info, &result );
    }
    if (ret && res_ptr) *res_ptr = result;
    return ret;
}

/***********************************************************************
 *		send_hardware_message
 */
NTSTATUS send_hardware_message( HWND hwnd, UINT flags, const INPUT *input, LPARAM lparam )
{
    struct send_message_info info;
    int prev_x, prev_y, new_x, new_y;
    NTSTATUS ret;
    BOOL wait;

    info.type     = MSG_HARDWARE;
    info.dest_tid = 0;
    info.hwnd     = hwnd;
    info.flags    = 0;
    info.timeout  = 0;
    info.params   = NULL;

    if (input->type == INPUT_MOUSE && (input->mi.dwFlags & (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_RIGHTDOWN)))
        clip_fullscreen_window( hwnd, FALSE );

    SERVER_START_REQ( send_hardware_message )
    {
        req->win        = wine_server_user_handle( hwnd );
        req->flags      = flags;
        req->input.type = input->type;
        switch (input->type)
        {
        case INPUT_MOUSE:
            req->input.mouse.x     = input->mi.dx;
            req->input.mouse.y     = input->mi.dy;
            req->input.mouse.data  = input->mi.mouseData;
            req->input.mouse.flags = input->mi.dwFlags;
            req->input.mouse.time  = input->mi.time;
            req->input.mouse.info  = input->mi.dwExtraInfo;
            break;
        case INPUT_KEYBOARD:
            if (input->ki.dwFlags & KEYEVENTF_SCANCODE)
            {
                UINT scan = input->ki.wScan;
                /* TODO: Use the keyboard layout of the target hwnd, once
                 * NtUserGetKeyboardLayout supports non-current threads. */
                HKL layout = NtUserGetKeyboardLayout( 0 );
                if (flags & SEND_HWMSG_INJECTED)
                {
                    scan = scan & 0xff;
                    if (input->ki.dwFlags & KEYEVENTF_EXTENDEDKEY) scan |= 0xe000;
                }
                req->input.kbd.vkey = map_scan_to_kbd_vkey( scan, layout );
                req->input.kbd.scan = input->ki.wScan & 0xff;
            }
            else
            {
                req->input.kbd.vkey = input->ki.wVk;
                req->input.kbd.scan = input->ki.wScan;
            }
            req->input.kbd.flags = input->ki.dwFlags & ~KEYEVENTF_SCANCODE;
            req->input.kbd.time  = input->ki.time;
            req->input.kbd.info  = input->ki.dwExtraInfo;
            break;
        case INPUT_HARDWARE:
            req->input.hw.msg    = input->hi.uMsg;
            req->input.hw.wparam = MAKELONG( input->hi.wParamL, input->hi.wParamH );
            switch (input->hi.uMsg)
            {
            case WM_INPUT:
            case WM_INPUT_DEVICE_CHANGE:
            {
                struct hid_packet *hid = (struct hid_packet *)lparam;
                req->input.hw.hid = hid->head;
                wine_server_add_data( req, hid->data, hid->head.count * hid->head.length );
                break;
            }
            default:
                req->input.hw.lparam = lparam;
                break;
            }
            break;
        }
        ret = wine_server_call( req );
        wait = reply->wait;
        prev_x = reply->prev_x;
        prev_y = reply->prev_y;
        new_x  = reply->new_x;
        new_y  = reply->new_y;
    }
    SERVER_END_REQ;

    if (!ret && (flags & SEND_HWMSG_INJECTED) && (prev_x != new_x || prev_y != new_y))
        user_driver->pSetCursorPos( new_x, new_y );

    if (wait)
    {
        LRESULT ignored;
        wait_message_reply( 0 );
        retrieve_reply( &info, 0, &ignored );
    }
    return ret;
}

/***********************************************************************
 *		NtUserPostQuitMessage  (win32u.@)
 */
BOOL WINAPI NtUserPostQuitMessage( INT exit_code )
{
    SERVER_START_REQ( post_quit_message )
    {
        req->exit_code = exit_code;
        wine_server_call( req );
    }
    SERVER_END_REQ;
    return TRUE;
}

/**********************************************************************
 *           NtUserDispatchMessage  (win32u.@)
 */
LRESULT WINAPI NtUserDispatchMessage( const MSG *msg )
{
    struct win_proc_params params;
    LRESULT retval = 0;

    /* Process timer messages */
    if (msg->lParam && msg->message == WM_TIMER)
    {
        params.func = (WNDPROC)msg->lParam;
        if (!init_win_proc_params( &params, msg->hwnd, msg->message,
                                   msg->wParam, NtGetTickCount(), FALSE ))
            return 0;
        return dispatch_win_proc_params( &params, sizeof(params), NULL, NULL );
    }
    if (msg->message == WM_SYSTIMER)
    {
        switch (msg->wParam)
        {
            case SYSTEM_TIMER_CARET:
                toggle_caret( msg->hwnd );
                return 0;

            case SYSTEM_TIMER_TRACK_MOUSE:
                update_mouse_tracking_info( msg->hwnd );
                return 0;
        }
    }

    if (!msg->hwnd) return 0;

    spy_enter_message( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message, msg->wParam, msg->lParam );

    if (init_window_call_params( &params, msg->hwnd, msg->message, msg->wParam, msg->lParam,
                                 FALSE, WMCHAR_MAP_DISPATCHMESSAGE ))
        retval = dispatch_win_proc_params( &params, sizeof(params), NULL, NULL );
    else if (!is_window( msg->hwnd )) RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
    else RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );

    spy_exit_message( SPY_RESULT_OK, msg->hwnd, msg->message, retval, msg->wParam, msg->lParam );

    if (msg->message == WM_PAINT)
    {
        /* send a WM_NCPAINT and WM_ERASEBKGND if the non-client area is still invalid */
        HRGN hrgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtUserGetUpdateRgn( msg->hwnd, hrgn, TRUE );
        NtGdiDeleteObjectApp( hrgn );
    }
    return retval;
}

static BOOL is_message_broadcastable( UINT msg )
{
    return msg < WM_USER || msg >= 0xc000;
}

/***********************************************************************
 *           broadcast_message
 */
static BOOL broadcast_message( struct send_message_info *info, DWORD_PTR *res_ptr )
{
    HWND *list;

    if (is_message_broadcastable( info->msg ) &&
        (list = list_window_children( 0 )))
    {
        int i;

        for (i = 0; list[i]; i++)
        {
            if (!is_window(list[i])) continue;
            if ((get_window_long( list[i], GWL_STYLE ) & (WS_POPUP|WS_CHILD)) == WS_CHILD)
                continue;

            switch(info->type)
            {
            case MSG_UNICODE:
            case MSG_OTHER_PROCESS:
                send_message_timeout( list[i], info->msg, info->wparam, info->lparam,
                                      info->flags, info->timeout, FALSE );
                break;
            case MSG_ASCII:
                send_message_timeout( list[i], info->msg, info->wparam, info->lparam,
                                      info->flags, info->timeout, TRUE );
                break;
            case MSG_NOTIFY:
                NtUserMessageCall( list[i], info->msg, info->wparam, info->lparam,
                                   0, NtUserSendNotifyMessage, FALSE );
                break;
            case MSG_CALLBACK:
                {
                    struct send_message_callback_params params =
                        { .callback = info->callback, .data = info->data };
                    NtUserMessageCall( list[i], info->msg, info->wparam, info->lparam,
                                       &params, NtUserSendMessageCallback, FALSE );
                    break;
                }
            case MSG_POSTED:
                NtUserPostMessage( list[i], info->msg, info->wparam, info->lparam );
                break;
            default:
                ERR( "bad type %d\n", info->type );
                break;
            }
        }

        free( list );
    }

    if (res_ptr) *res_ptr = 1;
    return TRUE;
}

static inline void *get_buffer( void *static_buffer, size_t size, size_t need )
{
    if (size >= need) return static_buffer;
    return malloc( need );
}

static inline void free_buffer( void *static_buffer, void *buffer )
{
    if (buffer != static_buffer) free( buffer );
}

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );

/**********************************************************************
 *           test_lb_for_string
 *
 * Return TRUE if the lparam is a string
 */
static inline BOOL test_lb_for_string( HWND hwnd, UINT msg )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    if (msg <= CB_MSGMAX)
        return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
    else
        return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));

}

static CPTABLEINFO *get_input_codepage( void )
{
    const NLS_LOCALE_DATA *locale;
    HKL hkl = NtUserGetKeyboardLayout( 0 );
    CPTABLEINFO *ret = NULL;

    locale = get_locale_data( LOWORD(hkl) );
    if (locale && locale->idefaultansicodepage != CP_UTF8)
        ret = get_cptable( locale->idefaultansicodepage );
    return ret ? ret : &ansi_cp;
}

/***********************************************************************
 *           map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
static BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping )
{
    char ch[2];
    WCHAR wch[2];

    wch[0] = wch[1] = 0;
    switch(message)
    {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        ch[0] = LOBYTE(*wparam);
        ch[1] = HIBYTE(*wparam);
        win32u_mbtowc( get_input_codepage(), wch, 2, ch, 2 );
        *wparam = MAKEWPARAM(wch[0], wch[1]);
        break;

    case WM_IME_CHAR:
        ch[0] = HIBYTE(*wparam);
        ch[1] = LOBYTE(*wparam);
        if (ch[0]) win32u_mbtowc( get_input_codepage(), wch, 2, ch, 2 );
        else win32u_mbtowc( get_input_codepage(), wch, 1, ch + 1, 1 );
        *wparam = MAKEWPARAM(wch[0], HIWORD(*wparam));
        break;
    }
    return TRUE;
}

/**********************************************************************
 *           call_messageAtoW
 *
 * Call a window procedure, translating args from Ansi to Unicode.
 */
static LRESULT call_messageAtoW( winproc_callback_t callback, HWND hwnd, UINT msg, WPARAM wparam,
                                 LPARAM lparam, LRESULT *result, void *arg, enum wm_char_mapping mapping )
{
    LRESULT ret = 0;

    TRACE( "(hwnd=%p,msg=%s,wp=%#lx,lp=%#lx)\n", hwnd, debugstr_msg_name( msg, hwnd ), (long)wparam, lparam );

    switch(msg)
    {
    case WM_MDICREATE:
        {
            WCHAR *ptr, buffer[512];
            DWORD title_lenA = 0, title_lenW = 0, class_lenA = 0, class_lenW = 0;
            MDICREATESTRUCTA *csA = (MDICREATESTRUCTA *)lparam;
            MDICREATESTRUCTW csW;

            memcpy( &csW, csA, sizeof(csW) );

            if (!IS_INTRESOURCE(csA->szTitle))
            {
                title_lenA = strlen(csA->szTitle) + 1;
                title_lenW = win32u_mbtowc_size( &ansi_cp, csA->szTitle, title_lenA );
            }
            if (!IS_INTRESOURCE(csA->szClass))
            {
                class_lenA = strlen(csA->szClass) + 1;
                class_lenW = win32u_mbtowc_size( &ansi_cp, csA->szClass, class_lenA );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), (title_lenW + class_lenW) * sizeof(WCHAR) )))
                break;

            if (title_lenW)
            {
                csW.szTitle = ptr;
                win32u_mbtowc( &ansi_cp, ptr, title_lenW, csA->szTitle, title_lenA );
            }
            if (class_lenW)
            {
                csW.szClass = ptr + title_lenW;
                win32u_mbtowc( &ansi_cp, ptr + title_lenW, class_lenW, csA->szClass, class_lenA );
            }
            ret = callback( hwnd, msg, wparam, (LPARAM)&csW, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        {
            WCHAR *ptr, buffer[512];
            LPSTR str = (LPSTR)lparam;
            DWORD len = wparam * sizeof(WCHAR);

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len ))) break;
            ret = callback( hwnd, msg, wparam, (LPARAM)ptr, result, arg );
            if (wparam)
            {
                len = 0;
                len = *result ? win32u_wctomb( &ansi_cp, str, wparam - 1, ptr, *result ) : 0;
                str[len] = 0;
                *result = len;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (!lparam || !test_lb_for_string( hwnd, msg ))
        {
            ret = callback( hwnd, msg, wparam, lparam, result, arg );
            break;
        }
        /* fall through */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!lparam)
        {
            ret = callback( hwnd, msg, wparam, lparam, result, arg );
        }
        else
        {
            WCHAR *ptr, buffer[512];
            LPCSTR strA = (LPCSTR)lparam;
            DWORD lenW, lenA = strlen(strA) + 1;

            lenW = win32u_mbtowc_size( &ansi_cp, strA, lenA );
            if ((ptr = get_buffer( buffer, sizeof(buffer), lenW * sizeof(WCHAR) )))
            {
                win32u_mbtowc( &ansi_cp, ptr, lenW, strA, lenA );
                ret = callback( hwnd, msg, wparam, (LPARAM)ptr, result, arg );
                free_buffer( buffer, ptr );
            }
        }
        break;

    case EM_GETLINE:
        {
            WCHAR *ptr, buffer[512];
            WORD len = *(WORD *)lparam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * sizeof(WCHAR) ))) break;
            *((WORD *)ptr) = len;   /* store the length */
            ret = callback( hwnd, msg, wparam, (LPARAM)ptr, result, arg );
            if (*result)
            {
                DWORD reslen;
                reslen = win32u_wctomb( &ansi_cp, (char *)lparam, len, ptr, *result );
                if (reslen < len) ((LPSTR)lparam)[reslen] = 0;
                *result = reslen;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETDLGCODE:
        if (lparam)
        {
            MSG newmsg = *(MSG *)lparam;
            if (map_wparam_AtoW( newmsg.message, &newmsg.wParam, WMCHAR_MAP_NOMAPPING ))
                ret = callback( hwnd, msg, wparam, (LPARAM)&newmsg, result, arg );
        }
        else ret = callback( hwnd, msg, wparam, lparam, result, arg );
        break;

    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case EM_SETPASSWORDCHAR:
    case WM_IME_CHAR:
        if (map_wparam_AtoW( msg, &wparam, mapping ))
            ret = callback( hwnd, msg, wparam, lparam, result, arg );
        break;

    case WM_GETTEXTLENGTH:
    case CB_GETLBTEXTLEN:
    case LB_GETTEXTLEN:
        ret = callback( hwnd, msg, wparam, lparam, result, arg );
        if (*result >= 0)
        {
            WCHAR *ptr, buffer[512];
            LRESULT res;
            DWORD len = *result + 1;
            /* Determine respective GETTEXT message */
            UINT msg_get_text = msg == WM_GETTEXTLENGTH ? WM_GETTEXT :
                (msg == CB_GETLBTEXTLEN ? CB_GETLBTEXT : LB_GETTEXT);
            /* wparam differs between the messages */
            WPARAM wp = msg == WM_GETTEXTLENGTH ? len : wparam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * sizeof(WCHAR) ))) break;

            res = send_message( hwnd, msg_get_text, wp, (LPARAM)ptr );
            *result = win32u_wctomb_size( &ansi_cp, ptr, res );
            free_buffer( buffer, ptr );
        }
        break;

    default:
        ret = callback( hwnd, msg, wparam, lparam, result, arg );
        break;
    }
    return ret;
}

/***********************************************************************
 *           process_message
 *
 * Backend implementation of the various SendMessage functions.
 */
static BOOL process_message( struct send_message_info *info, DWORD_PTR *res_ptr, BOOL ansi )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    INPUT_MESSAGE_SOURCE prev_source = thread_info->msg_source;
    DWORD dest_pid;
    BOOL ret;
    LRESULT result = 0;

    if (is_broadcast( info->hwnd )) return broadcast_message( info, res_ptr );

    if (!(info->dest_tid = get_window_thread( info->hwnd, &dest_pid ))) return FALSE;
    if (is_exiting_thread( info->dest_tid )) return FALSE;

    if (info->params && info->dest_tid == GetCurrentThreadId() &&
        !is_hooked( WH_CALLWNDPROC ) && !is_hooked( WH_CALLWNDPROCRET ) &&
        thread_info->recursion_count <= MAX_WINPROC_RECURSION)
    {
        /* if we're called from client side and need just a simple winproc call,
         * just fill dispatch params and let user32 do the rest */
        return init_window_call_params( info->params, info->hwnd, info->msg, info->wparam, info->lparam,
                                        ansi, info->wm_char );
    }

    thread_info->msg_source = msg_source_unavailable;
    spy_enter_message( SPY_SENDMESSAGE, info->hwnd, info->msg, info->wparam, info->lparam );

    if (info->dest_tid != GetCurrentThreadId() ||
        /* When the callback pointer is NULL and the data is 1, don't wait for the result */
        (info->type == MSG_CALLBACK && !info->callback && info->data == 1))
    {
        if (dest_pid != GetCurrentProcessId() && (info->type == MSG_ASCII || info->type == MSG_UNICODE))
            info->type = MSG_OTHER_PROCESS;

        /* MSG_ASCII can be sent unconverted except for WM_CHAR; everything else needs to be Unicode */
        if (ansi && (info->type != MSG_ASCII || info->msg == WM_CHAR))
            ret = call_messageAtoW( send_inter_thread_callback, info->hwnd, info->msg,
                                    info->wparam, info->lparam, &result, info, info->wm_char );
        else
            ret = send_inter_thread_message( info, &result );
    }
    else
    {
        result = call_window_proc( info->hwnd, info->msg, info->wparam, info->lparam,
                                   info->type, TRUE, info->wm_char, ansi );
        if (info->type == MSG_CALLBACK)
            call_sendmsg_callback( info->callback, info->hwnd, info->msg, info->data, result );
        ret = TRUE;
    }

    spy_exit_message( SPY_RESULT_OK, info->hwnd, info->msg, result, info->wparam, info->lparam );
    thread_info->msg_source = prev_source;
    if (ret && res_ptr) *res_ptr = result;
    return ret;
}

/***********************************************************************
 *           NtUserSetTimer (win32u.@)
 */
UINT_PTR WINAPI NtUserSetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc, ULONG tolerance )
{
    UINT_PTR ret;
    WNDPROC winproc = 0;

    if (proc) winproc = alloc_winproc( (WNDPROC)proc, TRUE );

    timeout = min( max( USER_TIMER_MINIMUM, timeout ), USER_TIMER_MAXIMUM );

    SERVER_START_REQ( set_win_timer )
    {
        req->win    = wine_server_user_handle( hwnd );
        req->msg    = WM_TIMER;
        req->id     = id;
        req->rate   = timeout;
        req->lparam = (ULONG_PTR)winproc;
        if (!wine_server_call_err( req ))
        {
            ret = reply->id;
            if (!ret) ret = TRUE;
        }
        else ret = 0;
    }
    SERVER_END_REQ;

    TRACE( "Added %p %lx %p timeout %d\n", hwnd, (long)id, winproc, timeout );
    return ret;
}

/***********************************************************************
 *           NtUserSetSystemTimer (win32u.@)
 */
UINT_PTR WINAPI NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout )
{
    UINT_PTR ret;

    TRACE( "window %p, id %#lx, timeout %u\n", hwnd, (long)id, timeout );

    timeout = min( max( USER_TIMER_MINIMUM, timeout ), USER_TIMER_MAXIMUM );

    SERVER_START_REQ( set_win_timer )
    {
        req->win    = wine_server_user_handle( hwnd );
        req->msg    = WM_SYSTIMER;
        req->id     = id;
        req->rate   = timeout;
        req->lparam = 0;
        if (!wine_server_call_err( req ))
        {
            ret = reply->id;
            if (!ret) ret = TRUE;
        }
        else ret = 0;
    }
    SERVER_END_REQ;

    return ret;
}

/***********************************************************************
 *           NtUserKillTimer (win32u.@)
 */
BOOL WINAPI NtUserKillTimer( HWND hwnd, UINT_PTR id )
{
    BOOL ret;

    SERVER_START_REQ( kill_win_timer )
    {
        req->win = wine_server_user_handle( hwnd );
        req->msg = WM_TIMER;
        req->id  = id;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserKillSystemTimer (win32u.@)
 */
BOOL WINAPI NtUserKillSystemTimer( HWND hwnd, UINT_PTR id )
{
    BOOL ret;

    SERVER_START_REQ( kill_win_timer )
    {
        req->win = wine_server_user_handle( hwnd );
        req->msg = WM_SYSTIMER;
        req->id  = id;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

static LRESULT send_window_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    struct win_proc_params *client_params, BOOL ansi )
{
    struct send_message_info info;
    DWORD_PTR res = 0;

    info.type    = ansi ? MSG_ASCII : MSG_UNICODE;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = SMTO_NORMAL;
    info.timeout = 0;
    info.wm_char = WMCHAR_MAP_SENDMESSAGE;
    info.params  = client_params;

    process_message( &info, &res, ansi );
    return res;
}

/* see SendMessageTimeoutW */
static LRESULT send_client_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, DWORD_PTR *res_ptr, BOOL ansi )
{
    struct send_message_info info;

    info.type    = ansi ? MSG_ASCII : MSG_UNICODE;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = flags;
    info.timeout = timeout;
    info.wm_char = WMCHAR_MAP_SENDMESSAGETIMEOUT;
    info.params  = NULL;

    return process_message( &info, res_ptr, ansi );
}

LRESULT send_message_timeout( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                              UINT flags, UINT timeout, BOOL ansi )
{
    struct send_message_info info;
    DWORD_PTR res = 0;

    if (!is_pointer_message( msg, wparam ))
    {
        send_client_message( hwnd, msg, wparam, lparam, flags, timeout, &res, ansi );
        return res;
    }

    info.type    = MSG_OTHER_PROCESS;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = flags;
    info.timeout = timeout;
    info.wm_char = WMCHAR_MAP_SENDMESSAGETIMEOUT;
    info.params  = NULL;

    process_message( &info, &res, ansi );
    return res;
}

/* see SendMessageW */
LRESULT send_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return send_message_timeout( hwnd, msg, wparam, lparam, SMTO_NORMAL, 0, FALSE );
}

/* see SendNotifyMessageW */
BOOL send_notify_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    struct send_message_info info;

    if (is_pointer_message( msg, wparam ))
    {
        RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    info.type    = MSG_NOTIFY;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = 0;
    info.wm_char = WMCHAR_MAP_SENDMESSAGETIMEOUT;
    info.params  = NULL;

    return process_message( &info, NULL, ansi );
}

/* see SendMessageCallbackW */
static BOOL send_message_callback( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                   const struct send_message_callback_params *params, BOOL ansi )
{
    struct send_message_info info;

    if (is_pointer_message( msg, wparam ))
    {
        RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    info.type     = MSG_CALLBACK;
    info.hwnd     = hwnd;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.callback = params->callback;
    info.data     = params->data;
    info.flags    = 0;
    info.wm_char  = WMCHAR_MAP_SENDMESSAGETIMEOUT;
    info.params   = NULL;

    return process_message( &info, NULL, ansi );
}

/***********************************************************************
 *           NtUserPostMessage  (win32u.@)
 */
BOOL WINAPI NtUserPostMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message( msg, wparam ))
    {
        RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    TRACE( "hwnd %p msg %x (%s) wp %lx lp %lx\n",
           hwnd, msg, debugstr_msg_name(msg, hwnd), (long)wparam, lparam );

    info.type   = MSG_POSTED;
    info.hwnd   = hwnd;
    info.msg    = msg;
    info.wparam = wparam;
    info.lparam = lparam;
    info.flags  = 0;
    info.params = NULL;

    if (is_broadcast(hwnd)) return broadcast_message( &info, NULL );

    if (!hwnd) return NtUserPostThreadMessage( GetCurrentThreadId(), msg, wparam, lparam );

    if (!(info.dest_tid = get_window_thread( hwnd, NULL ))) return FALSE;

    if (is_exiting_thread( info.dest_tid )) return TRUE;

    return put_message_in_queue( &info, NULL );
}

/**********************************************************************
 *           NtUserPostThreadMessage  (win32u.@)
 */
BOOL WINAPI NtUserPostThreadMessage( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message( msg, wparam ))
    {
        RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }
    if (is_exiting_thread( thread )) return TRUE;

    info.type     = MSG_POSTED;
    info.dest_tid = thread;
    info.hwnd     = 0;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.flags    = 0;
    info.params   = NULL;
    return put_message_in_queue( &info, NULL );
}

LRESULT WINAPI NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  void *result_info, DWORD type, BOOL ansi )
{
    switch (type)
    {
    case NtUserScrollBarWndProc:
        return scroll_bar_window_proc( hwnd, msg, wparam, lparam, ansi );

    case NtUserPopupMenuWndProc:
        return popup_menu_window_proc( hwnd, msg, wparam, lparam, ansi );

    case NtUserDesktopWindowProc:
        return desktop_window_proc( hwnd, msg, wparam, lparam, ansi );

    case NtUserDefWindowProc:
        return default_window_proc( hwnd, msg, wparam, lparam, ansi );

    case NtUserCallWindowProc:
        return init_win_proc_params( (struct win_proc_params *)result_info, hwnd, msg,
                                     wparam, lparam, ansi );

    case NtUserSendMessage:
        return send_window_message( hwnd, msg, wparam, lparam, result_info, ansi );

    case NtUserSendMessageTimeout:
        {
            struct send_message_timeout_params *params = (void *)result_info;
            DWORD_PTR res = 0;
            params->result = send_client_message( hwnd, msg, wparam, lparam, params->flags,
                                                  params->timeout, &res, ansi );
            return res;
        }

    case NtUserSendNotifyMessage:
        return send_notify_message( hwnd, msg, wparam, lparam, ansi );

    case NtUserSendMessageCallback:
        return send_message_callback( hwnd, msg, wparam, lparam, result_info, ansi );

    case NtUserClipboardWindowProc:
        return user_driver->pClipboardWindowProc( hwnd, msg, wparam, lparam );

    case NtUserGetDispatchParams:
        if (!hwnd) return FALSE;
        if (init_window_call_params( result_info, hwnd, msg, wparam, lparam,
                                     ansi, WMCHAR_MAP_DISPATCHMESSAGE ))
            return TRUE;
        if (!is_window( hwnd )) RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        else RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;

    case NtUserSpyEnter:
        spy_enter_message( ansi, hwnd, msg, wparam, lparam );
        return 0;

    case NtUserSpyGetMsgName:
        lstrcpynA( result_info, debugstr_msg_name( msg, hwnd ), wparam );
        return 0;

    case NtUserSpyExit:
        spy_exit_message( ansi, hwnd, msg, (LPARAM)result_info, wparam, lparam );
        return 0;

    case NtUserImeDriverCall:
        return ime_driver_call( hwnd, msg, wparam, lparam, result_info );

    case NtUserSystemTrayCall:
        return system_tray_call( hwnd, msg, wparam, lparam, result_info );

    case NtUserDragDropCall:
        return drag_drop_call( hwnd, msg, wparam, lparam, result_info );

    case NtUserPostDdeCall:
        return post_dde_message_call( hwnd, msg, wparam, lparam, result_info );

    default:
        FIXME( "%p %x %lx %lx %p %x %x\n", hwnd, msg, (long)wparam, lparam, result_info, type, ansi );
    }
    return 0;
}

/***********************************************************************
 *           NtUserTranslateMessage (win32u.@)
 */
BOOL WINAPI NtUserTranslateMessage( const MSG *msg, UINT flags )
{
    UINT message;
    WCHAR wp[8];
    BYTE state[256];
    INT len;

    if (flags) FIXME( "unsupported flags %x\n", flags );

    if (msg->message < WM_KEYFIRST || msg->message > WM_KEYLAST) return FALSE;
    if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
    {
        if (msg->wParam != VK_PROCESSKEY) return TRUE;
        return ImmTranslateMessage( msg->hwnd, msg->message, msg->wParam, msg->lParam );
    }
    if (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) return TRUE;

    TRACE_(key)( "Translating key %s (%04x), scancode %04x\n",
                 debugstr_vkey_name( msg->wParam ), LOWORD(msg->wParam), HIWORD(msg->lParam) );

    switch (msg->wParam)
    {
    case VK_PACKET:
        message = (msg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
        TRACE_(key)( "PostMessageW(%p,%s,%04x,%08x)\n", msg->hwnd,
                     debugstr_msg_name( message, msg->hwnd ),
                     HIWORD(msg->lParam), LOWORD(msg->lParam) );
        NtUserPostMessage( msg->hwnd, message, HIWORD(msg->lParam), LOWORD(msg->lParam) );
        return TRUE;

    case VK_PROCESSKEY:
        return ImmTranslateMessage( msg->hwnd, msg->message, msg->wParam, msg->lParam );
    }

    NtUserGetKeyboardState( state );
    len = NtUserToUnicodeEx( msg->wParam, HIWORD(msg->lParam), state, wp, ARRAY_SIZE(wp), 0,
                             NtUserGetKeyboardLayout(0) );
    if (len == -1)
    {
        message = msg->message == WM_KEYDOWN ? WM_DEADCHAR : WM_SYSDEADCHAR;
        TRACE_(key)( "-1 -> PostMessageW(%p,%s,%04x,%08lx)\n",
                     msg->hwnd, debugstr_msg_name( message, msg->hwnd ), wp[0], msg->lParam );
        NtUserPostMessage( msg->hwnd, message, wp[0], msg->lParam );
    }
    else if (len > 0)
    {
        INT i;

        message = msg->message == WM_KEYDOWN ? WM_CHAR : WM_SYSCHAR;
        TRACE_(key)( "%d -> PostMessageW(%p,%s,<x>,%08lx) for <x> in %s\n", len, msg->hwnd,
                     debugstr_msg_name(message, msg->hwnd), msg->lParam, debugstr_wn(wp, len) );
        for (i = 0; i < len; i++)
            NtUserPostMessage( msg->hwnd, message, wp[i], msg->lParam );
    }
    return TRUE;
}

/***********************************************************************
 *           NtUserGetCurrentInputMessageSource (win32u.@)
 */
BOOL WINAPI NtUserGetCurrentInputMessageSource( INPUT_MESSAGE_SOURCE *source )
{
    TRACE( "source %p.\n", source );

    if (!source)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    *source = NtUserGetThreadInfo()->msg_source;
    return TRUE;
}
