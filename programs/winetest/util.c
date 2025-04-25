/*
 * Utility functions.
 *
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Ferenc Wagner
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

#include "winetest.h"

void *xalloc (size_t len)
{
    void *p = malloc( len );

    if (!p) report (R_FATAL, "Out of memory.");
    return p;
}

void *xrealloc (void *op, size_t len)
{
    void *p = realloc(op, len);

    if (len && !p) report (R_FATAL, "Out of memory.");
    return p;
}

char *xstrdup( const char *str )
{
    char* res = strdup( str );
    if (!res) report (R_FATAL, "Out of memory.");
    return res;
}

static char *vstrfmtmake( const char *fmt, va_list ap ) __WINE_PRINTF_ATTR(1,0);
static char *vstrfmtmake( const char *fmt, va_list ap )
{
    size_t size = 1000;
    char *p;
    int n;

    if (!fmt) fmt = "";
    p = xalloc(size);
    while (1) {
        n = vsnprintf (p, size, fmt, ap);
        if (n < 0) size *= 2;   /* Windows */
        else if ((unsigned)n >= size) size = n+1; /* glibc */
        else break;
        p = xrealloc(p, size);
    }
    return p;
}

char *vstrmake(va_list ap)
{
    const char *fmt;

    fmt = va_arg (ap, const char*);
    return vstrfmtmake (fmt, ap);
}

char *strmake( const char *fmt, ... )
{
    va_list ap;
    char *p;

    va_start( ap, fmt );
    p = vstrfmtmake( fmt, ap );
    va_end( ap );
    return p;
}

void output( HANDLE file, const char *fmt, ... )
{
    va_list ap;
    DWORD written;
    char *buffer, *head;

    va_start (ap, fmt);
    buffer = vstrfmtmake (fmt, ap);
    head = buffer;
    va_end (ap);
    while (head[0]) {
        if (!WriteFile( file, head, strlen(head), &written, NULL ))
            report (R_FATAL, "Can't write logs: %u", GetLastError());
        head += written;
    }
    free(buffer);
}

int
goodtagchar (char c)
{
    return (('a'<=c && c<='z') ||
            ('A'<=c && c<='Z') ||
            ('0'<=c && c<='9') ||
            c=='-' || c=='.');
}

const char *
findbadtagchar (const char *tag)
{
    while (*tag)
        if (goodtagchar (*tag)) tag++;
        else return tag;
    return NULL;
}
