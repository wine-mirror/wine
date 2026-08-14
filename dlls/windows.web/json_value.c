/* WinRT Windows.Data.Json.JsonValue Implementation
 *
 * Copyright (C) 2024 Mohamad Al-Jaf
 * Copyright (C) 2026 Olivia Ryan
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
    union
    {
        boolean boolean_value;
        HSTRING string_value;
        double number_value;
        IJsonArray *array_value;
        IJsonObject *object_value;
    };
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
        if (impl->json_value_type == JsonValueType_String)
            WindowsDeleteString( impl->string_value );
        else if (impl->json_value_type == JsonValueType_Array)
            IJsonArray_Release( impl->array_value );
        else if (impl->json_value_type == JsonValueType_Object)
            IJsonObject_Release( impl->object_value );

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

    return WindowsDuplicateString( impl->string_value, value );
}

static HRESULT WINAPI json_value_GetNumber( IJsonValue *iface, DOUBLE *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_Number) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    *value = impl->number_value;
    return S_OK;
}

static HRESULT WINAPI json_value_GetBoolean( IJsonValue *iface, boolean *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_Boolean) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    *value = impl->boolean_value;
    return S_OK;
}

static HRESULT WINAPI json_value_GetArray( IJsonValue *iface, IJsonArray **value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    if (impl->json_value_type != JsonValueType_Array) return E_ILLEGAL_METHOD_CALL;

    IJsonArray_AddRef( impl->array_value );
    *value = impl->array_value;
    return S_OK;
}

static HRESULT WINAPI json_value_GetObject( IJsonValue *iface, IJsonObject **value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    if (impl->json_value_type != JsonValueType_Object) return E_ILLEGAL_METHOD_CALL;

    IJsonObject_AddRef( impl->object_value );
    *value = impl->object_value;
    return S_OK;
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

struct json_buffer
{
    const WCHAR *str;
    UINT32 len;
};

static WCHAR json_buffer_next( struct json_buffer *json, const WCHAR *valid )
{
    const WCHAR chr = *json->str;

    if (!json->len) return 0;
    if (valid && !wcschr( valid, chr )) return 0;
    json->str++;
    json->len--;

    return chr;
}

static BOOL json_buffer_take( struct json_buffer *json, const WCHAR *str, BOOL skip )
{
    static const WCHAR valid_whitespace[] = L" \t\n\r";
    UINT32 len = wcslen( str );

    while (skip && json_buffer_next( json, valid_whitespace )) { /* nothing */ }
    if (json->len < len || wcsncmp( json->str, str, len )) return FALSE;
    json->str += len;
    json->len -= len;

    return TRUE;
}

static HRESULT parse_json_string( struct json_buffer *json, HSTRING *output )
{
    const WCHAR valid_hex_chars[] = L"abcdefABCDEF0123456789";
    WCHAR chr, *buf, *dst;
    HRESULT hr;

    /* validate and escape string, assuming string occupies remainder of buffer */

    if (!json->len) return WEB_E_INVALID_JSON_STRING;
    if (!(buf = calloc( json->len, sizeof( WCHAR )))) return E_OUTOFMEMORY;
    dst = buf;

    while (json->len && *json->str != '"')
    {
        if (json_buffer_take( json, L"\\\"", FALSE ))      *(dst++) = '"';
        else if (json_buffer_take( json, L"\\\\", FALSE )) *(dst++) = '\\';
        else if (json_buffer_take( json, L"\\/", FALSE ))  *(dst++) = '/';
        else if (json_buffer_take( json, L"\\b", FALSE ))  *(dst++) = '\b';
        else if (json_buffer_take( json, L"\\f", FALSE ))  *(dst++) = '\f';
        else if (json_buffer_take( json, L"\\n", FALSE ))  *(dst++) = '\n';
        else if (json_buffer_take( json, L"\\r", FALSE ))  *(dst++) = '\r';
        else if (json_buffer_take( json, L"\\t", FALSE ))  *(dst++) = '\t';
        else if (json_buffer_take( json, L"\\u", FALSE ))
        {
            for (int i = 0; i < 4; i++)
            {
                if (!(chr = json_buffer_next( json, valid_hex_chars )))
                {
                    free( buf );
                    return WEB_E_INVALID_JSON_STRING;
                }

                *dst <<= 4;
                if (chr >= 'A') *dst |= (chr & 0x7) + 9;
                else *dst |= chr & 0xf;
            }
            dst++;
        }
        else if (*json->str >= ' ')
        {
            *(dst++) = *(json->str++);
            json->len--;
        }
        else
        {
            free( buf );
            return WEB_E_INVALID_JSON_STRING;
        }
    }

    hr = WindowsCreateString( buf, dst - buf, output );
    free( buf );
    return hr;
}

static HRESULT parse_json_value( struct json_buffer *json, IJsonValue **value );

static HRESULT parse_json_array( struct json_buffer *json, IJsonArray **value )
{
    IJsonArray *array;
    IJsonValue *child;
    HRESULT hr;

    if (FAILED(hr = IActivationFactory_ActivateInstance( json_array_factory, (IInspectable **)&array ))) return hr;

    while (json->len && *json->str != ']')
    {
        if (FAILED(hr = parse_json_value( json, &child ))) break;
        hr = json_array_push( array, child );
        IJsonValue_Release( child );
        if (FAILED(hr) || !json_buffer_take( json, L",", TRUE )) break;
        if (json_buffer_take( json, L"]", TRUE ))
        {
            hr = WEB_E_INVALID_JSON_STRING;
            break;
        }
    }

    if (FAILED(hr)) IJsonArray_Release( array );
    else *value = array;
    return hr;
}

