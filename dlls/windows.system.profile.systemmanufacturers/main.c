/* WinRT Windows.System.Profile.SystemManufacturers Implementation
 *
 * Copyright (C) 2022 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(smbios);

struct smbios_statics
{
    IActivationFactory IActivationFactory_iface;
    ISmbiosInformationStatics ISmbiosInformationStatics_iface;
    LONG ref;
};

static inline struct smbios_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct smbios_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct smbios_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_ISmbiosInformationStatics ))
    {
        *out = &impl->ISmbiosInformationStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct smbios_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct smbios_statics *impl = impl_from_IActivationFactory( iface );
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

DEFINE_IINSPECTABLE( statics, ISmbiosInformationStatics, struct smbios_statics, IActivationFactory_iface )

static HRESULT get_bios_serialnumber( BSTR *value )
{
    const WCHAR *class = L"Win32_BIOS";
    IEnumWbemClassObject *wbem_enum;
    IWbemClassObject *wbem_class;
    IWbemServices *wbem_service;
    IWbemLocator *wbem_locator;
    VARIANT serial;
    ULONG count;
    HRESULT hr;
    BSTR bstr;

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void**)&wbem_locator );
    if (FAILED(hr)) return hr;

    bstr = SysAllocString( L"ROOT\\CIMV2" );
    if (!bstr)
    {
        IWbemLocator_Release( wbem_locator );
        return E_OUTOFMEMORY;
    }
    hr = IWbemLocator_ConnectServer( wbem_locator, bstr, NULL, NULL, NULL, 0, NULL, NULL, &wbem_service );
    IWbemLocator_Release( wbem_locator );
    SysFreeString( bstr );
    if (FAILED(hr)) return hr;

    bstr = SysAllocString( class );
    if (!bstr)
    {
        IWbemServices_Release( wbem_service );
        return E_OUTOFMEMORY;
    }
    hr = IWbemServices_CreateInstanceEnum( wbem_service, bstr, WBEM_FLAG_SYSTEM_ONLY, NULL, &wbem_enum );
    IWbemServices_Release( wbem_service );
    SysFreeString( bstr );
    if (FAILED(hr)) return hr;

    hr = IEnumWbemClassObject_Next( wbem_enum, 1000, 1, &wbem_class, &count );
    IEnumWbemClassObject_Release( wbem_enum );
    if (FAILED(hr)) return hr;

    hr = IWbemClassObject_Get( wbem_class, L"SerialNumber", 0, &serial, NULL, NULL );
    IWbemClassObject_Release( wbem_class );
    if (FAILED(hr)) return hr;

    *value = V_BSTR( &serial );
    VariantClear( &serial );
    return hr;
}

static HRESULT WINAPI statics_get_SerialNumber( ISmbiosInformationStatics *iface, HSTRING *value )
{
    BSTR serial;
    HRESULT hr;

    TRACE( "iface %p, value %p.\n", iface, value );

    if (FAILED( hr = get_bios_serialnumber( &serial ) )) return hr;
    if (FAILED( hr = WindowsCreateString( serial, wcslen(serial), value ) )) return hr;

    TRACE( "Returning serial number: %s.\n", debugstr_w( serial ) );
    return hr;
}

static const struct ISmbiosInformationStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* ISmbiosInformationStatics methods */
    statics_get_SerialNumber,
};

static struct smbios_statics smbios_statics =
{
    {&factory_vtbl},
    {&statics_vtbl},
    1,
};

static IActivationFactory *smbios_factory = &smbios_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *name = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "classid %s, factory %p.\n", debugstr_hstring(classid), factory );

    *factory = NULL;

    if (!wcscmp( name, RuntimeClass_Windows_System_Profile_SystemManufacturers_SmbiosInformation ))
        IActivationFactory_QueryInterface( smbios_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
