/*
 * Heap definitions
 *
 * Copyright 2001 Francois Gouget.
 */
#ifndef __WINE_SEARCH_H
#define __WINE_SEARCH_H
#define __WINE_USE_MSVCRT

#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*MSVCRT_compar_fn_t)(const void*,const void*);

void*       _lfind(const void*,const void*,unsigned int*,unsigned int,MSVCRT_compar_fn_t);
void*       _lsearch(const void*,void*,unsigned int*,unsigned int,MSVCRT_compar_fn_t);

void*       MSVCRT(bsearch)(const void*,const void*,MSVCRT(size_t),MSVCRT(size_t),MSVCRT_compar_fn_t);
void        MSVCRT(qsort)(void*,MSVCRT(size_t),MSVCRT(size_t),MSVCRT_compar_fn_t);

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define lfind _lfind
#define lsearch _lsearch
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SEARCH_H */
