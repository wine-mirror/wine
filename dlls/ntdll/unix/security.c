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
        0,    /* TokenLinkedToken */
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
        SERVER_START_REQ( get_token_impersonation_level )
        {
            SECURITY_IMPERSONATION_LEVEL *level = info;
            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS) *level = reply->impersonation_level;
        }
        SERVER_END_REQ;
        break;

    case TokenStatistics:
        SERVER_START_REQ( get_token_statistics )
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
        SERVER_START_REQ( get_token_statistics )
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
        {
            TOKEN_ELEVATION_TYPE *type = info;
            FIXME("QueryInformationToken( ..., TokenElevationType, ...) semi-stub\n");
            *type = TokenElevationTypeFull;
        }
        break;

    case TokenElevation:
        {
            TOKEN_ELEVATION *elevation = info;
            FIXME("QueryInformationToken( ..., TokenElevation, ...) semi-stub\n");
            elevation->TokenIsElevated = TRUE;
        }
        break;

    case TokenSessionId:
        {
            *(DWORD *)info = 0;
            FIXME("QueryInformationToken( ..., TokenSessionId, ...) semi-stub\n");
        }
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
