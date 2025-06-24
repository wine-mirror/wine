/*
 * Definitions for Unix libraries
 *
 * Copyright (C) 2021 Alexandre Julliard
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

#ifndef __WINE_WINE_UNIXLIB_H
#define __WINE_WINE_UNIXLIB_H

#include "winternl.h"

typedef UINT64 unixlib_handle_t;

#ifdef WINE_UNIX_LIB

typedef NTSTATUS (*unixlib_entry_t)( void *args );

extern DECLSPEC_EXPORT const unixlib_entry_t __wine_unix_call_funcs[];
extern DECLSPEC_EXPORT const unixlib_entry_t __wine_unix_call_wow64_funcs[];

/* some useful private helpers from ntdll */

#ifdef __WINESRC__

NTSYSAPI const char *ntdll_get_build_dir(void);
NTSYSAPI const char *ntdll_get_data_dir(void);
NTSYSAPI NTSTATUS ntdll_get_dos_file_name( const char *unix_name, WCHAR **dos, UINT disposition );

/* exception handling */

#include <setjmp.h>

NTSYSAPI void ntdll_set_exception_jmp_buf( jmp_buf jmp );

#define __TRY \
    do { jmp_buf __jmp; \
         int __first = 1; \
         for (;;) if (!__first) \
         { \
             do {

#define __EXCEPT \
             } while(0); \
             ntdll_set_exception_jmp_buf( NULL ); \
             break; \
         } else { \
             if (setjmp( __jmp )) { \
                 do {

#define __ENDTRY \
                 } while (0); \
                 break; \
             } \
             ntdll_set_exception_jmp_buf( __jmp ); \
             __first = 0; \
         } \
    } while (0);

NTSYSAPI BOOLEAN KeAddSystemServiceTable( ULONG_PTR *funcs, ULONG_PTR *counters, ULONG limit,
                                          BYTE *arguments, ULONG index );
NTSYSAPI void ntdll_add_syscall_debug_info( UINT idx, const char **syscall_names,
                                            const char **usercall_names );

#endif  /* __WINESRC__ */

/* wide char string functions */

static inline int ntdll_iswspace( WCHAR wc )
{
    return ('\t' <= wc && wc <= '\r') || wc == ' ' || wc == 0xa0;
}

static inline size_t ntdll_wcslen( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline WCHAR *ntdll_wcscpy( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

static inline WCHAR *ntdll_wcscat( WCHAR *dst, const WCHAR *src )
{
    ntdll_wcscpy( dst + ntdll_wcslen(dst), src );
    return dst;
}

static inline int ntdll_wcscmp( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline int ntdll_wcsncmp( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline WCHAR *ntdll_wcschr( const WCHAR *str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return NULL;
}

static inline WCHAR *ntdll_wcsrchr( const WCHAR *str, WCHAR ch )
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}

static inline WCHAR *ntdll_wcspbrk( const WCHAR *str, const WCHAR *accept )
{
    for ( ; *str; str++) if (ntdll_wcschr( accept, *str )) return (WCHAR *)(ULONG_PTR)str;
    return NULL;
}

static inline SIZE_T ntdll_wcsspn( const WCHAR *str, const WCHAR *accept )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (!ntdll_wcschr( accept, *ptr )) break;
    return ptr - str;
}

static inline SIZE_T ntdll_wcscspn( const WCHAR *str, const WCHAR *reject )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (ntdll_wcschr( reject, *ptr )) break;
    return ptr - str;
}

static inline LONG ntdll_wcstol( const WCHAR *s, WCHAR **end, int base )
{
    BOOL negative = FALSE, empty = TRUE;
    LONG ret = 0;

    if (base < 0 || base == 1 || base > 36) return 0;
    if (end) *end = (WCHAR *)s;
    while (ntdll_iswspace(*s)) s++;

    if (*s == '-')
    {
        negative = TRUE;
        s++;
    }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        base = 16;
        s += 2;
    }
    if (base == 0) base = s[0] != '0' ? 10 : 8;

    while (*s)
    {
        int v;

        if ('0' <= *s && *s <= '9') v = *s - '0';
        else if ('A' <= *s && *s <= 'Z') v = *s - 'A' + 10;
        else if ('a' <= *s && *s <= 'z') v = *s - 'a' + 10;
        else break;
        if (v >= base) break;
        if (negative) v = -v;
        s++;
        empty = FALSE;

        if (!negative && (ret > MAXLONG / base || ret * base > MAXLONG - v))
            ret = MAXLONG;
        else if (negative && (ret < (LONG)MINLONG / base || ret * base < (LONG)(MINLONG - v)))
            ret = MINLONG;
        else
            ret = ret * base + v;
    }

    if (end && !empty) *end = (WCHAR *)s;
    return ret;
}

