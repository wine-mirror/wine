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
            *ReturnLength = reply->len + FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges );
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
        0,    /* TokenIntegrityLevel */
        0,    /* TokenUIAccess */
        0,    /* TokenMandatoryPolicy */
        0     /* TokenLogonSid */
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
        req->all_required = ((RequiredPrivileges->Control & PRIVILEGE_SET_ALL_NECESSARY) ? TRUE : FALSE);
        wine_server_add_data( req, RequiredPrivileges->Privilege,
            RequiredPrivileges->PrivilegeCount * sizeof(RequiredPrivileges->Privilege[0]) );
        wine_server_set_reply( req, RequiredPrivileges->Privilege,
            RequiredPrivileges->PrivilegeCount * sizeof(RequiredPrivileges->Privilege[0]) );

        status = wine_server_call( req );

        if (status == STATUS_SUCCESS)
            *Result = (reply->has_privileges ? TRUE : FALSE);
    }
    SERVER_END_REQ;
    return status;
}

/*
 *	Section
 */

/******************************************************************************
 *  NtQuerySection	[NTDLL.@]
 */
NTSTATUS WINAPI NtQuerySection(
	IN HANDLE SectionHandle,
	IN SECTION_INFORMATION_CLASS SectionInformationClass,
	OUT PVOID SectionInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	FIXME("(%p,%d,%p,0x%08x,%p) stub!\n",
	SectionHandle,SectionInformationClass,SectionInformation,Length,ResultLength);
	return 0;
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
static  ULONGLONG cpuHz = 1000000000; /* default to a 1GHz */

#define AUTH	0x68747541	/* "Auth" */
#define ENTI	0x69746e65	/* "enti" */
#define CAMD	0x444d4163	/* "cAMD" */

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
#endif
}

/* From xf86info havecpuid.c 1.11 */
static inline int have_cpuid(void)
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
#else
        return 0;
#endif
}

static inline void get_cpuinfo(SYSTEM_CPU_INFORMATION* info)
{
    unsigned int regs[4], regs2[4];

    if (!have_cpuid()) return;

    do_cpuid(0x00000000, regs);  /* get standard cpuid level and vendor name */
    if (regs[0]>=0x00000001)   /* Check for supported cpuid version */
    {
        do_cpuid(0x00000001, regs2); /* get cpu features */
        switch ((regs2[0] >> 8) & 0xf)  /* cpu family */
        {
        case 3: info->Level = 3;        break;
        case 4: info->Level = 4;        break;
        case 5: info->Level = 5;        break;
        case 15: /* PPro/2/3/4 has same info as P1 */
        case 6: info->Level = 6;         break;
        default:
            FIXME("unknown cpu family %d, please report! (-> setting to 386)\n",
                  (regs2[0] >> 8)&0xf);
            info->Level = 3;
            break;
        }
        user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED]       = !(regs2[3] & 1);
        user_shared_data->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE]   = (regs2[3] & (1 << 4 )) >> 4;
        user_shared_data->ProcessorFeatures[PF_PAE_ENABLED]                   = (regs2[3] & (1 << 6 )) >> 6;
        user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE]       = (regs2[3] & (1 << 8 )) >> 8;
        user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE]    = (regs2[3] & (1 << 23)) >> 23;
        user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE]   = (regs2[3] & (1 << 25)) >> 25;
        user_shared_data->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = (regs2[3] & (1 << 26)) >> 26;

        if (regs[1] == AUTH && regs[3] == ENTI && regs[2] == CAMD)
        {
            do_cpuid(0x80000000, regs);  /* get vendor cpuid level */
            if (regs[0] >= 0x80000001)
            {
                do_cpuid(0x80000001, regs2);  /* get vendor features */
                user_shared_data->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = (regs2[3] & (1 << 31 )) >> 31;
            }
        }
    }
}

