/*
 *	Rtl string functions
 *
 *	Copyright 1996-1998 Marcus Meissner
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wine/winestring.h"
#include "wine/unicode.h"
#include "heap.h"
#include "winnls.h"
#include "debugtools.h"
#include "ntdll_misc.h"
#include "ntddk.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

/*	STRING FUNCTIONS	*/

/**************************************************************************
 *	RtlInitString
 */
VOID WINAPI RtlInitString(PSTRING target,LPCSTR source)
{
	TRACE("%p %p(%s)\n", target, source, debugstr_a(source));

	target->Buffer = (LPSTR)source;
	if (source)
	{
	  target->Length = lstrlenA(source);
	  target->MaximumLength = target->Length+1;
	}
	else
	{
	  target->Length = target->MaximumLength = 0;
	}
}



/*	ANSI FUNCTIONS	*/

/**************************************************************************
 *	RtlInitAnsiString
 */
VOID WINAPI RtlInitAnsiString(
	PANSI_STRING target,
	LPCSTR source)
{
	TRACE("%p %p(%s)\n", target, source, debugstr_a(source));

	target->Buffer = (LPSTR)source;
	if (source)
	{
	  target->Length = lstrlenA(source);
	  target->MaximumLength = target->Length+1;
	}
	else
	{
	  target->Length = target->MaximumLength = 0;
	}
}

/**************************************************************************
 *	RtlFreeAnsiString
 */
VOID WINAPI RtlFreeAnsiString(
	PANSI_STRING AnsiString)
{
	TRACE("%p\n", AnsiString);
	dump_AnsiString(AnsiString, TRUE);

	if( AnsiString->Buffer )
	  HeapFree( GetProcessHeap(),0,AnsiString->Buffer );
}



/*	UNICODE FUNCTIONS	*/

/**************************************************************************
 *	RtlInitUnicodeString			[NTDLL.403]
 */
VOID WINAPI RtlInitUnicodeString(
	PUNICODE_STRING target,
	LPCWSTR source)
{
	TRACE("%p %p(%s)\n", target, source, debugstr_w(source));
	
	target->Buffer = (LPWSTR)source;
	if (source)
	{
	  target->Length = lstrlenW(source)*2;
	  target->MaximumLength = target->Length + 2;
	}
	else
	{
	  target->Length = target->MaximumLength = 0;
	}
}

/**************************************************************************
 *	RtlFreeUnicodeString			[NTDLL.377]
 */
VOID WINAPI RtlFreeUnicodeString(
	PUNICODE_STRING UnicodeString)
{
	TRACE("%p\n", UnicodeString);
	dump_UnicodeString(UnicodeString, TRUE);

	if (UnicodeString->Buffer)
	  HeapFree(GetProcessHeap(),0,UnicodeString->Buffer);
}

/*
	 COMPARE FUNCTIONS
*/

/**************************************************************************
 *	RtlEqualUnicodeString
 */
BOOLEAN WINAPI RtlEqualUnicodeString(
	IN PUNICODE_STRING s1,
	IN PUNICODE_STRING s2,
        IN BOOLEAN CaseInsensitive) 
{
	BOOLEAN ret;
	TRACE("(%p,%p,%x)\n",s1,s2,CaseInsensitive);
	dump_UnicodeString(s1, TRUE);
	dump_UnicodeString(s2, TRUE);

	if(!s1 || !s2 || !s1->Buffer || !s2->Buffer) return FALSE;
	if (s1->Length != s2->Length) return FALSE;

	if (CaseInsensitive)
	  ret = !strncmpiW(s1->Buffer,s2->Buffer,s1->Length/sizeof(WCHAR));
	else	
	  ret = !strncmpW(s1->Buffer,s2->Buffer,s1->Length/sizeof(WCHAR));
	return ret;
}

/******************************************************************************
 *	RtlCompareUnicodeString
 */
