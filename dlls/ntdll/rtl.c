/*
 * NT basis DLL
 * 
 * This file contains the Rtl* API functions. These should be implementable.
 * 
 * Copyright 1996-1998 Marcus Meissner
 *		  1999 Alex Korobka
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

#include "ntdll.h"
#include "ntdef.h"
#include "winreg.h"


/*	##############################
	######	SID FUNCTIONS	######
	##############################
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

/*	##############################################
	######	SECURITY DESCRIPTOR FUNCTIONS	######
	##############################################
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

/******************************************************************************
 *  RtlGetDaclSecurityDescriptor		[NTDLL] 
 *
 */
DWORD WINAPI RtlGetDaclSecurityDescriptor(
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
/**************************************************************************
 * RtlSetSaclSecurityDescriptor			[NTDLL.488]
 */
DWORD  WINAPI RtlSetSaclSecurityDescriptor (
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

/*	##############################
	######	ACL FUNCTIONS	######
	##############################
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
	LPACE_HEADER *x)
{
	LPACE_HEADER	ace;
	int		i;

	*x = 0;
	ace = (LPACE_HEADER)(acl+1);
	for (i=0;i<acl->AceCount;i++) {
		if ((DWORD)ace>=(((DWORD)acl)+acl->AclSize))
			return 0;
		ace = (LPACE_HEADER)(((BYTE*)ace)+ace->AceSize);
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
	LPACE_HEADER acestart,
	DWORD acelen)
{
	LPACE_HEADER	ace,targetace;
	int		nrofaces;

	if (acl->AclRevision != ACL_REVISION)
		return STATUS_INVALID_PARAMETER;
	if (!RtlFirstFreeAce(acl,&targetace))
		return STATUS_INVALID_PARAMETER;
	nrofaces=0;ace=acestart;
	while (((DWORD)ace-(DWORD)acestart)<acelen) {
		nrofaces++;
		ace = (LPACE_HEADER)(((BYTE*)ace)+ace->AceSize);
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
DWORD WINAPI RtlAddAccessAllowedAce(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  RtlGetAce		[NTDLL] 
 */
DWORD WINAPI RtlGetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce ) {
	FIXME(ntdll,"(%p,%ld,%p),stub!\n",pAcl,dwAceIndex,pAce);
	return 0;
}

/*	######################################
	######	STRING FUNCTIONS	######
	######################################
*/

/**************************************************************************
 *                 RtlAnsiStringToUnicodeString		[NTDLL.269]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING uni,PANSI_STRING ansi,BOOLEAN doalloc)
{
	DWORD	unilen = (ansi->Length+1)*sizeof(WCHAR);

	if (unilen>0xFFFF)
		return STATUS_INVALID_PARAMETER_2;
	uni->Length = unilen;
	if (doalloc) {
		uni->MaximumLength = unilen;
		uni->Buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,unilen);
		if (!uni->Buffer)
			return STATUS_NO_MEMORY;
	}
	if (unilen>uni->MaximumLength)
		return STATUS_BUFFER_OVERFLOW;
	lstrcpynAtoW(uni->Buffer,ansi->Buffer,unilen/2);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlOemStringToUnicodeString		[NTDLL.447]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlOemStringToUnicodeString(PUNICODE_STRING uni,PSTRING ansi,BOOLEAN doalloc)
{
	DWORD	unilen = (ansi->Length+1)*sizeof(WCHAR);

	if (unilen>0xFFFF)
		return STATUS_INVALID_PARAMETER_2;
	uni->Length = unilen;
	if (doalloc) {
		uni->MaximumLength = unilen;
		uni->Buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,unilen);
		if (!uni->Buffer)
			return STATUS_NO_MEMORY;
	}
	if (unilen>uni->MaximumLength)
		return STATUS_BUFFER_OVERFLOW;
	lstrcpynAtoW(uni->Buffer,ansi->Buffer,unilen/2);
	return STATUS_SUCCESS;
}
/**************************************************************************
 *                 RtlMultiByteToUnicodeN		[NTDLL.436]
 * FIXME: multibyte support
 */
DWORD /* NTSTATUS */ 
WINAPI RtlMultiByteToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen)
{
	DWORD	len;
	LPWSTR	x;

	len = oemlen;
	if (unilen/2 < len)
		len = unilen/2;
	x=(LPWSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len+1)*sizeof(WCHAR));
	lstrcpynAtoW(x,oemstr,len+1);
	memcpy(unistr,x,len*2);
	if (reslen) *reslen = len*2;
	return 0;
}

