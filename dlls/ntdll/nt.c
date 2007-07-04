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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"

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

    TRACE("(%p,0x%08x,%p,0x%08x,0x%08x,%p)\n",
        ExistingToken, DesiredAccess, ObjectAttributes,
        ImpersonationLevel, TokenType, NewToken);
        dump_ObjectAttributes(ObjectAttributes);

    SERVER_START_REQ( duplicate_token )
    {
        req->handle              = ExistingToken;
        req->access              = DesiredAccess;
        req->attributes          = ObjectAttributes ? ObjectAttributes->Attributes : 0;
        req->primary             = (TokenType == TokenPrimary);
        req->impersonation_level = ImpersonationLevel;
        status = wine_server_call( req );
        if (!status) *NewToken = reply->new_handle;
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
    NTSTATUS ret;

    TRACE("(%p,0x%08x,%p)\n", ProcessHandle,DesiredAccess, TokenHandle);

    SERVER_START_REQ( open_token )
    {
        req->handle     = ProcessHandle;
        req->access     = DesiredAccess;
        req->attributes = 0;
        req->flags      = 0;
        ret = wine_server_call( req );
        if (!ret) *TokenHandle = reply->token;
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
    NTSTATUS ret;

    TRACE("(%p,0x%08x,0x%08x,%p)\n",
          ThreadHandle,DesiredAccess, OpenAsSelf, TokenHandle);

    SERVER_START_REQ( open_token )
    {
        req->handle     = ThreadHandle;
        req->access     = DesiredAccess;
        req->attributes = 0;
        req->flags      = OPEN_TOKEN_THREAD;
        if (OpenAsSelf) req->flags |= OPEN_TOKEN_AS_SELF;
        ret = wine_server_call( req );
        if (!ret) *TokenHandle = reply->token;
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
        req->handle = TokenHandle;
        req->disable_all = DisableAllPrivileges;
        req->get_modified_state = (PreviousState != NULL);
        if (!DisableAllPrivileges)
        {
            wine_server_add_data( req, &NewState->Privileges,
                                  NewState->PrivilegeCount * sizeof(NewState->Privileges[0]) );
        }
        if (PreviousState && BufferLength >= FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ))
            wine_server_set_reply( req, &PreviousState->Privileges,
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
    ULONG len;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE("(%p,%d,%p,%d,%p)\n",
          token,tokeninfoclass,tokeninfo,tokeninfolength,retlen);

    switch (tokeninfoclass)
    {
    case TokenOwner:
        len = sizeof(TOKEN_OWNER) + sizeof(SID);
        break;
    case TokenPrimaryGroup:
        len = sizeof(TOKEN_PRIMARY_GROUP);
        break;
    case TokenDefaultDacl:
        len = sizeof(TOKEN_DEFAULT_DACL);
        break;
    case TokenSource:
        len = sizeof(TOKEN_SOURCE);
        break;
    case TokenType:
        len = sizeof (TOKEN_TYPE);
        break;
    case TokenImpersonationLevel:
        len = sizeof(SECURITY_IMPERSONATION_LEVEL);
        break;
#if 0
    case TokenStatistics:
#endif /* 0 */
    default:
        len = 0;
    }

    if (retlen) *retlen = len;

    if (tokeninfolength < len)
        return STATUS_BUFFER_TOO_SMALL;

    switch (tokeninfoclass)
    {
    case TokenUser:
        SERVER_START_REQ( get_token_user )
        {
            TOKEN_USER * tuser = tokeninfo;
            PSID sid = (PSID) (tuser + 1);
            DWORD sid_len = tokeninfolength < sizeof(TOKEN_USER) ? 0 : tokeninfolength - sizeof(TOKEN_USER);

            req->handle = token;
            wine_server_set_reply( req, sid, sid_len );
            status = wine_server_call( req );
            if (retlen) *retlen = reply->user_len + sizeof(TOKEN_USER);
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
        char stack_buffer[256];
        unsigned int server_buf_len = sizeof(stack_buffer);
        void *buffer = stack_buffer;
        BOOLEAN need_more_memory = FALSE;

        /* we cannot work out the size of the server buffer required for the
         * input size, since there are two factors affecting how much can be
         * stored in the buffer - number of groups and lengths of sids */
        do
        {
            SERVER_START_REQ( get_token_groups )
            {
                TOKEN_GROUPS *groups = tokeninfo;

                req->handle = token;
                wine_server_set_reply( req, buffer, server_buf_len );
                status = wine_server_call( req );
                if (status == STATUS_BUFFER_TOO_SMALL)
                {
                    if (buffer == stack_buffer)
                        buffer = RtlAllocateHeap(GetProcessHeap(), 0, reply->user_len);
                    else
                        buffer = RtlReAllocateHeap(GetProcessHeap(), 0, buffer, reply->user_len);
                    if (!buffer) return STATUS_NO_MEMORY;

                    server_buf_len = reply->user_len;
                    need_more_memory = TRUE;
                }
                else if (status == STATUS_SUCCESS)
                {
                    struct token_groups *tg = buffer;
                    unsigned int *attr = (unsigned int *)(tg + 1);
                    ULONG i;
                    const int non_sid_portion = (sizeof(struct token_groups) + tg->count * sizeof(unsigned long));
                    SID *sids = (SID *)((char *)tokeninfo + FIELD_OFFSET( TOKEN_GROUPS, Groups[tg->count] ));
                    ULONG needed_bytes = FIELD_OFFSET( TOKEN_GROUPS, Groups[tg->count] ) +
                        reply->user_len - non_sid_portion;

                    if (retlen) *retlen = needed_bytes;

                    if (needed_bytes <= tokeninfolength)
                    {
                        groups->GroupCount = tg->count;
                        memcpy( sids, (char *)buffer + non_sid_portion,
                                reply->user_len - non_sid_portion );

                        for (i = 0; i < tg->count; i++)
                        {
                            groups->Groups[i].Attributes = attr[i];
                            groups->Groups[i].Sid = sids;
                            sids = (SID *)((char *)sids + RtlLengthSid(sids));
                        }
                    }
                    else status = STATUS_BUFFER_TOO_SMALL;
                }
                else if (retlen) *retlen = 0;
            }
            SERVER_END_REQ;
        } while (need_more_memory);
        if (buffer != stack_buffer) RtlFreeHeap(GetProcessHeap(), 0, buffer);
        break;
    }
    case TokenPrimaryGroup:
        if (tokeninfo)
        {
            TOKEN_PRIMARY_GROUP *tgroup = tokeninfo;
            SID_IDENTIFIER_AUTHORITY sid = {SECURITY_NT_AUTHORITY};
            RtlAllocateAndInitializeSid( &sid,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &(tgroup->PrimaryGroup));
        }
        break;
    case TokenPrivileges:
        SERVER_START_REQ( get_token_privileges )
        {
            TOKEN_PRIVILEGES *tpriv = tokeninfo;
            req->handle = token;
            if (tpriv && tokeninfolength > FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ))
                wine_server_set_reply( req, &tpriv->Privileges, tokeninfolength - FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) );
            status = wine_server_call( req );
            if (retlen) *retlen = FIELD_OFFSET( TOKEN_PRIVILEGES, Privileges ) + reply->len;
            if (tpriv) tpriv->PrivilegeCount = reply->len / sizeof(LUID_AND_ATTRIBUTES);
        }
        SERVER_END_REQ;
        break;
    case TokenOwner:
        if (tokeninfo)
        {
            TOKEN_OWNER *owner = tokeninfo;
            PSID sid = (PSID) (owner + 1);
            SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};
            RtlInitializeSid(sid, &localSidAuthority, 1);
            *(RtlSubAuthoritySid(sid, 0)) = SECURITY_INTERACTIVE_RID;
            owner->Owner = sid;
        }
        break;
    case TokenImpersonationLevel:
        SERVER_START_REQ( get_token_impersonation_level )
        {
            SECURITY_IMPERSONATION_LEVEL *impersonation_level = tokeninfo;
            req->handle = token;
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
                *impersonation_level = reply->impersonation_level;
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
    FIXME("%p %d %p %u\n", TokenHandle, TokenInformationClass,
          TokenInformation, TokenInformationLength);
    return STATUS_NOT_IMPLEMENTED;
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
        req->handle = ClientToken;
        req->all_required = ((RequiredPrivileges->Control & PRIVILEGE_SET_ALL_NECESSARY) ? TRUE : FALSE);
        wine_server_add_data( req, &RequiredPrivileges->Privilege,
            RequiredPrivileges->PrivilegeCount * sizeof(RequiredPrivileges->Privilege[0]) );
        wine_server_set_reply( req, &RequiredPrivileges->Privilege,
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
    TRACE("\tMessageId           = %u\n",pLpcMessageIn->MessageId);
    TRACE("\tSectionSize         = %u\n",pLpcMessageIn->SectionSize);
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

            sbi.dwUnknown1 = 0;
            sbi.uKeMaximumIncrement = 0;
            sbi.uPageSize = 1024; /* FIXME */
            sbi.uMmNumberOfPhysicalPages = 12345; /* FIXME */
            sbi.uMmLowestPhysicalPage = 0; /* FIXME */
            sbi.uMmHighestPhysicalPage = 12345; /* FIXME */
            sbi.uAllocationGranularity = 65536; /* FIXME */
            sbi.pLowestUserAddress = 0; /* FIXME */
            sbi.pMmHighestUserAddress = (void*)~0; /* FIXME */
            sbi.uKeActiveProcessors = 1; /* FIXME */
            sbi.bKeNumberProcessors = 1; /* FIXME */
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
        {
            SYSTEM_CPU_INFORMATION sci;

            /* FIXME: move some code from kernel/cpu.c to process this */
            sci.Architecture = PROCESSOR_ARCHITECTURE_INTEL;
            sci.Level = 6; /* 686, aka Pentium II+ */
            sci.Revision = 0;
            sci.Reserved = 0;
            sci.FeatureSet = 0x1fff;
            len = sizeof(sci);

            if ( Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &sci, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemPerformanceInformation:
        {
            SYSTEM_PERFORMANCE_INFORMATION spi;

            memset(&spi, 0 , sizeof(spi));
            len = sizeof(spi);

            if (Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &spi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
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
            SYSTEM_PROCESS_INFORMATION* spi = (SYSTEM_PROCESS_INFORMATION*)SystemInformation;
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
                req->pid        = 0;
                if (!(ret = wine_server_call( req ))) hSnap = reply->handle;
            }
            SERVER_END_REQ;
            len = 0;
            while (ret == STATUS_SUCCESS)
            {
                SERVER_START_REQ( next_process )
                {
                    req->handle = hSnap;
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

                            spi->dwOffset = procstructlen - wlen;
                            spi->dwThreadCount = reply->threads;

                            /* spi->pszProcessName will be set later on */

                            spi->dwBasePriority = reply->priority;
                            spi->dwProcessID = (DWORD)reply->pid;
                            spi->dwParentProcessID = (DWORD)reply->ppid;
                            spi->dwHandleCount = reply->handles;

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
                            req->handle = hSnap;
                            req->reset = (j == 0);
                            if (!(ret = wine_server_call( req )))
                            {
                                j++;
                                if (reply->pid == spi->dwProcessID)
                                {
                                    /* ftKernelTime, ftUserTime, ftCreateTime;
                                     * dwTickCount, dwStartAddress
                                     */

                                    memset(&spi->ti[i], 0, sizeof(spi->ti));

                                    spi->ti[i].dwOwningPID = reply->pid;
                                    spi->ti[i].dwThreadID  = reply->tid;
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
                    spi->ProcessName.Buffer = (WCHAR*)((char*)spi + spi->dwOffset);
                    spi->ProcessName.Length = wlen - sizeof(WCHAR);
                    spi->ProcessName.MaximumLength = wlen;
                    memcpy( spi->ProcessName.Buffer, exename, wlen );
                    spi->dwOffset += wlen;

                    last = spi;
                    spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + spi->dwOffset);
                }
            }
            if (ret == STATUS_SUCCESS && last) last->dwOffset = 0;
            if (hSnap) NtClose(hSnap);
        }
        break;
    case SystemProcessorPerformanceInformation:
        {
            SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION sppi;

            memset(&sppi, 0 , sizeof(sppi)); /* FIXME */
            len = sizeof(sppi);

            if (Length >= len)
            {
                if (!SystemInformation) ret = STATUS_ACCESS_VIOLATION;
                else memcpy( SystemInformation, &sppi, len);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
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
			PSYSTEM_POWER_CAPABILITIES PowerCaps = (PSYSTEM_POWER_CAPABILITIES)lpOutputBuffer;
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
		default:
			FIXME("Unimplemented NtPowerInformation action: %d\n", InformationLevel);
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
