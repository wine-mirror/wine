#ifndef __WINE_XMALLOC_H
#define __WINE_XMALLOC_H

void *xmalloc( int size );
void *xcalloc( int size );
void *xrealloc( void *ptr, int size );
char *xstrdup( const char *str );

#endif  /* __WINE_XMALLOC_H */
