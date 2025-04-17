/* WinRT Windows.Devices.Enumeration implementation
 *
 * Copyright 2021 Gijs Vermeulen
 * Copyright 2022 Julian Klemann for CodeWeavers
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

#include "initguid.h"
#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enumeration);

struct device_watcher
{
    IDeviceWatcher IDeviceWatcher_iface;
    LONG ref;

    struct list stopped_handlers;
    HSTRING filter;
};

static inline struct device_watcher *impl_from_IDeviceWatcher( IDeviceWatcher *iface )
{
    return CONTAINING_RECORD( iface, struct device_watcher, IDeviceWatcher_iface );
}

static HRESULT WINAPI device_watcher_QueryInterface( IDeviceWatcher *iface, REFIID iid, void **out )
{
    struct device_watcher *impl = impl_from_IDeviceWatcher( iface );

    TRACE( "iface %p, iid %s, out %p stub!\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IDeviceWatcher ))
    {
        IInspectable_AddRef( (*out = &impl->IDeviceWatcher_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI device_watcher_AddRef( IDeviceWatcher *iface )
{
    struct device_watcher *impl = impl_from_IDeviceWatcher( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI device_watcher_Release( IDeviceWatcher *iface )
{
    struct device_watcher *impl = impl_from_IDeviceWatcher( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        typed_event_handlers_clear( &impl->stopped_handlers );
        WindowsDeleteString( impl->filter );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI device_watcher_GetIids( IDeviceWatcher *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_GetRuntimeClassName( IDeviceWatcher *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_GetTrustLevel( IDeviceWatcher *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_add_Added( IDeviceWatcher *iface, ITypedEventHandler_DeviceWatcher_DeviceInformation *handler,
                                                EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return S_OK;
}

static HRESULT WINAPI device_watcher_remove_Added( IDeviceWatcher *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_add_Updated( IDeviceWatcher *iface, ITypedEventHandler_DeviceWatcher_DeviceInformationUpdate *handler,
                                                  EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return S_OK;
}

static HRESULT WINAPI device_watcher_remove_Updated( IDeviceWatcher *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_add_Removed( IDeviceWatcher *iface, ITypedEventHandler_DeviceWatcher_DeviceInformationUpdate *handler,
                                                  EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_remove_Removed( IDeviceWatcher *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_add_EnumerationCompleted( IDeviceWatcher *iface, ITypedEventHandler_DeviceWatcher_IInspectable *handler,
                                                               EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_remove_EnumerationCompleted( IDeviceWatcher *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_add_Stopped( IDeviceWatcher *iface, ITypedEventHandler_DeviceWatcher_IInspectable *handler, EventRegistrationToken *token )
{
    struct device_watcher *impl = impl_from_IDeviceWatcher( iface );

    TRACE( "iface %p, handler %p, token %p.\n", iface, handler, token );
    return typed_event_handlers_append( &impl->stopped_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token );
}

static HRESULT WINAPI device_watcher_remove_Stopped( IDeviceWatcher *iface, EventRegistrationToken token )
{
    struct device_watcher *impl = impl_from_IDeviceWatcher( iface );

    TRACE( "iface %p, token %#I64x.\n", iface, token.value );
    return typed_event_handlers_remove( &impl->stopped_handlers, &token );
}

static HRESULT WINAPI device_watcher_Status( IDeviceWatcher *iface, DeviceWatcherStatus *status )
{
    FIXME( "iface %p, status %p stub!\n", iface, status );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_watcher_Start( IDeviceWatcher *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return S_OK;
}

static HRESULT WINAPI device_watcher_Stop( IDeviceWatcher *iface )
{
    struct device_watcher *impl = impl_from_IDeviceWatcher( iface );
    HRESULT hr;

    FIXME( "iface %p semi-stub!\n", iface );

    IDeviceWatcher_AddRef( &impl->IDeviceWatcher_iface );
    hr = typed_event_handlers_notify( &impl->stopped_handlers, (IInspectable *)iface, NULL );
    IDeviceWatcher_Release( &impl->IDeviceWatcher_iface );
    return hr;
}

static const struct IDeviceWatcherVtbl device_watcher_vtbl =
{
    device_watcher_QueryInterface,
    device_watcher_AddRef,
    device_watcher_Release,
    /* IInspectable methods */
    device_watcher_GetIids,
    device_watcher_GetRuntimeClassName,
    device_watcher_GetTrustLevel,
    /* IDeviceWatcher methods */
    device_watcher_add_Added,
    device_watcher_remove_Added,
    device_watcher_add_Updated,
    device_watcher_remove_Updated,
    device_watcher_add_Removed,
    device_watcher_remove_Removed,
    device_watcher_add_EnumerationCompleted,
    device_watcher_remove_EnumerationCompleted,
    device_watcher_add_Stopped,
    device_watcher_remove_Stopped,
    device_watcher_Status,
    device_watcher_Start,
    device_watcher_Stop,
};

