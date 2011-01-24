/*
 * Utility routines' prototypes etc.
 *
 * Copyright 1998,2000 Bertho A. Stultiens (BS)
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

#ifndef __WMC_UTILS_H
#define __WMC_UTILS_H

#include <stddef.h>	/* size_t */

#include "wmctypes.h"

void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *str);

#ifndef __GNUC__
#define __attribute__(X)
#endif

char *strmake(const char* fmt, ...) __attribute__((__format__ (__printf__, 1, 2 )));
int mcy_error(const char *s, ...) __attribute__((format (printf, 1, 2)));
int xyyerror(const char *s, ...) __attribute__((format (printf, 1, 2)));
int mcy_warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void internal_error(const char *file, int line, const char *s, ...) __attribute__((format (printf, 3, 4)));
void error(const char *s, ...) __attribute__((format (printf, 1, 2)));
void warning(const char *s, ...) __attribute__((format (printf, 1, 2)));

char *dup_basename(const char *name, const char *ext);

WCHAR *xunistrdup(const WCHAR * str);
WCHAR *unistrcpy(WCHAR *dst, const WCHAR *src);
int unistrlen(const WCHAR *s);
int unistricmp(const WCHAR *s1, const WCHAR *s2);
int unistrcmp(const WCHAR *s1, const WCHAR *s2);

/* buffer management */

extern int byte_swapped;
extern unsigned char *output_buffer;
extern size_t output_buffer_pos;
extern size_t output_buffer_size;

extern void init_output_buffer(void);
extern void flush_output_buffer( const char *name );
extern void put_data( const void *data, size_t size );
extern void put_byte( unsigned char val );
extern void put_word( unsigned short val );
extern void put_dword( unsigned int val );
extern void align_output( unsigned int align );

#endif
