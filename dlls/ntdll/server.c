/*
 * Wine server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

BOOL is_wow64 = FALSE;

/***********************************************************************
 *           wine_server_call (NTDLL.@)
 *
 * Perform a server call.
 *
 * PARAMS
 *     req_ptr [I/O] Function dependent data
 *
 * RETURNS
 *     Depends on server function being called, but usually an NTSTATUS code.
 *
 * NOTES
 *     Use the SERVER_START_REQ and SERVER_END_REQ to help you fill out the
 *     server request structure for the particular call. E.g:
 *|     SERVER_START_REQ( event_op )
 *|     {
 *|         req->handle = handle;
 *|         req->op     = SET_EVENT;
 *|         ret = wine_server_call( req );
 *|     }
 *|     SERVER_END_REQ;
 */
unsigned int CDECL wine_server_call( void *req_ptr )
{
    return unix_funcs->server_call( req_ptr );
}


/***********************************************************************
 *           wine_server_send_fd   (NTDLL.@)
 *
 * Send a file descriptor to the server.
 *
 * PARAMS
 *     fd [I] file descriptor to send
 *
 * RETURNS
 *     nothing
 */
void CDECL wine_server_send_fd( int fd )
{
    unix_funcs->server_send_fd( fd );
}


/***********************************************************************
 *           wine_server_fd_to_handle   (NTDLL.@)
 *
 * Allocate a file handle for a Unix file descriptor.
 *
 * PARAMS
 *     fd      [I] Unix file descriptor.
 *     access  [I] Win32 access flags.
 *     attributes [I] Object attributes.
 *     handle  [O] Address where Wine file handle will be stored.
 *
 * RETURNS
 *     NTSTATUS code
 */
int CDECL wine_server_fd_to_handle( int fd, unsigned int access, unsigned int attributes, HANDLE *handle )
{
    return unix_funcs->server_fd_to_handle( fd, access, attributes, handle );
}


/***********************************************************************
 *           wine_server_handle_to_fd   (NTDLL.@)
 *
 * Retrieve the file descriptor corresponding to a file handle.
 *
 * PARAMS
 *     handle  [I] Wine file handle.
 *     access  [I] Win32 file access rights requested.
 *     unix_fd [O] Address where Unix file descriptor will be stored.
 *     options [O] Address where the file open options will be stored. Optional.
 *
 * RETURNS
 *     NTSTATUS code
 */
int CDECL wine_server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd,
                                    unsigned int *options )
{
    return unix_funcs->server_handle_to_fd( handle, access, unix_fd, options );
}


/***********************************************************************
 *           wine_server_release_fd   (NTDLL.@)
 *
 * Release the Unix file descriptor returned by wine_server_handle_to_fd.
 *
 * PARAMS
 *     handle  [I] Wine file handle.
 *     unix_fd [I] Unix file descriptor to release.
 *
 * RETURNS
 *     nothing
 */
void CDECL wine_server_release_fd( HANDLE handle, int unix_fd )
{
    unix_funcs->server_release_fd( handle, unix_fd );
}
