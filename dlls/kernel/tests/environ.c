/*
 * Unit test suite for environment functions.
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

static void test_GetSetEnvironmentVariableA(void)
{
    char buf[256];
    BOOL ret;
    DWORD ret_size;
    static const char name[] = "SomeWildName";
    static const char name_cased[] = "sOMEwILDnAME";
    static const char value[] = "SomeWildValue";

    ret = SetEnvironmentVariableA(NULL, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_PARAMETER, "should fail with NULL, NULL");

    ret = SetEnvironmentVariableA("", "");
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_PARAMETER, "should fail with \"\", \"\"");

    ret = SetEnvironmentVariableA(name, "");
    ok(ret == TRUE, "should not fail with empty value");

    ret = SetEnvironmentVariableA("", value);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_PARAMETER, "should fail with empty name");

    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE, "unexpected error in SetEnvironmentVariableA");

    /* the following line should just crash */
    /* ret_size = GetEnvironmentVariableA(name, NULL, lstrlenA(value) + 1); */

    ret_size = GetEnvironmentVariableA(NULL, NULL, 0);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    ret_size = GetEnvironmentVariableA(NULL, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    ret_size = GetEnvironmentVariableA("", buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, 0);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");
    ok(ret_size == lstrlenA(value) + 1, "should return length with terminating 0");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value));
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");
    ok(ret_size == lstrlenA(value) + 1, "should return length with terminating 0");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer");
    ok(ret_size == lstrlenA(value), "should return length without terminating 0");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer");
    ok(ret_size == lstrlenA(value), "should return length without terminating 0");

    ret = SetEnvironmentVariableA(name, NULL);
    ok(ret == TRUE, "should erase existing variable");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer");

    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE, "unexpected error in SetEnvironmentVariableA");

    ret = SetEnvironmentVariableA(name, "");
    ok(ret == TRUE, "should not fail with empty value");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");
    ok(lstrcmpA(buf, "") == 0, "should copy an empty string");
}

static void test_GetSetEnvironmentVariableW(void)
{
    WCHAR buf[256];
    BOOL ret;
    DWORD ret_size;
    static const WCHAR name[] = {'S','o','m','e','W','i','l','d','N','a','m','e',0};
    static const WCHAR value[] = {'S','o','m','e','W','i','l','d','V','a','l','u','e',0};
    static const WCHAR name_cased[] = {'s','O','M','E','w','I','L','D','n','A','M','E',0};
    static const WCHAR empty_strW[] = { 0 };
    static const WCHAR fooW[] = {'f','o','o',0};

    ret = SetEnvironmentVariableW(NULL, NULL);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_PARAMETER, "should fail with NULL, NULL");

    ret = SetEnvironmentVariableW(empty_strW, empty_strW);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_PARAMETER, "should fail with \"\", \"\"");

    ret = SetEnvironmentVariableW(name, empty_strW);
    ok(ret == TRUE, "should not fail with empty value");

    ret = SetEnvironmentVariableW(empty_strW, value);
    ok(ret == FALSE && GetLastError() == ERROR_INVALID_PARAMETER, "should fail with empty name");

    ret = SetEnvironmentVariableW(name, value);
    ok(ret == TRUE, "unexpected error in SetEnvironmentVariableW");

    /* the following line should just crash */
    /* ret_size = GetEnvironmentVariableW(name, NULL, lstrlenW(value) + 1); */

    ret_size = GetEnvironmentVariableW(NULL, NULL, 0);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    ret_size = GetEnvironmentVariableW(NULL, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    ret_size = GetEnvironmentVariableW(empty_strW, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, 0);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");
    ok(ret_size == lstrlenW(value) + 1, "should return length with terminating 0");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value));

    todo_wine {
        ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");
    };
    ok(ret_size == lstrlenW(value) + 1, "should return length with terminating 0");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer");
    ok(ret_size == lstrlenW(value), "should return length without terminating 0");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name_cased, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer");
    ok(ret_size == lstrlenW(value), "should return length without terminating 0");

    ret = SetEnvironmentVariableW(name, NULL);
    ok(ret == TRUE, "should erase existing variable");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer");

    ret = SetEnvironmentVariableW(name, value);
    ok(ret == TRUE, "unexpected error in SetEnvironmentVariableW");

    ret = SetEnvironmentVariableW(name, empty_strW);
    ok(ret == TRUE, "should not fail with empty value");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND, "should not find variable");

    todo_wine {
        ok(lstrcmpW(buf, empty_strW) == 0, "should copy an empty string");
    };
}

START_TEST(environ)
{
    test_GetSetEnvironmentVariableA();
    test_GetSetEnvironmentVariableW();
}
