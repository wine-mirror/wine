/*
 * Windows hook functions
 *
 * Copyright 2002 Alexandre Julliard
 * Copyright 2005 Dmitry Timoshkov
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

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hook);

#define WH_WINEVENT (WH_MAXHOOK+1)


static BOOL is_hooked( INT id )
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (!thread_info->active_hooks) return TRUE;
    return (thread_info->active_hooks & (1 << (id - WH_MINHOOK))) != 0;
}

/***********************************************************************
 *           NtUserSetWinEventHook   (win32u.@)
 */
HWINEVENTHOOK WINAPI NtUserSetWinEventHook( DWORD event_min, DWORD event_max, HMODULE inst,
                                            UNICODE_STRING *module, WINEVENTPROC proc,
                                            DWORD pid, DWORD tid, DWORD flags )
{
    HWINEVENTHOOK handle = 0;

    if ((flags & WINEVENT_INCONTEXT) && !inst)
    {
        SetLastError(ERROR_HOOK_NEEDS_HMOD);
        return 0;
    }

    if (event_min > event_max)
    {
        SetLastError(ERROR_INVALID_HOOK_FILTER);
        return 0;
    }

    /* FIXME: what if the tid or pid belongs to another process? */
    if (tid) inst = 0; /* thread-local hook */

    SERVER_START_REQ( set_hook )
    {
        req->id        = WH_WINEVENT;
        req->pid       = pid;
        req->tid       = tid;
        req->event_min = event_min;
        req->event_max = event_max;
        req->flags     = flags;
        req->unicode   = 1;
        if (inst) /* make proc relative to the module base */
        {
            req->proc = wine_server_client_ptr( (void *)((char *)proc - (char *)inst) );
            wine_server_add_data( req, module->Buffer, module->Length );
        }
        else req->proc = wine_server_client_ptr( proc );

        if (!wine_server_call_err( req ))
        {
            handle = wine_server_ptr_handle( reply->handle );
            get_user_thread_info()->active_hooks = reply->active_hooks;
        }
    }
    SERVER_END_REQ;

    TRACE("-> %p\n", handle);
    return handle;
}

/***********************************************************************
 *           NtUserUnhookWinEvent   (win32u.@)
 */
BOOL WINAPI NtUserUnhookWinEvent( HWINEVENTHOOK handle )
{
    BOOL ret;

    SERVER_START_REQ( remove_hook )
    {
        req->handle = wine_server_user_handle( handle );
        req->id     = WH_WINEVENT;
        ret = !wine_server_call_err( req );
        if (ret) get_user_thread_info()->active_hooks = reply->active_hooks;
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserNotifyWinEvent   (win32u.@)
 */
void WINAPI NtUserNotifyWinEvent( DWORD event, HWND hwnd, LONG object_id, LONG child_id )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct win_hook_proc_params info;
    void *ret_ptr;
    ULONG ret_len;
    BOOL ret;

    TRACE( "%04x, %p, %d, %d\n", event, hwnd, object_id, child_id );

    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return;
    }

    if (!is_hooked( WH_WINEVENT ))
    {
        TRACE( "skipping hook mask %x\n", thread_info->active_hooks );
        return;
    }

    info.event     = event;
    info.hwnd      = hwnd;
    info.object_id = object_id;
    info.child_id  = child_id;

    SERVER_START_REQ( start_hook_chain )
    {
        req->id        = WH_WINEVENT;
        req->event     = event;
        req->window    = wine_server_user_handle( hwnd );
        req->object_id = object_id;
        req->child_id  = child_id;
        wine_server_set_reply( req, info.module, sizeof(info.module) - sizeof(WCHAR) );
        ret = !wine_server_call( req ) && reply->proc;
        if (ret)
        {
            info.module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            info.handle = wine_server_ptr_handle( reply->handle );
            info.proc   = wine_server_get_ptr( reply->proc );
            thread_info->active_hooks = reply->active_hooks;
        }
    }
    SERVER_END_REQ;
    if (!ret) return;

    do
    {
        TRACE( "calling WH_WINEVENT hook %p event %x hwnd %p %x %x module %s\n",
               info.proc, event, hwnd, object_id, child_id, debugstr_w(info.module) );

        KeUserModeCallback( NtUserCallWinEventHook, &info,
                            FIELD_OFFSET( struct win_hook_proc_params, module[lstrlenW(info.module) + 1] ),
                            &ret_ptr, &ret_len );

        SERVER_START_REQ( get_hook_info )
        {
            req->handle    = wine_server_user_handle( info.handle );
            req->get_next  = 1;
            req->event     = event;
            req->window    = wine_server_user_handle( hwnd );
            req->object_id = object_id;
            req->child_id  = child_id;
            wine_server_set_reply( req, info.module, sizeof(info.module) - sizeof(WCHAR) );
            ret = !wine_server_call( req ) && reply->proc;
            if (ret)
            {
                info.module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
                info.handle = wine_server_ptr_handle( reply->handle );
                info.proc   = wine_server_get_ptr( reply->proc );
            }
        }
        SERVER_END_REQ;
    }
    while (ret);

    SERVER_START_REQ( finish_hook_chain )
    {
        req->id = WH_WINEVENT;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}
