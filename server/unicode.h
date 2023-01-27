/*
 * Unicode routines for use inside the server
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#ifndef __WINE_SERVER_UNICODE_H
#define __WINE_SERVER_UNICODE_H

#include <stdio.h>

#include "windef.h"
#include "object.h"

extern int memicmp_strW( const WCHAR *str1, const WCHAR *str2, data_size_t len );
extern unsigned int hash_strW( const WCHAR *str, data_size_t len, unsigned int hash_size );
extern WCHAR *ascii_to_unicode_str( const char *str, struct unicode_str *ret ) __WINE_DEALLOC(free) __WINE_MALLOC;
extern int parse_strW( WCHAR *buffer, data_size_t *len, const char *src, char endchar );
extern int dump_strW( const WCHAR *str, data_size_t len, FILE *f, const char escape[2] );
extern struct fd *load_intl_file(void);

#endif  /* __WINE_SERVER_UNICODE_H */