/**************************************************************************
 *                 RtlOemToUnicodeN			[NTDLL.448]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlOemToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen)
{
	DWORD	len;
	LPWSTR	x;

	len = oemlen;
	if (unilen/2 < len)
		len = unilen/2;
	x=(LPWSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(len+1)*sizeof(WCHAR));
	lstrcpynAtoW(x,oemstr,len+1);
	memcpy(unistr,x,len*2);
	if (reslen) *reslen = len*2;
	return 0;
}

/**************************************************************************
 *                 RtlInitAnsiString			[NTDLL.399]
 */
VOID WINAPI RtlInitAnsiString(PANSI_STRING target,LPCSTR source)
{
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPSTR)source;
	if (!source)
		return;
	target->MaximumLength = lstrlen32A(target->Buffer);
	target->Length = target->MaximumLength+1;
}
/**************************************************************************
 *                 RtlInitString			[NTDLL.402]
 */
VOID WINAPI RtlInitString(PSTRING target,LPCSTR source)
{
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPSTR)source;
	if (!source)
		return;
	target->MaximumLength = lstrlen32A(target->Buffer);
	target->Length = target->MaximumLength+1;
}

/**************************************************************************
 *                 RtlInitUnicodeString			[NTDLL.403]
 */
VOID WINAPI RtlInitUnicodeString(PUNICODE_STRING target,LPCWSTR source)
{
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPWSTR)source;
	if (!source)
		return;
	target->MaximumLength = lstrlen32W(target->Buffer)*2;
	target->Length = target->MaximumLength+2;
}

/**************************************************************************
 *                 RtlFreeUnicodeString			[NTDLL.377]
 */
VOID WINAPI RtlFreeUnicodeString(PUNICODE_STRING str)
{
	if (str->Buffer)
		HeapFree(GetProcessHeap(),0,str->Buffer);
}

/**************************************************************************
 * RtlFreeAnsiString [NTDLL.373]
 */
VOID WINAPI RtlFreeAnsiString(PANSI_STRING AnsiString)
{
    if( AnsiString->Buffer )
        HeapFree( GetProcessHeap(),0,AnsiString->Buffer );
}


/**************************************************************************
 *                 RtlUnicodeToOemN			[NTDLL.515]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlUnicodeToOemN(LPSTR oemstr,DWORD oemlen,LPDWORD reslen,LPWSTR unistr,DWORD unilen)
{
	DWORD	len;
	LPSTR	x;

	len = oemlen;
	if (unilen/2 < len)
		len = unilen/2;
	x=(LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,len+1);
	lstrcpynWtoA(x,unistr,len+1);
	memcpy(oemstr,x,len);
	if (reslen) *reslen = len;
	return 0;
}

/**************************************************************************
 *                 RtlUnicodeStringToOemString		[NTDLL.511]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlUnicodeStringToOemString(PANSI_STRING oem,PUNICODE_STRING uni,BOOLEAN alloc)
{
	if (alloc) {
		oem->Buffer = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,uni->Length/2)+1;
		oem->MaximumLength = uni->Length/2+1;
	}
	oem->Length = uni->Length/2;
	lstrcpynWtoA(oem->Buffer,uni->Buffer,uni->Length/2+1);
	return 0;
}

/**************************************************************************
 *                 RtlUnicodeStringToAnsiString		[NTDLL.507]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlUnicodeStringToAnsiString(PANSI_STRING oem,PUNICODE_STRING uni,BOOLEAN alloc)
{
	if (alloc) {
		oem->Buffer = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,uni->Length/2)+1;
		oem->MaximumLength = uni->Length/2+1;
	}
	oem->Length = uni->Length/2;
	lstrcpynWtoA(oem->Buffer,uni->Buffer,uni->Length/2+1);
	return 0;
}

/**************************************************************************
 *                 RtlEqualUnicodeString		[NTDLL]
 */
DWORD WINAPI RtlEqualUnicodeString(PUNICODE_STRING s1,PUNICODE_STRING s2,DWORD x) {
	FIXME(ntdll,"(%s,%s,%ld),stub!\n",debugstr_w(s1->Buffer),debugstr_w(s2->Buffer),x);
	return 0;
	if (s1->Length != s2->Length)
		return 1;
	return !lstrncmp32W(s1->Buffer,s2->Buffer,s1->Length/2);
}

