/*
 * Copyright 2016 Austin English
 * Copyright 2025 Hans Leidekker for CodeWeavers
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(subst);

int __cdecl wmain(int argc, WCHAR *argv[])
{
    const WCHAR *device = NULL, *path = NULL;
    BOOL delete = FALSE;
    int i;

    if (argc == 1)
    {
        FIXME( "Device list not supported\n" );
        return 1;
    }

    if (argc != 3)
    {
        wprintf( L"Wrong number of arguments\n" );
        return 1;
    }

    for (i = 1; i < argc; i++)
    {
        if (!wcsicmp( argv[i], L"/d" )) delete = TRUE;
        else if (!device) device = argv[i];
        else path = argv[i];
    }

    if (DefineDosDeviceW( delete ? DDD_REMOVE_DEFINITION : 0, device, path )) return 0;

    if (!delete && GetLastError() == ERROR_ALREADY_EXISTS)
        wprintf( L"Device already exists.\n" );
    else if (delete && GetLastError() == ERROR_FILE_NOT_FOUND)
        wprintf( L"No such device.\n" );
    else
        WARN( "Failed to define DOS device (%lu).\n", GetLastError() );

    return 1;
}
