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

HANDLE logfile = 0;

void *heap_alloc (size_t len)
{
    void *p = HeapAlloc(GetProcessHeap(), 0, len);

    if (!p) report (R_FATAL, "Out of memory.");
    return p;
}

void *heap_realloc (void *op, size_t len)
{
    void *p = HeapReAlloc(GetProcessHeap(), 0, op, len);

    if (len && !p) report (R_FATAL, "Out of memory.");
    return p;
}

char *heap_strdup( const char *str )
{
    int len = strlen(str) + 1;
    char* res = HeapAlloc(GetProcessHeap(), 0, len);
    if (!res) report (R_FATAL, "Out of memory.");
    memcpy(res, str, len);
    return res;
}

void heap_free (void *op)
{
    HeapFree(GetProcessHeap(), 0, op);
}

static char *vstrfmtmake (size_t *lenp, const char *fmt, va_list ap)
{
    size_t size = 1000;
    char *p, *q;
    int n;

    p = HeapAlloc(GetProcessHeap(), 0, size);
    if (!p) return NULL;
    while (1) {
        n = vsnprintf (p, size, fmt, ap);
        if (n < 0) size *= 2;   /* Windows */
        else if ((unsigned)n >= size) size = n+1; /* glibc */
        else break;
        q = HeapReAlloc(GetProcessHeap(), 0, p, size);
        if (!q) {
          heap_free (p);
          return NULL;
       }
       p = q;
    }
    if (lenp) *lenp = n;
    return p;
}

char *vstrmake (size_t *lenp, va_list ap)
{
    const char *fmt;

    fmt = va_arg (ap, const char*);
    return vstrfmtmake (lenp, fmt, ap);
}

char * WINAPIV strmake (size_t *lenp, ...)
{
    va_list ap;
    char *p;

    va_start (ap, lenp);
    p = vstrmake (lenp, ap);
    if (!p) report (R_FATAL, "Out of memory.");
    va_end (ap);
    return p;
}

void WINAPIV xprintf (const char *fmt, ...)
{
    va_list ap;
    size_t size;
    DWORD written;
    char *buffer, *head;

    va_start (ap, fmt);
    buffer = vstrfmtmake (&size, fmt, ap);
    head = buffer;
    va_end (ap);
    while (size) {
        if (!WriteFile( logfile, head, size, &written, NULL ))
            report (R_FATAL, "Can't write logs: %u", GetLastError());
        head += written;
        size -= written;
    }
    heap_free (buffer);
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
