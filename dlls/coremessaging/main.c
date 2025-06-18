/* WinRT CoreMessaging Implementation
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

#include "initguid.h"
#include "private.h"
#include "dispatcherqueue.h"

WINE_DEFAULT_DEBUG_CHANNEL(messaging);

struct dispatcher_queue_controller_statics
{
    IActivationFactory IActivationFactory_iface;
    IDispatcherQueueControllerStatics IDispatcherQueueControllerStatics_iface;
    LONG ref;
};

static inline struct dispatcher_queue_controller_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct dispatcher_queue_controller_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct dispatcher_queue_controller_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IDispatcherQueueControllerStatics ))
    {
        *out = &impl->IDispatcherQueueControllerStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct dispatcher_queue_controller_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing ref to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct dispatcher_queue_controller_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing ref to %lu.\n", iface, ref );
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

struct dispatcher_queue_controller
{
    IDispatcherQueueController IDispatcherQueueController_iface;
    LONG ref;
};

static inline struct dispatcher_queue_controller *impl_from_IDispatcherQueueController( IDispatcherQueueController *iface )
{
    return CONTAINING_RECORD( iface, struct dispatcher_queue_controller, IDispatcherQueueController_iface );
}

static HRESULT WINAPI dispatcher_queue_controller_QueryInterface( IDispatcherQueueController *iface, REFIID iid, void **out )
{
    struct dispatcher_queue_controller *impl = impl_from_IDispatcherQueueController( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IDispatcherQueueController ))
    {
        *out = &impl->IDispatcherQueueController_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dispatcher_queue_controller_AddRef( IDispatcherQueueController *iface )
{
    struct dispatcher_queue_controller *impl = impl_from_IDispatcherQueueController( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing ref to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI dispatcher_queue_controller_Release( IDispatcherQueueController *iface )
{
    struct dispatcher_queue_controller *impl = impl_from_IDispatcherQueueController( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing ref to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI dispatcher_queue_controller_GetIids( IDispatcherQueueController *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI dispatcher_queue_controller_GetRuntimeClassName( IDispatcherQueueController *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI dispatcher_queue_controller_GetTrustLevel( IDispatcherQueueController *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI dispatcher_queue_controller_get_DispatcherQueue( IDispatcherQueueController *iface, IDispatcherQueue **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT shutdown_queue_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result )
{
    return S_OK;
}

static HRESULT WINAPI dispatcher_queue_controller_ShutdownQueueAsync( IDispatcherQueueController *iface, IAsyncAction **operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );

    if (!operation) return E_POINTER;
    *operation = NULL;

    return async_action_create( NULL, shutdown_queue_async, operation );
}

static const struct IDispatcherQueueControllerVtbl dispatcher_queue_controller_vtbl =
{
    dispatcher_queue_controller_QueryInterface,
    dispatcher_queue_controller_AddRef,
    dispatcher_queue_controller_Release,
    /* IInspectable methods */
    dispatcher_queue_controller_GetIids,
    dispatcher_queue_controller_GetRuntimeClassName,
    dispatcher_queue_controller_GetTrustLevel,
    /* IDispatcherQueueController methods */
    dispatcher_queue_controller_get_DispatcherQueue,
    dispatcher_queue_controller_ShutdownQueueAsync,
};

DEFINE_IINSPECTABLE( dispatcher_queue_controller_statics, IDispatcherQueueControllerStatics, struct dispatcher_queue_controller_statics, IActivationFactory_iface )

static HRESULT WINAPI dispatcher_queue_controller_statics_CreateOnDedicatedThread( IDispatcherQueueControllerStatics *iface, IDispatcherQueueController **result )
{
    FIXME( "iface %p, result %p stub!\n", iface, result );
    return E_NOTIMPL;
}

static const struct IDispatcherQueueControllerStaticsVtbl dispatcher_queue_controller_statics_vtbl =
{
    dispatcher_queue_controller_statics_QueryInterface,
    dispatcher_queue_controller_statics_AddRef,
    dispatcher_queue_controller_statics_Release,
    /* IInspectable methods */
    dispatcher_queue_controller_statics_GetIids,
    dispatcher_queue_controller_statics_GetRuntimeClassName,
    dispatcher_queue_controller_statics_GetTrustLevel,
    /* IDispatcherQueueControllerStatics methods */
    dispatcher_queue_controller_statics_CreateOnDedicatedThread,
};

static struct dispatcher_queue_controller_statics dispatcher_queue_controller_statics =
{
    {&factory_vtbl},
    {&dispatcher_queue_controller_statics_vtbl},
    1,
};

static IActivationFactory *dispatcher_queue_controller_factory = &dispatcher_queue_controller_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *name = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "classid %s, factory %p.\n", debugstr_hstring( classid ), factory );

    *factory = NULL;

    if (!wcscmp( name, RuntimeClass_Windows_System_DispatcherQueueController ))
        IActivationFactory_QueryInterface( dispatcher_queue_controller_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI CreateDispatcherQueueController( DispatcherQueueOptions options, PDISPATCHERQUEUECONTROLLER *queue_controller )
{
    struct dispatcher_queue_controller *impl;

    FIXME( "options.dwSize = %lu, options.threadType = %d, options.apartmentType = %d, queue_controller %p semi-stub!\n",
            options.dwSize, options.threadType, options.apartmentType, queue_controller );

    if (!queue_controller) return E_POINTER;
    if (options.dwSize != sizeof( DispatcherQueueOptions )) return E_INVALIDARG;
    if (options.threadType != DQTYPE_THREAD_DEDICATED && options.threadType != DQTYPE_THREAD_CURRENT) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    impl->IDispatcherQueueController_iface.lpVtbl = &dispatcher_queue_controller_vtbl;
    impl->ref = 1;

    *queue_controller = &impl->IDispatcherQueueController_iface;
    TRACE( "created IDispatcherQueueController %p.\n", *queue_controller );
    return S_OK;
}
