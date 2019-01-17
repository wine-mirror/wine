/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
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

#include <stdio.h>
#include "windows.h"
#include "winsvc.h"
#include "winioctl.h"
#include "winternl.h"
#include "wine/test.h"
#include "wine/heap.h"

#include "driver.h"

static HANDLE device;

static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static BOOL     (WINAPI *pRtlFreeUnicodeString)( PUNICODE_STRING );

static void load_resource(const char *name, char *filename)
{
    static char path[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, name, 0, filename);

    file = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n", filename, GetLastError());

    res = FindResourceA(NULL, name, "TESTDLL");
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static void unload_driver(SC_HANDLE service)
{
    SERVICE_STATUS status;

    CloseHandle(device);

    ControlService(service, SERVICE_CONTROL_STOP, &status);
    while (status.dwCurrentState == SERVICE_STOP_PENDING)
    {
        BOOL ret;
        Sleep(100);
        ret = QueryServiceStatus(service, &status);
        ok(ret, "QueryServiceStatus failed: %u\n", GetLastError());
    }
    ok(status.dwCurrentState == SERVICE_STOPPED,
       "expected SERVICE_STOPPED, got %d\n", status.dwCurrentState);

    DeleteService(service);
    CloseServiceHandle(service);
}

static SC_HANDLE load_driver(char *filename, const char *resname, const char *driver_name)
{
    SC_HANDLE manager, service;

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!manager && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Failed to open SC manager, not enough permissions\n");
        return FALSE;
    }
    ok(!!manager, "OpenSCManager failed\n");

    /* stop any old drivers running under this name */
    service = OpenServiceA(manager, driver_name, SERVICE_ALL_ACCESS);
    if (service) unload_driver(service);

    load_resource(resname, filename);
    trace("Trying to load driver %s\n", filename);

    service = CreateServiceA(manager, driver_name, driver_name,
                             SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
                             SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                             filename, NULL, NULL, NULL, NULL, NULL);
    ok(!!service, "CreateService failed: %u\n", GetLastError());

    CloseServiceHandle(manager);
    return service;
}

static BOOL start_driver(HANDLE service)
{
    SERVICE_STATUS status;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = StartServiceA(service, 0, NULL);
    if (!ret && (GetLastError() == ERROR_DRIVER_BLOCKED || GetLastError() == ERROR_INVALID_IMAGE_HASH))
    {
        /* If Secure Boot is enabled or the machine is 64-bit, it will reject an unsigned driver. */
        skip("Failed to start service; probably your machine doesn't accept unsigned drivers.\n");
        DeleteService(service);
        CloseServiceHandle(service);
        return FALSE;
    }
    ok(ret, "StartService failed: %u\n", GetLastError());

    /* wait for the service to start up properly */
    ret = QueryServiceStatus(service, &status);
    ok(ret, "QueryServiceStatus failed: %u\n", GetLastError());
    while (status.dwCurrentState == SERVICE_START_PENDING)
    {
        Sleep(100);
        ret = QueryServiceStatus(service, &status);
        ok(ret, "QueryServiceStatus failed: %u\n", GetLastError());
    }
    ok(status.dwCurrentState == SERVICE_RUNNING,
       "expected SERVICE_RUNNING, got %d\n", status.dwCurrentState);

    return TRUE;
}

