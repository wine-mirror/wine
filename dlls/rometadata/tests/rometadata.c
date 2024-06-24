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
#include <windows.h>
#include "initguid.h"
#include "cor.h"
#include "roapi.h"
#include "rometadata.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

static void test_MetaDataGetDispenser(void)
{
    IMetaDataDispenserEx *dispenser_ex;
    IMetaDataDispenser *dispenser;
    IUnknown *unknown;
    HRESULT hr;

    /* Invalid parameters */
    hr = MetaDataGetDispenser(&CLSID_NULL, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_NULL, (void **)&dispenser);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    /* Normal calls */
    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IUnknown, (void **)&unknown);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unknown);

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IMetaDataDispenser_Release(dispenser);

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenserEx, (void **)&dispenser_ex);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IMetaDataDispenserEx_Release(dispenser_ex);
}

START_TEST(rometadata)
{
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    test_MetaDataGetDispenser();

    RoUninitialize();
}
