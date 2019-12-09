/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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
#include "wine/port.h"

#include <assert.h>
#include <locale.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "winerror.h"
#include "winver.h"
#include "kernel_private.h"
#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

extern BOOL WINAPI Internal_EnumCalendarInfo( CALINFO_ENUMPROCW proc, LCID lcid, CALID id,
                                              CALTYPE type, BOOL unicode, BOOL ex,
                                              BOOL exex, LPARAM lparam );
extern BOOL WINAPI Internal_EnumDateFormats( DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags, BOOL unicode,
                                             BOOL ex, BOOL exex, LPARAM lparam );
extern BOOL WINAPI Internal_EnumLanguageGroupLocales( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                      DWORD flags, LONG_PTR param, BOOL unicode );
extern BOOL WINAPI Internal_EnumSystemCodePages( CODEPAGE_ENUMPROCW proc, DWORD flags, BOOL unicode );
extern BOOL WINAPI Internal_EnumSystemLanguageGroups( LANGUAGEGROUP_ENUMPROCW proc, DWORD flags,
                                                      LONG_PTR param, BOOL unicode );
extern BOOL WINAPI Internal_EnumTimeFormats( TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags,
                                             BOOL unicode, BOOL ex, LPARAM lparam );
extern BOOL WINAPI Internal_EnumUILanguages( UILANGUAGE_ENUMPROCW proc, DWORD flags,
                                             LONG_PTR param, BOOL unicode );

extern const unsigned short nameprep_char_type[] DECLSPEC_HIDDEN;
extern const WCHAR nameprep_mapping[] DECLSPEC_HIDDEN;

static inline unsigned short get_table_entry( const unsigned short *table, WCHAR ch )
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

/***********************************************************************
 *		get_lcid_codepage
 *
 * Retrieve the ANSI codepage for a given locale.
 */
static inline UINT get_lcid_codepage( LCID lcid )
{
    UINT ret;
    if (!GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER, (WCHAR *)&ret,
                         sizeof(ret)/sizeof(WCHAR) )) ret = 0;
    return ret;
}


/***********************************************************************
 *		create_registry_key
 *
 * Create the Control Panel\\International registry key.
 */
static inline HANDLE create_registry_key(void)
{
    static const WCHAR cplW[] = {'C','o','n','t','r','o','l',' ','P','a','n','e','l',0};
    static const WCHAR intlW[] = {'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE cpl_key, hkey = 0;

    if (RtlOpenCurrentUser( KEY_ALL_ACCESS, &hkey ) != STATUS_SUCCESS) return 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, cplW );

    if (!NtCreateKey( &cpl_key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ))
    {
        NtClose( attr.RootDirectory );
        attr.RootDirectory = cpl_key;
        RtlInitUnicodeString( &nameW, intlW );
        if (NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) hkey = 0;
    }
    NtClose( attr.RootDirectory );
    return hkey;
}


static BOOL get_dummy_preferred_ui_language( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    LCTYPE type;
    int lsize;

    FIXME("(0x%x %p %p %p) returning a dummy value (current locale)\n", flags, count, buffer, size);

    if (flags & MUI_LANGUAGE_ID)
        type = LOCALE_ILANGUAGE;
    else
        type = LOCALE_SNAME;

    lsize = GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, type, NULL, 0);
    if (!lsize)
    {
        /* keep last error from callee */
        return FALSE;
    }
    lsize++;
    if (!*size)
    {
        *size = lsize;
        *count = 1;
        return TRUE;
    }

    if (lsize > *size)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, type, buffer, *size))
    {
        /* keep last error from callee */
        return FALSE;
    }

    buffer[lsize-1] = 0;
    *size = lsize;
    *count = 1;
    TRACE("returned variable content: %d, \"%s\", %d\n", *count, debugstr_w(buffer), *size);
    return TRUE;

}

/***********************************************************************
 *             GetProcessPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetProcessPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buf, ULONG *size )
{
    FIXME( "%08x, %p, %p %p\n", flags, count, buf, size );
    return get_dummy_preferred_ui_language( flags, count, buf, size );
}

/***********************************************************************
 *             GetSystemPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetSystemPreferredUILanguages(DWORD flags, ULONG* count, WCHAR* buffer, ULONG* size)
{
    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID | MUI_MACHINE_LANGUAGE_SETTINGS))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (*size && !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return get_dummy_preferred_ui_language( flags, count, buffer, size );
}

/***********************************************************************
 *              SetProcessPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI SetProcessPreferredUILanguages( DWORD flags, PCZZWSTR buffer, PULONG count )
{
    FIXME("%u, %p, %p\n", flags, buffer, count );
    return TRUE;
}

/***********************************************************************
 *              SetThreadPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI SetThreadPreferredUILanguages( DWORD flags, PCZZWSTR buffer, PULONG count )
{
    FIXME( "%u, %p, %p\n", flags, buffer, count );
    return TRUE;
}

/***********************************************************************
 *              GetThreadPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetThreadPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buf, ULONG *size )
{
    FIXME( "%08x, %p, %p %p\n", flags, count, buf, size );
    return get_dummy_preferred_ui_language( flags, count, buf, size );
}

/******************************************************************************
 *             GetUserPreferredUILanguages (KERNEL32.@)
 */
BOOL WINAPI GetUserPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    TRACE( "%u %p %p %p\n", flags, count, buffer, size );

    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (*size && !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return get_dummy_preferred_ui_language( flags, count, buffer, size );
}

/******************************************************************************
 *		SetLocaleInfoA	[KERNEL32.@]
 *
 * Set information about an aspect of a locale.
 *
 * PARAMS
 *  lcid   [I] LCID of the locale
 *  lctype [I] LCTYPE_ flags from "winnls.h"
 *  data   [I] Information to set
 *
 * RETURNS
 *  Success: TRUE. The information given will be returned by GetLocaleInfoA()
 *           whenever it is called without LOCALE_NOUSEROVERRIDE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  - Values are only be set for the current user locale; the system locale
 *  settings cannot be changed.
 *  - Any settings changed by this call are lost when the locale is changed by
 *  the control panel (in Wine, this happens every time you change LANG).
 *  - The native implementation of this function does not check that lcid matches
 *  the current user locale, and simply sets the new values. Wine warns you in
 *  this case, but behaves the same.
 */
