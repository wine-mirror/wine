/* DeviceInformation implementation.
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

#include <roapi.h>

#include <wine/debug.h>
#include <wine/rbtree.h>

WINE_DEFAULT_DEBUG_CHANNEL(enumeration);

struct device_information
{
    IDeviceInformation IDeviceInformation_iface;
    LONG ref;
};

static inline struct device_information *impl_DeviceInterface_from_IDeviceInformation( IDeviceInformation *iface )
{
    return CONTAINING_RECORD( iface, struct device_information, IDeviceInformation_iface );
}

static HRESULT WINAPI device_information_QueryInterface( IDeviceInformation *iface, REFIID iid, void **out )
{
    TRACE( "iface %p, iid %s, out %p\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IDeviceInformation ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI device_information_AddRef( IDeviceInformation *iface )
{
    struct device_information *impl = impl_DeviceInterface_from_IDeviceInformation( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI device_information_Release( IDeviceInformation *iface )
{
    struct device_information *impl = impl_DeviceInterface_from_IDeviceInformation( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI device_information_GetIids( IDeviceInformation *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_GetRuntimeClassName( IDeviceInformation *iface, HSTRING *class_name )
{
    const static WCHAR *name = RuntimeClass_Windows_Devices_Enumeration_DeviceInformation;
    TRACE( "iface %p, class_name %p\n", iface, class_name );
    return WindowsCreateString( name, wcslen( name ), class_name );
}

static HRESULT WINAPI device_information_GetTrustLevel( IDeviceInformation *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_get_Id( IDeviceInformation *iface, HSTRING *id )
{
    FIXME( "iface %p, id %p stub!\n", iface, id );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_get_Name( IDeviceInformation *iface, HSTRING *name )
{
    FIXME( "iface %p, name %p stub!\n", iface, name );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_get_IsEnabled( IDeviceInformation *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_IsDefault( IDeviceInformation *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_get_EnclosureLocation( IDeviceInformation *iface, IEnclosureLocation **location )
{
    FIXME( "iface %p, location %p stub!\n", iface, location );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_get_Properties( IDeviceInformation *iface, IMapView_HSTRING_IInspectable **properties )
{
    FIXME( "iface %p, properties %p stub!\n", iface, properties );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_Update( IDeviceInformation *iface, IDeviceInformationUpdate *update )
{
    FIXME( "iface %p, update %p stub!\n", iface, update );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_GetThumbnailAsync( IDeviceInformation *iface, IAsyncOperation_DeviceThumbnail **async )
{
    FIXME( "iface %p, async %p stub!\n", iface, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI device_information_GetGlyphThumbnailAsync( IDeviceInformation *iface,
                                                                 IAsyncOperation_DeviceThumbnail **async )
{
    FIXME( "iface %p, async %p stub!\n", iface, async );
    return E_NOTIMPL;
}

static const struct IDeviceInformationVtbl device_information_vtbl =
{
    /* IUnknown */
    device_information_QueryInterface,
    device_information_AddRef,
    device_information_Release,
    /* IInspectable */
    device_information_GetIids,
    device_information_GetRuntimeClassName,
    device_information_GetTrustLevel,
    /* IDeviceInformation */
    device_information_get_Id,
    device_information_get_Name,
    device_information_get_IsEnabled,
    device_information_IsDefault,
    device_information_get_EnclosureLocation,
    device_information_get_Properties,
    device_information_Update,
    device_information_GetThumbnailAsync,
    device_information_GetGlyphThumbnailAsync,
};

HRESULT device_information_create( IDeviceInformation **info )
{
    struct device_information *impl;

    TRACE( "info %p\n", info );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IDeviceInformation_iface.lpVtbl = &device_information_vtbl;
    impl->ref = 1;

    *info = &impl->IDeviceInformation_iface;
    TRACE( "created DeviceInformation %p\n", impl );
    return S_OK;
}
