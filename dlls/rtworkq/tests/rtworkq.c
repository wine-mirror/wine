/*
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "rtworkq.h"

#include "wine/test.h"

/* Should be kept in sync with corresponding MFASYNC_CALLBACK_ constants. */
enum rtwq_callback_queue_id
{
    RTWQ_CALLBACK_QUEUE_UNDEFINED     = 0x00000000,
    RTWQ_CALLBACK_QUEUE_STANDARD      = 0x00000001,
    RTWQ_CALLBACK_QUEUE_RT            = 0x00000002,
    RTWQ_CALLBACK_QUEUE_IO            = 0x00000003,
    RTWQ_CALLBACK_QUEUE_TIMER         = 0x00000004,
    RTWQ_CALLBACK_QUEUE_MULTITHREADED = 0x00000005,
    RTWQ_CALLBACK_QUEUE_LONG_FUNCTION = 0x00000007,
    RTWQ_CALLBACK_QUEUE_PRIVATE_MASK  = 0xffff0000,
    RTWQ_CALLBACK_QUEUE_ALL           = 0xffffffff,
};

#define check_platform_lock_count(a) check_platform_lock_count_(__LINE__, a)
static void check_platform_lock_count_(unsigned int line, unsigned int expected)
{
    int i, count = 0;
    BOOL unexpected;
    DWORD queue;
    HRESULT hr;

    for (;;)
    {
        if (FAILED(hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue)))
        {
            unexpected = hr != RTWQ_E_SHUTDOWN;
            break;
        }
        RtwqUnlockWorkQueue(queue);

        hr = RtwqUnlockPlatform();
        if ((unexpected = FAILED(hr)))
            break;

        ++count;
    }

    for (i = 0; i < count; ++i)
        RtwqLockPlatform();

    if (unexpected)
        count = -1;

    ok_(__FILE__, line)(count == expected, "Unexpected lock count %d.\n", count);
}

struct test_callback
{
    IRtwqAsyncCallback IRtwqAsyncCallback_iface;
    LONG refcount;
    HANDLE event;
    UINT sleep_ms;
    IRtwqAsyncResult *result;
};

static struct test_callback *impl_from_IRtwqAsyncCallback(IRtwqAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_callback, IRtwqAsyncCallback_iface);
}

