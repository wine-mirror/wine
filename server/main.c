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

#include "object.h"
#include "file.h"
#include "thread.h"
#include "request.h"

/* command-line options */
int debug_level = 0;
int foreground = 0;
timeout_t master_socket_timeout = 3 * -TICKS_PER_SEC;  /* master socket timeout, default is 3 seconds */
const char *server_argv0;

/* parse-line args */
/* FIXME: should probably use getopt, and add a (more complete?) help option */

static void usage(void)
{
    fprintf(stderr, "\nusage: %s [options]\n\n", server_argv0);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "   -d<n>  set debug level to <n>\n");
    fprintf(stderr, "   -f     remain in the foreground for debugging\n");
    fprintf(stderr, "   -h     display this help message\n");
    fprintf(stderr, "   -k[n]  kill the current wineserver, optionally with signal n\n");
    fprintf(stderr, "   -p[n]  make server persistent, optionally for n seconds\n");
    fprintf(stderr, "   -v     display version information and exit\n");
    fprintf(stderr, "   -w     wait until the current wineserver terminates\n");
    fprintf(stderr, "\n");
}

static void parse_args( int argc, char *argv[] )
{
    int i, ret;

    server_argv0 = argv[0];
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'd':
                if (isdigit(argv[i][2])) debug_level = atoi( argv[i] + 2 );
                else debug_level++;
                break;
            case 'f':
                foreground = 1;
                break;
            case 'h':
                usage();
                exit(0);
                break;
            case 'k':
                if (isdigit(argv[i][2])) ret = kill_lock_owner( atoi(argv[i] + 2) );
                else ret = kill_lock_owner(-1);
                exit( !ret );
            case 'p':
                if (isdigit(argv[i][2]))
                    master_socket_timeout = (timeout_t)atoi( argv[i] + 2 ) * -TICKS_PER_SEC;
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
                fprintf( stderr, "wineserver: unknown option '%s'\n", argv[i] );
                usage();
                exit(1);
            }
        }
        else
        {
            fprintf( stderr, "wineserver: unknown argument '%s'.\n", argv[i] );
            usage();
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
    setvbuf( stderr, NULL, _IOLBF, 0 );

    if (debug_level) fprintf( stderr, "wineserver: starting (pid=%ld)\n", (long) getpid() );
    init_signals();
    init_directories();
    init_registry();
    main_loop();
    return 0;
}
