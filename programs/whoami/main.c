/*
 * Copyright 2020 Brendan Shanks for CodeWeavers
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

#define SECURITY_WIN32
#include <windows.h>
#include <security.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(whoami);

static int output_write(const WCHAR* str, int len)
{
    DWORD count;
    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, len, &count, NULL))
    {
        DWORD lenA;
        char* strA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile() with OEM code page.
         */
        lenA = WideCharToMultiByte(GetOEMCP(), 0, str, len,
                                   NULL, 0, NULL, NULL);
        strA = HeapAlloc(GetProcessHeap(), 0, lenA);
        if (!strA)
            return 0;

        WideCharToMultiByte(GetOEMCP(), 0, str, len, strA, lenA,
                            NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), strA, lenA, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, strA);
    }
    return count;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WCHAR *buf = NULL;
    ULONG size = 0;
    BOOL result;

    if (argc > 1)
    {
        int i;

        WINE_FIXME("unsupported arguments:");
        for (i = 0; i < argc; i++)
            WINE_FIXME(" %s", wine_dbgstr_w(argv[i]));
        WINE_FIXME("\n");
    }

    result = GetUserNameExW(NameSamCompatible, NULL, &size);
    if (result || GetLastError() != ERROR_MORE_DATA)
    {
        WINE_ERR("GetUserNameExW failed, result %d, error %ld\n", result, GetLastError());
        return 1;
    }

    buf = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!buf)
    {
        WINE_ERR("Memory allocation failed\n");
        return 1;
    }

    result = GetUserNameExW(NameSamCompatible, buf, &size);
    if (result)
    {
        output_write(buf, size);
        output_write(L"\r\n", 2);
    }
    else
        WINE_ERR("GetUserNameExW failed, error %ld\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, buf);
    return 0;
}
