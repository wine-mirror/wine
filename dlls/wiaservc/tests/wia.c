/*
 * Unit test suite for WIA system
 *
 * Copyright 2015 Nikolay Sivov for CodeWeavers
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

#include <stdio.h>

#define COBJMACROS

#include "objbase.h"
#include "initguid.h"
#include "wia_lh.h"
#include "sti.h"
#include "wiadef.h"

#include "wine/test.h"

static IWiaDevMgr *devmanager;

static void test_EnumDeviceInfo(void)
{
    IEnumWIA_DEV_INFO *devenum;
    HRESULT hr;
    ULONG count;

    hr = IWiaDevMgr_EnumDeviceInfo(devmanager, WIA_DEVINFO_ENUM_LOCAL, NULL);
    ok(FAILED(hr), "got 0x%08lx\n", hr);

    hr = IWiaDevMgr_EnumDeviceInfo(devmanager, WIA_DEVINFO_ENUM_LOCAL, &devenum);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IEnumWIA_DEV_INFO_GetCount(devenum, NULL);
    ok(FAILED(hr), "got 0x%08lx\n", hr);

    count = 1000;
    hr = IEnumWIA_DEV_INFO_GetCount(devenum, &count);
    todo_wine
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(count != 1000, "got %lu\n", count);

    IEnumWIA_DEV_INFO_Release(devenum);
}

static void test_SelectDeviceDlg(void)
{
    HRESULT hr;
    IWiaItem *root;
    hr = IWiaDevMgr_SelectDeviceDlg(devmanager, NULL, StiDeviceTypeDefault, 0, NULL, NULL);
    todo_wine
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hr = IWiaDevMgr_SelectDeviceDlg(devmanager, NULL, StiDeviceTypeDefault, 0, NULL, &root);
    todo_wine
    ok(hr == S_OK || hr == WIA_S_NO_DEVICE_AVAILABLE, "got 0x%08lx\n", hr);
}

START_TEST(wia)
{
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, &IID_IWiaDevMgr, (void**)&devmanager);
    if (FAILED(hr)) {
        win_skip("Failed to create WiaDevMgr instance, 0x%08lx\n", hr);
        CoUninitialize();
        return;
    }

    test_EnumDeviceInfo();
    test_SelectDeviceDlg();

    IWiaDevMgr_Release(devmanager);
    CoUninitialize();
}
