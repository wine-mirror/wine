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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wine/debug.h"

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

/* Structures used by NtConnectPort */

typedef struct LpcSectionInfo
{
  DWORD Length;
  HANDLE SectionHandle;
  DWORD Param1;
  DWORD SectionSize;
  DWORD ClientBaseAddress;
  DWORD ServerBaseAddress;
} LPCSECTIONINFO, *PLPCSECTIONINFO;

typedef struct LpcSectionMapInfo
{
  DWORD Length;
  DWORD SectionSize;
  DWORD ServerBaseAddress;
} LPCSECTIONMAPINFO, *PLPCSECTIONMAPINFO;

/* Structure used by NtAcceptConnectPort, NtReplyWaitReceivePort */

#define MAX_MESSAGE_DATA 328

typedef struct LpcMessage
{
  WORD ActualMessageLength;
  WORD TotalMessageLength;
  DWORD MessageType;
  DWORD ClientProcessId;
  DWORD ClientThreadId;
  DWORD MessageId;
  DWORD SharedSectionSize;
  BYTE MessageData[MAX_MESSAGE_DATA];
} LPCMESSAGE, *PLPCMESSAGE;

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
	FIXME("(%p,0x%08lx,%p,0x%08x,0x%08x,%p),stub!\n",
	ExistingToken, DesiredAccess, ObjectAttributes,
	ImpersonationLevel, TokenType, NewToken);
	dump_ObjectAttributes(ObjectAttributes);
	return 0;
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

    TRACE("(%p,0x%08lx,%p)\n", ProcessHandle,DesiredAccess, TokenHandle);

    SERVER_START_REQ( open_token )
    {
        req->handle = ProcessHandle;
        req->flags  = 0;
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

    TRACE("(%p,0x%08lx,0x%08x,%p)\n",
          ThreadHandle,DesiredAccess, OpenAsSelf, TokenHandle);

    SERVER_START_REQ( open_token )
    {
        req->handle = ThreadHandle;
        req->flags  = OPEN_TOKEN_THREAD;
        if (OpenAsSelf) req->flags |= OPEN_TOKEN_AS_SELF;
        ret = wine_server_call( req );
        if (!ret) *TokenHandle = reply->token;
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
 *  NtAdjustPrivilegesToken		[NTDLL.@]
 *  ZwAdjustGroupsToken		[NTDLL.@]
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
	FIXME("(%p,0x%08x,%p,0x%08lx,%p,%p),stub!\n",
	TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
	return 0;
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
	DWORD tokeninfoclass,
	LPVOID tokeninfo,
	DWORD tokeninfolength,
	LPDWORD retlen )
{
    unsigned int len = 0;

    FIXME("(%p,%ld,%p,%ld,%p): stub\n",
          token,tokeninfoclass,tokeninfo,tokeninfolength,retlen);

    switch (tokeninfoclass)
    {
    case TokenUser:
        len = sizeof(TOKEN_USER) + sizeof(SID);
        break;
    case TokenGroups:
        len = sizeof(TOKEN_GROUPS);
        break;
    case TokenPrivileges:
        len = sizeof(TOKEN_PRIVILEGES);
        break;
    case TokenOwner:
        len = sizeof(TOKEN_OWNER);
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
#if 0
    case TokenImpersonationLevel:
    case TokenStatistics:
#endif /* 0 */
    }

    /* FIXME: what if retlen == NULL ? */
    *retlen = len;

    if (tokeninfolength < len)
        return STATUS_INFO_LENGTH_MISMATCH;

    switch (tokeninfoclass)
    {
    case TokenUser:
        if( tokeninfo )
        {
            TOKEN_USER * tuser = tokeninfo;
            PSID sid = (PSID) (tuser + 1);
            SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};
            RtlInitializeSid(sid, &localSidAuthority, 1);
            *(RtlSubAuthoritySid(sid, 0)) = SECURITY_INTERACTIVE_RID;
            tuser->User.Sid = sid;
        }
        break;
    case TokenGroups:
        if (tokeninfo)
        {
            TOKEN_GROUPS *tgroups = tokeninfo;
            SID_IDENTIFIER_AUTHORITY sid = {SECURITY_NT_AUTHORITY};

            /* we need to show admin privileges ! */
            tgroups->GroupCount = 1;
            tgroups->Groups->Attributes = SE_GROUP_ENABLED;
            RtlAllocateAndInitializeSid( &sid,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &(tgroups->Groups->Sid));
        }
        break;
    case TokenPrivileges:
        if (tokeninfo)
        {
            TOKEN_PRIVILEGES *tpriv = tokeninfo;
            tpriv->PrivilegeCount = 1;
        }
        break;
    }
    return 0;
}

/*
 *	Section
 */

/******************************************************************************
 *  NtQuerySection	[NTDLL.@]
 */
NTSTATUS WINAPI NtQuerySection(
	IN HANDLE SectionHandle,
	IN PVOID SectionInformationClass,
	OUT PVOID SectionInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
	FIXME("(%p,%p,%p,0x%08lx,%p) stub!\n",
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
                             DWORD MaxConnectInfoLength,DWORD MaxDataLength,DWORD unknown)
{
  FIXME("(%p,%p,0x%08lx,0x%08lx,0x%08lx),stub!\n",PortHandle,ObjectAttributes,
        MaxConnectInfoLength,MaxDataLength,unknown);
  return 0;
}

/******************************************************************************
 *  NtConnectPort		[NTDLL.@]
 *  ZwConnectPort		[NTDLL.@]
 */
NTSTATUS WINAPI NtConnectPort(PHANDLE PortHandle,PUNICODE_STRING PortName,PVOID Unknown1,
                              PLPCSECTIONINFO sectionInfo,PLPCSECTIONMAPINFO mapInfo,PVOID Unknown2,
                              PVOID ConnectInfo,PDWORD pConnectInfoLength)
{
  FIXME("(%p,%s,%p,%p,%p,%p,%p,%p (%ld)),stub!\n",PortHandle,debugstr_w(PortName->Buffer),Unknown1,
        sectionInfo,mapInfo,Unknown2,ConnectInfo,pConnectInfoLength,pConnectInfoLength?*pConnectInfoLength:-1);
  if(ConnectInfo && pConnectInfoLength)
    TRACE("\tMessage = %s\n",debugstr_an(ConnectInfo,*pConnectInfoLength));
  return 0;
}

/******************************************************************************
 *  NtListenPort		[NTDLL.@]
 *  ZwListenPort		[NTDLL.@]
 */
NTSTATUS WINAPI NtListenPort(HANDLE PortHandle,PLPCMESSAGE pLpcMessage)
{
  FIXME("(%p,%p),stub!\n",PortHandle,pLpcMessage);
  return 0;
}

/******************************************************************************
 *  NtAcceptConnectPort	[NTDLL.@]
 *  ZwAcceptConnectPort	[NTDLL.@]
 */
NTSTATUS WINAPI NtAcceptConnectPort(PHANDLE PortHandle,DWORD Unknown,PLPCMESSAGE pLpcMessage,
                                    DWORD acceptIt,DWORD Unknown2,PLPCSECTIONMAPINFO mapInfo)
{
  FIXME("(%p,0x%08lx,%p,0x%08lx,0x%08lx,%p),stub!\n",PortHandle,Unknown,pLpcMessage,acceptIt,Unknown2,mapInfo);
  return 0;
}

/******************************************************************************
 *  NtCompleteConnectPort	[NTDLL.@]
 *  ZwCompleteConnectPort	[NTDLL.@]
 */
NTSTATUS WINAPI NtCompleteConnectPort(HANDLE PortHandle)
{
  FIXME("(%p),stub!\n",PortHandle);
  return 0;
}

/******************************************************************************
 *  NtRegisterThreadTerminatePort	[NTDLL.@]
 *  ZwRegisterThreadTerminatePort	[NTDLL.@]
 */
NTSTATUS WINAPI NtRegisterThreadTerminatePort(HANDLE PortHandle)
{
  FIXME("(%p),stub!\n",PortHandle);
  return 0;
}

/******************************************************************************
 *  NtRequestWaitReplyPort		[NTDLL.@]
 *  ZwRequestWaitReplyPort		[NTDLL.@]
 */
NTSTATUS WINAPI NtRequestWaitReplyPort(HANDLE PortHandle,PLPCMESSAGE pLpcMessageIn,PLPCMESSAGE pLpcMessageOut)
{
  FIXME("(%p,%p,%p),stub!\n",PortHandle,pLpcMessageIn,pLpcMessageOut);
  if(pLpcMessageIn)
  {
    TRACE("Message to send:\n");
    TRACE("\tActualMessageLength = %d\n",pLpcMessageIn->ActualMessageLength);
    TRACE("\tTotalMessageLength  = %d\n",pLpcMessageIn->TotalMessageLength);
    TRACE("\tMessageType         = %ld\n",pLpcMessageIn->MessageType);
    TRACE("\tClientProcessId     = %ld\n",pLpcMessageIn->ClientProcessId);
    TRACE("\tClientThreadId      = %ld\n",pLpcMessageIn->ClientThreadId);
    TRACE("\tMessageId           = %ld\n",pLpcMessageIn->MessageId);
    TRACE("\tSharedSectionSize   = %ld\n",pLpcMessageIn->SharedSectionSize);
    TRACE("\tMessageData         = %s\n",debugstr_an(pLpcMessageIn->MessageData,pLpcMessageIn->ActualMessageLength));
  }
  return 0;
}

/******************************************************************************
 *  NtReplyWaitReceivePort	[NTDLL.@]
 *  ZwReplyWaitReceivePort	[NTDLL.@]
 */
NTSTATUS WINAPI NtReplyWaitReceivePort(HANDLE PortHandle,PDWORD Unknown,PLPCMESSAGE pLpcMessageOut,PLPCMESSAGE pLpcMessageIn)
{
  FIXME("(%p,%p,%p,%p),stub!\n",PortHandle,Unknown,pLpcMessageOut,pLpcMessageIn);
  return 0;
}

/*
 *	Misc
 */

 /******************************************************************************
 *  NtSetIntervalProfile	[NTDLL.@]
 *  ZwSetIntervalProfile	[NTDLL.@]
 */
NTSTATUS WINAPI NtSetIntervalProfile(DWORD x1,DWORD x2) {
	FIXME("(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtQueryPerformanceCounter	[NTDLL.@]
 */
NTSTATUS WINAPI NtQueryPerformanceCounter(
	IN PLARGE_INTEGER Counter,
	IN PLARGE_INTEGER Frequency)
{
	FIXME("(%p, 0%p) stub\n",
	Counter, Frequency);
	return 0;
}

/******************************************************************************
 *  NtCreateMailslotFile	[NTDLL.@]
 *  ZwCreateMailslotFile	[NTDLL.@]
 */
NTSTATUS WINAPI NtCreateMailslotFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7,x8);
	return 0;
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

    TRACE("(0x%08x,%p,0x%08lx,%p)\n",
          SystemInformationClass,SystemInformation,Length,ResultLength);

    switch (SystemInformationClass)
    {
    case SystemBasicInformation:
        {
            SYSTEM_BASIC_INFORMATION* sbi = (SYSTEM_BASIC_INFORMATION*)SystemInformation;
            if (Length >= sizeof(*sbi))
            {
                sbi->dwUnknown1 = 0;
                sbi->uKeMaximumIncrement = 0;
                sbi->uPageSize = 1024; /* FIXME */
                sbi->uMmNumberOfPhysicalPages = 12345; /* FIXME */
                sbi->uMmLowestPhysicalPage = 0; /* FIXME */
                sbi->uMmHighestPhysicalPage = 12345; /* FIXME */
                sbi->uAllocationGranularity = 65536; /* FIXME */
                sbi->pLowestUserAddress = 0; /* FIXME */
                sbi->pMmHighestUserAddress = (void*)~0; /* FIXME */
                sbi->uKeActiveProcessors = 1; /* FIXME */
                sbi->bKeNumberProcessors = 1; /* FIXME */
                len = sizeof(*sbi);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemPerformanceInformation:
        {
            SYSTEM_PERFORMANCE_INFORMATION* spi = (SYSTEM_PERFORMANCE_INFORMATION*)SystemInformation;
            if (Length >= sizeof(*spi))
            {
                memset(spi, 0, sizeof(*spi)); /* FIXME */
                len = sizeof(*spi);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemTimeOfDayInformation:
        {
            SYSTEM_TIMEOFDAY_INFORMATION* sti = (SYSTEM_TIMEOFDAY_INFORMATION*)SystemInformation;
            if (Length >= sizeof(*sti))
            {
                sti->liKeBootTime.QuadPart = 0; /* FIXME */
                sti->liKeSystemTime.QuadPart = 0; /* FIXME */
                sti->liExpTimeZoneBias.QuadPart  = 0; /* FIXME */
                sti->uCurrentTimeZoneId = 0; /* FIXME */
                sti->dwReserved = 0;
                len = sizeof(*sti);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemProcessInformation:
        {
            SYSTEM_PROCESS_INFORMATION* spi = (SYSTEM_PROCESS_INFORMATION*)SystemInformation;
            SYSTEM_PROCESS_INFORMATION* last = NULL;
            HANDLE hSnap = 0;
            char procname[1024];
            DWORD wlen;

            SERVER_START_REQ( create_snapshot )
            {
                req->flags   = SNAP_PROCESS | SNAP_THREAD;
                req->inherit = FALSE;
                req->pid     = 0;
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
                    wine_server_set_reply( req, procname, sizeof(procname)-1 );
                    if (!(ret = wine_server_call( req )))
                    {
                        procname[wine_server_reply_size(reply)] = 0;
                        if (Length >= len + sizeof(*spi))
                        {
                            memset(spi, 0, sizeof(*spi));
                            spi->dwOffset = sizeof(*spi);
                            spi->dwThreadCount = reply->threads;
                            memset(&spi->ftCreationTime, 0, sizeof(spi->ftCreationTime));
                            /* spi->pszProcessName will be set later on */
                            spi->dwBasePriority = reply->priority;
                            spi->dwProcessID = (DWORD)reply->pid;
                            spi->dwParentProcessID = (DWORD)reply->ppid;
                            spi->dwHandleCount = reply->handles;
                            spi->dwVirtualBytesPeak = 0; /* FIXME */
                            spi->dwVirtualBytes = 0; /* FIXME */
                            spi->dwPageFaults = 0; /* FIXME */
                            spi->dwWorkingSetPeak = 0; /* FIXME */
                            spi->dwWorkingSet = 0; /* FIXME */
                            spi->dwUnknown5 = 0; /* FIXME */
                            spi->dwPagedPool = 0; /* FIXME */
                            spi->dwUnknown6 = 0; /* FIXME */
                            spi->dwNonPagedPool = 0; /* FIXME */
                            spi->dwPageFileBytesPeak = 0; /* FIXME */
                            spi->dwPrivateBytes = 0; /* FIXME */
                            spi->dwPageFileBytes = 0; /* FIXME */
                            /* spi->ti will be set later on */
                            len += sizeof(*spi) - sizeof(spi->ti);
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
                RtlMultiByteToUnicodeN(NULL, 0, &wlen, procname, strlen(procname) + 1);
                if (Length >= len + wlen + spi->dwThreadCount * sizeof(THREAD_INFO))
                {
                    int     i, j;

                    /* set thread info */
                    spi->dwOffset += spi->dwThreadCount * sizeof(THREAD_INFO);
                    len += spi->dwThreadCount * sizeof(THREAD_INFO);
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
                    spi->pszProcessName = (WCHAR*)((char*)spi + spi->dwOffset);
                    RtlMultiByteToUnicodeN( spi->pszProcessName, wlen, NULL, procname, strlen(procname) + 1);
                    len += wlen;
                    spi->dwOffset += wlen;

                    last = spi;
                    spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + spi->dwOffset);
                }
                else ret = STATUS_INFO_LENGTH_MISMATCH;
            }
            if (ret == STATUS_SUCCESS && last) last->dwOffset = 0;
            if (hSnap) NtClose(hSnap);
        }
        break;
    case SystemProcessorPerformanceInformation:
        {
            SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* sppi = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)SystemInformation;
            if (Length >= sizeof(*sppi))
            {
                memset(sppi, 0, sizeof(*sppi)); /* FIXME */
                len = sizeof(*sppi);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;

    case SystemCacheInformation:
        {
            SYSTEM_CACHE_INFORMATION* sci = (SYSTEM_CACHE_INFORMATION*)SystemInformation;
            if (Length >= sizeof(*sci))
            {
                memset(sci, 0, sizeof(*sci)); /* FIXME */
                len = sizeof(*sci);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
        break;
    case SystemRegistryQuotaInformation:
	/* Something to do with the size of the registry             *
	 * Since we don't have a size limitation, fake it            *
	 * This is almost certainly wrong.                           *
	 * This sets each of the three words in the struct to 32 MB, *
	 * which is enough to make the IE 5 installer happy.         */
        {
            SYSTEM_REGISTRY_QUOTA_INFORMATION* srqi = (SYSTEM_REGISTRY_QUOTA_INFORMATION*)SystemInformation;
            if (Length >= sizeof(*srqi))
            {
                FIXME("(0x%08x,%p,0x%08lx,%p) faking max registry size of 32 MB\n",
                      SystemInformationClass,SystemInformation,Length,ResultLength);
                srqi->RegistryQuotaAllowed = 0x2000000;
                srqi->RegistryQuotaUsed = 0x200000;
                srqi->Reserved1 = (void*)0x200000;
                if (ResultLength) *ResultLength = sizeof(*srqi);
            }
            else ret = STATUS_INFO_LENGTH_MISMATCH;
        }
	break;

    default:
	FIXME("(0x%08x,%p,0x%08lx,%p) stub\n",
	      SystemInformationClass,SystemInformation,Length,ResultLength);
	memset(SystemInformation, 0, Length);
        ret = STATUS_NOT_IMPLEMENTED;
    }
    if (ResultLength) *ResultLength = len;

    return ret;
}


/******************************************************************************
 *  NtCreatePagingFile		[NTDLL.@]
 *  ZwCreatePagingFile		[NTDLL.@]
 */
NTSTATUS WINAPI NtCreatePagingFile(
	IN PUNICODE_STRING PageFileName,
	IN ULONG MiniumSize,
	IN ULONG MaxiumSize,
	OUT PULONG ActualSize)
{
	FIXME("(%p(%s),0x%08lx,0x%08lx,%p),stub!\n",
	PageFileName->Buffer, debugstr_w(PageFileName->Buffer),MiniumSize,MaxiumSize,ActualSize);
	return 0;
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
 *  NtPowerInformation				[NTDLL.@]
 *
 */
NTSTATUS WINAPI NtPowerInformation(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub\n",x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtAllocateLocallyUniqueId (NTDLL.@)
 *
 * FIXME: the server should do that
 */
NTSTATUS WINAPI NtAllocateLocallyUniqueId(PLUID Luid)
{
    static LUID luid;

    FIXME("%p (0x%08lx%08lx)\n", Luid, luid.HighPart, luid.LowPart);

    luid.LowPart++;
    if (luid.LowPart==0)
        luid.HighPart++;
    Luid->HighPart = luid.HighPart;
    Luid->LowPart = luid.LowPart;

    return STATUS_SUCCESS;
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
