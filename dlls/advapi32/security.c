/*
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  FIXME: for all functions thunking down to Rtl* functions:  implement SetLastError()
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "rpcnterr.h"
#include "winreg.h"
#include "winternl.h"
#include "ntstatus.h"
#include "ntsecapi.h"
#include "accctrl.h"
#include "sddl.h"

#include "aclapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes);
static BOOL ParseStringAclToAcl(LPCWSTR StringAcl, LPDWORD lpdwFlags, 
    PACL pAcl, LPDWORD cBytes);
static BYTE ParseAceStringFlags(LPCWSTR* StringAcl);
static BYTE ParseAceStringType(LPCWSTR* StringAcl);
static DWORD ParseAceStringRights(LPCWSTR* StringAcl);
static BOOL ParseStringSecurityDescriptorToSecurityDescriptor(
    LPCWSTR StringSecurityDescriptor,
    SECURITY_DESCRIPTOR* SecurityDescriptor,
    LPDWORD cBytes);
static DWORD ParseAclStringFlags(LPCWSTR* StringAcl);

typedef struct _ACEFLAG
{
   LPCWSTR wstr;
   DWORD value;
} ACEFLAG, *LPACEFLAG;

/*
 * ACE access rights
 */
static const WCHAR SDDL_READ_CONTROL[]     = {'R','C',0};
static const WCHAR SDDL_WRITE_DAC[]        = {'W','D',0};
static const WCHAR SDDL_WRITE_OWNER[]      = {'W','O',0};
static const WCHAR SDDL_STANDARD_DELETE[]  = {'S','D',0};
static const WCHAR SDDL_GENERIC_ALL[]      = {'G','A',0};
static const WCHAR SDDL_GENERIC_READ[]     = {'G','R',0};
static const WCHAR SDDL_GENERIC_WRITE[]    = {'G','W',0};
static const WCHAR SDDL_GENERIC_EXECUTE[]  = {'G','X',0};

/*
 * ACE types
 */
static const WCHAR SDDL_ACCESS_ALLOWED[]        = {'A',0};
static const WCHAR SDDL_ACCESS_DENIED[]         = {'D',0};
static const WCHAR SDDL_OBJECT_ACCESS_ALLOWED[] = {'O','A',0};
static const WCHAR SDDL_OBJECT_ACCESS_DENIED[]  = {'O','D',0};
static const WCHAR SDDL_AUDIT[]                 = {'A','U',0};
static const WCHAR SDDL_ALARM[]                 = {'A','L',0};
static const WCHAR SDDL_OBJECT_AUDIT[]          = {'O','U',0};
static const WCHAR SDDL_OBJECT_ALARMp[]         = {'O','L',0};

/*
 * ACE flags
 */
static const WCHAR SDDL_CONTAINER_INHERIT[]  = {'C','I',0};
static const WCHAR SDDL_OBJECT_INHERIT[]     = {'O','I',0};
static const WCHAR SDDL_NO_PROPAGATE[]       = {'N','P',0};
static const WCHAR SDDL_INHERIT_ONLY[]       = {'I','O',0};
static const WCHAR SDDL_INHERITED[]          = {'I','D',0};
static const WCHAR SDDL_AUDIT_SUCCESS[]      = {'S','A',0};
static const WCHAR SDDL_AUDIT_FAILURE[]      = {'F','A',0};

#define CallWin32ToNt(func) \
	{ NTSTATUS ret; \
	  ret = (func); \
	  if (ret !=STATUS_SUCCESS) \
	  { SetLastError (RtlNtStatusToDosError(ret)); return FALSE; } \
	  return TRUE; \
	}

static void dumpLsaAttributes( PLSA_OBJECT_ATTRIBUTES oa )
{
	if (oa)
	{
	  TRACE("\n\tlength=%lu, rootdir=%p, objectname=%s\n\tattr=0x%08lx, sid=%p qos=%p\n",
		oa->Length, oa->RootDirectory,
		oa->ObjectName?debugstr_w(oa->ObjectName->Buffer):"null",
     		oa->Attributes, oa->SecurityDescriptor, oa->SecurityQualityOfService);
	}
}

/************************************************************
 *                ADVAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
BOOL ADVAPI_IsLocalComputer(LPCWSTR ServerName)
{
    if (!ServerName)
    {
        return TRUE;
    }
    else
    {
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
        BOOL Result;
        LPWSTR buf;

        buf = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
        Result = GetComputerNameW(buf,  &dwSize);
        if (Result && (ServerName[0] == '\\') && (ServerName[1] == '\\'))
            ServerName += 2;
        Result = Result && !lstrcmpW(ServerName, buf);
        HeapFree(GetProcessHeap(), 0, buf);

        return Result;
    }
}

#define ADVAPI_ForceLocalComputer(ServerName, FailureCode) \
    if (!ADVAPI_IsLocalComputer(ServerName)) \
    { \
        FIXME("Action Implemented for local computer only. " \
              "Requested for server %s\n", debugstr_w(ServerName)); \
        return FailureCode; \
    }

/*	##############################
	######	TOKEN FUNCTIONS ######
	##############################
*/

/******************************************************************************
 * OpenProcessToken			[ADVAPI32.@]
 * Opens the access token associated with a process handle.
 *
 * PARAMS
 *   ProcessHandle [I] Handle to process
 *   DesiredAccess [I] Desired access to process
 *   TokenHandle   [O] Pointer to handle of open access token
 *
 * RETURNS
 *  Success: TRUE. TokenHandle contains the access token.
 *  Failure: FALSE.
 *
 * NOTES
 *  See NtOpenProcessToken.
 */
BOOL WINAPI
OpenProcessToken( HANDLE ProcessHandle, DWORD DesiredAccess,
                  HANDLE *TokenHandle )
{
	CallWin32ToNt(NtOpenProcessToken( ProcessHandle, DesiredAccess, TokenHandle ));
}

/******************************************************************************
 * OpenThreadToken [ADVAPI32.@]
 *
 * Opens the access token associated with a thread handle.
 *
 * PARAMS
 *   ThreadHandle  [I] Handle to process
 *   DesiredAccess [I] Desired access to the thread
 *   OpenAsSelf    [I] ???
 *   TokenHandle   [O] Destination for the token handle
 *
 * RETURNS
 *  Success: TRUE. TokenHandle contains the access token.
 *  Failure: FALSE.
 *
 * NOTES
 *  See NtOpenThreadToken.
 */
BOOL WINAPI
OpenThreadToken( HANDLE ThreadHandle, DWORD DesiredAccess,
		 BOOL OpenAsSelf, HANDLE *TokenHandle)
{
	CallWin32ToNt (NtOpenThreadToken(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle));
}

/******************************************************************************
 * AdjustTokenPrivileges [ADVAPI32.@]
 *
 * Adjust the privileges of an open token handle.
 * 
 * PARAMS
 *  TokenHandle          [I]   Handle from OpenProcessToken() or OpenThreadToken() 
 *  DisableAllPrivileges [I]   TRUE=Remove all privileges, FALSE=Use NewState
 *  NewState             [I]   Desired new privileges of the token
 *  BufferLength         [I]   Length of NewState
 *  PreviousState        [O]   Destination for the previous state
 *  ReturnLength         [I/O] Size of PreviousState
 *
 *
 * RETURNS
 *  Success: TRUE. Privileges are set to NewState and PreviousState is updated.
 *  Failure: FALSE.
 *
 * NOTES
 *  See NtAdjustPrivilegesToken.
 */
BOOL WINAPI
AdjustTokenPrivileges( HANDLE TokenHandle, BOOL DisableAllPrivileges,
                       LPVOID NewState, DWORD BufferLength,
                       LPVOID PreviousState, LPDWORD ReturnLength )
{
	CallWin32ToNt(NtAdjustPrivilegesToken(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength));
}

/******************************************************************************
 * CheckTokenMembership [ADVAPI32.@]
 *
 * Determine if an access token is a member of a SID.
 * 
 * PARAMS
 *   TokenHandle [I] Handle from OpenProcessToken() or OpenThreadToken()
 *   SidToCheck  [I] SID that possibly contains the token
 *   IsMember    [O] Destination for result.
 *
 * RETURNS
 *  Success: TRUE. IsMember is TRUE if TokenHandle is a member, FALSE otherwise.
 *  Failure: FALSE.
 */
BOOL WINAPI
CheckTokenMembership( HANDLE TokenHandle, PSID SidToCheck,
                      PBOOL IsMember )
{
  FIXME("(%p %p %p) stub!\n", TokenHandle, SidToCheck, IsMember);

  *IsMember = TRUE;
  return(TRUE);
}

/******************************************************************************
 * GetTokenInformation [ADVAPI32.@]
 *
 * PARAMS
 *   token           [I] Handle from OpenProcessToken() or OpenThreadToken()
 *   tokeninfoclass  [I] A TOKEN_INFORMATION_CLASS from "winnt.h"
 *   tokeninfo       [O] Destination for token information
 *   tokeninfolength [I] Length of tokeninfo
 *   retlen          [O] Destination for returned token information length
 *
 * RETURNS
 *  Success: TRUE. tokeninfo contains retlen bytes of token information
 *  Failure: FALSE.
 *
 * NOTES
 *  See NtQueryInformationToken.
 */
