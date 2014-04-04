/*
 * Copyright 2014 Dmitry Timoshkov
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

#ifndef __WINE_SCHEDSVC_PRIVATE_H__
#define __WINE_SCHEDSVC_PRIVATE_H__

#include "wine/unicode.h"

void schedsvc_auto_start(void) DECLSPEC_HIDDEN;

static void *heap_alloc_zero(SIZE_T size) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc_zero(SIZE_T size)
{
    void *ptr = MIDL_user_allocate(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

static void *heap_alloc(SIZE_T size) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc(SIZE_T size)
{
    return MIDL_user_allocate(size);
}

static void *heap_realloc(void *ptr, SIZE_T size) __WINE_ALLOC_SIZE(2);
static inline void *heap_realloc(void *ptr, SIZE_T size)
{
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

static inline void heap_free(void *ptr)
{
    MIDL_user_free(ptr);
}

static inline WCHAR *heap_strdupW(const WCHAR *src)
{
    WCHAR *dst;
    unsigned len;
    if (!src) return NULL;
    len = (strlenW(src) + 1) * sizeof(WCHAR);
    if ((dst = heap_alloc(len))) memcpy(dst, src, len);
    return dst;
}

#endif /* __WINE_SCHEDSVC_PRIVATE_H__ */
