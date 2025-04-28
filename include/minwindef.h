/*
 * Basic types definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef _MINWINDEF_
#define _MINWINDEF_

#ifndef NO_STRICT
# ifndef STRICT
#  define STRICT
# endif /* STRICT */
#endif /* NO_STRICT */

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Calling conventions definitions */

#if (defined(__x86_64__) || defined(__powerpc64__) || defined(__aarch64__)) && !defined(_WIN64)
#define _WIN64
#endif

#ifndef _WIN64
# if defined(__i386__) && !defined(_X86_)
#  define _X86_
# endif
# if defined(_X86_) && !defined(__i386__)
#  define __i386__
# endif
#endif

#if !defined(_MSC_VER) && !defined(__MINGW32__)

#undef __stdcall
#undef __cdecl
#undef __fastcall
#undef __thiscall

#ifdef WINE_UNIX_LIB
# define __stdcall
# define __cdecl
#else
# if defined(__i386__) && defined(__GNUC__)
#  if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)) || defined(__APPLE__)
#   define __stdcall __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
#   define __cdecl __attribute__((__cdecl__)) __attribute__((__force_align_arg_pointer__))
#  else
#   define __stdcall __attribute__((__stdcall__))
#   define __cdecl __attribute__((__cdecl__))
#  endif
# elif defined(__x86_64__) && defined(__GNUC__)
#  if __has_attribute(__force_align_arg_pointer__)
#   define __stdcall __attribute__((ms_abi)) __attribute__((__force_align_arg_pointer__))
#  else
#   define __stdcall __attribute__((ms_abi))
#  endif
#  define __cdecl __stdcall
#  define __ms_va_list __builtin_ms_va_list
# else
#  define __stdcall
#  define __cdecl
# endif
# define __fastcall __stdcall
# define __thiscall __stdcall
#endif  /* WINE_UNIX_LIB */

#endif  /* _MSC_VER || __MINGW32__ */

#if !defined(__ms_va_list) && !defined(WINE_UNIX_LIB)
# define __ms_va_list va_list
#endif

#ifdef __WINESRC__
#define __ONLY_IN_WINELIB(x)	do_not_use_this_in_wine
#else
#define __ONLY_IN_WINELIB(x)	x
#endif

#ifndef _MSC_VER
#ifndef _stdcall
#define _stdcall    __ONLY_IN_WINELIB(__stdcall)
#endif
#ifndef _fastcall
#define _fastcall   __ONLY_IN_WINELIB(__stdcall)
#endif
#ifndef cdecl
#define cdecl       __ONLY_IN_WINELIB(__cdecl)
#endif
#ifndef _cdecl
#define _cdecl      __ONLY_IN_WINELIB(__cdecl)
#endif
#endif /* _MSC_VER */

#ifndef pascal
#define pascal      __ONLY_IN_WINELIB(__stdcall)
#endif
#ifndef _pascal
#define _pascal     __ONLY_IN_WINELIB(__stdcall)
#endif
#ifndef __export
#define __export    __ONLY_IN_WINELIB(__stdcall)
#endif
#ifndef near
#define near        __ONLY_IN_WINELIB(/* nothing */)
#endif
#ifndef far
#define far         __ONLY_IN_WINELIB(/* nothing */)
#endif
#ifndef _near
#define _near       __ONLY_IN_WINELIB(/* nothing */)
#endif
#ifndef _far
#define _far        __ONLY_IN_WINELIB(/* nothing */)
#endif
#ifndef NEAR
#define NEAR        __ONLY_IN_WINELIB(/* nothing */)
#endif
#ifndef FAR
#define FAR         __ONLY_IN_WINELIB(/* nothing */)
#endif

#ifndef _MSC_VER
# ifndef _declspec
#  define _declspec(x)    __ONLY_IN_WINELIB(/* nothing */)
# endif
# ifndef __declspec
#  define __declspec(x)   __ONLY_IN_WINELIB(/* nothing */)
# endif
#endif

#ifdef _MSC_VER
# define inline __inline
#endif

#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#define CDECL       __cdecl
#define _CDECL      __cdecl
#define APIENTRY    WINAPI
#define CONST       __ONLY_IN_WINELIB(const)
#ifndef WINAPIV
# define WINAPIV CDECL
#endif

/* Misc. constants. */

#undef NULL
#ifdef __cplusplus
#ifndef _WIN64
#define NULL 0
#else
#define NULL 0LL
#endif
#else
#define NULL  ((void*)0)
#endif

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE  1

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

