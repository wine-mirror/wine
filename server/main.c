/*
 * Server main function
 *
 * Copyright (C) 1998 Alexandre Julliard
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
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "object.h"
#include "file.h"
#include "thread.h"
#include "request.h"
#include "unicode.h"

/* command-line options */
int debug_level = 0;
int foreground = 0;
timeout_t master_socket_timeout = 3 * -TICKS_PER_SEC;  /* master socket timeout, default is 3 seconds */
const char *server_argv0;

/* parse-line args */

static void usage( FILE *fh )
{
    fprintf(fh, "Usage: %s [options]\n\n", server_argv0);
    fprintf(fh, "Options:\n");
    fprintf(fh, "   -d[n], --debug[=n]       set debug level to n or +1 if n not specified\n");
    fprintf(fh, "   -f,    --foreground      remain in the foreground for debugging\n");
    fprintf(fh, "   -h,    --help            display this help message\n");
    fprintf(fh, "   -k[n], --kill[=n]        kill the current wineserver, optionally with signal n\n");
    fprintf(fh, "   -p[n], --persistent[=n]  make server persistent, optionally for n seconds\n");
    fprintf(fh, "   -v,    --version         display version information and exit\n");
    fprintf(fh, "   -w,    --wait            wait until the current wineserver terminates\n");
    fprintf(fh, "\n");
}

static void parse_args( int argc, char *argv[] )
{
    int ret, optc;

    static struct option long_options[] =
    {
        {"debug",       2, NULL, 'd'},
        {"foreground",  0, NULL, 'f'},
        {"help",        0, NULL, 'h'},
        {"kill",        2, NULL, 'k'},
        {"persistent",  2, NULL, 'p'},
        {"version",     0, NULL, 'v'},
        {"wait",        0, NULL, 'w'},
        { NULL,         0, NULL, 0}
    };

    server_argv0 = argv[0];

    while ((optc = getopt_long( argc, argv, "d::fhk::p::vw", long_options, NULL )) != -1)
    {
        switch(optc)
        {
            case 'd':
                if (optarg && isdigit(*optarg))
                    debug_level = atoi( optarg );
                else
                    debug_level++;
                break;
            case 'f':
                foreground = 1;
                break;
            case 'h':
                usage(stdout);
                exit(0);
                break;
            case 'k':
                if (optarg && isdigit(*optarg))
                    ret = kill_lock_owner( atoi( optarg ) );
                else
                    ret = kill_lock_owner(-1);
                exit( !ret );
            case 'p':
                if (optarg && isdigit(*optarg))
                    master_socket_timeout = (timeout_t)atoi( optarg ) * -TICKS_PER_SEC;
                else
                    master_socket_timeout = TIMEOUT_INFINITE;
                break;
            case 'v':
                fprintf( stderr, "%s\n", PACKAGE_STRING );
                exit(0);
            case 'w':
                wait_for_lock();
                exit(0);
            default:
                usage(stderr);
                exit(1);
        }
    }
}

static void sigterm_handler( int signum )
{
    exit(1);  /* make sure atexit functions get called */
}

int main( int argc, char *argv[] )
{
    setvbuf( stderr, NULL, _IOLBF, 0 );
    parse_args( argc, argv );

    /* setup temporary handlers before the real signal initialization is done */
    signal( SIGPIPE, SIG_IGN );
    signal( SIGHUP, sigterm_handler );
    signal( SIGINT, sigterm_handler );
    signal( SIGQUIT, sigterm_handler );
    signal( SIGTERM, sigterm_handler );
    signal( SIGABRT, sigterm_handler );

    sock_init();
    open_master_socket();

    if (debug_level) fprintf( stderr, "wineserver: starting (pid=%ld)\n", (long) getpid() );
    set_current_time();
    init_signals();
    init_directories( load_intl_file() );
    init_registry();
    main_loop();
    return 0;
}
