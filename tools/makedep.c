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

typedef struct _INCL_FILE
{
    struct list        entry;
    char              *name;
    char              *filename;
    struct _INCL_FILE *included_by;   /* file that included this one */
    int                included_line; /* line where this file was included */
    int                system;        /* is it a system include (#include <name>) */
    struct _INCL_FILE *owner;
    struct _INCL_FILE *files[MAX_INCLUDES];
} INCL_FILE;

static struct list sources = LIST_INIT(sources);
static struct list includes = LIST_INIT(includes);

typedef struct _INCL_PATH
{
    struct list entry;
    const char *name;
} INCL_PATH;

static struct list paths = LIST_INIT(paths);

static const char *src_dir;
static const char *top_src_dir;
static const char *top_obj_dir;
static const char *OutputFileName = "Makefile";
static const char *Separator = "### Dependencies";
static const char *ProgramName;
static int input_line;

static const char Usage[] =
    "Usage: %s [options] [files]\n"
    "Options:\n"
    "   -Idir   Search for include files in directory 'dir'\n"
    "   -Cdir   Search for source files in directory 'dir'\n"
    "   -Sdir   Set the top source directory\n"
    "   -Sdir   Set the top object directory\n"
    "   -fxxx   Store output in file 'xxx' (default: Makefile)\n"
    "   -sxxx   Use 'xxx' as separator (default: \"### Dependencies\")\n";


/*******************************************************************
 *         fatal_error
 */
static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    vfprintf( stderr, msg, valist );
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
        fatal_error( "%s: Virtual memory exhausted.\n", ProgramName );
    return res;
}


/*******************************************************************
 *         xrealloc
 */
void *xrealloc (void *ptr, size_t size)
{
    void *res;
    assert( size );
    if (!(res = realloc( ptr, size )))
        fatal_error( "%s: Virtual memory exhausted.\n", ProgramName );
    return res;
}

/*******************************************************************
 *         xstrdup
 */
static char *xstrdup( const char *str )
{
    char *res = strdup( str );
    if (!res) fatal_error( "%s: Virtual memory exhausted.\n", ProgramName );
    return res;
}


/*******************************************************************
 *         strmake
 */
char *strmake( const char* fmt, ... )
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
            fgets( buffer + size - 1, size + 1, file );
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
                fgets( p, size - (p - buffer), file );
                input_line++;
                continue;
            }
        }
        return buffer;
    }
}

/*******************************************************************
 *         is_generated
 *
 * Test if a given file type is generated during the make process
 */
static int is_generated( const char *name )
{
    static const char * const extensions[] = { ".tab.h", ".mc.rc" };
    size_t i, len = strlen(name);
    for (i = 0; i < sizeof(extensions)/sizeof(extensions[0]); i++)
    {
        if (len <= strlen(extensions[i])) continue;
        if (!strcmp( name + len - strlen(extensions[i]), extensions[i] )) return 1;
    }
    return 0;
}

/*******************************************************************
 *         add_include_path
 *
 * Add a directory to the include path.
 */
static void add_include_path( const char *name )
{
    INCL_PATH *path = xmalloc( sizeof(*path) );
    list_add_tail( &paths, &path->entry );
    path->name = name;
}


/*******************************************************************
 *         add_src_file
 *
 * Add a source file to the list.
 */
static INCL_FILE *add_src_file( const char *name )
{
    INCL_FILE *file = xmalloc( sizeof(*file) );
    memset( file, 0, sizeof(*file) );
    file->name = xstrdup(name);
    list_add_tail( &sources, &file->entry );
    return file;
}


/*******************************************************************
 *         add_include
 *
 * Add an include file if it doesn't already exists.
 */
