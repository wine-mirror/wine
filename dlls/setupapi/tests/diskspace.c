/*
 * Unit tests for disk space functions
 *
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
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

static void test_SetupCreateDiskSpaceListA(void)
{
    HDSKSPC ret;

    ret = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    ret = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA(NULL, 0, ~0U);
    ok(ret == NULL ||
       broken(ret != NULL), /* NT4/Win9x/Win2k */
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    if (!ret)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
           GetLastError());
    else
        ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA(NULL, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA((void *)0xdeadbeef, 0, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA((void *)0xdeadbeef, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());
}

static void test_SetupCreateDiskSpaceListW(void)
{
    HDSKSPC ret;

    ret = SetupCreateDiskSpaceListW(NULL, 0, 0);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupCreateDiskSpaceListW is not implemented\n");
        return;
    }
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListW to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    ret = SetupCreateDiskSpaceListW(NULL, 0, SPDSL_IGNORE_DISK);
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListW to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW(NULL, 0, ~0U);
    ok(ret == NULL ||
       broken(ret != NULL), /* NT4/Win2k */
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    if (!ret)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
           GetLastError());
    else
        ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW(NULL, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW((void *)0xdeadbeef, 0, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW((void *)0xdeadbeef, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());
}

static void test_SetupDuplicateDiskSpaceListA(void)
{
    HDSKSPC handle, duplicate;

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, NULL, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    if (!handle)
    {
        skip("Failed to create a disk space handle\n");
        return;
    }

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0, SPDSL_IGNORE_DISK);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0, 0);
    ok(duplicate != NULL, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(duplicate != handle,
       "Expected new handle (%p) to be different from the old handle (%p)\n", duplicate, handle);

    ok(SetupDestroyDiskSpaceList(duplicate), "Expected SetupDestroyDiskSpaceList to succeed\n");
    ok(SetupDestroyDiskSpaceList(handle), "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_SetupDuplicateDiskSpaceListW(void)
{
    HDSKSPC handle, duplicate;

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, NULL, 0, 0);
    if (!duplicate && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupDuplicateDiskSpaceListW is not available\n");
        return;
    }
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListW to return a valid handle, got NULL\n");

    if (!handle)
    {
        skip("Failed to create a disk space handle\n");
        return;
    }

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0, SPDSL_IGNORE_DISK);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0, 0);
    ok(duplicate != NULL, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(duplicate != handle,
       "Expected new handle (%p) to be different from the old handle (%p)\n", duplicate, handle);

    ok(SetupDestroyDiskSpaceList(duplicate), "Expected SetupDestroyDiskSpaceList to succeed\n");
    ok(SetupDestroyDiskSpaceList(handle), "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_SetupQuerySpaceRequiredOnDriveA(void)
{
    char temp_path[MAX_PATH], drive[3];
    BOOL ret;
    HDSKSPC handle;
    LONGLONG space;
    HANDLE file;

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, NULL, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
    "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
    GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, "", NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, "", &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(handle, NULL, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* Win9x */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* Win9x */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "", NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "", &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "c:", &space, NULL, 0);
    ok(!ret, "got %d\n", ret);
    ok(space == 0xdeadbeef, "got space %I64d\n", space);
    ok(GetLastError() == ERROR_INVALID_DRIVE, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "C:/file1", 10000, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "C:/file2", 13, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "C:\\file1", 5000, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "C:/file1", 4000, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "D:/file1", 4000, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "c", &space, NULL, 0);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DRIVE, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "c:\\", &space, NULL, 0);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DRIVE, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "c:", &space, NULL, 0);
    todo_wine ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(space == (4096 * 3), "got space %I64d\n", space);

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    /* Test treatment of a file that exists. */

    GetTempPathA(ARRAY_SIZE(temp_path), temp_path);
    strcat(temp_path, "\\winetest_diskspace");

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(!!handle, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, temp_path, 10000, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    drive[0] = temp_path[0];
    drive[1] = ':';
    drive[2] = 0;

    file = CreateFileA(temp_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());
    SetFilePointer(file, 1025, NULL, FILE_BEGIN);
    ret = SetEndOfFile(file);
    ok(ret == TRUE, "got error %d\n", errno);

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    ok(!space, "got space %I64d\n", space);

    ret = SetupAddToDiskSpaceListA(handle, temp_path, 10000, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(space == -4096, "got space %I64d\n", space);

    ret = SetupAddToDiskSpaceListA(handle, temp_path, 4097, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(space == 4096, "got space %I64d\n", space);

    /* Delete after copy does not replace, and seems to be ignored. */
    ret = SetupAddToDiskSpaceListA(handle, temp_path, 10000, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(space == 4096, "got space %I64d\n", space);

    ret = SetupDestroyDiskSpaceList(handle);
    ok(ret, "got error %lu\n", GetLastError());

    /* Test copy without delete. */

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(!!handle, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, temp_path, 4097, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(space == 4096, "got space %I64d\n", space);

    ret = SetupDestroyDiskSpaceList(handle);
    ok(ret, "got error %lu\n", GetLastError());

    /* With SPDSL_IGNORE_DISK. */

    handle = SetupCreateDiskSpaceListW(NULL, 0, SPDSL_IGNORE_DISK);
    ok(!!handle, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, temp_path, 4097, FILEOP_COPY, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(space == (4096 * 2), "got space %I64d\n", space);

    ret = SetupDestroyDiskSpaceList(handle);
    ok(ret, "got error %lu\n", GetLastError());

    CloseHandle(file);
    ret = DeleteFileA(temp_path);
    ok(ret, "got error %lu\n", GetLastError());
}

static void test_SetupQuerySpaceRequiredOnDriveW(void)
{
    static const WCHAR emptyW[] = {0};

    BOOL ret;
    HDSKSPC handle;
    LONGLONG space;

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, NULL, NULL, NULL, 0);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupQuerySpaceRequiredOnDriveW is not available\n");
        return;
    }
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, emptyW, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, emptyW, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(handle, NULL, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* NT4/Win2k/XP/Win2k3 */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(handle, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* NT4/Win2k/XP/Win2k3 */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(handle, emptyW, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(handle, emptyW, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_add_files(void)
{
    HDSKSPC handle;
    BOOL ret;

    static const struct
    {
        const char *path;
        unsigned int op;
        DWORD error;
    }
    tests[] =
    {
        {"W:/file1", FILEOP_RENAME, ERROR_INVALID_PARAMETER},
        {"W:/file1", FILEOP_BACKUP, ERROR_INVALID_PARAMETER},
        {"W:/file1", FILEOP_DELETE},
        {"W:/file1", FILEOP_DELETE}, /* add the same file twice */
        {"w:file1", FILEOP_COPY},
        {"w:file1", FILEOP_COPY}, /* add the same file twice */
        {"W:/dir/", FILEOP_DELETE, ERROR_INVALID_PARAMETER},
        {"W:/dir/", FILEOP_COPY, ERROR_INVALID_PARAMETER},
        {"C:/windows", FILEOP_COPY},
        {"../file1", FILEOP_COPY},
        {"\\\\unc\\file1", FILEOP_COPY, ERROR_INVALID_PARAMETER},
        {"\\\\.\\file1", FILEOP_COPY, ERROR_INVALID_PARAMETER},
        {"\\\\?\\file1", FILEOP_COPY, ERROR_INVALID_PARAMETER},
        {"", FILEOP_COPY, ERROR_INVALID_NAME},
    };

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(!!handle, "got error %lu\n", GetLastError());

    for (unsigned int i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        SetLastError(0xdeadbeef);
        ret = SetupAddToDiskSpaceListA(handle, tests[i].path, 0, tests[i].op, 0, 0);
        ok(ret == !GetLastError(), "got %d\n", ret);
        todo_wine ok(GetLastError() == tests[i].error, "got error %lu for path %s, op %u\n",
                GetLastError(), tests[i].path, tests[i].op);
    }

    ret = SetupDestroyDiskSpaceList(handle);
    ok(ret, "got error %lu\n", GetLastError());
}

static void test_query_drives(void)
{
    char buffer[20], cwd[MAX_PATH];
    WCHAR bufferW[20];
    HDSKSPC handle;
    DWORD len;
    BOOL ret;

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(!!handle, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQueryDrivesInDiskSpaceListA(handle, NULL, 0, NULL);
    todo_wine ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQueryDrivesInDiskSpaceListA(handle, NULL, 0, &len);
    todo_wine ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(len == 1, "got len %lu\n", len);

    SetLastError(0xdeadbeef);
    len = 0xdeadbeef;
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 0, &len);
    ok(!ret, "got %d\n", ret);
    todo_wine ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    todo_wine ok(len == 1, "got len %lu\n", len);

    SetLastError(0xdeadbeef);
    len = 0xdeadbeef;
    memset(buffer, 0xcc, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 1, &len);
    todo_wine ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(len == 1, "got len %lu\n", len);
    todo_wine ok(!buffer[0], "got %s\n", debugstr_an(buffer, len));

    ret = SetupAddToDiskSpaceListA(handle, "P:/file1", 123, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "P:/file2", 123, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "H:file3", 123, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "R:/file4", 123, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    len = 0xdeadbeef;
    memset(buffer, 0xcc, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 4, &len);
    ok(!ret, "got %d\n", ret);
    todo_wine ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    todo_wine ok(len == 7, "got len %lu\n", len);
    todo_wine ok(!memcmp(buffer, "h:\0\xcc", 4), "got %s\n", debugstr_an(buffer, len));

    SetLastError(0xdeadbeef);
    len = 0xdeadbeef;
    memset(bufferW, 0xcc, sizeof(bufferW));
    ret = SetupQueryDrivesInDiskSpaceListW(handle, bufferW, 4, &len);
    ok(!ret, "got %d\n", ret);
    todo_wine ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    todo_wine ok(len == 7, "got len %lu\n", len);
    todo_wine ok(!memcmp(bufferW, L"h:\0\xcccc", 4 * sizeof(WCHAR)), "got %s\n", debugstr_wn(bufferW, len));

    SetLastError(0xdeadbeef);
    len = 0xdeadbeef;
    memset(buffer, 0xcc, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 8, &len);
    ok(!ret, "got %d\n", ret);
    todo_wine ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
    todo_wine ok(len == 10, "got len %lu\n", len);
    todo_wine ok(!memcmp(buffer, "h:\0p:\0\xcc\xcc", 8), "got %s\n", debugstr_an(buffer, len));

    SetLastError(0xdeadbeef);
    len = 0xdeadbeef;
    memset(buffer, 0xcc, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, sizeof(buffer), &len);
    todo_wine ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(len == 10, "got len %lu\n", len);
    todo_wine ok(!memcmp(buffer, "h:\0p:\0r:\0", 10), "got %s\n", debugstr_an(buffer, len));

    ret = SetupDestroyDiskSpaceList(handle);
    ok(ret, "got error %lu\n", GetLastError());

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(!!handle, "got error %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, "file5", 123, FILEOP_DELETE, 0, 0);
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    GetCurrentDirectoryA(sizeof(cwd), cwd);
    cwd[0] = tolower(cwd[0]);
    cwd[2] = cwd[3] = 0;

    SetLastError(0xdeadbeef);
    memset(buffer, 0xcc, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, sizeof(buffer), NULL);
    todo_wine ok(ret == TRUE, "got %d\n", ret);
    todo_wine ok(!GetLastError(), "got error %lu\n", GetLastError());
    todo_wine ok(!memcmp(buffer, cwd, 4), "expected %s, got %s\n", debugstr_an(cwd, 4), debugstr_an(buffer, len));

    ret = SetupDestroyDiskSpaceList(handle);
    ok(ret, "got error %lu\n", GetLastError());
}

START_TEST(diskspace)
{
    test_SetupCreateDiskSpaceListA();
    test_SetupCreateDiskSpaceListW();
    test_SetupDuplicateDiskSpaceListA();
    test_SetupDuplicateDiskSpaceListW();
    test_add_files();
    test_SetupQuerySpaceRequiredOnDriveA();
    test_SetupQuerySpaceRequiredOnDriveW();
    test_query_drives();
}
