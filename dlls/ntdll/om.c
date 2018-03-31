/*
 *	Object management functions
 *
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2005 Vitaliy Margolen
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


/*
 *	Generic object functions
 */

/******************************************************************************
 * NtQueryObject [NTDLL.@]
 * ZwQueryObject [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryObject(IN HANDLE handle,
                              IN OBJECT_INFORMATION_CLASS info_class,
                              OUT PVOID ptr, IN ULONG len, OUT PULONG used_len)
{
    NTSTATUS status;

    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n", handle, info_class, ptr, len, used_len);

    if (used_len) *used_len = 0;

    switch (info_class)
    {
    case ObjectBasicInformation:
        {
            POBJECT_BASIC_INFORMATION p = ptr;

            if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

            SERVER_START_REQ( get_object_info )
            {
                req->handle = wine_server_obj_handle( handle );
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    memset( p, 0, sizeof(*p) );
                    p->GrantedAccess = reply->access;
                    p->PointerCount = reply->ref_count;
                    p->HandleCount = reply->handle_count;
                    if (used_len) *used_len = sizeof(*p);
                }
            }
            SERVER_END_REQ;
        }
        break;
    case ObjectNameInformation:
        {
            OBJECT_NAME_INFORMATION* p = ptr;
            ANSI_STRING unix_name;

            /* first try as a file object */

            if (!(status = server_get_unix_name( handle, &unix_name )))
            {
                UNICODE_STRING nt_name;

                if (!(status = wine_unix_to_nt_file_name( &unix_name, &nt_name )))
                {
                    if (len < sizeof(*p))
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    else if (len < sizeof(*p) + nt_name.MaximumLength)
                        status = STATUS_BUFFER_OVERFLOW;
                    else
                    {
                        p->Name.Buffer = (WCHAR *)(p + 1);
                        p->Name.Length = nt_name.Length;
                        p->Name.MaximumLength = nt_name.MaximumLength;
                        memcpy( p->Name.Buffer, nt_name.Buffer, nt_name.MaximumLength );
                    }
                    if (used_len) *used_len = sizeof(*p) + nt_name.MaximumLength;
                    RtlFreeUnicodeString( &nt_name );
                }
                RtlFreeAnsiString( &unix_name );
                break;
            }
            else if (status != STATUS_OBJECT_TYPE_MISMATCH) break;

            /* not a file, treat as a generic object */

            SERVER_START_REQ( get_object_info )
            {
                req->handle = wine_server_obj_handle( handle );
                if (len > sizeof(*p)) wine_server_set_reply( req, p + 1, len - sizeof(*p) );
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    if (!reply->total)  /* no name */
                    {
                        if (sizeof(*p) > len) status = STATUS_INFO_LENGTH_MISMATCH;
                        else memset( p, 0, sizeof(*p) );
                        if (used_len) *used_len = sizeof(*p);
                    }
                    else if (sizeof(*p) + reply->total + sizeof(WCHAR) > len)
                    {
                        if (used_len) *used_len = sizeof(*p) + reply->total + sizeof(WCHAR);
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    else
                    {
                        ULONG res = wine_server_reply_size( reply );
                        p->Name.Buffer = (WCHAR *)(p + 1);
                        p->Name.Length = res;
                        p->Name.MaximumLength = res + sizeof(WCHAR);
                        p->Name.Buffer[res / sizeof(WCHAR)] = 0;
                        if (used_len) *used_len = sizeof(*p) + p->Name.MaximumLength;
                    }
                }
            }
            SERVER_END_REQ;
        }
        break;
    case ObjectTypeInformation:
        {
            OBJECT_TYPE_INFORMATION *p = ptr;

            SERVER_START_REQ( get_object_type )
            {
                req->handle = wine_server_obj_handle( handle );
                if (len > sizeof(*p)) wine_server_set_reply( req, p + 1, len - sizeof(*p) );
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    if (!reply->total)  /* no name */
                    {
                        if (sizeof(*p) > len) status = STATUS_INFO_LENGTH_MISMATCH;
                        else memset( p, 0, sizeof(*p) );
                        if (used_len) *used_len = sizeof(*p);
                    }
                    else if (sizeof(*p) + reply->total + sizeof(WCHAR) > len)
                    {
                        if (used_len) *used_len = sizeof(*p) + reply->total + sizeof(WCHAR);
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    else
                    {
                        ULONG res = wine_server_reply_size( reply );
                        p->TypeName.Buffer = (WCHAR *)(p + 1);
                        p->TypeName.Length = res;
                        p->TypeName.MaximumLength = res + sizeof(WCHAR);
                        p->TypeName.Buffer[res / sizeof(WCHAR)] = 0;
                        if (used_len) *used_len = sizeof(*p) + p->TypeName.MaximumLength;
                    }
                }
            }
            SERVER_END_REQ;
        }
        break;
    case ObjectDataInformation:
        {
            OBJECT_DATA_INFORMATION* p = ptr;

            if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

            SERVER_START_REQ( set_handle_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = 0;
                req->mask   = 0;
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    p->InheritHandle = (reply->old_flags & HANDLE_FLAG_INHERIT) != 0;
                    p->ProtectFromClose = (reply->old_flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;
                    if (used_len) *used_len = sizeof(*p);
                }
            }
            SERVER_END_REQ;
        }
        break;
    default:
        FIXME("Unsupported information class %u\n", info_class);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    return status;
}

