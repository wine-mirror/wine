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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "winsvc.h"
#include "winioctl.h"
#include "winternl.h"
#include "wine/test.h"
#include "wine/heap.h"

#include "driver.h"

static HANDLE device;

static BOOL (WINAPI *pRtlDosPathNameToNtPathName_U)(const WCHAR *, UNICODE_STRING *, WCHAR **, CURDIR *);
static BOOL (WINAPI *pRtlFreeUnicodeString)(UNICODE_STRING *);
static BOOL (WINAPI *pCancelIoEx)(HANDLE, OVERLAPPED *);
static BOOL (WINAPI *pSetFileCompletionNotificationModes)(HANDLE, UCHAR);

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
    ok(status.dwServiceType == SERVICE_KERNEL_DRIVER,
       "expected SERVICE_KERNEL_DRIVER, got %#x\n", status.dwServiceType);

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

static void test_overlapped(void)
{
    OVERLAPPED overlapped, overlapped2, *o;
    DWORD cancel_cnt, size;
    HANDLE file, port;
    ULONG_PTR key;
    BOOL res;

    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    overlapped2.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    file = CreateFileA("\\\\.\\WineTestDriver", FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                       0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    /* test cancelling all device requests */
    res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped2);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

    cancel_cnt = 0xdeadbeef;
    res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(cancel_cnt == 0, "cancel_cnt = %u\n", cancel_cnt);

    CancelIo(file);

    cancel_cnt = 0xdeadbeef;
    res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(cancel_cnt == 2, "cancel_cnt = %u\n", cancel_cnt);

    /* test cancelling selected overlapped event */
    if (pCancelIoEx)
    {
        res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_TEST_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped2);
        ok(!res && GetLastError() == ERROR_IO_PENDING, "DeviceIoControl failed: %u\n", GetLastError());

        pCancelIoEx(file, &overlapped);

        cancel_cnt = 0xdeadbeef;
        res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());
        ok(cancel_cnt == 1, "cancel_cnt = %u\n", cancel_cnt);

        pCancelIoEx(file, &overlapped2);

        cancel_cnt = 0xdeadbeef;
        res = DeviceIoControl(file, IOCTL_WINETEST_GET_CANCEL_COUNT, NULL, 0, &cancel_cnt, sizeof(cancel_cnt), NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());
        ok(cancel_cnt == 2, "cancel_cnt = %u\n", cancel_cnt);
    }

    port = CreateIoCompletionPort(file, NULL, 0xdeadbeef, 0);
    ok(port != NULL, "CreateIoCompletionPort failed, error %u\n", GetLastError());
    res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
    ok(!res && GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus returned %x(%u)\n", res, GetLastError());

    res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
    ok(res, "GetQueuedCompletionStatus failed: %u\n", GetLastError());
    ok(o == &overlapped, "o != overlapped\n");

    if (pSetFileCompletionNotificationModes)
    {
        res = pSetFileCompletionNotificationModes(file, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
        ok(res, "SetFileCompletionNotificationModes failed: %u\n", GetLastError());

        res = DeviceIoControl(file, IOCTL_WINETEST_RESET_CANCEL, NULL, 0, NULL, 0, NULL, &overlapped);
        ok(res, "DeviceIoControl failed: %u\n", GetLastError());
        res = GetQueuedCompletionStatus(port, &size, &key, &o, 0);
        ok(!res && GetLastError() == WAIT_TIMEOUT, "GetQueuedCompletionStatus returned %x(%u)\n", res, GetLastError());
    }

    CloseHandle(port);
    CloseHandle(overlapped.hEvent);
    CloseHandle(overlapped2.hEvent);
    CloseHandle(file);
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

static void test_file_handles(void)
{
    DWORD count, ret_size;
    HANDLE file, dup, file2;
    BOOL ret;

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 2, "got %u\n", count);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);

    file = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);

    file2 = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file2 != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 4, "got %u\n", count);

    ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "failed to duplicate handle: %u\n", GetLastError());

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CREATE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 4, "got %u\n", count);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);

    ret = DeviceIoControl(file, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);

    ret = DeviceIoControl(file2, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 4, "got %u\n", count);

    ret = DeviceIoControl(dup, IOCTL_WINETEST_GET_FSCONTEXT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);

    CloseHandle(dup);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 1, "got %u\n", count);

    CloseHandle(file2);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 2, "got %u\n", count);

    CloseHandle(file);

    ret = DeviceIoControl(device, IOCTL_WINETEST_GET_CLOSE_COUNT, NULL, 0, &count, sizeof(count), &ret_size, NULL);
    ok(ret, "ioctl failed: %u\n", GetLastError());
    ok(count == 3, "got %u\n", count);
}

static void test_return_status(void)
{
    NTSTATUS status;
    char buffer[7];
    DWORD ret_size;
    BOOL ret;

    strcpy(buffer, "abcdef");
    status = STATUS_SUCCESS;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    ok(ret, "ioctl failed\n");
    ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = STATUS_TIMEOUT;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    todo_wine ok(ret, "ioctl failed\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0x0eadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    todo_wine ok(ret, "ioctl failed\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0x4eadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    todo_wine ok(ret, "ioctl failed\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0x8eadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    ok(!ret, "ioctl succeeded\n");
    ok(GetLastError() == ERROR_MR_MID_NOT_FOUND, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "ghidef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);

    strcpy(buffer, "abcdef");
    status = 0xceadbeef;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(device, IOCTL_WINETEST_RETURN_STATUS, &status,
            sizeof(status), buffer, sizeof(buffer), &ret_size, NULL);
    ok(!ret, "ioctl succeeded\n");
    ok(GetLastError() == ERROR_MR_MID_NOT_FOUND, "got error %u\n", GetLastError());
    ok(!strcmp(buffer, "abcdef"), "got buffer %s\n", buffer);
    ok(ret_size == 3, "got size %u\n", ret_size);
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
    DWORD written;
    BOOL ret;

    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlFreeUnicodeString = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pCancelIoEx = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CancelIoEx");
    pSetFileCompletionNotificationModes = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                                                 "SetFileCompletionNotificationModes");

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
    test_overlapped();
    test_load_driver(service2);
    test_file_handles();
    test_return_status();

    /* We need a separate ioctl to call IoDetachDevice(); calling it in the
     * driver unload routine causes a live-lock. */
    ret = DeviceIoControl(device, IOCTL_WINETEST_DETACH, NULL, 0, NULL, 0, &written, NULL);
    ok(ret, "DeviceIoControl failed: %u\n", GetLastError());

    CloseHandle(device);

    unload_driver(service2);
    unload_driver(service);
    ret = DeleteFileA(filename);
    ok(ret, "DeleteFile failed: %u\n", GetLastError());
    ret = DeleteFileA(filename2);
    ok(ret, "DeleteFile failed: %u\n", GetLastError());

    test_driver3();
}
