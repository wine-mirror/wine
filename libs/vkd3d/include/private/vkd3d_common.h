/*
 * Copyright 2016 Józef Kucia for CodeWeavers
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

#ifndef __VKD3D_COMMON_H
#define __VKD3D_COMMON_H

#include "config.h"
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "vkd3d_types.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

#define DIV_ROUND_UP(a, b) ((a) % (b) == 0 ? (a) / (b) : (a) / (b) + 1)

#define STATIC_ASSERT(e) extern void __VKD3D_STATIC_ASSERT__(int [(e) ? 1 : -1])

#define MEMBER_SIZE(t, m) sizeof(((t *)0)->m)

#define VKD3D_MAKE_TAG(ch0, ch1, ch2, ch3) \
        ((uint32_t)(ch0) | ((uint32_t)(ch1) << 8) \
        | ((uint32_t)(ch2) << 16) | ((uint32_t)(ch3) << 24))

#define TAG_AON9 VKD3D_MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC VKD3D_MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_DXIL VKD3D_MAKE_TAG('D', 'X', 'I', 'L')
#define TAG_ISG1 VKD3D_MAKE_TAG('I', 'S', 'G', '1')
#define TAG_ISGN VKD3D_MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSG1 VKD3D_MAKE_TAG('O', 'S', 'G', '1')
#define TAG_OSG5 VKD3D_MAKE_TAG('O', 'S', 'G', '5')
#define TAG_OSGN VKD3D_MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_PCSG VKD3D_MAKE_TAG('P', 'C', 'S', 'G')
#define TAG_PSG1 VKD3D_MAKE_TAG('P', 'S', 'G', '1')
#define TAG_RD11 VKD3D_MAKE_TAG('R', 'D', '1', '1')
#define TAG_RDEF VKD3D_MAKE_TAG('R', 'D', 'E', 'F')
#define TAG_RTS0 VKD3D_MAKE_TAG('R', 'T', 'S', '0')
#define TAG_SDBG VKD3D_MAKE_TAG('S', 'D', 'B', 'G')
#define TAG_SHDR VKD3D_MAKE_TAG('S', 'H', 'D', 'R')
#define TAG_SHEX VKD3D_MAKE_TAG('S', 'H', 'E', 'X')
#define TAG_STAT VKD3D_MAKE_TAG('S', 'T', 'A', 'T')
#define TAG_TEXT VKD3D_MAKE_TAG('T', 'E', 'X', 'T')
#define TAG_XNAP VKD3D_MAKE_TAG('X', 'N', 'A', 'P')
#define TAG_XNAS VKD3D_MAKE_TAG('X', 'N', 'A', 'S')

static inline size_t align(size_t addr, size_t alignment)
{
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

#if defined(__GNUC__) || defined(__clang__)
# define VKD3D_NORETURN __attribute__((noreturn))
# ifdef __MINGW_PRINTF_FORMAT
#  define VKD3D_PRINTF_FUNC(fmt, args) __attribute__((format(__MINGW_PRINTF_FORMAT, fmt, args)))
# else
#  define VKD3D_PRINTF_FUNC(fmt, args) /* __attribute__((format(printf, fmt, args))) */
# endif
# define VKD3D_UNUSED __attribute__((unused))
# define VKD3D_UNREACHABLE __builtin_unreachable()
#else
# define VKD3D_NORETURN
# define VKD3D_PRINTF_FUNC(fmt, args)
# define VKD3D_UNUSED
# define VKD3D_UNREACHABLE (void)0
#endif  /* __GNUC__ */

VKD3D_NORETURN static inline void vkd3d_unreachable_(const char *filename, unsigned int line)
{
    fprintf(stderr, "%s:%u: Aborting, reached unreachable code.\n", filename, line);
    abort();
}

#ifdef NDEBUG
#define vkd3d_unreachable() VKD3D_UNREACHABLE
#else
#define vkd3d_unreachable() vkd3d_unreachable_(__FILE__, __LINE__)
#endif

static inline unsigned int vkd3d_popcount(unsigned int v)
{
#ifdef _MSC_VER
    return __popcnt(v);
#elif defined(__MINGW32__)
    return __builtin_popcount(v);
#else
    v -= (v >> 1) & 0x55555555;
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 24;
#endif
}

static inline bool vkd3d_bitmask_is_contiguous(unsigned int mask)
{
    unsigned int i, j;

    for (i = 0, j = 0; i < sizeof(mask) * CHAR_BIT; ++i)
    {
        if (mask & (1u << i))
            ++j;
        else if (j)
            break;
    }

    return vkd3d_popcount(mask) == j;
}

/* Undefined for x == 0. */
static inline unsigned int vkd3d_log2i(unsigned int x)
{
#ifdef _WIN32
    /* _BitScanReverse returns the index of the highest set bit,
     * unlike clz which is 31 - index. */
    ULONG result;
    _BitScanReverse(&result, x);
    return (unsigned int)result;
#elif defined(HAVE_BUILTIN_CLZ)
    return __builtin_clz(x) ^ 0x1f;
#else
    static const unsigned int l[] =
    {
        ~0u, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
          4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };
    unsigned int i;

    return (i = x >> 16) ? (x = i >> 8) ? l[x] + 24
            : l[i] + 16 : (i = x >> 8) ? l[i] + 8 : l[x];
#endif
}

