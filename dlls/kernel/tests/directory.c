/*
 * Unit test suite for directory functions.
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


/* If you change something in these tests, please do the same
 * for GetSystemDirectory tests.
 */
static void test_GetWindowsDirectoryA(void)
{
    UINT len, len_with_null;
    char buf[MAX_PATH];

    lstrcpyA(buf, "foo");
    len_with_null = GetWindowsDirectoryA(buf, 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH");

    lstrcpyA(buf, "foo");
    len = GetWindowsDirectoryA(buf, len_with_null - 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");
    ok(len == len_with_null, "should return length with terminating 0");

    lstrcpyA(buf, "foo");
    len = GetWindowsDirectoryA(buf, len_with_null);
    ok(lstrcmpA(buf, "foo") != 0, "should touch the buffer");
    ok(len == lstrlenA(buf), "returned length should be equal to the length of string");
    ok(len == (len_with_null - 1), "should return length without terminating 0");
}

static void test_GetWindowsDirectoryW(void)
{
    UINT len, len_with_null;
    WCHAR buf[MAX_PATH];
    static const WCHAR fooW[] = {'f','o','o',0};

    lstrcpyW(buf, fooW);
    len_with_null = GetWindowsDirectoryW(buf, 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH");

    lstrcpyW(buf, fooW);
    len = GetWindowsDirectoryW(buf, len_with_null - 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");
    ok(len == len_with_null, "should return length with terminating 0");

    lstrcpyW(buf, fooW);
    len = GetWindowsDirectoryW(buf, len_with_null);
    ok(lstrcmpW(buf, fooW) != 0, "should touch the buffer");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string");
    ok(len == (len_with_null - 1), "should return length without terminating 0");
}


/* If you change something in these tests, please do the same
 * for GetWindowsDirectory tests.
 */
static void test_GetSystemDirectoryA(void)
{
    UINT len, len_with_null;
    char buf[MAX_PATH];

    lstrcpyA(buf, "foo");
    len_with_null = GetSystemDirectoryA(buf, 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH");

    lstrcpyA(buf, "foo");
    len = GetSystemDirectoryA(buf, len_with_null - 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");
    ok(len == len_with_null, "should return length with terminating 0");

    lstrcpyA(buf, "foo");
    len = GetSystemDirectoryA(buf, len_with_null);
    ok(lstrcmpA(buf, "foo") != 0, "should touch the buffer");
    ok(len == lstrlenA(buf), "returned length should be equal to the length of string");
    ok(len == (len_with_null - 1), "should return length without terminating 0");
}

static void test_GetSystemDirectoryW(void)
{
    UINT len, len_with_null;
    WCHAR buf[MAX_PATH];
    static const WCHAR fooW[] = {'f','o','o',0};

    lstrcpyW(buf, fooW);
    len_with_null = GetSystemDirectoryW(buf, 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");
    ok(len_with_null <= MAX_PATH, "should fit into MAX_PATH");

    lstrcpyW(buf, fooW);
    len = GetSystemDirectoryW(buf, len_with_null - 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");
    ok(len == len_with_null, "should return length with terminating 0");

    lstrcpyW(buf, fooW);
    len = GetSystemDirectoryW(buf, len_with_null);
    ok(lstrcmpW(buf, fooW) != 0, "should touch the buffer");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string");
    ok(len == (len_with_null - 1), "should return length without terminating 0");
}

START_TEST(directory)
{
    test_GetWindowsDirectoryA();
    test_GetWindowsDirectoryW();
    test_GetSystemDirectoryA();
    test_GetSystemDirectoryW();
}
