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
           "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
           GetLastError());
    else
        ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA(NULL, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA((void *)0xdeadbeef, 0, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA((void *)0xdeadbeef, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
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
           "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
           GetLastError());
    else
        ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW(NULL, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW((void *)0xdeadbeef, 0, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW((void *)0xdeadbeef, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());
}

START_TEST(diskspace)
{
    test_SetupCreateDiskSpaceListA();
    test_SetupCreateDiskSpaceListW();
}
