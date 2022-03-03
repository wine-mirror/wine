/*
 * Audio capture filter unit tests
 *
 * Copyright 2018 Zebediah Figura
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
#include "dshow.h"
#include "wine/test.h"

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(IBaseFilter *filter)
{
    todo_wine check_interface(filter, &IID_IAMFilterMiscFlags, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IPersistPropertyBag, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);

    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IKsPropertySet, FALSE);
    check_interface(filter, &IID_IMediaPosition, FALSE);
    check_interface(filter, &IID_IMediaSeeking, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IQualityControl, FALSE);
    check_interface(filter, &IID_IQualProp, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVideoWindow, FALSE);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IBaseFilter)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(void)
{
    IBaseFilter *filter, *filter2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_AudioRecord, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_AudioRecord, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(filter2 == (IBaseFilter *)0xdeadbeef, "Got unexpected IBaseFilter %p.\n", filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %ld.\n", outer_ref);
}

static HRESULT WINAPI property_bag_QueryInterface(IPropertyBag *iface, REFIID iid, void **out)
{
    ok(0, "Unexpected call (iid %s).\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI property_bag_AddRef(IPropertyBag *iface)
{
    ok(0, "Unexpected call.\n");
    return 2;
}

static ULONG WINAPI property_bag_Release(IPropertyBag *iface)
{
    ok(0, "Unexpected call.\n");
    return 1;
}
static int ppb_id;
static unsigned int ppb_got_read;

static HRESULT WINAPI property_bag_Read(IPropertyBag *iface, const WCHAR *name, VARIANT *var, IErrorLog *log)
{
    if (!lstrcmpW(name, L"UseMixer"))
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    todo_wine ok(!lstrcmpW(name, L"WaveInId"), "Got unexpected name %s.\n", wine_dbgstr_w(name));
    ok(V_VT(var) == VT_I4, "Got unexpected type %u.\n", V_VT(var));
    ok(!log, "Got unexpected error log %p.\n", log);
    ppb_got_read++;
    return S_OK;
}

static HRESULT WINAPI property_bag_Write(IPropertyBag *iface, const WCHAR *name, VARIANT *var)
{
    ok(0, "Unexpected call (name %s).\n", wine_dbgstr_w(name));
    return E_FAIL;
}

static const IPropertyBagVtbl property_bag_vtbl =
{
    property_bag_QueryInterface,
    property_bag_AddRef,
    property_bag_Release,
    property_bag_Read,
    property_bag_Write,
};

static void test_property_bag(IMoniker *mon)
{
    IPropertyBag property_bag = {&property_bag_vtbl};
    IPropertyBag *devenum_bag;
    IPersistPropertyBag *ppb;
    VARIANT var;
    HRESULT hr;
    ULONG ref;

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&devenum_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(devenum_bag, L"WaveInId", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ppb_id = V_I4(&var);

    hr = CoCreateInstance(&CLSID_AudioRecord, NULL, CLSCTX_INPROC_SERVER,
            &IID_IPersistPropertyBag, (void **)&ppb);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IPersistPropertyBag_InitNew(ppb);
    todo_wine ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ppb_got_read = 0;
    hr = IPersistPropertyBag_Load(ppb, &property_bag, NULL);
    ok(hr == S_OK || broken(hr == E_FAIL) /* 8+, intermittent */, "Got hr %#lx.\n", hr);
    ok(ppb_got_read == 1, "Got %u calls to Read().\n", ppb_got_read);

    ref = IPersistPropertyBag_Release(ppb);
    ok(!ref, "Got unexpected refcount %ld.\n", ref);

    VariantClear(&var);
    IPropertyBag_Release(devenum_bag);
}

static void test_unconnected_filter_state(IBaseFilter *filter)
{
    FILTER_STATE state;
    HRESULT hr;

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    todo_wine ok(hr == VFW_S_CANT_CUE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    todo_wine ok(hr == VFW_S_CANT_CUE, "Got hr %#lx.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);
}

START_TEST(audiorecord)
{
    ICreateDevEnum *devenum;
    IEnumMoniker *enummon;
    IBaseFilter *filter;
    IMoniker *mon;
    WCHAR *name;
    HRESULT hr;
    ULONG ref;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICreateDevEnum, (void **)&devenum);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = ICreateDevEnum_CreateClassEnumerator(devenum, &CLSID_AudioInputDeviceCategory, &enummon, 0);
    if (hr == S_FALSE)
    {
        skip("No audio input devices present.\n");
        ICreateDevEnum_Release(devenum);
        CoUninitialize();
        return;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    test_aggregation();

    while (IEnumMoniker_Next(enummon, 1, &mon, NULL) == S_OK)
    {
        hr = IMoniker_GetDisplayName(mon, NULL, NULL, &name);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        trace("Testing device %s.\n", wine_dbgstr_w(name));
        CoTaskMemFree(name);

        test_property_bag(mon);

        hr = IMoniker_BindToObject(mon, NULL, NULL, &IID_IBaseFilter, (void **)&filter);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        test_interfaces(filter);
        test_unconnected_filter_state(filter);

        ref = IBaseFilter_Release(filter);
        ok(!ref, "Got outstanding refcount %ld.\n", ref);
        IMoniker_Release(mon);
    }

    IEnumMoniker_Release(enummon);
    ICreateDevEnum_Release(devenum);
    CoUninitialize();
}
