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

#include "windef.h"
#include "wine/unicode.h"
#include "object.h"

static inline WCHAR *strdupW( const WCHAR *str )
{
    size_t len = (strlenW(str) + 1) * sizeof(WCHAR);
    return memdup( str, len );
}

extern int dump_strW( const WCHAR *str, size_t len, FILE *f, char escape[2] );

#endif  /* __WINE_SERVER_UNICODE_H */
