/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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
#include "provider.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

static CRITICAL_SECTION manager_cs;
static CRITICAL_SECTION_DEBUG manager_cs_debug =
{
    0, 0, &manager_cs,
    { &manager_cs_debug.ProcessLocksList, &manager_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": manager_cs") }
};
static CRITICAL_SECTION manager_cs = { &manager_cs_debug, -1, 0, 0, 0, 0 };

static struct list controller_list = LIST_INIT( controller_list );

struct controller
{
    IGameController IGameController_iface;
    IGameControllerBatteryInfo IGameControllerBatteryInfo_iface;
    IInspectable *IInspectable_inner;
    LONG ref;

    struct list entry;
    IGameControllerProvider *provider;
    ICustomGameControllerFactory *factory;
};

static inline struct controller *impl_from_IGameController( IGameController *iface )
{
    return CONTAINING_RECORD( iface, struct controller, IGameController_iface );
}

static HRESULT WINAPI controller_QueryInterface( IGameController *iface, REFIID iid, void **out )
{
    struct controller *impl = impl_from_IGameController( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IGameController ))
    {
        IInspectable_AddRef( (*out = &impl->IGameController_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGameControllerBatteryInfo ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerBatteryInfo_iface) );
        return S_OK;
    }

    return IInspectable_QueryInterface( impl->IInspectable_inner, iid, out );
}

