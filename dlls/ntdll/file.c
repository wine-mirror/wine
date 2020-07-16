/*
 * Copyright 1999, 2000 Juergen Schmied
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"

#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "ddk/ntddser.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


/******************************************************************
 *		NtQueryEaFile  (NTDLL.@)
 *
 * Read extended attributes from NTFS files.
 *
 * PARAMS
 *  hFile         [I] File handle, must be opened with FILE_READ_EA access
 *  iosb          [O] Receives information about the operation on return
 *  buffer        [O] Output buffer
 *  length        [I] Length of output buffer
 *  single_entry  [I] Only read and return one entry
 *  ea_list       [I] Optional list with names of EAs to return
 *  ea_list_len   [I] Length of ea_list in bytes
 *  ea_index      [I] Optional pointer to 1-based index of attribute to return
 *  restart       [I] restart EA scan
 *
 * RETURNS
 *  Success: 0. Attributes read into buffer
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtQueryEaFile( HANDLE hFile, PIO_STATUS_BLOCK iosb, PVOID buffer, ULONG length,
                               BOOLEAN single_entry, PVOID ea_list, ULONG ea_list_len,
                               PULONG ea_index, BOOLEAN restart )
{
    FIXME("(%p,%p,%p,%d,%d,%p,%d,%p,%d) stub\n",
            hFile, iosb, buffer, length, single_entry, ea_list,
            ea_list_len, ea_index, restart);
    return STATUS_ACCESS_DENIED;
}


/******************************************************************
 *		NtSetEaFile  (NTDLL.@)
 *
 * Update extended attributes for NTFS files.
 *
 * PARAMS
 *  hFile         [I] File handle, must be opened with FILE_READ_EA access
 *  iosb          [O] Receives information about the operation on return
 *  buffer        [I] Buffer with EA information
 *  length        [I] Length of buffer
 *
 * RETURNS
 *  Success: 0. Attributes are updated
 *  Failure: An NTSTATUS error code describing the error.
 */
NTSTATUS WINAPI NtSetEaFile( HANDLE hFile, PIO_STATUS_BLOCK iosb, PVOID buffer, ULONG length )
{
    FIXME("(%p,%p,%p,%d) stub\n", hFile, iosb, buffer, length);
    return STATUS_ACCESS_DENIED;
}


/******************************************************************
 *		NtLockFile       (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtLockFile( HANDLE hFile, HANDLE lock_granted_event,
                            PIO_APC_ROUTINE apc, void* apc_user,
                            PIO_STATUS_BLOCK io_status, PLARGE_INTEGER offset,
                            PLARGE_INTEGER count, ULONG* key, BOOLEAN dont_wait,
                            BOOLEAN exclusive )
{
    NTSTATUS    ret;
    HANDLE      handle;
    BOOLEAN     async;
    static BOOLEAN     warn = TRUE;

    if (apc || io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (apc_user && warn)
    {
        FIXME("I/O completion on lock not implemented yet\n");
        warn = FALSE;
    }

    for (;;)
    {
        SERVER_START_REQ( lock_file )
        {
            req->handle      = wine_server_obj_handle( hFile );
            req->offset      = offset->QuadPart;
            req->count       = count->QuadPart;
            req->shared      = !exclusive;
            req->wait        = !dont_wait;
            ret = wine_server_call( req );
            handle = wine_server_ptr_handle( reply->handle );
            async  = reply->overlapped;
        }
        SERVER_END_REQ;
        if (ret != STATUS_PENDING)
        {
            if (!ret && lock_granted_event) NtSetEvent(lock_granted_event, NULL);
            return ret;
        }

        if (async)
        {
            FIXME( "Async I/O lock wait not implemented, might deadlock\n" );
            if (handle) NtClose( handle );
            return STATUS_PENDING;
        }
        if (handle)
        {
            NtWaitForSingleObject( handle, FALSE, NULL );
            NtClose( handle );
        }
        else
        {
            LARGE_INTEGER time;
    
            /* Unix lock conflict, sleep a bit and retry */
            time.QuadPart = 100 * (ULONGLONG)10000;
            time.QuadPart = -time.QuadPart;
            NtDelayExecution( FALSE, &time );
        }
    }
}


/******************************************************************
 *		NtUnlockFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtUnlockFile( HANDLE hFile, PIO_STATUS_BLOCK io_status,
                              PLARGE_INTEGER offset, PLARGE_INTEGER count,
                              PULONG key )
{
    NTSTATUS status;

    TRACE( "%p %x%08x %x%08x\n",
           hFile, offset->u.HighPart, offset->u.LowPart, count->u.HighPart, count->u.LowPart );

    if (io_status || key)
    {
        FIXME("Unimplemented yet parameter\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    SERVER_START_REQ( unlock_file )
    {
        req->handle = wine_server_obj_handle( hFile );
        req->offset = offset->QuadPart;
        req->count  = count->QuadPart;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *		NtCancelIoFileEx    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCancelIoFileEx( HANDLE hFile, PIO_STATUS_BLOCK iosb, PIO_STATUS_BLOCK io_status )
{
    TRACE("%p %p %p\n", hFile, iosb, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->iosb        = wine_server_client_ptr( iosb );
        req->only_thread = FALSE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;

    return io_status->u.Status;
}

/******************************************************************
 *		NtCancelIoFile    (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI NtCancelIoFile( HANDLE hFile, PIO_STATUS_BLOCK io_status )
{
    TRACE("%p %p\n", hFile, io_status );

    SERVER_START_REQ( cancel_async )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->iosb        = 0;
        req->only_thread = TRUE;
        io_status->u.Status = wine_server_call( req );
    }
    SERVER_END_REQ;

    return io_status->u.Status;
}