/******************************************************************
 *		NtSetInformationObject [NTDLL.@]
 *		ZwSetInformationObject [NTDLL.@]
 *
 */
NTSTATUS WINAPI NtSetInformationObject(IN HANDLE handle,
                                       IN OBJECT_INFORMATION_CLASS info_class,
                                       IN PVOID ptr, IN ULONG len)
{
    NTSTATUS status;

    TRACE("(%p,0x%08x,%p,0x%08x)\n", handle, info_class, ptr, len);

    switch (info_class)
    {
    case ObjectDataInformation:
        {
            OBJECT_DATA_INFORMATION* p = ptr;

            if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

            SERVER_START_REQ( set_handle_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = 0;
                req->mask   = HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE;
                if (p->InheritHandle)    req->flags |= HANDLE_FLAG_INHERIT;
                if (p->ProtectFromClose) req->flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        break;
    default:
        FIXME("Unsupported information class %u\n", info_class);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }
    return status;
}

/******************************************************************************
 *  NtQuerySecurityObject	[NTDLL.@]
 *
 * An ntdll analogue to GetKernelObjectSecurity().
 *
 */
NTSTATUS WINAPI
NtQuerySecurityObject(
	IN HANDLE Object,
	IN SECURITY_INFORMATION RequestedInformation,
	OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
    PISECURITY_DESCRIPTOR_RELATIVE psd = pSecurityDescriptor;
    NTSTATUS status;
    unsigned int buffer_size = 512;
    BOOLEAN need_more_memory;

    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n",
	Object, RequestedInformation, pSecurityDescriptor, Length, ResultLength);

    do
    {
        char *buffer = RtlAllocateHeap(GetProcessHeap(), 0, buffer_size);
        if (!buffer)
            return STATUS_NO_MEMORY;

        need_more_memory = FALSE;

        SERVER_START_REQ( get_security_object )
        {
            req->handle = wine_server_obj_handle( Object );
            req->security_info = RequestedInformation;
            wine_server_set_reply( req, buffer, buffer_size );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                struct security_descriptor *sd = (struct security_descriptor *)buffer;
                if (reply->sd_len)
                {
                    *ResultLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                        sd->owner_len + sd->group_len + sd->sacl_len + sd->dacl_len;
                    if (Length >= *ResultLength)
                    {
                        psd->Revision = SECURITY_DESCRIPTOR_REVISION;
                        psd->Sbz1 = 0;
                        psd->Control = sd->control | SE_SELF_RELATIVE;
                        psd->Owner = sd->owner_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) : 0;
                        psd->Group = sd->group_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sd->owner_len : 0;
                        psd->Sacl = sd->sacl_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sd->owner_len + sd->group_len : 0;
                        psd->Dacl = sd->dacl_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sd->owner_len + sd->group_len + sd->sacl_len : 0;
                        /* owner, group, sacl and dacl are the same type as in the server
                         * and in the same order so we copy the memory in one block */
                        memcpy((char *)pSecurityDescriptor + sizeof(SECURITY_DESCRIPTOR_RELATIVE),
                               buffer + sizeof(struct security_descriptor),
                               sd->owner_len + sd->group_len + sd->sacl_len + sd->dacl_len);
                    }
                    else
                        status = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    *ResultLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
                    if (Length >= *ResultLength)
                    {
                        memset(psd, 0, sizeof(*psd));
                        psd->Revision = SECURITY_DESCRIPTOR_REVISION;
                        psd->Control = SE_SELF_RELATIVE;
                    }
                    else
                        status = STATUS_BUFFER_TOO_SMALL;
                }
            }
            else if (status == STATUS_BUFFER_TOO_SMALL)
            {
                buffer_size = reply->sd_len;
                need_more_memory = TRUE;
            }
        }
        SERVER_END_REQ;
        RtlFreeHeap(GetProcessHeap(), 0, buffer);
    } while (need_more_memory);

    return status;
}


