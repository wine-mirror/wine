/*
 * CRTDLL multi-byte string functions
 *
 * Copyright 1999 Alexandre Julliard
 */

#include "config.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif

#include "windef.h"
#include "crtdll.h"


/*********************************************************************
 *           CRTDLL__mbsinc    (CRTDLL.205)
 */
LPSTR __cdecl CRTDLL__mbsinc( LPCSTR str )
{
    int len = mblen( str, MB_LEN_MAX );
    if (len < 1) len = 1;
    return (LPSTR)(str + len);
}


/*********************************************************************
 *           CRTDLL__mbslen    (CRTDLL.206)
 */
INT __cdecl CRTDLL__mbslen( LPCSTR str )
{
    INT len, total = 0;
    while ((len = mblen( str, MB_LEN_MAX )) > 0)
    {
        str += len;
        total++;
    }
    return total;
}


/*********************************************************************
 *           CRTDLL_mbtowc    (CRTDLL.430)
 */
INT __cdecl CRTDLL_mbtowc( WCHAR *dst, LPCSTR str, INT n )
{
    wchar_t res;
    int ret = mbtowc( &res, str, n );
    if (dst) *dst = (WCHAR)res;
    return ret;
}
