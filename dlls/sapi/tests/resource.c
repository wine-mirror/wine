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

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %d, expected %d.\n", rc, ref);
}

static void test_interfaces(void)
{
    ISpResourceManager *resource_manager, *resource_manager2;
    IDispatch *dispatch;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpResourceManager, (void **)&resource_manager);
    ok(hr == S_OK, "Failed to create ISpeechVoice interface: %#x.\n", hr);
    EXPECT_REF(resource_manager, 1);

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpResourceManager, (void **)&resource_manager2);
    ok(hr == S_OK, "Failed to create ISpeechVoice interface: %#x.\n", hr);
    todo_wine ok(resource_manager2 == resource_manager, "Expected managers to match.\n");
    todo_wine EXPECT_REF(resource_manager2, 2);
    todo_wine EXPECT_REF(resource_manager, 2);
    if (resource_manager2 == resource_manager) ISpResourceManager_Release(resource_manager2);
    EXPECT_REF(resource_manager, 1);

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create IUnknown interface: %#x.\n", hr);
    todo_wine ok(unk == (IUnknown *)resource_manager, "Expected unk to match existing manager.\n");
    todo_wine EXPECT_REF(unk, 2);
    todo_wine EXPECT_REF(resource_manager, 2);
    if (unk == (IUnknown *)resource_manager) IUnknown_Release(unk);
    EXPECT_REF(resource_manager, 1);

    hr = CoCreateInstance(&CLSID_SpResourceManager, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDispatch, (void **)&dispatch);
    ok(hr == E_NOINTERFACE, "Succeeded to create IDispatch interface: %#x.\n", hr);
    ok(!dispatch, "Expected NULL dispatch, got %p.", dispatch);

    ISpResourceManager_Release(resource_manager);
}

START_TEST(resource)
{
    CoInitialize(NULL);
    test_interfaces();
    CoUninitialize();
}
