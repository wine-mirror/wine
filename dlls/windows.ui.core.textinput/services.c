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

WINE_DEFAULT_DEBUG_CHANNEL(coreinputview);

extern const struct ICoreTextEditContextVtbl core_text_edit_context_vtbl;

struct core_text_services_manager_statics
{
    IActivationFactory IActivationFactory_iface;
    ICoreTextServicesManagerStatics ICoreTextServicesManagerStatics_iface;
    LONG ref;
};

struct core_text_services_manager
{
    ICoreTextServicesManager ICoreTextServicesManager_iface;
    LONG ref;
};

static inline struct core_text_services_manager_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct core_text_services_manager_statics, IActivationFactory_iface );
}

static inline struct core_text_services_manager *impl_from_ICoreTextServicesManager( ICoreTextServicesManager *iface )
{
    return CONTAINING_RECORD( iface, struct core_text_services_manager, ICoreTextServicesManager_iface );
}


static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct core_text_services_manager_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "xiface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICoreTextServicesManagerStatics ))
    {
        *out = &impl->ICoreTextServicesManagerStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct core_text_services_manager_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct core_text_services_manager_statics *impl = impl_from_IActivationFactory( iface );
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
    return S_OK;
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



static HRESULT WINAPI core_text_services_manager_QueryInterface( ICoreTextServicesManager *iface, REFIID iid, void **out )
{
    struct core_text_services_manager *impl = impl_from_ICoreTextServicesManager( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ICoreTextServicesManager ))
    {
        *out = &impl->ICoreTextServicesManager_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI core_text_services_manager_AddRef( ICoreTextServicesManager *iface )
{
    struct core_text_services_manager *impl = impl_from_ICoreTextServicesManager( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI core_text_services_manager_Release( ICoreTextServicesManager *iface )
{
    struct core_text_services_manager *impl = impl_from_ICoreTextServicesManager( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    if (!impl->ref)
    {
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI core_text_services_manager_GetIids( ICoreTextServicesManager *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_services_manager_GetRuntimeClassName( ICoreTextServicesManager *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_services_manager_GetTrustLevel( ICoreTextServicesManager *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p not stub!\n", iface, trust_level );
    *trust_level = FullTrust;
    return S_OK;
}
static HRESULT WINAPI core_text_services_manager_get_InputLanguage( ICoreTextServicesManager *iface, ILanguage **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_services_manager_add_InputLanguageChanged( ICoreTextServicesManager *iface, ITypedEventHandler_CoreTextServicesManager_IInspectable *handler, EventRegistrationToken* cookie )
{
    FIXME( "iface %p, handler %p, cookie %p stub!\n", iface, handler, cookie );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_services_manager_remove_InputLanguageChanged( ICoreTextServicesManager *iface, EventRegistrationToken cookie )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_text_services_manager_CreateEditContext( ICoreTextServicesManager *iface, ICoreTextEditContext **value )
{
    FIXME( "iface %p out %p created\n", iface, value );
    return E_NOTIMPL;
}

static const struct ICoreTextServicesManagerVtbl core_text_services_manager_vtbl =
{
    core_text_services_manager_QueryInterface,
    core_text_services_manager_AddRef,
    core_text_services_manager_Release,
    /* IInspectable methods */
    core_text_services_manager_GetIids,
    core_text_services_manager_GetRuntimeClassName,
    core_text_services_manager_GetTrustLevel,
    /* ICoreTextServicesManager methods */
    core_text_services_manager_get_InputLanguage,
    core_text_services_manager_add_InputLanguageChanged,
    core_text_services_manager_remove_InputLanguageChanged,
    core_text_services_manager_CreateEditContext
};

DEFINE_IINSPECTABLE( core_text_services_manager_statics, ICoreTextServicesManagerStatics, struct core_text_services_manager_statics, IActivationFactory_iface )

static HRESULT WINAPI core_text_services_manager_statics_GetForCurrentView( ICoreTextServicesManagerStatics *iface, ICoreTextServicesManager **out )
{
    struct core_text_services_manager *manager;

    TRACE( "iface %p out %p\n", iface, out );

    if (!(manager = calloc( 1, sizeof(*manager) ))) return E_OUTOFMEMORY;

    manager->ICoreTextServicesManager_iface.lpVtbl = &core_text_services_manager_vtbl;
    manager->ref = 1;

    *out = &manager->ICoreTextServicesManager_iface;

    return S_OK;
}

static const struct ICoreTextServicesManagerStaticsVtbl core_text_services_manager_statics_vtbl =
{
    core_text_services_manager_statics_QueryInterface,
    core_text_services_manager_statics_AddRef,
    core_text_services_manager_statics_Release,
    /* IInspectable methods */
    core_text_services_manager_statics_GetIids,
    core_text_services_manager_statics_GetRuntimeClassName,
    core_text_services_manager_statics_GetTrustLevel,
    /* ICoreTextServicesManagerStatics methods */
    core_text_services_manager_statics_GetForCurrentView
};

static struct core_text_services_manager_statics core_text_services_manager_statics =
{
    {&factory_vtbl},
    {&core_text_services_manager_statics_vtbl},
    1,
};

IActivationFactory *core_text_services_manager_factory = &core_text_services_manager_statics.IActivationFactory_iface;
