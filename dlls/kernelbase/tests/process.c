/*
 * Process tests
 *
 * Copyright 2021 Jinoh Kang
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
#include <stdlib.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <winternl.h>

#include "wine/test.h"

static NTSTATUS (WINAPI *pNtQueryObject)(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);

static BOOL (WINAPI *pCompareObjectHandles)(HANDLE, HANDLE);
static LPVOID (WINAPI *pMapViewOfFile3)(HANDLE, HANDLE, PVOID, ULONG64 offset, SIZE_T size,
        ULONG, ULONG, MEM_EXTENDED_PARAMETER *, ULONG);
static LPVOID (WINAPI *pVirtualAlloc2)(HANDLE, void *, SIZE_T, DWORD, DWORD, MEM_EXTENDED_PARAMETER *, ULONG);
static LPVOID (WINAPI *pVirtualAlloc2FromApp)(HANDLE, void *, SIZE_T, DWORD, DWORD, MEM_EXTENDED_PARAMETER *, ULONG);
static PVOID (WINAPI *pVirtualAllocFromApp)(PVOID, SIZE_T, DWORD, DWORD);
static BOOL (WINAPI *pVirtualProtectFromApp)(LPVOID,SIZE_T,ULONG,PULONG);
static HANDLE (WINAPI *pOpenFileMappingFromApp)( ULONG, BOOL, LPCWSTR);
static HANDLE (WINAPI *pCreateFileMappingFromApp)(HANDLE, PSECURITY_ATTRIBUTES, ULONG, ULONG64, PCWSTR);
static LPVOID (WINAPI *pMapViewOfFileFromApp)(HANDLE, ULONG, ULONG64, SIZE_T);
static BOOL (WINAPI *pUnmapViewOfFile2)(HANDLE, void *, ULONG);

static void test_CompareObjectHandles(void)
{
    HANDLE h1, h2;
    BOOL ret;

    if (!pCompareObjectHandles)
    {
        win_skip("CompareObjectHandles is not available.\n");
        return;
    }

    ret = pCompareObjectHandles( GetCurrentProcess(), GetCurrentProcess() );
    ok( ret, "comparing GetCurrentProcess() to self failed with %lu\n", GetLastError() );

    ret = pCompareObjectHandles( GetCurrentThread(), GetCurrentThread() );
    ok( ret, "comparing GetCurrentThread() to self failed with %lu\n", GetLastError() );

    SetLastError(0);
    ret = pCompareObjectHandles( GetCurrentProcess(), GetCurrentThread() );
    ok( !ret && GetLastError() == ERROR_NOT_SAME_OBJECT,
        "comparing GetCurrentProcess() to GetCurrentThread() returned %lu\n", GetLastError() );

    h1 = NULL;
    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                           &h1, 0, FALSE, DUPLICATE_SAME_ACCESS );
    ok( ret, "failed to duplicate current process handle: %lu\n", GetLastError() );

    ret = pCompareObjectHandles( GetCurrentProcess(), h1 );
    ok( ret, "comparing GetCurrentProcess() with %p failed with %lu\n", h1, GetLastError() );

    CloseHandle( h1 );

    h1 = CreateFileA( "\\\\.\\NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h1 != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    h2 = NULL;
    ret = DuplicateHandle( GetCurrentProcess(), h1, GetCurrentProcess(),
                           &h2, 0, FALSE, DUPLICATE_SAME_ACCESS );
    ok( ret, "failed to duplicate handle %p: %lu\n", h1, GetLastError() );

    ret = pCompareObjectHandles( h1, h2 );
    ok( ret, "comparing %p with %p failed with %lu\n", h1, h2, GetLastError() );

    CloseHandle( h2 );

    h2 = CreateFileA( "\\\\.\\NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h2 != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    SetLastError(0);
    ret = pCompareObjectHandles( h1, h2 );
    ok( !ret && GetLastError() == ERROR_NOT_SAME_OBJECT,
        "comparing %p with %p returned %lu\n", h1, h2, GetLastError() );

    CloseHandle( h2 );
    CloseHandle( h1 );
}

static void test_MapViewOfFile3(void)
{
    static const char testfile[] = "testfile.xxx";
    HANDLE file, mapping;
    void *ptr;
    BOOL ret;
    SYSTEM_INFO system_info;
    size_t file_size = 1024 * 1024;
    void *allocation;
    void *map_start, *map_end, *map_start_offset;
    void *view;

    if (!pMapViewOfFile3)
    {
        win_skip("MapViewOfFile3() is not supported.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    SetFilePointer( file, 12288, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = pMapViewOfFile3( mapping, NULL, NULL, 0, 4096, 0, PAGE_READONLY, NULL, 0);
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    CloseHandle( mapping );
    CloseHandle( file );
    ret = DeleteFileA( testfile );
    ok(ret, "Failed to delete a test file.\n");

    /* Tests for using MapViewOfFile3 together with MEM_RESERVE_PLACEHOLDER/MEM_REPLACE_PLACEHOLDER */
    /* like self pe-loading programs do (e.g. .net pe-loader). */
    /* With MEM_REPLACE_PLACEHOLDER, MapViewOfFile3/NtMapViewOfSection(Ex) shall relax alignment from 64k to pagesize */
    GetSystemInfo(&system_info);
    mapping = CreateFileMappingA(NULL, NULL, PAGE_READWRITE, 0, (DWORD)file_size, NULL);
    ok(mapping != NULL, "CreateFileMapping did not return a handle %lu\n", GetLastError());

    allocation = pVirtualAlloc2(GetCurrentProcess(), NULL, file_size,
                                MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    ok(allocation != NULL, "VirtualAlloc2 returned NULL %lu\n", GetLastError());

    map_start = (void*)((ULONG_PTR)allocation + system_info.dwPageSize);
    map_end = (void*)((ULONG_PTR)map_start + system_info.dwPageSize);
    ret = VirtualFree(map_start, system_info.dwPageSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "VirtualFree failed to split the placeholder %lu\n", GetLastError());

    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_start, system_info.dwPageSize,
                           system_info.dwPageSize, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    ok(view != NULL, "MapViewOfFile3 did not map the file mapping %lu\n", GetLastError());

    ret = UnmapViewOfFile(view);
    ok(ret, "UnmapViewOfFile failed %lu\n", GetLastError());

    map_start_offset = (void*)((ULONG_PTR)map_start - 1);
    SetLastError(0xdeadbeef);
    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_start_offset, system_info.dwPageSize,
                           system_info.dwPageSize, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    ok(view == NULL, "MapViewOfFile3 did map the file mapping, though baseaddr was not pagesize aligned\n");
    ok(GetLastError() == ERROR_MAPPED_ALIGNMENT, "MapViewOfFile3 did not return ERROR_MAPPED_ALIGNMENT(%u), instead it returned %lu\n",
       ERROR_MAPPED_ALIGNMENT, GetLastError());

    SetLastError(0xdeadbeef);
    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_start, system_info.dwPageSize-1,
                           system_info.dwPageSize, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    ok(view == NULL, "MapViewOfFile3 did map the file mapping, though offset was not pagesize aligned\n");
    ok(GetLastError() == ERROR_MAPPED_ALIGNMENT,
       "MapViewOfFile3 did not return ERROR_MAPPED_ALIGNMENT(%u), instead it returned %lu\n",
       ERROR_MAPPED_ALIGNMENT, GetLastError());

    ret = VirtualFree(allocation, 0, MEM_RELEASE);
    ok(ret, "VirtualFree of first remaining region failed: %lu\n", GetLastError());

    ret = VirtualFree(map_end, 0, MEM_RELEASE);
    ok(ret, "VirtualFree of remaining region failed: %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_end, system_info.dwPageSize, 0, 0, PAGE_READWRITE, NULL, 0);
    ok(view == NULL, "MapViewOfFile3 did map the file mapping, though baseaddr was not 64k aligned\n");
    ok(GetLastError() == ERROR_MAPPED_ALIGNMENT,
       "MapViewOfFile3 did not return ERROR_MAPPED_ALIGNMENT(%u), instead it returned %lu\n",
       ERROR_MAPPED_ALIGNMENT, GetLastError());

    SetLastError(0xdeadbeef);
    map_start = (void*)(((ULONG_PTR)allocation + system_info.dwAllocationGranularity) & ~((ULONG_PTR)system_info.dwAllocationGranularity-1));
    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_end,
                           system_info.dwPageSize, system_info.dwPageSize, 0, PAGE_READWRITE, NULL, 0);
    ok(view == NULL, "MapViewOfFile3 did map the file mapping, though offset was not 64k aligned\n");
    ok(GetLastError() == ERROR_MAPPED_ALIGNMENT,
       "MapViewOfFile3 did not return ERROR_MAPPED_ALIGNMENT(%u), instead it returned %lu\n",
       ERROR_MAPPED_ALIGNMENT, GetLastError());

    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_start, 0,
                           system_info.dwPageSize, 0, PAGE_READWRITE, NULL, 0);
    ok(view != NULL, "MapViewOfFile3 failed though both baseaddr and offset were 64k aligned %lu\n", GetLastError());

    ret = UnmapViewOfFile(view);
    ok(ret, "UnmapViewOfFile failed %lu\n", GetLastError());

    view = pMapViewOfFile3(mapping, GetCurrentProcess(), map_start,
                           4*system_info.dwAllocationGranularity, system_info.dwPageSize, 0, PAGE_READWRITE, NULL, 0);
    ok(view != NULL, "MapViewOfFile3 failed though both baseaddr and offset were 64k aligned %lu\n", GetLastError());

    ret = UnmapViewOfFile(view);
    ok(ret, "UnmapViewOfFile failed %lu\n", GetLastError());

    ok(CloseHandle(mapping), "CloseHandle failed on mapping\n");
}

