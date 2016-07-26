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

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif
#ifdef HAVE_MACHINE_CPU_H
# include <machine/cpu.h>
#endif
#ifdef HAVE_MACH_MACHINE_H
# include <mach/machine.h>
#endif

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <time.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"
#include "ddk/wdm.h"

#ifdef __APPLE__
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#endif

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

    TRACE("(%p,0x%08x,%s,0x%08x,0x%08x,%p)\n",
          ExistingToken, DesiredAccess, debugstr_ObjectAttributes(ObjectAttributes),
          ImpersonationLevel, TokenType, NewToken);

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
        req->attributes          = ObjectAttributes ? ObjectAttributes->Attributes : 0;
        req->primary             = (TokenType == TokenPrimary);
        req->impersonation_level = ImpersonationLevel;
        status = wine_server_call( req );
        if (!status) *NewToken = wine_server_ptr_handle( reply->new_handle );
    }
    SERVER_END_REQ;

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
        0,    /* TokenVirtualizationEnabled */
        sizeof(TOKEN_MANDATORY_LABEL) + sizeof(SID), /* TokenIntegrityLevel [sizeof(SID) includes one SubAuthority] */
        0,    /* TokenUIAccess */
        0,    /* TokenMandatoryPolicy */
        0,    /* TokenLogonSid */
        0,    /* TokenIsAppContainer */
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

 /******************************************************************************
 *  NtSetIntervalProfile	[NTDLL.@]
 *  ZwSetIntervalProfile	[NTDLL.@]
 */
NTSTATUS WINAPI NtSetIntervalProfile(
        ULONG Interval,
        KPROFILE_SOURCE Source)
{
    FIXME("%u,%d\n", Interval, Source);
    return STATUS_SUCCESS;
}

static  SYSTEM_CPU_INFORMATION cached_sci;

/*******************************************************************************
 * Architecture specific feature detection for CPUs
 *
 * This a set of mutually exclusive #if define()s each providing its own get_cpuinfo() to be called
 * from fill_cpu_info();
 */
#if defined(__i386__) || defined(__x86_64__)

#define AUTH	0x68747541	/* "Auth" */
#define ENTI	0x69746e65	/* "enti" */
#define CAMD	0x444d4163	/* "cAMD" */

#define GENU	0x756e6547	/* "Genu" */
#define INEI	0x49656e69	/* "ineI" */
#define NTEL	0x6c65746e	/* "ntel" */

/* Calls cpuid with an eax of 'ax' and returns the 16 bytes in *p
 * We are compiled with -fPIC, so we can't clobber ebx.
 */
static inline void do_cpuid(unsigned int ax, unsigned int *p)
{
#ifdef __i386__
	__asm__("pushl %%ebx\n\t"
                "cpuid\n\t"
                "movl %%ebx, %%esi\n\t"
                "popl %%ebx"
                : "=a" (p[0]), "=S" (p[1]), "=c" (p[2]), "=d" (p[3])
                :  "0" (ax));
#elif defined(__x86_64__)
	__asm__("push %%rbx\n\t"
                "cpuid\n\t"
                "movq %%rbx, %%rsi\n\t"
                "pop %%rbx"
                : "=a" (p[0]), "=S" (p[1]), "=c" (p[2]), "=d" (p[3])
                :  "0" (ax));
#endif
}

/* From xf86info havecpuid.c 1.11 */
static inline BOOL have_cpuid(void)
{
#ifdef __i386__
	unsigned int f1, f2;
	__asm__("pushfl\n\t"
                "pushfl\n\t"
                "popl %0\n\t"
                "movl %0,%1\n\t"
                "xorl %2,%0\n\t"
                "pushl %0\n\t"
                "popfl\n\t"
                "pushfl\n\t"
                "popl %0\n\t"
                "popfl"
                : "=&r" (f1), "=&r" (f2)
                : "ir" (0x00200000));
	return ((f1^f2) & 0x00200000) != 0;
#elif defined(__x86_64__)
        return TRUE;
#else
        return FALSE;
#endif
}

/* Detect if a SSE2 processor is capable of Denormals Are Zero (DAZ) mode.
 *
 * This function assumes you have already checked for SSE2/FXSAVE support. */
static inline BOOL have_sse_daz_mode(void)
{
#ifdef __i386__
    typedef struct DECLSPEC_ALIGN(16) _M128A {
        ULONGLONG Low;
        LONGLONG High;
    } M128A;

    typedef struct _XMM_SAVE_AREA32 {
        WORD ControlWord;
        WORD StatusWord;
        BYTE TagWord;
        BYTE Reserved1;
        WORD ErrorOpcode;
        DWORD ErrorOffset;
        WORD ErrorSelector;
        WORD Reserved2;
        DWORD DataOffset;
        WORD DataSelector;
        WORD Reserved3;
        DWORD MxCsr;
        DWORD MxCsr_Mask;
        M128A FloatRegisters[8];
        M128A XmmRegisters[16];
        BYTE Reserved4[96];
    } XMM_SAVE_AREA32;

    /* Intel says we need a zeroed 16-byte aligned buffer */
    char buffer[512 + 16];
    XMM_SAVE_AREA32 *state = (XMM_SAVE_AREA32 *)(((ULONG_PTR)buffer + 15) & ~15);
    memset(buffer, 0, sizeof(buffer));

    __asm__ __volatile__( "fxsave %0" : "=m" (*state) : "m" (*state) );

    return (state->MxCsr_Mask & (1 << 6)) >> 6;
#else /* all x86_64 processors include SSE2 with DAZ mode */
    return TRUE;
#endif
}

