/*
 * Generate include file dependencies
 *
 * Copyright 1996, 2013 Alexandre Julliard
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

#include "config.h"
#define NO_LIBWINE_PORT
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/list.h"

struct incl_file
{
    struct list        entry;
    char              *name;
    char              *filename;
    char              *sourcename;    /* source file name for generated headers */
    void              *args;          /* custom arguments for makefile rule */
    struct incl_file  *included_by;   /* file that included this one */
    int                included_line; /* line where this file was included */
    unsigned int       flags;         /* flags (see below) */
    struct incl_file  *owner;
    unsigned int       files_count;   /* files in use */
    unsigned int       files_size;    /* total allocated size */
    struct incl_file **files;
};

#define FLAG_SYSTEM         0x000001  /* is it a system include (#include <name>) */
#define FLAG_GENERATED      0x000002  /* generated file */
#define FLAG_INSTALL        0x000004  /* file to install */
#define FLAG_IDL_PROXY      0x000100  /* generates a proxy (_p.c) file */
#define FLAG_IDL_CLIENT     0x000200  /* generates a client (_c.c) file */
#define FLAG_IDL_SERVER     0x000400  /* generates a server (_s.c) file */
#define FLAG_IDL_IDENT      0x000800  /* generates an ident (_i.c) file */
#define FLAG_IDL_REGISTER   0x001000  /* generates a registration (_r.res) file */
#define FLAG_IDL_TYPELIB    0x002000  /* generates a typelib (.tlb) file */
#define FLAG_IDL_REGTYPELIB 0x004000  /* generates a registered typelib (_t.res) file */
#define FLAG_IDL_HEADER     0x008000  /* generates a header (.h) file */
#define FLAG_RC_PO          0x010000  /* rc file contains translations */
#define FLAG_C_IMPLIB       0x020000 /* file is part of an import library */
#define FLAG_SFD_FONTS      0x040000 /* sfd file generated bitmap fonts */

static const struct
{
    unsigned int flag;
    const char *ext;
} idl_outputs[] =
{
    { FLAG_IDL_TYPELIB,    ".tlb" },
    { FLAG_IDL_REGTYPELIB, "_t.res" },
    { FLAG_IDL_CLIENT,     "_c.c" },
    { FLAG_IDL_IDENT,      "_i.c" },
    { FLAG_IDL_PROXY,      "_p.c" },
    { FLAG_IDL_SERVER,     "_s.c" },
    { FLAG_IDL_REGISTER,   "_r.res" },
    { FLAG_IDL_HEADER,     ".h" }
};

static struct list sources = LIST_INIT(sources);
static struct list includes = LIST_INIT(includes);

struct strarray
{
    unsigned int count;  /* strings in use */
    unsigned int size;   /* total allocated size */
    const char **str;
};

static const struct strarray empty_strarray;

/* variables common to all makefiles */
static struct strarray linguas;
static struct strarray dll_flags;
static struct strarray target_flags;
static struct strarray msvcrt_flags;
static struct strarray extra_cflags;
static struct strarray cpp_flags;
static struct strarray unwind_flags;
static struct strarray libs;
static struct strarray cmdline_vars;
static const char *root_src_dir;
static const char *tools_dir;
static const char *tools_ext;
static const char *exe_ext;
static const char *dll_ext;
static const char *man_ext;
static const char *dll_prefix;
static const char *crosstarget;
static const char *fontforge;
static const char *convert;
static const char *rsvg;
static const char *icotool;

struct makefile
{
    struct strarray vars;
    struct strarray include_args;
    struct strarray define_args;
    struct strarray appmode;
    struct strarray imports;
    struct strarray delayimports;
    struct strarray extradllflags;
    const char     *base_dir;
    const char     *src_dir;
    const char     *obj_dir;
    const char     *top_src_dir;
    const char     *top_obj_dir;
    const char     *parent_dir;
    const char     *module;
    const char     *testdll;
    const char     *staticlib;
    const char     *importlib;
    int             use_msvcrt;
    int             is_win16;
};

static struct makefile *top_makefile;

static const char *makefile_name = "Makefile";
static const char *Separator = "### Dependencies";
static const char *input_file_name;
static const char *output_file_name;
static const char *temp_file_name;
static int relative_dir_mode;
static int input_line;
static int output_column;
static FILE *output_file;

static const char Usage[] =
    "Usage: makedep [options] directories\n"
    "Options:\n"
    "   -R from to  Compute the relative path between two directories\n"
    "   -fxxx       Store output in file 'xxx' (default: Makefile)\n"
    "   -sxxx       Use 'xxx' as separator (default: \"### Dependencies\")\n";


#ifndef __GNUC__
#define __attribute__(x)
#endif

