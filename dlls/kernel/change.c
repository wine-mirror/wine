/*
 * Win32 file change notification functions
 *
 * Copyright 1998 Ulrich Weigand
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "winbase.h"
#include "winerror.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/****************************************************************************
 *		FindFirstChangeNotificationA (KERNEL32.@)
 */
HANDLE WINAPI FindFirstChangeNotificationA( LPCSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter )
{
    HANDLE file, ret = INVALID_HANDLE_VALUE;

    TRACE( "%s %d %lx\n", debugstr_a(lpPathName), bWatchSubtree, dwNotifyFilter );

    if ((file = CreateFileA( lpPathName, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 )) == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;

    SERVER_START_REQ( create_change_notification )
    {
        req->handle  = file;
        req->subtree = bWatchSubtree;
        req->filter  = dwNotifyFilter;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    CloseHandle( file );
    return ret;
}

/****************************************************************************
 *		FindFirstChangeNotificationW (KERNEL32.@)
 */
HANDLE WINAPI FindFirstChangeNotificationW( LPCWSTR lpPathName, BOOL bWatchSubtree,
                                            DWORD dwNotifyFilter)
{
    HANDLE file, ret = INVALID_HANDLE_VALUE;

    TRACE( "%s %d %lx\n", debugstr_w(lpPathName), bWatchSubtree, dwNotifyFilter );

    if ((file = CreateFileW( lpPathName, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 )) == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;

    SERVER_START_REQ( create_change_notification )
    {
        req->handle  = file;
        req->subtree = bWatchSubtree;
        req->filter  = dwNotifyFilter;
        if (!wine_server_call_err( req )) ret = reply->handle;
    }
    SERVER_END_REQ;
    CloseHandle( file );
    return ret;
}

/****************************************************************************
 *		FindNextChangeNotification (KERNEL32.@)
 */
BOOL WINAPI FindNextChangeNotification( HANDLE handle )
{
    BOOL ret;

    TRACE("%p\n",handle);

    SERVER_START_REQ( next_change_notification )
    {
        req->handle = handle;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/****************************************************************************
 *		FindCloseChangeNotification (KERNEL32.@)
 */
BOOL WINAPI FindCloseChangeNotification( HANDLE handle )
{
    return CloseHandle( handle );
}