static inline void get_cpuinfo(SYSTEM_CPU_INFORMATION* info)
{
    unsigned int regs[4], regs2[4];

#if defined(__i386__)
    info->Architecture = PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(__x86_64__)
    info->Architecture = PROCESSOR_ARCHITECTURE_AMD64;
#endif

    /* We're at least a 386 */
    info->FeatureSet = CPU_FEATURE_VME | CPU_FEATURE_X86 | CPU_FEATURE_PGE;
    info->Level = 3;

    if (!have_cpuid()) return;

    do_cpuid(0x00000000, regs);  /* get standard cpuid level and vendor name */
    if (regs[0]>=0x00000001)   /* Check for supported cpuid version */
    {
        do_cpuid(0x00000001, regs2); /* get cpu features */

        if(regs2[3] & (1 << 3 )) info->FeatureSet |= CPU_FEATURE_PSE;
        if(regs2[3] & (1 << 4 )) info->FeatureSet |= CPU_FEATURE_TSC;
        if(regs2[3] & (1 << 8 )) info->FeatureSet |= CPU_FEATURE_CX8;
        if(regs2[3] & (1 << 11)) info->FeatureSet |= CPU_FEATURE_SEP;
        if(regs2[3] & (1 << 12)) info->FeatureSet |= CPU_FEATURE_MTRR;
        if(regs2[3] & (1 << 15)) info->FeatureSet |= CPU_FEATURE_CMOV;
        if(regs2[3] & (1 << 16)) info->FeatureSet |= CPU_FEATURE_PAT;
        if(regs2[3] & (1 << 23)) info->FeatureSet |= CPU_FEATURE_MMX;
        if(regs2[3] & (1 << 24)) info->FeatureSet |= CPU_FEATURE_FXSR;
        if(regs2[3] & (1 << 25)) info->FeatureSet |= CPU_FEATURE_SSE;
        if(regs2[3] & (1 << 26)) info->FeatureSet |= CPU_FEATURE_SSE2;

        user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED]       = !(regs2[3] & 1);
        user_shared_data->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE]   = (regs2[3] >> 4) & 1;
        user_shared_data->ProcessorFeatures[PF_PAE_ENABLED]                   = (regs2[3] >> 6) & 1;
        user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE]       = (regs2[3] >> 8) & 1;
        user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE]    = (regs2[3] >> 23) & 1;
        user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE]   = (regs2[3] >> 25) & 1;
        user_shared_data->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = (regs2[3] >> 26) & 1;
        user_shared_data->ProcessorFeatures[PF_SSE3_INSTRUCTIONS_AVAILABLE]   = regs2[2] & 1;
        user_shared_data->ProcessorFeatures[PF_XSAVE_ENABLED]                 = (regs2[2] >> 27) & 1;
        user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE128]           = (regs2[2] >> 13) & 1;

        if((regs2[3] & (1 << 26)) && (regs2[3] & (1 << 24))) /* has SSE2 and FXSAVE/FXRSTOR */
            user_shared_data->ProcessorFeatures[PF_SSE_DAZ_MODE_AVAILABLE] = have_sse_daz_mode();

        if (regs[1] == AUTH && regs[3] == ENTI && regs[2] == CAMD)
        {
            info->Level = (regs2[0] >> 8) & 0xf; /* family */
            if (info->Level == 0xf)  /* AMD says to add the extended family to the family if family is 0xf */
                info->Level += (regs2[0] >> 20) & 0xff;

            /* repack model and stepping to make a "revision" */
            info->Revision  = ((regs2[0] >> 16) & 0xf) << 12; /* extended model */
            info->Revision |= ((regs2[0] >> 4 ) & 0xf) << 8;  /* model          */
            info->Revision |= regs2[0] & 0xf;                 /* stepping       */

            do_cpuid(0x80000000, regs);  /* get vendor cpuid level */
            if (regs[0] >= 0x80000001)
            {
                do_cpuid(0x80000001, regs2);  /* get vendor features */
                user_shared_data->ProcessorFeatures[PF_VIRT_FIRMWARE_ENABLED]        = (regs2[2] >> 2) & 1;
                user_shared_data->ProcessorFeatures[PF_NX_ENABLED]                   = (regs2[3] >> 20) & 1;
                user_shared_data->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = (regs2[3] >> 31) & 1;
                if (regs2[3] >> 31) info->FeatureSet |= CPU_FEATURE_3DNOW;
            }
        }
        else if (regs[1] == GENU && regs[3] == INEI && regs[2] == NTEL)
        {
            info->Level = ((regs2[0] >> 8) & 0xf) + ((regs2[0] >> 20) & 0xff); /* family + extended family */
            if(info->Level == 15) info->Level = 6;

            /* repack model and stepping to make a "revision" */
            info->Revision  = ((regs2[0] >> 16) & 0xf) << 12; /* extended model */
            info->Revision |= ((regs2[0] >> 4 ) & 0xf) << 8;  /* model          */
            info->Revision |= regs2[0] & 0xf;                 /* stepping       */

            if(regs2[3] & (1 << 21)) info->FeatureSet |= CPU_FEATURE_DS;
            user_shared_data->ProcessorFeatures[PF_VIRT_FIRMWARE_ENABLED] = (regs2[2] >> 5) & 1;

            do_cpuid(0x80000000, regs);  /* get vendor cpuid level */
            if (regs[0] >= 0x80000001)
            {
                do_cpuid(0x80000001, regs2);  /* get vendor features */
                user_shared_data->ProcessorFeatures[PF_NX_ENABLED] = (regs2[3] >> 20) & 1;
            }
        }
        else
        {
            info->Level = (regs2[0] >> 8) & 0xf; /* family */

            /* repack model and stepping to make a "revision" */
            info->Revision = ((regs2[0] >> 4 ) & 0xf) << 8;  /* model    */
            info->Revision |= regs2[0] & 0xf;                /* stepping */
        }
    }
}

#elif defined(__powerpc__) || defined(__ppc__)

static inline void get_cpuinfo(SYSTEM_CPU_INFORMATION* info)
{
#ifdef __APPLE__
    size_t valSize;
    int value;

    valSize = sizeof(value);
    if (sysctlbyname("hw.optional.floatingpoint", &value, &valSize, NULL, 0) == 0)
        user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = !value;

    valSize = sizeof(value);
    if (sysctlbyname("hw.cpusubtype", &value, &valSize, NULL, 0) == 0)
    {
        switch (value)
        {
            case CPU_SUBTYPE_POWERPC_601:
            case CPU_SUBTYPE_POWERPC_602:       info->Level = 1;   break;
            case CPU_SUBTYPE_POWERPC_603:       info->Level = 3;   break;
            case CPU_SUBTYPE_POWERPC_603e:
            case CPU_SUBTYPE_POWERPC_603ev:     info->Level = 6;   break;
            case CPU_SUBTYPE_POWERPC_604:       info->Level = 4;   break;
            case CPU_SUBTYPE_POWERPC_604e:      info->Level = 9;   break;
            case CPU_SUBTYPE_POWERPC_620:       info->Level = 20;  break;
            case CPU_SUBTYPE_POWERPC_750:       /* G3/G4 derive from 603 so ... */
            case CPU_SUBTYPE_POWERPC_7400:
            case CPU_SUBTYPE_POWERPC_7450:      info->Level = 6;   break;
            case CPU_SUBTYPE_POWERPC_970:       info->Level = 9;
                /* :o) user_shared_data->ProcessorFeatures[PF_ALTIVEC_INSTRUCTIONS_AVAILABLE] ;-) */
                break;
            default: break;
        }
    }
#else
    FIXME("CPU Feature detection not implemented.\n");
#endif
    info->Architecture = PROCESSOR_ARCHITECTURE_PPC;
}

#elif defined(__arm__)

static inline void get_cpuinfo(SYSTEM_CPU_INFORMATION* info)
{
#ifdef linux
    char line[512];
    char *s, *value;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        while (fgets(line, sizeof(line), f) != NULL)
        {
            /* NOTE: the ':' is the only character we can rely on */
            if (!(value = strchr(line,':')))
                continue;
            /* terminate the valuename */
            s = value - 1;
            while ((s >= line) && isspace(*s)) s--;
            *(s + 1) = '\0';
            /* and strip leading spaces from value */
            value += 1;
            while (isspace(*value)) value++;
            if ((s = strchr(value,'\n')))
                *s='\0';
            if (!strcasecmp(line, "CPU architecture"))
            {
                if (isdigit(value[0]))
                    info->Level = atoi(value);
                continue;
            }
            if (!strcasecmp(line, "CPU revision"))
            {
                if (isdigit(value[0]))
                    info->Revision = atoi(value);
                continue;
            }
            if (!strcasecmp(line, "features"))
            {
                if (strstr(value, "vfpv3"))
                    user_shared_data->ProcessorFeatures[PF_ARM_VFP_32_REGISTERS_AVAILABLE] = TRUE;
                if (strstr(value, "neon"))
                    user_shared_data->ProcessorFeatures[PF_ARM_NEON_INSTRUCTIONS_AVAILABLE] = TRUE;
                continue;
            }
        }
        fclose(f);
    }
#elif defined(__FreeBSD__)
    size_t valsize;
    char buf[8];
    int value;

    valsize = sizeof(buf);
    if (!sysctlbyname("hw.machine_arch", &buf, &valsize, NULL, 0) &&
        sscanf(buf, "armv%i", &value) == 1)
        info->Level = value;

    valsize = sizeof(value);
    if (!sysctlbyname("hw.floatingpoint", &value, &valsize, NULL, 0))
        user_shared_data->ProcessorFeatures[PF_ARM_VFP_32_REGISTERS_AVAILABLE] = value;
#else
    FIXME("CPU Feature detection not implemented.\n");
#endif
    if (info->Level >= 8)
        user_shared_data->ProcessorFeatures[PF_ARM_V8_INSTRUCTIONS_AVAILABLE] = TRUE;
    info->Architecture = PROCESSOR_ARCHITECTURE_ARM;
}

#elif defined(__aarch64__)

static inline void get_cpuinfo(SYSTEM_CPU_INFORMATION* info)
{
#ifdef linux
    char line[512];
    char *s, *value;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        while (fgets(line, sizeof(line), f) != NULL)
        {
            /* NOTE: the ':' is the only character we can rely on */
            if (!(value = strchr(line,':')))
                continue;
            /* terminate the valuename */
            s = value - 1;
            while ((s >= line) && isspace(*s)) s--;
            *(s + 1) = '\0';
            /* and strip leading spaces from value */
            value += 1;
            while (isspace(*value)) value++;
            if ((s = strchr(value,'\n')))
                *s='\0';
            if (!strcasecmp(line, "CPU architecture"))
            {
                if (isdigit(value[0]))
                    info->Level = atoi(value);
                continue;
            }
            if (!strcasecmp(line, "CPU revision"))
            {
                if (isdigit(value[0]))
                    info->Revision = atoi(value);
                continue;
            }
            if (!strcasecmp(line, "Features"))
            {
                if (strstr(value, "crc32"))
                    user_shared_data->ProcessorFeatures[PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE] = TRUE;
                if (strstr(value, "aes"))
                    user_shared_data->ProcessorFeatures[PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE] = TRUE;
                continue;
            }
        }
        fclose(f);
    }
