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


#ifndef __GNUC__
#define __attribute__(X)
#endif

#ifndef DECLSPEC_NORETURN
# if defined(_MSC_VER) && (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#  define DECLSPEC_NORETURN __declspec(noreturn)
# elif defined(__GNUC__)
#  define DECLSPEC_NORETURN __attribute__((noreturn))
# else
#  define DECLSPEC_NORETURN
# endif
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum target_cpu
{
    CPU_x86, CPU_x86_64, CPU_POWERPC, CPU_ARM, CPU_ARM64
};

enum target_platform
{
    PLATFORM_UNSPECIFIED,
    PLATFORM_APPLE,
    PLATFORM_ANDROID,
    PLATFORM_SOLARIS,
    PLATFORM_WINDOWS,
    PLATFORM_MINGW,
    PLATFORM_CYGWIN
};

void DECLSPEC_NORETURN error(const char* s, ...);

void* xmalloc(size_t size);
void* xrealloc(void* p, size_t size);
char *xstrdup( const char *str );
char* strmake(const char* fmt, ...) __attribute__((__format__ (__printf__, 1, 2 )));
int strendswith(const char* str, const char* end);

struct strarray
{
    unsigned int count;  /* strings in use */
    unsigned int size;   /* total allocated size */
    const char **str;
};

static const struct strarray empty_strarray;

void strarray_add( struct strarray *array, const char *str );
void strarray_addall( struct strarray *array, struct strarray added );
struct strarray strarray_fromstring( const char *str, const char *delim );
char *strarray_tostring( struct strarray array, const char *sep );

typedef enum { 
    file_na, file_other, file_obj, file_res, file_rc, 
    file_arh, file_dll, file_so, file_def, file_spec
} file_type;

char* get_basename(const char* file);
void create_file(const char* name, int mode, const char* fmt, ...);
file_type get_file_type(const char* filename);
file_type get_lib_type(enum target_platform platform, struct strarray path, const char *library,
                       const char *prefix, const char *suffix, char** file);
const char *find_binary( struct strarray prefix, const char *name );
int spawn(struct strarray prefix, struct strarray arr, int ignore_errors);

extern int verbose;