BOOL WINAPI SetLocaleInfoA(LCID lcid, LCTYPE lctype, LPCSTR data)
{
    UINT codepage = CP_ACP;
    WCHAR *strW;
    DWORD len;
    BOOL ret;

    if (!(lctype & LOCALE_USE_CP_ACP)) codepage = get_lcid_codepage( lcid );

    if (!data)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    len = MultiByteToWideChar( codepage, 0, data, -1, NULL, 0 );
    if (!(strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    MultiByteToWideChar( codepage, 0, data, -1, strW, len );
    ret = SetLocaleInfoW( lcid, lctype, strW );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/******************************************************************************
 *              SetCPGlobal   (KERNEL32.@)
 *
 * Set the current Ansi code page Id for the system.
 *
 * PARAMS
 *    acp [I] code page ID to be the new ACP.
 *
 * RETURNS
 *    The previous ACP.
 */
UINT WINAPI SetCPGlobal( UINT acp )
{
    FIXME( "not supported\n" );
    return GetACP();
}


/***********************************************************************
 *           GetCPInfoExA   (KERNEL32.@)
 *
 * Get extended information about a code page.
 *
 * PARAMS
 *  codepage [I] Code page number
 *  dwFlags  [I] Reserved, must to 0.
 *  cpinfo   [O] Destination for code page information
 *
 * RETURNS
 *  Success: TRUE. cpinfo is updated with the information about codepage.
 *  Failure: FALSE, if codepage is invalid or cpinfo is NULL.
 */
BOOL WINAPI GetCPInfoExA( UINT codepage, DWORD dwFlags, LPCPINFOEXA cpinfo )
{
    CPINFOEXW cpinfoW;

    if (!GetCPInfoExW( codepage, dwFlags, &cpinfoW ))
      return FALSE;

    /* the layout is the same except for CodePageName */
    memcpy(cpinfo, &cpinfoW, sizeof(CPINFOEXA));
    WideCharToMultiByte(CP_ACP, 0, cpinfoW.CodePageName, -1, cpinfo->CodePageName, sizeof(cpinfo->CodePageName), NULL, NULL);
    return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePagesA   (KERNEL32.@)
 */
BOOL WINAPI EnumSystemCodePagesA( CODEPAGE_ENUMPROCA proc, DWORD flags )
{
    return Internal_EnumSystemCodePages( (CODEPAGE_ENUMPROCW)proc, flags, FALSE );
}


/******************************************************************************
 *           GetStringTypeExA    (KERNEL32.@)
 *
 * Get characteristics of the characters making up a string.
 *
 * PARAMS
 *  locale   [I] Locale Id for the string
 *  type     [I] CT_CTYPE1 = classification, CT_CTYPE2 = directionality, CT_CTYPE3 = typographic info
 *  src      [I] String to analyse
 *  count    [I] Length of src in chars, or -1 if src is NUL terminated
 *  chartype [O] Destination for the calculated characteristics
 *
 * RETURNS
 *  Success: TRUE. chartype is filled with the requested characteristics of each char
 *           in src.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI GetStringTypeExA( LCID locale, DWORD type, LPCSTR src, INT count, LPWORD chartype )
{
    return GetStringTypeA(locale, type, src, count, chartype);
}

/*************************************************************************
 *           FoldStringA    (KERNEL32.@)
 *
 * Map characters in a string.
 *
 * PARAMS
 *  dwFlags [I] Flags controlling chars to map (MAP_ constants from "winnls.h")
 *  src     [I] String to map
 *  srclen  [I] Length of src, or -1 if src is NUL terminated
 *  dst     [O] Destination for mapped string
 *  dstlen  [I] Length of dst, or 0 to find the required length for the mapped string
 *
 * RETURNS
 *  Success: The length of the string written to dst, including the terminating NUL. If
 *           dstlen is 0, the value returned is the same, but nothing is written to dst,
 *           and dst may be NULL.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI FoldStringA(DWORD dwFlags, LPCSTR src, INT srclen,
                       LPSTR dst, INT dstlen)
{
    INT ret = 0, srclenW = 0;
    WCHAR *srcW = NULL, *dstW = NULL;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    srclenW = MultiByteToWideChar(CP_ACP, 0, src, srclen, NULL, 0);
    srcW = HeapAlloc(GetProcessHeap(), 0, srclenW * sizeof(WCHAR));

    if (!srcW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto FoldStringA_exit;
    }

    MultiByteToWideChar(CP_ACP, 0, src, srclen, srcW, srclenW);

    ret = FoldStringW(dwFlags, srcW, srclenW, NULL, 0);
    if (ret && dstlen)
    {
        dstW = HeapAlloc(GetProcessHeap(), 0, ret * sizeof(WCHAR));

        if (!dstW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto FoldStringA_exit;
        }

        ret = FoldStringW(dwFlags, srcW, srclenW, dstW, ret);
        if (!WideCharToMultiByte(CP_ACP, 0, dstW, ret, dst, dstlen, NULL, NULL))
        {
            ret = 0;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    HeapFree(GetProcessHeap(), 0, dstW);

FoldStringA_exit:
    HeapFree(GetProcessHeap(), 0, srcW);
    return ret;
}

static HANDLE NLS_RegOpenKey(HANDLE hRootKey, LPCWSTR szKeyName)
{
    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES attr;
    HANDLE hkey;

    RtlInitUnicodeString( &keyName, szKeyName );
    InitializeObjectAttributes(&attr, &keyName, 0, hRootKey, NULL);

    if (NtOpenKey( &hkey, KEY_READ, &attr ) != STATUS_SUCCESS)
        hkey = 0;

    return hkey;
}

/******************************************************************************
 *           EnumSystemLanguageGroupsA    (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLanguageGroupsA( LANGUAGEGROUP_ENUMPROCA proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumSystemLanguageGroups( (LANGUAGEGROUP_ENUMPROCW)proc, flags, param, FALSE );
}

/******************************************************************************
 *           EnumLanguageGroupLocalesA    (KERNEL32.@)
 */
BOOL WINAPI EnumLanguageGroupLocalesA( LANGGROUPLOCALE_ENUMPROCA proc, LGRPID id,
                                       DWORD flags, LONG_PTR param )
{
    return Internal_EnumLanguageGroupLocales( (LANGGROUPLOCALE_ENUMPROCW)proc, id, flags, param, FALSE );
}

/******************************************************************************
 *		EnumCalendarInfoA	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoA( CALINFO_ENUMPROCA proc, LCID lcid, CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, lcid, id, type, FALSE, FALSE, FALSE, 0 );
}

/******************************************************************************
 *		EnumCalendarInfoExA	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoExA( CALINFO_ENUMPROCEXA proc, LCID lcid, CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, lcid, id, type, FALSE, TRUE, FALSE, 0 );
}

/**************************************************************************
 *              EnumDateFormatsExA    (KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsExA(DATEFMT_ENUMPROCEXA proc, LCID lcid, DWORD flags)
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, lcid, flags, FALSE, TRUE, FALSE, 0 );
}

/**************************************************************************
 *              EnumDateFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsA(DATEFMT_ENUMPROCA proc, LCID lcid, DWORD flags)
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, lcid, flags, FALSE, FALSE, FALSE, 0 );
}

/**************************************************************************
 *              EnumTimeFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumTimeFormatsA( TIMEFMT_ENUMPROCA proc, LCID lcid, DWORD flags )
{
    /* EnumTimeFormatsA doesn't support flags, EnumTimeFormatsW does. */
    if (flags & ~LOCALE_USE_CP_ACP)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }
    return Internal_EnumTimeFormats( (TIMEFMT_ENUMPROCW)proc, lcid, flags, FALSE, FALSE, 0 );
}

/******************************************************************************
 *           InvalidateNLSCache           (KERNEL32.@)
 *
 * Invalidate the cache of NLS values.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI InvalidateNLSCache(void)
{
  FIXME("() stub\n");
  return FALSE;
}

/******************************************************************************
 *           EnumUILanguagesA (KERNEL32.@)
 */
BOOL WINAPI EnumUILanguagesA( UILANGUAGE_ENUMPROCA proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumUILanguages( (UILANGUAGE_ENUMPROCW)proc, flags, param, FALSE );
}


enum locationkind {
    LOCATION_NATION = 0,
    LOCATION_REGION,
    LOCATION_BOTH
};

struct geoinfo_t {
    GEOID id;
    WCHAR iso2W[3];
    WCHAR iso3W[4];
    GEOID parent;
    INT   uncode;
    enum locationkind kind;
};

static const struct geoinfo_t geoinfodata[] = {
    { 2, {'A','G',0}, {'A','T','G',0}, 10039880,  28 }, /* Antigua and Barbuda */
    { 3, {'A','F',0}, {'A','F','G',0}, 47614,   4 }, /* Afghanistan */
    { 4, {'D','Z',0}, {'D','Z','A',0}, 42487,  12 }, /* Algeria */
    { 5, {'A','Z',0}, {'A','Z','E',0}, 47611,  31 }, /* Azerbaijan */
    { 6, {'A','L',0}, {'A','L','B',0}, 47610,   8 }, /* Albania */
    { 7, {'A','M',0}, {'A','R','M',0}, 47611,  51 }, /* Armenia */
    { 8, {'A','D',0}, {'A','N','D',0}, 47610,  20 }, /* Andorra */
    { 9, {'A','O',0}, {'A','G','O',0}, 42484,  24 }, /* Angola */
    { 10, {'A','S',0}, {'A','S','M',0}, 26286,  16 }, /* American Samoa */
    { 11, {'A','R',0}, {'A','R','G',0}, 31396,  32 }, /* Argentina */
    { 12, {'A','U',0}, {'A','U','S',0}, 10210825,  36 }, /* Australia */
    { 14, {'A','T',0}, {'A','U','T',0}, 10210824,  40 }, /* Austria */
    { 17, {'B','H',0}, {'B','H','R',0}, 47611,  48 }, /* Bahrain */
    { 18, {'B','B',0}, {'B','R','B',0}, 10039880,  52 }, /* Barbados */
    { 19, {'B','W',0}, {'B','W','A',0}, 10039883,  72 }, /* Botswana */
    { 20, {'B','M',0}, {'B','M','U',0}, 23581,  60 }, /* Bermuda */
    { 21, {'B','E',0}, {'B','E','L',0}, 10210824,  56 }, /* Belgium */
    { 22, {'B','S',0}, {'B','H','S',0}, 10039880,  44 }, /* Bahamas, The */
    { 23, {'B','D',0}, {'B','G','D',0}, 47614,  50 }, /* Bangladesh */
    { 24, {'B','Z',0}, {'B','L','Z',0}, 27082,  84 }, /* Belize */
    { 25, {'B','A',0}, {'B','I','H',0}, 47610,  70 }, /* Bosnia and Herzegovina */
    { 26, {'B','O',0}, {'B','O','L',0}, 31396,  68 }, /* Bolivia */
    { 27, {'M','M',0}, {'M','M','R',0}, 47599, 104 }, /* Myanmar */
    { 28, {'B','J',0}, {'B','E','N',0}, 42483, 204 }, /* Benin */
    { 29, {'B','Y',0}, {'B','L','R',0}, 47609, 112 }, /* Belarus */
    { 30, {'S','B',0}, {'S','L','B',0}, 20900,  90 }, /* Solomon Islands */
    { 32, {'B','R',0}, {'B','R','A',0}, 31396,  76 }, /* Brazil */
    { 34, {'B','T',0}, {'B','T','N',0}, 47614,  64 }, /* Bhutan */
    { 35, {'B','G',0}, {'B','G','R',0}, 47609, 100 }, /* Bulgaria */
    { 37, {'B','N',0}, {'B','R','N',0}, 47599,  96 }, /* Brunei */
    { 38, {'B','I',0}, {'B','D','I',0}, 47603, 108 }, /* Burundi */
    { 39, {'C','A',0}, {'C','A','N',0}, 23581, 124 }, /* Canada */
    { 40, {'K','H',0}, {'K','H','M',0}, 47599, 116 }, /* Cambodia */
    { 41, {'T','D',0}, {'T','C','D',0}, 42484, 148 }, /* Chad */
    { 42, {'L','K',0}, {'L','K','A',0}, 47614, 144 }, /* Sri Lanka */
    { 43, {'C','G',0}, {'C','O','G',0}, 42484, 178 }, /* Congo */
    { 44, {'C','D',0}, {'C','O','D',0}, 42484, 180 }, /* Congo (DRC) */
    { 45, {'C','N',0}, {'C','H','N',0}, 47600, 156 }, /* China */
    { 46, {'C','L',0}, {'C','H','L',0}, 31396, 152 }, /* Chile */
    { 49, {'C','M',0}, {'C','M','R',0}, 42484, 120 }, /* Cameroon */
    { 50, {'K','M',0}, {'C','O','M',0}, 47603, 174 }, /* Comoros */
    { 51, {'C','O',0}, {'C','O','L',0}, 31396, 170 }, /* Colombia */
    { 54, {'C','R',0}, {'C','R','I',0}, 27082, 188 }, /* Costa Rica */
    { 55, {'C','F',0}, {'C','A','F',0}, 42484, 140 }, /* Central African Republic */
    { 56, {'C','U',0}, {'C','U','B',0}, 10039880, 192 }, /* Cuba */
    { 57, {'C','V',0}, {'C','P','V',0}, 42483, 132 }, /* Cape Verde */
    { 59, {'C','Y',0}, {'C','Y','P',0}, 47611, 196 }, /* Cyprus */
    { 61, {'D','K',0}, {'D','N','K',0}, 10039882, 208 }, /* Denmark */
    { 62, {'D','J',0}, {'D','J','I',0}, 47603, 262 }, /* Djibouti */
    { 63, {'D','M',0}, {'D','M','A',0}, 10039880, 212 }, /* Dominica */
    { 65, {'D','O',0}, {'D','O','M',0}, 10039880, 214 }, /* Dominican Republic */
    { 66, {'E','C',0}, {'E','C','U',0}, 31396, 218 }, /* Ecuador */
    { 67, {'E','G',0}, {'E','G','Y',0}, 42487, 818 }, /* Egypt */
    { 68, {'I','E',0}, {'I','R','L',0}, 10039882, 372 }, /* Ireland */
    { 69, {'G','Q',0}, {'G','N','Q',0}, 42484, 226 }, /* Equatorial Guinea */
    { 70, {'E','E',0}, {'E','S','T',0}, 10039882, 233 }, /* Estonia */
    { 71, {'E','R',0}, {'E','R','I',0}, 47603, 232 }, /* Eritrea */
    { 72, {'S','V',0}, {'S','L','V',0}, 27082, 222 }, /* El Salvador */
    { 73, {'E','T',0}, {'E','T','H',0}, 47603, 231 }, /* Ethiopia */
    { 75, {'C','Z',0}, {'C','Z','E',0}, 47609, 203 }, /* Czech Republic */
    { 77, {'F','I',0}, {'F','I','N',0}, 10039882, 246 }, /* Finland */
    { 78, {'F','J',0}, {'F','J','I',0}, 20900, 242 }, /* Fiji Islands */
    { 80, {'F','M',0}, {'F','S','M',0}, 21206, 583 }, /* Micronesia */
    { 81, {'F','O',0}, {'F','R','O',0}, 10039882, 234 }, /* Faroe Islands */
    { 84, {'F','R',0}, {'F','R','A',0}, 10210824, 250 }, /* France */
    { 86, {'G','M',0}, {'G','M','B',0}, 42483, 270 }, /* Gambia, The */
    { 87, {'G','A',0}, {'G','A','B',0}, 42484, 266 }, /* Gabon */
    { 88, {'G','E',0}, {'G','E','O',0}, 47611, 268 }, /* Georgia */
    { 89, {'G','H',0}, {'G','H','A',0}, 42483, 288 }, /* Ghana */
    { 90, {'G','I',0}, {'G','I','B',0}, 47610, 292 }, /* Gibraltar */
    { 91, {'G','D',0}, {'G','R','D',0}, 10039880, 308 }, /* Grenada */
    { 93, {'G','L',0}, {'G','R','L',0}, 23581, 304 }, /* Greenland */
    { 94, {'D','E',0}, {'D','E','U',0}, 10210824, 276 }, /* Germany */
    { 98, {'G','R',0}, {'G','R','C',0}, 47610, 300 }, /* Greece */
    { 99, {'G','T',0}, {'G','T','M',0}, 27082, 320 }, /* Guatemala */
    { 100, {'G','N',0}, {'G','I','N',0}, 42483, 324 }, /* Guinea */
    { 101, {'G','Y',0}, {'G','U','Y',0}, 31396, 328 }, /* Guyana */
    { 103, {'H','T',0}, {'H','T','I',0}, 10039880, 332 }, /* Haiti */
    { 104, {'H','K',0}, {'H','K','G',0}, 47600, 344 }, /* Hong Kong S.A.R. */
    { 106, {'H','N',0}, {'H','N','D',0}, 27082, 340 }, /* Honduras */
    { 108, {'H','R',0}, {'H','R','V',0}, 47610, 191 }, /* Croatia */
    { 109, {'H','U',0}, {'H','U','N',0}, 47609, 348 }, /* Hungary */
    { 110, {'I','S',0}, {'I','S','L',0}, 10039882, 352 }, /* Iceland */
    { 111, {'I','D',0}, {'I','D','N',0}, 47599, 360 }, /* Indonesia */
    { 113, {'I','N',0}, {'I','N','D',0}, 47614, 356 }, /* India */
    { 114, {'I','O',0}, {'I','O','T',0}, 39070,  86 }, /* British Indian Ocean Territory */
    { 116, {'I','R',0}, {'I','R','N',0}, 47614, 364 }, /* Iran */
    { 117, {'I','L',0}, {'I','S','R',0}, 47611, 376 }, /* Israel */
    { 118, {'I','T',0}, {'I','T','A',0}, 47610, 380 }, /* Italy */
    { 119, {'C','I',0}, {'C','I','V',0}, 42483, 384 }, /* Côte d'Ivoire */
    { 121, {'I','Q',0}, {'I','R','Q',0}, 47611, 368 }, /* Iraq */
    { 122, {'J','P',0}, {'J','P','N',0}, 47600, 392 }, /* Japan */
    { 124, {'J','M',0}, {'J','A','M',0}, 10039880, 388 }, /* Jamaica */
    { 125, {'S','J',0}, {'S','J','M',0}, 10039882, 744 }, /* Jan Mayen */
    { 126, {'J','O',0}, {'J','O','R',0}, 47611, 400 }, /* Jordan */
    { 127, {'X','X',0}, {'X','X',0}, 161832256 }, /* Johnston Atoll */
    { 129, {'K','E',0}, {'K','E','N',0}, 47603, 404 }, /* Kenya */
    { 130, {'K','G',0}, {'K','G','Z',0}, 47590, 417 }, /* Kyrgyzstan */
    { 131, {'K','P',0}, {'P','R','K',0}, 47600, 408 }, /* North Korea */
    { 133, {'K','I',0}, {'K','I','R',0}, 21206, 296 }, /* Kiribati */
    { 134, {'K','R',0}, {'K','O','R',0}, 47600, 410 }, /* Korea */
    { 136, {'K','W',0}, {'K','W','T',0}, 47611, 414 }, /* Kuwait */
    { 137, {'K','Z',0}, {'K','A','Z',0}, 47590, 398 }, /* Kazakhstan */
    { 138, {'L','A',0}, {'L','A','O',0}, 47599, 418 }, /* Laos */
    { 139, {'L','B',0}, {'L','B','N',0}, 47611, 422 }, /* Lebanon */
    { 140, {'L','V',0}, {'L','V','A',0}, 10039882, 428 }, /* Latvia */
    { 141, {'L','T',0}, {'L','T','U',0}, 10039882, 440 }, /* Lithuania */
    { 142, {'L','R',0}, {'L','B','R',0}, 42483, 430 }, /* Liberia */
    { 143, {'S','K',0}, {'S','V','K',0}, 47609, 703 }, /* Slovakia */
    { 145, {'L','I',0}, {'L','I','E',0}, 10210824, 438 }, /* Liechtenstein */
    { 146, {'L','S',0}, {'L','S','O',0}, 10039883, 426 }, /* Lesotho */
    { 147, {'L','U',0}, {'L','U','X',0}, 10210824, 442 }, /* Luxembourg */
    { 148, {'L','Y',0}, {'L','B','Y',0}, 42487, 434 }, /* Libya */
    { 149, {'M','G',0}, {'M','D','G',0}, 47603, 450 }, /* Madagascar */
    { 151, {'M','O',0}, {'M','A','C',0}, 47600, 446 }, /* Macao S.A.R. */
    { 152, {'M','D',0}, {'M','D','A',0}, 47609, 498 }, /* Moldova */
    { 154, {'M','N',0}, {'M','N','G',0}, 47600, 496 }, /* Mongolia */
    { 156, {'M','W',0}, {'M','W','I',0}, 47603, 454 }, /* Malawi */
    { 157, {'M','L',0}, {'M','L','I',0}, 42483, 466 }, /* Mali */
    { 158, {'M','C',0}, {'M','C','O',0}, 10210824, 492 }, /* Monaco */
    { 159, {'M','A',0}, {'M','A','R',0}, 42487, 504 }, /* Morocco */
    { 160, {'M','U',0}, {'M','U','S',0}, 47603, 480 }, /* Mauritius */
    { 162, {'M','R',0}, {'M','R','T',0}, 42483, 478 }, /* Mauritania */
    { 163, {'M','T',0}, {'M','L','T',0}, 47610, 470 }, /* Malta */
    { 164, {'O','M',0}, {'O','M','N',0}, 47611, 512 }, /* Oman */
    { 165, {'M','V',0}, {'M','D','V',0}, 47614, 462 }, /* Maldives */
    { 166, {'M','X',0}, {'M','E','X',0}, 27082, 484 }, /* Mexico */
    { 167, {'M','Y',0}, {'M','Y','S',0}, 47599, 458 }, /* Malaysia */
    { 168, {'M','Z',0}, {'M','O','Z',0}, 47603, 508 }, /* Mozambique */
    { 173, {'N','E',0}, {'N','E','R',0}, 42483, 562 }, /* Niger */
    { 174, {'V','U',0}, {'V','U','T',0}, 20900, 548 }, /* Vanuatu */
    { 175, {'N','G',0}, {'N','G','A',0}, 42483, 566 }, /* Nigeria */
    { 176, {'N','L',0}, {'N','L','D',0}, 10210824, 528 }, /* Netherlands */
    { 177, {'N','O',0}, {'N','O','R',0}, 10039882, 578 }, /* Norway */
    { 178, {'N','P',0}, {'N','P','L',0}, 47614, 524 }, /* Nepal */
    { 180, {'N','R',0}, {'N','R','U',0}, 21206, 520 }, /* Nauru */
    { 181, {'S','R',0}, {'S','U','R',0}, 31396, 740 }, /* Suriname */
    { 182, {'N','I',0}, {'N','I','C',0}, 27082, 558 }, /* Nicaragua */
    { 183, {'N','Z',0}, {'N','Z','L',0}, 10210825, 554 }, /* New Zealand */
    { 184, {'P','S',0}, {'P','S','E',0}, 47611, 275 }, /* Palestinian Authority */
    { 185, {'P','Y',0}, {'P','R','Y',0}, 31396, 600 }, /* Paraguay */
    { 187, {'P','E',0}, {'P','E','R',0}, 31396, 604 }, /* Peru */
    { 190, {'P','K',0}, {'P','A','K',0}, 47614, 586 }, /* Pakistan */
    { 191, {'P','L',0}, {'P','O','L',0}, 47609, 616 }, /* Poland */
    { 192, {'P','A',0}, {'P','A','N',0}, 27082, 591 }, /* Panama */
    { 193, {'P','T',0}, {'P','R','T',0}, 47610, 620 }, /* Portugal */
    { 194, {'P','G',0}, {'P','N','G',0}, 20900, 598 }, /* Papua New Guinea */
    { 195, {'P','W',0}, {'P','L','W',0}, 21206, 585 }, /* Palau */
    { 196, {'G','W',0}, {'G','N','B',0}, 42483, 624 }, /* Guinea-Bissau */
    { 197, {'Q','A',0}, {'Q','A','T',0}, 47611, 634 }, /* Qatar */
    { 198, {'R','E',0}, {'R','E','U',0}, 47603, 638 }, /* Reunion */
    { 199, {'M','H',0}, {'M','H','L',0}, 21206, 584 }, /* Marshall Islands */
    { 200, {'R','O',0}, {'R','O','U',0}, 47609, 642 }, /* Romania */
    { 201, {'P','H',0}, {'P','H','L',0}, 47599, 608 }, /* Philippines */
    { 202, {'P','R',0}, {'P','R','I',0}, 10039880, 630 }, /* Puerto Rico */
    { 203, {'R','U',0}, {'R','U','S',0}, 47609, 643 }, /* Russia */
    { 204, {'R','W',0}, {'R','W','A',0}, 47603, 646 }, /* Rwanda */
    { 205, {'S','A',0}, {'S','A','U',0}, 47611, 682 }, /* Saudi Arabia */
    { 206, {'P','M',0}, {'S','P','M',0}, 23581, 666 }, /* St. Pierre and Miquelon */
    { 207, {'K','N',0}, {'K','N','A',0}, 10039880, 659 }, /* St. Kitts and Nevis */
    { 208, {'S','C',0}, {'S','Y','C',0}, 47603, 690 }, /* Seychelles */
    { 209, {'Z','A',0}, {'Z','A','F',0}, 10039883, 710 }, /* South Africa */
    { 210, {'S','N',0}, {'S','E','N',0}, 42483, 686 }, /* Senegal */
    { 212, {'S','I',0}, {'S','V','N',0}, 47610, 705 }, /* Slovenia */
    { 213, {'S','L',0}, {'S','L','E',0}, 42483, 694 }, /* Sierra Leone */
    { 214, {'S','M',0}, {'S','M','R',0}, 47610, 674 }, /* San Marino */
    { 215, {'S','G',0}, {'S','G','P',0}, 47599, 702 }, /* Singapore */
    { 216, {'S','O',0}, {'S','O','M',0}, 47603, 706 }, /* Somalia */
    { 217, {'E','S',0}, {'E','S','P',0}, 47610, 724 }, /* Spain */
    { 218, {'L','C',0}, {'L','C','A',0}, 10039880, 662 }, /* St. Lucia */
    { 219, {'S','D',0}, {'S','D','N',0}, 42487, 736 }, /* Sudan */
    { 220, {'S','J',0}, {'S','J','M',0}, 10039882, 744 }, /* Svalbard */
    { 221, {'S','E',0}, {'S','W','E',0}, 10039882, 752 }, /* Sweden */
    { 222, {'S','Y',0}, {'S','Y','R',0}, 47611, 760 }, /* Syria */
    { 223, {'C','H',0}, {'C','H','E',0}, 10210824, 756 }, /* Switzerland */
    { 224, {'A','E',0}, {'A','R','E',0}, 47611, 784 }, /* United Arab Emirates */
    { 225, {'T','T',0}, {'T','T','O',0}, 10039880, 780 }, /* Trinidad and Tobago */
    { 227, {'T','H',0}, {'T','H','A',0}, 47599, 764 }, /* Thailand */
    { 228, {'T','J',0}, {'T','J','K',0}, 47590, 762 }, /* Tajikistan */
    { 231, {'T','O',0}, {'T','O','N',0}, 26286, 776 }, /* Tonga */
    { 232, {'T','G',0}, {'T','G','O',0}, 42483, 768 }, /* Togo */
    { 233, {'S','T',0}, {'S','T','P',0}, 42484, 678 }, /* São Tomé and Príncipe */
    { 234, {'T','N',0}, {'T','U','N',0}, 42487, 788 }, /* Tunisia */
    { 235, {'T','R',0}, {'T','U','R',0}, 47611, 792 }, /* Turkey */
    { 236, {'T','V',0}, {'T','U','V',0}, 26286, 798 }, /* Tuvalu */
    { 237, {'T','W',0}, {'T','W','N',0}, 47600, 158 }, /* Taiwan */
    { 238, {'T','M',0}, {'T','K','M',0}, 47590, 795 }, /* Turkmenistan */
    { 239, {'T','Z',0}, {'T','Z','A',0}, 47603, 834 }, /* Tanzania */
    { 240, {'U','G',0}, {'U','G','A',0}, 47603, 800 }, /* Uganda */
    { 241, {'U','A',0}, {'U','K','R',0}, 47609, 804 }, /* Ukraine */
    { 242, {'G','B',0}, {'G','B','R',0}, 10039882, 826 }, /* United Kingdom */
    { 244, {'U','S',0}, {'U','S','A',0}, 23581, 840 }, /* United States */
    { 245, {'B','F',0}, {'B','F','A',0}, 42483, 854 }, /* Burkina Faso */
    { 246, {'U','Y',0}, {'U','R','Y',0}, 31396, 858 }, /* Uruguay */
    { 247, {'U','Z',0}, {'U','Z','B',0}, 47590, 860 }, /* Uzbekistan */
    { 248, {'V','C',0}, {'V','C','T',0}, 10039880, 670 }, /* St. Vincent and the Grenadines */
    { 249, {'V','E',0}, {'V','E','N',0}, 31396, 862 }, /* Bolivarian Republic of Venezuela */
    { 251, {'V','N',0}, {'V','N','M',0}, 47599, 704 }, /* Vietnam */
    { 252, {'V','I',0}, {'V','I','R',0}, 10039880, 850 }, /* Virgin Islands */
    { 253, {'V','A',0}, {'V','A','T',0}, 47610, 336 }, /* Vatican City */
    { 254, {'N','A',0}, {'N','A','M',0}, 10039883, 516 }, /* Namibia */
    { 257, {'E','H',0}, {'E','S','H',0}, 42487, 732 }, /* Western Sahara (disputed) */
    { 258, {'X','X',0}, {'X','X',0}, 161832256 }, /* Wake Island */
    { 259, {'W','S',0}, {'W','S','M',0}, 26286, 882 }, /* Samoa */
    { 260, {'S','Z',0}, {'S','W','Z',0}, 10039883, 748 }, /* Swaziland */
    { 261, {'Y','E',0}, {'Y','E','M',0}, 47611, 887 }, /* Yemen */
    { 263, {'Z','M',0}, {'Z','M','B',0}, 47603, 894 }, /* Zambia */
    { 264, {'Z','W',0}, {'Z','W','E',0}, 47603, 716 }, /* Zimbabwe */
    { 269, {'C','S',0}, {'S','C','G',0}, 47610, 891 }, /* Serbia and Montenegro (Former) */
    { 270, {'M','E',0}, {'M','N','E',0}, 47610, 499 }, /* Montenegro */
    { 271, {'R','S',0}, {'S','R','B',0}, 47610, 688 }, /* Serbia */
    { 273, {'C','W',0}, {'C','U','W',0}, 10039880, 531 }, /* Curaçao */
    { 276, {'S','S',0}, {'S','S','D',0}, 42487, 728 }, /* South Sudan */
    { 300, {'A','I',0}, {'A','I','A',0}, 10039880, 660 }, /* Anguilla */
    { 301, {'A','Q',0}, {'A','T','A',0}, 39070,  10 }, /* Antarctica */
    { 302, {'A','W',0}, {'A','B','W',0}, 10039880, 533 }, /* Aruba */
    { 303, {'X','X',0}, {'X','X',0}, 343 }, /* Ascension Island */
    { 304, {'X','X',0}, {'X','X',0}, 10210825 }, /* Ashmore and Cartier Islands */
    { 305, {'X','X',0}, {'X','X',0}, 161832256 }, /* Baker Island */
    { 306, {'B','V',0}, {'B','V','T',0}, 39070,  74 }, /* Bouvet Island */
    { 307, {'K','Y',0}, {'C','Y','M',0}, 10039880, 136 }, /* Cayman Islands */
    { 308, {'X','X',0}, {'X','X',0}, 10210824, 830, LOCATION_BOTH }, /* Channel Islands */
    { 309, {'C','X',0}, {'C','X','R',0}, 12, 162 }, /* Christmas Island */
    { 310, {'X','X',0}, {'X','X',0}, 27114 }, /* Clipperton Island */
    { 311, {'C','C',0}, {'C','C','K',0}, 10210825, 166 }, /* Cocos (Keeling) Islands */
    { 312, {'C','K',0}, {'C','O','K',0}, 26286, 184 }, /* Cook Islands */
    { 313, {'X','X',0}, {'X','X',0}, 10210825 }, /* Coral Sea Islands */
    { 314, {'X','X',0}, {'X','X',0}, 114 }, /* Diego Garcia */
    { 315, {'F','K',0}, {'F','L','K',0}, 31396, 238 }, /* Falkland Islands (Islas Malvinas) */
    { 317, {'G','F',0}, {'G','U','F',0}, 31396, 254 }, /* French Guiana */
    { 318, {'P','F',0}, {'P','Y','F',0}, 26286, 258 }, /* French Polynesia */
    { 319, {'T','F',0}, {'A','T','F',0}, 39070, 260 }, /* French Southern and Antarctic Lands */
    { 321, {'G','P',0}, {'G','L','P',0}, 10039880, 312 }, /* Guadeloupe */
    { 322, {'G','U',0}, {'G','U','M',0}, 21206, 316 }, /* Guam */
    { 323, {'X','X',0}, {'X','X',0}, 39070 }, /* Guantanamo Bay */
    { 324, {'G','G',0}, {'G','G','Y',0}, 308, 831 }, /* Guernsey */
    { 325, {'H','M',0}, {'H','M','D',0}, 39070, 334 }, /* Heard Island and McDonald Islands */
    { 326, {'X','X',0}, {'X','X',0}, 161832256 }, /* Howland Island */
    { 327, {'X','X',0}, {'X','X',0}, 161832256 }, /* Jarvis Island */
    { 328, {'J','E',0}, {'J','E','Y',0}, 308, 832 }, /* Jersey */
    { 329, {'X','X',0}, {'X','X',0}, 161832256 }, /* Kingman Reef */
    { 330, {'M','Q',0}, {'M','T','Q',0}, 10039880, 474 }, /* Martinique */
    { 331, {'Y','T',0}, {'M','Y','T',0}, 47603, 175 }, /* Mayotte */
    { 332, {'M','S',0}, {'M','S','R',0}, 10039880, 500 }, /* Montserrat */
    { 333, {'A','N',0}, {'A','N','T',0}, 10039880, 530, LOCATION_BOTH }, /* Netherlands Antilles (Former) */
    { 334, {'N','C',0}, {'N','C','L',0}, 20900, 540 }, /* New Caledonia */
    { 335, {'N','U',0}, {'N','I','U',0}, 26286, 570 }, /* Niue */
    { 336, {'N','F',0}, {'N','F','K',0}, 10210825, 574 }, /* Norfolk Island */
    { 337, {'M','P',0}, {'M','N','P',0}, 21206, 580 }, /* Northern Mariana Islands */
    { 338, {'X','X',0}, {'X','X',0}, 161832256 }, /* Palmyra Atoll */
    { 339, {'P','N',0}, {'P','C','N',0}, 26286, 612 }, /* Pitcairn Islands */
    { 340, {'X','X',0}, {'X','X',0}, 337 }, /* Rota Island */
    { 341, {'X','X',0}, {'X','X',0}, 337 }, /* Saipan */
    { 342, {'G','S',0}, {'S','G','S',0}, 39070, 239 }, /* South Georgia and the South Sandwich Islands */
    { 343, {'S','H',0}, {'S','H','N',0}, 42483, 654 }, /* St. Helena */
    { 346, {'X','X',0}, {'X','X',0}, 337 }, /* Tinian Island */
    { 347, {'T','K',0}, {'T','K','L',0}, 26286, 772 }, /* Tokelau */
    { 348, {'X','X',0}, {'X','X',0}, 343 }, /* Tristan da Cunha */
    { 349, {'T','C',0}, {'T','C','A',0}, 10039880, 796 }, /* Turks and Caicos Islands */
    { 351, {'V','G',0}, {'V','G','B',0}, 10039880,  92 }, /* Virgin Islands, British */
    { 352, {'W','F',0}, {'W','L','F',0}, 26286, 876 }, /* Wallis and Futuna */
    { 742, {'X','X',0}, {'X','X',0}, 39070, 2, LOCATION_REGION }, /* Africa */
    { 2129, {'X','X',0}, {'X','X',0}, 39070, 142, LOCATION_REGION }, /* Asia */
    { 10541, {'X','X',0}, {'X','X',0}, 39070, 150, LOCATION_REGION }, /* Europe */
    { 15126, {'I','M',0}, {'I','M','N',0}, 10039882, 833 }, /* Man, Isle of */
    { 19618, {'M','K',0}, {'M','K','D',0}, 47610, 807 }, /* Macedonia, Former Yugoslav Republic of */
    { 20900, {'X','X',0}, {'X','X',0}, 27114, 54, LOCATION_REGION }, /* Melanesia */
    { 21206, {'X','X',0}, {'X','X',0}, 27114, 57, LOCATION_REGION }, /* Micronesia */
    { 21242, {'X','X',0}, {'X','X',0}, 161832256 }, /* Midway Islands */
    { 23581, {'X','X',0}, {'X','X',0}, 10026358, 21, LOCATION_REGION }, /* Northern America */
    { 26286, {'X','X',0}, {'X','X',0}, 27114, 61, LOCATION_REGION }, /* Polynesia */
    { 27082, {'X','X',0}, {'X','X',0}, 161832257, 13, LOCATION_REGION }, /* Central America */
    { 27114, {'X','X',0}, {'X','X',0}, 39070, 9, LOCATION_REGION }, /* Oceania */
    { 30967, {'S','X',0}, {'S','X','M',0}, 10039880, 534 }, /* Sint Maarten (Dutch part) */
    { 31396, {'X','X',0}, {'X','X',0}, 161832257, 5, LOCATION_REGION }, /* South America */
    { 31706, {'M','F',0}, {'M','A','F',0}, 10039880, 663 }, /* Saint Martin (French part) */
    { 39070, {'X','X',0}, {'X','X',0}, 39070, 1, LOCATION_REGION }, /* World */
    { 42483, {'X','X',0}, {'X','X',0}, 742, 11, LOCATION_REGION }, /* Western Africa */
    { 42484, {'X','X',0}, {'X','X',0}, 742, 17, LOCATION_REGION }, /* Middle Africa */
    { 42487, {'X','X',0}, {'X','X',0}, 742, 15, LOCATION_REGION }, /* Northern Africa */
    { 47590, {'X','X',0}, {'X','X',0}, 2129, 143, LOCATION_REGION }, /* Central Asia */
    { 47599, {'X','X',0}, {'X','X',0}, 2129, 35, LOCATION_REGION }, /* South-Eastern Asia */
    { 47600, {'X','X',0}, {'X','X',0}, 2129, 30, LOCATION_REGION }, /* Eastern Asia */
    { 47603, {'X','X',0}, {'X','X',0}, 742, 14, LOCATION_REGION }, /* Eastern Africa */
    { 47609, {'X','X',0}, {'X','X',0}, 10541, 151, LOCATION_REGION }, /* Eastern Europe */
    { 47610, {'X','X',0}, {'X','X',0}, 10541, 39, LOCATION_REGION }, /* Southern Europe */
    { 47611, {'X','X',0}, {'X','X',0}, 2129, 145, LOCATION_REGION }, /* Middle East */
    { 47614, {'X','X',0}, {'X','X',0}, 2129, 34, LOCATION_REGION }, /* Southern Asia */
    { 7299303, {'T','L',0}, {'T','L','S',0}, 47599, 626 }, /* Democratic Republic of Timor-Leste */
    { 9914689, {'X','K',0}, {'X','K','S',0}, 47610, 906 }, /* Kosovo */
    { 10026358, {'X','X',0}, {'X','X',0}, 39070, 19, LOCATION_REGION }, /* Americas */
    { 10028789, {'A','X',0}, {'A','L','A',0}, 10039882, 248 }, /* Åland Islands */
    { 10039880, {'X','X',0}, {'X','X',0}, 161832257, 29, LOCATION_REGION }, /* Caribbean */
    { 10039882, {'X','X',0}, {'X','X',0}, 10541, 154, LOCATION_REGION }, /* Northern Europe */
    { 10039883, {'X','X',0}, {'X','X',0}, 742, 18, LOCATION_REGION }, /* Southern Africa */
    { 10210824, {'X','X',0}, {'X','X',0}, 10541, 155, LOCATION_REGION }, /* Western Europe */
    { 10210825, {'X','X',0}, {'X','X',0}, 27114, 53, LOCATION_REGION }, /* Australia and New Zealand */
    { 161832015, {'B','L',0}, {'B','L','M',0}, 10039880, 652 }, /* Saint Barthélemy */
    { 161832256, {'U','M',0}, {'U','M','I',0}, 27114, 581 }, /* U.S. Minor Outlying Islands */
    { 161832257, {'X','X',0}, {'X','X',0}, 10026358, 419, LOCATION_REGION }, /* Latin America and the Caribbean */
};

static const struct geoinfo_t *get_geoinfo_dataptr(GEOID geoid)
{
    int min, max;

    min = 0;
    max = ARRAY_SIZE(geoinfodata)-1;

    while (min <= max) {
        const struct geoinfo_t *ptr;
        int n = (min+max)/2;

        ptr = &geoinfodata[n];
        if (geoid == ptr->id)
            /* we don't need empty entries */
            return *ptr->iso2W ? ptr : NULL;

        if (ptr->id > geoid)
            max = n-1;
        else
            min = n+1;
    }

    return NULL;
}

/******************************************************************************
 *           GetUserGeoID (KERNEL32.@)
 *
 * Retrieves the ID of the user's geographic nation or region.
 *
 * PARAMS
 *   geoclass [I] One of GEOCLASS_NATION or GEOCLASS_REGION (SYSGEOCLASS enum from "winnls.h").
 *
 * RETURNS
 *   SUCCESS: The ID of the specified geographic class.
 *   FAILURE: GEOID_NOT_AVAILABLE.
 */
GEOID WINAPI GetUserGeoID(GEOCLASS geoclass)
{
    GEOID ret = 39070;
    static const WCHAR geoW[] = {'G','e','o',0};
    static const WCHAR nationW[] = {'N','a','t','i','o','n',0};
    static const WCHAR regionW[] = {'R','e','g','i','o','n',0};
    WCHAR bufferW[40], *end;
    HANDLE hkey, hsubkey = 0;
    UNICODE_STRING keyW;
    const KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)bufferW;
    DWORD count = sizeof(bufferW);
    RtlInitUnicodeString(&keyW, nationW);

    switch (geoclass)
    {
    case GEOCLASS_NATION:
        RtlInitUnicodeString(&keyW, nationW);
        break;
    case GEOCLASS_REGION:
        RtlInitUnicodeString(&keyW, regionW);
        break;
    default:
        WARN("Unknown geoclass %d\n", geoclass);
        return GEOID_NOT_AVAILABLE;
    }

    if (!(hkey = create_registry_key())) return ret;

    if ((hsubkey = NLS_RegOpenKey(hkey, geoW)))
    {
        if((NtQueryValueKey(hsubkey, &keyW, KeyValuePartialInformation,
                            bufferW, count, &count) == STATUS_SUCCESS ) && info->DataLength)
            ret = strtolW((const WCHAR*)info->Data, &end, 10);
    }

    NtClose(hkey);
    if (hsubkey) NtClose(hsubkey);
    return ret;
}

/******************************************************************************
 *           SetUserGeoID (KERNEL32.@)
 *
 * Sets the ID of the user's geographic location.
 *
 * PARAMS
 *   geoid [I] The geographic ID to be set.
 *
 * RETURNS
 *   SUCCESS: TRUE.
 *   FAILURE: FALSE. GetLastError() will return ERROR_INVALID_PARAMETER if the ID was invalid.
 */
BOOL WINAPI SetUserGeoID(GEOID geoid)
{
    const struct geoinfo_t *geoinfo = get_geoinfo_dataptr(geoid);
    static const WCHAR geoW[] = {'G','e','o',0};
    static const WCHAR nationW[] = {'N','a','t','i','o','n',0};
    static const WCHAR regionW[] = {'R','e','g','i','o','n',0};
    static const WCHAR formatW[] = {'%','i',0};
    UNICODE_STRING nameW, keyW;
    WCHAR bufferW[10];
    OBJECT_ATTRIBUTES attr;
    HANDLE hkey;

    if (!geoinfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(hkey = create_registry_key())) return FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString(&nameW, geoW);

    if (geoinfo->kind == LOCATION_NATION)
        RtlInitUnicodeString(&keyW, nationW);
    else
        RtlInitUnicodeString(&keyW, regionW);

    if (NtCreateKey(&hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL) != STATUS_SUCCESS)
    {
        NtClose(attr.RootDirectory);
        return FALSE;
    }

    sprintfW(bufferW, formatW, geoinfo->id);
    NtSetValueKey(hkey, &keyW, 0, REG_SZ, bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR));
    NtClose(attr.RootDirectory);
    NtClose(hkey);
    return TRUE;
}

