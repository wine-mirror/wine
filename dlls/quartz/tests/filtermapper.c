/*
 * Filtermapper unit tests for Quartz
 *
 * Copyright (C) 2008 Alexander Dorofeyev
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

#include "wine/test.h"
#include "winbase.h"
#include "dshow.h"

/* Helper function, checks if filter with given name was enumerated. */
static BOOL enum_find_filter(const WCHAR *wszFilterName, IEnumMoniker *pEnum)
{
    IMoniker *pMoniker = NULL;
    BOOL found = FALSE;
    ULONG nb;
    HRESULT hr;
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};

    while(!found && IEnumMoniker_Next(pEnum, 1, &pMoniker, &nb) == S_OK)
    {
        IPropertyBag * pPropBagCat = NULL;
        VARIANT var;

        VariantInit(&var);

        hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);
        ok(SUCCEEDED(hr), "IMoniker_BindToStorage failed with %x\n", hr);
        if (FAILED(hr) || !pPropBagCat)
        {
            VariantClear(&var);
            IMoniker_Release(pMoniker);
            continue;
        }

        hr = IPropertyBag_Read(pPropBagCat, wszFriendlyName, &var, NULL);
        ok(SUCCEEDED(hr), "IPropertyBag_Read failed with %x\n", hr);

        if (SUCCEEDED(hr))
        {
            if (!lstrcmpW((WCHAR*)V_UNION(&var, bstrVal), wszFilterName)) found = TRUE;
        }

        IPropertyBag_Release(pPropBagCat);
        IMoniker_Release(pMoniker);
        VariantClear(&var);
    }

    return found;
}

