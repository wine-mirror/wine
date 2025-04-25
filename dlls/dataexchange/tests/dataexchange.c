/*
 * Copyright 2024 Zhiyi Zhang for CodeWeavers
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
#include "dragdropinterop.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Networking_Connectivity
#include "windows.networking.connectivity.h"
#define WIDL_using_Windows_ApplicationModel_DataTransfer_DragDrop_Core
#include "windows.applicationmodel.datatransfer.dragdrop.core.h"

#include "wine/test.h"

static void test_ICoreDragDropManagerStatics(void)
{
    static const WCHAR *class_name = L"Windows.ApplicationModel.DataTransfer.DragDrop.Core.CoreDragDropManager";
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    ICoreDragDropManagerStatics *statics = NULL;
    ICoreDragDropManager *manager = NULL;
    IActivationFactory *factory = NULL;
    IDragDropManagerInterop *interop;
    HSTRING str;
    HRESULT hr;
    HWND hwnd;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = WindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);
    WindowsDeleteString(str);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        RoUninitialize();
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_ICoreDragDropManagerStatics, (void **)&statics);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ICoreDragDropManagerStatics_QueryInterface(statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tmp_inspectable == inspectable, "QueryInterface IID_IInspectable returned %p, expected %p.\n",
       tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);
    IInspectable_Release(inspectable);

    hr = ICoreDragDropManagerStatics_QueryInterface(statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tmp_agile_object == agile_object, "QueryInterface IID_IAgileObject returned %p, expected %p.\n",
       tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);
    IAgileObject_Release(agile_object);

    /* Parameter checks */
    if (0) /* Crash on Windows */
    {
    hr = ICoreDragDropManagerStatics_GetForCurrentView(statics, NULL);
    ok(hr == E_POINTER, "Got unexpected hr %#lx.\n", hr);

    /* Yes, this one crashes on Windows as well */
    hr = ICoreDragDropManagerStatics_GetForCurrentView(statics, &manager);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!!manager, "Got unexpected pointer.\n");
    ICoreDragDropManager_Release(manager);
    }

    /* IDragDropManagerInterop tests */
    hr = ICoreDragDropManagerStatics_QueryInterface(statics, &IID_IDragDropManagerInterop, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDragDropManagerInterop_GetForWindow(interop, NULL, &IID_IUnknown, (void **)&manager);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Got unexpected hr %#lx.\n", hr);

    hr = IDragDropManagerInterop_GetForWindow(interop, GetDesktopWindow(), &IID_IUnknown, (void **)&manager);
    ok(hr == E_ACCESSDENIED, "Got unexpected hr %#lx.\n", hr);

    hwnd = CreateWindowA("static", "test", WS_POPUP | WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(!!hwnd, "Failed to create a test window, error %lu.\n", GetLastError());

    hr = IDragDropManagerInterop_GetForWindow(interop, hwnd, &IID_IUnknown, (void **)&manager);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ICoreDragDropManager_Release(manager);

    hr = IDragDropManagerInterop_GetForWindow(interop, hwnd, &IID_ICoreDragDropManager, (void **)&manager);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ICoreDragDropManager_Release(manager);

    DestroyWindow(hwnd);
    IDragDropManagerInterop_Release(interop);
    ICoreDragDropManagerStatics_Release(statics);
    IActivationFactory_Release(factory);
    RoUninitialize();
}

struct target_requested_handler
{
    ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs
    ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs_iface;
};

static HRESULT WINAPI target_requested_handler_QueryInterface(ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs *iface,
                                                              REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IAgileObject)
        || IsEqualGUID(iid, &IID_ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI target_requested_handler_AddRef(ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs *iface)
{
    return 2;
}

static ULONG WINAPI target_requested_handler_Release(ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs *iface)
{
    return 1;
}

static HRESULT WINAPI target_requested_handler_Invoke(ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs *iface,
                                                      ICoreDragDropManager *sender, ICoreDropOperationTargetRequestedEventArgs *args)
{
    ok(0, "Unexpected call.\n");
    return S_OK;
}

static const ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgsVtbl target_requested_handler_vtbl =
{
    target_requested_handler_QueryInterface,
    target_requested_handler_AddRef,
    target_requested_handler_Release,
    target_requested_handler_Invoke,
};

static struct target_requested_handler target_requested_handler_added = {{&target_requested_handler_vtbl}};

static void test_ICoreDragDropManager(void)
{
    static const WCHAR *class_name = L"Windows.ApplicationModel.DataTransfer.DragDrop.Core.CoreDragDropManager";
    ICoreDragDropManagerStatics *statics = NULL;
    ICoreDragDropManager *manager = NULL;
    IActivationFactory *factory = NULL;
    IDragDropManagerInterop *interop;
    EventRegistrationToken token;
    boolean enabled;
    HSTRING str;
    HRESULT hr;
    HWND hwnd;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = WindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);
    WindowsDeleteString(str);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        RoUninitialize();
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_ICoreDragDropManagerStatics, (void **)&statics);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ICoreDragDropManagerStatics_QueryInterface(statics, &IID_IDragDropManagerInterop, (void **)&interop);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hwnd = CreateWindowA("static", "test", WS_POPUP | WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(!!hwnd, "Failed to create a test window, error %lu.\n", GetLastError());

    hr = IDragDropManagerInterop_GetForWindow(interop, hwnd, &IID_ICoreDragDropManager, (void **)&manager);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    token.value = 0;
    hr = ICoreDragDropManager_add_TargetRequested(manager, &target_requested_handler_added.ITypedEventHandler_CoreDragDropManager_CoreDropOperationTargetRequestedEventArgs_iface,
                                                  &token);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(token.value != 0, "Got unexpected hr %#lx.\n", hr);

    token.value++;
    hr = ICoreDragDropManager_remove_TargetRequested(manager, token);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    token.value--;

    hr = ICoreDragDropManager_remove_TargetRequested(manager, token);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    enabled = TRUE;
    hr = ICoreDragDropManager_get_AreConcurrentOperationsEnabled(manager, &enabled);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    todo_wine
    ok(enabled == FALSE, "Got unexpected state.\n");

    enabled = TRUE;
    hr = ICoreDragDropManager_put_AreConcurrentOperationsEnabled(manager, enabled);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ICoreDragDropManager_get_AreConcurrentOperationsEnabled(manager, &enabled);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(enabled == TRUE, "Got unexpected state.\n");

    ICoreDragDropManager_Release(manager);
    DestroyWindow(hwnd);
    IDragDropManagerInterop_Release(interop);
    ICoreDragDropManagerStatics_Release(statics);
    IActivationFactory_Release(factory);
    RoUninitialize();
}

START_TEST(dataexchange)
{
    test_ICoreDragDropManagerStatics();
    test_ICoreDragDropManager();
}
