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

#include "roapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enumeration);

struct device_information
{
    IDeviceInformation IDeviceInformation_iface;
    LONG ref;

    IMap_HSTRING_IInspectable *properties;
    HSTRING id;
};

static inline struct device_information *impl_from_IDeviceInformation( IDeviceInformation *iface )
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
    struct device_information *impl = impl_from_IDeviceInformation( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI device_information_Release( IDeviceInformation *iface )
{
    struct device_information *impl = impl_from_IDeviceInformation( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        if (impl->properties) IMap_HSTRING_IInspectable_Release( impl->properties );
        WindowsDeleteString( impl->id );
        free( impl );
    }

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
    struct device_information *impl = impl_from_IDeviceInformation( iface );
    TRACE( "iface %p, id %p\n", iface, id );
    return WindowsDuplicateString( impl->id, id );
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
    struct device_information *impl = impl_from_IDeviceInformation( iface );
    TRACE( "iface %p, properties %p.\n", iface, properties );
    return IMap_HSTRING_IInspectable_GetView( impl->properties, properties );
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

static const char *debugstr_DEVPROPKEY( const DEVPROPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s, %04lx}", debugstr_guid( &key->fmtid ), key->pid );
}

static HRESULT create_device_properties( const DEVPROPERTY *props, ULONG len, IMap_HSTRING_IInspectable **map )
{
    static const WCHAR *propertyset_name = RuntimeClass_Windows_Foundation_Collections_PropertySet;
    static const WCHAR *propertyvalue_name = RuntimeClass_Windows_Foundation_PropertyValue;

    IPropertyValueStatics *propval_statics = NULL;
    IPropertySet *propset;
    HSTRING_HEADER hdr;
    HSTRING str;
    HRESULT hr;
    ULONG i;

    TRACE( "props %p, len %lu, map %p.\n", props, len, map );

    WindowsCreateStringReference( propertyset_name, wcslen( propertyset_name ), &hdr, &str );
    if (FAILED(hr = RoActivateInstance( str, (IInspectable **)&propset ))) return hr;
    hr = IPropertySet_QueryInterface( propset, &IID_IMap_HSTRING_IInspectable, (void **)map );
    IPropertySet_Release( propset );
    if (FAILED(hr)) goto done;

    WindowsCreateStringReference( propertyvalue_name, wcslen( propertyvalue_name ), &hdr, &str );
    hr = RoGetActivationFactory( str, &IID_IPropertyValueStatics, (void **)&propval_statics );

    for (i = 0; SUCCEEDED(hr) && i < len; i++)
    {
        const DEVPROPERTY *prop = &props[i];
        const DEVPROPKEY *propkey = &prop->CompKey.Key;
        HSTRING canonical_name;
        IInspectable *val;
        boolean replaced;
        WCHAR *name;

        switch (prop->Type)
        {
        case DEVPROP_TYPE_BOOLEAN:
            hr = IPropertyValueStatics_CreateBoolean( propval_statics, !!*(DEVPROP_BOOLEAN *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_BYTE:
        case DEVPROP_TYPE_SBYTE:
            hr = IPropertyValueStatics_CreateUInt8( propval_statics, *(BYTE *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_UINT16:
            hr = IPropertyValueStatics_CreateUInt16( propval_statics, *(UINT16 *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_INT16:
            hr = IPropertyValueStatics_CreateInt16( propval_statics, *(INT16 *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_UINT32:
            hr = IPropertyValueStatics_CreateUInt32( propval_statics, *(UINT32 *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_INT32:
            hr = IPropertyValueStatics_CreateInt32( propval_statics, *(INT32 *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_UINT64:
            hr = IPropertyValueStatics_CreateUInt64( propval_statics, *(UINT64 *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_INT64:
            hr = IPropertyValueStatics_CreateInt64( propval_statics, *(INT64 *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_FLOAT:
            hr = IPropertyValueStatics_CreateSingle( propval_statics, *(FLOAT *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_DOUBLE:
            hr = IPropertyValueStatics_CreateDouble( propval_statics, *(DOUBLE *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_GUID:
            hr = IPropertyValueStatics_CreateGuid( propval_statics, *(GUID *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_FILETIME:
            hr = IPropertyValueStatics_CreateDateTime( propval_statics, *(DateTime *)prop->Buffer, &val );
            break;
        case DEVPROP_TYPE_STRING:
        {
            WindowsCreateStringReference( prop->Buffer, wcslen( prop->Buffer ), &hdr, &str );
            hr = IPropertyValueStatics_CreateString( propval_statics, str, &val );
            break;
        }
        default:
            FIXME("Unsupported DEVPROPTYPE: %#lx\n", prop->Type );
            continue;
        }
        if (FAILED(hr)) break;
        if (SUCCEEDED(hr = PSGetNameFromPropertyKey( (PROPERTYKEY *)propkey, &name )))
        {
            hr = WindowsCreateString( name, wcslen( name ), &canonical_name );
            CoTaskMemFree( name );
        }
        else if (hr == TYPE_E_ELEMENTNOTFOUND)
        {
            const GUID *fmtid = &propkey->fmtid;
            WCHAR buf[80];

            WARN( "Unknown property key: %s\n", debugstr_DEVPROPKEY( propkey ) );
            swprintf( buf, ARRAY_SIZE( buf ), L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} %lu", fmtid->Data1, fmtid->Data2, fmtid->Data3,
                      fmtid->Data4[0], fmtid->Data4[1], fmtid->Data4[2], fmtid->Data4[3], fmtid->Data4[4], fmtid->Data4[5], fmtid->Data4[6], fmtid->Data4[7], propkey->pid );
            hr = WindowsCreateString( buf, wcslen( buf ), &canonical_name );
        }

        if (SUCCEEDED(hr))
        {
            hr = IMap_HSTRING_IInspectable_Insert( *map, canonical_name, val, &replaced );
            WindowsDeleteString( canonical_name );
        }
        IInspectable_Release( val );
    }

done:
    if (propval_statics) IPropertyValueStatics_Release( propval_statics );
    if (FAILED(hr) && *map) IMap_HSTRING_IInspectable_Release( *map );
    return hr;
}

static const char *debugstr_DEV_OBJECT_TYPE( DEV_OBJECT_TYPE type )
{
    static const char *str[] = {
        "DevObjectTypeUnknown",
        "DevObjectTypeDeviceInterface",
        "DevObjectTypeDeviceContainer",
        "DevObjectTypeDevice",
        "DevObjectTypeDeviceInterfaceClass",
        "DevObjectTypeAEP",
        "DevObjectTypeAEPContainer",
        "DevObjectTypeDeviceInstallerClass",
        "DevObjectTypeDeviceInterfaceDisplay",
        "DevObjectTypeDeviceContainerDisplay",
        "DevObjectTypeAEPService",
        "DevObjectTypeDevicePanel",
        "DevObjectTypeAEPProtocol",
    };
    if (type >= ARRAY_SIZE( str )) return wine_dbg_sprintf( "(unknown %d)", type );
    return wine_dbg_sprintf( "%s", str[type] );
}

static const char *debugstr_DEV_OBJECT( const DEV_OBJECT *obj )
{
    if (!obj) return "(null)";
    return wine_dbg_sprintf( "{%s, %s, %lu, %p}", debugstr_DEV_OBJECT_TYPE( obj->ObjectType ), debugstr_w( obj->pszObjectId ), obj->cPropertyCount,
                             obj->pProperties );
}

HRESULT device_information_create( const DEV_OBJECT *obj, IDeviceInformation **info )
{
    struct device_information *impl;
    HRESULT hr;

    TRACE( "obj %s, info %p\n", debugstr_DEV_OBJECT( obj ), info );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IDeviceInformation_iface.lpVtbl = &device_information_vtbl;
    impl->ref = 1;
    if (FAILED(hr = create_device_properties( obj->pProperties, obj->cPropertyCount, &impl->properties )))
    {
        free( impl );
        return hr;
    }

    if (FAILED(hr = WindowsCreateString( obj->pszObjectId, wcslen( obj->pszObjectId ), &impl->id )))
    {
        IMap_HSTRING_IInspectable_Release( impl->properties );
        free( impl );
        return hr;
    }

    *info = &impl->IDeviceInformation_iface;
    TRACE( "created DeviceInformation %p\n", impl );
    return S_OK;
}
