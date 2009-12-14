/*
 * Copyright 2009 Maarten Lankhorst
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

#include "wine/test.h"

#define CINTERFACE
#define COBJMACROS

#include "initguid.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"

/* Some of the QueryInterface tests are really just to check if I got the IID's right :) */

/* IMMDeviceCollection appears to have no QueryInterface method and instead forwards to mme */
static void test_collection(IMMDeviceEnumerator *mme, IMMDeviceCollection *col)
{
    IMMDeviceCollection *col2;
    IMMDeviceEnumerator *mme2;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;
    UINT numdev;
    IMMDevice *dev;

    ref = IUnknown_AddRef(col);
    IUnknown_Release(col);
    ok(ref == 2, "Invalid reference count %u on collection\n", ref);

    hr = IUnknown_QueryInterface(col, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Cannot query for IID_IUnknown: 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok((LONG_PTR)col == (LONG_PTR)unk, "Pointers are not identical %p/%p/%p\n", col, unk, mme);
        IUnknown_Release(unk);
    }

    hr = IUnknown_QueryInterface(col, &IID_IMMDeviceCollection, (void**)&col2);
    ok(hr == S_OK, "Cannot query for IID_IMMDeviceCollection: 0x%08x\n", hr);
    if (hr == S_OK)
        IUnknown_Release(col2);

    hr = IUnknown_QueryInterface(col, &IID_IMMDeviceEnumerator, (void**)&mme2);
    ok(hr == E_NOINTERFACE, "Query for IID_IMMDeviceEnumerator returned: 0x%08x\n", hr);
    if (hr == S_OK)
        IUnknown_Release(mme2);

    hr = IMMDeviceCollection_GetCount(col, NULL);
    ok(hr == E_POINTER, "GetCount returned 0x%08x\n", hr);

    hr = IMMDeviceCollection_GetCount(col, &numdev);
    ok(hr == S_OK, "GetCount returned 0x%08x\n", hr);

    dev = (void*)(LONG_PTR)0x12345678;
    hr = IMMDeviceCollection_Item(col, numdev, &dev);
    ok(hr == E_INVALIDARG, "Asking for too high device returned 0x%08x\n", hr);
    ok(dev == NULL, "Returned non-null device\n");

    if (!numdev) return;
    hr = IMMDeviceCollection_Item(col, 0, NULL);
    ok(hr == E_POINTER, "Query with null pointer returned 0x%08x\n", hr);

    hr = IMMDeviceCollection_Item(col, 0, &dev);
    ok(hr == S_OK, "Valid Item returned 0x%08x\n", hr);
    ok(dev != NULL, "Device is null!\n");
    if (dev) IUnknown_Release(dev);
}

START_TEST(mmdevenum)
{
    HRESULT hr;
    IUnknown *unk = NULL;
    IMMDeviceEnumerator *mme;
    ULONG ref;
    IMMDeviceCollection *col;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        return;
    }

    /* Odd behavior.. bug? */
    ref = IUnknown_AddRef(mme);
    ok(ref == 3, "Invalid reference count after incrementing: %u\n", ref);
    IUnknown_Release(mme);

    hr = IUnknown_QueryInterface(mme, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "returned 0x%08x\n", hr);
    if (hr != S_OK) return;

    ok( (LONG_PTR)mme == (LONG_PTR)unk, "Pointers are unequal %p/%p\n", unk, mme);
    IUnknown_Release(unk);

    col = (void*)(LONG_PTR)0x12345678;
    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, 0xffff, DEVICE_STATEMASK_ALL, &col);
    ok(hr == E_INVALIDARG, "Setting invalid data flow returned 0x%08x\n", hr);
    ok(col == NULL, "Collection pointer non-null on failure\n");

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL+1, &col);
    ok(hr == E_INVALIDARG, "Setting invalid mask returned 0x%08x\n", hr);

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL, NULL);
    ok(hr == E_POINTER, "Invalid pointer returned: 0x%08x\n", hr);

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL, &col);
    ok(hr == S_OK, "Valid EnumAudioEndpoints returned 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(!!col, "Returned null pointer\n");
        if (col)
            test_collection(mme, col);
    }

    IUnknown_Release(mme);
}
