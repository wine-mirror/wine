/*
 * Helper functions for the Wine tools
 *
 * Copyright 2021 Alexandre Julliard
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

#ifndef __WINE_TOOLS_H
#define __WINE_TOOLS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
# include <direct.h>
# include <io.h>
# include <process.h>
# define mkdir(path,mode) mkdir(path)
# ifndef S_ISREG
#  define S_ISREG(mod) (((mod) & _S_IFMT) == _S_IFREG)
# endif
# ifdef _MSC_VER
#  define popen _popen
#  define pclose _pclose
#  define strtoll _strtoi64
#  define strtoull _strtoui64
#  define strncasecmp _strnicmp
#  define strcasecmp _stricmp
# endif
#else
# include <sys/wait.h>
# include <unistd.h>
# ifndef O_BINARY
#  define O_BINARY 0
# endif
# ifndef __int64
#   if defined(__x86_64__) || defined(__aarch64__) || defined(__powerpc64__)
#     define __int64 long
#   else
#     define __int64 long long
#   endif
# endif
#endif

#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif


static inline void *xmalloc( size_t size )
{
    void *res = malloc( size ? size : 1 );

    if (res == NULL)
    {
        fprintf( stderr, "Virtual memory exhausted.\n" );
        exit(1);
    }
    return res;
}

static inline void *xrealloc (void *ptr, size_t size)
{
    void *res = realloc( ptr, size );

    if (size && res == NULL)
    {
        fprintf( stderr, "Virtual memory exhausted.\n" );
        exit(1);
    }
    return res;
}

static inline char *xstrdup( const char *str )
{
    return strcpy( xmalloc( strlen(str)+1 ), str );
}

static inline int strendswith( const char *str, const char *end )
{
    int l = strlen( str );
    int m = strlen( end );
    return l >= m && !strcmp( str + l - m, end );
}

static char *strmake( const char* fmt, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));
static inline char *strmake( const char* fmt, ... )
{
    int n;
    size_t size = 100;
    va_list ap;

    for (;;)
    {
        char *p = xmalloc( size );
        va_start( ap, fmt );
	n = vsnprintf( p, size, fmt, ap );
	va_end( ap );
        if (n == -1) size *= 2;
        else if ((size_t)n >= size) size = n + 1;
        else return p;
        free( p );
    }
}

/* string array functions */

struct strarray
{
    unsigned int count;  /* strings in use */
    unsigned int size;   /* total allocated size */
    const char **str;
};

static const struct strarray empty_strarray;

static inline void strarray_add( struct strarray *array, const char *str )
{
    if (array->count == array->size)
    {
	if (array->size) array->size *= 2;
        else array->size = 16;
	array->str = xrealloc( array->str, sizeof(array->str[0]) * array->size );
    }
    array->str[array->count++] = str;
}

static inline void strarray_addall( struct strarray *array, struct strarray added )
{
    unsigned int i;

    for (i = 0; i < added.count; i++) strarray_add( array, added.str[i] );
}

static inline int strarray_exists( const struct strarray *array, const char *str )
{
    unsigned int i;

    for (i = 0; i < array->count; i++) if (!strcmp( array->str[i], str )) return 1;
    return 0;
}

static inline void strarray_add_uniq( struct strarray *array, const char *str )
{
    if (!strarray_exists( array, str )) strarray_add( array, str );
}

static inline void strarray_addall_uniq( struct strarray *array, struct strarray added )
{
    unsigned int i;

    for (i = 0; i < added.count; i++) strarray_add_uniq( array, added.str[i] );
}

static inline struct strarray strarray_fromstring( const char *str, const char *delim )
{
    struct strarray array = empty_strarray;
    char *buf = xstrdup( str );
    const char *tok;

    for (tok = strtok( buf, delim ); tok; tok = strtok( NULL, delim ))
        strarray_add( &array, xstrdup( tok ));
    free( buf );
    return array;
}

static inline struct strarray strarray_frompath( const char *path )
{
    if (!path) return empty_strarray;
#if defined(_WIN32) && !defined(__CYGWIN__)
    return strarray_fromstring( path, ";" );
#else
    return strarray_fromstring( path, ":" );
#endif
}

static inline char *strarray_tostring( struct strarray array, const char *sep )
{
    char *str;
    unsigned int i, len = 1 + (array.count - 1) * strlen(sep);

    if (!array.count) return xstrdup("");
    for (i = 0; i < array.count; i++) len += strlen( array.str[i] );
    str = xmalloc( len );
    strcpy( str, array.str[0] );
    for (i = 1; i < array.count; i++)
    {
        strcat( str, sep );
        strcat( str, array.str[i] );
    }
    return str;
}

static inline void strarray_qsort( struct strarray *array, int (*func)(const char **, const char **) )
{
    if (array->count) qsort( array->str, array->count, sizeof(*array->str), (void *)func );
}

static inline const char *strarray_bsearch( const struct strarray *array, const char *str,
                                            int (*func)(const char **, const char **) )
{
    char **res = NULL;

    if (array->count) res = bsearch( &str, array->str, array->count, sizeof(*array->str), (void *)func );
    return res ? *res : NULL;
}

static inline void strarray_trace( struct strarray args )
{
    unsigned int i;

    for (i = 0; i < args.count; i++)
    {
        if (strpbrk( args.str[i], " \t\n\r")) printf( "\"%s\"", args.str[i] );
        else printf( "%s", args.str[i] );
        putchar( i < args.count - 1 ? ' ' : '\n' );
    }
}

