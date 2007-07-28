/*
 * Heap definitions
 *
 * Copyright 2001 Francois Gouget.
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
#ifndef __WINE_MALLOC_H
#define __WINE_MALLOC_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
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

#if defined(__x86_64__) && !defined(_WIN64)
#define _WIN64
#endif

#if !defined(_MSC_VER) && !defined(__int64)
# ifdef _WIN64
#   define __int64 long
# else
#   define __int64 long long
# endif
#endif

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _HEAPINFO_DEFINED
#define _HEAPINFO_DEFINED
typedef struct _heapinfo
{
  int*           _pentry;
  size_t _size;
  int            _useflag;
} _HEAPINFO;
#endif /* _HEAPINFO_DEFINED */

extern unsigned int* __p__amblksiz(void);
#define _amblksiz (*__p__amblksiz());

#ifdef __cplusplus
extern "C" {
#endif

void*       _expand(void*,size_t);
int         _heapadd(void*,size_t);
int         _heapchk(void);
int         _heapmin(void);
int         _heapset(unsigned int);
size_t _heapused(size_t*,size_t*);
int         _heapwalk(_HEAPINFO*);
size_t _msize(void*);

void*       calloc(size_t,size_t);
void        free(void*);
void*       malloc(size_t);
void*       realloc(void*,size_t);

void        _aligned_free(void*);
void*       _aligned_malloc(size_t,size_t);
void*       _aligned_offset_malloc(size_t,size_t,size_t);
void*       _aligned_realloc(void*,size_t,size_t);
void*       _aligned_offset_realloc(void*,size_t,size_t,size_t);

size_t _get_sbh_threshold(void);
int _set_sbh_threshold(size_t size);

#ifdef __cplusplus
}
#endif

# ifdef __GNUC__
# define _alloca(x) __builtin_alloca((x))
# define alloca(x) __builtin_alloca((x))
# endif

#endif /* __WINE_MALLOC_H */
