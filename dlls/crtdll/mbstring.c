/*
 * CRTDLL multi-byte string functions
 *
 * Copyright 1999 Alexandre Julliard
 */

#include "windef.h"
#include "winbase.h"
#include "crtdll.h"


/*********************************************************************
 *           CRTDLL__mbsinc    (CRTDLL.205)
 */
LPSTR __cdecl CRTDLL__mbsinc( LPCSTR str )
{
    if (IsDBCSLeadByte( *str )) str++;
    return (LPSTR)(str + 1);
}


/*********************************************************************
 *           CRTDLL__mbslen    (CRTDLL.206)
 */
INT __cdecl CRTDLL__mbslen( LPCSTR str )
{
    INT len;
    for (len = 0; *str; len++, str++) if (IsDBCSLeadByte(str[0]) && str[1]) str++;
    return len;
}


/*********************************************************************
 *           CRTDLL_mbtowc    (CRTDLL.430)
 */
INT __cdecl CRTDLL_mbtowc( WCHAR *dst, LPCSTR str, INT n )
{
    if (n <= 0) return 0;
    if (!str) return 0;
    if (!MultiByteToWideChar( CP_ACP, 0, str, n, dst, 1 )) return 0;
    /* return the number of bytes from src that have been used */
    if (!*str) return 0;
    if (n >= 2 && IsDBCSLeadByte(*str) && str[1]) return 2;
    return 1;
}
