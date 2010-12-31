/*
 * ping stub
 * Copyright (C) 2010 Trey Hunner
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <windows.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ping);

static void usage(void)
{
    printf("Usage: ping [-n count] [-w timeout] target_name\n\n"
           "Options:\n"
           "    -n  Number of echo requests to send.\n"
           "    -w  Timeout in milliseconds to wait for each reply.\n");
}

int main(int argc, char** argv)
{
    unsigned int n = 0;
    int optc;

    WINE_FIXME( "this command currently just sleeps based on -n parameter\n" );

    while ((optc = getopt( argc, argv, "n:w:tal:fi:v:r:s:j:k:" )) != -1)
    {
        switch(optc)
        {
            case 'n':
                n = atoi(optarg);
                if (n == 0)
                {
                  printf("Bad value for option -n, valid range is from 1 to 4294967295.\n");
                  exit(1);
                }
                break;
            case '?':
                usage();
                exit(1);
            default:
                usage();
                WINE_FIXME( "this command currently only supports the -n parameter\n" );
                exit(1);
        }
    }

    if (n != 0)
      Sleep((n - 1) * 1000);

    return 0;
}
