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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wine/debug.h"

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
 *	Timer object
 */

/**************************************************************************
 *		NtCreateTimer				[NTDLL.@]
 *		ZwCreateTimer				[NTDLL.@]
 */
NTSTATUS WINAPI NtCreateTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN TIMER_TYPE TimerType)
{
	FIXME("(%p,0x%08lx,%p,0x%08x) stub\n",
	TimerHandle,DesiredAccess,ObjectAttributes, TimerType);
	dump_ObjectAttributes(ObjectAttributes);
	return 0;
}
/**************************************************************************
 *		NtSetTimer				[NTDLL.@]
 *		ZwSetTimer				[NTDLL.@]
 */
NTSTATUS WINAPI NtSetTimer(
	IN HANDLE TimerHandle,
	IN PLARGE_INTEGER DueTime,
	IN PTIMERAPCROUTINE TimerApcRoutine,
	IN PVOID TimerContext,
	IN BOOLEAN WakeTimer,
	IN ULONG Period OPTIONAL,
	OUT PBOOLEAN PreviousState OPTIONAL)
{
	FIXME("(%p,%p,%p,%p,%08x,0x%08lx,%p) stub\n",
	TimerHandle,DueTime,TimerApcRoutine,TimerContext,WakeTimer,Period,PreviousState);
	return 0;
}

/******************************************************************************
 * NtQueryTimerResolution [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryTimerResolution(DWORD x1,DWORD x2,DWORD x3)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx), stub!\n",x1,x2,x3);
	return 1;
}

/*
 *	Process object
 */

/******************************************************************************
 *  NtTerminateProcess			[NTDLL.@]
 *
 *  Native applications must kill themselves when done
 */
NTSTATUS WINAPI NtTerminateProcess( HANDLE handle, LONG exit_code )
{
    NTSTATUS ret;
    BOOL self;
    SERVER_START_REQ( terminate_process )
    {
        req->handle    = handle;
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = !ret && reply->self;
    }
    SERVER_END_REQ;
    if (self) exit( exit_code );
    return ret;
}

/******************************************************************************
*  NtQueryInformationProcess		[NTDLL.@]
*  ZwQueryInformationProcess		[NTDLL.@]
*
*/
NTSTATUS WINAPI NtQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength)
{
	NTSTATUS ret = STATUS_SUCCESS;
	ULONG len = 0;

	switch (ProcessInformationClass) {
	case ProcessDebugPort:
		/* "These are not the debuggers you are looking for." */
		/* set it to 0 aka "no debugger" to satisfy copy protections */
		if (ProcessInformationLength == 4)
		{
			memset(ProcessInformation,0,ProcessInformationLength);
			len = 4;
		}
		else
			ret = STATUS_INFO_LENGTH_MISMATCH;
		break;
	case ProcessWow64Information:
		if (ProcessInformationLength == 4)
		{
			memset(ProcessInformation,0,ProcessInformationLength);
			len = 4;
		}
		else
			ret = STATUS_INFO_LENGTH_MISMATCH;
		break;
	default:
		FIXME("(%p,0x%08x,%p,0x%08lx,%p),stub!\n",
			ProcessHandle,ProcessInformationClass,
			ProcessInformation,ProcessInformationLength,
			ReturnLength
		);
		break;
	}

	if (ReturnLength)
		*ReturnLength = len;

	return ret;
}

/******************************************************************************
 * NtSetInformationProcess [NTDLL.@]
 * ZwSetInformationProcess [NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	IN PVOID ProcessInformation,
	IN ULONG ProcessInformationLength)
{
	FIXME("(%p,0x%08x,%p,0x%08lx) stub\n",
	ProcessHandle,ProcessInformationClass,ProcessInformation,ProcessInformationLength);
	return 0;
}

/*
 *	Thread
 */

/******************************************************************************
*  NtQueryInformationThread		[NTDLL.@]
*  ZwQueryInformationThread		[NTDLL.@]
*
*/
NTSTATUS WINAPI NtQueryInformationThread(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength)
{
	FIXME("(%p,0x%08x,%p,0x%08lx,%p),stub!\n",
		ThreadHandle, ThreadInformationClass, ThreadInformation,
		ThreadInformationLength, ReturnLength);
	return 0;
}

/******************************************************************************
 *  NtSetInformationThread		[NTDLL.@]
 *  ZwSetInformationThread		[NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationThread(
	HANDLE ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID ThreadInformation,
	ULONG ThreadInformationLength)
{
	FIXME("(%p,0x%08x,%p,0x%08lx),stub!\n",
	ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
	return 0;
}

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
	FIXME("(%p,0x%08lx,%p): stub\n",
	ProcessHandle,DesiredAccess, TokenHandle);
	*TokenHandle = (HANDLE)0xcafe;
	return 0;
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
	FIXME("(%p,0x%08lx,0x%08x,%p): stub\n",
	ThreadHandle,DesiredAccess, OpenAsSelf, TokenHandle);
	*TokenHandle = (HANDLE)0xcafe;
	return 0;
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
        return STATUS_BUFFER_TOO_SMALL;

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
    switch(SystemInformationClass)
    {
    case 0x25:
	/* Something to do with the size of the registry             *
	 * Since we don't have a size limitation, fake it            *
	 * This is almost certainly wrong.                           *
	 * This sets each of the three words in the struct to 32 MB, *
	 * which is enough to make the IE 5 installer happy.         */
	FIXME("(0x%08x,%p,0x%08lx,%p) faking max registry size of 32 MB\n",
	      SystemInformationClass,SystemInformation,Length,ResultLength);
	*(DWORD *)SystemInformation = 0x2000000;
	*(((DWORD *)SystemInformation)+1) = 0x200000;
	*(((DWORD *)SystemInformation)+2) = 0x200000;
	break;

    default:
	FIXME("(0x%08x,%p,0x%08lx,%p) stub\n",
	      SystemInformationClass,SystemInformation,Length,ResultLength);
	ZeroMemory (SystemInformation, Length);
    }

    return STATUS_SUCCESS;
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
