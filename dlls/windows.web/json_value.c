/* WinRT Windows.Data.Json.JsonValue Implementation
 *
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

#include "private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(web);

struct json_value_statics
{
    IActivationFactory IActivationFactory_iface;
    IJsonValueStatics IJsonValueStatics_iface;
    LONG ref;
};

static inline struct json_value_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct json_value_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct json_value_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IJsonValueStatics ))
    {
        *out = &impl->IJsonValueStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct json_value_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct json_value_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

struct json_value
{
    IJsonValue IJsonValue_iface;
    LONG ref;

    JsonValueType json_value_type;
    HSTRING parsed_string;
    double parsed_number;
    boolean parsed_boolean;
    HSTRING string_value;
};

static inline struct json_value *impl_from_IJsonValue( IJsonValue *iface )
{
    return CONTAINING_RECORD( iface, struct json_value, IJsonValue_iface );
}

static HRESULT WINAPI json_value_QueryInterface( IJsonValue *iface, REFIID iid, void **out )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IJsonValue ))
    {
        *out = &impl->IJsonValue_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI json_value_AddRef( IJsonValue *iface )
{
    struct json_value *impl = impl_from_IJsonValue( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI json_value_Release( IJsonValue *iface )
{
    struct json_value *impl = impl_from_IJsonValue( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        WindowsDeleteString( impl->parsed_string );
        WindowsDeleteString( impl->string_value );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI json_value_GetIids( IJsonValue *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetRuntimeClassName( IJsonValue *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetTrustLevel( IJsonValue *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_get_ValueType( IJsonValue *iface, JsonValueType *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;

    *value = impl->json_value_type;
    return S_OK;
}

static HRESULT WINAPI json_value_Stringify( IJsonValue *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetString( IJsonValue *iface, HSTRING *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_String) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    return WindowsDuplicateString( impl->parsed_string, value );
}

static HRESULT WINAPI json_value_GetNumber( IJsonValue *iface, DOUBLE *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_Number) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    *value = impl->parsed_number;
    return S_OK;
}

static HRESULT WINAPI json_value_GetBoolean( IJsonValue *iface, boolean *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_Boolean) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    *value = impl->parsed_boolean;
    return S_OK;
}

static HRESULT WINAPI json_value_GetArray( IJsonValue *iface, IJsonArray **value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    FIXME( "iface %p, value %p stub!\n", iface, value );

    if (!value) return E_POINTER;
    if (impl->json_value_type != JsonValueType_Array) return E_ILLEGAL_METHOD_CALL;

    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetObject( IJsonValue *iface, IJsonObject **value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    FIXME( "iface %p, value %p stub!\n", iface, value );

    if (!value) return E_POINTER;
    if (impl->json_value_type != JsonValueType_Object) return E_ILLEGAL_METHOD_CALL;

    return E_NOTIMPL;
}

static const struct IJsonValueVtbl json_value_vtbl =
{
    json_value_QueryInterface,
    json_value_AddRef,
    json_value_Release,
    /* IInspectable methods */
    json_value_GetIids,
    json_value_GetRuntimeClassName,
    json_value_GetTrustLevel,
    /* IJsonValue methods */
    json_value_get_ValueType,
    json_value_Stringify,
    json_value_GetString,
    json_value_GetNumber,
    json_value_GetBoolean,
    json_value_GetArray,
    json_value_GetObject,
};

DEFINE_IINSPECTABLE( json_value_statics, IJsonValueStatics, struct json_value_statics, IActivationFactory_iface )

static HRESULT unescape_string( const WCHAR *src, HSTRING *output )
{
    UINT32 len = wcslen( src ) - 1, n;
    const WCHAR *end = src + len;
    HSTRING_BUFFER buf;
    HRESULT hr;
    WCHAR *dst;

    for (len = n = 0; len + n < end - src; len++) { if (src[len + n] == '\\') n++; }
    if (FAILED(hr = WindowsPreallocateStringBuffer( len, &dst, &buf ))) return hr;
    while (src != end) { if (*src == '\\' && ++src == end) break; *dst++ = *src++; }

    return WindowsPromoteStringBuffer( buf, output );
}

static HRESULT trim_string( HSTRING input, HSTRING *output )
{
    static const WCHAR valid_whitespace[] = L" \t\n\r";
    UINT32 len, start = 0, end;
    const WCHAR *json = WindowsGetStringRawBuffer( input, &len );

    end = len;
    while (start < end && wcschr( valid_whitespace, json[start] )) start++;
    while (end > start && wcschr( valid_whitespace, json[end - 1] )) end--;

    return WindowsCreateString( json + start, end - start, output );
}

