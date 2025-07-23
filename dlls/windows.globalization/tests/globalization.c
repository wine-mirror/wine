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
#define WIDL_using_Windows_Globalization
#include "windows.globalization.h"
#define WIDL_using_Windows_System_UserProfile
#include "windows.system.userprofile.h"

#include "wine/test.h"

/* kernel32.dll */
static INT (WINAPI *pGetUserDefaultGeoName)(LPWSTR, int);

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    IUnknown_Release( unk );
}

static void test_GlobalizationPreferences(void)
{
    static const WCHAR *class_name = L"Windows.System.UserProfile.GlobalizationPreferences";

    IGlobalizationPreferencesStatics *preferences_statics = NULL;
    IVectorView_HSTRING *languages = NULL, *calendars, *clocks, *currencies;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    HSTRING str, tmp_str;
    const WCHAR *buf;
    BOOLEAN found;
    HRESULT hr;
    UINT32 len;
    UINT32 i, size;

    GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH);

    hr = WindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        WindowsDeleteString(str);
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IGlobalizationPreferencesStatics, (void **)&preferences_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IGlobalizationPreferencesStatics failed, hr %#lx\n", hr);

    hr = IGlobalizationPreferencesStatics_QueryInterface(preferences_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);
    ok(tmp_inspectable == inspectable, "IGlobalizationPreferencesStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IGlobalizationPreferencesStatics_QueryInterface(preferences_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);
    ok(tmp_agile_object == agile_object, "IGlobalizationPreferencesStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IGlobalizationPreferencesStatics_get_HomeGeographicRegion(preferences_statics, &tmp_str);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_HomeGeographicRegion failed, hr %#lx\n", hr);

    buf = WindowsGetStringRawBuffer(tmp_str, &len);
    ok(buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len);
    if (pGetUserDefaultGeoName)
    {
        WCHAR country[16];
        DWORD geolen;
        geolen = pGetUserDefaultGeoName(country, ARRAY_SIZE(country));
        ok(broken(geolen == 1) || /* Win10 1709 */
           (wcslen(country) == len && !memcmp(buf, country, len)),
           "IGlobalizationPreferencesStatics_get_HomeGeographicRegion returned len %u, str %s, expected %s\n",
           len, wine_dbgstr_w(buf), wine_dbgstr_w(country));
    }

    WindowsDeleteString(tmp_str);

    hr = IGlobalizationPreferencesStatics_get_Languages(preferences_statics, &languages);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Languages failed, hr %#lx\n", hr);

    hr = IVectorView_HSTRING_QueryInterface(languages, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_HSTRING_QueryInterface failed, hr %#lx\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_HSTRING_QueryInterface returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_HSTRING_QueryInterface(languages, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IVectorView_HSTRING_QueryInterface failed, hr %#lx\n", hr);
    ok(tmp_agile_object != agile_object, "IVectorView_HSTRING_QueryInterface IID_IAgileObject returned agile_object\n");
    IAgileObject_Release(tmp_agile_object);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(languages, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#lx\n", hr);
    ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    hr = IVectorView_HSTRING_GetAt(languages, 0, &tmp_str);
    ok(hr == S_OK, "IVectorView_HSTRING_GetAt failed, hr %#lx\n", hr);
    buf = WindowsGetStringRawBuffer(tmp_str, &len);
    ok(buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len);

    ok(wcslen(locale) == len && !memcmp(buf, locale, len),
       "IGlobalizationPreferencesStatics_get_Languages 0 returned len %u, str %s, expected %s\n",
       len, wine_dbgstr_w(buf), wine_dbgstr_w(locale));

    i = 0xdeadbeef;
    found = FALSE;
    hr = IVectorView_HSTRING_IndexOf(languages, tmp_str, &i, &found);
    ok(hr == S_OK, "IVectorView_HSTRING_IndexOf failed, hr %#lx\n", hr);
    ok(i == 0 && found == TRUE, "IVectorView_HSTRING_IndexOf returned size %d, found %d\n", size, found);

    WindowsDeleteString(tmp_str);

    hr = WindowsCreateString(L"deadbeef", 8, &tmp_str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    i = 0xdeadbeef;
    found = TRUE;
    hr = IVectorView_HSTRING_IndexOf(languages, tmp_str, &i, &found);
    ok(hr == S_OK, "IVectorView_HSTRING_IndexOf failed, hr %#lx\n", hr);
    ok(i == 0 && found == FALSE, "IVectorView_HSTRING_IndexOf returned size %d, found %d\n", size, found);

    WindowsDeleteString(tmp_str);

    tmp_str = (HSTRING)0xdeadbeef;
    hr = IVectorView_HSTRING_GetAt(languages, size, &tmp_str);
    ok(hr == E_BOUNDS, "IVectorView_HSTRING_GetAt failed, hr %#lx\n", hr);
    ok(tmp_str == NULL, "IVectorView_HSTRING_GetAt returned %p\n", tmp_str);

    tmp_str = (HSTRING)0xdeadbeef;
    hr = IVectorView_HSTRING_GetMany(languages, size, 1, &tmp_str, &i);
    ok(hr == S_OK, "IVectorView_HSTRING_GetAt failed, hr %#lx\n", hr);
    ok(i == 0 && tmp_str == NULL, "IVectorView_HSTRING_GetMany returned count %u, str %p\n", i, tmp_str);

    hr = IVectorView_HSTRING_GetMany(languages, 0, 1, &tmp_str, &i);
    ok(hr == S_OK, "IVectorView_HSTRING_GetAt failed, hr %#lx\n", hr);
    ok(i == 1, "IVectorView_HSTRING_GetMany returned count %u, expected 1\n", i);

    buf = WindowsGetStringRawBuffer(tmp_str, &len);
    ok(buf != NULL && len > 0, "WindowsGetStringRawBuffer returned buf %p, len %u\n", buf, len);

    ok(wcslen(locale) == len && !memcmp(buf, locale, len),
       "IGlobalizationPreferencesStatics_get_Languages 0 returned len %u, str %s, expected %s\n",
       len, wine_dbgstr_w(buf), wine_dbgstr_w(locale));

    WindowsDeleteString(tmp_str);

    IVectorView_HSTRING_Release(languages);


    hr = IGlobalizationPreferencesStatics_get_Calendars(preferences_statics, &calendars);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Calendars failed, hr %#lx\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(calendars, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#lx\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    IVectorView_HSTRING_Release(calendars);


    hr = IGlobalizationPreferencesStatics_get_Clocks(preferences_statics, &clocks);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Clocks failed, hr %#lx\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(clocks, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#lx\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    IVectorView_HSTRING_Release(clocks);


    hr = IGlobalizationPreferencesStatics_get_Currencies(preferences_statics, &currencies);
    ok(hr == S_OK, "IGlobalizationPreferencesStatics_get_Currencies failed, hr %#lx\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_HSTRING_get_Size(currencies, &size);
    ok(hr == S_OK, "IVectorView_HSTRING_get_Size failed, hr %#lx\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_HSTRING_get_Size returned %u\n", size);

    IVectorView_HSTRING_Release(currencies);


    IGlobalizationPreferencesStatics_Release(preferences_statics);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    WindowsDeleteString(str);
}

static void test_Language(void)
{
    static const WCHAR *class_name = L"Windows.Globalization.Language";
    static const struct
    {
        const WCHAR *tag;
        enum LanguageLayoutDirection direction;
    }
    lang_layout_direction_tests[] =
    {
        {L"en-US", LanguageLayoutDirection_Ltr},
        {L"ar-SA", LanguageLayoutDirection_Rtl},
    };
    IAgileObject *agile_object, *tmp_agile_object;
    IInspectable *inspectable, *tmp_inspectable;
    enum LanguageLayoutDirection direction;
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
    ILanguageFactory *language_factory;
    IActivationFactory *factory;
    ILanguage2 *language2;
    ILanguage *language;
    HSTRING tag, str;
    const WCHAR *buf;
    unsigned int i;
    HRESULT hr;

    hr = WindowsCreateString(class_name, wcslen(class_name), &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Unexpected hr %#lx.\n", hr);
    WindowsDeleteString(str);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(class_name));
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_ILanguageFactory, (void **)&language_factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ILanguageFactory_QueryInterface(language_factory, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_inspectable == inspectable, "Unexpected interface pointer %p, expected %p.\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = ILanguageFactory_QueryInterface(language_factory, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_agile_object == agile_object, "Unexpected interface pointer %p, expected %p.\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    /* Invalid language tag */
    hr = WindowsCreateString(L"test-tag", 8, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ILanguageFactory_CreateLanguage(language_factory, str, &language);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ILanguage_Release(language);
    WindowsDeleteString(str);

    hr = WindowsCreateString(L"en-us", 5, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ILanguageFactory_CreateLanguage(language_factory, str, &language);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    WindowsDeleteString(str);

    hr = ILanguage_get_LanguageTag(language, &tag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    buf = WindowsGetStringRawBuffer(tag, NULL);
    ok(!wcscmp(buf, L"en-US"), "Unexpected tag %s.\n", debugstr_w(buf));
    GetLocaleInfoEx(L"en-us", LOCALE_SNAME, buffer, ARRAY_SIZE(buffer));
    ok(!wcscmp(buf, buffer), "Unexpected tag %s, locale name %s.\n", debugstr_w(buf), debugstr_w(buffer));

    WindowsDeleteString(tag);

    ILanguage_Release(language);

    /* ILanguage2 */
    for (i = 0; i < ARRAY_SIZE(lang_layout_direction_tests); i++)
    {
        winetest_push_context("%s", wine_dbgstr_w(lang_layout_direction_tests[i].tag));

        hr = WindowsCreateString(lang_layout_direction_tests[i].tag, wcslen(lang_layout_direction_tests[i].tag), &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = ILanguageFactory_CreateLanguage(language_factory, str, &language);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        WindowsDeleteString(str);

        hr = ILanguage_QueryInterface(language, &IID_ILanguage2, (void **)&language2);
        todo_wine
        ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* <= Windows 10 1507 */, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
        {
            ILanguage_Release(language);
            winetest_pop_context();
            break;
        }

        hr = ILanguage2_get_LayoutDirection(language2, &direction);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(direction == lang_layout_direction_tests[i].direction, "Expected direction %d, got %d.\n",
           lang_layout_direction_tests[i].direction, direction);

        ILanguage2_Release(language2);
        ILanguage_Release(language);
        winetest_pop_context();
    }

    ILanguageFactory_Release(language_factory);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);
}

static void test_GeographicRegion(void)
{
    static const WCHAR *class_name = RuntimeClass_Windows_Globalization_GeographicRegion;
    IGeographicRegionFactory *geographic_region_factory;
    IGeographicRegion *geographic_region;
    IActivationFactory *factory;
    HSTRING str, expect_str;
    HRESULT hr;
    INT32 res;
    LONG ref;

    hr = WindowsCreateString( class_name, wcslen( class_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_ActivateInstance( factory, (IInspectable **)&geographic_region );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( geographic_region, &IID_IUnknown );
    check_interface( geographic_region, &IID_IInspectable );
    check_interface( geographic_region, &IID_IAgileObject );

    hr = IGeographicRegion_get_CodeTwoLetter( geographic_region, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    ref = IGeographicRegion_Release( geographic_region );
    ok( ref == 0, "got ref %ld.\n", ref );

    hr = IActivationFactory_QueryInterface( factory, &IID_IGeographicRegionFactory, (void **)&geographic_region_factory );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = WindowsCreateString( L"US", wcslen( L"US" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IGeographicRegionFactory_CreateGeographicRegion( geographic_region_factory, str, &geographic_region );
    todo_wine ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    if (hr == S_OK)
    {
    hr = WindowsCreateString( L"US", wcslen( L"US" ), &expect_str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IGeographicRegion_get_CodeTwoLetter( geographic_region, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, expect_str, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got unexpected string %s.\n", debugstr_hstring(str) );
    WindowsDeleteString( str );
    WindowsDeleteString( expect_str );

    ref = IGeographicRegion_Release( geographic_region );
    ok( ref == 0, "got ref %ld.\n", ref );
    }

    ref = IGeographicRegionFactory_Release( geographic_region_factory );
    ok( ref == 2, "got ref %ld.\n", ref );

    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_ApplicationLanguages(void)
{
    static const WCHAR *class_name = RuntimeClass_Windows_Globalization_ApplicationLanguages;
    IApplicationLanguagesStatics *application_languages_statics;
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    IVectorView_HSTRING *languages;
    IActivationFactory *factory;
    BOOL found = FALSE;
    const WCHAR *ptr;
    UINT32 size, i;
    HSTRING str;
    HRESULT hr;
    LONG ref;
    int ret;

    hr = WindowsCreateString( class_name, wcslen( class_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (FAILED( hr ))
    {
        win_skip( "%s runtimeclass not found, skipping tests.\n", wine_dbgstr_w( class_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );
    check_interface( factory, &IID_IActivationFactory );
    check_interface( factory, &IID_IApplicationLanguagesStatics );

    hr = IActivationFactory_QueryInterface( factory, &IID_IApplicationLanguagesStatics, (void **)&application_languages_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IApplicationLanguagesStatics_get_Languages( application_languages_statics, &languages );
    ok( hr == S_OK || broken( hr == 0x80073d54 ) /* Win8 */, "got hr %#lx.\n", hr );
    if (FAILED( hr ))
        goto done;

    hr = IVectorView_HSTRING_get_Size( languages, &size );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( size > 0, "Got unexpected %u.\n", size );

    ret = GetUserDefaultLocaleName( locale, LOCALE_NAME_MAX_LENGTH );
    ok( ret > 0, "GetUserDefaultLocaleName failed, error %lu.\n", GetLastError() );

    for (i = 0; i < size; i++)
    {
        hr = IVectorView_HSTRING_GetAt( languages, i, &str );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ptr = WindowsGetStringRawBuffer( str, NULL );
        found = !wcscmp( ptr, locale );
        WindowsDeleteString( str );
        if (found)
            break;
    }
    ok( found, "Language not found.\n" );

done:
    ref = IApplicationLanguagesStatics_Release( application_languages_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(globalization)
{
    HMODULE kernel32;
    HRESULT hr;

    kernel32 = GetModuleHandleA("kernel32");
    pGetUserDefaultGeoName = (void*)GetProcAddress(kernel32, "GetUserDefaultGeoName");

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_GlobalizationPreferences();
    test_Language();
    test_GeographicRegion();
    test_ApplicationLanguages();

    RoUninitialize();
}