static inline ULONG ntdll_wcstoul( const WCHAR *s, WCHAR **end, int base )
{
    BOOL negative = FALSE, empty = TRUE;
    ULONG ret = 0;

    if (base < 0 || base == 1 || base > 36) return 0;
    if (end) *end = (WCHAR *)s;
    while (ntdll_iswspace(*s)) s++;

    if (*s == '-')
    {
        negative = TRUE;
        s++;
    }
    else if (*s == '+') s++;

    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        base = 16;
        s += 2;
    }
    if (base == 0) base = s[0] != '0' ? 10 : 8;

    while (*s)
    {
        int v;

        if ('0' <= *s && *s <= '9') v = *s - '0';
        else if ('A' <= *s && *s <= 'Z') v = *s - 'A' + 10;
        else if ('a' <= *s && *s <= 'z') v = *s - 'a' + 10;
        else break;
        if (v >= base) break;
        s++;
        empty = FALSE;

        if (ret > MAXDWORD / base || ret * base > MAXDWORD - v)
            ret = MAXDWORD;
        else
            ret = ret * base + v;
    }

    if (end && !empty) *end = (WCHAR *)s;
    return negative ? -ret : ret;
}

NTSYSAPI DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen );
NTSYSAPI int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict );
NTSYSAPI int ntdll_wcsicmp( const WCHAR *str1, const WCHAR *str2 );
NTSYSAPI int ntdll_wcsnicmp( const WCHAR *str1, const WCHAR *str2, int n );

#define iswspace(ch)       ntdll_iswspace(ch)
#define wcslen(str)        ntdll_wcslen(str)
#define wcscpy(dst,src)    ntdll_wcscpy(dst,src)
#define wcscat(dst,src)    ntdll_wcscat(dst,src)
#define wcscmp(s1,s2)      ntdll_wcscmp(s1,s2)
#define wcsncmp(s1,s2,n)   ntdll_wcsncmp(s1,s2,n)
#define wcschr(str,ch)     ntdll_wcschr(str,ch)
#define wcsrchr(str,ch)    ntdll_wcsrchr(str,ch)
#define wcspbrk(str,ac)    ntdll_wcspbrk(str,ac)
#define wcsspn(str,ac)     ntdll_wcsspn(str,ac)
#define wcscspn(str,rej)   ntdll_wcscspn(str,rej)
#define wcsicmp(s1, s2)    ntdll_wcsicmp(s1,s2)
#define wcsnicmp(s1, s2,n) ntdll_wcsnicmp(s1,s2,n)
#define wcstol(str,e,b)    ntdll_wcstol(str,e,b)
#define wcstoul(str,e,b)   ntdll_wcstoul(str,e,b)

#else /* WINE_UNIX_LIB */

extern unixlib_handle_t __wine_unixlib_handle;
extern NTSTATUS (WINAPI *__wine_unix_call_dispatcher)( unixlib_handle_t, unsigned int, void * );
extern NTSTATUS WINAPI __wine_init_unix_call(void);

#ifdef __arm64ec__
NTSTATUS __wine_unix_call_arm64ec( unixlib_handle_t handle, unsigned int code, void *args );
static inline NTSTATUS __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    return __wine_unix_call_arm64ec( handle, code, args );
}
#else
static inline NTSTATUS __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    return __wine_unix_call_dispatcher( handle, code, args );
}
#endif

#define WINE_UNIX_CALL(code,args) __wine_unix_call( __wine_unixlib_handle, (code), (args) )

#endif /* WINE_UNIX_LIB */

#endif  /* __WINE_WINE_UNIXLIB_H */
