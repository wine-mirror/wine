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

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
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

static const WCHAR waveinidW[] = {'W','a','v','e','I','n','I','d',0};
static const WCHAR usemixerW[] = {'U','s','e','M','i','x','e','r',0};
static int ppb_id;
static unsigned int ppb_got_read;

static HRESULT WINAPI property_bag_Read(IPropertyBag *iface, const WCHAR *name, VARIANT *var, IErrorLog *log)
{
    if (!lstrcmpW(name, usemixerW))
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    todo_wine ok(!lstrcmpW(name, waveinidW), "Got unexpected name %s.\n", wine_dbgstr_w(name));
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
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(devenum_bag, waveinidW, &var, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ppb_id = V_I4(&var);

    hr = CoCreateInstance(&CLSID_AudioRecord, NULL, CLSCTX_INPROC_SERVER,
            &IID_IPersistPropertyBag, (void **)&ppb);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPersistPropertyBag_InitNew(ppb);
    todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

    ppb_got_read = 0;
    hr = IPersistPropertyBag_Load(ppb, &property_bag, NULL);
    ok(hr == S_OK || broken(hr == E_FAIL) /* 8+, intermittent */, "Got hr %#x.\n", hr);
    ok(ppb_got_read == 1, "Got %u calls to Read().\n", ppb_got_read);

    ref = IPersistPropertyBag_Release(ppb);
    ok(!ref, "Got unexpected refcount %d.\n", ref);

    VariantClear(&var);
    IPropertyBag_Release(devenum_bag);
}

START_TEST(audiorecord)
{
    ICreateDevEnum *devenum;
    IEnumMoniker *enummon;
    IBaseFilter *filter;
    IMoniker *mon;
    WCHAR *name;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
            &IID_ICreateDevEnum, (void **)&devenum);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = ICreateDevEnum_CreateClassEnumerator(devenum, &CLSID_AudioInputDeviceCategory, &enummon, 0);
    if (hr == S_FALSE)
    {
        skip("No audio input devices present.\n");
        ICreateDevEnum_Release(devenum);
        CoUninitialize();
        return;
    }
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    while (IEnumMoniker_Next(enummon, 1, &mon, NULL) == S_OK)
    {
        hr = IMoniker_GetDisplayName(mon, NULL, NULL, &name);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        trace("Testing device %s.\n", wine_dbgstr_w(name));
        CoTaskMemFree(name);

        test_property_bag(mon);

        hr = IMoniker_BindToObject(mon, NULL, NULL, &IID_IBaseFilter, (void **)&filter);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        test_interfaces(filter);

        IMoniker_Release(mon);
    }

    IEnumMoniker_Release(enummon);
    ICreateDevEnum_Release(devenum);
    CoUninitialize();
}
