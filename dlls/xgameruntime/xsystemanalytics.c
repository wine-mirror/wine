/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystem
 *
 * Written by Weather
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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

struct x_system_analytics
{
    IXSystemAnalyticsImpl IXSystemAnalyticsImpl_iface;
    LONG ref;
};

static inline struct x_system_analytics *impl_from_IXSystemAnalyticsImpl( IXSystemAnalyticsImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_system_analytics, IXSystemAnalyticsImpl_iface );
}

static HRESULT WINAPI x_system_analytics_QueryInterface( IXSystemAnalyticsImpl *iface, REFIID iid, void **out )
{
    struct x_system_analytics *impl = impl_from_IXSystemAnalyticsImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown              ) ||
        IsEqualGUID( iid, &IID_IXSystemAnalyticsImpl ))
    {
        IXSystemAnalyticsImpl_AddRef( *out = &impl->IXSystemAnalyticsImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_system_analytics_AddRef( IXSystemAnalyticsImpl *iface )
{
    struct x_system_analytics *impl = impl_from_IXSystemAnalyticsImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_system_analytics_Release( IXSystemAnalyticsImpl *iface )
{
    struct x_system_analytics *impl = impl_from_IXSystemAnalyticsImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static XSystemAnalyticsInfo* WINAPI x_system_analytics_XSystemGetAnalyticsInfo( IXSystemAnalyticsImpl *iface, XSystemAnalyticsInfo *__ret )
{
    /* For Windows, XSystemAnalyticsInfo->form is always "Desktop" */
    const WCHAR *analytics_info_str = RuntimeClass_Windows_System_Profile_AnalyticsInfo;
    HSTRING analytics_info_class, deviceFamilyVersion, deviceFamily;
    const WCHAR *deviceFamilyVersionStr, *deviceFamilyStr;
    XSystemAnalyticsInfo info;
    char *str, *splitter;
    ULONGLONG version;
    HRESULT status;
    UINT32 strSize;

    IAnalyticsInfoStatics *analytics_info_statics = NULL;
    IAnalyticsVersionInfo *analytics_version_info = NULL;

    TRACE( "iface %p.\n", iface );

    status = WindowsCreateString( analytics_info_str, wcslen( analytics_info_str ), &analytics_info_class );
    if (FAILED( status )) return NULL;

    status = RoGetActivationFactory( analytics_info_class, &IID_IAnalyticsInfoStatics, (void **)&analytics_info_statics );
    WindowsDeleteString( analytics_info_class );
    if (FAILED( status )) return NULL;

    status = IAnalyticsInfoStatics_get_VersionInfo( analytics_info_statics, &analytics_version_info );
    IAnalyticsInfoStatics_Release( analytics_info_statics );
    if (FAILED( status )) return NULL;

    status = IAnalyticsVersionInfo_get_DeviceFamilyVersion( analytics_version_info, &deviceFamilyVersion );
    if (FAILED( status ))
    {
        IAnalyticsVersionInfo_Release( analytics_version_info );
        return NULL;
    }

    status = IAnalyticsVersionInfo_get_DeviceFamily( analytics_version_info, &deviceFamily );
    IAnalyticsVersionInfo_Release( analytics_version_info );
    if (FAILED( status ))
    {
        WindowsDeleteString( deviceFamilyVersion );
        return NULL;
    }

    deviceFamilyStr = WindowsGetStringRawBuffer( deviceFamily, NULL );
    strSize = WideCharToMultiByte( CP_UTF8, 0, deviceFamilyStr, -1, NULL, 0, NULL, NULL );

    str = (char *)malloc( strSize );
    if (!str)
    {
        WindowsDeleteString( deviceFamilyVersion );
        WindowsDeleteString( deviceFamily );
        return NULL;
    }

    if (!WideCharToMultiByte( CP_UTF8, 0, deviceFamilyStr, -1, str, strSize, NULL, NULL ))
    {
        WindowsDeleteString( deviceFamilyVersion );
        WindowsDeleteString( deviceFamily );
        free( str );
        return NULL;
    }

    splitter = strchr( str, '.' );
    if (splitter)
    {
        *splitter = '\0';

        strcpy( info.family, str );
        strcpy( info.form, splitter + 1 );
    }

    WindowsDeleteString( deviceFamily );
    free( str );

    deviceFamilyVersionStr = WindowsGetStringRawBuffer( deviceFamilyVersion, NULL );
    strSize = WideCharToMultiByte( CP_UTF8, 0, deviceFamilyVersionStr, -1, NULL, 0, NULL, NULL );

    str = (char *)malloc( strSize );
    if (!str)
    {
        WindowsDeleteString( deviceFamilyVersion );
        return NULL;
    }

    if (!WideCharToMultiByte( CP_UTF8, 0, deviceFamilyVersionStr, -1, str, strSize, NULL, NULL ))
    {
        WindowsDeleteString( deviceFamilyVersion );
        free( str );
        return NULL;
    }

    version = strtoull( str, NULL, 10 );
    info.osVersion.major = (UINT16)(version >> 48);
    info.osVersion.minor = (UINT16)((version >> 32) & 0xFFFF);
    info.osVersion.build = (UINT16)((version >> 16) & 0xFFFF);
    info.osVersion.revision = (UINT16)(version & 0xFFFF);

    WindowsDeleteString( deviceFamilyVersion );
    free( str );

    info.hostingOsVersion = info.osVersion;

    *__ret = info;

    return __ret;
}

static const struct IXSystemAnalyticsImplVtbl x_system_analytics_vtbl =
{
    x_system_analytics_QueryInterface,
    x_system_analytics_AddRef,
    x_system_analytics_Release,
    /* IXSystemAnalyticsImpl methods */
    x_system_analytics_XSystemGetAnalyticsInfo,
};

static struct x_system_analytics x_system_analytics =
{
    {&x_system_analytics_vtbl},
    0,
};

IXSystemAnalyticsImpl *x_system_analytics_impl = &x_system_analytics.IXSystemAnalyticsImpl_iface;
