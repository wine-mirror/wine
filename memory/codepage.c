/*
 * Code page functions
 *
 * Copyright 2000 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "thread.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(string);

/* current code pages */
static const union cptable *ansi_cptable;
static const union cptable *oem_cptable;
static const union cptable *mac_cptable;
static const union cptable *unix_cptable;  /* NULL if UTF8 */
static LCID default_lcid = MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT );

/* setup default codepage info before we can get at the locale stuff */
static void init_codepages(void)
{
    ansi_cptable = wine_cp_get_table( 1252 );
    oem_cptable  = wine_cp_get_table( 437 );
    mac_cptable  = wine_cp_get_table( 10000 );
    unix_cptable  = wine_cp_get_table( 28591 );
    assert( ansi_cptable );
    assert( oem_cptable );
    assert( mac_cptable );
    assert( unix_cptable );
}

/* find the table for a given codepage, handling CP_ACP etc. pseudo-codepages */
static const union cptable *get_codepage_table( unsigned int codepage )
{
    const union cptable *ret = NULL;

    if (!ansi_cptable) init_codepages();

    switch(codepage)
    {
    case CP_ACP:
        return ansi_cptable;
    case CP_OEMCP:
        return oem_cptable;
    case CP_MACCP:
        return mac_cptable;
    case CP_UTF7:
    case CP_UTF8:
        break;
    case CP_THREAD_ACP:
        if (!(codepage = NtCurrentTeb()->code_page)) return ansi_cptable;
        /* fall through */
    default:
        if (codepage == ansi_cptable->info.codepage) return ansi_cptable;
        if (codepage == oem_cptable->info.codepage) return oem_cptable;
        if (codepage == mac_cptable->info.codepage) return mac_cptable;
        ret = wine_cp_get_table( codepage );
        break;
    }
    return ret;
}

/* initialize default code pages from locale info */
/* FIXME: should be done in init_codepages, but it can't right now */
/* since it needs KERNEL32 to be loaded for the locale info. */
void CODEPAGE_Init( UINT ansi_cp, UINT oem_cp, UINT mac_cp, UINT unix_cp, LCID lcid )
{
    extern void __wine_init_codepages( const union cptable *ansi_cp, const union cptable *oem_cp );
    const union cptable *table;

    default_lcid = lcid;
    if (!ansi_cptable) init_codepages();  /* just in case */

    if ((table = wine_cp_get_table( ansi_cp ))) ansi_cptable = table;
    if ((table = wine_cp_get_table( oem_cp ))) oem_cptable = table;
    if ((table = wine_cp_get_table( mac_cp ))) mac_cptable = table;
    if (unix_cp == CP_UTF8)
        unix_cptable = NULL;
    else if ((table = wine_cp_get_table( unix_cp )))
        unix_cptable = table;

    __wine_init_codepages( ansi_cptable, oem_cptable );

    TRACE( "ansi=%03d oem=%03d mac=%03d unix=%03d\n",
           ansi_cptable->info.codepage, oem_cptable->info.codepage,
           mac_cptable->info.codepage, unix_cp );
}

/******************************************************************************
 *              GetACP   (KERNEL32.@)
 *
 * RETURNS
 *    Current ANSI code-page identifier, default if no current defined
 */
UINT WINAPI GetACP(void)
{
    if (!ansi_cptable) return 1252;
    return ansi_cptable->info.codepage;
}


/***********************************************************************
 *              GetOEMCP   (KERNEL32.@)
 */
UINT WINAPI GetOEMCP(void)
{
    if (!oem_cptable) return 437;
    return oem_cptable->info.codepage;
}


/***********************************************************************
 *           IsValidCodePage   (KERNEL32.@)
 */
BOOL WINAPI IsValidCodePage( UINT codepage )
{
    switch(codepage) {
    case CP_SYMBOL:
        return FALSE;
    case CP_UTF7:
    case CP_UTF8:
        return TRUE;
    default:
        return wine_cp_get_table( codepage ) != NULL;
    }
}


/***********************************************************************
 *		GetUserDefaultLangID (KERNEL32.@)
 */
LANGID WINAPI GetUserDefaultLangID(void)
{
    return LANGIDFROMLCID(default_lcid);
}


/***********************************************************************
 *		GetSystemDefaultLangID (KERNEL32.@)
 */
LANGID WINAPI GetSystemDefaultLangID(void)
{
    return GetUserDefaultLangID();
}


/***********************************************************************
 *		GetUserDefaultLCID (KERNEL32.@)
 */
LCID WINAPI GetUserDefaultLCID(void)
{
    return default_lcid;
}


/***********************************************************************
 *		GetSystemDefaultLCID (KERNEL32.@)
 */
LCID WINAPI GetSystemDefaultLCID(void)
{
    return GetUserDefaultLCID();
}


