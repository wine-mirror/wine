/*
 *	Security functions
 *
 *	Copyright 1996-1998 Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winestring.h"
#include "file.h"
#include "heap.h"
#include "winnls.h"
#include "debugtools.h"
#include "winerror.h"
#include "stackframe.h"

#include "ntddk.h"
#include "winreg.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

#define NT_SUCCESS(status) (status == STATUS_SUCCESS)

/*
 *	SID FUNCTIONS
 */

/******************************************************************************
 *  RtlAllocateAndInitializeSid		[NTDLL.265] 
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

	if (!(*pSid = HeapAlloc( GetProcessHeap(), 0, RtlLengthRequiredSid(nSubAuthorityCount))))
	  return FALSE;

	(*pSid)->Revision = SID_REVISION;

	if (pIdentifierAuthority)
	  memcpy(&(*pSid)->IdentifierAuthority, pIdentifierAuthority, sizeof (SID_IDENTIFIER_AUTHORITY));
	*GetSidSubAuthorityCount(*pSid) = nSubAuthorityCount;

	if (nSubAuthorityCount > 0)
          *GetSidSubAuthority(*pSid, 0) = nSubAuthority0;
	if (nSubAuthorityCount > 1)
          *GetSidSubAuthority(*pSid, 1) = nSubAuthority1;
	if (nSubAuthorityCount > 2)
          *GetSidSubAuthority(*pSid, 2) = nSubAuthority2;
	if (nSubAuthorityCount > 3)
          *GetSidSubAuthority(*pSid, 3) = nSubAuthority3;
	if (nSubAuthorityCount > 4)
          *GetSidSubAuthority(*pSid, 4) = nSubAuthority4;
	if (nSubAuthorityCount > 5)
          *GetSidSubAuthority(*pSid, 5) = nSubAuthority5;
        if (nSubAuthorityCount > 6)
	  *GetSidSubAuthority(*pSid, 6) = nSubAuthority6;
	if (nSubAuthorityCount > 7)
          *GetSidSubAuthority(*pSid, 7) = nSubAuthority7;

	return STATUS_SUCCESS;
}
/******************************************************************************
 *  RtlEqualSid		[NTDLL.352] 
 *
 */
BOOL WINAPI RtlEqualSid( PSID pSid1, PSID pSid2 )
{
    if (!RtlValidSid(pSid1) || !RtlValidSid(pSid2))
        return FALSE;

    if (*RtlSubAuthorityCountSid(pSid1) != *RtlSubAuthorityCountSid(pSid2))
        return FALSE;

    if (memcmp(pSid1, pSid2, GetLengthSid(pSid1)) != 0)
        return FALSE;

    return TRUE;
}

/******************************************************************************
 * RtlEqualPrefixSid	[ntdll.]
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
 *  RtlFreeSid		[NTDLL.376] 
 */
DWORD WINAPI RtlFreeSid(PSID pSid) 
{
	TRACE("(%p)\n", pSid);
	HeapFree( GetProcessHeap(), 0, pSid );
	return STATUS_SUCCESS;
}

/**************************************************************************
 * RtlLengthRequiredSid	[NTDLL.427]
 *
 * PARAMS
 *   nSubAuthorityCount []
 */
DWORD WINAPI RtlLengthRequiredSid(DWORD nrofsubauths)
{
	return (nrofsubauths-1)*sizeof(DWORD) + sizeof(SID);
}

/**************************************************************************
 *                 RtlLengthSid				[NTDLL.429]
 */
DWORD WINAPI RtlLengthSid(PSID pSid)
{
	TRACE("sid=%p\n",pSid);
	if (!pSid) return 0; 
	return RtlLengthRequiredSid(*RtlSubAuthorityCountSid(pSid));
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL.410]
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
	  *GetSidSubAuthority(pSid, i) = 0;

	return TRUE;
}

/**************************************************************************
 *                 RtlSubAuthoritySid			[NTDLL.497]
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
 * RtlIdentifierAuthoritySid	[NTDLL.395]
 *
 * PARAMS
 *   pSid []
 */
