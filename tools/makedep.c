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
    int                system;        /* is it a system include (#include <name>) */
    struct incl_file  *owner;
    struct incl_file  *files[MAX_INCLUDES];
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

static const char *src_dir;
static const char *top_src_dir;
static const char *top_obj_dir;
static const char *OutputFileName = "Makefile";
static const char *Separator = "### Dependencies";
static const char *input_file_name;
static int input_line;
static FILE *output_file;

static const char Usage[] =
    "Usage: makedep [options] [files]\n"
    "Options:\n"
    "   -Idir   Search for include files in directory 'dir'\n"
    "   -Cdir   Search for source files in directory 'dir'\n"
    "   -Sdir   Set the top source directory\n"
    "   -Tdir   Set the top object directory\n"
    "   -fxxx   Store output in file 'xxx' (default: Makefile)\n"
    "   -sxxx   Use 'xxx' as separator (default: \"### Dependencies\")\n";


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
        fprintf( stderr, " error:" );
    }
    else fprintf( stderr, "makedep: error:" );
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
 *         find_target_src_file
 *
 * Check if we have a source file as a target for the specified source with a different extension.
 */
static struct incl_file *find_target_src_file( const char *name, const char *ext )
{
    struct incl_file *ret;
    char *p, *match = xmalloc( strlen( name ) + strlen( ext ) + 1 );

    strcpy( match, name );
    if ((p = get_extension( match ))) strcpy( p, ext );
    else strcat( match, ext );
    ret = find_src_file( match );
    free( match );
    return ret;
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
    include->system = system;
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
        if (src_dir)
            filename = strmake( "%s/%.*s.y", src_dir, strlen(pFile->name) - 6, pFile->name );
        else
            filename = strmake( "%.*s.y", strlen(pFile->name) - 6, pFile->name );

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

    /* check for generated message resource */

    if (strendswith( pFile->name, ".mc.rc" ))
    {
        if (src_dir)
            filename = strmake( "%s/%s", src_dir, pFile->name );
        else
            filename = xstrdup( pFile->name );

        filename[strlen(filename) - 3] = 0;

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
        if (src_dir)
            filename = strmake( "%s/%.*s.idl", src_dir, strlen(pFile->name) - 2, pFile->name );
        else
            filename = strmake( "%.*s.idl", strlen(pFile->name) - 2, pFile->name );

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

    /* check for corresponding idl file in global includes */

    if (strendswith( pFile->name, ".h" ))
    {
        if (top_src_dir)
            filename = strmake( "%s/include/%.*s.idl",
                                top_src_dir, strlen(pFile->name) - 2, pFile->name );
        else if (top_obj_dir)
            filename = strmake( "%s/include/%.*s.idl",
                                top_obj_dir, strlen(pFile->name) - 2, pFile->name );
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
        if (top_src_dir)
            filename = strmake( "%s/include/%.*s.x",
                                top_src_dir, strlen(pFile->name) - 2, pFile->name );
        else if (top_obj_dir)
            filename = strmake( "%s/include/%.*s.x",
                                top_obj_dir, strlen(pFile->name) - 2, pFile->name );
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
    if (pFile->system) return NULL;  /* ignore system files we cannot find */

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

        /* check for normal #include */
        if (*p++ != '#') continue;
        while (*p && isspace(*p)) p++;
        if (strncmp( p, "include", 7 )) continue;
        p += 7;
        while (*p && isspace(*p)) p++;
        if (*p != '\"' && *p != '<' ) continue;
        quote = *p++;
        if (quote == '<') quote = '>';
        include = p;
        while (*p && (*p != quote)) p++;
        if (!*p) fatal_error( "malformed #include directive\n" );
        *p = 0;
        add_include( pFile, include, (quote == '>') );
    }
}

/*******************************************************************
 *         parse_c_file
 */
static void parse_c_file( struct incl_file *pFile, FILE *file )
{
    char *buffer, *include;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        char quote;
        char *p = buffer;
        while (*p && isspace(*p)) p++;
        if (*p++ != '#') continue;
        while (*p && isspace(*p)) p++;
        if (!strncmp( p, "include", 7 )) p += 7;
        else if (!strncmp( p, "import", 6 )) p += 6;
        else continue;
        while (*p && isspace(*p)) p++;
        if (*p != '\"' && *p != '<' ) continue;
        quote = *p++;
        if (quote == '<') quote = '>';
        include = p;
        while (*p && (*p != quote)) p++;
        if (!*p) fatal_error( "malformed #include directive\n" );
        *p = 0;
        add_include( pFile, include, (quote == '>') );
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
        }
        else  /* check for #include */
        {
            if (*p++ != '#') continue;
            while (*p && isspace(*p)) p++;
            if (strncmp( p, "include", 7 )) continue;
            p += 7;
            while (*p && isspace(*p)) p++;
            if (*p != '\"' && *p != '<' ) continue;
            quote = *p++;
            if (quote == '<') quote = '>';
            include = p;
            while (*p && (*p != quote)) p++;
            if (!*p) fatal_error( "malformed #include directive\n" );
            *p = 0;
        }
        add_include( pFile, include, (quote == '>') );
    }
}


