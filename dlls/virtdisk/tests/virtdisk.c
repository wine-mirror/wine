/*
 * Copyright 2018 Gijs Vermeulen
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
#include "initguid.h"
#include "virtdisk.h"
#include "wine/test.h"

static DWORD (WINAPI *pGetStorageDependencyInformation)(HANDLE,GET_STORAGE_DEPENDENCY_FLAG,ULONG,STORAGE_DEPENDENCY_INFO*,ULONG*);
static DWORD (WINAPI *pOpenVirtualDisk)(PVIRTUAL_STORAGE_TYPE,PCWSTR,VIRTUAL_DISK_ACCESS_MASK,OPEN_VIRTUAL_DISK_FLAG,POPEN_VIRTUAL_DISK_PARAMETERS,PHANDLE);

static void test_GetStorageDependencyInformation(void)
{
    DWORD ret;
    HANDLE handle;
    STORAGE_DEPENDENCY_INFO *info;
    ULONG size;

    handle = CreateFileA("C:", 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(handle != INVALID_HANDLE_VALUE, "Expected a handle\n");

    size = sizeof(STORAGE_DEPENDENCY_INFO);
    info = malloc(size);

    ret = pGetStorageDependencyInformation(handle, GET_STORAGE_DEPENDENCY_FLAG_DISK_HANDLE, 0, info, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    ret = pGetStorageDependencyInformation(handle, GET_STORAGE_DEPENDENCY_FLAG_DISK_HANDLE, size, NULL, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    free(info);
    CloseHandle(handle);
}

static void test_OpenVirtualDisk(void)
{
    DWORD ret;
    HANDLE handle;
    VIRTUAL_STORAGE_TYPE stgtype;
    OPEN_VIRTUAL_DISK_PARAMETERS param;
    static const WCHAR vdisk[] = L"test.vhd";

    ret = pOpenVirtualDisk(NULL, NULL, VIRTUAL_DISK_ACCESS_NONE, OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS, NULL, &handle);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    stgtype.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    stgtype.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;
    ret = pOpenVirtualDisk(&stgtype, NULL, VIRTUAL_DISK_ACCESS_NONE, OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS, NULL, &handle);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    param.Version = OPEN_VIRTUAL_DISK_VERSION_3;
    ret = pOpenVirtualDisk(&stgtype, vdisk, VIRTUAL_DISK_ACCESS_NONE, OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS, &param, &handle);
    ok((ret == ERROR_INVALID_PARAMETER) || (ret == ERROR_FILE_NOT_FOUND), "Expected ERROR_INVALID_PARAMETER or ERROR_FILE_NOT_FOUND (>= Win 10), got %ld\n", ret);

    param.Version = OPEN_VIRTUAL_DISK_VERSION_2;
    ret = pOpenVirtualDisk(&stgtype, vdisk, VIRTUAL_DISK_ACCESS_NONE, OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS, &param, &handle);
    ok((ret == ERROR_INVALID_PARAMETER) || (ret == ERROR_FILE_NOT_FOUND), "Expected ERROR_INVALID_PARAMETER or ERROR_FILE_NOT_FOUND (>= Win 8), got %ld\n", ret);

    param.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    ret = pOpenVirtualDisk(&stgtype, vdisk, 0xffffff, OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS, &param, &handle);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    ret = pOpenVirtualDisk(&stgtype, vdisk, VIRTUAL_DISK_ACCESS_NONE, OPEN_VIRTUAL_DISK_FLAG_NONE, &param, &handle);
    todo_wine ok(ret == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
}

START_TEST(virtdisk)
{
    HMODULE module = LoadLibraryA("virtdisk.dll");
    if(!module)
    {
        win_skip("virtdisk.dll not installed\n");
        return;
    }

    pGetStorageDependencyInformation = (void *)GetProcAddress( module, "GetStorageDependencyInformation" );
    pOpenVirtualDisk = (void *)GetProcAddress( module, "OpenVirtualDisk" );

    if (pGetStorageDependencyInformation)
        test_GetStorageDependencyInformation();
    else
        win_skip("GetStorageDependencyInformation is not available\n");

    if (pOpenVirtualDisk)
        test_OpenVirtualDisk();
    else
        win_skip("OpenVirtualDisk is not available\n");
}
