/* WinRT Windows.Data.Json.JsonArray Implementation
 *
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

struct json_array
{
    IJsonArray IJsonArray_iface;
    LONG ref;
    IJsonValue **elements;
    ULONG capacity;
    ULONG length;
};

static inline struct json_array *impl_from_IJsonArray( IJsonArray *iface )
{
    return CONTAINING_RECORD( iface, struct json_array, IJsonArray_iface );
}

HRESULT json_array_push( IJsonArray *iface, IJsonValue *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (impl->length == impl->capacity)
    {
        UINT32 capacity = max( 32, impl->capacity * 3 / 2 );
        IJsonValue **new = impl->elements;
        if (!(new = realloc( new, capacity * sizeof(*new) ))) return E_OUTOFMEMORY;
        impl->elements = new;
        impl->capacity = capacity;
    }

    impl->elements[impl->length++] = value;
    IJsonValue_AddRef( value );
    return S_OK;
}

static HRESULT WINAPI json_array_QueryInterface( IJsonArray *iface, REFIID iid, void **out )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IJsonArray ))
    {
        *out = &impl->IJsonArray_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI json_array_AddRef( IJsonArray *iface )
{
    struct json_array *impl = impl_from_IJsonArray( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI json_array_Release( IJsonArray *iface )
{
    struct json_array *impl = impl_from_IJsonArray( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        for (UINT32 i = 0; i < impl->length; i++)
            IJsonValue_Release( impl->elements[i] );

        free( impl->elements );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI json_array_GetIids( IJsonArray *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_GetRuntimeClassName( IJsonArray *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_GetTrustLevel( IJsonArray *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_array_GetObjectAt( IJsonArray *iface, UINT32 index, IJsonObject **value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetObject( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetArrayAt( IJsonArray *iface, UINT32 index, IJsonArray **value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetArray( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetStringAt( IJsonArray *iface, UINT32 index, HSTRING *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetString( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetNumberAt( IJsonArray *iface, UINT32 index, DOUBLE *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetNumber( impl->elements[index], value );
}

static HRESULT WINAPI json_array_GetBooleanAt( IJsonArray *iface, UINT32 index, boolean *value )
{
    struct json_array *impl = impl_from_IJsonArray( iface );

    TRACE( "iface %p, index %u, value %p\n", iface, index, value );

    if (!value) return E_INVALIDARG;
    if (index >= impl->length) return E_BOUNDS;

    return IJsonValue_GetBoolean( impl->elements[index], value );
}

static const struct IJsonArrayVtbl json_array_vtbl =
{
    json_array_QueryInterface,
    json_array_AddRef,
    json_array_Release,
    /* IInspectable methods */
    json_array_GetIids,
    json_array_GetRuntimeClassName,
    json_array_GetTrustLevel,
    /* IJsonArray methods */
    json_array_GetObjectAt,
    json_array_GetArrayAt,
    json_array_GetStringAt,
    json_array_GetNumberAt,
    json_array_GetBooleanAt,
};

struct json_array_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct json_array_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct json_array_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct json_array_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct json_array_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct json_array_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
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
    struct json_array *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    *instance = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonArray_iface.lpVtbl = &json_array_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IJsonArray_iface;
    return S_OK;
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

static struct json_array_statics json_array_statics =
{
    {&factory_vtbl},
    1,
};

IActivationFactory *json_array_factory = &json_array_statics.IActivationFactory_iface;