static HRESULT parse_json_key_value( struct json_buffer *json, HSTRING *key, IJsonValue **value )
{
    HSTRING name;
    HRESULT hr;

    if (!json_buffer_take( json, L"\"", TRUE )) return WEB_E_INVALID_JSON_STRING;
    if (FAILED(hr = parse_json_string( json, &name ))) return hr;

    if (!json_buffer_take( json, L"\"", FALSE )) hr = WEB_E_INVALID_JSON_STRING;
    else if (!json_buffer_take( json, L":", TRUE )) hr = WEB_E_INVALID_JSON_STRING;
    else hr = parse_json_value( json, value );

    if (FAILED(hr)) WindowsDeleteString( name );
    else *key = name;
    return hr;
}

static HRESULT parse_json_object( struct json_buffer *json, IJsonObject **value )
{
    IJsonObject *object;
    HRESULT hr;

    if (FAILED(hr = IActivationFactory_ActivateInstance( json_object_factory, (IInspectable**)&object ))) return hr;

    while (json->len && *json->str != '}')
    {
        IJsonValue *value;
        HSTRING key;

        if (FAILED(hr = parse_json_key_value( json, &key, &value ))) break;
        hr = IJsonObject_SetNamedValue( object, key, value );
        WindowsDeleteString( key );
        IJsonValue_Release( value );
        if (FAILED(hr) || !json_buffer_take( json, L",", TRUE )) break;
        if (json_buffer_take( json, L"}", TRUE ))
        {
            hr = WEB_E_INVALID_JSON_STRING;
            break;
        }
    }

    if (FAILED(hr)) IJsonObject_Release( object );
    else *value = object;
    return hr;
}

static HRESULT parse_json_value( struct json_buffer *json, IJsonValue **value )
{
    struct json_value *impl;
    HRESULT hr = S_OK;

    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;
    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;

    if (json_buffer_take( json, L"null", TRUE ))
    {
        impl->json_value_type = JsonValueType_Null;
    }
    else if (json_buffer_take( json, L"true", TRUE ))
    {
        impl->boolean_value = TRUE;
        impl->json_value_type = JsonValueType_Boolean;
    }
    else if (json_buffer_take( json, L"false", TRUE ))
    {
        impl->boolean_value = FALSE;
        impl->json_value_type = JsonValueType_Boolean;
    }
    else if (json_buffer_take( json, L"\"", TRUE ))
    {
        if (SUCCEEDED(hr = parse_json_string( json, &impl->string_value )))
        {
            impl->json_value_type = JsonValueType_String;
            if (!json_buffer_take( json, L"\"", FALSE )) hr = WEB_E_INVALID_JSON_STRING;
        }
    }
    else if (json_buffer_take( json, L"[", TRUE ))
    {
        if (SUCCEEDED(hr = parse_json_array( json, &impl->array_value )))
        {
            impl->json_value_type = JsonValueType_Array;
            if (!json_buffer_take( json, L"]", TRUE )) hr = WEB_E_INVALID_JSON_STRING;
        }
    }
    else if (json_buffer_take( json, L"{", TRUE ))
    {
        if (SUCCEEDED(hr = parse_json_object( json, &impl->object_value )))
        {
            impl->json_value_type = JsonValueType_Object;
            if (!json_buffer_take( json, L"}", TRUE )) hr = WEB_E_INVALID_JSON_STRING;
        }
    }
    else
    {
        double result = 0;
        WCHAR *end;

        errno = 0;
        result = wcstold( json->str, &end );

        json->len -= end - json->str;
        json->str = end;

        if (errno || errno == ERANGE) hr = WEB_E_INVALID_JSON_NUMBER;

        impl->number_value = result;
        impl->json_value_type = JsonValueType_Number;
    }

    if (FAILED(hr)) IJsonValue_Release( &impl->IJsonValue_iface );
    else *value = &impl->IJsonValue_iface;
    return hr;
}

static HRESULT parse_json( HSTRING string, IJsonValue **value )
{
    HRESULT hr;
    struct json_buffer json;
    json.str = WindowsGetStringRawBuffer( string, &json.len );

    if (FAILED(hr = parse_json_value( &json, value ))) return hr;
    if (!json_buffer_take( &json, L"", TRUE ) || json.len) return WEB_E_INVALID_JSON_STRING;
    return S_OK;
}

static HRESULT WINAPI json_value_statics_Parse( IJsonValueStatics *iface, HSTRING input, IJsonValue **value )
{
    HRESULT hr;

    TRACE( "iface %p, input %s, value %p\n", iface, debugstr_hstring( input ), value );

    if (!value) return E_POINTER;
    if (!input) return WEB_E_INVALID_JSON_STRING;

    if (SUCCEEDED(hr = parse_json( input, value )))
        TRACE( "created IJsonValue %p.\n", *value );

    return hr;
}

static HRESULT WINAPI json_value_statics_TryParse( IJsonValueStatics *iface, HSTRING input, IJsonValue **result, boolean *succeeded )
{
    FIXME( "iface %p, input %s, result %p, succeeded %p stub!\n", iface, debugstr_hstring( input ), result, succeeded );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_statics_CreateBooleanValue( IJsonValueStatics *iface, boolean input, IJsonValue **value )
{
    struct json_value *impl;

    TRACE( "iface %p, input %d, value %p\n", iface, input, value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;
    impl->json_value_type = JsonValueType_Boolean;
    impl->boolean_value = input != FALSE;

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI json_value_statics_CreateNumberValue( IJsonValueStatics *iface, DOUBLE input, IJsonValue **value )
{
    struct json_value *impl;

    TRACE( "iface %p, input %f, value %p\n", iface, input, value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;
    impl->json_value_type = JsonValueType_Number;
    impl->number_value = input;

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
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
