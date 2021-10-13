/*
 * Window stations and desktops
 *
 * Copyright 2002 Alexandre Julliard
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"
#include "winternl.h"
#include "wine/server.h"

/***********************************************************************
 *           NtUserCloseWindowStation  (win32u.@)
 */
BOOL WINAPI NtUserCloseWindowStation( HWINSTA handle )
{
    BOOL ret;
    SERVER_START_REQ( close_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUSerGetProcessWindowStation  (win32u.@)
 */
HWINSTA WINAPI NtUserGetProcessWindowStation(void)
{
    HWINSTA ret = 0;

    SERVER_START_REQ( get_process_winstation )
    {
        if (!wine_server_call_err( req ))
            ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserSetProcessWindowStation  (win32u.@)
 */
BOOL WINAPI NtUserSetProcessWindowStation( HWINSTA handle )
{
    BOOL ret;

    SERVER_START_REQ( set_process_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserCloseDesktop  (win32u.@)
 */
BOOL WINAPI NtUserCloseDesktop( HDESK handle )
{
    BOOL ret;
    SERVER_START_REQ( close_desktop )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserGetThreadDesktop   (win32u.@)
 */
HDESK WINAPI NtUserGetThreadDesktop( DWORD thread )
{
    HDESK ret = 0;

    SERVER_START_REQ( get_thread_desktop )
    {
        req->tid = thread;
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}
