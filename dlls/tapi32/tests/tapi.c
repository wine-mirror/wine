/*
 * TAPI32 API tests
 *
 * Copyright 2019 Vijay Kiran Kamuju
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

#include "tapi.h"

#include "wine/test.h"

static void CALLBACK line_callback(DWORD hDev, DWORD dwMsg, DWORD_PTR dwInst, DWORD_PTR param1, DWORD_PTR param2, DWORD_PTR param3)
{
}

static void test_lineInitialize(void)
{
    ULONG ret;
    DWORD dev;
    HLINEAPP lnApp;

    ret = lineInitialize(NULL, NULL, NULL, NULL, NULL);
    todo_wine ok(ret == LINEERR_INVALPOINTER, "Expected return value LINEERR_INVALPOINTER, got %lx.\n", ret);

    ret = lineInitialize(&lnApp, NULL, NULL, NULL, NULL);
    todo_wine ok(ret == LINEERR_INVALPOINTER, "Expected return value LINEERR_INVALPOINTER, got %lx.\n", ret);

    ret = lineInitialize(&lnApp, GetModuleHandleA(NULL), NULL, NULL, NULL);
    todo_wine ok(ret == LINEERR_INVALPOINTER, "Expected return value LINEERR_INVALPOINTER, got %lx.\n", ret);

    ret = lineInitialize(&lnApp, GetModuleHandleA(NULL), line_callback, NULL, NULL);
    todo_wine ok(ret == LINEERR_INVALPOINTER, "Expected return value LINEERR_INVALPOINTER, got %lx.\n", ret);

    ret = lineInitialize(&lnApp, GetModuleHandleA(NULL), line_callback, NULL, &dev);
    ok(!ret, "unexpected return value, got %lu.\n", ret);

    ret = lineShutdown(NULL);
    todo_wine ok(ret == LINEERR_INVALAPPHANDLE, "Expected return value LINEERR_INVALAPPHANDLE, got %lx.\n", ret);

    ret = lineShutdown(lnApp);
    ok(!ret, "unexpected return value, got %lu.\n", ret);
}

START_TEST(tapi)
{
    test_lineInitialize();
}
