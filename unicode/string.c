/*
 * Unicode string manipulation functions
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "wine/unicode.h"

int strcmpiW( const WCHAR *str1, const WCHAR *str2 )
{
    for (;;)
    {
        int ret = toupperW(*str1) - toupperW(*str2);
        if (ret || !*str1) return ret;
        str1++;
        str2++;
    }
}

int strncmpiW( const WCHAR *str1, const WCHAR *str2, int n )
{
    int ret = 0;
    for ( ; n > 0; n--, str1++, str2++)
        if ((ret = toupperW(*str1) - toupperW(*str2)) || !*str1) break;
    return ret;
}

WCHAR *strstrW( const WCHAR *str, const WCHAR *sub )
{
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return (WCHAR *)str;
        str++;
    }
    return NULL;
}