/******************************************************************************
 *           GetGeoInfoW (KERNEL32.@)
 *
 * Retrieves information about a geographic location by its GeoID.
 *
 * PARAMS
 *   geoid    [I] The GeoID of the location of interest.
 *   geotype  [I] The type of information to be retrieved (SYSGEOTYPE enum from "winnls.h").
 *   data     [O] The output buffer to store the information.
 *   data_len [I] The length of the buffer, measured in WCHARs and including the null terminator.
 *   lang     [I] Language identifier. Must be 0 unless geotype is GEO_RFC1766 or GEO_LCID.
 *
 * RETURNS
 *   Success: The number of WCHARs (including null) written to the buffer -or-
 *            if no buffer was provided, the minimum length required to hold the full data.
 *   Failure: Zero. Call GetLastError() to determine the cause.
 *
 * NOTES
 *   On failure, GetLastError() will return one of the following values:
 *     - ERROR_INVALID_PARAMETER: the GeoID provided was invalid.
 *     - ERROR_INVALID_FLAGS: the specified geotype was invalid.
 *     - ERROR_INSUFFICIENT_BUFFER: the provided buffer was too small to hold the full data.
 *     - ERROR_CALL_NOT_IMPLEMENTED: (Wine implementation) we don't handle that geotype yet.
 *
 *   The list of available GeoIDs can be retrieved with EnumSystemGeoID(),
 *   or call GetUserGeoID() to retrieve the user's current location.
 *
 * TODO
 *   Currently, we only handle the following geotypes: GEO_ID, GEO_ISO2, GEO_ISO3,
 *   GEO_ISO_UN_NUMBER, GEO_PARENT and GEO_NATION.
 */
