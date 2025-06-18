/*
 * Compilers that uses ILP32, LP64 or P64 type models
 * for both Win32 and Win64 are supported by this file.
 *
 * Copyright (C) 1999 Patrik Stridvall
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

#ifndef _BASETSD_H_
#define _BASETSD_H_

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*
 * Win32 was easy to implement under Unix since most 32-bit Unices use the same
 * type model (ILP32) as Win32, where int, long and pointer are 32-bit.
 *
 * Win64, however, can cause some problems. Most 64-bit Unices use the LP64 type
 * model where int is 32-bit and long and pointer are 64-bit. Win64 on the other
 * hand uses the LLP64 type model where int and long are 32 bit and pointer is
 * 64-bit.
 */

#if (defined(__x86_64__) || defined(__powerpc64__) || defined(__aarch64__)) && !defined(_WIN64)
#define _WIN64
#endif

/* Type model independent typedefs */
/* The __intXX types are native types defined by the MS C compiler.
 * Apps that make use of them before they get defined here, can
 * simply add to the command line:
 *    -D__int8=char -D__int16=short -D__int32=int "-D__int64=long long"
 */
#if !defined(_MSC_VER) && !defined(__WIDL__)
#  ifndef __int8
#    define __int8  char
#  endif
#  ifndef __int16
#    define __int16 short
#  endif
#  ifndef __int32
#    define __int32 int
#  endif
#  ifndef __int64
#    if defined(_WIN64) && !defined(__MINGW64__)
#      define __int64 long
#    else
#      define __int64 long long
#    endif
#  endif
#endif /* !defined(_MSC_VER) */

#ifndef __has_declspec_attribute
# if defined(_MSC_VER)
#  define __has_declspec_attribute(x) 1
# else
#  define __has_declspec_attribute(x) 0
# endif
#endif

/* FIXME: DECLSPEC_ALIGN should be declared only in winnt.h, but we need it here too */
#ifndef DECLSPEC_ALIGN
# ifdef __GNUC__
#  define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
# elif __has_declspec_attribute(align) && !defined(MIDL_PASS)
#  define DECLSPEC_ALIGN(x) __declspec(align(x))
# else
#  define DECLSPEC_ALIGN(x)
# endif
#endif

typedef signed char      INT8, *PINT8;
typedef signed short     INT16, *PINT16;
typedef signed int       INT32, *PINT32;
typedef unsigned char    UINT8, *PUINT8;
typedef unsigned short   UINT16, *PUINT16;
typedef unsigned int     UINT32, *PUINT32;
typedef signed int       LONG32, *PLONG32;
typedef unsigned int     ULONG32, *PULONG32;
typedef unsigned int     DWORD32, *PDWORD32;

#ifdef _MSC_VER
typedef signed __int64   INT64, *PINT64;
typedef unsigned __int64 UINT64, *PUINT64;
typedef signed __int64   LONG64, *PLONG64;
typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;
#else
typedef signed __int64   DECLSPEC_ALIGN(8) INT64, *PINT64;
typedef unsigned __int64 DECLSPEC_ALIGN(8) UINT64, *PUINT64;
typedef signed __int64   DECLSPEC_ALIGN(8) LONG64, *PLONG64;
typedef unsigned __int64 DECLSPEC_ALIGN(8) ULONG64, *PULONG64;
typedef unsigned __int64 DECLSPEC_ALIGN(8) DWORD64, *PDWORD64;
#endif

/* Basic pointer-sized integer types */

#if defined(__midl) || defined(__WIDL__)

typedef /* [public] */ signed __int3264   INT_PTR, *PINT_PTR;
typedef /* [public] */ signed __int3264   LONG_PTR, *PLONG_PTR;
typedef /* [public] */ unsigned __int3264 UINT_PTR, *PUINT_PTR;
typedef /* [public] */ unsigned __int3264 ULONG_PTR, *PULONG_PTR;

#elif defined(_WIN64)

#define __int3264 __int64

typedef signed __int64   INT_PTR, *PINT_PTR;
typedef signed __int64   LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#else

#define __int3264 __int32

typedef int           INT_PTR, *PINT_PTR;
typedef unsigned int  UINT_PTR, *PUINT_PTR;
/* clang warns about long type in %I formats */
#if defined(__WINESRC__) && defined(__clang__) && (defined(__MINGW32__) || defined(_MSC_VER))
typedef int           LONG_PTR, *PLONG_PTR;
typedef unsigned int  ULONG_PTR, *PULONG_PTR;
#else
typedef long          LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif

#endif

/* Win32 or Win64 dependent typedef/defines. */

#ifdef _WIN64

#define MAXINT_PTR 0x7fffffffffffffff
#define MININT_PTR 0x8000000000000000
#define MAXUINT_PTR 0xffffffffffffffff

