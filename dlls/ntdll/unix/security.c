/*
 * Security functions
 *
 * Copyright 1996-1998 Marcus Meissner
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


/***********************************************************************
 *             NtOpenProcessToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenProcessToken( HANDLE process, DWORD access, HANDLE *handle )
{
    return NtOpenProcessTokenEx( process, access, 0, handle );
}


/***********************************************************************
 *             NtOpenProcessTokenEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenProcessTokenEx( HANDLE process, DWORD access, DWORD attributes, HANDLE *handle )
{
    NTSTATUS ret;

    TRACE( "(%p,0x%08x,0x%08x,%p)\n", process, access, attributes, handle );

    *handle = 0;

    SERVER_START_REQ( open_token )
    {
        req->handle     = wine_server_obj_handle( process );
        req->access     = access;
        req->attributes = attributes;
        req->flags      = 0;
        ret = wine_server_call( req );
        if (!ret) *handle = wine_server_ptr_handle( reply->token );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *             NtOpenThreadToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenThreadToken( HANDLE thread, DWORD access, BOOLEAN self, HANDLE *handle )
{
    return NtOpenThreadTokenEx( thread, access, self, 0, handle );
}


/***********************************************************************
 *             NtOpenThreadTokenEx  (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenThreadTokenEx( HANDLE thread, DWORD access, BOOLEAN self, DWORD attributes,
                                     HANDLE *handle )
{
    NTSTATUS ret;

    TRACE( "(%p,0x%08x,%u,0x%08x,%p)\n", thread, access, self, attributes, handle );

    *handle = 0;

    SERVER_START_REQ( open_token )
    {
        req->handle     = wine_server_obj_handle( thread );
        req->access     = access;
        req->attributes = attributes;
        req->flags      = OPEN_TOKEN_THREAD;
        if (self) req->flags |= OPEN_TOKEN_AS_SELF;
        ret = wine_server_call( req );
        if (!ret) *handle = wine_server_ptr_handle( reply->token );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *             NtDuplicateToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtDuplicateToken( HANDLE token, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                  SECURITY_IMPERSONATION_LEVEL level, TOKEN_TYPE type, HANDLE *handle )
{
    NTSTATUS status;
    data_size_t len;
    struct object_attributes *objattr;

    *handle = 0;
    if ((status = alloc_object_attributes( attr, &objattr, &len ))) return status;

    if (attr && attr->SecurityQualityOfService)
    {
        SECURITY_QUALITY_OF_SERVICE *qos = attr->SecurityQualityOfService;
        TRACE( "ObjectAttributes->SecurityQualityOfService = {%d, %d, %d, %s}\n",
               qos->Length, qos->ImpersonationLevel, qos->ContextTrackingMode,
               qos->EffectiveOnly ? "TRUE" : "FALSE");
        level = qos->ImpersonationLevel;
    }

    SERVER_START_REQ( duplicate_token )
    {
        req->handle              = wine_server_obj_handle( token );
        req->access              = access;
        req->primary             = (type == TokenPrimary);
        req->impersonation_level = level;
        wine_server_add_data( req, objattr, len );
        status = wine_server_call( req );
        if (!status) *handle = wine_server_ptr_handle( reply->new_handle );
    }
    SERVER_END_REQ;
    free( objattr );
    return status;
}


/***********************************************************************
 *             NtQueryInformationToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationToken( HANDLE token, TOKEN_INFORMATION_CLASS class,
                                         void *info, ULONG length, ULONG *retlen )
{
    static const ULONG info_len [] =
    {
        0,
        0,    /* TokenUser */
        0,    /* TokenGroups */
        0,    /* TokenPrivileges */
        0,    /* TokenOwner */
        0,    /* TokenPrimaryGroup */
        0,    /* TokenDefaultDacl */
        sizeof(TOKEN_SOURCE), /* TokenSource */
        sizeof(TOKEN_TYPE),  /* TokenType */
        sizeof(SECURITY_IMPERSONATION_LEVEL), /* TokenImpersonationLevel */
        sizeof(TOKEN_STATISTICS), /* TokenStatistics */
        0,    /* TokenRestrictedSids */
        sizeof(DWORD), /* TokenSessionId */
        0,    /* TokenGroupsAndPrivileges */
        0,    /* TokenSessionReference */
        0,    /* TokenSandBoxInert */
        0,    /* TokenAuditPolicy */
        0,    /* TokenOrigin */
        sizeof(TOKEN_ELEVATION_TYPE), /* TokenElevationType */
        sizeof(TOKEN_LINKED_TOKEN), /* TokenLinkedToken */
        sizeof(TOKEN_ELEVATION), /* TokenElevation */
        0,    /* TokenHasRestrictions */
        0,    /* TokenAccessInformation */
        0,    /* TokenVirtualizationAllowed */
        sizeof(DWORD), /* TokenVirtualizationEnabled */
        sizeof(TOKEN_MANDATORY_LABEL) + sizeof(SID), /* TokenIntegrityLevel [sizeof(SID) includes one SubAuthority] */
        0,    /* TokenUIAccess */
        0,    /* TokenMandatoryPolicy */
        0,    /* TokenLogonSid */
        sizeof(DWORD), /* TokenIsAppContainer */
        0,    /* TokenCapabilities */
        sizeof(TOKEN_APPCONTAINER_INFORMATION) + sizeof(SID), /* TokenAppContainerSid */
        0,    /* TokenAppContainerNumber */
        0,    /* TokenUserClaimAttributes*/
        0,    /* TokenDeviceClaimAttributes */
        0,    /* TokenRestrictedUserClaimAttributes */
        0,    /* TokenRestrictedDeviceClaimAttributes */
        0,    /* TokenDeviceGroups */
        0,    /* TokenRestrictedDeviceGroups */
        0,    /* TokenSecurityAttributes */
        0,    /* TokenIsRestricted */
        0     /* TokenProcessTrustLevel */
    };

    ULONG len = 0;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "(%p,%d,%p,%d,%p)\n", token, class, info, length, retlen );

    if (class < MaxTokenInfoClass) len = info_len[class];
    if (retlen) *retlen = len;
    if (length < len) return STATUS_BUFFER_TOO_SMALL;

    switch (class)
    {
    case TokenUser:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_USER *tuser = info;
            PSID sid = tuser + 1;
            DWORD sid_len = length < sizeof(TOKEN_USER) ? 0 : length - sizeof(TOKEN_USER);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = class;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->sid_len + sizeof(TOKEN_USER);
            if (status == STATUS_SUCCESS)
            {
                tuser->User.Sid = sid;
                tuser->User.Attributes = 0;
            }
        }
        SERVER_END_REQ;
        break;

    case TokenGroups:
    {
        /* reply buffer is always shorter than output one */
        void *buffer = malloc( length );

        SERVER_START_REQ( get_token_groups )
        {
            TOKEN_GROUPS *groups = info;

            req->handle = wine_server_obj_handle( token );
            wine_server_set_reply( req, buffer, length );
            status = wine_server_call( req );
            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                if (retlen) *retlen = reply->user_len;
            }
            else if (status == STATUS_SUCCESS)
            {
                struct token_groups *tg = buffer;
                unsigned int *attr = (unsigned int *)(tg + 1);
                ULONG i;
                const int non_sid_portion = (sizeof(struct token_groups) + tg->count * sizeof(unsigned int));
                SID *sids = (SID *)((char *)info + FIELD_OFFSET( TOKEN_GROUPS, Groups[tg->count] ));

                if (retlen) *retlen = reply->user_len;

                groups->GroupCount = tg->count;
                memcpy( sids, (char *)buffer + non_sid_portion,
                        reply->user_len - offsetof( TOKEN_GROUPS, Groups[tg->count] ));

                for (i = 0; i < tg->count; i++)
                {
                    groups->Groups[i].Attributes = attr[i];
                    groups->Groups[i].Sid = sids;
                    sids = (SID *)((char *)sids + offsetof( SID, SubAuthority[sids->SubAuthorityCount] ));
                }
             }
             else if (retlen) *retlen = 0;
        }
        SERVER_END_REQ;
        free( buffer );
        break;
    }

    case TokenPrimaryGroup:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_PRIMARY_GROUP *tgroup = info;
            PSID sid = tgroup + 1;
            DWORD sid_len = length < sizeof(TOKEN_PRIMARY_GROUP) ? 0 : length - sizeof(TOKEN_PRIMARY_GROUP);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = class;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->sid_len + sizeof(TOKEN_PRIMARY_GROUP);
            if (status == STATUS_SUCCESS) tgroup->PrimaryGroup = sid;
        }
        SERVER_END_REQ;
        break;

    case TokenPrivileges:
        SERVER_START_REQ( get_token_privileges )
        {
            TOKEN_PRIVILEGES *tpriv = info;
            req->handle = wine_server_obj_handle( token );
            if (tpriv && length > FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ))
                wine_server_set_reply( req, tpriv->Privileges, length - FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) );
            status = wine_server_call( req );
            if (retlen) *retlen = FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) + reply->len;
            if (tpriv) tpriv->PrivilegeCount = reply->len / sizeof(LUID_AND_ATTRIBUTES);
        }
        SERVER_END_REQ;
        break;

    case TokenOwner:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_OWNER *towner = info;
            PSID sid = towner + 1;
            DWORD sid_len = length < sizeof(TOKEN_OWNER) ? 0 : length - sizeof(TOKEN_OWNER);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = class;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->sid_len + sizeof(TOKEN_OWNER);
            if (status == STATUS_SUCCESS) towner->Owner = sid;
        }
        SERVER_END_REQ;
        break;

    case TokenImpersonationLevel:
        SERVER_START_REQ( get_token_info )
        {
            SECURITY_IMPERSONATION_LEVEL *level = info;
            req->handle = wine_server_obj_handle( token );
            if (!(status = wine_server_call( req )))
            {
                if (!reply->primary) *level = reply->impersonation_level;
                else status = STATUS_INVALID_PARAMETER;
            }
        }
        SERVER_END_REQ;
        break;

    case TokenStatistics:
        SERVER_START_REQ( get_token_info )
        {
            TOKEN_STATISTICS *statistics = info;
            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                statistics->TokenId.LowPart  = reply->token_id.low_part;
                statistics->TokenId.HighPart = reply->token_id.high_part;
                statistics->AuthenticationId.LowPart  = 0; /* FIXME */
                statistics->AuthenticationId.HighPart = 0; /* FIXME */
                statistics->ExpirationTime.u.HighPart = 0x7fffffff;
                statistics->ExpirationTime.u.LowPart  = 0xffffffff;
                statistics->TokenType = reply->primary ? TokenPrimary : TokenImpersonation;
                statistics->ImpersonationLevel = reply->impersonation_level;

                /* kernel information not relevant to us */
                statistics->DynamicCharged = 0;
                statistics->DynamicAvailable = 0;

                statistics->GroupCount = reply->group_count;
                statistics->PrivilegeCount = reply->privilege_count;
                statistics->ModifiedId.LowPart  = reply->modified_id.low_part;
                statistics->ModifiedId.HighPart = reply->modified_id.high_part;
            }
        }
        SERVER_END_REQ;
        break;

    case TokenType:
        SERVER_START_REQ( get_token_info )
        {
            TOKEN_TYPE *type = info;
            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS) *type = reply->primary ? TokenPrimary : TokenImpersonation;
        }
        SERVER_END_REQ;
        break;

    case TokenDefaultDacl:
        SERVER_START_REQ( get_token_default_dacl )
        {
            TOKEN_DEFAULT_DACL *default_dacl = info;
            ACL *acl = (ACL *)(default_dacl + 1);
            DWORD acl_len = length < sizeof(TOKEN_DEFAULT_DACL) ? 0 : length - sizeof(TOKEN_DEFAULT_DACL);

            req->handle = wine_server_obj_handle( token );
            wine_server_set_reply( req, acl, acl_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->acl_len + sizeof(TOKEN_DEFAULT_DACL);
            if (status == STATUS_SUCCESS)
            {
                if (reply->acl_len)
                    default_dacl->DefaultDacl = acl;
                else
                    default_dacl->DefaultDacl = NULL;
            }
        }
        SERVER_END_REQ;
        break;

    case TokenElevationType:
        SERVER_START_REQ( get_token_info )
        {
            TOKEN_ELEVATION_TYPE *type = info;

            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (!status) *type = reply->elevation;
        }
        SERVER_END_REQ;
        break;

    case TokenElevation:
        SERVER_START_REQ( get_token_info )
        {
            TOKEN_ELEVATION *elevation = info;

            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (!status) elevation->TokenIsElevated = (reply->elevation == TokenElevationTypeFull);
        }
        SERVER_END_REQ;
        break;

    case TokenSessionId:
        SERVER_START_REQ( get_token_info )
        {
            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (!status) *(DWORD *)info = reply->session_id;
        }
        SERVER_END_REQ;
        break;

    case TokenVirtualizationEnabled:
        {
            *(DWORD *)info = 0;
            TRACE("QueryInformationToken( ..., TokenVirtualizationEnabled, ...) semi-stub\n");
        }
        break;

    case TokenIntegrityLevel:
        {
            /* report always "S-1-16-12288" (high mandatory level) for now */
            static const SID high_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                                                            {SECURITY_MANDATORY_HIGH_RID}};

            TOKEN_MANDATORY_LABEL *tml = info;
            PSID psid = tml + 1;

            tml->Label.Sid = psid;
            tml->Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
            memcpy( psid, &high_level, sizeof(SID) );
        }
        break;

    case TokenAppContainerSid:
        {
            TOKEN_APPCONTAINER_INFORMATION *container = info;
            FIXME("QueryInformationToken( ..., TokenAppContainerSid, ...) semi-stub\n");
            container->TokenAppContainer = NULL;
        }
        break;

    case TokenIsAppContainer:
        {
            TRACE("TokenIsAppContainer semi-stub\n");
            *(DWORD *)info = 0;
            break;
        }

    case TokenLogonSid:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_GROUPS * groups = info;
            PSID sid = groups + 1;
            DWORD sid_len = length < sizeof(TOKEN_GROUPS) ? 0 : length - sizeof(TOKEN_GROUPS);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = class;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->sid_len + sizeof(TOKEN_GROUPS);
            if (status == STATUS_SUCCESS)
            {
                groups->GroupCount = 1;
                groups->Groups[0].Sid = sid;
                groups->Groups[0].Attributes = 0;
            }
        }
        SERVER_END_REQ;
        break;

    case TokenLinkedToken:
        SERVER_START_REQ( create_linked_token )
        {
            TOKEN_LINKED_TOKEN *linked = info;

            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (!status) linked->LinkedToken = wine_server_ptr_handle( reply->linked );
        }
        SERVER_END_REQ;
        break;

    default:
        ERR( "Unhandled token information class %u\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
    return status;
}


