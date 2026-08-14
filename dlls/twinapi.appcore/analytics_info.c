/* WinRT Windows.Globalization implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(twinapi);

struct analytics_version_info
{
    IAnalyticsVersionInfo IAnalyticsVersionInfo_iface;
    LONG ref;
};

static inline struct analytics_version_info *impl_from_IAnalyticsVersionInfo( IAnalyticsVersionInfo *iface )
{
    return CONTAINING_RECORD( iface, struct analytics_version_info, IAnalyticsVersionInfo_iface );
}

static HRESULT WINAPI analytics_version_info_QueryInterface( IAnalyticsVersionInfo *iface, REFIID iid, void **out )
{
    struct analytics_version_info *impl = impl_from_IAnalyticsVersionInfo( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAnalyticsVersionInfo ))
    {
        IAnalyticsVersionInfo_AddRef( (*out = &impl->IAnalyticsVersionInfo_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI analytics_version_info_AddRef( IAnalyticsVersionInfo *iface )
{
    struct analytics_version_info *impl = impl_from_IAnalyticsVersionInfo( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI analytics_version_info_Release( IAnalyticsVersionInfo *iface )
{
    struct analytics_version_info *impl = impl_from_IAnalyticsVersionInfo( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI analytics_version_info_GetIids( IAnalyticsVersionInfo *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI analytics_version_info_GetRuntimeClassName( IAnalyticsVersionInfo *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI analytics_version_info_GetTrustLevel( IAnalyticsVersionInfo *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI analytics_version_info_get_DeviceFamily( IAnalyticsVersionInfo *iface, HSTRING *value )
{
    TRACE( "iface %p, value %p\n", iface, value );
    return WindowsCreateString( L"Windows.Desktop", wcslen( L"Windows.Desktop" ), value );
}

static HRESULT WINAPI analytics_version_info_get_DeviceFamilyVersion( IAnalyticsVersionInfo *iface, HSTRING *value )
{
    DWORD revision, size = sizeof(revision);
    WCHAR buffer[32];
    UINT64 version;
    UINT len;

    TRACE( "iface %p, value %p\n", iface, value );

    if (RegGetValueW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", L"UBR",
                      RRF_RT_REG_DWORD, NULL, &revision, &size ))
        revision = 0;
    version = NtCurrentTeb()->Peb->OSMajorVersion & 0xffff;
    version = (version << 16) | (NtCurrentTeb()->Peb->OSMinorVersion & 0xffff);
    version = (version << 16) | (NtCurrentTeb()->Peb->OSBuildNumber & 0xffff);
    version = (version << 16) | revision;

    len = swprintf( buffer, ARRAY_SIZE(buffer), L"%I64u", version );
    return WindowsCreateString( buffer, len, value );
}

static IAnalyticsVersionInfoVtbl analytics_version_info_vtbl =
{
    analytics_version_info_QueryInterface,
    analytics_version_info_AddRef,
    analytics_version_info_Release,
    /* IInspectable methods */
    analytics_version_info_GetIids,
    analytics_version_info_GetRuntimeClassName,
    analytics_version_info_GetTrustLevel,
    /* IAnalyticsVersionInfo methods */
    analytics_version_info_get_DeviceFamily,
    analytics_version_info_get_DeviceFamilyVersion,
};

struct analytics_info_factory
{
    IActivationFactory IActivationFactory_iface;
    IAnalyticsInfoStatics IAnalyticsInfoStatics_iface;
    LONG ref;
};

static inline struct analytics_info_factory *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct analytics_info_factory, IActivationFactory_iface );
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct analytics_info_factory *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IAnalyticsInfoStatics ))
    {
        IAnalyticsInfoStatics_AddRef( (*out = &impl->IAnalyticsInfoStatics_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct analytics_info_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct analytics_info_factory *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **out )
{
    FIXME( "iface %p, out %p stub!\n", iface, out );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

DEFINE_IINSPECTABLE( statics, IAnalyticsInfoStatics, struct analytics_info_factory, IActivationFactory_iface );

static HRESULT WINAPI statics_get_VersionInfo( IAnalyticsInfoStatics *iface, IAnalyticsVersionInfo **value )
{
    struct analytics_version_info *info;

    TRACE( "iface %p, out %p.\n", iface, value );

    if (!(info = calloc( 1, sizeof(*info) ))) return E_OUTOFMEMORY;
    info->IAnalyticsVersionInfo_iface.lpVtbl = &analytics_version_info_vtbl;
    info->ref = 1;

    *value = &info->IAnalyticsVersionInfo_iface;
    return S_OK;
}

static HRESULT WINAPI statics_get_DeviceForm( IAnalyticsInfoStatics *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IAnalyticsInfoStaticsVtbl statics_vtbl =
{
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* IAnalyticsInfoStatics methods */
    statics_get_VersionInfo,
    statics_get_DeviceForm,
};

static struct analytics_info_factory factory =
{
    {&activation_factory_vtbl},
    {&statics_vtbl},
    1,
};

IActivationFactory *analytics_info_factory = &factory.IActivationFactory_iface;
