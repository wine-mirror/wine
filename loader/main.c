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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include "wine/library.h"
#include "main.h"

#ifdef __APPLE__
asm(".zerofill WINE_DOS, WINE_DOS, ___wine_dos, 0x20000000");
asm(".zerofill WINE_SHARED_HEAP, WINE_SHARED_HEAP, ___wine_shared_heap, 0x01020000");
extern char __wine_dos[0x20000000], __wine_shared_heap[0x01020000];

static const struct wine_preload_info wine_main_preload_info[] =
{
    { __wine_dos,         sizeof(__wine_dos) },          /* DOS area + PE exe */
    { __wine_shared_heap, sizeof(__wine_shared_heap) },  /* shared user data + shared heap */
    { 0, 0 }  /* end of list */
};
#else
/* the preloader will set this variable */
const struct wine_preload_info *wine_main_preload_info = NULL;
#endif

/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    char error[1024];
    int i;

    if (wine_main_preload_info)
    {
        for (i = 0; wine_main_preload_info[i].size; i++)
            wine_mmap_add_reserved_area( wine_main_preload_info[i].addr,
                                         wine_main_preload_info[i].size );
    }

    wine_pthread_set_functions( &pthread_functions, sizeof(pthread_functions) );
    wine_init( argc, argv, error, sizeof(error) );
    fprintf( stderr, "wine: failed to initialize: %s\n", error );
    exit(1);
}
