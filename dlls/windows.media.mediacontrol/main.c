/* WinRT Windows.Media.MediaControl Implementation
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

#include "initguid.h"
#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mediacontrol);

static EventRegistrationToken dummy_token = {.value = 0xdeadbeef};

struct media_control_statics
{
    IActivationFactory IActivationFactory_iface;
    ISystemMediaTransportControlsInterop ISystemMediaTransportControlsInterop_iface;
    LONG ref;
};

static inline struct media_control_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct media_control_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct media_control_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_ISystemMediaTransportControlsInterop ))
    {
        *out = &impl->ISystemMediaTransportControlsInterop_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct media_control_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct media_control_statics *impl = impl_from_IActivationFactory( iface );
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

struct music_properties
{
    IMusicDisplayProperties IMusicDisplayProperties_iface;
    IMusicDisplayProperties2 IMusicDisplayProperties2_iface;
    LONG ref;

    HSTRING album_title;
    HSTRING artist;
    HSTRING title;
};

static inline struct music_properties *impl_from_IMusicDisplayProperties( IMusicDisplayProperties *iface )
{
    return CONTAINING_RECORD( iface, struct music_properties, IMusicDisplayProperties_iface );
}

static HRESULT WINAPI music_properties_QueryInterface( IMusicDisplayProperties *iface, REFIID iid, void **out )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IMusicDisplayProperties ))
    {
        *out = &impl->IMusicDisplayProperties_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IMusicDisplayProperties2 ))
    {
        *out = &impl->IMusicDisplayProperties2_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI music_properties_AddRef( IMusicDisplayProperties *iface )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI music_properties_Release( IMusicDisplayProperties *iface )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        WindowsDeleteString( impl->album_title );
        WindowsDeleteString( impl->artist );
        WindowsDeleteString( impl->title );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI music_properties_GetIids( IMusicDisplayProperties *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI music_properties_GetRuntimeClassName( IMusicDisplayProperties *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI music_properties_GetTrustLevel( IMusicDisplayProperties *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI music_properties_get_Title( IMusicDisplayProperties *iface, HSTRING *value )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    return WindowsDuplicateString( impl->title, value );
}

static HRESULT WINAPI music_properties_put_Title( IMusicDisplayProperties *iface, HSTRING value )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    WindowsDeleteString( impl->title );
    return WindowsDuplicateString( value, &impl->title );
}

static HRESULT WINAPI music_properties_get_AlbumArtist( IMusicDisplayProperties *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI music_properties_put_AlbumArtist( IMusicDisplayProperties *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring(value) );
    return E_NOTIMPL;
}

static HRESULT WINAPI music_properties_get_Artist( IMusicDisplayProperties *iface, HSTRING *value )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    return WindowsDuplicateString( impl->artist, value );
}

static HRESULT WINAPI music_properties_put_Artist( IMusicDisplayProperties *iface, HSTRING value )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    WindowsDeleteString( impl->artist );
    return WindowsDuplicateString( value, &impl->artist );
}

static const IMusicDisplayPropertiesVtbl music_properties_vtbl =
{
    music_properties_QueryInterface,
    music_properties_AddRef,
    music_properties_Release,
    /* IInspectable methods */
    music_properties_GetIids,
    music_properties_GetRuntimeClassName,
    music_properties_GetTrustLevel,
    /* IMusicDisplayProperties methods */
    music_properties_get_Title,
    music_properties_put_Title,
    music_properties_get_AlbumArtist,
    music_properties_put_AlbumArtist,
    music_properties_get_Artist,
    music_properties_put_Artist,
};

DEFINE_IINSPECTABLE( music_properties2, IMusicDisplayProperties2, struct music_properties, IMusicDisplayProperties_iface )

static HRESULT STDMETHODCALLTYPE music_properties2_get_AlbumTitle( IMusicDisplayProperties2 *iface, HSTRING *value )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties2( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    return WindowsDuplicateString( impl->album_title, value );
}

static HRESULT STDMETHODCALLTYPE music_properties2_put_AlbumTitle( IMusicDisplayProperties2 *iface, HSTRING value )
{
    struct music_properties *impl = impl_from_IMusicDisplayProperties2( iface );
    TRACE( "iface %p, value %p\n", iface, value );
    WindowsDeleteString( impl->album_title );
    return WindowsDuplicateString( value, &impl->album_title );
}

