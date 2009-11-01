/*
 * Unit tests for Event Logging functions
 *
 * Copyright (c) 2009 Paul Vriens
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnt.h"

#include "wine/test.h"

static BOOL (WINAPI *pGetEventLogInformation)(HANDLE,DWORD,LPVOID,DWORD,LPDWORD);

static void init_function_pointers(void)
{
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

    pGetEventLogInformation = (void*)GetProcAddress(hadvapi32, "GetEventLogInformation");
}

static void test_open_close(void)
{
    HANDLE handle;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = CloseEventLog(NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE ||
       GetLastError() == ERROR_NOACCESS, /* W2K */
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA(NULL, NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA("IDontExist", NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA("IDontExist", "deadbeef");
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
       GetLastError() == RPC_S_INVALID_NET_ADDR, /* Some Vista and Win7 */
       "Expected RPC_S_SERVER_UNAVAILABLE, got %d\n", GetLastError());

    /* This one opens the Application log */
    handle = OpenEventLogA(NULL, "deadbeef");
    ok(handle != NULL, "Expected a handle\n");
    ret = CloseEventLog(handle);
    ok(ret, "Expected success\n");
    /* Close a second time */
    SetLastError(0xdeadbeef);
    ret = CloseEventLog(handle);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    }

    /* Empty servername should be read as local server */
    handle = OpenEventLogA("", "Application");
    ok(handle != NULL, "Expected a handle\n");
    CloseEventLog(handle);

    handle = OpenEventLogA(NULL, "Application");
    ok(handle != NULL, "Expected a handle\n");
    CloseEventLog(handle);
}

static void test_info(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD needed;
    EVENTLOG_FULL_INFORMATION efi;

    if (!pGetEventLogInformation)
    {
        /* NT4 */
        win_skip("GetEventLogInformation is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(NULL, 1, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "Expected ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(NULL, EVENTLOG_FULL_INFO, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, NULL, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, (LPVOID)&efi, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    needed = 0xdeadbeef;
    efi.dwFull = 0xdeadbeef;
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, (LPVOID)&efi, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == sizeof(EVENTLOG_FULL_INFORMATION), "Expected sizeof(EVENTLOG_FULL_INFORMATION), got %d\n", needed);
    ok(efi.dwFull == 0xdeadbeef, "Expected no change to the dwFull member\n");

    /* Not that we care, but on success last error is set to ERROR_IO_PENDING */
    efi.dwFull = 0xdeadbeef;
    needed *= 2;
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, (LPVOID)&efi, needed, &needed);
    ok(ret, "Expected succes\n");
    ok(needed == sizeof(EVENTLOG_FULL_INFORMATION), "Expected sizeof(EVENTLOG_FULL_INFORMATION), got %d\n", needed);
    ok(efi.dwFull == 0 || efi.dwFull == 1, "Expected 0 (not full) or 1 (full), got %d\n", efi.dwFull);

    CloseEventLog(handle);
}

static void test_count(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD count;

    SetLastError(0xdeadbeef);
    ret = GetNumberOfEventLogRecords(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(NULL, &count);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(count == 0xdeadbeef, "Expected count to stay unchanged\n");

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = GetNumberOfEventLogRecords(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected succes\n");
    ok(count != 0xdeadbeef, "Expected the number of records\n");

    CloseEventLog(handle);
}

static void test_oldest(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD oldest;

    SetLastError(0xdeadbeef);
    ret = GetOldestEventLogRecord(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    oldest = 0xdeadbeef;
    ret = GetOldestEventLogRecord(NULL, &oldest);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(oldest == 0xdeadbeef, "Expected oldest to stay unchanged\n");

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = GetOldestEventLogRecord(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    oldest = 0xdeadbeef;
    ret = GetOldestEventLogRecord(handle, &oldest);
    ok(ret, "Expected succes\n");
    ok(oldest != 0xdeadbeef, "Expected the number of the oldest record\n");

    CloseEventLog(handle);
}

static void test_backup(void)
{
    HANDLE handle;
    BOOL ret;
    const char backup[] = "backup.evt";

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(NULL, backup);
    ok(!ret, "Expected failure\n");
    ok(GetFileAttributesA(backup) == INVALID_FILE_ATTRIBUTES, "Expected no backup file\n");

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    ret = BackupEventLogA(handle, backup);
    ok(ret, "Expected succes\n");
    todo_wine
    ok(GetFileAttributesA("backup.evt") != INVALID_FILE_ATTRIBUTES, "Expected a backup file\n");

    /* Try to overwrite */
    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(handle, backup);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "Expected ERROR_ALREADY_EXISTS, got %d\n", GetLastError());
    }

    CloseEventLog(handle);
    DeleteFileA(backup);
}

static void test_read(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD count, toread, read, needed;
    void *buf;

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    read = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, &read, NULL);
    ok(!ret, "Expected failure\n");
    ok(read == 0xdeadbeef, "Expected 'read' parameter to remain unchanged\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, NULL, &needed);
    ok(!ret, "Expected failure\n");
    ok(needed == 0xdeadbeef, "Expected 'needed' parameter to remain unchanged\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* 'read' and 'needed' are only filled when the needed buffer size is passed back or when the call succeeds */
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, NULL, 0, &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    buf = NULL;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, buf);

    handle = OpenEventLogA(NULL, "Application");

    /* Show that we need the proper dwFlags with a (for the rest) proper call */
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, 0, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ | EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, buf);

    /* First check if there are any records (in practice only on Wine: FIXME) */
    count = 0;
    GetNumberOfEventLogRecords(handle, &count);
    if (!count)
    {
        skip("No records in the 'Application' log\n");
        CloseEventLog(handle);
        return;
    }

    /* Get the buffer size for the first record */
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
    read = needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    ok(read == 0, "Expected no bytes read\n");
    ok(needed > sizeof(EVENTLOGRECORD), "Expected the needed buffersize to be bigger than sizeof(EVENTLOGRECORD)\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Read the first record */
    toread = needed;
    buf = HeapReAlloc(GetProcessHeap(), 0, buf, toread);
    read = needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, buf, toread, &read, &needed);
    ok(ret, "Expected succes\n");
    ok(read == toread ||
       broken(read < toread), /* NT4 wants a buffer size way bigger than just 1 record */
       "Expected the requested size to be read\n");
    ok(needed == 0, "Expected no extra bytes to be read\n");
    HeapFree(GetProcessHeap(), 0, buf);

    CloseEventLog(handle);
}

START_TEST(eventlog)
{
    SetLastError(0xdeadbeef);
    CloseEventLog(NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Event log functions are not implemented\n");
        return;
    }

    init_function_pointers();

    /* Parameters only */
    test_open_close();
    test_info();
    test_count();
    test_oldest();
    test_backup();
    test_read();
}
