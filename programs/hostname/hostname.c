/*
 * Hostname display utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010-2011 Andrew Nguyen
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#if defined(__MINGW32__) || defined (_MSC_VER)
#include <winsock2.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winnls.h>
#include <winuser.h>

#include <wine/unicode.h>

#include "hostname.h"

static int hostname_vprintfW(const WCHAR *msg, va_list va_args)
{
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    wlen = vsprintfW(msg_buffer, msg, va_args);

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
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

static int hostname_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int len;

    va_start(va_args, msg);
    len = hostname_vprintfW(msg, va_args);
    va_end(va_args);

    return len;
}

static int hostname_message_printfW(int msg, ...)
{
    va_list va_args;
    WCHAR msg_buffer[8192];
    int len;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));

    va_start(va_args, msg);
    len = hostname_vprintfW(msg_buffer, va_args);
    va_end(va_args);

    return len;
}

static int hostname_message(int msg)
{
    static const WCHAR formatW[] = {'%','s',0};
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));

    return hostname_printfW(formatW, msg_buffer);
}

static void display_computer_name(void)
{
    static const WCHAR fmtW[] = {'%','s','\r','\n',0};

    char nameA[256];
    WCHAR nameW[256];

    gethostname(nameA, sizeof(nameA));
    MultiByteToWideChar(CP_UNIXCP, 0, nameA, sizeof(nameA), nameW, sizeof(nameW)/sizeof(WCHAR));

    hostname_printfW(fmtW, nameW);
}

int wmain(int argc, WCHAR *argv[])
{
    if (argc > 1)
    {
        static const WCHAR slashHelpW[] = {'/','?',0};

        unsigned int i;

        if (!strncmpW(argv[1], slashHelpW, sizeof(slashHelpW)/sizeof(WCHAR) - 1))
        {
            hostname_message(STRING_USAGE);
            return 1;
        }

        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                switch (argv[i][1])
                {
                    case 's':
                        /* Ignore the option and continue processing. */
                        break;
                    case '?':
                        hostname_message(STRING_USAGE);
                        return 1;
                    default:
                        hostname_message_printfW(STRING_INVALID_OPTION, argv[i][1]);
                        hostname_message(STRING_USAGE);
                        return 1;
                }
            }
            else
            {
                hostname_message(STRING_CANNOT_SET_HOSTNAME);
                hostname_message(STRING_USAGE);
                return 1;
            }
        }
    }

    display_computer_name();

    return 0;
}
