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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_SEARCH_H
#define __WINE_SEARCH_H
#define __WINE_USE_MSVCRT

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif


#ifdef __cplusplus
extern "C" {
#endif

void*       _lfind(const void*,const void*,unsigned int*,unsigned int,
                   int (*)(const void*,const void*));
void*       _lsearch(const void*,void*,unsigned int*,unsigned int,
                     int (*)(const void*,const void*));
void*       MSVCRT(bsearch)(const void*,const void*,MSVCRT(size_t),MSVCRT(size_t),
                            int (*)(const void*,const void*));
void        MSVCRT(qsort)(void*,MSVCRT(size_t),MSVCRT(size_t),
                          int (*)(const void*,const void*));

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define lfind _lfind
#define lsearch _lsearch
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SEARCH_H */
