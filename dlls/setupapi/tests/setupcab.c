/*
 * Unit tests for SetupIterateCabinet
 *
 * Copyright 2007 Hans Leidekker
 * Copyright 2010 Andrew Nguyen
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"
#include "wine/test.h"

static void create_source_fileA(LPSTR filename, const BYTE *data, DWORD size)
{
    HANDLE handle;
    DWORD written;

    handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(handle, data, size, &written, NULL);
    CloseHandle(handle);
}

static UINT CALLBACK dummy_callbackA(PVOID Context, UINT Notification,
                                     UINT_PTR Param1, UINT_PTR Param2)
{
    ok(0, "Received unexpected notification (%p, %u, %lu, %lu)\n", Context,
       Notification, Param1, Param2);
    return 0;
}

static void test_invalid_parametersA(void)
{
    BOOL ret;
    char source[MAX_PATH], temp[MAX_PATH];
    int i;

    const struct
    {
        PCSTR CabinetFile;
        PSP_FILE_CALLBACK MsgHandler;
        DWORD expected_lasterror;
        int todo_lasterror;
    } invalid_parameters[] =
    {
        {NULL,                  NULL,            ERROR_INVALID_PARAMETER},
        {NULL,                  dummy_callbackA, ERROR_INVALID_PARAMETER},
        {"c:\\nonexistent.cab", NULL,            ERROR_FILE_NOT_FOUND},
        {"c:\\nonexistent.cab", dummy_callbackA, ERROR_FILE_NOT_FOUND},
        {source,                NULL,            ERROR_INVALID_DATA, 1},
        {source,                dummy_callbackA, ERROR_INVALID_DATA, 1},
    };

    GetTempPathA(sizeof(temp), temp);
    GetTempFileNameA(temp, "doc", 0, source);

    create_source_fileA(source, NULL, 0);

    for (i = 0; i < sizeof(invalid_parameters)/sizeof(invalid_parameters[0]); i++)
    {
        SetLastError(0xdeadbeef);
        ret = SetupIterateCabinetA(invalid_parameters[i].CabinetFile, 0,
                                   invalid_parameters[i].MsgHandler, NULL);
        ok(!ret, "[%d] Expected SetupIterateCabinetA to return 0, got %d\n", i, ret);
        if (invalid_parameters[i].todo_lasterror)
        {
            todo_wine
            ok(GetLastError() == invalid_parameters[i].expected_lasterror,
               "[%d] Expected GetLastError() to return %u, got %u\n",
               i, invalid_parameters[i].expected_lasterror, GetLastError());
        }
        else
        {
            ok(GetLastError() == invalid_parameters[i].expected_lasterror,
               "[%d] Expected GetLastError() to return %u, got %u\n",
               i, invalid_parameters[i].expected_lasterror, GetLastError());
        }
    }

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetA("", 0, NULL, NULL);
    ok(!ret, "Expected SetupIterateCabinetA to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_FILE_NOT_FOUND, /* Win9x/NT4/Win2k */
       "Expected GetLastError() to return ERROR_NOT_ENOUGH_MEMORY, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupIterateCabinetA("", 0, dummy_callbackA, NULL);
    ok(!ret, "Expected SetupIterateCabinetA to return 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_FILE_NOT_FOUND, /* Win9x/NT4/Win2k */
       "Expected GetLastError() to return ERROR_NOT_ENOUGH_MEMORY, got %u\n",
       GetLastError());

    DeleteFileA(source);
}

START_TEST(setupcab)
{
    test_invalid_parametersA();
}