LONG WINAPI RtlCompareUnicodeString(
	PUNICODE_STRING s1,
	PUNICODE_STRING s2,
	BOOLEAN CaseInsensitive) 
{
	LONG ret;

	TRACE("(%p,%p,%x)\n",s1,s2,CaseInsensitive);
	dump_UnicodeString(s1, TRUE);
	dump_UnicodeString(s2, TRUE);

	if(!s1 || !s2 || !s1->Buffer || !s2->Buffer) return FALSE;
	
	if (s1->Length != s2->Length) return (s1->Length - s2->Length);

	if (CaseInsensitive)
	  ret = strncmpiW(s1->Buffer,s2->Buffer,s1->Length/sizeof(WCHAR));
	else	
	  ret = strncmpW(s1->Buffer,s2->Buffer,s1->Length/sizeof(WCHAR));
	return ret;
}

/*
	COPY BETWEEN ANSI_STRING or UNICODE_STRING
	there is no parameter checking, it just crashes
*/

/**************************************************************************
 *	RtlAnsiStringToUnicodeString
 *
 * NOTES:
 *  writes terminating 0
 */
NTSTATUS
WINAPI RtlAnsiStringToUnicodeString(
	PUNICODE_STRING uni,
	PANSI_STRING ansi,
	BOOLEAN doalloc)
{
	int len = ansi->Length * sizeof(WCHAR);

	TRACE("%p %p %u\n",uni, ansi, doalloc);
	dump_AnsiString(ansi, TRUE);
	dump_UnicodeString(uni, FALSE);

	if (len>0xfffe) return STATUS_INVALID_PARAMETER_2;
	uni->Length = len;
	if (doalloc) 
	{
	  uni->MaximumLength = len + sizeof(WCHAR);
	  uni->Buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,uni->MaximumLength);
	  if (!uni->Buffer) return STATUS_NO_MEMORY;
	}
	else if (len+sizeof(WCHAR) > uni->MaximumLength)
	{
	  return STATUS_BUFFER_OVERFLOW;
	}
	lstrcpynAtoW(uni->Buffer,ansi->Buffer,ansi->Length+1);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlUpcaseUnicodeString
 *
 * NOTES:
 *  destination string is never 0-terminated because dest can be equal to src
 *  and src might be not 0-terminated
 *  dest.Length only set when success
 */
