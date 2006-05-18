/*
 * Translate between Windows and Unix paths formats
 *
 * Copyright 2002 Mike Wetherell
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2005 Francois Gouget
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

#include "config.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/debug.h"

enum {
    SHORTFORMAT   = 1,
    LONGFORMAT    = 2,
    UNIXFORMAT    = 4,
    WINDOWSFORMAT = 8
};

static const char progname[] = "winepath";

/*
 * handle an option
 */
static int option(int shortopt, const WCHAR *longopt)
{
    static const char helpmsg[] =
    "Convert PATH(s) to Unix or Windows long or short paths.\n"
    "\n"
    "  -u, --unix    converts a Windows path to a Unix path\n"
    "  -w, --windows converts a Unix path to a long Windows path\n"
    "  -l, --long    converts a short Windows path to the long format\n"
    "  -s, --short   converts a long Windows path to the short format\n"
    "  -h, --help    output this help message and exit\n"
    "  -v, --version output version information and exit\n"
    "\n"
    "If more than one option is given then the input paths are output in\n"
    "all formats specified, in the order long, short, Unix, Windows.\n"
    "If no option is given the default is Unix format.\n";

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
        case 'w':
            return WINDOWSFORMAT;
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
    static const WCHAR windowsW[] = { 'w','i','n','d','o','w','s',0 };
    static const WCHAR helpW[] = { 'h','e','l','p',0 };
    static const WCHAR versionW[] = { 'v','e','r','s','i','o','n',0 };
    static const WCHAR nullW[] = { 0 };
    static const WCHAR *longopts[] = { longW, shortW, unixW, windowsW, helpW, versionW, nullW };
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
    LPSTR (*wine_get_unix_file_name_ptr)(LPCWSTR) = NULL;
    LPWSTR (*wine_get_dos_file_name_ptr)(LPCSTR) = NULL;
    WCHAR dos_pathW[MAX_PATH];
    char path[MAX_PATH];
    int outputformats;
    int i;

    outputformats = parse_options(argv);
    if (outputformats == 0)
        outputformats = UNIXFORMAT;

    if (outputformats & UNIXFORMAT) {
        wine_get_unix_file_name_ptr = (void*)
            GetProcAddress(GetModuleHandle("KERNEL32"),
                           "wine_get_unix_file_name");
        if (wine_get_unix_file_name_ptr == NULL) {
            fprintf(stderr, "%s: cannot get the address of "
                            "'wine_get_unix_file_name'\n", progname);
            exit(3);
        }
    }

    if (outputformats & WINDOWSFORMAT) {
        wine_get_dos_file_name_ptr = (void*)
            GetProcAddress(GetModuleHandle("KERNEL32"),
                           "wine_get_dos_file_name");
        if (wine_get_dos_file_name_ptr == NULL) {
            fprintf(stderr, "%s: cannot get the address of "
                            "'wine_get_dos_file_name'\n", progname);
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
        if (outputformats & WINDOWSFORMAT) {
            WCHAR* windows_name;
            char* unix_name;
            DWORD size;

            size=WideCharToMultiByte(CP_UNIXCP, 0, argv[i], -1, NULL, 0, NULL, NULL);
            unix_name=HeapAlloc(GetProcessHeap(), 0, size);
            WideCharToMultiByte(CP_UNIXCP, 0, argv[i], -1, unix_name, size, NULL, NULL);

            if ((windows_name = wine_get_dos_file_name_ptr(unix_name)))
            {
                WideCharToMultiByte(CP_UNIXCP, 0, windows_name, -1, path, MAX_PATH, NULL, NULL);
                printf("%s\n", path);
                HeapFree( GetProcessHeap(), 0, windows_name );
            }
            else printf( "\n" );
            HeapFree( GetProcessHeap(), 0, unix_name );
        }
    }

    exit(0);
}
