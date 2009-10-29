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
}
