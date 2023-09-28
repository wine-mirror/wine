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

struct media_control
{
    ISystemMediaTransportControls ISystemMediaTransportControls_iface;
    LONG ref;

    HWND window;
    MediaPlaybackStatus media_playback_status;
    boolean is_play_enabled;
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

static HRESULT WINAPI media_control_get_DisplayUpdater( ISystemMediaTransportControls *iface, __x_ABI_CWindows_CMedia_CISystemMediaTransportControlsDisplayUpdater **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
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
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_put_IsStopEnabled( ISystemMediaTransportControls *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
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
    return E_NOTIMPL;
}

static HRESULT WINAPI media_control_remove_ButtonPressed( ISystemMediaTransportControls *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return E_NOTIMPL;
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
