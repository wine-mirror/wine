/*
 * Win32 process handles
 *
 * Copyright 1998 Alexandre Julliard
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

#include <assert.h>
#include <stdio.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "winbase.h"
#include "wine/server.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);

/*********************************************************************
 *           CloseW32Handle (KERNEL.474)
 *           CloseHandle    (KERNEL32.@)
 */
BOOL WINAPI CloseHandle( HANDLE handle )
{
    NTSTATUS status;

    /* stdio handles need special treatment */
    if ((handle == (HANDLE)STD_INPUT_HANDLE) ||
        (handle == (HANDLE)STD_OUTPUT_HANDLE) ||
        (handle == (HANDLE)STD_ERROR_HANDLE))
        handle = GetStdHandle( (DWORD)handle );

    status = NtClose( handle );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/*********************************************************************
 *           GetHandleInformation   (KERNEL32.@)
 */
BOOL WINAPI GetHandleInformation( HANDLE handle, LPDWORD flags )
{
    BOOL ret;
    SERVER_START_REQ( set_handle_info )
    {
        req->handle = handle;
        req->flags  = 0;
        req->mask   = 0;
        req->fd     = -1;
        ret = !wine_server_call_err( req );
        if (ret && flags) *flags = reply->old_flags;
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           SetHandleInformation   (KERNEL32.@)
 */
BOOL WINAPI SetHandleInformation( HANDLE handle, DWORD mask, DWORD flags )
{
    BOOL ret;
    SERVER_START_REQ( set_handle_info )
    {
        req->handle = handle;
        req->flags  = flags;
        req->mask   = mask;
        req->fd     = -1;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/*********************************************************************
 *           DuplicateHandle   (KERNEL32.@)
 */
BOOL WINAPI DuplicateHandle( HANDLE source_process, HANDLE source,
                             HANDLE dest_process, HANDLE *dest,
                             DWORD access, BOOL inherit, DWORD options )
{
    NTSTATUS status = NtDuplicateObject( source_process, source, dest_process, dest,
                                         access, inherit ? OBJ_INHERIT : 0, options );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *           ConvertToGlobalHandle    		(KERNEL.476)
 *           ConvertToGlobalHandle    		(KERNEL32.@)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    HANDLE ret = INVALID_HANDLE_VALUE;
    DuplicateHandle( GetCurrentProcess(), hSrc, GetCurrentProcess(), &ret, 0, FALSE,
                     DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS | DUP_HANDLE_CLOSE_SOURCE );
    return ret;
}

/***********************************************************************
 *           SetHandleContext    		(KERNEL32.@)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context) {
    FIXME("(%p,%ld), stub. In case this got called by WSOCK32/WS2_32: the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetHandleContext    		(KERNEL32.@)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd) {
    FIXME("(%p), stub. In case this got called by WSOCK32/WS2_32: the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *           CreateSocketHandle    		(KERNEL32.@)
 */
HANDLE WINAPI CreateSocketHandle(void) {
    FIXME("(), stub. In case this got called by WSOCK32/WS2_32: the external WINSOCK DLLs won't work with WINE, don't use them.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}
