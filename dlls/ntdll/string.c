/*
 * NTDLL string functions
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"

#include <ctype.h>
#include <string.h>

#include "windef.h"

/*********************************************************************
 *                  _memicmp   (NTDLL)
 */
INT __cdecl _memicmp( LPCSTR s1, LPCSTR s2, DWORD len )
{
    int ret = 0;
    while (len--)
    {
        if ((ret = tolower(*s1) - tolower(*s2))) break;
        s1++;
        s2++;
    }
    return ret;
}

/*********************************************************************
 *                  _strupr   (NTDLL)
 */
LPSTR __cdecl _strupr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = toupper(*str);
    return ret;
}

/*********************************************************************
 *                  _strlwr   (NTDLL)
 *
 * convert a string in place to lowercase 
 */
LPSTR __cdecl _strlwr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = tolower(*str);
    return ret;
}


/*********************************************************************
 *                  _ultoa   (NTDLL)
 */
LPSTR  __cdecl _ultoa( unsigned long x, LPSTR buf, INT radix )
{
    char buffer[32], *p;

    p = buffer + sizeof(buffer);
    *--p = 0;
    do
    {
        int rem = x % radix;
        *--p = (rem <= 9) ? rem + '0' : rem + 'a' - 10;
        x /= radix;
    } while (x);
    strcpy( buf, p + 1 );
    return buf;
}


/*********************************************************************
 *                  _ltoa   (NTDLL)
 */
LPSTR  __cdecl _ltoa( long x, LPSTR buf, INT radix )
{
    LPSTR p = buf;
    if (x < 0)
    {
        *p++ = '-';
        x = -x;
    }
    _ultoa( x, p, radix );
    return buf;
}


/*********************************************************************
 *                  _itoa           (NTDLL)
 */
LPSTR  __cdecl _itoa( int x, LPSTR buf, INT radix )
{
    return _ltoa( x, buf, radix );
}
