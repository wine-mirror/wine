/*
 * Integrity/ACL viewer/editor stub
 * Copyright (C) 2009 Andrey Turkin
 * Copyright (C) 2016 Charles Davis
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

#include <stdlib.h>
#include <stdio.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(icacls);

int __cdecl main(int argc, char** argv)
{
    int i;

    WINE_FIXME("This is dummy icacls, not performing ACL manipulations\n");
    WINE_FIXME("stub:");
    for (i = 0; i < argc; i++)
        WINE_FIXME(" %s", wine_dbgstr_a(argv[i]));
    WINE_FIXME("\n");

    return 0;
}