BOOL WINAPI
GetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS tokeninfoclass,
		     LPVOID tokeninfo, DWORD tokeninfolength, LPDWORD retlen )
{
    TRACE("(%p, %s, %p, %ld, %p): \n",
          token,
          (tokeninfoclass == TokenUser) ? "TokenUser" :
          (tokeninfoclass == TokenGroups) ? "TokenGroups" :
          (tokeninfoclass == TokenPrivileges) ? "TokenPrivileges" :
          (tokeninfoclass == TokenOwner) ? "TokenOwner" :
          (tokeninfoclass == TokenPrimaryGroup) ? "TokenPrimaryGroup" :
          (tokeninfoclass == TokenDefaultDacl) ? "TokenDefaultDacl" :
          (tokeninfoclass == TokenSource) ? "TokenSource" :
          (tokeninfoclass == TokenType) ? "TokenType" :
          (tokeninfoclass == TokenImpersonationLevel) ? "TokenImpersonationLevel" :
          (tokeninfoclass == TokenStatistics) ? "TokenStatistics" :
          (tokeninfoclass == TokenRestrictedSids) ? "TokenRestrictedSids" :
          (tokeninfoclass == TokenSessionId) ? "TokenSessionId" :
          (tokeninfoclass == TokenGroupsAndPrivileges) ? "TokenGroupsAndPrivileges" :
          (tokeninfoclass == TokenSessionReference) ? "TokenSessionReference" :
          (tokeninfoclass == TokenSandBoxInert) ? "TokenSandBoxInert" :
          "Unknown",
          tokeninfo, tokeninfolength, retlen);
    CallWin32ToNt (NtQueryInformationToken( token, tokeninfoclass, tokeninfo, tokeninfolength, retlen));
}

/******************************************************************************
 * SetTokenInformation [ADVAPI32.@]
 *
 * Set information for an access token.
 *
 * PARAMS
 *   token           [I] Handle from OpenProcessToken() or OpenThreadToken()
 *   tokeninfoclass  [I] A TOKEN_INFORMATION_CLASS from "winnt.h"
 *   tokeninfo       [I] Token information to set
 *   tokeninfolength [I] Length of tokeninfo
 *
 * RETURNS
 *  Success: TRUE. The information for the token is set to tokeninfo.
 *  Failure: FALSE.
 */
BOOL WINAPI
SetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS tokeninfoclass,
		     LPVOID tokeninfo, DWORD tokeninfolength )
{
    TRACE("(%p, %s, %p, %ld): stub\n",
          token,
          (tokeninfoclass == TokenUser) ? "TokenUser" :
          (tokeninfoclass == TokenGroups) ? "TokenGroups" :
          (tokeninfoclass == TokenPrivileges) ? "TokenPrivileges" :
          (tokeninfoclass == TokenOwner) ? "TokenOwner" :
          (tokeninfoclass == TokenPrimaryGroup) ? "TokenPrimaryGroup" :
          (tokeninfoclass == TokenDefaultDacl) ? "TokenDefaultDacl" :
          (tokeninfoclass == TokenSource) ? "TokenSource" :
          (tokeninfoclass == TokenType) ? "TokenType" :
          (tokeninfoclass == TokenImpersonationLevel) ? "TokenImpersonationLevel" :
          (tokeninfoclass == TokenStatistics) ? "TokenStatistics" :
          (tokeninfoclass == TokenRestrictedSids) ? "TokenRestrictedSids" :
          (tokeninfoclass == TokenSessionId) ? "TokenSessionId" :
          (tokeninfoclass == TokenGroupsAndPrivileges) ? "TokenGroupsAndPrivileges" :
          (tokeninfoclass == TokenSessionReference) ? "TokenSessionReference" :
          (tokeninfoclass == TokenSandBoxInert) ? "TokenSandBoxInert" :
          "Unknown",
          tokeninfo, tokeninfolength);

    CallWin32ToNt (NtSetInformationToken( token, tokeninfoclass, tokeninfo, tokeninfolength ));
}

/*************************************************************************
 * SetThreadToken [ADVAPI32.@]
 *
 * Assigns an 'impersonation token' to a thread so it can assume the
 * security privileges of another thread or process.  Can also remove
 * a previously assigned token. 
 *
 * PARAMS
 *   thread          [O] Handle to thread to set the token for
 *   token           [I] Token to set
 *
 * RETURNS
 *  Success: TRUE. The threads access token is set to token
 *  Failure: FALSE.
 *
 * NOTES
 *  Only supported on NT or higher. On Win9X this function does nothing.
 *  See SetTokenInformation.
 */
BOOL WINAPI SetThreadToken(PHANDLE thread, HANDLE token)
{
    CallWin32ToNt (NtSetInformationThread( thread ? *thread : GetCurrentThread(),
                                           ThreadImpersonationToken, &token, sizeof token ));
}

/*	##############################
	######	SID FUNCTIONS	######
	##############################
*/

/******************************************************************************
 * AllocateAndInitializeSid [ADVAPI32.@]
 *
 * PARAMS
 *   pIdentifierAuthority []
 *   nSubAuthorityCount   []
 *   nSubAuthority0       []
 *   nSubAuthority1       []
 *   nSubAuthority2       []
 *   nSubAuthority3       []
 *   nSubAuthority4       []
 *   nSubAuthority5       []
 *   nSubAuthority6       []
 *   nSubAuthority7       []
 *   pSid                 []
 */
BOOL WINAPI
AllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                          BYTE nSubAuthorityCount,
                          DWORD nSubAuthority0, DWORD nSubAuthority1,
                          DWORD nSubAuthority2, DWORD nSubAuthority3,
                          DWORD nSubAuthority4, DWORD nSubAuthority5,
                          DWORD nSubAuthority6, DWORD nSubAuthority7,
                          PSID *pSid )
{
	CallWin32ToNt (RtlAllocateAndInitializeSid(
		pIdentifierAuthority, nSubAuthorityCount,
		nSubAuthority0, nSubAuthority1,	nSubAuthority2, nSubAuthority3,
		nSubAuthority4, nSubAuthority5, nSubAuthority6, nSubAuthority7,
		pSid ));
}

/******************************************************************************
 * FreeSid [ADVAPI32.@]
 *
 * PARAMS
 *   pSid []
 */
PVOID WINAPI
FreeSid( PSID pSid )
{
    	RtlFreeSid(pSid);
	return NULL; /* is documented like this */
}

/******************************************************************************
 * CopySid [ADVAPI32.@]
 *
 * PARAMS
 *   nDestinationSidLength []
 *   pDestinationSid       []
 *   pSourceSid            []
 */
BOOL WINAPI
CopySid( DWORD nDestinationSidLength, PSID pDestinationSid, PSID pSourceSid )
{
	return RtlCopySid(nDestinationSidLength, pDestinationSid, pSourceSid);
}

/******************************************************************************
 * IsValidSid [ADVAPI32.@]
 *
 * PARAMS
 *   pSid []
 */
BOOL WINAPI
IsValidSid( PSID pSid )
{
	return RtlValidSid( pSid );
}

/******************************************************************************
 * EqualSid [ADVAPI32.@]
 *
 * PARAMS
 *   pSid1 []
 *   pSid2 []
 */
BOOL WINAPI
EqualSid( PSID pSid1, PSID pSid2 )
{
	return RtlEqualSid( pSid1, pSid2 );
}

/******************************************************************************
 * EqualPrefixSid [ADVAPI32.@]
 */
BOOL WINAPI EqualPrefixSid (PSID pSid1, PSID pSid2)
{
	return RtlEqualPrefixSid(pSid1, pSid2);
}

/******************************************************************************
 * GetSidLengthRequired [ADVAPI32.@]
 *
 * PARAMS
 *   nSubAuthorityCount []
 */
DWORD WINAPI
GetSidLengthRequired( BYTE nSubAuthorityCount )
{
	return RtlLengthRequiredSid(nSubAuthorityCount);
}

/******************************************************************************
 * InitializeSid [ADVAPI32.@]
 *
 * PARAMS
 *   pIdentifierAuthority []
 */
BOOL WINAPI
InitializeSid (
	PSID pSid,
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	BYTE nSubAuthorityCount)
{
	return RtlInitializeSid(pSid, pIdentifierAuthority, nSubAuthorityCount);
}

/******************************************************************************
 * GetSidIdentifierAuthority [ADVAPI32.@]
 *
 * PARAMS
 *   pSid []
 */
PSID_IDENTIFIER_AUTHORITY WINAPI
GetSidIdentifierAuthority( PSID pSid )
{
	return RtlIdentifierAuthoritySid(pSid);
}

/******************************************************************************
 * GetSidSubAuthority [ADVAPI32.@]
 *
 * PARAMS
 *   pSid          []
 *   nSubAuthority []
 */
