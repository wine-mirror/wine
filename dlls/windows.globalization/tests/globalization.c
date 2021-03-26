/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_System_UserProfile
#include "windows.system.userprofile.h"

#include "wine/test.h"

static HRESULT (WINAPI *pRoActivateInstance)(HSTRING, IInspectable **);
static HRESULT (WINAPI *pRoGetActivationFactory)(HSTRING, REFIID, void **);
static HRESULT (WINAPI *pRoInitialize)(RO_INIT_TYPE);
static void    (WINAPI *pRoUninitialize)(void);
static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);
static WCHAR  *(WINAPI *pWindowsGetStringRawBuffer)(HSTRING, UINT32 *);

static void test_GlobalizationPreferences(void)
{
    static const WCHAR *class_name = L"Windows.System.UserProfile.GlobalizationPreferences";

    IGlobalizationPreferencesStatics *preferences_statics = NULL;
    IVectorView_HSTRING *languages = NULL, *calendars, *clocks, *currencies;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    HSTRING str, tmp_str;
    BOOLEAN found;
    HRESULT hr;
    UINT32 len;
    WCHAR *buf, locale[LOCALE_NAME_MAX_LENGTH], country[16];
    UINT32 i, size;

    GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH);
    GetUserDefaultGeoName(country, 16);

    hr = pRoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    hr = pWindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hr = pRoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#x\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        pWindowsDeleteString(str);
        pRoUninitialize();
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IGlobalizationPreferencesStatics, (void **)&preferences_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IGlobalizationPreferencesStatics failed, hr %#x\n", hr);

    hr = IGlobalizationPreferencesStatics_QueryInterface(preferences_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_QueryInterface IID_IInspectable failed, hr %#x\n", hr);
    ok(tmp_inspectable == inspectable, "IGlobalizationPreferencesStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IGlobalizationPreferencesStatics_QueryInterface(preferences_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);
    ok(tmp_agile_object == agile_object, "IGlobalizationPreferencesStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IGlobalizationPreferencesStatics_get_HomeGeographicRegion(preferences_statics, &tmp_str);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_HomeGeographicRegion failed, hr %#x\n", hr);

    buf = pWindowsGetStringRawBuffer(tmp_str, &len);
    ok(buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len);
    ok(wcslen(country) == len && !memcmp(buf, country, len),
       "IGlobalizationPreferencesStatics_get_HomeGeographicRegion returned len %u, str %s, expected %s\n",
       len, wine_dbgstr_w(buf), wine_dbgstr_w(country));

    pWindowsDeleteString(tmp_str);

    hr = IGlobalizationPreferencesStatics_get_Languages(preferences_statics, &languages);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Languages failed, hr %#x\n", hr);

    hr = IVectorView_HSTRING_QueryInterface(languages, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_HSTRING_QueryInterface failed, hr %#x\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_HSTRING_QueryInterface returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_HSTRING_QueryInterface(languages, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IVectorView_HSTRING_QueryInterface failed, hr %#x\n", hr);
    ok(tmp_agile_object != agile_object, "IVectorView_HSTRING_QueryInterface IID_IAgileObject returned agile_object\n");
    IAgileObject_Release(tmp_agile_object);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(languages, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#x\n", hr);
    ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    hr = IVectorView_HSTRING_GetAt(languages, 0, &tmp_str);
    ok(hr == S_OK, "IVectorView_HSTRING_GetAt failed, hr %#x\n", hr);
    buf = pWindowsGetStringRawBuffer(tmp_str, &len);
    ok(buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len);

    ok(wcslen(locale) == len && !memcmp(buf, locale, len),
       "IGlobalizationPreferencesStatics_get_Languages 0 returned len %u, str %s, expected %s\n",
       len, wine_dbgstr_w(buf), wine_dbgstr_w(locale));

    i = 0xdeadbeef;
    found = FALSE;
    hr = IVectorView_HSTRING_IndexOf(languages, tmp_str, &i, &found);
    ok(hr == S_OK, "IVectorView_HSTRING_IndexOf failed, hr %#x\n", hr);
    ok(i == 0 && found == TRUE, "IVectorView_HSTRING_IndexOf returned size %d, found %d\n", size, found);

    pWindowsDeleteString(tmp_str);

    hr = pWindowsCreateString(L"deadbeef", 8, &tmp_str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    i = 0xdeadbeef;
    found = TRUE;
    hr = IVectorView_HSTRING_IndexOf(languages, tmp_str, &i, &found);
    ok(hr == S_OK, "IVectorView_HSTRING_IndexOf failed, hr %#x\n", hr);
    ok(i == 0 && found == FALSE, "IVectorView_HSTRING_IndexOf returned size %d, found %d\n", size, found);

    pWindowsDeleteString(tmp_str);

    tmp_str = (HSTRING)0xdeadbeef;
    hr = IVectorView_HSTRING_GetAt(languages, size, &tmp_str);
    ok(hr == E_BOUNDS, "IVectorView_HSTRING_GetAt failed, hr %#x\n", hr);
    ok(tmp_str == NULL, "IVectorView_HSTRING_GetAt returned %p\n", tmp_str);

    tmp_str = (HSTRING)0xdeadbeef;
    hr = IVectorView_HSTRING_GetMany(languages, size, 1, &tmp_str, &i);
    ok(hr == S_OK, "IVectorView_HSTRING_GetAt failed, hr %#x\n", hr);
    ok(i == 0 && tmp_str == NULL, "IVectorView_HSTRING_GetMany returned count %u, str %p\n", i, tmp_str);

    hr = IVectorView_HSTRING_GetMany(languages, 0, 1, &tmp_str, &i);
    ok(hr == S_OK, "IVectorView_HSTRING_GetAt failed, hr %#x\n", hr);
    ok(i == 1, "IVectorView_HSTRING_GetMany returned count %u, expected 1\n", i);

    buf = pWindowsGetStringRawBuffer(tmp_str, &len);
    ok(buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len);

    ok(wcslen(locale) == len && !memcmp(buf, locale, len),
       "IGlobalizationPreferencesStatics_get_Languages 0 returned len %u, str %s, expected %s\n",
       len, wine_dbgstr_w(buf), wine_dbgstr_w(locale));

    pWindowsDeleteString(tmp_str);

    IVectorView_HSTRING_Release(languages);


    hr = IGlobalizationPreferencesStatics_get_Calendars(preferences_statics, &calendars);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Calendars failed, hr %#x\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(calendars, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#x\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    IVectorView_HSTRING_Release(calendars);


    hr = IGlobalizationPreferencesStatics_get_Clocks(preferences_statics, &clocks);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Clocks failed, hr %#x\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(clocks, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#x\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    IVectorView_HSTRING_Release(clocks);


    hr = IGlobalizationPreferencesStatics_get_Currencies(preferences_statics, &currencies);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Currencies failed, hr %#x\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(currencies, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#x\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    IVectorView_HSTRING_Release(currencies);


    IGlobalizationPreferencesStatics_Release(preferences_statics);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    pWindowsDeleteString(str);

    pRoUninitialize();
}

START_TEST(globalization)
{
    HMODULE combase;

    if (!(combase = LoadLibraryW(L"combase.dll")))
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return;
    }

#define LOAD_FUNCPTR(x) \
    if (!(p##x = (void*)GetProcAddress(combase, #x))) \
    { \
        win_skip("Failed to find %s in combase.dll, skipping tests.\n", #x); \
        return; \
    }

    LOAD_FUNCPTR(RoActivateInstance);
    LOAD_FUNCPTR(RoGetActivationFactory);
    LOAD_FUNCPTR(RoInitialize);
    LOAD_FUNCPTR(RoUninitialize);
    LOAD_FUNCPTR(WindowsCreateString);
    LOAD_FUNCPTR(WindowsDeleteString);
    LOAD_FUNCPTR(WindowsGetStringRawBuffer);
#undef LOAD_FUNCPTR

    test_GlobalizationPreferences();
}
