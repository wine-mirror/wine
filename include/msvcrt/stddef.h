/*
 * Time definitions
 *
 * Copyright 2000 Francois Gouget.
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
