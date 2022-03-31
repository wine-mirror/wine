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
#include "dde.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(relay);

#define MAX_WINPROC_RECURSION  64

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

#define MAX_PACK_COUNT 4

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

static const INPUT_MESSAGE_SOURCE msg_source_unavailable = { IMDT_UNAVAILABLE, IMO_UNAVAILABLE };

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
    params->is_dialog = FALSE;
    params->needs_unpack = FALSE;
    params->mapping = WMCHAR_MAP_CALLWINDOWPROC;
    params->dpi_awareness = get_window_dpi_awareness_context( params->hwnd );
    get_winproc_params( params );
    return TRUE;
}

static BOOL init_window_call_params( struct win_proc_params *params, HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, LRESULT *result, BOOL ansi,
                                     enum wm_char_mapping mapping )
{
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
    params->is_dialog = win->dlgInfo != NULL;
    release_win_ptr( win );

    params->hwnd = get_full_window_handle( hwnd );
    get_winproc_params( params );
    params->msg = msg;
    params->wparam = wParam;
    params->lparam = lParam;
    params->result = result;
    params->ansi = ansi;
    params->needs_unpack = FALSE;
    params->mapping = mapping;
    params->dpi_awareness = get_window_dpi_awareness_context( params->hwnd );
    return TRUE;
}

static BOOL dispatch_win_proc_params( struct win_proc_params *params, size_t size )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    void *ret_ptr;
    ULONG ret_len;

    if (thread_info->recursion_count > MAX_WINPROC_RECURSION) return FALSE;
    thread_info->recursion_count++;

    KeUserModeCallback( NtUserCallWindowProc, params, size, &ret_ptr, &ret_len );

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
        memcpy( &ps->wp, &wp, sizeof(wp) );
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
BOOL reply_message_result( LRESULT result, MSG *msg )
{
    struct received_message_info *info = get_user_thread_info()->receive_info;

    if (!info) return FALSE;
    reply_message( info, result, msg );
    if (msg) get_user_thread_info()->receive_info = info->prev;
    return TRUE;
}

/***********************************************************************
 *           handle_internal_message
 *
 * Handle an internal Wine message instead of calling the window proc.
 */
LRESULT handle_internal_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
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

/***********************************************************************
 *           NtUserWaitForInputIdle (win32u.@)
 */
DWORD WINAPI NtUserWaitForInputIdle( HANDLE process, DWORD timeout, BOOL wow )
{
    if (!user_callbacks) return 0;
    return user_callbacks->pWaitForInputIdle( process, timeout );
}

/**********************************************************************
 *           NtUserGetGUIThreadInfo  (win32u.@)
 */
BOOL WINAPI NtUserGetGUIThreadInfo( DWORD id, GUITHREADINFO *info )
{
    BOOL ret;

    if (info->cbSize != sizeof(*info))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
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

    params->needs_unpack = needs_unpack;
    if (size) memcpy( params + 1, buffer, size );

    /* first the WH_CALLWNDPROC hook */
    cwp.lParam  = lparam;
    cwp.wParam  = wparam;
    cwp.message = msg;
    cwp.hwnd    = params->hwnd;
    call_hooks( WH_CALLWNDPROC, HC_ACTION, same_thread, (LPARAM)&cwp, unicode );

    dispatch_win_proc_params( params, sizeof(*params) + size );
    if (params != &p) free( params );

    /* and finally the WH_CALLWNDPROCRET hook */
    cwpret.lResult = result;
    cwpret.lParam  = lparam;
    cwpret.wParam  = wparam;
    cwpret.message = msg;
    cwpret.hwnd    = params->hwnd;
    call_hooks( WH_CALLWNDPROCRET, HC_ACTION, same_thread, (LPARAM)&cwpret, unicode );
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
    INPUT_MESSAGE_SOURCE prev_source = thread_info->msg_source;
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

        thread_info->msg_source = prev_source;

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
                SetLastError( RtlNtStatusToDosError(res) );
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
                size = FIELD_OFFSET( struct win_hook_params, module[size / sizeof(WCHAR) + 1] );

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
                result = call_hooks( WH_KEYBOARD_LL, HC_ACTION, info.msg.wParam, (LPARAM)&hook, TRUE );
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
                result = call_hooks( WH_MOUSE_LL, HC_ACTION, info.msg.wParam, (LPARAM)&hook, TRUE );
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
                if (!user_callbacks) continue;
                if (!user_callbacks->process_hardware_message( &info.msg, hw_id, &msg_data->hardware,
                                                               hwnd, first, last, flags & PM_REMOVE ))
                {
                    TRACE("dropping msg %x\n", info.msg.message );
                    continue;  /* ignore it */
                }
                *msg = info.msg;
                thread_info->GetMessagePosVal = MAKELONG( info.msg.pt.x, info.msg.pt.y );
                thread_info->GetMessageTimeVal = info.msg.time;
                thread_info->GetMessageExtraInfoVal = msg_data->hardware.info;
                free( buffer );
                call_hooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, TRUE );
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
                if (!user_callbacks->unpack_dde_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                                         &info.msg.lParam, &buffer, size ))
                    continue;  /* ignore it */
            }
            *msg = info.msg;
            msg->pt = point_phys_to_win_dpi( info.msg.hwnd, info.msg.pt );
            thread_info->GetMessagePosVal = MAKELONG( msg->pt.x, msg->pt.y );
            thread_info->GetMessageTimeVal = info.msg.time;
            thread_info->GetMessageExtraInfoVal = 0;
            thread_info->msg_source = msg_source_unavailable;
            free( buffer );
            call_hooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, TRUE );
            return 1;
        }

        /* if we get here, we have a sent message; call the window procedure */
        info.prev = thread_info->receive_info;
        thread_info->receive_info = &info;
        thread_info->msg_source = msg_source_unavailable;
        result = call_window_proc( info.msg.hwnd, info.msg.message, info.msg.wParam,
                                   info.msg.lParam, (info.type != MSG_ASCII), FALSE,
                                   WMCHAR_MAP_RECVMESSAGE, needs_unpack, buffer, size );
        if (thread_info->receive_info == &info)
            reply_message_result( result, &info.msg );

        /* if some PM_QS* flags were specified, only handle sent messages from now on */
        if (HIWORD(flags) && !changed_mask) flags = PM_QS_SENDMESSAGE | LOWORD(flags);
    }
}

