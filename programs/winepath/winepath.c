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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>

#include "wine/debug.h"

enum {
    SHORTFORMAT   = 1,
    LONGFORMAT    = 2,
    UNIXFORMAT    = 4,
    WINDOWSFORMAT = 8,
    PRINT0        = 16,
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
    "  -l, --long    converts the short Windows path of an existing file or\n"
    "                directory to the long format\n"
    "  -s, --short   converts the long Windows path of an existing file or\n"
    "                directory to the short format\n"
    "  -0            separate output with \\0 character, instead of a newline\n"
    "  -h, --help    output this help message and exit\n"
    "\n"
    "If more than one option is given then the input paths are output in\n"
    "all formats specified, in the order long, short, Unix, Windows.\n"
    "If no option is given the default is Unix format.\n";

    switch (shortopt) {
        case 'h':
            printf("Usage: %s [OPTION] [PATH]...\n", progname);
            printf(helpmsg);
            exit(0);
        case 'l':
            return LONGFORMAT;
        case 's':
            return SHORTFORMAT;
        case 'u':
            return UNIXFORMAT;
        case 'w':
            return WINDOWSFORMAT;
        case '0':
            return PRINT0;
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
static int parse_options(WCHAR *argv[])
{
    static const WCHAR *longopts[] = { L"long", L"short", L"unix", L"windows", L"help", NULL };
    int outputformats = 0;
    BOOL done = FALSE;
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
                done = TRUE;
            } else {
                /* long option */
                for (j = 0; longopts[j]; j++)
                    if (!lstrcmpiW(argv[i]+2, longopts[j]))
                        break;
                if (longopts[j]) outputformats |= option(longopts[j][0], argv[i]);
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
int __cdecl wmain(int argc, WCHAR *argv[])
{
    LPSTR (*CDECL wine_get_unix_file_name_ptr)(LPCWSTR) = NULL;
    LPWSTR (*CDECL wine_get_dos_file_name_ptr)(LPCSTR) = NULL;
    WCHAR dos_pathW[MAX_PATH];
    char path[MAX_PATH];
    int outputformats;
    int i;
    int separator;

    setmode( fileno(stdout), O_BINARY );  /* avoid crlf */

    outputformats = parse_options(argv);

    if (outputformats & PRINT0)
    {
        separator = '\0';
        outputformats ^= PRINT0;
    }
    else
        separator = '\n';

    if (outputformats == 0)
        outputformats = UNIXFORMAT;

    if (outputformats & UNIXFORMAT) {
        wine_get_unix_file_name_ptr = (void*)
            GetProcAddress(GetModuleHandleA("KERNEL32"),
                           "wine_get_unix_file_name");
        if (wine_get_unix_file_name_ptr == NULL) {
            fprintf(stderr, "%s: cannot get the address of "
                            "'wine_get_unix_file_name'\n", progname);
            exit(3);
        }
    }

    if (outputformats & WINDOWSFORMAT) {
        wine_get_dos_file_name_ptr = (void*)
            GetProcAddress(GetModuleHandleA("KERNEL32"),
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
            if (GetLongPathNameW(argv[i], dos_pathW, MAX_PATH))
                WideCharToMultiByte(CP_UNIXCP, 0, dos_pathW, -1, path, MAX_PATH, NULL, NULL);
            printf("%s%c", path, separator);
        }
        if (outputformats & SHORTFORMAT) {
            if (GetShortPathNameW(argv[i], dos_pathW, MAX_PATH))
                WideCharToMultiByte(CP_UNIXCP, 0, dos_pathW, -1, path, MAX_PATH, NULL, NULL);
            printf("%s%c", path, separator);
        }
        if (outputformats & UNIXFORMAT) {
            WCHAR *ntpath, *tail;
            int ntpathlen=lstrlenW(argv[i]);
            ntpath = malloc(sizeof(*ntpath)*(ntpathlen+1));
            lstrcpyW(ntpath, argv[i]);
            tail=NULL;
            while (1)
            {
                char *unix_name;
                WCHAR *slash, *c;

                unix_name = wine_get_unix_file_name_ptr(ntpath);
                if (unix_name)
                {
                    if (tail)
                    {
                        WideCharToMultiByte(CP_UNIXCP, 0, tail+1, -1, path, MAX_PATH, NULL, NULL);
                        printf("%s/%s%c", unix_name, path, separator);
                    }
                    else
                    {
                        printf("%s%c", unix_name, separator);
                    }
                    HeapFree( GetProcessHeap(), 0, unix_name );
                    break;
                }

                slash=(tail ? tail : ntpath+ntpathlen);
                while (slash != ntpath && *slash != '/' && *slash != '\\')
                    slash--;
                if (slash == ntpath)
                {
                    /* This is a complete path conversion failure.
                     * It would typically happen if ntpath == "".
                     */
                    printf("%c", separator);
                    break;
                }
                c=slash+1;
                while (*c != '\0' && *c != '*' && *c != '?' &&
                       *c != '<' && *c != '>' && *c != '|' && *c != '"')
                    c++;
                if (*c != '\0')
                {
                    /* If this is not a valid NT path to start with,
                     * then obviously we cannot convert it.
                     */
                    printf("%c", separator);
                    break;
                }
                if (tail)
                    *tail='/';
                tail=slash;
                *tail='\0';
            }
            free(ntpath);
        }
        if (outputformats & WINDOWSFORMAT) {
            WCHAR* windows_name;
            char* unix_name;
            DWORD size;

            size=WideCharToMultiByte(CP_UNIXCP, 0, argv[i], -1, NULL, 0, NULL, NULL);
            unix_name = malloc(size);
            WideCharToMultiByte(CP_UNIXCP, 0, argv[i], -1, unix_name, size, NULL, NULL);

            if ((windows_name = wine_get_dos_file_name_ptr(unix_name)))
            {
                WideCharToMultiByte(CP_UNIXCP, 0, windows_name, -1, path, MAX_PATH, NULL, NULL);
                printf("%s%c", path, separator);
                HeapFree( GetProcessHeap(), 0, windows_name );
            }
            else printf("%c", separator);
            free( unix_name );
        }
    }

    exit(0);
}
