/*
   xmalloc - a safe malloc

   Use this function instead of malloc whenever you don't intend to check
   the return value yourself, for instance because you don't have a good
   way to handle a zero return value.

   Typically, Wine's own memory requests should be handled by this function,
   while the clients should use malloc directly (and Wine should return an
   error to the client if allocation fails).

   Copyright 1995 by Morten Welinder.

*/

#include <stdio.h>
#include <string.h>
#include "xmalloc.h"

void *
xmalloc (size_t size)
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}


void *
xrealloc (void *ptr, size_t size)
{
    void *res = realloc (ptr, size);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}


char *xstrdup( const char *str )
{
    char *res = strdup( str );
    if (!res)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}