static INCL_FILE *add_include( INCL_FILE *pFile, const char *name, int line, int system )
{
    INCL_FILE *include;
    char *ext;
    int pos;

    for (pos = 0; pos < MAX_INCLUDES; pos++) if (!pFile->files[pos]) break;
    if (pos >= MAX_INCLUDES)
        fatal_error( "%s: %s: too many included files, please fix MAX_INCLUDES\n",
                     ProgramName, pFile->name );

    /* enforce some rules for the Wine tree */

    if (!memcmp( name, "../", 3 ))
        fatal_error( "%s:%d: #include directive with relative path not allowed\n",
                     pFile->filename, line );

    if (!strcmp( name, "config.h" ))
    {
        if ((ext = strrchr( pFile->filename, '.' )) && !strcmp( ext, ".h" ))
            fatal_error( "%s:%d: config.h must not be included by a header file\n",
                         pFile->filename, line );
        if (pos)
            fatal_error( "%s:%d: config.h must be included before anything else\n",
                         pFile->filename, line );
    }
    else if (!strcmp( name, "wine/port.h" ))
    {
        if ((ext = strrchr( pFile->filename, '.' )) && !strcmp( ext, ".h" ))
            fatal_error( "%s:%d: wine/port.h must not be included by a header file\n",
                         pFile->filename, line );
        if (!pos) fatal_error( "%s:%d: config.h must be included before wine/port.h\n",
                               pFile->filename, line );
        if (pos > 1)
            fatal_error( "%s:%d: wine/port.h must be included before everything except config.h\n",
                         pFile->filename, line );
        if (strcmp( pFile->files[0]->name, "config.h" ))
            fatal_error( "%s:%d: config.h must be included before wine/port.h\n",
                         pFile->filename, line );
    }

    LIST_FOR_EACH_ENTRY( include, &includes, INCL_FILE, entry )
        if (!strcmp( name, include->name )) goto found;

    include = xmalloc( sizeof(INCL_FILE) );
    memset( include, 0, sizeof(INCL_FILE) );
    include->name = xstrdup(name);
    include->included_by = pFile;
    include->included_line = line;
    include->system = system || pFile->system;
    list_add_tail( &includes, &include->entry );
found:
    pFile->files[pos] = include;
    return include;
}


/*******************************************************************
 *         open_src_file
 */
static FILE *open_src_file( INCL_FILE *pFile )
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
    if (!file)
    {
        perror( pFile->name );
        exit(1);
    }
    return file;
}


/*******************************************************************
 *         open_include_file
 */
static FILE *open_include_file( INCL_FILE *pFile )
{
    FILE *file = NULL;
    char *filename, *p;
    INCL_PATH *path;

    errno = ENOENT;

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
    LIST_FOR_EACH_ENTRY( path, &paths, INCL_PATH, entry )
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

    if (pFile->included_by->system) return NULL;  /* ignore if included by a system file */

    perror( pFile->name );
    while (pFile->included_by)
    {
        fprintf( stderr, "  %s was first included from %s:%d\n",
                 pFile->name, pFile->included_by->name, pFile->included_line );
        pFile = pFile->included_by;
    }
    exit(1);

found:
    pFile->filename = filename;
    return file;
}


/*******************************************************************
 *         parse_idl_file
 */
static void parse_idl_file( INCL_FILE *pFile, FILE *file )
{
    char *buffer, *include;

    input_line = 0;
    while ((buffer = get_line( file )))
    {
        char quote;
        char *p = buffer;
        while (*p && isspace(*p)) p++;

        if (!strncmp( p, "import", 6 ))
        {
            p += 6;
            while (*p && isspace(*p)) p++;
            if (*p != '\"') continue;
        }
        else
        {
            if (*p++ != '#') continue;
            while (*p && isspace(*p)) p++;
            if (strncmp( p, "include", 7 )) continue;
            p += 7;
            while (*p && isspace(*p)) p++;
            if (*p != '\"' && *p != '<' ) continue;
        }

        quote = *p++;
        if (quote == '<') quote = '>';
        include = p;
        while (*p && (*p != quote)) p++;
        if (!*p) fatal_error( "%s:%d: Malformed #include or import directive\n",
                              pFile->filename, input_line );
        *p = 0;
        add_include( pFile, include, input_line, (quote == '>') );
    }
}

/*******************************************************************
 *         parse_c_file
 */
static void parse_c_file( INCL_FILE *pFile, FILE *file )
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
        if (strncmp( p, "include", 7 )) continue;
        p += 7;
        while (*p && isspace(*p)) p++;
        if (*p != '\"' && *p != '<' ) continue;
        quote = *p++;
        if (quote == '<') quote = '>';
        include = p;
        while (*p && (*p != quote)) p++;
        if (!*p) fatal_error( "%s:%d: Malformed #include directive\n",
                              pFile->filename, input_line );
        *p = 0;
        add_include( pFile, include, input_line, (quote == '>') );
    }
}


/*******************************************************************
 *         parse_file
 */
static void parse_file( INCL_FILE *pFile, int src )
{
    char *ext;
    FILE *file;

    if (is_generated( pFile->name ))
    {
        /* file is generated during make, don't try to open it */
        pFile->filename = xstrdup( pFile->name );
        return;
    }

    file = src ? open_src_file( pFile ) : open_include_file( pFile );
    if (!file) return;
    ext = get_extension( pFile->name );
    if (ext && !strcmp( ext, ".idl" )) parse_idl_file( pFile, file );
    else parse_c_file( pFile, file );
    fclose(file);
}


