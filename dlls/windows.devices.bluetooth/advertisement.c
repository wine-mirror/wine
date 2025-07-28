/* windows.Devices.Bluetooth.Advertisement Implementation
 *
 * Copyright 2025 Vibhav Pant
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

WINE_DEFAULT_DEBUG_CHANNEL( bluetooth );

struct adv_watcher
{
    IBluetoothLEAdvertisementWatcher IBluetoothLEAdvertisementWatcher_iface;
    LONG ref;
};

static inline struct adv_watcher *impl_from_IBluetoothLEAdvertisementWatcher( IBluetoothLEAdvertisementWatcher *iface )
{
    return CONTAINING_RECORD( iface, struct adv_watcher, IBluetoothLEAdvertisementWatcher_iface );
}

static HRESULT WINAPI adv_watcher_QueryInterface( IBluetoothLEAdvertisementWatcher *iface, REFIID iid, void **out )
{
    struct adv_watcher *impl = impl_from_IBluetoothLEAdvertisementWatcher( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IBluetoothLEAdvertisementWatcher ))
    {
        IBluetoothLEAdvertisementWatcher_AddRef(( *out = &impl->IBluetoothLEAdvertisementWatcher_iface ));
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI adv_watcher_AddRef( IBluetoothLEAdvertisementWatcher *iface )
{
    struct adv_watcher *impl = impl_from_IBluetoothLEAdvertisementWatcher( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI adv_watcher_Release( IBluetoothLEAdvertisementWatcher *iface )
{
    struct adv_watcher *impl = impl_from_IBluetoothLEAdvertisementWatcher( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "(%p)\n", iface );
    return ref;
}

static HRESULT WINAPI adv_watcher_GetIids( IBluetoothLEAdvertisementWatcher *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_GetRuntimeClassName( IBluetoothLEAdvertisementWatcher *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p): stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_GetTrustLevel( IBluetoothLEAdvertisementWatcher *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_MinSamplingInternal( IBluetoothLEAdvertisementWatcher *iface, TimeSpan *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_MaxSamplingInternal( IBluetoothLEAdvertisementWatcher *iface, TimeSpan *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_MinOutOfRangeTimeout( IBluetoothLEAdvertisementWatcher *iface, TimeSpan *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_MaxOutOfRangeTimeout( IBluetoothLEAdvertisementWatcher *iface, TimeSpan *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_Status( IBluetoothLEAdvertisementWatcher *iface, BluetoothLEAdvertisementWatcherStatus *status )
{
    FIXME( "(%p, %p): stub!\n", iface, status );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_ScanningMode( IBluetoothLEAdvertisementWatcher *iface, BluetoothLEScanningMode *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_put_ScanningMode( IBluetoothLEAdvertisementWatcher *iface, BluetoothLEScanningMode value )
{
    FIXME( "(%p, %d): stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_SignalStrengthFilter( IBluetoothLEAdvertisementWatcher *iface, IBluetoothSignalStrengthFilter **filter )
{
    FIXME( "(%p, %p): stub!\n", iface, filter );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_put_SignalStrengthFilter( IBluetoothLEAdvertisementWatcher *iface, IBluetoothSignalStrengthFilter *filter )
{
    FIXME( "(%p, %p): stub!\n", iface, filter );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_get_AdvertisementFilter( IBluetoothLEAdvertisementWatcher *iface, IBluetoothLEAdvertisementFilter **filter )
{
    FIXME( "(%p, %p): stub!\n", iface, filter );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_put_AdvertisementFilter( IBluetoothLEAdvertisementWatcher *iface, IBluetoothLEAdvertisementFilter *filter )
{
    FIXME( "(%p, %p): stub!\n", iface, filter );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_Start( IBluetoothLEAdvertisementWatcher *iface )
{
    FIXME( "(%p): stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_Stop( IBluetoothLEAdvertisementWatcher *iface )
{
    FIXME( "(%p): stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_add_Received( IBluetoothLEAdvertisementWatcher *iface,
                                                ITypedEventHandler_BluetoothLEAdvertisementWatcher_BluetoothLEAdvertisementReceivedEventArgs *handler,
                                                EventRegistrationToken *token )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_remove_Received( IBluetoothLEAdvertisementWatcher *iface, EventRegistrationToken token )
{
    FIXME( "(%p, %I64x): stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_add_Stopped( IBluetoothLEAdvertisementWatcher *iface,
                                               ITypedEventHandler_BluetoothLEAdvertisementWatcher_BluetoothLEAdvertisementWatcherStoppedEventArgs *handler,
                                               EventRegistrationToken *token )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, handler, token );
    return E_NOTIMPL;
}


static HRESULT WINAPI adv_watcher_remove_Stopped( IBluetoothLEAdvertisementWatcher *iface, EventRegistrationToken token )
{
    FIXME( "(%p, %I64x): stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static const IBluetoothLEAdvertisementWatcherVtbl adv_watcher_vtbl =
{
    /* IUnknown */
    adv_watcher_QueryInterface,
    adv_watcher_AddRef,
    adv_watcher_Release,
    /* IInspectable */
    adv_watcher_GetIids,
    adv_watcher_GetRuntimeClassName,
    adv_watcher_GetTrustLevel,
    /* IBluetoothLEAdvertisementWatcher */
    adv_watcher_get_MinSamplingInternal,
    adv_watcher_get_MaxSamplingInternal,
    adv_watcher_get_MinOutOfRangeTimeout,
    adv_watcher_get_MaxOutOfRangeTimeout,
    adv_watcher_get_Status,
    adv_watcher_get_ScanningMode,
    adv_watcher_put_ScanningMode,
    adv_watcher_get_SignalStrengthFilter,
    adv_watcher_put_SignalStrengthFilter,
    adv_watcher_get_AdvertisementFilter,
    adv_watcher_put_AdvertisementFilter,
    adv_watcher_Start,
    adv_watcher_Stop,
    adv_watcher_add_Received,
    adv_watcher_remove_Received,
    adv_watcher_add_Stopped,
    adv_watcher_remove_Stopped
};

