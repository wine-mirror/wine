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

struct test_callback
{
    IRtwqAsyncCallback IRtwqAsyncCallback_iface;
    LONG refcount;
    HANDLE event;
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

    hr = RtwqPutWorkItem(RTWQ_CALLBACK_QUEUE_PRIVATE_MASK, 0, result);
    todo_wine
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    todo_wine
    ok(res == 0, "got %#lx\n", res);
    todo_wine
    ok(callback_result == result, "Expected result %p, got %p.\n", result, callback_result);

    hr = RtwqPutWorkItem(RTWQ_CALLBACK_QUEUE_PRIVATE_MASK & (RTWQ_CALLBACK_QUEUE_PRIVATE_MASK - 1), 0, result);
    todo_wine
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IRtwqAsyncCallback_iface, 100, &callback_result);
    todo_wine
    ok(res == 0, "got %#lx\n", res);
    todo_wine
    ok(callback_result == result, "Expected result %p, got %p.\n", result, callback_result);

    IRtwqAsyncResult_Release(result);

    IRtwqAsyncCallback_Release(&test_callback->IRtwqAsyncCallback_iface);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

START_TEST(rtworkq)
{
    test_platform_init();
    test_undefined_queue_id();
}
