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
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static void WINAPI x_game_streaming_XGameStreamingUninitialize( IXGameStreamingImpl3 *iface )
{
    FIXME( "iface %p stub!\n", iface );
}

static BOOLEAN WINAPI x_game_streaming_XGameStreamingIsStreaming( IXGameStreamingImpl3 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return FALSE;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingRegisterClientPropertiesChanged( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, XTaskQueueHandle queue, void *context, XGameStreamingClientPropertiesChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, client %llu, queue %p, context %p, callback %p, token %p stub!\n", iface, client, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_game_streaming_XGameStreamingUnregisterClientPropertiesChanged( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, client %llu, token %p, wait %d stub!\n", iface, client, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetStreamPhysicalDimensions( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, UINT32 *horizontalMm, UINT32 *verticalMm )
{
    FIXME( "iface %p, client %llu, horizontalMm %p, verticalMm %p stub!\n", iface, client, horizontalMm, verticalMm );
    return E_NOTIMPL;
}

static UINT32 WINAPI x_game_streaming_XGameStreamingGetClientCount( IXGameStreamingImpl3 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return 0;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetClients( IXGameStreamingImpl3 *iface, UINT32 clientCount, XGameStreamingClientId *clients, UINT32 *clientsUsed )
{
    FIXME( "iface %p, clientCount %u, clients %p, clientsUsed %p stub!\n", iface, clientCount, clients, clientsUsed );
    return E_NOTIMPL;
}

static XGameStreamingConnectionState WINAPI x_game_streaming_XGameStreamingGetConnectionState( IXGameStreamingImpl3 *iface, XGameStreamingClientId client )
{
    FIXME( "iface %p, client %llu stub!\n", iface, client );
    return XGameStreamingConnectionState_Disconnected;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingRegisterConnectionStateChanged( IXGameStreamingImpl3 *iface, XTaskQueueHandle queue, void *context, XGameStreamingConnectionStateChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_game_streaming_XGameStreamingUnregisterConnectionStateChanged( IXGameStreamingImpl3 *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetStreamAddedLatency( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, UINT32 *averageInputLatencyUs, UINT32 *averageOutputLatencyUs, UINT32 *standardDeviationUs )
{
    FIXME( "iface %p, client %llu, averageInputLatencyUs %p, averageOutputLatencyUs %p, standardDeviationUs %p stub!\n", iface, client, averageInputLatencyUs, averageOutputLatencyUs, standardDeviationUs );
    return E_NOTIMPL;
}

static SIZE_T WINAPI x_game_streaming_XGameStreamingGetServerLocationNameSize( IXGameStreamingImpl3 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return 0;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetServerLocationName( IXGameStreamingImpl3 *iface, SIZE_T serverLocationNameSize, char *serverLocationName )
{
    FIXME( "iface %p, serverLocationNameSize %Iu, serverLocationName %p stub!\n", iface, serverLocationNameSize, serverLocationName );
    return E_NOTIMPL;
}

static void WINAPI x_game_streaming_XGameStreamingHideTouchControls( IXGameStreamingImpl3 *iface )
{
    FIXME( "iface %p stub!\n", iface );
}

static void WINAPI x_game_streaming_XGameStreamingShowTouchControlLayout( IXGameStreamingImpl3 *iface, const char *layout )
{
    FIXME( "iface %p, layout %s stub!\n", iface, debugstr_a( layout ) );
}

static void WINAPI x_game_streaming_XGameStreamingHideTouchControlsOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client )
{
    FIXME( "iface %p, client %llu stub!\n", iface, client );
}

static void WINAPI x_game_streaming_XGameStreamingShowTouchControlLayoutOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, const char *layout )
{
    FIXME( "iface %p, client %llu, layout %s stub!\n", iface, client, debugstr_a( layout ) );
}

static HRESULT WINAPI x_game_streaming_XGameStreamingIsTouchInputEnabled( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, BOOLEAN *touchInputEnabled )
{
    FIXME( "iface %p, client %llu, touchInputEnabled %p stub!\n", iface, client, touchInputEnabled );
    return E_NOTIMPL;
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
    FIXME( "iface %p, gamepadReading %p, gamepadPhysicality %p stub!\n", iface, gamepadReading, gamepadPhysicality );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingUpdateTouchControlsState( IXGameStreamingImpl3 *iface, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    FIXME( "iface %p, operationCount %Iu, operations %p stub!\n", iface, operationCount, operations );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingUpdateTouchControlsStateOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    FIXME( "iface %p, client %llu, operationCount %Iu, operations %p stub!\n", iface, client, operationCount, operations );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingShowTouchControlsWithStateUpdate( IXGameStreamingImpl3 *iface, const char *layout, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    FIXME( "iface %p, layout %s, operationCount %Iu, operations %p stub!\n", iface, debugstr_a( layout ), operationCount, operations );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingShowTouchControlsWithStateUpdateOnClient( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, const char *layout, SIZE_T operationCount, const XGameStreamingTouchControlsStateOperation *operations )
{
    FIXME( "iface %p, client %llu, layout %s, operationCount %Iu, operations %p stub!\n", iface, client, debugstr_a( layout ), operationCount, operations );
    return E_NOTIMPL;
}

static SIZE_T WINAPI x_game_streaming_XGameStreamingGetTouchBundleVersionNameSize( IXGameStreamingImpl3 *iface, XGameStreamingClientId client )
{
    FIXME( "iface %p, client %llu stub!\n", iface, client );
    return 0;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetTouchBundleVersion( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, XVersion *version, SIZE_T versionNameSize, char *versionName )
{
    FIXME( "iface %p, client %llu, version %p, versionNameSize %Iu, versionName %p stub!\n", iface, client, version, versionNameSize, versionName );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetClientIPAddress( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, SIZE_T ipAddressSize, char *ipAddress )
{
    FIXME( "iface %p, client %llu, ipAddressSize %Iu, ipAddress %p stub!\n", iface, client, ipAddressSize, ipAddress );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetSessionId( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, SIZE_T sessionIdSize, char *sessionId, SIZE_T *sessionIdUsed )
{
    FIXME( "iface %p, client %llu, sessionIdSize %Iu, sessionId %p, sessionIdUsed %p stub!\n", iface, client, sessionIdSize, sessionId, sessionIdUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingGetDisplayDetails( IXGameStreamingImpl3 *iface, XGameStreamingClientId client, UINT32 maxSupportedPixels, float widestSupportedAspectRatio, float tallestSupportedAspectRatio, XGameStreamingDisplayDetails *displayDetails )
{
    FIXME( "iface %p, client %llu, maxSupportedPixels %u, widestSupportedAspectRatio %a, tallestSupportedAspectRatio %a, displayDetails %p stub!\n", iface, client, maxSupportedPixels, widestSupportedAspectRatio, tallestSupportedAspectRatio, displayDetails );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_game_streaming_XGameStreamingSetResolution( IXGameStreamingImpl3 *iface, UINT32 width, UINT32 height )
{
    FIXME( "iface %p, width %u, height %u stub!\n", iface, width, height );
    return E_NOTIMPL;
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
