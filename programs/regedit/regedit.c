/*
 * Windows regedit.exe registry editor implementation.
 *
 * Copyright 2002 Andriy Palamarchuk
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

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>
#include "wine/unicode.h"
#include "regproc.h"

static const char *usage =
    "Usage:\n"
    "    regedit filename\n"
    "    regedit /E filename [regpath]\n"
    "    regedit /D regpath\n"
    "\n"
    "filename - registry file name\n"
    "regpath - name of the registry key\n"
    "\n"
    "When called without any switches, adds the content of the specified\n"
    "file to the registry\n"
    "\n"
    "Switches:\n"
    "    /E - exports contents of the specified registry key to the specified\n"
    "	file. Exports the whole registry if no key is specified.\n"
    "    /D - deletes specified registry key\n"
    "    /S - silent execution, can be used with any other switch.\n"
    "	Default. The only existing mode, exists for compatibility with Windows regedit.\n"
    "    /V - advanced mode, can be used with any other switch.\n"
    "	Ignored, exists for compatibility with Windows regedit.\n"
    "    /L - location of system.dat file. Can be used with any other switch.\n"
    "	Ignored. Exists for compatibility with Windows regedit.\n"
    "    /R - location of user.dat file. Can be used with any other switch.\n"
    "	Ignored. Exists for compatibility with Windows regedit.\n"
    "    /? - print this help. Any other switches are ignored.\n"
    "    /C - create registry from file. Not implemented.\n"
    "\n"
    "The switches are case-insensitive, can be prefixed either by '-' or '/'.\n"
    "This program is command-line compatible with Microsoft Windows\n"
    "regedit.\n";

typedef enum {
    ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;

static BOOL PerformRegAction(REGEDIT_ACTION action, WCHAR **argv, int *i)
{
    switch (action) {
    case ACTION_ADD: {
            WCHAR *filename = argv[*i];
            WCHAR hyphen[] = {'-',0};
            FILE *reg_file;

            if (!strcmpW(filename, hyphen))
                reg_file = stdin;
            else
            {
                int size;
                WCHAR *realname = NULL;
                WCHAR rb_mode[] = {'r','b',0};

                size = SearchPathW(NULL, filename, NULL, 0, NULL, NULL);
                if (size > 0)
                {
                    realname = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
                    size = SearchPathW(NULL, filename, NULL, size, realname, NULL);
                }
                if (size == 0)
                {
                    fprintf(stderr, "regedit: File not found \"%ls\" (%d)\n",
                            filename, GetLastError());
                    exit(1);
                }
                reg_file = _wfopen(realname, rb_mode);
                if (reg_file == NULL)
                {
                    WCHAR regedit[] = {'r','e','g','e','d','i','t',0};
                    _wperror(regedit);
                    fprintf(stderr, "regedit: Can't open file \"%ls\"\n", filename);
                    exit(1);
                }
                import_registry_file(reg_file);
                if (realname)
                {
                    HeapFree(GetProcessHeap(),0,realname);
                    fclose(reg_file);
                }
            }
            break;
        }
    case ACTION_DELETE:
            delete_registry_key(argv[*i]);
            break;
    case ACTION_EXPORT: {
            WCHAR *filename = argv[*i];
            WCHAR *key_name = argv[++(*i)];

            if (key_name && *key_name)
                export_registry_key(filename, key_name, REG_FORMAT_4);
            else
                export_registry_key(filename, NULL, REG_FORMAT_4);
            break;
        }
    default:
        fprintf(stderr, "regedit: Unhandled action!\n");
        exit(1);
        break;
    }
    return TRUE;
}

BOOL ProcessCmdLine(WCHAR *cmdline)
{
    WCHAR **argv;
    int argc, i;
    REGEDIT_ACTION action = ACTION_ADD;

    argv = CommandLineToArgvW(cmdline, &argc);

    if (!argv)
        return FALSE;

    if (argc == 1)
    {
        LocalFree(argv);
        return FALSE;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '/' && argv[i][0] != '-')
            break; /* No flags specified. */

        if (!argv[i][1] && argv[i][0] == '-')
            break; /* '-' is a filename. It indicates we should use stdin. */

        if (argv[i][1] && argv[i][2] && argv[i][2] != ':')
            break; /* This is a file path beginning with '/'. */

        switch (toupperW(argv[i][1]))
        {
        case '?':
            fprintf(stderr, usage);
            exit(0);
            break;
        case 'D':
            action = ACTION_DELETE;
            break;
        case 'E':
            action = ACTION_EXPORT;
            break;
        case 'C':
        case 'L':
        case 'R':
            /* unhandled */;
            break;
        case 'S':
        case 'V':
            /* ignored */;
            break;
        default:
            fprintf(stderr, "regedit: Invalid switch [%ls]\n", argv[i]);
            exit(1);
        }
    }

    if (i == argc)
    {
        switch (action)
        {
        case ACTION_ADD:
        case ACTION_EXPORT:
            fprintf(stderr, "regedit: No file name was specified\n\n");
            break;
        case ACTION_DELETE:
            fprintf(stderr,"regedit: No registry key was specified for removal\n\n");
            break;
        }
        fprintf(stderr, usage);
        exit(1);
    }

    for (; i < argc; i++)
        PerformRegAction(action, argv, &i);

    LocalFree(argv);

    return TRUE;
}
