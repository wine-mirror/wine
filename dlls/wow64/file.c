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
#include "winioctl.h"
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


static FILE_OBJECTID_BUFFER windir_id, sysdir_id;

static inline NTSTATUS get_file_id( HANDLE handle, FILE_OBJECTID_BUFFER *id )
{
    IO_STATUS_BLOCK io;

    return NtFsControlFile( handle, 0, NULL, NULL, &io, FSCTL_GET_OBJECT_ID, NULL, 0, id, sizeof(*id) );
}

static inline ULONG starts_with_path( const WCHAR *name, ULONG name_len, const WCHAR *prefix )
{
    ULONG len = wcslen( prefix );

    if (name_len < len) return 0;
    if (wcsnicmp( name, prefix, len )) return 0;
    if (name_len > len && name[len] != '\\') return 0;
    return len;
}


/***********************************************************************
 *           init_file_redirects
 */
void init_file_redirects(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING windows = RTL_CONSTANT_STRING( L"\\??\\C:\\windows" );
    UNICODE_STRING system32 = RTL_CONSTANT_STRING( L"\\??\\C:\\windows\\system32" );
    IO_STATUS_BLOCK io;
    HANDLE handle;

    InitializeObjectAttributes( &attr, &windows, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!NtOpenFile( &handle, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io,
                     FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT |
                     FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE ))
    {
        get_file_id( handle, &windir_id );
        NtClose( handle );
    }
    attr.ObjectName = &system32;
    if (!NtOpenFile( &handle, SYNCHRONIZE | FILE_LIST_DIRECTORY, &attr, &io,
                     FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT |
                     FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE ))
    {
        get_file_id( handle, &sysdir_id );
        NtClose( handle );
    }
}


/***********************************************************************
 *           replace_path
 *
 * Helper for get_file_redirect().
 */
static BOOL replace_path( OBJECT_ATTRIBUTES *attr, ULONG prefix_len, const WCHAR *match,
                          const WCHAR *replace_dir, const WCHAR *replace_name )
{
    const WCHAR *name = attr->ObjectName->Buffer;
    ULONG match_len, replace_len, len = attr->ObjectName->Length / sizeof(WCHAR);
    UNICODE_STRING str;
    WCHAR *p;

    if (!starts_with_path( name + prefix_len, len - prefix_len, match )) return FALSE;

    match_len = wcslen( match );
    replace_len = wcslen( replace_dir );
    if (replace_name) replace_len += wcslen( replace_name );
    str.Length = (len + replace_len - match_len) * sizeof(WCHAR);
    str.MaximumLength = str.Length + sizeof(WCHAR);
    if (!(p = str.Buffer = Wow64AllocateTemp( str.MaximumLength ))) return FALSE;

    memcpy( p, name, prefix_len * sizeof(WCHAR) );
    p += prefix_len;
    wcscpy( p, replace_dir );
    p += wcslen(p);
    if (replace_name)
    {
        wcscpy( p, replace_name );
        p += wcslen(p);
    }
    name += prefix_len + match_len;
    len -= prefix_len + match_len;
    memcpy( p, name, len * sizeof(WCHAR) );
    p[len] = 0;
    *attr->ObjectName = str;
    return TRUE;
}


/***********************************************************************
 *           get_file_redirect
 */