/***********************************************************************
 *             NtSetInformationToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationToken( HANDLE token, TOKEN_INFORMATION_CLASS class,
                                       void *info, ULONG length )
{
    NTSTATUS ret = STATUS_NOT_IMPLEMENTED;

    TRACE( "%p %d %p %u\n", token, class, info, length );

    switch (class)
    {
    case TokenDefaultDacl:
        if (length < sizeof(TOKEN_DEFAULT_DACL))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if (!info)
        {
            ret = STATUS_ACCESS_VIOLATION;
            break;
        }
        SERVER_START_REQ( set_token_default_dacl )
        {
            ACL *acl = ((TOKEN_DEFAULT_DACL *)info)->DefaultDacl;
            WORD size;

            if (acl) size = acl->AclSize;
            else size = 0;
            req->handle = wine_server_obj_handle( token );
            wine_server_add_data( req, acl, size );
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        break;

    case TokenSessionId:
        if (length < sizeof(DWORD))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if (!info)
        {
            ret = STATUS_ACCESS_VIOLATION;
            break;
        }
        FIXME("TokenSessionId stub!\n");
        ret = STATUS_SUCCESS;
        break;

    case TokenIntegrityLevel:
        FIXME( "TokenIntegrityLevel stub!\n" );
        ret = STATUS_SUCCESS;
        break;

    default:
        FIXME( "unimplemented class %u\n", class );
        break;
    }
    return ret;
}


/***********************************************************************
 *             NtCreateLowBoxToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateLowBoxToken( HANDLE *token_handle, HANDLE token, ACCESS_MASK access,
                                     OBJECT_ATTRIBUTES *attr, SID *sid, ULONG count,
                                     SID_AND_ATTRIBUTES *capabilities, ULONG handle_count, HANDLE *handle )
{
    FIXME("(%p, %p, %x, %p, %p, %u, %p, %u, %p): stub\n",
          token_handle, token, access, attr, sid, count, capabilities, handle_count, handle );

    /* we need to return a NULL handle since later it will be passed to NtClose and that must not fail */
    *token_handle = NULL;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *             NtAdjustGroupsToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtAdjustGroupsToken( HANDLE token, BOOLEAN reset, TOKEN_GROUPS *groups,
                                     ULONG length, TOKEN_GROUPS *prev, ULONG *retlen )
{
    FIXME( "%p %d %p %u %p %p\n", token, reset, groups, length, prev, retlen );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *             NtAdjustPrivilegesToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtAdjustPrivilegesToken( HANDLE token, BOOLEAN disable, TOKEN_PRIVILEGES *privs,
                                         DWORD length, TOKEN_PRIVILEGES *prev, DWORD *retlen )
{
    NTSTATUS ret;

    TRACE( "(%p,0x%08x,%p,0x%08x,%p,%p)\n", token, disable, privs, length, prev, retlen );

    SERVER_START_REQ( adjust_token_privileges )
    {
        req->handle = wine_server_obj_handle( token );
        req->disable_all = disable;
        req->get_modified_state = (prev != NULL);
        if (!disable)
            wine_server_add_data( req, privs->Privileges,
                                  privs->PrivilegeCount * sizeof(privs->Privileges[0]) );
        if (prev && length >= FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ))
            wine_server_set_reply( req, prev->Privileges,
                                   length - FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) );
        ret = wine_server_call( req );
        if (prev)
        {
            if (retlen) *retlen = reply->len + FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges );
            prev->PrivilegeCount = reply->len / sizeof(LUID_AND_ATTRIBUTES);
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *             NtFilterToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtFilterToken( HANDLE token, ULONG flags, TOKEN_GROUPS *disable_sids,
                               TOKEN_PRIVILEGES *privileges, TOKEN_GROUPS *restrict_sids, HANDLE *new_token )
{
    data_size_t privileges_len = 0;
    data_size_t sids_len = 0;
    SID *sids = NULL;
    NTSTATUS status;

    TRACE( "%p %#x %p %p %p %p\n", token, flags, disable_sids, privileges,
           restrict_sids, new_token );

    if (flags)
        FIXME( "flags %#x unsupported\n", flags );

    if (restrict_sids)
        FIXME( "support for restricting sids not yet implemented\n" );

    if (privileges)
        privileges_len = privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);

    if (disable_sids)
    {
        DWORD len, i;
        BYTE *tmp;

        for (i = 0; i < disable_sids->GroupCount; i++)
        {
            SID *sid = disable_sids->Groups[i].Sid;
            sids_len += offsetof( SID, SubAuthority[sid->SubAuthorityCount] );
        }

        sids = malloc( sids_len );
        if (!sids) return STATUS_NO_MEMORY;

        for (i = 0, tmp = (BYTE *)sids; i < disable_sids->GroupCount; i++, tmp += len)
        {
            SID *sid = disable_sids->Groups[i].Sid;
            len = offsetof( SID, SubAuthority[sid->SubAuthorityCount] );
            memcpy( tmp, disable_sids->Groups[i].Sid, len );
        }
    }

    SERVER_START_REQ( filter_token )
    {
        req->handle          = wine_server_obj_handle( token );
        req->flags           = flags;
        req->privileges_size = privileges_len;
        wine_server_add_data( req, privileges->Privileges, privileges_len );
        wine_server_add_data( req, sids, sids_len );
        status = wine_server_call( req );
        if (!status) *new_token = wine_server_ptr_handle( reply->new_handle );
    }
    SERVER_END_REQ;

    free( sids );
    return status;
}



