/*
 * Simplified implementation of the standard install tool
 *
 * Copyright 2025 Alexandre Julliard
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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tools.h"

/* options */
static bool backup;
static bool create_symlink;
static bool directories;
static bool preserve_timestamps;
static bool strip;
static bool verbose;
static const char *group;
static const char *mode;
static const char *strip_program;
static const char *suffix = "~";
static const char *target_dir;
static const char *user;

const char *temp_dir = NULL;
struct strarray temp_files = { 0 };

static void fatal_error( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));

/*******************************************************************
 *         fatal_error
 */
static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    fprintf( stderr, "install: error: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}


/*******************************************************************
 *         exit_on_signal
 */
static void exit_on_signal( int sig )
{
    exit( 1 );  /* this will call the atexit functions */
}


/*******************************************************************
 *         spawn_cmdline
 */
static void spawn_cmdline( struct strarray args )
{
    int status;

    if (verbose) strarray_trace( args );
    if ((status = strarray_spawn( args )))
    {
	if (status > 0) printf( "%s failed with status %u\n", args.str[0], status );
	else fatal_perror( "%s failed", args.str[0] );
    }
}


/*******************************************************************
 *         spawn_program
 */
static void spawn_program( const char *prog, const char *arg, struct strarray files )
{
    static const unsigned int max_cmdline = 30000;  /* to be on the safe side */
    struct strarray args = empty_strarray, cmd = strarray_fromstring( prog, " " );
    unsigned int len = 0;

    if (arg) strarray_add( &cmd, arg );

    strarray_addall( &args, cmd );
    STRARRAY_FOR_EACH( file, &files )
    {
        if (len > max_cmdline)
        {
            spawn_cmdline( args );
            args = empty_strarray;
            strarray_addall( &args, cmd );
            len = 0;
        }
        strarray_add( &args, file );
        len += strlen( file ) + 1;
    }
    spawn_cmdline( args );
}


/*******************************************************************
 *         set_file_permissions
 */
static void set_file_permissions( struct strarray files )
{
    if (user)
    {
        const char *chown = getenv( "CHOWNPROG" );
        spawn_program( chown ? chown : "chown", user, files );
    }
    if (group)
    {
        const char *chgrp = getenv( "CHGRPPROG" );
        spawn_program( chgrp ? chgrp : "chgrp", group, files );
    }
    if (strip)
    {
        if (!strip_program) strip_program = getenv( "STRIPPROG" );
        spawn_program( strip_program ? strip_program : "strip", NULL, files );
    }
    if (mode)
    {
        const char *chmod = getenv( "CHMODPROG" );
        spawn_program( chmod ? chmod : "chmod", mode, files );
    }
}


/*******************************************************************
 *         copy_file
 */
static void copy_file( const char *src, const char *dst )
{
    int input, output;
    char buffer[1024 * 1024];
    ssize_t size, pos, ret;

    if (verbose) printf( "%s -> %s\n", src, dst );

    if ((input = open( src, O_RDONLY | O_BINARY )) == -1)
        fatal_perror( "%s", src );
    if ((output = open( dst, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0755 )) == -1)
    {
        if (errno == EEXIST) fatal_error( "not overwriting just created %s\n", get_basename(dst) );
        fatal_perror( "%s", src );
    }

    while ((size = read( input, buffer, sizeof(buffer) )) > 0)
    {
        for (pos = 0; pos < size; pos += ret)
        {
            ret = write( output, buffer + pos, size - pos );
            if (ret <= 0) fatal_perror( "failed to write to %s", dst );
        }
    }
    if (size < 0) fatal_perror( "failed to read %s", src );
    close( input );
    close( output );
}


/*******************************************************************
 *         rename_file
 */
static void rename_file( const char *src, const char *dst )
{
    int ret;

    if (backup)
    {
        char *backup = strmake( "%s%s", dst, suffix );

        unlink( backup );
        ret = rename( dst, backup );
        if (!ret && verbose) printf( "%s -> %s\n", dst, backup );
    }

    if (verbose) printf( "%s -> %s\n", src, dst );

    unlink( dst );
    if (rename( src, dst ) == -1) fatal_perror( "failed to create %s", dst );
}


/*******************************************************************
 *         install_files
 */
static void install_files( struct strarray files, const char *dest_dir, const char *dest_file )
{
    struct strarray installed = empty_strarray;

    if (!files.count) fatal_error( "no input file specified\n" );

    mkdir_p( dest_dir );
    temp_dir = make_temp_dir( dest_dir );

    STRARRAY_FOR_EACH( src_file, &files )
    {
        const char *base = get_basename( dest_file ? dest_file : src_file );
        const char *dest = strmake( "%s/%s", temp_dir, base );

        strarray_add( &temp_files, dest );
        copy_file( src_file, dest );
        strarray_add( &installed, dest );
    }

    set_file_permissions( installed );

    STRARRAY_FOR_EACH( file, &installed )
    {
        const char *base = get_basename( file );
        const char *dest = strmake( "%s/%s", dest_dir, base );

        rename_file( file, dest );
    }
    temp_files = empty_strarray;
}


/*******************************************************************
 *         install_symlink
 */
static void install_symlink( const char *dest_dir, const char *src, const char *dst )
{
#ifndef _WIN32
    if (!dest_dir) dest_dir = get_dirname( dst );
    else dst = strmake( "%s/%s", dest_dir, get_basename(dst) );

    mkdir_p( dest_dir );
    if (verbose) printf( "symlink %s -> %s\n", src, dst );
    unlink( dst );
    if (symlink( src, dst )) fatal_perror( "cannot create symlink %s", dst );
#else
    fatal_error( "symlinks are not supported on Windows\n" );
#endif
}


/*******************************************************************
 *         command-line option handling
 */
static const char usage_str[] =
"Usage: install [OPTIONS] SRC DST\n"
"       install [OPTIONS] -t DIR [FILES]\n"
"       install [OPTIONS] -d [DIRS]\n\n";

enum long_options_values
{
    LONG_OPT_STRIP_PROGRAM = 1,
};

static const char short_options[] = "bcdg:Lm:o:psS:t:v";

static const struct long_option long_options[] =
{
    { "strip-program",       1, LONG_OPT_STRIP_PROGRAM },
    /* aliases for short options */
    { "directory",           0, 'd' },
    { "group",               1, 'g' },
    { "mode",                1, 'm' },
    { "owner",               1, 'o' },
    { "preserve-timestamps", 0, 'p' },
    { "strip",               0, 's' },
    { "suffix",              1, 'S' },
    { "target-directory",    1, 't' },
    { "verbose",             0, 'v' },
    { NULL }
};

static void usage( int exit_code )
{
    fprintf( stderr, "%s", usage_str );
    exit( exit_code );
}

static void option_callback( int optc, char *optarg )
{
    switch (optc)
    {
    case LONG_OPT_STRIP_PROGRAM:
        strip_program = xstrdup( optarg );
        break;
    case 'b':
        backup = true;
        break;
    case 'c': /* ignored */
        break;
    case 'd':
        directories = true;
        break;
    case 'g':
        group = xstrdup( optarg );
        break;
    case 'L':
        create_symlink = true;
        break;
    case 'm':
        mode = xstrdup( optarg );
        break;
    case 'o':
        user = xstrdup( optarg );
        break;
    case 'p':
        preserve_timestamps = true;
        break;
    case 's':
        strip = true;
        break;
    case 'S':
        suffix = xstrdup( optarg );
        break;
    case 't':
        target_dir = xstrdup( optarg );
        break;
    case 'v':
        verbose = true;
        break;
    case '?':
        fprintf( stderr, "install: %s\n\n", optarg );
        usage( 1 );
        break;
    }
}


/*******************************************************************
 *         main
 */
int main( int argc, char *argv[] )
{
    const char *dest = NULL;
    struct strarray files = parse_options( argc, argv, short_options, long_options, 0, option_callback );

#ifndef _WIN32
    umask( 022 );
#endif

    atexit( remove_temp_files );
    init_signals( exit_on_signal );

    if (!files.count) fatal_error( "no input file specified\n" );

    if (directories)
    {
        STRARRAY_FOR_EACH( file, &files ) mkdir_p( file );
        return 0;
    }

    if (create_symlink)
    {
        if (files.count != 2) fatal_error( "-L option requires two file arguments\n" );
        install_symlink( target_dir, files.str[0], files.str[1] );
        return 0;
    }

    if (!target_dir)  /* compute target dir from destination file */
    {
        dest = files.str[--files.count];
        if (!files.count) fatal_error( "no input file specified\n" );
        if (files.count > 1) fatal_error( "use -t option to install multiple files\n" );
        target_dir = get_dirname( dest );
    }

    install_files( files, target_dir, dest );
    return 0;
}
