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

#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "wine/debug.h"
#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(regedit);

static BOOL silent;

static void output_formatstring(BOOL with_help, UINT id, va_list va_args)
{
    WCHAR buffer[4096];
    WCHAR fmt[1536];
    DWORD len;
    LCID  current_lcid;

    if (silent && with_help) return;

    current_lcid = GetThreadLocale();
    if (silent) /* force en-US not to have localized strings */
        SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        if (silent) SetThreadLocale(current_lcid);
        return;
    }

    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING,
                         fmt, 0, 0, buffer, ARRAY_SIZE(buffer), &va_args);
    if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    {
        WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        if (silent) SetThreadLocale(current_lcid);
        return;
    }
    if (with_help &&
        !LoadStringW(GetModuleHandleW(NULL), STRING_HELP,
                     &buffer[wcslen(buffer)], ARRAY_SIZE(buffer) - wcslen(buffer)))
    {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        if (silent) SetThreadLocale(current_lcid);
        return;
    }
    if (silent)
    {
        MESSAGE("%ls", buffer);
        SetThreadLocale(current_lcid);
    }
    else
        MessageBoxW(NULL, buffer, MAKEINTRESOURCEW(IDS_APP_TITLE), MB_OK | MB_ICONHAND);
}

void WINAPIV output_message(unsigned int id, ...)
{
    va_list va_args;

    va_start(va_args, id);
    output_formatstring(FALSE, id, va_args);
    va_end(va_args);
}

void WINAPIV error_exit(void)
{
    exit(0); /* regedit.exe always terminates with error code zero */
}

static void WINAPIV usage(unsigned int id, ...)
{
    va_list va_args;

    va_start(va_args, id);
    output_formatstring(TRUE, id, va_args);
    va_end(va_args);

    error_exit();
}

typedef enum {
    ACTION_ADD, ACTION_EXPORT, ACTION_DELETE
} REGEDIT_ACTION;

static void PerformRegAction(REGEDIT_ACTION action, WCHAR **argv, int *i)
{
    switch (action) {
    case ACTION_ADD: {
            WCHAR *filename = argv[*i];
            WCHAR *realname = NULL;
            FILE *reg_file;

            if (!lstrcmpW(filename, L"-"))
                reg_file = stdin;
            else
            {
                int size;

                size = SearchPathW(NULL, filename, NULL, 0, NULL, NULL);
                if (size > 0)
                {
                    realname = malloc(size * sizeof(WCHAR));
                    size = SearchPathW(NULL, filename, NULL, size, realname, NULL);
                }
                if (size == 0)
                {
                    output_message(STRING_FILE_NOT_FOUND, filename);
                    free(realname);
                    return;
                }
                reg_file = _wfopen(realname, L"rb");
                if (reg_file == NULL)
                {
                    _wperror(L"regedit");
                    output_message(STRING_CANNOT_OPEN_FILE, filename);
                    free(realname);
                    return;
                }
            }
            import_registry_file(reg_file);
            if (realname)
            {
                free(realname);
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
                export_registry_key(filename, key_name, REG_FORMAT_5);
            else
                export_registry_key(filename, NULL, REG_FORMAT_5);
            break;
        }
    default:
        output_message(STRING_UNHANDLED_ACTION);
        error_exit();
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

        switch (towupper(argv[i][1]))
        {
        case '?':
            output_message(STRING_USAGE);
            error_exit();
            break;
        case 'D':
            action = ACTION_DELETE;
            break;
        case 'E':
            action = ACTION_EXPORT;
            break;
        case 'C':
        case 'L':
        case 'M':
        case 'R':
            /* unhandled */;
            break;
        case 'S':
            silent = TRUE;
            break;
        case 'V':
            /* ignored */;
            break;
        default:
            usage(STRING_INVALID_SWITCH, argv[i]);
        }
    }

    if (i == argc)
    {
        switch (action)
        {
        case ACTION_ADD:
        case ACTION_EXPORT:
            usage(STRING_NO_FILENAME);
            break;
        case ACTION_DELETE:
            usage(STRING_NO_REG_KEY);
            break;
        }
    }

    for (; i < argc; i++)
        PerformRegAction(action, argv, &i);

    LocalFree(argv);

    return TRUE;
}