static inline int strarray_spawn( struct strarray args )
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    strarray_add( &args, NULL );
    return _spawnvp( _P_WAIT, args.str[0], args.str );
#else
    pid_t pid, wret;
    int status;

    if (!(pid = fork()))
    {
        strarray_add( &args, NULL );
        execvp( args.str[0], (char **)args.str );
        _exit(1);
    }
    if (pid == -1) return -1;

    while (pid != (wret = waitpid( pid, &status, 0 )))
        if (wret == -1 && errno != EINTR) break;

    if (pid == wret && WIFEXITED(status)) return WEXITSTATUS(status);
    return 255; /* abnormal exit with an abort or an interrupt */
#endif
}

static inline char *get_basename( const char *file )
{
    const char *ret = strrchr( file, '/' );
    return xstrdup( ret ? ret + 1 : file );
}

static inline char *get_basename_noext( const char *file )
{
    char *ext, *ret = get_basename( file );
    if ((ext = strrchr( ret, '.' ))) *ext = 0;
    return ret;
}

static inline char *get_dirname( const char *file )
{
    const char *end = strrchr( file, '/' );
    if (!end) return xstrdup( "." );
    if (end == file) end++;
    return strmake( "%.*s", (int)(end - file), file );
}

static inline char *replace_extension( const char *name, const char *old_ext, const char *new_ext )
{
    int name_len = strlen( name );

    if (strendswith( name, old_ext )) name_len -= strlen( old_ext );
    return strmake( "%.*s%s", name_len, name, new_ext );
}


static inline int make_temp_file( const char *prefix, const char *suffix, char **name )
{
    static unsigned int value;
    int fd, count;
    const char *tmpdir = NULL;

    if (!prefix) prefix = "tmp";
    if (!suffix) suffix = "";
    value += time(NULL) + getpid();

    for (count = 0; count < 0x8000; count++)
    {
        if (tmpdir)
            *name = strmake( "%s/%s-%08x%s", tmpdir, prefix, value, suffix );
        else
            *name = strmake( "%s-%08x%s", prefix, value, suffix );
        fd = open( *name, O_RDWR | O_CREAT | O_EXCL, 0600 );
        if (fd >= 0) return fd;
        value += 7777;
        if (errno == EACCES && !tmpdir && !strchr( prefix, '/' ))
        {
            if (!(tmpdir = getenv("TMPDIR"))) tmpdir = "/tmp";
        }
        free( *name );
    }
    fprintf( stderr, "failed to create temp file for %s%s\n", prefix, suffix );
    exit(1);
}


/* command-line option parsing */
/* partly based on the Glibc getopt() implementation */

struct long_option
{
    const char *name;
    int has_arg;
    int val;
};

static inline struct strarray parse_options( int argc, char **argv, const char *short_opts,
                                             const struct long_option *long_opts, int long_only,
                                             void (*callback)( int, char* ) )
{
    struct strarray ret = empty_strarray;
    const char *flag;
    char *start, *end;
    int i;

#define OPT_ERR(fmt) { callback( '?', strmake( fmt, argv[1] )); continue; }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-' || !argv[i][1])  /* not an option */
        {
            strarray_add( &ret, argv[i] );
            continue;
        }
        if (!strcmp( argv[i], "--" ))
        {
            /* add remaining args */
            while (++i < argc) strarray_add( &ret, argv[i] );
            break;
        }
        start = argv[i] + 1 + (argv[i][1] == '-');

        if (argv[i][1] == '-' || (long_only && (argv[i][2] || !strchr( short_opts, argv[i][1] ))))
        {
            /* handle long option */
            const struct long_option *opt, *found = NULL;
            int count = 0;

            if (!(end = strchr( start, '=' ))) end = start + strlen(start);
            for (opt = long_opts; opt && opt->name; opt++)
            {
                if (strncmp( opt->name, start, end - start )) continue;
                if (!opt->name[end - start])  /* exact match */
                {
                    found = opt;
                    count = 1;
                    break;
                }
                if (!found)
                {
                    found = opt;
                    count++;
                }
                else if (long_only || found->has_arg != opt->has_arg || found->val != opt->val)
                {
                    count++;
                }
            }

            if (count > 1) OPT_ERR( "option '%s' is ambiguous" );

            if (found)
            {
                if (*end)
                {
                    if (!found->has_arg) OPT_ERR( "argument not allowed in '%s'" );
                    end++;  /* skip '=' */
                }
                else if (found->has_arg == 1)
                {
                    if (i == argc - 1) OPT_ERR( "option '%s' requires an argument" );
                    end = argv[++i];
                }
                else end = NULL;

                callback( found->val, end );
                continue;
            }
            if (argv[i][1] == '-' || !long_only || !strchr( short_opts, argv[i][1] ))
                OPT_ERR( "unrecognized option '%s'" );
        }

        /* handle short option */
        for ( ; *start; start++)
        {
            if (!(flag = strchr( short_opts, *start ))) OPT_ERR( "invalid option '%s'" );
            if (flag[1] == ':')
            {
                end = start + 1;
                if (!*end) end = NULL;
                if (flag[2] != ':' && !end)
                {
                    if (i == argc - 1) OPT_ERR( "option '%s' requires an argument" );
                    end = argv[++i];
                }
                callback( *start, end );
                break;
            }
            callback( *start, NULL );
        }
    }
    return ret;
#undef OPT_ERR
}

#endif /* __WINE_TOOLS_H */
