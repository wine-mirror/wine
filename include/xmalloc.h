#ifndef __WINE_XMALLOC_H
#define __WINE_XMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

void *xmalloc( int size );
void *xcalloc( int size );
void *xrealloc( void *ptr, int size );
char *xstrdup( const char *str );

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_XMALLOC_H */
