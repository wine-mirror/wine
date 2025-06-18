/* WinRT Windows.Devices.Usb UsbDevice Implementation
 *
 * Copyright (C) 2023 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(usb);

struct usb_device_statics
{
    IActivationFactory IActivationFactory_iface;
    IUsbDeviceStatics IUsbDeviceStatics_iface;
    LONG ref;
};

static inline struct usb_device_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct usb_device_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct usb_device_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IUsbDeviceStatics ))
    {
        *out = &impl->IUsbDeviceStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct usb_device_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct usb_device_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( usb_device_statics, IUsbDeviceStatics, struct usb_device_statics, IActivationFactory_iface )

static HRESULT WINAPI usb_device_statics_GetDeviceSelector( IUsbDeviceStatics *iface, UINT32 vendor,
                                                            UINT32 product, GUID class, HSTRING *value )
{
    FIXME( "iface %p, vendor %d, product %d, class %s, value %p stub!\n", iface, vendor, product, debugstr_guid(&class), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI usb_device_statics_GetDeviceSelectorGuidOnly( IUsbDeviceStatics *iface, GUID class, HSTRING *value )
{
    FIXME( "iface %p, class %s, value %p stub!\n", iface, debugstr_guid(&class), value );
    return E_NOTIMPL;
}

static HRESULT WINAPI usb_device_statics_GetDeviceSelectorVidPidOnly( IUsbDeviceStatics *iface, UINT32 vendor,
                                                                      UINT32 product, HSTRING *value )
{
    static const WCHAR *format = L"System.Devices.InterfaceClassGuid:=\"{DEE824EF-729B-4A0E-9C14-B7117D33A817}\""
                                 L" AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True"
                                 L" AND System.DeviceInterface.WinUsb.UsbVendorId:=%d"
                                 L" AND System.DeviceInterface.WinUsb.UsbProductId:=%d";
    WCHAR buffer[254 + 20];
    HRESULT hr;

    TRACE( "iface %p, vendor %d, product %d, value %p.\n", iface, vendor, product, value );

    if (!value) return E_INVALIDARG;

    swprintf( buffer, ARRAYSIZE(buffer), format, (INT32)vendor, (INT32)product );
    hr = WindowsCreateString( buffer, wcslen(buffer), value );

    TRACE( "Returning value = %s\n", debugstr_hstring(*value) );
    return hr;
}

static HRESULT WINAPI usb_device_statics_GetDeviceClassSelector( IUsbDeviceStatics *iface, IUsbDeviceClass *class, HSTRING *value )
{
    FIXME( "iface %p, class %p, value %p stub!\n", iface, class, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI usb_device_statics_FromIdAsync( IUsbDeviceStatics *iface, HSTRING id, IAsyncOperation_UsbDevice **operation )
{
    FIXME( "iface %p, id %s, operation %p stub!\n", iface, debugstr_hstring(id), operation );
    return E_NOTIMPL;
}

static const struct IUsbDeviceStaticsVtbl usb_device_statics_vtbl =
{
    usb_device_statics_QueryInterface,
    usb_device_statics_AddRef,
    usb_device_statics_Release,
    /* IInspectable methods */
    usb_device_statics_GetIids,
    usb_device_statics_GetRuntimeClassName,
    usb_device_statics_GetTrustLevel,
    /* IUsbDeviceStatics methods */
    usb_device_statics_GetDeviceSelector,
    usb_device_statics_GetDeviceSelectorGuidOnly,
    usb_device_statics_GetDeviceSelectorVidPidOnly,
    usb_device_statics_GetDeviceClassSelector,
    usb_device_statics_FromIdAsync,
};

static struct usb_device_statics usb_device_statics =
{
    {&factory_vtbl},
    {&usb_device_statics_vtbl},
    1,
};

IActivationFactory *usb_device_factory = &usb_device_statics.IActivationFactory_iface;
