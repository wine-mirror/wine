/*
 * Generate a C file containing a list of tests
 *
 * Copyright 2002, 2005 Alexandre Julliard
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 2005 Royce Mitchell III for the ReactOS Project
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
 *
 ****** Keep in sync with tools/winapi/msvcmaker:_generate_testlist_c *****
 */

#include "config.h"

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static const char *output_file;

static void cleanup_files(void)
{
    if (output_file) unlink( output_file );
}

static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    fprintf( stderr, "make_ctests: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}

static void fatal_perror( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    fprintf( stderr, "make_ctests: " );
    vfprintf( stderr, msg, valist );
    perror( " " );
    va_end( valist );
    exit(1);
}

static void *xmalloc( size_t size )
{
    void *res = malloc (size ? size : 1);
    if (!res) fatal_error( "virtual memory exhausted.\n" );
    return res;
}

static char* basename( const char* filename )
{
    const char *p, *p2;
    char *out;
    size_t out_len;

    p = strrchr ( filename, '/' );
    if ( !p )
        p = filename;
    else
        ++p;

    /* look for backslashes, too... */
    p2 = strrchr ( p, '\\' );
    if ( p2 ) p = p2 + 1;

    /* find extension... */
    p2 = strrchr ( p, '.' );
    if ( !p2 )
        p2 = p + strlen(p);

    /* malloc a copy */
    out_len = p2-p;
    out = xmalloc ( out_len+1 );
    memcpy ( out, p, out_len );
    out[out_len] = '\0';
    return out;
}

int main( int argc, const char** argv )
{
    int i, count = 0;
    FILE *out = stdout;
    char **tests = xmalloc( argc * sizeof(*tests) );

    for (i = 1; i < argc; i++)
    {
        if (!strcmp( argv[i], "-o" ) && i < argc-1)
        {
            output_file = argv[++i];
            continue;
        }
        tests[count++] = basename( argv[i] );
    }

    atexit( cleanup_files );
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif

    if (output_file)
    {
        if (!(out = fopen( output_file, "w" )))
            fatal_perror( "cannot create %s", output_file );
    }

    fprintf( out,
             "/* Automatically generated file; DO NOT EDIT!! */\n"
             "\n"
             "#define WIN32_LEAN_AND_MEAN\n"
             "#include <windows.h>\n\n"
             "#define STANDALONE\n"
             "#include \"wine/test.h\"\n\n" );

    for (i = 0; i < count; i++) fprintf( out, "extern void func_%s(void);\n", tests[i] );

    fprintf( out,
             "\n"
             "const struct test winetest_testlist[] =\n"
             "{\n" );

    for (i = 0; i < count; i++) fprintf( out, "    { \"%s\", func_%s },\n", tests[i], tests[i] );

    fprintf( out,
             "    { 0, 0 }\n"
             "};\n" );

    if (output_file && fclose( out ))
        fatal_perror( "error writing to %s", output_file );

    output_file = NULL;
    return 0;
}