/* Standard data types */

#ifndef BASETYPES
#define BASETYPES
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, *PUSHORT;
#if !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES) && !defined(WINE_UNIX_LIB)
typedef unsigned long ULONG, *PULONG;
#else
typedef unsigned int ULONG, *PULONG;
#endif
#endif

typedef void                                   *LPVOID;
typedef const void                             *LPCVOID;
typedef int             BOOL,       *PBOOL,    *LPBOOL;
typedef unsigned char   BYTE,       *PBYTE,    *LPBYTE;
typedef unsigned short  WORD,       *PWORD,    *LPWORD;
typedef int             INT,        *PINT,     *LPINT;
typedef unsigned int    UINT,       *PUINT;
typedef float           FLOAT,      *PFLOAT;
typedef char                        *PSZ;
#if !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES) && !defined(WINE_UNIX_LIB)
typedef long                                   *LPLONG;
typedef unsigned long   DWORD,      *PDWORD,   *LPDWORD;
#else
typedef int                                    *LPLONG;
typedef unsigned int    DWORD,      *PDWORD,   *LPDWORD;
#endif

/* Macros to map Winelib names to the correct implementation name */
/* Note that Winelib is purely Win32.                             */

#ifdef __WINESRC__
#define WINE_NO_UNICODE_MACROS 1
#endif

#ifdef WINE_NO_UNICODE_MACROS
# define WINELIB_NAME_AW(func) \
    func##_must_be_suffixed_with_W_or_A_in_this_context \
    func##_must_be_suffixed_with_W_or_A_in_this_context
#else  /* WINE_NO_UNICODE_MACROS */
# ifdef UNICODE
#  define WINELIB_NAME_AW(func) func##W
# else
#  define WINELIB_NAME_AW(func) func##A
# endif
#endif  /* WINE_NO_UNICODE_MACROS */

#ifdef WINE_NO_UNICODE_MACROS
# define DECL_WINELIB_TYPE_AW(type)  /* nothing */
#else
# define DECL_WINELIB_TYPE_AW(type)  typedef WINELIB_NAME_AW(type) type;
#endif

#include <winnt.h>

/* Polymorphic types */

typedef UINT_PTR        WPARAM;
typedef LONG_PTR        LPARAM;
typedef LONG_PTR        LRESULT;

typedef WORD            ATOM;

/* Handle types */

typedef int HFILE;
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;
DECLARE_HANDLE(HKL);
DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HWINSTA);

/* Handle types that must remain interchangeable even with strict on */

typedef HINSTANCE HMODULE;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
typedef HANDLE GLOBALHANDLE;
typedef HANDLE LOCALHANDLE;

/* Callback function pointers types */

#ifndef WINE_NO_STRICT_PROTOTYPES
typedef INT_PTR (CALLBACK *FARPROC)(void);
typedef INT_PTR (CALLBACK *NEARPROC)(void);
typedef INT_PTR (CALLBACK *PROC)(void);
#else
typedef INT_PTR (CALLBACK *FARPROC)();
typedef INT_PTR (CALLBACK *NEARPROC)();
typedef INT_PTR (CALLBACK *PROC)();
#endif

/* Macros to split words and longs. */

#define LOBYTE(w)              ((BYTE)((DWORD_PTR)(w) & 0xFF))
#define HIBYTE(w)              ((BYTE)((DWORD_PTR)(w) >> 8))

#define LOWORD(l)              ((WORD)((DWORD_PTR)(l) & 0xFFFF))
#define HIWORD(l)              ((WORD)((DWORD_PTR)(l) >> 16))

#define MAKEWORD(low,high)     ((WORD)(((BYTE)((DWORD_PTR)(low) & 0xFF)) | ((WORD)((BYTE)((DWORD_PTR)(high) & 0xFF))) << 8))
#define MAKELONG(low,high)     ((LONG)(((WORD)((DWORD_PTR)(low) & 0xFFFF)) | ((DWORD)((WORD)((DWORD_PTR)(high) & 0xFFFF))) << 16))

/* min and max macros */
#ifndef NOMINMAX
#ifndef max
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif
#endif  /* NOMINMAX */

#ifdef MAX_PATH /* Work-around for Mingw */
#undef MAX_PATH
#endif /* MAX_PATH */

#define MAX_PATH        260

typedef struct _FILETIME {
    DWORD  dwLowDateTime;
    DWORD  dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#define _FILETIME_

#ifdef __cplusplus
}
#endif

#endif /* _MINWINDEF_ */
