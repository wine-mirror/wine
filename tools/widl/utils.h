/*
 * Utility routines' prototypes etc.
 *
 * Copyright 1998 Bertho A. Stultiens (BS)
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

#ifndef __WIDL_UTILS_H
#define __WIDL_UTILS_H

#include "widltypes.h"
#include "parser.h"

void error(const char *s, ...) __attribute__((format (printf, 1, 2))) __attribute__((noreturn));
void error_at( const struct location *, const char *s, ... ) __attribute__((format( printf, 2, 3 ))) __attribute__((noreturn));
#define error_loc( ... ) error_at( NULL, ## __VA_ARGS__ )
void warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void warning_at( const struct location *, const char *s, ... ) __attribute__((format( printf, 2, 3 )));
#define warning_loc( ... ) warning_at( NULL, ## __VA_ARGS__ )
void chat(const char *s, ...) __attribute__((format (printf, 1, 2)));

struct strbuf
{
    char *buf;
    size_t pos;
    size_t len;
};
extern void strappend( struct strbuf *str, const char *fmt, ... ) __attribute__((__format__( __printf__, 2, 3 )));

size_t widl_getline(char **linep, size_t *lenp, FILE *fp);

/* buffer management */

extern void add_output_to_resources( const char *type, const char *name );
extern void flush_output_resources( const char *name );
extern void put_pword( unsigned int val );
extern void put_str( int indent, const char *format, ... ) __attribute__((format (printf, 2, 3)));
#endif
