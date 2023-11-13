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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


static TOKEN_GROUPS *token_groups_32to64( const TOKEN_GROUPS32 *groups32 )
{
    TOKEN_GROUPS *groups;
    ULONG i;

    if (!groups32) return NULL;
    groups = Wow64AllocateTemp( offsetof( TOKEN_GROUPS, Groups[groups32->GroupCount] ));
    groups->GroupCount = groups32->GroupCount;
    for (i = 0; i < groups->GroupCount; i++)
    {
        groups->Groups[i].Sid = ULongToPtr( groups32->Groups[i].Sid );
        groups->Groups[i].Attributes = groups32->Groups[i].Attributes;
    }
    return groups;
}


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
 *           wow64_NtAdjustGroupsToken
 */
NTSTATUS WINAPI wow64_NtAdjustGroupsToken( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN reset = get_ulong( &args );
    TOKEN_GROUPS32 *groups = get_ptr( &args );
    ULONG len = get_ulong( &args );
    TOKEN_GROUPS32 *prev = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );

    FIXME( "%p %d %p %lu %p %p\n", handle, reset, groups, len, prev, retlen );
    return STATUS_NOT_IMPLEMENTED;
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
 *           wow64_NtCreateLowBoxToken
 */
NTSTATUS WINAPI wow64_NtCreateLowBoxToken( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    HANDLE token = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    SID *sid = get_ptr( &args );
    ULONG count = get_ulong( &args );
    SID_AND_ATTRIBUTES32 *capabilities32 = get_ptr( &args );
    ULONG handle_count = get_ulong( &args );
    ULONG *handles32 = get_ptr( &args );

    FIXME( "%p %p %lx %p %p %lu %p %lu %p: stub\n",
           handle_ptr, token, access, attr32, sid, count, capabilities32, handle_count, handles32 );

    *handle_ptr = 0;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           wow64_NtCreateToken
 */
NTSTATUS WINAPI wow64_NtCreateToken( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    TOKEN_TYPE type = get_ulong( &args );
    LUID *luid = get_ptr( &args );
    LARGE_INTEGER *expire = get_ptr( &args );
    TOKEN_USER32 *user32 = get_ptr( &args );
    TOKEN_GROUPS32 *groups32 = get_ptr( &args );
    TOKEN_PRIVILEGES *privs = get_ptr( &args );
    TOKEN_OWNER32 *owner32 = get_ptr( &args );
    TOKEN_PRIMARY_GROUP32 *group32 = get_ptr( &args );
    TOKEN_DEFAULT_DACL32 *dacl32 = get_ptr( &args );
    TOKEN_SOURCE *source = get_ptr( &args );

    struct object_attr64 attr;
    TOKEN_USER user;
    TOKEN_OWNER owner;
    TOKEN_PRIMARY_GROUP group;
    TOKEN_DEFAULT_DACL dacl;
    HANDLE handle = 0;
    NTSTATUS status;

    status = NtCreateToken( &handle, access, objattr_32to64( &attr, attr32 ), type, luid, expire,
                            token_user_32to64( &user, user32 ), token_groups_32to64( groups32 ), privs,
                            token_owner_32to64( &owner, owner32 ), token_primary_group_32to64( &group, group32 ),
                            token_default_dacl_32to64( &dacl, dacl32 ), source );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtDuplicateToken
 */
NTSTATUS WINAPI wow64_NtDuplicateToken( UINT *args )
{
    HANDLE token = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    BOOLEAN effective_only = get_ulong( &args );
    TOKEN_TYPE type = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtDuplicateToken( token, access, objattr_32to64( &attr, attr32 ), effective_only, type, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtFilterToken
 */
NTSTATUS WINAPI wow64_NtFilterToken( UINT *args )
{
    HANDLE token = get_handle( &args );
    ULONG flags = get_ulong( &args );
    TOKEN_GROUPS32 *disable_sids32 = get_ptr( &args );
    TOKEN_PRIVILEGES *privs = get_ptr( &args );
    TOKEN_GROUPS32 *restrict_sids32 = get_ptr( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtFilterToken( token, flags, token_groups_32to64( disable_sids32 ), privs,
                            token_groups_32to64( restrict_sids32 ), &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCompareTokens
 */
NTSTATUS WINAPI wow64_NtCompareTokens( UINT *args )
{
    HANDLE first = get_handle( &args );
    HANDLE second = get_handle( &args );
    BOOLEAN *equal = get_ptr( &args );

    return NtCompareTokens( first, second, equal );
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
 *           wow64_NtQueryInformationToken
 */
NTSTATUS WINAPI wow64_NtQueryInformationToken( UINT *args )
{
    HANDLE handle = get_handle( &args );
    TOKEN_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;
    ULONG ret_size, sid_len;

    switch (class)
    {
    case TokenPrivileges: /* TOKEN_PRIVILEGES */
    case TokenImpersonationLevel:  /* SECURITY_IMPERSONATION_LEVEL */
    case TokenStatistics:  /* TOKEN_STATISTICS */
    case TokenType: /* TOKEN_TYPE */
    case TokenElevationType:  /* TOKEN_ELEVATION_TYPE */
    case TokenElevation: /* TOKEN_ELEVATION */
    case TokenSessionId:  /* ULONG */
    case TokenVirtualizationEnabled:  /* ULONG */
    case TokenIsAppContainer:  /* ULONG */
        /* nothing to map */
        return NtQueryInformationToken( handle, class, info, len, retlen );

    case TokenUser:  /* TOKEN_USER + SID */
    case TokenIntegrityLevel:  /* TOKEN_MANDATORY_LABEL + SID */
    {
        ULONG_PTR buffer[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(ULONG_PTR)];
        TOKEN_USER *user = (TOKEN_USER *)buffer;
        TOKEN_USER32 *user32 = info;
        SID *sid;

        status = NtQueryInformationToken( handle, class, &buffer, sizeof(buffer), &ret_size );
        if (status) return status;
        sid = user->User.Sid;
        sid_len = offsetof( SID, SubAuthority[sid->SubAuthorityCount] );
        if (len >= sizeof(*user32) + sid_len)
        {
            user32->User.Sid = PtrToUlong( user32 + 1 );
            user32->User.Attributes = user->User.Attributes;
            memcpy( user32 + 1, sid, sid_len );
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        if (retlen) *retlen = sizeof(*user32) + sid_len;
        return status;
    }

    case TokenOwner:  /* TOKEN_OWNER + SID  */
    case TokenPrimaryGroup:  /* TOKEN_PRIMARY_GROUP + SID */
    case TokenAppContainerSid:  /* TOKEN_APPCONTAINER_INFORMATION + SID */
    {
        ULONG_PTR buffer[(sizeof(TOKEN_OWNER) + SECURITY_MAX_SID_SIZE) / sizeof(ULONG_PTR)];
        TOKEN_OWNER *owner = (TOKEN_OWNER *)buffer;
        TOKEN_OWNER32 *owner32 = info;
        SID *sid;

        status = NtQueryInformationToken( handle, class, &buffer, sizeof(buffer), &ret_size );
        if (status) return status;
        sid = owner->Owner;
        sid_len = offsetof( SID, SubAuthority[sid->SubAuthorityCount] );
        if (len >= sizeof(*owner32) + sid_len)
        {
            owner32->Owner = PtrToUlong( owner32 + 1 );
            memcpy( owner32 + 1, sid, sid_len );
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        if (retlen) *retlen = sizeof(*owner32) + sid_len;
        return status;
    }

    case TokenGroups:  /* TOKEN_GROUPS */
    case TokenLogonSid:   /* TOKEN_GROUPS */
    {
        TOKEN_GROUPS32 *groups32 = info;
        TOKEN_GROUPS *groups;
        ULONG i, group_len, group32_len;

        status = NtQueryInformationToken( handle, class, NULL, 0, &ret_size );
        if (status != STATUS_BUFFER_TOO_SMALL) return status;
        groups = Wow64AllocateTemp( ret_size );
        status = NtQueryInformationToken( handle, class, groups, ret_size, &ret_size );
        if (status) return status;
        group_len = offsetof( TOKEN_GROUPS, Groups[groups->GroupCount] );
        group32_len = offsetof( TOKEN_GROUPS32, Groups[groups->GroupCount] );
        sid_len = ret_size - group_len;
        ret_size = group32_len + sid_len;
        if (len >= ret_size)
        {
            SID *sid = (SID *)((char *)groups + group_len);
            SID *sid32 = (SID *)((char *)groups32 + group32_len);

            memcpy( sid32, sid, sid_len );
            groups32->GroupCount = groups->GroupCount;
            for (i = 0; i < groups->GroupCount; i++)
            {
                groups32->Groups[i].Sid = PtrToUlong(sid32) + ((char *)groups->Groups[i].Sid - (char *)sid);
                groups32->Groups[i].Attributes = groups->Groups[i].Attributes;
            }
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        if (retlen) *retlen = ret_size;
        return status;
    }

    case TokenDefaultDacl:  /* TOKEN_DEFAULT_DACL + ACL */
    {
        ULONG size = len + sizeof(TOKEN_DEFAULT_DACL) - sizeof(TOKEN_DEFAULT_DACL32);
        TOKEN_DEFAULT_DACL32 *dacl32 = info;
        TOKEN_DEFAULT_DACL *dacl = Wow64AllocateTemp( size );

        status = NtQueryInformationToken( handle, class, dacl, size, &ret_size );
        if (!status)
        {
            dacl32->DefaultDacl = dacl->DefaultDacl ? PtrToUlong( dacl32 + 1 ) : 0;
            memcpy( dacl32 + 1, dacl->DefaultDacl, ret_size - sizeof(*dacl) );
        }
        if (retlen) *retlen = ret_size + sizeof(*dacl32) - sizeof(*dacl);
        return status;
    }

    case TokenLinkedToken:  /* TOKEN_LINKED_TOKEN */
    {
        TOKEN_LINKED_TOKEN link;

        status = NtQueryInformationToken( handle, class, &link, sizeof(link), &ret_size );
        if (!status) *(ULONG *)info = HandleToLong( link.LinkedToken );
        if (retlen) *retlen = sizeof(ULONG);
        return status;
    }

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
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
 *           wow64_NtSetInformationToken
 */
NTSTATUS WINAPI wow64_NtSetInformationToken( UINT *args )
{
    HANDLE handle = get_handle( &args );
    TOKEN_INFORMATION_CLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    switch (class)
    {
    case TokenSessionId:   /* ULONG */
        return NtSetInformationToken( handle, class, ptr, len );

    case TokenDefaultDacl:   /* TOKEN_DEFAULT_DACL */
        if (len >= sizeof(TOKEN_DEFAULT_DACL32))
        {
            TOKEN_DEFAULT_DACL32 *dacl32 = ptr;
            TOKEN_DEFAULT_DACL dacl = { ULongToPtr( dacl32->DefaultDacl ) };

            return NtSetInformationToken( handle, class, &dacl, sizeof(dacl) );
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
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