static void main_test(void)
{
    static const WCHAR dokW[] = {'d','o','k',0};
    WCHAR temppathW[MAX_PATH], pathW[MAX_PATH];
    struct test_input *test_input;
    UNICODE_STRING pathU;
    DWORD len, written, read;
    LONG new_failures;
    char buffer[512];
    HANDLE okfile;
    BOOL res;

    /* Create a temporary file that the driver will write ok/trace output to. */
    GetTempPathW(MAX_PATH, temppathW);
    GetTempFileNameW(temppathW, dokW, 0, pathW);
    pRtlDosPathNameToNtPathName_U( pathW, &pathU, NULL, NULL );

    len = pathU.Length + sizeof(WCHAR);
    test_input = heap_alloc( offsetof( struct test_input, path[len / sizeof(WCHAR)]) );
    test_input->running_under_wine = !strcmp(winetest_platform, "wine");
    test_input->winetest_report_success = winetest_report_success;
    test_input->winetest_debug = winetest_debug;
    memcpy(test_input->path, pathU.Buffer, len);
    res = DeviceIoControl(device, IOCTL_WINETEST_MAIN_TEST, test_input,
                          offsetof( struct test_input, path[len / sizeof(WCHAR)]),
                          &new_failures, sizeof(new_failures), &written, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(written == sizeof(new_failures), "got size %x\n", written);

    okfile = CreateFileW(pathW, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(okfile != INVALID_HANDLE_VALUE, "failed to create %s: %u\n", wine_dbgstr_w(pathW), GetLastError());

    /* Print the ok/trace output and then add to our failure count. */
    do {
        ReadFile(okfile, buffer, sizeof(buffer), &read, NULL);
        printf("%.*s", read, buffer);
    } while (read == sizeof(buffer));
    winetest_add_failures(new_failures);

    pRtlFreeUnicodeString(&pathU);
    heap_free(test_input);
    CloseHandle(okfile);
    DeleteFileW(pathW);
}

static void test_basic_ioctl(void)
{
    DWORD written;
    char buf[32];
    BOOL res;

    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, NULL, 0, buf,
                          sizeof(buf), &written, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(written == sizeof(teststr), "got size %d\n", written);
    ok(!strcmp(buf, teststr), "got '%s'\n", buf);
}

static void test_load_driver(SC_HANDLE service)
{
    SERVICE_STATUS status;
    BOOL load, res;
    DWORD sz;

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "got state %#x\n", status.dwCurrentState);

    load = TRUE;
    res = DeviceIoControl(device, IOCTL_WINETEST_LOAD_DRIVER, &load, sizeof(load), NULL, 0, &sz, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_RUNNING, "got state %#x\n", status.dwCurrentState);

    load = FALSE;
    res = DeviceIoControl(device, IOCTL_WINETEST_LOAD_DRIVER, &load, sizeof(load), NULL, 0, &sz, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());

    res = QueryServiceStatus(service, &status);
    ok(res, "QueryServiceStatusEx failed: %u\n", GetLastError());
    ok(status.dwCurrentState == SERVICE_STOPPED, "got state %#x\n", status.dwCurrentState);
}

static void test_driver3(void)
{
    char filename[MAX_PATH];
    SC_HANDLE service;
    BOOL ret;

    service = load_driver(filename, "driver3.dll", "WineTestDriver3");
    ok(service != NULL, "driver3 failed to load\n");

    ret = StartServiceA(service, 0, NULL);
    ok(!ret, "driver3 should fail to start\n");
    ok(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ||
       GetLastError() == ERROR_INVALID_FUNCTION ||
       GetLastError() == ERROR_PROC_NOT_FOUND /* XP */ ||
       GetLastError() == ERROR_FILE_NOT_FOUND /* Win7 */, "got %u\n", GetLastError());

    DeleteService(service);
    CloseServiceHandle(service);
    DeleteFileA(filename);
}

START_TEST(ntoskrnl)
{
    char filename[MAX_PATH], filename2[MAX_PATH];
    SC_HANDLE service, service2;

    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlFreeUnicodeString = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");

    if (!(service = load_driver(filename, "driver.dll", "WineTestDriver")))
        return;
    if (!start_driver(service))
    {
        DeleteFileA(filename);
        return;
    }
    service2 = load_driver(filename2, "driver2.dll", "WineTestDriver2");

    device = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    test_basic_ioctl();
    main_test();
    test_load_driver(service2);

    unload_driver(service2);
    unload_driver(service);
    ok(DeleteFileA(filename), "DeleteFile failed: %u\n", GetLastError());
    ok(DeleteFileA(filename2), "DeleteFile failed: %u\n", GetLastError());

    test_driver3();
}
