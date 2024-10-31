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

START_TEST(dataexchange)
{
    test_ICoreDragDropManagerStatics();
}
