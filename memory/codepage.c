/*
 * Code page functions
 *
 * Copyright 2000 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>

#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(string);

/* current code pages */
static unsigned int ansi_cp = 1252;  /* Windows 3.1 ISO Latin */
static unsigned int oem_cp = 437;    /* MS-DOS United States */
static unsigned int mac_cp = 10000;  /* Mac Roman */

static const union cptable *ansi_cptable;
static const union cptable *oem_cptable;
static const union cptable *mac_cptable;


/* find the table for a given codepage, handling CP_ACP etc. pseudo-codepages */
static const union cptable *get_codepage_table( unsigned int codepage )
{
    const union cptable *ret = NULL;

    if (!ansi_cptable)  /* initialize them */
    {
        /* FIXME: should load from the registry */
        ansi_cptable = cp_get_table( ansi_cp );
        oem_cptable = cp_get_table( oem_cp );
        mac_cptable = cp_get_table( mac_cp );
        assert( ansi_cptable );
        assert( oem_cptable );
        assert( mac_cptable );
    }

    switch(codepage)
    {
    case CP_ACP:        return ansi_cptable;
    case CP_OEMCP:      return oem_cptable;
    case CP_MACCP:      return mac_cptable;
    case CP_THREAD_ACP: return ansi_cptable; /* FIXME */
    case CP_UTF7:
    case CP_UTF8:
        break;
    default:
        if (codepage == ansi_cp) return ansi_cptable;
        if (codepage == oem_cp) return oem_cptable;
        if (codepage == mac_cp) return mac_cptable;
        ret = cp_get_table( codepage );
        break;
    }
    return ret;
}

/******************************************************************************
 *              GetACP   (KERNEL32)
 *
 * RETURNS
 *    Current ANSI code-page identifier, default if no current defined
 */
UINT WINAPI GetACP(void)
{
    return ansi_cp;
}


/***********************************************************************
 *              GetOEMCP   (KERNEL32)
 */
UINT WINAPI GetOEMCP(void)
{
    return oem_cp;
}


/***********************************************************************
 *           IsValidCodePage   (KERNEL32)
 */
BOOL WINAPI IsValidCodePage( UINT codepage )
{
    return cp_get_table( codepage ) != NULL;
}


/***********************************************************************
 *           IsDBCSLeadByteEx   (KERNEL32)
 */
BOOL WINAPI IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const union cptable *table = get_codepage_table( codepage );
    return table && is_dbcs_leadbyte( table, testchar );
}


/***********************************************************************
 *           IsDBCSLeadByte   (KERNEL32)
 */
BOOL WINAPI IsDBCSLeadByte( BYTE testchar )
{
    return is_dbcs_leadbyte( ansi_cptable, testchar );
}


/***********************************************************************
 *           GetCPInfo   (KERNEL32)
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
 *              EnumSystemCodePagesA   (KERNEL32)
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
 *              EnumSystemCodePagesW   (KERNEL32)
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
 *              MultiByteToWideChar   (KERNEL32)
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

    if (srclen == -1) srclen = strlen(src) + 1;

    if (page >= CP_UTF7)
    {
        FIXME("UTF not supported\n");
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    }

    if (!(table = get_codepage_table( page )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (flags & MB_COMPOSITE) FIXME("MB_COMPOSITE not supported\n");
    if (flags & MB_USEGLYPHCHARS) FIXME("MB_USEGLYPHCHARS not supported\n");

    ret = cp_mbstowcs( table, flags, src, srclen, dst, dstlen );

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
 *              WideCharToMultiByte   (KERNEL32)
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

    if (srclen == -1) srclen = strlenW(src) + 1;

    if (page >= CP_UTF7)
    {
        FIXME("UTF not supported\n");
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    }

    if (!(table = get_codepage_table( page )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

/*    if (flags & WC_COMPOSITECHECK) FIXME( "WC_COMPOSITECHECK (%lx) not supported\n", flags );*/

    ret = cp_wcstombs( table, flags, src, srclen, dst, dstlen, defchar, used ? &used_tmp : NULL );
    if (used) *used = used_tmp;

    if (ret == -1)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }
    return ret;
}
