/*
 *    Font related tests
 *
 * Copyright 2012, 2014-2017 Nikolay Sivov for CodeWeavers
 * Copyright 2014 Aric Stewart for CodeWeavers
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

#include "windows.h"
#include "initguid.h"
#include "msopc.h"

#include "wine/test.h"

static IOpcFactory *create_factory(void)
{
    IOpcFactory *factory = NULL;
    CoCreateInstance(&CLSID_OpcFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IOpcFactory, (void **)&factory);
    return factory;
}

static void test_package(void)
{
    IOpcPartSet *partset, *partset2;
    IOpcFactory *factory;
    IOpcPackage *package;
    HRESULT hr;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(SUCCEEDED(hr) || broken(hr == E_NOTIMPL) /* Vista */, "Failed to create a package, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        IOpcFactory_Release(factory);
        return;
    }

    hr = IOpcPackage_GetPartSet(package, &partset);
    ok(SUCCEEDED(hr), "Failed to create a part set, hr %#x.\n", hr);

    hr = IOpcPackage_GetPartSet(package, &partset2);
    ok(SUCCEEDED(hr), "Failed to create a part set, hr %#x.\n", hr);
    ok(partset == partset2, "Expected same part set instance.\n");

    IOpcPackage_Release(package);

    IOpcFactory_Release(factory);
}

START_TEST(opcservices)
{
    IOpcFactory *factory;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(SUCCEEDED(hr), "Failed to initialize COM, hr %#x.\n", hr);

    if (!(factory = create_factory())) {
        win_skip("Failed to create IOpcFactory factory.\n");
        CoUninitialize();
        return;
    }

    test_package();

    IOpcFactory_Release(factory);

    CoUninitialize();
}
