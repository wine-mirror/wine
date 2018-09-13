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
#include "virtdisk.h"
#include "wine/heap.h"
#include "wine/test.h"

static DWORD (WINAPI *pGetStorageDependencyInformation)(HANDLE,GET_STORAGE_DEPENDENCY_FLAG,ULONG,STORAGE_DEPENDENCY_INFO*,ULONG*);

static void test_GetStorageDependencyInformation(void)
{
    DWORD ret;
    HANDLE handle;
    STORAGE_DEPENDENCY_INFO *info;
    ULONG size;

    handle = CreateFileA("C:", 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(handle != INVALID_HANDLE_VALUE, "Expected a handle\n");

    size = sizeof(STORAGE_DEPENDENCY_INFO);
    info = heap_alloc(size);

    ret = pGetStorageDependencyInformation(handle, GET_STORAGE_DEPENDENCY_FLAG_DISK_HANDLE, 0, info, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", ret);

    ret = pGetStorageDependencyInformation(handle, GET_STORAGE_DEPENDENCY_FLAG_DISK_HANDLE, size, NULL, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", ret);

    heap_free(info);
    CloseHandle(handle);
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
    if (pGetStorageDependencyInformation)
        test_GetStorageDependencyInformation();
    else
        win_skip("GetStorageDependencyInformation is not available\n");
}
