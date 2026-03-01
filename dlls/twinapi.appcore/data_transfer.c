/*
 * Copyright 2026 Alistair Leslie-Hughes
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

WINE_DEFAULT_DEBUG_CHANNEL(twinapi);

struct data_transfer_manager_statics
{
    IActivationFactory IActivationFactory_iface;
    IDataTransferManagerStatics IDataTransferManagerStatics_iface;
    IDataTransferManagerInterop IDataTransferManagerInterop_iface;
    LONG ref;
};

static inline struct data_transfer_manager_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct data_transfer_manager_statics, IActivationFactory_iface );
}

static inline struct data_transfer_manager_statics *impl_from_IDataTransferManagerStatics( IDataTransferManagerStatics *iface )
{
    return CONTAINING_RECORD( iface, struct data_transfer_manager_statics, IDataTransferManagerStatics_iface );
}

static inline struct data_transfer_manager_statics *impl_from_IDataTransferManagerInterop( IDataTransferManagerInterop *iface )
{
    return CONTAINING_RECORD( iface, struct data_transfer_manager_statics, IDataTransferManagerInterop_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct data_transfer_manager_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IDataTransferManagerStatics ))
    {
        IInspectable_AddRef( (*out = &impl->IDataTransferManagerStatics_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IDataTransferManagerInterop ))
    {
        IInspectable_AddRef( (*out = &impl->IDataTransferManagerInterop_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct data_transfer_manager_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct data_transfer_manager_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p\n", iface, instance );
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

static HRESULT WINAPI data_transfer_manager_statics_QueryInterface( IDataTransferManagerStatics *iface,
                                                                REFIID iid, void **out )
{
    struct data_transfer_manager_statics *impl = impl_from_IDataTransferManagerStatics( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IDataTransferManagerStatics ))
    {
        IDataTransferManagerStatics_AddRef( (*out = &impl->IDataTransferManagerStatics_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI data_transfer_manager_statics_AddRef( IDataTransferManagerStatics *iface )
{
    struct data_transfer_manager_statics *impl = impl_from_IDataTransferManagerStatics( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI data_transfer_manager_statics_Release( IDataTransferManagerStatics *iface )
{
    struct data_transfer_manager_statics *impl = impl_from_IDataTransferManagerStatics( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI data_transfer_manager_statics_GetIids( IDataTransferManagerStatics *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_transfer_manager_statics_GetRuntimeClassName( IDataTransferManagerStatics *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_transfer_manager_statics_GetTrustLevel( IDataTransferManagerStatics *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_transfer_manager_statics_ShowShareUI( IDataTransferManagerStatics *iface )
{
    FIXME( "iface %p\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_transfer_manager_statics_GetForCurrentView( IDataTransferManagerStatics *iface, IDataTransferManager **result )
{
    FIXME( "iface %p, result %p\n", iface, result );
    return E_NOTIMPL;
}

static IDataTransferManagerStaticsVtbl data_transfer_manager_statics_vtbl =
{
    data_transfer_manager_statics_QueryInterface,
    data_transfer_manager_statics_AddRef,
    data_transfer_manager_statics_Release,
    /* IInspectable methods */
    data_transfer_manager_statics_GetIids,
    data_transfer_manager_statics_GetRuntimeClassName,
    data_transfer_manager_statics_GetTrustLevel,
    /* IDataTransferManagerStatics methods */
    data_transfer_manager_statics_ShowShareUI,
    data_transfer_manager_statics_GetForCurrentView
};

static HRESULT WINAPI data_transfer_manager_interop_QueryInterface( IDataTransferManagerInterop *iface,
                                                                REFIID iid, void **out )
{
    struct data_transfer_manager_statics *impl = impl_from_IDataTransferManagerInterop( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IDataTransferManagerInterop ))
    {
        IDataTransferManagerInterop_AddRef( (*out = &impl->IDataTransferManagerInterop_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI data_transfer_manager_interop_AddRef( IDataTransferManagerInterop *iface )
{
    struct data_transfer_manager_statics *impl = impl_from_IDataTransferManagerInterop( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI data_transfer_manager_interop_Release( IDataTransferManagerInterop *iface )
{
    struct data_transfer_manager_statics *impl = impl_from_IDataTransferManagerInterop( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI data_transfer_manager_interop_GetForWindow( IDataTransferManagerInterop *iface, HWND appWindow, REFIID iid, void **result )
{
    FIXME( "iface %p, appWindow %p, iid %s, result %p\n", iface, appWindow, debugstr_guid( iid ), result );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_transfer_manager_interop_ShowShareUIForWindow( IDataTransferManagerInterop *iface, HWND appWindow )
{
    FIXME( "iface %p, appWindow %p\n", iface, appWindow );
    return E_NOTIMPL;
}

static IDataTransferManagerInteropVtbl data_transfer_manager_interop_vtbl =
{
    data_transfer_manager_interop_QueryInterface,
    data_transfer_manager_interop_AddRef,
    data_transfer_manager_interop_Release,
    /* IDataTransferManagerInterop methods */
    data_transfer_manager_interop_GetForWindow,
    data_transfer_manager_interop_ShowShareUIForWindow
};

static struct data_transfer_manager_statics data_transfer_manager_statics =
{
    {&factory_vtbl},
    {&data_transfer_manager_statics_vtbl},
    {&data_transfer_manager_interop_vtbl},
    0,
};

IActivationFactory *data_transfer_manager_statics_factory = &data_transfer_manager_statics.IActivationFactory_iface;
