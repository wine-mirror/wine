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
#include "wine/debug.h"
#include "regproc.h"

WINE_DEFAULT_DEBUG_CHANNEL(regedit);

static void output_writeconsole(const WCHAR *str, DWORD wlen)
{
    DWORD count, ret;

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char  *msgA;

        /* WriteConsole() fails on Windows if its output is redirected. If this occurs,
         * we should call WriteFile() and assume the console encoding is still correct.
         */
        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len);
        if (!msgA) return;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, str, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }
}

static void output_formatstring(const WCHAR *fmt, __ms_va_list va_args)
{
    WCHAR *str;
    DWORD len;

    SetLastError(NO_ERROR);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
    if (len == 0 && GetLastError() != NO_ERROR)
    {
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }
    output_writeconsole(str, len);
    LocalFree(str);
}

void __cdecl output_message(unsigned int id, ...)
{
    WCHAR fmt[1536];
    __ms_va_list va_args;

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, sizeof(fmt)/sizeof(*fmt)))
    {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        return;
    }
    __ms_va_start(va_args, id);
    output_formatstring(fmt, va_args);
    __ms_va_end(va_args);
}

typedef enum {
    ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;

static void PerformRegAction(REGEDIT_ACTION action, WCHAR **argv, int *i)
{
    switch (action) {
    case ACTION_ADD: {
            WCHAR *filename = argv[*i];
            WCHAR hyphen[] = {'-',0};
            WCHAR *realname = NULL;
            FILE *reg_file;

            if (!strcmpW(filename, hyphen))
                reg_file = stdin;
            else
            {
                int size;
                WCHAR rb_mode[] = {'r','b',0};

                size = SearchPathW(NULL, filename, NULL, 0, NULL, NULL);
                if (size > 0)
                {
                    realname = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
                    size = SearchPathW(NULL, filename, NULL, size, realname, NULL);
                }
                if (size == 0)
                {
                    output_message(STRING_FILE_NOT_FOUND, filename);
                    HeapFree(GetProcessHeap(), 0, realname);
                    return;
                }
                reg_file = _wfopen(realname, rb_mode);
                if (reg_file == NULL)
                {
                    WCHAR regedit[] = {'r','e','g','e','d','i','t',0};
                    _wperror(regedit);
                    output_message(STRING_CANNOT_OPEN_FILE, filename);
                    HeapFree(GetProcessHeap(), 0, realname);
                    return;
                }
            }
            import_registry_file(reg_file);
            if (realname)
            {
                HeapFree(GetProcessHeap(), 0, realname);
                fclose(reg_file);
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
        output_message(STRING_UNHANDLED_ACTION);
        exit(1);
        break;
    }
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
            output_message(STRING_USAGE);
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
            output_message(STRING_INVALID_SWITCH, argv[i]);
            output_message(STRING_HELP);
            exit(1);
        }
    }

    if (i == argc)
    {
        switch (action)
        {
        case ACTION_ADD:
        case ACTION_EXPORT:
            output_message(STRING_NO_FILENAME);
            break;
        case ACTION_DELETE:
            output_message(STRING_NO_REG_KEY);
            break;
        }
        output_message(STRING_HELP);
        exit(1);
    }

    for (; i < argc; i++)
        PerformRegAction(action, argv, &i);

    LocalFree(argv);

    return TRUE;
}
