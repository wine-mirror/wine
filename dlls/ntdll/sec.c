/*
 *	Security functions
 *
 *	Copyright 1996-1998 Marcus Meissner
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/exception.h"
#include "file.h"
#include "winnls.h"
#include "wine/debug.h"
#include "winerror.h"
#include "stackframe.h"

#include "winternl.h"
#include "winreg.h"
#include "ntdll_misc.h"
#include "msvcrt/excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

#define NT_SUCCESS(status) (status == STATUS_SUCCESS)

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/*
 *	SID FUNCTIONS
 */

/******************************************************************************
 *  RtlAllocateAndInitializeSid		[NTDLL.@]
 *
 */
BOOLEAN WINAPI RtlAllocateAndInitializeSid (
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	BYTE nSubAuthorityCount,
	DWORD nSubAuthority0, DWORD nSubAuthority1,
	DWORD nSubAuthority2, DWORD nSubAuthority3,
	DWORD nSubAuthority4, DWORD nSubAuthority5,
	DWORD nSubAuthority6, DWORD nSubAuthority7,
	PSID *pSid )
{
	TRACE("(%p, 0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,%p)\n",
		pIdentifierAuthority,nSubAuthorityCount,
		nSubAuthority0, nSubAuthority1,	nSubAuthority2, nSubAuthority3,
		nSubAuthority4, nSubAuthority5,	nSubAuthority6, nSubAuthority7, pSid);

	if (!(*pSid = RtlAllocateHeap( GetProcessHeap(), 0, RtlLengthRequiredSid(nSubAuthorityCount))))
	  return FALSE;

	(*pSid)->Revision = SID_REVISION;

	if (pIdentifierAuthority)
	  memcpy(&(*pSid)->IdentifierAuthority, pIdentifierAuthority, sizeof (SID_IDENTIFIER_AUTHORITY));
	*RtlSubAuthorityCountSid(*pSid) = nSubAuthorityCount;

	if (nSubAuthorityCount > 0)
          *RtlSubAuthoritySid(*pSid, 0) = nSubAuthority0;
	if (nSubAuthorityCount > 1)
          *RtlSubAuthoritySid(*pSid, 1) = nSubAuthority1;
	if (nSubAuthorityCount > 2)
          *RtlSubAuthoritySid(*pSid, 2) = nSubAuthority2;
	if (nSubAuthorityCount > 3)
          *RtlSubAuthoritySid(*pSid, 3) = nSubAuthority3;
	if (nSubAuthorityCount > 4)
          *RtlSubAuthoritySid(*pSid, 4) = nSubAuthority4;
	if (nSubAuthorityCount > 5)
          *RtlSubAuthoritySid(*pSid, 5) = nSubAuthority5;
        if (nSubAuthorityCount > 6)
	  *RtlSubAuthoritySid(*pSid, 6) = nSubAuthority6;
	if (nSubAuthorityCount > 7)
          *RtlSubAuthoritySid(*pSid, 7) = nSubAuthority7;

	return STATUS_SUCCESS;
}
/******************************************************************************
 *  RtlEqualSid		[NTDLL.@]
 *
 */
BOOL WINAPI RtlEqualSid( PSID pSid1, PSID pSid2 )
{
    if (!RtlValidSid(pSid1) || !RtlValidSid(pSid2))
        return FALSE;

    if (*RtlSubAuthorityCountSid(pSid1) != *RtlSubAuthorityCountSid(pSid2))
        return FALSE;

    if (memcmp(pSid1, pSid2, RtlLengthSid(pSid1)) != 0)
        return FALSE;

    return TRUE;
}

/******************************************************************************
 * RtlEqualPrefixSid	[NTDLL.@]
 */
BOOL WINAPI RtlEqualPrefixSid (PSID pSid1, PSID pSid2)
{
    if (!RtlValidSid(pSid1) || !RtlValidSid(pSid2))
        return FALSE;

    if (*RtlSubAuthorityCountSid(pSid1) != *RtlSubAuthorityCountSid(pSid2))
        return FALSE;

    if (memcmp(pSid1, pSid2, RtlLengthRequiredSid(pSid1->SubAuthorityCount - 1)) != 0)
        return FALSE;

    return TRUE;
}


/******************************************************************************
 *  RtlFreeSid		[NTDLL.@]
 */
DWORD WINAPI RtlFreeSid(PSID pSid)
{
	TRACE("(%p)\n", pSid);
	RtlFreeHeap( GetProcessHeap(), 0, pSid );
	return STATUS_SUCCESS;
}