/******************************************************************
 *		fill_cpu_info
 *
 * inits a couple of places with CPU related information:
 * - cached_sci & cpuHZ in this file
 * - Peb->NumberOfProcessors
 * - SharedUserData->ProcessFeatures[] array
 *
 * It creates a registry subhierarchy, looking like:
 * "\HARDWARE\DESCRIPTION\System\CentralProcessor\<processornumber>\Identifier (CPU x86)".
 * Note that there is a hierarchy for every processor installed, so this
 * supports multiprocessor systems. This is done like Win95 does it, I think.
 *
 * It creates some registry entries in the environment part:
 * "\HKLM\System\CurrentControlSet\Control\Session Manager\Environment". These are
 * always present. When deleted, Windows will add them again.
 */
void fill_cpu_info(void)
{
    memset(&cached_sci, 0, sizeof(cached_sci));
    /* choose sensible defaults ...
     * FIXME: perhaps overridable with precompiler flags?
     */
#ifdef __i386__
    cached_sci.Architecture     = PROCESSOR_ARCHITECTURE_INTEL;
    cached_sci.Level		= 5; /* 586 */
#elif defined(__x86_64__)
    cached_sci.Architecture     = PROCESSOR_ARCHITECTURE_AMD64;
#elif defined(__powerpc__)
    cached_sci.Architecture     = PROCESSOR_ARCHITECTURE_PPC;
#elif defined(__arm__)
    cached_sci.Architecture     = PROCESSOR_ARCHITECTURE_ARM;
#elif defined(__sparc__)
    cached_sci.Architecture     = PROCESSOR_ARCHITECTURE_SPARC;
#else
#error Unknown CPU
#endif
    cached_sci.Revision   = 0;
    cached_sci.Reserved   = 0;
    cached_sci.FeatureSet = 0x1fff;

    NtCurrentTeb()->Peb->NumberOfProcessors = 1;

    /* Hmm, reasonable processor feature defaults? */

#ifdef linux
    {
	char line[200];
	FILE *f = fopen ("/proc/cpuinfo", "r");

	if (!f)
		return;
	while (fgets(line,200,f) != NULL)
        {
            char	*s,*value;

            /* NOTE: the ':' is the only character we can rely on */
            if (!(value = strchr(line,':')))
                continue;

            /* terminate the valuename */
            s = value - 1;
            while ((s >= line) && ((*s == ' ') || (*s == '\t'))) s--;
            *(s + 1) = '\0';

            /* and strip leading spaces from value */
            value += 1;
            while (*value==' ') value++;
            if ((s = strchr(value,'\n')))
                *s='\0';

            if (!strcasecmp(line, "processor"))
            {
                /* processor number counts up... */
                unsigned int x;

                if (sscanf(value, "%d",&x))
                    if (x + 1 > NtCurrentTeb()->Peb->NumberOfProcessors)
                        NtCurrentTeb()->Peb->NumberOfProcessors = x + 1;

                continue;
            }
            if (!strcasecmp(line, "model"))
            {
                /* First part of Revision */
                int	x;

                if (sscanf(value, "%d",&x))
                    cached_sci.Revision = cached_sci.Revision | (x << 8);

                continue;
            }

            /* 2.1 and ARM method */
            if (!strcasecmp(line, "cpu family") || !strcasecmp(line, "CPU architecture"))
            {
                if (isdigit(value[0]))
                {
                    cached_sci.Level = atoi(value);
                }
                continue;
            }
            /* old 2.0 method */
            if (!strcasecmp(line, "cpu"))
            {
                if (isdigit(value[0]) && value[1] == '8' && value[2] == '6' && value[3] == 0)
                {
                    switch (cached_sci.Level = value[0] - '0')
                    {
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                        break;
                    default:
                        FIXME("unknown Linux 2.0 cpu family '%s', please report ! (-> setting to 386)\n", value);
                        cached_sci.Level = 3;
                        break;
                    }
                }
                continue;
            }
            if (!strcasecmp(line, "stepping"))
            {
                /* Second part of Revision */
                int	x;

                if (sscanf(value, "%d",&x))
                    cached_sci.Revision = cached_sci.Revision | x;
                continue;
            }
            if (!strcasecmp(line, "cpu MHz"))
            {
                double cmz;
                if (sscanf( value, "%lf", &cmz ) == 1)
                {
                    /* SYSTEMINFO doesn't have a slot for cpu speed, so store in a global */
                    cpuHz = cmz * 1000 * 1000;
                }
                continue;
            }
            if (!strcasecmp(line, "fdiv_bug"))
            {
                if (!strncasecmp(value, "yes",3))
                    user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = TRUE;
                continue;
            }
            if (!strcasecmp(line, "fpu"))
            {
                if (!strncasecmp(value, "no",2))
                    user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = TRUE;
                continue;
            }
            if (!strcasecmp(line, "flags") || !strcasecmp(line, "features"))
            {
                if (strstr(value, "cx8"))
                    user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
                if (strstr(value, "cx16"))
                    user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE128] = TRUE;
                if (strstr(value, "mmx"))
                    user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
                if (strstr(value, "nx"))
                    user_shared_data->ProcessorFeatures[PF_NX_ENABLED] = TRUE;
                if (strstr(value, "tsc"))
                    user_shared_data->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
                if (strstr(value, "3dnow"))
                {
                    user_shared_data->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = TRUE;
                    cached_sci.FeatureSet |= CPU_FEATURE_3DNOW;
                }
                /* This will also catch sse2, but we have sse itself
                 * if we have sse2, so no problem */
                if (strstr(value, "sse"))
                {
                    user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
                    cached_sci.FeatureSet |= CPU_FEATURE_SSE;
                }
                if (strstr(value, "sse2"))
                {
                    user_shared_data->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;
                    cached_sci.FeatureSet |= CPU_FEATURE_SSE2;
                }
                if (strstr(value, "pni"))
                    user_shared_data->ProcessorFeatures[PF_SSE3_INSTRUCTIONS_AVAILABLE] = TRUE;
                if (strstr(value, "pae"))
                    user_shared_data->ProcessorFeatures[PF_PAE_ENABLED] = TRUE;
                if (strstr(value, "ht"))
                    cached_sci.FeatureSet |= CPU_FEATURE_HTT;
                continue;
            }
	}
	fclose(f);
    }