BOOL get_file_redirect( OBJECT_ATTRIBUTES *attr )
{
    static const WCHAR * const no_redirect[] =
    {
        L"system32\\catroot", L"system32\\catroot2", L"system32\\driversstore",
        L"system32\\drivers\\etc", L"system32\\logfiles", L"system32\\spool"
    };
    static const WCHAR windirW[] = L"\\??\\C:\\windows\\";
    const WCHAR *name = attr->ObjectName->Buffer;
    unsigned int i, prefix_len = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
    const WCHAR *syswow64dir;
    UNICODE_STRING redir;

    if (!len) return FALSE;

    if (!attr->RootDirectory)
    {
        prefix_len = wcslen( windirW );
        if (len < prefix_len || wcsnicmp( name, windirW, prefix_len )) return FALSE;
    }
    else
    {
        FILE_OBJECTID_BUFFER id;

        if (get_file_id( attr->RootDirectory, &id )) return FALSE;
        if (memcmp( &id, &windir_id, sizeof(id) ))
        {
            if (memcmp( &id, &sysdir_id, sizeof(id) )) return FALSE;
            if (NtCurrentTeb()->TlsSlots[WOW64_TLS_FILESYSREDIR]) return FALSE;
            if (name[0] == '\\') return FALSE;

            /* only check for paths that should NOT be redirected */
            for (i = 0; i < ARRAY_SIZE( no_redirect ); i++)
                if (starts_with_path( name, len, no_redirect[i] + 9 /* "system32\\" */)) return FALSE;

            /* redirect everything else */
            syswow64dir = get_machine_wow64_dir( current_machine );
            redir.Length = (wcslen(syswow64dir) + 1 + len) * sizeof(WCHAR);
            redir.MaximumLength = redir.Length + sizeof(WCHAR);
            if (!(redir.Buffer = Wow64AllocateTemp( redir.MaximumLength ))) return FALSE;
            wcscpy( redir.Buffer, syswow64dir );
            wcscat( redir.Buffer, L"\\" );
            memcpy( redir.Buffer + wcslen(redir.Buffer), name, len * sizeof(WCHAR) );
            redir.Buffer[redir.Length / sizeof(WCHAR)] = 0;
            attr->RootDirectory = 0;
            *attr->ObjectName = redir;
            return TRUE;
        }
    }

    /* sysnative is redirected even when redirection is disabled */

    if (replace_path( attr, prefix_len, L"sysnative", L"system32", NULL )) return TRUE;

    if (NtCurrentTeb()->TlsSlots[WOW64_TLS_FILESYSREDIR]) return FALSE;

    for (i = 0; i < ARRAY_SIZE( no_redirect ); i++)
        if (starts_with_path( name + prefix_len, len - prefix_len, no_redirect[i] )) return FALSE;

    syswow64dir = get_machine_wow64_dir( current_machine ) + wcslen( windirW );
    if (replace_path( attr, prefix_len, L"system32", syswow64dir, NULL )) return TRUE;
    if (replace_path( attr, prefix_len, L"regedit.exe", syswow64dir, L"\\regedit.exe" )) return TRUE;
    return FALSE;
}


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
 *           wow64_NtCancelSynchronousIoFile
 */
NTSTATUS WINAPI wow64_NtCancelSynchronousIoFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_STATUS_BLOCK32 *io_ptr = get_ptr( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtCancelSynchronousIoFile( handle, (IO_STATUS_BLOCK *)io_ptr, iosb_32to64( &io, io32 ));
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
    status = NtCreateFile( &handle, access, objattr_32to64_redirect( &attr, attr32 ),
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

    return NtDeleteFile( objattr_32to64_redirect( &attr, attr32 ));
}


/**********************************************************************
 *           wow64_NtDeviceIoControlFile
 */
NTSTATUS WINAPI wow64_NtDeviceIoControlFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    ULONG code = get_ulong( &args );
    void *in_buf = get_ptr( &args );
    ULONG in_len = get_ulong( &args );
    void *out_buf = get_ptr( &args );
    ULONG out_len = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtDeviceIoControlFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                                    iosb_32to64( &io, io32 ), code, in_buf, in_len, out_buf, out_len );
    put_iosb( io32, &io );
    return status;
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
 *           wow64_NtFlushBuffersFileEx
 */
NTSTATUS WINAPI wow64_NtFlushBuffersFileEx( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG flags = get_ulong( &args );
    void *params = get_ptr( &args );
    ULONG size = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtFlushBuffersFileEx( handle, flags, params, size, iosb_32to64( &io, io32 ) );
    put_iosb( io32, &io );
    return status;
}


