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
static BOOL     (WINAPI *pSetupDiCreateDeviceInterfaceA)(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, PCSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
static BOOL     (WINAPI *pSetupDiDestroyDeviceInfoList)(HDEVINFO);
static BOOL     (WINAPI *pSetupDiEnumDeviceInfo)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
static HKEY     (WINAPI *pSetupDiOpenClassRegKeyExA)(GUID*,REGSAM,DWORD,PCSTR,PVOID);
static BOOL     (WINAPI *pSetupDiCreateDeviceInfoA)(HDEVINFO, PCSTR, GUID *, PCSTR, HWND, DWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiGetDeviceInstanceIdA)(HDEVINFO, PSP_DEVINFO_DATA, PSTR, DWORD, PDWORD);
static BOOL     (WINAPI *pSetupDiGetDeviceInterfaceDetailA)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, PDWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiRegisterDeviceInfo)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PSP_DETSIG_CMPPROC, PVOID, PSP_DEVINFO_DATA);

static void init_function_pointers(void)
{
    hSetupAPI = GetModuleHandleA("setupapi.dll");

    pSetupDiCreateDeviceInfoA = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoA");
    pSetupDiCreateDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoList");
    pSetupDiCreateDeviceInfoListExW = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoListExW");
    pSetupDiCreateDeviceInterfaceA = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInterfaceA");
    pSetupDiDestroyDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiDestroyDeviceInfoList");
    pSetupDiEnumDeviceInfo = (void *)GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInfo");
    pSetupDiEnumDeviceInterfaces = (void *)GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInterfaces");
    pSetupDiGetDeviceInstanceIdA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceInstanceIdA");
    pSetupDiGetDeviceInterfaceDetailA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceInterfaceDetailA");
    pSetupDiOpenClassRegKeyExA = (void *)GetProcAddress(hSetupAPI, "SetupDiOpenClassRegKeyExA");
    pSetupDiRegisterDeviceInfo = (void *)GetProcAddress(hSetupAPI, "SetupDiRegisterDeviceInfo");
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
    ok(!ret && GetLastError() == ERROR_INVALID_DEVINST_NAME,
     "Expected ERROR_INVALID_DEVINST_NAME, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\0000", NULL,
     NULL, NULL, 0, NULL);
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
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        /* Finally, with all three required parameters, this succeeds: */
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, NULL);
        ok(ret, "pSetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* This fails because the device ID already exists.. */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        ok(!ret && GetLastError() == ERROR_DEVINST_ALREADY_EXISTS,
         "Expected ERROR_DEVINST_ALREADY_EXISTS, got %08x\n", GetLastError());
        /* whereas this "fails" because cbSize is wrong.. */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
        /* and this finally succeeds. */
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* There were three devices added, however - the second failure just
         * resulted in the SP_DEVINFO_DATA not getting copied.
         */
        SetLastError(0xdeadbeef);
        i = 0;
        while (pSetupDiEnumDeviceInfo(set, i, &devInfo))
            i++;
        ok(i == 3, "Expected 3 devices, got %d\n", i);
        ok(GetLastError() == ERROR_NO_MORE_ITEMS,
         "SetupDiEnumDeviceInfo failed: %08x\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testGetDeviceInstanceId(void)
{
    BOOL ret;
    HDEVINFO set;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    SP_DEVINFO_DATA devInfo = { 0 };

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiGetDeviceInstanceIdA)
    {
        skip("No SetupDiGetDeviceInstanceIdA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInstanceIdA(NULL, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLEHANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInstanceIdA(NULL, &devInfo, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLEHANDLE, got %08x\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %08x\n",
     GetLastError());
    if (set)
    {
        char instanceID[MAX_PATH];
        DWORD size;

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
         "Expected ERROR_INSUFFICIENT_BUFFER, got %08x\n", GetLastError());
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, instanceID,
         sizeof(instanceID), NULL);
        ok(ret, "SetupDiGetDeviceInstanceIdA failed: %08x\n", GetLastError());
        ok(!lstrcmpA(instanceID, "ROOT\\LEGACY_BOGUS\\0000"),
         "Unexpected instance ID %s\n", instanceID);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid,
         NULL, NULL, DICD_GENERATE_ID, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, instanceID,
         sizeof(instanceID), NULL);
        ok(ret, "SetupDiGetDeviceInstanceIdA failed: %08x\n", GetLastError());
        ok(!lstrcmpA(instanceID, "ROOT\\LEGACY_BOGUS\\0001"),
         "Unexpected instance ID %s\n", instanceID);
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testRegisterDeviceInfo(void)
{
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiRegisterDeviceInfo)
    {
        skip("No SetupDiRegisterDeviceInfo\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ret = pSetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { 0 };

        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, NULL, 0, NULL, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        ret = pSetupDiCreateDeviceInfoA(set, "USB\\BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        ok(ret || GetLastError() == ERROR_DEVINST_ALREADY_EXISTS,
                "SetupDiCreateDeviceInfoA failed: %d\n", GetLastError());
        if (ret)
        {
            /* If it already existed, registering it again will fail */
            ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL,
             NULL);
            ok(ret, "SetupDiCreateDeviceInfoA failed: %d\n", GetLastError());
        }
        /* FIXME: On Win2K+ systems, this is now persisted to registry in
         * HKLM\System\CCS\Enum\USB\Bogus\0000.  I don't check because the
         * Win9x location is different.
         * FIXME: the key also becomes undeletable.  How to get rid of it?
         */
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testCreateDeviceInterface(void)
{
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiCreateDeviceInterfaceA ||
     !pSetupDiEnumDeviceInterfaces)
    {
        skip("No SetupDiCreateDeviceInterfaceA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInterfaceA(NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInterfaceA(NULL, NULL, &guid, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { 0 };
        SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(interfaceData),
            { 0 } };
        DWORD i;

        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, NULL, NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, NULL, NULL, 0,
                NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        ret = pSetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid,
                NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, NULL, NULL, 0,
                NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0,
                NULL);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        /* Creating the same interface a second time succeeds */
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0,
                NULL);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, "Oogah", 0,
                NULL);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        ret = pSetupDiEnumDeviceInterfaces(set, &devInfo, &guid, 0,
                &interfaceData);
        ok(ret, "SetupDiEnumDeviceInterfaces failed: %d\n", GetLastError());
        i = 0;
        while (pSetupDiEnumDeviceInterfaces(set, &devInfo, &guid, i,
                    &interfaceData))
            i++;
        ok(i == 2, "expected 2 interfaces, got %d\n", i);
        ok(GetLastError() == ERROR_NO_MORE_ITEMS,
         "SetupDiEnumDeviceInterfaces failed: %08x\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testGetDeviceInterfaceDetail(void)
{
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiCreateDeviceInterfaceA ||
     !pSetupDiGetDeviceInterfaceDetailA)
    {
        skip("No SetupDiGetDeviceInterfaceDetailA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInterfaceDetailA(NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { sizeof(devInfo), { 0 } };
        SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(interfaceData),
            { 0 } };
        DWORD size = 0;

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, NULL, NULL, 0, NULL,
                NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        ret = pSetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid,
                NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0,
                &interfaceData);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL,
                0, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
         "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL,
                100, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL,
                0, &size, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
         "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
        if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            LPBYTE buf = HeapAlloc(GetProcessHeap(), 0, size);
            SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail =
                (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buf;

            detail->cbSize = 0;
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
             "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
            detail->cbSize = size;
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            todo_wine
            ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
             "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            ok(ret, "SetupDiGetDeviceInterfaceDetailA failed: %d\n",
                    GetLastError());
            HeapFree(GetProcessHeap(), 0, buf);
        }
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
    testGetDeviceInstanceId();
    testRegisterDeviceInfo();
    testCreateDeviceInterface();
    testGetDeviceInterfaceDetail();
}
