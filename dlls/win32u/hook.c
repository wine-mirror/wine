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

#define WH_WINEVENT (WH_MAXHOOK+1)


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
