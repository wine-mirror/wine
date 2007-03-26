/*
 * Miscellaneous tests
 *
 * Copyright 2007 James Hawkins
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
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

static CHAR CURR_DIR[MAX_PATH];

/* test:
 *  - fails if not administrator
 *  - what if it's not a .inf file?
 *  - copied to %windir%/Inf
 *  - SourceInfFileName should be a full path
 *  - SourceInfFileName should be <= MAX_PATH
 *  - copy styles
 */

static void append_str(char **str, const char *data)
{
    sprintf(*str, data);
    *str += strlen(*str);
}

static void create_inf_file(LPCSTR filename)
{
    char data[1024];
    char *ptr = data;
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    append_str(&ptr, "[Version]\n");
    append_str(&ptr, "Signature=\"$Chicago$\"\n");
    append_str(&ptr, "AdvancedINF=2.5\n");
    append_str(&ptr, "[DefaultInstall]\n");
    append_str(&ptr, "RegisterOCXs=RegisterOCXsSection\n");
    append_str(&ptr, "[RegisterOCXsSection]\n");
    append_str(&ptr, "%%11%%\\ole32.dll\n");

    WriteFile(hf, data, ptr - data, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void get_temp_filename(LPSTR path)
{
    CHAR temp[MAX_PATH];
    LPSTR ptr;

    GetTempFileName(CURR_DIR, "set", 0, temp);
    ptr = strrchr(temp, '\\');

    lstrcpy(path, ptr + 1);
}

static BOOL file_exists(LPSTR path)
{
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

static BOOL check_format(LPSTR path, LPSTR inf)
{
    CHAR check[MAX_PATH];
    BOOL res;

    static const CHAR format[] = "\\INF\\oem";

    GetWindowsDirectory(check, MAX_PATH);
    lstrcat(check, format);
    res = !strncmp(check, path, lstrlen(check)) &&
          path[lstrlen(check)] != '\\';

    return (!inf) ? res : res && (inf == path + lstrlen(check) - 3);
}

static void test_SetupCopyOEMInf(void)
{
    CHAR toolong[MAX_PATH * 2];
    CHAR path[MAX_PATH], dest[MAX_PATH];
    CHAR tmpfile[MAX_PATH];
    LPSTR inf;
    DWORD size;
    BOOL res;

    /* try NULL SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(NULL, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* try empty SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf("", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    /* try nonexistent SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf("nonexistent", NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    /* try a long SourceInfFileName */
    memset(toolong, 'a', MAX_PATH * 2);
    toolong[MAX_PATH * 2 - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(toolong, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    get_temp_filename(tmpfile);
    create_inf_file(tmpfile);

    /* try a relative SourceInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(tmpfile, NULL, 0, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }
    ok(file_exists(tmpfile), "Expected tmpfile to exist\n");

    /* try SP_COPY_REPLACEONLY, dest does not exist */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }
    ok(file_exists(tmpfile), "Expected source inf to exist\n");

    /* try an absolute SourceInfFileName, without DestinationInfFileName */
    lstrcpy(path, CURR_DIR);
    lstrcat(path, "\\");
    lstrcat(path, tmpfile);
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, 0, NULL, 0, NULL, NULL);
    todo_wine
    {
        ok(res == TRUE, "Expected TRUE, got %d\n", res);
        ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    }
    ok(file_exists(path), "Expected source inf to exist\n");

    /* try SP_COPY_REPLACEONLY, dest exists */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    todo_wine
    {
        ok(res == TRUE, "Expected TRUE, got %d\n", res);
        ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    }
    ok(file_exists(path), "Expected source inf to exist\n");

    /* try SP_COPY_NOOVERWRITE */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, SP_COPY_NOOVERWRITE, NULL, 0, NULL, NULL);
    ok(res == FALSE, "Expected FALSE, got %d\n", res);
    todo_wine
    {
        ok(GetLastError() == ERROR_FILE_EXISTS,
           "Expected ERROR_FILE_EXISTS, got %d\n", GetLastError());
    }

    /* get the DestinationInfFileName */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, 0, dest, MAX_PATH, NULL, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(lstrlen(dest) != 0, "Expected a non-zero length string\n");
    ok(file_exists(dest), "Expected destination inf to exist\n");
    todo_wine
    {
        ok(check_format(dest, NULL), "Expected %%windir%%\\inf\\OEMx.inf, got %s\n", dest);
    }
    ok(file_exists(path), "Expected source inf to exist\n");

    /* get the DestinationInfFileName and DestinationInfFileNameSize */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, 0, dest, MAX_PATH, &size, NULL);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(lstrlen(dest) + 1 == size, "Expected sizes to match, got (%d, %d)\n", lstrlen(dest), size);
    ok(file_exists(dest), "Expected destination inf to exist\n");
    todo_wine
    {
        ok(check_format(dest, NULL), "Expected %%windir%%\\inf\\OEMx.inf, got %s\n", dest);
    }
    ok(file_exists(path), "Expected source inf to exist\n");

    /* get the DestinationInfFileName, DestinationInfFileNameSize, and DestinationInfFileNameComponent */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, 0, dest, MAX_PATH, &size, &inf);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
    ok(lstrlen(dest) + 1 == size, "Expected sizes to match, got (%d, %d)\n", lstrlen(dest), size);
    ok(file_exists(dest), "Expected destination inf to exist\n");
    todo_wine
    {
        ok(check_format(dest, inf), "Expected %%windir%%\\inf\\OEMx.inf, got %s\n", dest);
    }
    ok(file_exists(path), "Expected source inf to exist\n");

    /* try SP_COPY_DELETESOURCE */
    SetLastError(0xdeadbeef);
    res = SetupCopyOEMInf(path, NULL, SPOST_NONE, SP_COPY_DELETESOURCE, NULL, 0, NULL, NULL);
    todo_wine
    {
        ok(res == TRUE, "Expected TRUE, got %d\n", res);
        ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", GetLastError());
        ok(!file_exists(path), "Expected source inf to not exist\n");
    }
}

START_TEST(misc)
{
    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    test_SetupCopyOEMInf();
}
