/*
 * NT basis DLL
 *
 * This file contains the Nt* API functions of NTDLL.DLL.
 * In the original ntdll.dll they all seem to just call int 0x2e (down to the NTOSKRNL)
 *
 * Copyright 1996-1998 Marcus Meissner
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"
#include "ddk/wdm.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

/*
 *	Token
 */

/******************************************************************************
 *  NtDuplicateToken		[NTDLL.@]
 *  ZwDuplicateToken		[NTDLL.@]
 */
NTSTATUS WINAPI NtDuplicateToken(
        IN HANDLE ExistingToken,
        IN ACCESS_MASK DesiredAccess,
        IN POBJECT_ATTRIBUTES ObjectAttributes,
        IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
        IN TOKEN_TYPE TokenType,
        OUT PHANDLE NewToken)
{
    NTSTATUS status;
    data_size_t len;
    struct object_attributes *objattr;

    TRACE("(%p,0x%08x,%s,0x%08x,0x%08x,%p)\n",
          ExistingToken, DesiredAccess, debugstr_ObjectAttributes(ObjectAttributes),
          ImpersonationLevel, TokenType, NewToken);

    if ((status = alloc_object_attributes( ObjectAttributes, &objattr, &len ))) return status;

    if (ObjectAttributes && ObjectAttributes->SecurityQualityOfService)
    {
        SECURITY_QUALITY_OF_SERVICE *SecurityQOS = ObjectAttributes->SecurityQualityOfService;
        TRACE("ObjectAttributes->SecurityQualityOfService = {%d, %d, %d, %s}\n",
            SecurityQOS->Length, SecurityQOS->ImpersonationLevel,
            SecurityQOS->ContextTrackingMode,
            SecurityQOS->EffectiveOnly ? "TRUE" : "FALSE");
        ImpersonationLevel = SecurityQOS->ImpersonationLevel;
    }

    SERVER_START_REQ( duplicate_token )
    {
        req->handle              = wine_server_obj_handle( ExistingToken );
        req->access              = DesiredAccess;
        req->primary             = (TokenType == TokenPrimary);
        req->impersonation_level = ImpersonationLevel;
        wine_server_add_data( req, objattr, len );
        status = wine_server_call( req );
        if (!status) *NewToken = wine_server_ptr_handle( reply->new_handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return status;
}

/******************************************************************************
 *  NtOpenProcessToken		[NTDLL.@]
 *  ZwOpenProcessToken		[NTDLL.@]
 */
NTSTATUS WINAPI NtOpenProcessToken(
	HANDLE ProcessHandle,
	DWORD DesiredAccess,
	HANDLE *TokenHandle)
{
    return NtOpenProcessTokenEx( ProcessHandle, DesiredAccess, 0, TokenHandle );
}

/******************************************************************************
 *  NtOpenProcessTokenEx   [NTDLL.@]
 *  ZwOpenProcessTokenEx   [NTDLL.@]
 */
NTSTATUS WINAPI NtOpenProcessTokenEx( HANDLE process, DWORD access, DWORD attributes,
                                      HANDLE *handle )
{
    NTSTATUS ret;

    TRACE("(%p,0x%08x,0x%08x,%p)\n", process, access, attributes, handle);

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

/******************************************************************************
 *  NtOpenThreadToken		[NTDLL.@]
 *  ZwOpenThreadToken		[NTDLL.@]
 */
NTSTATUS WINAPI NtOpenThreadToken(
	HANDLE ThreadHandle,
	DWORD DesiredAccess,
	BOOLEAN OpenAsSelf,
	HANDLE *TokenHandle)
{
    return NtOpenThreadTokenEx( ThreadHandle, DesiredAccess, OpenAsSelf, 0, TokenHandle );
}

/******************************************************************************
 *  NtOpenThreadTokenEx   [NTDLL.@]
 *  ZwOpenThreadTokenEx   [NTDLL.@]
 */
NTSTATUS WINAPI NtOpenThreadTokenEx( HANDLE thread, DWORD access, BOOLEAN as_self, DWORD attributes,
                                     HANDLE *handle )
{
    NTSTATUS ret;

    TRACE("(%p,0x%08x,%u,0x%08x,%p)\n", thread, access, as_self, attributes, handle );

    SERVER_START_REQ( open_token )
    {
        req->handle     = wine_server_obj_handle( thread );
        req->access     = access;
        req->attributes = attributes;
        req->flags      = OPEN_TOKEN_THREAD;
        if (as_self) req->flags |= OPEN_TOKEN_AS_SELF;
        ret = wine_server_call( req );
        if (!ret) *handle = wine_server_ptr_handle( reply->token );
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
 *  NtAdjustPrivilegesToken		[NTDLL.@]
 *  ZwAdjustPrivilegesToken		[NTDLL.@]
 *
 * FIXME: parameters unsafe
 */
NTSTATUS WINAPI NtAdjustPrivilegesToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES NewState,
	IN DWORD BufferLength,
	OUT PTOKEN_PRIVILEGES PreviousState,
	OUT PDWORD ReturnLength)
{
    NTSTATUS ret;

    TRACE("(%p,0x%08x,%p,0x%08x,%p,%p)\n",
        TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);

    SERVER_START_REQ( adjust_token_privileges )
    {
        req->handle = wine_server_obj_handle( TokenHandle );
        req->disable_all = DisableAllPrivileges;
        req->get_modified_state = (PreviousState != NULL);
        if (!DisableAllPrivileges)
        {
            wine_server_add_data( req, NewState->Privileges,
                                  NewState->PrivilegeCount * sizeof(NewState->Privileges[0]) );
        }
        if (PreviousState && BufferLength >= FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ))
            wine_server_set_reply( req, PreviousState->Privileges,
                                   BufferLength - FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) );
        ret = wine_server_call( req );
        if (PreviousState)
        {
            if (ReturnLength) *ReturnLength = reply->len + FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges );
            PreviousState->PrivilegeCount = reply->len / sizeof(LUID_AND_ATTRIBUTES);
        }
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
*  NtQueryInformationToken		[NTDLL.@]
*  ZwQueryInformationToken		[NTDLL.@]
*
* NOTES
*  Buffer for TokenUser:
*   0x00 TOKEN_USER the PSID field points to the SID
*   0x08 SID
*
*/
NTSTATUS WINAPI NtQueryInformationToken(
	HANDLE token,
	TOKEN_INFORMATION_CLASS tokeninfoclass,
	PVOID tokeninfo,
	ULONG tokeninfolength,
	PULONG retlen )
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

    TRACE("(%p,%d,%p,%d,%p)\n",
          token,tokeninfoclass,tokeninfo,tokeninfolength,retlen);

    if (tokeninfoclass < MaxTokenInfoClass)
        len = info_len[tokeninfoclass];

    if (retlen) *retlen = len;

    if (tokeninfolength < len)
        return STATUS_BUFFER_TOO_SMALL;

    switch (tokeninfoclass)
    {
    case TokenUser:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_USER * tuser = tokeninfo;
            PSID sid = tuser + 1;
            DWORD sid_len = tokeninfolength < sizeof(TOKEN_USER) ? 0 : tokeninfolength - sizeof(TOKEN_USER);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = tokeninfoclass;
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
        void *buffer;

        /* reply buffer is always shorter than output one */
        buffer = tokeninfolength ? RtlAllocateHeap(GetProcessHeap(), 0, tokeninfolength) : NULL;

        SERVER_START_REQ( get_token_groups )
        {
            TOKEN_GROUPS *groups = tokeninfo;

            req->handle = wine_server_obj_handle( token );
            wine_server_set_reply( req, buffer, tokeninfolength );
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
                SID *sids = (SID *)((char *)tokeninfo + FIELD_OFFSET( TOKEN_GROUPS, Groups[tg->count] ));

                if (retlen) *retlen = reply->user_len;

                groups->GroupCount = tg->count;
                memcpy( sids, (char *)buffer + non_sid_portion,
                        reply->user_len - FIELD_OFFSET( TOKEN_GROUPS, Groups[tg->count] ));

                for (i = 0; i < tg->count; i++)
                {
                    groups->Groups[i].Attributes = attr[i];
                    groups->Groups[i].Sid = sids;
                    sids = (SID *)((char *)sids + RtlLengthSid(sids));
                }
             }
             else if (retlen) *retlen = 0;
        }
        SERVER_END_REQ;

        RtlFreeHeap(GetProcessHeap(), 0, buffer);
        break;
    }
    case TokenPrimaryGroup:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_PRIMARY_GROUP *tgroup = tokeninfo;
            PSID sid = tgroup + 1;
            DWORD sid_len = tokeninfolength < sizeof(TOKEN_PRIMARY_GROUP) ? 0 : tokeninfolength - sizeof(TOKEN_PRIMARY_GROUP);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = tokeninfoclass;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->sid_len + sizeof(TOKEN_PRIMARY_GROUP);
            if (status == STATUS_SUCCESS)
                tgroup->PrimaryGroup = sid;
        }
        SERVER_END_REQ;
        break;
    case TokenPrivileges:
        SERVER_START_REQ( get_token_privileges )
        {
            TOKEN_PRIVILEGES *tpriv = tokeninfo;
            req->handle = wine_server_obj_handle( token );
            if (tpriv && tokeninfolength > FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ))
                wine_server_set_reply( req, tpriv->Privileges, tokeninfolength - FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) );
            status = wine_server_call( req );
            if (retlen) *retlen = FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) + reply->len;
            if (tpriv) tpriv->PrivilegeCount = reply->len / sizeof(LUID_AND_ATTRIBUTES);
        }
        SERVER_END_REQ;
        break;
    case TokenOwner:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_OWNER *towner = tokeninfo;
            PSID sid = towner + 1;
            DWORD sid_len = tokeninfolength < sizeof(TOKEN_OWNER) ? 0 : tokeninfolength - sizeof(TOKEN_OWNER);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = tokeninfoclass;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->sid_len + sizeof(TOKEN_OWNER);
            if (status == STATUS_SUCCESS)
                towner->Owner = sid;
        }
        SERVER_END_REQ;
        break;
    case TokenImpersonationLevel:
        SERVER_START_REQ( get_token_impersonation_level )
        {
            SECURITY_IMPERSONATION_LEVEL *impersonation_level = tokeninfo;
            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
                *impersonation_level = reply->impersonation_level;
        }
        SERVER_END_REQ;
        break;
    case TokenStatistics:
        SERVER_START_REQ( get_token_statistics )
        {
            TOKEN_STATISTICS *statistics = tokeninfo;
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
            TOKEN_TYPE *token_type = tokeninfo;
            req->handle = wine_server_obj_handle( token );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
                *token_type = reply->primary ? TokenPrimary : TokenImpersonation;
        }
        SERVER_END_REQ;
        break;
    case TokenDefaultDacl:
        SERVER_START_REQ( get_token_default_dacl )
        {
            TOKEN_DEFAULT_DACL *default_dacl = tokeninfo;
            ACL *acl = (ACL *)(default_dacl + 1);
            DWORD acl_len;

            if (tokeninfolength < sizeof(TOKEN_DEFAULT_DACL)) acl_len = 0;
            else acl_len = tokeninfolength - sizeof(TOKEN_DEFAULT_DACL);

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
            TOKEN_ELEVATION_TYPE *elevation_type = tokeninfo;
            FIXME("QueryInformationToken( ..., TokenElevationType, ...) semi-stub\n");
            *elevation_type = TokenElevationTypeFull;
        }
        break;
    case TokenElevation:
        {
            TOKEN_ELEVATION *elevation = tokeninfo;
            FIXME("QueryInformationToken( ..., TokenElevation, ...) semi-stub\n");
            elevation->TokenIsElevated = TRUE;
        }
        break;
    case TokenSessionId:
        {
            *((DWORD*)tokeninfo) = 0;
            FIXME("QueryInformationToken( ..., TokenSessionId, ...) semi-stub\n");
        }
        break;
    case TokenVirtualizationEnabled:
        {
            *(DWORD *)tokeninfo = 0;
            TRACE("QueryInformationToken( ..., TokenVirtualizationEnabled, ...) semi-stub\n");
        }
        break;
    case TokenIntegrityLevel:
        {
            /* report always "S-1-16-12288" (high mandatory level) for now */
            static const SID high_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                                                            {SECURITY_MANDATORY_HIGH_RID}};

            TOKEN_MANDATORY_LABEL *tml = tokeninfo;
            PSID psid = tml + 1;

            tml->Label.Sid = psid;
            tml->Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
            memcpy(psid, &high_level, sizeof(SID));
        }
        break;
    case TokenAppContainerSid:
        {
            TOKEN_APPCONTAINER_INFORMATION *container = tokeninfo;
            FIXME("QueryInformationToken( ..., TokenAppContainerSid, ...) semi-stub\n");
            container->TokenAppContainer = NULL;
        }
        break;
    case TokenIsAppContainer:
        {
            TRACE("TokenIsAppContainer semi-stub\n");
            *(DWORD*)tokeninfo = 0;
            break;
        }
    case TokenLogonSid:
        SERVER_START_REQ( get_token_sid )
        {
            TOKEN_GROUPS * groups = tokeninfo;
            PSID sid = groups + 1;
            DWORD sid_len = tokeninfolength < sizeof(TOKEN_GROUPS) ? 0 : tokeninfolength - sizeof(TOKEN_GROUPS);

            req->handle = wine_server_obj_handle( token );
            req->which_sid = tokeninfoclass;
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
        {
            ERR("Unhandled Token Information class %d!\n", tokeninfoclass);
            return STATUS_NOT_IMPLEMENTED;
        }
    }
    return status;
}