/**************************************************************************
 *                 RtlUpcaseUnicodeString		[NTDLL.520]
 */
DWORD WINAPI RtlUpcaseUnicodeString(PUNICODE_STRING dest,PUNICODE_STRING src,BOOLEAN doalloc)
{
	LPWSTR	s,t;
	DWORD	i,len;

	len = src->Length;
	if (doalloc) {
		dest->MaximumLength = len; 
		dest->Buffer = (LPWSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,len);
		if (!dest->Buffer)
			return STATUS_NO_MEMORY;

	}
	if (dest->MaximumLength < len)
		return STATUS_BUFFER_OVERFLOW;
	s=dest->Buffer;t=src->Buffer;
	/* len is in bytes */
	for (i=0;i<len/2;i++)
		s[i] = towupper(t[i]);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlxOemStringToUnicodeSize		[NTDLL.549]
 */
UINT32 WINAPI RtlxOemStringToUnicodeSize(PSTRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlxAnsiStringToUnicodeSize		[NTDLL.548]
 */
UINT32 WINAPI RtlxAnsiStringToUnicodeSize(PANSI_STRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlIsTextUnicode			[NTDLL.417]
 *
 *	Apply various feeble heuristics to guess whether
 *	the text buffer contains Unicode.
 *	FIXME: should implement more tests.
 */
DWORD WINAPI RtlIsTextUnicode(LPVOID buf, DWORD len, DWORD *pf)
{
	LPWSTR s = buf;
	DWORD flags = -1, out_flags = 0;

	if (!len)
		goto out;
	if (pf)
		flags = *pf;
	/*
	 * Apply various tests to the text string. According to the
	 * docs, each test "passed" sets the corresponding flag in
	 * the output flags. But some of the tests are mutually
	 * exclusive, so I don't see how you could pass all tests ...
	 */

	/* Check for an odd length ... pass if even. */
	if (!(len & 1))
		out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

	/* Check for the special unicode marker byte. */
	if (*s == 0xFEFF)
		out_flags |= IS_TEXT_UNICODE_SIGNATURE;

	/*
	 * Check whether the string passed all of the tests.
	 */
	flags &= ITU_IMPLEMENTED_TESTS;
	if ((out_flags & flags) != flags)
		len = 0;
out:
	if (pf)
		*pf = out_flags;
	return len;
}


/******************************************************************************
 *	RtlCompareUnicodeString	[NTDLL] 
 */
NTSTATUS WINAPI RtlCompareUnicodeString(
	PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive) 
{
	FIXME(ntdll,"(%s,%s,0x%08x),stub!\n",debugstr_w(String1->Buffer),debugstr_w(String1->Buffer),CaseInSensitive);
	return 0;
}


/*	######################################
	######	RESOURCE FUNCTIONS	######
	######################################
*/
/***********************************************************************
 *           RtlInitializeResource	(NTDLL.409)
 *
 * xxxResource() functions implement multiple-reader-single-writer lock.
 * The code is based on information published in WDJ January 1999 issue.
 */
void WINAPI RtlInitializeResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	rwl->iNumberActive = 0;
	rwl->uExclusiveWaiters = 0;
	rwl->uSharedWaiters = 0;
	rwl->hOwningThreadId = 0;
	rwl->dwTimeoutBoost = 0; /* no info on this one, default value is 0 */
	InitializeCriticalSection( &rwl->rtlCS );
	rwl->hExclusiveReleaseSemaphore = CreateSemaphore32A( NULL, 0, 65535, NULL );
	rwl->hSharedReleaseSemaphore = CreateSemaphore32A( NULL, 0, 65535, NULL );
    }
}


/***********************************************************************
 *           RtlDeleteResource		(NTDLL.330)
 */
void WINAPI RtlDeleteResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	EnterCriticalSection( &rwl->rtlCS );
	if( rwl->iNumberActive || rwl->uExclusiveWaiters || rwl->uSharedWaiters )
	    MSG("Deleting active MRSW lock (%p), expect failure\n", rwl );
	rwl->hOwningThreadId = 0;
	rwl->uExclusiveWaiters = rwl->uSharedWaiters = 0;
	rwl->iNumberActive = 0;
	CloseHandle( rwl->hExclusiveReleaseSemaphore );
	CloseHandle( rwl->hSharedReleaseSemaphore );
	LeaveCriticalSection( &rwl->rtlCS );
	DeleteCriticalSection( &rwl->rtlCS );
    }
}


