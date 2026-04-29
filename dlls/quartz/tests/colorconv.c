/*
 * Color Space Converter filter unit tests
 *
 * Copyright 2026 Brendan McGrath
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
#include "wine/fil_data.h"
#include "wine/test.h"

#include <uuids.h>

static HRESULT create_filter_mapper(IFilterMapper2 **mapper)
{
    return CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (void **)mapper);
}

static HRESULT create_color_conv_property_bag(IPropertyBag **property_bag)
{
    IEnumMoniker *moniker_enum;
    ICreateDevEnum *dev_enum;
    IMoniker *moniker;
    VARIANT var;
    HRESULT hr;
    GUID clsid;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void **)&dev_enum);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ICreateDevEnum_CreateClassEnumerator(dev_enum, &CLSID_LegacyAmFilterCategory, &moniker_enum, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);

    while ((hr = IEnumMoniker_Next(moniker_enum, 1, &moniker, NULL)) == S_OK)
    {
        hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)property_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IPropertyBag_Read(*property_bag, L"CLSID", &var, 0);

        if (hr == S_OK)
            IIDFromString(var.bstrVal, &clsid);

        VariantClear(&var);
        IMoniker_Release(moniker);

        if (hr == S_OK && IsEqualGUID(&clsid, &CLSID_Colour))
            break;

        IPropertyBag_Release(*property_bag);
    }

    if (hr != S_OK)
        *property_bag = NULL;

    return hr;
}

static const GUID *subtypes[] =
{
    &MEDIASUBTYPE_RGB8,
    &MEDIASUBTYPE_RGB555,
    &MEDIASUBTYPE_RGB565,
    &MEDIASUBTYPE_RGB24,
    &MEDIASUBTYPE_RGB32,
    &MEDIASUBTYPE_ARGB32,
};

static void test_registration(void)
{
    REGFILTER2 **ret, *reg_filter;
    const REGPINTYPES *pin_types;
    IAMFilterData *filter_data;
    IPropertyBag *property_bag;
    const REGFILTERPINS2 *pin;
    IFilterMapper2 *mapper;
    VARIANT var;
    HRESULT hr;
    BYTE *data;
    ULONG len;
    int i, j;

    hr = create_color_conv_property_bag(&property_bag);

    if (hr != S_OK)
    {
        skip("Skipping registration tests.\n");
        return;
    }

    VariantInit(&var);
    hr = IPropertyBag_Read(property_bag, L"FilterData", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = create_filter_mapper(&mapper);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterMapper2_QueryInterface(mapper, &IID_IAMFilterData, (void **)&filter_data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = SafeArrayAccessData(var.parray, (void **)&data);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    len = var.parray->rgsabound[0].cElements;
    ok(len == 368, "Got len %ld.\n", len);

    hr = IAMFilterData_ParseFilterData(filter_data, data, len, (BYTE **)&ret);
    reg_filter = *ret;
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(reg_filter->dwVersion == 2, "Got dwVersion %lu.\n", reg_filter->dwVersion);
    ok(reg_filter->dwMerit == MERIT_UNLIKELY + 1, "Got merit %#lx.\n", reg_filter->dwMerit);
    ok(reg_filter->cPins2 == 2, "Got cPins2 %lu.\n", reg_filter->cPins2);
    ok(reg_filter->rgPins2 != NULL, "Expected rgPins2 to be non-NULL.\n");

    for (i = 0; i < reg_filter->cPins2; i++)
    {
        winetest_push_context("pin %d", i);

        pin = reg_filter->rgPins2 + i;
        ok(pin->dwFlags == (i ? REG_PINFLAG_B_OUTPUT : 0), "Got dwFlags %#lx.\n", pin->dwFlags);
        ok(pin->cInstances == 0, "Got cInstances %u.\n", pin->cInstances);
        ok(pin->nMediaTypes == ARRAY_SIZE(subtypes), "Got nMediaTypes %u.\n", pin->nMediaTypes);
        ok(pin->lpMediaType != NULL, "Expected lpMediaType to be non-NULL.\n");
        ok(pin->nMediums == 0, "Got nMediums %u.\n", pin->nMediums);
        ok(pin->clsPinCategory == NULL, "Expected clsPinCategory to be NULL.\n");

        for (j = 0; j < ARRAY_SIZE(subtypes); j++)
        {
            winetest_push_context("media type %d", j);

            pin_types = pin->lpMediaType + j;
            ok(IsEqualGUID(pin_types->clsMajorType, &MEDIATYPE_Video), "Got major type %s.\n",
                    wine_dbgstr_guid(pin_types->clsMajorType));
            ok(IsEqualGUID(pin_types->clsMinorType, subtypes[j]), "Got minor type %s.\n",
                    wine_dbgstr_guid(pin_types->clsMinorType));

            winetest_pop_context();
        }

        winetest_pop_context();
    }

    CoTaskMemFree(reg_filter);

    hr = SafeArrayUnaccessData(var.parray);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = VariantClear(&var);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IAMFilterData_Release(filter_data);
    IFilterMapper2_Release(mapper);
    IPropertyBag_Release(property_bag);
}

START_TEST(colorconv)
{
    CoInitialize(NULL);

    test_registration();

    CoUninitialize();
}
