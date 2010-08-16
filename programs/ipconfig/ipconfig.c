/*
 * IP configuration utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010 Andrew Nguyen
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
#include <wine/debug.h>
#include <wine/unicode.h>

#include "ipconfig.h"

WINE_DEFAULT_DEBUG_CHANNEL(ipconfig);

static int ipconfig_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    va_start(va_args, msg);
    wlen = vsprintfW(msg_buffer, msg, va_args);
    va_end(va_args);

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char *msgA;

        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen,
            NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len);
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen, msgA, len,
            NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }

    return count;
}

static int ipconfig_message(int msg)
{
    static const WCHAR formatW[] = {'%','s',0};
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));
    return ipconfig_printfW(formatW, msg_buffer);
}

int wmain(int argc, WCHAR *argv[])
{
    static const WCHAR slashHelp[] = {'/','?',0};
    static const WCHAR slashAll[] = {'/','a','l','l',0};

    if (argc > 1)
    {
        if (!strcmpW(slashHelp, argv[1]))
        {
            ipconfig_message(STRING_USAGE);
            return 1;
        }
        else if (!strcmpiW(slashAll, argv[1]))
        {
            if (argv[2])
            {
                ipconfig_message(STRING_INVALID_CMDLINE);
                ipconfig_message(STRING_USAGE);
                return 1;
            }

            WINE_FIXME("/all option is not currently handled\n");
        }
        else
        {
            ipconfig_message(STRING_INVALID_CMDLINE);
            ipconfig_message(STRING_USAGE);
            return 1;
        }
    }

    WINE_FIXME("Network interface output is not currently implemented\n");
    return 0;
}
