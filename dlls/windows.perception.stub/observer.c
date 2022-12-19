/* WinRT Windows.Perception.Stub Observer Implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(perception);

struct observer
{
    IActivationFactory IActivationFactory_iface;
    ISpatialSurfaceObserverStatics ISpatialSurfaceObserverStatics_iface;
    ISpatialSurfaceObserverStatics2 ISpatialSurfaceObserverStatics2_iface;
    LONG ref;
};

static inline struct observer *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct observer, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct observer *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_ISpatialSurfaceObserverStatics ))
    {
        *out = &impl->ISpatialSurfaceObserverStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ISpatialSurfaceObserverStatics2 ))
    {
        *out = &impl->ISpatialSurfaceObserverStatics2_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct observer *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct observer *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( observer_statics, ISpatialSurfaceObserverStatics, struct observer, IActivationFactory_iface )

static HRESULT WINAPI observer_statics_RequestAccessAsync( ISpatialSurfaceObserverStatics *iface, IAsyncOperation_SpatialPerceptionAccessStatus **result )
{
    FIXME( "iface %p, result %p stub!\n", iface, result );
    return E_NOTIMPL;
}

static const struct ISpatialSurfaceObserverStaticsVtbl observer_statics_vtbl =
{
    observer_statics_QueryInterface,
    observer_statics_AddRef,
    observer_statics_Release,
    /* IInspectable methods */
    observer_statics_GetIids,
    observer_statics_GetRuntimeClassName,
    observer_statics_GetTrustLevel,
    /* ISpatialSurfaceObserverStatics methods */
    observer_statics_RequestAccessAsync,
};

DEFINE_IINSPECTABLE( observer_statics2, ISpatialSurfaceObserverStatics2, struct observer, IActivationFactory_iface )

static HRESULT WINAPI observer_statics2_IsSupported( ISpatialSurfaceObserverStatics2 *iface, boolean *value )
{
    TRACE( "iface %p, value %p.\n", iface, value );

    *value = FALSE;
    return S_OK;
}

static const struct ISpatialSurfaceObserverStatics2Vtbl observer_statics2_vtbl =
{
    observer_statics2_QueryInterface,
    observer_statics2_AddRef,
    observer_statics2_Release,
    /* IInspectable methods */
    observer_statics2_GetIids,
    observer_statics2_GetRuntimeClassName,
    observer_statics2_GetTrustLevel,
    /* ISpatialSurfaceObserverStatics2 methods */
    observer_statics2_IsSupported,
};

static struct observer observer_statics =
{
    {&factory_vtbl},
    {&observer_statics_vtbl},
    {&observer_statics2_vtbl},
    1,
};

IActivationFactory *observer_factory = &observer_statics.IActivationFactory_iface;