static HRESULT WINAPI testcallback_QueryInterface(IRtwqAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IRtwqAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IRtwqAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testcallback_AddRef(IRtwqAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IRtwqAsyncCallback(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI testcallback_Release(IRtwqAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IRtwqAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    if (!refcount)
    {
        CloseHandle(callback->event);
        free(callback);
    }

    return refcount;
}

static HRESULT WINAPI testcallback_GetParameters(IRtwqAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    ok(flags != NULL && queue != NULL, "Unexpected arguments.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testcallback_Invoke(IRtwqAsyncCallback *iface, IRtwqAsyncResult *result)
{
    struct test_callback *callback = impl_from_IRtwqAsyncCallback(iface);

    if (callback->sleep_ms)
        Sleep(callback->sleep_ms);

    callback->result = result;
    SetEvent(callback->event);

    return S_OK;
}

static const IRtwqAsyncCallbackVtbl testcallbackvtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    testcallback_Invoke,
};

static struct test_callback * create_test_callback(void)
{
    struct test_callback *callback = calloc(1, sizeof(*callback));

    callback->IRtwqAsyncCallback_iface.lpVtbl = &testcallbackvtbl;
    callback->refcount = 1;
    callback->event = CreateEventA(NULL, FALSE, FALSE, NULL);

    return callback;
}

static DWORD wait_async_callback_result(IRtwqAsyncCallback *iface, DWORD timeout, IRtwqAsyncResult **result)
{
    struct test_callback *callback = impl_from_IRtwqAsyncCallback(iface);
    DWORD res = WaitForSingleObject(callback->event, timeout);

    *result = callback->result;
    callback->result = NULL;

    return res;
}

static void test_platform_init(void)
{
    APTTYPEQUALIFIER qualifier;
    APTTYPE apttype;
    HRESULT hr;

    /* Startup initializes MTA. */
    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(apttype == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
                "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    /* Try with STA initialized before startup. */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    CoUninitialize();

    /* Startup -> init main STA -> uninitialize -> shutdown */
    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(apttype == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
                "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    CoUninitialize();

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_undefined_queue_id(void)
{
    IRtwqAsyncResult *result, *callback_result;
    struct test_callback *test_callback;
    HRESULT hr;
    DWORD res;

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    test_callback = create_test_callback();

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = RtwqPutWorkItem(RTWQ_CALLBACK_QUEUE_UNDEFINED, 0, result);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    ok(callback_result == result, "Expected result %p, got %p.\n", result, callback_result);

    hr = RtwqPutWorkItem(0xffff, 0, result);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    ok(callback_result == result, "Expected result %p, got %p.\n", result, callback_result);

    hr = RtwqPutWorkItem(0x4000, 0, result);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    ok(callback_result == result, "Expected result %p, got %p.\n", result, callback_result);

    hr = RtwqPutWorkItem(0x10000, 0, result);
    ok(hr == RTWQ_E_INVALID_WORKQUEUE, "got %#lx\n", hr);

    IRtwqAsyncResult_Release(result);

    IRtwqAsyncCallback_Release(&test_callback->IRtwqAsyncCallback_iface);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_work_queue(void)
{
    IRtwqAsyncResult *result, *result2, *callback_result;
    struct test_callback *test_callback;
    RTWQWORKITEM_KEY key, key2;
    DWORD res, queue;
    LONG refcount;
    HRESULT hr;

    hr = RtwqLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue);
    ok(hr == RTWQ_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = RtwqUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);

    test_callback = create_test_callback();

    /* Create async results without startup. */
    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    IRtwqAsyncResult_Release(result);
    IRtwqAsyncResult_Release(result2);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    /* Before startup the platform lock count does not track the maximum AsyncResult count. */
    check_platform_lock_count(1);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    /* Startup only locks once. */
    check_platform_lock_count(1);
    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    /* Platform locked by the AsyncResult object. */
    check_platform_lock_count(2);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    check_platform_lock_count(3);

    IRtwqAsyncResult_Release(result);
    IRtwqAsyncResult_Release(result2);
    /* Platform lock count for AsyncResult objects does not decrease
     * unless the platform is in shutdown state. */
    check_platform_lock_count(3);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    /* Platform lock count tracks the maximum AsyncResult count plus one for startup. */
    check_platform_lock_count(3);

    hr = RtwqPutWorkItem(RTWQ_CALLBACK_QUEUE_STANDARD, 0, result);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    /* TODO: Wine often has a release call pending in another thread at this point. */
    refcount = IRtwqAsyncResult_Release(result);
    flaky_wine
    ok(!refcount, "Unexpected refcount %ld.\n", refcount);
    check_platform_lock_count(3);

    hr = RtwqLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    hr = RtwqLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    check_platform_lock_count(5);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* Platform is in shutdown state if either the lock count or the startup count is <= 0. */
    hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue);
    ok(hr == RTWQ_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    /* Platform can be unlocked after shutdown. */
    hr = RtwqUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    /* Platform locks for AsyncResult objects were released on shutdown, but the explicit lock was not. */
    check_platform_lock_count(2);
    hr = RtwqUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = RtwqUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);
    /* Zero lock count. */
    hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue);
    ok(hr == RTWQ_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = RtwqUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);
    /* Negative lock count. */
    hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue);
    ok(hr == RTWQ_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = RtwqLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    hr = RtwqLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    check_platform_lock_count(2);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* Release an AsyncResult object after shutdown. Platform lock count tracks the AsyncResult
     * count. It's not possible to show if unlock occurs immedately or on the next startup. */
    IRtwqAsyncResult_Release(result);
    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    check_platform_lock_count(2);
    /* Release an AsyncResult object after shutdown and startup. */
    IRtwqAsyncResult_Release(result);
    check_platform_lock_count(2);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqScheduleWorkItem(result, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    check_platform_lock_count(2);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqScheduleWorkItem(result2, -5000, &key2);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    check_platform_lock_count(3);

    hr = RtwqCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);
    hr = RtwqCancelWorkItem(key2);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);
    IRtwqAsyncResult_Release(result);
    IRtwqAsyncResult_Release(result2);
    check_platform_lock_count(3);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqScheduleWorkItem(result, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    check_platform_lock_count(3);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    /* Shutdown while a scheduled item is pending leaks the AsyncResult. */
    refcount = IRtwqAsyncResult_Release(result);
    ok(refcount == 1, "Unexpected refcount %ld.\n", refcount);
    IRtwqAsyncResult_Release(result);

    hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue);
    ok(hr == RTWQ_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = RtwqCancelWorkItem(key);
    ok(hr == RTWQ_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 0, &callback_result);
    ok(res == WAIT_TIMEOUT, "got res %#lx\n", res);

    IRtwqAsyncCallback_Release(&test_callback->IRtwqAsyncCallback_iface);
}

static void test_scheduled_items(void)
{
    struct test_callback *test_callback;
    RTWQWORKITEM_KEY key, key2;
    IRtwqAsyncResult *result;
    ULONG refcount;
    HRESULT hr;

    test_callback = create_test_callback();

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqScheduleWorkItem(result, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    IRtwqAsyncResult_Release(result);

    hr = RtwqCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    refcount = IRtwqAsyncCallback_Release(&test_callback->IRtwqAsyncCallback_iface);
    ok(refcount == 0, "Unexpected refcount %lu.\n", refcount);

    hr = RtwqCancelWorkItem(key);
    ok(hr == RTWQ_E_NOT_FOUND || broken(hr == S_OK) /* < win10 */, "Unexpected hr %#lx.\n", hr);

    test_callback = create_test_callback();

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);

    hr = RtwqPutWaitingWorkItem(NULL, 0, result, &key);
    ok(hr == S_OK, "Failed to add waiting item, hr %#lx.\n", hr);

    hr = RtwqPutWaitingWorkItem(NULL, 0, result, &key2);
    ok(hr == S_OK, "Failed to add waiting item, hr %#lx.\n", hr);

    hr = RtwqCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    hr = RtwqCancelWorkItem(key2);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    Sleep(20);
    /* One or two callback invocations with RTWQ_E_OPERATION_CANCELLED may have been
     * pending when RtwqShutdown() was called. Release depends upon their execution. */
    refcount = IRtwqAsyncResult_Release(result);
    ok(refcount == 0, "Unexpected refcount %lu.\n", refcount);

    IRtwqAsyncCallback_Release(&test_callback->IRtwqAsyncCallback_iface);
}

static void test_queue_shutdown(void)
{
    IRtwqAsyncResult *result, *result2, *callback_result;
    struct test_callback *test_callback, *test_callback2;
    DWORD res, queue;
    LONG refcount;
    HRESULT hr;

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    test_callback = create_test_callback();
    test_callback2 = create_test_callback();
    test_callback->sleep_ms = 10;
    test_callback2->sleep_ms = 10;

    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqCreateAsyncResult(NULL, &test_callback2->IRtwqAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqPutWorkItem(RTWQ_CALLBACK_QUEUE_STANDARD, 0, result);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = RtwqPutWorkItem(RTWQ_CALLBACK_QUEUE_STANDARD, 0, result2);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* At least the second item will be delayed until after shutdown. Execution still occurs after shutdown. */
    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    res = wait_async_callback_result(&test_callback2->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);

    refcount = IRtwqAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %ld.\n", refcount);
    refcount = IRtwqAsyncResult_Release(result2);
    ok(!refcount, "Unexpected refcount %ld.\n", refcount);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = RtwqAllocateWorkQueue(RTWQ_STANDARD_WORKQUEUE, &queue);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = RtwqCreateAsyncResult(NULL, &test_callback->IRtwqAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqCreateAsyncResult(NULL, &test_callback2->IRtwqAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = RtwqPutWorkItem(queue, 0, result);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = RtwqPutWorkItem(queue, 0, result2);
    ok(hr == S_OK, "got %#lx\n", hr);

    /* Execution still occurs after unlock. */
    RtwqUnlockWorkQueue(queue);

    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    res = wait_async_callback_result(&test_callback2->IRtwqAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);

    refcount = IRtwqAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %ld.\n", refcount);
    /* An internal reference to result2 may still be held here even in Windows. */
    IRtwqAsyncResult_Release(result2);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IRtwqAsyncCallback_Release(&test_callback->IRtwqAsyncCallback_iface);
    IRtwqAsyncCallback_Release(&test_callback2->IRtwqAsyncCallback_iface);
}

START_TEST(rtworkq)
{
    test_platform_init();
    test_undefined_queue_id();
    test_work_queue();
    test_scheduled_items();
    test_queue_shutdown();
}