/******************************************************************************
 *  NtDuplicateObject		[NTDLL.@]
 *  ZwDuplicateObject		[NTDLL.@]
 */
NTSTATUS WINAPI NtDuplicateObject( HANDLE source_process, HANDLE source,
                                   HANDLE dest_process, PHANDLE dest,
                                   ACCESS_MASK access, ULONG attributes, ULONG options )
{
    NTSTATUS ret;
    SERVER_START_REQ( dup_handle )
    {
        req->src_process = wine_server_obj_handle( source_process );
        req->src_handle  = wine_server_obj_handle( source );
        req->dst_process = wine_server_obj_handle( dest_process );
        req->access      = access;
        req->attributes  = attributes;
        req->options     = options;

        if (!(ret = wine_server_call( req )))
        {
            if (dest) *dest = wine_server_ptr_handle( reply->handle );
            if (reply->closed && reply->self)
            {
                int fd = server_remove_fd_from_cache( source );
                if (fd != -1) close( fd );
            }
        }
    }
    SERVER_END_REQ;
    return ret;
}

static LONG WINAPI invalid_handle_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == EXCEPTION_INVALID_HANDLE) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/* Everquest 2 / Pirates of the Burning Sea hooks NtClose, so we need a wrapper */
NTSTATUS close_handle( HANDLE handle )
{
    NTSTATUS ret;
    int fd = server_remove_fd_from_cache( handle );

    SERVER_START_REQ( close_handle )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (fd != -1) close( fd );

    if (ret == STATUS_INVALID_HANDLE && handle && NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            EXCEPTION_RECORD record;
            record.ExceptionCode    = EXCEPTION_INVALID_HANDLE;
            record.ExceptionFlags   = 0;
            record.ExceptionRecord  = NULL;
            record.ExceptionAddress = NULL;
            record.NumberParameters = 0;
            RtlRaiseException( &record );
        }
        __EXCEPT(invalid_handle_exception_handler)
        {
        }
        __ENDTRY
    }

    return ret;
}

/**************************************************************************
 *                 NtClose				[NTDLL.@]
 *
 * Close a handle reference to an object.
 * 
 * PARAMS
 *  Handle [I] handle to close
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtClose( HANDLE Handle )
{
    return close_handle( Handle );
}

/*
 *	Directory functions
 */

