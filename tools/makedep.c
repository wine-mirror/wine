/*
 * Generate include file dependencies
 *
 * Copyright 1996 Alexandre Julliard
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
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/list.h"

/* Max first-level includes per file */
#define MAX_INCLUDES 200

struct incl_file
{
    struct list        entry;
    char              *name;
    char              *filename;
    char              *sourcename;    /* source file name for generated headers */
    struct incl_file  *included_by;   /* file that included this one */
    int                included_line; /* line where this file was included */
    unsigned int       flags;         /* flags (see below) */
    struct incl_file  *owner;
    struct incl_file  *files[MAX_INCLUDES];
};

#define FLAG_SYSTEM       0x0001  /* is it a system include (#include <name>) */
#define FLAG_IDL_PROXY    0x0002  /* generates a proxy (_p.c) file */
#define FLAG_IDL_CLIENT   0x0004  /* generates a client (_c.c) file */
#define FLAG_IDL_SERVER   0x0008  /* generates a server (_s.c) file */
#define FLAG_IDL_IDENT    0x0010  /* generates an ident (_i.c) file */
#define FLAG_IDL_REGISTER 0x0020  /* generates a registration (_r.res) file */
#define FLAG_IDL_TYPELIB  0x0040  /* generates a typelib (.tlb) file */
#define FLAG_IDL_HEADER   0x0080  /* generates a header (.h) file */
#define FLAG_RC_PO        0x0100  /* rc file contains translations */
#define FLAG_C_IMPLIB     0x0200  /* file is part of an import library */

static const struct
{
    unsigned int flag;
    const char *ext;
    const char *widl_arg;
} idl_outputs[] =
{
    { FLAG_IDL_TYPELIB,  ".tlb",   "$(TARGETFLAGS) $(IDLFLAGS) -t" },
    { FLAG_IDL_TYPELIB,  "_t.res", "$(TARGETFLAGS) $(IDLFLAGS) -t" },
    { FLAG_IDL_CLIENT,   "_c.c",   "$(IDLFLAGS) -c" },
    { FLAG_IDL_IDENT,    "_i.c",   "$(IDLFLAGS) -u" },
    { FLAG_IDL_PROXY,    "_p.c",   "$(IDLFLAGS) -p" },
    { FLAG_IDL_SERVER,   "_s.c",   "$(IDLFLAGS) -s" },
    { FLAG_IDL_REGISTER, "_r.res", "$(IDLFLAGS) -r" },
    { FLAG_IDL_HEADER,   ".h",     "$(IDLFLAGS) -h" },
};

static struct list sources = LIST_INIT(sources);
static struct list includes = LIST_INIT(includes);

struct object_extension
{
    struct list entry;
    const char *extension;
};

static struct list object_extensions = LIST_INIT(object_extensions);

struct incl_path
{
    struct list entry;
    const char *name;
};

static struct list paths = LIST_INIT(paths);

struct strarray
{
    unsigned int count;  /* strings in use */
    unsigned int size;   /* total allocated size */
    const char **str;
};

static const char *src_dir;
static const char *top_src_dir;
static const char *top_obj_dir;
static const char *parent_dir;
static const char *OutputFileName = "Makefile";
static const char *Separator = "### Dependencies";
static const char *input_file_name;
static int relative_dir_mode;
static int input_line;
static FILE *output_file;

static const char Usage[] =
    "Usage: makedep [options] [files]\n"
    "Options:\n"
    "   -Idir      Search for include files in directory 'dir'\n"
    "   -Cdir      Search for source files in directory 'dir'\n"
    "   -Sdir      Set the top source directory\n"
    "   -Tdir      Set the top object directory\n"
    "   -Pdir      Set the parent source directory\n"
    "   -R from to Compute the relative path between two directories\n"
    "   -fxxx      Store output in file 'xxx' (default: Makefile)\n"
    "   -sxxx      Use 'xxx' as separator (default: \"### Dependencies\")\n";


#ifndef __GNUC__
#define __attribute__(x)
#endif

