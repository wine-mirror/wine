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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

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
    return ret ? "wine-pthread" : "wine-kthread";
}

/* build a new full path from the specified path and name */
static const char *build_new_path( const char *path, const char *name )
{
    const char *p;
    char *ret;

    if (!(p = strrchr( path, '/' ))) return name;
    p++;
    ret = xmalloc( (p - path) + strlen(name) + 1 );
    memcpy( ret, path, p - path );
    strcpy( ret + (p - path), name );
    return ret;
}

static void check_vmsplit( void *stack )
{
    if (stack < (void *)0x80000000)
    {
        /* if the stack is below 0x80000000, assume we can safely try a munmap there */
        if (munmap( (void *)0x80000000, 1 ) == -1 && errno == EINVAL)
            fprintf( stderr,
                     "Warning: memory above 0x80000000 doesn't seem to be accessible.\n"
                     "Wine requires a 3G/1G user/kernel memory split to work properly.\n" );
    }
}

static void set_max_limit( int limit )
{
    struct rlimit rlimit;

    if (!getrlimit( limit, &rlimit ))
    {
        rlimit.rlim_cur = rlimit.rlim_max;
        setrlimit( limit, &rlimit );
    }
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    const char *loader = getenv( "WINELOADER" );
    const char *threads = get_threading();
    const char *new_argv0 = build_new_path( argv[0], threads );

    wine_init_argv0_path( new_argv0 );

    /* set the address space limit before starting the preloader */
    set_max_limit( RLIMIT_AS );

    if (loader)
    {
        /* update WINELOADER with the new name */
        const char *new_name = build_new_path( loader, threads );
        char *new_loader = xmalloc( sizeof("WINELOADER=") + strlen(new_name) );
        strcpy( new_loader, "WINELOADER=" );
        strcat( new_loader, new_name );
        putenv( new_loader );
        loader = new_name;
    }

    check_vmsplit( &argc );
    wine_exec_wine_binary( NULL, argv, loader );
    fprintf( stderr, "wine: could not exec %s\n", threads );
    exit(1);
}