static HRESULT STDMETHODCALLTYPE music_properties2_get_TrackNumber( IMusicDisplayProperties2 *iface, UINT32 *value )
{
    FIXME( "iface %p, value %p stub\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE music_properties2_put_TrackNumber( IMusicDisplayProperties2 *iface, UINT32 value )
{
    FIXME( "iface %p, value %u stub\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE music_properties2_get_Genres( IMusicDisplayProperties2 *iface, IVector_HSTRING **value )
{
    FIXME( "iface %p, value %p stub\n", iface, value );
    return E_NOTIMPL;
}

static const struct IMusicDisplayProperties2Vtbl music_properties2_vtbl =
{
    music_properties2_QueryInterface,
    music_properties2_AddRef,
    music_properties2_Release,
    /* IInspectable methods */
    music_properties2_GetIids,
    music_properties2_GetRuntimeClassName,
    music_properties2_GetTrustLevel,
    /* IMusicDisplayProperties2 methods */
    music_properties2_get_AlbumTitle,
    music_properties2_put_AlbumTitle,
    music_properties2_get_TrackNumber,
    music_properties2_put_TrackNumber,
    music_properties2_get_Genres,
};

struct display_updater
{
    ISystemMediaTransportControlsDisplayUpdater ISystemMediaTransportControlsDisplayUpdater_iface;
    LONG ref;

    MediaPlaybackType playback_type;
};

static inline struct display_updater *impl_from_ISystemMediaTransportControlsDisplayUpdater( ISystemMediaTransportControlsDisplayUpdater *iface )
{
    return CONTAINING_RECORD( iface, struct display_updater, ISystemMediaTransportControlsDisplayUpdater_iface );
}

static HRESULT WINAPI display_updater_QueryInterface( ISystemMediaTransportControlsDisplayUpdater *iface, REFIID iid, void **out )
{
    struct display_updater *impl = impl_from_ISystemMediaTransportControlsDisplayUpdater( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ISystemMediaTransportControlsDisplayUpdater ))
    {
        *out = &impl->ISystemMediaTransportControlsDisplayUpdater_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI display_updater_AddRef( ISystemMediaTransportControlsDisplayUpdater *iface )
{
    struct display_updater *impl = impl_from_ISystemMediaTransportControlsDisplayUpdater( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI display_updater_Release( ISystemMediaTransportControlsDisplayUpdater *iface )
{
    struct display_updater *impl = impl_from_ISystemMediaTransportControlsDisplayUpdater( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI display_updater_GetIids( ISystemMediaTransportControlsDisplayUpdater *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_GetRuntimeClassName( ISystemMediaTransportControlsDisplayUpdater *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_GetTrustLevel( ISystemMediaTransportControlsDisplayUpdater *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_get_Type( ISystemMediaTransportControlsDisplayUpdater *iface, MediaPlaybackType *value )
{
    struct display_updater *impl = impl_from_ISystemMediaTransportControlsDisplayUpdater( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->playback_type;
    return S_OK;
}

static HRESULT WINAPI display_updater_put_Type( ISystemMediaTransportControlsDisplayUpdater *iface, MediaPlaybackType value )
{
    struct display_updater *impl = impl_from_ISystemMediaTransportControlsDisplayUpdater( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    if (value < MediaPlaybackType_Unknown || value > MediaPlaybackType_Image) return E_INVALIDARG;
    impl->playback_type = value;
    return S_OK;
}

static HRESULT WINAPI display_updater_get_AppMediaId( ISystemMediaTransportControlsDisplayUpdater *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_put_AppMediaId( ISystemMediaTransportControlsDisplayUpdater *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring( value ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_get_Thumbnail( ISystemMediaTransportControlsDisplayUpdater *iface, __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStreamReference **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_put_Thumbnail( ISystemMediaTransportControlsDisplayUpdater *iface, __x_ABI_CWindows_CStorage_CStreams_CIRandomAccessStreamReference *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_get_MusicProperties( ISystemMediaTransportControlsDisplayUpdater *iface, IMusicDisplayProperties **value )
{
    struct music_properties *impl;

    TRACE( "iface %p, value %p\n", iface, value );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IMusicDisplayProperties_iface.lpVtbl = &music_properties_vtbl;
    impl->IMusicDisplayProperties2_iface.lpVtbl = &music_properties2_vtbl;
    impl->ref = 1;

    *value = &impl->IMusicDisplayProperties_iface;
    TRACE( "created IMusicDisplayProperties %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI display_updater_get_VideoProperties( ISystemMediaTransportControlsDisplayUpdater *iface, __x_ABI_CWindows_CMedia_CIVideoDisplayProperties **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_get_ImageProperties( ISystemMediaTransportControlsDisplayUpdater *iface, __x_ABI_CWindows_CMedia_CIImageDisplayProperties **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_CopyFromFileAsync( ISystemMediaTransportControlsDisplayUpdater *iface, MediaPlaybackType type, IStorageFile *source,
    IAsyncOperation_boolean **operation )
{
    FIXME( "iface %p, type %d, source %p, operation %p stub!\n", iface, type, source, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI display_updater_ClearAll( ISystemMediaTransportControlsDisplayUpdater *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return S_OK;
}

static HRESULT WINAPI display_updater_Update( ISystemMediaTransportControlsDisplayUpdater *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return S_OK;
}

static const struct ISystemMediaTransportControlsDisplayUpdaterVtbl display_updater_vtbl =
{
    display_updater_QueryInterface,
    display_updater_AddRef,
    display_updater_Release,
    /* IInspectable methods */
    display_updater_GetIids,
    display_updater_GetRuntimeClassName,
    display_updater_GetTrustLevel,
    /* ISystemMediaTransportControlsDisplayUpdater methods */
    display_updater_get_Type,
    display_updater_put_Type,
    display_updater_get_AppMediaId,
    display_updater_put_AppMediaId,
    display_updater_get_Thumbnail,
    display_updater_put_Thumbnail,
    display_updater_get_MusicProperties,
    display_updater_get_VideoProperties,
    display_updater_get_ImageProperties,
    display_updater_CopyFromFileAsync,
    display_updater_ClearAll,
    display_updater_Update,
};

struct media_control
{
    ISystemMediaTransportControls ISystemMediaTransportControls_iface;
    LONG ref;

    HWND window;
    MediaPlaybackStatus media_playback_status;
    boolean is_play_enabled;
    boolean is_stop_enabled;
    boolean is_pause_enabled;
    boolean is_previous_enabled;
    boolean is_next_enabled;
    boolean is_enabled;
};

static inline struct media_control *impl_from_ISystemMediaTransportControls( ISystemMediaTransportControls *iface )
{
    return CONTAINING_RECORD( iface, struct media_control, ISystemMediaTransportControls_iface );
}

static HRESULT WINAPI media_control_QueryInterface( ISystemMediaTransportControls *iface, REFIID iid, void **out )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_ISystemMediaTransportControls ))
    {
        *out = &impl->ISystemMediaTransportControls_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_control_AddRef( ISystemMediaTransportControls *iface )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI media_control_Release( ISystemMediaTransportControls *iface )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI media_control_GetIids( ISystemMediaTransportControls *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_GetRuntimeClassName( ISystemMediaTransportControls *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_GetTrustLevel( ISystemMediaTransportControls *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_get_PlaybackStatus( ISystemMediaTransportControls *iface, MediaPlaybackStatus *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->media_playback_status;
    return S_OK;
}

static HRESULT WINAPI media_control_put_PlaybackStatus( ISystemMediaTransportControls *iface, MediaPlaybackStatus value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    if (value < MediaPlaybackStatus_Closed || value > MediaPlaybackStatus_Paused) return E_INVALIDARG;
    impl->media_playback_status = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_DisplayUpdater( ISystemMediaTransportControls *iface, ISystemMediaTransportControlsDisplayUpdater **value )
{
    struct display_updater *impl;

    FIXME( "iface %p, value %p semi-stub!\n", iface, value );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->ISystemMediaTransportControlsDisplayUpdater_iface.lpVtbl = &display_updater_vtbl;
    impl->ref = 1;

    *value = &impl->ISystemMediaTransportControlsDisplayUpdater_iface;
    TRACE( "created ISystemMediaTransportControlsDisplayUpdater %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI media_control_get_SoundLevel( ISystemMediaTransportControls *iface, SoundLevel *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_get_IsEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->is_enabled;
    return S_OK;
}

static HRESULT WINAPI media_control_put_IsEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    impl->is_enabled = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_IsPlayEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->is_play_enabled;
    return S_OK;
}

static HRESULT WINAPI media_control_put_IsPlayEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    impl->is_play_enabled = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_IsStopEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->is_stop_enabled;
    return S_OK;
}

static HRESULT WINAPI media_control_put_IsStopEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    impl->is_stop_enabled = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_IsPauseEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->is_pause_enabled;
    return S_OK;
}

static HRESULT WINAPI media_control_put_IsPauseEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    impl->is_pause_enabled = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_IsRecordEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_put_IsRecordEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_get_IsFastForwardEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_put_IsFastForwardEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_get_IsRewindEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_put_IsRewindEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_get_IsPreviousEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->is_previous_enabled;
    return S_OK;
}

static HRESULT WINAPI media_control_put_IsPreviousEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    impl->is_previous_enabled = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_IsNextEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    *value = impl->is_next_enabled;
    return S_OK;
}

static HRESULT WINAPI media_control_put_IsNextEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    struct media_control *impl = impl_from_ISystemMediaTransportControls( iface );

    TRACE( "iface %p, value %d\n", iface, value );

    impl->is_next_enabled = value;
    return S_OK;
}

static HRESULT WINAPI media_control_get_IsChannelUpEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_put_IsChannelUpEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_get_IsChannelDownEnabled( ISystemMediaTransportControls *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_put_IsChannelDownEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_add_ButtonPressed( ISystemMediaTransportControls *iface,
    ITypedEventHandler_SystemMediaTransportControls_SystemMediaTransportControlsButtonPressedEventArgs *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    *token = dummy_token;
    return S_OK;
}

static HRESULT WINAPI media_control_remove_ButtonPressed( ISystemMediaTransportControls *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI media_control_add_PropertyChanged( ISystemMediaTransportControls *iface,
    ITypedEventHandler_SystemMediaTransportControls_SystemMediaTransportControlsPropertyChangedEventArgs *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_remove_PropertyChanged( ISystemMediaTransportControls *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
}

static const struct ISystemMediaTransportControlsVtbl media_control_vtbl =
{
    media_control_QueryInterface,
    media_control_AddRef,
    media_control_Release,
    /* IInspectable methods */
    media_control_GetIids,
    media_control_GetRuntimeClassName,
    media_control_GetTrustLevel,
    /* ISystemMediaTransportControls methods */
    media_control_get_PlaybackStatus,
    media_control_put_PlaybackStatus,
    media_control_get_DisplayUpdater,
    media_control_get_SoundLevel,
    media_control_get_IsEnabled,
    media_control_put_IsEnabled,
    media_control_get_IsPlayEnabled,
    media_control_put_IsPlayEnabled,
    media_control_get_IsStopEnabled,
    media_control_put_IsStopEnabled,
    media_control_get_IsPauseEnabled,
    media_control_put_IsPauseEnabled,
    media_control_get_IsRecordEnabled,
    media_control_put_IsRecordEnabled,
    media_control_get_IsFastForwardEnabled,
    media_control_put_IsFastForwardEnabled,
    media_control_get_IsRewindEnabled,
    media_control_put_IsRewindEnabled,
    media_control_get_IsPreviousEnabled,
    media_control_put_IsPreviousEnabled,
    media_control_get_IsNextEnabled,
    media_control_put_IsNextEnabled,
    media_control_get_IsChannelUpEnabled,
    media_control_put_IsChannelUpEnabled,
    media_control_get_IsChannelDownEnabled,
    media_control_put_IsChannelDownEnabled,
    media_control_add_ButtonPressed,
    media_control_remove_ButtonPressed,
    media_control_add_PropertyChanged,
    media_control_remove_PropertyChanged,
};

DEFINE_IINSPECTABLE( media_control_statics, ISystemMediaTransportControlsInterop, struct media_control_statics, IActivationFactory_iface )

static HRESULT WINAPI media_control_statics_GetForWindow( ISystemMediaTransportControlsInterop *iface, HWND window, REFIID riid, void **control )
{
    struct media_control *impl;
    HRESULT hr;

    TRACE( "iface %p, window %p, riid %s, control %p\n", iface, window, debugstr_guid( riid ), control );

    if (!window) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->ISystemMediaTransportControls_iface.lpVtbl = &media_control_vtbl;
    impl->ref = 2;
    impl->window = window;

    TRACE( "created ISystemMediaTransportControls %p.\n", *control );

    hr = ISystemMediaTransportControls_QueryInterface( &impl->ISystemMediaTransportControls_iface, riid, control );
    ISystemMediaTransportControls_Release( &impl->ISystemMediaTransportControls_iface );
    return hr;
}

static const struct ISystemMediaTransportControlsInteropVtbl media_control_statics_vtbl =
{
    media_control_statics_QueryInterface,
    media_control_statics_AddRef,
    media_control_statics_Release,
    /* IInspectable methods */
    media_control_statics_GetIids,
    media_control_statics_GetRuntimeClassName,
    media_control_statics_GetTrustLevel,
    /* ISystemMediaTransportControlsInterop methods */
    media_control_statics_GetForWindow,
};

static struct media_control_statics media_control_statics =
{
    {&factory_vtbl},
    {&media_control_statics_vtbl},
    1,
};

static IActivationFactory *media_control_factory = &media_control_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid( clsid ), debugstr_guid( riid ), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *buffer = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "class %s, factory %p.\n", debugstr_hstring( classid ), factory );

    *factory = NULL;

    if (!wcscmp( buffer, RuntimeClass_Windows_Media_SystemMediaTransportControls ))
        IActivationFactory_QueryInterface( media_control_factory, &IID_IActivationFactory, (void **)factory );

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
