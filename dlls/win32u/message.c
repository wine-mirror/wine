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
#include "win32u_private.h"
#include "ntuser_private.h"
#include "hidusage.h"
#include "dbt.h"
#include "dde.h"
#include "ddk/imm.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(key);
WINE_DECLARE_DEBUG_CHANNEL(relay);

#define MAX_WINPROC_RECURSION  64

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

#define MAX_PACK_COUNT 4

/* info about the message currently being received by the current thread */
struct received_message_info
{
    UINT  type;
    MSG   msg;
    UINT  flags;  /* InSendMessageEx return flags */
    LRESULT result;
    struct received_message_info *prev;
};

#define MSG_CLIENT_MESSAGE 0xff

struct packed_hook_extra_info
{
    user_handle_t handle;
    DWORD         __pad;
    ULONGLONG     lparam;
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
    0,
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
    params->needs_unpack = FALSE;
    params->mapping = WMCHAR_MAP_CALLWINDOWPROC;
    params->dpi_awareness = get_window_dpi_awareness_context( params->hwnd );
    get_winproc_params( params, TRUE );
    return TRUE;
}

static BOOL init_window_call_params( struct win_proc_params *params, HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, LRESULT *result, BOOL ansi,
                                     enum wm_char_mapping mapping )
{
    BOOL is_dialog;
    WND *win;

    user_check_not_lock();

    if (!(win = get_win_ptr( hwnd ))) return FALSE;
    if (win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;
    if (win->tid != GetCurrentThreadId())
    {
        release_win_ptr( win );
        return FALSE;
    }
    params->func = win->winproc;
    params->ansi_dst = !(win->flags & WIN_ISUNICODE);
    is_dialog = win->dlgInfo != NULL;
    release_win_ptr( win );

    params->hwnd = get_full_window_handle( hwnd );
    params->msg = msg;
    params->wparam = wParam;
    params->lparam = lParam;
    params->result = result;
    params->ansi = ansi;
    params->needs_unpack = FALSE;
    params->mapping = mapping;
    params->dpi_awareness = get_window_dpi_awareness_context( params->hwnd );
    get_winproc_params( params, !is_dialog );
    return TRUE;
}

static BOOL dispatch_win_proc_params( struct win_proc_params *params, size_t size )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    void *ret_ptr;
    ULONG ret_len;

    if (thread_info->recursion_count > MAX_WINPROC_RECURSION) return FALSE;
    thread_info->recursion_count++;

    KeUserModeCallback( NtUserCallWinProc, params, size, &ret_ptr, &ret_len );
    if (ret_len == sizeof(*params->result)) *params->result = *(LRESULT *)ret_ptr;