/**************************************************************************
 * RtlLengthRequiredSid	[NTDLL.@]
 *
 * PARAMS
 *   nSubAuthorityCount []
 */
DWORD WINAPI RtlLengthRequiredSid(DWORD nrofsubauths)
{
	return (nrofsubauths-1)*sizeof(DWORD) + sizeof(SID);
}

/**************************************************************************
 *                 RtlLengthSid				[NTDLL.@]
 */
DWORD WINAPI RtlLengthSid(PSID pSid)
{
	TRACE("sid=%p\n",pSid);
	if (!pSid) return 0;
	return RtlLengthRequiredSid(*RtlSubAuthorityCountSid(pSid));
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL.@]
 */
BOOL WINAPI RtlInitializeSid(
	PSID pSid,
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	BYTE nSubAuthorityCount)
{
	int i;
	if (nSubAuthorityCount >= SID_MAX_SUB_AUTHORITIES)
	  return FALSE;

	pSid->Revision = SID_REVISION;
	pSid->SubAuthorityCount = nSubAuthorityCount;
	if (pIdentifierAuthority)
	  memcpy(&pSid->IdentifierAuthority, pIdentifierAuthority, sizeof (SID_IDENTIFIER_AUTHORITY));

	for (i = 0; i < nSubAuthorityCount; i++)
	  *RtlSubAuthoritySid(pSid, i) = 0;

	return TRUE;
}

/**************************************************************************
 *                 RtlSubAuthoritySid			[NTDLL.@]
 *
 * PARAMS
 *   pSid          []
 *   nSubAuthority []
 */
LPDWORD WINAPI RtlSubAuthoritySid( PSID pSid, DWORD nSubAuthority )
{
	return &(pSid->SubAuthority[nSubAuthority]);
}

/**************************************************************************
 * RtlIdentifierAuthoritySid	[NTDLL.@]
 *
 * PARAMS
 *   pSid []
 */
PSID_IDENTIFIER_AUTHORITY WINAPI RtlIdentifierAuthoritySid( PSID pSid )
{
	return &(pSid->IdentifierAuthority);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL.@]
 *
 * PARAMS
 *   pSid          []
 *   nSubAuthority []
 */
LPBYTE WINAPI RtlSubAuthorityCountSid(PSID pSid)
{
	return &(pSid->SubAuthorityCount);
}

/**************************************************************************
 *                 RtlCopySid				[NTDLL.@]
 */
DWORD WINAPI RtlCopySid( DWORD nDestinationSidLength, PSID pDestinationSid, PSID pSourceSid )
{
	if (!pSourceSid || !RtlValidSid(pSourceSid) ||
	    (nDestinationSidLength < RtlLengthSid(pSourceSid)))
          return FALSE;

	if (nDestinationSidLength < (pSourceSid->SubAuthorityCount*4+8))
	  return FALSE;

	memmove(pDestinationSid, pSourceSid, pSourceSid->SubAuthorityCount*4+8);
	return TRUE;
}
/******************************************************************************
 * RtlValidSid [NTDLL.@]
 *
 * PARAMS
 *   pSid []
 */
BOOL WINAPI
RtlValidSid( PSID pSid )
{
    BOOL ret;
    __TRY
    {
        ret = TRUE;
        if (!pSid || pSid->Revision != SID_REVISION ||
            pSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)
        {
            ret = FALSE;
        }
    }
    __EXCEPT(page_fault)
    {
        WARN("(%p): invalid pointer!\n", pSid);
        return FALSE;
    }
    __ENDTRY
    return ret;
}


/*
 *	security descriptor functions
 */

/**************************************************************************
 * RtlCreateSecurityDescriptor			[NTDLL.@]
 *
 * RETURNS:
 *  0 success,
 *  STATUS_INVALID_OWNER, STATUS_PRIVILEGE_NOT_HELD, STATUS_NO_INHERITANCE,
 *  STATUS_NO_MEMORY
 */
