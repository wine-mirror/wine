/*
 * Copyright 2007 Tim Schwartz
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

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc < 2)
    {
        printf("The syntax of this command is:\n\n");
        printf("NET [ HELP ]\n");
        return 1;
    }

    if(!strcasecmp(argv[1], "help"))
    {
        printf("The syntax of this command is:\n\n");
        printf("NET HELP command\n    -or-\nNET command /HELP\n\n");
        printf("   Commands available are:\n");
        printf("   NET HELP\n");
    }
    return ret;
}