    thread_info->recursion_count--;
    return TRUE;
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
                            void **buffer, size_t size )
{
    size_t minsize = 0;
    union packed_structs *ps = *buffer;

    switch(message)
    {
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
        }
        break;
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
    case WM_GETDLGCODE:
        if (lparam && size >= sizeof(ps->msg))
        {
            MSG *msg = (MSG *)lparam;
            msg->hwnd    = wine_server_ptr_handle( ps->msg.hwnd );
            msg->message = ps->msg.message;
            msg->wParam  = (ULONG_PTR)unpack_ptr( ps->msg.wParam );
            msg->lParam  = (ULONG_PTR)unpack_ptr( ps->msg.lParam );
            msg->time    = ps->msg.time;
            msg->pt      = ps->msg.pt;
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

/***********************************************************************
 *           copy_reply
 *
 * Copy a message reply received from client.
 */
static void copy_reply( LRESULT result, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                        WPARAM wparam_src, LPARAM lparam_src )
{
    size_t copy_size = 0;

    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTW *dst = (CREATESTRUCTW *)lparam;
            CREATESTRUCTW *src = (CREATESTRUCTW *)lparam_src;
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
    case WM_GETTEXT:
    case CB_GETLBTEXT:
    case LB_GETTEXT:
        copy_size = (result + 1) * sizeof(WCHAR);
        break;
    case WM_GETMINMAXINFO:
        copy_size = sizeof(MINMAXINFO);
        break;
    case WM_MEASUREITEM:
        copy_size = sizeof(MEASUREITEMSTRUCT);
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        copy_size = sizeof(WINDOWPOS);
        break;
    case WM_STYLECHANGING:
        copy_size = sizeof(STYLESTRUCT);
        break;
    case WM_GETDLGCODE:
        if (lparam) copy_size = sizeof(MSG);
        break;
    case SBM_GETSCROLLINFO:
        copy_size = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLBARINFO:
        copy_size = sizeof(SCROLLBARINFO);
        break;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case WM_SIZING:
    case WM_MOVING:
        copy_size = sizeof(RECT);
        break;
    case EM_GETLINE:
    {
        WORD *ptr = (WORD *)lparam;
        copy_size = ptr[-1] * sizeof(WCHAR);
        break;
    }
    case LB_GETSELITEMS:
        copy_size = wparam * sizeof(UINT);
        break;
    case WM_MDIGETACTIVE:
        if (lparam) copy_size = sizeof(BOOL);
        break;
    case WM_NCCALCSIZE:
        if (wparam)
        {
            NCCALCSIZE_PARAMS *dst = (NCCALCSIZE_PARAMS *)lparam;
            NCCALCSIZE_PARAMS *src = (NCCALCSIZE_PARAMS *)lparam_src;
            dst->rgrc[0] = src->rgrc[0];
            dst->rgrc[1] = src->rgrc[1];
            dst->rgrc[2] = src->rgrc[2];
            *dst->lppos = *src->lppos;
            return;
        }
        copy_size = sizeof(RECT);
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam) *(DWORD *)wparam = *(DWORD *)wparam_src;
        if (lparam) copy_size = sizeof(DWORD);
        break;
    case WM_NEXTMENU:
        copy_size = sizeof(MDINEXTMENU);
        break;
    case WM_MDICREATE:
        copy_size = sizeof(MDICREATESTRUCTW);
        break;
    case WM_ASKCBFORMATNAME:
        copy_size = (lstrlenW((WCHAR *)lparam) + 1) * sizeof(WCHAR);
        break;
    default:
        return;
    }

    if (copy_size) memcpy( (void *)lparam, (void *)lparam_src, copy_size );
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
 *           reply_message_result
 *
 * Send a reply to a sent message and update thread receive info.
 */
BOOL reply_message_result( LRESULT result )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct received_message_info *info = thread_info->receive_info;

    while (info && info->type == MSG_CLIENT_MESSAGE) info = info->prev;
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

    if (info->type == MSG_CLIENT_MESSAGE)
    {
        copy_reply( result, hwnd, message, info->msg.wParam, info->msg.lParam, wparam, lparam );
        info->result = result;
        return TRUE;
    }

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
        return set_window_style( hwnd, wparam, lparam );
    case WM_WINE_SETACTIVEWINDOW:
        if (!wparam && NtUserGetForegroundWindow() == hwnd) return 0;
        return (LRESULT)NtUserSetActiveWindow( (HWND)wparam );
    case WM_WINE_KEYBOARD_LL_HOOK:
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;

        return call_current_hook( h_extra->handle, HC_ACTION, wparam, h_extra->lparam );
    }
    case WM_WINE_CLIPCURSOR:
        if (wparam)
        {
            RECT rect;
            get_clip_cursor( &rect );
            return user_driver->pClipCursor( &rect );
        }
        return user_driver->pClipCursor( NULL );
    case WM_WINE_UPDATEWINDOWSTATE:
        update_window_state( hwnd );
        return 0;
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
    BOOL ret;

    if (info->cbSize != sizeof(*info))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = id;
        if ((ret = !wine_server_call_err( req )))
        {
            info->flags          = 0;
            info->hwndActive     = wine_server_ptr_handle( reply->active );
            info->hwndFocus      = wine_server_ptr_handle( reply->focus );
            info->hwndCapture    = wine_server_ptr_handle( reply->capture );
            info->hwndMenuOwner  = wine_server_ptr_handle( reply->menu_owner );
            info->hwndMoveSize   = wine_server_ptr_handle( reply->move_size );
            info->hwndCaret      = wine_server_ptr_handle( reply->caret );
            info->rcCaret.left   = reply->rect.left;
            info->rcCaret.top    = reply->rect.top;
            info->rcCaret.right  = reply->rect.right;
            info->rcCaret.bottom = reply->rect.bottom;
            if (reply->menu_owner) info->flags |= GUI_INMENUMODE;
            if (reply->move_size) info->flags |= GUI_INMOVESIZE;
            if (reply->caret) info->flags |= GUI_CARETBLINKING;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           call_window_proc
 *
 * Call a window procedure and the corresponding hooks.
 */
static LRESULT call_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                 BOOL unicode, BOOL same_thread, enum wm_char_mapping mapping,
                                 BOOL needs_unpack, void *buffer, size_t size )
{
    struct win_proc_params p, *params = &p;
    LRESULT result = 0;
    CWPSTRUCT cwp;
    CWPRETSTRUCT cwpret;

    if (msg & 0x80000000)
        return handle_internal_message( hwnd, msg, wparam, lparam );

    if (!needs_unpack) size = 0;
    if (!is_current_thread_window( hwnd )) return 0;
    if (size && !(params = malloc( sizeof(*params) + size ))) return 0;
    if (!init_window_call_params( params, hwnd, msg, wparam, lparam, &result, !unicode, mapping ))
    {
        if (params != &p) free( params );
        return 0;
    }

    if (needs_unpack)
    {
        params->needs_unpack = TRUE;
        params->ansi = FALSE;
        if (size) memcpy( params + 1, buffer, size );
    }

    /* first the WH_CALLWNDPROC hook */
    cwp.lParam  = lparam;
    cwp.wParam  = wparam;
    cwp.message = msg;
    cwp.hwnd    = params->hwnd;
    call_hooks( WH_CALLWNDPROC, HC_ACTION, same_thread, (LPARAM)&cwp, sizeof(cwp) );

    dispatch_win_proc_params( params, sizeof(*params) + size );
    if (params != &p) free( params );

    /* and finally the WH_CALLWNDPROCRET hook */
    cwpret.lResult = result;
    cwpret.lParam  = lparam;
    cwpret.wParam  = wparam;
    cwpret.message = msg;
    cwpret.hwnd    = params->hwnd;
    call_hooks( WH_CALLWNDPROCRET, HC_ACTION, same_thread, (LPARAM)&cwpret, sizeof(cwpret) );
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

    if (call_hooks( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
                    LOWORD(msg->wParam), msg->lParam, 0 ))
    {
        /* skip this message */
        call_hooks( WH_CBT, HCBT_KEYSKIPPED, LOWORD(msg->wParam), msg->lParam, 0 );
        accept_hardware_message( hw_id );
        return FALSE;
    }
    if (remove) accept_hardware_message( hw_id );
    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );

    if (remove && msg->message == WM_KEYDOWN)
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

        msg->hwnd = window_from_point( msg->hwnd, msg->pt, &hittest );
        if (!msg->hwnd) /* As a heuristic, try the next window if it's the owner of orig */
        {
            HWND next = get_window_relative( orig, GW_HWNDNEXT );

            if (next && get_window_relative( orig, GW_OWNER ) == next &&
                is_current_thread_window( next ))
                msg->hwnd = window_from_point( next, msg->pt, &hittest );
        }
    }

    if (!msg->hwnd || !is_current_thread_window( msg->hwnd ))
    {
        accept_hardware_message( hw_id );
        return FALSE;
    }

    msg->pt = point_phys_to_win_dpi( msg->hwnd, msg->pt );
    SetThreadDpiAwarenessContext( get_window_dpi_awareness_context( msg->hwnd ));

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
        call_hooks( WH_CBT, HCBT_CLICKSKIPPED, message, (LPARAM)&hook, sizeof(hook) );
        accept_hardware_message( hw_id );
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
                LONG ret = send_message( msg->hwnd, WM_MOUSEACTIVATE, (WPARAM)hwndTop,
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
    DPI_AWARENESS_CONTEXT context;
    BOOL ret = FALSE;

    thread_info->msg_source.deviceType = msg_data->source.device;
    thread_info->msg_source.originId   = msg_data->source.origin;

    /* hardware messages are always in physical coords */
    context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );

    if (msg->message == WM_INPUT || msg->message == WM_INPUT_DEVICE_CHANGE)
        ret = process_rawinput_message( msg, hw_id, msg_data );
    else if (is_keyboard_message( msg->message ))
        ret = process_keyboard_message( msg, hw_id, hwnd_filter, first, last, remove );
    else if (is_mouse_message( msg->message ))
        ret = process_mouse_message( msg, hw_id, msg_data->info, hwnd_filter, first, last, remove );
    else
        ERR( "unknown message type %x\n", msg->message );
    SetThreadDpiAwarenessContext( context );
    return ret;
}