/**************************************************************************
 * NtOpenDirectoryObject [NTDLL.@]
 * ZwOpenDirectoryObject [NTDLL.@]
 *
 * Open a namespace directory object.
 * 
 * PARAMS
 *  DirectoryHandle  [O] Destination for the new directory handle
 *  DesiredAccess    [I] Desired access to the directory
 *  ObjectAttributes [I] Structure describing the directory
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtOpenDirectoryObject( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr)
{
    NTSTATUS ret;

    if (!handle) return STATUS_ACCESS_VIOLATION;
    if ((ret = validate_open_object_attributes( attr ))) return ret;

    TRACE("(%p,0x%08x,%s)\n", handle, access, debugstr_ObjectAttributes(attr));

    SERVER_START_REQ(open_directory)
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtCreateDirectoryObject	[NTDLL.@]
 *  ZwCreateDirectoryObject	[NTDLL.@]
 *
 * Create a namespace directory object.
 * 
 * PARAMS
 *  DirectoryHandle  [O] Destination for the new directory handle
 *  DesiredAccess    [I] Desired access to the directory
 *  ObjectAttributes [I] Structure describing the directory
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtCreateDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess,
                                        OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if (!DirectoryHandle) return STATUS_ACCESS_VIOLATION;
    TRACE("(%p,0x%08x,%s)\n", DirectoryHandle, DesiredAccess, debugstr_ObjectAttributes(attr));

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ(create_directory)
    {
        req->access = DesiredAccess;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *DirectoryHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}

/******************************************************************************
 * NtQueryDirectoryObject [NTDLL.@]
 * ZwQueryDirectoryObject [NTDLL.@]
 *
 * Read information from a namespace directory.
 * 
 * PARAMS
 *  handle        [I]   Handle to a directory object
 *  buffer        [O]   Buffer to hold the read data
 *  size          [I]   Size of the buffer in bytes
 *  single_entry  [I]   If TRUE, return a single entry, if FALSE, return as many as fit in the buffer
 *  restart       [I]   If TRUE, start scanning from the start, if FALSE, scan from Context
 *  context       [I/O] Indicates what point of the directory the scan is at
 *  ret_size      [O]   Caller supplied storage for the number of bytes written (or NULL)
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtQueryDirectoryObject(HANDLE handle, PDIRECTORY_BASIC_INFORMATION buffer,
                                       ULONG size, BOOLEAN single_entry, BOOLEAN restart,
                                       PULONG context, PULONG ret_size)
{
    NTSTATUS ret;

    if (restart) *context = 0;

    if (single_entry)
    {
        if (size <= sizeof(*buffer) + 2*sizeof(WCHAR)) return STATUS_BUFFER_OVERFLOW;

        SERVER_START_REQ( get_directory_entry )
        {
            req->handle = wine_server_obj_handle( handle );
            req->index = *context;
            wine_server_set_reply( req, buffer + 1, size - sizeof(*buffer) - 2*sizeof(WCHAR) );
            if (!(ret = wine_server_call( req )))
            {
                buffer->ObjectName.Buffer = (WCHAR *)(buffer + 1);
                buffer->ObjectName.Length = reply->name_len;
                buffer->ObjectName.MaximumLength = reply->name_len + sizeof(WCHAR);
                buffer->ObjectTypeName.Buffer = (WCHAR *)(buffer + 1) + reply->name_len/sizeof(WCHAR) + 1;
                buffer->ObjectTypeName.Length = wine_server_reply_size( reply ) - reply->name_len;
                buffer->ObjectTypeName.MaximumLength = buffer->ObjectTypeName.Length + sizeof(WCHAR);
                /* make room for the terminating null */
                memmove( buffer->ObjectTypeName.Buffer, buffer->ObjectTypeName.Buffer - 1,
                         buffer->ObjectTypeName.Length );
                buffer->ObjectName.Buffer[buffer->ObjectName.Length/sizeof(WCHAR)] = 0;
                buffer->ObjectTypeName.Buffer[buffer->ObjectTypeName.Length/sizeof(WCHAR)] = 0;
                (*context)++;
            }
        }
        SERVER_END_REQ;
        if (ret_size)
            *ret_size = buffer->ObjectName.MaximumLength + buffer->ObjectTypeName.MaximumLength + sizeof(*buffer);
    }
    else
    {
        FIXME("multiple entries not implemented\n");
        ret = STATUS_NOT_IMPLEMENTED;
    }

    return ret;
}

/*
 *	Link objects
 */