#elif defined (__NetBSD__)
    {
        int mib[2];
        int value;
        size_t val_len;
        char model[256];
        char *cpuclass;
        FILE *f = fopen("/var/run/dmesg.boot", "r");

        /* first deduce as much as possible from the sysctls */
        mib[0] = CTL_MACHDEP;
#ifdef CPU_FPU_PRESENT
        mib[1] = CPU_FPU_PRESENT;
        val_len = sizeof(value);
        if (sysctl(mib, 2, &value, &val_len, NULL, 0) >= 0)
            user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = !value;
#endif
#ifdef CPU_SSE
        mib[1] = CPU_SSE;   /* this should imply MMX */
        val_len = sizeof(value);
        if (sysctl(mib, 2, &value, &val_len, NULL, 0) >= 0)
            if (value) user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
#endif
#ifdef CPU_SSE2
        mib[1] = CPU_SSE2;  /* this should imply MMX */
        val_len = sizeof(value);
        if (sysctl(mib, 2, &value, &val_len, NULL, 0) >= 0)
            if (value) user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
#endif
        mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
        val_len = sizeof(value);
        if (sysctl(mib, 2, &value, &val_len, NULL, 0) >= 0)
            if (value > NtCurrentTeb()->Peb->NumberOfProcessors)
                NtCurrentTeb()->Peb->NumberOfProcessors = value;
        mib[1] = HW_MODEL;
        val_len = sizeof(model)-1;
        if (sysctl(mib, 2, model, &val_len, NULL, 0) >= 0)
        {
            model[val_len] = '\0'; /* just in case */
            cpuclass = strstr(model, "-class");
            if (cpuclass != NULL) {
                while(cpuclass > model && cpuclass[0] != '(') cpuclass--;
                if (!strncmp(cpuclass+1, "386", 3))
                {
                    cached_sci.Level= 3;
                }
                if (!strncmp(cpuclass+1, "486", 3))
                {
                    cached_sci.Level= 4;
                }
                if (!strncmp(cpuclass+1, "586", 3))
                {
                    cached_sci.Level= 5;
                }
                if (!strncmp(cpuclass+1, "686", 3))
                {
                    cached_sci.Level= 6;
                    /* this should imply MMX */
                    user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
                }
            }
        }

        /* it may be worth reading from /var/run/dmesg.boot for
           additional information such as CX8, MMX and TSC
           (however this information should be considered less
           reliable than that from the sysctl calls) */
        if (f != NULL)
        {
            while (fgets(model, 255, f) != NULL)
            {
                int cpu, features;
                if (sscanf(model, "cpu%d: features %x<", &cpu, &features) == 2)
                {
                    /* we could scan the string but it is easier
                       to test the bits directly */
                    if (features & 0x1)
                        user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = TRUE;
                    if (features & 0x10)
                        user_shared_data->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
                    if (features & 0x100)
                        user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
                    if (features & 0x800000)
                        user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;

                    break;
                }
            }
            fclose(f);
        }
    }
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__)
    {
        int ret, num;
        size_t len;

        get_cpuinfo( &cached_sci );

        /* Check for OS support of SSE -- Is this used, and should it be sse1 or sse2? */
        /*len = sizeof(num);
          ret = sysctlbyname("hw.instruction_sse", &num, &len, NULL, 0);
          if (!ret)
          user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = num;*/

        len = sizeof(num);
        ret = sysctlbyname("hw.ncpu", &num, &len, NULL, 0);
        if (!ret)
            NtCurrentTeb()->Peb->NumberOfProcessors = num;

        len = sizeof(num);
        if (!sysctlbyname("hw.clockrate", &num, &len, NULL, 0))
            cpuHz = num * 1000 * 1000;
    }
