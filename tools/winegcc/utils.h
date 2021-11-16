/*
 * Useful functions for winegcc
 *
 * Copyright 2000 Francois Gouget
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 2003 Richard Cohen
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

#include "../tools.h"

#ifndef DECLSPEC_NORETURN
# if defined(_MSC_VER) && (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#  define DECLSPEC_NORETURN __declspec(noreturn)
# elif defined(__GNUC__)
#  define DECLSPEC_NORETURN __attribute__((noreturn))
# else
#  define DECLSPEC_NORETURN
# endif
#endif

void DECLSPEC_NORETURN error(const char* s, ...);

typedef enum { 
    file_na, file_other, file_obj, file_res, file_rc, 
    file_arh, file_dll, file_so, file_def, file_spec
} file_type;

void create_file(const char* name, int mode, const char* fmt, ...);
file_type get_file_type(const char* filename);
file_type get_lib_type(struct target target, struct strarray path, const char *library,
                       const char *prefix, const char *suffix, char** file);
const char *find_binary( struct strarray prefix, const char *name );
int spawn(struct strarray prefix, struct strarray arr, int ignore_errors);

extern int verbose;
