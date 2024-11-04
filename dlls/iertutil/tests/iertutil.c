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
#include "initguid.h"
#include "roapi.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "wine/test.h"

#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"

static void test_IUriRuntimeClassFactory(void)
{
    static const WCHAR *class_name = L"Windows.Foundation.Uri";
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IUriRuntimeClassFactory *uri_factory = NULL;
    IActivationFactory *factory = NULL;
    IUriRuntimeClass *uri_class = NULL;
    HSTRING str, uri;
    HRESULT hr;

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

    hr = IActivationFactory_QueryInterface(factory, &IID_IUriRuntimeClassFactory, (void **)&uri_factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IUriRuntimeClassFactory_QueryInterface(uri_factory, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tmp_inspectable == inspectable, "QueryInterface IID_IInspectable returned %p, expected %p.\n",
       tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);
    IInspectable_Release(inspectable);

    hr = IUriRuntimeClassFactory_QueryInterface(uri_factory, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(tmp_agile_object == agile_object, "QueryInterface IID_IAgileObject returned %p, expected %p.\n",
       tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);
    IAgileObject_Release(agile_object);

    /* Test IUriRuntimeClassFactory_CreateUri() */
    hr = WindowsCreateString(L"https://www.winehq.org", wcslen(L"https://www.winehq.org"), &uri);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IUriRuntimeClassFactory_CreateUri(uri_factory, NULL, &uri_class);
    ok(hr == E_POINTER, "Got unexpected hr %#lx.\n", hr);

    hr = IUriRuntimeClassFactory_CreateUri(uri_factory, uri, &uri_class);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUriRuntimeClass_Release(uri_class);

    WindowsDeleteString(uri);

    IUriRuntimeClassFactory_Release(uri_factory);
    IActivationFactory_Release(factory);
    RoUninitialize();
}

START_TEST(iertutil)
{
    test_IUriRuntimeClassFactory();
}