#define check_region_size(p, s) check_region_size_(p, s, __LINE__)
static void check_region_size_(void *p, SIZE_T s, unsigned int line)
{
    MEMORY_BASIC_INFORMATION info;
    SIZE_T ret;

    memset(&info, 0, sizeof(info));
    ret = VirtualQuery(p, &info, sizeof(info));
    ok_(__FILE__,line)(ret == sizeof(info), "Unexpected return value.\n");
    ok_(__FILE__,line)(info.RegionSize == s, "Unexpected size %Iu, expected %Iu.\n", info.RegionSize, s);
}

static void test_VirtualAlloc2(void)
{
    void *placeholder1, *placeholder2, *view1, *view2, *addr;
    MEMORY_BASIC_INFORMATION info;
    char *p, *p1, *p2;
    HANDLE section;
    SIZE_T size;
    BOOL ret;

    if (!pVirtualAlloc2)
    {
        win_skip("VirtualAlloc2() is not supported.\n");
        return;
    }

    size = 0x80000;
    addr = pVirtualAlloc2(NULL, NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE, NULL, 0);
    ok(!!addr, "Failed to allocate, error %lu.\n", GetLastError());
    ret = VirtualFree(addr, 0, MEM_RELEASE);
    ok(ret, "Unexpected return value %d, error %lu.\n", ret, GetLastError());

    placeholder1 = pVirtualAlloc2(NULL, NULL, 2 * size, MEM_RESERVE, PAGE_NOACCESS, NULL, 0);
    ok(!!placeholder1, "Failed to create a placeholder range.\n");
    ret = VirtualFree(placeholder1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(!ret && GetLastError() == ERROR_INVALID_ADDRESS, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = VirtualFree(placeholder1, 0, MEM_RELEASE);
    ok(ret, "Unexpected return value %d, error %lu.\n", ret, GetLastError());

    /* Placeholder splitting functionality */
    placeholder1 = pVirtualAlloc2(NULL, NULL, 2 * size, MEM_RESERVE_PLACEHOLDER | MEM_RESERVE, PAGE_NOACCESS, NULL, 0);
    ok(!!placeholder1, "Failed to create a placeholder range.\n");

    memset(&info, 0, sizeof(info));
    VirtualQuery(placeholder1, &info, sizeof(info));
    ok(info.AllocationProtect == PAGE_NOACCESS, "Unexpected protection %#lx.\n", info.AllocationProtect);
    ok(info.State == MEM_RESERVE, "Unexpected state %#lx.\n", info.State);
    ok(info.Type == MEM_PRIVATE, "Unexpected type %#lx.\n", info.Type);
    ok(info.RegionSize == 2 * size, "Unexpected size.\n");

    ret = VirtualFree(placeholder1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "Failed to split placeholder.\n");

    memset(&info, 0, sizeof(info));
    VirtualQuery(placeholder1, &info, sizeof(info));
    ok(info.AllocationProtect == PAGE_NOACCESS, "Unexpected protection %#lx.\n", info.AllocationProtect);
    ok(info.State == MEM_RESERVE, "Unexpected state %#lx.\n", info.State);
    ok(info.Type == MEM_PRIVATE, "Unexpected type %#lx.\n", info.Type);
    ok(info.RegionSize == size, "Unexpected size.\n");

    placeholder2 = (void *)((BYTE *)placeholder1 + size);
    memset(&info, 0, sizeof(info));
    VirtualQuery(placeholder2, &info, sizeof(info));
    ok(info.AllocationProtect == PAGE_NOACCESS, "Unexpected protection %#lx.\n", info.AllocationProtect);
    ok(info.State == MEM_RESERVE, "Unexpected state %#lx.\n", info.State);
    ok(info.Type == MEM_PRIVATE, "Unexpected type %#lx.\n", info.Type);
    ok(info.RegionSize == size, "Unexpected size.\n");

    section = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, NULL);
    ok(!!section, "Failed to create a section.\n");

    view1 = pMapViewOfFile3(section, NULL, NULL, 0, size, 0, PAGE_READWRITE, NULL, 0);
    ok(!!view1, "Failed to map a section.\n");
    ret = VirtualFree( view1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER );
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = pUnmapViewOfFile2(GetCurrentProcess(), view1, MEM_PRESERVE_PLACEHOLDER);
    ok(!ret && GetLastError() == ERROR_INVALID_ADDRESS, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = pUnmapViewOfFile2(GetCurrentProcess(), view1, 0);
    ok(ret, "Got error %lu.\n", GetLastError());

    view1 = pMapViewOfFile3(section, NULL, placeholder1, 0, size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    ok(view1 == placeholder1, "Address does not match.\n");

    view2 = pMapViewOfFile3(section, NULL, placeholder2, 0, size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    ok(view2 == placeholder2, "Address does not match.\n");

    memset(&info, 0, sizeof(info));
    VirtualQuery(placeholder1, &info, sizeof(info));
    ok(info.AllocationProtect == PAGE_READWRITE, "Unexpected protection %#lx.\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "Unexpected state %#lx.\n", info.State);
    ok(info.Type == MEM_MAPPED, "Unexpected type %#lx.\n", info.Type);
    ok(info.RegionSize == size, "Unexpected size.\n");

    memset(&info, 0, sizeof(info));
    VirtualQuery(placeholder2, &info, sizeof(info));
    ok(info.AllocationProtect == PAGE_READWRITE, "Unexpected protection %#lx.\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "Unexpected state %#lx.\n", info.State);
    ok(info.Type == MEM_MAPPED, "Unexpected type %#lx.\n", info.Type);
    ok(info.RegionSize == size, "Unexpected size.\n");

    ret = pUnmapViewOfFile2(NULL, view1, MEM_PRESERVE_PLACEHOLDER);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Got error %lu.\n", GetLastError());

    ret = VirtualFree( placeholder1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER );
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got ret %d, error %lu.\n", ret, GetLastError());

    ret = pUnmapViewOfFile2(GetCurrentProcess(), view1, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = pUnmapViewOfFile2(GetCurrentProcess(), view1, MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "Got error %lu.\n", GetLastError());

    memset(&info, 0, sizeof(info));
    VirtualQuery(placeholder1, &info, sizeof(info));
    ok(info.AllocationProtect == PAGE_NOACCESS, "Unexpected protection %#lx.\n", info.AllocationProtect);
    ok(info.State == MEM_RESERVE, "Unexpected state %#lx.\n", info.State);
    ok(info.Type == MEM_PRIVATE, "Unexpected type %#lx.\n", info.Type);
    ok(info.RegionSize == size, "Unexpected size.\n");

    ret = pUnmapViewOfFile2(GetCurrentProcess(), view1, MEM_PRESERVE_PLACEHOLDER);
    ok(!ret && GetLastError() == ERROR_INVALID_ADDRESS, "Got error %lu.\n", GetLastError());

    ret = UnmapViewOfFile(view1);
    ok(!ret && GetLastError() == ERROR_INVALID_ADDRESS, "Got error %lu.\n", GetLastError());

    view1 = pMapViewOfFile3(section, NULL, placeholder1, 0, size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    ok(view1 == placeholder1, "Address does not match.\n");
    CloseHandle(section);

    ret = VirtualFree( view1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER );
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got ret %d, error %lu.\n", ret, GetLastError());

    ret = pUnmapViewOfFile2(GetCurrentProcess(), view1, MEM_UNMAP_WITH_TRANSIENT_BOOST | MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "Got error %lu.\n", GetLastError());

    ret = VirtualFree( placeholder1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER );
    ok(!ret && GetLastError() == ERROR_INVALID_ADDRESS, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = VirtualFreeEx(GetCurrentProcess(), placeholder1, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER );
    ok(!ret && GetLastError() == ERROR_INVALID_ADDRESS, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = VirtualFree(placeholder1, 0, MEM_RELEASE);
    ok(ret, "Got error %lu.\n", GetLastError());

    UnmapViewOfFile(view2);

    VirtualFree(placeholder1, 0, MEM_RELEASE);
    VirtualFree(placeholder2, 0, MEM_RELEASE);

    /* Split in three regions. */
    p = pVirtualAlloc2(NULL, NULL, 2 * size, MEM_RESERVE_PLACEHOLDER | MEM_RESERVE, PAGE_NOACCESS, NULL, 0);
    ok(!!p, "Failed to create a placeholder range.\n");

    p1 = p + size / 2;
    p2 = p1 + size / 4;
    ret = VirtualFree(p1, size / 4, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "Failed to split a placeholder.\n");
    check_region_size(p, size / 2);
    check_region_size(p1, size / 4);
    check_region_size(p2, 2 * size - size / 2 - size / 4);
    ret = VirtualFree(p, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");
    ret = VirtualFree(p1, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");
    ret = VirtualFree(p2, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");

    /* Split in two regions, specifying lower part. */
    p = pVirtualAlloc2(NULL, NULL, 2 * size, MEM_RESERVE_PLACEHOLDER | MEM_RESERVE, PAGE_NOACCESS, NULL, 0);
    ok(!!p, "Failed to create a placeholder range.\n");

    p1 = p;
    p2 = p + size / 2;
    ret = VirtualFree(p1, 0, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got ret %d, error %lu.\n", ret, GetLastError());
    ret = VirtualFree(p1, size / 2, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "Failed to split a placeholder.\n");
    check_region_size(p1, size / 2);
    check_region_size(p2, 2 * size - size / 2);
    ret = VirtualFree(p1, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");
    ret = VirtualFree(p2, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");

    /* Split in two regions, specifying second half. */
    p = pVirtualAlloc2(NULL, NULL, 2 * size, MEM_RESERVE_PLACEHOLDER | MEM_RESERVE, PAGE_NOACCESS, NULL, 0);
    ok(!!p, "Failed to create a placeholder range.\n");

    p1 = p;
    p2 = p + size;
    ret = VirtualFree(p2, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    ok(ret, "Failed to split a placeholder.\n");
    check_region_size(p1, size);
    check_region_size(p2, size);
    ret = VirtualFree(p1, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");
    ret = VirtualFree(p2, 0, MEM_RELEASE);
    ok(ret, "Failed to release a region.\n");
}

static void test_VirtualAllocFromApp(void)
{
    static const DWORD prot[] =
    {
        PAGE_EXECUTE,
        PAGE_EXECUTE_READ,
        PAGE_EXECUTE_READWRITE,
        PAGE_EXECUTE_WRITECOPY,
    };
    unsigned int i;
    BOOL ret;
    void *p;

    if (!pVirtualAllocFromApp)
    {
        win_skip("VirtualAllocFromApp is not available.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    p = pVirtualAllocFromApp(NULL, 0x1000, MEM_RESERVE, PAGE_READWRITE);
    ok(p && GetLastError() == 0xdeadbeef, "Got unexpected mem %p, GetLastError() %lu.\n", p, GetLastError());
    ret = VirtualFree(p, 0, MEM_RELEASE);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    for (i = 0; i < ARRAY_SIZE(prot); ++i)
    {
        SetLastError(0xdeadbeef);
        p = pVirtualAllocFromApp(NULL, 0x1000, MEM_RESERVE, prot[i]);
        ok(!p && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected mem %p, GetLastError() %lu.\n",
                p, GetLastError());
    }
}

static void test_VirtualAlloc2FromApp(void)
{
    static const DWORD prot[] =
    {
        PAGE_EXECUTE,
        PAGE_EXECUTE_READ,
        PAGE_EXECUTE_READWRITE,
        PAGE_EXECUTE_WRITECOPY,
    };
    unsigned int i;
    void *addr;
    BOOL ret;

    if (!pVirtualAlloc2FromApp)
    {
        win_skip("VirtualAlloc2FromApp is not available.\n");
        return;
    }

    addr = pVirtualAlloc2FromApp(NULL, NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE, NULL, 0);
    ok(!!addr, "Failed to allocate, error %lu.\n", GetLastError());
    ret = VirtualFree(addr, 0, MEM_RELEASE);
    ok(ret, "Unexpected return value %d, error %lu.\n", ret, GetLastError());

    for (i = 0; i < ARRAY_SIZE(prot); ++i)
    {
        SetLastError(0xdeadbeef);
        addr = pVirtualAlloc2FromApp(NULL, NULL, 0x1000, MEM_COMMIT, prot[i], NULL, 0);
        ok(!addr && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected mem %p, GetLastError() %lu.\n",
                addr, GetLastError());
    }
}

static void test_VirtualProtectFromApp(void)
{
    ULONG old_prot;
    void *p;
    BOOL ret;

    if (!pVirtualProtectFromApp)
    {
        win_skip("VirtualProtectFromApp is not available.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    p = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
    ok(p != NULL, "VirtualAlloc failed err %lu.\n", GetLastError());
    ret = pVirtualProtectFromApp(p, 0x1000, PAGE_READONLY, &old_prot);
    ok(ret, "Failed to change protection err %lu\n", GetLastError());
    ok(old_prot == PAGE_READWRITE, "wrong old_prot %lu\n", old_prot);

    ret = pVirtualProtectFromApp(p, 0x100000, PAGE_READWRITE, &old_prot);
    ok(!ret, "Call worked with overflowing size\n");
    ok(old_prot == PAGE_NOACCESS, "wrong old_prot %lu\n", old_prot);

    ret = pVirtualProtectFromApp(p, 0x1000, PAGE_EXECUTE_READ, NULL);
    ok(!ret, "Call worked without old_prot\n");

    ret = pVirtualProtectFromApp(p, 0x1000, PAGE_GUARD, &old_prot);
    ok(!ret, "Call worked with an invalid new_prot parameter old_prot %lu\n", old_prot);

    /* Works on desktop, but not on UWP */
    ret = pVirtualProtectFromApp(p, 0x1000, PAGE_EXECUTE_READWRITE, &old_prot);
    ok(ret || broken(GetLastError() == ERROR_INVALID_PARAMETER) /* Win10-1507 */, "Failed err %lu\n", GetLastError());
    if (ret) ok(old_prot == PAGE_READONLY, "wrong old_prot %lu\n", old_prot);

    ret = VirtualFree(p, 0, MEM_RELEASE);
    ok(ret, "Failed to free mem error %lu.\n", GetLastError());
}

static void test_OpenFileMappingFromApp(void)
{
    OBJECT_BASIC_INFORMATION info;
    HANDLE file, mapping;
    NTSTATUS status;
    ULONG length;

    if (!pOpenFileMappingFromApp)
    {
        win_skip("OpenFileMappingFromApp is not available.\n");
        return;
    }

    file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READ, 0, 4090, "foo");
    ok(!!file, "Failed to create a mapping.\n");

    mapping = pOpenFileMappingFromApp(FILE_MAP_READ, FALSE, L"foo");
    ok(!!mapping, "Failed to open a mapping.\n");
    status = pNtQueryObject(mapping, ObjectBasicInformation, &info, sizeof(info), &length);
    ok(!status, "Failed to get object information.\n");
    ok(info.GrantedAccess == SECTION_MAP_READ, "Unexpected access mask %#lx.\n", info.GrantedAccess);
    CloseHandle(mapping);

    mapping = pOpenFileMappingFromApp(FILE_MAP_EXECUTE, FALSE, L"foo");
    ok(!!mapping, "Failed to open a mapping.\n");
    status = pNtQueryObject(mapping, ObjectBasicInformation, &info, sizeof(info), &length);
    ok(!status, "Failed to get object information.\n");
    todo_wine
    ok(info.GrantedAccess == SECTION_MAP_EXECUTE, "Unexpected access mask %#lx.\n", info.GrantedAccess);
    CloseHandle(mapping);

    CloseHandle(file);
}

static void test_CreateFileMappingFromApp(void)
{
    OBJECT_BASIC_INFORMATION info;
    NTSTATUS status;
    ULONG length;
    HANDLE file;

    if (!pCreateFileMappingFromApp)
    {
        win_skip("CreateFileMappingFromApp is not available.\n");
        return;
    }

    file = pCreateFileMappingFromApp(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 1024, L"foo");
    ok(!!file || broken(!file) /* Win8 */, "Failed to create a mapping, error %lu.\n", GetLastError());
    if (!file) return;

    status = pNtQueryObject(file, ObjectBasicInformation, &info, sizeof(info), &length);
    ok(!status, "Failed to get object information.\n");
    ok(info.GrantedAccess & SECTION_MAP_EXECUTE, "Unexpected access mask %#lx.\n", info.GrantedAccess);

    CloseHandle(file);
}

static void test_MapViewOfFileFromApp(void)
{
    static const char testfile[] = "testfile.xxx";
    HANDLE file, mapping;
    void *ptr;
    BOOL ret;

    if (!pMapViewOfFileFromApp)
    {
        win_skip("MapViewOfFileFromApp() is not supported.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to create a file, error %lu.\n", GetLastError() );
    SetFilePointer( file, 12288, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "Failed to create file mapping, error %lu.\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = pMapViewOfFileFromApp( mapping, PAGE_EXECUTE_READWRITE, 0, 4096 );
    ok( ptr != NULL, "Mapping failed, error %lu.\n", GetLastError() );
    UnmapViewOfFile( ptr );

    CloseHandle( mapping );
    CloseHandle( file );
    ret = DeleteFileA( testfile );
    ok(ret, "Failed to delete a test file.\n");
}

static void test_QueryProcessCycleTime(void)
{
    ULONG64 cycles1, cycles2;
    BOOL ret;

    ret = QueryProcessCycleTime( GetCurrentProcess(), &cycles1 );
    ok( ret, "QueryProcessCycleTime failed, error %lu.\n", GetLastError() );

    ret = QueryProcessCycleTime( GetCurrentProcess(), &cycles2 );
    ok( ret, "QueryProcessCycleTime failed, error %lu.\n", GetLastError() );

    todo_wine
    ok( cycles2 > cycles1, "CPU cycles used by process should be increasing.\n" );
}

static void init_funcs(void)
{
    HMODULE hmod = GetModuleHandleA("kernelbase.dll");

#define X(f) { p##f = (void*)GetProcAddress(hmod, #f); }
    X(CompareObjectHandles);
    X(CreateFileMappingFromApp);
    X(MapViewOfFile3);
    X(MapViewOfFileFromApp);
    X(OpenFileMappingFromApp);
    X(VirtualAlloc2);
    X(VirtualAlloc2FromApp);
    X(VirtualAllocFromApp);
    X(VirtualProtectFromApp);
    X(UnmapViewOfFile2);

    hmod = GetModuleHandleA("ntdll.dll");

    X(NtQueryObject);
#undef X
}

START_TEST(process)
{
    init_funcs();

    test_CompareObjectHandles();
    test_MapViewOfFile3();
    test_VirtualAlloc2();
    test_VirtualAllocFromApp();
    test_VirtualAlloc2FromApp();
    test_VirtualProtectFromApp();
    test_OpenFileMappingFromApp();
    test_CreateFileMappingFromApp();
    test_MapViewOfFileFromApp();
    test_QueryProcessCycleTime();
}