static void fatal_error( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static void fatal_perror( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static void output( const char *format, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static char *strmake( const char* fmt, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));

/*******************************************************************
 *         fatal_error
 */
static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (input_line) fprintf( stderr, "%d:", input_line );
        fprintf( stderr, " error: " );
    }
    else fprintf( stderr, "makedep: error: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}


/*******************************************************************
 *         fatal_perror
 */
static void fatal_perror( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (input_line) fprintf( stderr, "%d:", input_line );
        fprintf( stderr, " error: " );
    }
    else fprintf( stderr, "makedep: error: " );
    vfprintf( stderr, msg, valist );
    perror( " " );
    va_end( valist );
    exit(1);
}


/*******************************************************************
 *         cleanup_files
 */
static void cleanup_files(void)
{
    if (temp_file_name) unlink( temp_file_name );
    if (output_file_name) unlink( output_file_name );
}


/*******************************************************************
 *         exit_on_signal
 */
static void exit_on_signal( int sig )
{
    exit( 1 );  /* this will call the atexit functions */
}


/*******************************************************************
 *         xmalloc
 */
static void *xmalloc( size_t size )
{
    void *res;
    if (!(res = malloc (size ? size : 1)))
        fatal_error( "Virtual memory exhausted.\n" );
    return res;
}


/*******************************************************************
 *         xrealloc
 */
static void *xrealloc (void *ptr, size_t size)
{
    void *res;
    assert( size );
    if (!(res = realloc( ptr, size )))
        fatal_error( "Virtual memory exhausted.\n" );
    return res;
}

/*******************************************************************
 *         xstrdup
 */
static char *xstrdup( const char *str )
{
    char *res = strdup( str );
    if (!res) fatal_error( "Virtual memory exhausted.\n" );
    return res;
}


/*******************************************************************
 *         strmake
 */
static char *strmake( const char* fmt, ... )
{
    int n;
    size_t size = 100;
    va_list ap;

    for (;;)
    {
        char *p = xmalloc (size);
        va_start(ap, fmt);
        n = vsnprintf (p, size, fmt, ap);
        va_end(ap);
        if (n == -1) size *= 2;
        else if ((size_t)n >= size) size = n + 1;
        else return p;
        free(p);
    }
}


/*******************************************************************
 *         strendswith
 */
static int strendswith( const char* str, const char* end )
{
    int l = strlen(str);
    int m = strlen(end);

    return l >= m && strcmp(str + l - m, end) == 0;
}


/*******************************************************************
 *         output
 */
static void output( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = vfprintf( output_file, format, valist );
    va_end( valist );
    if (ret < 0) fatal_perror( "output" );
    if (format[0] && format[strlen(format) - 1] == '\n') output_column = 0;
    else output_column += ret;
}


/*******************************************************************
 *         strarray_add
 */
static void strarray_add( struct strarray *array, const char *str )
{
    if (array->count == array->size)
    {
	if (array->size) array->size *= 2;
        else array->size = 16;
	array->str = xrealloc( array->str, sizeof(array->str[0]) * array->size );
    }
    array->str[array->count++] = str;
}


/*******************************************************************
 *         strarray_addall
 */
static void strarray_addall( struct strarray *array, struct strarray added )
{
    unsigned int i;

    for (i = 0; i < added.count; i++) strarray_add( array, added.str[i] );
}


/*******************************************************************
 *         strarray_exists
 */
static int strarray_exists( struct strarray *array, const char *str )
{
    unsigned int i;

    for (i = 0; i < array->count; i++) if (!strcmp( array->str[i], str )) return 1;
    return 0;
}


/*******************************************************************
 *         strarray_add_uniq
 */
static void strarray_add_uniq( struct strarray *array, const char *str )
{
    if (!strarray_exists( array, str )) strarray_add( array, str );
}


/*******************************************************************
 *         strarray_get_value
 *
 * Find a value in a name/value pair string array.
 */
static char *strarray_get_value( const struct strarray *array, const char *name )
{
    unsigned int i;

    for (i = 0; i < array->count; i += 2)
        if (!strcmp( array->str[i], name )) return xstrdup( array->str[i + 1] );
    return NULL;
}


/*******************************************************************
 *         strarray_set_value
 *
 * Define a value in a name/value pair string array.
 */
static void strarray_set_value( struct strarray *array, const char *name, const char *value )
{
    unsigned int i;

    /* redefining a variable replaces the previous value */
    for (i = 0; i < array->count; i += 2)
    {
        if (strcmp( array->str[i], name )) continue;
        array->str[i + 1] = value;
        return;
    }
    strarray_add( array, name );
    strarray_add( array, value );
}


/*******************************************************************
 *         output_filename
 */
static void output_filename( const char *name )
{
    if (output_column + strlen(name) + 1 > 100)
    {
        output( " \\\n" );
        output( "  " );
    }
    else if (output_column) output( " " );
    output( "%s", name );
}


/*******************************************************************
 *         output_filenames
 */
static void output_filenames( struct strarray array )
{
    unsigned int i;

    for (i = 0; i < array.count; i++) output_filename( array.str[i] );
}


/*******************************************************************
 *         get_extension
 */
static char *get_extension( char *filename )
{
    char *ext = strrchr( filename, '.' );
    if (ext && strchr( ext, '/' )) ext = NULL;
    return ext;
}


/*******************************************************************
 *         replace_extension
 */
static char *replace_extension( const char *name, const char *old_ext, const char *new_ext )
{
    char *ret;
    int name_len = strlen( name );
    int ext_len = strlen( old_ext );

    if (name_len >= ext_len && !strcmp( name + name_len - ext_len, old_ext )) name_len -= ext_len;
    ret = xmalloc( name_len + strlen( new_ext ) + 1 );
    memcpy( ret, name, name_len );
    strcpy( ret + name_len, new_ext );
    return ret;
}


/*******************************************************************
 *         strarray_replace_extension
 */
static struct strarray strarray_replace_extension( const struct strarray *array,
                                                   const char *old_ext, const char *new_ext )
{
    unsigned int i;
    struct strarray ret;

    ret.count = ret.size = array->count;
    ret.str = xmalloc( sizeof(ret.str[0]) * ret.size );
    for (i = 0; i < array->count; i++) ret.str[i] = replace_extension( array->str[i], old_ext, new_ext );
    return ret;
}


/*******************************************************************
 *         replace_substr
 */
static char *replace_substr( const char *str, const char *start, unsigned int len, const char *replace )
{
    unsigned int pos = start - str;
    char *ret = xmalloc( pos + strlen(replace) + strlen(start + len) + 1 );
    memcpy( ret, str, pos );
    strcpy( ret + pos, replace );
    strcat( ret + pos, start + len );
    return ret;
}


/*******************************************************************
 *         get_relative_path
 *
 * Determine where the destination path is located relative to the 'from' path.
 */
static char *get_relative_path( const char *from, const char *dest )
{
    const char *start;
    char *ret, *p;
    unsigned int dotdots = 0;

    /* a path of "." is equivalent to an empty path */
    if (!strcmp( from, "." )) from = "";

    for (;;)
    {
        while (*from == '/') from++;
        while (*dest == '/') dest++;
        start = dest;  /* save start of next path element */
        if (!*from) break;

        while (*from && *from != '/' && *from == *dest) { from++; dest++; }
        if ((!*from || *from == '/') && (!*dest || *dest == '/')) continue;

        /* count remaining elements in 'from' */
        do
        {
            dotdots++;
            while (*from && *from != '/') from++;
            while (*from == '/') from++;
        }
        while (*from);
        break;
    }

    if (!start[0] && !dotdots) return NULL;  /* empty path */

    ret = xmalloc( 3 * dotdots + strlen( start ) + 1 );
    for (p = ret; dotdots; dotdots--, p += 3) memcpy( p, "../", 3 );

    if (start[0]) strcpy( p, start );
    else p[-1] = 0;  /* remove trailing slash */
    return ret;
}


/*******************************************************************
 *         concat_paths
 */
static char *concat_paths( const char *base, const char *path )
{
    if (!base) return xstrdup( path && path[0] ? path : "." );
    if (!path || !path[0]) return xstrdup( base );
    if (path[0] == '/') return xstrdup( path );
    return strmake( "%s/%s", base, path );
}


/*******************************************************************
 *         base_dir_path
 */
static char *base_dir_path( struct makefile *make, const char *path )
{
    return concat_paths( make->base_dir, path );
}


/*******************************************************************
 *         obj_dir_path
 */
static char *obj_dir_path( struct makefile *make, const char *path )
{
    return concat_paths( make->obj_dir, path );
}


/*******************************************************************
 *         src_dir_path
 */
static char *src_dir_path( struct makefile *make, const char *path )
{
    if (make->src_dir) return concat_paths( make->src_dir, path );
    return obj_dir_path( make, path );
}


/*******************************************************************
 *         top_obj_dir_path
 */
static char *top_obj_dir_path( struct makefile *make, const char *path )
{
    return concat_paths( make->top_obj_dir, path );
}


/*******************************************************************
 *         top_dir_path
 */
static char *top_dir_path( struct makefile *make, const char *path )
{
    if (make->top_src_dir) return concat_paths( make->top_src_dir, path );
    return top_obj_dir_path( make, path );
}


/*******************************************************************
 *         tools_dir_path
 */
static char *tools_dir_path( struct makefile *make, const char *path )
{
    if (tools_dir) return top_obj_dir_path( make, strmake( "%s/tools/%s", tools_dir, path ));
    return top_obj_dir_path( make, strmake( "tools/%s", path ));
}


/*******************************************************************
 *         tools_path
 */
static char *tools_path( struct makefile *make, const char *name )
{
    return strmake( "%s/%s%s", tools_dir_path( make, name ), name, tools_ext );
}


/*******************************************************************
 *         get_line
 */
static char *get_line( FILE *file )
{
    static char *buffer;
    static unsigned int size;

    if (!size)
    {
        size = 1024;
        buffer = xmalloc( size );
    }
    if (!fgets( buffer, size, file )) return NULL;
    input_line++;

    for (;;)
    {
        char *p = buffer + strlen(buffer);
        /* if line is larger than buffer, resize buffer */
        while (p == buffer + size - 1 && p[-1] != '\n')
        {
            buffer = xrealloc( buffer, size * 2 );
            if (!fgets( buffer + size - 1, size + 1, file )) break;
            p = buffer + strlen(buffer);
            size *= 2;
        }
        if (p > buffer && p[-1] == '\n')
        {
            *(--p) = 0;
            if (p > buffer && p[-1] == '\r') *(--p) = 0;
            if (p > buffer && p[-1] == '\\')
            {
                *(--p) = 0;
                /* line ends in backslash, read continuation line */
                if (!fgets( p, size - (p - buffer), file )) return buffer;
                input_line++;
                continue;
            }
        }
        return buffer;
    }
}

/*******************************************************************
 *         find_src_file
 */
static struct incl_file *find_src_file( const char *name )
{
    struct incl_file *file;

    LIST_FOR_EACH_ENTRY( file, &sources, struct incl_file, entry )
        if (!strcmp( name, file->name )) return file;
    return NULL;
}

/*******************************************************************
 *         find_include_file
 */
static struct incl_file *find_include_file( const char *name )
{
    struct incl_file *file;

    LIST_FOR_EACH_ENTRY( file, &includes, struct incl_file, entry )
        if (!strcmp( name, file->name )) return file;
    return NULL;
}

/*******************************************************************
 *         add_include
 *
 * Add an include file if it doesn't already exists.
 */
static struct incl_file *add_include( struct incl_file *parent, const char *name, int system )
{
    struct incl_file *include;
    char *ext;

    if (parent->files_count >= parent->files_size)
    {
        parent->files_size *= 2;
        if (parent->files_size < 16) parent->files_size = 16;
        parent->files = xrealloc( parent->files, parent->files_size * sizeof(*parent->files) );
    }

    /* enforce some rules for the Wine tree */

    if (!memcmp( name, "../", 3 ))
        fatal_error( "#include directive with relative path not allowed\n" );

    if (!strcmp( name, "config.h" ))
    {
        if ((ext = strrchr( parent->filename, '.' )) && !strcmp( ext, ".h" ))
            fatal_error( "config.h must not be included by a header file\n" );
        if (parent->files_count)
            fatal_error( "config.h must be included before anything else\n" );
    }
    else if (!strcmp( name, "wine/port.h" ))
    {
        if ((ext = strrchr( parent->filename, '.' )) && !strcmp( ext, ".h" ))
            fatal_error( "wine/port.h must not be included by a header file\n" );
        if (!parent->files_count) fatal_error( "config.h must be included before wine/port.h\n" );
        if (parent->files_count > 1)
            fatal_error( "wine/port.h must be included before everything except config.h\n" );
        if (strcmp( parent->files[0]->name, "config.h" ))
            fatal_error( "config.h must be included before wine/port.h\n" );
    }

    LIST_FOR_EACH_ENTRY( include, &includes, struct incl_file, entry )
        if (!strcmp( name, include->name )) goto found;

    include = xmalloc( sizeof(*include) );
    memset( include, 0, sizeof(*include) );
    include->name = xstrdup(name);
    include->included_by = parent;
    include->included_line = input_line;
    if (system) include->flags |= FLAG_SYSTEM;
    list_add_tail( &includes, &include->entry );
found:
    parent->files[parent->files_count++] = include;
    return include;
}


/*******************************************************************
 *         add_generated_source
 *
 * Add a generated source file to the list.
 */
static struct incl_file *add_generated_source( struct makefile *make,
                                               const char *name, const char *filename )
{
    struct incl_file *file;

    if ((file = find_src_file( name ))) return file;  /* we already have it */
    file = xmalloc( sizeof(*file) );
    memset( file, 0, sizeof(*file) );
    file->name = xstrdup( name );
    file->filename = obj_dir_path( make, filename ? filename : name );
    file->flags = FLAG_GENERATED;
    list_add_tail( &sources, &file->entry );
    return file;
}


/*******************************************************************
 *         open_file
 */
static FILE *open_file( struct makefile *make, const char *path )
{
    return fopen( base_dir_path( make, path ), "r" );
}

/*******************************************************************
 *         open_src_file
 */
static FILE *open_src_file( struct makefile *make, struct incl_file *pFile )
{
    FILE *file;

    /* try in source dir */
    pFile->filename = src_dir_path( make, pFile->name );
    file = open_file( make, pFile->filename );

    /* now try parent dir */
    if (!file && make->parent_dir)
    {
        pFile->filename = src_dir_path( make, strmake( "%s/%s", make->parent_dir, pFile->name ));
        file = open_file( make, pFile->filename );
    }
    if (!file) fatal_perror( "open %s", pFile->name );
    return file;
}


/*******************************************************************
 *         open_include_file
 */
static FILE *open_include_file( struct makefile *make, struct incl_file *pFile )
{
    FILE *file = NULL;
    char *filename, *p;
    unsigned int i, len;

    errno = ENOENT;

    /* check for generated bison header */

    if (strendswith( pFile->name, ".tab.h" ))
    {
        filename = src_dir_path( make, replace_extension( pFile->name, ".tab.h", ".y" ));
        if ((file = open_file( make, filename )))
        {
            pFile->sourcename = filename;
            pFile->filename = obj_dir_path( make, pFile->name );
            /* don't bother to parse it */
            fclose( file );
            return NULL;
        }
        free( filename );
    }

    /* check for corresponding idl file in source dir */

    if (strendswith( pFile->name, ".h" ))
    {
        filename = src_dir_path( make, replace_extension( pFile->name, ".h", ".idl" ));
        if ((file = open_file( make, filename )))
        {
            pFile->sourcename = filename;
            pFile->filename = obj_dir_path( make, pFile->name );
            return file;
        }
        free( filename );
    }

    /* now try in source dir */
    filename = src_dir_path( make, pFile->name );
    if ((file = open_file( make, filename ))) goto found;
    free( filename );

    /* now try in parent source dir */
    if (make->parent_dir)
    {
        filename = src_dir_path( make, strmake( "%s/%s", make->parent_dir, pFile->name ));
        if ((file = open_file( make, filename ))) goto found;
        free( filename );
    }

    /* check for corresponding idl file in global includes */

    if (strendswith( pFile->name, ".h" ))
    {
        filename = top_dir_path( make, strmake( "include/%s",
                                                replace_extension( pFile->name, ".h", ".idl" )));
        if ((file = open_file( make, filename )))
        {
            pFile->sourcename = filename;
            pFile->filename = top_obj_dir_path( make, strmake( "include/%s", pFile->name ));
            return file;
        }
        free( filename );
    }

    /* check for corresponding .in file in global includes (for config.h.in) */

    if (strendswith( pFile->name, ".h" ))
    {
        filename = top_dir_path( make, strmake( "include/%s",
                                                replace_extension( pFile->name, ".h", ".h.in" )));
        if ((file = open_file( make, filename )))
        {
            pFile->sourcename = filename;
            pFile->filename = top_obj_dir_path( make, strmake( "include/%s", pFile->name ));
            return file;
        }
        free( filename );
    }

    /* check for corresponding .x file in global includes */

    if (strendswith( pFile->name, "tmpl.h" ))
    {
        filename = top_dir_path( make, strmake( "include/%s",
                                                replace_extension( pFile->name, ".h", ".x" )));
        if ((file = open_file( make, filename )))
        {
            pFile->sourcename = filename;
            pFile->filename = top_obj_dir_path( make, strmake( "include/%s", pFile->name ));
            return file;
        }
        free( filename );
    }

    /* check in global includes source dir */

    filename = top_dir_path( make, strmake( "include/%s", pFile->name ));
    if ((file = open_file( make, filename ))) goto found;

    /* check in global msvcrt includes */
    if (make->use_msvcrt)
    {
        filename = top_dir_path( make, strmake( "include/msvcrt/%s", pFile->name ));
        if ((file = open_file( make, filename ))) goto found;
    }

    /* now search in include paths */
    for (i = 0; i < make->include_args.count; i++)
    {
        const char *dir = make->include_args.str[i] + 2;  /* skip -I */
        if (*dir == '/')
        {
            /* ignore absolute paths that don't point into the source dir */
            if (!make->top_src_dir) continue;
            len = strlen( make->top_src_dir );
            if (strncmp( dir, make->top_src_dir, len )) continue;
            if (dir[len] && dir[len] != '/') continue;
        }
        filename = strmake( "%s/%s", dir, pFile->name );
        if ((file = open_file( make, filename ))) goto found;
        free( filename );
    }
    if (pFile->flags & FLAG_SYSTEM) return NULL;  /* ignore system files we cannot find */

    /* try in src file directory */
    if ((p = strrchr(pFile->included_by->filename, '/')))
    {
        int l = p - pFile->included_by->filename + 1;
        filename = xmalloc(l + strlen(pFile->name) + 1);
        memcpy( filename, pFile->included_by->filename, l );
        strcpy( filename + l, pFile->name );
        if ((file = open_file( make, filename ))) goto found;
        free( filename );
    }

    fprintf( stderr, "%s:%d: error: ", pFile->included_by->filename, pFile->included_line );
    perror( pFile->name );
    pFile = pFile->included_by;
    while (pFile && pFile->included_by)
    {
        const char *parent = pFile->included_by->sourcename;
        if (!parent) parent = pFile->included_by->name;
        fprintf( stderr, "%s:%d: note: %s was first included here\n",
                 parent, pFile->included_line, pFile->name );
        pFile = pFile->included_by;
    }
    exit(1);

found:
    pFile->filename = filename;
    return file;
}


/*******************************************************************
 *         parse_include_directive
 */
static void parse_include_directive( struct incl_file *source, char *str )
{
    char quote, *include, *p = str;

    while (*p && isspace(*p)) p++;
    if (*p != '\"' && *p != '<' ) return;
    quote = *p++;
    if (quote == '<') quote = '>';
    include = p;
    while (*p && (*p != quote)) p++;
    if (!*p) fatal_error( "malformed include directive '%s'\n", str );
    *p = 0;
    add_include( source, include, (quote == '>') );
}


/*******************************************************************
 *         parse_pragma_directive
 */
static void parse_pragma_directive( struct incl_file *source, char *str )
{
    char *flag, *p = str;

    if (!isspace( *p )) return;
    while (*p && isspace(*p)) p++;
    p = strtok( p, " \t" );
    if (strcmp( p, "makedep" )) return;

    while ((flag = strtok( NULL, " \t" )))
    {
        if (!strcmp( flag, "depend" ))
        {
            while ((p = strtok( NULL, " \t" ))) add_include( source, p, 0 );
            return;
        }
        else if (!strcmp( flag, "install" )) source->flags |= FLAG_INSTALL;

        if (strendswith( source->name, ".idl" ))
        {
            if (!strcmp( flag, "header" )) source->flags |= FLAG_IDL_HEADER;
            else if (!strcmp( flag, "proxy" )) source->flags |= FLAG_IDL_PROXY;
            else if (!strcmp( flag, "client" )) source->flags |= FLAG_IDL_CLIENT;
            else if (!strcmp( flag, "server" )) source->flags |= FLAG_IDL_SERVER;
            else if (!strcmp( flag, "ident" )) source->flags |= FLAG_IDL_IDENT;
            else if (!strcmp( flag, "typelib" )) source->flags |= FLAG_IDL_TYPELIB;
            else if (!strcmp( flag, "register" )) source->flags |= FLAG_IDL_REGISTER;
            else if (!strcmp( flag, "regtypelib" )) source->flags |= FLAG_IDL_REGTYPELIB;
        }
        else if (strendswith( source->name, ".rc" ))
        {
            if (!strcmp( flag, "po" )) source->flags |= FLAG_RC_PO;
        }
        else if (strendswith( source->name, ".sfd" ))
        {
            if (!strcmp( flag, "font" ))
            {
                struct strarray *array = source->args;

                if (!array)
                {
                    source->args = array = xmalloc( sizeof(*array) );
                    *array = empty_strarray;
                    source->flags |= FLAG_SFD_FONTS;
                }
                strarray_add( array, xstrdup( strtok( NULL, "" )));
                return;
            }
        }
        else if (!strcmp( flag, "implib" )) source->flags |= FLAG_C_IMPLIB;
    }
}


/*******************************************************************
 *         parse_cpp_directive
 */
static void parse_cpp_directive( struct incl_file *source, char *str )
{
    while (*str && isspace(*str)) str++;
    if (*str++ != '#') return;
    while (*str && isspace(*str)) str++;

    if (!strncmp( str, "include", 7 ))
        parse_include_directive( source, str + 7 );
    else if (!strncmp( str, "import", 6 ) && strendswith( source->name, ".m" ))
        parse_include_directive( source, str + 6 );
    else if (!strncmp( str, "pragma", 6 ))
        parse_pragma_directive( source, str + 6 );
}


/*******************************************************************
 *         parse_idl_file
 *
 * If for_h_file is non-zero, it means we are not interested in the idl file
 * itself, but only in the contents of the .h file that will be generated from it.
 */
static void parse_idl_file( struct incl_file *pFile, FILE *file, int for_h_file )
{
    char *buffer, *include;

    input_line = 0;
    if (for_h_file)
    {
        /* generated .h file always includes these */
        add_include( pFile, "rpc.h", 1 );
        add_include( pFile, "rpcndr.h", 1 );
    }

    while ((buffer = get_line( file )))
    {
        char quote;
        char *p = buffer;
        while (*p && isspace(*p)) p++;

        if (!strncmp( p, "import", 6 ))
        {
            p += 6;
            while (*p && isspace(*p)) p++;
            if (*p != '"') continue;
            include = ++p;
            while (*p && (*p != '"')) p++;
            if (!*p) fatal_error( "malformed import directive\n" );
            *p = 0;
            if (for_h_file && strendswith( include, ".idl" )) strcpy( p - 4, ".h" );
            add_include( pFile, include, 0 );
            continue;
        }

        if (for_h_file)  /* only check for #include inside cpp_quote */
        {
            if (strncmp( p, "cpp_quote", 9 )) continue;
            p += 9;
            while (*p && isspace(*p)) p++;
            if (*p++ != '(') continue;
            while (*p && isspace(*p)) p++;
            if (*p++ != '"') continue;
            if (*p++ != '#') continue;
            while (*p && isspace(*p)) p++;
            if (strncmp( p, "include", 7 )) continue;
            p += 7;
            while (*p && isspace(*p)) p++;
            if (*p == '\\' && p[1] == '"')
            {
                p += 2;
                quote = '"';
            }
            else
            {
                if (*p++ != '<' ) continue;
                quote = '>';
            }
            include = p;
            while (*p && (*p != quote)) p++;
            if (!*p || (quote == '"' && p[-1] != '\\'))
                fatal_error( "malformed #include directive inside cpp_quote\n" );
            if (quote == '"') p--;  /* remove backslash */
            *p = 0;
            add_include( pFile, include, (quote == '>') );
            continue;
        }

        parse_cpp_directive( pFile, p );
    }
}

/*******************************************************************
 *         parse_c_file
 */
static void parse_c_file( struct incl_file *pFile, FILE *file )
{
    char *buffer;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        parse_cpp_directive( pFile, buffer );
    }
}


