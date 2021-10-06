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

typedef NTSTATUS (*unixlib_entry_t)( void *args );
typedef UINT64 unixlib_handle_t;

extern NTSTATUS WINAPI __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args );

#ifdef WINE_UNIX_LIB

/* some useful helpers from ntdll */
extern const char *ntdll_get_build_dir(void);
extern const char *ntdll_get_data_dir(void);
extern DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen );
extern int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict );
extern NTSTATUS ntdll_init_syscalls( ULONG id, SYSTEM_SERVICE_TABLE *table, void **dispatcher );

/* exception handling */

#ifdef __i386__
typedef struct { int reg[16]; } __wine_jmp_buf;
#elif defined(__x86_64__)
typedef struct { DECLSPEC_ALIGN(16) struct { unsigned __int64 Part[2]; } reg[16]; } __wine_jmp_buf;
#elif defined(__arm__)
typedef struct { int reg[28]; } __wine_jmp_buf;
#elif defined(__aarch64__)
typedef struct { __int64 reg[24]; } __wine_jmp_buf;
#else
typedef struct { int reg; } __wine_jmp_buf;
#endif

extern int __cdecl __attribute__ ((__nothrow__,__returns_twice__)) __wine_setjmpex( __wine_jmp_buf *buf,
                                                   EXCEPTION_REGISTRATION_RECORD *frame );
extern void DECLSPEC_NORETURN __cdecl __wine_longjmp( __wine_jmp_buf *buf, int retval );
extern void ntdll_set_exception_jmp_buf( __wine_jmp_buf *jmp );

#define __TRY \
    do { __wine_jmp_buf __jmp; \
         int __first = 1; \
         for (;;) if (!__first) \
         { \
             do {

#define __EXCEPT \
             } while(0); \
             ntdll_set_exception_jmp_buf( NULL ); \
             break; \
         } else { \
             if (__wine_setjmpex( &__jmp, NULL )) { \
                 do {

#define __ENDTRY \
                 } while (0); \
                 break; \
             } \
             ntdll_set_exception_jmp_buf( &__jmp ); \
             __first = 0; \
         } \
    } while (0);

#endif /* WINE_UNIX_LIB */

#endif  /* __WINE_WINE_UNIXLIB_H */