static void test_fm2_enummatchingfilters(void)
{
    IFilterMapper2 *pMapper = NULL;
    HRESULT hr;
    REGFILTER2 rgf2;
    REGFILTERPINS2 rgPins2[2];
    REGPINTYPES rgPinType;
    static const WCHAR wszFilterName1[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', '1', 0 };
    static const WCHAR wszFilterName2[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', '2', 0 };
    CLSID clsidFilter1;
    CLSID clsidFilter2;
    IEnumMoniker *pEnum = NULL;
    BOOL found;

    ZeroMemory(&rgf2, sizeof(rgf2));

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (LPVOID*)&pMapper);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    if (FAILED(hr)) goto out;

    hr = CoCreateGuid(&clsidFilter1);
    ok(hr == S_OK, "CoCreateGuid failed with %x\n", hr);
    hr = CoCreateGuid(&clsidFilter2);
    ok(hr == S_OK, "CoCreateGuid failed with %x\n", hr);

    /* Test that a test renderer filter is returned when enumerating filters with bRender=FALSE */
    rgf2.dwVersion = 2;
    rgf2.dwMerit = MERIT_UNLIKELY;
    S1(U(rgf2)).cPins2 = 1;
    S1(U(rgf2)).rgPins2 = rgPins2;

    rgPins2[0].dwFlags = REG_PINFLAG_B_RENDERER;
    rgPins2[0].cInstances = 1;
    rgPins2[0].nMediaTypes = 1;
    rgPins2[0].lpMediaType = &rgPinType;
    rgPins2[0].nMediums = 0;
    rgPins2[0].lpMedium = NULL;
    rgPins2[0].clsPinCategory = NULL;

    rgPinType.clsMajorType = &GUID_NULL;
    rgPinType.clsMinorType = &GUID_NULL;

    hr = IFilterMapper2_RegisterFilter(pMapper, &clsidFilter1, wszFilterName1, NULL,
                    &CLSID_LegacyAmFilterCategory, NULL, &rgf2);
    ok(hr == S_OK, "IFilterMapper2_RegisterFilter failed with %x\n", hr);

    rgPins2[0].dwFlags = 0;

    rgPins2[1].dwFlags = REG_PINFLAG_B_OUTPUT;
    rgPins2[1].cInstances = 1;
    rgPins2[1].nMediaTypes = 1;
    rgPins2[1].lpMediaType = &rgPinType;
    rgPins2[1].nMediums = 0;
    rgPins2[1].lpMedium = NULL;
    rgPins2[1].clsPinCategory = NULL;

    S1(U(rgf2)).cPins2 = 2;

    hr = IFilterMapper2_RegisterFilter(pMapper, &clsidFilter2, wszFilterName2, NULL,
                    &CLSID_LegacyAmFilterCategory, NULL, &rgf2);
    ok(hr == S_OK, "IFilterMapper2_RegisterFilter failed with %x\n", hr);

    hr = IFilterMapper2_EnumMatchingFilters(pMapper, &pEnum, 0, TRUE, MERIT_UNLIKELY, TRUE,
                0, NULL, NULL, &GUID_NULL, FALSE, FALSE, 0, NULL, NULL, &GUID_NULL);
    ok(hr == S_OK, "IFilterMapper2_EnumMatchingFilters failed with %x\n", hr);
    if (SUCCEEDED(hr) && pEnum)
    {
        found = enum_find_filter(wszFilterName1, pEnum);
        ok(found, "EnumMatchingFilters failed to return the test filter 1\n");
        found = enum_find_filter(wszFilterName2, pEnum);
        ok(found, "EnumMatchingFilters failed to return the test filter 2\n");
    }

    if (pEnum) IEnumMoniker_Release(pEnum);
    pEnum = NULL;

    /* Non renderer must not be returned with bRender=TRUE */

    hr = IFilterMapper2_EnumMatchingFilters(pMapper, &pEnum, 0, TRUE, MERIT_UNLIKELY, TRUE,
                0, NULL, NULL, &GUID_NULL, TRUE, FALSE, 0, NULL, NULL, &GUID_NULL);
    ok(hr == S_OK, "IFilterMapper2_EnumMatchingFilters failed with %x\n", hr);

    if (SUCCEEDED(hr) && pEnum)
    {
        found = enum_find_filter(wszFilterName1, pEnum);
        ok(found, "EnumMatchingFilters failed to return the test filter 1\n");
        found = enum_find_filter(wszFilterName2, pEnum);
        ok(!found, "EnumMatchingFilters should not return the test filter 2\n");
    }

    hr = IFilterMapper2_UnregisterFilter(pMapper, &CLSID_LegacyAmFilterCategory, NULL,
            &clsidFilter1);
    ok(SUCCEEDED(hr), "IFilterMapper2_UnregisterFilter failed with %x\n", hr);

    hr = IFilterMapper2_UnregisterFilter(pMapper, &CLSID_LegacyAmFilterCategory, NULL,
            &clsidFilter2);
    ok(SUCCEEDED(hr), "IFilterMapper2_UnregisterFilter failed with %x\n", hr);

    out:

    if (pEnum) IEnumMoniker_Release(pEnum);
    if (pMapper) IFilterMapper2_Release(pMapper);
}

static void test_legacy_filter_registration(void)
{
    IFilterMapper2 *pMapper2 = NULL;
    IFilterMapper *pMapper = NULL;
    HRESULT hr;
    static const WCHAR wszFilterName[] = {'T', 'e', 's', 't', 'f', 'i', 'l', 't', 'e', 'r', 0 };
    static const WCHAR wszPinName[] = {'P', 'i', 'n', '1', 0 };
    CLSID clsidFilter;
    WCHAR wszRegKey[MAX_PATH];
    static const WCHAR wszClsid[] = {'C','L','S','I','D', 0};
    static const WCHAR wszSlash[] = {'\\', 0};
    LONG lRet;
    HKEY hKey = NULL;
    IEnumMoniker *pEnum = NULL;
    BOOL found;
    IEnumRegFilters *pRegEnum = NULL;

    /* Test if legacy filter registration scheme works (filter is added to HKCR\Filter). IFilterMapper_RegisterFilter
     * registers in this way. Filters so registered must then be accessible through both IFilterMapper_EnumMatchingFilters
     * and IFilterMapper2_EnumMatchingFilters. */
    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (LPVOID*)&pMapper2);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IFilterMapper2_QueryInterface(pMapper2, &IID_IFilterMapper, (LPVOID)&pMapper);
    ok(hr == S_OK, "IFilterMapper2_QueryInterface failed with %x\n", hr);
    if (FAILED(hr)) goto out;

    /* Register a test filter. */
    hr = CoCreateGuid(&clsidFilter);
    ok(hr == S_OK, "CoCreateGuid failed with %x\n", hr);

    lstrcpyW(wszRegKey, wszClsid);
    lstrcatW(wszRegKey, wszSlash);
    lRet = StringFromGUID2(&clsidFilter, wszRegKey + lstrlenW(wszRegKey), MAX_PATH - lstrlenW(wszRegKey));
    ok(lRet > 0, "StringFromGUID2 failed\n");
    if (!lRet) goto out;

    /* Register---- functions need a filter class key to write pin and pin media type data to. Create a bogus
     * class key for it. */
    lRet = RegCreateKeyExW(HKEY_CLASSES_ROOT, wszRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    ok(lRet == ERROR_SUCCESS, "RegCreateKeyExW failed with %x\n", HRESULT_FROM_WIN32(lRet));

    /* Set default value - this is interpreted as "friendly name" later. */
    lRet = RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)wszFilterName, (lstrlenW(wszFilterName) + 1) * 2);
    ok(lRet == ERROR_SUCCESS, "RegSetValueExW failed with %x\n", HRESULT_FROM_WIN32(lRet));

    if (hKey) RegCloseKey(hKey);
    hKey = NULL;

    hr = IFilterMapper_RegisterFilter(pMapper, clsidFilter, wszFilterName, MERIT_UNLIKELY);
    ok(hr == S_OK, "IFilterMapper_RegisterFilter failed with %x\n", hr);

    hr = IFilterMapper_RegisterPin(pMapper, clsidFilter, wszPinName, TRUE, FALSE, FALSE, FALSE, GUID_NULL, NULL);
    ok(hr == S_OK, "IFilterMapper_RegisterPin failed with %x\n", hr);

    hr = IFilterMapper_RegisterPinType(pMapper, clsidFilter, wszPinName, GUID_NULL, GUID_NULL);
    ok(hr == S_OK, "IFilterMapper_RegisterPinType failed with %x\n", hr);

    hr = IFilterMapper2_EnumMatchingFilters(pMapper2, &pEnum, 0, TRUE, MERIT_UNLIKELY, TRUE,
                0, NULL, NULL, &GUID_NULL, FALSE, FALSE, 0, NULL, NULL, &GUID_NULL);
    ok(hr == S_OK, "IFilterMapper2_EnumMatchingFilters failed with %x\n", hr);
    if (SUCCEEDED(hr) && pEnum)
    {
        found = enum_find_filter(wszFilterName, pEnum);
        ok(found, "IFilterMapper2_EnumMatchingFilters failed to return the test filter\n");
    }

    if (pEnum) IEnumMoniker_Release(pEnum);
    pEnum = NULL;

    found = FALSE;
    hr = IFilterMapper_EnumMatchingFilters(pMapper, &pRegEnum, MERIT_UNLIKELY, TRUE, GUID_NULL, GUID_NULL,
            FALSE, FALSE, GUID_NULL, GUID_NULL);
    ok(hr == S_OK, "IFilterMapper_EnumMatchingFilters failed with %x\n", hr);
    if (SUCCEEDED(hr) && pRegEnum)
    {
        ULONG cFetched;
        REGFILTER *prgf;

        while(!found && IEnumRegFilters_Next(pRegEnum, 1, &prgf, &cFetched) == S_OK)
        {
            if (!lstrcmpW(prgf->Name, wszFilterName)) found = TRUE;

            CoTaskMemFree(prgf);
        }

        IEnumRegFilters_Release(pRegEnum);
    }
    ok(found, "IFilterMapper_EnumMatchingFilters failed to return the test filter\n");

    hr = IFilterMapper_UnregisterFilter(pMapper, clsidFilter);
    ok(hr == S_OK, "FilterMapper_UnregisterFilter failed with %x\n", hr);

    lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszClsid, 0, KEY_WRITE | DELETE, &hKey);
    ok(lRet == ERROR_SUCCESS, "RegOpenKeyExW failed with %x\n", HRESULT_FROM_WIN32(lRet));

    lRet = StringFromGUID2(&clsidFilter, wszRegKey, MAX_PATH);
    ok(lRet > 0, "StringFromGUID2 failed\n");

    lRet = RegDeleteKeyW(hKey, wszRegKey);
    ok(lRet == ERROR_SUCCESS, "RegDeleteKeyW failed with %x\n", HRESULT_FROM_WIN32(lRet));

    if (hKey) RegCloseKey(hKey);
    hKey = NULL;

    out:

    if (pMapper) IFilterMapper_Release(pMapper);
    if (pMapper2) IFilterMapper2_Release(pMapper2);
}

START_TEST(filtermapper)
{
    CoInitialize(NULL);

    test_fm2_enummatchingfilters();
    test_legacy_filter_registration();

    CoUninitialize();
}