struct device_information_statics
{
    IActivationFactory IActivationFactory_iface;
    IDeviceInformationStatics IDeviceInformationStatics_iface;
    IDeviceInformationStatics2 IDeviceInformationStatics2_iface;
    LONG ref;
};

static inline struct device_information_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct device_information_statics, IActivationFactory_iface );
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct device_information_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p stub!\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IDeviceInformationStatics ))
    {
        IInspectable_AddRef( (*out = &impl->IDeviceInformationStatics_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IDeviceInformationStatics2 ))
    {
        IInspectable_AddRef( (*out = &impl->IDeviceInformationStatics2_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct device_information_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct device_information_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

DEFINE_IINSPECTABLE( device_statics, IDeviceInformationStatics, struct device_information_statics, IActivationFactory_iface );

static HRESULT WINAPI device_statics_CreateFromIdAsync( IDeviceInformationStatics *iface, HSTRING id,
                                                        IAsyncOperation_DeviceInformation **op )
{
    FIXME( "iface %p, id %s, op %p stub!\n", iface, debugstr_hstring(id), op );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_CreateFromIdAsyncAdditionalProperties( IDeviceInformationStatics *iface, HSTRING id,
                                                                            IIterable_HSTRING *additional_properties,
                                                                            IAsyncOperation_DeviceInformation **op )
{
    FIXME( "iface %p, id %s, additional_properties %p, op %p stub!\n", iface, debugstr_hstring(id), additional_properties, op );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_FindAllAsync( IDeviceInformationStatics *iface,
                                                   IAsyncOperation_DeviceInformationCollection **op )
{
    FIXME( "iface %p, op %p stub!\n", iface, op );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_FindAllAsyncDeviceClass( IDeviceInformationStatics *iface, DeviceClass class,
                                                              IAsyncOperation_DeviceInformationCollection **op )
{
    FIXME( "iface %p, class %d, op %p stub!\n", iface, class, op );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_FindAllAsyncAqsFilter( IDeviceInformationStatics *iface, HSTRING filter,
                                                            IAsyncOperation_DeviceInformationCollection **op )
{
    FIXME( "iface %p, aqs %p, op %p stub!\n", iface, debugstr_hstring(filter), op );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_FindAllAsyncAqsFilterAndAdditionalProperties( IDeviceInformationStatics *iface, HSTRING filter,
                                                                                   IIterable_HSTRING *additional_properties,
                                                                                   IAsyncOperation_DeviceInformationCollection **op )
{
    FIXME( "iface %p, aqs %p, additional_properties %p, op %p stub!\n", iface, debugstr_hstring(filter), additional_properties, op );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_CreateWatcher( IDeviceInformationStatics *iface, IDeviceWatcher **watcher )
{
    FIXME( "iface %p, watcher %p stub!\n", iface, watcher );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_CreateWatcherDeviceClass( IDeviceInformationStatics *iface, DeviceClass class, IDeviceWatcher **watcher )
{
    FIXME( "iface %p, class %d, watcher %p stub!\n", iface, class, watcher );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics_CreateWatcherAqsFilter( IDeviceInformationStatics *iface, HSTRING filter, IDeviceWatcher **watcher )
{
    struct device_watcher *this;
    HRESULT hr;

    TRACE( "iface %p, filter %s, watcher %p\n", iface, debugstr_hstring(filter), watcher );

    if (!(this = calloc( 1, sizeof(*this) ))) return E_OUTOFMEMORY;

    this->IDeviceWatcher_iface.lpVtbl = &device_watcher_vtbl;
    this->ref = 1;
    if (FAILED(hr = WindowsDuplicateString( filter, &this->filter )))
    {
        free( this );
        return hr;
    }

    list_init( &this->stopped_handlers );

    *watcher = &this->IDeviceWatcher_iface;
    return S_OK;
}

static HRESULT WINAPI device_statics_CreateWatcherAqsFilterAndAdditionalProperties( IDeviceInformationStatics *iface, HSTRING filter,
                                                                                    IIterable_HSTRING *additional_properties, IDeviceWatcher **watcher )
{
    FIXME( "iface %p, aqs %p, additional_properties %p, watcher %p stub!\n", iface, debugstr_hstring(filter), additional_properties, watcher );
    return E_NOTIMPL;
}

static const struct IDeviceInformationStaticsVtbl device_statics_vtbl =
{
    device_statics_QueryInterface,
    device_statics_AddRef,
    device_statics_Release,
    /* IInspectable methods */
    device_statics_GetIids,
    device_statics_GetRuntimeClassName,
    device_statics_GetTrustLevel,
    /* IDeviceInformationStatics methods */
    device_statics_CreateFromIdAsync,
    device_statics_CreateFromIdAsyncAdditionalProperties,
    device_statics_FindAllAsync,
    device_statics_FindAllAsyncDeviceClass,
    device_statics_FindAllAsyncAqsFilter,
    device_statics_FindAllAsyncAqsFilterAndAdditionalProperties,
    device_statics_CreateWatcher,
    device_statics_CreateWatcherDeviceClass,
    device_statics_CreateWatcherAqsFilter,
    device_statics_CreateWatcherAqsFilterAndAdditionalProperties,
};

DEFINE_IINSPECTABLE( device_statics2, IDeviceInformationStatics2, struct device_information_statics, IActivationFactory_iface );

static HRESULT WINAPI device_statics2_GetAqsFilterFromDeviceClass( IDeviceInformationStatics2 *iface, DeviceClass device_class,
                                                                   HSTRING *filter )
{
    FIXME( "iface %p, device_class %u, filter %p stub!\n", iface, device_class, filter );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics2_CreateFromIdAsync( IDeviceInformationStatics2 *iface, HSTRING device_id,
                                                         IIterable_HSTRING *additional_properties, DeviceInformationKind kind,
                                                         IAsyncOperation_DeviceInformation **async_operation )
{
    FIXME( "iface %p, device_id %s, additional_properties %p, kind %u, async_operation %p stub!\n",
            iface, debugstr_hstring( device_id ), additional_properties, kind, async_operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics2_FindAllAsync( IDeviceInformationStatics2 *iface, HSTRING filter,
                                                    IIterable_HSTRING *additional_properties, DeviceInformationKind kind,
                                                    IAsyncOperation_DeviceInformationCollection **async_operation )
{
    FIXME( "iface %p, filter %s, additional_properties %p, kind %u, async_operation %p stub!\n",
            iface, debugstr_hstring( filter ), additional_properties, kind, async_operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_statics2_CreateWatcher( IDeviceInformationStatics2 *iface, HSTRING filter,
                                                     IIterable_HSTRING *additional_properties, DeviceInformationKind kind,
                                                     IDeviceWatcher **watcher )
{
    struct device_watcher *this;
    HRESULT hr;

    FIXME( "iface %p, filter %s, additional_properties %p, kind %u, watcher %p semi-stub!\n",
            iface, debugstr_hstring( filter ), additional_properties, kind, watcher );

    if (!(this = calloc( 1, sizeof(*this) )))
        return E_OUTOFMEMORY;

    this->IDeviceWatcher_iface.lpVtbl = &device_watcher_vtbl;
    this->ref = 1;
    if (FAILED(hr = WindowsDuplicateString( filter, &this->filter )))
    {
        free( this );
        return hr;
    }

    list_init( &this->stopped_handlers );

    *watcher = &this->IDeviceWatcher_iface;
    return S_OK;
}

static const struct IDeviceInformationStatics2Vtbl device_statics2_vtbl =
{
    device_statics2_QueryInterface,
    device_statics2_AddRef,
    device_statics2_Release,
    /* IInspectable methods */
    device_statics2_GetIids,
    device_statics2_GetRuntimeClassName,
    device_statics2_GetTrustLevel,
    /* IDeviceInformationStatics2 methods */
    device_statics2_GetAqsFilterFromDeviceClass,
    device_statics2_CreateFromIdAsync,
    device_statics2_FindAllAsync,
    device_statics2_CreateWatcher
};

static struct device_information_statics device_information_statics =
{
    {&activation_factory_vtbl},
    {&device_statics_vtbl},
    {&device_statics2_vtbl},
    1
};

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid( clsid ), debugstr_guid( riid ), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "classid %s, factory %p.\n", debugstr_hstring( classid ), factory );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Devices_Enumeration_DeviceInformation ))
        IActivationFactory_QueryInterface( &device_information_statics.IActivationFactory_iface,
                &IID_IActivationFactory, (void **)factory );
    else if (!wcscmp( buffer, RuntimeClass_Windows_Devices_Enumeration_DeviceAccessInformation ))
        IActivationFactory_QueryInterface( device_access_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