/***********************************************************************
 *           peek_message
 *
 * Peek for a message matching the given parameters. Return 0 if none are
 * available; -1 on error.
 * All pending sent messages are processed before returning.
 */
static int peek_message( MSG *msg, HWND hwnd, UINT first, UINT last, UINT flags, UINT changed_mask )
{
    LRESULT result;
    struct user_thread_info *thread_info = get_user_thread_info();
    INPUT_MESSAGE_SOURCE prev_source = thread_info->client_info.msg_source;
    struct received_message_info info;
    unsigned int hw_id = 0;  /* id of previous hardware message */
    void *buffer;
    size_t buffer_size = 1024;

    if (!(buffer = malloc( buffer_size ))) return -1;

    if (!first && !last) last = ~0;
    if (hwnd == HWND_BROADCAST) hwnd = HWND_TOPMOST;

    for (;;)
    {
        NTSTATUS res;
        size_t size = 0;
        const message_data_t *msg_data = buffer;
        BOOL needs_unpack = FALSE;

        thread_info->client_info.msg_source = prev_source;

        SERVER_START_REQ( get_message )
        {
            req->flags     = flags;
            req->get_win   = wine_server_user_handle( hwnd );
            req->get_first = first;
            req->get_last  = last;
            req->hw_id     = hw_id;
            req->wake_mask = changed_mask & (QS_SENDMESSAGE | QS_SMRESULT);
            req->changed_mask = changed_mask;
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
                thread_info->active_hooks = reply->active_hooks;
            }
            else buffer_size = reply->total;
        }
        SERVER_END_REQ;

        if (res)
        {
            free( buffer );
            if (res == STATUS_PENDING)
            {
                thread_info->wake_mask = changed_mask & (QS_SENDMESSAGE | QS_SMRESULT);
                thread_info->changed_mask = changed_mask;
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
               info.msg.hwnd, info.msg.wParam, info.msg.lParam );

        switch(info.type)
        {
        case MSG_ASCII:
        case MSG_UNICODE:
            info.flags = ISMEX_SEND;
            break;
        case MSG_NOTIFY:
            info.flags = ISMEX_NOTIFY;
            if (!unpack_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                 &info.msg.lParam, &buffer, size ))
                continue;
            needs_unpack = TRUE;
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
                       hook.vkCode, hook.scanCode, hook.flags, hook.time, hook.dwExtraInfo );
                result = call_hooks( WH_KEYBOARD_LL, HC_ACTION, info.msg.wParam,
                                     (LPARAM)&hook, sizeof(hook) );
            }
            else if (info.msg.message == WH_MOUSE_LL && size >= sizeof(msg_data->hardware))
            {
                MSLLHOOKSTRUCT hook;

                hook.pt          = info.msg.pt;
                hook.mouseData   = info.msg.lParam;
                hook.flags       = msg_data->hardware.flags;
                hook.time        = info.msg.time;
                hook.dwExtraInfo = msg_data->hardware.info;
                TRACE( "calling mouse LL hook pos %d,%d data %x flags %x time %u info %lx\n",
                       hook.pt.x, hook.pt.y, hook.mouseData, hook.flags, hook.time, hook.dwExtraInfo );
                result = call_hooks( WH_MOUSE_LL, HC_ACTION, info.msg.wParam,
                                     (LPARAM)&hook, sizeof(hook) );
            }
            reply_message( &info, result, &info.msg );
            continue;
        case MSG_OTHER_PROCESS:
            info.flags = ISMEX_SEND;
            if (!unpack_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                 &info.msg.lParam, &buffer, size ))
            {
                /* ignore it */
                reply_message( &info, 0, &info.msg );
                continue;
            }
            needs_unpack = TRUE;
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
                free( buffer );
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
                        free( buffer );
                        return 0;
                    }
                }
                else
                    peek_message( msg, info.msg.hwnd, info.msg.message,
                                  info.msg.message, flags | PM_REMOVE, changed_mask );
                continue;
            }
            if (info.msg.message >= WM_DDE_FIRST && info.msg.message <= WM_DDE_LAST)
            {
                struct unpack_dde_message_result result;
                struct unpack_dde_message_params *params;
                void *ret_ptr;
                ULONG len;
                BOOL ret;

                len = FIELD_OFFSET( struct unpack_dde_message_params, data[size] );
                if (!(params = malloc( len )))
                    continue;
                params->result  = &result;
                params->hwnd    = info.msg.hwnd;
                params->message = info.msg.message;
                params->wparam  = info.msg.wParam;
                params->lparam  = info.msg.lParam;
                if (size) memcpy( params->data, buffer, size );
                ret = KeUserModeCallback( NtUserUnpackDDEMessage, params, len, &ret_ptr, &len );
                if (len == sizeof(result)) result = *(struct unpack_dde_message_result *)ret_ptr;
                free( params );
                if (!ret) continue; /* ignore it */
                info.msg.wParam = result.wparam;
                info.msg.lParam = result.lparam;
            }
            *msg = info.msg;
            msg->pt = point_phys_to_win_dpi( info.msg.hwnd, info.msg.pt );
            thread_info->client_info.message_pos   = MAKELONG( msg->pt.x, msg->pt.y );
            thread_info->client_info.message_time  = info.msg.time;
            thread_info->client_info.message_extra = 0;
            thread_info->client_info.msg_source = msg_source_unavailable;
            free( buffer );
            call_hooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, sizeof(*msg) );
            return 1;
        }

        /* if we get here, we have a sent message; call the window procedure */
        info.prev = thread_info->receive_info;
        thread_info->receive_info = &info;
        thread_info->client_info.msg_source = msg_source_unavailable;
        thread_info->client_info.receive_flags = info.flags;
        result = call_window_proc( info.msg.hwnd, info.msg.message, info.msg.wParam,
                                   info.msg.lParam, (info.type != MSG_ASCII), FALSE,
                                   WMCHAR_MAP_RECVMESSAGE, needs_unpack, buffer, size );
        if (thread_info->receive_info == &info)
            reply_winproc_result( result, info.msg.hwnd, info.msg.message,
                                  info.msg.wParam, info.msg.lParam );

        /* if some PM_QS* flags were specified, only handle sent messages from now on */
        if (HIWORD(flags) && !changed_mask) flags = PM_QS_SENDMESSAGE | LOWORD(flags);
    }
}

