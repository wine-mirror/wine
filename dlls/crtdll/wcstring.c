/*
 * CRTDLL wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
 */

#include "config.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "crtdll.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(crtdll);


/*********************************************************************
 *           CRTDLL__wcsdup    (CRTDLL.320)
 */
LPWSTR __cdecl CRTDLL__wcsdup( LPCWSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        int size = (strlenW(str) + 1) * sizeof(WCHAR);
        ret = CRTDLL_malloc( size );
        if (ret) memcpy( ret, str, size );
    }
    return ret;
}


/*********************************************************************
 *           CRTDLL__wcsicoll    (CRTDLL.322)
 */
INT __cdecl CRTDLL__wcsicoll( LPCWSTR str1, LPCWSTR str2 )
{
    /* FIXME: handle collates */
    return strcmpiW( str1, str2 );
}


/*********************************************************************
 *           CRTDLL__wcsnset    (CRTDLL.325)
 */
LPWSTR __cdecl CRTDLL__wcsnset( LPWSTR str, WCHAR c, INT n )
{
    LPWSTR ret = str;
    while ((n-- > 0) && *str) *str++ = c;
    return ret;
}


/*********************************************************************
 *           CRTDLL__wcsrev    (CRTDLL.326)
 */
LPWSTR __cdecl CRTDLL__wcsrev( LPWSTR str )
{
    LPWSTR ret = str;
    LPWSTR end = str + strlenW(str) - 1;
    while (end > str)
    {
        WCHAR t = *end;
        *end--  = *str;
        *str++  = t;
    }
    return ret;
}


/*********************************************************************
 *           CRTDLL__wcsset    (CRTDLL.327)
 */
LPWSTR __cdecl CRTDLL__wcsset( LPWSTR str, WCHAR c )
{
    LPWSTR ret = str;
    while (*str) *str++ = c;
    return ret;
}


/*********************************************************************
 *           CRTDLL_wcscoll    (CRTDLL.506)
 */
DWORD __cdecl CRTDLL_wcscoll( LPCWSTR str1, LPCWSTR str2 )
{
    /* FIXME: handle collates */
    return strcmpW( str1, str2 );
}


/*********************************************************************
 *           CRTDLL_wcspbrk    (CRTDLL.514)
 */
LPWSTR __cdecl CRTDLL_wcspbrk( LPCWSTR str, LPCWSTR accept )
{
    LPCWSTR p;
    while (*str)
    {
        for (p = accept; *p; p++) if (*p == *str) return (LPWSTR)str;
        str++;
    }
    return NULL;
}


/*********************************************************************
 *           CRTDLL_wctomb    (CRTDLL.524)
 */
INT __cdecl CRTDLL_wctomb( LPSTR dst, WCHAR ch )
{
    return WideCharToMultiByte( CP_ACP, 0, ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *           CRTDLL_iswalnum    (CRTDLL.405)
 */
INT __cdecl CRTDLL_iswalnum( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *           CRTDLL_iswalpha    (CRTDLL.406)
 */
INT __cdecl CRTDLL_iswalpha( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *           CRTDLL_iswcntrl    (CRTDLL.408)
 */
INT __cdecl CRTDLL_iswcntrl( WCHAR wc )
{
    return get_char_typeW(wc) & C1_CNTRL;
}

/*********************************************************************
 *           CRTDLL_iswdigit    (CRTDLL.410)
 */
INT __cdecl CRTDLL_iswdigit( WCHAR wc )
{
    return get_char_typeW(wc) & C1_DIGIT;
}

/*********************************************************************
 *           CRTDLL_iswgraph    (CRTDLL.411)
 */
INT __cdecl CRTDLL_iswgraph( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *           CRTDLL_iswlower    (CRTDLL.412)
 */
INT __cdecl CRTDLL_iswlower( WCHAR wc )
{
    return get_char_typeW(wc) & C1_LOWER;
}

/*********************************************************************
 *           CRTDLL_iswprint    (CRTDLL.413)
 */
INT __cdecl CRTDLL_iswprint( WCHAR wc )
{
    return get_char_typeW(wc) & (C1_ALPHA|C1_BLANK|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *           CRTDLL_iswpunct    (CRTDLL.414)
 */
INT __cdecl CRTDLL_iswpunct( WCHAR wc )
{
    return get_char_typeW(wc) & C1_PUNCT;
}

/*********************************************************************
 *           CRTDLL_iswspace    (CRTDLL.415)
 */
INT __cdecl CRTDLL_iswspace( WCHAR wc )
{
    return get_char_typeW(wc) & C1_SPACE;
}

/*********************************************************************
 *           CRTDLL_iswupper    (CRTDLL.416)
 */
INT __cdecl CRTDLL_iswupper( WCHAR wc )
{
    return get_char_typeW(wc) & C1_UPPER;
}

/*********************************************************************
 *           CRTDLL_iswxdigit    (CRTDLL.417)
 */
INT __cdecl CRTDLL_iswxdigit( WCHAR wc )
{
    return get_char_typeW(wc) & C1_XDIGIT;
}
