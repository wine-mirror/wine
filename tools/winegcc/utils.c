/*
 * Useful functions for winegcc/winewrap
 *
 * Copyright 2000 Francois Gouget
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 2003 Richard Cohen
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "utils.h"

#if !defined(min)
# define min(x,y) (((x) < (y)) ? (x) : (y))
#endif

int verbose = 0;

void error(const char *s, ...)
{
    va_list ap;
    
    va_start(ap, s);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

void *xmalloc(size_t size)
{
    void *p;
    if ((p = malloc (size)) == NULL)
	error("Can not malloc %d bytes.", size);

    return p;
}

void *xrealloc(void* p, size_t size)
{
    void* p2;
    if ((p2 = realloc (p, size)) == NULL)
	error("Can not realloc %d bytes.", size);

    return p2;
}

char *strmake(const char *fmt, ...)
{
    int n;
    size_t size = 100;
    char *p;
    va_list ap;
    p = xmalloc (size);

    while (1)
    {
        va_start(ap, fmt);
	n = vsnprintf (p, size, fmt, ap);
	va_end(ap);
        if (n > -1 && n < size) return p;
	size = min( size*2, n+1 );
	p = xrealloc (p, size);
    }
}

strarray *strarray_alloc(void)
{
    strarray *arr = xmalloc(sizeof(*arr));
    arr->maximum = arr->size = 0;
    arr->base = NULL;
    return arr;
}

void strarray_free(strarray* arr)
{
    free(arr->base);
    free(arr);
}

void strarray_add(strarray* arr, char* str)
{
    if (arr->size == arr->maximum)
    {
	arr->maximum += 10;
	arr->base = xrealloc(arr->base, sizeof(*(arr->base)) * arr->maximum);
    }
    arr->base[arr->size++] = str;
}

void spawn(strarray* arr)
{
    int i, status;
    char **argv = arr->base;

    if (verbose)
    {
	for(i = 0; argv[i]; i++) printf("%s ", argv[i]);
	printf("\n");
    }
    if (!(status = spawnvp( _P_WAIT, argv[0], argv))) return;

    if (status > 0) error("%s failed.", argv[0]);
    else perror("Error:");
    exit(3);
}