/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages.
 */
static void process_sent_messages(void)
{
    MSG msg;
    peek_message( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE, 0 );
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
        SERVER_START_REQ( get_msg_queue )
        {
            wine_server_call( req );
            ret = wine_server_ptr_handle( reply->handle );
        }
        SERVER_END_REQ;
        thread_info->server_queue = ret;
        if (!ret) ERR( "Cannot get server thread queue\n" );
    }
    return ret;
}

/* check for driver events if we detect that the app is not properly consuming messages */
static inline void check_for_driver_events( UINT msg )
{
    if (get_user_thread_info()->message_count > 200)
    {
        LARGE_INTEGER zero = { .QuadPart = 0 };
        flush_window_surfaces( FALSE );
        user_driver->pMsgWaitForMultipleObjectsEx( 0, NULL, &zero, QS_ALLINPUT, 0 );
    }
    else if (msg == WM_TIMER || msg == WM_SYSTIMER)
    {
        /* driver events should have priority over timers, so make sure we'll check for them soon */
        get_user_thread_info()->message_count += 100;
    }
    else get_user_thread_info()->message_count++;
}

/* helper for kernel32->ntdll timeout format conversion */
static inline LARGE_INTEGER *get_nt_timeout( LARGE_INTEGER *time, DWORD timeout )
{
    if (timeout == INFINITE) return NULL;
    time->QuadPart = (ULONGLONG)timeout * -10000;
    return time;
}

