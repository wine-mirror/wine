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

extern void wine_init( int argc, char *argv[], char *error, int error_size );

/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    char error[1024];

    wine_init( argc, argv, error, sizeof(error) );
    fprintf( stderr, "wine: failed to initialize: %s\n", error );
    exit(1);
}
