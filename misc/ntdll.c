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
#include "heap.h"
#include "stddebug.h"
#include "debug.h"
#include "module.h"
#include "heap.h"

/**************************************************************************
 *                 RtlLengthRequiredSid			[NTDLL]
 */
DWORD
RtlLengthRequiredSid(DWORD nrofsubauths) {
	return sizeof(DWORD)*nrofsubauths+sizeof(SID);
}

/**************************************************************************
 *                 RtlNormalizeProcessParams		[NTDLL]
 */
LPVOID
RtlNormalizeProcessParams(LPVOID x)
{
    fprintf(stdnimp,"RtlNormalizeProcessParams(%p), stub.\n",x);
    return x;
}

/**************************************************************************
 *                 RtlInitializeSid			[NTDLL]
 */
DWORD
RtlInitializeSid(LPSID lpsid,LPSID_IDENTIFIER_AUTHORITY lpsidauth,DWORD c) {
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
LPDWORD
RtlSubAuthoritySid(LPSID lpsid,DWORD nr) {
	return &(lpsid->SubAuthority[nr]);
}

/**************************************************************************
 *                 RtlSubAuthorityCountSid		[NTDLL]
 */
LPBYTE
RtlSubAuthorityCountSid(LPSID lpsid) {
	return ((LPBYTE)lpsid)+1;
}

/**************************************************************************
 *                 RtlCopySid				[NTDLL]
 */
DWORD
RtlCopySid(DWORD len,LPSID to,LPSID from) {
	if (len<(from->SubAuthorityCount*4+8))
		return STATUS_BUFFER_TOO_SMALL;
	memmove(to,from,from->SubAuthorityCount*4+8);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlAnsiStringToUnicodeString		[NTDLL]
 */
DWORD /* NTSTATUS */
RtlAnsiStringToUnicodeString(LPUNICODE_STRING uni,LPANSI_STRING ansi,BOOL32 doalloc) {
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
DWORD /* NTSTATUS */
RtlOemStringToUnicodeString(LPUNICODE_STRING uni,LPSTRING ansi,BOOL32 doalloc) {
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
 *                 RtlOemToUnicodeN			[NTDLL]
 */
DWORD /* NTSTATUS */
RtlOemToUnicodeN(LPWSTR unistr,DWORD unilen,LPDWORD reslen,LPSTR oemstr,DWORD oemlen) {
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
VOID
RtlInitAnsiString(LPANSI_STRING target,LPCSTR source) {
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
VOID
RtlInitString(LPSTRING target,LPCSTR source) {
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
VOID
RtlInitUnicodeString(LPUNICODE_STRING target,LPCWSTR source) {
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPWSTR)source;
	if (!source)
		return;
	target->Length = lstrlen32W(target->Buffer)*2;
	target->MaximumLength = target->Length+2;
}

/**************************************************************************
 *                 RtlUnicodeToOemN			[NTDLL]
 */
DWORD /* NTSTATUS */
RtlUnicodeToOemN(LPSTR oemstr,DWORD oemlen,LPDWORD reslen,LPWSTR unistr,DWORD unilen) {
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
DWORD /* NTSTATUS */
RtlUnicodeStringToOemString(LPUNICODE_STRING uni,LPANSI_STRING oem,BOOL32 alloc)
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
DWORD
RtlNtStatusToDosError(DWORD error) {
	/* FIXME: map STATUS_ to ERROR_ */
	return error;
}

/**************************************************************************
 *                 RtlGetNtProductType			[NTDLL]
 */
DWORD
RtlGetNtProductType(LPVOID x) {
	/* FIXME : find documentation for this one */
	return 0;
}

/**************************************************************************
 *                 RtlUpcaseUnicodeString		[NTDLL]
 */
DWORD
RtlUpcaseUnicodeString(LPUNICODE_STRING dest,LPUNICODE_STRING src,BOOL32 doalloc) {
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
	for (i=0;i<len;i++) {
		s[i]=toupper(t[i]);
		s++;
		t++;
	}
	return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlxOemStringToUnicodeSize		[NTDLL]
 */
UINT32
RtlxOemStringToUnicodeSize(LPSTRING str) {
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlxAnsiStringToUnicodeSize		[NTDLL]
 */
UINT32
RtlxAnsiStringToUnicodeSize(LPANSI_STRING str) {
	return str->Length*2+2;
}