/* wait for message or signaled handle */
static DWORD wait_message( DWORD count, const HANDLE *handles, DWORD timeout, DWORD mask, DWORD flags )
{
    LARGE_INTEGER time;
    DWORD ret, lock;
    void *ret_ptr;
    ULONG ret_len;

    if (enable_thunk_lock)
        lock = KeUserModeCallback( NtUserThunkLock, NULL, 0, &ret_ptr, &ret_len );

    ret = user_driver->pMsgWaitForMultipleObjectsEx( count, handles, get_nt_timeout( &time, timeout ),
                                                     mask, flags );
    if (HIWORD(ret))  /* is it an error code? */
    {
        RtlSetLastWin32Error( RtlNtStatusToDosError(ret) );
        ret = WAIT_FAILED;
    }
    if (ret == WAIT_TIMEOUT && !count && !timeout) NtYieldExecution();
    if ((mask & QS_INPUT) == QS_INPUT) get_user_thread_info()->message_count = 0;

    if (enable_thunk_lock)
        KeUserModeCallback( NtUserThunkLock, &lock, sizeof(lock), &ret_ptr, &ret_len );

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
    struct user_thread_info *thread_info = get_user_thread_info();
    DWORD ret;

    assert( count );  /* we must have at least the server queue */

    flush_window_surfaces( TRUE );

    if (thread_info->wake_mask != wake_mask || thread_info->changed_mask != changed_mask)
    {
        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = wake_mask;
            req->changed_mask = changed_mask;
            req->skip_wait    = 0;
            wine_server_call( req );
        }
        SERVER_END_REQ;
        thread_info->wake_mask = wake_mask;
        thread_info->changed_mask = changed_mask;
    }

    ret = wait_message( count, handles, timeout, changed_mask, flags );

    if (ret != WAIT_TIMEOUT) thread_info->wake_mask = thread_info->changed_mask = 0;
    return ret;
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
 *           NtUserPeekMessage  (win32u.@)
 */
