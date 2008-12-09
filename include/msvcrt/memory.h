/*
 * Memory definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_MEMORY_H
#define __WINE_MEMORY_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#if defined(__x86_64__) && !defined(_WIN64)
#define _WIN64
#endif

#if !defined(_MSC_VER) && !defined(__int64)
# if defined(_WIN64) && !defined(__MINGW64__)
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CRT_MEMORY_DEFINED
#define _CRT_MEMORY_DEFINED

void*       memchr(const void*,int,size_t);
int         memcmp(const void*,const void*,size_t);
void*       memcpy(void*,const void*,size_t);
void*       memset(void*,int,size_t);
void*       _memccpy(void*,const void*,int,unsigned int);
int         _memicmp(const void*,const void*,unsigned int);

static inline int memicmp(const void* s1, const void* s2, size_t len) { return _memicmp(s1, s2, len); }
static inline void* memccpy(void *s1, const void *s2, int c, size_t n) { return _memccpy(s1, s2, c, n); }

#endif /* _CRT_MEMORY_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MEMORY_H */
