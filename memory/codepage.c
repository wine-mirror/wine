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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static LCID default_lcid = MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT );

/* setup default codepage info before we can get at the locale stuff */
static void init_codepages(void)
{
    ansi_cptable = cp_get_table( 1252 );
    oem_cptable  = cp_get_table( 437 );
    mac_cptable  = cp_get_table( 10000 );
    assert( ansi_cptable );
    assert( oem_cptable );
    assert( mac_cptable );
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
        ret = cp_get_table( codepage );
        break;
    }
    return ret;
}

/* initialize default code pages from locale info */
/* FIXME: should be done in init_codepages, but it can't right now */
/* since it needs KERNEL32 to be loaded for the locale info. */
void CODEPAGE_Init( UINT ansi, UINT oem, UINT mac, LCID lcid )
{
    extern void __wine_init_codepages( const union cptable *ansi, const union cptable *oem );
    const union cptable *table;

    default_lcid = lcid;
    if (!ansi_cptable) init_codepages();  /* just in case */

    if ((table = cp_get_table( ansi ))) ansi_cptable = table;
    if ((table = cp_get_table( oem ))) oem_cptable = table;
    if ((table = cp_get_table( mac ))) mac_cptable = table;
    __wine_init_codepages( ansi_cptable, oem_cptable );

    TRACE( "ansi=%03d oem=%03d mac=%03d\n", ansi_cptable->info.codepage,
           oem_cptable->info.codepage, mac_cptable->info.codepage );
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
        return cp_get_table( codepage ) != NULL;
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
 */
BOOL WINAPI IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const union cptable *table = get_codepage_table( codepage );
    return table && is_dbcs_leadbyte( table, testchar );
}


/***********************************************************************
 *           IsDBCSLeadByte   (KERNEL32.@)
 *           IsDBCSLeadByte   (KERNEL.207)
 */
BOOL WINAPI IsDBCSLeadByte( BYTE testchar )
{
    if (!ansi_cptable) return FALSE;
    return is_dbcs_leadbyte( ansi_cptable, testchar );
}


/***********************************************************************
 *           GetCPInfo   (KERNEL32.@)
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
 *              EnumSystemCodePagesA   (KERNEL32.@)
 */
BOOL WINAPI EnumSystemCodePagesA( CODEPAGE_ENUMPROCA lpfnCodePageEnum, DWORD flags )
{
    const union cptable *table;
    char buffer[10];
    int index = 0;

    for (;;)
    {
        if (!(table = cp_enum_table( index++ ))) break;
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
        if (!(table = cp_enum_table( index++ ))) break;
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
 * PARAMS
 *   page [in]    Codepage character set to convert from
 *   flags [in]   Character mapping flags
 *   src [in]     Source string buffer
 *   srclen [in]  Length of source string buffer
 *   dst [in]     Destination buffer
 *   dstlen [in]  Length of destination buffer
 *
 * NOTES
 *   The returned length includes the null terminator character.
 *
 * RETURNS
 *   Success: If dstlen > 0, number of characters written to destination
 *            buffer.  If dstlen == 0, number of characters needed to do
 *            conversion.
 *   Failure: 0. Occurs if not enough space is available.
 *
 * ERRORS
 *   ERROR_INSUFFICIENT_BUFFER
 *   ERROR_INVALID_PARAMETER
 *   ERROR_NO_UNICODE_TRANSLATION
 *
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
        FIXME("UTF not supported\n");
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    case CP_UTF8:
        ret = utf8_mbstowcs( flags, src, srclen, dst, dstlen );
        break;
    default:
        if (!(table = get_codepage_table( page )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        ret = cp_mbstowcs( table, flags, src, srclen, dst, dstlen );
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
 * PARAMS
 *   page [in]    Codepage character set to convert to
 *   flags [in]   Character mapping flags
 *   src [in]     Source string buffer
 *   srclen [in]  Length of source string buffer
 *   dst [in]     Destination buffer
 *   dstlen [in]  Length of destination buffer
 *   defchar [in] Default character to use for conversion if no exact
 *		    conversion can be made
 *   used [out]   Set if default character was used in the conversion
 *
 * NOTES
 *   The returned length includes the null terminator character.
 *
 * RETURNS
 *   Success: If dstlen > 0, number of characters written to destination
 *            buffer.  If dstlen == 0, number of characters needed to do
 *            conversion.
 *   Failure: 0. Occurs if not enough space is available.
 *
 * ERRORS
 *   ERROR_INSUFFICIENT_BUFFER
 *   ERROR_INVALID_PARAMETER
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
    case CP_UTF8:
        ret = utf8_wcstombs( src, srclen, dst, dstlen );
        break;
    default:
        if (!(table = get_codepage_table( page )))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        ret = cp_wcstombs( table, flags, src, srclen, dst, dstlen,
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
