/*
 * Translate between Wine and Unix paths
 *
 * Copyright 2002 Mike Wetherell
 * Copyright 2005 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/debug.h"

enum {
    SHORTFORMAT = 1,
    LONGFORMAT  = 2,
    UNIXFORMAT  = 4
};

static const char progname[] = "winepath";

/* Wine specific functions */
typedef LPSTR (*wine_get_unix_file_name_t) ( LPCWSTR dos );
/*
 * handle an option
 */
static int option(int shortopt, const WCHAR *longopt)
{
    static const char helpmsg[] =
    "Convert PATH(s) to Unix or Windows long or short paths.\n"
    "\n"
    "  -u, --unix    output Unix format\n"
    "  -l, --long    output Windows long format\n"
    "  -s, --short   output Windows short format \n"
    "  -h, --help    output this help message and exit\n"
    "  -v, --version output version information and exit\n"
    "\n"
    "The input paths can be in any format. If more than one option is given\n"
    "then the input paths are output in all formats specified, in the order\n"
    "Unix, long, short. If no option is given the default is Unix format.\n";

    switch (shortopt) {
        case 'h':
            printf("Usage: %s [OPTION] [PATH]...\n", progname);
            printf(helpmsg);
            exit(0);
        case 'v':
            printf("%s version " PACKAGE_VERSION "\n", progname);
            exit(0);
        case 'l':
            return LONGFORMAT;
        case 's':
            return SHORTFORMAT;
        case 'u':
            return UNIXFORMAT;
    }

    fprintf(stderr, "%s: invalid option ", progname);
    if (longopt)
        fprintf(stderr, "%s\n", wine_dbgstr_w(longopt));
    else
        fprintf(stderr, "'-%c'\n", shortopt);
    fprintf(stderr, "Try '%s --help' for help\n", progname);
    exit(2);
}

/*
 * Parse command line options
 */
static int parse_options(const WCHAR *argv[])
{
    static const WCHAR longW[] = { 'l','o','n','g',0 };
    static const WCHAR shortW[] = { 's','h','o','r','t',0 };
    static const WCHAR unixW[] = { 'u','n','i','x',0 };
    static const WCHAR helpW[] = { 'h','e','l','p',0 };
    static const WCHAR versionW[] = { 'v','e','r','s','i','o','n',0 };
    static const WCHAR nullW[] = { 0 };
    static const WCHAR *longopts[] = { longW, shortW, unixW, helpW, versionW, nullW };
    int outputformats = 0;
    int done = 0;
    int i, j;

    for (i = 1; argv[i] && !done; )
    {
        if (argv[i][0] != '-') {
            /* not an option */
            i++;
            continue;
        }

        if (argv[i][1] == '-') {
            if (argv[i][2] == 0) {
                /* '--' end of options */
                done = 1;
            } else {
                /* long option */
                for (j = 0; longopts[j][0]; j++)
                    if (!lstrcmpiW(argv[i]+2, longopts[j]))
                        break;
                outputformats |= option(longopts[j][0], argv[i]);
            }
        } else {
            /* short options */
            for (j = 1; argv[i][j]; j++)
                outputformats |= option(argv[i][j], NULL);
        }

        /* remove option */
        for (j = i + 1; argv[j - 1]; j++)
            argv[j - 1] = argv[j];
    }

    return outputformats;
}

/*
 * Main function
 */
int wmain(int argc, const WCHAR *argv[])
{
    wine_get_unix_file_name_t wine_get_unix_file_name_ptr = NULL;
    WCHAR dos_pathW[MAX_PATH];
    char path[MAX_PATH];
    int outputformats;
    int i;

    outputformats = parse_options(argv);
    if (outputformats == 0)
        outputformats = UNIXFORMAT;

    if (outputformats & UNIXFORMAT) {
        wine_get_unix_file_name_ptr = (wine_get_unix_file_name_t)
            GetProcAddress(GetModuleHandle("KERNEL32"),
                           "wine_get_unix_file_name");
        if (wine_get_unix_file_name_ptr == NULL) {
            fprintf(stderr, "%s: cannot get the address of "
                            "'wine_get_unix_file_name'\n", progname);
            exit(3);
        }
    }

    for (i = 1; argv[i]; i++)
    {
        *path='\0';
        if (outputformats & LONGFORMAT) {
            if (GetFullPathNameW(argv[i], MAX_PATH, dos_pathW, NULL))
                WideCharToMultiByte(CP_UNIXCP, 0, dos_pathW, -1, path, MAX_PATH, NULL, NULL);
            printf("%s\n", path);
        }
        if (outputformats & SHORTFORMAT) {
            if (GetShortPathNameW(argv[i], dos_pathW, MAX_PATH))
                WideCharToMultiByte(CP_UNIXCP, 0, dos_pathW, -1, path, MAX_PATH, NULL, NULL);
            printf("%s\n", path);
        }
        if (outputformats & UNIXFORMAT) {
            char *unix_name;

            if ((unix_name = wine_get_unix_file_name_ptr(argv[i])))
            {
                printf("%s\n", unix_name);
                HeapFree( GetProcessHeap(), 0, unix_name );
            }
            else printf( "\n" );
        }
    }

    exit(0);
}
