/* IGatt* Implementation
 *
 * Copyright 2026 Vibhav Pant
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
#include "bthledef.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( bluetooth );

struct gatt_service
{
    IGattDeviceService IGattDeviceService_iface;
    LONG ref;

    BTH_LE_GATT_SERVICE service;
};

static inline struct gatt_service *impl_from_IGattDeviceService( IGattDeviceService *iface )
{
    return CONTAINING_RECORD( iface, struct gatt_service, IGattDeviceService_iface );
}

static HRESULT WINAPI gatt_service_QueryInterface( IGattDeviceService *iface, REFIID iid, void **out )
{
    struct gatt_service *impl = impl_from_IGattDeviceService( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IGattDeviceService ))
    {
        IGattDeviceService_AddRef(( *out = &impl->IGattDeviceService_iface ));
        return S_OK;
    }
    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI gatt_service_AddRef( IGattDeviceService *iface )
{
    struct gatt_service *impl = impl_from_IGattDeviceService( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI gatt_service_Release( IGattDeviceService *iface )
{
    struct gatt_service *impl = impl_from_IGattDeviceService( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "(%p)\n", iface );
    if (!ref)
        free( impl );
    return ref;
}

static HRESULT WINAPI gatt_service_GetIids( IGattDeviceService *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI gatt_service_GetRuntimeClassName( IGattDeviceService *iface, HSTRING *class_name )
{
    FIXME( "(%p, %p): stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI gatt_service_GetTrustLevel( IGattDeviceService *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI gatt_service_GetCharacteristics( IGattDeviceService *iface, GUID uuid, IVectorView_GattCharacteristic **chars )
{
    FIXME( "(%p, %s, %p): stub!\n", iface, debugstr_guid( &uuid ), chars );
    return E_NOTIMPL;
}

static HRESULT WINAPI gatt_service_GetIncludedServices( IGattDeviceService *iface, GUID uuid, IVectorView_GattDeviceService **services )
{
    FIXME( "(%p, %s, %p): stub!\n", iface, debugstr_guid( &uuid ), services );
    return E_NOTIMPL;
}

static HRESULT WINAPI gatt_service_get_DeviceId( IGattDeviceService *iface, HSTRING *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI gatt_service_get_Uuid( IGattDeviceService *iface, GUID *value )
{
    struct gatt_service *impl = impl_from_IGattDeviceService( iface );
    TRACE( "(%p, %p)\n", iface, value );

    if (impl->service.ServiceUuid.IsShortUuid)
    {
        *value = BTH_LE_ATT_BLUETOOTH_BASE_GUID;
        value->Data1 = impl->service.ServiceUuid.Value.ShortUuid;
    }
    else
        *value = impl->service.ServiceUuid.Value.LongUuid;
    return S_OK;
}

static HRESULT WINAPI gatt_service_get_AttributeHandle( IGattDeviceService *iface, UINT16 *value )
{
    FIXME( "(%p, %p): stub!\n", iface, value );
    return E_NOTIMPL;
}

static const IGattDeviceServiceVtbl gatt_service_vtbl =
{
    /* IUnknown */
    gatt_service_QueryInterface,
    gatt_service_AddRef,
    gatt_service_Release,
    /* IInspectable */
    gatt_service_GetIids,
    gatt_service_GetRuntimeClassName,
    gatt_service_GetTrustLevel,
    /* IGattDeviceService */
    gatt_service_GetCharacteristics,
    gatt_service_GetIncludedServices,
    gatt_service_get_DeviceId,
    gatt_service_get_Uuid,
    gatt_service_get_AttributeHandle
};

HRESULT gatt_service_create( const BTH_LE_GATT_SERVICE *svc, IGattDeviceService **service )
{
    struct gatt_service *impl;

    if (!(impl = calloc( 1, sizeof( *impl ) )))
        return E_OUTOFMEMORY;
    impl->IGattDeviceService_iface.lpVtbl = &gatt_service_vtbl;
    impl->ref = 1;
    impl->service = *svc;
    *service = &impl->IGattDeviceService_iface;
    return S_OK;
}
