/*
 *	Security functions
 *
 *	Copyright 1996-1998 Marcus Meissner
 * 	Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
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

#include <stdarg.h>
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
#include "winnls.h"
#include "winerror.h"
#include "winreg.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "excpt.h"
#include "wine/library.h"
#include "wine/debug.h"

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

	if (!(*pSid = RtlAllocateHeap( ntdll_get_process_heap(), 0,
                                       RtlLengthRequiredSid(nSubAuthorityCount))))
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
 * Determine if two SIDs are equal.
 *
 * PARAMS
 *  pSid1 [I] Source SID
 *  pSid2 [I] SID to compare with
 *
 * RETURNS
 *  TRUE, if pSid1 is equal to pSid2,
 *  FALSE otherwise.
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
 *
 * Free the resources used by a SID.
 *
 * PARAMS
 *  pSid [I] SID to Free.
 *
 * RETURNS
 *  STATUS_SUCCESS.
 */
DWORD WINAPI RtlFreeSid(PSID pSid)
{
	TRACE("(%p)\n", pSid);
	RtlFreeHeap( ntdll_get_process_heap(), 0, pSid );
	return STATUS_SUCCESS;
}

/**************************************************************************
 * RtlLengthRequiredSid	[NTDLL.@]
 *
 * Determine the amount of memory a SID will use
 *
 * PARAMS
 *   nrofsubauths [I] Number of Sub Authorities in the SID.
 *
 * RETURNS
 *   The size, in bytes, of a SID with nrofsubauths Sub Authorities.
 */
DWORD WINAPI RtlLengthRequiredSid(DWORD nrofsubauths)
{
	return (nrofsubauths-1)*sizeof(DWORD) + sizeof(SID);
}

/**************************************************************************
 *                 RtlLengthSid				[NTDLL.@]
 *
 * Determine the amount of memory a SID is using
 *
 * PARAMS
 *  pSid [I] SID to ge the size of.
 *
 * RETURNS
 *  The size, in bytes, of pSid.
 */
DWORD WINAPI RtlLengthSid(PSID pSid)
{
	TRACE("sid=%p\n",pSid);
	if (!pSid) return 0;
	return RtlLengthRequiredSid(*RtlSubAuthorityCountSid(pSid));
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL.@]
 *
 * Initialise a SID.
 *
 * PARAMS
 *  pSid                 [I] SID to initialise
 *  pIdentifierAuthority [I] Identifier Authority
 *  nSubAuthorityCount   [I] Number of Sub Authorities
 *
 * RETURNS
 *  Success: TRUE. pSid is initialised withe the details given.
 *  Failure: FALSE, if nSubAuthorityCount is >= SID_MAX_SUB_AUTHORITIES.
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
 * Return the Sub Authority of a SID
 *
 * PARAMS
 *   pSid          [I] SID to get the Sub Authority from.
 *   nSubAuthority [I] Sub Authority number.
 *
 * RETURNS
 *   A pointer to The Sub Authority value of pSid.
 */
LPDWORD WINAPI RtlSubAuthoritySid( PSID pSid, DWORD nSubAuthority )
{
	return &(pSid->SubAuthority[nSubAuthority]);
}

/**************************************************************************
 * RtlIdentifierAuthoritySid	[NTDLL.@]
 *
 * Return the Identifier Authority of a SID.
 *
 * PARAMS
 *   pSid [I] SID to get the Identifier Authority from.
 *
 * RETURNS
 *   A pointer to the Identifier Authority value of pSid.
 */
PSID_IDENTIFIER_AUTHORITY WINAPI RtlIdentifierAuthoritySid( PSID pSid )
{
	return &(pSid->IdentifierAuthority);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL.@]
 *
 * Get the number of Sub Authorities in a SID.
 *
 * PARAMS
 *   pSid [I] SID to get the count from.
 *
 * RETURNS
 *  A pointer to the Sub Authority count of pSid.
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
 * Determine if a SID is valid.
 *
 * PARAMS
 *   pSid [I] SID to check
 *
 * RETURNS
 *   TRUE if pSid is valid,
 *   FALSE otherwise.
 */