/***********************************************************************
 *          RtlAcquireResourceExclusive	(NTDLL.256)
 */
BYTE WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK rwl, BYTE fWait)
{
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    EnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive == 0 ) /* lock is free */
    {
	rwl->iNumberActive = -1;
	retVal = 1;
    }
    else if( rwl->iNumberActive < 0 ) /* exclusive lock in progress */
    {
	 if( rwl->hOwningThreadId == GetCurrentThreadId() )
	 {
	     retVal = 1;
	     rwl->iNumberActive--;
	     goto done;
	 }
wait:
	 if( fWait )
	 {
	     rwl->uExclusiveWaiters++;

	     LeaveCriticalSection( &rwl->rtlCS );
	     if( WaitForSingleObject( rwl->hExclusiveReleaseSemaphore, INFINITE32 ) == WAIT_FAILED )
		 goto done;
	     goto start; /* restart the acquisition to avoid deadlocks */
	 }
    }
    else  /* one or more shared locks are in progress */
	 if( fWait )
	     goto wait;
	 
    if( retVal == 1 )
	rwl->hOwningThreadId = GetCurrentThreadId();
done:
    LeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}

/***********************************************************************
 *          RtlAcquireResourceShared	(NTDLL.257)
 */
BYTE WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK rwl, BYTE fWait)
{
    DWORD dwWait = WAIT_FAILED;
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    EnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive < 0 )
    {
	if( rwl->hOwningThreadId == GetCurrentThreadId() )
	{
	    rwl->iNumberActive--;
	    retVal = 1;
	    goto done;
	}
	
	if( fWait )
	{
	    rwl->uSharedWaiters++;
	    LeaveCriticalSection( &rwl->rtlCS );
	    if( (dwWait = WaitForSingleObject( rwl->hSharedReleaseSemaphore, INFINITE32 )) == WAIT_FAILED )
		goto done;
	    goto start;
	}
    }
    else 
    {
	if( dwWait != WAIT_OBJECT_0 ) /* otherwise RtlReleaseResource() has already done it */
	    rwl->iNumberActive++;
	retVal = 1;
    }
done:
    LeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}


/***********************************************************************
 *           RtlReleaseResource		(NTDLL.471)
 */
void WINAPI RtlReleaseResource(LPRTL_RWLOCK rwl)
{
    EnterCriticalSection( &rwl->rtlCS );

    if( rwl->iNumberActive > 0 ) /* have one or more readers */
    {
	if( --rwl->iNumberActive == 0 )
	{
	    if( rwl->uExclusiveWaiters )
	    {
wake_exclusive:
		rwl->uExclusiveWaiters--;
		ReleaseSemaphore( rwl->hExclusiveReleaseSemaphore, 1, NULL );
	    }
	}
    }
    else 
    if( rwl->iNumberActive < 0 ) /* have a writer, possibly recursive */
    {
	if( ++rwl->iNumberActive == 0 )
	{
	    rwl->hOwningThreadId = 0;
	    if( rwl->uExclusiveWaiters )
		goto wake_exclusive;
	    else
		if( rwl->uSharedWaiters )
		{
		    UINT32 n = rwl->uSharedWaiters;
		    rwl->iNumberActive = rwl->uSharedWaiters; /* prevent new writers from joining until
							       * all queued readers have done their thing */
		    rwl->uSharedWaiters = 0;
		    ReleaseSemaphore( rwl->hSharedReleaseSemaphore, n, NULL );
		}
	}
    }
    LeaveCriticalSection( &rwl->rtlCS );
}


/***********************************************************************
 *           RtlDumpResource		(NTDLL.340)
 */
void WINAPI RtlDumpResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	MSG("RtlDumpResource(%p):\n\tactive count = %i\n\twaiting readers = %i\n\twaiting writers = %i\n",  
		rwl, rwl->iNumberActive, rwl->uSharedWaiters, rwl->uExclusiveWaiters );
	if( rwl->iNumberActive )
	    MSG("\towner thread = %08x\n", rwl->hOwningThreadId );
    }
}

/*	##############################
	######	MISC FUNCTIONS	######
	##############################
*/

/******************************************************************************
 *	DbgPrint	[NTDLL] 
 */
