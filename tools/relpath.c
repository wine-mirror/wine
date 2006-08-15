/*
 * Compute the relative path needed to go from one Unix dir to another
 *
 * Copyright 2006 Alexandre Julliard
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* determine where the destination path is located relative to the 'from' path */
static const char *get_relative_path( const char *from, const char *dest, unsigned int *dotdots )
{
#define DIR_END(p)  (*(p) == 0 || *(p) == '/')
    const char *start;

    /* a path of "." is equivalent to an empty path */
    if (!strcmp( from, "." )) from = "";

    *dotdots = 0;
    for (;;)
    {
        while (*from == '/') from++;
        while (*dest == '/') dest++;
        start = dest;  /* save start of next path element */
        if (!*from) break;

        while (!DIR_END(from) && *from == *dest) { from++; dest++; }
        if (DIR_END(from) && DIR_END(dest)) continue;

        /* count remaining elements in 'from' */
        do
        {
            (*dotdots)++;
            while (!DIR_END(from)) from++;
            while (*from == '/') from++;
        }
        while (*from);
        break;
    }
    return start;
#undef DIR_END
}


int main( int argc, char *argv[] )
{
    const char *start;
    unsigned int dotdots = 0;

    if (argc != 3)
    {
        fprintf( stderr, "Usage: %s fromdir todir\n", argv[0] );
        exit(1);
    }
    start = get_relative_path( argv[1], argv[2], &dotdots );

    if (!start[0] && !dotdots) printf( ".\n" );
    else
    {
        while (dotdots)
        {
            printf( ".." );
            dotdots--;
            if (dotdots || start[0]) printf( "/" );
        }
        printf( "%s\n", start );
    }
    exit(0);
}