/*******************************************************************
 *         parse_rc_file
 */
static void parse_rc_file( struct incl_file *pFile, FILE *file )
{
    char *buffer, *include;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        char quote;
        char *p = buffer;
        while (*p && isspace(*p)) p++;

        if (p[0] == '/' && p[1] == '*')  /* check for magic makedep comment */
        {
            p += 2;
            while (*p && isspace(*p)) p++;
            if (strncmp( p, "@makedep:", 9 )) continue;
            p += 9;
            while (*p && isspace(*p)) p++;
            quote = '"';
            if (*p == quote)
            {
                include = ++p;
                while (*p && *p != quote) p++;
            }
            else
            {
                include = p;
                while (*p && !isspace(*p) && *p != '*') p++;
            }
            if (!*p)
                fatal_error( "malformed makedep comment\n" );
            *p = 0;
            add_include( pFile, include, (quote == '>') );
            continue;
        }

        parse_cpp_directive( pFile, buffer );
    }
}


/*******************************************************************
 *         parse_in_file
 */
static void parse_in_file( struct incl_file *source, FILE *file )
{
    char *p, *buffer;

    /* make sure it gets rebuilt when the version changes */
    add_include( source, "config.h", 1 );

    if (!strendswith( source->filename, ".man.in" )) return;  /* not a man page */

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        if (strncmp( buffer, ".TH", 3 )) continue;
        if (!(p = strtok( buffer, " \t" ))) continue;  /* .TH */
        if (!(p = strtok( NULL, " \t" ))) continue;  /* program name */
        if (!(p = strtok( NULL, " \t" ))) continue;  /* man section */
        source->args = xstrdup( p );
        return;
    }
}


/*******************************************************************
 *         parse_sfd_file
 */