static inline void *vkd3d_memmem( const void *haystack, size_t haystack_len, const void *needle, size_t needle_len)
{
    const char *str = haystack;

    while (haystack_len >= needle_len)
    {
        if (!memcmp(str, needle, needle_len))
            return (char *)str;
        ++str;
        --haystack_len;
    }
    return NULL;
}

static inline bool vkd3d_bound_range(size_t start, size_t count, size_t limit)
{
#ifdef HAVE_BUILTIN_ADD_OVERFLOW
    size_t sum;

    return !__builtin_add_overflow(start, count, &sum) && sum <= limit;
#else
    return start <= limit && count <= limit - start;
#endif
}

static inline bool vkd3d_object_range_overflow(size_t start, size_t count, size_t size)
{
    return (~(size_t)0 - start) / size < count;
}

static inline uint16_t vkd3d_make_u16(uint8_t low, uint8_t high)
{
    return low | ((uint16_t)high << 8);
}

static inline uint32_t vkd3d_make_u32(uint16_t low, uint16_t high)
{
    return low | ((uint32_t)high << 16);
}

static inline int vkd3d_u32_compare(uint32_t x, uint32_t y)
{
    return (x > y) - (x < y);
}

static inline bool bitmap_clear(uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] &= ~(1u << (idx & 0x1f));
}

static inline bool bitmap_set(uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] |= (1u << (idx & 0x1f));
}

static inline bool bitmap_is_set(const uint32_t *map, unsigned int idx)
{
    return map[idx >> 5] & (1u << (idx & 0x1f));
}

static inline int ascii_isupper(int c)
{
    return 'A' <= c && c <= 'Z';
}

static inline int ascii_tolower(int c)
{
    return ascii_isupper(c) ? c - 'A' + 'a' : c;
}

static inline int ascii_strncasecmp(const char *a, const char *b, size_t n)
{
    int c_a, c_b;

    while (n--)
    {
        c_a = ascii_tolower(*a++);
        c_b = ascii_tolower(*b++);
        if (c_a != c_b || !c_a)
            return c_a - c_b;
    }
    return 0;
}

static inline int ascii_strcasecmp(const char *a, const char *b)
{
    int c_a, c_b;

    do
    {
        c_a = ascii_tolower(*a++);
        c_b = ascii_tolower(*b++);
    } while (c_a == c_b && c_a != '\0');

    return c_a - c_b;
}

#ifndef _WIN32
# if HAVE_SYNC_ADD_AND_FETCH
static inline LONG InterlockedIncrement(LONG volatile *x)
{
    return __sync_add_and_fetch(x, 1);
}
static inline LONG64 InterlockedIncrement64(LONG64 volatile *x)
{
    return __sync_add_and_fetch(x, 1);
}
static inline LONG InterlockedAdd(LONG volatile *x, LONG val)
{
    return __sync_add_and_fetch(x, val);
}
# else
#  error "InterlockedIncrement() not implemented for this platform"
# endif  /* HAVE_SYNC_ADD_AND_FETCH */

# if HAVE_SYNC_SUB_AND_FETCH
static inline LONG InterlockedDecrement(LONG volatile *x)
{
    return __sync_sub_and_fetch(x, 1);
}
# else
#  error "InterlockedDecrement() not implemented for this platform"
# endif

#endif  /* _WIN32 */

static inline void vkd3d_parse_version(const char *version, int *major, int *minor)
{
    *major = atoi(version);

    while (isdigit(*version))
        ++version;
    if (*version == '.')
        ++version;

    *minor = atoi(version);
}

HRESULT hresult_from_vkd3d_result(int vkd3d_result);

#ifdef _WIN32
static inline void *vkd3d_dlopen(const char *name)
{
    return LoadLibraryA(name);
}

static inline void *vkd3d_dlsym(void *handle, const char *symbol)
{
    return GetProcAddress(handle, symbol);
}

static inline int vkd3d_dlclose(void *handle)
{
    return FreeLibrary(handle);
}

static inline const char *vkd3d_dlerror(void)
{
    unsigned int error = GetLastError();
    static char message[256];

    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, message, sizeof(message), NULL))
        return message;
    sprintf(message, "Unknown error %u.\n", error);
    return message;
}
#elif defined(HAVE_DLFCN_H)
#include <dlfcn.h>

static inline void *vkd3d_dlopen(const char *name)
{
    return dlopen(name, RTLD_NOW);
}

static inline void *vkd3d_dlsym(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}

static inline int vkd3d_dlclose(void *handle)
{
    return dlclose(handle);
}

static inline const char *vkd3d_dlerror(void)
{
    return dlerror();
}
#else
static inline void *vkd3d_dlopen(const char *name)
{
    return NULL;
}

static inline void *vkd3d_dlsym(void *handle, const char *symbol)
{
    return NULL;
}

static inline int vkd3d_dlclose(void *handle)
{
    return 0;
}

static inline const char *vkd3d_dlerror(void)
{
    return "Not implemented for this platform.\n";
}
#endif

#endif  /* __VKD3D_COMMON_H */
