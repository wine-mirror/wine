/*
 * NT basis DLL
 * 
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "win.h"
#include "windows.h"
#include "ntdll.h"
#include "heap.h"
#include "debug.h"
#include "module.h"
#include "heap.h"

/**************************************************************************
 *                 RtlLengthRequiredSid			[NTDLL]
 */
DWORD WINAPI RtlLengthRequiredSid(DWORD nrofsubauths)
{
	return sizeof(DWORD)*nrofsubauths+sizeof(SID);
}

/**************************************************************************
 *                 RtlLengthSid				[NTDLL]
 */
DWORD WINAPI RtlLengthSid(LPSID sid)
{
	return sizeof(DWORD)*sid->SubAuthorityCount+sizeof(SID);
}

/**************************************************************************
 *                 RtlCreateAcl				[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlCreateAcl(LPACL acl,DWORD size,DWORD rev)
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
 *                 RtlFirstFreeAce			[NTDLL]
 * looks for the AceCount+1 ACE, and if it is still within the alloced
 * ACL, return a pointer to it
 */
BOOL32 WINAPI RtlFirstFreeAce(LPACL acl,LPACE_HEADER *x)
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
 *                 RtlAddAce				[NTDLL]
 */
DWORD /* NTSTATUS */  WINAPI RtlAddAce(LPACL acl,DWORD rev,DWORD xnrofaces,
                                       LPACE_HEADER acestart,DWORD acelen)
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
	return 0;
}

/**************************************************************************
 *                 RtlCreateSecurityDescriptor		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlCreateSecurityDescriptor(LPSECURITY_DESCRIPTOR lpsd,DWORD rev)
{
	if (rev!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	memset(lpsd,'\0',sizeof(*lpsd));
	lpsd->Revision = SECURITY_DESCRIPTOR_REVISION;
	return 0;
}

/**************************************************************************
 *                 RtlSetDaclSecurityDescriptor		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlSetDaclSecurityDescriptor ( LPSECURITY_DESCRIPTOR lpsd,BOOL32 daclpresent,LPACL dacl,BOOL32 dacldefaulted )
{
	if (lpsd->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (lpsd->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;
	if (!daclpresent) {
		lpsd->Control &= ~SE_DACL_PRESENT;
		return 0;
	}
	lpsd->Control |= SE_DACL_PRESENT;
	lpsd->Dacl = dacl;
	if (dacldefaulted)
		lpsd->Control |= SE_DACL_DEFAULTED;
	else
		lpsd->Control &= ~SE_DACL_DEFAULTED;
	return 0;
}

/**************************************************************************
 *                 RtlSetSaclSecurityDescriptor		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlSetSaclSecurityDescriptor (
LPSECURITY_DESCRIPTOR lpsd,BOOL32 saclpresent,LPACL sacl,BOOL32 sacldefaulted
)
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
	return 0;
}

/**************************************************************************
 *                 RtlSetOwnerSecurityDescriptor		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlSetOwnerSecurityDescriptor (LPSECURITY_DESCRIPTOR lpsd,LPSID owner,BOOL32 ownerdefaulted)
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
	return 0;
}

/**************************************************************************
 *                 RtlSetOwnerSecurityDescriptor		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlSetGroupSecurityDescriptor (LPSECURITY_DESCRIPTOR lpsd,LPSID group,BOOL32 groupdefaulted)
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
	return 0;
}


/**************************************************************************
 *                 RtlNormalizeProcessParams		[NTDLL]
 */
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x)
{
    FIXME(ntdll,"(%p), stub.\n",x);
    return x;
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL]
 */
DWORD WINAPI RtlInitializeSid(LPSID lpsid,LPSID_IDENTIFIER_AUTHORITY lpsidauth,
                              DWORD c)
{
	BYTE	a = c&0xff;

	if (a>=SID_MAX_SUB_AUTHORITIES)
		return a;
	lpsid->SubAuthorityCount = a;
	lpsid->Revision		 = SID_REVISION;
	memcpy(&(lpsid->IdentifierAuthority),lpsidauth,sizeof(SID_IDENTIFIER_AUTHORITY));
	return 0;
}

/**************************************************************************
 *                 RtlSubAuthoritySid			[NTDLL]
 */
LPDWORD WINAPI RtlSubAuthoritySid(LPSID lpsid,DWORD nr)
{
	return &(lpsid->SubAuthority[nr]);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL]
 */
LPBYTE WINAPI RtlSubAuthorityCountSid(LPSID lpsid)
{
	return ((LPBYTE)lpsid)+1;
}

/**************************************************************************
 *                 RtlCopySid				[NTDLL]
 */
