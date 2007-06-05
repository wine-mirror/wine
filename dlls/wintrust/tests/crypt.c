/* Unit test suite for wintrust crypt functions
 *
 * Copyright 2007 Paul Vriens
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windows.h"
#include "mscat.h"

#include "wine/test.h"

static HMODULE hWintrust = 0;

static BOOL (WINAPI * pCryptCATAdminAcquireContext)(HCATADMIN*, const GUID*, DWORD);
static BOOL (WINAPI * pCryptCATAdminReleaseContext)(HCATADMIN, DWORD);

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

static BOOL InitFunctionPtrs(void)
{
    hWintrust = LoadLibraryA("wintrust.dll");

    if(!hWintrust)
    {
        skip("Could not load wintrust.dll\n");
        return FALSE;
    }

    WINTRUST_GET_PROC(CryptCATAdminAcquireContext)
    WINTRUST_GET_PROC(CryptCATAdminReleaseContext)

    return TRUE;
}

static void test_context(void)
{
    BOOL ret;
    HCATADMIN hca;
    static GUID dummy   = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    static GUID unknown = { 0xC689AABA, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }}; /* WINTRUST.DLL */

    if (!pCryptCATAdminAcquireContext || !pCryptCATAdminReleaseContext)
    {
        skip("CryptCATAdminAcquireContext and/or CryptCATAdminReleaseContext are not available\n");
        return;
    }

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* NULL GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, NULL, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(NULL, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* Proper release */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected no change in last error, got %d\n", GetLastError());

    /* Try to release a second time */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminReleaseContext(hca, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* NULL context handle and dummy GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(NULL, &dummy, 0);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    /* Correct context handle and dummy GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, &dummy, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");

    /* Correct context handle and GUID */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 0);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");

    /* Flags not equal to 0 */
    SetLastError(0xdeadbeef);
    ret = pCryptCATAdminAcquireContext(&hca, &unknown, 1);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       GetLastError() == 0xdeadbeef /* Vista */,
       "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
    ok(hca != NULL, "Expected a context handle, got NULL\n");

    ret = pCryptCATAdminReleaseContext(hca, 0);
    ok(ret, "Expected success\n");
}

START_TEST(crypt)
{
    if(!InitFunctionPtrs())
        return;

    test_context();

    FreeLibrary(hWintrust);
}
