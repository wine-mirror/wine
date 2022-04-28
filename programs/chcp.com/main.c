/*
 * Copyright 2019 Erich E. Hoover
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
#include "resource.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(chcp);

static void output_writeconsole(const WCHAR *str, DWORD wlen)
{
    DWORD count;

    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, wlen, &count, NULL))
    {
        DWORD len;
        char  *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte(GetOEMCP(), 0, str, wlen, NULL, 0, NULL, NULL);
        msgA = malloc(len);
        if (!msgA) return;

        WideCharToMultiByte(GetOEMCP(), 0, str, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        free(msgA);
    }
}

static void output_formatstring(const WCHAR *fmt, va_list va_args)
{
    WCHAR *str;
    DWORD len;

    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
    if (!len && GetLastError() != ERROR_NO_WORK_DONE)
    {
        WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }
    output_writeconsole(str, len);
    LocalFree(str);
}

static void WINAPIV output_message(unsigned int id, ...)
{
    WCHAR *fmt = NULL;
    int len;
    va_list va_args;

    if (!(len = LoadStringW(GetModuleHandleW(NULL), id, (WCHAR *)&fmt, 0)))
    {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        return;
    }

    len++;
    fmt = malloc(len * sizeof(WCHAR));
    if (!fmt) return;

    LoadStringW(GetModuleHandleW(NULL), id, fmt, len);

    va_start(va_args, id);
    output_formatstring(fmt, va_args);
    va_end(va_args);

    free(fmt);
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int i;

    if (argc == 1)
    {
        output_message(STRING_ACTIVE_CODE_PAGE, GetConsoleCP());
        return 0;
    }
    else if (argc == 2)
    {
        int codepage, success;

        if (!lstrcmpW(argv[1], L"/?"))
        {
            output_message(STRING_USAGE);
            return 0;
        }

        codepage = _wtoi(argv[1]);
        success = SetConsoleCP(codepage) && SetConsoleOutputCP(codepage);

        if (success)
            output_message(STRING_ACTIVE_CODE_PAGE, codepage);
        else
            output_message(STRING_INVALID_CODE_PAGE);

        return !success;
    }

    WINE_FIXME("unexpected arguments:");
    for (i = 0; i < argc; i++)
        WINE_FIXME(" %s", wine_dbgstr_w(argv[i]));
    WINE_FIXME("\n");

    return 0;
}