static void fatal_error( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static void fatal_perror( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static int output( const char *format, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
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
static int output( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = vfprintf( output_file, format, valist );
    va_end( valist );
    if (ret < 0) fatal_perror( "output" );
    return ret;
}


/*******************************************************************
 *         strarray_init
 */
static void strarray_init( struct strarray *array )
{
    array->count = 0;
    array->size  = 0;
    array->str   = NULL;
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
 *         output_filename
 */
static void output_filename( const char *name, int *column )
{
    if (*column + strlen(name) + 1 > 100)
    {
        output( " \\\n" );
        *column = 0;
    }
    *column += output( " %s", name );
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
static char *replace_extension( const char *name, unsigned int old_len, const char *new_ext )
{
    char *ret = xmalloc( strlen( name ) + strlen( new_ext ) + 1 );
    strcpy( ret, name );
    strcpy( ret + strlen( ret ) - old_len, new_ext );
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
 *         add_object_extension
 *
 * Add an extension for object files.
 */
static void add_object_extension( const char *ext )
{
    struct object_extension *object_extension = xmalloc( sizeof(*object_extension) );
    list_add_tail( &object_extensions, &object_extension->entry );
    object_extension->extension = ext;
}

/*******************************************************************
 *         add_include_path
 *
 * Add a directory to the include path.
 */
static void add_include_path( const char *name )
{
    struct incl_path *path = xmalloc( sizeof(*path) );
    list_add_tail( &paths, &path->entry );
    path->name = name;
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
static struct incl_file *add_include( struct incl_file *pFile, const char *name, int system )
{
    struct incl_file *include;
    char *ext;
    int pos;

    for (pos = 0; pos < MAX_INCLUDES; pos++) if (!pFile->files[pos]) break;
    if (pos >= MAX_INCLUDES)
        fatal_error( "too many included files, please fix MAX_INCLUDES\n" );

    /* enforce some rules for the Wine tree */

    if (!memcmp( name, "../", 3 ))
        fatal_error( "#include directive with relative path not allowed\n" );

    if (!strcmp( name, "config.h" ))
    {
        if ((ext = strrchr( pFile->filename, '.' )) && !strcmp( ext, ".h" ))
            fatal_error( "config.h must not be included by a header file\n" );
        if (pos)
            fatal_error( "config.h must be included before anything else\n" );
    }
    else if (!strcmp( name, "wine/port.h" ))
    {
        if ((ext = strrchr( pFile->filename, '.' )) && !strcmp( ext, ".h" ))
            fatal_error( "wine/port.h must not be included by a header file\n" );
        if (!pos) fatal_error( "config.h must be included before wine/port.h\n" );
        if (pos > 1)
            fatal_error( "wine/port.h must be included before everything except config.h\n" );
        if (strcmp( pFile->files[0]->name, "config.h" ))
            fatal_error( "config.h must be included before wine/port.h\n" );
    }

    LIST_FOR_EACH_ENTRY( include, &includes, struct incl_file, entry )
        if (!strcmp( name, include->name )) goto found;

    include = xmalloc( sizeof(*include) );
    memset( include, 0, sizeof(*include) );
    include->name = xstrdup(name);
    include->included_by = pFile;
    include->included_line = input_line;
    if (system) include->flags |= FLAG_SYSTEM;
    list_add_tail( &includes, &include->entry );
found:
    pFile->files[pos] = include;
    return include;
}


/*******************************************************************
 *         open_src_file
 */
static FILE *open_src_file( struct incl_file *pFile )
{
    FILE *file;

    /* first try name as is */
    if ((file = fopen( pFile->name, "r" )))
    {
        pFile->filename = xstrdup( pFile->name );
        return file;
    }
    /* now try in source dir */
    if (src_dir)
    {
        pFile->filename = strmake( "%s/%s", src_dir, pFile->name );
        file = fopen( pFile->filename, "r" );
    }
    /* now try parent dir */
    if (!file && parent_dir)
    {
        if (src_dir)
            pFile->filename = strmake( "%s/%s/%s", src_dir, parent_dir, pFile->name );
        else
            pFile->filename = strmake( "%s/%s", parent_dir, pFile->name );
        if ((file = fopen( pFile->filename, "r" ))) return file;
        file = fopen( pFile->filename, "r" );
    }
    if (!file) fatal_perror( "open %s", pFile->name );
    return file;
}


/*******************************************************************
 *         open_include_file
 */
static FILE *open_include_file( struct incl_file *pFile )
{
    FILE *file = NULL;
    char *filename, *p;
    struct incl_path *path;

    errno = ENOENT;

    /* check for generated bison header */

    if (strendswith( pFile->name, ".tab.h" ))
    {
        filename = replace_extension( pFile->name, 6, ".y" );
        if (src_dir) filename = strmake( "%s/%s", src_dir, filename );

        if ((file = fopen( filename, "r" )))
        {
            pFile->sourcename = filename;
            pFile->filename = xstrdup( pFile->name );
            /* don't bother to parse it */
            fclose( file );
            return NULL;
        }
        free( filename );
    }

    /* check for corresponding idl file in source dir */

    if (strendswith( pFile->name, ".h" ))
    {
        filename = replace_extension( pFile->name, 2, ".idl" );
        if (src_dir) filename = strmake( "%s/%s", src_dir, filename );

        if ((file = fopen( filename, "r" )))
        {
            pFile->sourcename = filename;
            pFile->filename = xstrdup( pFile->name );
            return file;
        }
        free( filename );
    }

    /* first try name as is */
    if ((file = fopen( pFile->name, "r" )))
    {
        pFile->filename = xstrdup( pFile->name );
        return file;
    }

    /* now try in source dir */
    if (src_dir)
    {
        filename = strmake( "%s/%s", src_dir, pFile->name );
        if ((file = fopen( filename, "r" ))) goto found;
        free( filename );
    }

    /* now try in parent source dir */
    if (parent_dir)
    {
        if (src_dir)
            filename = strmake( "%s/%s/%s", src_dir, parent_dir, pFile->name );
        else
            filename = strmake( "%s/%s", parent_dir, pFile->name );
        if ((file = fopen( filename, "r" ))) goto found;
        free( filename );
    }

    /* check for corresponding idl file in global includes */

    if (strendswith( pFile->name, ".h" ))
    {
        filename = replace_extension( pFile->name, 2, ".idl" );
        if (top_src_dir)
            filename = strmake( "%s/include/%s", top_src_dir, filename );
        else if (top_obj_dir)
            filename = strmake( "%s/include/%s", top_obj_dir, filename );
        else
            filename = NULL;

        if (filename && (file = fopen( filename, "r" )))
        {
            pFile->sourcename = filename;
            pFile->filename = strmake( "%s/include/%s", top_obj_dir, pFile->name );
            return file;
        }
        free( filename );
    }

    /* check for corresponding .x file in global includes */

    if (strendswith( pFile->name, "tmpl.h" ))
    {
        filename = replace_extension( pFile->name, 2, ".x" );
        if (top_src_dir)
            filename = strmake( "%s/include/%s", top_src_dir, filename );
        else if (top_obj_dir)
            filename = strmake( "%s/include/%s", top_obj_dir, filename );
        else
            filename = NULL;

        if (filename && (file = fopen( filename, "r" )))
        {
            pFile->sourcename = filename;
            pFile->filename = strmake( "%s/include/%s", top_obj_dir, pFile->name );
            return file;
        }
        free( filename );
    }

    /* now try in global includes */
    if (top_obj_dir)
    {
        filename = strmake( "%s/include/%s", top_obj_dir, pFile->name );
        if ((file = fopen( filename, "r" ))) goto found;
        free( filename );
    }
    if (top_src_dir)
    {
        filename = strmake( "%s/include/%s", top_src_dir, pFile->name );
        if ((file = fopen( filename, "r" ))) goto found;
        free( filename );
    }

    /* now search in include paths */
    LIST_FOR_EACH_ENTRY( path, &paths, struct incl_path, entry )
    {
        filename = strmake( "%s/%s", path->name, pFile->name );
        if ((file = fopen( filename, "r" ))) goto found;
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
        if ((file = fopen( filename, "r" ))) goto found;
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
        if (strendswith( source->name, ".idl" ))
        {
            if (!strcmp( flag, "header" )) source->flags |= FLAG_IDL_HEADER;
            else if (!strcmp( flag, "proxy" )) source->flags |= FLAG_IDL_PROXY;
            else if (!strcmp( flag, "client" )) source->flags |= FLAG_IDL_CLIENT;
            else if (!strcmp( flag, "server" )) source->flags |= FLAG_IDL_SERVER;
            else if (!strcmp( flag, "ident" )) source->flags |= FLAG_IDL_IDENT;
            else if (!strcmp( flag, "typelib" )) source->flags |= FLAG_IDL_TYPELIB;
            else if (!strcmp( flag, "register" )) source->flags |= FLAG_IDL_REGISTER;
        }
        else if (strendswith( source->name, ".rc" ))
        {
            if (!strcmp( flag, "po" )) source->flags |= FLAG_RC_PO;
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
 *         parse_man_page
 */
static void parse_man_page( struct incl_file *source, FILE *file )
{
    char *p, *buffer;

    /* make sure it gets rebuilt when the version changes */
    add_include( source, "config.h", 1 );

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        if (strncmp( buffer, ".TH", 3 )) continue;
        if (!(p = strtok( buffer, " \t" ))) continue;  /* .TH */
        if (!(p = strtok( NULL, " \t" ))) continue;  /* program name */
        if (!(p = strtok( NULL, " \t" ))) continue;  /* man section */
        source->sourcename = xstrdup( p );  /* abuse source name to store section */
        return;
    }
}


/*******************************************************************
 *         parse_generated_idl
 */
static void parse_generated_idl( struct incl_file *source )
{
    char *header = replace_extension( source->name, 4, ".h" );

    source->filename = xstrdup( source->name );

    if (strendswith( source->name, "_c.c" ))
    {
        add_include( source, header, 0 );
    }
    else if (strendswith( source->name, "_i.c" ))
    {
        add_include( source, "rpc.h", 1 );
        add_include( source, "rpcndr.h", 1 );
        add_include( source, "guiddef.h", 1 );
    }
    else if (strendswith( source->name, "_p.c" ))
    {
        add_include( source, "objbase.h", 1 );
        add_include( source, "rpcproxy.h", 1 );
        add_include( source, "wine/exception.h", 1 );
        add_include( source, header, 0 );
    }
    else if (strendswith( source->name, "_s.c" ))
    {
        add_include( source, "wine/exception.h", 1 );
        add_include( source, header, 0 );
    }

    free( header );
}

/*******************************************************************
 *         is_generated_idl
 */
static int is_generated_idl( struct incl_file *source )
{
    return (strendswith( source->name, "_c.c" ) ||
            strendswith( source->name, "_i.c" ) ||
            strendswith( source->name, "_p.c" ) ||
            strendswith( source->name, "_s.c" ));
}

/*******************************************************************
 *         parse_file
 */
static void parse_file( struct incl_file *source, int src )
{
    FILE *file;

    /* don't try to open certain types of files */
    if (strendswith( source->name, ".tlb" ) ||
        strendswith( source->name, ".x" ))
    {
        source->filename = xstrdup( source->name );
        return;
    }

    file = src ? open_src_file( source ) : open_include_file( source );
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
    else if (strendswith( source->filename, ".man.in" ))
        parse_man_page( source, file );
    fclose(file);
    input_file_name = NULL;
}


/*******************************************************************
 *         add_src_file
 *
 * Add a source file to the list.
 */
static struct incl_file *add_src_file( const char *name )
{
    struct incl_file *file;

    if ((file = find_src_file( name ))) return file;  /* we already have it */
    file = xmalloc( sizeof(*file) );
    memset( file, 0, sizeof(*file) );
    file->name = xstrdup(name);
    list_add_tail( &sources, &file->entry );

    /* special cases for generated files */

    if (is_generated_idl( file ))
    {
        parse_generated_idl( file );
        return file;
    }

    if (!strcmp( file->name, "dlldata.o" ))
    {
        file->filename = xstrdup( "dlldata.c" );
        add_include( file, "objbase.h", 1 );
        add_include( file, "rpcproxy.h", 1 );
        return file;
    }

    if (!strcmp( file->name, "testlist.o" ))
    {
        file->filename = xstrdup( "testlist.c" );
        add_include( file, "wine/test.h", 1 );
        return file;
    }

    if (strendswith( file->name, ".o" ))
    {
        /* default to .c for unknown extra object files */
        file->filename = replace_extension( file->name, 2, ".c" );
        return file;
    }

    if (strendswith( file->name, ".tlb" ) ||
        strendswith( file->name, ".res" ) ||
        strendswith( file->name, ".pot" ) ||
        strendswith( file->name, ".x" ))
    {
        file->filename = xstrdup( file->name );
        return file;
    }

    parse_file( file, 1 );
    return file;
}


/*******************************************************************
 *         add_generated_sources
 */
static void add_generated_sources(void)
{
    unsigned int i;
    struct incl_file *source, *next;

    LIST_FOR_EACH_ENTRY_SAFE( source, next, &sources, struct incl_file, entry )
    {
        if (!source->flags) continue;
        for (i = 0; i < sizeof(idl_outputs) / sizeof(idl_outputs[0]); i++)
        {
            if (!(source->flags & idl_outputs[i].flag)) continue;
            if (!strendswith( idl_outputs[i].ext, ".c" )) continue;
            add_src_file( replace_extension( source->name, 4, idl_outputs[i].ext ));
        }
        if (source->flags & FLAG_IDL_PROXY) add_src_file( "dlldata.o" );
    }

    LIST_FOR_EACH_ENTRY_SAFE( source, next, &sources, struct incl_file, entry )
    {
        if (strendswith( source->name, "_c.c" ) ||
            strendswith( source->name, "_i.c" ) ||
            strendswith( source->name, "_p.c" ) ||
            strendswith( source->name, "_s.c" ) ||
            strendswith( source->name, ".tlb" ))
        {
            char *idl = replace_extension( source->name, 4, ".idl" );
            struct incl_file *file = add_src_file( idl );
            if (strendswith( source->name, "_c.c" )) file->flags |= FLAG_IDL_CLIENT;
            else if (strendswith( source->name, "_i.c" )) file->flags |= FLAG_IDL_IDENT;
            else if (strendswith( source->name, "_p.c" )) file->flags |= FLAG_IDL_PROXY;
            else if (strendswith( source->name, "_s.c" )) file->flags |= FLAG_IDL_SERVER;
            else if (strendswith( source->name, ".tlb" )) file->flags |= FLAG_IDL_TYPELIB;
            continue;
        }
        if (strendswith( source->name, "_r.res" ) ||
            strendswith( source->name, "_t.res" ))
        {
            char *idl = replace_extension( source->name, 6, ".idl" );
            struct incl_file *file = add_src_file( idl );
            if (strendswith( source->name, "_r.res" )) file->flags |= FLAG_IDL_REGISTER;
            else if (strendswith( source->name, "_t.res" )) file->flags |= FLAG_IDL_TYPELIB;
            continue;
        }
        if (strendswith( source->name, ".pot" ))
        {
            char *rc = replace_extension( source->name, 4, ".rc" );
            struct incl_file *file = add_src_file( rc );
            file->flags |= FLAG_RC_PO;
        }
    }
}


/*******************************************************************
 *         output_include
 */
static void output_include( struct incl_file *pFile, struct incl_file *owner, int *column )
{
    int i;

    if (pFile->owner == owner) return;
    if (!pFile->filename) return;
    pFile->owner = owner;
    output_filename( pFile->filename, column );
    for (i = 0; i < MAX_INCLUDES; i++)
        if (pFile->files[i]) output_include( pFile->files[i], owner, column );
}


/*******************************************************************
 *         output_sources
 */
static void output_sources(void)
{
    struct incl_file *source;
    struct strarray clean_files, subdirs;
    int i, column, po_srcs = 0, mc_srcs = 0;
    int is_test = find_src_file( "testlist.o" ) != NULL;

    strarray_init( &clean_files );
    strarray_init( &subdirs );

    LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
    {
        char *obj = xstrdup( source->name );
        char *ext = get_extension( obj );

        if (!ext) fatal_error( "unsupported file type %s\n", source->name );
        *ext++ = 0;
        column = 0;

        if (!strcmp( ext, "y" ))  /* yacc file */
        {
            /* add source file dependency for parallel makes */
            char *header = strmake( "%s.tab.h", obj );

            if (find_include_file( header ))
            {
                output( "%s.tab.h: %s\n", obj, source->filename );
                output( "\t$(BISON) $(BISONFLAGS) -p %s_ -o %s.tab.c -d %s\n",
                        obj, obj, source->filename );
                output( "%s.tab.c: %s %s\n", obj, source->filename, header );
                strarray_add( &clean_files, strmake( "%s.tab.h", obj ));
            }
            else output( "%s.tab.c: %s\n", obj, source->filename );

            output( "\t$(BISON) $(BISONFLAGS) -p %s_ -o $@ %s\n", obj, source->filename );
            output( "%s.tab.o: %s.tab.c\n", obj, obj );
            output( "\t$(CC) -c $(ALLCFLAGS) -o $@ %s.tab.c\n", obj );
            strarray_add( &clean_files, strmake( "%s.tab.c", obj ));
            strarray_add( &clean_files, strmake( "%s.tab.o", obj ));
            column += output( "%s.tab.o:", obj );
            free( header );
        }
        else if (!strcmp( ext, "l" ))  /* lex file */
        {
            output( "%s.yy.c: %s\n", obj, source->filename );
            output( "\t$(FLEX) $(LEXFLAGS) -o$@ %s\n", source->filename );
            output( "%s.yy.o: %s.yy.c\n", obj, obj );
            output( "\t$(CC) -c $(ALLCFLAGS) -o $@ %s.yy.c\n", obj );
            strarray_add( &clean_files, strmake( "%s.yy.c", obj ));
            strarray_add( &clean_files, strmake( "%s.yy.o", obj ));
            column += output( "%s.yy.o:", obj );
        }
        else if (!strcmp( ext, "rc" ))  /* resource file */
        {
            if (source->flags & FLAG_RC_PO)
            {
                output( "%s.res: $(WRC) $(ALL_MO_FILES) %s\n", obj, source->filename );
                output( "\t$(WRC) $(RCFLAGS) -o $@ %s\n", source->filename );
                column += output( "%s.res rsrc.pot:", obj );
                po_srcs++;
            }
            else
            {
                output( "%s.res: $(WRC) %s\n", obj, source->filename );
                output( "\t$(WRC) $(RCFLAGS) -o $@ %s\n", source->filename );
                column += output( "%s.res:", obj );
            }
            strarray_add( &clean_files, strmake( "%s.res", obj ));
        }
        else if (!strcmp( ext, "mc" ))  /* message file */
        {
            output( "%s.res: $(WMC) $(ALL_MO_FILES) %s\n", obj, source->filename );
            output( "\t$(WMC) -U -O res $(PORCFLAGS) -o $@ %s\n", source->filename );
            strarray_add( &clean_files, strmake( "%s.res", obj ));
            mc_srcs++;
            column += output( "msg.pot %s.res:", obj );
        }
        else if (!strcmp( ext, "idl" ))  /* IDL file */
        {
            char *targets[8];
            unsigned int i, nb_targets = 0;

            if (!source->flags || find_include_file( strmake( "%s.h", obj )))
                source->flags |= FLAG_IDL_HEADER;

            for (i = 0; i < sizeof(idl_outputs) / sizeof(idl_outputs[0]); i++)
            {
                if (!(source->flags & idl_outputs[i].flag)) continue;
                output( "%s%s: $(WIDL)\n", obj, idl_outputs[i].ext );
                output( "\t$(WIDL) %s -o $@ %s\n", idl_outputs[i].widl_arg, source->filename );
                targets[nb_targets++] = strmake( "%s%s", obj, idl_outputs[i].ext );
            }
            for (i = 0; i < nb_targets; i++)
            {
                column += output( "%s%c", targets[i], i < nb_targets - 1 ? ' ' : ':' );
                strarray_add( &clean_files, targets[i] );
            }
            column += output( " %s", source->filename );
        }
        else if (!strcmp( ext, "in" ))  /* man page */
        {
            if (strendswith( obj, ".man" ) && source->sourcename)
            {
                char *dir, *dest = replace_extension( obj, 4, "" );
                char *lang = strchr( dest, '.' );
                if (lang)
                {
                    *lang++ = 0;
                    dir = strmake( "$(DESTDIR)$(mandir)/%s/man%s", lang, source->sourcename );
                }
                else dir = strmake( "$(DESTDIR)$(mandir)/man%s", source->sourcename );
                output( "install-man-pages:: %s %s\n", obj, dir );
                output( "\t$(INSTALL_DATA) %s %s/%s.%s\n",
                        obj, dir, dest, source->sourcename );
                output( "uninstall::\n" );
                output( "\t$(RM) %s/%s.%s\n",
                        dir, dest, source->sourcename );
                free( dest );
                strarray_add( &subdirs, dir );
            }
            strarray_add( &clean_files, xstrdup(obj) );
            output( "%s: %s\n", obj, source->filename );
            output( "\t$(SED_CMD) %s >$@ || ($(RM) $@ && false)\n", source->filename );
            column += output( "%s:", obj );
        }
        else if (!strcmp( ext, "tlb" ) || !strcmp( ext, "res" ) || !strcmp( ext, "pot" ))
        {
            continue;  /* nothing to do for typelib files */
        }
        else
        {
            struct object_extension *obj_ext;
            LIST_FOR_EACH_ENTRY( obj_ext, &object_extensions, struct object_extension, entry )
            {
                strarray_add( &clean_files, strmake( "%s.%s", obj, obj_ext->extension ));
                output( "%s.%s: %s\n", obj, obj_ext->extension, source->filename );
                if (strstr( obj_ext->extension, "cross" ))
                    output( "\t$(CROSSCC) -c $(ALLCROSSCFLAGS) -o $@ %s\n", source->filename );
                else
                    output( "\t$(CC) -c $(ALLCFLAGS) -o $@ %s\n", source->filename );
            }
            if (source->flags & FLAG_C_IMPLIB)
            {
                strarray_add( &clean_files, strmake( "%s.cross.o", obj ));
                output( "%s.cross.o: %s\n", obj, source->filename );
                output( "\t$(CROSSCC) -c $(ALLCROSSCFLAGS) -o $@ %s\n", source->filename );
            }
            if (is_test && !strcmp( ext, "c" ) && !is_generated_idl( source ))
            {
                output( "%s.ok:\n", obj );
                output( "\t$(RUNTEST) $(RUNTESTFLAGS) %s && touch $@\n", obj );
            }
            LIST_FOR_EACH_ENTRY( obj_ext, &object_extensions, struct object_extension, entry )
                column += output( "%s.%s ", obj, obj_ext->extension );
            if (source->flags & FLAG_C_IMPLIB) column += output( "%s.cross.o", obj );
            column += output( ":" );
        }
        free( obj );

        for (i = 0; i < MAX_INCLUDES; i++)
            if (source->files[i]) output_include( source->files[i], source, &column );
        output( "\n" );
    }

    /* rules for files that depend on multiple sources */

    if (po_srcs)
    {
        column = output( "rsrc.pot: $(WRC)" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (source->flags & FLAG_RC_PO) output_filename( source->filename, &column );
        output( "\n" );
        column = output( "\t$(WRC) $(RCFLAGS) -O pot -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (source->flags & FLAG_RC_PO) output_filename( source->filename, &column );
        output( "\n" );
        strarray_add( &clean_files, "rsrc.pot" );
    }

    if (mc_srcs)
    {
        column = output( "msg.pot: $(WMC)" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".mc" )) output_filename( source->filename, &column );
        output( "\n" );
        column = output( "\t$(WMC) -O pot -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".mc" )) output_filename( source->filename, &column );
        output( "\n" );
        strarray_add( &clean_files, "msg.pot" );
    }

    if (find_src_file( "dlldata.o" ))
    {
        output( "dlldata.c: $(WIDL) Makefile.in\n" );
        column = output( "\t$(WIDL) $(IDLFLAGS) --dlldata-only -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (source->flags & FLAG_IDL_PROXY) output_filename( source->filename, &column );
        output( "\n" );
        strarray_add( &clean_files, "dlldata.c" );
    }

    if (is_test)
    {
        output( "testlist.c: $(MAKECTESTS) Makefile.in\n" );
        column = output( "\t$(MAKECTESTS) -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".c" ) && !is_generated_idl( source ))
                output_filename( source->name, &column );
        output( "\n" );
        column = output( "check test:" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".c" ) && !is_generated_idl( source ))
                output_filename( replace_extension( source->name, 2, ".ok" ), &column );
        output( "\n" );
        output( "clean testclean::\n" );
        column = output( "\t$(RM)" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".c" ) && !is_generated_idl( source ))
                output_filename( replace_extension( source->name, 2, ".ok" ), &column );
        output( "\n" );
        strarray_add( &clean_files, "testlist.c" );
    }

    if (clean_files.count)
    {
        output( "clean::\n" );
        column = output( "\t$(RM)" );
        for (i = 0; i < clean_files.count; i++) output_filename( clean_files.str[i], &column );
        output( "\n" );
    }

    if (subdirs.count)
    {
        for (i = column = 0; i < subdirs.count; i++) output_filename( subdirs.str[i], &column );
        output( ":\n" );
        output( "\t$(MKDIR_P) -m 755 $@\n" );
    }
}


/*******************************************************************
 *         create_temp_file
 */
static FILE *create_temp_file( char **tmp_name )
{
    char *name = xmalloc( strlen(OutputFileName) + 13 );
    unsigned int i, id = getpid();
    int fd;
    FILE *ret = NULL;

    for (i = 0; i < 100; i++)
    {
        sprintf( name, "%s.tmp%08x", OutputFileName, id );
        if ((fd = open( name, O_RDWR | O_CREAT | O_EXCL, 0666 )) != -1)
        {
            ret = fdopen( fd, "w" );
            break;
        }
        if (errno != EEXIST) break;
        id += 7777;
    }
    if (!ret) fatal_error( "failed to create output file for '%s'\n", OutputFileName );
    *tmp_name = name;
    return ret;
}


/*******************************************************************
 *         output_dependencies
 */
static void output_dependencies(void)
{
    char *tmp_name = NULL;

    if (Separator && ((output_file = fopen( OutputFileName, "r" ))))
    {
        char buffer[1024];
        FILE *tmp_file = create_temp_file( &tmp_name );
        int found = 0;

        while (fgets( buffer, sizeof(buffer), output_file ) && !found)
        {
            if (fwrite( buffer, 1, strlen(buffer), tmp_file ) != strlen(buffer))
                fatal_error( "failed to write to %s\n", tmp_name );
            found = !strncmp( buffer, Separator, strlen(Separator) );
        }
        fclose( output_file );
        output_file = tmp_file;
        if (!found && list_head(&sources)) output( "\n%s\n", Separator );
    }
    else
    {
        if (!(output_file = fopen( OutputFileName, Separator ? "a" : "w" )))
            fatal_perror( "%s", OutputFileName );
    }

    output_sources();

    fclose( output_file );
    output_file = NULL;

    if (tmp_name)
    {
        int ret = rename( tmp_name, OutputFileName );
        if (ret == -1 && errno == EEXIST)
        {
            /* rename doesn't overwrite on windows */
            unlink( OutputFileName );
            ret = rename( tmp_name, OutputFileName );
        }
        if (ret == -1)
        {
            unlink( tmp_name );
            fatal_error( "failed to rename output file to '%s'\n", OutputFileName );
        }
        free( tmp_name );
    }
}


/*******************************************************************
 *         parse_option
 */
static void parse_option( const char *opt )
{
    switch(opt[1])
    {
    case 'I':
        if (opt[2]) add_include_path( opt + 2 );
        break;
    case 'C':
        src_dir = opt + 2;
        break;
    case 'S':
        top_src_dir = opt + 2;
        break;
    case 'T':
        top_obj_dir = opt + 2;
        break;
    case 'P':
        parent_dir = opt + 2;
        break;
    case 'f':
        if (opt[2]) OutputFileName = opt + 2;
        break;
    case 'R':
        relative_dir_mode = 1;
        break;
    case 's':
        if (opt[2]) Separator = opt + 2;
        else Separator = NULL;
        break;
    case 'x':
        if (opt[2]) add_object_extension( opt + 2 );
        break;
    default:
        fprintf( stderr, "Unknown option '%s'\n%s", opt, Usage );
        exit(1);
    }
}


/*******************************************************************
 *         main
 */
int main( int argc, char *argv[] )
{
    struct incl_file *pFile;
    struct incl_path *path, *next;
    int i, j;

    i = 1;
    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            parse_option( argv[i] );
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

    /* ignore redundant source paths */
    if (src_dir && !strcmp( src_dir, "." )) src_dir = NULL;
    if (top_src_dir && top_obj_dir && !strcmp( top_src_dir, top_obj_dir )) top_src_dir = NULL;

    /* set the default extension list for object files */
    if (list_empty( &object_extensions ))
        add_object_extension( "o" );

    /* get rid of absolute paths that don't point into the source dir */
    LIST_FOR_EACH_ENTRY_SAFE( path, next, &paths, struct incl_path, entry )
    {
        if (path->name[0] != '/') continue;
        if (top_src_dir)
        {
            if (!strncmp( path->name, top_src_dir, strlen(top_src_dir) ) &&
                path->name[strlen(top_src_dir)] == '/') continue;
        }
        list_remove( &path->entry );
        free( path );
    }

    for (i = 1; i < argc; i++) add_src_file( argv[i] );
    add_generated_sources();

    LIST_FOR_EACH_ENTRY( pFile, &includes, struct incl_file, entry ) parse_file( pFile, 0 );
    output_dependencies();
    return 0;
}
