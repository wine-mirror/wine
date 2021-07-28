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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


/**********************************************************************
 *           wow64_NtCancelIoFile
 */
NTSTATUS WINAPI wow64_NtCancelIoFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtCancelIoFile( handle, iosb_32to64( &io, io32 ));
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtCancelIoFileEx
 */
NTSTATUS WINAPI wow64_NtCancelIoFileEx( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io_ptr = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtCancelIoFileEx( handle, (IO_STATUS_BLOCK *)io_ptr, iosb_32to64( &io, io32 ));
    put_iosb( io32, &io );
    return status;
}


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
 *           wow64_NtCreateMailslotFile
 */
NTSTATUS WINAPI wow64_NtCreateMailslotFile( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    ULONG options = get_ulong( &args );
    ULONG quota = get_ulong( &args );
    ULONG msg_size = get_ulong( &args );
    LARGE_INTEGER *timeout = get_ptr( &args );

    struct object_attr64 attr;
    IO_STATUS_BLOCK io;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateMailslotFile( &handle, access, objattr_32to64( &attr, attr32 ),
                                   iosb_32to64( &io, io32 ), options, quota, msg_size, timeout );
    put_handle( handle_ptr, handle );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateNamedPipeFile
 */
NTSTATUS WINAPI wow64_NtCreateNamedPipeFile( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    ULONG sharing = get_ulong( &args );
    ULONG dispo = get_ulong( &args );
    ULONG options = get_ulong( &args );
    ULONG pipe_type = get_ulong( &args );
    ULONG read_mode = get_ulong( &args );
    ULONG completion_mode = get_ulong( &args );
    ULONG max_inst = get_ulong( &args );
    ULONG inbound_quota = get_ulong( &args );
    ULONG outbound_quota = get_ulong( &args );
    LARGE_INTEGER *timeout = get_ptr( &args );

    struct object_attr64 attr;
    IO_STATUS_BLOCK io;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateNamedPipeFile( &handle, access, objattr_32to64( &attr, attr32 ),
                                    iosb_32to64( &io, io32 ), sharing, dispo, options,
                                    pipe_type, read_mode, completion_mode, max_inst,
                                    inbound_quota, outbound_quota, timeout );
    put_handle( handle_ptr, handle );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtCreatePagingFile
 */
NTSTATUS WINAPI wow64_NtCreatePagingFile( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    LARGE_INTEGER *min_size = get_ptr( &args );
    LARGE_INTEGER *max_size = get_ptr( &args );
    LARGE_INTEGER *actual_size = get_ptr( &args );

    UNICODE_STRING str;

    return NtCreatePagingFile( unicode_str_32to64( &str, str32 ), min_size, max_size, actual_size );
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
 *           wow64_NtNotifyChangeDirectoryFile
 */
NTSTATUS WINAPI wow64_NtNotifyChangeDirectoryFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG filter = get_ulong( &args );
    BOOLEAN subtree = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtNotifyChangeDirectoryFile( handle, event, apc_32to64( apc ),
                                          apc_param_32to64( apc, apc_param ), iosb_32to64( &io, io32 ),
                                          buffer, len, filter, subtree );
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
 *           wow64_NtRemoveIoCompletion
 */
NTSTATUS WINAPI wow64_NtRemoveIoCompletion( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *key_ptr = get_ptr( &args );
    ULONG *value_ptr = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    LARGE_INTEGER *timeout = get_ptr( &args );

    IO_STATUS_BLOCK io;
    ULONG_PTR key, value;
    NTSTATUS status;

    status = NtRemoveIoCompletion( handle, &key, &value, iosb_32to64( &io, io32 ), timeout );
    if (!status)
    {
        *key_ptr = key;
        *value_ptr = value;
    }
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtRemoveIoCompletionEx
 */
NTSTATUS WINAPI wow64_NtRemoveIoCompletionEx( UINT *args )
{
    HANDLE handle = get_handle( &args );
    FILE_IO_COMPLETION_INFORMATION32 *info32 = get_ptr( &args );
    ULONG count = get_ulong( &args );
    ULONG *written = get_ptr( &args );
    LARGE_INTEGER *timeout = get_ptr( &args );
    BOOLEAN alertable = get_ulong( &args );

    NTSTATUS status;
    ULONG i;
    FILE_IO_COMPLETION_INFORMATION *info = Wow64AllocateTemp( count * sizeof(*info) );

    status = NtRemoveIoCompletionEx( handle, info, count, written, timeout, alertable );
    for (i = 0; i < *written; i++)
    {
        info32[i].CompletionKey             = info[i].CompletionKey;
        info32[i].CompletionValue           = info[i].CompletionValue;
        info32[i].IoStatusBlock.Status      = info[i].IoStatusBlock.Status;
        info32[i].IoStatusBlock.Information = info[i].IoStatusBlock.Information;
    }
    return status;
}


/**********************************************************************
 *           wow64_NtSetEaFile
 */
NTSTATUS WINAPI wow64_NtSetEaFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtSetEaFile( handle, iosb_32to64( &io, io32 ), ptr, len );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtSetInformationFile
 */
NTSTATUS WINAPI wow64_NtSetInformationFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    FILE_INFORMATION_CLASS class = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    switch (class)
    {
    case FileBasicInformation:   /* FILE_BASIC_INFORMATION */
    case FilePositionInformation:   /* FILE_POSITION_INFORMATION */
    case FileEndOfFileInformation:   /* FILE_END_OF_FILE_INFORMATION */
    case FilePipeInformation:   /* FILE_PIPE_INFORMATION */
    case FileMailslotSetInformation:   /* FILE_MAILSLOT_SET_INFORMATION */
    case FileIoCompletionNotificationInformation:   /* FILE_IO_COMPLETION_NOTIFICATION_INFORMATION */
    case FileIoPriorityHintInformation:   /* FILE_IO_PRIORITY_HINT_INFO */
    case FileValidDataLengthInformation:   /* FILE_VALID_DATA_LENGTH_INFORMATION */
    case FileDispositionInformation:   /* FILE_DISPOSITION_INFORMATION */
        status = NtSetInformationFile( handle, iosb_32to64( &io, io32 ), ptr, len, class );
        break;

    case FileRenameInformation:   /* FILE_RENAME_INFORMATION */
    case FileLinkInformation:   /* FILE_LINK_INFORMATION */
        if (len >= sizeof(FILE_RENAME_INFORMATION32))
        {
            FILE_RENAME_INFORMATION32 *info32 = ptr;
            FILE_RENAME_INFORMATION *info;
            ULONG size;

            size = offsetof( FILE_RENAME_INFORMATION, FileName[info32->FileNameLength/sizeof(WCHAR)] );
            info = Wow64AllocateTemp( size );
            info->ReplaceIfExists = info32->ReplaceIfExists;
            info->RootDirectory   = LongToHandle( info32->RootDirectory );
            info->FileNameLength  = info32->FileNameLength;
            memcpy( info->FileName, info32->FileName, info->FileNameLength );
            status = NtSetInformationFile( handle, iosb_32to64( &io, io32 ), info, size, class );
        }
        else status = io.Status = STATUS_INVALID_PARAMETER_3;
        break;

    case FileCompletionInformation:   /* FILE_COMPLETION_INFORMATION */
        if (len >= sizeof(FILE_COMPLETION_INFORMATION32))
        {
            FILE_COMPLETION_INFORMATION32 *info32 = ptr;
            FILE_COMPLETION_INFORMATION info;

            info.CompletionPort = LongToHandle( info32->CompletionPort );
            info.CompletionKey  = info32->CompletionKey;
            status = NtSetInformationFile( handle, iosb_32to64( &io, io32 ), &info, sizeof(info), class );
        }
        else status = io.Status = STATUS_INVALID_PARAMETER_3;
        break;

    default:
        FIXME( "unsupported class %u\n", class );
        status = io.Status = STATUS_INVALID_INFO_CLASS;
        break;
    }
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtSetVolumeInformationFile
 */
NTSTATUS WINAPI wow64_NtSetVolumeInformationFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    FS_INFORMATION_CLASS class = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtSetVolumeInformationFile( handle, iosb_32to64( &io, io32 ), ptr, len, class );
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