INT WINAPI GetGeoInfoW(GEOID geoid, GEOTYPE geotype, LPWSTR data, int data_len, LANGID lang)
{
    const struct geoinfo_t *ptr;
    WCHAR buffW[12];
    const WCHAR *str = buffW;
    int len;
    static const WCHAR fmtW[] = {'%','d',0};

    TRACE("%d %d %p %d %d\n", geoid, geotype, data, data_len, lang);

    if (!(ptr = get_geoinfo_dataptr(geoid))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    switch (geotype) {
    case GEO_NATION:
        if (ptr->kind != LOCATION_NATION) return 0;
        /* fall through */
    case GEO_ID:
        sprintfW(buffW, fmtW, ptr->id);
        break;
    case GEO_ISO_UN_NUMBER:
        sprintfW(buffW, fmtW, ptr->uncode);
        break;
    case GEO_PARENT:
        sprintfW(buffW, fmtW, ptr->parent);
        break;
    case GEO_ISO2:
        str = ptr->iso2W;
        break;
    case GEO_ISO3:
        str = ptr->iso3W;
        break;
    case GEO_RFC1766:
    case GEO_LCID:
    case GEO_FRIENDLYNAME:
    case GEO_OFFICIALNAME:
    case GEO_TIMEZONES:
    case GEO_OFFICIALLANGUAGES:
    case GEO_LATITUDE:
    case GEO_LONGITUDE:
        FIXME("type %d is not supported\n", geotype);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
    default:
        WARN("unrecognized type %d\n", geotype);
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    len = strlenW(str) + 1;
    if (!data || !data_len)
        return len;

    memcpy(data, str, min(len, data_len)*sizeof(WCHAR));
    if (data_len < len)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return data_len < len ? 0 : len;
}

/******************************************************************************
 *           GetGeoInfoA (KERNEL32.@)
 */
INT WINAPI GetGeoInfoA(GEOID geoid, GEOTYPE geotype, LPSTR data, int data_len, LANGID lang)
{
    WCHAR *buffW;
    INT len;

    TRACE("%d %d %p %d %d\n", geoid, geotype, data, data_len, lang);

    len = GetGeoInfoW(geoid, geotype, NULL, 0, lang);
    if (!len)
        return 0;

    buffW = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    if (!buffW)
        return 0;

    GetGeoInfoW(geoid, geotype, buffW, len, lang);
    len = WideCharToMultiByte(CP_ACP, 0, buffW, -1, NULL, 0, NULL, NULL);
    if (!data || !data_len) {
        HeapFree(GetProcessHeap(), 0, buffW);
        return len;
    }

    len = WideCharToMultiByte(CP_ACP, 0, buffW, -1, data, data_len, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, buffW);

    if (data_len < len)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return data_len < len ? 0 : len;
}

/******************************************************************************
 *           EnumSystemGeoID    (KERNEL32.@)
 *
 * Calls a user's function for every location available on the system.
 *
 * PARAMS
 *  geoclass   [I] Type of location desired (SYSGEOTYPE enum from "winnls.h")
 *  parent     [I] GeoID for the parent
 *  enumproc   [I] Callback function to call for each location (prototype in "winnls.h")
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  The enumproc function returns TRUE to continue enumerating
 *  or FALSE to interrupt the enumeration.
 *
 *  On failure, GetLastError() returns one of the following values:
 *   - ERROR_INVALID_PARAMETER: no callback function was provided.
 *   - ERROR_INVALID_FLAGS: the location type was invalid.
 *
 * TODO
 *  On Windows 10, this function filters out those locations which
 *  simultaneously lack ISO and UN codes (e.g. Johnson Atoll).
 */
BOOL WINAPI EnumSystemGeoID(GEOCLASS geoclass, GEOID parent, GEO_ENUMPROC enumproc)
{
    INT i;

    TRACE("(%d, %d, %p)\n", geoclass, parent, enumproc);

    if (!enumproc) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (geoclass != GEOCLASS_NATION && geoclass != GEOCLASS_REGION && geoclass != GEOCLASS_ALL) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(geoinfodata); i++) {
        const struct geoinfo_t *ptr = &geoinfodata[i];

        if (geoclass == GEOCLASS_NATION && (ptr->kind != LOCATION_NATION))
            continue;

        /* LOCATION_BOTH counts as region. */
        if (geoclass == GEOCLASS_REGION && (ptr->kind == LOCATION_NATION))
            continue;

        if (parent && ptr->parent != parent)
            continue;

        if (!enumproc(ptr->id))
            return TRUE;
    }

    return TRUE;
}


enum {
    BASE = 36,
    TMIN = 1,
    TMAX = 26,
    SKEW = 38,
    DAMP = 700,
    INIT_BIAS = 72,
    INIT_N = 128
};

static inline INT adapt(INT delta, INT numpoints, BOOL firsttime)
{
    INT k;

    delta /= (firsttime ? DAMP : 2);
    delta += delta/numpoints;

    for(k=0; delta>((BASE-TMIN)*TMAX)/2; k+=BASE)
        delta /= BASE-TMIN;
    return k+((BASE-TMIN+1)*delta)/(delta+SKEW);
}

/******************************************************************************
 *           IdnToAscii (KERNEL32.@)
 * Implementation of Punycode based on RFC 3492.
 */
INT WINAPI IdnToAscii(DWORD dwFlags, LPCWSTR lpUnicodeCharStr, INT cchUnicodeChar,
                      LPWSTR lpASCIICharStr, INT cchASCIIChar)
{
    static const WCHAR prefixW[] = {'x','n','-','-'};

    WCHAR *norm_str;
    INT i, label_start, label_end, norm_len, out_label, out = 0;

    TRACE("%x %p %d %p %d\n", dwFlags, lpUnicodeCharStr, cchUnicodeChar,
        lpASCIICharStr, cchASCIIChar);

    norm_len = IdnToNameprepUnicode(dwFlags, lpUnicodeCharStr, cchUnicodeChar, NULL, 0);
    if(!norm_len)
        return 0;
    norm_str = HeapAlloc(GetProcessHeap(), 0, norm_len*sizeof(WCHAR));
    if(!norm_str) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    norm_len = IdnToNameprepUnicode(dwFlags, lpUnicodeCharStr,
            cchUnicodeChar, norm_str, norm_len);
    if(!norm_len) {
        HeapFree(GetProcessHeap(), 0, norm_str);
        return 0;
    }

    for(label_start=0; label_start<norm_len;) {
        INT n = INIT_N, bias = INIT_BIAS;
        INT delta = 0, b = 0, h;

        out_label = out;
        for(i=label_start; i<norm_len && norm_str[i]!='.' &&
                norm_str[i]!=0x3002 && norm_str[i]!='\0'; i++)
            if(norm_str[i] < 0x80)
                b++;
        label_end = i;

        if(b == label_end-label_start) {
            if(label_end < norm_len)
                b++;
            if(!lpASCIICharStr) {
                out += b;
            }else if(out+b <= cchASCIIChar) {
                memcpy(lpASCIICharStr+out, norm_str+label_start, b*sizeof(WCHAR));
                out += b;
            }else {
                HeapFree(GetProcessHeap(), 0, norm_str);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            label_start = label_end+1;
            continue;
        }

        if(!lpASCIICharStr) {
            out += 5+b; /* strlen(xn--...-) */
        }else if(out+5+b <= cchASCIIChar) {
            memcpy(lpASCIICharStr+out, prefixW, sizeof(prefixW));
            out += 4;
            for(i=label_start; i<label_end; i++)
                if(norm_str[i] < 0x80)
                    lpASCIICharStr[out++] = norm_str[i];
            lpASCIICharStr[out++] = '-';
        }else {
            HeapFree(GetProcessHeap(), 0, norm_str);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        if(!b)
            out--;

        for(h=b; h<label_end-label_start;) {
            INT m = 0xffff, q, k;

            for(i=label_start; i<label_end; i++) {
                if(norm_str[i]>=n && m>norm_str[i])
                    m = norm_str[i];
            }
            delta += (m-n)*(h+1);
            n = m;

            for(i=label_start; i<label_end; i++) {
                if(norm_str[i] < n) {
                    delta++;
                }else if(norm_str[i] == n) {
                    for(q=delta, k=BASE; ; k+=BASE) {
                        INT t = k<=bias ? TMIN : k>=bias+TMAX ? TMAX : k-bias;
                        INT disp = q<t ? q : t+(q-t)%(BASE-t);
                        if(!lpASCIICharStr) {
                            out++;
                        }else if(out+1 <= cchASCIIChar) {
                            lpASCIICharStr[out++] = disp<='z'-'a' ?
                                'a'+disp : '0'+disp-'z'+'a'-1;
                        }else {
                            HeapFree(GetProcessHeap(), 0, norm_str);
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                            return 0;
                        }
                        if(q < t)
                            break;
                        q = (q-t)/(BASE-t);
                    }
                    bias = adapt(delta, h+1, h==b);
                    delta = 0;
                    h++;
                }
            }
            delta++;
            n++;
        }

        if(out-out_label > 63) {
            HeapFree(GetProcessHeap(), 0, norm_str);
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end < norm_len) {
            if(!lpASCIICharStr) {
                out++;
            }else if(out+1 <= cchASCIIChar) {
                lpASCIICharStr[out++] = norm_str[label_end] ? '.' : 0;
            }else {
                HeapFree(GetProcessHeap(), 0, norm_str);
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
        }
        label_start = label_end+1;
    }

    HeapFree(GetProcessHeap(), 0, norm_str);
    return out;
}

/******************************************************************************
 *           IdnToNameprepUnicode (KERNEL32.@)
 */
INT WINAPI IdnToNameprepUnicode(DWORD dwFlags, LPCWSTR lpUnicodeCharStr, INT cchUnicodeChar,
                                LPWSTR lpNameprepCharStr, INT cchNameprepChar)
{
    enum {
        UNASSIGNED = 0x1,
        PROHIBITED = 0x2,
        BIDI_RAL   = 0x4,
        BIDI_L     = 0x8
    };

    const WCHAR *ptr;
    WORD flags;
    WCHAR buf[64], *map_str, norm_str[64], ch;
    DWORD i, map_len, norm_len, mask, label_start, label_end, out = 0;
    BOOL have_bidi_ral, prohibit_bidi_ral, ascii_only;

    TRACE("%x %p %d %p %d\n", dwFlags, lpUnicodeCharStr, cchUnicodeChar,
        lpNameprepCharStr, cchNameprepChar);

    if(dwFlags & ~(IDN_ALLOW_UNASSIGNED|IDN_USE_STD3_ASCII_RULES)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    if(!lpUnicodeCharStr || cchUnicodeChar<-1) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if(cchUnicodeChar == -1)
        cchUnicodeChar = strlenW(lpUnicodeCharStr)+1;
    if(!cchUnicodeChar || (cchUnicodeChar==1 && lpUnicodeCharStr[0]==0)) {
        SetLastError(ERROR_INVALID_NAME);
        return 0;
    }

    for(label_start=0; label_start<cchUnicodeChar;) {
        ascii_only = TRUE;
        for(i=label_start; i<cchUnicodeChar; i++) {
            ch = lpUnicodeCharStr[i];

            if(i!=cchUnicodeChar-1 && !ch) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }
            /* check if ch is one of label separators defined in RFC3490 */
            if(!ch || ch=='.' || ch==0x3002 || ch==0xff0e || ch==0xff61)
                break;

            if(ch > 0x7f) {
                ascii_only = FALSE;
                continue;
            }

            if((dwFlags&IDN_USE_STD3_ASCII_RULES) == 0)
                continue;
            if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
                    || (ch>='0' && ch<='9') || ch=='-')
                continue;

            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }
        label_end = i;
        /* last label may be empty */
        if(label_start==label_end && ch) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if((dwFlags&IDN_USE_STD3_ASCII_RULES) && (lpUnicodeCharStr[label_start]=='-' ||
                    lpUnicodeCharStr[label_end-1]=='-')) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(ascii_only) {
            /* maximal label length is 63 characters */
            if(label_end-label_start > 63) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }
            if(label_end < cchUnicodeChar)
                label_end++;

            if(!lpNameprepCharStr) {
                out += label_end-label_start;
            }else if(out+label_end-label_start <= cchNameprepChar) {
                memcpy(lpNameprepCharStr+out, lpUnicodeCharStr+label_start,
                        (label_end-label_start)*sizeof(WCHAR));
                if(lpUnicodeCharStr[label_end-1] > 0x7f)
                    lpNameprepCharStr[out+label_end-label_start-1] = '.';
                out += label_end-label_start;
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            label_start = label_end;
            continue;
        }

        map_len = 0;
        for(i=label_start; i<label_end; i++) {
            ch = lpUnicodeCharStr[i];
            ptr = nameprep_mapping + nameprep_mapping[ch>>8];
            ptr = nameprep_mapping + ptr[(ch>>4)&0x0f] + 3*(ch&0x0f);

            if(!ptr[0]) map_len++;
            else if(!ptr[1]) map_len++;
            else if(!ptr[2]) map_len += 2;
            else if(ptr[0]!=0xffff || ptr[1]!=0xffff || ptr[2]!=0xffff) map_len += 3;
        }
        if(map_len*sizeof(WCHAR) > sizeof(buf)) {
            map_str = HeapAlloc(GetProcessHeap(), 0, map_len*sizeof(WCHAR));
            if(!map_str) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
        }else {
            map_str = buf;
        }
        map_len = 0;
        for(i=label_start; i<label_end; i++) {
            ch = lpUnicodeCharStr[i];
            ptr = nameprep_mapping + nameprep_mapping[ch>>8];
            ptr = nameprep_mapping + ptr[(ch>>4)&0x0f] + 3*(ch&0x0f);

            if(!ptr[0]) {
                map_str[map_len++] = ch;
            }else if(!ptr[1]) {
                map_str[map_len++] = ptr[0];
            }else if(!ptr[2]) {
                map_str[map_len++] = ptr[0];
                map_str[map_len++] = ptr[1];
            }else if(ptr[0]!=0xffff || ptr[1]!=0xffff || ptr[2]!=0xffff) {
                map_str[map_len++] = ptr[0];
                map_str[map_len++] = ptr[1];
                map_str[map_len++] = ptr[2];
            }
        }

        norm_len = FoldStringW(MAP_FOLDCZONE, map_str, map_len, norm_str, ARRAY_SIZE(norm_str)-1);
        if(map_str != buf)
            HeapFree(GetProcessHeap(), 0, map_str);
        if(!norm_len) {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end < cchUnicodeChar) {
            norm_str[norm_len++] = lpUnicodeCharStr[label_end] ? '.' : 0;
            label_end++;
        }

        if(!lpNameprepCharStr) {
            out += norm_len;
        }else if(out+norm_len <= cchNameprepChar) {
            memcpy(lpNameprepCharStr+out, norm_str, norm_len*sizeof(WCHAR));
            out += norm_len;
        }else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }

        have_bidi_ral = prohibit_bidi_ral = FALSE;
        mask = PROHIBITED;
        if((dwFlags&IDN_ALLOW_UNASSIGNED) == 0)
            mask |= UNASSIGNED;
        for(i=0; i<norm_len; i++) {
            ch = norm_str[i];
            flags = get_table_entry( nameprep_char_type, ch );

            if(flags & mask) {
                SetLastError((flags & PROHIBITED) ? ERROR_INVALID_NAME
                        : ERROR_NO_UNICODE_TRANSLATION);
                return 0;
            }

            if(flags & BIDI_RAL)
                have_bidi_ral = TRUE;
            if(flags & BIDI_L)
                prohibit_bidi_ral = TRUE;
        }

        if(have_bidi_ral) {
            ch = norm_str[0];
            flags = get_table_entry( nameprep_char_type, ch );
            if((flags & BIDI_RAL) == 0)
                prohibit_bidi_ral = TRUE;

            ch = norm_str[norm_len-1];
            flags = get_table_entry( nameprep_char_type, ch );
            if((flags & BIDI_RAL) == 0)
                prohibit_bidi_ral = TRUE;
        }

        if(have_bidi_ral && prohibit_bidi_ral) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        label_start = label_end;
    }

    return out;
}

/******************************************************************************
 *           IdnToUnicode (KERNEL32.@)
 */
INT WINAPI IdnToUnicode(DWORD dwFlags, LPCWSTR lpASCIICharStr, INT cchASCIIChar,
                        LPWSTR lpUnicodeCharStr, INT cchUnicodeChar)
{
    INT i, label_start, label_end, out_label, out = 0;
    WCHAR ch;

    TRACE("%x %p %d %p %d\n", dwFlags, lpASCIICharStr, cchASCIIChar,
        lpUnicodeCharStr, cchUnicodeChar);

    for(label_start=0; label_start<cchASCIIChar;) {
        INT n = INIT_N, pos = 0, old_pos, w, k, bias = INIT_BIAS, delim=0, digit, t;

        out_label = out;
        for(i=label_start; i<cchASCIIChar; i++) {
            ch = lpASCIICharStr[i];

            if(ch>0x7f || (i!=cchASCIIChar-1 && !ch)) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }

            if(!ch || ch=='.')
                break;
            if(ch == '-')
                delim = i;

            if((dwFlags&IDN_USE_STD3_ASCII_RULES) == 0)
                continue;
            if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
                    || (ch>='0' && ch<='9') || ch=='-')
                continue;

            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }
        label_end = i;
        /* last label may be empty */
        if(label_start==label_end && ch) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if((dwFlags&IDN_USE_STD3_ASCII_RULES) && (lpASCIICharStr[label_start]=='-' ||
                    lpASCIICharStr[label_end-1]=='-')) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }
        if(label_end-label_start > 63) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end-label_start<4 ||
                tolowerW(lpASCIICharStr[label_start])!='x' ||
                tolowerW(lpASCIICharStr[label_start+1])!='n' ||
                lpASCIICharStr[label_start+2]!='-' || lpASCIICharStr[label_start+3]!='-') {
            if(label_end < cchASCIIChar)
                label_end++;

            if(!lpUnicodeCharStr) {
                out += label_end-label_start;
            }else if(out+label_end-label_start <= cchUnicodeChar) {
                memcpy(lpUnicodeCharStr+out, lpASCIICharStr+label_start,
                        (label_end-label_start)*sizeof(WCHAR));
                out += label_end-label_start;
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            label_start = label_end;
            continue;
        }

        if(delim == label_start+3)
            delim++;
        if(!lpUnicodeCharStr) {
            out += delim-label_start-4;
        }else if(out+delim-label_start-4 <= cchUnicodeChar) {
            memcpy(lpUnicodeCharStr+out, lpASCIICharStr+label_start+4,
                    (delim-label_start-4)*sizeof(WCHAR));
            out += delim-label_start-4;
        }else {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        if(out != out_label)
            delim++;

        for(i=delim; i<label_end;) {
            old_pos = pos;
            w = 1;
            for(k=BASE; ; k+=BASE) {
                ch = i<label_end ? tolowerW(lpASCIICharStr[i++]) : 0;
                if((ch<'a' || ch>'z') && (ch<'0' || ch>'9')) {
                    SetLastError(ERROR_INVALID_NAME);
                    return 0;
                }
                digit = ch<='9' ? ch-'0'+'z'-'a'+1 : ch-'a';
                pos += digit*w;
                t = k<=bias ? TMIN : k>=bias+TMAX ? TMAX : k-bias;
                if(digit < t)
                    break;
                w *= BASE-t;
            }
            bias = adapt(pos-old_pos, out-out_label+1, old_pos==0);
            n += pos/(out-out_label+1);
            pos %= out-out_label+1;

            if((dwFlags&IDN_ALLOW_UNASSIGNED)==0 &&
                    get_table_entry(nameprep_char_type, n)==1/*UNASSIGNED*/) {
                SetLastError(ERROR_INVALID_NAME);
                return 0;
            }
            if(!lpUnicodeCharStr) {
                out++;
            }else if(out+1 <= cchASCIIChar) {
                memmove(lpUnicodeCharStr+out_label+pos+1,
                        lpUnicodeCharStr+out_label+pos,
                        (out-out_label-pos)*sizeof(WCHAR));
                lpUnicodeCharStr[out_label+pos] = n;
                out++;
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            pos++;
        }

        if(out-out_label > 63) {
            SetLastError(ERROR_INVALID_NAME);
            return 0;
        }

        if(label_end < cchASCIIChar) {
            if(!lpUnicodeCharStr) {
                out++;
            }else if(out+1 <= cchUnicodeChar) {
                lpUnicodeCharStr[out++] = lpASCIICharStr[label_end];
            }else {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
        }
        label_start = label_end+1;
    }

    return out;
}


/******************************************************************************
 *           GetFileMUIPath (KERNEL32.@)
 */

BOOL WINAPI GetFileMUIPath(DWORD flags, PCWSTR filepath, PWSTR language, PULONG languagelen,
                           PWSTR muipath, PULONG muipathlen, PULONGLONG enumerator)
{
    FIXME("stub: 0x%x, %s, %s, %p, %p, %p, %p\n", flags, debugstr_w(filepath),
           debugstr_w(language), languagelen, muipath, muipathlen, enumerator);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return FALSE;
}

/******************************************************************************
 *           GetFileMUIInfo (KERNEL32.@)
 */

BOOL WINAPI GetFileMUIInfo(DWORD flags, PCWSTR path, FILEMUIINFO *info, DWORD *size)
{
    FIXME("stub: %u, %s, %p, %p\n", flags, debugstr_w(path), info, size);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