/***********************************************************************
 *		GetUserDefaultUILanguage (KERNEL32.@)
 */
LANGID WINAPI GetUserDefaultUILanguage(void)
{
    return GetUserDefaultLangID();
}


/***********************************************************************
 *		GetSystemDefaultUILanguage (KERNEL32.@)
 */
LANGID WINAPI GetSystemDefaultUILanguage(void)
{
    return GetSystemDefaultLangID();
}


/***********************************************************************
 *           IsDBCSLeadByteEx   (KERNEL32.@)
 *
 * Determine if a character is a lead byte in a given code page.
 *
 * PARAMS
 *  codepage [I] Code page for the test.
 *  testchar [I] Character to test
 *
 * RETURNS
 *  TRUE, if testchar is a lead byte in codepage,
 *  FALSE otherwise.
 */
BOOL WINAPI IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const union cptable *table = get_codepage_table( codepage );
    return table && is_dbcs_leadbyte( table, testchar );
}


/***********************************************************************
 *           IsDBCSLeadByte   (KERNEL32.@)
 *           IsDBCSLeadByte   (KERNEL.207)
 *
 * Determine if a character is a lead byte.
 *
 * PARAMS
 *  testchar [I] Character to test
 *
 * RETURNS
 *  TRUE, if testchar is a lead byte in the Ansii code page,
 *  FALSE otherwise.
 */
BOOL WINAPI IsDBCSLeadByte( BYTE testchar )
{
    if (!ansi_cptable) return FALSE;
    return is_dbcs_leadbyte( ansi_cptable, testchar );
}


/***********************************************************************
 *           GetCPInfo   (KERNEL32.@)
 *
 * Get information about a code page.
 *
 * PARAMS
 *  codepage [I] Code page number
 *  cpinfo   [O] Destination for code page information
 *
 * RETURNS
 *  Success: TRUE. cpinfo is updated with the information about codepage.
 *  Failure: FALSE, if codepage is invalid or cpinfo is NULL.
 */
BOOL WINAPI GetCPInfo( UINT codepage, LPCPINFO cpinfo )
{
    const union cptable *table = get_codepage_table( codepage );

    if (!table)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (table->info.def_char & 0xff00)
    {
        cpinfo->DefaultChar[0] = table->info.def_char & 0xff00;
        cpinfo->DefaultChar[1] = table->info.def_char & 0x00ff;
    }
    else
    {
        cpinfo->DefaultChar[0] = table->info.def_char & 0xff;
        cpinfo->DefaultChar[1] = 0;
    }
    if ((cpinfo->MaxCharSize = table->info.char_size) == 2)
        memcpy( cpinfo->LeadByte, table->dbcs.lead_bytes, sizeof(cpinfo->LeadByte) );
    else
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;

    return TRUE;
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
    const union cptable *table = get_codepage_table( codepage );

    if (!GetCPInfo( codepage, (LPCPINFO)cpinfo ))
      return FALSE;

    cpinfo->CodePage = codepage;
    cpinfo->UnicodeDefaultChar = table->info.def_unicode_char;
    strcpy(cpinfo->CodePageName, table->info.name);
    return TRUE;
}

/***********************************************************************
 *           GetCPInfoExW   (KERNEL32.@)
 *
 * Unicode version of GetCPInfoExA.
 */
BOOL WINAPI GetCPInfoExW( UINT codepage, DWORD dwFlags, LPCPINFOEXW cpinfo )
{
    const union cptable *table = get_codepage_table( codepage );

    if (!GetCPInfo( codepage, (LPCPINFO)cpinfo ))
      return FALSE;

    cpinfo->CodePage = codepage;
    cpinfo->UnicodeDefaultChar = table->info.def_unicode_char;
    MultiByteToWideChar( CP_ACP, 0, table->info.name, -1, cpinfo->CodePageName,
                         sizeof(cpinfo->CodePageName)/sizeof(WCHAR));
    return TRUE;
}

/***********************************************************************
 *              EnumSystemCodePagesA   (KERNEL32.@)
 */
BOOL WINAPI EnumSystemCodePagesA( CODEPAGE_ENUMPROCA lpfnCodePageEnum, DWORD flags )
{
    const union cptable *table;
    char buffer[10];
    int index = 0;

    for (;;)
    {
        if (!(table = wine_cp_enum_table( index++ ))) break;
        sprintf( buffer, "%d", table->info.codepage );
        if (!lpfnCodePageEnum( buffer )) break;
    }
    return TRUE;
}


/***********************************************************************
 *              EnumSystemCodePagesW   (KERNEL32.@)
 */
