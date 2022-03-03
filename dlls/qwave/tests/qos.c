/*
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

#include <windows.h>
#include "qos2.h"

#include "wine/test.h"

static void test_QOSCreateHandle(void)
{
    QOS_VERSION ver;
    HANDLE h;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = QOSCreateHandle(NULL, NULL);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = QOSCreateHandle(&ver, NULL);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ver.MajorVersion = 0;
    ver.MinorVersion = 0;
    ret = QOSCreateHandle(&ver, &h);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    ver.MajorVersion = 1;
    ver.MinorVersion = 0;
    ret = QOSCreateHandle(&ver, &h);
    todo_wine ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    SetLastError(0xdeadbeef);
    ver.MajorVersion = 1;
    ver.MinorVersion = 5;
    ret = QOSCreateHandle(&ver, &h);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ver.MajorVersion = 2;
    ver.MinorVersion = 0;
    ret = QOSCreateHandle(&ver, &h);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
}

START_TEST(qos)
{
    test_QOSCreateHandle();
}