#else
    FIXME("CPU Feature detection not implemented.\n");
#endif
    info->Level = max(info->Level, 8);
    user_shared_data->ProcessorFeatures[PF_ARM_V8_INSTRUCTIONS_AVAILABLE] = TRUE;
    info->Architecture = PROCESSOR_ARCHITECTURE_ARM64;
}

#endif /* End architecture specific feature detection for CPUs */

/******************************************************************
 *		fill_cpu_info
 *
 * inits a couple of places with CPU related information:
 * - cached_sci in this file
 * - Peb->NumberOfProcessors
 * - SharedUserData->ProcessFeatures[] array
 */
void fill_cpu_info(void)
{
    long num;

#ifdef _SC_NPROCESSORS_ONLN
    num = sysconf(_SC_NPROCESSORS_ONLN);
    if (num < 1)
    {
        num = 1;
        WARN("Failed to detect the number of processors.\n");
    }
#elif defined(CTL_HW) && defined(HW_NCPU)
    int mib[2];
    size_t len = sizeof(num);
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &num, &len, NULL, 0) != 0)
    {
        num = 1;
        WARN("Failed to detect the number of processors.\n");
    }
#else
    num = 1;
    FIXME("Detecting the number of processors is not supported.\n");
#endif
    NtCurrentTeb()->Peb->NumberOfProcessors = num;

    memset(&cached_sci, 0, sizeof(cached_sci));
    get_cpuinfo(&cached_sci);

    TRACE("<- CPU arch %d, level %d, rev %d, features 0x%x\n",
          cached_sci.Architecture, cached_sci.Level, cached_sci.Revision, cached_sci.FeatureSet);
}

static BOOL grow_logical_proc_buf(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *max_len)
{
    if (pdata)
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *new_data;

        *max_len *= 2;
        new_data = RtlReAllocateHeap(GetProcessHeap(), 0, *pdata, *max_len*sizeof(*new_data));
        if (!new_data)
            return FALSE;

        *pdata = new_data;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *new_dataex;

        *max_len *= 2;
        new_dataex = RtlReAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdataex, *max_len*sizeof(*new_dataex));
        if (!new_dataex)
            return FALSE;

        *pdataex = new_dataex;
    }

    return TRUE;
}

static DWORD log_proc_ex_size_plus(DWORD size)
{
    /* add SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX.Relationship and .Size */
    return sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(DWORD) + size;
}