BOOL WINAPI NtUserPeekMessage( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    MSG msg;
    int ret;

    user_check_not_lock();
    check_for_driver_events( 0 );

    ret = peek_message( &msg, hwnd, first, last, flags, 0 );
    if (ret < 0) return FALSE;

    if (!ret)
    {
        flush_window_surfaces( TRUE );
        ret = wait_message( 0, NULL, 0, QS_ALLINPUT, 0 );
        /* if we received driver events, check again for a pending message */
        if (ret == WAIT_TIMEOUT || peek_message( &msg, hwnd, first, last, flags, 0 ) <= 0) return FALSE;
    }

    check_for_driver_events( msg.message );

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
    HANDLE server_queue = get_server_queue_handle();
    unsigned int mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    int ret;

    user_check_not_lock();
    check_for_driver_events( 0 );

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

    while (!(ret = peek_message( msg, hwnd, first, last, PM_REMOVE | (mask << 16), mask )))
    {
        wait_objects( 1, &server_queue, INFINITE, mask & (QS_SENDMESSAGE | QS_SMRESULT), mask, 0 );
    }
    if (ret < 0) return -1;

    check_for_driver_events( msg->message );

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
    message_data_t msg_data;
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
        params.type     = info->type;
        return KeUserModeCallback( NtUserPostDDEMessage, &params, sizeof(params), &ret_ptr, &ret_len );
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
        if ((res = wine_server_call( req )))
        {
            if (res == STATUS_INVALID_PARAMETER)
                /* FIXME: find a STATUS_ value for this one */
                RtlSetLastWin32Error( ERROR_INVALID_THREAD_ID );
            else
                RtlSetLastWin32Error( RtlNtStatusToDosError(res) );
        }
    }
    SERVER_END_REQ;
    return !res;
}

/***********************************************************************
 *           wait_message_reply
 *
 * Wait until a sent message gets replied to.
 */