BOOLEAN WINAPI RtlValidSid( PSID pSid )
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
 * Initialise a SECURITY_DESCRIPTOR.
 *
 * PARAMS
 *  lpsd [O] Descriptor to initialise.
 *  rev  [I] Revision, must be set to SECURITY_DESCRIPTOR_REVISION.
 *
 * RETURNS:
 *  Success: STATUS_SUCCESS.
 *  Failure: STATUS_UNKNOWN_REVISION if rev is incorrect.
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
 * Determine if a SECURITY_DESCRIPTOR is valid.
 *
 * PARAMS
 *  SecurityDescriptor [I] Descriptor to check.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: STATUS_INVALID_SECURITY_DESCR or STATUS_UNKNOWN_REVISION.
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
	ULONG offset = 0;
	ULONG Size = SECURITY_DESCRIPTOR_MIN_LENGTH;

	if ( SecurityDescriptor == NULL )
		return 0;

	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
		offset = (ULONG) SecurityDescriptor;

	if ( SecurityDescriptor->Owner != NULL )
		Size += RtlLengthSid((PSID)((LPBYTE)SecurityDescriptor->Owner + offset));

	if ( SecurityDescriptor->Group != NULL )
		Size += RtlLengthSid((PSID)((LPBYTE)SecurityDescriptor->Group + offset));

	if ( SecurityDescriptor->Sacl != NULL )
		Size += ((PACL)((LPBYTE)SecurityDescriptor->Sacl + offset))->AclSize;

	if ( SecurityDescriptor->Dacl != NULL )
		Size += ((PACL)((LPBYTE)SecurityDescriptor->Dacl + offset))->AclSize;

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

	if (SecurityDescriptor->Owner != NULL)
	{
            if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
                *Owner = (PSID)((LPBYTE)SecurityDescriptor +
                                (ULONG)SecurityDescriptor->Owner);
            else
                *Owner = SecurityDescriptor->Owner;

            if ( SecurityDescriptor->Control & SE_OWNER_DEFAULTED )
                *OwnerDefaulted = TRUE;
            else
                *OwnerDefaulted = FALSE;
        }
	else
	    *Owner = NULL;

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

	if (SecurityDescriptor->Group != NULL)
	{
            if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
                *Group = (PSID)((LPBYTE)SecurityDescriptor +
                                (ULONG)SecurityDescriptor->Group);
            else
                *Group = SecurityDescriptor->Group;

            if ( SecurityDescriptor->Control & SE_GROUP_DEFAULTED )
                *GroupDefaulted = TRUE;
            else
                *GroupDefaulted = FALSE;
	}
	else
	    *Group = NULL;

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
    ULONG offsetRel;
    ULONG length;
    PSECURITY_DESCRIPTOR pAbs = pAbsoluteSecurityDescriptor;
    PSECURITY_DESCRIPTOR pRel = pSelfRelativeSecurityDescriptor;

    TRACE(" %p %p %p(%ld)\n", pAbs, pRel, lpdwBufferLength,
        lpdwBufferLength ? *lpdwBufferLength: -1);

    if (!lpdwBufferLength || !pAbs)
        return STATUS_INVALID_PARAMETER;

    length = RtlLengthSecurityDescriptor(pAbs);
    if (*lpdwBufferLength < length)
    {
        *lpdwBufferLength = length;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!pRel)
        return STATUS_INVALID_PARAMETER;

    if (pAbs->Control & SE_SELF_RELATIVE)
    {
        memcpy(pRel, pAbs, length);
        return STATUS_SUCCESS;
    }

    pRel->Revision = pAbs->Revision;
    pRel->Sbz1 = pAbs->Sbz1;
    pRel->Control = pAbs->Control | SE_SELF_RELATIVE;

    offsetRel = sizeof(SECURITY_DESCRIPTOR);
    pRel->Owner = (PSID) offsetRel;
    length = RtlLengthSid(pAbs->Owner);
    memcpy((LPBYTE)pRel + offsetRel, pAbs->Owner, length);

    offsetRel += length;
    pRel->Group = (PSID) offsetRel;
    length = RtlLengthSid(pAbs->Group);
    memcpy((LPBYTE)pRel + offsetRel, pAbs->Group, length);

    if (pRel->Control & SE_SACL_PRESENT)
    {
        offsetRel += length;
        pRel->Sacl = (PACL) offsetRel;
        length = pAbs->Sacl->AclSize;
        memcpy((LPBYTE)pRel + offsetRel, pAbs->Sacl, length);
    }
    else
    {
        pRel->Sacl = NULL;
    }

    if (pRel->Control & SE_DACL_PRESENT)
    {
        offsetRel += length;
        pRel->Dacl = (PACL) offsetRel;
        length = pAbs->Dacl->AclSize;
        memcpy((LPBYTE)pRel + offsetRel, pAbs->Dacl, length);
    }
    else
    {
        pRel->Dacl = NULL;
    }

    return STATUS_SUCCESS;
}


