/*
 * System clock unit tests
 *
 * Copyright (C) 2007 Alex Villacís Lasso
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

#define COBJMACROS
#include "dshow.h"
#include "wine/test.h"

static ULONGLONG (WINAPI *pGetTickCount64)(void);

static IReferenceClock *create_system_clock(void)
{
    IReferenceClock *filter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER,
            &IID_IReferenceClock, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    return filter;
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    IReferenceClock *clock = create_system_clock();
    ULONG ref;

    check_interface(clock, &IID_IReferenceClock, TRUE);
    check_interface(clock, &IID_IUnknown, TRUE);

    check_interface(clock, &IID_IDirectDraw, FALSE);

    ref = IReferenceClock_Release(clock);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_get_time(void)
{
    IReferenceClock *clock = create_system_clock();
    REFERENCE_TIME time1, time2;
    HRESULT hr;
    ULONG ref;

    hr = IReferenceClock_GetTime(clock, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IReferenceClock_GetTime(clock, &time1);
    if (pGetTickCount64)
        time2 = pGetTickCount64() * 10000;
    else
        time2 = GetTickCount() * 10000;
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time1 % 10000 == 0, "Expected no less than 1ms coarseness, but got time %s.\n",
            wine_dbgstr_longlong(time1));
    ok(abs(time1 - time2) < 20 * 10000, "Expected about %s, got %s.\n",
            wine_dbgstr_longlong(time2), wine_dbgstr_longlong(time1));

    hr = IReferenceClock_GetTime(clock, &time2);
    ok(hr == (time2 == time1 ? S_FALSE : S_OK), "Got hr %#x.\n", hr);

    Sleep(100);
    hr = IReferenceClock_GetTime(clock, &time2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(time2 - time1 > 98 * 10000, "Expected about %s, but got %s.\n",
            wine_dbgstr_longlong(time1 + 98 * 10000), wine_dbgstr_longlong(time2));

    ref = IReferenceClock_Release(clock);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_advise(void)
{
    IReferenceClock *clock = create_system_clock();
    HANDLE event, semaphore;
    REFERENCE_TIME current;
    DWORD_PTR cookie;
    unsigned int i;
    HRESULT hr;
    ULONG ref;

    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    semaphore = CreateSemaphoreA(NULL, 0, 10, NULL);

    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdviseTime(clock, -1000 * 10000, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(WaitForSingleObject(event, 480) == WAIT_TIMEOUT, "Event should not be signaled.\n");
    ok(!WaitForSingleObject(event, 40), "Event should be signaled.\n");

    hr = IReferenceClock_Unadvise(clock, cookie);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    ResetEvent(event);
    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#x.\n", hr);
    hr = IReferenceClock_AdviseTime(clock, current, 500 * 10000, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IReferenceClock_Unadvise(clock, cookie);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(WaitForSingleObject(event, 520) == WAIT_TIMEOUT, "Event should not be signaled.\n");

    ResetEvent(event);
    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#x.\n", hr);
    hr = IReferenceClock_AdviseTime(clock, current + 500 * 10000, 0, (HEVENT)event, &cookie);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(WaitForSingleObject(event, 480) == WAIT_TIMEOUT, "Event should not be signaled.\n");
    ok(!WaitForSingleObject(event, 40), "Event should be signaled.\n");

    hr = IReferenceClock_GetTime(clock, &current);
    ok(SUCCEEDED(hr), "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, 500 * 10000, (HSEMAPHORE)semaphore, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, 0, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, -500 * 10000, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, -500 * 10000, 1000 * 10000, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IReferenceClock_AdvisePeriodic(clock, current, 500 * 10000, (HSEMAPHORE)semaphore, &cookie);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!WaitForSingleObject(semaphore, 10), "Semaphore should be signaled.\n");
    for (i = 0; i < 5; ++i)
    {
        ok(WaitForSingleObject(semaphore, 480) == WAIT_TIMEOUT, "Semaphore should not be signaled.\n");
        ok(!WaitForSingleObject(semaphore, 40), "Semaphore should be signaled.\n");
    }

    hr = IReferenceClock_Unadvise(clock, cookie);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(WaitForSingleObject(semaphore, 520) == WAIT_TIMEOUT, "Semaphore should not be signaled.\n");

    CloseHandle(event);
    CloseHandle(semaphore);

    ref = IReferenceClock_Release(clock);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(systemclock)
{
    CoInitialize(NULL);

    pGetTickCount64 = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetTickCount64");

    test_interfaces();
    test_get_time();
    test_advise();

    CoUninitialize();
}
