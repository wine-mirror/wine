/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_System_Threading
#include "windows.system.threading.h"

#include "wine/test.h"

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static const WCHAR *threadpool_class_name = L"Windows.System.Threading.ThreadPool";

static void test_interfaces(void)
{
    IThreadPoolStatics *threadpool_statics;
    IActivationFactory *factory;
    HSTRING classid;
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    hr = WindowsCreateString(threadpool_class_name, wcslen(threadpool_class_name), &classid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = RoGetActivationFactory(classid, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Unexpected hr %#lx.\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(threadpool_class_name));
        return;
    }

    check_interface(factory, &IID_IInspectable, TRUE);
    check_interface(factory, &IID_IAgileObject, TRUE);
    check_interface(factory, &IID_IThreadPoolStatics, TRUE);

    hr = IActivationFactory_QueryInterface(factory, &IID_IThreadPoolStatics, (void **)&threadpool_statics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(threadpool_statics, &IID_IInspectable, TRUE);
    check_interface(threadpool_statics, &IID_IAgileObject, TRUE);

    IThreadPoolStatics_Release(threadpool_statics);
    IActivationFactory_Release(factory);
    WindowsDeleteString(classid);

    RoUninitialize();
}

struct work_item
{
    IWorkItemHandler IWorkItemHandler_iface;
    LONG refcount;
    HANDLE event;
};

static struct work_item * impl_from_IWorkItemHandler(IWorkItemHandler *iface)
{
    return CONTAINING_RECORD(iface, struct work_item, IWorkItemHandler_iface);
}

static HRESULT STDMETHODCALLTYPE work_item_QueryInterface(IWorkItemHandler *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IWorkItemHandler)
        || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IWorkItemHandler_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE work_item_AddRef(IWorkItemHandler *iface)
{
    struct work_item *item = impl_from_IWorkItemHandler(iface);
    return InterlockedIncrement(&item->refcount);
}

static ULONG STDMETHODCALLTYPE work_item_Release(IWorkItemHandler *iface)
{
    struct work_item *item = impl_from_IWorkItemHandler(iface);
    ULONG refcount = InterlockedDecrement(&item->refcount);

    if (!refcount)
    {
        CloseHandle(item->event);
        free(item);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE work_item_Invoke(IWorkItemHandler *iface, IAsyncAction *action)
{
    struct work_item *item = impl_from_IWorkItemHandler(iface);
    IAsyncActionCompletedHandler *handler;
    IAsyncInfo *async_info;
    AsyncStatus status;
    HRESULT hr;

    hr = IAsyncAction_QueryInterface(action, &IID_IAsyncInfo, (void **)&async_info);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IAsyncInfo_get_Status(async_info, &status);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) ok(status == Started, "Unexpected status %d.\n", status);

    hr = IAsyncInfo_Cancel(async_info);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IAsyncInfo_Release(async_info);

    hr = IAsyncAction_GetResults(action);
    todo_wine
    ok(hr == E_ILLEGAL_METHOD_CALL, "Unexpected hr %#lx.\n", hr);

    handler = (void *)0xdeadbeef;
    hr = IAsyncAction_get_Completed(action, &handler);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!handler, "Unexpected pointer %p.\n", handler);

    hr = IAsyncAction_put_Completed(action, NULL);
    todo_wine
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    SetEvent(item->event);

    return S_OK;
}

static const IWorkItemHandlerVtbl work_item_vtbl =
{
    work_item_QueryInterface,
    work_item_AddRef,
    work_item_Release,
    work_item_Invoke,
};

static HRESULT create_work_item(IWorkItemHandler **item)
{
    struct work_item *object;

    *item = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IWorkItemHandler_iface.lpVtbl = &work_item_vtbl;
    object->refcount = 1;
    object->event = CreateEventW(NULL, FALSE, FALSE, NULL);

    *item = &object->IWorkItemHandler_iface;

    return S_OK;
}

static void test_RunAsync(void)
{
    IActivationFactory *factory = NULL;
    IThreadPoolStatics *threadpool_statics;
    IWorkItemHandler *item_iface;
    struct work_item *item;
    IAsyncAction *action;
    HSTRING classid;
    HRESULT hr;
    DWORD ret;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);

    hr = WindowsCreateString(threadpool_class_name, wcslen(threadpool_class_name), &classid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = RoGetActivationFactory(classid, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Unexpected hr %#lx.\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
        return;

    hr = IActivationFactory_QueryInterface(factory, &IID_IThreadPoolStatics, (void **)&threadpool_statics);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IThreadPoolStatics_RunAsync(threadpool_statics, NULL, &action);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = create_work_item(&item_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    item = impl_from_IWorkItemHandler(item_iface);

    hr = IThreadPoolStatics_RunWithPriorityAsync(threadpool_statics, item_iface, WorkItemPriority_High + 1, &action);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IThreadPoolStatics_RunAsync(threadpool_statics, item_iface, &action);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(item->event, 1000);
    ok(!ret, "Unexpected wait result %lu.\n", ret);

    check_interface(action, &IID_IAsyncAction, TRUE);
    check_interface(action, &IID_IAsyncInfo, TRUE);
    check_interface(action, &IID_IInspectable, TRUE);

    IAsyncAction_Release(action);

    hr = IThreadPoolStatics_RunWithPriorityAsync(threadpool_statics, item_iface, WorkItemPriority_Normal, &action);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(item->event, 1000);
    ok(!ret, "Unexpected wait result %lu.\n", ret);
    IAsyncAction_Release(action);

    hr = IThreadPoolStatics_RunWithPriorityAndOptionsAsync(threadpool_statics, item_iface, WorkItemPriority_Normal,
            WorkItemOptions_TimeSliced, &action);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(item->event, 1000);
    ok(!ret, "Unexpected wait result %lu.\n", ret);
    IAsyncAction_Release(action);

    hr = IThreadPoolStatics_RunWithPriorityAndOptionsAsync(threadpool_statics, item_iface, WorkItemPriority_Low,
            WorkItemOptions_TimeSliced, &action);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(item->event, 1000);
    ok(!ret, "Unexpected wait result %lu.\n", ret);
    IAsyncAction_Release(action);

    hr = IThreadPoolStatics_RunWithPriorityAndOptionsAsync(threadpool_statics, item_iface, WorkItemPriority_High,
            WorkItemOptions_TimeSliced, &action);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(item->event, 1000);
    ok(!ret, "Unexpected wait result %lu.\n", ret);
    IAsyncAction_Release(action);

    IWorkItemHandler_Release(item_iface);

    IThreadPoolStatics_Release(threadpool_statics);
    IActivationFactory_Release(factory);

    WindowsDeleteString(classid);

    RoUninitialize();
}

START_TEST(threadpool)
{
    test_interfaces();
    test_RunAsync();
}
