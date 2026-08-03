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

#include "wmctypes.h"

int mcy_error(const char *s, ...) __attribute__((format (printf, 1, 2)));
int xyyerror(const char *s, ...) __attribute__((format (printf, 1, 2)));
int mcy_warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void internal_error(const char *file, int line, const char *s, ...) __attribute__((format (printf, 3, 4)));
void fatal_perror( const char *msg, ... ) __attribute__((format (printf, 1, 2), noreturn));
void error(const char *s, ...) __attribute__((format (printf, 1, 2)));
void warning(const char *s, ...) __attribute__((format (printf, 1, 2)));

WCHAR *xunistrdup(const WCHAR * str);
WCHAR *unistrcpy(WCHAR *dst, const WCHAR *src);
int unistrlen(const WCHAR *s);
int unistricmp(const WCHAR *s1, const WCHAR *s2);
int unistrcmp(const WCHAR *s1, const WCHAR *s2);
WCHAR *utf8_to_unicode( const char *src, int srclen, int *dstlen );
char *unicode_to_utf8( const WCHAR *src, int srclen, int *dstlen );
int is_valid_codepage(int id);
WCHAR *codepage_to_unicode( int codepage, const char *src, int srclen, int *dstlen );
unsigned int get_language_from_name( const char *name );

#endif