static void parse_sfd_file( struct incl_file *source, FILE *file )
{
    char *p, *eol, *buffer;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        if (strncmp( buffer, "UComments:", 10 )) continue;
        p = buffer + 10;
        while (*p == ' ') p++;
        if (p[0] == '"' && p[1] && buffer[strlen(buffer) - 1] == '"')
        {
            p++;
            buffer[strlen(buffer) - 1] = 0;
        }
        while ((eol = strstr( p, "+AAoA" )))
        {
            *eol = 0;
            while (*p && isspace(*p)) p++;
            if (*p++ == '#')
            {
                while (*p && isspace(*p)) p++;
                if (!strncmp( p, "pragma", 6 )) parse_pragma_directive( source, p + 6 );
            }
            p = eol + 5;
        }
        while (*p && isspace(*p)) p++;
        if (*p++ != '#') return;
        while (*p && isspace(*p)) p++;
        if (!strncmp( p, "pragma", 6 )) parse_pragma_directive( source, p + 6 );
        return;
    }
}


/*******************************************************************
 *         parse_file
 */
static void parse_file( struct makefile *make, struct incl_file *source, int src )
{
    FILE *file;

    /* don't try to open certain types of files */
    if (strendswith( source->name, ".tlb" ))
    {
        source->filename = obj_dir_path( make, source->name );
        return;
    }

    file = src ? open_src_file( make, source ) : open_include_file( make, source );
    if (!file) return;
    input_file_name = source->filename;

    if (source->sourcename && strendswith( source->sourcename, ".idl" ))
        parse_idl_file( source, file, 1 );
    else if (strendswith( source->filename, ".idl" ))
        parse_idl_file( source, file, 0 );
    else if (strendswith( source->filename, ".c" ) ||
             strendswith( source->filename, ".m" ) ||
             strendswith( source->filename, ".h" ) ||
             strendswith( source->filename, ".l" ) ||
             strendswith( source->filename, ".y" ))
        parse_c_file( source, file );
    else if (strendswith( source->filename, ".rc" ))
        parse_rc_file( source, file );
    else if (strendswith( source->filename, ".in" ))
        parse_in_file( source, file );
    else if (strendswith( source->filename, ".sfd" ))
        parse_sfd_file( source, file );
    fclose(file);
    input_file_name = NULL;
}


/*******************************************************************
 *         add_src_file
 *
 * Add a source file to the list.
 */
static struct incl_file *add_src_file( struct makefile *make, const char *name )
{
    struct incl_file *file;

    if ((file = find_src_file( name ))) return file;  /* we already have it */
    file = xmalloc( sizeof(*file) );
    memset( file, 0, sizeof(*file) );
    file->name = xstrdup(name);
    list_add_tail( &sources, &file->entry );
    parse_file( make, file, 1 );
    return file;
}


/*******************************************************************
 *         get_make_variable
 */
static char *get_make_variable( struct makefile *make, const char *name )
{
    char *ret;

    if ((ret = strarray_get_value( &cmdline_vars, name ))) return ret;
    if ((ret = strarray_get_value( &make->vars, name ))) return ret;
    if (top_makefile && (ret = strarray_get_value( &top_makefile->vars, name ))) return ret;
    return NULL;
}


/*******************************************************************
 *         get_expanded_make_variable
 */
static char *get_expanded_make_variable( struct makefile *make, const char *name )
{
    char *p, *end, *var, *expand, *tmp;

    expand = get_make_variable( make, name );
    if (!expand) return NULL;

    p = expand;
    while ((p = strchr( p, '$' )))
    {
        if (p[1] == '(')
        {
            if (!(end = strchr( p + 2, ')' ))) fatal_error( "syntax error in '%s'\n", expand );
            *end++ = 0;
            if (strchr( p + 2, ':' )) fatal_error( "pattern replacement not supported for '%s'\n", p + 2 );
            var = get_make_variable( make, p + 2 );
            tmp = replace_substr( expand, p, end - p, var ? var : "" );
            free( var );
        }
        else if (p[1] == '{')  /* don't expand ${} variables */
        {
            if (!(end = strchr( p + 2, '}' ))) fatal_error( "syntax error in '%s'\n", expand );
            p = end + 1;
            continue;
        }
        else if (p[1] == '$')
        {
            tmp = replace_substr( expand, p, 2, "$" );
        }
        else fatal_error( "syntax error in '%s'\n", expand );

        /* switch to the new string */
        p = tmp + (p - expand);
        free( expand );
        expand = tmp;
    }

    /* consider empty variables undefined */
    p = expand;
    while (*p && isspace(*p)) p++;
    if (*p) return expand;
    free( expand );
    return NULL;
}


/*******************************************************************
 *         get_expanded_make_var_array
 */
static struct strarray get_expanded_make_var_array( struct makefile *make, const char *name )
{
    struct strarray ret = empty_strarray;
    char *value, *token;

    if ((value = get_expanded_make_variable( make, name )))
        for (token = strtok( value, " \t" ); token; token = strtok( NULL, " \t" ))
            strarray_add( &ret, token );
    return ret;
}


/*******************************************************************
 *         set_make_variable
 */
static int set_make_variable( struct strarray *array, const char *assignment )
{
    char *p, *name;

    p = name = xstrdup( assignment );
    while (isalnum(*p) || *p == '_') p++;
    if (name == p) return 0;  /* not a variable */
    if (isspace(*p))
    {
        *p++ = 0;
        while (isspace(*p)) p++;
    }
    if (*p != '=') return 0;  /* not an assignment */
    *p++ = 0;
    while (isspace(*p)) p++;

    strarray_set_value( array, name, p );
    return 1;
}


/*******************************************************************
 *         parse_makefile
 */
static struct makefile *parse_makefile( const char *path, const char *separator )
{
    char *buffer;
    FILE *file;
    struct makefile *make = xmalloc( sizeof(*make) );

    memset( make, 0, sizeof(*make) );
    if (path)
    {
        make->top_obj_dir = get_relative_path( path, "" );
        make->base_dir = path;
        if (!strcmp( make->base_dir, "." )) make->base_dir = NULL;
    }

    input_file_name = base_dir_path( make, makefile_name );
    if (!(file = fopen( input_file_name, "r" ))) fatal_perror( "open" );

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        if (separator && !strncmp( buffer, separator, strlen(separator) )) break;
        if (*buffer == '\t') continue;  /* command */
        while (isspace( *buffer )) buffer++;
        if (*buffer == '#') continue;  /* comment */
        set_make_variable( &make->vars, buffer );
    }
    fclose( file );
    input_file_name = NULL;
    return make;
}


/*******************************************************************
 *         add_generated_sources
 */
static void add_generated_sources( struct makefile *make )
{
    struct incl_file *source, *next, *file;

    LIST_FOR_EACH_ENTRY_SAFE( source, next, &sources, struct incl_file, entry )
    {
        if (source->flags & FLAG_IDL_CLIENT)
        {
            file = add_generated_source( make, replace_extension( source->name, ".idl", "_c.c" ), NULL );
            add_include( file, replace_extension( source->name, ".idl", ".h" ), 0 );
        }
        if (source->flags & FLAG_IDL_SERVER)
        {
            file = add_generated_source( make, replace_extension( source->name, ".idl", "_s.c" ), NULL );
            add_include( file, "wine/exception.h", 0 );
            add_include( file, replace_extension( source->name, ".idl", ".h" ), 0 );
        }
        if (source->flags & FLAG_IDL_IDENT)
        {
            file = add_generated_source( make, replace_extension( source->name, ".idl", "_i.c" ), NULL );
            add_include( file, "rpc.h", 0 );
            add_include( file, "rpcndr.h", 0 );
            add_include( file, "guiddef.h", 0 );
        }
        if (source->flags & FLAG_IDL_PROXY)
        {
            file = add_generated_source( make, "dlldata.o", "dlldata.c" );
            add_include( file, "objbase.h", 0 );
            add_include( file, "rpcproxy.h", 0 );
            file = add_generated_source( make, replace_extension( source->name, ".idl", "_p.c" ), NULL );
            add_include( file, "objbase.h", 0 );
            add_include( file, "rpcproxy.h", 0 );
            add_include( file, "wine/exception.h", 0 );
            add_include( file, replace_extension( source->name, ".idl", ".h" ), 0 );
        }
        if (source->flags & FLAG_IDL_REGTYPELIB)
        {
            add_generated_source( make, replace_extension( source->name, ".idl", "_t.res" ), NULL );
        }
        if (source->flags & FLAG_IDL_REGISTER)
        {
            add_generated_source( make, replace_extension( source->name, ".idl", "_r.res" ), NULL );
        }
        if (strendswith( source->name, ".y" ))
        {
            file = add_generated_source( make, replace_extension( source->name, ".y", ".tab.c" ), NULL );
            /* steal the includes list from the source file */
            file->files_count = source->files_count;
            file->files_size = source->files_size;
            file->files = source->files;
            source->files_count = source->files_size = 0;
            source->files = NULL;
        }
        if (strendswith( source->name, ".l" ))
        {
            file = add_generated_source( make, replace_extension( source->name, ".l", ".yy.c" ), NULL );
            /* steal the includes list from the source file */
            file->files_count = source->files_count;
            file->files_size = source->files_size;
            file->files = source->files;
            source->files_count = source->files_size = 0;
            source->files = NULL;
        }
    }
    if (get_make_variable( make, "TESTDLL" ))
    {
        file = add_generated_source( make, "testlist.o", "testlist.c" );
        add_include( file, "wine/test.h", 0 );
    }
}


/*******************************************************************
 *         create_dir
 */
static void create_dir( const char *dir )
{
    char *p, *path;

    p = path = xstrdup( dir );
    while ((p = strchr( p, '/' )))
    {
        *p = 0;
        if (mkdir( path, 0755 ) == -1 && errno != EEXIST) fatal_perror( "mkdir %s", path );
        *p++ = '/';
        while (*p == '/') p++;
    }
    if (mkdir( path, 0755 ) == -1 && errno != EEXIST) fatal_perror( "mkdir %s", path );
    free( path );
}


/*******************************************************************
 *         output_filenames_obj_dir
 */
static void output_filenames_obj_dir( struct makefile *make, struct strarray array )
{
    unsigned int i;

    for (i = 0; i < array.count; i++) output_filename( obj_dir_path( make, array.str[i] ));
}


/*******************************************************************
 *         output_include
 */
static void output_include( struct incl_file *pFile, struct incl_file *owner )
{
    int i;

    if (pFile->owner == owner) return;
    if (!pFile->filename) return;
    pFile->owner = owner;
    output_filename( pFile->filename );
    for (i = 0; i < pFile->files_count; i++) output_include( pFile->files[i], owner );
}


/*******************************************************************
 *         output_sources
 */