static void wait_message_reply( UINT flags )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    HANDLE server_queue = get_server_queue_handle();
    unsigned int wake_mask = QS_SMRESULT | ((flags & SMTO_BLOCK) ? 0 : QS_SENDMESSAGE);

    for (;;)
    {
        unsigned int wake_bits = 0;

        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = wake_mask;
            req->changed_mask = wake_mask;
            req->skip_wait    = 1;
            if (!wine_server_call( req )) wake_bits = reply->wake_bits & wake_mask;
        }
        SERVER_END_REQ;

        thread_info->wake_mask = thread_info->changed_mask = 0;

        if (wake_bits & QS_SMRESULT) return;  /* got a result */
        if (wake_bits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            process_sent_messages();
            continue;
        }

        wait_message( 1, &server_queue, INFINITE, wake_mask, 0 );
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
    NTSTATUS status;
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
           info->hwnd, info->msg, debugstr_msg_name(info->msg, info->hwnd), info->wparam,
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
           info->hwnd, info->msg, debugstr_msg_name(info->msg, info->hwnd), info->wparam, info->lparam );

    user_check_not_lock();

    if (!put_message_in_queue( info, &reply_size )) return 0;

    /* there's no reply to wait for on notify/callback messages */
    if (info->type == MSG_NOTIFY || info->type == MSG_CALLBACK) return 1;

    wait_message_reply( info->flags );
    return retrieve_reply( info, reply_size, res_ptr );
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
NTSTATUS send_hardware_message( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput, UINT flags )
{
    struct user_key_state_info *key_state_info = get_user_thread_info()->key_state;
    struct send_message_info info;
    int prev_x, prev_y, new_x, new_y;
    INT counter = global_key_state_counter;
    USAGE hid_usage_page, hid_usage;
    NTSTATUS ret;
    BOOL wait;

    info.type     = MSG_HARDWARE;
    info.dest_tid = 0;
    info.hwnd     = hwnd;
    info.flags    = 0;
    info.timeout  = 0;
    info.params   = NULL;

    if (input->type == INPUT_HARDWARE && rawinput->header.dwType == RIM_TYPEHID)
    {
        if (input->hi.uMsg == WM_INPUT_DEVICE_CHANGE)
        {
            hid_usage_page = ((USAGE *)rawinput->data.hid.bRawData)[0];
            hid_usage = ((USAGE *)rawinput->data.hid.bRawData)[1];
        }
        if (input->hi.uMsg == WM_INPUT &&
            !rawinput_device_get_usages( rawinput->header.hDevice, &hid_usage_page, &hid_usage ))
        {
            WARN( "unable to get HID usages for device %p\n", rawinput->header.hDevice );
            return STATUS_INVALID_HANDLE;
        }
    }

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
            req->input.kbd.vkey  = input->ki.wVk;
            req->input.kbd.scan  = input->ki.wScan;
            req->input.kbd.flags = input->ki.dwFlags;
            req->input.kbd.time  = input->ki.time;
            req->input.kbd.info  = input->ki.dwExtraInfo;
            break;
        case INPUT_HARDWARE:
            req->input.hw.msg    = input->hi.uMsg;
            req->input.hw.lparam = MAKELONG( input->hi.wParamL, input->hi.wParamH );
            switch (input->hi.uMsg)
            {
            case WM_INPUT:
            case WM_INPUT_DEVICE_CHANGE:
                req->input.hw.rawinput.type = rawinput->header.dwType;
                switch (rawinput->header.dwType)
                {
                case RIM_TYPEHID:
                    req->input.hw.rawinput.hid.device = HandleToUlong( rawinput->header.hDevice );
                    req->input.hw.rawinput.hid.param = rawinput->header.wParam;
                    req->input.hw.rawinput.hid.usage_page = hid_usage_page;
                    req->input.hw.rawinput.hid.usage = hid_usage;
                    req->input.hw.rawinput.hid.count = rawinput->data.hid.dwCount;
                    req->input.hw.rawinput.hid.length = rawinput->data.hid.dwSizeHid;
                    wine_server_add_data( req, rawinput->data.hid.bRawData,
                                          rawinput->data.hid.dwCount * rawinput->data.hid.dwSizeHid );
                    break;
                default:
                    assert( 0 );
                    break;
                }
            }
            break;
        }
        if (key_state_info) wine_server_set_reply( req, key_state_info->state,
                                                   sizeof(key_state_info->state) );
        ret = wine_server_call( req );
        wait = reply->wait;
        prev_x = reply->prev_x;
        prev_y = reply->prev_y;
        new_x  = reply->new_x;
        new_y  = reply->new_y;
    }
    SERVER_END_REQ;

    if (!ret)
    {
        if (key_state_info)
        {
            key_state_info->time    = NtGetTickCount();
            key_state_info->counter = counter;
        }
        if ((flags & SEND_HWMSG_INJECTED) && (prev_x != new_x || prev_y != new_y))
            user_driver->pSetCursorPos( new_x, new_y );
    }

    if (wait)
    {
        LRESULT ignored;
        wait_message_reply( 0 );
        retrieve_reply( &info, 0, &ignored );
    }
    return ret;
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
        params.result = &retval; /* FIXME */
        if (!init_win_proc_params( &params, msg->hwnd, msg->message,
                                   msg->wParam, NtGetTickCount(), FALSE ))
            return 0;
        dispatch_win_proc_params( &params, sizeof(params) );
        return retval;
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
                                 &retval, FALSE, WMCHAR_MAP_DISPATCHMESSAGE ))
        dispatch_win_proc_params( &params, sizeof(params) );
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
        (list = list_window_children( 0, get_desktop_window(), NULL, 0 )))
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
    }

    if (res_ptr) *res_ptr = 1;
    return TRUE;
}

