/*
 * Unicode routines for use inside the server
 *
 * Copyright (C) 1999 Alexandre Julliard
 */

#ifndef __WINE_SERVER_UNICODE_H
#define __WINE_SERVER_UNICODE_H

#ifndef __WINE_SERVER__
#error This file can only be used in the Wine server
#endif

#include "config.h"

#include <ctype.h>
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif

#include "windef.h"
#include "object.h"

static inline size_t strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline int strcmpW( const WCHAR *str1, const WCHAR *str2 ) 
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

#ifndef HAVE_WCTYPE_H
/* FIXME */
#define towupper(ch) (HIBYTE(ch) ? ch : (WCHAR)toupper(LOBYTE(ch)))
#endif

static inline int strcmpiW( const WCHAR *str1, const WCHAR *str2 ) 
{
    while (*str1 && (towupper(*str1) == towupper(*str2))) { str1++; str2++; }
    return towupper(*str1) - towupper(*str2);
}

static inline WCHAR *strcpyW( WCHAR *src, const WCHAR *dst ) 
{
    const WCHAR *ret = dst;
    while ((*src++ = *dst++));
    return (WCHAR *)ret;
}

static inline WCHAR *strdupW( const WCHAR *str )
{
    size_t len = (strlenW(str) + 1) * sizeof(WCHAR);
    return memdup( str, len );
}

extern int dump_strW( const WCHAR *str, size_t len, FILE *f, char escape[2] );

#endif  /* __WINE_SERVER_UNICODE_H */