static ULONG WINAPI controller_AddRef( IGameController *iface )
{
    struct controller *impl = impl_from_IGameController( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI controller_Release( IGameController *iface )
{
    struct controller *impl = impl_from_IGameController( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        /* guard against re-entry if inner releases an outer iface */
        InterlockedIncrement( &impl->ref );
        IInspectable_Release( impl->IInspectable_inner );
        ICustomGameControllerFactory_Release( impl->factory );
        IGameControllerProvider_Release( impl->provider );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI controller_GetIids( IGameController *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_GetRuntimeClassName( IGameController *iface, HSTRING *class_name )
{
    struct controller *impl = impl_from_IGameController( iface );
    return IInspectable_GetRuntimeClassName( impl->IInspectable_inner, class_name );
}

static HRESULT WINAPI controller_GetTrustLevel( IGameController *iface, TrustLevel *trust_level )
{
    struct controller *impl = impl_from_IGameController( iface );
    return IInspectable_GetTrustLevel( impl->IInspectable_inner, trust_level );
}

static HRESULT WINAPI controller_add_HeadsetConnected( IGameController *iface, ITypedEventHandler_IGameController_Headset *handler,
                                                       EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_remove_HeadsetConnected( IGameController *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_add_HeadsetDisconnected( IGameController *iface, ITypedEventHandler_IGameController_Headset *handler,
                                                          EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_remove_HeadsetDisconnected( IGameController *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_add_UserChanged( IGameController *iface,
                                                  ITypedEventHandler_IGameController_UserChangedEventArgs *handler,
                                                  EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_remove_UserChanged( IGameController *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_get_Headset( IGameController *iface, IHeadset **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_get_IsWireless( IGameController *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI controller_get_User( IGameController *iface, __x_ABI_CWindows_CSystem_CIUser **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IGameControllerVtbl controller_vtbl =
{
    controller_QueryInterface,
    controller_AddRef,
    controller_Release,
    /* IInspectable methods */
    controller_GetIids,
    controller_GetRuntimeClassName,
    controller_GetTrustLevel,
    /* IGameController methods */
    controller_add_HeadsetConnected,
    controller_remove_HeadsetConnected,
    controller_add_HeadsetDisconnected,
    controller_remove_HeadsetDisconnected,
    controller_add_UserChanged,
    controller_remove_UserChanged,
    controller_get_Headset,
    controller_get_IsWireless,
    controller_get_User,
};

DEFINE_IINSPECTABLE( battery, IGameControllerBatteryInfo, struct controller, IGameController_iface )

static HRESULT WINAPI battery_TryGetBatteryReport( IGameControllerBatteryInfo *iface, IBatteryReport **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IGameControllerBatteryInfoVtbl battery_vtbl =
{
    battery_QueryInterface,
    battery_AddRef,
    battery_Release,
    /* IInspectable methods */
    battery_GetIids,
    battery_GetRuntimeClassName,
    battery_GetTrustLevel,
    /* IGameControllerBatteryInfo methods */
    battery_TryGetBatteryReport,
};

struct manager_statics
{
    IActivationFactory IActivationFactory_iface;
    IGameControllerFactoryManagerStatics IGameControllerFactoryManagerStatics_iface;
    IGameControllerFactoryManagerStatics2 IGameControllerFactoryManagerStatics2_iface;
    LONG ref;
};

static inline struct manager_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct manager_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct manager_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGameControllerFactoryManagerStatics ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerFactoryManagerStatics_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IGameControllerFactoryManagerStatics2 ))
    {
        IInspectable_AddRef( (*out = &impl->IGameControllerFactoryManagerStatics2_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct manager_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct manager_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( statics, IGameControllerFactoryManagerStatics, struct manager_statics, IActivationFactory_iface )

static HRESULT WINAPI
statics_RegisterCustomFactoryForGipInterface( IGameControllerFactoryManagerStatics *iface,
                                              ICustomGameControllerFactory *factory,
                                              GUID interface_id )
{
    FIXME( "iface %p, factory %p, interface_id %s stub!\n", iface, factory, debugstr_guid(&interface_id) );
    return E_NOTIMPL;
}

static HRESULT WINAPI
statics_RegisterCustomFactoryForHardwareId( IGameControllerFactoryManagerStatics *iface,
                                            ICustomGameControllerFactory *factory,
                                            UINT16 vendor_id, UINT16 product_id )
{
    FIXME( "iface %p, factory %p, vendor_id %u, product_id %u stub!\n", iface, factory, vendor_id, product_id );
    return E_NOTIMPL;
}

static HRESULT WINAPI
statics_RegisterCustomFactoryForXusbType( IGameControllerFactoryManagerStatics *iface,
                                          ICustomGameControllerFactory *factory,
                                          XusbDeviceType type, XusbDeviceSubtype subtype )
{
    FIXME( "iface %p, factory %p, type %d, subtype %d stub!\n", iface, factory, type, subtype );
    return E_NOTIMPL;
}

static const struct IGameControllerFactoryManagerStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IGameControllerFactoryManagerStatics methods */
    statics_RegisterCustomFactoryForGipInterface,
    statics_RegisterCustomFactoryForHardwareId,
    statics_RegisterCustomFactoryForXusbType,
};

DEFINE_IINSPECTABLE( statics2, IGameControllerFactoryManagerStatics2, struct manager_statics, IActivationFactory_iface )

static HRESULT WINAPI
statics2_TryGetFactoryControllerFromGameController( IGameControllerFactoryManagerStatics2 *iface,
                                                    ICustomGameControllerFactory *factory,
                                                    IGameController *controller, IGameController **value )
{
    struct controller *entry, *other;
    IGameController *tmp_controller;
    BOOL found = FALSE;

    TRACE( "iface %p, factory %p, controller %p, value %p.\n", iface, factory, controller, value );

    /* Spider Man Remastered passes a IRawGameController instead of IGameController, query the iface again */
    if (FAILED(IGameController_QueryInterface( controller, &IID_IGameController, (void **)&tmp_controller ))) goto done;

    EnterCriticalSection( &manager_cs );

    LIST_FOR_EACH_ENTRY( entry, &controller_list, struct controller, entry )
        if ((found = &entry->IGameController_iface == tmp_controller)) break;

    if (!found) WARN( "Failed to find controller %p\n", controller );
    else
    {
        LIST_FOR_EACH_ENTRY( other, &controller_list, struct controller, entry )
            if ((found = entry->provider == other->provider && other->factory == factory)) break;
        if (!found) WARN( "Failed to find controller %p, factory %p\n", controller, factory );
        else IGameController_AddRef( (*value = &other->IGameController_iface) );
    }

    LeaveCriticalSection( &manager_cs );

    IGameController_Release( tmp_controller );

done:
    if (!found) *value = NULL;
    return S_OK;
}

static const struct IGameControllerFactoryManagerStatics2Vtbl statics2_vtbl =
{
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* IGameControllerFactoryManagerStatics2 methods */
    statics2_TryGetFactoryControllerFromGameController,
};

static struct manager_statics manager_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    {&statics2_vtbl},
    1,
};

IGameControllerFactoryManagerStatics2 *manager_factory = &manager_statics.IGameControllerFactoryManagerStatics2_iface;

static HRESULT controller_create( ICustomGameControllerFactory *factory, IGameControllerProvider *provider,
                                  struct controller **out )
{
    IGameControllerImpl *inner_impl;
    struct controller *impl;
    HRESULT hr;

    if (!(impl = malloc(sizeof(*impl)))) return E_OUTOFMEMORY;
    impl->IGameController_iface.lpVtbl = &controller_vtbl;
    impl->IGameControllerBatteryInfo_iface.lpVtbl = &battery_vtbl;
    impl->ref = 1;

    if (FAILED(hr = ICustomGameControllerFactory_CreateGameController( factory, provider, &impl->IInspectable_inner )))
        WARN( "Failed to create game controller, hr %#lx\n", hr );
    else if (FAILED(hr = IInspectable_QueryInterface( impl->IInspectable_inner, &IID_IGameControllerImpl, (void **)&inner_impl )))
        WARN( "Failed to find IGameControllerImpl iface, hr %#lx\n", hr );
    else
    {
        if (FAILED(hr = IGameControllerImpl_Initialize( inner_impl, &impl->IGameController_iface, provider )))
            WARN( "Failed to initialize game controller, hr %#lx\n", hr );
        IGameControllerImpl_Release( inner_impl );
    }

    if (FAILED(hr))
    {
        if (impl->IInspectable_inner) IInspectable_Release( impl->IInspectable_inner );
        free( impl );
        return hr;
    }

    ICustomGameControllerFactory_AddRef( (impl->factory = factory) );
    IGameControllerProvider_AddRef( (impl->provider = provider) );

    *out = impl;
    return S_OK;
}

void manager_on_provider_created( IGameControllerProvider *provider )
{
    IWineGameControllerProvider *wine_provider;
    struct list *entry, *next, *list;
    struct controller *controller;
    WineGameControllerType type;
    HRESULT hr;

    TRACE( "provider %p\n", provider );

    if (FAILED(IGameControllerProvider_QueryInterface( provider, &IID_IWineGameControllerProvider,
                                                       (void **)&wine_provider )))
    {
        FIXME( "IWineGameControllerProvider isn't implemented by provider %p\n", provider );
        return;
    }
    if (FAILED(hr = IWineGameControllerProvider_get_Type( wine_provider, &type )))
    {
        WARN( "Failed to get controller type, hr %#lx\n", hr );
        type = WineGameControllerType_Joystick;
    }
    IWineGameControllerProvider_Release( wine_provider );

    EnterCriticalSection( &manager_cs );

    if (list_empty( &controller_list )) list = &controller_list;
    else list = list_tail( &controller_list );

    if (SUCCEEDED(controller_create( controller_factory, provider, &controller )))
        list_add_tail( &controller_list, &controller->entry );

    switch (type)
    {
    case WineGameControllerType_Joystick: break;
    case WineGameControllerType_Gamepad:
        if (SUCCEEDED(controller_create( gamepad_factory, provider, &controller )))
            list_add_tail( &controller_list, &controller->entry );
        break;
    case WineGameControllerType_RacingWheel:
        if (SUCCEEDED(controller_create( racing_wheel_factory, provider, &controller )))
            list_add_tail( &controller_list, &controller->entry );
        break;
    }

    LIST_FOR_EACH_SAFE( entry, next, list )
    {
        controller = LIST_ENTRY( entry, struct controller, entry );
        hr = ICustomGameControllerFactory_OnGameControllerAdded( controller->factory,
                                                                 &controller->IGameController_iface );
        if (FAILED(hr)) WARN( "OnGameControllerAdded failed, hr %#lx\n", hr );
        if (next == &controller_list) break;
    }

    LeaveCriticalSection( &manager_cs );
}

void manager_on_provider_removed( IGameControllerProvider *provider )
{
    struct controller *controller, *next;

    TRACE( "provider %p\n", provider );

    EnterCriticalSection( &manager_cs );

    LIST_FOR_EACH_ENTRY( controller, &controller_list, struct controller, entry )
    {
        if (controller->provider != provider) continue;
        ICustomGameControllerFactory_OnGameControllerRemoved( controller->factory,
                                                              &controller->IGameController_iface );
    }

    LIST_FOR_EACH_ENTRY_SAFE( controller, next, &controller_list, struct controller, entry )
    {
        if (controller->provider != provider) continue;
        list_remove( &controller->entry );
        IGameController_Release( &controller->IGameController_iface );
    }

    LeaveCriticalSection( &manager_cs );
}
