/*
 *	Rtl string functions
 *
 *	Copyright 1996-1998 Marcus Meissner
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_WCTYPE_H
# include <wctype.h>
#endif
#include "wine/winestring.h"
#include "crtdll.h"
#include "heap.h"
#include "winnls.h"
#include "debugtools.h"

#include "ntddk.h"

DEFAULT_DEBUG_CHANNEL(ntdll)
/*
 *	STRING FUNCTIONS
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
	target->MaximumLength = lstrlenA(target->Buffer);
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
	target->MaximumLength = lstrlenA(target->Buffer);
	target->Length = target->MaximumLength+1;
}

/**************************************************************************
 *                 RtlInitUnicodeString			[NTDLL.403]
 */
VOID WINAPI RtlInitUnicodeString(PUNICODE_STRING target,LPCWSTR source)
{
	TRACE("%p %p(%s)\n", target, source, debugstr_w(source));
	
	target->Length = target->MaximumLength = 0;
	target->Buffer = (LPWSTR)source;
	if (!source)
		return;
	target->MaximumLength = lstrlenW(target->Buffer)*2;
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
	FIXME("(%s,%s,%ld),stub!\n",debugstr_w(s1->Buffer),debugstr_w(s2->Buffer),x);
	return 0;
	if (s1->Length != s2->Length)
		return 1;
	return !CRTDLL_wcsncmp(s1->Buffer,s2->Buffer,s1->Length/2);
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
UINT WINAPI RtlxOemStringToUnicodeSize(PSTRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *                 RtlxAnsiStringToUnicodeSize		[NTDLL.548]
 */
UINT WINAPI RtlxAnsiStringToUnicodeSize(PANSI_STRING str)
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
	FIXME("(%s,%s,0x%08x),stub!\n",debugstr_w(String1->Buffer),debugstr_w(String1->Buffer),CaseInSensitive);
	return 0;
}


