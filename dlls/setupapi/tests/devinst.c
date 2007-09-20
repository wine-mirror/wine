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
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoList)(GUID*,HWND);
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoListExW)(GUID*,HWND,PCWSTR,PVOID);
static BOOL     (WINAPI *pSetupDiDestroyDeviceInfoList)(HDEVINFO);
static BOOL     (WINAPI *pSetupDiEnumDeviceInfo)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
static HKEY     (WINAPI *pSetupDiOpenClassRegKeyExA)(GUID*,REGSAM,DWORD,PCSTR,PVOID);
static BOOL     (WINAPI *pSetupDiCreateDeviceInfoA)(HDEVINFO, PCSTR, GUID *, PCSTR, HWND, DWORD, PSP_DEVINFO_DATA);

static void init_function_pointers(void)
{
    hSetupAPI = GetModuleHandleA("setupapi.dll");

    pSetupDiCreateDeviceInfoA = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoA");
    pSetupDiCreateDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoList");
    pSetupDiCreateDeviceInfoListExW = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoListExW");
    pSetupDiDestroyDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiDestroyDeviceInfoList");
    pSetupDiEnumDeviceInfo = (void *)GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInfo");
    pSetupDiOpenClassRegKeyExA = (void *)GetProcAddress(hSetupAPI, "SetupDiOpenClassRegKeyExA");
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
    if (error == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("SetupDiCreateDeviceInfoListExW is not implemented\n");
        return;
    }
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_PARAMETER, "GetLastError returned wrong value : %d, (expected %d)\n", error, ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set MachineName to something */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);

    error = GetLastError();
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_MACHINENAME, "GetLastError returned wrong value : %d, (expected %d)\n", error, ERROR_INVALID_MACHINENAME);

    /* create empty DeviceInfoList */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(devlist && devlist != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected != %p)\n", devlist, error, INVALID_HANDLE_VALUE);

    /* destroy DeviceInfoList */
    ret = pSetupDiDestroyDeviceInfoList(devlist);
    ok(ret, "SetupDiDestroyDeviceInfoList failed : %d\n", error);
}

static void test_SetupDiOpenClassRegKeyExA(void)
{
    /* This is a unique guid for testing purposes */
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    static const CHAR guidString[] = "{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    HKEY hkey;

    /* Check return value for nonexistent key */
    hkey = pSetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS,
        DIOCR_INSTALLER, NULL, NULL);
    ok(hkey == INVALID_HANDLE_VALUE,
        "returned %p (expected INVALID_HANDLE_VALUE)\n", hkey);

    /* Test it for a key that exists */
    hkey = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS);
    if (hkey != INVALID_HANDLE_VALUE)
    {
        HKEY classKey;
        if (RegCreateKeyA(hkey, guidString, &classKey) == ERROR_SUCCESS)
        {
            RegCloseKey(classKey);
            SetLastError(0xdeadbeef);
            classKey = pSetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS,
                DIOCR_INSTALLER, NULL, NULL);
            ok(classKey != INVALID_HANDLE_VALUE,
                "opening class registry key failed with error %d\n",
                GetLastError());
            if (classKey != INVALID_HANDLE_VALUE)
                RegCloseKey(classKey);
            RegDeleteKeyA(hkey, guidString);
        }
        else
            trace("failed to create registry key for test\n");
    }
    else
        trace("failed to open classes key\n");
}

static void testCreateDeviceInfo(void)
{
    BOOL ret;
    HDEVINFO set;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiEnumDeviceInfo ||
     !pSetupDiDestroyDeviceInfoList || !pSetupDiCreateDeviceInfoA)
    {
        skip("No SetupDiCreateDeviceInfoA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_DEVINST_NAME,
     "Expected ERROR_INVALID_DEVINST_NAME, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\0000", NULL,
     NULL, NULL, 0, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLEHANDLE, got %08x\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %08x\n",
     GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { 0 };
        DWORD i;

        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL,
         NULL, NULL, 0, NULL);
        todo_wine
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        /* Finally, with all three required parameters, this succeeds: */
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, NULL);
        todo_wine
        ok(ret, "pSetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* This fails because the device ID already exists.. */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        todo_wine
        ok(!ret && GetLastError() == ERROR_DEVINST_ALREADY_EXISTS,
         "Expected ERROR_DEVINST_ALREADY_EXISTS, got %08x\n", GetLastError());
        /* whereas this "fails" because cbSize is wrong.. */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
        todo_wine
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
        /* and this finally succeeds. */
        todo_wine
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* There were three devices added, however - the second failure just
         * resulted in the SP_DEVINFO_DATA not getting copied.
         */
        SetLastError(0xdeadbeef);
        i = 0;
        while (pSetupDiEnumDeviceInfo(set, i, &devInfo))
            i++;
        todo_wine
        ok(i == 3, "Expected 3 devices, got %d\n", i);
        ok(GetLastError() == ERROR_NO_MORE_ITEMS,
         "SetupDiEnumDeviceInfo failed: %08x\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

START_TEST(devinst)
{
    init_function_pointers();

    if (pSetupDiCreateDeviceInfoListExW && pSetupDiDestroyDeviceInfoList)
        test_SetupDiCreateDeviceInfoListEx();
    else
        skip("SetupDiCreateDeviceInfoListExW and/or SetupDiDestroyDeviceInfoList not available\n");

    if (pSetupDiOpenClassRegKeyExA)
        test_SetupDiOpenClassRegKeyExA();
    else
        skip("SetupDiOpenClassRegKeyExA is not available\n");
    testCreateDeviceInfo();
}