typedef int HALF_PTR, *PHALF_PTR;
typedef unsigned int UHALF_PTR, *PUHALF_PTR;

#define MAXHALF_PTR 0x7fffffff
#define MINHALF_PTR 0x80000000
#define MAXUHALF_PTR 0xffffffff

#if !defined(__midl) && !defined(__WIDL__)

#if !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES) && !defined(WINE_UNIX_LIB)

static inline unsigned long HandleToULong(const void *h)
{
    return (unsigned long)(ULONG_PTR)h;
}

static inline long HandleToLong(const void *h)
{
    return (long)(LONG_PTR)h;
}

static inline unsigned long PtrToUlong(const void *p)
{
    return (unsigned long)(ULONG_PTR)p;
}

static inline long PtrToLong(const void *p)
{
    return (long)(LONG_PTR)p;
}


#else

static inline unsigned HandleToULong(const void *h)
{
    return (unsigned)(ULONG_PTR)h;
}

static inline int HandleToLong(const void *h)
{
    return (int)(LONG_PTR)h;
}

static inline unsigned PtrToUlong(const void *p)
{
    return (unsigned)(ULONG_PTR)p;
}

static inline int PtrToLong(const void *p)
{
    return (int)(LONG_PTR)p;
}


#endif /* !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES) && !defined(WINE_UNIX_LIB) */

static inline void *ULongToHandle(ULONG32 ul)
{
    return (void *)(ULONG_PTR)ul;
}

static inline void *LongToHandle(LONG32 l)
{
    return (void *)(LONG_PTR)l;
}

static inline UINT32 PtrToUint(const void *p)
{
    return (UINT32)(UINT_PTR)p;
}

static inline INT32 PtrToInt(const void *p)
{
    return (INT32)(INT_PTR)p;
}

static inline UINT16 PtrToUshort(const void *p)
{
    return (UINT16)(ULONG_PTR)p;
}

static inline INT16 PtrToShort(const void *p)
{
    return (INT16)(LONG_PTR)p;
}

static inline void *IntToPtr(INT32 i)
{
    return (void *)(INT_PTR)i;
}

static inline void *UIntToPtr(UINT32 ui)
{
    return (void *)(UINT_PTR)ui;
}

static inline void *LongToPtr(LONG32 l)
{
    return (void *)(LONG_PTR)l;
}

static inline void *ULongToPtr(ULONG32 ul)
{
    return (void *)(ULONG_PTR)ul;
}

#endif  /* !__midl && !__WIDL__ */

#else /* FIXME: defined(_WIN32) */

#define MAXINT_PTR 0x7fffffff
#define MININT_PTR 0x80000000
#define MAXUINT_PTR 0xffffffff

typedef signed short HALF_PTR, *PHALF_PTR;
typedef unsigned short UHALF_PTR, *PUHALF_PTR;

#define MAXUHALF_PTR 0xffff
#define MAXHALF_PTR 0x7fff
#define MINHALF_PTR 0x8000

#define HandleToULong(h)        ((ULONG)(ULONG_PTR)(h))
#define HandleToLong(h)         ((LONG)(LONG_PTR)(h))
#define ULongToHandle(ul)       ((HANDLE)(ULONG_PTR)(ul))
#define LongToHandle(l)         ((HANDLE)(LONG_PTR)(l))
#define PtrToUlong(p)           ((ULONG)(ULONG_PTR)(p))
#define PtrToLong(p)            ((LONG)(LONG_PTR)(p))
#define PtrToUint(p)            ((UINT)(UINT_PTR)(p))
#define PtrToInt(p)             ((INT)(INT_PTR)(p))
#define PtrToUshort(p)          ((USHORT)(ULONG_PTR)(p))
#define PtrToShort(p)           ((SHORT)(LONG_PTR)(p))
#define IntToPtr(i)             ((void *)(INT_PTR)((INT)i))
#define UIntToPtr(ui)           ((void *)(UINT_PTR)((UINT)ui))
#define LongToPtr(l)            ((void *)(LONG_PTR)((LONG)l))
#define ULongToPtr(ul)          ((void *)(ULONG_PTR)((ULONG)ul))

#endif /* defined(_WIN64) || defined(_WIN32) */

#define HandleToUlong(h)        HandleToULong(h)
#define UlongToHandle(ul)       ULongToHandle(ul)
#define UintToPtr(ui)           UIntToPtr(ui)
#define UlongToPtr(ul)          ULongToPtr(ul)

typedef LONG_PTR SHANDLE_PTR;
typedef ULONG_PTR HANDLE_PTR;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;
typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef ULONG_PTR KAFFINITY, *PKAFFINITY;

#define MINLONGLONG             ((LONGLONG)~MAXLONGLONG)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(_BASETSD_H_) */
