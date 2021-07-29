/*
 * WoW64 security functions
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
 *           wow64_NtAccessCheck
 */
NTSTATUS WINAPI wow64_NtAccessCheck( UINT *args )
{
    SECURITY_DESCRIPTOR *sd32 = get_ptr( &args );
    HANDLE handle = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    GENERIC_MAPPING *mapping = get_ptr( &args );
    PRIVILEGE_SET *privs = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );
    ACCESS_MASK *access_granted = get_ptr( &args );
    NTSTATUS *access_status = get_ptr( &args );

    SECURITY_DESCRIPTOR sd;

    return NtAccessCheck( secdesc_32to64( &sd, sd32 ), handle, access, mapping,
                          privs, retlen, access_granted, access_status );
}


/**********************************************************************
 *           wow64_NtAccessCheckAndAuditAlarm
 */
NTSTATUS WINAPI wow64_NtAccessCheckAndAuditAlarm( UINT *args )
{
    UNICODE_STRING32 *subsystem32 = get_ptr( &args );
    HANDLE handle = get_handle( &args );
    UNICODE_STRING32 *typename32 = get_ptr( &args );
    UNICODE_STRING32 *objname32 = get_ptr( &args );
    SECURITY_DESCRIPTOR *sd32 = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    GENERIC_MAPPING *mapping = get_ptr( &args );
    BOOLEAN creation = get_ulong( &args );
    ACCESS_MASK *access_granted = get_ptr( &args );
    BOOLEAN *access_status = get_ptr( &args );
    BOOLEAN *onclose = get_ptr( &args );

    UNICODE_STRING subsystem, typename, objname;
    SECURITY_DESCRIPTOR sd;

    return NtAccessCheckAndAuditAlarm( unicode_str_32to64( &subsystem, subsystem32 ), handle,
                                       unicode_str_32to64( &typename, typename32 ),
                                       unicode_str_32to64( &objname, objname32 ),
                                       secdesc_32to64( &sd, sd32 ), access, mapping, creation,
                                       access_granted, access_status, onclose );
}


/**********************************************************************
 *           wow64_NtAdjustPrivilegesToken
 */
NTSTATUS WINAPI wow64_NtAdjustPrivilegesToken( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN disable = get_ulong( &args );
    TOKEN_PRIVILEGES *privs = get_ptr( &args );
    ULONG len = get_ulong( &args );
    TOKEN_PRIVILEGES *prev = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );

    return NtAdjustPrivilegesToken( handle, disable, privs, len, prev, retlen );
}


/**********************************************************************
 *           wow64_NtDuplicateToken
 */
NTSTATUS WINAPI wow64_NtDuplicateToken( UINT *args )
{
    HANDLE token = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    SECURITY_IMPERSONATION_LEVEL level = get_ulong( &args );
    TOKEN_TYPE type = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtDuplicateToken( token, access, objattr_32to64( &attr, attr32 ),
                                        level, type, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtImpersonateAnonymousToken
 */
NTSTATUS WINAPI wow64_NtImpersonateAnonymousToken( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtImpersonateAnonymousToken( handle );
}


/**********************************************************************
 *           wow64_NtOpenProcessToken
 */
NTSTATUS WINAPI wow64_NtOpenProcessToken( UINT *args )
{
    HANDLE process = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenProcessToken( process, access, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenProcessTokenEx
 */
NTSTATUS WINAPI wow64_NtOpenProcessTokenEx( UINT *args )
{
    HANDLE process = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG attributes = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenProcessTokenEx( process, access, attributes, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenThreadToken
 */
NTSTATUS WINAPI wow64_NtOpenThreadToken( UINT *args )
{
    HANDLE thread = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    BOOLEAN self = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenThreadToken( thread, access, self, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenThreadTokenEx
 */
NTSTATUS WINAPI wow64_NtOpenThreadTokenEx( UINT *args )
{
    HANDLE thread = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    BOOLEAN self = get_ulong( &args );
    ULONG attributes = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenThreadTokenEx( thread, access, self, attributes, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtPrivilegeCheck
 */
NTSTATUS WINAPI wow64_NtPrivilegeCheck( UINT *args )
{
    HANDLE token = get_handle( &args );
    PRIVILEGE_SET *privs = get_ptr( &args );
    BOOLEAN *res = get_ptr( &args );

    return NtPrivilegeCheck( token, privs, res );
}


/**********************************************************************
 *           wow64_NtQuerySecurityObject
 */
NTSTATUS WINAPI wow64_NtQuerySecurityObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    SECURITY_INFORMATION info = get_ulong( &args );
    SECURITY_DESCRIPTOR *sd = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    /* returned descriptor is always SE_SELF_RELATIVE, no mapping needed */
    return NtQuerySecurityObject( handle, info, sd, len, retlen );
}


/**********************************************************************
 *           wow64_NtSetSecurityObject
 */
NTSTATUS WINAPI wow64_NtSetSecurityObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    SECURITY_INFORMATION info = get_ulong( &args );
    SECURITY_DESCRIPTOR *sd32 = get_ptr( &args );

    SECURITY_DESCRIPTOR sd;

    return NtSetSecurityObject( handle, info, secdesc_32to64( &sd, sd32 ));
}
