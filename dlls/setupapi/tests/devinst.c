/*
 * Devinst tests
 *
 * Copyright 2006 Christian Gmeiner
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"


static void test_SetupDiCreateDeviceInfoListEx(void) 
{
    HDEVINFO devlist;
    BOOL ret;
    DWORD error;
    static CHAR notnull[] = "NotNull";
    static const WCHAR machine[] = { 'd','u','m','m','y',0 };

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set Reserved to a value, which is not NULL */
    devlist = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, notnull);

    error = GetLastError();
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %ld (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_PARAMETER, "GetLastError returned wrong value : %ld, (expected %d)\n", error, ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set MachineName to something */
    devlist = SetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);

    error = GetLastError();
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %ld (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_MACHINENAME, "GetLastError returned wrong value : %ld, (expected %d)\n", error, ERROR_INVALID_MACHINENAME);

    /* create empty DeviceInfoList */
    devlist = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(devlist && devlist != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %ld (expected != %p)\n", devlist, error, INVALID_HANDLE_VALUE);

    /* destroy DeviceInfoList */
    ret = SetupDiDestroyDeviceInfoList(devlist);
    ok(ret, "SetupDiDestroyDeviceInfoList failed : %ld\n", error);
}

START_TEST(devinst)
{
    test_SetupDiCreateDeviceInfoListEx();
}
