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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_BASETSD_H
#define __WINE_BASETSD_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*
 * Win32 was easy to implement under Unix since most (all?) 32-bit
 * Unices uses the same type model (ILP32) as Win32, where int, long
 * and pointer are 32-bit.
 *
 * Win64, however, will cause some problems when implemented under Unix.
 * Linux/{Alpha, Sparc64} and most (all?) other 64-bit Unices uses
 * the LP64 type model where int is 32-bit and long and pointer are
 * 64-bit. Win64 on the other hand uses the P64 (sometimes called LLP64)
 * type model where int and long are 32 bit and pointer is 64-bit.
 */

/* Type model indepent typedefs */

#ifndef _MSC_VER
typedef char      __int8;
typedef short     __int16;
typedef int       __int32;
#ifndef __MINGW__
typedef long long __int64;
#else
/* Using a typedef can tweak bugs in the C++ parser under Mingw32 */
#define __int64 long long
#endif /* !defined(__MINGW__) */
#endif /* !defined(_MSC_VER) */

typedef unsigned char      __uint8;
typedef unsigned short     __uint16;
typedef unsigned int       __uint32;

#ifndef _MSC_VER
typedef unsigned long long __uint64;
#else
typedef unsigned __int64 __uint64;
#endif /* !defined(_MSC_VER) */

#if defined(_WIN64)

typedef __uint32 __ptr32;
typedef void    *__ptr64;

#else /* FIXME: defined(_WIN32) */

typedef void    *__ptr32;
#ifndef _MSC_VER
typedef __uint64 __ptr64;
#endif /* !defined(_MSC_VER) */

#endif

/* Always signed and 32 bit wide */

typedef __int32 LONG32;
typedef __int32 INT32;

typedef LONG32 *PLONG32;
typedef INT32  *PINT32;

/* Always unsigned and 32 bit wide */

typedef __uint32 ULONG32;
typedef __uint32 DWORD32;
typedef __uint32 UINT32;

typedef ULONG32 *PULONG32;
typedef DWORD32 *PDWORD32;
typedef UINT32  *PUINT32;

/* Always signed and 64 bit wide */

typedef __int64 LONG64;
typedef __int64 INT64;

typedef LONG64 *PLONG64;
typedef INT64  *PINT64;

/* Always unsigned and 64 bit wide */

typedef __uint64 ULONG64;
typedef __uint64 DWORD64;
typedef __uint64 UINT64;

typedef ULONG64 *PULONG64;
typedef DWORD64 *PDWORD64;
typedef UINT64  *PUINT64;

/* Win32 or Win64 dependent typedef/defines. */

#ifdef _WIN64

typedef __int64 INT_PTR, *PINT_PTR;
typedef __uint64 UINT_PTR, *PUINT_PTR;

#define MAXINT_PTR 0x7fffffffffffffff
#define MININT_PTR 0x8000000000000000
#define MAXUINT_PTR 0xffffffffffffffff

typedef __int32 HALF_PTR, *PHALF_PTR;
typedef __int32 UHALF_PTR, *PUHALF_PTR;

#define MAXHALF_PTR 0x7fffffff
#define MINHALF_PTR 0x80000000
#define MAXUHALF_PTR 0xffffffff

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef __uint64 ULONG_PTR, *PULONG_PTR;
typedef __uint64 DWORD_PTR, *PDWORD_PTR;

#else /* FIXME: defined(_WIN32) */

typedef __int32 INT_PTR, *PINT_PTR;
typedef __uint32 UINT_PTR, *PUINT_PTR;

#define MAXINT_PTR 0x7fffffff
#define MININT_PTR 0x80000000
#define MAXUINT_PTR 0xffffffff

typedef __int16 HALF_PTR, *PHALF_PTR;
typedef __uint16 UHALF_PTR, *PUHALF_PTR;

#define MAXUHALF_PTR 0xffff
#define MAXHALF_PTR 0x7fff
#define MINHALF_PTR 0x8000

typedef __int32 LONG_PTR, *PLONG_PTR;
typedef __uint32 ULONG_PTR, *PULONG_PTR;
typedef __uint32 DWORD_PTR, *PDWORD_PTR;

#endif /* defined(_WIN64) || defined(_WIN32) */

typedef INT_PTR SSIZE_T, *PSSIZE_T;
typedef UINT_PTR SIZE_T, *PSIZE_T;

/* Some Wine-specific definitions */

/* Architecture dependent settings. */
/* These are hardcoded to avoid dependencies on config.h in Winelib apps. */
#if defined(__i386__)
# undef  WORDS_BIGENDIAN
# undef  BITFIELDS_BIGENDIAN
# define ALLOW_UNALIGNED_ACCESS
#elif defined(__sparc__)
# define WORDS_BIGENDIAN
# define BITFIELDS_BIGENDIAN
# undef  ALLOW_UNALIGNED_ACCESS
#elif defined(__PPC__)
# define WORDS_BIGENDIAN
# define BITFIELDS_BIGENDIAN
# undef  ALLOW_UNALIGNED_ACCESS
#elif !defined(RC_INVOKED)
# error Unknown CPU architecture!
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_BASETSD_H) */
