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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
int master_socket_timeout = 3;  /* master socket timeout in seconds, default is 3 s */
const char *server_argv0;

/* name space for synchronization objects */
struct namespace *sync_namespace;

/* parse-line args */
/* FIXME: should probably use getopt, and add a (more complete?) help option */

static void usage(void)
{
    fprintf(stderr, "\nusage: %s [options]\n\n", server_argv0);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "   -d<n>  set debug level to <n>\n");
    fprintf(stderr, "   -p[n]  make server persistent, optionally for n seconds\n");
    fprintf(stderr, "   -w     wait until the current wineserver terminates\n");
    fprintf(stderr, "   -k[n]  kill the current wineserver, optionally with signal n\n");
    fprintf(stderr, "   -h     display this help message\n");
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
            case 'h':
                usage();
                exit(0);
                break;
            case 'p':
                if (isdigit(argv[i][2])) master_socket_timeout = atoi( argv[i] + 2 );
                else master_socket_timeout = -1;
                break;
            case 'w':
                wait_for_lock();
                exit(0);
            case 'k':
                if (isdigit(argv[i][2])) ret = kill_lock_owner( atoi(argv[i] + 2) );
                else ret = kill_lock_owner(-1);
                exit( !ret );
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

static void sigterm_handler()
{
    exit(1);  /* make sure atexit functions get called */
}

/* initialize signal handling */
static void signal_init(void)
{
    signal( SIGPIPE, SIG_IGN );
    signal( SIGHUP, sigterm_handler );
    signal( SIGINT, sigterm_handler );
    signal( SIGQUIT, sigterm_handler );
    signal( SIGTERM, sigterm_handler );
    signal( SIGABRT, sigterm_handler );
}

int main( int argc, char *argv[] )
{
    parse_args( argc, argv );
    signal_init();
    sock_init();
    open_master_socket();
    sync_namespace = create_namespace( 37, TRUE );
    setvbuf( stderr, NULL, _IOLBF, 0 );

    if (debug_level) fprintf( stderr, "wineserver: starting (pid=%ld)\n", (long) getpid() );
    init_registry();
    main_loop();

#ifdef DEBUG_OBJECTS
    dump_objects();  /* dump any remaining objects */
#endif
    return 0;
}
