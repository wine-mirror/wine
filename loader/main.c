/*
 * Emulator initialisation code
 *
 * Copyright 2000 Alexandre Julliard
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
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "wine/library.h"
#include "main.h"

#ifdef __APPLE__

asm(".zerofill WINE_DOS, WINE_DOS, ___wine_dos, 0x60000000");
asm(".zerofill WINE_SHARED_HEAP, WINE_SHARED_HEAP, ___wine_shared_heap, 0x02000000");
extern char __wine_dos[0x60000000], __wine_shared_heap[0x02000000];

static const struct wine_preload_info wine_main_preload_info[] =
{
    { __wine_dos,         sizeof(__wine_dos) },          /* DOS area + PE exe */
    { __wine_shared_heap, sizeof(__wine_shared_heap) },  /* shared user data + shared heap */
    { 0, 0 }  /* end of list */
};

static inline void reserve_area( void *addr, size_t size )
{
    wine_anon_mmap( addr, size, PROT_NONE, MAP_FIXED | MAP_NORESERVE );
    wine_mmap_add_reserved_area( addr, size );
}

#else  /* __APPLE__ */

/* the preloader will set this variable */
const struct wine_preload_info *wine_main_preload_info = NULL;

static inline void reserve_area( void *addr, size_t size )
{
    wine_mmap_add_reserved_area( addr, size );
}

#endif  /* __APPLE__ */

/***********************************************************************
 *           check_command_line
 *
 * Check if command line is one that needs to be handled specially.
 */
static void check_command_line( int argc, char *argv[] )
{
    static const char usage[] =
        "Usage: wine PROGRAM [ARGUMENTS...]   Run the specified program\n"
        "       wine --help                   Display this help and exit\n"
        "       wine --version                Output version information and exit";

    if (argc <= 1)
    {
        fprintf( stderr, "%s\n", usage );
        exit(1);
    }
    if (!strcmp( argv[1], "--help" ))
    {
        printf( "%s\n", usage );
        exit(0);
    }
    if (!strcmp( argv[1], "--version" ))
    {
        printf( "%s\n", wine_get_build_id() );
        exit(0);
    }
}


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    char error[1024];
    int i;

    check_command_line( argc, argv );
    if (wine_main_preload_info)
    {
        for (i = 0; wine_main_preload_info[i].size; i++)
            reserve_area( wine_main_preload_info[i].addr, wine_main_preload_info[i].size );
    }

    wine_pthread_set_functions( &pthread_functions, sizeof(pthread_functions) );
    wine_init( argc, argv, error, sizeof(error) );
    fprintf( stderr, "wine: failed to initialize: %s\n", error );
    exit(1);
}
