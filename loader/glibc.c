/*
 * glibc threading support
 *
 * Copyright 2003 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "wine/library.h"

/* malloc wrapper */
static void *xmalloc( size_t size )
{
    void *res;

    if (!size) size = 1;
    if (!(res = malloc( size )))
    {
        fprintf( stderr, "wine: virtual memory exhausted\n" );
        exit(1);
    }
    return res;
}

/* separate thread to check for NPTL and TLS features */
static void *needs_pthread( void *arg )
{
    pid_t tid = gettid();
    /* check for NPTL */
    if (tid != -1 && tid != getpid()) return (void *)1;
    /* check for TLS glibc */
    return (void *)(wine_get_gs() != 0);
}

/* return the name of the Wine threading variant to use */
static const char *get_threading(void)
{
    pthread_t id;
    void *ret;

    pthread_create( &id, NULL, needs_pthread, NULL );
    pthread_join( id, &ret );
    return ret ? "-pthread" : "-kthread";
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    const char *loader = getenv( "WINELOADER" );
    const char *threads = get_threading();
    const char *name;
    char *new_name;

    if (!(name = loader))
    {
        /* if no WINELOADER specified, get base name of argv[0] */
        if ((name = strrchr( argv[0], '/' ))) name++;
        else name = argv[0];
    }
    new_name = xmalloc( strlen(name) + strlen(threads) + 1 );
    strcpy( new_name, name );
    strcat( new_name, threads );

    if (loader)
    {
        /* update WINELOADER with the new name */
        char *new_loader = xmalloc( sizeof("WINELOADER=") + strlen(new_name) );
        strcpy( new_loader, "WINELOADER=" );
        strcat( new_loader, new_name );
        putenv( new_loader );
        argv[0] = new_name;
        execv( argv[0], argv );
    }
    else
    {
        wine_init_argv0_path( argv[0] );
        wine_exec_wine_binary( new_name, argv, NULL );
    }
    fprintf( stderr, "wine: could not exec %s\n", argv[0] );
    exit(1);
}
