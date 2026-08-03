/* WinRT Windows.Storage.ApplicationData ApplicationDataContainer Implementation
 *
 * Copyright (C) 2024 Onni Kukkonen
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
#include "pathcch.h"

WINE_DEFAULT_DEBUG_CHANNEL(data);

struct appdata_container_impl
{
    IApplicationDataContainer IApplicationDataContainer_iface;
    LONG ref;
};

static inline struct appdata_container_impl *impl_from_IApplicationDataContainer( IApplicationDataContainer *iface )
{
    return CONTAINING_RECORD( iface, struct appdata_container_impl, IApplicationDataContainer_iface );
}

static HRESULT WINAPI appdata_container_QueryInterface( IApplicationDataContainer *iface, REFIID iid, void **out )
{
    struct appdata_container_impl *impl = impl_from_IApplicationDataContainer( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IApplicationDataContainer ))
    {
        *out = &impl->IApplicationDataContainer_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI appdata_container_AddRef( IApplicationDataContainer *iface )
{
    struct appdata_container_impl *impl = impl_from_IApplicationDataContainer( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI appdata_container_Release( IApplicationDataContainer *iface )
{
    struct appdata_container_impl *impl = impl_from_IApplicationDataContainer( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI appdata_container_GetIids( IApplicationDataContainer *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI appdata_container_GetRuntimeClassName( IApplicationDataContainer *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI appdata_container_GetTrustLevel( IApplicationDataContainer *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI appdata_container_get_Name( IApplicationDataContainer *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI appdata_container_get_Locality( IApplicationDataContainer *iface, ApplicationDataLocality *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI appdata_container_get_Values( IApplicationDataContainer *iface, IPropertySet **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = create_propertyset();
    return S_OK;
}

static HRESULT WINAPI appdata_container_get_Containers( IApplicationDataContainer *iface, IMapView_HSTRING_ApplicationDataContainer **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI appdata_container_CreateContainer( IApplicationDataContainer *iface, HSTRING name, ApplicationDataCreateDisposition disposition, IApplicationDataContainer **container )
{
    FIXME( "iface %p, name %s, disposition %d, container %p semi-stub!\n", iface, debugstr_hstring(name), disposition, container );
    *container = create_data_container();
    return S_OK;
}

static HRESULT WINAPI appdata_container_DeleteContainer( IApplicationDataContainer *iface, HSTRING name )
{
    FIXME( "iface %p, name %s stub!\n", iface, debugstr_hstring(name) );
    return S_OK;
}


static const struct IApplicationDataContainerVtbl appdata_container_vtbl =
{
    appdata_container_QueryInterface,
    appdata_container_AddRef,
    appdata_container_Release,
    /* IInspectable methods */
    appdata_container_GetIids,
    appdata_container_GetRuntimeClassName,
    appdata_container_GetTrustLevel,
    /* IApplicationDataContainer methods */
    appdata_container_get_Name,
    appdata_container_get_Locality,
    appdata_container_get_Values,
    appdata_container_get_Containers,
    appdata_container_CreateContainer,
    appdata_container_DeleteContainer,
};

IApplicationDataContainer *create_data_container() {
    struct appdata_container_impl *object;

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;

    object->IApplicationDataContainer_iface.lpVtbl = &appdata_container_vtbl;
    object->ref = 1;

    return &object->IApplicationDataContainer_iface;
}