/**********************************************************************
 *           wow64_NtFsControlFile
 */
NTSTATUS WINAPI wow64_NtFsControlFile( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE event = get_handle( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    IO_STATUS_BLOCK32 *io32 = get_ptr( &args );
    ULONG code = get_ulong( &args );
    void *in_buf = get_ptr( &args );
    ULONG in_len = get_ulong( &args );
    void *out_buf = get_ptr( &args );
    ULONG out_len = get_ulong( &args );

    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtFsControlFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                              iosb_32to64( &io, io32 ), code, in_buf, in_len, out_buf, out_len );
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
    status = NtOpenFile( &handle, access, objattr_32to64_redirect( &attr, attr32 ),
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

    return NtQueryAttributesFile( objattr_32to64_redirect( &attr, attr32 ), info );
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

    return NtQueryFullAttributesFile( objattr_32to64_redirect( &attr, attr32 ), info );
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

    if (pBTCpuNotifyReadFile) pBTCpuNotifyReadFile( handle, buffer, len, FALSE, 0 );
    status = NtReadFile( handle, event, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                         iosb_32to64( &io, io32 ), buffer, len, offset, key );
    if (pBTCpuNotifyReadFile) pBTCpuNotifyReadFile( handle, buffer, len, TRUE, status );
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
    FILE_IO_COMPLETION_INFORMATION *info;

    if (!count) return STATUS_INVALID_PARAMETER;

    info = Wow64AllocateTemp( count * sizeof(*info) );

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
    case FileDispositionInformationEx:   /* FILE_DISPOSITION_INFORMATION_EX */
        status = NtSetInformationFile( handle, iosb_32to64( &io, io32 ), ptr, len, class );
        break;

    case FileRenameInformation:   /* FILE_RENAME_INFORMATION */
    case FileRenameInformationEx:   /* FILE_RENAME_INFORMATION */
    case FileLinkInformation:   /* FILE_LINK_INFORMATION */
    case FileLinkInformationEx:   /* FILE_LINK_INFORMATION */
        if (len >= sizeof(FILE_RENAME_INFORMATION32))
        {
            OBJECT_ATTRIBUTES attr;
            UNICODE_STRING name;
            FILE_RENAME_INFORMATION32 *info32 = ptr;
            FILE_RENAME_INFORMATION *info;
            ULONG size;

            name.Buffer = info32->FileName;
            name.Length = info32->FileNameLength;
            InitializeObjectAttributes( &attr, &name, 0, LongToHandle( info32->RootDirectory ), 0 );
            get_file_redirect( &attr );
            size = offsetof( FILE_RENAME_INFORMATION, FileName[name.Length/sizeof(WCHAR)] );
            info = Wow64AllocateTemp( size );
            info->Flags           = info32->Flags;
            info->RootDirectory   = attr.RootDirectory;
            info->FileNameLength  = name.Length;
            memcpy( info->FileName, name.Buffer, info->FileNameLength );
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


/**********************************************************************
 *           wow64_wine_nt_to_unix_file_name
 */
NTSTATUS WINAPI wow64_wine_nt_to_unix_file_name( UINT *args )
{
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    char *nameA = get_ptr( &args );
    ULONG *size = get_ptr( &args );
    UINT disposition = get_ulong( &args );

    struct object_attr64 attr;

    return wine_nt_to_unix_file_name( objattr_32to64_redirect( &attr, attr32 ), nameA, size, disposition );
}


/**********************************************************************
 *           wow64_wine_unix_to_nt_file_name
 */
NTSTATUS WINAPI wow64_wine_unix_to_nt_file_name( UINT *args )
{
    const char *name = get_ptr( &args );
    WCHAR *buffer = get_ptr( &args );
    ULONG *size = get_ptr( &args );

    return wine_unix_to_nt_file_name( name, buffer, size );
}
