/*
 * Copyright (C) 2024 Mohamad Al-Jaf
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

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Data_Json
#include "windows.data.json.h"

#include "wine/test.h"

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

static void test_JsonObjectStatics(void)
{
    static const WCHAR *json_object_name = L"Windows.Data.Json.JsonObject";
    IActivationFactory *factory = (void *)0xdeadbeef;
    IInspectable *inspectable = (void *)0xdeadbeef;
    IJsonObject *json_object = (void *)0xdeadbeef;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( json_object_name, wcslen( json_object_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( json_object_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IJsonObject, (void **)&json_object );
    ok( hr == E_NOINTERFACE, "got hr %#lx.\n", hr );

    hr = WindowsCreateString( json_object_name, wcslen( json_object_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoActivateInstance( str, &inspectable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    hr = IInspectable_QueryInterface( inspectable, &IID_IJsonObject, (void **)&json_object );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( inspectable, &IID_IAgileObject );

    IJsonObject_Release( json_object );
    IInspectable_Release( inspectable );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_JsonValueStatics(void)
{
    static const WCHAR *json_value_statics_name = L"Windows.Data.Json.JsonValue";
    IJsonValueStatics *json_value_statics = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IJsonValue *json_value = (void *)0xdeadbeef;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( json_value_statics_name, wcslen( json_value_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( json_value_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IJsonValueStatics, (void **)&json_value_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IJsonValueStatics_CreateStringValue( json_value_statics, NULL, (IJsonValue **)&json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (hr == S_OK) IJsonValue_Release( json_value );
    hr = WindowsCreateString( L"Wine", wcslen( L"Wine" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_CreateStringValue( json_value_statics, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_CreateStringValue( json_value_statics, str, (IJsonValue **)&json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    ref = IJsonValue_Release( json_value );
    ok( ref == 0, "got ref %ld.\n", ref );
    ref = IJsonValueStatics_Release( json_value_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(web)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_JsonObjectStatics();
    test_JsonValueStatics();

    RoUninitialize();
}
