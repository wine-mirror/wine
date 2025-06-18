/*
 * Speech API (SAPI) resource manager tests.
 *
 * Copyright 2020 Gijs Vermeulen
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

#include "sapiddk.h"
#include "sperror.h"

#include "wine/test.h"

static void test_interfaces(void)
{
    ISpResourceManager *resource_manager, *resource_manager2;
    IDispatch *dispatch;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpResourceManager, (void **)&resource_manager);
    ok(hr == S_OK, "Failed to create ISpeechVoice interface: %#lx.\n", hr);
    ok(!!resource_manager, "Expected non-NULL resource manager.\n");

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpResourceManager, (void **)&resource_manager2);
    ok(hr == S_OK, "Failed to create ISpeechVoice interface: %#lx.\n", hr);
    ok(!!resource_manager2, "Expected non-NULL resource manager.\n");
    todo_wine ok(resource_manager2 == resource_manager, "Expected managers to match.\n");
    ISpResourceManager_Release(resource_manager2);

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create IUnknown interface: %#lx.\n", hr);
    ok(!!unk, "Expected non-NULL unk.\n");
    todo_wine ok(unk == (IUnknown *)resource_manager, "Expected unk to match existing manager.\n");
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDispatch, (void **)&dispatch);
    ok(hr == E_NOINTERFACE, "Succeeded to create IDispatch interface: %#lx.\n", hr);
    ok(!dispatch, "Expected NULL dispatch, got %p.", dispatch);

    ISpResourceManager_Release(resource_manager);
}

START_TEST(resource)
{
    CoInitialize(NULL);
    test_interfaces();
    CoUninitialize();
}
