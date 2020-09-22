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
#include "winsock2.h"
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

static BOOL start_driver(HANDLE service, BOOL vista_plus)
{
    SERVICE_STATUS status;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = StartServiceA(service, 0, NULL);
    if (!ret && (GetLastError() == ERROR_DRIVER_BLOCKED || GetLastError() == ERROR_INVALID_IMAGE_HASH
            || (vista_plus && GetLastError() == ERROR_FILE_NOT_FOUND)))
    {
        if (vista_plus && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            skip("Windows Vista or newer is required to run this service.\n");
        }
        else
        {
            /* If Secure Boot is enabled or the machine is 64-bit, it will reject an unsigned driver. */
            skip("Failed to start service; probably your machine doesn't accept unsigned drivers.\n");
        }
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

static ULONG64 modified_value;

static void main_test(void)
{
    static const WCHAR dokW[] = {'d','o','k',0};
    WCHAR temppathW[MAX_PATH], pathW[MAX_PATH];
    struct test_input *test_input;
    DWORD len, written, read;
    UNICODE_STRING pathU;
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
    test_input->process_id = GetCurrentProcessId();
    test_input->teststr_offset = (SIZE_T)((BYTE *)&teststr - (BYTE *)NtCurrentTeb()->Peb->ImageBaseAddress);
    test_input->modified_value = &modified_value;
    modified_value = 0;

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
    char inbuf[64], buf[32];
    DWORD written;
    BOOL res;

    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, NULL, 0, buf,
                          sizeof(buf), &written, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(written == sizeof(teststr), "got size %d\n", written);
    ok(!strcmp(buf, teststr), "got '%s'\n", buf);

    memset(buf, 0, sizeof(buf));
    res = DeviceIoControl(device, IOCTL_WINETEST_BASIC_IOCTL, inbuf,
            sizeof(inbuf), buf, 10, &written, NULL);
    ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    ok(written == 10, "got size %d\n", written);
    ok(!strcmp(buf, "Wine is no"), "got '%s'\n", buf);
}

static void test_mismatched_status_ioctl(void)
{
    DWORD written;
    char buf[32];
    BOOL res;

    res = DeviceIoControl(device, IOCTL_WINETEST_MISMATCHED_STATUS, NULL, 0, buf,
                          sizeof(buf), &written, NULL);
    todo_wine ok(res, "DeviceIoControl failed: %u\n", GetLastError());
    todo_wine ok(!strcmp(buf, teststr), "got '%s'\n", buf);
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

static BOOL compare_unicode_string(const WCHAR *buffer, ULONG len, const WCHAR *expect)
{
    return len == wcslen(expect) * sizeof(WCHAR) && !memcmp(buffer, expect, len);
}

static void test_object_info(void)
{
    char buffer[200];
    OBJECT_NAME_INFORMATION *name_info = (OBJECT_NAME_INFORMATION *)buffer;
    OBJECT_TYPE_INFORMATION *type_info = (OBJECT_TYPE_INFORMATION *)buffer;
    FILE_NAME_INFORMATION *file_info = (FILE_NAME_INFORMATION *)buffer;
    HANDLE file;
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    ULONG size;

    status = NtQueryObject(device, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryObject(device, ObjectTypeInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(type_info->TypeName.Buffer, type_info->TypeName.Length, L"File"),
            "wrong name %s\n", debugstr_wn(type_info->TypeName.Buffer, type_info->TypeName.Length / sizeof(WCHAR)));

    status = NtQueryInformationFile(device, &io, buffer, sizeof(buffer), FileNameInformation);
    todo_wine ok(status == STATUS_INVALID_DEVICE_REQUEST, "got %#x\n", status);

    file = CreateFileA("\\\\.\\WineTestDriver\\subfile", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    todo_wine ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());
    if (file == INVALID_HANDLE_VALUE) return;

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), &size);
    ok(!status, "got %#x\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %u\n", size);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver\\subfile"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, size - 2, &size);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#x\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %u\n", size);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver\\subfil"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(*name_info), &size);
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#x\n", status);
    ok(size == sizeof(*name_info) + sizeof(L"\\Device\\WineTestDriver\\subfile"), "wrong size %u\n", size);

    status = NtQueryObject(file, ObjectTypeInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(type_info->TypeName.Buffer, type_info->TypeName.Length, L"File"),
            "wrong name %s\n", debugstr_wn(type_info->TypeName.Buffer, type_info->TypeName.Length / sizeof(WCHAR)));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(file_info->FileName, file_info->FileNameLength, L"\\subfile"),
            "wrong name %s\n", debugstr_wn(file_info->FileName, file_info->FileNameLength / sizeof(WCHAR)));

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\notimpl", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_NOT_IMPLEMENTED, "got %#x\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\badparam", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(!status, "got %#x\n", status);
    ok(compare_unicode_string(name_info->Name.Buffer, name_info->Name.Length, L"\\Device\\WineTestDriver"),
            "wrong name %s\n", debugstr_w(name_info->Name.Buffer));

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_INVALID_PARAMETER, "got %#x\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\genfail", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_UNSUCCESSFUL, "got %#x\n", status);

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_UNSUCCESSFUL, "got %#x\n", status);

    CloseHandle(file);

    file = CreateFileA("\\\\.\\WineTestDriver\\badtype", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    status = NtQueryObject(file, ObjectNameInformation, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "got %#x\n", status);

    status = NtQueryInformationFile(file, &io, buffer, sizeof(buffer), FileNameInformation);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "got %#x\n", status);

    CloseHandle(file);
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