static HRESULT adv_watcher_create( IBluetoothLEAdvertisementWatcher **watcher )
{
    struct adv_watcher *impl;

    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;
    impl->IBluetoothLEAdvertisementWatcher_iface.lpVtbl = &adv_watcher_vtbl;
    impl->ref = 1;
    *watcher = &impl->IBluetoothLEAdvertisementWatcher_iface;
    return S_OK;
}

struct adv_watcher_factory
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct adv_watcher_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct adv_watcher_factory, IActivationFactory_iface );
}

static HRESULT WINAPI adv_watcher_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct adv_watcher_factory *impl = impl_from_IActivationFactory( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef(( *out = &impl->IActivationFactory_iface ));
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI adv_watcher_factory_AddRef( IActivationFactory *iface )
{
    struct adv_watcher_factory *impl = impl_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI adv_watcher_factory_Release( IActivationFactory *iface )
{
    struct adv_watcher_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "(%p)\n", iface );
    return ref;
}

static HRESULT WINAPI adv_watcher_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p): stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI adv_watcher_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    TRACE( "(%p, %p)\n", iface, instance );
    return adv_watcher_create( (IBluetoothLEAdvertisementWatcher **)instance );
}

static const struct IActivationFactoryVtbl adv_watcher_factory_vtbl =
{
    adv_watcher_factory_QueryInterface,
    adv_watcher_factory_AddRef,
    adv_watcher_factory_Release,
    /* IInspectable */
    adv_watcher_factory_GetIids,
    adv_watcher_factory_GetRuntimeClassName,
    adv_watcher_factory_GetTrustLevel,
    /* IActivationFactory */
    adv_watcher_factory_ActivateInstance
};

static struct adv_watcher_factory adv_watcher_factory =
{
    {&adv_watcher_factory_vtbl},
    1
};

IActivationFactory *advertisement_watcher_factory = &adv_watcher_factory.IActivationFactory_iface;