PDWORD WINAPI
GetSidSubAuthority( PSID pSid, DWORD nSubAuthority )
{
	return RtlSubAuthoritySid(pSid, nSubAuthority);
}

/******************************************************************************
 * GetSidSubAuthorityCount [ADVAPI32.@]
 *
 * PARAMS
 *   pSid []
 */
PUCHAR WINAPI
GetSidSubAuthorityCount (PSID pSid)
{
	return RtlSubAuthorityCountSid(pSid);
}

/******************************************************************************
 * GetLengthSid [ADVAPI32.@]
 *
 * PARAMS
 *   pSid []
 */
DWORD WINAPI
GetLengthSid (PSID pSid)
{
	return RtlLengthSid(pSid);
}

/*	##############################################
	######	SECURITY DESCRIPTOR FUNCTIONS	######
	##############################################
*/

/******************************************************************************
 * InitializeSecurityDescriptor [ADVAPI32.@]
 *
 * PARAMS
 *   pDescr   []
 *   revision []
 */
BOOL WINAPI
InitializeSecurityDescriptor( PSECURITY_DESCRIPTOR pDescr, DWORD revision )
{
	CallWin32ToNt (RtlCreateSecurityDescriptor(pDescr, revision ));
}


/******************************************************************************
 * MakeAbsoluteSD [ADVAPI32.@]
 */
BOOL WINAPI MakeAbsoluteSD (
        IN PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
	OUT PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
	OUT LPDWORD lpdwAbsoluteSecurityDescriptorSize,
	OUT PACL pDacl,
	OUT LPDWORD lpdwDaclSize,
	OUT PACL pSacl,
	OUT LPDWORD lpdwSaclSize,
	OUT PSID pOwner,
	OUT LPDWORD lpdwOwnerSize,
	OUT PSID pPrimaryGroup,
	OUT LPDWORD lpdwPrimaryGroupSize)
{
	CallWin32ToNt (RtlSelfRelativeToAbsoluteSD(pSelfRelativeSecurityDescriptor,
            pAbsoluteSecurityDescriptor, lpdwAbsoluteSecurityDescriptorSize,
	    pDacl, lpdwDaclSize, pSacl, lpdwSaclSize, pOwner, lpdwOwnerSize,
	    pPrimaryGroup, lpdwPrimaryGroupSize));
}


/******************************************************************************
 * GetSecurityDescriptorLength [ADVAPI32.@]
 */
DWORD WINAPI GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR pDescr)
{
	return (RtlLengthSecurityDescriptor(pDescr));
}

/******************************************************************************
 * GetSecurityDescriptorOwner [ADVAPI32.@]
 *
 * PARAMS
 *   pOwner            []
 *   lpbOwnerDefaulted []
 */
BOOL WINAPI
GetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pDescr, PSID *pOwner,
			    LPBOOL lpbOwnerDefaulted )
{
	CallWin32ToNt (RtlGetOwnerSecurityDescriptor( pDescr, pOwner, (PBOOLEAN)lpbOwnerDefaulted ));
}

/******************************************************************************
 * SetSecurityDescriptorOwner [ADVAPI32.@]
 *
 * PARAMS
 */
BOOL WINAPI SetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pSecurityDescriptor,
				   PSID pOwner, BOOL bOwnerDefaulted)
{
	CallWin32ToNt (RtlSetOwnerSecurityDescriptor(pSecurityDescriptor, pOwner, bOwnerDefaulted));
}
/******************************************************************************
 * GetSecurityDescriptorGroup			[ADVAPI32.@]
 */
BOOL WINAPI GetSecurityDescriptorGroup(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	PSID *Group,
	LPBOOL GroupDefaulted)
{
	CallWin32ToNt (RtlGetGroupSecurityDescriptor(SecurityDescriptor, Group, (PBOOLEAN)GroupDefaulted));
}
/******************************************************************************
 * SetSecurityDescriptorGroup [ADVAPI32.@]
 */
BOOL WINAPI SetSecurityDescriptorGroup ( PSECURITY_DESCRIPTOR SecurityDescriptor,
					   PSID Group, BOOL GroupDefaulted)
{
	CallWin32ToNt (RtlSetGroupSecurityDescriptor( SecurityDescriptor, Group, GroupDefaulted));
}

/******************************************************************************
 * IsValidSecurityDescriptor [ADVAPI32.@]
 *
 * PARAMS
 *   lpsecdesc []
 */
BOOL WINAPI
IsValidSecurityDescriptor( PSECURITY_DESCRIPTOR SecurityDescriptor )
{
	CallWin32ToNt (RtlValidSecurityDescriptor(SecurityDescriptor));
}

/******************************************************************************
 *  GetSecurityDescriptorDacl			[ADVAPI32.@]
 */
BOOL WINAPI GetSecurityDescriptorDacl(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	OUT LPBOOL lpbDaclPresent,
	OUT PACL *pDacl,
	OUT LPBOOL lpbDaclDefaulted)
{
	CallWin32ToNt (RtlGetDaclSecurityDescriptor(pSecurityDescriptor, (PBOOLEAN)lpbDaclPresent,
					       pDacl, (PBOOLEAN)lpbDaclDefaulted));
}

/******************************************************************************
 *  SetSecurityDescriptorDacl			[ADVAPI32.@]
 */
BOOL WINAPI
SetSecurityDescriptorDacl (
	PSECURITY_DESCRIPTOR lpsd,
	BOOL daclpresent,
	PACL dacl,
	BOOL dacldefaulted )
{
	CallWin32ToNt (RtlSetDaclSecurityDescriptor (lpsd, daclpresent, dacl, dacldefaulted ));
}
/******************************************************************************
 *  GetSecurityDescriptorSacl			[ADVAPI32.@]
 */
BOOL WINAPI GetSecurityDescriptorSacl(
	IN PSECURITY_DESCRIPTOR lpsd,
	OUT LPBOOL lpbSaclPresent,
	OUT PACL *pSacl,
	OUT LPBOOL lpbSaclDefaulted)
{
	CallWin32ToNt (RtlGetSaclSecurityDescriptor(lpsd,
	   (PBOOLEAN)lpbSaclPresent, pSacl, (PBOOLEAN)lpbSaclDefaulted));
}

/**************************************************************************
 * SetSecurityDescriptorSacl			[ADVAPI32.@]
 */
BOOL WINAPI SetSecurityDescriptorSacl (
	PSECURITY_DESCRIPTOR lpsd,
	BOOL saclpresent,
	PACL lpsacl,
	BOOL sacldefaulted)
{
	CallWin32ToNt (RtlSetSaclSecurityDescriptor(lpsd, saclpresent, lpsacl, sacldefaulted));
}
/******************************************************************************
 * MakeSelfRelativeSD [ADVAPI32.@]
 *
 * PARAMS
 *   lpabssecdesc  []
 *   lpselfsecdesc []
 *   lpbuflen      []
 */
BOOL WINAPI
MakeSelfRelativeSD(
	IN PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
	IN PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
	IN OUT LPDWORD lpdwBufferLength)
{
	CallWin32ToNt (RtlMakeSelfRelativeSD(pAbsoluteSecurityDescriptor,pSelfRelativeSecurityDescriptor, lpdwBufferLength));
}

/******************************************************************************
 * GetSecurityDescriptorControl			[ADVAPI32.@]
 */

BOOL WINAPI GetSecurityDescriptorControl ( PSECURITY_DESCRIPTOR  pSecurityDescriptor,
		 PSECURITY_DESCRIPTOR_CONTROL pControl, LPDWORD lpdwRevision)
{
	CallWin32ToNt (RtlGetControlSecurityDescriptor(pSecurityDescriptor,pControl,lpdwRevision));
}

/*	##############################
	######	ACL FUNCTIONS	######
	##############################
*/

/*************************************************************************
 * InitializeAcl [ADVAPI32.@]
 */
DWORD WINAPI InitializeAcl(PACL acl, DWORD size, DWORD rev)
{
	CallWin32ToNt (RtlCreateAcl(acl, size, rev));
}

/******************************************************************************
 *  AddAccessAllowedAce [ADVAPI32.@]
 */
BOOL WINAPI AddAccessAllowedAce(
        IN OUT PACL pAcl,
        IN DWORD dwAceRevision,
        IN DWORD AccessMask,
        IN PSID pSid)
{
	CallWin32ToNt(RtlAddAccessAllowedAce(pAcl, dwAceRevision, AccessMask, pSid));
}

/******************************************************************************
 *  AddAccessAllowedAceEx [ADVAPI32.@]
 */
BOOL WINAPI AddAccessAllowedAceEx(
        IN OUT PACL pAcl,
        IN DWORD dwAceRevision,
	IN DWORD AceFlags,
        IN DWORD AccessMask,
        IN PSID pSid)
{
	CallWin32ToNt(RtlAddAccessAllowedAceEx(pAcl, dwAceRevision, AceFlags, AccessMask, pSid));
}

/******************************************************************************
 *  AddAccessDeniedAce [ADVAPI32.@]
 */