/******************************************************************************
*  NtSetInformationToken		[NTDLL.@]
*  ZwSetInformationToken		[NTDLL.@]
*/
NTSTATUS WINAPI NtSetInformationToken(
        HANDLE TokenHandle,
        TOKEN_INFORMATION_CLASS TokenInformationClass,
        PVOID TokenInformation,
        ULONG TokenInformationLength)
{
    NTSTATUS ret = STATUS_NOT_IMPLEMENTED;

    TRACE("%p %d %p %u\n", TokenHandle, TokenInformationClass,
           TokenInformation, TokenInformationLength);

    switch (TokenInformationClass)
    {
    case TokenDefaultDacl:
        if (TokenInformationLength < sizeof(TOKEN_DEFAULT_DACL))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if (!TokenInformation)
        {
            ret = STATUS_ACCESS_VIOLATION;
            break;
        }
        SERVER_START_REQ( set_token_default_dacl )
        {
            ACL *acl = ((TOKEN_DEFAULT_DACL *)TokenInformation)->DefaultDacl;
            WORD size;

            if (acl) size = acl->AclSize;
            else size = 0;

            req->handle = wine_server_obj_handle( TokenHandle );
            wine_server_add_data( req, acl, size );
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        break;
    case TokenSessionId:
        if (TokenInformationLength < sizeof(DWORD))
        {
            ret = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if (!TokenInformation)
        {
            ret = STATUS_ACCESS_VIOLATION;
            break;
        }
        FIXME("TokenSessionId stub!\n");
        ret = STATUS_SUCCESS;
        break;
    case TokenIntegrityLevel:
        FIXME("TokenIntegrityLevel stub!\n");
        ret = STATUS_SUCCESS;
        break;
    default:
        FIXME("unimplemented class %u\n", TokenInformationClass);
        break;
    }

    return ret;
}

/******************************************************************************
*  NtAdjustGroupsToken		[NTDLL.@]
*  ZwAdjustGroupsToken		[NTDLL.@]
*/
NTSTATUS WINAPI NtAdjustGroupsToken(
        HANDLE TokenHandle,
        BOOLEAN ResetToDefault,
        PTOKEN_GROUPS NewState,
        ULONG BufferLength,
        PTOKEN_GROUPS PreviousState,
        PULONG ReturnLength)
{
    FIXME("%p %d %p %u %p %p\n", TokenHandle, ResetToDefault,
          NewState, BufferLength, PreviousState, ReturnLength);
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
*  NtPrivilegeCheck		[NTDLL.@]
*  ZwPrivilegeCheck		[NTDLL.@]
*/
NTSTATUS WINAPI NtPrivilegeCheck(
    HANDLE ClientToken,
    PPRIVILEGE_SET RequiredPrivileges,
    PBOOLEAN Result)
{
    NTSTATUS status;
    SERVER_START_REQ( check_token_privileges )
    {
        req->handle = wine_server_obj_handle( ClientToken );
        req->all_required = (RequiredPrivileges->Control & PRIVILEGE_SET_ALL_NECESSARY) != 0;
        wine_server_add_data( req, RequiredPrivileges->Privilege,
            RequiredPrivileges->PrivilegeCount * sizeof(RequiredPrivileges->Privilege[0]) );
        wine_server_set_reply( req, RequiredPrivileges->Privilege,
            RequiredPrivileges->PrivilegeCount * sizeof(RequiredPrivileges->Privilege[0]) );

        status = wine_server_call( req );

        if (status == STATUS_SUCCESS)
            *Result = reply->has_privileges != 0;
    }
    SERVER_END_REQ;
    return status;
}

/*
 *	ports
 */

/******************************************************************************
 *  NtCreatePort		[NTDLL.@]
 *  ZwCreatePort		[NTDLL.@]
 */
NTSTATUS WINAPI NtCreatePort(PHANDLE PortHandle,POBJECT_ATTRIBUTES ObjectAttributes,
                             ULONG MaxConnectInfoLength,ULONG MaxDataLength,PULONG reserved)
{
  FIXME("(%p,%p,%u,%u,%p),stub!\n",PortHandle,ObjectAttributes,
        MaxConnectInfoLength,MaxDataLength,reserved);
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtConnectPort		[NTDLL.@]
 *  ZwConnectPort		[NTDLL.@]
 */
NTSTATUS WINAPI NtConnectPort(
        PHANDLE PortHandle,
        PUNICODE_STRING PortName,
        PSECURITY_QUALITY_OF_SERVICE SecurityQos,
        PLPC_SECTION_WRITE WriteSection,
        PLPC_SECTION_READ ReadSection,
        PULONG MaximumMessageLength,
        PVOID ConnectInfo,
        PULONG pConnectInfoLength)
{
    FIXME("(%p,%s,%p,%p,%p,%p,%p,%p),stub!\n",
          PortHandle,debugstr_w(PortName->Buffer),SecurityQos,
          WriteSection,ReadSection,MaximumMessageLength,ConnectInfo,
          pConnectInfoLength);
    if (ConnectInfo && pConnectInfoLength)
        TRACE("\tMessage = %s\n",debugstr_an(ConnectInfo,*pConnectInfoLength));
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtSecureConnectPort                (NTDLL.@)
 *  ZwSecureConnectPort                (NTDLL.@)
 */
NTSTATUS WINAPI NtSecureConnectPort(
        PHANDLE PortHandle,
        PUNICODE_STRING PortName,
        PSECURITY_QUALITY_OF_SERVICE SecurityQos,
        PLPC_SECTION_WRITE WriteSection,
        PSID pSid,
        PLPC_SECTION_READ ReadSection,
        PULONG MaximumMessageLength,
        PVOID ConnectInfo,
        PULONG pConnectInfoLength)
{
    FIXME("(%p,%s,%p,%p,%p,%p,%p,%p,%p),stub!\n",
          PortHandle,debugstr_w(PortName->Buffer),SecurityQos,
          WriteSection,pSid,ReadSection,MaximumMessageLength,ConnectInfo,
          pConnectInfoLength);
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtListenPort		[NTDLL.@]
 *  ZwListenPort		[NTDLL.@]
 */
NTSTATUS WINAPI NtListenPort(HANDLE PortHandle,PLPC_MESSAGE pLpcMessage)
{
  FIXME("(%p,%p),stub!\n",PortHandle,pLpcMessage);
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtAcceptConnectPort	[NTDLL.@]
 *  ZwAcceptConnectPort	[NTDLL.@]
 */
NTSTATUS WINAPI NtAcceptConnectPort(
        PHANDLE PortHandle,
        ULONG PortIdentifier,
        PLPC_MESSAGE pLpcMessage,
        BOOLEAN Accept,
        PLPC_SECTION_WRITE WriteSection,
        PLPC_SECTION_READ ReadSection)
{
  FIXME("(%p,%u,%p,%d,%p,%p),stub!\n",
        PortHandle,PortIdentifier,pLpcMessage,Accept,WriteSection,ReadSection);
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtCompleteConnectPort	[NTDLL.@]
 *  ZwCompleteConnectPort	[NTDLL.@]
 */
NTSTATUS WINAPI NtCompleteConnectPort(HANDLE PortHandle)
{
  FIXME("(%p),stub!\n",PortHandle);
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtRegisterThreadTerminatePort	[NTDLL.@]
 *  ZwRegisterThreadTerminatePort	[NTDLL.@]
 */
NTSTATUS WINAPI NtRegisterThreadTerminatePort(HANDLE PortHandle)
{
  FIXME("(%p),stub!\n",PortHandle);
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtRequestWaitReplyPort		[NTDLL.@]
 *  ZwRequestWaitReplyPort		[NTDLL.@]
 */
NTSTATUS WINAPI NtRequestWaitReplyPort(
        HANDLE PortHandle,
        PLPC_MESSAGE pLpcMessageIn,
        PLPC_MESSAGE pLpcMessageOut)
{
  FIXME("(%p,%p,%p),stub!\n",PortHandle,pLpcMessageIn,pLpcMessageOut);
  if(pLpcMessageIn)
  {
    TRACE("Message to send:\n");
    TRACE("\tDataSize            = %u\n",pLpcMessageIn->DataSize);
    TRACE("\tMessageSize         = %u\n",pLpcMessageIn->MessageSize);
    TRACE("\tMessageType         = %u\n",pLpcMessageIn->MessageType);
    TRACE("\tVirtualRangesOffset = %u\n",pLpcMessageIn->VirtualRangesOffset);
    TRACE("\tClientId.UniqueProcess = %p\n",pLpcMessageIn->ClientId.UniqueProcess);
    TRACE("\tClientId.UniqueThread  = %p\n",pLpcMessageIn->ClientId.UniqueThread);
    TRACE("\tMessageId           = %lu\n",pLpcMessageIn->MessageId);
    TRACE("\tSectionSize         = %lu\n",pLpcMessageIn->SectionSize);
    TRACE("\tData                = %s\n",
      debugstr_an((const char*)pLpcMessageIn->Data,pLpcMessageIn->DataSize));
  }
  return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtReplyWaitReceivePort	[NTDLL.@]
 *  ZwReplyWaitReceivePort	[NTDLL.@]
 */
NTSTATUS WINAPI NtReplyWaitReceivePort(
        HANDLE PortHandle,
        PULONG PortIdentifier,
        PLPC_MESSAGE ReplyMessage,
        PLPC_MESSAGE Message)
{
  FIXME("(%p,%p,%p,%p),stub!\n",PortHandle,PortIdentifier,ReplyMessage,Message);
  return STATUS_NOT_IMPLEMENTED;
}

/*
 *	Misc
 */

/***********************************************************************
 * RtlIsProcessorFeaturePresent [NTDLL.@]
 */
BOOLEAN WINAPI RtlIsProcessorFeaturePresent( UINT feature )
{
    return feature < PROCESSOR_FEATURE_MAX && user_shared_data->ProcessorFeatures[feature];
}

/******************************************************************************
 * RtlGetNativeSystemInformation [NTDLL.@]
 */
NTSTATUS WINAPI /* DECLSPEC_HOTPATCH */ RtlGetNativeSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
    FIXME( "semi-stub, SystemInformationClass %#x, SystemInformation %p, Length %#x, ResultLength %p.\n",
            SystemInformationClass, SystemInformation, Length, ResultLength );

    /* RtlGetNativeSystemInformation function is the same as NtQuerySystemInformation on some Win7
     * versions but there are differences for earlier and newer versions, at least:
     *     - HighestUserAddress field for SystemBasicInformation;
     *     - Some information classes are not supported by RtlGetNativeSystemInformation. */
    return NtQuerySystemInformation( SystemInformationClass, SystemInformation, Length, ResultLength );
}

/******************************************************************************
 *  NtCreatePagingFile		[NTDLL.@]
 *  ZwCreatePagingFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtCreatePagingFile(
	PUNICODE_STRING PageFileName,
	PLARGE_INTEGER MinimumSize,
	PLARGE_INTEGER MaximumSize,
	PLARGE_INTEGER ActualSize)
{
    FIXME("(%p %p %p %p) stub\n", PageFileName, MinimumSize, MaximumSize, ActualSize);
    return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtDisplayString				[NTDLL.@]
 *
 * writes a string to the nt-textmode screen eg. during startup
 */
NTSTATUS WINAPI NtDisplayString ( PUNICODE_STRING string )
{
    STRING stringA;
    NTSTATUS ret;

    if (!(ret = RtlUnicodeStringToAnsiString( &stringA, string, TRUE )))
    {
        MESSAGE( "%.*s", stringA.Length, stringA.Buffer );
        RtlFreeAnsiString( &stringA );
    }
    return ret;
}

/******************************************************************************
 *  NtAllocateLocallyUniqueId (NTDLL.@)
 */
NTSTATUS WINAPI NtAllocateLocallyUniqueId(PLUID Luid)
{
    NTSTATUS status;

    TRACE("%p\n", Luid);

    if (!Luid)
        return STATUS_ACCESS_VIOLATION;

    SERVER_START_REQ( allocate_locally_unique_id )
    {
        status = wine_server_call( req );
        if (!status)
        {
            Luid->LowPart = reply->luid.low_part;
            Luid->HighPart = reply->luid.high_part;
        }
    }
    SERVER_END_REQ;

    return status;
}

/******************************************************************************
 *        VerSetConditionMask   (NTDLL.@)
 */
ULONGLONG WINAPI VerSetConditionMask( ULONGLONG dwlConditionMask, DWORD dwTypeBitMask,
                                      BYTE dwConditionMask)
{
    if(dwTypeBitMask == 0)
	return dwlConditionMask;
    dwConditionMask &= 0x07;
    if(dwConditionMask == 0)
	return dwlConditionMask;

    if(dwTypeBitMask & VER_PRODUCT_TYPE)
	dwlConditionMask |= dwConditionMask << 7*3;
    else if (dwTypeBitMask & VER_SUITENAME)
	dwlConditionMask |= dwConditionMask << 6*3;
    else if (dwTypeBitMask & VER_SERVICEPACKMAJOR)
	dwlConditionMask |= dwConditionMask << 5*3;
    else if (dwTypeBitMask & VER_SERVICEPACKMINOR)
	dwlConditionMask |= dwConditionMask << 4*3;
    else if (dwTypeBitMask & VER_PLATFORMID)
	dwlConditionMask |= dwConditionMask << 3*3;
    else if (dwTypeBitMask & VER_BUILDNUMBER)
	dwlConditionMask |= dwConditionMask << 2*3;
    else if (dwTypeBitMask & VER_MAJORVERSION)
	dwlConditionMask |= dwConditionMask << 1*3;
    else if (dwTypeBitMask & VER_MINORVERSION)
	dwlConditionMask |= dwConditionMask << 0*3;
    return dwlConditionMask;
}

/******************************************************************************
 *  NtAccessCheckAndAuditAlarm   (NTDLL.@)
 *  ZwAccessCheckAndAuditAlarm   (NTDLL.@)
 */
NTSTATUS WINAPI NtAccessCheckAndAuditAlarm(PUNICODE_STRING SubsystemName, HANDLE HandleId, PUNICODE_STRING ObjectTypeName,
                                           PUNICODE_STRING ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor,
                                           ACCESS_MASK DesiredAccess, PGENERIC_MAPPING GenericMapping, BOOLEAN ObjectCreation,
                                           PACCESS_MASK GrantedAccess, PBOOLEAN AccessStatus, PBOOLEAN GenerateOnClose)
{
    FIXME("(%s, %p, %s, %p, 0x%08x, %p, %d, %p, %p, %p), stub\n", debugstr_us(SubsystemName), HandleId,
          debugstr_us(ObjectTypeName), SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation,
          GrantedAccess, AccessStatus, GenerateOnClose);

    return STATUS_NOT_IMPLEMENTED;
}
