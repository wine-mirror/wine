/*
 * DxDiag Implementation
 *
 * Copyright 2009 Austin English
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

struct command_line_info
{
    WCHAR outfile[MAX_PATH];
    BOOL whql_check;
};

static void usage(void)
{
    WINE_FIXME("Usage message box is not implemented\n");
    ExitProcess(0);
}

static BOOL process_file_name(const WCHAR *cmdline, WCHAR *filename, size_t filename_len)
{
    const WCHAR *endptr;
    size_t len;

    /* Skip any intervening spaces. */
    while (*cmdline == ' ')
        cmdline++;

    /* Ignore filename quoting, if any. */
    if (*cmdline == '"' && (endptr = strrchrW(cmdline, '"')))
    {
        /* Reject a string with only one quote. */
        if (cmdline == endptr)
            return FALSE;

        cmdline++;
    }
    else
        endptr = cmdline + strlenW(cmdline);

    len = endptr - cmdline;
    if (len == 0 || len >= filename_len)
        return FALSE;

    memcpy(filename, cmdline, len * sizeof(WCHAR));
    filename[len] = '\0';

    return TRUE;
}

/*
    Process options [/WHQL:ON|OFF][/X outfile|/T outfile]
    Returns TRUE if options were present, FALSE otherwise
    Only one of /X and /T is allowed, /WHQL must come before /X and /T,
    and the rest of the command line after /X or /T is interpreted as a
    filename. If a non-option portion of the command line is encountered,
    dxdiag assumes that the string is a filename for the /T option.

    Native does not interpret quotes, but quotes are parsed here because of how
    Wine handles the command line.
*/

static BOOL process_command_line(const WCHAR *cmdline, struct command_line_info *info)
{
    static const WCHAR whql_colonW[] = {'w','h','q','l',':',0};
    static const WCHAR offW[] = {'o','f','f',0};
    static const WCHAR onW[] = {'o','n',0};

    info->whql_check = FALSE;

    while (*cmdline)
    {
        /* Skip whitespace before arg */
        while (*cmdline == ' ')
            cmdline++;

        /* If no option is specified, treat the command line as a filename. */
        if (*cmdline != '-' && *cmdline != '/')
            return process_file_name(cmdline, info->outfile, sizeof(info->outfile)/sizeof(WCHAR));

        cmdline++;

        switch (*cmdline)
        {
        case 'T':
        case 't':
            return process_file_name(cmdline + 1, info->outfile, sizeof(info->outfile)/sizeof(WCHAR));
        case 'X':
        case 'x':
            return process_file_name(cmdline + 1, info->outfile, sizeof(info->outfile)/sizeof(WCHAR));
        case 'W':
        case 'w':
            if (strncmpiW(cmdline, whql_colonW, 5))
                return FALSE;

            cmdline += 5;

            if (!strncmpiW(cmdline, offW, 3))
            {
                info->whql_check = FALSE;
                cmdline += 2;
            }
            else if (!strncmpiW(cmdline, onW, 2))
            {
                info->whql_check = TRUE;
                cmdline++;
            }
            else
                return FALSE;

            break;
        default:
            return FALSE;
        }

        cmdline++;
    }

    return TRUE;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR cmdline, int cmdshow)
{
    struct command_line_info info;

    if (!process_command_line(cmdline, &info))
        usage();

    WINE_TRACE("WHQL check: %s\n", info.whql_check ? "TRUE" : "FALSE");

    return 0;
}