BOOL WINAPI AddAccessDeniedAce(
        IN OUT PACL pAcl,
        IN DWORD dwAceRevision,
        IN DWORD AccessMask,
        IN PSID pSid)
{
	CallWin32ToNt(RtlAddAccessDeniedAce(pAcl, dwAceRevision, AccessMask, pSid));
}

/******************************************************************************
 *  AddAccessDeniedAceEx [ADVAPI32.@]
 */
BOOL WINAPI AddAccessDeniedAceEx(
        IN OUT PACL pAcl,
        IN DWORD dwAceRevision,
	IN DWORD AceFlags,
        IN DWORD AccessMask,
        IN PSID pSid)
{
	CallWin32ToNt(RtlAddAccessDeniedAceEx(pAcl, dwAceRevision, AceFlags, AccessMask, pSid));
}

/******************************************************************************
 *  AddAce [ADVAPI32.@]
 */
BOOL WINAPI AddAce(
        IN OUT PACL pAcl,
        IN DWORD dwAceRevision,
        IN DWORD dwStartingAceIndex,
        LPVOID pAceList,
        DWORD nAceListLength)
{
	CallWin32ToNt(RtlAddAce(pAcl, dwAceRevision, dwStartingAceIndex, pAceList, nAceListLength));
}

/******************************************************************************
 * DeleteAce [ADVAPI32.@]
 */
BOOL WINAPI DeleteAce(PACL pAcl, DWORD dwAceIndex)
{
    CallWin32ToNt(RtlDeleteAce(pAcl, dwAceIndex));
}

/******************************************************************************
 *  FindFirstFreeAce [ADVAPI32.@]
 */
BOOL WINAPI FindFirstFreeAce(IN PACL pAcl, LPVOID * pAce)
{
	return RtlFirstFreeAce(pAcl, (PACE_HEADER *)pAce);
}

/******************************************************************************
 * GetAce [ADVAPI32.@]
 */
BOOL WINAPI GetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce )
{
    CallWin32ToNt(RtlGetAce(pAcl, dwAceIndex, pAce));
}

/******************************************************************************
 * GetAclInformation [ADVAPI32.@]
 */
BOOL WINAPI GetAclInformation(
  PACL pAcl,
  LPVOID pAclInformation,
  DWORD nAclInformationLength,
  ACL_INFORMATION_CLASS dwAclInformationClass)
{
	CallWin32ToNt(RtlQueryInformationAcl(pAcl, pAclInformation,
		nAclInformationLength, dwAclInformationClass));
}

/******************************************************************************
 *  IsValidAcl [ADVAPI32.@]
 */
BOOL WINAPI IsValidAcl(IN PACL pAcl)
{
	return RtlValidAcl(pAcl);
}

/*	##############################
	######	MISC FUNCTIONS	######
	##############################
*/

static const char * const DefaultPrivNames[] =
{
    NULL, NULL,
    "SeCreateTokenPrivilege", "SeAssignPrimaryTokenPrivilege",
    "SeLockMemoryPrivilege", "SeIncreaseQuotaPrivilege",
    "SeUnsolicitedInputPrivilege", "SeMachineAccountPrivilege",
    "SeTcbPrivilege", "SeSecurityPrivilege",
    "SeTakeOwnershipPrivilege", "SeLoadDriverPrivilege",
    "SeSystemProfilePrivilege", "SeSystemtimePrivilege",
    "SeProfileSingleProcessPrivilege", "SeIncreaseBasePriorityPrivilege",
    "SeCreatePagefilePrivilege", "SeCreatePermanentPrivilege",
    "SeBackupPrivilege", "SeRestorePrivilege",
    "SeShutdownPrivilege", "SeDebugPrivilege",
    "SeAuditPrivilege", "SeSystemEnvironmentPrivilege",
    "SeChangeNotifyPrivilege", "SeRemoteShutdownPrivilege",
    "SeUndockPrivilege", "SeSyncAgentPrivilege",
    "SeEnableDelegationPrivilege", "SeManageVolumePrivilege",
    "SeImpersonatePrivilege", "SeCreateGlobalPrivilege",
};
#define NUMPRIVS (sizeof DefaultPrivNames/sizeof DefaultPrivNames[0])

/******************************************************************************
 * LookupPrivilegeValueW			[ADVAPI32.@]
 *
 * See LookupPrivilegeValueA.
 */
BOOL WINAPI
LookupPrivilegeValueW( LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid )
{
    UINT i;
    WCHAR priv[0x28];

    TRACE("%s,%s,%p\n",debugstr_w(lpSystemName), debugstr_w(lpName), lpLuid);

    for( i=0; i<NUMPRIVS; i++ )
    {
        if( !DefaultPrivNames[i] )
            continue;
        MultiByteToWideChar( CP_ACP, 0, DefaultPrivNames[i], -1,
                             priv, sizeof priv );
        if( strcmpW( priv, lpName) )
            continue;
        lpLuid->LowPart = i;
        lpLuid->HighPart = 0;
        TRACE( "%s -> %08lx-%08lx\n",debugstr_w( lpSystemName ),
               lpLuid->HighPart, lpLuid->LowPart );
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * LookupPrivilegeValueA			[ADVAPI32.@]
 *
 * Retrieves LUID used on a system to represent the privilege name.
 *
 * PARAMS
 *  lpSystemName [I] Name of the system
 *  lpName       [I] Name of the privilege
 *  lpLuid       [O] Destination for the resulting LUID
 *
 * RETURNS
 *  Success: TRUE. lpLuid contains the requested LUID.
 *  Failure: FALSE.
 */
BOOL WINAPI
LookupPrivilegeValueA( LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid )
{
    UNICODE_STRING lpSystemNameW;
    UNICODE_STRING lpNameW;
    BOOL ret;

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpNameW,lpName);
    ret = LookupPrivilegeValueW(lpSystemNameW.Buffer, lpNameW.Buffer, lpLuid);
    RtlFreeUnicodeString(&lpNameW);
    RtlFreeUnicodeString(&lpSystemNameW);
    return ret;
}


/******************************************************************************
 * LookupPrivilegeNameA			[ADVAPI32.@]
 */
BOOL WINAPI
LookupPrivilegeNameA( LPCSTR lpSystemName, PLUID lpLuid, LPSTR lpName, LPDWORD cchName)
{
    FIXME("%s %p %p %p\n", debugstr_a(lpSystemName), lpLuid, lpName, cchName);
    return FALSE;
}

/******************************************************************************
 * LookupPrivilegeNameW			[ADVAPI32.@]
 */
BOOL WINAPI
LookupPrivilegeNameW( LPCWSTR lpSystemName, PLUID lpLuid, LPWSTR lpName, LPDWORD cchName)
{
    FIXME("%s %p %p %p\n", debugstr_w(lpSystemName), lpLuid, lpName, cchName);
    return FALSE;
}

/******************************************************************************
 * GetFileSecurityA [ADVAPI32.@]
 *
 * Obtains Specified information about the security of a file or directory.
 *
 * PARAMS
 *  lpFileName           [I] Name of the file to get info for
 *  RequestedInformation [I] SE_ flags from "winnt.h"
 *  pSecurityDescriptor  [O] Destination for security information
 *  nLength              [I] Length of pSecurityDescriptor
 *  lpnLengthNeeded      [O] Destination for length of returned security information
 *
 * RETURNS
 *  Success: TRUE. pSecurityDescriptor contains the requested information.
 *  Failure: FALSE. lpnLengthNeeded contains the required space to return the info. 
 *
 * NOTES
 *  The information returned is constrained by the callers access rights and
 *  privileges.
 */
BOOL WINAPI
GetFileSecurityA( LPCSTR lpFileName,
                    SECURITY_INFORMATION RequestedInformation,
                    PSECURITY_DESCRIPTOR pSecurityDescriptor,
                    DWORD nLength, LPDWORD lpnLengthNeeded )
{
    DWORD len;
    BOOL r;
    LPWSTR name = NULL;

    if( lpFileName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpFileName, -1, NULL, 0 );
        name = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpFileName, -1, name, len );
    }

    r = GetFileSecurityW( name, RequestedInformation, pSecurityDescriptor,
                          nLength, lpnLengthNeeded );
    if( name )
        HeapFree( GetProcessHeap(), 0, name );

    return r;
}

/******************************************************************************
 * GetFileSecurityW [ADVAPI32.@]
 *
 * See GetFileSecurityA.
 */
BOOL WINAPI
GetFileSecurityW( LPCWSTR lpFileName,
                    SECURITY_INFORMATION RequestedInformation,
                    PSECURITY_DESCRIPTOR pSecurityDescriptor,
                    DWORD nLength, LPDWORD lpnLengthNeeded )
{
  FIXME("(%s) : stub\n", debugstr_w(lpFileName) );
  return TRUE;
}


/******************************************************************************
 * LookupAccountSidA [ADVAPI32.@]
 */
