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

#include <stdlib.h>
#include <string.h>
#include "xmalloc.h"
#include "debugtools.h"

void *xmalloc( size_t size )
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        MESSAGE("Virtual memory exhausted.\n");
        exit (1);
    }
    memset(res,0,size);
    return res;
}