void __cdecl DbgPrint(LPCSTR fmt,LPVOID args) {
	char buf[512];

	wvsprintf32A(buf,fmt,&args);
	MSG("DbgPrint says: %s",buf);
	/* hmm, raise exception? */
}
DWORD NtRaiseException ( DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments,CONST ULONG_PTR *lpArguments)
{	FIXME(ntdll,"0x%08lx 0x%08lx 0x%08lx %p\n", dwExceptionCode, dwExceptionFlags, nNumberOfArguments, lpArguments);
	return 0;
}

DWORD RtlRaiseException ( DWORD x)
{	FIXME(ntdll, "0x%08lx\n", x);
	return 0;
}
/******************************************************************************
 *  RtlAcquirePebLock		[NTDLL] 
 */
VOID WINAPI RtlAcquirePebLock(void) {
	FIXME(ntdll,"()\n");
	/* enter critical section ? */
}

/******************************************************************************
 *  RtlReleasePebLock		[NTDLL] 
 */
VOID WINAPI RtlReleasePebLock(void) {
	FIXME(ntdll,"()\n");
	/* leave critical section ? */
}

/******************************************************************************
 *  RtlAdjustPrivilege		[NTDLL] 
 */
DWORD WINAPI RtlAdjustPrivilege(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  RtlIntegerToChar	[NTDLL] 
 */
DWORD WINAPI RtlIntegerToChar(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}
/******************************************************************************
 *  RtlSystemTimeToLocalTime 	[NTDLL] 
 */
DWORD WINAPI RtlSystemTimeToLocalTime(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}
/******************************************************************************
 *  RtlTimeToTimeFields 	[NTDLL] 
 */
DWORD WINAPI RtlTimeToTimeFields(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}
/******************************************************************************
 *  RtlSetEnvironmentVariable		[NTDLL] 
 */
DWORD WINAPI RtlSetEnvironmentVariable(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) {
	FIXME(ntdll,"(0x%08lx,%s,%s),stub!\n",x1,debugstr_w(key->Buffer),debugstr_w(val->Buffer));
	return 0;
}

/******************************************************************************
 *  RtlNewSecurityObject		[NTDLL] 
 */
DWORD WINAPI RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  RtlDeleteSecurityObject		[NTDLL] 
 */
DWORD WINAPI RtlDeleteSecurityObject(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 0;
}

/******************************************************************************
 *  RtlToTimeInSecondsSince1980		[NTDLL] 
 */
BOOLEAN WINAPI RtlTimeToSecondsSince1980(LPFILETIME ft,LPDWORD timeret) {
	/* 1980 = 1970+10*365 days +  29. februar 1972 + 29.februar 1976 */
	*timeret = DOSFS_FileTimeToUnixTime(ft,NULL) - (10*365+2)*24*3600;
	return 1;
}

/******************************************************************************
 *  RtlToTimeInSecondsSince1970		[NTDLL] 
 */
BOOLEAN WINAPI RtlTimeToSecondsSince1970(LPFILETIME ft,LPDWORD timeret) {
	*timeret = DOSFS_FileTimeToUnixTime(ft,NULL);
	return 1;
}
/**************************************************************************
 *                 RtlNormalizeProcessParams		[NTDLL.441]
 */
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x)
{
    FIXME(ntdll,"(%p), stub\n",x);
    return x;
}

/**************************************************************************
 *                 RtlNtStatusToDosError			[NTDLL.442]
 */
DWORD WINAPI RtlNtStatusToDosError(DWORD error)
{
	FIXME(ntdll, "(%lx): map STATUS_ to ERROR_\n",error);
	switch (error)
	{ case STATUS_SUCCESS:			return ERROR_SUCCESS;
	  case STATUS_INVALID_PARAMETER:	return ERROR_BAD_ARGUMENTS;
	  case STATUS_BUFFER_TOO_SMALL:		return ERROR_INSUFFICIENT_BUFFER;
/*	  case STATUS_INVALID_SECURITY_DESCR:	return ERROR_INVALID_SECURITY_DESCR;*/
	  case STATUS_NO_MEMORY:		return ERROR_NOT_ENOUGH_MEMORY;
/*	  case STATUS_UNKNOWN_REVISION:
	  case STATUS_BUFFER_OVERFLOW:*/
	}
	FIXME(ntdll, "unknown status (%lx)\n",error);
	return ERROR_SUCCESS;
}

/**************************************************************************
 *                 RtlGetNtProductType			[NTDLL.390]
 */
