/*
 * FreeBSD loader
 *
 * Copyright 2007 Alexandre Julliard
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#include "wine/library.h"

/* build a new full path from the specified path and name */
static const char *build_new_path( const char *path, const char *name )
{
    const char *p;
    char *ret;

    if (!(p = strrchr( path, '/' ))) return name;
    p++;
    ret = malloc( (p - path) + strlen(name) + 1 );
    memcpy( ret, path, p - path );
    strcpy( ret + (p - path), name );
    return ret;
}

/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    const char *new_argv0 = build_new_path( argv[0], "wine-pthread" );
    struct rlimit rl;
    rl.rlim_cur = 0x02000000;
    rl.rlim_max = 0x02000000;
    setrlimit( RLIMIT_DATA, &rl );
    wine_init_argv0_path( new_argv0 );
    wine_exec_wine_binary( NULL, argv, NULL );
    fprintf( stderr, "wine: could not exec %s\n", new_argv0 );
    exit(1);
}
