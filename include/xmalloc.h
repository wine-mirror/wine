#ifndef __WINE_XMALLOC_H
#define __WINE_XMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

void *xmalloc( size_t size );
void *xcalloc( size_t size );
void *xrealloc( void *ptr, size_t size );
char *xstrdup( const char *str );

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_XMALLOC_H */
