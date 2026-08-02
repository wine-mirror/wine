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

#ifndef __WRC_UTILS_H
#define __WRC_UTILS_H

#include "wrctypes.h"

int compare_striA( const char *str1, const char *str2 );
int compare_striW( const WCHAR *str1, const WCHAR *str2 );
int parser_error(const char *s, ...) __attribute__((format (printf, 1, 2)));
int parser_warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void fatal_perror( const char *msg, ... ) __attribute__((format (printf, 1, 2), noreturn));
void error(const char *s, ...) __attribute__((format (printf, 1, 2), noreturn));
void warning(const char *s, ...) __attribute__((format (printf, 1, 2)));
void chat(const char *s, ...) __attribute__((format (printf, 1, 2)));

int compare_name_id(const name_id_t *n1, const name_id_t *n2);
const char *get_nameid_str(const name_id_t *n);
string_t *convert_string_unicode( const string_t *str, int codepage );
char *convert_string_utf8( const string_t *str, int codepage );
void free_string( string_t *str );
int check_valid_utf8( const string_t *str, int codepage );
int get_language_codepage( language_t lang );
language_t get_language_from_name( const char *name );
int is_valid_codepage(int cp);

#endif