BOOLEAN WINAPI RtlGetNtProductType(LPDWORD type)
{
    FIXME(ntdll, "(%p): stub\n", type);
    *type=3; /* dunno. 1 for client, 3 for server? */
    return 1;
}

/**************************************************************************
 *                 NTDLL_chkstk				[NTDLL.862]
 *                 NTDLL_alloca_probe				[NTDLL.861]
 * Glorified "enter xxxx".
 */
REGS_ENTRYPOINT(NTDLL_chkstk)
{
    ESP_reg(context) -= EAX_reg(context);
}
REGS_ENTRYPOINT(NTDLL_alloca_probe)
{
    ESP_reg(context) -= EAX_reg(context);
}

/******************************************************************************
 * RtlTimeToElapsedTimeFields [NTDLL.502]
 */
DWORD WINAPI RtlTimeToElapsedTimeFields( DWORD x1, DWORD x2 )
{
    FIXME(ntdll,"(%lx,%lx): stub\n",x1,x2);
    return 0;
}


/******************************************************************************
 * RtlExtendedLargeIntegerDivide [NTDLL.359]
 */
INT32 WINAPI RtlExtendedLargeIntegerDivide(
	LARGE_INTEGER dividend,
	DWORD divisor,
	LPDWORD rest
) {
#if SIZEOF_LONG_LONG==8
	long long x1 = *(long long*)&dividend;

	if (*rest)
		*rest = x1 % divisor;
	return x1/divisor;
#else
	FIXME(ntdll,"((%d<<32)+%d,%d,%p), implement this using normal integer arithmetic!\n",dividend.HighPart,dividend.LowPart,divisor,rest);
	return 0;
#endif
}

/******************************************************************************
 * RtlExtendedLargeIntegerMultiply [NTDLL.359]
 * Note: This even works, since gcc returns 64bit values in eax/edx just like
 * the caller expects. However... The relay code won't grok this I think.
 */
long long /*LARGE_INTEGER*/ 
WINAPI RtlExtendedIntegerMultiply(
	LARGE_INTEGER factor1,INT32 factor2
) {
#if SIZEOF_LONG_LONG==8
	return (*(long long*)&factor1)*factor2;
#else
	FIXME(ntdll,"((%d<<32)+%d,%ld), implement this using normal integer arithmetic!\n",factor1.HighPart,factor1.LowPart,factor2);
	return 0;
#endif
}

/******************************************************************************
 *  RtlFormatCurrentUserKeyPath		[NTDLL.371] 
 */
DWORD WINAPI RtlFormatCurrentUserKeyPath(DWORD x)
{
    FIXME(ntdll,"(0x%08lx): stub\n",x);
    return 1;
}

/******************************************************************************
 *  RtlOpenCurrentUser		[NTDLL] 
 */
DWORD WINAPI RtlOpenCurrentUser(DWORD x1, DWORD *x2)
{
/* Note: this is not the correct solution, 
 * But this works pretty good on wine and NT4.0 binaries
 */
	if  ( x1 == 0x2000000 )  {
		*x2 = HKEY_CURRENT_USER; 
		return TRUE;
	}
		
	return FALSE;
}
/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U		[NTDLL.338]
 *
 * FIXME: convert to UNC or whatever is expected here
 */
BOOLEAN  WINAPI RtlDosPathNameToNtPathName_U(
	LPWSTR from,PUNICODE_STRING us,DWORD x2,DWORD x3)
{
	LPSTR	fromA = HEAP_strdupWtoA(GetProcessHeap(),0,from);

	FIXME(ntdll,"(%s,%p,%08lx,%08lx)\n",fromA,us,x2,x3);
	if (us)
		RtlInitUnicodeString(us,HEAP_strdupW(GetProcessHeap(),0,from));
	return TRUE;
}

/******************************************************************************
 *  RtlCreateEnvironment		[NTDLL] 
 */
DWORD WINAPI RtlCreateEnvironment(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}


/******************************************************************************
 *  RtlDestroyEnvironment		[NTDLL] 
 */
DWORD WINAPI RtlDestroyEnvironment(DWORD x) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x);
	return 0;
}

/******************************************************************************
 *  RtlQueryEnvironmentVariable_U		[NTDLL] 
 */
DWORD WINAPI RtlQueryEnvironmentVariable_U(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) {
	FIXME(ntdll,"(0x%08lx,%s,%p),stub!\n",x1,debugstr_w(key->Buffer),val);
	return 0;
}