/******************************************************************************
 *  NtOpenSymbolicLinkObject	[NTDLL.@]
 *  ZwOpenSymbolicLinkObject	[NTDLL.@]
 *
 * Open a namespace symbolic link object.
 * 
 * PARAMS
 *  LinkHandle       [O] Destination for the new symbolic link handle
 *  DesiredAccess    [I] Desired access to the symbolic link
 *  ObjectAttributes [I] Structure describing the symbolic link
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtOpenSymbolicLinkObject( HANDLE *handle, ACCESS_MASK access,
                                          const OBJECT_ATTRIBUTES *attr)
{
    NTSTATUS ret;

    TRACE("(%p,0x%08x,%s)\n", handle, access, debugstr_ObjectAttributes(attr));

    if (!handle) return STATUS_ACCESS_VIOLATION;
    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ(open_symlink)
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtCreateSymbolicLinkObject	[NTDLL.@]
 *  ZwCreateSymbolicLinkObject	[NTDLL.@]
 *
 * Open a namespace symbolic link object.
 * 
 * PARAMS
 *  SymbolicLinkHandle [O] Destination for the new symbolic link handle
 *  DesiredAccess      [I] Desired access to the symbolic link
 *  ObjectAttributes   [I] Structure describing the symbolic link
 *  TargetName         [I] Name of the target symbolic link points to
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtCreateSymbolicLinkObject(OUT PHANDLE SymbolicLinkHandle,IN ACCESS_MASK DesiredAccess,
	                                   POBJECT_ATTRIBUTES attr, PUNICODE_STRING TargetName)
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if (!SymbolicLinkHandle || !attr || !TargetName) return STATUS_ACCESS_VIOLATION;
    if (!TargetName->Buffer) return STATUS_INVALID_PARAMETER;

    TRACE("(%p,0x%08x,%s -> %s)\n", SymbolicLinkHandle, DesiredAccess,
          debugstr_ObjectAttributes(attr), debugstr_us(TargetName));

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ(create_symlink)
    {
        req->access = DesiredAccess;
        wine_server_add_data( req, objattr, len );
        wine_server_add_data(req, TargetName->Buffer, TargetName->Length);
        ret = wine_server_call( req );
        *SymbolicLinkHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}

/******************************************************************************
 *  NtQuerySymbolicLinkObject	[NTDLL.@]
 *  ZwQuerySymbolicLinkObject	[NTDLL.@]
 *
 * Query a namespace symbolic link object target name.
 *
 * PARAMS
 *  handle     [I] Handle to a symbolic link object
 *  target     [O] Destination for the symbolic link target
 *  length     [O] Size of returned data
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtQuerySymbolicLinkObject( HANDLE handle, PUNICODE_STRING target, PULONG length )
{
    NTSTATUS ret;

    TRACE("(%p,%p,%p)\n", handle, target, length );

    if (!target) return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ(query_symlink)
    {
        req->handle = wine_server_obj_handle( handle );
        if (target->MaximumLength >= sizeof(WCHAR))
            wine_server_set_reply( req, target->Buffer, target->MaximumLength - sizeof(WCHAR) );
        if (!(ret = wine_server_call( req )))
        {
            target->Length = wine_server_reply_size(reply);
            target->Buffer[target->Length / sizeof(WCHAR)] = 0;
            if (length) *length = reply->total + sizeof(WCHAR);
        }
        else if (length && ret == STATUS_BUFFER_TOO_SMALL) *length = reply->total + sizeof(WCHAR);
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtAllocateUuids   [NTDLL.@]
 */
NTSTATUS WINAPI NtAllocateUuids( ULARGE_INTEGER *time, ULONG *delta, ULONG *sequence, UCHAR *seed )
{
    FIXME("(%p,%p,%p,%p), stub.\n", time, delta, sequence, seed);
    return STATUS_SUCCESS;
}

/**************************************************************************
 *  NtMakeTemporaryObject	[NTDLL.@]
 *  ZwMakeTemporaryObject	[NTDLL.@]
 *
 * Make a permanent object temporary.
 *
 * PARAMS
 *  Handle [I] handle to permanent object
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtMakeTemporaryObject( HANDLE Handle )
{
    FIXME("(%p), stub.\n", Handle);
    return STATUS_SUCCESS;
}
