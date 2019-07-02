/*
 * Rtl string functions
 *
 * Copyright (C) 1996-1998 Marcus Meissner
 * Copyright (C) 2000      Alexandre Julliard
 * Copyright (C) 2003      Thomas Mertes
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

#define GUID_STRING_LENGTH    38

UINT NlsAnsiCodePage = 0;
BYTE NlsMbCodePageTag = 0;
BYTE NlsMbOemCodePageTag = 0;

extern const union cptable cptable_20127;  /* 7-bit ASCII */

static const union cptable *ansi_table = &cptable_20127;
static const union cptable *oem_table = &cptable_20127;
static const union cptable* unix_table; /* NULL if UTF8 */


/**************************************************************************
 *	__wine_init_codepages   (NTDLL.@)
 *
 * Set the code page once kernel32 is loaded. Should be done differently.
 */
void CDECL __wine_init_codepages( const union cptable *ansi, const union cptable *oem,
                                  const union cptable *ucp)
{
    ansi_table = ansi;
    oem_table = oem;
    unix_table = ucp;
    NlsAnsiCodePage = ansi->info.codepage;
}

int ntdll_umbstowcs(DWORD flags, const char* src, int srclen, WCHAR* dst, int dstlen)
{
#ifdef __APPLE__
    /* work around broken Mac OS X filesystem that enforces decomposed Unicode */
    if (!unix_table) flags |= MB_COMPOSITE;
#endif
    return (unix_table) ?
        wine_cp_mbstowcs( unix_table, flags, src, srclen, dst, dstlen ) :
        wine_utf8_mbstowcs( flags, src, srclen, dst, dstlen );
}

int ntdll_wcstoumbs(DWORD flags, const WCHAR* src, int srclen, char* dst, int dstlen,
                    const char* defchar, int *used )
{
    if (unix_table)
        return wine_cp_wcstombs( unix_table, flags, src, srclen, dst, dstlen, defchar, used );
    if (used) *used = 0;  /* all chars are valid for UTF-8 */
    return wine_utf8_wcstombs( flags, src, srclen, dst, dstlen );
}

/**************************************************************************
 *      RtlInitAnsiString   (NTDLL.@)
 *
 * Initializes a buffered ansi string.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Assigns source to target->Buffer. The length of source is assigned to
 *  target->Length and target->MaximumLength. If source is NULL the length
 *  of source is assumed to be 0.
 */
void WINAPI RtlInitAnsiString(
    PANSI_STRING target, /* [I/O] Buffered ansi string to be initialized */
    PCSZ source)         /* [I]   '\0' terminated string used to initialize target */
{
    if ((target->Buffer = (PCHAR) source))
    {
        target->Length = strlen(source);
        target->MaximumLength = target->Length + 1;
    }
    else target->Length = target->MaximumLength = 0;
}

/**************************************************************************
 *      RtlInitAnsiStringEx   (NTDLL.@)
 *
 * Initializes a buffered ansi string.
 *
 * RETURNS
 *  An appropriate NTSTATUS value.
 *
 * NOTES
 *  Assigns source to target->Buffer. The length of source is assigned to
 *  target->Length and target->MaximumLength. If source is NULL the length
 *  of source is assumed to be 0.
 */
NTSTATUS WINAPI RtlInitAnsiStringEx(PANSI_STRING target, PCSZ source)
{
    if (source)
    {
        unsigned int len = strlen(source);
        if (len+1 > 0xffff)
            return STATUS_NAME_TOO_LONG;

        target->Buffer = (PCHAR) source;
        target->Length = len;
        target->MaximumLength = len + 1;
    }
    else
    {
        target->Buffer = NULL;
        target->Length = 0;
        target->MaximumLength = 0;
    }
    return STATUS_SUCCESS;
}

/**************************************************************************
 *      RtlInitString   (NTDLL.@)
 *
 * Initializes a buffered string.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Assigns source to target->Buffer. The length of source is assigned to
 *  target->Length and target->MaximumLength. If source is NULL the length
 *  of source is assumed to be 0.
 */
void WINAPI RtlInitString(
    PSTRING target, /* [I/O] Buffered string to be initialized */
    PCSZ source)    /* [I]   '\0' terminated string used to initialize target */
{
    RtlInitAnsiString( target, source );
}


/**************************************************************************
 *	RtlFreeAnsiString   (NTDLL.@)
 */
void WINAPI RtlFreeAnsiString( PSTRING str )
{
    if (str->Buffer)
    {
        RtlFreeHeap( GetProcessHeap(), 0, str->Buffer );
        RtlZeroMemory( str, sizeof(*str) );
    }
}


/**************************************************************************
 *	RtlFreeOemString   (NTDLL.@)
 */
void WINAPI RtlFreeOemString( PSTRING str )
{
    RtlFreeAnsiString( str );
}


/**************************************************************************
 *	RtlCopyString   (NTDLL.@)
 */
void WINAPI RtlCopyString( STRING *dst, const STRING *src )
{
    if (src)
    {
        unsigned int len = min( src->Length, dst->MaximumLength );
        memcpy( dst->Buffer, src->Buffer, len );
        dst->Length = len;
    }
    else dst->Length = 0;
}


/**************************************************************************
 *      RtlInitUnicodeString   (NTDLL.@)
 *
 * Initializes a buffered unicode string.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Assigns source to target->Buffer. The length of source is assigned to
 *  target->Length and target->MaximumLength. If source is NULL the length
 *  of source is assumed to be 0.
 */
void WINAPI RtlInitUnicodeString(
    PUNICODE_STRING target, /* [I/O] Buffered unicode string to be initialized */
    PCWSTR source)          /* [I]   '\0' terminated unicode string used to initialize target */
{
    if ((target->Buffer = (PWSTR) source))
    {
        unsigned int length = strlenW(source) * sizeof(WCHAR);
        if (length > 0xfffc)
            length = 0xfffc;
        target->Length = length;
        target->MaximumLength = target->Length + sizeof(WCHAR);
    }
    else target->Length = target->MaximumLength = 0;
}


/**************************************************************************
 *      RtlInitUnicodeStringEx   (NTDLL.@)
 *
 * Initializes a buffered unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. target is initialized.
 *  Failure: STATUS_NAME_TOO_LONG, if the source string is larger than 65532 bytes.
 *
 * NOTES
 *  Assigns source to target->Buffer. The length of source is assigned to
 *  target->Length and target->MaximumLength. If source is NULL the length
 *  of source is assumed to be 0.
 */
