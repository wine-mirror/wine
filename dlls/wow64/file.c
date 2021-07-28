/*
 * WoW64 file functions
 *
 * Copyright 2021 Alexandre Julliard
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wow64_private.h"


/**********************************************************************
 *           wow64_NtCreateFile
 */
NTSTATUS WINAPI wow64_NtCreateFile( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    LARGE_INTEGER *alloc_size = get_ptr( &args );
    ULONG attributes = get_ulong( &args );
    ULONG sharing = get_ulong( &args );
    ULONG disposition = get_ulong( &args );
    ULONG options = get_ulong( &args );
    void *ea_buffer = get_ptr( &args );
    ULONG ea_length = get_ulong( &args );

    struct object_attr64 attr;
    IO_STATUS_BLOCK io;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateFile( &handle, access, objattr_32to64( &attr, attr32 ),
                           iosb_32to64( &io, io32 ), alloc_size, attributes,
                           sharing, disposition, options, ea_buffer, ea_length );
    put_handle( handle_ptr, handle );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtDeleteFile
 */
NTSTATUS WINAPI wow64_NtDeleteFile( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;

    return NtDeleteFile( objattr_32to64( &attr, attr32 ));
}


/**********************************************************************
 *           wow64_NtFlushBuffersFile
 */
NTSTATUS WINAPI wow64_NtFlushBuffersFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtFlushBuffersFile( handle, iosb_32to64( &io, io32 ));
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtLockFile
 */
NTSTATUS WINAPI wow64_NtLockFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    LARGE_INTEGER *offset = get_ptr( &args );
    LARGE_INTEGER *count = get_ptr( &args );
    ULONG *key = get_ptr( &args );
    BOOLEAN dont_wait = get_ulong( &args );
    BOOLEAN exclusive = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtLockFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                         iosb_32to64( &io, io32 ), offset, count, key, dont_wait, exclusive );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenFile
 */
NTSTATUS WINAPI wow64_NtOpenFile( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    ULONG sharing = get_ulong( &args );
    ULONG options = get_ulong( &args );

    struct object_attr64 attr;
    IO_STATUS_BLOCK io;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenFile( &handle, access, objattr_32to64( &attr, attr32 ),
                         iosb_32to64( &io, io32 ), sharing, options );
    put_handle( handle_ptr, handle );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtQueryAttributesFile
 */
NTSTATUS WINAPI wow64_NtQueryAttributesFile( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    FILE_BASIC_INFORMATION *info = get_ptr( &args );

    struct object_attr64 attr;

    return NtQueryAttributesFile( objattr_32to64( &attr, attr32 ), info );
}


/**********************************************************************
 *           wow64_NtQueryDirectoryFile
 */
NTSTATUS WINAPI wow64_NtQueryDirectoryFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    FILE_INFORMATION_CLASS class = get_ulong( &args );
    BOOLEAN single_entry = get_ulong( &args );
    UNICODE_STRING32 *mask32 = get_ptr( &args );
    BOOLEAN restart_scan = get_ulong( &args );

    UNICODE_STRING mask;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryDirectoryFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                                   iosb_32to64( &io, io32 ), buffer, len, class, single_entry,
                                   unicode_str_32to64( &mask, mask32 ), restart_scan );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtQueryEaFile
 */
NTSTATUS WINAPI wow64_NtQueryEaFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    BOOLEAN single_entry = get_ulong( &args );
    void *list = get_ptr( &args );
    ULONG list_len = get_ulong( &args );
    ULONG *index = get_ptr( &args );
    BOOLEAN restart = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryEaFile( handle, iosb_32to64( &io, io32 ), buffer, len,
                            single_entry, list, list_len, index, restart );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtQueryFullAttributesFile
 */
NTSTATUS WINAPI wow64_NtQueryFullAttributesFile( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    FILE_NETWORK_OPEN_INFORMATION *info = get_ptr( &args );

    struct object_attr64 attr;

    return NtQueryFullAttributesFile( objattr_32to64( &attr, attr32 ), info );
}


/**********************************************************************
 *           wow64_NtQueryInformationFile
 */
NTSTATUS WINAPI wow64_NtQueryInformationFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    FILE_INFORMATION_CLASS class = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( handle, iosb_32to64( &io, io32 ), info, len, class );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtQueryVolumeInformationFile
 */
NTSTATUS WINAPI wow64_NtQueryVolumeInformationFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    FS_INFORMATION_CLASS class = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryVolumeInformationFile( handle, iosb_32to64( &io, io32 ), buffer, len, class );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtReadFile
 */
NTSTATUS WINAPI wow64_NtReadFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    LARGE_INTEGER *offset = get_ptr( &args );
    ULONG *key = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtReadFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                         iosb_32to64( &io, io32 ), buffer, len, offset, key );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtReadFileScatter
 */
NTSTATUS WINAPI wow64_NtReadFileScatter( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    FILE_SEGMENT_ELEMENT *segments = get_ptr( &args );
    ULONG len = get_ulong( &args );
    LARGE_INTEGER *offset = get_ptr( &args );
    ULONG *key = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtReadFileScatter( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                                iosb_32to64( &io, io32 ), segments, len, offset, key );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtUnlockFile
 */
NTSTATUS WINAPI wow64_NtUnlockFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    LARGE_INTEGER *offset = get_ptr( &args );
    LARGE_INTEGER *count = get_ptr( &args );
    ULONG *key = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtUnlockFile( handle, iosb_32to64( &io, io32 ), offset, count, key );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtWriteFile
 */
NTSTATUS WINAPI wow64_NtWriteFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    LARGE_INTEGER *offset = get_ptr( &args );
    ULONG *key = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtWriteFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                          iosb_32to64( &io, io32 ), buffer, len, offset, key );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtWriteFileGather
 */
NTSTATUS WINAPI wow64_NtWriteFileGather( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    FILE_SEGMENT_ELEMENT *segments = get_ptr( &args );
    ULONG len = get_ulong( &args );
    LARGE_INTEGER *offset = get_ptr( &args );
    ULONG *key = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtWriteFileGather( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                                iosb_32to64( &io, io32 ), segments, len, offset, key );
    put_iosb( io32, &io );
    return status;
}
