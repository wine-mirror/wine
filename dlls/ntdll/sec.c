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
#include "winuser.h"
#include "wine/winestring.h"
#include "file.h"
#include "heap.h"
#include "winnls.h"
#include "debugstr.h"
#include "debug.h"
#include "winuser.h"
#include "winerror.h"
#include "stackframe.h"

#include "ntddk.h"
#include "winreg.h"

DEFAULT_DEBUG_CHANNEL(ntdll)

/*
 *	SID FUNCTIONS
 */

/******************************************************************************
 *  RtlAllocateAndInitializeSid		[NTDLL.265] 
 *
 */
BOOLEAN WINAPI RtlAllocateAndInitializeSid (PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	DWORD nSubAuthorityCount,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,DWORD x9,DWORD x10, PSID pSid) 
{
	FIXME(ntdll,"(%p,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,%p),stub!\n",
		pIdentifierAuthority,nSubAuthorityCount,x3,x4,x5,x6,x7,x8,x9,x10,pSid);
	return 0;
}
/******************************************************************************
 *  RtlEqualSid		[NTDLL.352] 
 *
 */
DWORD WINAPI RtlEqualSid(DWORD x1,DWORD x2) 
{	
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n", x1,x2);
	return TRUE;
}

/******************************************************************************
 *  RtlFreeSid		[NTDLL.376] 
 */
DWORD WINAPI RtlFreeSid(DWORD x1) 
{
	FIXME(ntdll,"(0x%08lx),stub!\n", x1);
	return TRUE;
}

/**************************************************************************
 *                 RtlLengthRequiredSid			[NTDLL.427]
 */
DWORD WINAPI RtlLengthRequiredSid(DWORD nrofsubauths)
{
	return sizeof(DWORD)*nrofsubauths+sizeof(SID);
}

/**************************************************************************
 *                 RtlLengthSid				[NTDLL.429]
 */
DWORD WINAPI RtlLengthSid(PSID sid)
{
	TRACE(ntdll,"sid=%p\n",sid);
	if (!sid)
	  return FALSE; 
	return sizeof(DWORD)*sid->SubAuthorityCount+sizeof(SID);
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL.410]
 */
DWORD WINAPI RtlInitializeSid(PSID PSID,PSID_IDENTIFIER_AUTHORITY PSIDauth,
                              DWORD c)
{
	BYTE	a = c&0xff;

	if (a>=SID_MAX_SUB_AUTHORITIES)
		return a;
	PSID->SubAuthorityCount = a;
	PSID->Revision		 = SID_REVISION;
	memcpy(&(PSID->IdentifierAuthority),PSIDauth,sizeof(SID_IDENTIFIER_AUTHORITY));
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlSubAuthoritySid			[NTDLL.497]
 */
LPDWORD WINAPI RtlSubAuthoritySid(PSID PSID,DWORD nr)
{
	return &(PSID->SubAuthority[nr]);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL.496]
 */

LPBYTE WINAPI RtlSubAuthorityCountSid(PSID PSID)
{
	return ((LPBYTE)PSID)+1;
}

/**************************************************************************
 *                 RtlCopySid				[NTDLL.302]
 */
DWORD WINAPI RtlCopySid(DWORD len,PSID to,PSID from)
{	if (!from)
		return 0;
	if (len<(from->SubAuthorityCount*4+8))
		return STATUS_BUFFER_TOO_SMALL;
	memmove(to,from,from->SubAuthorityCount*4+8);
	return STATUS_SUCCESS;
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
	TRACE(ntdll,"(%p,%p,%p,%p)\n",
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
	TRACE(ntdll,"(%p,%p,%p,%p)\n",
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

/*
 *	access control list's
 */

/**************************************************************************
 *                 RtlCreateAcl				[NTDLL.306]
 *
 * NOTES
 *    This should return NTSTATUS
 */
DWORD WINAPI RtlCreateAcl(PACL acl,DWORD size,DWORD rev)
{
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
DWORD WINAPI RtlAddAccessAllowedAce(DWORD x1,DWORD x2,DWORD x3,DWORD x4) 
{
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  RtlGetAce		[NTDLL] 
 */
DWORD WINAPI RtlGetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce ) 
{
	FIXME(ntdll,"(%p,%ld,%p),stub!\n",pAcl,dwAceIndex,pAce);
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
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

