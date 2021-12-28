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

static BOOL (WINAPI *pCompareObjectHandles)(HANDLE, HANDLE);

static void test_CompareObjectHandles(void)
{
    HANDLE h1, h2;
    BOOL ret;

    if (!pCompareObjectHandles)
    {
        skip("CompareObjectHandles is not available.\n");
        return;
    }

    ret = pCompareObjectHandles( GetCurrentProcess(), GetCurrentProcess() );
    ok( ret, "comparing GetCurrentProcess() to self failed with %u\n", GetLastError() );

    ret = pCompareObjectHandles( GetCurrentThread(), GetCurrentThread() );
    ok( ret, "comparing GetCurrentThread() to self failed with %u\n", GetLastError() );

    SetLastError(0);
    ret = pCompareObjectHandles( GetCurrentProcess(), GetCurrentThread() );
    ok( !ret && GetLastError() == ERROR_NOT_SAME_OBJECT,
        "comparing GetCurrentProcess() to GetCurrentThread() returned %u\n", GetLastError() );

    h1 = NULL;
    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                           &h1, 0, FALSE, DUPLICATE_SAME_ACCESS );
    ok( ret, "failed to duplicate current process handle: %u\n", GetLastError() );

    ret = pCompareObjectHandles( GetCurrentProcess(), h1 );
    ok( ret, "comparing GetCurrentProcess() with %p failed with %u\n", h1, GetLastError() );

    CloseHandle( h1 );

    h1 = CreateFileA( "\\\\.\\NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h1 != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError() );

    h2 = NULL;
    ret = DuplicateHandle( GetCurrentProcess(), h1, GetCurrentProcess(),
                           &h2, 0, FALSE, DUPLICATE_SAME_ACCESS );
    ok( ret, "failed to duplicate handle %p: %u\n", h1, GetLastError() );

    ret = pCompareObjectHandles( h1, h2 );
    ok( ret, "comparing %p with %p failed with %u\n", h1, h2, GetLastError() );

    CloseHandle( h2 );

    h2 = CreateFileA( "\\\\.\\NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h2 != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError() );

    SetLastError(0);
    ret = pCompareObjectHandles( h1, h2 );
    ok( !ret && GetLastError() == ERROR_NOT_SAME_OBJECT,
        "comparing %p with %p returned %u\n", h1, h2, GetLastError() );

    CloseHandle( h2 );
    CloseHandle( h1 );
}

START_TEST(process)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("kernel32.dll");
    pCompareObjectHandles = (void *)GetProcAddress(hmod, "CompareObjectHandles");
    ok(!pCompareObjectHandles, "expected CompareObjectHandles only in kernelbase.dll\n");

    hmod = GetModuleHandleA("kernelbase.dll");
    pCompareObjectHandles = (void *)GetProcAddress(hmod, "CompareObjectHandles");

    test_CompareObjectHandles();
}
