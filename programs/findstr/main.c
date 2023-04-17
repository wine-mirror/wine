/*
 * Copyright 2012 Qian Hong
 * Copyright 2023 Zhiyi Zhang for CodeWeavers
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

#include <windows.h>
#include <stdio.h>
#include "findstr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(findstr);

static int WINAPIV findstr_error_wprintf(int msg, ...)
{
    WCHAR msg_buffer[MAXSTRING];
    va_list va_args;
    int ret;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));
    va_start(va_args, msg);
    ret = vfwprintf(stderr, msg_buffer, va_args);
    va_end(va_args);
    return ret;
}

static int findstr_message(int msg)
{
    WCHAR msg_buffer[MAXSTRING];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));
    return wprintf(msg_buffer);
}

static BOOL add_file(struct findstr_file **head, const WCHAR *path)
{
    struct findstr_file **ptr, *new_file;

    ptr = head;
    while (*ptr)
        ptr = &((*ptr)->next);

    new_file = calloc(1, sizeof(*new_file));
    if (!new_file)
    {
        WINE_ERR("Out of memory.\n");
        return FALSE;
    }

    if (!path)
    {
        new_file->file = stdin;
    }
    else
    {
        new_file->file = _wfopen(path, L"rt,ccs=unicode");
        if (!new_file->file)
        {
            findstr_error_wprintf(STRING_CANNOT_OPEN, path);
            return FALSE;
        }
    }

    *ptr = new_file;
    return TRUE;
}

static void add_string(struct findstr_string **head, const WCHAR *string)
{
    struct findstr_string **ptr, *new_string;

    ptr = head;
    while (*ptr)
        ptr = &((*ptr)->next);

    new_string = calloc(1, sizeof(*new_string));
    if (!new_string)
    {
        WINE_ERR("Out of memory.\n");
        return;
    }

    new_string->string = string;
    *ptr = new_string;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    struct findstr_string *string_head = NULL, *current_string, *next_string;
    struct findstr_file *file_head = NULL, *current_file, *next_file;
    WCHAR *string, *ptr, *buffer, line[MAXSTRING];
    BOOL has_string = FALSE, has_file = FALSE;
    int ret = 1, i, j;

    for (i = 0; i < argc; i++)
        WINE_TRACE("%s ", wine_dbgstr_w(argv[i]));
    WINE_TRACE("\n");

    if (argc == 1)
    {
        findstr_error_wprintf(STRING_BAD_COMMAND_LINE);
        return 2;
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            if (argv[i][1] == '\0')
            {
                findstr_error_wprintf(STRING_BAD_COMMAND_LINE);
                return 2;
            }

            j = 1;
            while (argv[i][j] != '\0')
            {
                switch(argv[i][j])
                {
                case '?':
                    findstr_message(STRING_USAGE);
                    ret = 0;
                    goto done;
                case 'C':
                case 'c':
                    if (argv[i][j + 1] == ':')
                    {
                        ptr = argv[i] + j + 2;
                        if (*ptr == '"')
                            ptr++;

                        string = ptr;
                        while (*ptr != '"' && *ptr != '\0' )
                            ptr++;
                        *ptr = '\0';
                        j = ptr - argv[i] - 1;
                        add_string(&string_head, string);
                        has_string = TRUE;
                    }
                    break;
                default:
                    findstr_error_wprintf(STRING_IGNORED, argv[i][j]);
                    break;
                }

                j++;
            }
        }
        else if (!has_string)
        {
            string = wcstok(argv[i], L" ", &buffer);
            if (string)
            {
                add_string(&string_head, string);
                has_string = TRUE;
            }
            while ((string = wcstok(NULL, L" ", &buffer)))
                add_string(&string_head, string);
        }
        else
        {
            if (!add_file(&file_head, argv[i]))
                goto done;
            has_file = TRUE;
        }
    }

    if (!has_string)
    {
        findstr_error_wprintf(STRING_BAD_COMMAND_LINE);
        ret = 2;
        goto done;
    }

    if (!has_file)
        add_file(&file_head, NULL);

    current_file = file_head;
    while (current_file)
    {
        while (fgetws(line, ARRAY_SIZE(line), current_file->file))
        {
            current_string = string_head;
            while (current_string)
            {
                if (wcsstr(line, current_string->string))
                {
                    wprintf(line);
                    if (current_file->file == stdin)
                        wprintf(L"\n");
                    ret = 0;
                }

                current_string = current_string->next;
            }
        }

        current_file = current_file->next;
    }

done:
    current_file = file_head;
    while (current_file)
    {
        next_file = current_file->next;
        if (current_file->file != stdin)
            fclose(current_file->file);
        free(current_file);
        current_file = next_file;
    }

    current_string = string_head;
    while (current_string)
    {
        next_string = current_string->next;
        free(current_string);
        current_string = next_string;
    }
    return ret;
}
