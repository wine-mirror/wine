/*
 * Copyright 2017 Hugh McMaster
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
#include <stdlib.h>

#include <wine/unicode.h>
#include <wine/debug.h>

#include "reg.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static void write_file(HANDLE hFile, const WCHAR *str)
{
    DWORD written;

    WriteFile(hFile, str, lstrlenW(str) * sizeof(WCHAR), &written, NULL);
}

static WCHAR *escape_string(WCHAR *str, size_t str_len, size_t *line_len)
{
    size_t i, escape_count, pos;
    WCHAR *buf;

    for (i = 0, escape_count = 0; i < str_len; i++)
    {
        WCHAR c = str[i];
        if (c == '\r' || c == '\n' || c == '\\' || c == '"' || c == '\0')
            escape_count++;
    }

    buf = heap_xalloc((str_len + escape_count + 1) * sizeof(WCHAR));

    for (i = 0, pos = 0; i < str_len; i++, pos++)
    {
        WCHAR c = str[i];

        switch (c)
        {
        case '\r':
            buf[pos++] = '\\';
            buf[pos] = 'r';
            break;
        case '\n':
            buf[pos++] = '\\';
            buf[pos] = 'n';
            break;
        case '\\':
            buf[pos++] = '\\';
            buf[pos] = '\\';
            break;
        case '"':
            buf[pos++] = '\\';
            buf[pos] = '"';
            break;
        case '\0':
            buf[pos++] = '\\';
            buf[pos] = '0';
            break;
        default:
            buf[pos] = c;
        }
    }

    buf[pos] = 0;
    *line_len = pos;
    return buf;
}

static size_t export_value_name(HANDLE hFile, WCHAR *name, size_t len)
{
    static const WCHAR quoted_fmt[] = {'"','%','s','"','=',0};
    static const WCHAR default_name[] = {'@','=',0};
    size_t line_len;

    if (name && *name)
    {
        WCHAR *str = escape_string(name, len, &line_len);
        WCHAR *buf = heap_xalloc((line_len + 4) * sizeof(WCHAR));
        line_len = sprintfW(buf, quoted_fmt, str);
        write_file(hFile, buf);
        heap_free(buf);
        heap_free(str);
    }
    else
    {
        line_len = lstrlenW(default_name);
        write_file(hFile, default_name);
    }

    return line_len;
}

static void export_newline(HANDLE hFile)
{
    static const WCHAR newline[] = {'\r','\n',0};

    write_file(hFile, newline);
}

static void export_key_name(HANDLE hFile, WCHAR *name)
{
    static const WCHAR fmt[] = {'\r','\n','[','%','s',']','\r','\n',0};
    WCHAR *buf;

    buf = heap_xalloc((lstrlenW(name) + 7) * sizeof(WCHAR));
    sprintfW(buf, fmt, name);
    write_file(hFile, buf);
    heap_free(buf);
}

static int export_registry_data(HANDLE hFile, HKEY key, WCHAR *path)
{
    LONG rc;
    DWORD max_value_len = 256, value_len;
    DWORD i;
    WCHAR *value_name;

    export_key_name(hFile, path);

    value_name = heap_xalloc(max_value_len * sizeof(WCHAR));

    i = 0;
    for (;;)
    {
        value_len = max_value_len;
        rc = RegEnumValueW(key, i, value_name, &value_len, NULL, NULL, NULL, NULL);

        if (rc == ERROR_SUCCESS)
        {
            export_value_name(hFile, value_name, value_len);
            FIXME(": export of data types not yet implemented\n");
            export_newline(hFile);
            i++;
        }
        else if (rc == ERROR_MORE_DATA)
        {
            max_value_len *= 2;
            value_name = heap_xrealloc(value_name, max_value_len * sizeof(WCHAR));
        }
        else break;
    }

    heap_free(value_name);
    return 0;
}

static void export_file_header(HANDLE hFile)
{
    static const WCHAR header[] = { 0xfeff,'W','i','n','d','o','w','s',' ',
                                   'R','e','g','i','s','t','r','y',' ','E','d','i','t','o','r',' ',
                                   'V','e','r','s','i','o','n',' ','5','.','0','0','\r','\n'};

    write_file(hFile, header);
}

static HANDLE create_file(const WCHAR *filename, DWORD action)
{
    return CreateFileW(filename, GENERIC_WRITE, 0, NULL, action, FILE_ATTRIBUTE_NORMAL, NULL);
}

static HANDLE get_file_handle(WCHAR *filename, BOOL overwrite_file)
{
    HANDLE hFile = create_file(filename, overwrite_file ? CREATE_ALWAYS : CREATE_NEW);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();

        if (error == ERROR_FILE_EXISTS)
        {
            if (!ask_confirm(STRING_OVERWRITE_FILE, filename))
            {
                output_message(STRING_CANCELLED);
                exit(0);
            }

            hFile = create_file(filename, CREATE_ALWAYS);
        }
        else
        {
            WCHAR *str;

            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, 0, (WCHAR *)&str, 0, NULL);
            output_writeconsole(str, lstrlenW(str));
            LocalFree(str);
            exit(1);
        }
    }

    return hFile;
}

static BOOL is_overwrite_switch(const WCHAR *s)
{
    if (strlenW(s) > 2)
        return FALSE;

    if ((s[0] == '/' || s[0] == '-') && (s[1] == 'y' || s[1] == 'Y'))
        return TRUE;

    return FALSE;
}

int reg_export(int argc, WCHAR *argv[])
{
    HKEY root, hkey;
    WCHAR *path, *long_key;
    BOOL overwrite_file = FALSE;
    HANDLE hFile;
    int ret;

    if (argc == 3 || argc > 5)
        goto error;

    if (!parse_registry_key(argv[2], &root, &path, &long_key))
        return 1;

    if (argc == 5 && !(overwrite_file = is_overwrite_switch(argv[4])))
        goto error;

    if (RegOpenKeyExW(root, path, 0, KEY_READ, &hkey))
    {
        output_message(STRING_INVALID_KEY);
        return 1;
    }

    hFile = get_file_handle(argv[3], overwrite_file);
    export_file_header(hFile);
    ret = export_registry_data(hFile, hkey, long_key);
    export_newline(hFile);
    CloseHandle(hFile);

    RegCloseKey(hkey);

    return ret;

error:
    output_message(STRING_INVALID_SYNTAX);
    output_message(STRING_FUNC_HELP, struprW(argv[1]));
    return 1;
}