NTSTATUS WINAPI RtlCreateSecurityDescriptor(
	PSECURITY_DESCRIPTOR lpsd,
	DWORD rev)
{
	if (rev!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	memset(lpsd,'\0',sizeof(*lpsd));
	lpsd->Revision = SECURITY_DESCRIPTOR_REVISION;
	return STATUS_SUCCESS;
}
/**************************************************************************
 * RtlValidSecurityDescriptor			[NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlValidSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor)
{
	if ( ! SecurityDescriptor )
		return STATUS_INVALID_SECURITY_DESCR;
	if ( SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION )
		return STATUS_UNKNOWN_REVISION;

	return STATUS_SUCCESS;
}

/**************************************************************************
 *  RtlLengthSecurityDescriptor			[NTDLL.@]
 */
ULONG WINAPI RtlLengthSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor)
{
	ULONG Size;
	Size = SECURITY_DESCRIPTOR_MIN_LENGTH;
	if ( SecurityDescriptor == NULL )
		return 0;

	if ( SecurityDescriptor->Owner != NULL )
		Size += SecurityDescriptor->Owner->SubAuthorityCount;
	if ( SecurityDescriptor->Group != NULL )
		Size += SecurityDescriptor->Group->SubAuthorityCount;


	if ( SecurityDescriptor->Sacl != NULL )
		Size += SecurityDescriptor->Sacl->AclSize;
	if ( SecurityDescriptor->Dacl != NULL )
		Size += SecurityDescriptor->Dacl->AclSize;

	return Size;
}

/******************************************************************************
 *  RtlGetDaclSecurityDescriptor		[NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlGetDaclSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	OUT PBOOLEAN lpbDaclPresent,
	OUT PACL *pDacl,
	OUT PBOOLEAN lpbDaclDefaulted)
{
	TRACE("(%p,%p,%p,%p)\n",
	pSecurityDescriptor, lpbDaclPresent, *pDacl, lpbDaclDefaulted);

	if (pSecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
	  return STATUS_UNKNOWN_REVISION ;

	if ( (*lpbDaclPresent = (SE_DACL_PRESENT & pSecurityDescriptor->Control) ? 1 : 0) )
	{
	  if ( SE_SELF_RELATIVE & pSecurityDescriptor->Control)
	  { *pDacl = (PACL) ((LPBYTE)pSecurityDescriptor + (DWORD)pSecurityDescriptor->Dacl);
	  }
	  else
	  { *pDacl = pSecurityDescriptor->Dacl;
	  }
	}

	*lpbDaclDefaulted = (( SE_DACL_DEFAULTED & pSecurityDescriptor->Control ) ? 1 : 0);

	return STATUS_SUCCESS;
}

/**************************************************************************
 *  RtlSetDaclSecurityDescriptor		[NTDLL.@]
 */
