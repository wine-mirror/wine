/* WinRT Windows.Media.Transcoding.MediaTranscoder Implementation
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
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

WINE_DEFAULT_DEBUG_CHANNEL(media);

struct media_transcoder
{
    IMediaTranscoder IMediaTranscoder_iface;
    LONG ref;
};

static inline struct media_transcoder *impl_from_IMediaTranscoder( IMediaTranscoder *iface )
{
    return CONTAINING_RECORD( iface, struct media_transcoder, IMediaTranscoder_iface );
}

static HRESULT WINAPI media_transcoder_QueryInterface( IMediaTranscoder *iface, REFIID iid, void **out )
{
    struct media_transcoder *impl = impl_from_IMediaTranscoder( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IMediaTranscoder ))
    {
        *out = &impl->IMediaTranscoder_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_transcoder_AddRef( IMediaTranscoder *iface )
{
    struct media_transcoder *impl = impl_from_IMediaTranscoder( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI media_transcoder_Release( IMediaTranscoder *iface )
{
    struct media_transcoder *impl = impl_from_IMediaTranscoder( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI media_transcoder_GetIids( IMediaTranscoder *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_GetRuntimeClassName( IMediaTranscoder *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_GetTrustLevel( IMediaTranscoder *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_put_TrimStartTime( IMediaTranscoder *iface, TimeSpan value )
{
    FIXME( "iface %p, value %I64d stub!\n", iface, value.Duration );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_get_TrimStartTime( IMediaTranscoder *iface, TimeSpan *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_put_TrimStopTime( IMediaTranscoder *iface, TimeSpan value )
{
    FIXME( "iface %p, value %I64d stub!\n", iface, value.Duration );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_get_TrimStopTime( IMediaTranscoder *iface, TimeSpan *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_put_AlwaysReencode( IMediaTranscoder *iface, boolean value )
{
    FIXME( "iface %p, value %u stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_get_AlwaysReencode( IMediaTranscoder *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_put_HardwareAccelerationEnabled( IMediaTranscoder *iface, boolean value )
{
    FIXME( "iface %p, value %u stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_get_HardwareAccelerationEnabled( IMediaTranscoder *iface, boolean *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_AddAudioEffect( IMediaTranscoder *iface, HSTRING activatable_class_id )
{
    FIXME( "iface %p, activatable_class_id %s stub!\n", iface, debugstr_hstring( activatable_class_id ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_AddAudioEffectWithSettings( IMediaTranscoder *iface, HSTRING activatable_class_id,
                                                                   boolean effect_required, IPropertySet *configuration )
{
    FIXME( "iface %p, activatable_class_id %s, effect_required %u, configuration %p stub!\n",
           iface, debugstr_hstring( activatable_class_id ), effect_required, configuration );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_AddVideoEffect( IMediaTranscoder *iface, HSTRING activatable_class_id )
{
    FIXME( "iface %p, activatable_class_id %s stub!\n", iface, debugstr_hstring( activatable_class_id ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_AddVideoEffectWithSettings( IMediaTranscoder *iface, HSTRING activatable_class_id,
                                                                   boolean effect_required, IPropertySet *configuration )
{
    FIXME( "iface %p, activatable_class_id %s, effect_required %u, configuration %p stub!\n",
           iface, debugstr_hstring( activatable_class_id ), effect_required, configuration );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_ClearEffects( IMediaTranscoder *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_PrepareFileTranscodeAsync( IMediaTranscoder *iface, IStorageFile *source, IStorageFile *destination,
                                                                  IMediaEncodingProfile *profile, IAsyncOperation_PrepareTranscodeResult **operation )
{
    FIXME( "iface %p, source %p, destination %p, profile %p, operation %p stub!\n", iface, source, destination, profile, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI media_transcoder_PrepareStreamTranscodeAsync( IMediaTranscoder *iface, IRandomAccessStream *source, IRandomAccessStream *destination,
                                                                    IMediaEncodingProfile *profile, IAsyncOperation_PrepareTranscodeResult **operation )
{
    FIXME( "iface %p, source %p, destination %p, profile %p, operation %p stub!\n", iface, source, destination, profile, operation );
    return E_NOTIMPL;
}

static const struct IMediaTranscoderVtbl media_transcoder_vtbl =
{
    /* IUnknown methods */
    media_transcoder_QueryInterface,
    media_transcoder_AddRef,
    media_transcoder_Release,
    /* IInspectable methods */
    media_transcoder_GetIids,
    media_transcoder_GetRuntimeClassName,
    media_transcoder_GetTrustLevel,
    /* IMediaTranscoder methods */
    media_transcoder_put_TrimStartTime,
    media_transcoder_get_TrimStartTime,
    media_transcoder_put_TrimStopTime,
    media_transcoder_get_TrimStopTime,
    media_transcoder_put_AlwaysReencode,
    media_transcoder_get_AlwaysReencode,
    media_transcoder_put_HardwareAccelerationEnabled,
    media_transcoder_get_HardwareAccelerationEnabled,
    media_transcoder_AddAudioEffect,
    media_transcoder_AddAudioEffectWithSettings,
    media_transcoder_AddVideoEffect,
    media_transcoder_AddVideoEffectWithSettings,
    media_transcoder_ClearEffects,
    media_transcoder_PrepareFileTranscodeAsync,
    media_transcoder_PrepareStreamTranscodeAsync,
};

struct media_transcoder_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct media_transcoder_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct media_transcoder_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct media_transcoder_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct media_transcoder_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct media_transcoder_statics *impl = impl_from_IActivationFactory( iface );
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
    struct media_transcoder *impl;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof( *impl ) )))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IMediaTranscoder_iface.lpVtbl = &media_transcoder_vtbl;
    impl->ref = 1;

    *instance = (IInspectable *)&impl->IMediaTranscoder_iface;
    return S_OK;
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

static struct media_transcoder_statics media_transcoder_statics =
{
    {&factory_vtbl},
    1,
};

IActivationFactory *media_transcoder_factory = &media_transcoder_statics.IActivationFactory_iface;