/**************************************************************************
+ *                 RtlSelfRelativeToAbsoluteSD [NTDLL.@]
+ */
NTSTATUS WINAPI RtlSelfRelativeToAbsoluteSD(
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
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR pAbs = pAbsoluteSecurityDescriptor;
    PSECURITY_DESCRIPTOR pRel = pSelfRelativeSecurityDescriptor;

    if (!pRel ||
        !lpdwAbsoluteSecurityDescriptorSize ||
        !lpdwDaclSize ||
        !lpdwSaclSize ||
        !lpdwOwnerSize ||
        !lpdwPrimaryGroupSize ||
        ~pRel->Control & SE_SELF_RELATIVE)
        return STATUS_INVALID_PARAMETER;

    /* Confirm buffers are sufficiently large */
    if (*lpdwAbsoluteSecurityDescriptorSize < sizeof(SECURITY_DESCRIPTOR))
    {
        *lpdwAbsoluteSecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR);
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (pRel->Control & SE_DACL_PRESENT &&
        *lpdwDaclSize  < ((PACL)((LPBYTE)pRel->Dacl + (ULONG)pRel))->AclSize)
    {
        *lpdwDaclSize = ((PACL)((LPBYTE)pRel->Dacl + (ULONG)pRel))->AclSize;
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (pRel->Control & SE_SACL_PRESENT &&
        *lpdwSaclSize  < ((PACL)((LPBYTE)pRel->Sacl + (ULONG)pRel))->AclSize)
    {
        *lpdwSaclSize = ((PACL)((LPBYTE)pRel->Sacl + (ULONG)pRel))->AclSize;
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (pRel->Owner &&
        *lpdwOwnerSize < RtlLengthSid((PSID)((LPBYTE)pRel->Owner + (ULONG)pRel)))
    {
        *lpdwOwnerSize = RtlLengthSid((PSID)((LPBYTE)pRel->Owner + (ULONG)pRel));
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (pRel->Group &&
        *lpdwPrimaryGroupSize < RtlLengthSid((PSID)((LPBYTE)pRel->Group + (ULONG)pRel)))
    {
        *lpdwPrimaryGroupSize = RtlLengthSid((PSID)((LPBYTE)pRel->Group + (ULONG)pRel));
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (status != STATUS_SUCCESS)
        return status;

    /* Copy structures */
    pAbs->Revision = pRel->Revision;
    pAbs->Control = pRel->Control & ~SE_SELF_RELATIVE;

    if (pRel->Control & SE_SACL_PRESENT)
    {
        PACL pAcl = (PACL)((LPBYTE)pRel->Sacl + (ULONG)pRel);

        memcpy(pSacl, pAcl, pAcl->AclSize);
        pAbs->Sacl = pSacl;
    }

    if (pRel->Control & SE_DACL_PRESENT)
    {
        PACL pAcl = (PACL)((LPBYTE)pRel->Dacl + (ULONG)pRel);
        memcpy(pDacl, pAcl, pAcl->AclSize);
        pAbs->Dacl = pDacl;
    }

    if (pRel->Owner)
    {
        PSID psid = (PSID)((LPBYTE)pRel->Owner + (ULONG)pRel);
        memcpy(pOwner, psid, RtlLengthSid(psid));
        pAbs->Owner = pOwner;
    }

    if (pRel->Group)
    {
        PSID psid = (PSID)((LPBYTE)pRel->Group + (ULONG)pRel);
        memcpy(pPrimaryGroup, psid, RtlLengthSid(psid));
        pAbs->Group = pPrimaryGroup;
    }

    return status;
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
	return STATUS_SUCCESS;
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

/**************************************************************************
 *                 RtlDeleteAce				[NTDLL.@]
 */
NTSTATUS  WINAPI RtlDeleteAce(PACL pAcl, DWORD dwAceIndex)
{
	NTSTATUS status;
	PACE_HEADER pAce;

	status = RtlGetAce(pAcl,dwAceIndex,(LPVOID*)&pAce);

	if (STATUS_SUCCESS == status)
	{
		PACE_HEADER pcAce;
		DWORD len = 0;

		pcAce = (PACE_HEADER)(((BYTE*)pAce)+pAce->AceSize);
		for (; dwAceIndex < pAcl->AceCount; dwAceIndex++)
		{
			len += pcAce->AceSize;
			pcAce = (PACE_HEADER)(((BYTE*)pcAce) + pcAce->AceSize);
		}

		memcpy(pAce, ((BYTE*)pAce)+pAce->AceSize, len);
                pAcl->AceCount--;
	}

	return status;
}

/******************************************************************************
 *  RtlAddAccessAllowedAce		[NTDLL.@]
 */
NTSTATUS WINAPI RtlAddAccessAllowedAce(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AccessMask,
	IN PSID pSid)
{
	return RtlAddAccessAllowedAceEx( pAcl, dwAceRevision, 0, AccessMask, pSid);
}
 
/******************************************************************************
 *  RtlAddAccessAllowedAceEx		[NTDLL.@]
 */
NTSTATUS WINAPI RtlAddAccessAllowedAceEx(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AceFlags,
	IN DWORD AccessMask,
	IN PSID pSid)
{
	DWORD dwLengthSid;
	ACCESS_ALLOWED_ACE * pAaAce;
	DWORD dwSpaceLeft;

	TRACE("(%p,0x%08lx,0x%08lx,%p)\n",
		pAcl, dwAceRevision, AccessMask, pSid);

	if (!RtlValidSid(pSid))
		return STATUS_INVALID_SID;
	if (!RtlValidAcl(pAcl))
		return STATUS_INVALID_ACL;

	dwLengthSid = RtlLengthSid(pSid);
	if (!RtlFirstFreeAce(pAcl, (PACE_HEADER *) &pAaAce))
		return STATUS_INVALID_ACL;

	if (!pAaAce)
		return STATUS_ALLOTTED_SPACE_EXCEEDED;

	dwSpaceLeft = (DWORD)pAcl + pAcl->AclSize - (DWORD)pAaAce;
	if (dwSpaceLeft < sizeof(*pAaAce) - sizeof(pAaAce->SidStart) + dwLengthSid)
		return STATUS_ALLOTTED_SPACE_EXCEEDED;

	pAaAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
	pAaAce->Header.AceFlags = AceFlags;
	pAaAce->Header.AceSize = sizeof(*pAaAce) - sizeof(pAaAce->SidStart) + dwLengthSid;
	pAaAce->Mask = AccessMask;
	pAcl->AceCount++;
	RtlCopySid(dwLengthSid, (PSID)&pAaAce->SidStart, pSid);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlAddAccessDeniedAce		[NTDLL.@]
 */
NTSTATUS WINAPI RtlAddAccessDeniedAce(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AccessMask,
	IN PSID pSid)
{
	return RtlAddAccessDeniedAceEx( pAcl, dwAceRevision, 0, AccessMask, pSid);
}

/******************************************************************************
 *  RtlAddAccessDeniedAceEx		[NTDLL.@]
 */
NTSTATUS WINAPI RtlAddAccessDeniedAceEx(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AceFlags,
	IN DWORD AccessMask,
	IN PSID pSid)
{
	DWORD dwLengthSid;
	DWORD dwSpaceLeft;
	ACCESS_DENIED_ACE * pAdAce;

	TRACE("(%p,0x%08lx,0x%08lx,%p)\n",
		pAcl, dwAceRevision, AccessMask, pSid);

	if (!RtlValidSid(pSid))
		return STATUS_INVALID_SID;
	if (!RtlValidAcl(pAcl))
		return STATUS_INVALID_ACL;

	dwLengthSid = RtlLengthSid(pSid);
	if (!RtlFirstFreeAce(pAcl, (PACE_HEADER *) &pAdAce))
		return STATUS_INVALID_ACL;

	if (!pAdAce)
		return STATUS_ALLOTTED_SPACE_EXCEEDED;

	dwSpaceLeft = (DWORD)pAcl + pAcl->AclSize - (DWORD)pAdAce;
	if (dwSpaceLeft < sizeof(*pAdAce) - sizeof(pAdAce->SidStart) + dwLengthSid)
		return STATUS_ALLOTTED_SPACE_EXCEEDED;

	pAdAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
	pAdAce->Header.AceFlags = AceFlags;
	pAdAce->Header.AceSize = sizeof(*pAdAce) - sizeof(pAdAce->SidStart) + dwLengthSid;
	pAdAce->Mask = AccessMask;
	pAcl->AceCount++;
	RtlCopySid(dwLengthSid, (PSID)&pAdAce->SidStart, pSid);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlValidAcl		[NTDLL.@]
 */
BOOLEAN WINAPI RtlValidAcl(PACL pAcl)
{
        BOOLEAN ret;
	TRACE("(%p)\n", pAcl);

	__TRY
	{
		PACE_HEADER	ace;
		int		i;

                if (pAcl->AclRevision != ACL_REVISION)
                    ret = FALSE;
                else
                {
                    ace = (PACE_HEADER)(pAcl+1);
                    ret = TRUE;
                    for (i=0;i<=pAcl->AceCount;i++)
                    {
                        if ((char *)ace > (char *)pAcl + pAcl->AclSize)
                        {
                            ret = FALSE;
                            break;
                        }
                        ace = (PACE_HEADER)(((BYTE*)ace)+ace->AceSize);
                    }
                }
	}
	__EXCEPT(page_fault)
	{
		WARN("(%p): invalid pointer!\n", pAcl);
		return 0;
	}
	__ENDTRY
        return ret;
}

/******************************************************************************
 *  RtlGetAce		[NTDLL.@]
 */
DWORD WINAPI RtlGetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce )
{
	PACE_HEADER ace;

	TRACE("(%p,%ld,%p)\n",pAcl,dwAceIndex,pAce);

	if ((dwAceIndex < 0) || (dwAceIndex > pAcl->AceCount))
		return STATUS_INVALID_PARAMETER;

	ace = (PACE_HEADER)(pAcl + 1);
	for (;dwAceIndex;dwAceIndex--)
		ace = (PACE_HEADER)(((BYTE*)ace)+ace->AceSize);

	*pAce = (LPVOID) ace;

	return STATUS_SUCCESS;
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
 *  ZwAccessCheck		[NTDLL.@]
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

/******************************************************************************
 * RtlQueryInformationAcl (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryInformationAcl(
    PACL pAcl,
    LPVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass)
{
    NTSTATUS status = STATUS_SUCCESS;

    TRACE("pAcl=%p pAclInfo=%p len=%ld, class=%d\n", 
        pAcl, pAclInformation, nAclInformationLength, dwAclInformationClass);

    switch (dwAclInformationClass)
    {
        case AclRevisionInformation:
        {
            PACL_REVISION_INFORMATION paclrev = (PACL_REVISION_INFORMATION) pAclInformation;

            if (nAclInformationLength < sizeof(ACL_REVISION_INFORMATION))
                status = STATUS_INVALID_PARAMETER;
            else
                paclrev->AclRevision = pAcl->AclRevision;

            break;
        }

        case AclSizeInformation:
        {
            PACL_SIZE_INFORMATION paclsize = (PACL_SIZE_INFORMATION) pAclInformation;

            if (nAclInformationLength < sizeof(ACL_SIZE_INFORMATION))
                status = STATUS_INVALID_PARAMETER;
            else
            {
                INT i;
                PACE_HEADER ace;

                paclsize->AceCount = pAcl->AceCount;

                paclsize->AclBytesInUse = 0;
		ace = (PACE_HEADER) (pAcl + 1);

                for (i = 0; i < pAcl->AceCount; i++)
                {
                    paclsize->AclBytesInUse += ace->AceSize;
		    ace = (PACE_HEADER)(((BYTE*)ace)+ace->AceSize);
                }

		if (pAcl->AclSize < paclsize->AclBytesInUse)
                {
                    WARN("Acl has %ld bytes free\n", pAcl->AclSize - paclsize->AclBytesInUse);
                    paclsize->AclBytesFree = 0;
                    paclsize->AclBytesInUse = pAcl->AclSize;
                }
                else
                    paclsize->AclBytesFree = pAcl->AclSize - paclsize->AclBytesInUse;
            }

            break;
        }

        default:
            WARN("Unknown AclInformationClass value: %d\n", dwAclInformationClass);
            status = STATUS_INVALID_PARAMETER;
    }

    return status;
}
