/* WinRT Windows.System.Profile.SystemIdentification Implementation
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(systemid);

struct system_id_statics
{
    IActivationFactory IActivationFactory_iface;
    ISystemIdentificationStatics ISystemIdentificationStatics_iface;
    LONG ref;
};

static inline struct system_id_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct system_id_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct system_id_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ISystemIdentificationStatics ))
    {
        *out = &impl->ISystemIdentificationStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct system_id_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct system_id_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
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

struct system_identification_info
{
    ISystemIdentificationInfo ISystemIdentificationInfo_iface;
    LONG ref;

    SystemIdentificationSource system_id_source;
};

static inline struct system_identification_info *impl_from_ISystemIdentificationInfo( ISystemIdentificationInfo *iface )
{
    return CONTAINING_RECORD( iface, struct system_identification_info, ISystemIdentificationInfo_iface );
}

static HRESULT WINAPI system_identification_info_QueryInterface( ISystemIdentificationInfo *iface, REFIID iid, void **out )
{
    struct system_identification_info *impl = impl_from_ISystemIdentificationInfo( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_ISystemIdentificationInfo ))
    {
        *out = &impl->ISystemIdentificationInfo_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI system_identification_info_AddRef( ISystemIdentificationInfo *iface )
{
    struct system_identification_info *impl = impl_from_ISystemIdentificationInfo( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI system_identification_info_Release( ISystemIdentificationInfo *iface )
{
    struct system_identification_info *impl = impl_from_ISystemIdentificationInfo( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI system_identification_info_GetIids( ISystemIdentificationInfo *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI system_identification_info_GetRuntimeClassName( ISystemIdentificationInfo *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI system_identification_info_GetTrustLevel( ISystemIdentificationInfo *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI system_identification_info_get_Id( ISystemIdentificationInfo *iface, IBuffer **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI system_identification_info_get_Source( ISystemIdentificationInfo *iface, SystemIdentificationSource *value )
{
    struct system_identification_info *impl = impl_from_ISystemIdentificationInfo( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_INVALIDARG;

    *value = impl->system_id_source;
    return S_OK;
}

static const struct ISystemIdentificationInfoVtbl system_identification_info_vtbl =
{
    system_identification_info_QueryInterface,
    system_identification_info_AddRef,
    system_identification_info_Release,
    /* IInspectable methods */
    system_identification_info_GetIids,
    system_identification_info_GetRuntimeClassName,
    system_identification_info_GetTrustLevel,
    /* ISystemIdentificationInfo methods */
    system_identification_info_get_Id,
    system_identification_info_get_Source,
};

DEFINE_IINSPECTABLE( system_id_statics, ISystemIdentificationStatics, struct system_id_statics, IActivationFactory_iface )

static HRESULT WINAPI system_id_statics_GetSystemIdForPublisher( ISystemIdentificationStatics *iface, ISystemIdentificationInfo **result )
{
    struct system_identification_info *impl;

    FIXME( "iface %p, result %p semi-stub!\n", iface, result );

    if (!result) return E_INVALIDARG;
    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    impl->ISystemIdentificationInfo_iface.lpVtbl = &system_identification_info_vtbl;
    impl->system_id_source = SystemIdentificationSource_None;
    impl->ref = 1;

    *result = &impl->ISystemIdentificationInfo_iface;
    TRACE( "created ISystemIdentificationInfo %p.\n", *result );
    return S_OK;
}

static HRESULT WINAPI system_id_statics_GetSystemIdForUser( ISystemIdentificationStatics *iface, __x_ABI_CWindows_CSystem_CIUser *user, ISystemIdentificationInfo **result )
{
    FIXME( "iface %p, user %p, result %p stub!\n", iface, user, result );
    return E_NOTIMPL;
}

static const struct ISystemIdentificationStaticsVtbl system_id_statics_vtbl =
{
    system_id_statics_QueryInterface,
    system_id_statics_AddRef,
    system_id_statics_Release,
    /* IInspectable methods */
    system_id_statics_GetIids,
    system_id_statics_GetRuntimeClassName,
    system_id_statics_GetTrustLevel,
    /* ISystemIdentificationStatics methods */
    system_id_statics_GetSystemIdForPublisher,
    system_id_statics_GetSystemIdForUser,
};

static struct system_id_statics system_id_statics =
{
    {&factory_vtbl},
    {&system_id_statics_vtbl},
    1,
};

static IActivationFactory *system_id_factory = &system_id_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid( clsid ), debugstr_guid( riid ), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *name = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "classid %s, factory %p.\n", debugstr_hstring( classid ), factory );

    *factory = NULL;

    if (!wcscmp( name, RuntimeClass_Windows_System_Profile_SystemIdentification ))
        IActivationFactory_QueryInterface( system_id_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
