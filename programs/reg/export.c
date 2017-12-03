/*
 * Copyright 2017 Hugh McMaster
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
#include <stdlib.h>

#include <wine/unicode.h>
#include <wine/debug.h>

#include "reg.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static HANDLE create_file(const WCHAR *filename, DWORD action)
{
    return CreateFileW(filename, GENERIC_WRITE, 0, NULL, action, FILE_ATTRIBUTE_NORMAL, NULL);
}

static HANDLE get_file_handle(WCHAR *filename, BOOL overwrite_file)
{
    HANDLE hFile = create_file(filename, overwrite_file ? CREATE_ALWAYS : CREATE_NEW);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();

        if (error == ERROR_FILE_EXISTS)
        {
            if (!ask_confirm(STRING_OVERWRITE_FILE, filename))
            {
                output_message(STRING_CANCELLED);
                exit(0);
            }

            hFile = create_file(filename, CREATE_ALWAYS);
        }
        else
        {
            WCHAR *str;

            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, (WCHAR *)&str, 0, NULL);
            output_writeconsole(str, lstrlenW(str));
            LocalFree(str);
            exit(1);
        }
    }

    return hFile;
}

static BOOL is_overwrite_switch(const WCHAR *s)
{
    if (strlenW(s) > 2)
        return FALSE;

    if ((s[0] == '/' || s[0] == '-') && (s[1] == 'y' || s[1] == 'Y'))
        return TRUE;

    return FALSE;
}

int reg_export(int argc, WCHAR *argv[])
{
    HKEY root, hkey;
    WCHAR *path, *long_key;
    BOOL overwrite_file = FALSE;
    HANDLE hFile;

    if (argc == 3 || argc > 5)
        goto error;

    if (!parse_registry_key(argv[2], &root, &path, &long_key))
        return 1;

    if (argc == 5 && !(overwrite_file = is_overwrite_switch(argv[4])))
        goto error;

    if (RegOpenKeyExW(root, path, 0, KEY_READ, &hkey))
    {
        output_message(STRING_INVALID_KEY);
        return 1;
    }

    hFile = get_file_handle(argv[3], overwrite_file);
    FIXME(": operation not yet implemented\n");
    CloseHandle(hFile);

    RegCloseKey(hkey);

    return 1;

error:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, struprW(argv[1]));
    return 1;
}