BOOL WINAPI
LookupAccountSidA(
	IN LPCSTR system,
	IN PSID sid,
	OUT LPSTR account,
	IN OUT LPDWORD accountSize,
	OUT LPSTR domain,
	IN OUT LPDWORD domainSize,
	OUT PSID_NAME_USE name_use )
{
	static const char ac[] = "Administrator";
	static const char dm[] = "DOMAIN";
	FIXME("(%s,sid=%p,%p,%p(%lu),%p,%p(%lu),%p): semi-stub\n",
	      debugstr_a(system),sid,
	      account,accountSize,accountSize?*accountSize:0,
	      domain,domainSize,domainSize?*domainSize:0,
	      name_use);

	if (accountSize) *accountSize = strlen(ac)+1;
	if (account && (*accountSize > strlen(ac)))
	  strcpy(account, ac);

	if (domainSize) *domainSize = strlen(dm)+1;
	if (domain && (*domainSize > strlen(dm)))
	  strcpy(domain,dm);

	if (name_use) *name_use = SidTypeUser;
	return TRUE;
}

/******************************************************************************
 * LookupAccountSidW [ADVAPI32.@]
 *
 * PARAMS
 *   system      []
 *   sid         []
 *   account     []
 *   accountSize []
 *   domain      []
 *   domainSize  []
 *   name_use    []
 */
BOOL WINAPI
LookupAccountSidW(
	IN LPCWSTR system,
	IN PSID sid,
	OUT LPWSTR account,
	IN OUT LPDWORD accountSize,
	OUT LPWSTR domain,
	IN OUT LPDWORD domainSize,
	OUT PSID_NAME_USE name_use )
{
    static const WCHAR ac[] = {'A','d','m','i','n','i','s','t','r','a','t','o','r',0};
    static const WCHAR dm[] = {'D','O','M','A','I','N',0};
	FIXME("(%s,sid=%p,%p,%p(%lu),%p,%p(%lu),%p): semi-stub\n",
	      debugstr_w(system),sid,
	      account,accountSize,accountSize?*accountSize:0,
	      domain,domainSize,domainSize?*domainSize:0,
	      name_use);

	if (accountSize) *accountSize = strlenW(ac)+1;
	if (account && (*accountSize > strlenW(ac)))
            strcpyW(account, ac);

	if (domainSize) *domainSize = strlenW(dm)+1;
	if (domain && (*domainSize > strlenW(dm)))
            strcpyW(domain,dm);

	if (name_use) *name_use = SidTypeUser;
	return TRUE;
}

/******************************************************************************
 * SetFileSecurityA [ADVAPI32.@]
 * Sets the security of a file or directory
 */
BOOL WINAPI SetFileSecurityA( LPCSTR lpFileName,
                                SECURITY_INFORMATION RequestedInformation,
                                PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    DWORD len;
    BOOL r;
    LPWSTR name = NULL;

    if( lpFileName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpFileName, -1, NULL, 0 );
        name = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpFileName, -1, name, len );
    }

    r = SetFileSecurityW( name, RequestedInformation, pSecurityDescriptor );
    if( name )
        HeapFree( GetProcessHeap(), 0, name );

    return r;
}

/******************************************************************************
 * SetFileSecurityW [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * PARAMS
 *   lpFileName           []
 *   RequestedInformation []
 *   pSecurityDescriptor  []
 */
BOOL WINAPI
SetFileSecurityW( LPCWSTR lpFileName,
                    SECURITY_INFORMATION RequestedInformation,
                    PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
  FIXME("(%s) : stub\n", debugstr_w(lpFileName) );
  return TRUE;
}

/******************************************************************************
 * QueryWindows31FilesMigration [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
BOOL WINAPI
QueryWindows31FilesMigration( DWORD x1 )
{
	FIXME("(%ld):stub\n",x1);
	return TRUE;
}

/******************************************************************************
 * SynchronizeWindows31FilesAndWindowsNTRegistry [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,
                                               DWORD x4 )
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub\n",x1,x2,x3,x4);
	return TRUE;
}

/******************************************************************************
 * LsaOpenPolicy [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
NTSTATUS WINAPI
LsaOpenPolicy(
	IN PLSA_UNICODE_STRING SystemName,
	IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
	IN ACCESS_MASK DesiredAccess,
	IN OUT PLSA_HANDLE PolicyHandle)
{
	FIXME("(%s,%p,0x%08lx,%p):stub\n",
              SystemName?debugstr_w(SystemName->Buffer):"null",
	      ObjectAttributes, DesiredAccess, PolicyHandle);
        ADVAPI_ForceLocalComputer(SystemName ? SystemName->Buffer : NULL,
                                  STATUS_ACCESS_VIOLATION);
	dumpLsaAttributes(ObjectAttributes);
	if(PolicyHandle) *PolicyHandle = (LSA_HANDLE)0xcafe;
	return STATUS_SUCCESS;
}

/******************************************************************************
 * LsaQueryInformationPolicy [ADVAPI32.@]
 */
NTSTATUS WINAPI
LsaQueryInformationPolicy(
	IN LSA_HANDLE PolicyHandle,
        IN POLICY_INFORMATION_CLASS InformationClass,
	OUT PVOID *Buffer)
{
	FIXME("(%p,0x%08x,%p):stub\n",
              PolicyHandle, InformationClass, Buffer);

	if(!Buffer) return FALSE;
	switch (InformationClass)
	{
	  case PolicyAuditEventsInformation: /* 2 */
	    {
	      PPOLICY_AUDIT_EVENTS_INFO p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POLICY_AUDIT_EVENTS_INFO));
	      p->AuditingMode = FALSE; /* no auditing */
	      *Buffer = p;
	    }
	    break;
	  case PolicyPrimaryDomainInformation: /* 3 */
	  case PolicyAccountDomainInformation: /* 5 */
	    {
	      struct di
	      { POLICY_PRIMARY_DOMAIN_INFO ppdi;
		SID sid;
	      };
	      SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};

	      struct di * xdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(xdi));
              HKEY key;
              BOOL useDefault = TRUE;
              LONG ret;

              if ((ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
               "System\\CurrentControlSet\\Services\\VxD\\VNETSUP", 0,
               KEY_READ, &key)) == ERROR_SUCCESS)
              {
                  DWORD size = 0;
                  static const WCHAR wg[] = { 'W','o','r','k','g','r','o','u','p',0 };

                  ret = RegQueryValueExW(key, wg, NULL, NULL, NULL, &size);
                  if (ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
                  {
                      xdi->ppdi.Name.Buffer = HeapAlloc(GetProcessHeap(),
                       HEAP_ZERO_MEMORY, size);
                      if ((ret = RegQueryValueExW(key, wg, NULL, NULL,
                       (LPBYTE)xdi->ppdi.Name.Buffer, &size)) == ERROR_SUCCESS)
                      {
                          xdi->ppdi.Name.Length = (USHORT)size;
                          useDefault = FALSE;
                      }
                      else
                      {
                          HeapFree(GetProcessHeap(), 0, xdi->ppdi.Name.Buffer);
                          xdi->ppdi.Name.Buffer = NULL;
                      }
                  }
                  RegCloseKey(key);
              }
              if (useDefault)
                  RtlCreateUnicodeStringFromAsciiz(&(xdi->ppdi.Name), "DOMAIN");
              TRACE("setting domain to %s\n", debugstr_w(xdi->ppdi.Name.Buffer));

	      xdi->ppdi.Sid = &(xdi->sid);
	      xdi->sid.Revision = SID_REVISION;
	      xdi->sid.SubAuthorityCount = 1;
	      xdi->sid.IdentifierAuthority = localSidAuthority;
	      xdi->sid.SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
	      *Buffer = xdi;
	    }
	    break;
	  case 	PolicyAuditLogInformation:
	  case 	PolicyPdAccountInformation:
	  case 	PolicyLsaServerRoleInformation:
	  case 	PolicyReplicaSourceInformation:
	  case 	PolicyDefaultQuotaInformation:
	  case 	PolicyModificationInformation:
	  case 	PolicyAuditFullSetInformation:
	  case 	PolicyAuditFullQueryInformation:
	  case 	PolicyDnsDomainInformation:
	    {
	      FIXME("category not implemented\n");
	      return FALSE;
	    }
	}
	return TRUE;
}

/******************************************************************************
 * LsaLookupSids [ADVAPI32.@]
 */
NTSTATUS WINAPI
LsaLookupSids(
	IN LSA_HANDLE PolicyHandle,
	IN ULONG Count,
	IN PSID *Sids,
	OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
	OUT PLSA_TRANSLATED_NAME *Names )
{
	FIXME("%p %lu %p %p %p\n",
	  PolicyHandle, Count, Sids, ReferencedDomains, Names);
	return FALSE;
}

/******************************************************************************
 * LsaFreeMemory [ADVAPI32.@]
 */
NTSTATUS WINAPI
LsaFreeMemory(IN PVOID Buffer)
{
	TRACE("(%p)\n",Buffer);
	return HeapFree(GetProcessHeap(), 0, Buffer);
}
/******************************************************************************
 * LsaClose [ADVAPI32.@]
 */
NTSTATUS WINAPI
LsaClose(IN LSA_HANDLE ObjectHandle)
{
	FIXME("(%p):stub\n",ObjectHandle);
	return 0xc0000000;
}

