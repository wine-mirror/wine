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
