/*
 * NT basis DLL
 * 
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "win.h"
#include "windows.h"
#include "stddebug.h"
#include "debug.h"
#include "module.h"
#include "xmalloc.h"
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
		return 0xC0000023;
	memmove(to,from,from->SubAuthorityCount*4+8);
	return 0;
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
	x=(LPWSTR)xmalloc((len+1)*sizeof(WCHAR));
	lstrcpynAtoW(x,oemstr,len+1);
	memcpy(unistr,x,len*2);
	if (reslen) *reslen = len*2;
	return 0;
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
	x=(LPSTR)xmalloc(len+1);
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
		oem->Buffer = (LPSTR)xmalloc(uni->Length/2)+1;
		oem->MaximumLength = uni->Length/2+1;
	}
	oem->Length = uni->Length/2;
	lstrcpynWtoA(oem->Buffer,uni->Buffer,uni->Length/2+1);
	return 0;
}