/******************************************************************************
 * LsaNtStatusToWinError [ADVAPI32.@]
 *
 * PARAMS
 *   Status [I]
 */
ULONG WINAPI
LsaNtStatusToWinError(NTSTATUS Status)
{
    return RtlNtStatusToDosError(Status);
}

/******************************************************************************
 * NotifyBootConfigStatus [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
BOOL WINAPI
NotifyBootConfigStatus( DWORD x1 )
{
	FIXME("(0x%08lx):stub\n",x1);
	return 1;
}

/******************************************************************************
 * RevertToSelf [ADVAPI32.@]
 *
 * PARAMS
 *   void []
 */
BOOL WINAPI
RevertToSelf( void )
{
	FIXME("(), stub\n");
	return TRUE;
}

/******************************************************************************
 * ImpersonateSelf [ADVAPI32.@]
 */
BOOL WINAPI
ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
	return RtlImpersonateSelf(ImpersonationLevel);
}

/******************************************************************************
 * ImpersonateLoggedOnUser [ADVAPI32.@]
 */
BOOL WINAPI ImpersonateLoggedOnUser(HANDLE hToken)
{
    FIXME("(%p):stub returning FALSE\n", hToken);
    return FALSE;
}

/******************************************************************************
 * AccessCheck [ADVAPI32.@]
 *
 * FIXME check cast LPBOOL to PBOOLEAN
 */
BOOL WINAPI
AccessCheck(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	HANDLE ClientToken,
	DWORD DesiredAccess,
	PGENERIC_MAPPING GenericMapping,
	PPRIVILEGE_SET PrivilegeSet,
	LPDWORD PrivilegeSetLength,
	LPDWORD GrantedAccess,
	LPBOOL AccessStatus)
{
	CallWin32ToNt (NtAccessCheck(SecurityDescriptor, ClientToken, DesiredAccess,
	  GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, (PBOOLEAN)AccessStatus));
}


/******************************************************************************
 * AccessCheckByType [ADVAPI32.@]
 */
BOOL WINAPI AccessCheckByType(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    PSID PrincipalSelfSid,
    HANDLE ClientToken, 
    DWORD DesiredAccess, 
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength, 
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus)
{
	FIXME("stub\n");

	*AccessStatus = TRUE;

	return !*AccessStatus;
}


/*************************************************************************
 * SetKernelObjectSecurity [ADVAPI32.@]
 */
BOOL WINAPI SetKernelObjectSecurity (
	IN HANDLE Handle,
	IN SECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor )
{
	CallWin32ToNt (NtSetSecurityObject (Handle, SecurityInformation, SecurityDescriptor));
}


/******************************************************************************
 *  AddAuditAccessAce [ADVAPI32.@]
 */
BOOL WINAPI AddAuditAccessAce(
        IN OUT PACL pAcl,
        IN DWORD dwAceRevision,
        IN DWORD dwAccessMask,
        IN PSID pSid,
        IN BOOL bAuditSuccess,
        IN BOOL bAuditFailure)
{
        FIXME("Stub\n");
        return TRUE;
}

/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 */
BOOL WINAPI
LookupAccountNameA(
	IN LPCSTR system,
	IN LPCSTR account,
	OUT PSID sid,
	OUT LPDWORD cbSid,
	LPSTR ReferencedDomainName,
	IN OUT LPDWORD cbReferencedDomainName,
	OUT PSID_NAME_USE name_use )
{
    FIXME("(%s,%s,%p,%p,%p,%p,%p), stub.\n",system,account,sid,cbSid,ReferencedDomainName,cbReferencedDomainName,name_use);
    return FALSE;
}

/******************************************************************************
 * PrivilegeCheck [ADVAPI32.@]
 */
BOOL WINAPI PrivilegeCheck( HANDLE ClientToken, PPRIVILEGE_SET RequiredPrivileges, LPBOOL pfResult)
{
	FIXME("stub %p %p %p\n", ClientToken, RequiredPrivileges, pfResult);
	if (pfResult)
		*pfResult=TRUE;
        return TRUE;
}

/******************************************************************************
 * AccessCheckAndAuditAlarmA [ADVAPI32.@]
 */
BOOL WINAPI AccessCheckAndAuditAlarmA(LPCSTR Subsystem, LPVOID HandleId, LPSTR ObjectTypeName,
  LPSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess,
  PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess,
  LPBOOL AccessStatus, LPBOOL pfGenerateOnClose)
{
	FIXME("stub (%s,%p,%s,%s,%p,%08lx,%p,%x,%p,%p,%p)\n", debugstr_a(Subsystem),
		HandleId, debugstr_a(ObjectTypeName), debugstr_a(ObjectName),
		SecurityDescriptor, DesiredAccess, GenericMapping,
		ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose);
	return TRUE;
}

/******************************************************************************
 * AccessCheckAndAuditAlarmW [ADVAPI32.@]
 */
BOOL WINAPI AccessCheckAndAuditAlarmW(LPCWSTR Subsystem, LPVOID HandleId, LPWSTR ObjectTypeName,
  LPWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess,
  PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess,
  LPBOOL AccessStatus, LPBOOL pfGenerateOnClose)
{
	FIXME("stub (%s,%p,%s,%s,%p,%08lx,%p,%x,%p,%p,%p)\n", debugstr_w(Subsystem),
		HandleId, debugstr_w(ObjectTypeName), debugstr_w(ObjectName),
		SecurityDescriptor, DesiredAccess, GenericMapping,
		ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose);
	return TRUE;
}


/******************************************************************************
 * GetSecurityInfoExW [ADVAPI32.@]
 */
DWORD WINAPI GetSecurityInfoExW(
	HANDLE hObject, SE_OBJECT_TYPE ObjectType, 
	SECURITY_INFORMATION SecurityInfo, LPCWSTR lpProvider,
	LPCWSTR lpProperty, PACTRL_ACCESSW *ppAccessList, 
	PACTRL_AUDITW *ppAuditList, LPWSTR *lppOwner, LPWSTR *lppGroup
)
{
  FIXME("stub!\n");
  return ERROR_BAD_PROVIDER; 
}

/******************************************************************************
 * BuildTrusteeWithSidA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithSidA(PTRUSTEEA pTrustee, PSID pSid)
{
    TRACE("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pSid;
}

/******************************************************************************
 * BuildTrusteeWithSidW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithSidW(PTRUSTEEW pTrustee, PSID pSid)
{
    TRACE("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pSid;
}

/******************************************************************************
 * BuildTrusteeWithNameA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithNameA(PTRUSTEEA pTrustee, LPSTR name)
{
    TRACE("%p %s\n", pTrustee, debugstr_a(name) );

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}

/******************************************************************************
 * BuildTrusteeWithNameW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithNameW(PTRUSTEEW pTrustee, LPWSTR name)
{
    TRACE("%p %s\n", pTrustee, debugstr_w(name) );

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}

/******************************************************************************
 * SetEntriesInAclA [ADVAPI32.@]
 */
DWORD WINAPI SetEntriesInAclA( ULONG count, PEXPLICIT_ACCESSA pEntries,
                               PACL OldAcl, PACL* NewAcl )
{
    FIXME("%ld %p %p %p\n",count,pEntries,OldAcl,NewAcl);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * SetEntriesInAclW [ADVAPI32.@]
 */
DWORD WINAPI SetEntriesInAclW( ULONG count, PEXPLICIT_ACCESSW pEntries,
                               PACL OldAcl, PACL* NewAcl )
{
    FIXME("%ld %p %p %p\n",count,pEntries,OldAcl,NewAcl);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * SetNamedSecurityInfoA [ADVAPI32.@]
 */
DWORD WINAPI SetNamedSecurityInfoA(LPSTR pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID psidOwner, PSID psidGroup, PACL pDacl, PACL pSacl)
{
    DWORD len;
    LPWSTR wstr = NULL;
    DWORD r;

    TRACE("%s %d %ld %p %p %p %p\n", debugstr_a(pObjectName), ObjectType,
           SecurityInfo, psidOwner, psidGroup, pDacl, pSacl);

    if( pObjectName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, NULL, 0 );
        wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, wstr, len );
    }

    r = SetNamedSecurityInfoW( wstr, ObjectType, SecurityInfo, psidOwner,
                           psidGroup, pDacl, pSacl );

    if( wstr )
        HeapFree( GetProcessHeap(), 0, wstr );

    return r;
}

/******************************************************************************
 * AreAnyAccessesGranted [ADVAPI32.@]
 *
 * Determines whether or not any of a set of specified access permissions have
 * been granted or not.
 *
 * PARAMS
 *   GrantedAccess [I] The permissions that have been granted.
 *   DesiredAccess [I] The permissions that you want to have.
 *
 * RETURNS
 *   Nonzero if any of the permissions have been granted, zero if none of the
 *   permissions have been granted.
 */

BOOL WINAPI AreAnyAccessesGranted( DWORD GrantedAccess, DWORD DesiredAccess )
{
    return (GrantedAccess & DesiredAccess) != 0;
}

