/*
 * Definitions for Wine preloader
 *
 * Copyright (C) 2004 Mike McCormack for CodeWeavers
 * Copyright (C) 2004 Alexandre Julliard
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

#ifndef __WINE_LOADER_PRELOADER_H
#define __WINE_LOADER_PRELOADER_H

#include <stdarg.h>

#include "main.h"

static inline int wld_strcmp( const char *str1, const char *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline int wld_strncmp( const char *str1, const char *str2, size_t len )
{
    if (len <= 0) return 0;
    while ((--len > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline void *wld_memset( void *dest, int val, size_t len )
{
    char *dst = dest;
    while (len--) *dst++ = val;
    return dest;
}

extern int wld_vsprintf(char *buffer, const char *fmt, va_list args );
extern __attribute__((format(printf,1,2))) void wld_printf(const char *fmt, ... );
extern __attribute__((noreturn,format(printf,1,2))) void fatal_error(const char *fmt, ... );

extern void preload_reserve( const char *str, struct wine_preload_info *preload_info, size_t page_mask );

/* remove a range from the preload list */
static inline void remove_preload_range( int i, struct wine_preload_info *preload_info )
{
    while (preload_info[i].size)
    {
        preload_info[i].addr = preload_info[i+1].addr;
        preload_info[i].size = preload_info[i+1].size;
        i++;
    }
}

#endif /* __WINE_LOADER_PRELOADER_H */
