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

static void test_JsonArrayStatics(void)
{
    static const WCHAR *json_value_statics_name = L"Windows.Data.Json.JsonValue";
    static const WCHAR *json_array_name = L"Windows.Data.Json.JsonArray";
    IJsonValueStatics *json_value_statics = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IInspectable *inspectable = (void *)0xdeadbeef;
    IJsonObject *child_object = (void *)0xdeadbeef;
    IJsonArray *child_array = (void *)0xdeadbeef;
    IJsonArray *json_array = (void *)0xdeadbeef;
    IJsonValue *json_value = (void *)0xdeadbeef;
    BOOLEAN child_boolean;
    HSTRING child_string;
    DOUBLE child_number;
    HSTRING str = NULL;
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

    hr = IActivationFactory_QueryInterface( factory, &IID_IJsonValueStatics, (void **)&json_value_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IActivationFactory_Release( factory );

    hr = WindowsCreateString( json_array_name, wcslen( json_array_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( json_array_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IJsonArray, (void **)&json_array );
    ok( hr == E_NOINTERFACE, "got hr %#lx.\n", hr );

    hr = WindowsCreateString( json_array_name, wcslen( json_array_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoActivateInstance( str, &inspectable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    hr = IInspectable_QueryInterface( inspectable, &IID_IJsonArray, (void **)&json_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( inspectable, &IID_IAgileObject );

    hr = IJsonArray_GetObjectAt( json_array, 0, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetObjectAt( json_array, 0, &child_object );
    ok( hr == E_BOUNDS, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetArrayAt( json_array, 0, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetArrayAt( json_array, 0, &child_array );
    ok( hr == E_BOUNDS, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetStringAt( json_array, 0, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetStringAt( json_array, 0, &child_string );
    ok( hr == E_BOUNDS, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetNumberAt( json_array, 0, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetNumberAt( json_array, 0, &child_number );
    ok( hr == E_BOUNDS, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, &child_boolean );
    ok( hr == E_BOUNDS, "got hr %#lx.\n", hr );

    IJsonArray_Release( json_array );
    IInspectable_Release( inspectable );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );

    hr = WindowsCreateString( L"[{}]", wcslen( L"[{}]" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = IJsonValue_GetArray( json_value, &json_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( json_value );
    hr = IJsonArray_GetObjectAt( json_array, 0, &child_object );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonObject_Release( child_object );
    hr = IJsonArray_GetArrayAt( json_array, 0, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetStringAt( json_array, 0, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetNumberAt( json_array, 0, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    IJsonArray_Release( json_array );

    hr = WindowsCreateString( L"[[]]", wcslen( L"[[]]" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = IJsonValue_GetArray( json_value, &json_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( json_value );
    hr = IJsonArray_GetObjectAt( json_array, 0, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetArrayAt( json_array, 0, &child_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonArray_Release( child_array );
    hr = IJsonArray_GetStringAt( json_array, 0, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetNumberAt( json_array, 0, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    IJsonArray_Release( json_array );

    hr = WindowsCreateString( L"[\"Hello, World!\"]", wcslen( L"[\"Hello, World!\"]" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = IJsonValue_GetArray( json_value, &json_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( json_value );
    hr = IJsonArray_GetObjectAt( json_array, 0, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetArrayAt( json_array, 0, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetStringAt( json_array, 0, &child_string );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( child_string );
    hr = IJsonArray_GetNumberAt( json_array, 0, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    IJsonArray_Release( json_array );

    hr = WindowsCreateString( L"[12.6]", wcslen( L"[12.6]" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = IJsonValue_GetArray( json_value, &json_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( json_value );
    hr = IJsonArray_GetObjectAt( json_array, 0, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetArrayAt( json_array, 0, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetStringAt( json_array, 0, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetNumberAt( json_array, 0, &child_number );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    IJsonArray_Release( json_array );

    hr = WindowsCreateString( L"[true]", wcslen( L"[true]" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = IJsonValue_GetArray( json_value, &json_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( json_value );
    hr = IJsonArray_GetObjectAt( json_array, 0, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetArrayAt( json_array, 0, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetStringAt( json_array, 0, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetNumberAt( json_array, 0, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonArray_GetBooleanAt( json_array, 0, &child_boolean );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonArray_Release( json_array );

    IJsonValueStatics_Release( json_value_statics );
}

static void test_JsonObjectStatics(void)
{
    static const WCHAR *json_value_statics_name = L"Windows.Data.Json.JsonValue";
    static const WCHAR *json_object_name = L"Windows.Data.Json.JsonObject";
    IJsonValueStatics *json_value_statics = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IInspectable *inspectable = (void *)0xdeadbeef;
    IJsonObject *child_object = (void *)0xdeadbeef;
    IJsonObject *json_object = (void *)0xdeadbeef;
    IJsonArray *child_array = (void *)0xdeadbeef;
    IJsonValue *child_value = (void *)0xdeadbeef;
    BOOLEAN child_boolean;
    HSTRING child_string;
    DOUBLE child_number;
    HSTRING str = NULL;
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

    hr = IActivationFactory_QueryInterface( factory, &IID_IJsonValueStatics, (void **)&json_value_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IActivationFactory_Release( factory );

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
    IInspectable_Release( inspectable );

    hr = WindowsCreateString( L"key", wcslen( L"key" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    /* key pair does not exist */

    hr = IJsonObject_GetNamedValue( json_object, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedValue( json_object, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedValue( json_object, NULL, &child_value );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedValue( json_object, str, &child_value );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );

    hr = IJsonObject_GetNamedObject( json_object, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedObject( json_object, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedObject( json_object, NULL, &child_object );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedObject( json_object, str, &child_object );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );

    hr = IJsonObject_GetNamedArray( json_object, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, NULL, &child_array );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, str, &child_array );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );

    hr = IJsonObject_GetNamedString( json_object, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, NULL, &child_string );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, str, &child_string );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );

    hr = IJsonObject_GetNamedNumber( json_object, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, NULL, &child_number );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, str, &child_number );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );

    hr = IJsonObject_GetNamedBoolean( json_object, NULL, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, NULL, &child_boolean );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, &child_boolean );
    ok( hr == WEB_E_JSON_VALUE_NOT_FOUND, "got hr %#lx.\n", hr );

    /* key pair exists */

    WindowsDeleteString( str );
    hr = WindowsCreateString( L"{}", wcslen( L"{}" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = WindowsCreateString( L"key", wcslen( L"key" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_SetNamedValue( json_object, str, child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedValue( json_object, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedObject( json_object, str, &child_object );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonObject_Release( child_object );
    hr = IJsonObject_GetNamedArray( json_object, str, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, str, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, str, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

    WindowsDeleteString( str );
    hr = WindowsCreateString( L"[]", wcslen( L"[]" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    hr = WindowsCreateString( L"key", wcslen( L"key" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_SetNamedValue( json_object, str, child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedValue( json_object, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedObject( json_object, str, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, str, &child_array );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonArray_Release( child_array );
    hr = IJsonObject_GetNamedString( json_object, str, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, str, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

    hr = IJsonValueStatics_CreateStringValue( json_value_statics, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_SetNamedValue( json_object, str, child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedValue( json_object, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedObject( json_object, str, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, str, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, str, &child_string );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( child_string );
    hr = IJsonObject_GetNamedNumber( json_object, str, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

    hr = IJsonValueStatics_CreateNumberValue( json_value_statics, 10, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_SetNamedValue( json_object, str, child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedValue( json_object, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedObject( json_object, str, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, str, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, str, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, str, &child_number );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, &child_boolean );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

    hr = IJsonValueStatics_CreateBooleanValue( json_value_statics, FALSE, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonObject_SetNamedValue( json_object, str, child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedValue( json_object, str, &child_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IJsonValue_Release( child_value );
    hr = IJsonObject_GetNamedObject( json_object, str, &child_object );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedArray( json_object, str, &child_array );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedString( json_object, str, &child_string );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedNumber( json_object, str, &child_number );
    ok( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
    hr = IJsonObject_GetNamedBoolean( json_object, str, &child_boolean );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    IJsonObject_Release( json_object );
    IJsonValueStatics_Release( json_value_statics );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

#define check_json( json_value_statics, json, expected_json_value_type, valid ) check_json_( __LINE__, json_value_statics, json, expected_json_value_type, valid )
static void check_json_( unsigned int line, IJsonValueStatics *json_value_statics, const WCHAR *json, JsonValueType expected_json_value_type, boolean valid )
{
    HSTRING str = NULL, parsed_str = NULL, empty_space = NULL;
    IJsonObject *json_object = (void *)0xdeadbeef;
    IJsonArray *json_array = (void *)0xdeadbeef;
    IJsonValue *json_value = (void *)0xdeadbeef;
    boolean parsed_boolean, expected_boolean;
    JsonValueType json_value_type;
    DOUBLE parsed_num;
    HRESULT hr;
    LONG ref;
    int res;

    hr = WindowsCreateString( json, wcslen( json ), &str );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    if (!valid)
    {
        if (expected_json_value_type == JsonValueType_Number)
            ok_(__FILE__, line)( hr == WEB_E_INVALID_JSON_NUMBER, "got hr %#lx.\n", hr );
        else
            ok_(__FILE__, line)( hr == WEB_E_INVALID_JSON_STRING, "got hr %#lx.\n", hr );

        WindowsDeleteString( str );
        return;
    }

    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    if (FAILED(hr)) return;
    hr = IJsonValue_get_ValueType( json_value, &json_value_type );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    ok_(__FILE__, line)( json_value_type == expected_json_value_type, "got json_value_type %d.\n", json_value_type );

    switch (expected_json_value_type)
    {
        case JsonValueType_Null:
            hr = IJsonValue_GetString( json_value, NULL );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetString( json_value, &parsed_str );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

            hr = IJsonValue_GetNumber( json_value, NULL );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetNumber( json_value, &parsed_num );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

            hr = IJsonValue_GetBoolean( json_value, NULL );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetBoolean( json_value, &parsed_boolean );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );

            hr = IJsonValue_GetArray( json_value, NULL );
            ok_(__FILE__, line)( hr == E_POINTER, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetArray( json_value, &json_array );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
            if (hr == S_OK) IJsonArray_Release( json_array );

            hr = IJsonValue_GetObject( json_value, NULL );
            ok_(__FILE__, line)( hr == E_POINTER, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetObject( json_value, &json_object );
            ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "got hr %#lx.\n", hr );
            if (hr == S_OK) IJsonObject_Release( json_object );
            break;
        case JsonValueType_Boolean:
            hr = WindowsCreateString( L" ", wcslen( L" " ), &empty_space );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            hr = WindowsTrimStringStart( str, empty_space, &parsed_str );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            hr = WindowsTrimStringEnd( parsed_str, empty_space, &parsed_str );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            expected_boolean = !wcscmp( L"true", WindowsGetStringRawBuffer( parsed_str, NULL ) );

            hr = IJsonValue_GetBoolean( json_value, NULL );
            ok_(__FILE__, line)( hr == E_POINTER, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetBoolean( json_value, &parsed_boolean );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            ok_(__FILE__, line)( parsed_boolean == expected_boolean, "boolean mismatch, got %d, expected %d.\n", parsed_boolean, expected_boolean );
            break;
        case JsonValueType_Number:
            parsed_num = 0xdeadbeef;
            hr = IJsonValue_GetNumber( json_value, NULL );
            ok_(__FILE__, line)( hr == E_POINTER, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetNumber( json_value, &parsed_num );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            ok_(__FILE__, line)( parsed_num != 0xdeadbeef, "failed to get parsed_num\n" );
            break;
        case JsonValueType_String:
            hr = IJsonValue_GetString( json_value, NULL );
            ok_(__FILE__, line)( hr == E_POINTER, "got hr %#lx.\n", hr );
            hr = IJsonValue_GetString( json_value, &parsed_str );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            hr = WindowsCompareStringOrdinal( str, parsed_str, &res );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            ok_(__FILE__, line)( res != 0, "got same HSTRINGS str = %s, parsed_str = %s.\n", wine_dbgstr_hstring( str ), wine_dbgstr_hstring( parsed_str ) );
            break;
        case JsonValueType_Array:
            hr = IJsonValue_GetArray( json_value, &json_array );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            if (hr == S_OK) IJsonArray_Release( json_array );
            break;
        case JsonValueType_Object:
            hr = IJsonValue_GetObject( json_value, &json_object );
            ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
            if (hr == S_OK) IJsonObject_Release( json_object );
            break;
    }

    WindowsDeleteString( empty_space );
    WindowsDeleteString( parsed_str );
    WindowsDeleteString( str );
    ref = IJsonValue_Release( json_value );
    ok_(__FILE__, line)( ref == 0, "got ref %ld.\n", ref );
}

WCHAR *create_non_null_terminated( const WCHAR *str )
{
    UINT len = wcslen( str );
    WCHAR *buffer = malloc( (len + 1) * sizeof( WCHAR ) );
    if (buffer)
    {
        memcpy( buffer, str, len * sizeof( WCHAR ) );
        buffer[len] = 1;
        return buffer;
    }
    trace( "create_non_null_terminated failed to return a string\n" );
    return NULL;
}

#define check_non_null_terminated_json( json_value_statics, json, expected_json_value_type ) \
        check_non_null_terminated_json_( __LINE__, json_value_statics, json, expected_json_value_type )
static void check_non_null_terminated_json_( unsigned int line, IJsonValueStatics *json_value_statics, const WCHAR *json, JsonValueType expected_json_value_type )
{
    WCHAR *str = create_non_null_terminated( json );
    check_json_( line, json_value_statics, str, expected_json_value_type, FALSE );
    free( str );
}

static void test_JsonValueStatics(void)
{
    static const WCHAR *json_value_statics_name = L"Windows.Data.Json.JsonValue";
    IJsonValueStatics *json_value_statics = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IJsonValue *json_value = (void *)0xdeadbeef;
    JsonValueType json_value_type;
    HSTRING str = NULL;
    const WCHAR *json;
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

    hr = IJsonValueStatics_CreateBooleanValue( json_value_statics, FALSE, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_CreateBooleanValue( json_value_statics, FALSE, &json_value );
    ok( hr == S_OK, "got hr %#lx,\n", hr );
    hr = IJsonValue_get_ValueType( json_value, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValue_get_ValueType( json_value, &json_value_type );
    ok( json_value_type == JsonValueType_Boolean, "got JsonValueType %d.\n", json_value_type );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IJsonValue_Release( json_value );
    ok( ref == 0, "got ref %ld.\n", ref );

    hr = IJsonValueStatics_CreateNumberValue( json_value_statics, 0, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_CreateNumberValue( json_value_statics, 0, &json_value );
    ok( hr == S_OK, "got hr %#lx,\n", hr );
    hr = IJsonValue_get_ValueType( json_value, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValue_get_ValueType( json_value, &json_value_type );
    ok( json_value_type == JsonValueType_Number, "got JsonValueType %d.\n", json_value_type );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IJsonValue_Release( json_value );
    ok( ref == 0, "got ref %ld.\n", ref );

    hr = IJsonValueStatics_CreateStringValue( json_value_statics, NULL, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValue_get_ValueType( json_value, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValue_get_ValueType( json_value, &json_value_type );
    ok( json_value_type == JsonValueType_String, "got JsonValueType %d.\n", json_value_type );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IJsonValue_Release( json_value );
    ok( ref == 0, "got ref %ld.\n", ref );
    hr = WindowsCreateString( L"Wine", wcslen( L"Wine" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_CreateStringValue( json_value_statics, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_CreateStringValue( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValue_get_ValueType( json_value, &json_value_type );
    ok( json_value_type == JsonValueType_String, "got JsonValueType %d.\n", json_value_type );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    ref = IJsonValue_Release( json_value );
    ok( ref == 0, "got ref %ld.\n", ref );

    hr = IJsonValueStatics_Parse( json_value_statics, NULL, &json_value );
    ok( hr == WEB_E_INVALID_JSON_STRING, "got hr %#lx.\n", hr );
    hr = WindowsCreateString( L"Wine", wcslen( L"Wine" ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == WEB_E_INVALID_JSON_STRING, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    /* Valid JSON */

    json = L"\"Wine\\\"\"";
    hr = WindowsCreateString( json, wcslen( json ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    if (SUCCEEDED(hr))
    {
        HSTRING parsed_str = NULL;
        int res;

        json = L"Wine\"";
        hr = WindowsCreateString( json, wcslen( json ), &str );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        hr = IJsonValue_GetString( json_value, &parsed_str );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        hr = WindowsCompareStringOrdinal( str, parsed_str, &res );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( res == 0, "got different HSTRINGS str = %s, parsed_str = %s.\n", wine_dbgstr_hstring( str ), wine_dbgstr_hstring( parsed_str ) );

        WindowsDeleteString( parsed_str );
        WindowsDeleteString( str );
        IJsonValue_Release( json_value );
    }

    json = L"\"\\\"\\\\\\/\\b\\f\\n\\r\\t\\u0000\\u0057\\u0069\\u006e\\u0065\\udAbC\\uDcEf\"";
    hr = WindowsCreateString( json, wcslen( json ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );
    if (SUCCEEDED(hr))
    {
        HSTRING parsed_str = NULL;
        int res;

        json = L"\"\\/\b\f\n\r\t\0Wine\U000BF0EF";
        hr = WindowsCreateString( json, 15, &str );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        hr = IJsonValue_GetString( json_value, &parsed_str );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        hr = WindowsCompareStringOrdinal( str, parsed_str, &res );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( res == 0, "got different HSTRINGS str = %s, parsed_str = %s.\n", wine_dbgstr_hstring( str ), wine_dbgstr_hstring( parsed_str ) );

        WindowsDeleteString( parsed_str );
        WindowsDeleteString( str );
        IJsonValue_Release( json_value );
    }

    json = L"null";
    check_json( json_value_statics, json, JsonValueType_Null, TRUE );
    json = L"false";
    check_json( json_value_statics, json, JsonValueType_Boolean, TRUE );
    json = L" true ";
    check_json( json_value_statics, json, JsonValueType_Boolean, TRUE );
    json = L"\"true\"";
    check_json( json_value_statics, json, JsonValueType_String, TRUE );
    json = L" 9.22 ";
    check_json( json_value_statics, json, JsonValueType_Number, TRUE );
    json = L" \"The Wine     Project\"";
    check_json( json_value_statics, json, JsonValueType_String, TRUE );
    json = L"\r\t\n \"The Wine     Project\"";
    check_json( json_value_statics, json, JsonValueType_String, TRUE );
    json = L"[\"Wine\", \"Linux\"]";
    check_json( json_value_statics, json, JsonValueType_Array, TRUE );
    json = L"{"
            "    \"Wine\": \"The Wine Project\","
            "    \"Linux\": [\"Arch\", \"BTW\"]"
            "}";
    check_json( json_value_statics, json, JsonValueType_Object, TRUE );

    /* Invalid JSON */

    json = L"null";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_Null );
    json = L"false";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_Boolean );
    json = L" true ";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_Boolean );
    json = L"\"true\"";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_String );
    json = L" 9.22 ";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_String );
    json = L" \"Wine\"";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_String );
    json = L"[\"Wine\", \"Linux\"]";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_Array );
    json = L"{"
            "    \"Wine\": \"The Wine Project\","
            "    \"Linux\": [\"Arch\", \"BTW\"]"
            "}";
    check_non_null_terminated_json( json_value_statics, json, JsonValueType_Object );

    json = L"\" \"\"";
    hr = WindowsCreateString( json, wcslen( json ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IJsonValueStatics_Parse( json_value_statics, str, &json_value );
    ok( hr == WEB_E_INVALID_JSON_STRING, "got hr %#lx.\n", hr );
    WindowsDeleteString( str );

    json = L"True";
    check_json( json_value_statics, json, JsonValueType_Boolean, FALSE );
    json = L"1.7976931348623158e+3080";
    check_json( json_value_statics, json, JsonValueType_Number, FALSE );
    json = L"2.2250738585072014e-3080";
    check_json( json_value_statics, json, JsonValueType_Number, FALSE );
    json = L" \"Wine\":";
    check_json( json_value_statics, json, JsonValueType_String, FALSE );
    json = L" \"The Wine \t Project\"";
    check_json( json_value_statics, json, JsonValueType_String, FALSE );
    json = L"\v \"The Wine     Project\"";
    check_json( json_value_statics, json, JsonValueType_String, FALSE );
    json = L"\"\\\"";
    check_json( json_value_statics, json, JsonValueType_String, FALSE );
    json = L"\"\\u123\"";
    check_json( json_value_statics, json, JsonValueType_String, FALSE );
    json = L"[\"Wine\" \"Linux\"]";
    check_json( json_value_statics, json, JsonValueType_Array, FALSE );
    json = L"[\"Wine\", \"Linux\",]";
    check_json( json_value_statics, json, JsonValueType_Array, FALSE );
    json = L"{"
            "    \"Wine\": \"The Wine Project\","
            "    \"Linux\": [\"Arch\", \"BTW\"]"
            "";
    check_json( json_value_statics, json, JsonValueType_Object, FALSE );
    json = L"{"
            "    \"Wine\": \"The Wine Project\","
            "    \"Linux\": [\"Arch\", \"BTW\"],"
            "}";
    check_json( json_value_statics, json, JsonValueType_Object, FALSE );

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

    test_JsonArrayStatics();
    test_JsonObjectStatics();
    test_JsonValueStatics();

    RoUninitialize();
}