DWORD WINAPI RtlUpcaseUnicodeString(
	PUNICODE_STRING dest,
	PUNICODE_STRING src,
	BOOLEAN doalloc)
{
	int i, len = src->Length;

	TRACE("(%p,%p,%x)\n",dest, src, doalloc);
	dump_UnicodeString(dest, FALSE);
	dump_UnicodeString(src, TRUE);

	if (doalloc)
	{
	  dest->Buffer = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
	  if (!dest->Buffer) return STATUS_NO_MEMORY;
	  dest->MaximumLength = len; 
	}
	else if (len > dest->MaximumLength)
	{
	  return STATUS_BUFFER_OVERFLOW;
	}

	for (i=0; i < len/sizeof(WCHAR); i++)
	{
	  dest->Buffer[i] = toupperW(src->Buffer[i]);
	}
	dest->Length = len;
	return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlOemStringToUnicodeString
 *
 * NOTES
 *  writes terminating 0
 *  buffer must be bigger than (ansi->Length+1)*2
 *  if astr.Length > 0x7ffe it returns STATUS_INVALID_PARAMETER_2
 *
 * FIXME
 *  OEM to unicode
 */
NTSTATUS
WINAPI RtlOemStringToUnicodeString(
	PUNICODE_STRING uni,
	PSTRING ansi,
	BOOLEAN doalloc)
{
	int len = ansi->Length * sizeof(WCHAR);

	if (len > 0xfffe) return STATUS_INVALID_PARAMETER_2;

	uni->Length = len;
	if (doalloc) 
	{
	  uni->Buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, len + sizeof(WCHAR));
	  if (!uni->Buffer) return STATUS_NO_MEMORY;
	  uni->MaximumLength = len + sizeof(WCHAR);
	}
	else if (len+1 > uni->MaximumLength )
	{
	  return STATUS_BUFFER_OVERFLOW;
	}
	lstrcpynAtoW(uni->Buffer,ansi->Buffer,ansi->Length+1);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlUnicodeStringToOemString
 *
 * NOTES
 *   allocates uni->Length+1
 *   writes terminating 0
 */
NTSTATUS
WINAPI RtlUnicodeStringToOemString(
	PANSI_STRING oem,
	PUNICODE_STRING uni,
	BOOLEAN doalloc)
{
	int len = uni->Length/sizeof(WCHAR);

	TRACE("%p %s %i\n", oem, debugstr_us(uni), doalloc);

	oem->Length = len;
	if (doalloc)
	{
	  oem->Buffer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len+1);
	  if (! oem->Buffer) return STATUS_NO_MEMORY;
	  oem->MaximumLength = len + 1;
	}
	else if (oem->MaximumLength <= len)
	{
	  return STATUS_BUFFER_OVERFLOW;
	}
	lstrcpynWtoA(oem->Buffer, uni->Buffer, len+1);
	return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlUpcaseUnicodeStringToOemString
 *
 * NOTES
 *  writes terminating 0
 */
NTSTATUS
WINAPI RtlUpcaseUnicodeStringToOemString(
	PANSI_STRING oem,
	PUNICODE_STRING uni,
	BOOLEAN doalloc)
{
	int i, len = uni->Length/sizeof(WCHAR);

	TRACE("%p %s %i\n", oem, debugstr_us(uni), doalloc);

	oem->Length = len;
	if (doalloc)
	{
	  oem->Buffer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len+1);
	  if (! oem->Buffer) return STATUS_NO_MEMORY;
	  oem->MaximumLength = len + 1;
	}
	else if (oem->MaximumLength <= len)
	{
	  return STATUS_BUFFER_OVERFLOW;
	}

	for (i=0; i < len; i++)
	{
	  oem->Buffer[i] = toupperW((char)(uni->Buffer[i]));
	}
	oem->Buffer[i] = 0;
	return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlUnicodeStringToAnsiString
 *
 * NOTES
 *  writes terminating 0
 *  copys a part if the buffer is to small
 */
NTSTATUS
WINAPI RtlUnicodeStringToAnsiString(
	PANSI_STRING ansi,
	PUNICODE_STRING uni,
	BOOLEAN doalloc)
{
	int len = uni->Length/sizeof(WCHAR);
	NTSTATUS ret = STATUS_SUCCESS;

	TRACE("%p %s %i\n", ansi, debugstr_us(uni), doalloc);

	ansi->Length = len;
	if (doalloc)
	{
	  ansi->Buffer = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len+1);
	  if (! ansi->Buffer) return STATUS_NO_MEMORY;
	  ansi->MaximumLength = len + 1;
	}
	else if (ansi->MaximumLength <= len)
	{
	  ansi->Length = ansi->MaximumLength - 1;
	  ret = STATUS_BUFFER_OVERFLOW;
	}
	lstrcpynWtoA(ansi->Buffer, uni->Buffer, ansi->Length+1);
	return ret;
}

/*
	COPY BETWEEN ANSI/UNICODE_STRING AND MULTIBYTE STRINGS
*/
/**************************************************************************
 *	RtlMultiByteToUnicodeN
 *
 * NOTES
 *  dest can be equal to src
 *  if unistr is to small a part is copyed
 *
 * FIXME
 *  multibyte support
 */
NTSTATUS
WINAPI RtlMultiByteToUnicodeN(
	LPWSTR unistr,
	DWORD unilen,
	LPDWORD reslen,
	LPSTR oemstr,
	DWORD oemlen)
{
	UINT len;
	int i;

	TRACE("%p %lu, %p, %s, %lu\n", oemstr, oemlen, reslen, debugstr_w(unistr), unilen);

	len = (unilen/sizeof(WCHAR) < oemlen) ? unilen/sizeof(WCHAR) : oemlen;

	for (i = len-1; i>=0; i--)
	  unistr[i] = (WCHAR)oemstr[i];

	if (reslen) *reslen = len * 2;
	return 0;
}

/**************************************************************************
 *	RtlOemToUnicodeN
 */