static DWORD WINAPI wsk_test_thread(void *parameter)
{
    static const char test_send_string[] = "Client test string 1.";
    static const WORD version = MAKEWORD(2, 2);
    SOCKET s_listen, s_accept, s_connect;
    struct sockaddr_in addr;
    char buffer[256];
    int ret, err;
    WSADATA data;
    int opt_val;

    ret = WSAStartup(version, &data);
    ok(!ret, "WSAStartup() failed, ret %u.\n", ret);

    s_connect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s_connect != INVALID_SOCKET, "Error creating socket, WSAGetLastError() %u.\n", WSAGetLastError());

    s_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ok(s_listen != INVALID_SOCKET, "Error creating socket, WSAGetLastError() %u.\n", WSAGetLastError());

    opt_val = 1;
    setsockopt(s_listen, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt_val, sizeof(opt_val));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CLIENT_LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(0x7f000001);
    ret = bind(s_listen, (struct sockaddr *)&addr, sizeof(addr));
    ok(!ret, "Got unexpected ret %d, WSAGetLastError() %u.\n", ret, WSAGetLastError());

    ret = listen(s_listen, SOMAXCONN);
    ok(!ret, "Got unexpected ret %d, WSAGetLastError() %u.\n", ret, WSAGetLastError());

    addr.sin_port = htons(SERVER_LISTEN_PORT);

    ret = connect(s_connect, (struct sockaddr *)&addr, sizeof(addr));
    while (ret && ((err = WSAGetLastError()) == WSAECONNREFUSED || err == WSAECONNABORTED))
    {
        SwitchToThread();
        ret = connect(s_connect, (struct sockaddr *)&addr, sizeof(addr));
    }
    ok(!ret, "Error connecting, WSAGetLastError() %u.\n", WSAGetLastError());

    ret = send(s_connect, test_send_string, sizeof(test_send_string), 0);
    ok(ret == sizeof(test_send_string), "Got unexpected ret %d.\n", ret);

    ret = recv(s_connect, buffer, sizeof(buffer), 0);
    ok(ret == sizeof(buffer), "Got unexpected ret %d.\n", ret);
    ok(!strcmp(buffer, "Server test string 1."), "Received unexpected data.\n");

    s_accept = accept(s_listen, NULL, NULL);
    ok(s_accept != INVALID_SOCKET, "Error creating socket, WSAGetLastError() %u.\n", WSAGetLastError());

    closesocket(s_accept);
    closesocket(s_connect);
    closesocket(s_listen);
    return TRUE;
}

static void test_driver4(void)
{
    char filename[MAX_PATH];
    SC_HANDLE service;
    HANDLE hthread;
    DWORD written;
    BOOL ret;

    if (!(service = load_driver(filename, "driver4.dll", "WineTestDriver4")))
        return;

    if (!start_driver(service, TRUE))
    {
        DeleteFileA(filename);
        return;
    }

    device = CreateFileA("\\\\.\\WineTestDriver4", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    hthread = CreateThread(NULL, 0, wsk_test_thread, NULL, 0, NULL);
    main_test();
    WaitForSingleObject(hthread, INFINITE);

    ret = DeviceIoControl(device, IOCTL_WINETEST_DETACH, NULL, 0, NULL, 0, &written, NULL);
    ok(ret, "DeviceIoControl failed: %u\n", GetLastError());

    CloseHandle(device);

    unload_driver(service);
    ret = DeleteFileA(filename);
    ok(ret, "DeleteFile failed: %u\n", GetLastError());
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

    subtest("driver");
    if (!(service = load_driver(filename, "driver.dll", "WineTestDriver")))
        return;
    if (!start_driver(service, FALSE))
    {
        DeleteFileA(filename);
        return;
    }
    service2 = load_driver(filename2, "driver2.dll", "WineTestDriver2");

    device = CreateFileA("\\\\.\\WineTestDriver", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(device != INVALID_HANDLE_VALUE, "failed to open device: %u\n", GetLastError());

    test_basic_ioctl();
    test_mismatched_status_ioctl();

    main_test();
    todo_wine ok(modified_value == 0xdeadbeeffeedcafe, "Got unexpected value %#I64x.\n", modified_value);

    test_overlapped();
    test_load_driver(service2);
    test_file_handles();
    test_return_status();
    test_object_info();

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
    subtest("driver4");
    test_driver4();
}