BOOL WINAPI EnumSystemCodePagesW( CODEPAGE_ENUMPROCW lpfnCodePageEnum, DWORD flags )
{
    const union cptable *table;
    WCHAR buffer[10], *p;
    int page, index = 0;

    for (;;)
    {
        if (!(table = wine_cp_enum_table( index++ ))) break;
        p = buffer + sizeof(buffer)/sizeof(WCHAR);
        *--p = 0;
        page = table->info.codepage;
        do
        {
            *--p = '0' + (page % 10);
            page /= 10;
        } while( page );
        if (!lpfnCodePageEnum( p )) break;
    }
    return TRUE;
}


/***********************************************************************
 *              MultiByteToWideChar   (KERNEL32.@)
 *
 * Convert a multibyte character string into a Unicode string.
 *
 * PARAMS
 *   page   [I] Codepage character set to convert from
 *   flags  [I] Character mapping flags
 *   src    [I] Source string buffer
 *   srclen [I] Length of src, or -1 if src is NUL terminated
 *   dst    [O] Destination buffer
 *   dstlen [I] Length of dst, or 0 to compute the required length
 *
 * RETURNS
 *   Success: If dstlen > 0, the number of characters written to dst.
 *            If dstlen == 0, the number of characters needed to perform the
 *            conversion. In both cases the count includes the terminating NUL.
 *   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
 *            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
 *            and dstlen != 0; ERROR_INVALID_PARAMETER,  if an invalid parameter
 *            is passed, and ERROR_NO_UNICODE_TRANSLATION if no translation is
 *            possible for src.
 */
INT WINAPI MultiByteToWideChar( UINT page, DWORD flags, LPCSTR src, INT srclen,
                                LPWSTR dst, INT dstlen )
{
    const union cptable *table;
    int ret;

    if (!src || (!dst && dstlen))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = strlen(src) + 1;

    if (flags & MB_USEGLYPHCHARS) FIXME("MB_USEGLYPHCHARS not supported\n");

    switch(page)
    {
    case CP_UTF7:
        FIXME("UTF-7 not supported\n");
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    case CP_UNIXCP:
        if (unix_cptable)
        {
            ret = wine_cp_mbstowcs( unix_cptable, flags, src, srclen, dst, dstlen );
            break;
        }
        /* fall through */
    case CP_UTF8:
        ret = wine_utf8_mbstowcs( flags, src, srclen, dst, dstlen );
        break;
    default:
        if (!(table = get_codepage_table( page )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        ret = wine_cp_mbstowcs( table, flags, src, srclen, dst, dstlen );
        break;
    }

    if (ret < 0)
    {
        switch(ret)
        {
        case -1: SetLastError( ERROR_INSUFFICIENT_BUFFER ); break;
        case -2: SetLastError( ERROR_NO_UNICODE_TRANSLATION ); break;
        }
        ret = 0;
    }
    return ret;
}


/***********************************************************************
 *              WideCharToMultiByte   (KERNEL32.@)
 *
 * Convert a Unicode character string into a multibyte string.
 *
 * PARAMS
 *   page    [I] Code page character set to convert to
 *   flags   [I] Mapping Flags (MB_ constants from "winnls.h").
 *   src     [I] Source string buffer
 *   srclen  [I] Length of src, or -1 if src is NUL terminated
 *   dst     [O] Destination buffer
 *   dstlen  [I] Length of dst, or 0 to compute the required length
 *   defchar [I] Default character to use for conversion if no exact
 *		    conversion can be made
 *   used    [O] Set if default character was used in the conversion
 *
 * RETURNS
 *   Success: If dstlen > 0, the number of characters written to dst.
 *            If dstlen == 0, number of characters needed to perform the
 *            conversion. In both cases the count includes the terminating NUL.
 *   Failure: 0. Use GetLastError() to determine the cause. Possible errors are
 *            ERROR_INSUFFICIENT_BUFFER, if not enough space is available in dst
 *            and dstlen != 0, and ERROR_INVALID_PARAMETER, if an invalid
 *            parameter was given.
 */
INT WINAPI WideCharToMultiByte( UINT page, DWORD flags, LPCWSTR src, INT srclen,
                                LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
{
    const union cptable *table;
    int ret, used_tmp;

    if (!src || (!dst && dstlen))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = strlenW(src) + 1;

    switch(page)
    {
    case CP_UTF7:
        FIXME("UTF-7 not supported\n");
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    case CP_UNIXCP:
        if (unix_cptable)
        {
            ret = wine_cp_wcstombs( unix_cptable, flags, src, srclen, dst, dstlen,
                                    defchar, used ? &used_tmp : NULL );
            break;
        }
        /* fall through */
    case CP_UTF8:
        ret = wine_utf8_wcstombs( src, srclen, dst, dstlen );
        break;
    default:
        if (!(table = get_codepage_table( page )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        ret = wine_cp_wcstombs( table, flags, src, srclen, dst, dstlen,
                                defchar, used ? &used_tmp : NULL );
        if (used) *used = used_tmp;
        break;
    }

    if (ret == -1)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }
    return ret;
}
