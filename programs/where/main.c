/*
 * Copyright 2020 Louis Lenders
 * Copyright 2024 Alex Henrie
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

#include <windows.h>
#include <fileapi.h>
#include <shlwapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(where);

static BOOL found;

static void search(const WCHAR *search_path, const WCHAR *pattern)
{
    static const WCHAR *extensions[] = {L"", L".bat", L".cmd", L".com", L".exe"};
    WCHAR glob[MAX_PATH];
    WCHAR match_path[MAX_PATH];
    WIN32_FIND_DATAW match;
    HANDLE handle;
    int i;

    for (i = 0; i < ARRAY_SIZE(extensions); i++)
    {
        if (wcslen(search_path) + 1 + wcslen(pattern) + wcslen(extensions[i]) + 1 > ARRAY_SIZE(glob))
        {
            ERR("Path too long\n");
            return;
        }
        /* Treat the extension as part of the pattern */
        wcscpy(glob, search_path);
        wcscat(glob, L"\\");
        wcscat(glob, pattern);
        wcscat(glob, extensions[i]);

        handle = FindFirstFileExW(glob, FindExInfoBasic, &match, 0, NULL, 0);
        if (handle != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (PathCombineW(match_path, search_path, match.cFileName))
                {
                    printf("%ls\n", match_path);
                    found = TRUE;
                }
            }
            while (FindNextFileW(handle, &match));
            FindClose(handle);
        }
    }
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WCHAR *pattern, *colon, *search_paths, *search_path, *next_search_path;
    int i;

    for (i = 0; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            FIXME("Unsupported option %ls\n", argv[i]);
            return 1;
        }
    }

    for (i = 1; i < argc; i++)
    {
        pattern = argv[i];
        colon = wcsrchr(pattern, ':');

        /* Check for a set of search paths prepended to the pattern */
        if (colon)
        {
            *colon = 0;
            search_paths = pattern;
            pattern = colon + 1;
        }
        else
        {
            search_paths = _wgetenv(L"PATH");
        }

        if (wcspbrk(pattern, L"\\/\r\n")) continue; /* Silently ignore invalid patterns */

        if (!colon)
        {
            /* If the search paths were not explicitly specified, search the current directory first */
            WCHAR current_dir[MAX_PATH];
            if (GetCurrentDirectoryW(ARRAY_SIZE(current_dir), current_dir))
                search(current_dir, pattern);
        }

        next_search_path = wcsdup(search_paths);
        while ((search_path = wcstok(NULL, L";", &next_search_path)))
            search(search_path, pattern);
    }

    if (!found)
    {
        fputs("File not found\n", stderr);
        return 1;
    }

    return 0;
}
