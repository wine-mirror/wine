/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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
#include "initguid.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "weakreference.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"
#define WIDL_using_Windows_UI
#include "windows.ui.h"
#define WIDL_using_Windows_UI_ViewManagement
#include "windows.ui.viewmanagement.h"

#include "wine/test.h"

static const WCHAR *subkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
static const WCHAR *name = L"AppsUseLightTheme";
static const HKEY root = HKEY_CURRENT_USER;

#define check_interface( obj, iid, exp ) check_interface_( __LINE__, obj, iid, exp )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL supported )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr, expected_hr;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_( __FILE__, line )( hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static DWORD get_app_theme(void)
{
    DWORD ret = 0, len = sizeof(ret), type;
    HKEY hkey;

    if (RegOpenKeyExW( root, subkey, 0, KEY_QUERY_VALUE, &hkey )) return 1;
    if (RegQueryValueExW( hkey, name, NULL, &type, (BYTE *)&ret, &len ) || type != REG_DWORD) ret = 1;
    RegCloseKey( hkey );
    return ret;
}

static DWORD set_app_theme( DWORD mode )
{
    DWORD ret = 1, len = sizeof(ret);
    HKEY hkey;

    if (RegOpenKeyExW( root, subkey, 0, KEY_SET_VALUE, &hkey )) return 0;
    if (RegSetValueExW( hkey, name, 0, REG_DWORD, (const BYTE *)&mode, len )) ret = 0;
    RegCloseKey( hkey );
    return ret;
}

static BOOL get_accent_palette( DWORD *colors, DWORD *len )
{
    BOOL ret = TRUE;
    DWORD type;
    HKEY hkey;

    if (RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_QUERY_VALUE, &hkey )) return FALSE;
    if (RegQueryValueExW( hkey, L"AccentPalette", NULL, &type, (BYTE *)colors, len ) || type != REG_BINARY) ret = FALSE;
    RegCloseKey( hkey );
    return ret;
}

static BOOL delete_accent_palette(void)
{
    BOOL ret = TRUE;
    HRESULT res;
    HKEY hkey;

    res = RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_SET_VALUE, &hkey );
    if (res != 0) return res == ERROR_FILE_NOT_FOUND;

    res = RegDeleteValueW( hkey, L"AccentPalette" );
    if (res != 0 && res != ERROR_FILE_NOT_FOUND) ret = FALSE;

    RegCloseKey( hkey );
    return ret;
}

static DWORD set_accent_palette( DWORD *colors, DWORD len )
{
    DWORD ret = 1;
    HKEY hkey;

    if (RegOpenKeyExW( HKEY_CURRENT_USER,  L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_SET_VALUE, &hkey )) return 0;
    if (RegSetValueExW( hkey, L"AccentPalette", 0, REG_BINARY, (const BYTE *)&colors, len )) ret = 0;
    RegCloseKey( hkey );
    return ret;
}

static void reset_color( Color *value )
{
    value->A = 1;
    value->R = 1;
    value->G = 1;
    value->B = 1;
}

static BOOL compare_dword_color( Color color, DWORD dword )
{
    return (color.R == (dword & 0xFF) && color.G == ((dword >> 8) & 0xFF) && color.B == ((dword >> 16) & 0xFF));
}

static void test_single_accent( IUISettings3 *uisettings3, UIColorType type, DWORD expected )
{
    Color value;
    HRESULT hr;
    BOOL result;

    reset_color( &value );
    hr = IUISettings3_GetColorValue( uisettings3, type, &value );
    ok( hr == S_OK, "GetColorValue returned %#lx\n", hr );

    result = compare_dword_color( value, expected );
    ok( result, "got unexpected value.A == %d value.R == %d value.G == %d value.B == %d (type = %d)\n", value.A, value.R, value.G, value.B, type );
}