/*******************************************************************
 *         output_include
 */
static void output_include( FILE *file, INCL_FILE *pFile,
                            INCL_FILE *owner, int *column )
{
    int i;

    if (pFile->owner == owner) return;
    if (!pFile->filename) return;
    pFile->owner = owner;
    if (*column + strlen(pFile->filename) + 1 > 70)
    {
        fprintf( file, " \\\n" );
        *column = 0;
    }
    fprintf( file, " %s", pFile->filename );
    *column += strlen(pFile->filename) + 1;
    for (i = 0; i < MAX_INCLUDES; i++)
        if (pFile->files[i]) output_include( file, pFile->files[i],
                                             owner, column );
}


/*******************************************************************
 *         output_src
 */
static void output_src( FILE *file, INCL_FILE *pFile, int *column )
{
    char *obj = xstrdup( pFile->name );
    char *ext = get_extension( obj );
    if (ext)
    {
        *ext++ = 0;
        if (!strcmp( ext, "y" ))  /* yacc file */
        {
            *column += fprintf( file, "%s.tab.o: %s.tab.c", obj, obj );
        }
        else if (!strcmp( ext, "l" ))  /* lex file */
        {
            *column += fprintf( file, "%s.o: %s.c", LEX_OUTPUT_ROOT, LEX_OUTPUT_ROOT );
        }
        else if (!strcmp( ext, "rc" ))  /* resource file */
        {
            *column += fprintf( file, "%s.res: %s", obj, pFile->filename );
        }
        else if (!strcmp( ext, "mc" ))  /* message file */
        {
            *column += fprintf( file, "%s.mc.rc: %s", obj, pFile->filename );
        }
        else if (!strcmp( ext, "idl" ))  /* IDL file */
        {
            *column += fprintf( file, "%s.h: %s", obj, pFile->filename );
        }
        else
        {
            *column += fprintf( file, "%s.o: %s", obj, pFile->filename );
        }
    }
    free( obj );
}


/*******************************************************************
 *         output_dependencies
 */
static void output_dependencies(void)
{
    INCL_FILE *pFile;
    int i, column;
    FILE *file = NULL;
    char *buffer;

    if (Separator && ((file = fopen( OutputFileName, "r+" ))))
    {
        while ((buffer = get_line( file )))
            if (!strncmp( buffer, Separator, strlen(Separator) )) break;
        ftruncate( fileno(file), ftell(file) );
        fseek( file, 0L, SEEK_END );
    }
    if (!file)
    {
        if (!(file = fopen( OutputFileName, Separator ? "a" : "w" )))
        {
            perror( OutputFileName );
            exit(1);
        }
    }
    LIST_FOR_EACH_ENTRY( pFile, &sources, INCL_FILE, entry )
    {
        column = 0;
        output_src( file, pFile, &column );
        for (i = 0; i < MAX_INCLUDES; i++)
            if (pFile->files[i]) output_include( file, pFile->files[i],
                                                 pFile, &column );
        fprintf( file, "\n" );
    }
    fclose(file);
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
    default:
        fprintf( stderr, "Unknown option '%s'\n", opt );
        fprintf( stderr, Usage, ProgramName );
        exit(1);
    }
}


/*******************************************************************
 *         main
 */
int main( int argc, char *argv[] )
{
    INCL_FILE *pFile;
    INCL_PATH *path, *next;
    int i, j;

    ProgramName = argv[0];

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

    /* get rid of absolute paths that don't point into the source dir */
    LIST_FOR_EACH_ENTRY_SAFE( path, next, &paths, INCL_PATH, entry )
    {
        if (path->name[0] != '/') continue;
        if (top_src_dir)
        {
            if (!strncmp( path->name, top_src_dir, strlen(top_src_dir) )) continue;
            if (path->name[strlen(top_src_dir)] == '/') continue;
        }
        list_remove( &path->entry );
        free( path );
    }

    for (i = 1; i < argc; i++)
    {
        pFile = add_src_file( argv[i] );
        parse_file( pFile, 1 );
    }
    LIST_FOR_EACH_ENTRY( pFile, &includes, INCL_FILE, entry ) parse_file( pFile, 0 );
    if (!list_empty( &sources )) output_dependencies();
    return 0;
}
