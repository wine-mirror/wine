/*
 * Unit test suite for drive functions.
 *
 * Copyright 2002 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"

void test_GetDriveTypeA(void)
{
    char drive[] = "?:\\";
    DWORD logical_drives;
    UINT type;

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        type = GetDriveTypeA(drive);
        ok(type > 0 && type <= 6, "not a valid drive %c: type %u", drive[0], type);

        if (!(logical_drives & 1))
            ok(type == DRIVE_NO_ROOT_DIR,
               "GetDriveTypeA should return DRIVE_NO_ROOT_DIR for a not existing drive %c: but not %u",
               drive[0], type);

        logical_drives >>= 1;
    }
}

void test_GetDriveTypeW(void)
{
    WCHAR drive[] = {'?',':','\\',0};
    DWORD logical_drives;
    UINT type;

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        type = GetDriveTypeW(drive);
        ok(type > 0 && type <= 6, "not a valid drive %c: type %u", drive[0], type);

        if (!(logical_drives & 1))
            ok(type == DRIVE_NO_ROOT_DIR,
               "GetDriveTypeW should return DRIVE_NO_ROOT_DIR for a not existing drive %c: but not %u",
               drive[0], type);

        logical_drives >>= 1;
    }
}

void test_GetDiskFreeSpaceA(void)
{
    BOOL ret;
    DWORD sectors_per_cluster, bytes_per_sector, free_clusters, total_clusters;
    char drive[] = "?:\\";
    DWORD logical_drives;

    ret = GetDiskFreeSpaceA(NULL, NULL, NULL, NULL, NULL);
    ok(ret, "GetDiskFreeSpaceA error %ld", GetLastError());

    ret = GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld", GetLastError());

    ret = GetDiskFreeSpaceA("", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND, "GetDiskFreeSpaceA should return ERROR_PATH_NOT_FOUND for \"\"");

    ret = GetDiskFreeSpaceA("\\", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld", GetLastError());

    ret = GetDiskFreeSpaceA("/", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld", GetLastError());

    ret = GetDiskFreeSpaceA(".", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_INVALID_NAME, "GetDiskFreeSpaceA should return ERROR_INVALID_NAME for \".\"");

    ret = GetDiskFreeSpaceA("..", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_INVALID_NAME, "GetDiskFreeSpaceA should return ERROR_INVALID_NAME for \"..\"");

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        ret = GetDiskFreeSpaceA(drive, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
        if (!(logical_drives & 1))
            ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
               "GetDiskFreeSpaceA should return ERROR_PATH_NOT_FOUND for a not existing drive %c",
               drive[0]);
        else
            ok(ret, "GetDiskFreeSpaceA error %ld", GetLastError());

        logical_drives >>= 1;
    }
}

void test_GetDiskFreeSpaceW(void)
{
    BOOL ret;
    DWORD sectors_per_cluster, bytes_per_sector, free_clusters, total_clusters;
    WCHAR drive[] = {'?',':','\\',0};
    DWORD logical_drives;
    static const WCHAR empty_pathW[] = { 0 };
    static const WCHAR root_pathW[] = { '\\', 0 };
    static const WCHAR unix_style_root_pathW[] = { '/', 0 };
    static const WCHAR cur_dirW[] = { '.', 0 };
    static const WCHAR upper_dirW[] = { '.','.', 0 };

    ret = GetDiskFreeSpaceW(NULL, NULL, NULL, NULL, NULL);
    ok(ret, "GetDiskFreeSpaceW error %ld", GetLastError());

    ret = GetDiskFreeSpaceW(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %ld", GetLastError());

    ret = GetDiskFreeSpaceW(empty_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND, "GetDiskFreeSpaceW should return ERROR_PATH_NOT_FOUND for \"\"");

    ret = GetDiskFreeSpaceW(root_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %ld", GetLastError());

    ret = GetDiskFreeSpaceW(unix_style_root_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %ld", GetLastError());

    ret = GetDiskFreeSpaceW(cur_dirW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_INVALID_NAME, "GetDiskFreeSpaceW should return ERROR_INVALID_NAME for \".\"");

    ret = GetDiskFreeSpaceW(upper_dirW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_INVALID_NAME, "GetDiskFreeSpaceW should return ERROR_INVALID_NAME for \"..\"");

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        ret = GetDiskFreeSpaceW(drive, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
        if (!(logical_drives & 1))
            ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
               "GetDiskFreeSpaceW should return ERROR_PATH_NOT_FOUND for a not existing drive %c",
               drive[0]);
        else
            ok(ret, "GetDiskFreeSpaceW error %ld", GetLastError());

        logical_drives >>= 1;
    }
}

START_TEST(drive)
{
    test_GetDriveTypeA();
    test_GetDriveTypeW();

    test_GetDiskFreeSpaceA();
    test_GetDiskFreeSpaceW();
}
