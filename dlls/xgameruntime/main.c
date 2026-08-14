/*
 * Xbox Game runtime Library
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

#include <initguid.h>
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xgameruntime);

DWORD tlsIndex;

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    TRACE( "hinst %p, reason %lu, reserved %p.\n", hinst, reason, reserved );

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            if ((tlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) return FALSE;
        case DLL_THREAD_ATTACH:
            TlsSetValue( tlsIndex, FALSE );
            break;
        case DLL_PROCESS_DETACH:
            TlsFree( tlsIndex );
            break;
    }
    return TRUE;
}

struct initialize_options
{
    UINT32 unk;
    BOOL isInline;
    const char *gameConfig;
};

HRESULT WINAPI InitializeApiImplEx2( ULONG gdkVer, ULONG gsVer, char mode, const struct initialize_options *options )
{
    TRACE( "gdkVer %ld, gsVer %ld, mode %d, options %p.\n", gdkVer, gsVer, mode, options );
    return S_OK;
}

HRESULT WINAPI InitializeApiImplEx( ULONG gdkVer, ULONG gsVer, char mode )
{
    return InitializeApiImplEx2( gdkVer, gsVer, mode, NULL );
}

HRESULT WINAPI InitializeApiImpl( ULONG gdkVer, ULONG gsVer )
{
    return InitializeApiImplEx2( gdkVer, gsVer, 0, NULL );
}

HRESULT WINAPI QueryApiImpl( REFCLSID clsid, REFIID iid, void **out )
{
    TRACE( "clsid %s, iid %s, out %p.\n", debugstr_guid( clsid ), debugstr_guid( iid ), out );

    if (IsEqualGUID( clsid, &CLSID_XAccessibilityImpl ))
        return IXAccessibilityImpl_QueryInterface( x_accessibility_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XAppCaptureImpl ))
        return IXAppCaptureImpl_QueryInterface( x_app_capture_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XAppCaptureMetadataImpl ))
        return IXAppCaptureMetadataImpl_QueryInterface( x_app_capture_metadata_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XDisplayImpl ))
        return IXDisplayImpl_QueryInterface( x_display_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XErrorImpl ))
        return IXErrorImpl_QueryInterface( x_error_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameImpl ))
        return IXGameImpl_QueryInterface( x_game_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameActivationImpl ))
        return IXGameActivationImpl_QueryInterface( x_game_activation_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameEventImpl ))
        return IXGameEventImpl_QueryInterface( x_game_event_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameInviteImpl ))
        return IXGameInviteImpl_QueryInterface( x_game_invite_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameProtocolImpl ))
        return IXGameProtocolImpl_QueryInterface( x_game_protocol_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameRuntimeFeatureImpl ))
        return IXGameRuntimeFeatureImpl_QueryInterface( x_game_runtime_feature_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameSaveImpl ))
        return IXGameSaveImpl_QueryInterface( x_game_save_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameStreamingImpl ))
        return IXGameStreamingImpl_QueryInterface( x_game_streaming_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XGameUiImpl ))
        return IXGameUiImpl_QueryInterface( x_game_ui_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XLauncherImpl ))
        return IXLauncherImpl_QueryInterface( x_launcher_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XNetworkingImpl ))
        return IXNetworkingImpl_QueryInterface( x_networking_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XPackageImpl ))
        return IXPackageImpl_QueryInterface( x_package_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XPersistentLocalStorageImpl ))
        return IXPersistentLocalStorageImpl_QueryInterface( x_persistent_local_storage_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XStoreImpl ))
        return IXStoreImpl_QueryInterface( x_store_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XSystemImpl ))
        return IXSystemImpl_QueryInterface( x_system_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XSystemAnalyticsImpl ))
        return IXSystemAnalyticsImpl_QueryInterface( x_system_analytics_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XThreadingImpl ))
        return IXThreadingImpl_QueryInterface( x_threading_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XUserImpl ))
        return IXUserImpl_QueryInterface( x_user_impl, iid, out );
    if (IsEqualGUID( clsid, &CLSID_XUserDeviceImpl ))
        return IXUserDeviceImpl_QueryInterface( x_user_device_impl, iid, out );

    return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
}