/*******************************************************************
 *         parse_generated_idl
 */
static void parse_generated_idl( struct incl_file *source )
{
    char *header, *basename;

    basename = xstrdup( source->name );
    basename[strlen(basename) - 4] = 0;
    header = strmake( "%s.h", basename );
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
    free( basename );
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

    /* special case for source files generated from idl */
    if (is_generated_idl( source ))
    {
        parse_generated_idl( source );
        return;
    }

    if (!strcmp( source->name, "dlldata.o" ))
    {
        source->filename = xstrdup( "dlldata.c" );
        add_include( source, "objbase.h", 1 );
        add_include( source, "rpcproxy.h", 1 );
        return;
    }

    if (!strcmp( source->name, "testlist.o" ))
    {
        source->filename = xstrdup( "testlist.c" );
        add_include( source, "wine/test.h", 1 );
        return;
    }

    if (strendswith( source->name, ".o" ))
    {
        /* default to .c for unknown extra object files */
        source->filename = xstrdup( source->name );
        source->filename[strlen(source->filename) - 1] = 'c';
        return;
    }

    /* don't try to open certain types of files */
    if (strendswith( source->name, ".tlb" ) ||
        strendswith( source->name, ".res" ) ||
        strendswith( source->name, ".pot" ) ||
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

    if (find_src_file( name )) return NULL;  /* we already have it */
    file = xmalloc( sizeof(*file) );
    memset( file, 0, sizeof(*file) );
    file->name = xstrdup(name);
    list_add_tail( &sources, &file->entry );
    parse_file( file, 1 );
    return file;
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
    int i, column, po_srcs = 0, mc_srcs = 0;

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
            }
            else output( "%s.tab.c: %s\n", obj, source->filename );

            output( "\t$(BISON) $(BISONFLAGS) -p %s_ -o $@ %s\n", obj, source->filename );
            column += output( "%s.tab.o: %s.tab.c", obj, obj );
            free( header );
        }
        else if (!strcmp( ext, "l" ))  /* lex file */
        {
            output( "%s.yy.c: %s\n", obj, source->filename );
            output( "\t$(FLEX) $(LEXFLAGS) -o$@ %s\n", source->filename );
            column += output( "%s.yy.o: %s.yy.c", obj, obj );
        }
        else if (!strcmp( ext, "rc" ))  /* resource file */
        {
            if (find_target_src_file( source->name, ".pot" ))
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
        }
        else if (!strcmp( ext, "mc" ))  /* message file */
        {
            output( "%s.res: $(WMC) $(ALL_MO_FILES) %s\n", obj, source->filename );
            output( "\t$(WMC) -U -O res $(PORCFLAGS) -o $@ %s\n", source->filename );
            mc_srcs++;
            column += output( "msg.pot %s.res:", obj );
        }
        else if (!strcmp( ext, "idl" ))  /* IDL file */
        {
            char *targets[8];
            int nb_targets = 0;
            char ending[] = "_?.c";
            const char *suffix;
            char *header = strmake( "%s.h", obj );

            if (find_target_src_file( source->name, ".tlb" ))
            {
                output( "%s.tlb %s_t.res: $(WIDL)\n", obj, obj );
                output( "\t$(WIDL) $(TARGETFLAGS) $(IDLFLAGS) -t -o $@ %s\n", source->filename );
                targets[nb_targets++] = strmake( "%s.tlb", obj );
                targets[nb_targets++] = strmake( "%s_t.res", obj );
            }

            for (suffix = "cips"; *suffix; suffix++)
            {
                ending[1] = *suffix;
                if (!find_target_src_file( source->name, ending )) continue;
                output( "%s%s: $(WIDL)\n", obj, ending );
                output( "\t$(WIDL) $(IDLFLAGS) -%c -o $@ %s\n",
                        *suffix == 'i' ? 'u' : *suffix, source->filename );
                targets[nb_targets++] = strmake( "%s%s", obj, ending );
            }

            if (find_target_src_file( source->name, "_r.res" ))
            {
                output( "%s_r.res: $(WIDL)\n", obj );
                output( "\t$(WIDL) $(IDLFLAGS) -r -o $@ %s\n", source->filename );
                targets[nb_targets++] = strmake( "%s_r.res", obj );
            }

            if (!nb_targets || find_include_file( header ))
            {
                output( "%s.h: $(WIDL)\n", obj );
                output( "\t$(WIDL) $(IDLFLAGS) -h -o $@ %s\n", source->filename );
                targets[nb_targets++] = header;
            }
            else free( header );

            for (i = 0; i < nb_targets; i++)
            {
                column += output( "%s%c", targets[i], i < nb_targets - 1 ? ' ' : ':' );
                free( targets[i] );
            }
            column += output( " %s", source->filename );
        }
        else if (!strcmp( ext, "tlb" ) || !strcmp( ext, "res" ) || !strcmp( ext, "pot" ))
        {
            continue;  /* nothing to do for typelib files */
        }
        else
        {
            struct object_extension *ext;
            LIST_FOR_EACH_ENTRY( ext, &object_extensions, struct object_extension, entry )
                column += output( "%s.%s ", obj, ext->extension );
            column += output( ": %s", source->filename );
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
            if (strendswith( source->name, ".rc" ) && find_target_src_file( source->name, ".pot" ))
                output_filename( source->filename, &column );
        output( "\n" );
        column = output( "\t$(WRC) $(RCFLAGS) -O pot -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".rc" ) && find_target_src_file( source->name, ".pot" ))
                output_filename( source->filename, &column );
        output( "\n" );
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
    }

    if (find_src_file( "dlldata.o" ))
    {
        output( "dlldata.c: $(WIDL) Makefile.in\n" );
        column = output( "\t$(WIDL) $(IDLFLAGS) --dlldata-only -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".idl" ) && find_target_src_file( source->name, "_p.c" ))
                output_filename( source->filename, &column );
        output( "\n" );
    }

    if (find_src_file( "testlist.o" ))
    {
        output( "testlist.c: $(MAKECTESTS) Makefile.in\n" );
        column = output( "\t$(MAKECTESTS) -o $@" );
        LIST_FOR_EACH_ENTRY( source, &sources, struct incl_file, entry )
            if (strendswith( source->name, ".c" ) && !is_generated_idl( source ))
                output_filename( source->filename, &column );
        output( "\n" );
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
    case 'f':
        if (opt[2]) OutputFileName = opt + 2;
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

    LIST_FOR_EACH_ENTRY( pFile, &includes, struct incl_file, entry ) parse_file( pFile, 0 );
    output_dependencies();
    return 0;
}