static struct strarray output_sources( struct makefile *make, struct strarray *testlist_files )
{
    struct incl_file *source;
    unsigned int i;
    struct strarray object_files = empty_strarray;
    struct strarray crossobj_files = empty_strarray;
    struct strarray res_files = empty_strarray;
    struct strarray clean_files = empty_strarray;
    struct strarray po_files = empty_strarray;
    struct strarray mo_files = empty_strarray;
    struct strarray mc_files = empty_strarray;
    struct strarray ok_files = empty_strarray;
    struct strarray dlldata_files = empty_strarray;
    struct strarray c2man_files = empty_strarray;
    struct strarray implib_objs = empty_strarray;
    struct strarray includes = empty_strarray;
    struct strarray subdirs = empty_strarray;
    struct strarray phony_targets = empty_strarray;
    struct strarray all_targets = get_expanded_make_var_array( make, "PROGRAMS" );

    for (i = 0; i < linguas.count; i++)
        strarray_add( &mo_files, strmake( "%s/%s.mo", top_obj_dir_path( make, "po" ), linguas.str[i] ));

    strarray_add( &includes, strmake( "-I%s", obj_dir_path( make, "" )));
    if (make->src_dir) strarray_add( &includes, strmake( "-I%s", make->src_dir ));
    if (make->parent_dir) strarray_add( &includes, strmake( "-I%s", src_dir_path( make, make->parent_dir )));
    if (make->top_obj_dir) strarray_add( &includes, strmake( "-I%s", top_obj_dir_path( make, "include" )));
    if (make->top_src_dir) strarray_add( &includes, strmake( "-I%s", top_dir_path( make, "include" )));
    if (make->use_msvcrt) strarray_add( &includes, strmake( "-I%s", top_dir_path( make, "include/msvcrt" )));
    strarray_addall( &includes, make->include_args );

    LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
    {
        struct strarray extradefs;
        char *obj = xstrdup( source->name );
        char *ext = get_extension( obj );

        if (!ext) fatal_error( "unsupported file type %s\n", source->name );
        *ext++ = 0;

        if (make->src_dir && strchr( obj, '/' ))
        {
            char *subdir = base_dir_path( make, obj );
            *strrchr( subdir, '/' ) = 0;
            strarray_add_uniq( &subdirs, subdir );
        }

        extradefs = get_expanded_make_var_array( make, strmake( "%s_EXTRADEFS", obj ));

        if (!strcmp( ext, "y" ))  /* yacc file */
        {
            /* add source file dependency for parallel makes */
            char *header = strmake( "%s.tab.h", obj );

            if (find_include_file( header ))
            {
                output( "%s: %s\n", obj_dir_path( make, header ), source->filename );
                output( "\t$(BISON) -p %s_ -o %s.tab.c -d %s\n",
                        obj, obj_dir_path( make, obj ), source->filename );
                output( "%s.tab.c: %s %s\n", obj_dir_path( make, obj ),
                        source->filename, obj_dir_path( make, header ));
                strarray_add( &clean_files, header );
            }
            else output( "%s.tab.c: %s\n", obj, source->filename );

            output( "\t$(BISON) -p %s_ -o $@ %s\n", obj, source->filename );
            continue;  /* no dependencies */
        }
        else if (!strcmp( ext, "x" ))  /* template file */
        {
            output( "%s.h: %s%s %s\n", obj_dir_path( make, obj ),
                    tools_dir_path( make, "make_xftmpl" ), tools_ext, source->filename );
            output( "\t%s%s -H -o $@ %s\n",
                    tools_dir_path( make, "make_xftmpl" ), tools_ext, source->filename );
            strarray_add( &clean_files, strmake( "%s.h", obj ));
            continue;  /* no dependencies */
        }
        else if (!strcmp( ext, "l" ))  /* lex file */
        {
            output( "%s.yy.c: %s\n", obj_dir_path( make, obj ), source->filename );
            output( "\t$(FLEX) -o$@ %s\n", source->filename );
            continue;  /* no dependencies */
        }
        else if (!strcmp( ext, "rc" ))  /* resource file */
        {
            strarray_add( &res_files, strmake( "%s.res", obj ));
            output( "%s.res: %s %s\n", obj_dir_path( make, obj ),
                    tools_path( make, "wrc" ), source->filename );
            output( "\t%s -o $@ %s", tools_path( make, "wrc" ), source->filename );
            if (make->is_win16) output_filename( "-m16" );
            else output_filenames( target_flags );
            output_filename( "--nostdinc" );
            output_filenames( includes );
            output_filenames( make->define_args );
            output_filenames( extradefs );
            if (mo_files.count && (source->flags & FLAG_RC_PO))
            {
                strarray_add( &po_files, source->filename );
                output_filename( strmake( "--po-dir=%s", top_obj_dir_path( make, "po" )));
                output( "\n" );
                output( "%s.res:", obj_dir_path( make, obj ));
                output_filenames( mo_files );
                output( "\n" );
                output( "%s ", obj_dir_path( make, "rsrc.pot" ));
            }
            else output( "\n" );
            output( "%s.res:", obj_dir_path( make, obj ));
        }
        else if (!strcmp( ext, "mc" ))  /* message file */
        {
            strarray_add( &res_files, strmake( "%s.res", obj ));
            output( "%s.res: %s %s\n", obj_dir_path( make, obj ),
                    tools_path( make, "wmc" ), source->filename );
            output( "\t%s -U -O res -o $@ %s", tools_path( make, "wmc" ), source->filename );
            if (mo_files.count)
            {
                strarray_add( &mc_files, source->filename );
                output_filename( strmake( "--po-dir=%s", top_obj_dir_path( make, "po" )));
                output( "\n" );
                output( "%s.res:", obj_dir_path( make, obj ));
                output_filenames( mo_files );
                output( "\n" );
                output( "%s ", obj_dir_path( make, "msg.pot" ));
            }
            else output( "\n" );
            output( "%s.res:", obj_dir_path( make, obj ));
        }
        else if (!strcmp( ext, "idl" ))  /* IDL file */
        {
            struct strarray targets = empty_strarray;
            char *dest;

            if (!source->flags || find_include_file( strmake( "%s.h", obj )))
                source->flags |= FLAG_IDL_HEADER;

            for (i = 0; i < sizeof(idl_outputs) / sizeof(idl_outputs[0]); i++)
            {
                if (!(source->flags & idl_outputs[i].flag)) continue;
                dest = strmake( "%s%s", obj, idl_outputs[i].ext );
                if (!find_src_file( dest )) strarray_add( &clean_files, dest );
                strarray_add( &targets, dest );
            }
            if (source->flags & FLAG_IDL_PROXY) strarray_add( &dlldata_files, source->name );
            output_filenames_obj_dir( make, targets );
            output( ": %s\n", tools_path( make, "widl" ));
            output( "\t%s -o $@ %s", tools_path( make, "widl" ), source->filename );
            output_filenames( target_flags );
            output_filenames( includes );
            output_filenames( make->define_args );
            output_filenames( extradefs );
            output_filenames( get_expanded_make_var_array( make, "EXTRAIDLFLAGS" ));
            output( "\n" );
            output_filenames_obj_dir( make, targets );
            output( ": %s", source->filename );
        }
        else if (!strcmp( ext, "in" ))  /* .in file or man page */
        {
            if (strendswith( obj, ".man" ) && source->args)
            {
                char *dir, *dest = replace_extension( obj, ".man", "" );
                char *lang = strchr( dest, '.' );
                char *section = source->args;
                if (lang)
                {
                    *lang++ = 0;
                    dir = strmake( "$(DESTDIR)$(mandir)/%s/man%s", lang, section );
                }
                else dir = strmake( "$(DESTDIR)$(mandir)/man%s", section );
                output( "install-man-pages:: %s\n", obj_dir_path( make, obj ));
                output( "\t$(INSTALL_DATA) %s %s/%s.%s\n", obj_dir_path( make, obj ), dir, dest, section );
                output( "uninstall::\n" );
                output( "\t$(RM) %s/%s.%s\n", dir, dest, section );
                free( dest );
                free( dir );
                strarray_add( &all_targets, xstrdup(obj) );
                strarray_add_uniq( &phony_targets, "install-man-pages" );
                strarray_add_uniq( &phony_targets, "uninstall" );
            }
            else strarray_add( &clean_files, xstrdup(obj) );
            output( "%s: %s\n", obj_dir_path( make, obj ), source->filename );
            output( "\t$(SED_CMD) %s >$@ || ($(RM) $@ && false)\n", source->filename );
            output( "%s:", obj_dir_path( make, obj ));
        }
        else if (!strcmp( ext, "sfd" ))  /* font file */
        {
            char *ttf_file = src_dir_path( make, strmake( "%s.ttf", obj ));
            if (fontforge && !make->src_dir)
            {
                output( "%s: %s\n", ttf_file, source->filename );
                output( "\t%s -script %s %s $@\n",
                        fontforge, top_dir_path( make, "fonts/genttf.ff" ), source->filename );
                if (!(source->flags & FLAG_SFD_FONTS)) output( "all: %s\n", ttf_file );
            }
            if (source->flags & FLAG_INSTALL)
            {
                output( "install install-lib::\n" );
                output( "\t$(INSTALL_DATA) %s $(DESTDIR)$(fontdir)/%s.ttf\n", ttf_file, obj );
                output( "uninstall::\n" );
                output( "\t$(RM) $(DESTDIR)$(fontdir)/%s.ttf\n", obj );
            }
            if (source->flags & FLAG_SFD_FONTS)
            {
                struct strarray *array = source->args;

                for (i = 0; i < array->count; i++)
                {
                    char *font = strtok( xstrdup(array->str[i]), " \t" );
                    char *args = strtok( NULL, "" );

                    strarray_add( &all_targets, xstrdup( font ));
                    output( "%s: %s %s\n", obj_dir_path( make, font ),
                            tools_path( make, "sfnt2fon" ), ttf_file );
                    output( "\t%s -o $@ %s %s\n", tools_path( make, "sfnt2fon" ), ttf_file, args );
                    output( "install install-lib:: %s\n", font );
                    output( "\t$(INSTALL_DATA) %s $(DESTDIR)$(fontdir)/%s\n",
                            obj_dir_path( make, font ), font );
                    output( "uninstall::\n" );
                    output( "\t$(RM) $(DESTDIR)$(fontdir)/%s\n", font );
                }
            }
            if (source->flags & (FLAG_INSTALL | FLAG_SFD_FONTS))
            {
                strarray_add_uniq( &phony_targets, "install" );
                strarray_add_uniq( &phony_targets, "install-lib" );
                strarray_add_uniq( &phony_targets, "uninstall" );
            }
            continue;  /* no dependencies */
        }
        else if (!strcmp( ext, "svg" ))  /* svg file */
        {
            if (convert && rsvg && icotool && !make->src_dir)
            {
                output( "%s.ico %s.bmp: %s\n",
                        src_dir_path( make, obj ), src_dir_path( make, obj ), source->filename );
                output( "\tCONVERT=\"%s\" ICOTOOL=\"%s\" RSVG=\"%s\" %s %s $@\n", convert, icotool, rsvg,
                        top_dir_path( make, "tools/buildimage" ), source->filename );
            }
            continue;  /* no dependencies */
        }
        else if (!strcmp( ext, "res" ))
        {
            strarray_add( &res_files, source->name );
            continue;  /* no dependencies */
        }
        else
        {
            int need_cross = make->testdll ||
                (source->flags & FLAG_C_IMPLIB) ||
                (make->module && make->staticlib);

            if ((source->flags & FLAG_GENERATED) &&
                (!make->testdll || !strendswith( source->filename, "testlist.c" )))
                strarray_add( &clean_files, source->filename );
            if (source->flags & FLAG_C_IMPLIB) strarray_add( &implib_objs, strmake( "%s.o", obj ));
            strarray_add( &object_files, strmake( "%s.o", obj ));
            output( "%s.o: %s\n", obj_dir_path( make, obj ), source->filename );
            output( "\t$(CC) -c -o $@ %s", source->filename );
            output_filenames( includes );
            output_filenames( make->define_args );
            output_filenames( extradefs );
            if (make->module || make->staticlib || make->testdll)
            {
                output_filenames( dll_flags );
                if (make->use_msvcrt) output_filenames( msvcrt_flags );
            }
            output_filenames( extra_cflags );
            output_filenames( cpp_flags );
            output_filename( "$(CFLAGS)" );
            output( "\n" );
            if (crosstarget && need_cross)
            {
                strarray_add( &crossobj_files, strmake( "%s.cross.o", obj ));
                output( "%s.cross.o: %s\n", obj_dir_path( make, obj ), source->filename );
                output( "\t$(CROSSCC) -c -o $@ %s", source->filename );
                output_filenames( includes );
                output_filenames( make->define_args );
                output_filenames( extradefs );
                output_filename( "-DWINE_CROSSTEST" );
                output_filenames( cpp_flags );
                output_filename( "$(CFLAGS)" );
                output( "\n" );
            }
            if (make->testdll && !strcmp( ext, "c" ) && !(source->flags & FLAG_GENERATED))
            {
                strarray_add( &ok_files, strmake( "%s.ok", obj ));
                output( "%s.ok:\n", obj_dir_path( make, obj ));
                output( "\t%s $(RUNTESTFLAGS) -T %s -M %s -p %s%s %s && touch $@\n",
                        top_dir_path( make, "tools/runtest" ), top_obj_dir_path( make, "" ), make->testdll,
                        replace_extension( make->testdll, ".dll", "_test.exe" ), dll_ext, obj );
            }
            if (!strcmp( ext, "c" ) && !(source->flags & FLAG_GENERATED))
                strarray_add( &c2man_files, source->filename );
            output( "%s.o", obj_dir_path( make, obj ));
            if (crosstarget && need_cross) output( " %s.cross.o", obj_dir_path( make, obj ));
            output( ":" );
        }
        free( obj );

        for (i = 0; i < source->files_count; i++) output_include( source->files[i], source );
        output( "\n" );
    }

