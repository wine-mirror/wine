/*
 * Copyright 2021 Jactry Zeng for CodeWeavers
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
#include "comsvcs.h"

#include "wine/test.h"

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    ok(0, "Unexpected call.\n");
    return 1;
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    ok(0, "Unexpected call.\n");
    return 0;
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_interfaces(void)
{
    ISharedPropertyGroupManager *manager, *manager1;
    ULONG refcount, expected_refcount;
    IDispatch *dispatch;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SharedPropertyGroupManager, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_ISharedPropertyGroupManager, (void **)&manager);
    ok(hr == CLASS_E_NOAGGREGATION, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SharedPropertyGroupManager, NULL, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    expected_refcount = get_refcount(unk) + 1;
    hr = IUnknown_QueryInterface(unk, &IID_ISharedPropertyGroupManager, (void **)&manager);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    refcount = get_refcount(unk);
    ok(refcount == expected_refcount, "Got refcount: %lu, expected %lu.\n", refcount, expected_refcount);

    expected_refcount = get_refcount(manager) + 1;
    hr = CoCreateInstance(&CLSID_SharedPropertyGroupManager, NULL, CLSCTX_INPROC_SERVER,
            &IID_ISharedPropertyGroupManager, (void **)&manager1);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(manager1 == manager, "Got wrong pointer: %p.\n", manager1);
    refcount = get_refcount(manager1);
    ok(refcount == expected_refcount, "Got refcount: %lu, expected %lu.\n", refcount, expected_refcount);
    refcount = get_refcount(manager);
    ok(refcount == expected_refcount, "Got refcount: %lu, expected %lu.\n", refcount, expected_refcount);
    ISharedPropertyGroupManager_Release(manager1);

    hr = IUnknown_QueryInterface(unk, &IID_IDispatch, (void **)&dispatch);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    refcount = get_refcount(dispatch);
    ok(refcount == expected_refcount, "Got refcount: %lu, expected %lu.\n", refcount, expected_refcount);
    refcount = get_refcount(manager);
    ok(refcount == expected_refcount, "Got refcount: %lu, expected %lu.\n", refcount, expected_refcount);

    IDispatch_Release(dispatch);
    IUnknown_Release(unk);
    ISharedPropertyGroupManager_Release(manager);
}

START_TEST(property)
{
    CoInitialize(NULL);

    test_interfaces();

    CoUninitialize();
}