static HRESULT parse_json_value( HSTRING input, struct json_value *impl )
{
    UINT32 len;
    const WCHAR *json = WindowsGetStringRawBuffer( input, &len );
    HRESULT hr = S_OK;

    /* FIXME: Handle all JSON edge cases */

    if (!len) return WEB_E_INVALID_JSON_STRING;

    if (len == 4 && !wcsncmp( L"null", json, 4 ))
    {
        impl->json_value_type = JsonValueType_Null;
    }
    else if ((len == 4 && !wcsncmp( L"true", json, 4 )) || (len == 5 && !wcsncmp( L"false", json, 5 )))
    {
        impl->parsed_boolean = len == 4;
        impl->json_value_type = JsonValueType_Boolean;
    }
    else if (json[0] == '\"' && json[len - 1] == '\"')
    {
        json++;
        len -= 2;

        if (len <= 2) return WEB_E_INVALID_JSON_STRING;
        if (FAILED(hr = unescape_string( json, &impl->parsed_string ))) return hr;

        impl->json_value_type = JsonValueType_String;
    }
    else if (json[0] == '[' && json[len - 1] == ']')
    {
        FIXME( "Array parsing not implemented!\n" );
        impl->json_value_type = JsonValueType_Array;
    }
    else if (json[0] == '{' && json[len - 1] == '}')
    {
        FIXME( "Object parsing not implemented!\n" );
        impl->json_value_type = JsonValueType_Object;
    }
    else
    {
        double result = 0;
        WCHAR *end;

        errno = 0;
        result = wcstold( json, &end );

        if (errno || errno == ERANGE || end != json + len) return WEB_E_INVALID_JSON_NUMBER;

        impl->parsed_number = result;
        impl->json_value_type = JsonValueType_Number;
    }

    return hr;
}

static HRESULT parse_json( HSTRING json, struct json_value *impl )
{
    HSTRING trimmed_json = NULL;
    HRESULT hr = trim_string( json, &trimmed_json );

    if (SUCCEEDED(hr) && WindowsIsStringEmpty( trimmed_json )) hr = WEB_E_INVALID_JSON_STRING;
    if (SUCCEEDED(hr)) hr = parse_json_value( trimmed_json, impl );

    WindowsDeleteString( trimmed_json );
    return hr;
}

static HRESULT WINAPI json_value_statics_Parse( IJsonValueStatics *iface, HSTRING input, IJsonValue **value )
{
    struct json_value *impl;
    HRESULT hr;

    FIXME( "iface %p, input %s, value %p semi-stub\n", iface, debugstr_hstring( input ), value );

    if (!value) return E_POINTER;
    if (!input) return WEB_E_INVALID_JSON_STRING;
    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    if (FAILED(hr = parse_json( input, impl )))
    {
        free( impl );
        return hr;
    }
    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI json_value_statics_TryParse( IJsonValueStatics *iface, HSTRING input, IJsonValue **result, boolean *succeeded )
{
    FIXME( "iface %p, input %s, result %p, succeeded %p stub!\n", iface, debugstr_hstring( input ), result, succeeded );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_statics_CreateBooleanValue( IJsonValueStatics *iface, boolean input, IJsonValue **value )
{
    FIXME( "iface %p, input %d, value %p stub!\n", iface, input, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_statics_CreateNumberValue( IJsonValueStatics *iface, DOUBLE input, IJsonValue **value )
{
    FIXME( "iface %p, input %f, value %p stub!\n", iface, input, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_statics_CreateStringValue( IJsonValueStatics *iface, HSTRING input, IJsonValue **value )
{
    struct json_value *impl;
    HRESULT hr;

    TRACE( "iface %p, input %s, value %p\n", iface, debugstr_hstring( input ), value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;
    impl->json_value_type = JsonValueType_String;
    if (FAILED(hr = WindowsDuplicateString( input, &impl->string_value )))
    {
         free( impl );
         return hr;
    }

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static const struct IJsonValueStaticsVtbl json_value_statics_vtbl =
{
    json_value_statics_QueryInterface,
    json_value_statics_AddRef,
    json_value_statics_Release,
    /* IInspectable methods */
    json_value_statics_GetIids,
    json_value_statics_GetRuntimeClassName,
    json_value_statics_GetTrustLevel,
    /* IJsonValueStatics methods */
    json_value_statics_Parse,
    json_value_statics_TryParse,
    json_value_statics_CreateBooleanValue,
    json_value_statics_CreateNumberValue,
    json_value_statics_CreateStringValue,
};

static struct json_value_statics json_value_statics =
{
    {&factory_vtbl},
    {&json_value_statics_vtbl},
    1,
};

IActivationFactory *json_value_factory = &json_value_statics.IActivationFactory_iface;