static void test_AccentColor( IUISettings3 *uisettings3 )
{
    DWORD default_palette[8];
    DWORD default_palette_len = sizeof(default_palette);
    DWORD accent_palette[8];
    DWORD accent_palette_len = sizeof(accent_palette);
    Color value;
    HRESULT hr;

    if (!get_accent_palette( default_palette, &default_palette_len )) default_palette_len = 0;

    /* deleting AccentPalette fills a default value */
    ok( delete_accent_palette(), "failed to delete AccentPalette key.\n");
    ok( !get_accent_palette( accent_palette, &accent_palette_len ), "AccentPalette should not be available.\n" );

    hr = IUISettings3_GetColorValue( uisettings3, UIColorType_Accent, &value );
    ok( hr == S_OK, "GetColorValue returned %#lx\n", hr );

    ok( get_accent_palette( accent_palette, &accent_palette_len ), "failed to retrieve AccentPalette key.\n" );

    test_single_accent( uisettings3, UIColorType_Accent, accent_palette[3] );
    test_single_accent( uisettings3, UIColorType_AccentDark1, accent_palette[4] );
    test_single_accent( uisettings3, UIColorType_AccentDark2, accent_palette[5] );
    test_single_accent( uisettings3, UIColorType_AccentDark3, accent_palette[6] );
    test_single_accent( uisettings3, UIColorType_AccentLight1, accent_palette[2] );
    test_single_accent( uisettings3, UIColorType_AccentLight2, accent_palette[1] );
    test_single_accent( uisettings3, UIColorType_AccentLight3, accent_palette[0] );

    if (default_palette_len) set_accent_palette( default_palette, default_palette_len );
}