#elif defined(__sun)
    {
        int num = sysconf( _SC_NPROCESSORS_ONLN );

        if (num == -1) num = 1;
        get_cpuinfo( &cached_sci );
        NtCurrentTeb()->Peb->NumberOfProcessors = num;
    }
#elif defined (__OpenBSD__)
    {
        int mib[2], num, ret;
        size_t len;

        mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
        len = sizeof(num);

        ret = sysctl(mib, 2, &num, &len, NULL, 0);
        if (!ret)
            NtCurrentTeb()->Peb->NumberOfProcessors = num;
    }
#elif defined (__APPLE__)
    {
        size_t valSize;
        unsigned long long longVal;
        int value;
        int cputype;
        char buffer[1024];

        valSize = sizeof(int);
        if (sysctlbyname ("hw.optional.floatingpoint", &value, &valSize, NULL, 0) == 0)
        {
            if (value)
                user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = FALSE;
            else
                user_shared_data->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = TRUE;
        }
        valSize = sizeof(int);
        if (sysctlbyname ("hw.ncpu", &value, &valSize, NULL, 0) == 0)
            NtCurrentTeb()->Peb->NumberOfProcessors = value;

        /* FIXME: we don't use the "hw.activecpu" value... but the cached one */

        valSize = sizeof(int);
        if (sysctlbyname ("hw.cputype", &cputype, &valSize, NULL, 0) == 0)
        {
            switch (cputype)
            {
            case CPU_TYPE_POWERPC:
                cached_sci.Architecture = PROCESSOR_ARCHITECTURE_PPC;
                valSize = sizeof(int);
                if (sysctlbyname ("hw.cpusubtype", &value, &valSize, NULL, 0) == 0)
                {
                    switch (value)
                    {
                    case CPU_SUBTYPE_POWERPC_601:
                    case CPU_SUBTYPE_POWERPC_602:       cached_sci.Level = 1;   break;
                    case CPU_SUBTYPE_POWERPC_603:       cached_sci.Level = 3;   break;
                    case CPU_SUBTYPE_POWERPC_603e:
                    case CPU_SUBTYPE_POWERPC_603ev:     cached_sci.Level = 6;   break;
                    case CPU_SUBTYPE_POWERPC_604:       cached_sci.Level = 4;   break;
                    case CPU_SUBTYPE_POWERPC_604e:      cached_sci.Level = 9;   break;
                    case CPU_SUBTYPE_POWERPC_620:       cached_sci.Level = 20;  break;
                    case CPU_SUBTYPE_POWERPC_750:       /* G3/G4 derive from 603 so ... */
                    case CPU_SUBTYPE_POWERPC_7400:
                    case CPU_SUBTYPE_POWERPC_7450:      cached_sci.Level = 6;   break;
                    case CPU_SUBTYPE_POWERPC_970:       cached_sci.Level = 9;
                        /* :o) user_shared_data->ProcessorFeatures[PF_ALTIVEC_INSTRUCTIONS_AVAILABLE] ;-) */
                        break;
                    default: break;
                    }
                }
                break; /* CPU_TYPE_POWERPC */
            case CPU_TYPE_I386:
                cached_sci.Architecture = PROCESSOR_ARCHITECTURE_INTEL;
                valSize = sizeof(int);
                if (sysctlbyname ("machdep.cpu.family", &value, &valSize, NULL, 0) == 0)
                {
                    cached_sci.Level = value;
                }
                valSize = sizeof(int);
                if (sysctlbyname ("machdep.cpu.model", &value, &valSize, NULL, 0) == 0)
                    cached_sci.Revision = (value << 8);
                valSize = sizeof(int);
                if (sysctlbyname ("machdep.cpu.stepping", &value, &valSize, NULL, 0) == 0)
                    cached_sci.Revision |= value;
                valSize = sizeof(buffer);
                if (sysctlbyname ("machdep.cpu.features", buffer, &valSize, NULL, 0) == 0)
                {
                    if (!valSize) FIXME("Buffer not large enough, please increase\n");
                    if (strstr(buffer, "CX8"))   user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
                    if (strstr(buffer, "CX16"))  user_shared_data->ProcessorFeatures[PF_COMPARE_EXCHANGE128] = TRUE;
                    if (strstr(buffer, "MMX"))   user_shared_data->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer, "TSC"))   user_shared_data->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
                    if (strstr(buffer, "3DNOW")) user_shared_data->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer, "SSE"))   user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer, "SSE2"))  user_shared_data->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer, "SSE3"))  user_shared_data->ProcessorFeatures[PF_SSE3_INSTRUCTIONS_AVAILABLE] = TRUE;
                    if (strstr(buffer, "PAE"))   user_shared_data->ProcessorFeatures[PF_PAE_ENABLED] = TRUE;
                    if (strstr(buffer, "HTT"))   cached_sci.FeatureSet |= CPU_FEATURE_HTT;
                }
                break; /* CPU_TYPE_I386 */
            default: break;
            } /* switch (cputype) */
        }
        valSize = sizeof(longVal);
        if (!sysctlbyname("hw.cpufrequency", &longVal, &valSize, NULL, 0))
            cpuHz = longVal;
    }
