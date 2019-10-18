/*
 * WDM video capture filter unit tests
 *
 * Copyright 2019 Damjan Jovanovic
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

static void test_media_types(IPin *pin)
{
    IEnumMediaTypes *enum_media_types;
    AM_MEDIA_TYPE mt, *pmt;
    HRESULT hr;

    hr = IPin_EnumMediaTypes(pin, &enum_media_types);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    while (IEnumMediaTypes_Next(enum_media_types, 1, &pmt, NULL) == S_OK)
    {
        hr = IPin_QueryAccept(pin, pmt);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        CoTaskMemFree(pmt);
    }
    IEnumMediaTypes_Release(enum_media_types);

    hr = IPin_QueryAccept(pin, NULL);
    todo_wine ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    memset(&mt, 0, sizeof(mt));
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr != S_OK, "Got hr %#x.\n", hr);

    mt.majortype = MEDIATYPE_Video;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr != S_OK, "Got hr %#x.\n", hr);

    mt.formattype = FORMAT_VideoInfo;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr != S_OK, "Got hr %#x.\n", hr);

    mt.formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, &mt);
    ok(hr != S_OK, "Got hr %#x.\n", hr);
}

static void test_capture(IBaseFilter *filter)
{
    IEnumPins *enum_pins;
    IPin *pin;
    HRESULT hr;

    hr = IBaseFilter_EnumPins(filter, &enum_pins);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    while ((hr = IEnumPins_Next(enum_pins, 1, &pin, NULL)) == S_OK)
    {
        PIN_DIRECTION pin_direction;
        IPin_QueryDirection(pin, &pin_direction);
        if (pin_direction == PINDIR_OUTPUT)
            test_media_types(pin);
        IPin_Release(pin);
    }

    IEnumPins_Release(enum_pins);
}

START_TEST(videocapture)
{
    ICreateDevEnum *dev_enum;
    IEnumMoniker *class_enum;
    IBaseFilter *filter;
    IMoniker *moniker;
    WCHAR *name;
    HRESULT hr;
    ULONG ref;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
            &IID_ICreateDevEnum, (void **)&dev_enum);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = ICreateDevEnum_CreateClassEnumerator(dev_enum, &CLSID_VideoInputDeviceCategory, &class_enum, 0);
    if (hr == S_FALSE)
    {
        skip("No video capture devices present.\n");
        ICreateDevEnum_Release(dev_enum);
        CoUninitialize();
        return;
    }
    ok(hr == S_OK, "Got hr=%#x.\n", hr);

    while (IEnumMoniker_Next(class_enum, 1, &moniker, NULL) == S_OK)
    {
        hr = IMoniker_GetDisplayName(moniker, NULL, NULL, &name);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        trace("Testing device %s.\n", wine_dbgstr_w(name));
        CoTaskMemFree(name);

        hr = IMoniker_BindToObject(moniker, NULL, NULL, &IID_IBaseFilter, (void**)&filter);
        if (hr == S_OK)
        {
            test_capture(filter);
            ref = IBaseFilter_Release(filter);
            ok(!ref, "Got outstanding refcount %d.\n", ref);
        }
        else
            skip("Failed to open capture device, hr=%#x.\n", hr);

        IMoniker_Release(moniker);
    }

    ICreateDevEnum_Release(dev_enum);
    IEnumMoniker_Release(class_enum);
    CoUninitialize();
}
