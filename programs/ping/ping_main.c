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

#include <unistd.h>
#include <windows.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ping);

static void usage(void)
{
    WINE_MESSAGE( "Usage: ping [-n count] [-w timeout] target_name\n\n" );
    WINE_MESSAGE( "Options:\n" );
    WINE_MESSAGE( "    -n  Number of echo requests to send.\n" );
    WINE_MESSAGE( "    -w  Timeout in milliseconds to wait for each reply.\n" );
}

int main(int argc, char** argv)
{
    int n = 0;
    int optc;

    WINE_FIXME( "this command currently just sleeps based on -n parameter\n" );

    while ((optc = getopt( argc, argv, "n:w:tal:fi:v:r:s:j:k:" )) != -1)
    {
        switch(optc)
        {
            case 'n':
                n = atoi( optarg );
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

    Sleep(n * 1000);
    return 0;
}
