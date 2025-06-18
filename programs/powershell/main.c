/*
 * Copyright 2017 Jactry Zeng for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(powershell);

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int i;

    WINE_FIXME("stub.\n");
    for (i = 0; i < argc; i++)
    {
        WINE_FIXME("argv[%d] %s\n", i, wine_dbgstr_w(argv[i]));
        if (!wcsicmp(argv[i], L"-command") && i < argc - 1 && !wcscmp(argv[i + 1], L"-"))
        {
            char command[4096], *p;

            ++i;
            while (fgets(command, sizeof(command), stdin))
            {
                WINE_FIXME("command %s.\n", debugstr_a(command));
                p = command;
                while (*p && !isspace(*p)) ++p;
                *p = 0;
                if (!stricmp(command, "exit"))
                    break;
            }
        }
    }
    return 0;
}
