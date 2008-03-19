/*
 * Copyright 2008 Andrew Riedi
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
#include "reg.h"

static int reg_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    va_start(va_args, msg);
    wvsprintf(msg_buffer, msg, va_args);
    va_end(va_args);

    wlen = lstrlenW(msg_buffer);
    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char  *msgA;

        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen,
            NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len * sizeof(char));
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen, msgA, len,
            NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }

    return count;
}

static int reg_message(int msg, ...)
{
    va_list va_args;
    WCHAR msg_buffer[8192];

    LoadString(GetModuleHandle(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));
    va_start(va_args, msg);
    reg_printfW(msg_buffer, va_args);
    va_end(va_args);

    return 0;
}

static int reg_add(WCHAR *key_name, WCHAR *value_name, BOOL value_empty,
    WCHAR *type, WCHAR separator, WCHAR *data, BOOL force)
{
    const WCHAR stubW[] = {'S','T','U','B',' ','A','D','D',' ','-',' ','%','s',
        ' ','%','s',' ','%','d',' ','%','s',' ','%','s',' ','%','d','\n',0};
    reg_printfW(stubW, key_name, value_name, value_empty, type, data, force);

    return 1;
}

static int reg_delete(WCHAR *key_name, WCHAR *value_name, BOOL value_empty,
    BOOL value_all, BOOL force)
{
    const WCHAR stubW[] = {'S','T','U','B',' ','D','E','L','E','T','E',' ','-',
        ' ','%','s',' ','%','s',' ','%','d',' ','%','d',' ','%','d','\n',0};
    reg_printfW(stubW, key_name, value_name, value_empty, value_all, force);

    return 1;
}

static int reg_query(WCHAR *key_name, WCHAR *value_name, BOOL value_empty,
    BOOL subkey)
{
    const WCHAR stubW[] = {'S','T','U','B',' ','Q','U','E','R','Y',' ','-',' ',
        '%','s',' ','%','s',' ','%','d',' ','%','d','\n',0};
    reg_printfW(stubW, key_name, value_name, value_empty, subkey);

    return 1;
}

int wmain(int argc, WCHAR *argvW[])
{
    int i;

    const WCHAR addW[] = {'a','d','d',0};
    const WCHAR deleteW[] = {'d','e','l','e','t','e',0};
    const WCHAR queryW[] = {'q','u','e','r','y',0};
    const WCHAR slashDW[] = {'/','d',0};
    const WCHAR slashFW[] = {'/','f',0};
    const WCHAR slashSW[] = {'/','s',0};
    const WCHAR slashTW[] = {'/','t',0};
    const WCHAR slashVW[] = {'/','v',0};
    const WCHAR slashVAW[] = {'/','v','a',0};
    const WCHAR slashVEW[] = {'/','v','e',0};

    if (argc < 2)
    {
        reg_message(STRING_USAGE);
        return 1;
    }

    if (!lstrcmpiW(argvW[1], addW))
    {
        WCHAR *key_name, *value_name = NULL, *type = NULL, separator = '\0', *data = NULL;
        BOOL value_empty = FALSE, force = FALSE;

        if (argc < 3)
        {
            reg_message(STRING_ADD_USAGE);
            return 1;
        }

        key_name = argvW[2];
        for (i = 1; i < argc; i++)
        {
            if (!lstrcmpiW(argvW[i], slashVW))
                value_name = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashVEW))
                value_empty = TRUE;
            else if (!lstrcmpiW(argvW[i], slashTW))
                type = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashSW))
                separator = argvW[++i][0];
            else if (!lstrcmpiW(argvW[i], slashDW))
                data = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashFW))
                force = TRUE;
        }
        return reg_add(key_name, value_name, value_empty, type, separator,
            data, force);
    }
    else if (!lstrcmpiW(argvW[1], deleteW))
    {
        WCHAR *key_name, *value_name = NULL;
        BOOL value_empty = FALSE, value_all = FALSE, force = FALSE;

        if (argc < 3)
        {
            reg_message(STRING_DELETE_USAGE);
            return 1;
        }

        key_name = argvW[2];
        for (i = 1; i < argc; i++)
        {
            if (!lstrcmpiW(argvW[i], slashVW))
                value_name = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashVEW))
                value_empty = TRUE;
            else if (!lstrcmpiW(argvW[i], slashVAW))
                value_all = TRUE;
            else if (!lstrcmpiW(argvW[i], slashFW))
                force = TRUE;
        }
        return reg_delete(key_name, value_name, value_empty, value_all, force);
    }
    else if (!lstrcmpiW(argvW[1], queryW))
    {
        WCHAR *key_name, *value_name = NULL;
        BOOL value_empty = FALSE, subkey = FALSE;

        if (argc < 3)
        {
            reg_message(STRING_QUERY_USAGE);
            return 1;
        }

        key_name = argvW[2];
        for (i = 1; i < argc; i++)
        {
            if (!lstrcmpiW(argvW[i], slashVW))
                value_name = argvW[++i];
            else if (!lstrcmpiW(argvW[i], slashVEW))
                value_empty = TRUE;
            else if (!lstrcmpiW(argvW[i], slashSW))
                subkey = TRUE;
        }
        return reg_query(key_name, value_name, value_empty, subkey);
    }

    return 0;
}