static inline BOOL logical_proc_info_add_by_id(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *len, DWORD *pmax_len,
        LOGICAL_PROCESSOR_RELATIONSHIP rel, DWORD id, ULONG_PTR mask)
{
    if (pdata) {
        DWORD i;

        if(rel == RelationProcessorPackage){
            for(i=0; i<*len; i++)
            {
                if ((*pdata)[i].Relationship!=rel || (*pdata)[i].u.Reserved[1]!=id)
                    continue;

                (*pdata)[i].ProcessorMask |= mask;
                return TRUE;
            }
        }else
            i = *len;

        while(*len == *pmax_len)
        {
            if (!grow_logical_proc_buf(pdata, NULL, pmax_len))
                return FALSE;
        }

        (*pdata)[i].Relationship = rel;
        (*pdata)[i].ProcessorMask = mask;
        /* TODO: set processor core flags */
        (*pdata)[i].u.Reserved[0] = 0;
        (*pdata)[i].u.Reserved[1] = id;
        *len = i+1;
    }else{
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex = *pdataex;
        DWORD ofs = 0;

        while(ofs < *len)
        {
            dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);
            if (rel == RelationProcessorPackage && dataex->Relationship == rel && dataex->u.Processor.Reserved[1] == id)
            {
                dataex->u.Processor.GroupMask[0].Mask |= mask;
                return TRUE;
            }
            ofs += dataex->Size;
        }

        /* TODO: For now, just one group. If more than 64 processors, then we
         * need another group. */

        while (ofs + log_proc_ex_size_plus(sizeof(PROCESSOR_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
                return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);

        dataex->Relationship = rel;
        dataex->Size = log_proc_ex_size_plus(sizeof(PROCESSOR_RELATIONSHIP));
        dataex->u.Processor.Flags = 0; /* TODO */
        dataex->u.Processor.EfficiencyClass = 0;
        dataex->u.Processor.GroupCount = 1;
        dataex->u.Processor.GroupMask[0].Mask = mask;
        dataex->u.Processor.GroupMask[0].Group = 0;
        /* mark for future lookup */
        dataex->u.Processor.Reserved[0] = 0;
        dataex->u.Processor.Reserved[1] = id;

        *len += dataex->Size;
    }

    return TRUE;
}

static inline BOOL logical_proc_info_add_cache(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *len,
        DWORD *pmax_len, ULONG_PTR mask, CACHE_DESCRIPTOR *cache)
{
    if (pdata)
    {
        DWORD i;

        for (i=0; i<*len; i++)
        {
            if ((*pdata)[i].Relationship==RelationCache && (*pdata)[i].ProcessorMask==mask
                    && (*pdata)[i].u.Cache.Level==cache->Level && (*pdata)[i].u.Cache.Type==cache->Type)
                return TRUE;
        }

        while (*len == *pmax_len)
            if (!grow_logical_proc_buf(pdata, NULL, pmax_len))
                return FALSE;

        (*pdata)[i].Relationship = RelationCache;
        (*pdata)[i].ProcessorMask = mask;
        (*pdata)[i].u.Cache = *cache;
        *len = i+1;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex = *pdataex;
        DWORD ofs;

        for (ofs = 0; ofs < *len; )
        {
            dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);
            if (dataex->Relationship == RelationCache && dataex->u.Cache.GroupMask.Mask == mask &&
                    dataex->u.Cache.Level == cache->Level && dataex->u.Cache.Type == cache->Type)
                return TRUE;
            ofs += dataex->Size;
        }

        while (ofs + log_proc_ex_size_plus(sizeof(CACHE_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
                return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);

        dataex->Relationship = RelationCache;
        dataex->Size = log_proc_ex_size_plus(sizeof(CACHE_RELATIONSHIP));
        dataex->u.Cache.Level = cache->Level;
        dataex->u.Cache.Associativity = cache->Associativity;
        dataex->u.Cache.LineSize = cache->LineSize;
        dataex->u.Cache.CacheSize = cache->Size;
        dataex->u.Cache.Type = cache->Type;
        dataex->u.Cache.GroupMask.Mask = mask;
        dataex->u.Cache.GroupMask.Group = 0;

        *len += dataex->Size;
    }

    return TRUE;
}

static inline BOOL logical_proc_info_add_numa_node(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *len, DWORD *pmax_len, ULONG_PTR mask,
        DWORD node_id)
{
    if (pdata)
    {
        while (*len == *pmax_len)
            if (!grow_logical_proc_buf(pdata, NULL, pmax_len))
                return FALSE;

        (*pdata)[*len].Relationship = RelationNumaNode;
        (*pdata)[*len].ProcessorMask = mask;
        (*pdata)[*len].u.NumaNode.NodeNumber = node_id;
        (*len)++;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

        while (*len + log_proc_ex_size_plus(sizeof(NUMA_NODE_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
                return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + *len);

        dataex->Relationship = RelationNumaNode;
        dataex->Size = log_proc_ex_size_plus(sizeof(NUMA_NODE_RELATIONSHIP));
        dataex->u.NumaNode.NodeNumber = node_id;
        dataex->u.NumaNode.GroupMask.Mask = mask;
        dataex->u.NumaNode.GroupMask.Group = 0;

        *len += dataex->Size;
    }

    return TRUE;
}

static inline BOOL logical_proc_info_add_group(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex,
        DWORD *len, DWORD *pmax_len, DWORD num_cpus, ULONG_PTR mask)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

    while (*len + log_proc_ex_size_plus(sizeof(GROUP_RELATIONSHIP)) > *pmax_len)
    {
        if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
            return FALSE;
    }

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + *len);

    dataex->Relationship = RelationGroup;
    dataex->Size = log_proc_ex_size_plus(sizeof(GROUP_RELATIONSHIP));
    dataex->u.Group.MaximumGroupCount = 1;
    dataex->u.Group.ActiveGroupCount = 1;
    dataex->u.Group.GroupInfo[0].MaximumProcessorCount = num_cpus;
    dataex->u.Group.GroupInfo[0].ActiveProcessorCount = num_cpus;
    dataex->u.Group.GroupInfo[0].ActiveProcessorMask = mask;

    *len += dataex->Size;

    return TRUE;
}

#ifdef linux
/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static NTSTATUS create_logical_proc_info(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex, DWORD *max_len)
{
    static const char core_info[] = "/sys/devices/system/cpu/cpu%u/%s";
    static const char cache_info[] = "/sys/devices/system/cpu/cpu%u/cache/index%u/%s";
    static const char numa_info[] = "/sys/devices/system/node/node%u/cpumap";

    FILE *fcpu_list, *fnuma_list, *f;
    DWORD len = 0, beg, end, i, j, r, num_cpus = 0;
    char op, name[MAX_PATH];
    ULONG_PTR all_cpus_mask = 0;

    fcpu_list = fopen("/sys/devices/system/cpu/online", "r");
    if(!fcpu_list)
        return STATUS_NOT_IMPLEMENTED;

    while(!feof(fcpu_list))
    {
        if(!fscanf(fcpu_list, "%u%c ", &beg, &op))
            break;
        if(op == '-') fscanf(fcpu_list, "%u%c ", &end, &op);
        else end = beg;

        for(i=beg; i<=end; i++)
        {
            if(i > 8*sizeof(ULONG_PTR))
            {
                FIXME("skipping logical processor %d\n", i);
                continue;
            }

            sprintf(name, core_info, i, "physical_package_id");
            f = fopen(name, "r");
            if(f)
            {
                fscanf(f, "%u", &r);
                fclose(f);
            }
            else r = 0;
            if(!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorPackage, r, (ULONG_PTR)1 << i))
            {
                fclose(fcpu_list);
                return STATUS_NO_MEMORY;
            }

            sprintf(name, core_info, i, "core_id");
            f = fopen(name, "r");
            if(f)
            {
                fscanf(f, "%u", &r);
                fclose(f);
            }
            else r = i;
            if(!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorCore, r, (ULONG_PTR)1 << i))
            {
                fclose(fcpu_list);
                return STATUS_NO_MEMORY;
            }

            for(j=0; j<4; j++)
            {
                CACHE_DESCRIPTOR cache;
                ULONG_PTR mask = 0;

                sprintf(name, cache_info, i, j, "shared_cpu_map");
                f = fopen(name, "r");
                if(!f) continue;
                while(!feof(f))
                {
                    if(!fscanf(f, "%x%c ", &r, &op))
                        break;
                    mask = (sizeof(ULONG_PTR)>sizeof(int) ? mask<<(8*sizeof(DWORD)) : 0) + r;
                }
                fclose(f);

                sprintf(name, cache_info, i, j, "level");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u", &r);
                fclose(f);
                cache.Level = r;

                sprintf(name, cache_info, i, j, "ways_of_associativity");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u", &r);
                fclose(f);
                cache.Associativity = r;

                sprintf(name, cache_info, i, j, "coherency_line_size");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u", &r);
                fclose(f);
                cache.LineSize = r;

                sprintf(name, cache_info, i, j, "size");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%u%c", &r, &op);
                fclose(f);
                if(op != 'K')
                    WARN("unknown cache size %u%c\n", r, op);
                cache.Size = (op=='K' ? r*1024 : r);

                sprintf(name, cache_info, i, j, "type");
                f = fopen(name, "r");
                if(!f) continue;
                fscanf(f, "%s", name);
                fclose(f);
                if(!memcmp(name, "Data", 5))
                    cache.Type = CacheData;
                else if(!memcmp(name, "Instruction", 11))
                    cache.Type = CacheInstruction;
                else
                    cache.Type = CacheUnified;

                if(!logical_proc_info_add_cache(data, dataex, &len, max_len, mask, &cache))
                {
                    fclose(fcpu_list);
                    return STATUS_NO_MEMORY;
                }
            }
        }
    }
    fclose(fcpu_list);

    if(data){
        for(i=0; i<len; i++){
            if((*data)[i].Relationship == RelationProcessorCore){
                all_cpus_mask |= (*data)[i].ProcessorMask;
                ++num_cpus;
            }
        }
    }else{
        for(i = 0; i < len; ){
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *infoex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*dataex) + i);
            if(infoex->Relationship == RelationProcessorCore){
                all_cpus_mask |= infoex->u.Processor.GroupMask[0].Mask;
                ++num_cpus;
            }
            i += infoex->Size;
        }
    }

    fnuma_list = fopen("/sys/devices/system/node/online", "r");
    if(!fnuma_list)
    {
        if(!logical_proc_info_add_numa_node(data, dataex, &len, max_len, all_cpus_mask, 0))
            return STATUS_NO_MEMORY;
    }
    else
    {
        while(!feof(fnuma_list))
        {
            if(!fscanf(fnuma_list, "%u%c ", &beg, &op))
                break;
            if(op == '-') fscanf(fnuma_list, "%u%c ", &end, &op);
            else end = beg;

            for(i=beg; i<=end; i++)
            {
                ULONG_PTR mask = 0;

                sprintf(name, numa_info, i);
                f = fopen(name, "r");
                if(!f) continue;
                while(!feof(f))
                {
                    if(!fscanf(f, "%x%c ", &r, &op))
                        break;
                    mask = (sizeof(ULONG_PTR)>sizeof(int) ? mask<<(8*sizeof(DWORD)) : 0) + r;
                }
                fclose(f);

                if(!logical_proc_info_add_numa_node(data, dataex, &len, max_len, mask, i))
                {
                    fclose(fnuma_list);
                    return STATUS_NO_MEMORY;
                }
            }
        }
        fclose(fnuma_list);
    }

    if(dataex)
        logical_proc_info_add_group(dataex, &len, max_len, num_cpus, all_cpus_mask);

    if(data)
        *max_len = len * sizeof(**data);
    else
        *max_len = len;

    return STATUS_SUCCESS;
}
#elif defined(__APPLE__)
/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static NTSTATUS create_logical_proc_info(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex, DWORD *max_len)
{
    DWORD pkgs_no, cores_no, lcpu_no, lcpu_per_core, cores_per_package, assoc, len = 0;
    DWORD cache_ctrs[10] = {0};
    ULONG_PTR all_cpus_mask = 0;
    CACHE_DESCRIPTOR cache[10];
    LONGLONG cache_size, cache_line_size, cache_sharing[10];
    size_t size;
    DWORD p,i,j,k;

    lcpu_no = NtCurrentTeb()->Peb->NumberOfProcessors;

    size = sizeof(pkgs_no);
    if(sysctlbyname("hw.packages", &pkgs_no, &size, NULL, 0))
        pkgs_no = 1;

    size = sizeof(cores_no);
    if(sysctlbyname("hw.physicalcpu", &cores_no, &size, NULL, 0))
        cores_no = lcpu_no;

    TRACE("%u logical CPUs from %u physical cores across %u packages\n",
            lcpu_no, cores_no, pkgs_no);

    lcpu_per_core = lcpu_no / cores_no;
    cores_per_package = cores_no / pkgs_no;

    memset(cache, 0, sizeof(cache));
    cache[1].Level = 1;
    cache[1].Type = CacheInstruction;
    cache[1].Associativity = 8; /* reasonable default */
    cache[1].LineSize = 0x40; /* reasonable default */
    cache[2].Level = 1;
    cache[2].Type = CacheData;
    cache[2].Associativity = 8;
    cache[2].LineSize = 0x40;
    cache[3].Level = 2;
    cache[3].Type = CacheUnified;
    cache[3].Associativity = 8;
    cache[3].LineSize = 0x40;
    cache[4].Level = 3;
    cache[4].Type = CacheUnified;
    cache[4].Associativity = 12;
    cache[4].LineSize = 0x40;

    size = sizeof(cache_line_size);
    if(!sysctlbyname("hw.cachelinesize", &cache_line_size, &size, NULL, 0))
    {
        for(i=1; i<5; i++)
            cache[i].LineSize = cache_line_size;
    }

    /* TODO: set actual associativity for all caches */
    size = sizeof(assoc);
    if(!sysctlbyname("machdep.cpu.cache.L2_associativity", &assoc, &size, NULL, 0))
        cache[3].Associativity = assoc;

    size = sizeof(cache_size);
    if(!sysctlbyname("hw.l1icachesize", &cache_size, &size, NULL, 0))
        cache[1].Size = cache_size;
    size = sizeof(cache_size);
    if(!sysctlbyname("hw.l1dcachesize", &cache_size, &size, NULL, 0))
        cache[2].Size = cache_size;
    size = sizeof(cache_size);
    if(!sysctlbyname("hw.l2cachesize", &cache_size, &size, NULL, 0))
        cache[3].Size = cache_size;
    size = sizeof(cache_size);
    if(!sysctlbyname("hw.l3cachesize", &cache_size, &size, NULL, 0))
        cache[4].Size = cache_size;

    size = sizeof(cache_sharing);
    if(sysctlbyname("hw.cacheconfig", cache_sharing, &size, NULL, 0) < 0){
        cache_sharing[1] = lcpu_per_core;
        cache_sharing[2] = lcpu_per_core;
        cache_sharing[3] = lcpu_per_core;
        cache_sharing[4] = lcpu_no;
    }else{
        /* in cache[], indexes 1 and 2 are l1 caches */
        cache_sharing[4] = cache_sharing[3];
        cache_sharing[3] = cache_sharing[2];
        cache_sharing[2] = cache_sharing[1];
    }

    for(p = 0; p < pkgs_no; ++p){
        for(j = 0; j < cores_per_package && p * cores_per_package + j < cores_no; ++j){
            ULONG_PTR mask = 0;

            for(k = 0; k < lcpu_per_core; ++k)
                mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

            all_cpus_mask |= mask;

            /* add to package */
            if(!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorPackage, p, mask))
                return STATUS_NO_MEMORY;

            /* add new core */
            if(!logical_proc_info_add_by_id(data, dataex, &len, max_len, RelationProcessorCore, p, mask))
                return STATUS_NO_MEMORY;

            for(i = 1; i < 5; ++i){
                if(cache_ctrs[i] == 0 && cache[i].Size > 0){
                    mask = 0;
                    for(k = 0; k < cache_sharing[i]; ++k)
                        mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

                    if(!logical_proc_info_add_cache(data, dataex, &len, max_len, mask, &cache[i]))
                        return STATUS_NO_MEMORY;
                }

                cache_ctrs[i] += lcpu_per_core;

                if(cache_ctrs[i] == cache_sharing[i])
                    cache_ctrs[i] = 0;
            }
        }
    }

    /* OSX doesn't support NUMA, so just make one NUMA node for all CPUs */
    if(!logical_proc_info_add_numa_node(data, dataex, &len, max_len, all_cpus_mask, 0))
        return STATUS_NO_MEMORY;

    if(dataex)
        logical_proc_info_add_group(dataex, &len, max_len, lcpu_no, all_cpus_mask);

    if(data)
        *max_len = len * sizeof(**data);
    else
        *max_len = len;

    return STATUS_SUCCESS;
}
#else
static NTSTATUS create_logical_proc_info(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex, DWORD *max_len)
{
    FIXME("stub\n");
    return STATUS_NOT_IMPLEMENTED;
}
#endif

