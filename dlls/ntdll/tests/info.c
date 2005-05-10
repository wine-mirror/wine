/* Unit test suite for *Information* Registry API functions
 *
 * Copyright 2005 Paul Vriens
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
 *
 */

#include "ntdll_test.h"

static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

static HMODULE hntdll = 0;

#define NTDLL_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hntdll, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(hntdll); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    if(!hntdll) {
      trace("Could not load ntdll.dll\n");
      return FALSE;
    }
    if (hntdll)
    {
      NTDLL_GET_PROC(NtQuerySystemInformation)
    }
    return TRUE;
}

static void test_query_basic()
{
    DWORD status;
    ULONG ReturnLength;
    SYSTEM_BASIC_INFORMATION sbi;

    /* This test also covers some basic parameter testing that should be the same for 
     * every information class
    */

    /* Use a nonexistent info class */
    trace("Check nonexistent info class\n");
    status = pNtQuerySystemInformation(-1, NULL, 0, NULL);
    ok( status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got %08lx\n", status);

    /* Use an existing class but with a zero-length buffer */
    trace("Check zero-length buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, 0, NULL);
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);

    /* Use an existing class, correct length but no SystemInformation buffer */
    trace("Check no SystemInformation buffer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, NULL, sizeof(sbi), NULL);
    ok( status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

    /* Use a existing class, correct length, a pointer to a buffer but no ReturnLength pointer */
    trace("Check no ReturnLength pointer\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), NULL);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Finally some correct calls */
    trace("Check with correct parameters\n");
    status = pNtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok( sizeof(sbi) == ReturnLength, "Inconsistent length (%08x) <-> (%ld)\n", sizeof(sbi), ReturnLength);

    /* Check if we have some return values */
    trace("Number of Processors : %d\n", sbi.NumberOfProcessors);
    ok( sbi.NumberOfProcessors > 0, "Expected more than 0 processors, got %d\n", sbi.NumberOfProcessors);
}

static void test_query_handle()
{
    DWORD status;
    ULONG ReturnLength;
    ULONG SystemInformationLength = sizeof(SYSTEM_HANDLE_INFORMATION);
    SYSTEM_HANDLE_INFORMATION* shi = HeapAlloc(GetProcessHeap(), 0, SystemInformationLength);

    /* Request the needed length : a SystemInformationLength greater than one struct sets ReturnLength */
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);

    /* The following check assumes more than one handle on any given system */
    todo_wine
    {
        ok( status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    }
    ok( ReturnLength > 0, "Expected ReturnLength to be > 0, it was %ld\n", ReturnLength);

    SystemInformationLength = ReturnLength;
    shi = HeapReAlloc(GetProcessHeap(), 0, shi , SystemInformationLength);
    status = pNtQuerySystemInformation(SystemHandleInformation, shi, SystemInformationLength, &ReturnLength);
    ok( status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    /* Check if we have some return values */
    trace("Number of Handles : %ld\n", shi->Count);
    todo_wine
    {
        /* our implementation is a stub for now */
        ok( shi->Count > 1, "Expected more than 1 handles, got (%ld)\n", shi->Count);
    }

    HeapFree( GetProcessHeap(), 0, shi);
}


START_TEST(info)
{
    if(!InitFunctionPtrs())
        return;

    /* 0 SystemBasicInformation */
    trace("Starting test_query_basic()\n");
    test_query_basic();

    /* 0x10 SystemHandleInformation */
    trace("Starting test_query_handle()\n");
    test_query_handle();

    FreeLibrary(hntdll);
}