/***********************************************************************
 *             NtPrivilegeCheck  (NTDLL.@)
 */
NTSTATUS WINAPI NtPrivilegeCheck( HANDLE token, PRIVILEGE_SET *privs, BOOLEAN *res )
{
    NTSTATUS status;

    SERVER_START_REQ( check_token_privileges )
    {
        req->handle = wine_server_obj_handle( token );
        req->all_required = (privs->Control & PRIVILEGE_SET_ALL_NECESSARY) != 0;
        wine_server_add_data( req, privs->Privilege, privs->PrivilegeCount * sizeof(privs->Privilege[0]) );
        wine_server_set_reply( req, privs->Privilege, privs->PrivilegeCount * sizeof(privs->Privilege[0]) );
        status = wine_server_call( req );
        if (status == STATUS_SUCCESS) *res = reply->has_privileges != 0;
    }
    SERVER_END_REQ;
    return status;
}


/***********************************************************************
 *             NtImpersonateAnonymousToken  (NTDLL.@)
 */
NTSTATUS WINAPI NtImpersonateAnonymousToken( HANDLE thread )
{
    FIXME( "(%p): stub\n", thread );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *             NtAccessCheck  (NTDLL.@)
 */
NTSTATUS WINAPI NtAccessCheck( PSECURITY_DESCRIPTOR descr, HANDLE token, ACCESS_MASK access,
                               GENERIC_MAPPING *mapping, PRIVILEGE_SET *privs, ULONG *retlen,
                               ULONG *access_granted, NTSTATUS *access_status)
{
    struct object_attributes *objattr;
    data_size_t len;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG priv_len;

    TRACE( "(%p, %p, %08x, %p, %p, %p, %p, %p)\n",
           descr, token, access, mapping, privs, retlen, access_granted, access_status );

    if (!privs || !retlen) return STATUS_ACCESS_VIOLATION;
    priv_len = *retlen;

    /* reuse the object attribute SD marshalling */
    InitializeObjectAttributes( &attr, NULL, 0, 0, descr );
    if ((status = alloc_object_attributes( &attr, &objattr, &len ))) return status;

    SERVER_START_REQ( access_check )
    {
        req->handle = wine_server_obj_handle( token );
        req->desired_access = access;
        req->mapping.read = mapping->GenericRead;
        req->mapping.write = mapping->GenericWrite;
        req->mapping.exec = mapping->GenericExecute;
        req->mapping.all = mapping->GenericAll;
        wine_server_add_data( req, objattr + 1, objattr->sd_len );
        wine_server_set_reply( req, privs->Privilege, priv_len - offsetof( PRIVILEGE_SET, Privilege ) );

        status = wine_server_call( req );

        if (status == STATUS_SUCCESS)
        {
            *retlen = max( offsetof( PRIVILEGE_SET, Privilege ) + reply->privileges_len, sizeof(PRIVILEGE_SET) );
            if (priv_len >= *retlen)
            {
                privs->PrivilegeCount = reply->privileges_len / sizeof(LUID_AND_ATTRIBUTES);
                *access_status = reply->access_status;
                *access_granted = reply->access_granted;
            }
            else status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    SERVER_END_REQ;
    free( objattr );
    return status;
}


/***********************************************************************
 *             NtAccessCheckAndAuditAlarm  (NTDLL.@)
 */
NTSTATUS WINAPI NtAccessCheckAndAuditAlarm( UNICODE_STRING *subsystem, HANDLE handle,
                                            UNICODE_STRING *typename, UNICODE_STRING *objectname,
                                            PSECURITY_DESCRIPTOR descr, ACCESS_MASK access,
                                            GENERIC_MAPPING *mapping, BOOLEAN creation,
                                            ACCESS_MASK *access_granted, BOOLEAN *access_status,
                                            BOOLEAN *onclose )
{
    FIXME( "(%s, %p, %s, %p, 0x%08x, %p, %d, %p, %p, %p), stub\n",
           debugstr_us(subsystem), handle, debugstr_us(typename), descr, access,
           mapping, creation, access_granted, access_status, onclose );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *             NtQuerySecurityObject  (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySecurityObject( HANDLE handle, SECURITY_INFORMATION info,
                                       PSECURITY_DESCRIPTOR descr, ULONG length, ULONG *retlen )
{
    SECURITY_DESCRIPTOR_RELATIVE *psd = descr;
    NTSTATUS status;
    void *buffer;
    unsigned int buffer_size = 512;

    TRACE( "(%p,0x%08x,%p,0x%08x,%p)\n", handle, info, descr, length, retlen );

    for (;;)
    {
        if (!(buffer = malloc( buffer_size ))) return STATUS_NO_MEMORY;

        SERVER_START_REQ( get_security_object )
        {
            req->handle = wine_server_obj_handle( handle );
            req->security_info = info;
            wine_server_set_reply( req, buffer, buffer_size );
            status = wine_server_call( req );
            buffer_size = reply->sd_len;
        }
        SERVER_END_REQ;

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            free( buffer );
            continue;
        }
        if (status == STATUS_SUCCESS)
        {
            struct security_descriptor *sd = buffer;

            if (!buffer_size) memset( sd, 0, sizeof(*sd) );
            *retlen = sizeof(*psd) + sd->owner_len + sd->group_len + sd->sacl_len + sd->dacl_len;
            if (length >= *retlen)
            {
                DWORD len = sizeof(*psd);
                memset( psd, 0, len );
                psd->Revision = SECURITY_DESCRIPTOR_REVISION;
                psd->Control = sd->control | SE_SELF_RELATIVE;
                if (sd->owner_len) { psd->Owner = len; len += sd->owner_len; }
                if (sd->group_len) { psd->Group = len; len += sd->group_len; }
                if (sd->sacl_len) { psd->Sacl = len; len += sd->sacl_len; }
                if (sd->dacl_len) { psd->Dacl = len; len += sd->dacl_len; }
                /* owner, group, sacl and dacl are the same type as in the server
                 * and in the same order so we copy the memory in one block */
                memcpy( psd + 1, sd + 1, len - sizeof(*psd) );
            }
            else status = STATUS_BUFFER_TOO_SMALL;
        }
        free( buffer );
        return status;
    }
}


/***********************************************************************
 *             NtSetSecurityObject  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetSecurityObject( HANDLE handle, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr )
{
    struct object_attributes *objattr;
    struct security_descriptor *sd;
    data_size_t len;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE( "%p 0x%08x %p\n", handle, info, descr );

    if (!descr) return STATUS_ACCESS_VIOLATION;

    /* reuse the object attribute SD marshalling */
    InitializeObjectAttributes( &attr, NULL, 0, 0, descr );
    if ((status = alloc_object_attributes( &attr, &objattr, &len ))) return status;
    sd = (struct security_descriptor *)(objattr + 1);
    if (info & OWNER_SECURITY_INFORMATION && !sd->owner_len)
    {
        free( objattr );
        return STATUS_INVALID_SECURITY_DESCR;
    }
    if (info & GROUP_SECURITY_INFORMATION && !sd->group_len)
    {
        free( objattr );
        return STATUS_INVALID_SECURITY_DESCR;
    }
    if (info & (SACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION)) sd->control |= SE_SACL_PRESENT;
    if (info & DACL_SECURITY_INFORMATION) sd->control |= SE_DACL_PRESENT;

    SERVER_START_REQ( set_security_object )
    {
        req->handle = wine_server_obj_handle( handle );
        req->security_info = info;
        wine_server_add_data( req, sd, objattr->sd_len );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    free( objattr );
    return status;
}


/***********************************************************************
 *             NtAllocateLocallyUniqueId  (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateLocallyUniqueId( LUID *luid )
{
    NTSTATUS status;

    TRACE( "%p\n", luid );

    if (!luid) return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ( allocate_locally_unique_id )
    {
        status = wine_server_call( req );
        if (!status)
        {
            luid->LowPart = reply->luid.low_part;
            luid->HighPart = reply->luid.high_part;
        }
    }
    SERVER_END_REQ;
    return status;
}


/***********************************************************************
 *             NtAllocateUuids  (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateUuids( ULARGE_INTEGER *time, ULONG *delta, ULONG *sequence, UCHAR *seed )
{
    FIXME( "(%p,%p,%p,%p), stub.\n", time, delta, sequence, seed );
    return STATUS_SUCCESS;
}
