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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
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

static BOOL PerformRegAction(REGEDIT_ACTION action, char **argv, int *i)
{
    switch (action) {
    case ACTION_ADD: {
            char *filename = argv[*i];
            FILE *reg_file;

            if (filename[0]) {
                char* realname = NULL;

                if (strcmp(filename, "-") == 0)
                {
                    reg_file = stdin;
                }
                else
                {
                    int size;

                    size = SearchPathA(NULL, filename, NULL, 0, NULL, NULL);
                    if (size > 0)
                    {
                        realname = HeapAlloc(GetProcessHeap(), 0, size);
                        size = SearchPathA(NULL, filename, NULL, size, realname, NULL);
                    }
                    if (size == 0)
                    {
                        fprintf(stderr, "regedit: File not found \"%s\" (%d)\n",
                                filename, GetLastError());
                        exit(1);
                    }
                    reg_file = fopen(realname, "rb");
                    if (reg_file == NULL)
                    {
                        perror("");
                        fprintf(stderr, "regedit: Can't open file \"%s\"\n", filename);
                        exit(1);
                    }
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
    case ACTION_DELETE: {
            WCHAR *reg_key_nameW = GetWideString(argv[*i]);

            delete_registry_key(reg_key_nameW);
            HeapFree(GetProcessHeap(), 0, reg_key_nameW);
            break;
        }
    case ACTION_EXPORT: {
            char *filename = argv[*i];
            WCHAR* filenameW;

            filenameW = GetWideString(filename);
            if (filenameW[0]) {
                char *reg_key_name = argv[++(*i)];
                WCHAR* reg_key_nameW;

                reg_key_nameW = GetWideString(reg_key_name);
                export_registry_key(filenameW, reg_key_nameW, REG_FORMAT_4);
                HeapFree(GetProcessHeap(), 0, reg_key_nameW);
            } else {
                export_registry_key(filenameW, NULL, REG_FORMAT_4);
            }
            HeapFree(GetProcessHeap(), 0, filenameW);
            break;
        }
    default:
        fprintf(stderr, "regedit: Unhandled action!\n");
        exit(1);
        break;
    }
    return TRUE;
}

static char *get_token(char *input, char **next)
{
    char *ch = input;
    char *str;

    while (*ch && isspace(*ch))
        ch++;

    str = ch;

    if (*ch == '"') {
        ch++;
        str = ch;
        for (;;) {
            while (*ch && (*ch != '"'))
                ch++;

            if (!*ch)
                break;

            if (*(ch - 1) == '\\') {
                ch++;
                continue;
            }
            break;
        }
    }
    else {
        while (*ch && !isspace(*ch))
            ch++;
    }

    if (*ch) {
        *ch = 0;
        ch++;
    }

    *next = ch;
    return str;
}

BOOL ProcessCmdLine(LPSTR lpCmdLine)
{
    char *s = lpCmdLine;
    char **argv;
    char *tok;
    int argc = 0, i = 1;
    REGEDIT_ACTION action = ACTION_ADD;

    if (!*lpCmdLine)
        return FALSE;

    while (*s) {
        if (isspace(*s))
            i++;
        s++;
    }

    s = lpCmdLine;
    argv = HeapAlloc(GetProcessHeap(), 0, i * sizeof(char *));

    for (i = 0; *s; i++)
    {
        tok = get_token(s, &s);
        argv[i] = HeapAlloc(GetProcessHeap(), 0, strlen(tok) + 1);
        strcpy(argv[i], tok);
        argc++;
    }

    for (i = 0; i < argc; i++)
    {
        if (argv[i][0] != '/' && argv[i][0] != '-')
            break; /* No flags specified. */

        if (!argv[i][1] && argv[i][0] == '-')
            break; /* '-' is a filename. It indicates we should use stdin. */

        if (argv[i][1] && argv[i][2] && argv[i][2] != ':')
            break; /* This is a file path beginning with '/'. */

        switch (toupper(argv[i][1]))
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
            fprintf(stderr, "regedit: Invalid switch [%s]\n", argv[i]);
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

    for (i = 0; i < argc; i++)
        HeapFree(GetProcessHeap(), 0, argv[i]);
    HeapFree(GetProcessHeap(), 0, argv);

    return TRUE;
}