/******************************************************************************
 * NtQuerySystemInformation [NTDLL.@]
 * ZwQuerySystemInformation [NTDLL.@]
 *
 * ARGUMENTS:
 *  SystemInformationClass	Index to a certain information structure
 *	SystemTimeAdjustmentInformation	SYSTEM_TIME_ADJUSTMENT
 *	SystemCacheInformation		SYSTEM_CACHE_INFORMATION
 *	SystemConfigurationInformation	CONFIGURATION_INFORMATION
 *	observed (class/len):
 *		0x0/0x2c
 *		0x12/0x18
 *		0x2/0x138
 *		0x8/0x600
 *              0x25/0xc
 *  SystemInformation	caller supplies storage for the information structure
 *  Length		size of the structure
 *  ResultLength	Data written
 */
NTSTATUS WINAPI NtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
    NTSTATUS    ret = STATUS_SUCCESS;
    ULONG       len = 0;

    TRACE("(0x%08x,%p,0x%08x,%p)\n",
          SystemInformationClass,SystemInformation,Length,ResultLength);

    switch (SystemInformationClass)
    {
    case SystemBasicInformation:
        {
            SYSTEM_BASIC_INFORMATION sbi;

            virtual_get_system_info( &sbi );
            len = sizeof(sbi);

            if ( Length == len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &sbi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemCpuInformation:
        if (Length >= (len = sizeof(cached_sci)))
        {
            if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
            else memcpy(SystemInformation, &cached_sci, len);
        }
        else ret = STATUS_INFO_LENGTH_MISMATCH;
        break;
    case SystemPerformanceInformation:
        {
            SYSTEM_PERFORMANCE_INFORMATION spi;
            static BOOL fixme_written = FALSE;
            FILE *fp;

            memset(&spi, 0 , sizeof(spi));
            len = sizeof(spi);

            spi.Reserved3 = 0x7fffffff; /* Available paged pool memory? */

            if ((fp = fopen("/proc/uptime", "r")))
            {
                double uptime, idle_time;

                fscanf(fp, "%lf %lf", &uptime, &idle_time);
                fclose(fp);
                spi.IdleTime.QuadPart = 10000000 * idle_time;
            }
            else
            {
                static ULONGLONG idle;
                /* many programs expect IdleTime to change so fake change */
                spi.IdleTime.QuadPart = ++idle;
            }

            if (Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &spi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
            if(!fixme_written) {
                FIXME("info_class SYSTEM_PERFORMANCE_INFORMATION\n");
                fixme_written = TRUE;
            }
        }
        break;
    case SystemTimeOfDayInformation:
        {
            SYSTEM_TIMEOFDAY_INFORMATION sti;

            memset(&sti, 0 , sizeof(sti));

            /* liKeSystemTime, liExpTimeZoneBias, uCurrentTimeZoneId */
            sti.liKeBootTime.QuadPart = server_start_time;

            if (Length <= sizeof(sti))
            {
                len = Length;
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &sti, Length);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemProcessInformation:
        {
            SYSTEM_PROCESS_INFORMATION* spi = SystemInformation;
            SYSTEM_PROCESS_INFORMATION* last = NULL;
            HANDLE hSnap = 0;
            WCHAR procname[1024];
            WCHAR* exename;
            DWORD wlen = 0;
            DWORD procstructlen = 0;

            SERVER_START_REQ( create_snapshot )
            {
                req->flags      = SNAP_PROCESS | SNAP_THREAD;
                req->attributes = 0;
                if (!(ret = wine_server_call( req )))
                    hSnap = wine_server_ptr_handle( reply->handle );
            }
            SERVER_END_REQ;
            len = 0;
            while (ret == STATUS_SUCCESS)
            {
                SERVER_START_REQ( next_process )
                {
                    req->handle = wine_server_obj_handle( hSnap );
                    req->reset = (len == 0);
                    wine_server_set_reply( req, procname, sizeof(procname)-sizeof(WCHAR) );
                    if (!(ret = wine_server_call( req )))
                    {
                        /* Make sure procname is 0 terminated */
                        procname[wine_server_reply_size(reply) / sizeof(WCHAR)] = 0;

                        /* Get only the executable name, not the path */
                        if ((exename = strrchrW(procname, '\\')) != NULL) exename++;
                        else exename = procname;

                        wlen = (strlenW(exename) + 1) * sizeof(WCHAR);

                        procstructlen = sizeof(*spi) + wlen + ((reply->threads - 1) * sizeof(SYSTEM_THREAD_INFORMATION));

                        if (Length >= len + procstructlen)
                        {
                            /* ftCreationTime, ftUserTime, ftKernelTime;
                             * vmCounters, ioCounters
                             */
 
                            memset(spi, 0, sizeof(*spi));

                            spi->NextEntryOffset = procstructlen - wlen;
                            spi->dwThreadCount = reply->threads;

                            /* spi->pszProcessName will be set later on */

                            spi->dwBasePriority = reply->priority;
                            spi->UniqueProcessId = UlongToHandle(reply->pid);
                            spi->ParentProcessId = UlongToHandle(reply->ppid);
                            spi->HandleCount = reply->handles;

                            /* spi->ti will be set later on */

                        }
                        len += procstructlen;
                    }
                }
                SERVER_END_REQ;
 
                if (ret != STATUS_SUCCESS)
                {
                    if (ret == STATUS_NO_MORE_FILES) ret = STATUS_SUCCESS;
                    break;
                }

                if (Length >= len)
                {
                    int     i, j;

                    /* set thread info */
                    i = j = 0;
                    while (ret == STATUS_SUCCESS)
                    {
                        SERVER_START_REQ( next_thread )
                        {
                            req->handle = wine_server_obj_handle( hSnap );
                            req->reset = (j == 0);
                            if (!(ret = wine_server_call( req )))
                            {
                                j++;
                                if (UlongToHandle(reply->pid) == spi->UniqueProcessId)
                                {
                                    /* ftKernelTime, ftUserTime, ftCreateTime;
                                     * dwTickCount, dwStartAddress
                                     */

                                    memset(&spi->ti[i], 0, sizeof(spi->ti));

                                    spi->ti[i].CreateTime.QuadPart = 0xdeadbeef;
                                    spi->ti[i].ClientId.UniqueProcess = UlongToHandle(reply->pid);
                                    spi->ti[i].ClientId.UniqueThread  = UlongToHandle(reply->tid);
                                    spi->ti[i].dwCurrentPriority = reply->base_pri + reply->delta_pri;
                                    spi->ti[i].dwBasePriority = reply->base_pri;
                                    i++;
                                }
                            }
                        }
                        SERVER_END_REQ;
                    }
                    if (ret == STATUS_NO_MORE_FILES) ret = STATUS_SUCCESS;

                    /* now append process name */
                    spi->ProcessName.Buffer = (WCHAR*)((char*)spi + spi->NextEntryOffset);
                    spi->ProcessName.Length = wlen - sizeof(WCHAR);
                    spi->ProcessName.MaximumLength = wlen;
                    memcpy( spi->ProcessName.Buffer, exename, wlen );
                    spi->NextEntryOffset += wlen;

                    last = spi;
                    spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + spi->NextEntryOffset);
                }
            }
            if (ret == STATUS_SUCCESS && last) last->NextEntryOffset = 0;
            if (len > Length) ret = STATUS_INFO_LENGTH_MISMATCH;
            if (hSnap) NtClose(hSnap);
        }
        break;
    case SystemProcessorPerformanceInformation:
        {
            SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *sppi = NULL;
            unsigned int cpus = 0;
            int out_cpus = Length / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

            if (out_cpus == 0)
            {
                len = 0;
                ret = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }
            else
#ifdef __APPLE__
            {
                processor_cpu_load_info_data_t *pinfo;
                mach_msg_type_number_t info_count;

                if (host_processor_info (mach_host_self (),
                                         PROCESSOR_CPU_LOAD_INFO,
                                         &cpus,
                                         (processor_info_array_t*)&pinfo,
                                         &info_count) == 0)
                {
                    int i;
                    cpus = min(cpus,out_cpus);
                    len = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * cpus;
                    sppi = RtlAllocateHeap(GetProcessHeap(), 0,len);
                    for (i = 0; i < cpus; i++)
                    {
                        sppi[i].IdleTime.QuadPart = pinfo[i].cpu_ticks[CPU_STATE_IDLE];
                        sppi[i].KernelTime.QuadPart = pinfo[i].cpu_ticks[CPU_STATE_SYSTEM];
                        sppi[i].UserTime.QuadPart = pinfo[i].cpu_ticks[CPU_STATE_USER];
                    }
                    vm_deallocate (mach_task_self (), (vm_address_t) pinfo, info_count * sizeof(natural_t));
                }
            }
#else
            {
                FILE *cpuinfo = fopen("/proc/stat", "r");
                if (cpuinfo)
                {
                    unsigned long clk_tck = sysconf(_SC_CLK_TCK);
                    unsigned long usr,nice,sys,idle,remainder[8];
                    int i, count;
                    char name[32];
                    char line[255];

                    /* first line is combined usage */
                    while (fgets(line,255,cpuinfo))
                    {
                        count = sscanf(line, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                       name, &usr, &nice, &sys, &idle,
                                       &remainder[0], &remainder[1], &remainder[2], &remainder[3],
                                       &remainder[4], &remainder[5], &remainder[6], &remainder[7]);

                        if (count < 5 || strncmp( name, "cpu", 3 )) break;
                        for (i = 0; i + 5 < count; ++i) sys += remainder[i];
                        sys += idle;
                        usr += nice;
                        cpus = atoi( name + 3 ) + 1;
                        if (cpus > out_cpus) break;
                        len = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * cpus;
                        if (sppi)
                            sppi = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sppi, len );
                        else
                            sppi = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, len );

                        sppi[cpus-1].IdleTime.QuadPart   = (ULONGLONG)idle * 10000000 / clk_tck;
                        sppi[cpus-1].KernelTime.QuadPart = (ULONGLONG)sys * 10000000 / clk_tck;
                        sppi[cpus-1].UserTime.QuadPart   = (ULONGLONG)usr * 10000000 / clk_tck;
                    }
                    fclose(cpuinfo);
                }
            }
#endif

            if (cpus == 0)
            {
                static int i = 1;
                unsigned int n;
                cpus = min(NtCurrentTeb()->Peb->NumberOfProcessors, out_cpus);
                len = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * cpus;
                sppi = RtlAllocateHeap(GetProcessHeap(), 0, len);
                FIXME("stub info_class SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION\n");
                /* many programs expect these values to change so fake change */
                for (n = 0; n < cpus; n++)
                {
                    sppi[n].KernelTime.QuadPart = 1 * i;
                    sppi[n].UserTime.QuadPart   = 2 * i;
                    sppi[n].IdleTime.QuadPart   = 3 * i;
                }
                i++;
            }

            if (Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, sppi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;

            RtlFreeHeap(GetProcessHeap(),0,sppi);
        }
        break;
    case SystemModuleInformation:
        /* FIXME: should be system-wide */
        if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
        else ret = LdrQueryProcessModuleInformation( SystemInformation, Length, &len );
        break;
    case SystemHandleInformation:
        {
            struct handle_info *info;
            DWORD i, num_handles;

            if (Length < sizeof(SYSTEM_HANDLE_INFORMATION))
            {
                ret = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            if (!SystemInformation)
            {
                ret = STATUS_ACCESS_VIOLATION;
                break;
            }

            num_handles = (Length - FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handle )) / sizeof(SYSTEM_HANDLE_ENTRY);
            if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*info) * num_handles )))
                return STATUS_NO_MEMORY;

            SERVER_START_REQ( get_system_handles )
            {
                wine_server_set_reply( req, info, sizeof(*info) * num_handles );
                if (!(ret = wine_server_call( req )))
                {
                    SYSTEM_HANDLE_INFORMATION *shi = SystemInformation;
                    shi->Count = wine_server_reply_size( req ) / sizeof(*info);
                    len = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handle[shi->Count] );
                    for (i = 0; i < shi->Count; i++)
                    {
                        memset( &shi->Handle[i], 0, sizeof(shi->Handle[i]) );
                        shi->Handle[i].OwnerPid     = info[i].owner;
                        shi->Handle[i].HandleValue  = info[i].handle;
                        shi->Handle[i].AccessMask   = info[i].access;
                        /* FIXME: Fill out ObjectType, HandleFlags, ObjectPointer */
                    }
                }
                else if (ret == STATUS_BUFFER_TOO_SMALL)
                {
                    len = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handle[reply->count] );
                    ret = STATUS_INFO_LENGTH_MISMATCH;
                }
            }
            SERVER_END_REQ;

            RtlFreeHeap( GetProcessHeap(), 0, info );
        }
        break;
    case SystemCacheInformation:
        {
            SYSTEM_CACHE_INFORMATION sci;

            memset(&sci, 0, sizeof(sci)); /* FIXME */
            len = sizeof(sci);

            if ( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &sci, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
            FIXME("info_class SYSTEM_CACHE_INFORMATION\n");
        }
        break;
    case SystemInterruptInformation:
        {
            SYSTEM_INTERRUPT_INFORMATION sii;

            memset(&sii, 0, sizeof(sii));
            len = sizeof(sii);

            if ( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &sii, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
            FIXME("info_class SYSTEM_INTERRUPT_INFORMATION\n");
        }
        break;
    case SystemKernelDebuggerInformation:
        {
            SYSTEM_KERNEL_DEBUGGER_INFORMATION skdi;

            skdi.DebuggerEnabled = FALSE;
            skdi.DebuggerNotPresent = TRUE;
            len = sizeof(skdi);

            if ( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &skdi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemRegistryQuotaInformation:
        {
	    /* Something to do with the size of the registry             *
	     * Since we don't have a size limitation, fake it            *
	     * This is almost certainly wrong.                           *
	     * This sets each of the three words in the struct to 32 MB, *
	     * which is enough to make the IE 5 installer happy.         */
            SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;

            srqi.RegistryQuotaAllowed = 0x2000000;
            srqi.RegistryQuotaUsed = 0x200000;
            srqi.Reserved1 = (void*)0x200000;
            len = sizeof(srqi);

            if ( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else
                {
                    FIXME("SystemRegistryQuotaInformation: faking max registry size of 32 MB\n");
                    memcpy( SystemInformation, &srqi, len);
                }
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
	break;
    case SystemLogicalProcessorInformation:
        {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buf;

            /* Each logical processor may use up to 7 entries in returned table:
             * core, numa node, package, L1i, L1d, L2, L3 */
            len = 7 * NtCurrentTeb()->Peb->NumberOfProcessors;
            buf = RtlAllocateHeap(GetProcessHeap(), 0, len * sizeof(*buf));
            if(!buf)
            {
                ret = STATUS_NO_MEMORY;
                break;
            }

            ret = create_logical_proc_info(&buf, NULL, &len);
            if( ret != STATUS_SUCCESS )
            {
                RtlFreeHeap(GetProcessHeap(), 0, buf);
                break;
            }

            if( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, buf, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
            RtlFreeHeap(GetProcessHeap(), 0, buf);
        }
        break;
    case SystemRecommendedSharedDataAlignment:
        {
            len = sizeof(DWORD);
            if (Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else *((DWORD *)SystemInformation) = 64;
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    default:
	FIXME("(0x%08x,%p,0x%08x,%p) stub\n",
	      SystemInformationClass,SystemInformation,Length,ResultLength);

        /* Several Information Classes are not implemented on Windows and return 2 different values 
         * STATUS_NOT_IMPLEMENTED or STATUS_INVALID_INFO_CLASS
         * in 95% of the cases it's STATUS_INVALID_INFO_CLASS, so use this as the default
        */
        ret = STATUS_INVALID_INFO_CLASS;
    }

    if (ResultLength) *ResultLength = len;

    return ret;
}

/******************************************************************************
 * NtQuerySystemInformationEx [NTDLL.@]
 * ZwQuerySystemInformationEx [NTDLL.@]
 */
NTSTATUS WINAPI NtQuerySystemInformationEx(SYSTEM_INFORMATION_CLASS SystemInformationClass,
    void *Query, ULONG QueryLength, void *SystemInformation, ULONG Length, ULONG *ResultLength)
{
    ULONG len = 0;
    NTSTATUS ret = STATUS_NOT_IMPLEMENTED;

    TRACE("(0x%08x,%p,%u,%p,%u,%p) stub\n", SystemInformationClass, Query, QueryLength, SystemInformation,
        Length, ResultLength);

    switch (SystemInformationClass) {
    case SystemLogicalProcessorInformationEx:
        {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buf;

            if (!Query || QueryLength < sizeof(DWORD))
            {
                ret = STATUS_INVALID_PARAMETER;
                break;
            }

            if (*(DWORD*)Query != RelationAll)
                FIXME("Relationship filtering not implemented: 0x%x\n", *(DWORD*)Query);

            len = 3 * sizeof(*buf);
            buf = RtlAllocateHeap(GetProcessHeap(), 0, len);
            if (!buf)
            {
                ret = STATUS_NO_MEMORY;
                break;
            }

            ret = create_logical_proc_info(NULL, &buf, &len);
            if (ret != STATUS_SUCCESS)
            {
                RtlFreeHeap(GetProcessHeap(), 0, buf);
                break;
            }

            if (Length >= len)
            {
                if (!SystemInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else
                    memcpy( SystemInformation, buf, len);
            }
            else
                ret = STATUS_INFO_LENGTH_MISMATCH;

            RtlFreeHeap(GetProcessHeap(), 0, buf);

            break;
        }
    default:
        FIXME("(0x%08x,%p,%u,%p,%u,%p) stub\n", SystemInformationClass, Query, QueryLength, SystemInformation,
            Length, ResultLength);
        break;
    }

    if (ResultLength)
        *ResultLength = len;

    return ret;
}

/******************************************************************************
 * NtSetSystemInformation [NTDLL.@]
 * ZwSetSystemInformation [NTDLL.@]
 */
NTSTATUS WINAPI NtSetSystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG Length)
{
    FIXME("(0x%08x,%p,0x%08x) stub\n",SystemInformationClass,SystemInformation,Length);
    return STATUS_SUCCESS;
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
 *  NtInitiatePowerAction                       [NTDLL.@]
 *
 */
NTSTATUS WINAPI NtInitiatePowerAction(
	IN POWER_ACTION SystemAction,
	IN SYSTEM_POWER_STATE MinSystemState,
	IN ULONG Flags,
	IN BOOLEAN Asynchronous)
{
        FIXME("(%d,%d,0x%08x,%d),stub\n",
		SystemAction,MinSystemState,Flags,Asynchronous);
        return STATUS_NOT_IMPLEMENTED;
}

#ifdef linux
/* Fallback using /proc/cpuinfo for Linux systems without cpufreq. For
 * most distributions on recent enough hardware, this is only likely to
 * happen while running in virtualized environments such as QEMU. */
static ULONG mhz_from_cpuinfo(void)
{
    char line[512];
    char *s, *value;
    double cmz = 0;
    FILE* f = fopen("/proc/cpuinfo", "r");
    if(f) {
        while (fgets(line, sizeof(line), f) != NULL) {
            if (!(value = strchr(line,':')))
                continue;
            s = value - 1;
            while ((s >= line) && isspace(*s)) s--;
            *(s + 1) = '\0';
            value++;
            if (!strcasecmp(line, "cpu MHz")) {
                sscanf(value, " %lf", &cmz);
                break;
            }
        }
        fclose(f);
    }
    return cmz;
}
#endif

/******************************************************************************
 *  NtPowerInformation				[NTDLL.@]
 *
 */
NTSTATUS WINAPI NtPowerInformation(
	IN POWER_INFORMATION_LEVEL InformationLevel,
	IN PVOID lpInputBuffer,
	IN ULONG nInputBufferSize,
	IN PVOID lpOutputBuffer,
	IN ULONG nOutputBufferSize)
{
	TRACE("(%d,%p,%d,%p,%d)\n",
		InformationLevel,lpInputBuffer,nInputBufferSize,lpOutputBuffer,nOutputBufferSize);
	switch(InformationLevel) {
		case SystemPowerCapabilities: {
			PSYSTEM_POWER_CAPABILITIES PowerCaps = lpOutputBuffer;
			FIXME("semi-stub: SystemPowerCapabilities\n");
			if (nOutputBufferSize < sizeof(SYSTEM_POWER_CAPABILITIES))
				return STATUS_BUFFER_TOO_SMALL;
			/* FIXME: These values are based off a native XP desktop, should probably use APM/ACPI to get the 'real' values */
			PowerCaps->PowerButtonPresent = TRUE;
			PowerCaps->SleepButtonPresent = FALSE;
			PowerCaps->LidPresent = FALSE;
			PowerCaps->SystemS1 = TRUE;
			PowerCaps->SystemS2 = FALSE;
			PowerCaps->SystemS3 = FALSE;
			PowerCaps->SystemS4 = TRUE;
			PowerCaps->SystemS5 = TRUE;
			PowerCaps->HiberFilePresent = TRUE;
			PowerCaps->FullWake = TRUE;
			PowerCaps->VideoDimPresent = FALSE;
			PowerCaps->ApmPresent = FALSE;
			PowerCaps->UpsPresent = FALSE;
			PowerCaps->ThermalControl = FALSE;
			PowerCaps->ProcessorThrottle = FALSE;
			PowerCaps->ProcessorMinThrottle = 100;
			PowerCaps->ProcessorMaxThrottle = 100;
			PowerCaps->DiskSpinDown = TRUE;
			PowerCaps->SystemBatteriesPresent = FALSE;
			PowerCaps->BatteriesAreShortTerm = FALSE;
			PowerCaps->BatteryScale[0].Granularity = 0;
			PowerCaps->BatteryScale[0].Capacity = 0;
			PowerCaps->BatteryScale[1].Granularity = 0;
			PowerCaps->BatteryScale[1].Capacity = 0;
			PowerCaps->BatteryScale[2].Granularity = 0;
			PowerCaps->BatteryScale[2].Capacity = 0;
			PowerCaps->AcOnLineWake = PowerSystemUnspecified;
			PowerCaps->SoftLidWake = PowerSystemUnspecified;
			PowerCaps->RtcWake = PowerSystemSleeping1;
			PowerCaps->MinDeviceWakeState = PowerSystemUnspecified;
			PowerCaps->DefaultLowLatencyWake = PowerSystemUnspecified;
			return STATUS_SUCCESS;
		}
		case SystemExecutionState: {
			PULONG ExecutionState = lpOutputBuffer;
			WARN("semi-stub: SystemExecutionState\n"); /* Needed for .NET Framework, but using a FIXME is really noisy. */
			if (lpInputBuffer != NULL)
				return STATUS_INVALID_PARAMETER;
			/* FIXME: The actual state should be the value set by SetThreadExecutionState which is not currently implemented. */
			*ExecutionState = ES_USER_PRESENT;
			return STATUS_SUCCESS;
		}
		case ProcessorInformation: {
			const int cannedMHz = 1000; /* We fake a 1GHz processor if we can't conjure up real values */
			PROCESSOR_POWER_INFORMATION* cpu_power = lpOutputBuffer;
			int i, out_cpus;

			if ((lpOutputBuffer == NULL) || (nOutputBufferSize == 0))
				return STATUS_INVALID_PARAMETER;
			out_cpus = NtCurrentTeb()->Peb->NumberOfProcessors;
			if ((nOutputBufferSize / sizeof(PROCESSOR_POWER_INFORMATION)) < out_cpus)
				return STATUS_BUFFER_TOO_SMALL;
#if defined(linux)
			{
				char filename[128];
				FILE* f;

				for(i = 0; i < out_cpus; i++) {
					sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
					f = fopen(filename, "r");
					if (f && (fscanf(f, "%d", &cpu_power[i].CurrentMhz) == 1)) {
						cpu_power[i].CurrentMhz /= 1000;
						fclose(f);
					}
					else {
						if(i == 0) {
							cpu_power[0].CurrentMhz = mhz_from_cpuinfo();
							if(cpu_power[0].CurrentMhz == 0)
								cpu_power[0].CurrentMhz = cannedMHz;
						}
						else
							cpu_power[i].CurrentMhz = cpu_power[0].CurrentMhz;
						if(f) fclose(f);
					}

					sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
					f = fopen(filename, "r");
					if (f && (fscanf(f, "%d", &cpu_power[i].MaxMhz) == 1)) {
						cpu_power[i].MaxMhz /= 1000;
						fclose(f);
					}
					else {
						cpu_power[i].MaxMhz = cpu_power[i].CurrentMhz;
						if(f) fclose(f);
					}

					sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
					f = fopen(filename, "r");
					if(f && (fscanf(f, "%d", &cpu_power[i].MhzLimit) == 1)) {
						cpu_power[i].MhzLimit /= 1000;
						fclose(f);
					}
					else
					{
						cpu_power[i].MhzLimit = cpu_power[i].MaxMhz;
						if(f) fclose(f);
					}

					cpu_power[i].Number = i;
					cpu_power[i].MaxIdleState = 0;     /* FIXME */
					cpu_power[i].CurrentIdleState = 0; /* FIXME */
				}
			}
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__)
			{
				int num;
				size_t valSize = sizeof(num);
				if (sysctlbyname("hw.clockrate", &num, &valSize, NULL, 0))
					num = cannedMHz;
				for(i = 0; i < out_cpus; i++) {
					cpu_power[i].CurrentMhz = num;
					cpu_power[i].MaxMhz = num;
					cpu_power[i].MhzLimit = num;
					cpu_power[i].Number = i;
					cpu_power[i].MaxIdleState = 0;     /* FIXME */
					cpu_power[i].CurrentIdleState = 0; /* FIXME */
				}
			}
#elif defined (__APPLE__)
			{
				size_t valSize;
				unsigned long long currentMhz;
				unsigned long long maxMhz;

				valSize = sizeof(currentMhz);
				if (!sysctlbyname("hw.cpufrequency", &currentMhz, &valSize, NULL, 0))
					currentMhz /= 1000000;
				else
					currentMhz = cannedMHz;

				valSize = sizeof(maxMhz);
				if (!sysctlbyname("hw.cpufrequency_max", &maxMhz, &valSize, NULL, 0))
					maxMhz /= 1000000;
				else
					maxMhz = currentMhz;

				for(i = 0; i < out_cpus; i++) {
					cpu_power[i].CurrentMhz = currentMhz;
					cpu_power[i].MaxMhz = maxMhz;
					cpu_power[i].MhzLimit = maxMhz;
					cpu_power[i].Number = i;
					cpu_power[i].MaxIdleState = 0;     /* FIXME */
					cpu_power[i].CurrentIdleState = 0; /* FIXME */
				}
			}
#else
			for(i = 0; i < out_cpus; i++) {
				cpu_power[i].CurrentMhz = cannedMHz;
				cpu_power[i].MaxMhz = cannedMHz;
				cpu_power[i].MhzLimit = cannedMHz;
				cpu_power[i].Number = i;
				cpu_power[i].MaxIdleState = 0; /* FIXME */
				cpu_power[i].CurrentIdleState = 0; /* FIXME */
			}
			WARN("Unable to detect CPU MHz for this platform. Reporting %d MHz.\n", cannedMHz);
#endif
			for(i = 0; i < out_cpus; i++) {
				TRACE("cpu_power[%d] = %u %u %u %u %u %u\n", i, cpu_power[i].Number,
					  cpu_power[i].MaxMhz, cpu_power[i].CurrentMhz, cpu_power[i].MhzLimit,
					  cpu_power[i].MaxIdleState, cpu_power[i].CurrentIdleState);
			}
			return STATUS_SUCCESS;
		}
		default:
			/* FIXME: Needed by .NET Framework */
			WARN("Unimplemented NtPowerInformation action: %d\n", InformationLevel);
			return STATUS_NOT_IMPLEMENTED;
	}
}

/******************************************************************************
 *  NtShutdownSystem				[NTDLL.@]
 *
 */
NTSTATUS WINAPI NtShutdownSystem(SHUTDOWN_ACTION Action)
{
    FIXME("%d\n",Action);
    return STATUS_SUCCESS;
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

/******************************************************************************
 *  NtSystemDebugControl   (NTDLL.@)
 *  ZwSystemDebugControl   (NTDLL.@)
 */
NTSTATUS WINAPI NtSystemDebugControl(SYSDBG_COMMAND command, PVOID inbuffer, ULONG inbuflength, PVOID outbuffer,
                                     ULONG outbuflength, PULONG retlength)
{
    FIXME("(%d, %p, %d, %p, %d, %p), stub\n", command, inbuffer, inbuflength, outbuffer, outbuflength, retlength);

    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  NtSetLdtEntries   (NTDLL.@)
 *  ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries(ULONG selector1, ULONG entry1_low, ULONG entry1_high,
                                ULONG selector2, ULONG entry2_low, ULONG entry2_high)
{
    FIXME("(%u, %u, %u, %u, %u, %u): stub\n", selector1, entry1_low, entry1_high, selector2, entry2_low, entry2_high);

    return STATUS_NOT_IMPLEMENTED;
}