/******************************************************************************
 * SetNamedSecurityInfoW [ADVAPI32.@]
 */
DWORD WINAPI SetNamedSecurityInfoW(LPWSTR pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID psidOwner, PSID psidGroup, PACL pDacl, PACL pSacl)
{
    FIXME("%s %d %ld %p %p %p %p\n", debugstr_w(pObjectName), ObjectType,
           SecurityInfo, psidOwner, psidGroup, pDacl, pSacl);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * GetExplicitEntriesFromAclA [ADVAPI32.@]
 */
DWORD WINAPI GetExplicitEntriesFromAclA( PACL pacl, PULONG pcCountOfExplicitEntries,
        PEXPLICIT_ACCESSA* pListOfExplicitEntries)
{
    FIXME("%p %p %p\n",pacl, pcCountOfExplicitEntries, pListOfExplicitEntries);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * GetExplicitEntriesFromAclW [ADVAPI32.@]
 */
DWORD WINAPI GetExplicitEntriesFromAclW( PACL pacl, PULONG pcCountOfExplicitEntries,
        PEXPLICIT_ACCESSW* pListOfExplicitEntries)
{
    FIXME("%p %p %p\n",pacl, pcCountOfExplicitEntries, pListOfExplicitEntries);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/******************************************************************************
 * ParseAclStringFlags
 */
static DWORD ParseAclStringFlags(LPCWSTR* StringAcl)
{
    DWORD flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl != '(')
    {
        if (*szAcl == 'P')
	{
            flags |= SE_DACL_PROTECTED;
	}
        else if (*szAcl == 'A')
        {
            szAcl++;
            if (*szAcl == 'R')
                flags |= SE_DACL_AUTO_INHERIT_REQ;
	    else if (*szAcl == 'I')
                flags |= SE_DACL_AUTO_INHERITED;
        }
        szAcl++;
    }

    *StringAcl = szAcl;
    return flags;
}

/******************************************************************************
 * ParseAceStringType
 */
ACEFLAG AceType[] =
{
    { SDDL_ACCESS_ALLOWED, ACCESS_ALLOWED_ACE_TYPE },
    { SDDL_ALARM,          SYSTEM_ALARM_ACE_TYPE },
    { SDDL_AUDIT,          SYSTEM_AUDIT_ACE_TYPE },
    { SDDL_ACCESS_DENIED,  ACCESS_DENIED_ACE_TYPE },
    /*
    { SDDL_OBJECT_ACCESS_ALLOWED, ACCESS_ALLOWED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ACCESS_DENIED,  ACCESS_DENIED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ALARM,          SYSTEM_ALARM_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_AUDIT,          SYSTEM_AUDIT_OBJECT_ACE_TYPE },
    */
    { NULL, 0 },
};

static BYTE ParseAceStringType(LPCWSTR* StringAcl)
{
    UINT len = 0;
    LPCWSTR szAcl = *StringAcl;
    LPACEFLAG lpaf = AceType;

    while (lpaf->wstr &&
        (len = strlenW(lpaf->wstr)) &&
        strncmpW(lpaf->wstr, szAcl, len))
        lpaf++;

    if (!lpaf->wstr)
        return 0;

    *StringAcl += len;
    return lpaf->value;
}


/******************************************************************************
 * ParseAceStringFlags
 */
ACEFLAG AceFlags[] =
{
    { SDDL_CONTAINER_INHERIT, CONTAINER_INHERIT_ACE },
    { SDDL_AUDIT_FAILURE,     FAILED_ACCESS_ACE_FLAG },
    { SDDL_INHERITED,         INHERITED_ACE },
    { SDDL_INHERIT_ONLY,      INHERIT_ONLY_ACE },
    { SDDL_NO_PROPAGATE,      NO_PROPAGATE_INHERIT_ACE },
    { SDDL_OBJECT_INHERIT,    OBJECT_INHERIT_ACE },
    { SDDL_AUDIT_SUCCESS,     SUCCESSFUL_ACCESS_ACE_FLAG },
    { NULL, 0 },
};

static BYTE ParseAceStringFlags(LPCWSTR* StringAcl)
{
    UINT len = 0;
    BYTE flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl != ';')
    {
        LPACEFLAG lpaf = AceFlags;

        while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
            lpaf++;

        if (!lpaf->wstr)
            return 0;

	flags |= lpaf->value;
        szAcl += len;
    }

    *StringAcl = szAcl;
    return flags;
}


/******************************************************************************
 * ParseAceStringRights
 */
ACEFLAG AceRights[] =
{
    { SDDL_GENERIC_ALL,     GENERIC_ALL },
    { SDDL_GENERIC_READ,    GENERIC_READ },
    { SDDL_GENERIC_WRITE,   GENERIC_WRITE },
    { SDDL_GENERIC_EXECUTE, GENERIC_EXECUTE },
    { SDDL_READ_CONTROL,    READ_CONTROL },
    { SDDL_STANDARD_DELETE, DELETE },
    { SDDL_WRITE_DAC,       WRITE_DAC },
    { SDDL_WRITE_OWNER,     WRITE_OWNER },
    { NULL, 0 },
};

static DWORD ParseAceStringRights(LPCWSTR* StringAcl)
{
    UINT len = 0;
    DWORD rights = 0;
    LPCWSTR szAcl = *StringAcl;

    if ((*szAcl == '0') && (*(szAcl + 1) == 'x'))
    {
        LPCWSTR p = szAcl;

	while (*p && *p != ';')
            p++;

	if (p - szAcl <= 8)
	{
	    rights = strtoulW(szAcl, NULL, 16);
	    *StringAcl = p;
	}
	else
            WARN("Invalid rights string format: %s\n", debugstr_wn(szAcl, p - szAcl));
    }
    else
    {
        while (*szAcl != ';')
        {
            LPACEFLAG lpaf = AceRights;

            while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
	    {
               lpaf++;
	    }

            if (!lpaf->wstr)
                return 0;

	    rights |= lpaf->value;
            szAcl += len;
        }
    }

    *StringAcl = szAcl;
    return rights;
}


/******************************************************************************
 * ParseStringAclToAcl
 * 
 * dacl_flags(string_ace1)(string_ace2)... (string_acen) 
 */
static BOOL ParseStringAclToAcl(LPCWSTR StringAcl, LPDWORD lpdwFlags, 
    PACL pAcl, LPDWORD cBytes)
{
    DWORD val;
    DWORD sidlen;
    DWORD length = sizeof(ACL);
    PACCESS_ALLOWED_ACE pAce = NULL; /* pointer to current ACE */

    TRACE("%s\n", debugstr_w(StringAcl));

    if (!StringAcl)
	return FALSE;

    if (pAcl) /* pAce is only useful if we're setting values */
        pAce = (PACCESS_ALLOWED_ACE) ((LPBYTE)pAcl + sizeof(PACL));

    /* Parse ACL flags */
    *lpdwFlags = ParseAclStringFlags(&StringAcl);

    /* Parse ACE */
    while (*StringAcl == '(')
    {
        StringAcl++;

        /* Parse ACE type */
        val = ParseAceStringType(&StringAcl);
	if (pAce)
            pAce->Header.AceType = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE flags */
	val = ParseAceStringFlags(&StringAcl);
	if (pAce)
            pAce->Header.AceFlags = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE rights */
	val = ParseAceStringRights(&StringAcl);
	if (pAce)
            pAce->Mask = val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE object guid */
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE inherit object guid */
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE account sid */
        if (ParseStringSidToSid(StringAcl, pAce ? (PSID)&pAce->SidStart : NULL, &sidlen))
	{
            while (*StringAcl && *StringAcl != ')')
                StringAcl++;
	}

        if (*StringAcl != ')')
            goto lerr;
        StringAcl++;

	length += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidlen;
    }

    *cBytes = length;
    return TRUE;

lerr:
    WARN("Invalid ACE string format\n");
    return FALSE;
}


/******************************************************************************
 * ParseStringSecurityDescriptorToSecurityDescriptor
 */
static BOOL ParseStringSecurityDescriptorToSecurityDescriptor(
    LPCWSTR StringSecurityDescriptor,
    SECURITY_DESCRIPTOR* SecurityDescriptor,
    LPDWORD cBytes)
{
    BOOL bret = FALSE;
    WCHAR toktype;
    WCHAR tok[MAX_PATH];
    LPCWSTR lptoken;
    LPBYTE lpNext = NULL;

    *cBytes = 0;

    if (SecurityDescriptor)
        lpNext = ((LPBYTE) SecurityDescriptor) + sizeof(SECURITY_DESCRIPTOR);

    while (*StringSecurityDescriptor)
    {
        toktype = *StringSecurityDescriptor;

	/* Expect char identifier followed by ':' */
	StringSecurityDescriptor++;
        if (*StringSecurityDescriptor != ':')
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto lend;
        }
	StringSecurityDescriptor++;

	/* Extract token */
	lptoken = StringSecurityDescriptor;
	while (*lptoken && *lptoken != ':')
            lptoken++;

	if (*lptoken)
            lptoken--;

	strncpyW(tok, StringSecurityDescriptor, lptoken - StringSecurityDescriptor);

        switch (toktype)
	{
            case 'O':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, (PSID)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Owner = (PSID) ((DWORD) lpNext -
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
                }

		*cBytes += bytes;

                break;
            }

            case 'G':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, (PSID)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Group = (PSID) ((DWORD) lpNext - 
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
                }

		*cBytes += bytes;

                break;
            }

            case 'D':
	    {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_DACL_PRESENT | flags;
                    SecurityDescriptor->Dacl = (PACL) ((DWORD) lpNext -
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
		}

		*cBytes += bytes;

		break;
            }

            case 'S':
            {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_SACL_PRESENT | flags;
                    SecurityDescriptor->Sacl = (PACL) ((DWORD) lpNext -
                        (DWORD) SecurityDescriptor);
                    lpNext += bytes; /* Advance to next token */
		}

		*cBytes += bytes;

		break;
            }

            default:
                FIXME("Unknown token\n");
                SetLastError(ERROR_INVALID_PARAMETER);
		goto lend;
	}

        StringSecurityDescriptor = lptoken;
    }

    bret = TRUE;