DWORD WINAPI RtlCopySid(DWORD len,LPSID to,LPSID from)
{
	if (len<(from->SubAuthorityCount*4+8))
		return STATUS_BUFFER_TOO_SMALL;
	memmove(to,from,from->SubAuthorityCount*4+8);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlAnsiStringToUnicodeString		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlAnsiStringToUnicodeString(LPUNICODE_STRING uni,LPANSI_STRING ansi,BOOL32 doalloc)
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
 *                 RtlOemStringToUnicodeString		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlOemStringToUnicodeString(LPUNICODE_STRING uni,LPSTRING ansi,BOOL32 doalloc)
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
 *                 RtlMultiByteToUnicodeN		[NTDLL]
 * FIXME: multibyte support
 */
DWORD /* NTSTATUS */ WINAPI RtlMultiByteToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen)
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
 *                 RtlOemToUnicodeN			[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlOemToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen)
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
 *                 RtlInitString			[NTDLL]
 */
VOID WINAPI RtlInitAnsiString(LPANSI_STRING target,LPCSTR source)
{
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPSTR)source;
	if (!source)
		return;
	target->Length = lstrlen32A(target->Buffer);
	target->MaximumLength = target->Length+1;
}
/**************************************************************************
 *                 RtlInitString			[NTDLL]
 */
VOID WINAPI RtlInitString(LPSTRING target,LPCSTR source)
{
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPSTR)source;
	if (!source)
		return;
	target->Length = lstrlen32A(target->Buffer);
	target->MaximumLength = target->Length+1;
}

/**************************************************************************
 *                 RtlInitUnicodeString			[NTDLL]
 */
VOID WINAPI RtlInitUnicodeString(LPUNICODE_STRING target,LPCWSTR source)
{
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPWSTR)source;
	if (!source)
		return;
	target->Length = lstrlen32W(target->Buffer)*2;
	target->MaximumLength = target->Length+2;
}

/**************************************************************************
 *                 RtlFreeUnicodeString			[NTDLL]
 */
VOID WINAPI RtlFreeUnicodeString(LPUNICODE_STRING str)
{
	if (str->Buffer)
		HeapFree(GetProcessHeap(),0,str->Buffer);
}

/**************************************************************************
 *                 RtlUnicodeToOemN			[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlUnicodeToOemN(LPSTR oemstr,DWORD oemlen,LPDWORD reslen,LPWSTR unistr,DWORD unilen)
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
 *                 RtlUnicodeStringToOemString		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlUnicodeStringToOemString(LPANSI_STRING oem,LPUNICODE_STRING uni,BOOL32 alloc)
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
 *                 RtlUnicodeStringToAnsiString		[NTDLL]
 */
DWORD /* NTSTATUS */ WINAPI RtlUnicodeStringToAnsiString(LPUNICODE_STRING uni,LPANSI_STRING oem,BOOL32 alloc)
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
 *                 RtlNtStatusToDosErro			[NTDLL]
 */
DWORD WINAPI RtlNtStatusToDosError(DWORD error)
{
	/* FIXME: map STATUS_ to ERROR_ */
	return error;
}

/**************************************************************************
 *                 RtlGetNtProductType			[NTDLL]
 */
DWORD WINAPI RtlGetNtProductType(LPVOID x)
{
	/* FIXME : find documentation for this one */
	return 0;
}

/**************************************************************************
 *                 RtlUpcaseUnicodeString		[NTDLL]
 */
DWORD WINAPI RtlUpcaseUnicodeString(LPUNICODE_STRING dest,LPUNICODE_STRING src,BOOL32 doalloc)
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
		s[i]=toupper(t[i]);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlxOemStringToUnicodeSize		[NTDLL]
 */
UINT32 WINAPI RtlxOemStringToUnicodeSize(LPSTRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlxAnsiStringToUnicodeSize		[NTDLL]
 */
UINT32 WINAPI RtlxAnsiStringToUnicodeSize(LPANSI_STRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U		[NTDLL]
 *
 * FIXME: convert to UNC or whatever is expected here
 */
BOOL32  WINAPI RtlDosPathNameToNtPathName_U(
	LPWSTR from,LPUNICODE_STRING us,DWORD x2,DWORD x3)
{
	LPSTR	fromA = HEAP_strdupWtoA(GetProcessHeap(),0,from);

	FIXME(ntdll,"(%s,%p,%08lx,%08lx)\n",fromA,us,x2,x3);
	if (us)
		RtlInitUnicodeString(us,HEAP_strdupW(GetProcessHeap(),0,from));
	return TRUE;
}

/**************************************************************************
 *                 NtOpenFile				[NTDLL]
 */
DWORD WINAPI NtOpenFile(DWORD x1,DWORD flags,DWORD x3,DWORD x4,DWORD alignment,DWORD x6)
{
	FIXME(ntdll,"(%08lx,%08lx,%08lx,%08lx,%08lx,%08lx)\n",
	      x1,flags,x3,x4,alignment,x6);
	/* returns file io completion status */
	return 0;
}


/**************************************************************************
 *                 NTDLL_chkstk   (NTDLL.862)
 */
void NTDLL_chkstk(void)
{
    /* FIXME: should subtract %eax bytes from stack pointer */
}