static void test_UISettings(void)
{
    static const WCHAR *uisettings_name = L"Windows.UI.ViewManagement.UISettings";
    IActivationFactory *factory;
    IUISettings3 *uisettings3;
    IInspectable *inspectable;
    DWORD default_theme;
    UIColorType type;
    Color value;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( uisettings_name, wcslen( uisettings_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( uisettings_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IUISettings3, FALSE );

    hr = RoActivateInstance( str, &inspectable );
    ok( hr == S_OK, "Got unexpected hr %#lx.\n", hr );
    WindowsDeleteString( str );

    hr = IInspectable_QueryInterface( inspectable, &IID_IUISettings3, (void **)&uisettings3 );
    ok( hr == S_OK || broken( hr == E_NOINTERFACE ), "Got unexpected hr %#lx.\n", hr );
    if (FAILED(hr))
    {
        win_skip( "IUISettings3 not supported.\n" );
        goto skip_uisettings3;
    }

    check_interface( inspectable, &IID_IInspectable, TRUE );
    check_interface( inspectable, &IID_IAgileObject, TRUE );
    check_interface( inspectable, &IID_IUISettings, TRUE );
    check_interface( inspectable, &IID_IUISettings2, TRUE );
    check_interface( inspectable, &IID_IUISettings3, TRUE );
    check_interface( inspectable, &IID_IWeakReferenceSource, TRUE );
    check_interface( inspectable, &IID_IWeakReference, FALSE );

    test_AccentColor( uisettings3 );

    default_theme = get_app_theme();

    /* Light Theme */
    if (!set_app_theme( 1 )) goto done;

    reset_color( &value );
    type = UIColorType_Foreground;
    hr = IUISettings3_GetColorValue( uisettings3, type, &value );
    ok( hr == S_OK, "GetColorValue returned %#lx\n", hr );
    ok( value.A == 255 && value.R == 0 && value.G == 0 && value.B == 0,
        "got unexpected value.A == %d value.R == %d value.G == %d value.B == %d\n", value.A, value.R, value.G, value.B );

    reset_color( &value );
    type = UIColorType_Background;
    hr = IUISettings3_GetColorValue( uisettings3, type, &value );
    ok( hr == S_OK, "GetColorValue returned %#lx\n", hr );
    ok( value.A == 255 && value.R == 255 && value.G == 255 && value.B == 255,
        "got unexpected value.A == %d value.R == %d value.G == %d value.B == %d\n", value.A, value.R, value.G, value.B );

    /* Dark Theme */
    if (!set_app_theme( 0 )) goto done;

    reset_color( &value );
    type = UIColorType_Foreground;
    hr = IUISettings3_GetColorValue( uisettings3, type, &value );
    ok( hr == S_OK, "GetColorValue returned %#lx\n", hr );
    ok( value.A == 255 && value.R == 255 && value.G == 255 && value.B == 255,
        "got unexpected value.A == %d value.R == %d value.G == %d value.B == %d\n", value.A, value.R, value.G, value.B );

    reset_color( &value );
    type = UIColorType_Background;
    hr = IUISettings3_GetColorValue( uisettings3, type, &value );
    ok( hr == S_OK, "GetColorValue returned %#lx\n", hr );
    ok( value.A == 255 && value.R == 0 && value.G == 0 && value.B == 0,
        "got unexpected value.A == %d value.R == %d value.G == %d value.B == %d\n", value.A, value.R, value.G, value.B );

done:
    set_app_theme( default_theme );
    IUISettings3_Release( uisettings3 );

skip_uisettings3:
    IInspectable_Release( inspectable );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_UISettings_weak_ref(void)
{
    static const WCHAR *uisettings_name = L"Windows.UI.ViewManagement.UISettings";
    IWeakReferenceSource *weak_reference_source;
    IInspectable *inspectable, *inspectable2;
    IWeakReference *weak_reference;
    IActivationFactory *factory;
    IUISettings *uisettings;
    IUnknown *unknown;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( uisettings_name, wcslen( uisettings_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( uisettings_name ) );
        return;
    }

    hr = RoActivateInstance( str, &inspectable );
    ok( hr == S_OK, "Got unexpected hr %#lx.\n", hr );
    WindowsDeleteString( str );

    hr = IInspectable_QueryInterface( inspectable, &IID_IWeakReferenceSource, (void **)&weak_reference_source );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IWeakReferenceSource_GetWeakReference( weak_reference_source, &weak_reference );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IWeakReferenceSource_Release( weak_reference_source );

    check_interface( weak_reference, &IID_IUnknown, TRUE );
    check_interface( weak_reference, &IID_IWeakReference, TRUE );
    check_interface( weak_reference, &IID_IInspectable, FALSE );
    check_interface( weak_reference, &IID_IAgileObject, FALSE );
    check_interface( weak_reference, &IID_IUISettings, FALSE );

    hr = IWeakReference_Resolve( weak_reference, &IID_IUnknown, (IInspectable **)&unknown );
    ok( hr == S_OK && unknown, "got hr %#lx.\n", hr );
    hr = IWeakReference_Resolve( weak_reference, &IID_IInspectable, &inspectable2 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( (void *)inspectable2 == (void *)unknown, "Interfaces are not the same.\n" );
    IInspectable_Release( inspectable2 );
    hr = IWeakReference_Resolve( weak_reference, &IID_IUISettings, (IInspectable **)&uisettings );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( (void *)uisettings == (void *)unknown, "Interfaces are not the same.\n" );
    IUISettings_Release( uisettings );
    IUnknown_Release( unknown );

    /* Free inspectable, weak reference should fail to resolve now */
    IInspectable_Release( inspectable );

    inspectable2 = (void *)0xdeadbeef;
    hr = IWeakReference_Resolve( weak_reference, &IID_IInspectable, &inspectable2 );
    ok( hr == S_OK && !inspectable2, "got hr %#lx.\n", hr );

    IWeakReference_Release( weak_reference );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_AccessibilitySettings(void)
{
    static const WCHAR *class_name = RuntimeClass_Windows_UI_ViewManagement_AccessibilitySettings;
    HIGHCONTRASTW high_contrast = {0};
    IAccessibilitySettings *settings;
    IActivationFactory *factory;
    IInspectable *inspectable;
    boolean value;
    HSTRING str;
    HRESULT hr;
    BOOL ret;
    LONG ref;

    hr = WindowsCreateString( class_name, wcslen( class_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (FAILED( hr ))
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( class_name ) );
        WindowsDeleteString( str );
        return;
    }

    check_interface( factory, &IID_IUnknown, TRUE );
    check_interface( factory, &IID_IInspectable, TRUE );
    check_interface( factory, &IID_IActivationFactory, TRUE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IAccessibilitySettings, FALSE );

    hr = RoActivateInstance( str, &inspectable );
    ok( hr == S_OK, "Got unexpected hr %#lx.\n", hr );
    WindowsDeleteString( str );

    hr = IInspectable_QueryInterface( inspectable, &IID_IAccessibilitySettings, (void **)&settings );
    ok( hr == S_OK, "Got unexpected hr %#lx.\n", hr );

    check_interface( inspectable, &IID_IUnknown, TRUE );
    check_interface( inspectable, &IID_IInspectable, TRUE );
    check_interface( inspectable, &IID_IAgileObject, TRUE );
    check_interface( inspectable, &IID_IAccessibilitySettings, TRUE );

    hr = IAccessibilitySettings_get_HighContrast( settings, &value );
    todo_wine
    ok( hr == S_OK, "Got unexpected hr %#lx.\n", hr );
    if ( hr == S_OK )
    {
        high_contrast.cbSize = sizeof(high_contrast);
        ret = SystemParametersInfoW( SPI_GETHIGHCONTRAST, sizeof(high_contrast), &high_contrast, 0 );
        ok( ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError() );
        ok( value == !!(high_contrast.dwFlags & HCF_HIGHCONTRASTON), "Got unexpected high contrast value.\n" );
    }

    IAccessibilitySettings_Release( settings );
    IInspectable_Release( inspectable );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(uisettings)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_UISettings();
    test_UISettings_weak_ref();
    test_AccessibilitySettings();

    RoUninitialize();
}