NTSTATUS
WINAPI RtlOemToUnicodeN(
	LPWSTR unistr,
	DWORD unilen,
	LPDWORD reslen,
	LPSTR oemstr,
	DWORD oemlen)
{
	UINT len;
	int i;

	TRACE("%p %lu, %p, %s, %lu\n", oemstr, oemlen, reslen, debugstr_w(unistr), unilen);

	len = (unilen/sizeof(WCHAR) < oemlen) ? unilen/sizeof(WCHAR) : oemlen;

	for (i = len-1; i>=0; i--)
	  unistr[i] = (WCHAR)oemstr[i];

	if (reslen) *reslen = len * 2;
	return 0;
}


/**************************************************************************
 *	RtlUnicodeToOemN
 */
NTSTATUS
WINAPI RtlUnicodeToOemN(
	LPSTR oemstr,
	DWORD oemlen,
	LPDWORD reslen,
	LPWSTR unistr,
	DWORD unilen)
{
	UINT len;
	int i;

	TRACE("%p %lu, %p, %s, %lu\n", oemstr, oemlen, reslen, debugstr_w(unistr), unilen);

	len = (oemlen < unilen/sizeof(WCHAR)) ? unilen/sizeof(WCHAR) : oemlen;

	for (i = len-1; i>=0; i--)
	  oemstr[i] = (CHAR)unistr[i];

	if (reslen) *reslen = len * 2;
	return 0;
}

/**************************************************************************
 *	RtlUpcaseUnicodeToOemN
 */
NTSTATUS
WINAPI RtlUpcaseUnicodeToOemN(
	LPSTR oemstr,
	DWORD oemlen,
	LPDWORD reslen,
	LPWSTR unistr,
	DWORD unilen)
{
	UINT len;
	int i;

	TRACE("%p %lu, %p, %s, %lu\n", oemstr, oemlen, reslen, debugstr_w(unistr), unilen);

	len = (oemlen < unilen/sizeof(WCHAR)) ? unilen/sizeof(WCHAR) : oemlen;

	for (i = len-1; i>=0; i--)
	  oemstr[i] = toupper((CHAR)unistr[i]);

	if (reslen) *reslen = len * 2;
	return 0;
}

/**************************************************************************
 *	RtlUnicodeToMultiByteN
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteN(
	PCHAR  MbString,
	ULONG  MbSize,
	PULONG ResultSize,
	PWCHAR UnicodeString,
	ULONG  UnicodeSize)
{
        int Size = 0, i;

	TRACE("(%p,%lu,%p,%p(%s),%lu)\n",
	MbString, MbSize, ResultSize, UnicodeString, debugstr_w(UnicodeString), UnicodeSize);

	Size = (UnicodeSize > (MbSize*sizeof(WCHAR))) ? MbSize: UnicodeSize/sizeof(WCHAR);

	if (ResultSize != NULL) *ResultSize = Size;

	for(i = 0; i < Size; i++)
	{
	  *(MbString++) = *(UnicodeString++);
        }
	return STATUS_SUCCESS;      
}

/*
	STRING SIZE

	Rtlx* supports multibyte
*/

/**************************************************************************
 *	RtlxOemStringToUnicodeSize
 */
UINT WINAPI RtlxOemStringToUnicodeSize(PSTRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *	RtlxAnsiStringToUnicodeSize
 */
UINT WINAPI RtlxAnsiStringToUnicodeSize(PANSI_STRING str)
{
	return str->Length*2+2;
}

/**************************************************************************
 *	RtlxUnicodeStringToOemSize
 *
 */
UINT WINAPI RtlxUnicodeStringToOemSize(PUNICODE_STRING str)
{
	return str->Length+1;
}

/*
	MISC
*/

/**************************************************************************
 *	RtlIsTextUnicode
 *
 *	Apply various feeble heuristics to guess whether
 *	the text buffer contains Unicode.
 *	FIXME: should implement more tests.
 */
DWORD WINAPI RtlIsTextUnicode(
	LPVOID buf,
	DWORD len,
	DWORD *pf)
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
 *	RtlPrefixUnicodeString
 */
NTSTATUS WINAPI RtlPrefixUnicodeString(
	PUNICODE_STRING a,
	PUNICODE_STRING b,
	DWORD x)
{
	TRACE("(%s,%s,%lx)\n",debugstr_us(a),debugstr_us(b),x);
	return STATUS_SUCCESS;
}