/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages.
 */
void process_sent_messages(void)
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
        flush_window_surfaces( FALSE );
        user_driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, QS_ALLINPUT, 0 );
    }
    else if (msg == WM_TIMER || msg == WM_SYSTIMER)
    {
        /* driver events should have priority over timers, so make sure we'll check for them soon */
        get_user_thread_info()->message_count += 100;
    }
    else get_user_thread_info()->message_count++;
}

/* wait for message or signaled handle */
static DWORD wait_message( DWORD count, const HANDLE *handles, DWORD timeout, DWORD mask, DWORD flags )
{
    DWORD ret, lock;
    void *ret_ptr;
    ULONG ret_len;

    if (enable_thunk_lock)
        lock = KeUserModeCallback( NtUserThunkLock, NULL, 0, &ret_ptr, &ret_len );

    ret = user_driver->pMsgWaitForMultipleObjectsEx( count, handles, timeout, mask, flags );
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
        SetLastError( ERROR_NOACCESS );
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

/**********************************************************************
 *           dispatch_message
 */
LRESULT dispatch_message( const MSG *msg, BOOL ansi )
{
    struct win_proc_params params;
    LRESULT retval = 0;

    /* Process timer messages */
    if (msg->lParam && (msg->message == WM_TIMER || msg->message == WM_SYSTIMER))
    {
        params.func = (WNDPROC)msg->lParam;
        params.result = &retval; /* FIXME */
        if (!init_win_proc_params( &params, msg->hwnd, msg->message,
                                   msg->wParam, NtGetTickCount(), ansi ))
            return 0;
        __TRY
        {
            dispatch_win_proc_params( &params, sizeof(params) );
        }
        __EXCEPT
        {
            retval = 0;
        }
        __ENDTRY
        return retval;
    }
    if (!msg->hwnd) return 0;

    spy_enter_message( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message, msg->wParam, msg->lParam );

    if (init_window_call_params( &params, msg->hwnd, msg->message, msg->wParam, msg->lParam,
                                 &retval, ansi, WMCHAR_MAP_DISPATCHMESSAGE ))
        dispatch_win_proc_params( &params, sizeof(params) );
    else if (!is_window( msg->hwnd )) SetLastError( ERROR_INVALID_WINDOW_HANDLE );
    else SetLastError( ERROR_MESSAGE_SYNC_ONLY );

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

/**********************************************************************
 *           NtUserDispatchMessage  (win32u.@)
 */
LRESULT WINAPI NtUserDispatchMessage( const MSG *msg )
{
    return dispatch_message( msg, FALSE );
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
UINT_PTR WINAPI NtUserSetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc )
{
    UINT_PTR ret;
    WNDPROC winproc = 0;

    if (proc) winproc = alloc_winproc( (WNDPROC)proc, TRUE );

    timeout = min( max( USER_TIMER_MINIMUM, timeout ), USER_TIMER_MAXIMUM );

    SERVER_START_REQ( set_win_timer )
    {
        req->win    = wine_server_user_handle( hwnd );
        req->msg    = WM_SYSTIMER;
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

static BOOL send_window_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                 LRESULT *result, BOOL ansi )
{
    /* FIXME: move implementation from user32 */
    if (!user_callbacks) return FALSE;
    *result = ansi
        ? user_callbacks->pSendMessageA( hwnd, msg, wparam, lparam )
        : user_callbacks->pSendMessageW( hwnd, msg, wparam, lparam );
    return TRUE;
}

/* see SendMessageW */
LRESULT send_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    LRESULT result = 0;
    send_window_message( hwnd, msg, wparam, lparam, &result, FALSE );
    return result;
}

/* see SendNotifyMessageW */
static BOOL send_notify_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    return user_callbacks && user_callbacks->pSendNotifyMessageW( hwnd, msg, wparam, lparam );
}

/* see PostMessageW */
LRESULT post_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    /* FIXME: move implementation from user32 */
    if (!user_callbacks) return 0;
    return user_callbacks->pPostMessageW( hwnd, msg, wparam, lparam );
}

BOOL WINAPI NtUserMessageCall( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                               ULONG_PTR result_info, DWORD type, BOOL ansi )
{
    switch (type)
    {
    case FNID_CALLWNDPROC:
        return init_win_proc_params( (struct win_proc_params *)result_info, hwnd, msg,
                                     wparam, lparam, ansi );
    case FNID_SENDMESSAGE:
        return send_window_message( hwnd, msg, wparam, lparam, (LRESULT *)result_info, ansi );
    case FNID_SENDNOTIFYMESSAGE:
        return send_notify_message( hwnd, msg, wparam, lparam, ansi );
    case FNID_SPYENTER:
        spy_enter_message( ansi, hwnd, msg, wparam, lparam );
        return 0;
    case FNID_SPYEXIT:
        spy_exit_message( ansi, hwnd, msg, result_info, wparam, lparam );
        return 0;
    default:
        FIXME( "%p %x %lx %lx %lx %x %x\n", hwnd, msg, wparam, lparam, result_info, type, ansi );
    }
    return 0;
}
