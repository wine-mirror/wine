/*
 * NT basis DLL
 * 
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "win.h"
#include "windows.h"
#include "winnls.h"
#include "ntdll.h"
#include "heap.h"
#include "debug.h"
#include "module.h"
#include "heap.h"
#include "debugstr.h"
#include "winreg.h"

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
DWORD WINAPI RtlLengthSid(LPSID sid)
{	TRACE(ntdll,"sid=%p\n",sid);
	if (!sid)
	  return FALSE; 
	return sizeof(DWORD)*sid->SubAuthorityCount+sizeof(SID);
}

/**************************************************************************
 *                 RtlCreateAcl				[NTDLL.306]
 *
 * NOTES
 *    This should return NTSTATUS
 */
DWORD WINAPI RtlCreateAcl(LPACL acl,DWORD size,DWORD rev)
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
 *                 RtlAddAce				[NTDLL.260]
 */
DWORD /* NTSTATUS */  
WINAPI RtlAddAce(LPACL acl,DWORD rev,DWORD xnrofaces,
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
 *                 RtlCreateSecurityDescriptor		[NTDLL.313]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlCreateSecurityDescriptor(LPSECURITY_DESCRIPTOR lpsd,DWORD rev)
{
	if (rev!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	memset(lpsd,'\0',sizeof(*lpsd));
	lpsd->Revision = SECURITY_DESCRIPTOR_REVISION;
	return 0;
}

/**************************************************************************
 *                 RtlSetDaclSecurityDescriptor		[NTDLL.483]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlSetDaclSecurityDescriptor ( LPSECURITY_DESCRIPTOR lpsd,BOOL32 daclpresent,LPACL dacl,BOOL32 dacldefaulted )
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
 *                 RtlSetSaclSecurityDescriptor		[NTDLL.488]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlSetSaclSecurityDescriptor (
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
 *                 RtlSetOwnerSecurityDescriptor		[NTDLL.487]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlSetOwnerSecurityDescriptor (LPSECURITY_DESCRIPTOR lpsd,LPSID owner,BOOL32 ownerdefaulted)
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
 *                 RtlSetGroupSecurityDescriptor		[NTDLL.485]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlSetGroupSecurityDescriptor (LPSECURITY_DESCRIPTOR lpsd,LPSID group,BOOL32 groupdefaulted)
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
 *                 RtlNormalizeProcessParams		[NTDLL.441]
 */
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x)
{
    FIXME(ntdll,"(%p), stub\n",x);
    return x;
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL.410]
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
 *                 RtlSubAuthoritySid			[NTDLL.497]
 */
LPDWORD WINAPI RtlSubAuthoritySid(LPSID lpsid,DWORD nr)
{
	return &(lpsid->SubAuthority[nr]);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL.496]
 */
LPBYTE WINAPI RtlSubAuthorityCountSid(LPSID lpsid)
{
	return ((LPBYTE)lpsid)+1;
}

/**************************************************************************
 *                 RtlCopySid				[NTDLL.302]
 */
DWORD WINAPI RtlCopySid(DWORD len,LPSID to,LPSID from)
{	if (!from)
		return 0;
	if (len<(from->SubAuthorityCount*4+8))
		return STATUS_BUFFER_TOO_SMALL;
	memmove(to,from,from->SubAuthorityCount*4+8);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlAnsiStringToUnicodeString		[NTDLL.269]
 */
DWORD /* NTSTATUS */ 
WINAPI RtlAnsiStringToUnicodeString(LPUNICODE_STRING uni,LPANSI_STRING ansi,BOOL32 doalloc)
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
WINAPI RtlOemStringToUnicodeString(LPUNICODE_STRING uni,LPSTRING ansi,BOOL32 doalloc)
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
 *                 RtlInitString			[NTDLL.402]
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
 *                 RtlInitUnicodeString			[NTDLL.403]
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
 *                 RtlFreeUnicodeString			[NTDLL.377]
 */
VOID WINAPI RtlFreeUnicodeString(LPUNICODE_STRING str)
{
	if (str->Buffer)
		HeapFree(GetProcessHeap(),0,str->Buffer);
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
WINAPI RtlUnicodeStringToOemString(LPANSI_STRING oem,LPUNICODE_STRING uni,BOOL32 alloc)
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
WINAPI RtlUnicodeStringToAnsiString(LPANSI_STRING oem,LPUNICODE_STRING uni,BOOL32 alloc)
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
 *                 RtlNtStatusToDosErro			[NTDLL.442]
 */
DWORD WINAPI RtlNtStatusToDosError(DWORD error)
{
    FIXME(ntdll, "(%lx): map STATUS_ to ERROR_\n",error);
    return error;
}

/**************************************************************************
 *                 RtlGetNtProductType			[NTDLL.390]
 */
BOOL32 WINAPI RtlGetNtProductType(LPDWORD type)
{
    FIXME(ntdll, "(%p): stub\n", type);
    *type=3; /* dunno. 1 for client, 3 for server? */
    return 1;
}

/**************************************************************************
 *                 RtlUpcaseUnicodeString		[NTDLL.520]
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
		s[i] = towupper(t[i]);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlxOemStringToUnicodeSize		[NTDLL.549]
 */
UINT32 WINAPI RtlxOemStringToUnicodeSize(LPSTRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlxAnsiStringToUnicodeSize		[NTDLL.548]
 */
UINT32 WINAPI RtlxAnsiStringToUnicodeSize(LPANSI_STRING str)
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

/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U		[NTDLL.338]
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
 *                 NtOpenFile				[NTDLL.127]
 */
DWORD WINAPI NtOpenFile(DWORD x1,DWORD flags,DWORD x3,DWORD x4,DWORD alignment,DWORD x6)
{
	FIXME(ntdll,"(%08lx,0x%08lx,%08lx,%08lx,%08lx,%08lx): stub\n",
	      x1,flags,x3,x4,alignment,x6);
	/* returns file io completion status */
	return 0;
}

/**************************************************************************
 *		NtCreateFile				[NTDLL.73]
 */
DWORD /* NTSTATUS */ 
WINAPI NtCreateFile(PHANDLE filehandle,DWORD access,LPLONG attributes,
			LPLONG status,LPVOID x5,DWORD x6,DWORD x7,
			LPLONG x8,DWORD x9,DWORD x10,LPLONG x11)
{
	/* parameter count checked with wine debugger */
	FIXME(ntdll,"(%p,%lx,%lx,%lx,%p,%08lx,%08lx,%p,%08lx,%08lx,%p): empty stub\n",
	  filehandle,access,*attributes,*status,x5,x6,x7,x8,x9,x10,x11);
	return 0;
}
/**************************************************************************
 *		NtCreateTimer				[NTDLL.87]
 */
DWORD WINAPI NtCreateTimer(DWORD x1, DWORD x2, DWORD x3)
{
	/* parameter count checked (obscure doc from internet said 4, debugger showed only 3) */
	FIXME(ntdll,"(%08lx,%08lx,%08lx): empty stub\n",
		x1,x2,x3);
	return 0;
}
/**************************************************************************
 *		NtSetTimer				[NTDLL.221]
 */
DWORD WINAPI NtSetTimer(DWORD x1,DWORD x2,DWORD x3,DWORD x4,
		DWORD x5,DWORD x6)
{
	/* parameter count checked (obscure doc from internet said 7, debugger showed only 6) */
	FIXME(ntdll,"(%08lx,%08lx,%08lx,%08lx,%08lx,%08lx): empty stub\n",
		x1,x2,x3,x4,x5,x6);
	return 0;
}

/**************************************************************************
 *		NtCreateEvent				[NTDLL.71]
 */
DWORD /* NTSTATUS */ 
WINAPI NtCreateEvent(PHANDLE eventhandle, DWORD desiredaccess, 
	DWORD attributes, DWORD eventtype, DWORD initialstate)
{
	/* parameter count checked with wine debugger */
	FIXME(ntdll,"(%p,%08lx,%08lx,%08lx,%08lx): empty stub\n",
		eventhandle,desiredaccess,attributes,eventtype,initialstate);
	return 0;
}
/**************************************************************************
 *		NtDeviceIoControlFile			[NTDLL.94]
 */
DWORD /* NTSTATUS */ 
WINAPI NtDeviceIoControlFile(HANDLE32 filehandle, HANDLE32 event, 
	DWORD x3, DWORD x4, DWORD x5, UINT32 iocontrolcode,
	LPVOID inputbuffer,  DWORD inputbufferlength,
	LPVOID outputbuffer, DWORD outputbufferlength)
{
	/* parameter count checked with wine debugger */
	FIXME(ntdll,"(%x,%x,%08lx,%08lx,%08lx,%08x,%lx,%lx): empty stub\n",
		filehandle,event,x3,x4,x5,iocontrolcode,inputbufferlength,outputbufferlength);
	return 0;
}
/**************************************************************************
 *                 NTDLL_chkstk				[NTDLL.862]
 */

VOID WINAPI NTDLL_chkstk (DWORD x1, DWORD x2, DWORD x3, DWORD x4,
	DWORD x5, DWORD x6, DWORD x7, DWORD x8,
	DWORD x9, DWORD x10)
{
	/* FIXME: should subtract %eax bytes from stack pointer */
	FIXME(ntdll, "(void): stub\n");
}


/**************************************************************************
 * NtOpenDirectoryObject [NTDLL.124]
 */
DWORD WINAPI NtOpenDirectoryObject(DWORD x1,DWORD x2,DWORD x3)
{
    FIXME(ntdll,"(%lx,%lx,%lx): stub\n",x1,x2,x3);
    return 0;
}


/******************************************************************************
 * NtQueryDirectoryObject [NTDLL.149]
 */
DWORD WINAPI NtQueryDirectoryObject( DWORD x1, DWORD x2, DWORD x3, DWORD x4,
                                     DWORD x5, DWORD x6, DWORD x7 )
{
    FIXME(ntdll,"(%lx,%lx,%lx,%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4,x5,x6,x7);
    return 0;
}


/**************************************************************************
 * RtlFreeAnsiString [NTDLL.373]
 */
VOID WINAPI RtlFreeAnsiString(LPANSI_STRING AnsiString)
{
    if( AnsiString->Buffer )
        HeapFree( GetProcessHeap(),0,AnsiString->Buffer );
}


/******************************************************************************
 * NtQuerySystemInformation [NTDLL.168]
 */
DWORD WINAPI NtQuerySystemInformation( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
    FIXME(ntdll,"(%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4);
    return 0;
}


/******************************************************************************
 * NtQueryObject [NTDLL.161]
 */
DWORD WINAPI NtQueryObject( DWORD x1, DWORD x2 ,DWORD x3, DWORD x4, DWORD x5 )
{
    FIXME(ntdll,"(0x%lx,%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4,x5);
    return 0;
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
 * NtSetInformationProcess [NTDLL.207]
 */
DWORD WINAPI NtSetInformationProcess( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
    FIXME(ntdll,"(%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4);
    return 0;
}

/******************************************************************************
 * NtFsControlFile [NTDLL.108]
 */
VOID WINAPI NtFsControlFile(VOID)
{
    FIXME(ntdll,"(void): stub\n");
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

DWORD WINAPI NtOpenKey(DWORD x1,DWORD x2,LPUNICODE_STRING key) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx(%s)),stub!\n",x1,x2,key,key->Buffer);
	return RegOpenKey32W(HKEY_LOCAL_MACHINE,key->Buffer,x1);
}

DWORD WINAPI NtQueryValueKey(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME(ntdll,"(%08lx,%08lx,%08lx,%08lx,%08lx,%08lx),stub!\n",
		x1,x2,x3,x4,x5,x6
	);
	return 0;
}

DWORD WINAPI NtQueryTimerResolution(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx), stub!\n",x1,x2,x3);
	return 1;
}

/**************************************************************************
 *                 NtClose				[NTDLL.65]
 */
DWORD WINAPI NtClose(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 1;
}
/******************************************************************************
*  NtQueryInformationProcess		[NTDLL.] 
*
*/
DWORD WINAPI NtQueryInformationProcess(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5
	);
	return 0;
}
/******************************************************************************
*  NtQueryInformationThread		[NTDLL.] 
*
*/
DWORD WINAPI NtQueryInformationThread(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5
	);
	return 0;
}
/******************************************************************************
*  NtQueryInformationToken		[NTDLL.156] 
*
*/
DWORD WINAPI NtQueryInformationToken(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5
	);
	return 0;
}
/******************************************************************************
*  RtlFormatCurrentUserKeyPath		[NTDLL.371] 
*
*/
DWORD WINAPI RtlFormatCurrentUserKeyPath()
{
    FIXME(ntdll,"(): stub\n");
    return 1;
}
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
/******************************************************************************
*  RtlAllocateAndInitializeSid		[NTDLL.265] 
*
*/
BOOL32 WINAPI RtlAllocateAndInitializeSid (LPSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,DWORD nSubAuthorityCount,
		DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,DWORD x9,DWORD x10, LPSID pSid) 
{	FIXME(ntdll,"(%p,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,%p),stub!\n",
		pIdentifierAuthority,nSubAuthorityCount,x3,x4,x5,x6,x7,x8,x9,x10,pSid);
	return 0;
}
/******************************************************************************
*  RtlEqualSid		[NTDLL.352] 
*
*/
DWORD WINAPI RtlEqualSid(DWORD x1,DWORD x2) 
{	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n", x1,x2);
	return TRUE;
}

/******************************************************************************
 *  RtlFreeSid		[NTDLL.376] 
 */
DWORD WINAPI RtlFreeSid(DWORD x1) 
{	FIXME(ntdll,"(0x%08lx),stub!\n", x1);
	return TRUE;
}

/******************************************************************************
 *  NtCreatePagingFile		[NTDLL] 
 */
DWORD WINAPI NtCreatePagingFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  RtlGetDaclSecurityDescriptor		[NTDLL] 
 */
DWORD WINAPI RtlGetDaclSecurityDescriptor(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtDuplicateObject		[NTDLL] 
 */
DWORD WINAPI NtDuplicateObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,
	DWORD x6,DWORD x7
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7);
	return 0;
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
DWORD WINAPI RtlQueryEnvironmentVariable_U(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  RtlSetEnvironmentVariable		[NTDLL] 
 */
DWORD WINAPI RtlSetEnvironmentVariable(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtDuplicateToken		[NTDLL] 
 */
DWORD WINAPI NtDuplicateToken(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtAdjustPrivilegesToken		[NTDLL] 
 */
DWORD WINAPI NtAdjustPrivilegesToken(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtOpenProcessToken		[NTDLL] 
 */
DWORD WINAPI NtOpenProcessToken(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtSetInformationThread		[NTDLL] 
 */
DWORD WINAPI NtSetInformationThread(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtOpenThreadToken		[NTDLL] 
 */
DWORD WINAPI NtOpenThreadToken(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
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
 *  NtSetVolumeInformationFile		[NTDLL] 
 */
DWORD WINAPI NtSetVolumeInformationFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,
	DWORD x5
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtCreatePort		[NTDLL] 
 */
DWORD WINAPI NtCreatePort(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5);
	return 0;
}

