#ifndef __WINE_XMALLOC_H
#define __WINE_XMALLOC_H

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#define size_t unsigned int
#endif

void *xmalloc (size_t);
void *xrealloc (void *, size_t);
char *xstrdup( const char * );

#endif  /* __WINE_XMALLOC_H */
