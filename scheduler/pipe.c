/*
 * Win32 pipes
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

#include <assert.h>
#include "winerror.h"
#include "winbase.h"
#include "wine/server.h"


/***********************************************************************
 *	CreatePipe    (KERNEL32.@)
 */
BOOL WINAPI CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe,
                          LPSECURITY_ATTRIBUTES sa, DWORD size )
{
    BOOL ret;
    SERVER_START_REQ( create_pipe )
    {
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if ((ret = !wine_server_call_err( req )))
        {
            *hReadPipe  = reply->handle_read;
            *hWritePipe = reply->handle_write;
        }
    }
    SERVER_END_REQ;
    return ret;
}