lend:
    return bret;
}

/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorW [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(
        LPCWSTR StringSecurityDescriptor,
        DWORD StringSDRevision,
        PSECURITY_DESCRIPTOR* SecurityDescriptor,
        PULONG SecurityDescriptorSize)
{
    DWORD cBytes;
    SECURITY_DESCRIPTOR* psd;
    BOOL bret = FALSE;

    TRACE("%s\n", debugstr_w(StringSecurityDescriptor));

    if (GetVersion() & 0x80000000)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto lend;
    }
    else if (StringSDRevision != SID_REVISION)
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
	goto lend;
    }

    /* Compute security descriptor length */
    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
        NULL, &cBytes))
	goto lend;

    psd = *SecurityDescriptor = (SECURITY_DESCRIPTOR*) LocalAlloc(
        GMEM_ZEROINIT, cBytes);

    psd->Revision = SID_REVISION;
    psd->Control |= SE_SELF_RELATIVE;

    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
        psd, &cBytes))
    {
        LocalFree(psd);
	goto lend;
    }

    if (SecurityDescriptorSize)
        *SecurityDescriptorSize = cBytes;

    bret = TRUE;
 
lend:
    TRACE(" ret=%d\n", bret);
    return bret;
}

/******************************************************************************
 * ConvertStringSidToSidW [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSidToSidW(LPCWSTR StringSid, PSID* Sid)
{
    BOOL bret = FALSE;
    DWORD cBytes;

    if (GetVersion() & 0x80000000)
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else if (ParseStringSidToSid(StringSid, NULL, &cBytes))
    {
        PSID pSid = *Sid = (PSID) LocalAlloc(0, cBytes);

        bret = ParseStringSidToSid(StringSid, pSid, &cBytes);
        if (!bret)
            LocalFree(*Sid); 
    }

    return bret;
}

/******************************************************************************
 * ConvertSidToStringSidW [ADVAPI32.@]
 *
 *  format of SID string is:
 *    S-<count>-<auth>-<subauth1>-<subauth2>-<subauth3>...
 *  where
 *    <rev> is the revision of the SID encoded as decimal
 *    <auth> is the identifier authority encoded as hex
 *    <subauthN> is the subauthority id encoded as decimal
 */
BOOL WINAPI ConvertSidToStringSidW( PSID pSid, LPWSTR *pstr )
{
    DWORD sz, i;
    LPWSTR str;
    WCHAR fmt[] = { 
        'S','-','%','u','-','%','2','X','%','2','X','%','X','%','X','%','X','%','X',0 };
    WCHAR subauthfmt[] = { '-','%','u',0 };
    SID* pisid=pSid;

    TRACE("%p %p\n", pSid, pstr );

    if( !IsValidSid( pSid ) )
        return FALSE;

    if (pisid->Revision != SDDL_REVISION)
        return FALSE;

    sz = 14 + pisid->SubAuthorityCount * 11;
    str = LocalAlloc( 0, sz*sizeof(WCHAR) );
    sprintfW( str, fmt, pisid->Revision,
         pisid->IdentifierAuthority.Value[2],
         pisid->IdentifierAuthority.Value[3],
         pisid->IdentifierAuthority.Value[0]&0x0f,
         pisid->IdentifierAuthority.Value[4]&0x0f,
         pisid->IdentifierAuthority.Value[1]&0x0f,
         pisid->IdentifierAuthority.Value[5]&0x0f);
    for( i=0; i<pisid->SubAuthorityCount; i++ )
        sprintfW( str + strlenW(str), subauthfmt, pisid->SubAuthority[i] );
    *pstr = str;

    return TRUE;
}

/******************************************************************************
 * ConvertSidToStringSidA [ADVAPI32.@]
 */
BOOL WINAPI ConvertSidToStringSidA(PSID pSid, LPSTR *pstr)
{
    LPWSTR wstr = NULL;
    LPSTR str;
    UINT len;

    TRACE("%p %p\n", pSid, pstr );

    if( !ConvertSidToStringSidW( pSid, &wstr ) )
        return FALSE;

    len = WideCharToMultiByte( CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL );
    str = LocalAlloc( 0, len );
    WideCharToMultiByte( CP_ACP, 0, wstr, -1, str, len, NULL, NULL );
    LocalFree( wstr );

    *pstr = str;

    return TRUE;
}

/******************************************************************************
 * ComputeStringSidSize
 */
static DWORD ComputeStringSidSize(LPCWSTR StringSid)
{
    int ctok = 0;
    DWORD size = sizeof(SID);

    while (*StringSid)
    {
        if (*StringSid == '-')
            ctok++;
        StringSid++;
    }

    if (ctok > 3)
        size += (ctok - 3) * sizeof(DWORD);

    return size;
}

/******************************************************************************
 * ParseStringSidToSid
 */
static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes)
{
    BOOL bret = FALSE;
    SID* pisid=pSid;

    if (!StringSid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    *cBytes = ComputeStringSidSize(StringSid);
    if (!pisid) /* Simply compute the size */
        return TRUE;

    if (*StringSid != 'S' || *StringSid != '-') /* S-R-I-S-S */
    {
        int i = 0;
	int csubauth = ((*cBytes - sizeof(SID)) / sizeof(DWORD)) + 1;

        StringSid += 2; /* Advance to Revision */
        pisid->Revision = atoiW(StringSid);

        if (pisid->Revision != SDDL_REVISION)
           goto lend; /* ERROR_INVALID_SID */

	pisid->SubAuthorityCount = csubauth;

	while (*StringSid && *StringSid != '-')
            StringSid++; /* Advance to identifier authority */

        pisid->IdentifierAuthority.Value[5] = atoiW(StringSid);

	if (pisid->IdentifierAuthority.Value[5] > 5)
            goto lend; /* ERROR_INVALID_SID */
    
        while (*StringSid)
	{	
	    while (*StringSid && *StringSid != '-')
                StringSid++;

            pisid->SubAuthority[i++] = atoiW(StringSid);
        }

	if (i != pisid->SubAuthorityCount)
            goto lend; /* ERROR_INVALID_SID */

        bret = TRUE;
    }
    else /* String constant format  - Only available in winxp and above */
    {
        pisid->Revision = SDDL_REVISION;
	pisid->SubAuthorityCount = 1;

	FIXME("String constant not supported: %s\n", debugstr_wn(StringSid, 2));

	/* TODO: Lookup string of well-known SIDs in table */
	pisid->IdentifierAuthority.Value[5] = 0;
	pisid->SubAuthority[0] = 0;

        bret = TRUE;
    }

lend:
    if (!bret)
        SetLastError(ERROR_INVALID_SID);

    return bret;
}

/******************************************************************************
 * GetNamedSecurityInfoA [ADVAPI32.@]
 */
DWORD WINAPI GetNamedSecurityInfoA(LPSTR pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID* ppsidOwner, PSID* ppsidGroup, PACL* ppDacl, PACL* ppSacl,
        PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    DWORD len;
    LPWSTR wstr = NULL;
    DWORD r;

    TRACE("%s %d %ld %p %p %p %p %p\n", pObjectName, ObjectType, SecurityInfo,
        ppsidOwner, ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor);

    if( pObjectName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, NULL, 0 );
        wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, wstr, len );
    }

    r = GetNamedSecurityInfoW( wstr, ObjectType, SecurityInfo, ppsidOwner,
                           ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor );

    if( wstr )
        HeapFree( GetProcessHeap(), 0, wstr );

    return r;
}

/******************************************************************************
 * GetNamedSecurityInfoW [ADVAPI32.@]
 */
DWORD WINAPI GetNamedSecurityInfoW(LPWSTR pObjectName,
        SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo,
        PSID* ppsidOwner, PSID* ppsidGroup, PACL* ppDacl, PACL* ppSacl,
        PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    FIXME("%s %d %ld %p %p %p %p %p\n", debugstr_w(pObjectName), ObjectType, SecurityInfo,
        ppsidOwner, ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