NTSTATUS WINAPI RtlInitUnicodeStringEx(
    PUNICODE_STRING target, /* [I/O] Buffered unicode string to be initialized */
    PCWSTR source)          /* [I]   '\0' terminated unicode string used to initialize target */
{
    if (source != NULL) {
        unsigned int len = strlenW(source) * sizeof(WCHAR);

        if (len > 0xFFFC) {
            return STATUS_NAME_TOO_LONG;
        } else {
            target->Length = len;
            target->MaximumLength = len + sizeof(WCHAR);
            target->Buffer = (PWSTR) source;
        } /* if */
    } else {
        target->Length = 0;
        target->MaximumLength = 0;
        target->Buffer = NULL;
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlCreateUnicodeString   (NTDLL.@)
 *
 * Creates a UNICODE_STRING from a null-terminated Unicode string.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOLEAN WINAPI RtlCreateUnicodeString( PUNICODE_STRING target, LPCWSTR src )
{
    int len = (strlenW(src) + 1) * sizeof(WCHAR);
    if (!(target->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len ))) return FALSE;
    memcpy( target->Buffer, src, len );
    target->MaximumLength = len;
    target->Length = len - sizeof(WCHAR);
    return TRUE;
}


/**************************************************************************
 *	RtlCreateUnicodeStringFromAsciiz   (NTDLL.@)
 *
 * Creates a UNICODE_STRING from a null-terminated Ascii string.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOLEAN WINAPI RtlCreateUnicodeStringFromAsciiz( PUNICODE_STRING target, LPCSTR src )
{
    STRING ansi;
    RtlInitAnsiString( &ansi, src );
    return !RtlAnsiStringToUnicodeString( target, &ansi, TRUE );
}


/**************************************************************************
 *	RtlFreeUnicodeString   (NTDLL.@)
 *
 * Frees a UNICODE_STRING created with RtlCreateUnicodeString() or 
 * RtlCreateUnicodeStringFromAsciiz().
 *
 * RETURNS
 *     nothing
 */
void WINAPI RtlFreeUnicodeString( PUNICODE_STRING str )
{
    if (str->Buffer)
    {
        RtlFreeHeap( GetProcessHeap(), 0, str->Buffer );
        RtlZeroMemory( str, sizeof(*str) );
    }
}


/**************************************************************************
 *	RtlCopyUnicodeString   (NTDLL.@)
 *
 * Copies from one UNICODE_STRING to another.
 *
 * RETURNS
 *     nothing
 */
void WINAPI RtlCopyUnicodeString( UNICODE_STRING *dst, const UNICODE_STRING *src )
{
    if (src)
    {
        unsigned int len = min( src->Length, dst->MaximumLength );
        memcpy( dst->Buffer, src->Buffer, len );
        dst->Length = len;
        /* append terminating '\0' if enough space */
        if (len < dst->MaximumLength) dst->Buffer[len / sizeof(WCHAR)] = 0;
    }
    else dst->Length = 0;
}


/**************************************************************************
 *      RtlDuplicateUnicodeString   (NTDLL.@)
 *
 * Duplicates a unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. destination contains the duplicated unicode string.
 *  Failure: STATUS_INVALID_PARAMETER, if one of the parameters is illegal.
 *           STATUS_NO_MEMORY, if the allocation fails.
 *
 * NOTES
 *  For add_nul there are several possible values:
 *  0 = destination will not be '\0' terminated,
 *  1 = destination will be '\0' terminated,
 *  3 = like 1 but for an empty source string produce '\0' terminated empty
 *     Buffer instead of assigning NULL to the Buffer.
 *  Other add_nul values are invalid.
 */
NTSTATUS WINAPI RtlDuplicateUnicodeString(
    int add_nul,                  /* [I] flag */
    const UNICODE_STRING *source, /* [I] Unicode string to be duplicated */
    UNICODE_STRING *destination)  /* [O] destination for the duplicated unicode string */
{
    if (source == NULL || destination == NULL ||
        source->Length > source->MaximumLength ||
        (source->Length == 0 && source->MaximumLength > 0 && source->Buffer == NULL) ||
        add_nul == 2 || add_nul >= 4 || add_nul < 0) {
        return STATUS_INVALID_PARAMETER;
    } else {
        if (source->Length == 0 && add_nul != 3) {
            destination->Length = 0;
            destination->MaximumLength = 0;
            destination->Buffer = NULL;
        } else {
            unsigned int destination_max_len = source->Length;

            if (add_nul) {
                destination_max_len += sizeof(WCHAR);
            } /* if */
            destination->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, destination_max_len);
            if (destination->Buffer == NULL) {
                return STATUS_NO_MEMORY;
            } else {
                memcpy(destination->Buffer, source->Buffer, source->Length);
                destination->Length = source->Length;
                destination->MaximumLength = source->Length;
                /* append terminating '\0' if enough space */
                if (add_nul) {
                    destination->MaximumLength = destination_max_len;
                    destination->Buffer[destination->Length / sizeof(WCHAR)] = 0;
                } /* if */
            } /* if */
        } /* if */
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlEraseUnicodeString   (NTDLL.@)
 *
 * Overwrites a UNICODE_STRING with zeros.
 *
 * RETURNS
 *     nothing
 */
void WINAPI RtlEraseUnicodeString( UNICODE_STRING *str )
{
    if (str->Buffer)
    {
        memset( str->Buffer, 0, str->MaximumLength );
        str->Length = 0;
    }
}


/*
    COMPARISON FUNCTIONS
*/


/******************************************************************************
 *	RtlCompareString   (NTDLL.@)
 */
LONG WINAPI RtlCompareString( const STRING *s1, const STRING *s2, BOOLEAN CaseInsensitive )
{
    unsigned int len;
    LONG ret = 0;
    LPCSTR p1, p2;

    len = min(s1->Length, s2->Length);
    p1 = s1->Buffer;
    p2 = s2->Buffer;

    if (CaseInsensitive)
    {
        while (!ret && len--) ret = RtlUpperChar(*p1++) - RtlUpperChar(*p2++);
    }
    else
    {
        while (!ret && len--) ret = *p1++ - *p2++;
    }
    if (!ret) ret = s1->Length - s2->Length;
    return ret;
}


/******************************************************************************
 *	RtlCompareUnicodeStrings   (NTDLL.@)
 */
LONG WINAPI RtlCompareUnicodeStrings( const WCHAR *s1, SIZE_T len1, const WCHAR *s2, SIZE_T len2,
                                      BOOLEAN case_insensitive )
{
    LONG ret = 0;
    SIZE_T len = min( len1, len2 );

    if (case_insensitive)
    {
        while (!ret && len--) ret = toupperW(*s1++) - toupperW(*s2++);
    }
    else
    {
        while (!ret && len--) ret = *s1++ - *s2++;
    }
    if (!ret) ret = len1 - len2;
    return ret;
}


/******************************************************************************
 *	RtlCompareUnicodeString   (NTDLL.@)
 */
LONG WINAPI RtlCompareUnicodeString( const UNICODE_STRING *s1, const UNICODE_STRING *s2,
                                     BOOLEAN CaseInsensitive )
{
    return RtlCompareUnicodeStrings( s1->Buffer, s1->Length / sizeof(WCHAR),
                                     s2->Buffer, s2->Length / sizeof(WCHAR), CaseInsensitive );
}


/**************************************************************************
 *	RtlEqualString   (NTDLL.@)
 *
 * Determine if two strings are equal.
 *
 * PARAMS
 *  s1              [I] Source string
 *  s2              [I] String to compare to s1
 *  CaseInsensitive [I] TRUE = Case insensitive, FALSE = Case sensitive
 *
 * RETURNS
 *  Non-zero if s1 is equal to s2, 0 otherwise.
 */
BOOLEAN WINAPI RtlEqualString( const STRING *s1, const STRING *s2, BOOLEAN CaseInsensitive )
{
    if (s1->Length != s2->Length) return FALSE;
    return !RtlCompareString( s1, s2, CaseInsensitive );
}


/**************************************************************************
 *	RtlEqualUnicodeString   (NTDLL.@)
 *
 * Unicode version of RtlEqualString.
 */
BOOLEAN WINAPI RtlEqualUnicodeString( const UNICODE_STRING *s1, const UNICODE_STRING *s2,
                                      BOOLEAN CaseInsensitive )
{
    if (s1->Length != s2->Length) return FALSE;
    return !RtlCompareUnicodeString( s1, s2, CaseInsensitive );
}


/**************************************************************************
 *	RtlPrefixString   (NTDLL.@)
 *
 * Determine if one string is a prefix of another.
 *
 * PARAMS
 *  s1          [I] Prefix to look for in s2
 *  s2          [I] String that may contain s1 as a prefix
 *  ignore_case [I] TRUE = Case insensitive, FALSE = Case sensitive
 *
 * RETURNS
 *  TRUE if s2 contains s1 as a prefix, FALSE otherwise.
 */
BOOLEAN WINAPI RtlPrefixString( const STRING *s1, const STRING *s2, BOOLEAN ignore_case )
{
    unsigned int i;

    if (s1->Length > s2->Length) return FALSE;
    if (ignore_case)
    {
        for (i = 0; i < s1->Length; i++)
            if (RtlUpperChar(s1->Buffer[i]) != RtlUpperChar(s2->Buffer[i])) return FALSE;
    }
    else
    {
        for (i = 0; i < s1->Length; i++)
            if (s1->Buffer[i] != s2->Buffer[i]) return FALSE;
    }
    return TRUE;
}


/**************************************************************************
 *	RtlPrefixUnicodeString   (NTDLL.@)
 *
 * Unicode version of RtlPrefixString.
 */
BOOLEAN WINAPI RtlPrefixUnicodeString( const UNICODE_STRING *s1,
                                       const UNICODE_STRING *s2,
                                       BOOLEAN ignore_case )
{
    unsigned int i;

    if (s1->Length > s2->Length) return FALSE;
    if (ignore_case)
    {
        for (i = 0; i < s1->Length / sizeof(WCHAR); i++)
            if (toupperW(s1->Buffer[i]) != toupperW(s2->Buffer[i])) return FALSE;
    }
    else
    {
        for (i = 0; i < s1->Length / sizeof(WCHAR); i++)
            if (s1->Buffer[i] != s2->Buffer[i]) return FALSE;
    }
    return TRUE;
}


/**************************************************************************
 *	RtlEqualComputerName   (NTDLL.@)
 *
 * Determine if two computer names are the same.
 *
 * PARAMS
 *  left  [I] First computer name
 *  right [I] Second computer name
 *
 * RETURNS
 *  0 if the names are equal, non-zero otherwise.
 *
 * NOTES
 *  The comparison is case insensitive.
 */
NTSTATUS WINAPI RtlEqualComputerName(const UNICODE_STRING *left,
                                     const UNICODE_STRING *right)
{
    NTSTATUS ret;
    STRING upLeft, upRight;

    if (!(ret = RtlUpcaseUnicodeStringToOemString( &upLeft, left, TRUE )))
    {
       if (!(ret = RtlUpcaseUnicodeStringToOemString( &upRight, right, TRUE )))
       {
         ret = RtlEqualString( &upLeft, &upRight, FALSE );
         RtlFreeOemString( &upRight );
       }
       RtlFreeOemString( &upLeft );
    }
    return ret;
}


/**************************************************************************
 *	RtlEqualDomainName   (NTDLL.@)
 *
 * Determine if two domain names are the same.
 *
 * PARAMS
 *  left  [I] First domain name
 *  right [I] Second domain name
 *
 * RETURNS
 *  0 if the names are equal, non-zero otherwise.
 *
 * NOTES
 *  The comparison is case insensitive.
 */
NTSTATUS WINAPI RtlEqualDomainName(const UNICODE_STRING *left,
                                   const UNICODE_STRING *right)
{
    return RtlEqualComputerName(left, right);
}


/**************************************************************************
 *      RtlAnsiCharToUnicodeChar   (NTDLL.@)
 *
 * Converts the first ansi character to a unicode character.
 *
 * PARAMS
 *  ansi [I/O] Pointer to the ansi string.
 *
 * RETURNS
 *  Unicode representation of the first character in the ansi string.
 *
 * NOTES
 *  Upon successful completion, the char pointer ansi points to is
 *  incremented by the size of the character.
 */
WCHAR WINAPI RtlAnsiCharToUnicodeChar(LPSTR *ansi)
{
    WCHAR str;
    DWORD charSize = sizeof(CHAR);

    if (wine_is_dbcs_leadbyte(ansi_table, **ansi))
        charSize++;

    RtlMultiByteToUnicodeN(&str, sizeof(WCHAR), NULL, *ansi, charSize);
    *ansi += charSize;

    return str;
}

/*
        COPY BETWEEN ANSI_STRING or UNICODE_STRING
        there is no parameter checking, it just crashes
*/


/**************************************************************************
 *      RtlAnsiStringToUnicodeString   (NTDLL.@)
 *
 * Converts an ansi string to a unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. uni contains the converted string
 *  Failure: STATUS_BUFFER_OVERFLOW, if doalloc is FALSE and ansi is too small.
 *           STATUS_NO_MEMORY, if doalloc is TRUE and the allocation fails.
 *           STATUS_INVALID_PARAMETER_2, if the unicode string would be larger than 65535.
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 */
NTSTATUS WINAPI RtlAnsiStringToUnicodeString(
    PUNICODE_STRING uni, /* [I/O] Destination for the unicode string */
    PCANSI_STRING ansi,  /* [I]   Ansi string to be converted */
    BOOLEAN doalloc)     /* [I]   TRUE=Allocate new buffer for uni, FALSE=Use existing buffer */
{
    DWORD total = RtlAnsiStringToUnicodeSize( ansi );

    if (total > 0xffff) return STATUS_INVALID_PARAMETER_2;
    uni->Length = total - sizeof(WCHAR);
    if (doalloc)
    {
        uni->MaximumLength = total;
        if (!(uni->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, total )))
            return STATUS_NO_MEMORY;
    }
    else if (total > uni->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    RtlMultiByteToUnicodeN( uni->Buffer, uni->Length, NULL, ansi->Buffer, ansi->Length );
    uni->Buffer[uni->Length / sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlOemStringToUnicodeString   (NTDLL.@)
 *
 * Converts an oem string to a unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. uni contains the converted string
 *  Failure: STATUS_BUFFER_OVERFLOW, if doalloc is FALSE and oem is too small.
 *           STATUS_NO_MEMORY, if doalloc is TRUE and the allocation fails.
 *           STATUS_INVALID_PARAMETER_2, if the unicode string would be larger than 65535.
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 */
NTSTATUS WINAPI RtlOemStringToUnicodeString(
    UNICODE_STRING *uni, /* [I/O] Destination for the unicode string */
    const STRING *oem,   /* [I]   Oem string to be converted */
    BOOLEAN doalloc)     /* [I]   TRUE=Allocate new buffer for uni, FALSE=Use existing buffer */
{
    DWORD total = RtlOemStringToUnicodeSize( oem );

    if (total > 0xffff) return STATUS_INVALID_PARAMETER_2;
    uni->Length = total - sizeof(WCHAR);
    if (doalloc)
    {
        uni->MaximumLength = total;
        if (!(uni->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, total )))
            return STATUS_NO_MEMORY;
    }
    else if (total > uni->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    RtlOemToUnicodeN( uni->Buffer, uni->Length, NULL, oem->Buffer, oem->Length );
    uni->Buffer[uni->Length / sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUnicodeStringToAnsiString   (NTDLL.@)
 *
 * Converts a unicode string to an ansi string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. ansi contains the converted string
 *  Failure: STATUS_BUFFER_OVERFLOW, if doalloc is FALSE and ansi is too small.
 *           STATUS_NO_MEMORY, if doalloc is TRUE and the allocation fails.
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS WINAPI RtlUnicodeStringToAnsiString(
    STRING *ansi,              /* [I/O] Destination for the ansi string */
    const UNICODE_STRING *uni, /* [I]   Unicode string to be converted */
    BOOLEAN doalloc)           /* [I]   TRUE=Allocate new buffer for ansi, FALSE=Use existing buffer */
{
    NTSTATUS ret = STATUS_SUCCESS;
    DWORD len = RtlUnicodeStringToAnsiSize( uni );

    ansi->Length = len - 1;
    if (doalloc)
    {
        ansi->MaximumLength = len;
        if (!(ansi->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len )))
            return STATUS_NO_MEMORY;
    }
    else if (ansi->MaximumLength < len)
    {
        if (!ansi->MaximumLength) return STATUS_BUFFER_OVERFLOW;
        ansi->Length = ansi->MaximumLength - 1;
        ret = STATUS_BUFFER_OVERFLOW;
    }

    RtlUnicodeToMultiByteN( ansi->Buffer, ansi->Length, NULL, uni->Buffer, uni->Length );
    ansi->Buffer[ansi->Length] = 0;
    return ret;
}


/**************************************************************************
 *	RtlUnicodeStringToOemString   (NTDLL.@)
 *
 * Converts a Rtl Unicode string to an OEM string.
 *
 * PARAMS
 *  oem     [O] Destination for OEM string
 *  uni     [I] Source Unicode string
 *  doalloc [I] TRUE=Allocate new buffer for oem,FALSE=Use existing buffer
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. oem contains the converted string
 *  Failure: STATUS_BUFFER_OVERFLOW, if doalloc is FALSE and oem is too small.
 *           STATUS_NO_MEMORY, if doalloc is TRUE and allocation fails.
 *
 * NOTES
 *   If doalloc is TRUE, the length allocated is uni->Length + 1.
 *   This function always '\0' terminates the string returned.
 */
NTSTATUS WINAPI RtlUnicodeStringToOemString( STRING *oem,
                                             const UNICODE_STRING *uni,
                                             BOOLEAN doalloc )
{
    NTSTATUS ret = STATUS_SUCCESS;
    DWORD len = RtlUnicodeStringToOemSize( uni );

    oem->Length = len - 1;
    if (doalloc)
    {
        oem->MaximumLength = len;
        if (!(oem->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len )))
            return STATUS_NO_MEMORY;
    }
    else if (oem->MaximumLength < len)
    {
        if (!oem->MaximumLength) return STATUS_BUFFER_OVERFLOW;
        oem->Length = oem->MaximumLength - 1;
        ret = STATUS_BUFFER_OVERFLOW;
    }

    RtlUnicodeToOemN( oem->Buffer, oem->Length, NULL, uni->Buffer, uni->Length );
    oem->Buffer[oem->Length] = 0;
    return ret;
}


/**************************************************************************
 *	RtlMultiByteToUnicodeN   (NTDLL.@)
 *
 * Converts a multi-byte string to a Unicode string.
 *
 * RETURNS
 *  NTSTATUS code
 *
 * NOTES
 *  Performs a partial copy if dst is too small.
 */
NTSTATUS WINAPI RtlMultiByteToUnicodeN( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                        LPCSTR src, DWORD srclen )
{

    int ret = wine_cp_mbstowcs( ansi_table, 0, src, srclen, dst, dstlen/sizeof(WCHAR) );
    if (reslen)
        *reslen = (ret >= 0) ? ret*sizeof(WCHAR) : dstlen; /* overflow -> we filled up to dstlen */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlOemToUnicodeN   (NTDLL.@)
 *
 * Converts a multi-byte string in the OEM code page to a Unicode string.
 *
 * RETURNS
 *  NTSTATUS code
 */
NTSTATUS WINAPI RtlOemToUnicodeN( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                  LPCSTR src, DWORD srclen )
{
    int ret = wine_cp_mbstowcs( oem_table, 0, src, srclen, dst, dstlen/sizeof(WCHAR) );
    if (reslen)
        *reslen = (ret >= 0) ? ret*sizeof(WCHAR) : dstlen; /* overflow -> we filled up to dstlen */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUnicodeToMultiByteN   (NTDLL.@)
 *
 * Converts a Unicode string to a multi-byte string in the ANSI code page.
 *
 * RETURNS
 *  NTSTATUS code
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteN( LPSTR dst, DWORD dstlen, LPDWORD reslen,
                                        LPCWSTR src, DWORD srclen )
{
    int ret = wine_cp_wcstombs( ansi_table, 0, src, srclen / sizeof(WCHAR),
                                dst, dstlen, NULL, NULL );
    if (reslen)
        *reslen = (ret >= 0) ? ret : dstlen; /* overflow -> we filled up to dstlen */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUnicodeToOemN   (NTDLL.@)
 *
 * Converts a Unicode string to a multi-byte string in the OEM code page.
 *
 * RETURNS
 *  NTSTATUS code
 */
NTSTATUS WINAPI RtlUnicodeToOemN( LPSTR dst, DWORD dstlen, LPDWORD reslen,
                                  LPCWSTR src, DWORD srclen )
{
    int ret = wine_cp_wcstombs( oem_table, 0, src, srclen / sizeof(WCHAR),
                                dst, dstlen, NULL, NULL );
    if (reslen)
        *reslen = (ret >= 0) ? ret : dstlen; /* overflow -> we filled up to dstlen */
    return STATUS_SUCCESS;
}


/*
     CASE CONVERSIONS
*/


/**************************************************************************
 *	RtlUpperChar   (NTDLL.@)
 *
 * Converts an Ascii character to uppercase.
 *
 * PARAMS
 *  ch [I] Character to convert
 *
 * RETURNS
 *  The uppercase character value.
 *
 * NOTES
 *  For the input characters from 'a' .. 'z' it returns 'A' .. 'Z'.
 *  All other input characters are returned unchanged. The locale and
 *  multibyte characters are not taken into account (as native DLL).
 */
CHAR WINAPI RtlUpperChar( CHAR ch )
{
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 'A';
    } else {
        return ch;
    } /* if */
}


/**************************************************************************
 *	RtlUpperString   (NTDLL.@)
 *
 * Converts an Ascii string to uppercase.
 *
 * PARAMS
 *  dst [O] Destination for converted string
 *  src [I] Source string to convert
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  For the src characters from 'a' .. 'z' it assigns 'A' .. 'Z' to dst.
 *  All other src characters are copied unchanged to dst. The locale and
 *  multibyte characters are not taken into account (as native DLL).
 *  The number of character copied is the minimum of src->Length and
 *  the dst->MaximumLength.
 */
void WINAPI RtlUpperString( STRING *dst, const STRING *src )
{
    unsigned int i, len = min(src->Length, dst->MaximumLength);

    for (i = 0; i < len; i++) dst->Buffer[i] = RtlUpperChar(src->Buffer[i]);
    dst->Length = len;
}


/**************************************************************************
 *	RtlUpcaseUnicodeChar   (NTDLL.@)
 *
 * Converts a Unicode character to uppercase.
 *
 * PARAMS
 *  wch [I] Character to convert
 *
 * RETURNS
 *  The uppercase character value.
 */
WCHAR WINAPI RtlUpcaseUnicodeChar( WCHAR wch )
{
    return toupperW(wch);
}


/**************************************************************************
 *	RtlDowncaseUnicodeChar   (NTDLL.@)
 *
 * Converts a Unicode character to lowercase.
 *
 * PARAMS
 *  wch [I] Character to convert
 *
 * RETURNS
 *  The lowercase character value.
 */
WCHAR WINAPI RtlDowncaseUnicodeChar(WCHAR wch)
{
    return tolowerW(wch);
}


/**************************************************************************
 *	RtlUpcaseUnicodeString   (NTDLL.@)
 *
 * Converts a Unicode string to uppercase.
 *
 * PARAMS
 *  dest    [O] Destination for converted string
 *  src     [I] Source string to convert
 *  doalloc [I] TRUE=Allocate a buffer for dest if it doesn't have one
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. dest contains the converted string.
 *  Failure: STATUS_NO_MEMORY, if doalloc is TRUE and memory allocation fails, or
 *           STATUS_BUFFER_OVERFLOW, if doalloc is FALSE and dest is too small.
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
NTSTATUS WINAPI RtlUpcaseUnicodeString( UNICODE_STRING *dest,
                                        const UNICODE_STRING *src,
                                        BOOLEAN doalloc)
{
    DWORD i, len = src->Length;

    if (doalloc)
    {
        dest->MaximumLength = len;
        if (!(dest->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len )))
            return STATUS_NO_MEMORY;
    }
    else if (len > dest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < len/sizeof(WCHAR); i++) dest->Buffer[i] = toupperW(src->Buffer[i]);
    dest->Length = len;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlDowncaseUnicodeString   (NTDLL.@)
 *
 * Converts a Unicode string to lowercase.
 *
 * PARAMS
 *  dest    [O] Destination for converted string
 *  src     [I] Source string to convert
 *  doalloc [I] TRUE=Allocate a buffer for dest if it doesn't have one
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. dest contains the converted string.
 *  Failure: STATUS_NO_MEMORY, if doalloc is TRUE and memory allocation fails, or
 *           STATUS_BUFFER_OVERFLOW, if doalloc is FALSE and dest is too small.
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
NTSTATUS WINAPI RtlDowncaseUnicodeString(
    UNICODE_STRING *dest,
    const UNICODE_STRING *src,
    BOOLEAN doalloc)
{
    DWORD i;
    DWORD len = src->Length;

    if (doalloc) {
        dest->MaximumLength = len;
        if (!(dest->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len ))) {
            return STATUS_NO_MEMORY;
        } /* if */
    } else if (len > dest->MaximumLength) {
        return STATUS_BUFFER_OVERFLOW;
    } /* if */

    for (i = 0; i < len/sizeof(WCHAR); i++) {
        dest->Buffer[i] = tolowerW(src->Buffer[i]);
    } /* for */
    dest->Length = len;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUpcaseUnicodeStringToAnsiString   (NTDLL.@)
 *
 * Converts a Unicode string to the equivalent ANSI upper-case representation.
 *
 * RETURNS
 *  NTSTATUS code
 *
 * NOTES
 *  writes terminating 0
 */
NTSTATUS WINAPI RtlUpcaseUnicodeStringToAnsiString( STRING *dst,
                                                    const UNICODE_STRING *src,
                                                    BOOLEAN doalloc )
{
    NTSTATUS ret;
    UNICODE_STRING upcase;

    if (!(ret = RtlUpcaseUnicodeString( &upcase, src, TRUE )))
    {
        ret = RtlUnicodeStringToAnsiString( dst, &upcase, doalloc );
        RtlFreeUnicodeString( &upcase );
    }
    return ret;
}


/**************************************************************************
 *	RtlUpcaseUnicodeStringToOemString   (NTDLL.@)
 *
 * Converts a UNICODE_STRING to the equivalent OEM upper-case representation
 * stored in STRING format.
 *
 * RETURNS
 *  NTSTATUS code
 *
 * NOTES
 *  writes terminating 0
 */
NTSTATUS WINAPI RtlUpcaseUnicodeStringToOemString( STRING *dst,
                                                   const UNICODE_STRING *src,
                                                   BOOLEAN doalloc )
{
    NTSTATUS ret;
    UNICODE_STRING upcase;

    if (!(ret = RtlUpcaseUnicodeString( &upcase, src, TRUE )))
    {
        ret = RtlUnicodeStringToOemString( dst, &upcase, doalloc );
        RtlFreeUnicodeString( &upcase );
    }
    return ret;
}


/**************************************************************************
 *	RtlUpcaseUnicodeStringToCountedOemString   (NTDLL.@)
 *
 * Converts a UNICODE_STRING to the equivalent OEM upper-case representation
 * stored in STRING format.
 *
 * RETURNS
 *  NTSTATUS code
 *
 * NOTES
 *  Same as RtlUpcaseUnicodeStringToOemString but doesn't write terminating null
 */
NTSTATUS WINAPI RtlUpcaseUnicodeStringToCountedOemString( STRING *oem,
                                                          const UNICODE_STRING *uni,
                                                          BOOLEAN doalloc )
{
    NTSTATUS ret;
    UNICODE_STRING upcase;
    WCHAR tmp[32];

    upcase.Buffer = tmp;
    upcase.MaximumLength = sizeof(tmp);
    ret = RtlUpcaseUnicodeString( &upcase, uni, FALSE );
    if (ret == STATUS_BUFFER_OVERFLOW) ret = RtlUpcaseUnicodeString( &upcase, uni, TRUE );

    if (!ret)
    {
        DWORD len = RtlUnicodeStringToOemSize( &upcase ) - 1;
        oem->Length = len;
        if (doalloc)
        {
            oem->MaximumLength = len;
            if (!(oem->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len )))
            {
                ret = STATUS_NO_MEMORY;
                goto done;
            }
        }
        else if (oem->MaximumLength < len)
        {
            ret = STATUS_BUFFER_OVERFLOW;
            oem->Length = oem->MaximumLength;
            if (!oem->MaximumLength) goto done;
        }
        RtlUnicodeToOemN( oem->Buffer, oem->Length, NULL, upcase.Buffer, upcase.Length );
    done:
        if (upcase.Buffer != tmp) RtlFreeUnicodeString( &upcase );
    }
    return ret;
}


/**************************************************************************
 *	RtlUpcaseUnicodeToMultiByteN   (NTDLL.@)
 *
 * Converts a Unicode string to the equivalent ANSI upper-case representation.
 *
 * RETURNS
 *  NTSTATUS code
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToMultiByteN( LPSTR dst, DWORD dstlen, LPDWORD reslen,
                                              LPCWSTR src, DWORD srclen )
{
    NTSTATUS ret;
    LPWSTR upcase;
    DWORD i;

    if (!(upcase = RtlAllocateHeap( GetProcessHeap(), 0, srclen ))) return STATUS_NO_MEMORY;
    for (i = 0; i < srclen/sizeof(WCHAR); i++) upcase[i] = toupperW(src[i]);
    ret = RtlUnicodeToMultiByteN( dst, dstlen, reslen, upcase, srclen );
    RtlFreeHeap( GetProcessHeap(), 0, upcase );
    return ret;
}


/**************************************************************************
 *	RtlUpcaseUnicodeToOemN   (NTDLL.@)
 *
 * Converts a Unicode string to the equivalent OEM upper-case representation.
 *
 * RETURNS
 *  NTSTATUS code
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToOemN( LPSTR dst, DWORD dstlen, LPDWORD reslen,
                                        LPCWSTR src, DWORD srclen )
{
    NTSTATUS ret;
    LPWSTR upcase;
    DWORD i;

    if (!(upcase = RtlAllocateHeap( GetProcessHeap(), 0, srclen ))) return STATUS_NO_MEMORY;
    for (i = 0; i < srclen/sizeof(WCHAR); i++) upcase[i] = toupperW(src[i]);
    ret = RtlUnicodeToOemN( dst, dstlen, reslen, upcase, srclen );
    RtlFreeHeap( GetProcessHeap(), 0, upcase );
    return ret;
}


/*
        STRING SIZE
*/


/**************************************************************************
 *      RtlOemStringToUnicodeSize   (NTDLL.@)
 *      RtlxOemStringToUnicodeSize  (NTDLL.@)
 *
 * Calculate the size in bytes necessary for the Unicode conversion of str,
 * including the terminating '\0'.
 *
 * PARAMS
 *  str [I] String to calculate the size of
 *
 * RETURNS
 *  The calculated size.
 */
UINT WINAPI RtlOemStringToUnicodeSize( const STRING *str )
{
    int ret = wine_cp_mbstowcs( oem_table, 0, str->Buffer, str->Length, NULL, 0 );
    return (ret + 1) * sizeof(WCHAR);
}


/**************************************************************************
 *      RtlAnsiStringToUnicodeSize   (NTDLL.@)
 *      RtlxAnsiStringToUnicodeSize  (NTDLL.@)
 *
 * Calculate the size in bytes necessary for the Unicode conversion of str,
 * including the terminating '\0'.
 *
 * PARAMS
 *  str [I] String to calculate the size of
 *
 * RETURNS
 *  The calculated size.
 */
DWORD WINAPI RtlAnsiStringToUnicodeSize( const STRING *str )
{
    DWORD ret;
    RtlMultiByteToUnicodeSize( &ret, str->Buffer, str->Length );
    return ret + sizeof(WCHAR);
}


/**************************************************************************
 *      RtlMultiByteToUnicodeSize   (NTDLL.@)
 *
 * Compute the size in bytes necessary for the Unicode conversion of str,
 * without the terminating '\0'.
 *
 * PARAMS
 *  size [O] Destination for size
 *  str  [I] String to calculate the size of
 *  len  [I] Length of str
 *
 * RETURNS
 *  STATUS_SUCCESS.
 */
NTSTATUS WINAPI RtlMultiByteToUnicodeSize( DWORD *size, LPCSTR str, UINT len )
{
    *size = wine_cp_mbstowcs( ansi_table, 0, str, len, NULL, 0 ) * sizeof(WCHAR);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlUnicodeToMultiByteSize   (NTDLL.@)
 *
 * Calculate the size in bytes necessary for the multibyte conversion of str,
 * without the terminating '\0'.
 *
 * PARAMS
 *  size [O] Destination for size
 *  str  [I] String to calculate the size of
 *  len  [I] Length of str
 *
 * RETURNS
 *  STATUS_SUCCESS.
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteSize( PULONG size, LPCWSTR str, ULONG len )
{
    *size = wine_cp_wcstombs( ansi_table, 0, str, len / sizeof(WCHAR), NULL, 0, NULL, NULL );
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlUnicodeStringToAnsiSize   (NTDLL.@)
 *      RtlxUnicodeStringToAnsiSize  (NTDLL.@)
 *
 * Calculate the size in bytes necessary for the Ansi conversion of str,
 * including the terminating '\0'.
 *
 * PARAMS
 *  str [I] String to calculate the size of
 *
 * RETURNS
 *  The calculated size.
 */
DWORD WINAPI RtlUnicodeStringToAnsiSize( const UNICODE_STRING *str )
{
    DWORD ret;
    RtlUnicodeToMultiByteSize( &ret, str->Buffer, str->Length );
    return ret + 1;
}


/**************************************************************************
 *      RtlUnicodeStringToOemSize   (NTDLL.@)
 *      RtlxUnicodeStringToOemSize  (NTDLL.@)
 *
 * Calculate the size in bytes necessary for the OEM conversion of str,
 * including the terminating '\0'.
 *
 * PARAMS
 *  str [I] String to calculate the size of
 *
 * RETURNS
 *  The calculated size.
 */
DWORD WINAPI RtlUnicodeStringToOemSize( const UNICODE_STRING *str )
{
    return wine_cp_wcstombs( oem_table, 0, str->Buffer, str->Length / sizeof(WCHAR),
                             NULL, 0, NULL, NULL ) + 1;
}


/**************************************************************************
 *      RtlAppendAsciizToString   (NTDLL.@)
 *
 * Concatenates a buffered character string and a '\0' terminated character
 * string
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. src is appended to dest.
 *  Failure: STATUS_BUFFER_TOO_SMALL, if the buffer of dest is too small
 *                  to hold the concatenated string.
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is never '\0' terminated.
 */
NTSTATUS WINAPI RtlAppendAsciizToString(
    STRING *dest, /* [I/O] Buffered character string to which src is concatenated */
    LPCSTR src)   /* [I]   '\0' terminated character string to be concatenated */
{
    if (src != NULL) {
        unsigned int src_len = strlen(src);
        unsigned int dest_len  = src_len + dest->Length;

        if (dest_len > dest->MaximumLength) return STATUS_BUFFER_TOO_SMALL;
        memcpy(dest->Buffer + dest->Length, src, src_len);
        dest->Length = dest_len;
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlAppendStringToString   (NTDLL.@)
 *
 * Concatenates two buffered character strings
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. src is appended to dest.
 *  Failure: STATUS_BUFFER_TOO_SMALL, if the buffer of dest is too small
 *                  to hold the concatenated string.
 *
 * NOTES
 *  if src->length is zero dest is unchanged.
 *  dest is never '\0' terminated.
 */
NTSTATUS WINAPI RtlAppendStringToString(
    STRING *dest,       /* [I/O] Buffered character string to which src is concatenated */
    const STRING *src)  /* [I]   Buffered character string to be concatenated */
{
    if (src->Length != 0) {
        unsigned int dest_len = src->Length + dest->Length;

        if (dest_len > dest->MaximumLength) return STATUS_BUFFER_TOO_SMALL;
        memcpy(dest->Buffer + dest->Length, src->Buffer, src->Length);
        dest->Length = dest_len;
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlAppendUnicodeToString   (NTDLL.@)
 *
 * Concatenates a buffered unicode string and a '\0' terminated unicode 
 * string
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. src is appended to dest.
 *  Failure: STATUS_BUFFER_TOO_SMALL, if the buffer of dest is too small
 *                  to hold the concatenated string.
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is '\0' terminated when the MaximumLength allows it.
 *  When dest fits exactly in MaximumLength characters the '\0' is omitted.
 *
 * DIFFERENCES
 *  Does not write in the src->Buffer beyond MaximumLength when
 *  MaximumLength is odd as the native function does.
 */
NTSTATUS WINAPI RtlAppendUnicodeToString(
    UNICODE_STRING *dest, /* [I/O] Buffered unicode string to which src is concatenated */
    LPCWSTR src)          /* [I]   '\0' terminated unicode string to be concatenated */
{
    if (src != NULL) {
        unsigned int src_len = strlenW(src) * sizeof(WCHAR);
        unsigned int dest_len  = src_len + dest->Length;

        if (dest_len > dest->MaximumLength) return STATUS_BUFFER_TOO_SMALL;
        memcpy(dest->Buffer + dest->Length/sizeof(WCHAR), src, src_len);
        dest->Length = dest_len;
        /* append terminating '\0' if enough space */
        if (dest_len + sizeof(WCHAR) <= dest->MaximumLength) {
            dest->Buffer[dest_len / sizeof(WCHAR)] = 0;
        } /* if */
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlAppendUnicodeStringToString   (NTDLL.@)
 *
 * Concatenates two buffered unicode strings
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. src is appended to dest.
 *  Failure: STATUS_BUFFER_TOO_SMALL, if the buffer of dest is too small
 *                  to hold the concatenated string.
 *
 * NOTES
 *  if src->length is zero dest is unchanged.
 *  dest is '\0' terminated when the MaximumLength allows it.
 *  When dest fits exactly in MaximumLength characters the '\0' is omitted.
 *
 * DIFFERENCES
 *  Does not write in the src->Buffer beyond MaximumLength when
 *  MaximumLength is odd as the native function does.
 */
NTSTATUS WINAPI RtlAppendUnicodeStringToString(
    UNICODE_STRING *dest,      /* [I/O] Buffered unicode string to which src is concatenated */
    const UNICODE_STRING *src) /* [I]   Buffered unicode string to be concatenated */
{
    if (src->Length != 0) {
        unsigned int dest_len = src->Length + dest->Length;

        if (dest_len > dest->MaximumLength) return STATUS_BUFFER_TOO_SMALL;
        memcpy(dest->Buffer + dest->Length/sizeof(WCHAR), src->Buffer, src->Length);
        dest->Length = dest_len;
        /* append terminating '\0' if enough space */
        if (dest_len + sizeof(WCHAR) <= dest->MaximumLength) {
            dest->Buffer[dest_len / sizeof(WCHAR)] = 0;
        } /* if */
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlFindCharInUnicodeString   (NTDLL.@)
 *
 * Searches for one of several unicode characters in a unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. pos contains the position after the character found.
 *  Failure: STATUS_NOT_FOUND, if none of the search_chars are in main_str.
 */
NTSTATUS WINAPI RtlFindCharInUnicodeString(
    int flags,                          /* [I] Flags */
    const UNICODE_STRING *main_str,     /* [I] Unicode string in which one or more characters are searched */
    const UNICODE_STRING *search_chars, /* [I] Unicode string which contains the characters to search for */
    USHORT *pos)                        /* [O] Position of the first character found + 2 */
{
    unsigned int main_idx, search_idx;

    switch (flags) {
        case 0:
            for (main_idx = 0; main_idx < main_str->Length / sizeof(WCHAR); main_idx++) {
                for (search_idx = 0; search_idx < search_chars->Length / sizeof(WCHAR); search_idx++) {
                    if (main_str->Buffer[main_idx] == search_chars->Buffer[search_idx]) {
                        *pos = (main_idx + 1) * sizeof(WCHAR);
                        return STATUS_SUCCESS;
                    }
                }
            }
            *pos = 0;
            return STATUS_NOT_FOUND;
        case 1:
            main_idx = main_str->Length / sizeof(WCHAR);
            while (main_idx-- > 0) {
                for (search_idx = 0; search_idx < search_chars->Length / sizeof(WCHAR); search_idx++) {
                    if (main_str->Buffer[main_idx] == search_chars->Buffer[search_idx]) {
                        *pos = main_idx * sizeof(WCHAR);
                        return STATUS_SUCCESS;
                    }
                }
            }
            *pos = 0;
            return STATUS_NOT_FOUND;
        case 2:
            for (main_idx = 0; main_idx < main_str->Length / sizeof(WCHAR); main_idx++) {
                search_idx = 0;
                while (search_idx < search_chars->Length / sizeof(WCHAR) &&
                         main_str->Buffer[main_idx] != search_chars->Buffer[search_idx]) {
                    search_idx++;
                }
                if (search_idx >= search_chars->Length / sizeof(WCHAR)) {
                    *pos = (main_idx + 1) * sizeof(WCHAR);
                    return STATUS_SUCCESS;
                }
            }
            *pos = 0;
            return STATUS_NOT_FOUND;
        case 3:
            main_idx = main_str->Length / sizeof(WCHAR);
            while (main_idx-- > 0) {
                search_idx = 0;
                while (search_idx < search_chars->Length / sizeof(WCHAR) &&
                         main_str->Buffer[main_idx] != search_chars->Buffer[search_idx]) {
                    search_idx++;
                }
                if (search_idx >= search_chars->Length / sizeof(WCHAR)) {
                    *pos = main_idx * sizeof(WCHAR);
                    return STATUS_SUCCESS;
                }
            }
            *pos = 0;
            return STATUS_NOT_FOUND;
    } /* switch */
    return STATUS_NOT_FOUND;
}


/*
        MISC
*/

/**************************************************************************
 *	RtlIsTextUnicode (NTDLL.@)
 *
 * Attempt to guess whether a text buffer is Unicode.
 *
 * PARAMS
 *  buf [I] Text buffer to test
 *  len [I] Length of buf
 *  pf  [O] Destination for test results
 *
 * RETURNS
 *  TRUE if the buffer is likely Unicode, FALSE otherwise.
 *
 * FIXME
 *  Should implement more tests.
 */
BOOLEAN WINAPI RtlIsTextUnicode( LPCVOID buf, INT len, INT *pf )
{
    static const WCHAR std_control_chars[] = {'\r','\n','\t',' ',0x3000,0};
    static const WCHAR byterev_control_chars[] = {0x0d00,0x0a00,0x0900,0x2000,0};
    const WCHAR *s = buf;
    int i;
    unsigned int flags = ~0U, out_flags = 0;

    if (len < sizeof(WCHAR))
    {
        /* FIXME: MSDN documents IS_TEXT_UNICODE_BUFFER_TOO_SMALL but there is no such thing... */
        if (pf) *pf = 0;
        return FALSE;
    }
    if (pf)
        flags = *pf;
    /*
     * Apply various tests to the text string. According to the
     * docs, each test "passed" sets the corresponding flag in
     * the output flags. But some of the tests are mutually
     * exclusive, so I don't see how you could pass all tests ...
     */

    /* Check for an odd length ... pass if even. */
    if (len & 1) out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

    if (((const char *)buf)[len - 1] == 0)
        len--;  /* Windows seems to do something like that to avoid e.g. false IS_TEXT_UNICODE_NULL_BYTES  */

    len /= sizeof(WCHAR);
    /* Windows only checks the first 256 characters */
    if (len > 256) len = 256;

    /* Check for the special byte order unicode marks. */
    if (*s == 0xFEFF) out_flags |= IS_TEXT_UNICODE_SIGNATURE;
    if (*s == 0xFFFE) out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;

    /* apply some statistical analysis */
    if (flags & IS_TEXT_UNICODE_STATISTICS)
    {
        int stats = 0;
        /* FIXME: checks only for ASCII characters in the unicode stream */
        for (i = 0; i < len; i++)
        {
            if (s[i] <= 255) stats++;
        }
        if (stats > len / 2)
            out_flags |= IS_TEXT_UNICODE_STATISTICS;
    }

    /* Check for unicode NULL chars */
    if (flags & IS_TEXT_UNICODE_NULL_BYTES)
    {
        for (i = 0; i < len; i++)
        {
            if (!(s[i] & 0xff) || !(s[i] >> 8))
            {
                out_flags |= IS_TEXT_UNICODE_NULL_BYTES;
                break;
            }
        }
    }

    if (flags & IS_TEXT_UNICODE_CONTROLS)
    {
        for (i = 0; i < len; i++)
        {
            if (strchrW(std_control_chars, s[i]))
            {
                out_flags |= IS_TEXT_UNICODE_CONTROLS;
                break;
            }
        }
    }

    if (flags & IS_TEXT_UNICODE_REVERSE_CONTROLS)
    {
        for (i = 0; i < len; i++)
        {
            if (strchrW(byterev_control_chars, s[i]))
            {
                out_flags |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
                break;
            }
        }
    }

    if (pf)
    {
        out_flags &= *pf;
        *pf = out_flags;
    }
    /* check for flags that indicate it's definitely not valid Unicode */
    if (out_flags & (IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK)) return FALSE;
    /* now check for invalid ASCII, and assume Unicode if so */
    if (out_flags & IS_TEXT_UNICODE_NOT_ASCII_MASK) return TRUE;
    /* now check for Unicode flags */
    if (out_flags & IS_TEXT_UNICODE_UNICODE_MASK) return TRUE;
    /* no flags set */
    return FALSE;
}


/**************************************************************************
 *      RtlCharToInteger   (NTDLL.@)
 *
 * Converts a character string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 *  This function does not read garbage behind '\0' as the native version does.
 */
NTSTATUS WINAPI RtlCharToInteger(
    PCSZ str,      /* [I] '\0' terminated single-byte string containing a number */
    ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    ULONG *value)  /* [O] Destination for the converted value */
{
    CHAR chCurrent;
    int digit;
    ULONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (*str != '\0' && *str <= ' ') {
	str++;
    } /* while */

    if (*str == '+') {
	str++;
    } else if (*str == '-') {
        bMinus = TRUE;
	str++;
    } /* if */

    if (base == 0) {
	base = 10;
	if (str[0] == '0') {
	    if (str[1] == 'b') {
		str += 2;
		base = 2;
	    } else if (str[1] == 'o') {
		str += 2;
		base = 8;
	    } else if (str[1] == 'x') {
		str += 2;
		base = 16;
	    } /* if */
	} /* if */
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    if (value == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } /* if */

    while (*str != '\0') {
	chCurrent = *str;
	if (chCurrent >= '0' && chCurrent <= '9') {
	    digit = chCurrent - '0';
	} else if (chCurrent >= 'A' && chCurrent <= 'Z') {
	    digit = chCurrent - 'A' + 10;
	} else if (chCurrent >= 'a' && chCurrent <= 'z') {
	    digit = chCurrent - 'a' + 10;
	} else {
	    digit = -1;
	} /* if */
	if (digit < 0 || digit >= base) {
	    *value = bMinus ? -RunningTotal : RunningTotal;
	    return STATUS_SUCCESS;
	} /* if */

	RunningTotal = RunningTotal * base + digit;
	str++;
    } /* while */

    *value = bMinus ? -RunningTotal : RunningTotal;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlIntegerToChar   (NTDLL.@)
 *
 * Converts an unsigned integer to a character string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. str contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_BUFFER_OVERFLOW, if str would be larger than length.
 *           STATUS_ACCESS_VIOLATION, if str is NULL.
 *
 * NOTES
 *  Instead of base 0 it uses 10 as base.
 *  Writes at most length characters to the string str.
 *  Str is '\0' terminated when length allows it.
 *  When str fits exactly in length characters the '\0' is omitted.
 */
NTSTATUS WINAPI RtlIntegerToChar(
    ULONG value,   /* [I] Value to be converted */
    ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    ULONG length,  /* [I] Length of the str buffer in bytes */
    PCHAR str)     /* [O] Destination for the converted value */
{
    CHAR buffer[33];
    PCHAR pos;
    CHAR digit;
    ULONG len;

    if (base == 0) {
	base = 10;
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    pos = &buffer[32];
    *pos = '\0';

    do {
	pos--;
	digit = value % base;
	value = value / base;
	if (digit < 10) {
	    *pos = '0' + digit;
	} else {
	    *pos = 'A' + digit - 10;
	} /* if */
    } while (value != 0L);

    len = &buffer[32] - pos;
    if (len > length) {
	return STATUS_BUFFER_OVERFLOW;
    } else if (str == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } else if (len == length) {
	memcpy(str, pos, len);
    } else {
	memcpy(str, pos, len + 1);
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlUnicodeStringToInteger (NTDLL.@)
 *
 * Converts a unicode string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 *  This function does not read garbage on string length 0 as the native
 *  version does.
 */
NTSTATUS WINAPI RtlUnicodeStringToInteger(
    const UNICODE_STRING *str, /* [I] Unicode string to be converted */
    ULONG base,                /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    ULONG *value)              /* [O] Destination for the converted value */
{
    LPWSTR lpwstr = str->Buffer;
    USHORT CharsRemaining = str->Length / sizeof(WCHAR);
    WCHAR wchCurrent;
    int digit;
    ULONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (CharsRemaining >= 1 && *lpwstr <= ' ') {
	lpwstr++;
	CharsRemaining--;
    } /* while */

    if (CharsRemaining >= 1) {
	if (*lpwstr == '+') {
	    lpwstr++;
	    CharsRemaining--;
	} else if (*lpwstr == '-') {
            bMinus = TRUE;
	    lpwstr++;
	    CharsRemaining--;
	} /* if */
    } /* if */

    if (base == 0) {
	base = 10;
	if (CharsRemaining >= 2 && lpwstr[0] == '0') {
	    if (lpwstr[1] == 'b') {
		lpwstr += 2;
		CharsRemaining -= 2;
		base = 2;
	    } else if (lpwstr[1] == 'o') {
		lpwstr += 2;
		CharsRemaining -= 2;
		base = 8;
	    } else if (lpwstr[1] == 'x') {
		lpwstr += 2;
		CharsRemaining -= 2;
		base = 16;
	    } /* if */
	} /* if */
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    if (value == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } /* if */

    while (CharsRemaining >= 1) {
	wchCurrent = *lpwstr;
	if (wchCurrent >= '0' && wchCurrent <= '9') {
	    digit = wchCurrent - '0';
	} else if (wchCurrent >= 'A' && wchCurrent <= 'Z') {
	    digit = wchCurrent - 'A' + 10;
	} else if (wchCurrent >= 'a' && wchCurrent <= 'z') {
	    digit = wchCurrent - 'a' + 10;
	} else {
	    digit = -1;
	} /* if */
	if (digit < 0 || digit >= base) {
	    *value = bMinus ? -RunningTotal : RunningTotal;
	    return STATUS_SUCCESS;
	} /* if */

	RunningTotal = RunningTotal * base + digit;
	lpwstr++;
	CharsRemaining--;
    } /* while */

    *value = bMinus ? -RunningTotal : RunningTotal;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlIntegerToUnicodeString (NTDLL.@)
 *
 * Converts an unsigned integer to a '\0' terminated unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. str contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_BUFFER_OVERFLOW, if str is too small to hold the string
 *                  (with the '\0' termination). In this case str->Length
 *                  is set to the length, the string would have (which can
 *                  be larger than the MaximumLength).
 *
 * NOTES
 *  Instead of base 0 it uses 10 as base.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 *  Do not return STATUS_BUFFER_OVERFLOW when the string is long enough.
 *  The native function does this when the string would be longer than 16
 *  characters even when the string parameter is long enough.
 */
NTSTATUS WINAPI RtlIntegerToUnicodeString(
    ULONG value,         /* [I] Value to be converted */
    ULONG base,          /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    UNICODE_STRING *str) /* [O] Destination for the converted value */
{
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

    if (base == 0) {
	base = 10;
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    pos = &buffer[32];
    *pos = '\0';

    do {
	pos--;
	digit = value % base;
	value = value / base;
	if (digit < 10) {
	    *pos = '0' + digit;
	} else {
	    *pos = 'A' + digit - 10;
	} /* if */
    } while (value != 0L);

    str->Length = (&buffer[32] - pos) * sizeof(WCHAR);
    if (str->Length >= str->MaximumLength) {
	return STATUS_BUFFER_OVERFLOW;
    } else {
	memcpy(str->Buffer, pos, str->Length + sizeof(WCHAR));
    } /* if */
    return STATUS_SUCCESS;
}


/*************************************************************************
 * RtlGUIDFromString (NTDLL.@)
 *
 * Convert a string representation of a GUID into a GUID.
 *
 * PARAMS
 *  str  [I] String representation in the format "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
 *  guid [O] Destination for the converted GUID
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. guid contains the converted value.
 *  Failure: STATUS_INVALID_PARAMETER, if str is not in the expected format.
 *
 * SEE ALSO
 *  See RtlStringFromGUID.
 */
NTSTATUS WINAPI RtlGUIDFromString(PUNICODE_STRING str, GUID* guid)
{
  int i = 0;
  const WCHAR *lpszCLSID = str->Buffer;
  BYTE* lpOut = (BYTE*)guid;

  TRACE("(%s,%p)\n", debugstr_us(str), guid);

  /* Convert string: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
   * to memory:       DWORD... WORD WORD BYTES............
   */
  while (i <= 37)
  {
    switch (i)
    {
    case 0:
      if (*lpszCLSID != '{')
        return STATUS_INVALID_PARAMETER;
      break;

    case 9: case 14: case 19: case 24:
      if (*lpszCLSID != '-')
        return STATUS_INVALID_PARAMETER;
      break;

    case 37:
      if (*lpszCLSID != '}')
        return STATUS_INVALID_PARAMETER;
      break;

    default:
      {
        WCHAR ch = *lpszCLSID, ch2 = lpszCLSID[1];
        unsigned char byte;

        /* Read two hex digits as a byte value */
        if      (ch >= '0' && ch <= '9') ch = ch - '0';
        else if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') ch = ch - 'A' + 10;
        else return STATUS_INVALID_PARAMETER;

        if      (ch2 >= '0' && ch2 <= '9') ch2 = ch2 - '0';
        else if (ch2 >= 'a' && ch2 <= 'f') ch2 = ch2 - 'a' + 10;
        else if (ch2 >= 'A' && ch2 <= 'F') ch2 = ch2 - 'A' + 10;
        else return STATUS_INVALID_PARAMETER;

        byte = ch << 4 | ch2;

        switch (i)
        {
#ifndef WORDS_BIGENDIAN
        /* For Big Endian machines, we store the data such that the
         * dword/word members can be read as DWORDS and WORDS correctly. */
        /* Dword */
        case 1:  lpOut[3] = byte; break;
        case 3:  lpOut[2] = byte; break;
        case 5:  lpOut[1] = byte; break;
        case 7:  lpOut[0] = byte; lpOut += 4;  break;
        /* Word */
        case 10: case 15: lpOut[1] = byte; break;
        case 12: case 17: lpOut[0] = byte; lpOut += 2; break;
#endif
        /* Byte */
        default: lpOut[0] = byte; lpOut++; break;
        }
        lpszCLSID++; /* Skip 2nd character of byte */
        i++;
      }
    }
    lpszCLSID++;
    i++;
  }

  return STATUS_SUCCESS;
}

/*************************************************************************
 * RtlStringFromGUID (NTDLL.@)
 *
 * Convert a GUID into a string representation of a GUID.
 *
 * PARAMS
 *  guid [I] GUID to convert
 *  str  [O] Destination for the converted string
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. str contains the converted value.
 *  Failure: STATUS_NO_MEMORY, if memory for str cannot be allocated.
 *
 * SEE ALSO
 *  See RtlGUIDFromString.
 */
NTSTATUS WINAPI RtlStringFromGUID(const GUID* guid, UNICODE_STRING *str)
{
  static const WCHAR szFormat[] = { '{','%','0','8','l','X','-',
    '%','0','4','X','-',  '%','0','4','X','-','%','0','2','X','%','0','2','X',
    '-',   '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
    '%','0','2','X','%','0','2','X','}','\0' };

  TRACE("(%p,%p)\n", guid, str);

  str->Length = GUID_STRING_LENGTH * sizeof(WCHAR);
  str->MaximumLength = str->Length + sizeof(WCHAR);
  str->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, str->MaximumLength);
  if (!str->Buffer)
  {
    str->Length = str->MaximumLength = 0;
    return STATUS_NO_MEMORY;
  }
  sprintfW(str->Buffer, szFormat, guid->Data1, guid->Data2, guid->Data3,
          guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
          guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

  return STATUS_SUCCESS;
}

/******************************************************************************
 * RtlHashUnicodeString [NTDLL.@]
 */
NTSTATUS WINAPI RtlHashUnicodeString(PCUNICODE_STRING string, BOOLEAN case_insensitive, ULONG alg, ULONG *hash)
{
    unsigned int i;

    if (!string || !hash) return STATUS_INVALID_PARAMETER;

    switch (alg)
    {
    case HASH_STRING_ALGORITHM_DEFAULT:
    case HASH_STRING_ALGORITHM_X65599:
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    *hash = 0;
    for (i = 0; i < string->Length/sizeof(WCHAR); i++)
        *hash = *hash*65599 + (case_insensitive ? toupperW(string->Buffer[i]) : string->Buffer[i]);

    return STATUS_SUCCESS;
}