    /* rules for files that depend on multiple sources */

    if (po_files.count)
    {
        output( "%s: %s", obj_dir_path( make, "rsrc.pot" ), tools_path( make, "wrc" ) );
        output_filenames( po_files );
        output( "\n" );
        output( "\t%s -O pot -o $@", tools_path( make, "wrc" ));
        output_filenames( po_files );
        if (make->is_win16) output_filename( "-m16" );
        else output_filenames( target_flags );
        output_filename( "--nostdinc" );
        output_filenames( includes );
        output_filenames( make->define_args );
        output( "\n" );
        strarray_add( &clean_files, "rsrc.pot" );
    }

    if (mc_files.count)
    {
        output( "%s: %s", obj_dir_path( make, "msg.pot" ), tools_path( make, "wmc" ));
        output_filenames( mc_files );
        output( "\n" );
        output( "\t%s -O pot -o $@", tools_path( make, "wmc" ));
        output_filenames( mc_files );
        output( "\n" );
        strarray_add( &clean_files, "msg.pot" );
    }

    if (dlldata_files.count)
    {
        output( "%s: %s %s\n", obj_dir_path( make, "dlldata.c" ),
                tools_path( make, "widl" ), src_dir_path( make, "Makefile.in" ));
        output( "\t%s --dlldata-only -o $@", tools_path( make, "widl" ));
        output_filenames( dlldata_files );
        output( "\n" );
    }

    if (make->module && !make->staticlib)
    {
        struct strarray all_libs = empty_strarray;
        char *module_path = obj_dir_path( make, make->module );
        char *spec_file = NULL;

        if (!make->appmode.count)
            spec_file = src_dir_path( make, replace_extension( make->module, ".dll", ".spec" ));
        for (i = 0; i < make->delayimports.count; i++)
            strarray_add( &all_libs, strmake( "-l%s", make->delayimports.str[i] ));
        for (i = 0; i < make->imports.count; i++)
            strarray_add( &all_libs, strmake( "-l%s", make->imports.str[i] ));
        for (i = 0; i < make->delayimports.count; i++)
            strarray_add( &all_libs, strmake( "-Wb,-d%s", make->delayimports.str[i] ));
        strarray_add( &all_libs, "-lwine" );
        strarray_add( &all_libs, top_obj_dir_path( make, "libs/port/libwine_port.a" ));
        strarray_addall( &all_libs, get_expanded_make_var_array( make, "EXTRALIBS" ));
        strarray_addall( &all_libs, libs );

        if (*dll_ext)
        {
            strarray_add( &all_targets, strmake( "%s%s", make->module, dll_ext ));
            strarray_add( &all_targets, strmake( "%s.fake", make->module ));
            output( "%s%s %s.fake:", module_path, dll_ext, module_path );
        }
        else
        {
            strarray_add( &all_targets, make->module );
            output( "%s:", module_path );
        }
        if (spec_file) output_filename( spec_file );
        output_filenames_obj_dir( make, object_files );
        output_filenames_obj_dir( make, res_files );
        output( "\n" );
        output( "\t%s -o $@", tools_path( make, "winegcc" ));
        output_filename( strmake( "-B%s", tools_dir_path( make, "winebuild" )));
        if (tools_dir) output_filename( strmake( "--sysroot=%s", top_obj_dir_path( make, "" )));
        output_filenames( target_flags );
        output_filenames( unwind_flags );
        if (spec_file)
        {
            output( " -shared %s", spec_file );
            output_filenames( make->extradllflags );
        }
        else output_filenames( make->appmode );
        output_filenames_obj_dir( make, object_files );
        output_filenames_obj_dir( make, res_files );
        output_filenames( all_libs );
        output_filename( "$(LDFLAGS)" );
        output( "\n" );

        if (spec_file && make->importlib)
        {
            char *importlib_path = obj_dir_path( make, strmake( "lib%s", make->importlib ));
            if (*dll_ext)
            {
                strarray_add( &clean_files, strmake( "lib%s.def", make->importlib ));
                output( "%s.def: %s %s\n", importlib_path, tools_path( make, "winebuild" ), spec_file );
                output( "\t%s -w --def -o $@ --export %s", tools_path( make, "winebuild" ), spec_file );
                output_filenames( target_flags );
                if (make->is_win16) output_filename( "-m16" );
                output( "\n" );
                if (implib_objs.count)
                {
                    strarray_add( &clean_files, strmake( "lib%s.def.a", make->importlib ));
                    output( "%s.def.a:", importlib_path );
                    output_filenames_obj_dir( make, implib_objs );
                    output( "\n" );
                    output( "\t$(RM) $@\n" );
                    output( "\t$(AR) $(ARFLAGS) $@" );
                    output_filenames_obj_dir( make, implib_objs );
                    output( "\n" );
                    output( "\t$(RANLIB) $@\n" );
                }
            }
            else
            {
                strarray_add( &clean_files, strmake( "lib%s.a", make->importlib ));
                output( "%s.a: %s %s", importlib_path, tools_path( make, "winebuild" ), spec_file );
                output_filenames_obj_dir( make, implib_objs );
                output( "\n" );
                output( "\t%s -w --implib -o $@ --export %s", tools_path( make, "winebuild" ), spec_file );
                output_filenames( target_flags );
                output_filenames_obj_dir( make, implib_objs );
                output( "\n" );
            }
            if (crosstarget && !make->is_win16)
            {
                struct strarray cross_files = strarray_replace_extension( &implib_objs, ".o", ".cross.o" );
                strarray_add( &clean_files, strmake( "lib%s.cross.a", make->importlib ));
                output( "%s.cross.a: %s %s", importlib_path, tools_path( make, "winebuild" ), spec_file );
                output_filenames_obj_dir( make, cross_files );
                output( "\n" );
                output( "\t%s -b %s -w --implib -o $@ --export %s",
                        tools_path( make, "winebuild" ), crosstarget, spec_file );
                output_filenames_obj_dir( make, cross_files );
                output( "\n" );
            }
        }

        if (spec_file)
        {
            if (c2man_files.count)
            {
                output( "manpages::\n" );
                output( "\t%s -w %s", top_dir_path( make, "tools/c2man.pl" ), spec_file );
                output_filename( strmake( "-R%s", top_dir_path( make, "" )));
                output_filename( strmake( "-I%s", top_dir_path( make, "include" )));
                output_filename( strmake( "-o %s/man%s",
                                          top_obj_dir_path( make, "documentation" ), man_ext ));
                output_filenames( c2man_files );
                output( "\n" );
                output( "htmlpages::\n" );
                output( "\t%s -Th -w %s", top_dir_path( make, "tools/c2man.pl" ), spec_file );
                output_filename( strmake( "-R%s", top_dir_path( make, "" )));
                output_filename( strmake( "-I%s", top_dir_path( make, "include" )));
                output_filename( strmake( "-o %s",
                                          top_obj_dir_path( make, "documentation/html" )));
                output_filenames( c2man_files );
                output( "\n" );
                output( "sgmlpages::\n" );
                output( "\t%s -Ts -w %s", top_dir_path( make, "tools/c2man.pl" ), spec_file );
                output_filename( strmake( "-R%s", top_dir_path( make, "" )));
                output_filename( strmake( "-I%s", top_dir_path( make, "include" )));
                output_filename( strmake( "-o %s",
                                          top_obj_dir_path( make, "documentation/api-guide" )));
                output_filenames( c2man_files );
                output( "\n" );
                output( "xmlpages::\n" );
                output( "\t%s -Tx -w %s", top_dir_path( make, "tools/c2man.pl" ), spec_file );
                output_filename( strmake( "-R%s", top_dir_path( make, "" )));
                output_filename( strmake( "-I%s", top_dir_path( make, "include" )));
                output_filename( strmake( "-o %s",
                                          top_obj_dir_path( make, "documentation/api-guide-xml" )));
                output_filenames( c2man_files );
                output( "\n" );
                strarray_add( &phony_targets, "manpages" );
                strarray_add( &phony_targets, "htmlpages" );
                strarray_add( &phony_targets, "sgmlpages" );
                strarray_add( &phony_targets, "xmlpages" );
            }
            else output( "manpages htmlpages sgmlpages xmlpages::\n" );
        }
    }

