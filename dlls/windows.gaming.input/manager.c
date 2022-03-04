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

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

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
    FIXME( "iface %p, factory %p, controller %p, value %p stub!\n", iface, factory, controller, value );
    return E_NOTIMPL;
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

IActivationFactory *manager_factory = &manager_statics.IActivationFactory_iface;

void manager_on_provider_created( IGameControllerProvider *provider )
{
    FIXME( "provider %p stub!\n", provider );
}

void manager_on_provider_removed( IGameControllerProvider *provider )
{
    FIXME( "provider %p stub!\n", provider );
}
