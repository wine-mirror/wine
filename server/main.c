/*
 * Server main function
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "object.h"
#include "thread.h"

int main( int argc, char *argv[] )
{
    int fd;

    if (argc != 2) goto error;
    if (!isdigit( *argv[1] )) goto error;
    fd = atoi( argv[1] );
    /* make sure the fd is valid */
    if (fcntl( fd, F_GETFL, 0 ) == -1) goto error;

    debug_level = 1;

    if (debug_level) fprintf( stderr, "Server: starting (pid=%ld)\n", (long) getpid() );
    create_initial_thread( fd );
    if (debug_level) fprintf( stderr, "Server: exiting (pid=%ld)\n", (long) getpid() );

#ifdef DEBUG_OBJECTS
    dump_objects();  /* dump any remaining objects */
#endif

    exit(0);

 error:    
    fprintf( stderr, "%s: must be run from Wine.\n", argv[0] );
    exit(1);
}
