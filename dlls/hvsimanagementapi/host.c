/* WinRT Windows.Security.Isolation IsolatedWindowsEnvironmentHost Implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(hvsi);

struct isolated_host_statics
{
    IActivationFactory IActivationFactory_iface;
    IIsolatedWindowsEnvironmentHostStatics IIsolatedWindowsEnvironmentHostStatics_iface;
    LONG ref;
};

static inline struct isolated_host_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct isolated_host_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct isolated_host_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IIsolatedWindowsEnvironmentHostStatics ))
    {
        *out = &impl->IIsolatedWindowsEnvironmentHostStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct isolated_host_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct isolated_host_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( isolated_host_statics, IIsolatedWindowsEnvironmentHostStatics, struct isolated_host_statics, IActivationFactory_iface )

static HRESULT WINAPI isolated_host_statics_get_IsReady( IIsolatedWindowsEnvironmentHostStatics *iface, boolean *value )
{
    TRACE( "iface %p, value %p\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static HRESULT WINAPI isolated_host_statics_get_HostErrors( IIsolatedWindowsEnvironmentHostStatics *iface,
                                                            IVectorView_IsolatedWindowsEnvironmentHostError **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IIsolatedWindowsEnvironmentHostStaticsVtbl isolated_host_statics_vtbl =
{
    isolated_host_statics_QueryInterface,
    isolated_host_statics_AddRef,
    isolated_host_statics_Release,
    /* IInspectable methods */
    isolated_host_statics_GetIids,
    isolated_host_statics_GetRuntimeClassName,
    isolated_host_statics_GetTrustLevel,
    /* IIsolatedWindowsEnvironmentHostStatics methods */
    isolated_host_statics_get_IsReady,
    isolated_host_statics_get_HostErrors,
};

static struct isolated_host_statics isolated_host_statics =
{
    {&factory_vtbl},
    {&isolated_host_statics_vtbl},
    1,
};

IActivationFactory *isolated_host_factory = &isolated_host_statics.IActivationFactory_iface;