static BOOL process_packed_message( struct send_message_info *info, LRESULT *res_ptr, BOOL ansi )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct received_message_info receive_info;
    struct packed_message data;
    size_t buffer_size = 0, i;
    void *buffer = NULL;
    char *ptr;

    pack_message( info->hwnd, info->msg, info->wparam, info->lparam, &data );
    if (data.count == -1) return FALSE;

    for (i = 0; i < data.count; i++) buffer_size += data.size[i];
    if (!(buffer = malloc( buffer_size ))) return FALSE;
    for (ptr = buffer, i = 0; i < data.count; i++)
    {
        memcpy( ptr, data.data[i], data.size[i] );
        ptr += data.size[i];
    }

    receive_info.type = MSG_CLIENT_MESSAGE;
    receive_info.msg.hwnd = info->hwnd;
    receive_info.msg.message = info->msg;
    receive_info.msg.wParam = info->wparam;
    receive_info.msg.lParam = info->lparam;
    receive_info.flags = 0;
    receive_info.prev = thread_info->receive_info;
    receive_info.result = 0;
    thread_info->receive_info = &receive_info;

    *res_ptr = call_window_proc( info->hwnd, info->msg, info->wparam, info->lparam,
                                 !ansi, TRUE, info->wm_char, TRUE, buffer, buffer_size );
    if (thread_info->receive_info == &receive_info)
    {
        thread_info->receive_info = receive_info.prev;
        *res_ptr = receive_info.result;
    }
    free( buffer );
    return TRUE;
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
                                        NULL, ansi, info->wm_char );
    }

    thread_info->msg_source = msg_source_unavailable;
    spy_enter_message( SPY_SENDMESSAGE, info->hwnd, info->msg, info->wparam, info->lparam );

    if (info->dest_tid != GetCurrentThreadId())
    {
        if (dest_pid != GetCurrentProcessId() && (info->type == MSG_ASCII || info->type == MSG_UNICODE))
            info->type = MSG_OTHER_PROCESS;
        ret = send_inter_thread_message( info, &result );
    }
    else if (info->type != MSG_OTHER_PROCESS)
    {
        result = call_window_proc( info->hwnd, info->msg, info->wparam, info->lparam,
                                   !ansi, TRUE, info->wm_char, FALSE, NULL, 0 );
        if (info->type == MSG_CALLBACK)
            call_sendmsg_callback( info->callback, info->hwnd, info->msg, info->data, result );
        ret = TRUE;
    }
    else
    {
        ret = process_packed_message( info, &result, ansi );
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

    TRACE( "Added %p %lx %p timeout %d\n", hwnd, id, winproc, timeout );
    return ret;
}

/***********************************************************************
 *           NtUserSetSystemTimer (win32u.@)
 */
UINT_PTR WINAPI NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout )
{
    UINT_PTR ret;

    TRACE( "window %p, id %#lx, timeout %u\n", hwnd, id, timeout );

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

/* see KillSystemTimer */
BOOL kill_system_timer( HWND hwnd, UINT_PTR id )
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
    info.wm_char = WMCHAR_MAP_SENDMESSAGETIMEOUT;
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
           hwnd, msg, debugstr_msg_name(msg, hwnd), wparam, lparam );

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
        return popup_menu_window_proc( hwnd, msg, wparam, lparam );

    case NtUserDesktopWindowProc:
        return desktop_window_proc( hwnd, msg, wparam, lparam );

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

    case NtUserWinProcResult:
        return reply_winproc_result( (LRESULT)result_info, hwnd, msg, wparam, lparam );

    case NtUserGetDispatchParams:
        if (!hwnd) return FALSE;
        if (init_window_call_params( result_info, hwnd, msg, wparam, lparam,
                                     NULL, ansi, WMCHAR_MAP_DISPATCHMESSAGE ))
            return TRUE;
        if (!is_window( hwnd )) RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        else RtlSetLastWin32Error( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;

    case NtUserSendDriverMessage:
        /* used by driver to send packed messages */
        return send_message( hwnd, msg, wparam, lparam );

    case NtUserSpyEnter:
        spy_enter_message( ansi, hwnd, msg, wparam, lparam );
        return 0;

    case NtUserSpyGetMsgName:
        lstrcpynA( result_info, debugstr_msg_name( msg, hwnd ), wparam );
        return 0;

    case NtUserSpyExit:
        spy_exit_message( ansi, hwnd, msg, (LPARAM)result_info, wparam, lparam );
        return 0;

    default:
        FIXME( "%p %x %lx %lx %p %x %x\n", hwnd, msg, wparam, lparam, result_info, type, ansi );
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
    if (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) return TRUE;

    TRACE_(key)( "Translating key %s (%04lX), scancode %04x\n",
                 debugstr_vkey_name( msg->wParam ), msg->wParam, HIWORD(msg->lParam) );

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
