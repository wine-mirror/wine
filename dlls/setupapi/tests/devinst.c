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
#include "guiddef.h"
#include "setupapi.h"

#include "wine/test.h"

/* function pointers */
static HMODULE hSetupAPI;
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoListExW)(GUID*,HWND,PCWSTR,PVOID);
static BOOL     (WINAPI *pSetupDiDestroyDeviceInfoList)(HDEVINFO);
static HKEY     (WINAPI *pSetupDiOpenClassRegKeyExA)(GUID*,REGSAM,DWORD,PCSTR,PVOID);

static void init_function_pointers(void)
{
    hSetupAPI = LoadLibraryA("setupapi.dll");

    if (hSetupAPI)
    {
        pSetupDiCreateDeviceInfoListExW = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoListExW");
        pSetupDiDestroyDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiDestroyDeviceInfoList");
        pSetupDiOpenClassRegKeyExA = (void *)GetProcAddress(hSetupAPI, "SetupDiOpenClassRegKeyExA");
    }
}

static void test_SetupDiCreateDeviceInfoListEx(void) 
{
    HDEVINFO devlist;
    BOOL ret;
    DWORD error;
    static CHAR notnull[] = "NotNull";
    static const WCHAR machine[] = { 'd','u','m','m','y',0 };

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set Reserved to a value, which is not NULL */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, notnull);

    error = GetLastError();
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %ld (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_PARAMETER, "GetLastError returned wrong value : %ld, (expected %d)\n", error, ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set MachineName to something */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);

    error = GetLastError();
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %ld (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_MACHINENAME, "GetLastError returned wrong value : %ld, (expected %d)\n", error, ERROR_INVALID_MACHINENAME);

    /* create empty DeviceInfoList */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(devlist && devlist != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %ld (expected != %p)\n", devlist, error, INVALID_HANDLE_VALUE);

    /* destroy DeviceInfoList */
    ret = pSetupDiDestroyDeviceInfoList(devlist);
    ok(ret, "SetupDiDestroyDeviceInfoList failed : %ld\n", error);
}

static void test_SetupDiOpenClassRegKeyExA(void)
{
    /* This is a unique guid for testing purposes */
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HKEY hkey;

    /* Check return value for non-existent key */
    hkey = SetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS,
        DIOCR_INSTALLER, NULL, NULL);
    ok(hkey == INVALID_HANDLE_VALUE,
        "returned %p (expected INVALID_HANDLE_VALUE)\n", hkey);
}

START_TEST(devinst)
{
    init_function_pointers();
    if (!hSetupAPI)
        return;

    if (pSetupDiCreateDeviceInfoListExW && pSetupDiDestroyDeviceInfoList)
        test_SetupDiCreateDeviceInfoListEx();
    else
        trace("Needed calls for SetupDiCreateDeviceInfoListEx not all available, skipping test.\n");

    if (pSetupDiOpenClassRegKeyExA)
        test_SetupDiOpenClassRegKeyExA();
    else
        trace("Needed call for SetupDiOpenClassRegKeyExA not available, skipping test.\n");
}
