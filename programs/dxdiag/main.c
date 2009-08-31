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

/*
    Process options [/WHQL:ON|OFF][/X outfile][/T outfile]
    Returns TRUE if options were present, FALSE otherwise
    FIXME: Native behavior seems to be:
    Only one of /X and /T is allowed, /WHQL must come before /X and /T,
    quotes are optional around the filename, even if it contains spaces.
*/

static BOOL ProcessCommandLine(const WCHAR *s)
{
    WCHAR outfile[MAX_PATH+1];
    int opt_t = FALSE;
    int opt_x = FALSE;
    int opt_help = FALSE;
    int opt_given = FALSE;
    int want_arg = FALSE;

    outfile[0] = 0;
    while (*s) {
        /* Skip whitespace before arg */
        while (*s == ' ')
            s++;
        /* Check for option */
        if (*s != '-' && *s != '/')
            return FALSE;
        s++;
        switch (*s++) {
        case 'T':
        case 't': opt_t = TRUE; want_arg = TRUE; opt_given = TRUE; break;
        case 'X':
        case 'x': opt_x = TRUE; want_arg = TRUE; opt_given = TRUE; break;
        case 'W':
        case 'w':
            opt_given = TRUE;
            while (isalphaW(*s) || *s == ':')
                s++;
            break;
        default: opt_help = TRUE; opt_given = TRUE; break;
        }
        /* Skip any spaces before next option or filename */
        while (*s == ' ')
            s++;
        if (want_arg) {
            int i;
            if (*s == '"')
                s++;
            for (i=0; i < MAX_PATH && *s && *s != '"'; i++, s++)
                outfile[i] = *s;
            outfile[i] = 0;
            break;
        }
    }
    if (opt_help)
        WINE_FIXME("help unimplemented\n");
    if (opt_t)
        WINE_FIXME("/t unimplemented\n");
    if (opt_x)
        WINE_FIXME("/x unimplemented\n");
    return opt_given;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR cmdline, int cmdshow)
{
    if (ProcessCommandLine(cmdline))
        return 0;

    return 0;
}
