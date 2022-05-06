/*
 * Synchronization tests
 *
 * Copyright 2018 Daniel Lehman
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
#include <stdlib.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <winternl.h>

#include "wine/test.h"

static BOOL (WINAPI *pWaitOnAddress)(volatile void *, void *, SIZE_T, DWORD);
static void (WINAPI *pWakeByAddressAll)(void *);
static void (WINAPI *pWakeByAddressSingle)(void *);

static LONG address;
static DWORD WINAPI test_WaitOnAddress_func(void *arg)
{
    BOOL ret = FALSE;
    LONG compare;

    do
    {
        while (!(compare = address))
        {
            SetLastError(0xdeadbeef);
            ret = pWaitOnAddress(&address, &compare, sizeof(compare), INFINITE);
            ok(ret, "wait failed\n");
            ok(GetLastError() == 0xdeadbeef || broken(GetLastError() == ERROR_SUCCESS) /* Win 8 */,
                    "got error %ld\n", GetLastError());
        }
    } while (InterlockedCompareExchange(&address, compare - 1, compare) != compare);

    return 0;
}

static void test_WaitOnAddress(void)
{
    DWORD gle, val, nthreads;
    HANDLE threads[8];
    LONG compare;
    BOOL ret;
    int i;

    if (!pWaitOnAddress)
    {
        win_skip("WaitOnAddress not supported, skipping test\n");
        return;
    }

    address = 0;
    compare = 0;
    if (0) /* crash on Windows */
    {
        ret = pWaitOnAddress(&address, NULL, 4, 0);
        ret = pWaitOnAddress(NULL, &compare, 4, 0);
    }

    /* invalid arguments */
    SetLastError(0xdeadbeef);
    pWakeByAddressSingle(NULL);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %ld\n", gle);

    SetLastError(0xdeadbeef);
    pWakeByAddressAll(NULL);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %ld\n", gle);

    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(NULL, NULL, 0, 0);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "got %ld\n", gle);
    ok(!ret, "got %d\n", ret);

    address = 0;
    compare = 0;
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 5, 0);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "got %ld\n", gle);
    ok(!ret, "got %d\n", ret);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));
    ok(compare == 0, "got %s\n", wine_dbgstr_longlong(compare));

    /* no waiters */
    address = 0;
    SetLastError(0xdeadbeef);
    pWakeByAddressSingle(&address);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %ld\n", gle);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));

    SetLastError(0xdeadbeef);
    pWakeByAddressAll(&address);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "got %ld\n", gle);
    ok(address == 0, "got %s\n", wine_dbgstr_longlong(address));

    /* different address size */
    address = 0;
    compare = 0xff00;
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 2, 0);
    gle = GetLastError();
    ok(gle == 0xdeadbeef || broken(gle == ERROR_SUCCESS) /* Win 8 */, "got %ld\n", gle);
    ok(ret, "got %d\n", ret);

    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 1, 0);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "got %ld\n", gle);
    ok(!ret, "got %d\n", ret);

    /* simple wait case */
    address = 0;
    compare = 1;
    SetLastError(0xdeadbeef);
    ret = pWaitOnAddress(&address, &compare, 4, 0);
    gle = GetLastError();
    ok(gle == 0xdeadbeef || broken(gle == ERROR_SUCCESS) /* Win 8 */, "got %ld\n", gle);
    ok(ret, "got %d\n", ret);

    /* WakeByAddressAll */
    address = 0;
    for (i = 0; i < ARRAY_SIZE(threads); i++)
        threads[i] = CreateThread(NULL, 0, test_WaitOnAddress_func, NULL, 0, NULL);

    Sleep(100);
    address = ARRAY_SIZE(threads);
    pWakeByAddressAll(&address);
    val = WaitForMultipleObjects(ARRAY_SIZE(threads), threads, TRUE, 5000);
    ok(val == WAIT_OBJECT_0, "got %ld\n", val);
    for (i = 0; i < ARRAY_SIZE(threads); i++)
        CloseHandle(threads[i]);
    ok(!address, "got unexpected value %s\n", wine_dbgstr_longlong(address));

    /* WakeByAddressSingle */
    address = 0;
    for (i = 0; i < ARRAY_SIZE(threads); i++)
        threads[i] = CreateThread(NULL, 0, test_WaitOnAddress_func, NULL, 0, NULL);

    Sleep(100);
    nthreads = ARRAY_SIZE(threads);
    address = ARRAY_SIZE(threads);
    while (nthreads)
    {
        pWakeByAddressSingle(&address);
        val = WaitForMultipleObjects(nthreads, threads, FALSE, 2000);
        ok(val < WAIT_OBJECT_0 + nthreads, "got %lu\n", val);
        CloseHandle(threads[val]);
        memmove(&threads[val], &threads[val+1], (nthreads - val - 1) * sizeof(threads[0]));
        nthreads--;
    }
    ok(!address, "got unexpected value %s\n", wine_dbgstr_longlong(address));
}

static void test_Sleep(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER t1, t2;
    double elapsed_time, min, max;
    ULONG dummy, r1, r2;
    NTSTATUS status;
    BOOL ret;
    int i;

    ret = QueryPerformanceFrequency(&frequency);
    ok(ret, "QueryPerformanceFrequency failed\n");

    ret = QueryPerformanceCounter(&t1);
    ok(ret, "QueryPerformanceCounter failed\n");

    /* Get the timer resolution before... */
    r1 = 156250;
    status = NtQueryTimerResolution(&dummy, &dummy, &r1);
    ok(status == STATUS_SUCCESS, "NtQueryTimerResolution() failed (%lx)\n", status);

    for (i = 0; i < 50; i++) {
        Sleep(1);
    }

    ret = QueryPerformanceCounter(&t2);
    ok(ret, "QueryPerformanceCounter failed\n");

    /* ...and after in case some other process changes it during this test */
    r2 = 156250;
    status = NtQueryTimerResolution(&dummy, &dummy, &r2);
    ok(status == STATUS_SUCCESS, "NtQueryTimerResolution() failed (%lx)\n", status);

    elapsed_time = (t2.QuadPart - t1.QuadPart) / (double)frequency.QuadPart;
    min = 50.0 * (r1 < r2 ? r1 : r2) / 10000000.0;
    max = 50.0 * (r1 < r2 ? r2 : r1) / 10000000.0;

    /* Windows tries to work around the timer resolution by sometimes
     * returning early such that the total may be a bit less than requested!
     * Also add an extra 1s to account for potential scheduling delays.
     */
    ok(0.7 * min <= elapsed_time && elapsed_time <= 1.0 + max,
       "got %f, expected between %f and %f\n", elapsed_time, min, max);
}

START_TEST(sync)
{
    HMODULE hmod;

    hmod = LoadLibraryA("kernel32.dll");
    pWaitOnAddress       = (void *)GetProcAddress(hmod, "WaitOnAddress");
    ok(!pWaitOnAddress, "expected only in kernelbase.dll\n");

    hmod = LoadLibraryA("kernelbase.dll");
    pWaitOnAddress       = (void *)GetProcAddress(hmod, "WaitOnAddress");
    pWakeByAddressAll    = (void *)GetProcAddress(hmod, "WakeByAddressAll");
    pWakeByAddressSingle = (void *)GetProcAddress(hmod, "WakeByAddressSingle");

    test_WaitOnAddress();
    test_Sleep();
}
