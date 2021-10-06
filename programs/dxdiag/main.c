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
#include <dxdiag.h>
#include <commctrl.h>

#include "wine/debug.h"
#include "dxdiag_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

HINSTANCE hInstance;

struct command_line_info
{
    WCHAR outfile[MAX_PATH];
    enum output_type output_type;
    BOOL whql_check;
};

static void usage(void)
{
    WCHAR title[MAX_STRING_LEN];
    WCHAR usage[MAX_STRING_LEN];

    LoadStringW(hInstance, STRING_DXDIAG_TOOL, title, ARRAY_SIZE(title));
    LoadStringW(hInstance, STRING_USAGE, usage, ARRAY_SIZE(usage));

    MessageBoxW(NULL, usage, title, MB_OK | MB_ICONWARNING);

    ExitProcess(0);
}

static BOOL process_file_name(const WCHAR *cmdline, enum output_type output_type, WCHAR *filename, size_t filename_len)
{
    const WCHAR *endptr;
    size_t len;

    /* Skip any intervening spaces. */
    while (*cmdline == ' ')
        cmdline++;

    /* Ignore filename quoting, if any. */
    if (*cmdline == '"' && (endptr = wcsrchr(cmdline, '"')))
    {
        /* Reject a string with only one quote. */
        if (cmdline == endptr)
            return FALSE;

        cmdline++;
    }
    else
        endptr = cmdline + lstrlenW(cmdline);

    len = endptr - cmdline;
    if (len == 0 || len >= filename_len)
        return FALSE;

    memcpy(filename, cmdline, len * sizeof(WCHAR));
    filename[len] = '\0';

    /* Append an extension appropriate for the output type if the filename does not have one. */
    if (!wcsrchr(filename, '.'))
    {
        const WCHAR *filename_ext = get_output_extension(output_type);

        if (len + lstrlenW(filename_ext) >= filename_len)
            return FALSE;

        lstrcatW(filename, filename_ext);
    }

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
    info->whql_check = FALSE;
    info->output_type = OUTPUT_NONE;

    while (*cmdline)
    {
        /* Skip whitespace before arg */
        while (*cmdline == ' ')
            cmdline++;

        /* If no option is specified, treat the command line as a filename. */
        if (*cmdline != '-' && *cmdline != '/')
        {
            info->output_type = OUTPUT_TEXT;
            return process_file_name(cmdline, OUTPUT_TEXT, info->outfile,
                                     ARRAY_SIZE(info->outfile));
        }

        cmdline++;

        switch (*cmdline)
        {
        case 'T':
        case 't':
            info->output_type = OUTPUT_TEXT;
            return process_file_name(cmdline + 1, OUTPUT_TEXT, info->outfile,
                                     ARRAY_SIZE(info->outfile));
        case 'X':
        case 'x':
            info->output_type = OUTPUT_XML;
            return process_file_name(cmdline + 1, OUTPUT_XML, info->outfile,
                                     ARRAY_SIZE(info->outfile));
        case 'W':
        case 'w':
            if (wcsnicmp(cmdline, L"whql:", 5))
                return FALSE;

            cmdline += 5;

            if (!wcsnicmp(cmdline, L"off", 3))
            {
                info->whql_check = FALSE;
                cmdline += 2;
            }
            else if (!wcsnicmp(cmdline, L"on", 2))
            {
                info->whql_check = TRUE;
                cmdline++;
            }
            else
                return FALSE;

            break;

        case '6':
            if (wcsnicmp(cmdline, L"64bit", 5))
                return FALSE;
            cmdline += 5;
            break;

        case 'd':
        case 'D':
            if (wcsnicmp(cmdline, L"dontskip", 8))
                return FALSE;
            cmdline += 8;
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
    struct dxdiag_information *dxdiag_info;

    InitCommonControls();

    hInstance = hInst;

    if (!process_command_line(cmdline, &info))
        usage();

    WINE_TRACE("WHQL check: %s\n", info.whql_check ? "TRUE" : "FALSE");
    WINE_TRACE("Output type: %d\n", info.output_type);
    if (info.output_type != OUTPUT_NONE)
        WINE_TRACE("Output filename: %s\n", debugstr_output_type(info.output_type));

    CoInitialize(NULL);

    dxdiag_info = collect_dxdiag_information(info.whql_check);
    if (!dxdiag_info)
    {
        WINE_ERR("DxDiag information collection failed\n");
        CoUninitialize();
        return 1;
    }

    if (info.output_type != OUTPUT_NONE)
        output_dxdiag_information(dxdiag_info, info.outfile, info.output_type);
    else
        WINE_FIXME("Information dialog is not implemented\n");

    free_dxdiag_information(dxdiag_info);

    CoUninitialize();
    return 0;
}