    if (make->staticlib)
    {
        strarray_add( &all_targets, make->staticlib );
        output( "%s:", obj_dir_path( make, make->staticlib ));
        output_filenames_obj_dir( make, object_files );
        output( "\n\t$(RM) $@\n" );
        output( "\t$(AR) $(ARFLAGS) $@" );
        output_filenames_obj_dir( make, object_files );
        output( "\n\t$(RANLIB) $@\n" );
        if (crosstarget && make->module)
        {
            char *name = replace_extension( make->staticlib, ".a", ".cross.a" );

            strarray_add( &all_targets, name );
            output( "%s:", obj_dir_path( make, name ));
            output_filenames_obj_dir( make, crossobj_files );
            output( "\n\t$(RM) $@\n" );
            output( "\t%s-ar $(ARFLAGS) $@", crosstarget );
            output_filenames_obj_dir( make, crossobj_files );
            output( "\n\t%s-ranlib $@\n", crosstarget );
        }
    }

    if (make->testdll)
    {
        char *testmodule = replace_extension( make->testdll, ".dll", "_test.exe" );
        char *stripped = replace_extension( make->testdll, ".dll", "_test-stripped.exe" );
        char *testres = replace_extension( make->testdll, ".dll", "_test.res" );
        struct strarray all_libs = empty_strarray;

        for (i = 0; i < make->imports.count; i++)
            strarray_add( &all_libs, strmake( "-l%s", make->imports.str[i] ));
        strarray_addall( &all_libs, get_expanded_make_var_array( make, "LIBS" ));

        strarray_add( &all_targets, strmake( "%s%s", testmodule, dll_ext ));
        strarray_add( &clean_files, strmake( "%s%s", stripped, dll_ext ));
        output( "%s%s:\n", obj_dir_path( make, testmodule ), dll_ext );
        output( "\t%s -o $@", tools_path( make, "winegcc" ));
        output_filename( strmake( "-B%s", tools_dir_path( make, "winebuild" )));
        if (tools_dir) output_filename( strmake( "--sysroot=%s", top_obj_dir_path( make, "" )));
        output_filenames( target_flags );
        output_filenames( unwind_flags );
        output_filenames( make->appmode );
        output_filenames_obj_dir( make, object_files );
        output_filenames_obj_dir( make, res_files );
        output_filenames( all_libs );
        output_filename( "$(LDFLAGS)" );
        output( "\n" );
        output( "%s%s:\n", obj_dir_path( make, stripped ), dll_ext );
        output( "\t%s -o $@", tools_path( make, "winegcc" ));
        output_filename( strmake( "-B%s", tools_dir_path( make, "winebuild" )));
        if (tools_dir) output_filename( strmake( "--sysroot=%s", top_obj_dir_path( make, "" )));
        output_filenames( target_flags );
        output_filenames( unwind_flags );
        output_filename( strmake( "-Wb,-F,%s", testmodule ));
        output_filenames( make->appmode );
        output_filenames_obj_dir( make, object_files );
        output_filenames_obj_dir( make, res_files );
        output_filenames( all_libs );
        output_filename( "$(LDFLAGS)" );
        output( "\n" );
        output( "%s%s %s%s:", obj_dir_path( make, testmodule ), dll_ext,
                obj_dir_path( make, stripped ), dll_ext );
        output_filenames_obj_dir( make, object_files );
        output_filenames_obj_dir( make, res_files );
        output( "\n" );

        output( "all: %s/%s\n", top_obj_dir_path( make, "programs/winetest" ), testres );
        output( "%s/%s: %s%s\n", top_obj_dir_path( make, "programs/winetest" ), testres,
                obj_dir_path( make, stripped ), dll_ext );
        output( "\techo \"%s TESTRES \\\"%s%s\\\"\" | %s -o $@\n",
                testmodule, obj_dir_path( make, stripped ), dll_ext, tools_path( make, "wrc" ));

        if (crosstarget)
        {
            char *crosstest = replace_extension( make->testdll, ".dll", "_crosstest.exe" );

            strarray_add( &clean_files, crosstest );
            output( "%s: %s\n", obj_dir_path( make, "crosstest" ), obj_dir_path( make, crosstest ));
            output( "%s:", obj_dir_path( make, crosstest ));
            output_filenames_obj_dir( make, crossobj_files );
            output_filenames_obj_dir( make, res_files );
            output( "\n" );
            output( "\t%s -o $@ -b %s", tools_path( make, "winegcc" ), crosstarget );
            output_filename( strmake( "-B%s", tools_dir_path( make, "winebuild" )));
            if (tools_dir) output_filename( strmake( "--sysroot=%s", top_obj_dir_path( make, "" )));
            output_filename( "--lib-suffix=.cross.a" );
            output_filenames_obj_dir( make, crossobj_files );
            output_filenames_obj_dir( make, res_files );
            output_filenames( all_libs );
            output_filename( "$(LDFLAGS)" );
            output( "\n" );
            strarray_add( &phony_targets, obj_dir_path( make, "crosstest" ));
            if (make->obj_dir) output( "crosstest: %s\n", obj_dir_path( make, "crosstest" ));
        }

        output_filenames_obj_dir( make, ok_files );
        output( ": %s%s ../%s%s\n", testmodule, dll_ext, make->testdll, dll_ext );
        output( "check test:" );
        output_filenames_obj_dir( make, ok_files );
        output( "\n" );
        output( "testclean::\n" );
        output( "\t$(RM)" );
        output_filenames_obj_dir( make, ok_files );
        output( "\n" );
        strarray_addall( &clean_files, ok_files );
        strarray_add( &phony_targets, "check" );
        strarray_add( &phony_targets, "test" );
        strarray_add( &phony_targets, "testclean" );
        *testlist_files = strarray_replace_extension( &ok_files, ".ok", "" );
    }

    if (all_targets.count)
    {
        output( "all:" );
        output_filenames_obj_dir( make, all_targets );
        output( "\n" );
    }

    strarray_addall( &clean_files, object_files );
    strarray_addall( &clean_files, crossobj_files );
    strarray_addall( &clean_files, res_files );
    strarray_addall( &clean_files, all_targets );
    strarray_addall( &clean_files, get_expanded_make_var_array( make, "EXTRA_TARGETS" ));

    if (clean_files.count)
    {
        output( "%s::\n", obj_dir_path( make, "clean" ));
        output( "\t$(RM)" );
        output_filenames_obj_dir( make, clean_files );
        output( "\n" );
        if (make->obj_dir) output( "__clean__: %s\n", obj_dir_path( make, "clean" ));
        strarray_add( &phony_targets, obj_dir_path( make, "clean" ));
    }

    if (make->top_obj_dir)
    {
        output( "depend:\n" );
        output( "\t@cd %s && $(MAKE) %s\n", make->top_obj_dir, base_dir_path( make, "depend" ));
        strarray_add( &phony_targets, "depend" );
    }

    if (phony_targets.count)
    {
        output( ".PHONY:" );
        output_filenames( phony_targets );
        output( "\n" );
    }

    for (i = 0; i < subdirs.count; i++) create_dir( subdirs.str[i] );

    return clean_files;
}


/*******************************************************************
 *         create_temp_file
 */
static FILE *create_temp_file( const char *orig )
{
    char *name = xmalloc( strlen(orig) + 13 );
    unsigned int i, id = getpid();
    int fd;
    FILE *ret = NULL;

    for (i = 0; i < 100; i++)
    {
        sprintf( name, "%s.tmp%08x", orig, id );
        if ((fd = open( name, O_RDWR | O_CREAT | O_EXCL, 0666 )) != -1)
        {
            ret = fdopen( fd, "w" );
            break;
        }
        if (errno != EEXIST) break;
        id += 7777;
    }
    if (!ret) fatal_error( "failed to create output file for '%s'\n", orig );
    temp_file_name = name;
    return ret;
}


/*******************************************************************
 *         rename_temp_file
 */
static void rename_temp_file( const char *dest )
{
    int ret = rename( temp_file_name, dest );
    if (ret == -1 && errno == EEXIST)
    {
        /* rename doesn't overwrite on windows */
        unlink( dest );
        ret = rename( temp_file_name, dest );
    }
    if (ret == -1) fatal_error( "failed to rename output file to '%s'\n", dest );
    temp_file_name = NULL;
}


/*******************************************************************
 *         are_files_identical
 */
static int are_files_identical( FILE *file1, FILE *file2 )
{
    for (;;)
    {
        char buffer1[8192], buffer2[8192];
        int size1 = fread( buffer1, 1, sizeof(buffer1), file1 );
        int size2 = fread( buffer2, 1, sizeof(buffer2), file2 );
        if (size1 != size2) return 0;
        if (!size1) return feof( file1 ) && feof( file2 );
        if (memcmp( buffer1, buffer2, size1 )) return 0;
    }
}


/*******************************************************************
 *         rename_temp_file_if_changed
 */
static void rename_temp_file_if_changed( const char *dest )
{
    FILE *file1, *file2;
    int do_rename = 1;

    if ((file1 = fopen( dest, "r" )))
    {
        if ((file2 = fopen( temp_file_name, "r" )))
        {
            do_rename = !are_files_identical( file1, file2 );
            fclose( file2 );
        }
        fclose( file1 );
    }
    if (!do_rename)
    {
        unlink( temp_file_name );
        temp_file_name = NULL;
    }
    else rename_temp_file( dest );
}


/*******************************************************************
 *         output_testlist
 */
static void output_testlist( const char *dest, struct strarray files )
{
    int i;

    output_file = create_temp_file( dest );

    output( "/* Automatically generated by make depend; DO NOT EDIT!! */\n\n" );
    output( "#define WIN32_LEAN_AND_MEAN\n" );
    output( "#include <windows.h>\n\n" );
    output( "#define STANDALONE\n" );
    output( "#include \"wine/test.h\"\n\n" );

    for (i = 0; i < files.count; i++) output( "extern void func_%s(void);\n", files.str[i] );
    output( "\n" );
    output( "const struct test winetest_testlist[] =\n" );
    output( "{\n" );
    for (i = 0; i < files.count; i++) output( "    { \"%s\", func_%s },\n", files.str[i], files.str[i] );
    output( "    { 0, 0 }\n" );
    output( "};\n" );

    if (fclose( output_file )) fatal_perror( "write" );
    output_file = NULL;
    rename_temp_file_if_changed( dest );
}


