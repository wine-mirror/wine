/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XAccessibility and XSpeechSynthesis
 *
 * Copyright 2026 Olivia Ryan
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

struct x_accessibility
{
    IXAccessibilityImpl2 IXAccessibilityImpl2_iface;
    LONG ref;
};

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_accessibility *impl_from_IXAccessibilityImpl2( IXAccessibilityImpl2 *iface )
{
    return CONTAINING_RECORD( iface, struct x_accessibility, IXAccessibilityImpl2_iface );
}

static HRESULT WINAPI x_accessibility_QueryInterface( IXAccessibilityImpl2 *iface, REFIID iid, void **out )
{
    struct x_accessibility *impl = impl_from_IXAccessibilityImpl2( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown             ) ||
        IsEqualGUID( iid, &IID_IXAccessibilityImpl  ) ||
        IsEqualGUID( iid, &IID_IXAccessibilityImpl2 ))
    {
        IXAccessibilityImpl_AddRef( *out = &impl->IXAccessibilityImpl2_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_accessibility_AddRef( IXAccessibilityImpl2 *iface )
{
    struct x_accessibility *impl = impl_from_IXAccessibilityImpl2( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_accessibility_Release( IXAccessibilityImpl2 *iface )
{
    struct x_accessibility *impl = impl_from_IXAccessibilityImpl2( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_accessibility_XClosedCaptionGetProperties( IXAccessibilityImpl2 *iface, XClosedCaptionProperties *properties )
{
    FIXME( "iface %p, properties %p stub!\n", iface, properties );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XClosedCaptionSetEnabled( IXAccessibilityImpl2 *iface, BOOLEAN enabled )
{
    FIXME( "iface %p, enabled %d stub!\n", iface, enabled );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XHighContrastGetMode( IXAccessibilityImpl2 *iface, XHighContrastMode *mode )
{
    FIXME( "iface %p, mode %p stub!\n", iface, mode );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechToTextSetPositionHint( IXAccessibilityImpl2 *iface, XSpeechToTextPositionHint position )
{
    FIXME( "iface %p, position %d stub!\n", iface, position );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechToTextSendString( IXAccessibilityImpl2 *iface, const char *speakerName, const char *content, XSpeechToTextType type )
{
    FIXME( "iface %p, speakerName %s, content %s, type %d stub!\n", iface, debugstr_a( speakerName ), debugstr_a( content ), type );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerEnumerateInstalledVoices( IXAccessibilityImpl2 *iface, void *context, XSpeechSynthesizerInstalledVoicesCallback *callback )
{
    FIXME( "iface %p, context %p, callback %p stub!\n", iface, context, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerCreate( IXAccessibilityImpl2 *iface, XSpeechSynthesizerHandle *speechSynthesizer )
{
    FIXME( "iface %p, speechSynthesizer %p stub!\n", iface, speechSynthesizer );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerCloseHandle( IXAccessibilityImpl2 *iface, XSpeechSynthesizerHandle speechSynthesizer )
{
    FIXME( "iface %p, speechSynthesizer %p stub!\n", iface, speechSynthesizer );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerSetDefaultVoice( IXAccessibilityImpl2 *iface, XSpeechSynthesizerHandle speechSynthesizer )
{
    FIXME( "iface %p, speechSynthesizer %p stub!\n", iface, speechSynthesizer );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerSetCustomVoice( IXAccessibilityImpl2 *iface, XSpeechSynthesizerHandle speechSynthesizer, const char *voiceId )
{
    FIXME( "iface %p, speechSynthesizer %p, voiceId %s stub!\n", iface, speechSynthesizer, debugstr_a( voiceId ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerCreateStreamFromText( IXAccessibilityImpl2 *iface, XSpeechSynthesizerHandle speechSynthesizer, const char *text, XSpeechSynthesizerStreamHandle *speechSynthesisStream )
{
    FIXME( "iface %p, speechSynthesizer %p, text %s, speechSynthesisStream %p stub!\n", iface, speechSynthesizer, debugstr_a( text ), speechSynthesisStream );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerCloseStreamHandle( IXAccessibilityImpl2 *iface, XSpeechSynthesizerStreamHandle speechSynthesisStream )
{
    FIXME( "iface %p, speechSynthesisStream %p stub!\n", iface, speechSynthesisStream );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerGetStreamDataSize( IXAccessibilityImpl2 *iface, XSpeechSynthesizerStreamHandle speechSynthesisStream, SIZE_T *bufferSize )
{
    FIXME( "iface %p, speechSynthesisStream %p, bufferSize %p stub!\n", iface, speechSynthesisStream, bufferSize );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerGetStreamData( IXAccessibilityImpl2 *iface, XSpeechSynthesizerStreamHandle speechSynthesisStream, SIZE_T bufferSize, void *buffer, SIZE_T *bufferUsed )
{
    FIXME( "iface %p, speechSynthesisStream %p, bufferSize %Iu, buffer %p, bufferUsed %p stub!\n", iface, speechSynthesisStream, bufferSize, buffer, bufferUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechToTextBeginHypothesisString( IXAccessibilityImpl2 *iface, const char *speakerName, const char *content, XSpeechToTextType type, UINT32 *hypothesisId )
{
    FIXME( "iface %p, speakerName %s, content %s, type %d, hypothesisId %p stub!\n", iface, debugstr_a( speakerName ), debugstr_a( content ), type, hypothesisId );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechToTextUpdateHypothesisString( IXAccessibilityImpl2 *iface, UINT32 hypothesisId, const char *content )
{
    FIXME( "iface %p, hypothesisId %u, content %s stub!\n", iface, hypothesisId, debugstr_a( content ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechToTextFinalizeHypothesisString( IXAccessibilityImpl2 *iface, UINT32 hypothesisId, const char *content )
{
    FIXME( "iface %p, hypothesisId %u, content %s stub!\n", iface, hypothesisId, debugstr_a( content ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechToTextCancelHypothesisString( IXAccessibilityImpl2 *iface, UINT32 hypothesisId )
{
    FIXME( "iface %p, hypothesisId %u stub!\n", iface, hypothesisId );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_accessibility_XSpeechSynthesizerCreateStreamFromSsml( IXAccessibilityImpl2 *iface, XSpeechSynthesizerHandle speechSynthesizer, const char *ssml, XSpeechSynthesizerStreamHandle *speechSynthesisStream )
{
    FIXME( "iface %p, speechSynthesizer %p, ssml %s, speechSynthesisStream %p stub!\n", iface, speechSynthesizer, debugstr_a( ssml ), speechSynthesisStream );
    return E_NOTIMPL;
}

static const struct IXAccessibilityImpl2Vtbl x_accessibility_vtbl =
{
    x_accessibility_QueryInterface,
    x_accessibility_AddRef,
    x_accessibility_Release,
    /* IXAccessibilityImpl/IXAccessibilityImpl2 methods */
    x_accessibility_XClosedCaptionGetProperties,
    x_accessibility_XClosedCaptionSetEnabled,
    x_accessibility_XHighContrastGetMode,
    x_accessibility_XSpeechToTextSetPositionHint,
    x_accessibility_XSpeechToTextSendString,
    x_accessibility_XSpeechSynthesizerEnumerateInstalledVoices,
    x_accessibility_XSpeechSynthesizerCreate,
    x_accessibility_XSpeechSynthesizerCloseHandle,
    x_accessibility_XSpeechSynthesizerSetDefaultVoice,
    x_accessibility_XSpeechSynthesizerSetCustomVoice,
    x_accessibility_XSpeechSynthesizerCreateStreamFromText,
    x_accessibility_XSpeechSynthesizerCloseStreamHandle,
    x_accessibility_XSpeechSynthesizerGetStreamDataSize,
    x_accessibility_XSpeechSynthesizerGetStreamData,
    x_accessibility_XSpeechToTextBeginHypothesisString,
    x_accessibility_XSpeechToTextUpdateHypothesisString,
    x_accessibility_XSpeechToTextFinalizeHypothesisString,
    x_accessibility_XSpeechToTextCancelHypothesisString,
    x_accessibility_XSpeechSynthesizerCreateStreamFromSsml,
};

static struct x_accessibility x_accessibility =
{
    {&x_accessibility_vtbl},
    0,
};

IXAccessibilityImpl *x_accessibility_impl = (IXAccessibilityImpl *)&x_accessibility.IXAccessibilityImpl2_iface;
