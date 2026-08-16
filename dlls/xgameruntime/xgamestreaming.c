/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGameStreaming
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

struct x_game_streaming
{
    IXGameStreamingImpl3 IXGameStreamingImpl3_iface;
    LONG ref;
};

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_game_streaming *impl_from_IXGameStreamingImpl3( IXGameStreamingImpl3 *iface )
{
    return CONTAINING_RECORD( iface, struct x_game_streaming, IXGameStreamingImpl3_iface );
}

static HRESULT WINAPI x_game_streaming_QueryInterface( IXGameStreamingImpl3 *iface, REFIID iid, void **out )
{
    struct x_game_streaming *impl = impl_from_IXGameStreamingImpl3( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown             ) ||
        IsEqualGUID( iid, &IID_IXGameStreamingImpl  ) ||
        IsEqualGUID( iid, &IID_IXGameStreamingImpl2 ) ||
        IsEqualGUID( iid, &IID_IXGameStreamingImpl3 ))
    {
        IXGameStreamingImpl3_AddRef( *out = &impl->IXGameStreamingImpl3_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_game_streaming_AddRef( IXGameStreamingImpl3 *iface )
{
    struct x_game_streaming *impl = impl_from_IXGameStreamingImpl3( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_game_streaming_Release( IXGameStreamingImpl3 *iface )
{
    struct x_game_streaming *impl = impl_from_IXGameStreamingImpl3( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingInitialize( IXGameStreamingImpl3 *iface )
{
    TRACE( "iface %p\n", iface );
    return S_OK;
}

static void WINAPI x_game_streaming_XGameStreamingUninitialize( IXGameStreamingImpl3 *iface )
{
    TRACE( "iface %p\n", iface );
}

static BOOLEAN WINAPI x_game_streaming_XGameStreamingIsStreaming( IXGameStreamingImpl3 *iface )
{
    TRACE( "iface %p\n", iface );
    return FALSE;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingRegisterClientPropertiesChanged( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, XTaskQueueHandle queue, void *context, XGameStreamingClientPropertiesChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, client %llu, queue %p, context %p, callback %p, token %p\n", iface, client, queue, context, callback, token );
    if (token) token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_game_streaming_XGameStreamingUnregisterClientPropertiesChanged( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p, client %llu\n", iface, client );
    return TRUE;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetStreamPhysicalDimensions( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, UINT32 *horizontalMm, UINT32 *verticalMm )
{
    TRACE( "iface %p, client %llu, horizontalMm %p, verticalMm %p\n", iface, client, horizontalMm, verticalMm );
    if (horizontalMm) *horizontalMm = 0;
    if (verticalMm)   *verticalMm   = 0;
    return E_NOTIMPL;
}

static UINT32 WINAPI x_game_streaming_XGameStreamingGetClientCount( IXGameStreamingImpl3 *iface )
{
    TRACE( "iface %p\n", iface );
    return 0;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetClients( IXGameStreamingImpl3 *iface, UINT32 clientCount, XGameStreamingClientId *clients, UINT32 *clientsUsed )
{
    TRACE( "iface %p, clientCount %u, clients %p, clientsUsed %p\n", iface, clientCount, clients, clientsUsed );
    if (clientsUsed) *clientsUsed = 0;
    return S_OK;
}

static XGameStreamingConnectionState WINAPI x_game_streaming_XGameStreamingGetConnectionState( IXGameStreamingImpl3 *iface, XGameStreamingClientId client )
{
    TRACE( "iface %p, client %llu\n", iface, client );
    return XGameStreamingConnectionState_Disconnected;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingRegisterConnectionStateChanged( IXGameStreamingImpl3 *iface, XTaskQueueHandle queue, void *context, XGameStreamingConnectionStateChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, context %p, callback %p, token %p\n", iface, queue, context, callback, token );
    if (token) token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_game_streaming_XGameStreamingUnregisterConnectionStateChanged( IXGameStreamingImpl3 *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p\n", iface );
    return TRUE;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetStreamAddedLatency( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, UINT32 *averageInputLatencyUs, UINT32 *averageOutputLatencyUs, UINT32 *standardDeviationUs )
{
    TRACE( "iface %p, client %llu\n", iface, client );
    if (averageInputLatencyUs)  *averageInputLatencyUs  = 0;
    if (averageOutputLatencyUs) *averageOutputLatencyUs = 0;
    if (standardDeviationUs)    *standardDeviationUs    = 0;
    return E_NOTIMPL;
}

static SIZE_T WINAPI x_game_streaming_XGameStreamingGetServerLocationNameSize( IXGameStreamingImpl3 *iface )
{
    TRACE( "iface %p\n", iface );
    return 0;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetServerLocationName( IXGameStreamingImpl3 *iface, SIZE_T serverLocationNameSize, char *serverLocationName )
{
    TRACE( "iface %p, serverLocationNameSize %Iu, serverLocationName %p\n", iface, serverLocationNameSize, serverLocationName );
    return E_NOTIMPL;
}

static void WINAPI x_game_streaming_XGameStreamingHideTouchControls( IXGameStreamingImpl3 *iface )
{
    TRACE( "iface %p\n", iface );
}

static void WINAPI x_game_streaming_XGameStreamingShowTouchControlLayout( IXGameStreamingImpl3 *iface, const char *layout )
{
    TRACE( "iface %p, layout %s\n", iface, debugstr_a( layout ) );
}

static void WINAPI x_game_streaming_XGameStreamingHideTouchControlsOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client )
{
    TRACE( "iface %p, client %llu\n", iface, client );
}

static void WINAPI x_game_streaming_XGameStreamingShowTouchControlLayoutOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, const char *layout )
{
    TRACE( "iface %p, client %llu, layout %s\n", iface, client, debugstr_a( layout ) );
}

static HRESULT WINAPI x_game_streaming_XGameStreamingIsTouchInputEnabled( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, BOOLEAN *touchInputEnabled )
{
    TRACE( "iface %p, client %llu, touchInputEnabled %p\n", iface, client, touchInputEnabled );
    if (touchInputEnabled) *touchInputEnabled = FALSE;
    return S_OK;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetLastFrameDisplayed( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, D3D12XBOX_FRAME_PIPELINE_TOKEN *framePipelineToken )
{
    FIXME( "iface %p, client %llu, framePipelineToken %p stub!\n", iface, client, framePipelineToken );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetAssociatedFrame( IXGameStreamingImpl3 *iface, IGameInputReading *gamepadReading, D3D12XBOX_FRAME_PIPELINE_TOKEN *framePipelineToken )
{
    FIXME( "iface %p, gamepadReading %p, framePipelineToken %p stub!\n", iface, gamepadReading, framePipelineToken );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetGamepadPhysicality( IXGameStreamingImpl3 *iface, IGameInputReading *gamepadReading, XGameStreamingGamepadPhysicality *gamepadPhysicality )
{
    TRACE( "iface %p, gamepadReading %p, gamepadPhysicality %p\n", iface, gamepadReading, gamepadPhysicality );
    if (gamepadPhysicality) memset( gamepadPhysicality, 0, sizeof(*gamepadPhysicality) );
    return S_OK;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingUpdateTouchControlsState( IXGameStreamingImpl3 *iface, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    TRACE( "iface %p, operationCount %Iu, operations %p\n", iface, operationCount, operations );
    return S_OK;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingUpdateTouchControlsStateOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    TRACE( "iface %p, client %llu, operationCount %Iu, operations %p\n", iface, client, operationCount, operations );
    return S_OK;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingShowTouchControlsWithStateUpdate( IXGameStreamingImpl3 *iface, const char *layout, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    TRACE( "iface %p, layout %s, operationCount %Iu, operations %p\n", iface, debugstr_a( layout ), operationCount, operations );
    return S_OK;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingShowTouchControlsWithStateUpdateOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, const char *layout, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    TRACE( "iface %p, client %llu, layout %s, operationCount %Iu, operations %p\n", iface, client, debugstr_a( layout ), operationCount, operations );
    return S_OK;
}

static SIZE_T WINAPI x_game_streaming_XGameStreamingGetTouchBundleVersionNameSize( IXGameStreamingImpl3 *iface, XGameStreamingClientId client )
{
    TRACE( "iface %p, client %llu\n", iface, client );
    return 0;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetTouchBundleVersion( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, XVersion *version, SIZE_T versionNameSize, char *versionName )
{
    TRACE( "iface %p, client %llu, version %p, versionNameSize %Iu, versionName %p\n", iface, client, version, versionNameSize, versionName );
    if (version) memset( version, 0, sizeof(*version) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetClientIPAddress( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, SIZE_T ipAddressSize, char *ipAddress )
{
    TRACE( "iface %p, client %llu, ipAddressSize %Iu, ipAddress %p\n", iface, client, ipAddressSize, ipAddress );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetSessionId( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, SIZE_T sessionIdSize, char *sessionId, SIZE_T *sessionIdUsed )
{
    TRACE( "iface %p, client %llu, sessionIdSize %Iu, sessionId %p, sessionIdUsed %p\n", iface, client, sessionIdSize, sessionId, sessionIdUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetDisplayDetails( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, UINT32 maxSupportedPixels, float widestSupportedAspectRatio, float tallestSupportedAspectRatio, XGameStreamingDisplayDetails *displayDetails )
{
    TRACE( "iface %p, client %llu, maxSupportedPixels %u, displayDetails %p\n", iface, client, maxSupportedPixels, displayDetails );
    if (displayDetails) memset( displayDetails, 0, sizeof(*displayDetails) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingSetResolution( IXGameStreamingImpl3 *iface, UINT32 width, UINT32 height )
{
    TRACE( "iface %p, width %u, height %u\n", iface, width, height );
    return S_OK;
}

static const struct IXGameStreamingImpl3Vtbl x_game_streaming_vtbl =
{
    x_game_streaming_QueryInterface,
    x_game_streaming_AddRef,
    x_game_streaming_Release,
    /* IXGameStreamingImpl methods */
    x_game_streaming_XGameStreamingInitialize,
    x_game_streaming_XGameStreamingUninitialize,
    x_game_streaming_XGameStreamingIsStreaming,
    x_game_streaming_XGameStreamingRegisterClientPropertiesChanged,
    x_game_streaming_XGameStreamingUnregisterClientPropertiesChanged,
    x_game_streaming_XGameStreamingGetStreamPhysicalDimensions,
    x_game_streaming_XGameStreamingGetClientCount,
    x_game_streaming_XGameStreamingGetClients,
    x_game_streaming_XGameStreamingGetConnectionState,
    x_game_streaming_XGameStreamingRegisterConnectionStateChanged,
    x_game_streaming_XGameStreamingUnregisterConnectionStateChanged,
    x_game_streaming_XGameStreamingGetStreamAddedLatency,
    x_game_streaming_XGameStreamingGetServerLocationNameSize,
    x_game_streaming_XGameStreamingGetServerLocationName,
    x_game_streaming_XGameStreamingHideTouchControls,
    x_game_streaming_XGameStreamingShowTouchControlLayout,
    x_game_streaming_XGameStreamingHideTouchControlsOnClient,
    x_game_streaming_XGameStreamingShowTouchControlLayoutOnClient,
    x_game_streaming_XGameStreamingIsTouchInputEnabled,
    x_game_streaming_XGameStreamingGetLastFrameDisplayed,
    x_game_streaming_XGameStreamingGetAssociatedFrame,
    x_game_streaming_XGameStreamingGetGamepadPhysicality,
    x_game_streaming_XGameStreamingUpdateTouchControlsState,
    x_game_streaming_XGameStreamingUpdateTouchControlsStateOnClient,
    x_game_streaming_XGameStreamingShowTouchControlsWithStateUpdate,
    x_game_streaming_XGameStreamingShowTouchControlsWithStateUpdateOnClient,
    x_game_streaming_XGameStreamingGetTouchBundleVersionNameSize,
    x_game_streaming_XGameStreamingGetTouchBundleVersion,
    x_game_streaming_XGameStreamingGetClientIPAddress,
    /* IXGameStreamingImpl2 methods */
    x_game_streaming_XGameStreamingGetSessionId,
    /* IXGameStreamingImpl3 methods */
    x_game_streaming_XGameStreamingGetDisplayDetails,
    x_game_streaming_XGameStreamingSetResolution,
};

static struct x_game_streaming x_game_streaming =
{
    {&x_game_streaming_vtbl},
    0,
};

IXGameStreamingImpl *x_game_streaming_impl = (IXGameStreamingImpl *)&x_game_streaming.IXGameStreamingImpl3_iface;
