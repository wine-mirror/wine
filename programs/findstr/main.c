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
#include <io.h>
#include <fcntl.h>
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
        _setmode(_fileno(stdin), _O_BINARY);
    }
    else
    {
        new_file->file = _wfopen(path, L"rb");
        if (!new_file->file)
        {
            findstr_error_wprintf(STRING_CANNOT_OPEN, path);
            return FALSE;
        }
    }

    *ptr = new_file;
    return TRUE;
}

static inline char *strdupWA(const WCHAR *src)
{
    char *dst = NULL;
    if (src)
    {
        int len = WideCharToMultiByte(GetOEMCP(), 0, src, -1, NULL, 0, NULL, NULL);
        if ((dst = malloc(len))) WideCharToMultiByte(GetOEMCP(), 0, src, -1, dst, len, NULL, NULL);
    }
    return dst;
}

static void add_string(struct findstr_string **head, const WCHAR *string)
{
    struct findstr_string **ptr, *new_string;
    char *str = strdupWA(string);

    if (!str)
    {
        WINE_ERR("Out of memory.\n");
        return;
    }

    ptr = head;
    while (*ptr)
        ptr = &((*ptr)->next);

    new_string = calloc(1, sizeof(*new_string));
    if (!new_string)
    {
        WINE_ERR("Out of memory.\n");
        return;
    }

    new_string->string = str;
    *ptr = new_string;
}

static BOOL match_substring(const char *str, const char *substr, BOOL case_sensitive)
{
    if (case_sensitive) return !!strstr(str, substr);

    while (*str)
    {
        const char *p1 = str, *p2 = substr;

        while (*p1 && *p2 && tolower(*p1) == tolower(*p2))
        {
            p1++;
            p2++;
        }
        if (!*p2) return TRUE;
        str++;
    }
    return FALSE;
}

static inline BOOL is_op(char c, const char *regexp, UINT pos)
{
    if (!pos) return (*regexp == c);
    return (regexp[pos] == c && regexp[pos - 1] != '\\');
}

static inline BOOL match_char(char c1, char c2, BOOL case_sensitive)
{
    if (case_sensitive) return c1 == c2;
    return tolower(c1) == tolower(c2);
}

static BOOL match_star(char, const char *, const char *, UINT, BOOL);

static BOOL match_here(const char *str, const char *regexp, UINT pos, BOOL case_sensitive)
{
    if (regexp[pos] == '\\' && regexp[pos + 1]) pos++;
    if (!regexp[pos]) return TRUE;
    if (is_op('*', regexp, pos + 1)) return match_star(regexp[pos], str, regexp, pos + 2, case_sensitive);
    if (is_op('$', regexp, pos) && !regexp[pos + 1]) return (str[0] == '\n' || (str[0] == '\r' && str[1] == '\n'));
    if (*str && (is_op('.', regexp, pos) || match_char(*str, regexp[pos], case_sensitive)))
        return match_here(str + 1, regexp, pos + 1, case_sensitive);
    return FALSE;
}

static BOOL match_star(char c, const char *str, const char *regexp, UINT pos, BOOL case_sensitive)
{
    do { if (match_here(str, regexp, pos, case_sensitive)) return TRUE; }
    while (*str && (match_char(*str++, c, case_sensitive) || c == '.'));
    return FALSE;
}

static BOOL match_regexp(const char *str, const char *regexp, BOOL case_sensitive)
{
    if (strstr(regexp, "[")) FIXME("character ranges (i.e. [abc], [^a-z]) are not supported\n");
    if (strstr(regexp, "\\<") || strstr(regexp, "\\>")) FIXME("word position (i.e. \\< and \\>) not supported\n");

    if (regexp[0] == '^') return match_here(str, regexp, 1, case_sensitive);
    do { if (match_here(str, regexp, 0, case_sensitive)) return TRUE; } while (*str++);
    return FALSE;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    struct findstr_string *string_head = NULL, *current_string, *next_string;
    struct findstr_file *file_head = NULL, *current_file, *next_file;
    char line[MAXSTRING];
    WCHAR *string, *ptr, *buffer;
    BOOL has_string = FALSE, has_file = FALSE, case_sensitive = TRUE, regular_expression = TRUE;
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
                switch (argv[i][j])
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
                case 'I':
                case 'i':
                    case_sensitive = FALSE;
                    break;
                case 'L':
                case 'l':
                    regular_expression = FALSE;
                    break;
                case 'R':
                case 'r':
                    regular_expression = TRUE;
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

    _setmode(_fileno(stdout), _O_BINARY);

    current_file = file_head;
    while (current_file)
    {
        while (fgets(line, ARRAY_SIZE(line), current_file->file))
        {
            current_string = string_head;
            while (current_string)
            {
                BOOL match;

                if (regular_expression)
                    match = match_regexp(line, current_string->string, case_sensitive);
                else
                    match = match_substring(line, current_string->string, case_sensitive);
                if (match)
                {
                    printf("%s",line);
                    if (current_file->file == stdin && line[0] && line[strlen(line) - 1] != '\n')
                        printf("\r\n");
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
