/*
 * Copyright (C) 2025 Paul Gofman for CodeWeavers
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

#include "private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(perception);

struct exporter
{
    IActivationFactory IActivationFactory_iface;
    ISpatialAnchorExporterStatics ISpatialAnchorExporterStatics_iface;
    LONG ref;
};

static inline struct exporter *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct exporter, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct exporter *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_ISpatialAnchorExporterStatics ))
    {
        *out = &impl->ISpatialAnchorExporterStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct exporter *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct exporter *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( exporter_statics, ISpatialAnchorExporterStatics, struct exporter, IActivationFactory_iface )

static HRESULT request_access_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result, BOOL called_async )
{
    result->vt = VT_UI4;
    result->ulVal = SpatialPerceptionAccessStatus_DeniedBySystem;
    return S_OK;
}

static HRESULT WINAPI exporter_statics_GetDefault( ISpatialAnchorExporterStatics *iface, ISpatialAnchorExporter **value )
{
    FIXME( "iface %p, value %p stub.\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI exporter_statics_RequestAccessAsync( ISpatialAnchorExporterStatics *iface, IAsyncOperation_SpatialPerceptionAccessStatus **result )
{
    TRACE( "iface %p, result %p stub.\n", iface, result );
    return async_operation_request_access_create( (IUnknown *)iface, NULL, request_access_async, result );
}

static const struct ISpatialAnchorExporterStaticsVtbl exporter_statics_vtbl =
{
    exporter_statics_QueryInterface,
    exporter_statics_AddRef,
    exporter_statics_Release,
    /* IInspectable methods */
    exporter_statics_GetIids,
    exporter_statics_GetRuntimeClassName,
    exporter_statics_GetTrustLevel,
    /* ISpatialAnchorExporterStatics methods */
    exporter_statics_GetDefault,
    exporter_statics_RequestAccessAsync,
};

static struct exporter exporter_statics =
{
    {&factory_vtbl},
    {&exporter_statics_vtbl},
    1,
};

IActivationFactory *anchor_exporter_factory = &exporter_statics.IActivationFactory_iface;
