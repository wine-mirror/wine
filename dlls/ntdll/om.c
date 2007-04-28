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

    TRACE("(%p,0x%08x,%p,0x%08x,%p): stub\n",
          handle, info_class, ptr, len, used_len);

    if (used_len) *used_len = 0;

    switch (info_class)
    {
    case ObjectBasicInformation:
        {
            POBJECT_BASIC_INFORMATION p = (POBJECT_BASIC_INFORMATION)ptr;

            if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

            SERVER_START_REQ( get_object_info )
            {
                req->handle = handle;
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    memset( p, 0, sizeof(*p) );
                    p->GrantedAccess = reply->access;
                    p->PointerCount = reply->ref_count;
                    p->HandleCount = 1; /* at least one */
                    if (used_len) *used_len = sizeof(*p);
                }
            }
            SERVER_END_REQ;
        }
        break;
    case ObjectDataInformation:
        {
            OBJECT_DATA_INFORMATION* p = (OBJECT_DATA_INFORMATION*)ptr;

            if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

            SERVER_START_REQ( set_handle_info )
            {
                req->handle = handle;
                req->flags  = 0;
                req->mask   = 0;
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    p->InheritHandle = (reply->old_flags & HANDLE_FLAG_INHERIT) ? TRUE : FALSE;
                    p->ProtectFromClose = (reply->old_flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) ? TRUE : FALSE;
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

    TRACE("(%p,0x%08x,%p,0x%08x): stub\n",
          handle, info_class, ptr, len);

    switch (info_class)
    {
    case ObjectDataInformation:
        {
            OBJECT_DATA_INFORMATION* p = (OBJECT_DATA_INFORMATION*)ptr;

            if (len < sizeof(*p)) return STATUS_INVALID_BUFFER_SIZE;

            SERVER_START_REQ( set_handle_info )
            {
                req->handle = handle;
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
 * NOTES
 *  only the lowest 4 bit of SecurityObjectInformationClass are used
 *  0x7-0xf returns STATUS_ACCESS_DENIED (even running with system privileges)
 *
 * FIXME
 *  We are constructing a fake sid (Administrators:Full, System:Full, Everyone:Read)
 */
NTSTATUS WINAPI
NtQuerySecurityObject(
	IN HANDLE Object,
	IN SECURITY_INFORMATION RequestedInformation,
	OUT PSECURITY_DESCRIPTOR pSecurityDesriptor,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	static const SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};
	static const SID_IDENTIFIER_AUTHORITY worldSidAuthority = {SECURITY_WORLD_SID_AUTHORITY};
	BYTE Buffer[256];
	PISECURITY_DESCRIPTOR_RELATIVE psd = (PISECURITY_DESCRIPTOR_RELATIVE)Buffer;
	UINT BufferIndex = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

	FIXME("(%p,0x%08x,%p,0x%08x,%p) stub!\n",
	Object, RequestedInformation, pSecurityDesriptor, Length, ResultLength);

	RequestedInformation &= 0x0000000f;

	if (RequestedInformation & SACL_SECURITY_INFORMATION) return STATUS_ACCESS_DENIED;

	ZeroMemory(Buffer, 256);
	RtlCreateSecurityDescriptor((PSECURITY_DESCRIPTOR)psd, SECURITY_DESCRIPTOR_REVISION);
	psd->Control = SE_SELF_RELATIVE |
	  ((RequestedInformation & DACL_SECURITY_INFORMATION) ? SE_DACL_PRESENT:0);

	/* owner: administrator S-1-5-20-220*/
	if (OWNER_SECURITY_INFORMATION & RequestedInformation)
	{
	  SID* psid = (SID*)&(Buffer[BufferIndex]);

	  psd->Owner = BufferIndex;
	  BufferIndex += RtlLengthRequiredSid(2);

	  psid->Revision = SID_REVISION;
	  psid->SubAuthorityCount = 2;
	  psid->IdentifierAuthority = localSidAuthority;
	  psid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
	  psid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
	}

	/* group: built in domain S-1-5-12 */
	if (GROUP_SECURITY_INFORMATION & RequestedInformation)
	{
	  SID* psid = (SID*) &(Buffer[BufferIndex]);

	  psd->Group = BufferIndex;
	  BufferIndex += RtlLengthRequiredSid(1);

	  psid->Revision = SID_REVISION;
	  psid->SubAuthorityCount = 1;
	  psid->IdentifierAuthority = localSidAuthority;
	  psid->SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
	}

	/* discretionary ACL */
	if (DACL_SECURITY_INFORMATION & RequestedInformation)
	{
	  /* acl header */
	  PACL pacl = (PACL)&(Buffer[BufferIndex]);
	  PACCESS_ALLOWED_ACE pace;
	  SID* psid;

	  psd->Dacl = BufferIndex;

	  pacl->AclRevision = MIN_ACL_REVISION;
	  pacl->AceCount = 3;
	  pacl->AclSize = BufferIndex; /* storing the start index temporary */

	  BufferIndex += sizeof(ACL);

	  /* ACE System - full access */
	  pace = (PACCESS_ALLOWED_ACE)&(Buffer[BufferIndex]);
	  BufferIndex += sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD);

	  pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
	  pace->Header.AceFlags = CONTAINER_INHERIT_ACE;
	  pace->Header.AceSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD) + RtlLengthRequiredSid(1);
          pace->Mask = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
	  pace->SidStart = BufferIndex;

	  /* SID S-1-5-12 (System) */
	  psid = (SID*)&(Buffer[BufferIndex]);

	  BufferIndex += RtlLengthRequiredSid(1);

	  psid->Revision = SID_REVISION;
	  psid->SubAuthorityCount = 1;
	  psid->IdentifierAuthority = localSidAuthority;
	  psid->SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;

	  /* ACE Administrators - full access*/
	  pace = (PACCESS_ALLOWED_ACE) &(Buffer[BufferIndex]);
	  BufferIndex += sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD);

	  pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
	  pace->Header.AceFlags = CONTAINER_INHERIT_ACE;
	  pace->Header.AceSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD) + RtlLengthRequiredSid(2);
          pace->Mask = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
	  pace->SidStart = BufferIndex;

	  /* S-1-5-12 (Administrators) */
	  psid = (SID*)&(Buffer[BufferIndex]);

	  BufferIndex += RtlLengthRequiredSid(2);

	  psid->Revision = SID_REVISION;
	  psid->SubAuthorityCount = 2;
	  psid->IdentifierAuthority = localSidAuthority;
	  psid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
	  psid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;

	  /* ACE Everyone - read access */
	  pace = (PACCESS_ALLOWED_ACE)&(Buffer[BufferIndex]);
	  BufferIndex += sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD);

	  pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
	  pace->Header.AceFlags = CONTAINER_INHERIT_ACE;
	  pace->Header.AceSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD) + RtlLengthRequiredSid(1);
	  pace->Mask = READ_CONTROL| 0x19;
	  pace->SidStart = BufferIndex;

	  /* SID S-1-1-0 (Everyone) */
	  psid = (SID*)&(Buffer[BufferIndex]);

	  BufferIndex += RtlLengthRequiredSid(1);

	  psid->Revision = SID_REVISION;
	  psid->SubAuthorityCount = 1;
	  psid->IdentifierAuthority = worldSidAuthority;
	  psid->SubAuthority[0] = 0;

	  /* calculate used bytes */
	  pacl->AclSize = BufferIndex - pacl->AclSize;
	}
	*ResultLength = BufferIndex;
	TRACE("len=%u\n", *ResultLength);
	if (Length < *ResultLength) return STATUS_BUFFER_TOO_SMALL;
	memcpy(pSecurityDesriptor, Buffer, *ResultLength);

	return STATUS_SUCCESS;
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
        req->src_process = source_process;
        req->src_handle  = source;
        req->dst_process = dest_process;
        req->access      = access;
        req->attributes  = attributes;
        req->options     = options;

        if (!(ret = wine_server_call( req )))
        {
            if (dest) *dest = reply->handle;
            if (reply->closed)
            {
                if (reply->self)
                {
                    int fd = server_remove_fd_from_cache( source );
                    if (fd != -1) close( fd );
                }
            }
            else if (options & DUPLICATE_CLOSE_SOURCE)
                WARN( "failed to close handle %p in process %p\n", source, source_process );
        }
    }
    SERVER_END_REQ;
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
    NTSTATUS ret;
    int fd = server_remove_fd_from_cache( Handle );

    SERVER_START_REQ( close_handle )
    {
        req->handle = Handle;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (fd != -1) close( fd );
    return ret;
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
NTSTATUS WINAPI NtOpenDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess,
                                      POBJECT_ATTRIBUTES ObjectAttributes)
{
    NTSTATUS ret;
    TRACE("(%p,0x%08x)\n", DirectoryHandle, DesiredAccess);
    dump_ObjectAttributes(ObjectAttributes);

    if (!DirectoryHandle) return STATUS_ACCESS_VIOLATION;
    if (!ObjectAttributes) return STATUS_INVALID_PARAMETER;
    /* Have to test it here because server won't know difference between
     * ObjectName == NULL and ObjectName == "" */
    if (!ObjectAttributes->ObjectName)
    {
        if (ObjectAttributes->RootDirectory)
            return STATUS_OBJECT_NAME_INVALID;
        else
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    SERVER_START_REQ(open_directory)
    {
        req->access = DesiredAccess;
        req->attributes = ObjectAttributes->Attributes;
        req->rootdir = ObjectAttributes->RootDirectory;
        if (ObjectAttributes->ObjectName)
            wine_server_add_data(req, ObjectAttributes->ObjectName->Buffer,
                                 ObjectAttributes->ObjectName->Length);
        ret = wine_server_call( req );
        *DirectoryHandle = reply->handle;
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
                                        POBJECT_ATTRIBUTES ObjectAttributes)
{
    NTSTATUS ret;
    TRACE("(%p,0x%08x)\n", DirectoryHandle, DesiredAccess);
    dump_ObjectAttributes(ObjectAttributes);

    if (!DirectoryHandle) return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ(create_directory)
    {
        req->access = DesiredAccess;
        req->attributes = ObjectAttributes ? ObjectAttributes->Attributes : 0;
        req->rootdir = ObjectAttributes ? ObjectAttributes->RootDirectory : 0;
        if (ObjectAttributes && ObjectAttributes->ObjectName)
            wine_server_add_data(req, ObjectAttributes->ObjectName->Buffer,
                                 ObjectAttributes->ObjectName->Length);
        ret = wine_server_call( req );
        *DirectoryHandle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 * NtQueryDirectoryObject [NTDLL.@]
 * ZwQueryDirectoryObject [NTDLL.@]
 *
 * Read information from a namespace directory.
 * 
 * PARAMS
 *  DirectoryHandle   [I]   Handle to a directory object
 *  Buffer            [O]   Buffer to hold the read data
 *  BufferLength      [I]   Size of the buffer in bytes
 *  ReturnSingleEntry [I]   If TRUE, return a single entry, if FALSE, return as many as fit in the buffer
 *  RestartScan       [I]   If TRUE, start scanning from the start, if FALSE, scan from Context
 *  Context           [I/O] Indicates what point of the directory the scan is at
 *  ReturnLength      [O]   Caller supplied storage for the number of bytes written (or NULL)
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtQueryDirectoryObject(IN HANDLE DirectoryHandle, OUT PDIRECTORY_BASIC_INFORMATION Buffer,
                                       IN ULONG BufferLength, IN BOOLEAN ReturnSingleEntry, IN BOOLEAN RestartScan,
                                       IN OUT PULONG Context, OUT PULONG ReturnLength OPTIONAL)
{
    FIXME("(%p,%p,0x%08x,0x%08x,0x%08x,%p,%p), stub\n", DirectoryHandle, Buffer, BufferLength, ReturnSingleEntry,
          RestartScan, Context, ReturnLength);

    return STATUS_NOT_IMPLEMENTED;
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
NTSTATUS WINAPI NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle, IN ACCESS_MASK DesiredAccess,
                                         IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    NTSTATUS ret;
    TRACE("(%p,0x%08x,%p)\n",LinkHandle, DesiredAccess, ObjectAttributes);
    dump_ObjectAttributes(ObjectAttributes);

    if (!LinkHandle) return STATUS_ACCESS_VIOLATION;
    if (!ObjectAttributes) return STATUS_INVALID_PARAMETER;
    /* Have to test it here because server won't know difference between
     * ObjectName == NULL and ObjectName == "" */
    if (!ObjectAttributes->ObjectName)
    {
        if (ObjectAttributes->RootDirectory)
            return STATUS_OBJECT_NAME_INVALID;
        else
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    SERVER_START_REQ(open_symlink)
    {
        req->access = DesiredAccess;
        req->attributes = ObjectAttributes->Attributes;
        req->rootdir = ObjectAttributes->RootDirectory;
        if (ObjectAttributes->ObjectName)
            wine_server_add_data(req, ObjectAttributes->ObjectName->Buffer,
                                 ObjectAttributes->ObjectName->Length);
        ret = wine_server_call( req );
        *LinkHandle = reply->handle;
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
	                                   IN POBJECT_ATTRIBUTES ObjectAttributes,
                                           IN PUNICODE_STRING TargetName)
{
    NTSTATUS ret;
    TRACE("(%p,0x%08x,%p, -> %s)\n", SymbolicLinkHandle, DesiredAccess, ObjectAttributes,
                                      debugstr_us(TargetName));
    dump_ObjectAttributes(ObjectAttributes);

    if (!SymbolicLinkHandle || !TargetName) return STATUS_ACCESS_VIOLATION;
    if (!TargetName->Buffer) return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ(create_symlink)
    {
        req->access = DesiredAccess;
        req->attributes = ObjectAttributes ? ObjectAttributes->Attributes : 0;
        req->rootdir = ObjectAttributes ? ObjectAttributes->RootDirectory : 0;
        if (ObjectAttributes && ObjectAttributes->ObjectName)
        {
            req->name_len = ObjectAttributes->ObjectName->Length;
            wine_server_add_data(req, ObjectAttributes->ObjectName->Buffer,
                                 ObjectAttributes->ObjectName->Length);
        }
        else
            req->name_len = 0;
        wine_server_add_data(req, TargetName->Buffer, TargetName->Length);
        ret = wine_server_call( req );
        *SymbolicLinkHandle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQuerySymbolicLinkObject	[NTDLL.@]
 *  ZwQuerySymbolicLinkObject	[NTDLL.@]
 *
 * Query a namespace symbolic link object target name.
 * 
 * PARAMS
 *  LinkHandle     [I] Handle to a symbolic link object
 *  LinkTarget     [O] Destination for the symbolic link target
 *  ReturnedLength [O] Size of returned data
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtQuerySymbolicLinkObject(IN HANDLE LinkHandle, IN OUT PUNICODE_STRING LinkTarget,
                                          OUT PULONG ReturnedLength OPTIONAL)
{
    NTSTATUS ret;
    TRACE("(%p,%p,%p)\n", LinkHandle, LinkTarget, ReturnedLength);

    if (!LinkTarget) return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ(query_symlink)
    {
        req->handle = LinkHandle;
        wine_server_set_reply( req, LinkTarget->Buffer, LinkTarget->MaximumLength );
        if (!(ret = wine_server_call( req )))
        {
            LinkTarget->Length = wine_server_reply_size(reply);
            if (ReturnedLength) *ReturnedLength = LinkTarget->Length;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtAllocateUuids   [NTDLL.@]
 */
NTSTATUS WINAPI NtAllocateUuids(
        PULARGE_INTEGER Time,
        PULONG Range,
        PULONG Sequence)
{
        FIXME("(%p,%p,%p), stub.\n", Time, Range, Sequence);
	return 0;
}
