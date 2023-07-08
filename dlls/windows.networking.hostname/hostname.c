/* WinRT Windows.Networking.Hostname Hostname Implementation
 *
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

#include "private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hostname);

struct hostname_statics
{
    IActivationFactory IActivationFactory_iface;
    IHostNameFactory IHostNameFactory_iface;
    LONG ref;
};

static inline struct hostname_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct hostname_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct hostname_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IHostNameFactory ))
    {
        *out = &impl->IHostNameFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct hostname_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct hostname_statics *impl = impl_from_IActivationFactory( iface );
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

struct hostname
{
    IHostName IHostName_iface;
    LONG ref;

    HSTRING raw_name;
};

static inline struct hostname *impl_from_IHostName( IHostName *iface )
{
    return CONTAINING_RECORD( iface, struct hostname, IHostName_iface );
}

static HRESULT WINAPI hostname_QueryInterface( IHostName *iface, REFIID iid, void **out )
{
    struct hostname *impl = impl_from_IHostName( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IHostName ))
    {
        *out = &impl->IHostName_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI hostname_AddRef( IHostName *iface )
{
    struct hostname *impl = impl_from_IHostName( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI hostname_Release( IHostName *iface )
{
    struct hostname *impl = impl_from_IHostName( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        WindowsDeleteString( impl->raw_name );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI hostname_GetIids( IHostName *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_GetRuntimeClassName( IHostName *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_GetTrustLevel( IHostName *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_get_IPInformation( IHostName *iface, IIPInformation **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_get_RawName( IHostName *iface, HSTRING *value )
{
    struct hostname *impl = impl_from_IHostName( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_INVALIDARG;
    return WindowsDuplicateString( impl->raw_name, value );
}

static HRESULT WINAPI hostname_get_DisplayName( IHostName *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_get_CanonicalName( IHostName *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_get_Type( IHostName *iface, HostNameType *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI hostname_IsEqual( IHostName *iface, IHostName *name, boolean *equal )
{
    FIXME( "iface %p, name %p, equal %p stub!\n", iface, name, equal );
    return E_NOTIMPL;
}

static const struct IHostNameVtbl hostname_vtbl =
{
    hostname_QueryInterface,
    hostname_AddRef,
    hostname_Release,
    /* IInspectable methods */
    hostname_GetIids,
    hostname_GetRuntimeClassName,
    hostname_GetTrustLevel,
    /* IHostName methods */
    hostname_get_IPInformation,
    hostname_get_RawName,
    hostname_get_DisplayName,
    hostname_get_CanonicalName,
    hostname_get_Type,
    hostname_IsEqual,
};

DEFINE_IINSPECTABLE( hostname_factory, IHostNameFactory, struct hostname_statics, IActivationFactory_iface )

static HRESULT WINAPI hostname_factory_CreateHostName( IHostNameFactory *iface, HSTRING name, IHostName **value )
{
    struct hostname *impl;

    TRACE( "iface %p, name %s, value %p\n", iface, debugstr_hstring(name), value );

    if (!value) return E_POINTER;
    if (!name) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IHostName_iface.lpVtbl = &hostname_vtbl;
    impl->ref = 1;
    WindowsDuplicateString( name, &impl->raw_name );

    *value = &impl->IHostName_iface;
    TRACE( "created IHostName %p.\n", *value );
    return S_OK;
}

static const struct IHostNameFactoryVtbl hostname_factory_vtbl =
{
    hostname_factory_QueryInterface,
    hostname_factory_AddRef,
    hostname_factory_Release,
    /* IInspectable methods */
    hostname_factory_GetIids,
    hostname_factory_GetRuntimeClassName,
    hostname_factory_GetTrustLevel,
    /* IHostNameFactory methods */
    hostname_factory_CreateHostName,
};

static struct hostname_statics hostname_statics =
{
    {&factory_vtbl},
    {&hostname_factory_vtbl},
    1,
};

IActivationFactory *hostname_factory = &hostname_statics.IActivationFactory_iface;