NTSTATUS WINAPI RtlSetDaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	BOOLEAN daclpresent,
	PACL dacl,
	BOOLEAN dacldefaulted )
{
	if (lpsd->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (lpsd->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;

	if (!daclpresent)
	{	lpsd->Control &= ~SE_DACL_PRESENT;
		return TRUE;
	}

	lpsd->Control |= SE_DACL_PRESENT;
	lpsd->Dacl = dacl;

	if (dacldefaulted)
		lpsd->Control |= SE_DACL_DEFAULTED;
	else
		lpsd->Control &= ~SE_DACL_DEFAULTED;

	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlGetSaclSecurityDescriptor		[NTDLL.@]
 *
 */
NTSTATUS WINAPI RtlGetSaclSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
	OUT PBOOLEAN lpbSaclPresent,
	OUT PACL *pSacl,
	OUT PBOOLEAN lpbSaclDefaulted)
{
	TRACE("(%p,%p,%p,%p)\n",
	pSecurityDescriptor, lpbSaclPresent, *pSacl, lpbSaclDefaulted);

	if (pSecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
	  return STATUS_UNKNOWN_REVISION ;

	if ( (*lpbSaclPresent = (SE_SACL_PRESENT & pSecurityDescriptor->Control) ? 1 : 0) )
	{
	  if ( SE_SELF_RELATIVE & pSecurityDescriptor->Control)
	  { *pSacl = (PACL) ((LPBYTE)pSecurityDescriptor + (DWORD)pSecurityDescriptor->Sacl);
	  }
	  else
	  { *pSacl = pSecurityDescriptor->Sacl;
	  }
	}

	*lpbSaclDefaulted = (( SE_SACL_DEFAULTED & pSecurityDescriptor->Control ) ? 1 : 0);

	return STATUS_SUCCESS;
}

/**************************************************************************
 * RtlSetSaclSecurityDescriptor			[NTDLL.@]
 */
NTSTATUS WINAPI RtlSetSaclSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	BOOLEAN saclpresent,
	PACL sacl,
	BOOLEAN sacldefaulted)
{
	if (lpsd->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (lpsd->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;
	if (!saclpresent) {
		lpsd->Control &= ~SE_SACL_PRESENT;
		return 0;
	}
	lpsd->Control |= SE_SACL_PRESENT;
	lpsd->Sacl = sacl;
	if (sacldefaulted)
		lpsd->Control |= SE_SACL_DEFAULTED;
	else
		lpsd->Control &= ~SE_SACL_DEFAULTED;
	return STATUS_SUCCESS;
}

/**************************************************************************
 * RtlGetOwnerSecurityDescriptor		[NTDLL.@]
 */
NTSTATUS WINAPI RtlGetOwnerSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	PSID *Owner,
	PBOOLEAN OwnerDefaulted)
{
	if ( !SecurityDescriptor  || !Owner || !OwnerDefaulted )
		return STATUS_INVALID_PARAMETER;

	*Owner = SecurityDescriptor->Owner;
	if ( *Owner != NULL )  {
		if ( SecurityDescriptor->Control & SE_OWNER_DEFAULTED )
			*OwnerDefaulted = TRUE;
		else
			*OwnerDefaulted = FALSE;
	}
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlSetOwnerSecurityDescriptor		[NTDLL.@]
 */
NTSTATUS WINAPI RtlSetOwnerSecurityDescriptor(
	PSECURITY_DESCRIPTOR lpsd,
	PSID owner,
	BOOLEAN ownerdefaulted)
{
	if (lpsd->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (lpsd->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;

	lpsd->Owner = owner;
	if (ownerdefaulted)
		lpsd->Control |= SE_OWNER_DEFAULTED;
	else
		lpsd->Control &= ~SE_OWNER_DEFAULTED;
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlSetGroupSecurityDescriptor		[NTDLL.@]
 */
NTSTATUS WINAPI RtlSetGroupSecurityDescriptor (
	PSECURITY_DESCRIPTOR lpsd,
	PSID group,
	BOOLEAN groupdefaulted)
{
	if (lpsd->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (lpsd->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;

	lpsd->Group = group;
	if (groupdefaulted)
		lpsd->Control |= SE_GROUP_DEFAULTED;
	else
		lpsd->Control &= ~SE_GROUP_DEFAULTED;
	return STATUS_SUCCESS;
}
/**************************************************************************
 *                 RtlGetGroupSecurityDescriptor		[NTDLL.@]
 */
NTSTATUS WINAPI RtlGetGroupSecurityDescriptor(
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	PSID *Group,
	PBOOLEAN GroupDefaulted)
{
	if ( !SecurityDescriptor || !Group || !GroupDefaulted )
		return STATUS_INVALID_PARAMETER;

	*Group = SecurityDescriptor->Group;
	if ( *Group != NULL )  {
		if ( SecurityDescriptor->Control & SE_GROUP_DEFAULTED )
			*GroupDefaulted = TRUE;
		else
			*GroupDefaulted = FALSE;
	}
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlMakeSelfRelativeSD		[NTDLL.@]
 */
NTSTATUS WINAPI RtlMakeSelfRelativeSD(
	IN PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
	IN PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
	IN OUT LPDWORD lpdwBufferLength)
{
	FIXME("(%p,%p,%p(%lu))\n", pAbsoluteSecurityDescriptor,
	pSelfRelativeSecurityDescriptor, lpdwBufferLength,*lpdwBufferLength);
	return STATUS_SUCCESS;
}

/*
 *	access control list's
 */

/**************************************************************************
 *                 RtlCreateAcl				[NTDLL.@]
 *
 * NOTES
 *    This should return NTSTATUS
 */
NTSTATUS WINAPI RtlCreateAcl(PACL acl,DWORD size,DWORD rev)
{
	TRACE("%p 0x%08lx 0x%08lx\n", acl, size, rev);

	if (rev!=ACL_REVISION)
		return STATUS_INVALID_PARAMETER;
	if (size<sizeof(ACL))
		return STATUS_BUFFER_TOO_SMALL;
	if (size>0xFFFF)
		return STATUS_INVALID_PARAMETER;

	memset(acl,'\0',sizeof(ACL));
	acl->AclRevision	= rev;
	acl->AclSize		= size;
	acl->AceCount		= 0;
	return 0;
}

/**************************************************************************
 *                 RtlFirstFreeAce			[NTDLL.@]
 * looks for the AceCount+1 ACE, and if it is still within the alloced
 * ACL, return a pointer to it
 */
BOOLEAN WINAPI RtlFirstFreeAce(
	PACL acl,
	PACE_HEADER *x)
{
	PACE_HEADER	ace;
	int		i;

	*x = 0;
	ace = (PACE_HEADER)(acl+1);
	for (i=0;i<acl->AceCount;i++) {
		if ((DWORD)ace>=(((DWORD)acl)+acl->AclSize))
			return 0;
		ace = (PACE_HEADER)(((BYTE*)ace)+ace->AceSize);
	}
	if ((DWORD)ace>=(((DWORD)acl)+acl->AclSize))
		return 0;
	*x = ace;
	return 1;
}

/**************************************************************************
 *                 RtlAddAce				[NTDLL.@]
 */
NTSTATUS WINAPI RtlAddAce(
	PACL acl,
	DWORD rev,
	DWORD xnrofaces,
	PACE_HEADER acestart,
	DWORD acelen)
{
	PACE_HEADER	ace,targetace;
	int		nrofaces;

	if (acl->AclRevision != ACL_REVISION)
		return STATUS_INVALID_PARAMETER;
	if (!RtlFirstFreeAce(acl,&targetace))
		return STATUS_INVALID_PARAMETER;
	nrofaces=0;ace=acestart;
	while (((DWORD)ace-(DWORD)acestart)<acelen) {
		nrofaces++;
		ace = (PACE_HEADER)(((BYTE*)ace)+ace->AceSize);
	}
	if ((DWORD)targetace+acelen>(DWORD)acl+acl->AclSize) /* too much aces */
		return STATUS_INVALID_PARAMETER;
	memcpy((LPBYTE)targetace,acestart,acelen);
	acl->AceCount+=nrofaces;
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlAddAccessAllowedAce		[NTDLL.@]
 */
BOOL WINAPI RtlAddAccessAllowedAce(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AccessMask,
	IN PSID pSid)
{
	FIXME("(%p,0x%08lx,0x%08lx,%p),stub!\n",
	pAcl, dwAceRevision, AccessMask, pSid);
	return TRUE;
}

/******************************************************************************
 *  RtlGetAce		[NTDLL.@]
 */
DWORD WINAPI RtlGetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce )
{
	FIXME("(%p,%ld,%p),stub!\n",pAcl,dwAceIndex,pAce);
	return 0;
}

/*
 *	misc
 */

/******************************************************************************
 *  RtlAdjustPrivilege		[NTDLL.@]
 */
DWORD WINAPI RtlAdjustPrivilege(DWORD x1,DWORD x2,DWORD x3,DWORD x4)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  RtlImpersonateSelf		[NTDLL.@]
 */
BOOL WINAPI
RtlImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
	FIXME("(%08x), stub\n", ImpersonationLevel);
	return TRUE;
}

/******************************************************************************
 *  NtAccessCheck		[NTDLL.@]
 */
NTSTATUS WINAPI
NtAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ACCESS_MASK DesiredAccess,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PPRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PBOOLEAN AccessStatus)
{
	FIXME("(%p, %p, %08lx, %p, %p, %p, %p, %p), stub\n",
          SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping,
          PrivilegeSet, ReturnLength, GrantedAccess, AccessStatus);
	*AccessStatus = TRUE;
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtSetSecurityObject		[NTDLL.@]
 */
NTSTATUS WINAPI
NtSetSecurityObject(
        IN HANDLE Handle,
        IN SECURITY_INFORMATION SecurityInformation,
        IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
	FIXME("%p 0x%08lx %p\n", Handle, SecurityInformation, SecurityDescriptor);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * RtlGetControlSecurityDescriptor (NTDLL.@)
 */

NTSTATUS WINAPI RtlGetControlSecurityDescriptor(
	PSECURITY_DESCRIPTOR  pSecurityDescriptor,
	PSECURITY_DESCRIPTOR_CONTROL pControl,
	LPDWORD lpdwRevision)
{
	FIXME("(%p,%p,%p),stub!\n",pSecurityDescriptor,pControl,lpdwRevision);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * RtlConvertSidToUnicodeString (NTDLL.@)
 *
 * The returned SID is used to access the USER registry hive usually
 *
 * the native function returns something like
 * "S-1-5-21-0000000000-000000000-0000000000-500";
 */
NTSTATUS WINAPI RtlConvertSidToUnicodeString(
       PUNICODE_STRING String,
       PSID Sid,
       BOOLEAN AllocateString)
{
        const char *p = wine_get_user_name();
        NTSTATUS status;
        ANSI_STRING AnsiStr;

	FIXME("(%p %p %u)\n", String, Sid, AllocateString);

        RtlInitAnsiString(&AnsiStr, p);
        status = RtlAnsiStringToUnicodeString(String, &AnsiStr, AllocateString);

        TRACE("%s (%u %u)\n",debugstr_w(String->Buffer),String->Length,String->MaximumLength);
        return status;
}