#else
    FIXME("not yet supported on this system\n");
#endif
    TRACE("<- CPU arch %d, level %d, rev %d, features 0x%x\n",
          cached_sci.Architecture, cached_sci.Level, cached_sci.Revision, cached_sci.FeatureSet);
}

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

                            len += procstructlen;
                        }
                        else ret = STATUS_INFO_LENGTH_MISMATCH;
                    }
                }
                SERVER_END_REQ;
 
                if (ret != STATUS_SUCCESS)
                {
                    if (ret == STATUS_NO_MORE_FILES) ret = STATUS_SUCCESS;
                    break;
                }
                else /* Length is already checked for */
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
                int n;
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
            SYSTEM_HANDLE_INFORMATION shi;

            memset(&shi, 0, sizeof(shi));
            len = sizeof(shi);

            if ( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &shi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
            FIXME("info_class SYSTEM_HANDLE_INFORMATION\n");
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
			PPROCESSOR_POWER_INFORMATION cpu_power = lpOutputBuffer;

			WARN("semi-stub: ProcessorInformation\n");
			if (nOutputBufferSize < sizeof(PROCESSOR_POWER_INFORMATION))
				return STATUS_BUFFER_TOO_SMALL;
                        cpu_power->Number = NtCurrentTeb()->Peb->NumberOfProcessors;
                        cpu_power->MaxMhz = cpuHz / 1000000;
                        cpu_power->CurrentMhz = cpuHz / 1000000;
                        cpu_power->MhzLimit = cpuHz / 1000000;
                        cpu_power->MaxIdleState = 0; /* FIXME */
                        cpu_power->CurrentIdleState = 0; /* FIXME */
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
