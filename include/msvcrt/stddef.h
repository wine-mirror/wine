/*
 * Time definitions
 *
 * Copyright 2000 Francois Gouget.
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
#ifndef __WINE_STDDEF_H
#define __WINE_STDDEF_H
#define __WINE_USE_MSVCRT

#include "winnt.h"


typedef int ptrdiff_t;

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

/* Best to leave this one alone: wchar_t */
#ifdef WINE_DEFINE_WCHAR_T
typedef unsigned short wchar_t;
#endif


#define offsetof(s,m)       (size_t)&(((s*)NULL)->m)


#ifdef __cplusplus
extern "C" {
#endif

unsigned long               __threadid();
unsigned long               __threadhandle();
#define _threadid          (__threadid())

#ifdef __cplusplus
}
#endif

#endif /* __WINE_STDDEF_H */