PSID_IDENTIFIER_AUTHORITY WINAPI RtlIdentifierAuthoritySid( PSID pSid )
{
	return &(pSid->IdentifierAuthority);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL.496]
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
 *                 RtlCopySid				[NTDLL.302]
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
 * RtlValidSid [NTDLL.532]
 *
 * PARAMS
 *   pSid []
 */
BOOL WINAPI
RtlValidSid( PSID pSid )
{
    if (IsBadReadPtr(pSid, 4))
    {
        WARN("(%p): invalid pointer!\n", pSid);
        return FALSE;
    }

    if (pSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)
        return FALSE;

    if (!pSid || pSid->Revision != SID_REVISION)
        return FALSE;

    return TRUE;
}


/*
 *	security descriptor functions
 */

/**************************************************************************
 * RtlCreateSecurityDescriptor			[NTDLL.313]
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
 * RtlValidSecurityDescriptor			[NTDLL.313]
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
 *  RtlLengthSecurityDescriptor			[NTDLL]
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
 *  RtlGetDaclSecurityDescriptor		[NTDLL] 
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
 *  RtlSetDaclSecurityDescriptor		[NTDLL.483]
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
 *  RtlGetSaclSecurityDescriptor		[NTDLL] 
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
 * RtlSetSaclSecurityDescriptor			[NTDLL.488]
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
 * RtlGetOwnerSecurityDescriptor		[NTDLL.488]
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
 *                 RtlSetOwnerSecurityDescriptor		[NTDLL.487]
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
 *                 RtlSetGroupSecurityDescriptor		[NTDLL.485]
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
 *                 RtlGetGroupSecurityDescriptor		[NTDLL]
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
 *                 RtlMakeSelfRelativeSD		[NTDLL]
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
 *                 RtlCreateAcl				[NTDLL.306]
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
 *                 RtlFirstFreeAce			[NTDLL.370]
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
 *                 RtlAddAce				[NTDLL.260]
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
 *  RtlAddAccessAllowedAce		[NTDLL] 
 */
BOOL WINAPI RtlAddAccessAllowedAce(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AccessMask,
	IN PSID pSid)
{
	FIXME("(%p,0x%08lx,0x%08lx,%p),stub!\n",
	pAcl, dwAceRevision, AccessMask, pSid);
	return 0;
}

/******************************************************************************
 *  RtlGetAce		[NTDLL] 
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
 *  RtlAdjustPrivilege		[NTDLL] 
 */
DWORD WINAPI RtlAdjustPrivilege(DWORD x1,DWORD x2,DWORD x3,DWORD x4) 
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  RtlImpersonateSelf		[NTDLL] 
 */
BOOL WINAPI
RtlImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
	FIXME("(%08x), stub\n", ImpersonationLevel);
	return TRUE;
}

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
	FIXME("(%p, %04x, %08lx, %p, %p, %p, %p, %p), stub\n",
          SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, 
          PrivilegeSet, ReturnLength, GrantedAccess, AccessStatus);
	*AccessStatus = TRUE;
	return STATUS_SUCCESS;
}

NTSTATUS WINAPI
NtSetSecurityObject(
        IN HANDLE Handle,
        IN SECURITY_INFORMATION SecurityInformation,
        IN PSECURITY_DESCRIPTOR SecurityDescriptor) 
{
	FIXME("0x%08x 0x%08lx %p\n", Handle, SecurityInformation, SecurityDescriptor);
	return STATUS_SUCCESS;
}

/******************************************************************************
 * RtlGetControlSecurityDescriptor
 */

NTSTATUS WINAPI RtlGetControlSecurityDescriptor(
	PSECURITY_DESCRIPTOR  pSecurityDescriptor,
	PSECURITY_DESCRIPTOR_CONTROL pControl,
	LPDWORD lpdwRevision)
{
	FIXME("(%p,%p,%p),stub!\n",pSecurityDescriptor,pControl,lpdwRevision);
	return STATUS_SUCCESS;
}		
