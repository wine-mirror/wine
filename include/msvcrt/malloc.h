/*
 * Heap definitions
 *
 * Copyright 2001 Francois Gouget.
 */
#ifndef __WINE_MALLOC_H
#define __WINE_MALLOC_H

#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


/* heap function constants */
#define _HEAPEMPTY    -1
#define _HEAPOK       -2
#define _HEAPBADBEGIN -3
#define _HEAPBADNODE  -4
#define _HEAPEND      -5
#define _HEAPBADPTR   -6

#define _FREEENTRY     0
#define _USEDENTRY     1


#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

typedef struct _heapinfo
{
  int*           _pentry;
  MSVCRT(size_t) _size;
  int            _useflag;
} _HEAPINFO;


#ifdef __cplusplus
extern "C" {
#endif

void*       _alloca(MSVCRT(size_t));
void*       _expand(void*,MSVCRT(size_t));
int         _heapadd(void*,MSVCRT(size_t));
int         _heapchk(void);
int         _heapmin(void);
int         _heapset(unsigned int);
MSVCRT(size_t) _heapused(MSVCRT(size_t)*,MSVCRT(size_t)*);
int         _heapwalk(_HEAPINFO*);
MSVCRT(size_t) _msize(void*);

void*       MSVCRT(calloc)(MSVCRT(size_t),MSVCRT(size_t));
void        MSVCRT(free)(void*);
void*       MSVCRT(malloc)(MSVCRT(size_t));
void*       MSVCRT(realloc)(void*,MSVCRT(size_t));

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define alloca _alloca
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_MALLOC_H */