/*******************************************************************
 *         output_gitignore
 */
static void output_gitignore( const char *dest, struct strarray files )
{
    int i;

    output_file = create_temp_file( dest );

    output( "# Automatically generated by make depend; DO NOT EDIT!!\n" );
    for (i = 0; i < files.count; i++)
    {
        if (!strchr( files.str[i], '/' )) output( "/" );
        output( "%s\n", files.str[i] );
    }

    if (fclose( output_file )) fatal_perror( "write" );
    output_file = NULL;
    rename_temp_file( dest );
}


/*******************************************************************
 *         output_dependencies
 */
static void output_dependencies( struct makefile *make, const char *path )
{
    struct strarray targets, testlist_files = empty_strarray, ignore_files = empty_strarray;

    if (Separator && ((output_file = fopen( path, "r" ))))
    {
        char buffer[1024];
        FILE *tmp_file = create_temp_file( path );
        int found = 0;

        while (fgets( buffer, sizeof(buffer), output_file ) && !found)
        {
            if (fwrite( buffer, 1, strlen(buffer), tmp_file ) != strlen(buffer)) fatal_perror( "write" );
            found = !strncmp( buffer, Separator, strlen(Separator) );
        }
        if (fclose( output_file )) fatal_perror( "write" );
        output_file = tmp_file;
        if (!found) output( "\n%s\n", Separator );
    }
    else
    {
        if (!(output_file = fopen( path, Separator ? "a" : "w" )))
            fatal_perror( "%s", path );
    }

    targets = output_sources( make, &testlist_files );

    fclose( output_file );
    output_file = NULL;
    if (temp_file_name) rename_temp_file( path );

    strarray_add( &ignore_files, ".gitignore" );
    strarray_add( &ignore_files, "Makefile" );
    if (testlist_files.count) strarray_add( &ignore_files, "testlist.c" );
    strarray_addall( &ignore_files, targets );

    if (testlist_files.count)
        output_testlist( base_dir_path( make, "testlist.c" ), testlist_files );
    if (!make->src_dir && make->base_dir)
        output_gitignore( base_dir_path( make, ".gitignore" ), ignore_files );
}


/*******************************************************************
 *         update_makefile
 */
static void update_makefile( const char *path )
{
    static const char *source_vars[] =
    {
        "C_SRCS",
        "OBJC_SRCS",
        "RC_SRCS",
        "MC_SRCS",
        "IDL_SRCS",
        "BISON_SRCS",
        "LEX_SRCS",
        "XTEMPLATE_SRCS",
        "SVG_SRCS",
        "FONT_SRCS",
        "IN_SRCS",
        "MANPAGES",
        NULL
    };
    const char **var;
    unsigned int i;
    struct strarray value;
    struct incl_file *file;
    struct makefile *make;

    make = parse_makefile( path, Separator );

    if (root_src_dir)
    {
        make->top_src_dir = concat_paths( make->top_obj_dir, root_src_dir );
        make->src_dir = concat_paths( make->top_src_dir, make->base_dir );
    }
    strarray_set_value( &make->vars, "top_builddir", top_obj_dir_path( make, "" ));
    strarray_set_value( &make->vars, "top_srcdir", top_dir_path( make, "" ));
    strarray_set_value( &make->vars, "srcdir", src_dir_path( make, "" ));

    make->parent_dir    = get_expanded_make_variable( make, "PARENTSRC" );
    make->module        = get_expanded_make_variable( make, "MODULE" );
    make->testdll       = get_expanded_make_variable( make, "TESTDLL" );
    make->staticlib     = get_expanded_make_variable( make, "STATICLIB" );
    make->importlib     = get_expanded_make_variable( make, "IMPORTLIB" );

    make->appmode       = get_expanded_make_var_array( make, "APPMODE" );
    make->imports       = get_expanded_make_var_array( make, "IMPORTS" );
    make->delayimports  = get_expanded_make_var_array( make, "DELAYIMPORTS" );
    make->extradllflags = get_expanded_make_var_array( make, "EXTRADLLFLAGS" );

    if (make->module && strendswith( make->module, ".a" )) make->staticlib = make->module;

    make->is_win16   = strarray_exists( &make->extradllflags, "-m16" );
    make->use_msvcrt = strarray_exists( &make->appmode, "-mno-cygwin" );

    for (i = 0; i < make->imports.count && !make->use_msvcrt; i++)
        make->use_msvcrt = !strncmp( make->imports.str[i], "msvcr", 5 );

    make->include_args = empty_strarray;
    make->define_args = empty_strarray;
    strarray_add( &make->define_args, "-D__WINESRC__" );

    value = get_expanded_make_var_array( make, "EXTRAINCL" );
    for (i = 0; i < value.count; i++)
        if (!strncmp( value.str[i], "-I", 2 ))
            strarray_add_uniq( &make->include_args, value.str[i] );
        else
            strarray_add_uniq( &make->define_args, value.str[i] );
    strarray_addall( &make->define_args, get_expanded_make_var_array( make, "EXTRADEFS" ));

    list_init( &sources );
    list_init( &includes );

    for (var = source_vars; *var; var++)
    {
        value = get_expanded_make_var_array( make, *var );
        for (i = 0; i < value.count; i++) add_src_file( make, value.str[i] );
    }

    add_generated_sources( make );

    value = get_expanded_make_var_array( make, "EXTRA_OBJS" );
    for (i = 0; i < value.count; i++)
    {
        /* default to .c for unknown extra object files */
        if (strendswith( value.str[i], ".o" ))
            add_generated_source( make, value.str[i], replace_extension( value.str[i], ".o", ".c" ) );
        else
            add_generated_source( make, value.str[i], NULL );
    }

    LIST_FOR_EACH_ENTRY( file, &includes, struct incl_file, entry ) parse_file( make, file, 0 );

    output_file_name = base_dir_path( make, makefile_name );
    output_dependencies( make, output_file_name );
    output_file_name = NULL;
}


/*******************************************************************
 *         parse_makeflags
 */
static void parse_makeflags( const char *flags )
{
    const char *p = flags;
    char *var, *buffer = xmalloc( strlen(flags) + 1 );

    while (*p)
    {
        while (isspace(*p)) p++;
        var = buffer;
        while (*p && !isspace(*p))
        {
            if (*p == '\\' && p[1]) p++;
            *var++ = *p++;
        }
        *var = 0;
        if (var > buffer) set_make_variable( &cmdline_vars, buffer );
    }
}


/*******************************************************************
 *         parse_option
 */
static int parse_option( const char *opt )
{
    if (opt[0] != '-')
    {
        if (strchr( opt, '=' )) return set_make_variable( &cmdline_vars, opt );
        return 0;
    }
    switch(opt[1])
    {
    case 'f':
        if (opt[2]) makefile_name = opt + 2;
        break;
    case 'R':
        relative_dir_mode = 1;
        break;
    case 's':
        if (opt[2]) Separator = opt + 2;
        else Separator = NULL;
        break;
    default:
        fprintf( stderr, "Unknown option '%s'\n%s", opt, Usage );
        exit(1);
    }
    return 1;
}


/*******************************************************************
 *         main
 */
int main( int argc, char *argv[] )
{
    const char *makeflags = getenv( "MAKEFLAGS" );
    int i, j;

    if (makeflags) parse_makeflags( makeflags );

    i = 1;
    while (i < argc)
    {
        if (parse_option( argv[i] ))
        {
            for (j = i; j < argc; j++) argv[j] = argv[j+1];
            argc--;
        }
        else i++;
    }

    if (relative_dir_mode)
    {
        char *relpath;

        if (argc != 3)
        {
            fprintf( stderr, "Option -r needs two directories\n%s", Usage );
            exit( 1 );
        }
        relpath = get_relative_path( argv[1], argv[2] );
        printf( "%s\n", relpath ? relpath : "." );
        exit( 0 );
    }

    if (argc <= 1)
    {
        fprintf( stderr, "%s", Usage );
        exit( 1 );
    }
    atexit( cleanup_files );
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif

    top_makefile = parse_makefile( NULL, "# End of common header" );

    linguas      = get_expanded_make_var_array( top_makefile, "LINGUAS" );
    target_flags = get_expanded_make_var_array( top_makefile, "TARGETFLAGS" );
    msvcrt_flags = get_expanded_make_var_array( top_makefile, "MSVCRTFLAGS" );
    dll_flags    = get_expanded_make_var_array( top_makefile, "DLLFLAGS" );
    extra_cflags = get_expanded_make_var_array( top_makefile, "EXTRACFLAGS" );
    cpp_flags    = get_expanded_make_var_array( top_makefile, "CPPFLAGS" );
    unwind_flags = get_expanded_make_var_array( top_makefile, "UNWINDFLAGS" );
    libs         = get_expanded_make_var_array( top_makefile, "LIBS" );

    root_src_dir = get_expanded_make_variable( top_makefile, "srcdir" );
    tools_dir    = get_expanded_make_variable( top_makefile, "TOOLSDIR" );
    tools_ext    = get_expanded_make_variable( top_makefile, "TOOLSEXT" );
    exe_ext      = get_expanded_make_variable( top_makefile, "EXEEXT" );
    man_ext      = get_expanded_make_variable( top_makefile, "api_manext" );
    dll_ext      = (exe_ext && !strcmp( exe_ext, ".exe" )) ? "" : ".so";
    dll_prefix   = get_expanded_make_variable( top_makefile, "DLLPREFIX" );
    crosstarget  = get_expanded_make_variable( top_makefile, "CROSSTARGET" );
    fontforge    = get_expanded_make_variable( top_makefile, "FONTFORGE" );
    convert      = get_expanded_make_variable( top_makefile, "CONVERT" );
    rsvg         = get_expanded_make_variable( top_makefile, "RSVG" );
    icotool      = get_expanded_make_variable( top_makefile, "ICOTOOL" );

    if (root_src_dir && !strcmp( root_src_dir, "." )) root_src_dir = NULL;
    if (tools_dir && !strcmp( tools_dir, "." )) tools_dir = NULL;
    if (!tools_ext) tools_ext = "";
    if (!dll_prefix) dll_prefix = "";
    if (!man_ext) man_ext = "3w";

    for (i = 1; i < argc; i++) update_makefile( argv[i] );
    return 0;
}
