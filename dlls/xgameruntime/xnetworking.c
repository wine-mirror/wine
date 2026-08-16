/*
 * Xbox Game runtime Library
 *  GDK Component: Networking API -> XNetworking
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

struct x_networking
{
    IXNetworkingImpl2 IXNetworkingImpl2_iface;
    LONG ref;
};

static inline struct x_networking *impl_from_IXNetworkingImpl2( IXNetworkingImpl2 *iface )
{
    return CONTAINING_RECORD( iface, struct x_networking, IXNetworkingImpl2_iface );
}

static HRESULT WINAPI x_networking_QueryInterface( IXNetworkingImpl2 *iface, REFIID iid, void **out )
{
    struct x_networking *impl = impl_from_IXNetworkingImpl2( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown          ) ||
        IsEqualGUID( iid, &IID_IXNetworkingImpl  ) ||
        IsEqualGUID( iid, &IID_IXNetworkingImpl2 ))
    {
        IXNetworkingImpl_AddRef( *out = &impl->IXNetworkingImpl2_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_networking_AddRef( IXNetworkingImpl2 *iface )
{
    struct x_networking *impl = impl_from_IXNetworkingImpl2( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_networking_Release( IXNetworkingImpl2 *iface )
{
    struct x_networking *impl = impl_from_IXNetworkingImpl2( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT xnet_complete_inline( XAsyncBlock *async, HRESULT hr, const void *data, SIZE_T size )
{
    struct xasync_state *state = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*state) );
    if (!state) return E_OUTOFMEMORY;
    state->result    = hr;
    state->completed = TRUE;
    if (size && data) {
        state->result_buf = HeapAlloc( GetProcessHeap(), 0, size );
        if (!state->result_buf) { HeapFree( GetProcessHeap(), 0, state ); return E_OUTOFMEMORY; }
        memcpy( state->result_buf, data, size );
        state->result_size = size;
    }
    async->internal[0] = state;
    if (async->callback) async->callback( async );
    return S_OK;
}

static HRESULT WINAPI x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPort( IXNetworkingImpl2 *iface, UINT16 *preferredLocalUdpMultiplayerPort )
{
    TRACE( "iface %p\n", iface );
    *preferredLocalUdpMultiplayerPort = 3074;
    return S_OK;
}

static HRESULT WINAPI x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync( IXNetworkingImpl2 *iface, XAsyncBlock *asyncBlock )
{
    UINT16 port = 3074;
    TRACE( "iface %p, asyncBlock %p\n", iface, asyncBlock );
    return xnet_complete_inline( asyncBlock, S_OK, &port, sizeof(port) );
}

static HRESULT WINAPI x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult( IXNetworkingImpl2 *iface, XAsyncBlock *asyncBlock, UINT16 *preferredLocalUdpMultiplayerPort )
{
    TRACE( "iface %p, asyncBlock %p\n", iface, asyncBlock );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, asyncBlock, NULL, sizeof(*preferredLocalUdpMultiplayerPort), preferredLocalUdpMultiplayerPort, NULL );
}

static HRESULT WINAPI x_networking_XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged( IXNetworkingImpl2 *iface, XTaskQueueHandle queue, void *context, XNetworkingPreferredLocalUdpMultiplayerPortChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, token %p\n", iface, queue, token );
    token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_networking_XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged( IXNetworkingImpl2 *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p\n", iface );
    return TRUE;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlAsync( IXNetworkingImpl2 *iface, const char *url, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, url %p, asyncBlock %p stub!\n", iface, url, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResultSize( IXNetworkingImpl2 *iface, XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResult( IXNetworkingImpl2 *iface, XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %Iu, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlUtf16Async( IXNetworkingImpl2 *iface, const WCHAR *url, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, url %p, asyncBlock %p stub!\n", iface, url, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize( IXNetworkingImpl2 *iface, XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult( IXNetworkingImpl2 *iface, XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %Iu, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingVerifyServerCertificate( IXNetworkingImpl2 *iface, void *requestHandle, const XNetworkingSecurityInformation *securityInformation )
{
    FIXME( "iface %p, requestHandle %p, securityInformation %p stub!\n", iface, requestHandle, securityInformation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingGetConnectivityHint( IXNetworkingImpl2 *iface, XNetworkingConnectivityHint *connectivityHint )
{
    TRACE( "iface %p, connectivityHint %p\n", iface, connectivityHint );
    if (!connectivityHint) return E_POINTER;
    memset( connectivityHint, 0, sizeof(*connectivityHint) );
    connectivityHint->connectivityLevel    = XNetworkingConnectivityLevelHint_InternetAccess;
    connectivityHint->connectivityCost     = XNetworkingConnectivityCostHint_Unrestricted;
    return S_OK;
}

static HRESULT WINAPI x_networking_XNetworkingRegisterConnectivityHintChanged( IXNetworkingImpl2 *iface, XTaskQueueHandle queue, void *context, XNetworkingConnectivityHintChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, token %p\n", iface, queue, token );
    token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_networking_XNetworkingUnregisterConnectivityHintChanged( IXNetworkingImpl2 *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p\n", iface );
    return TRUE;
}

static HRESULT WINAPI x_networking_XNetworkingQueryConfigurationSetting( IXNetworkingImpl2 *iface, XNetworkingConfigurationSetting configurationSetting, UINT64 *value )
{
    TRACE( "iface %p, configurationSetting %d, value %p\n", iface, configurationSetting, value );
    if (value) *value = 0;
    return S_OK;
}

static HRESULT WINAPI x_networking_XNetworkingSetConfigurationSetting( IXNetworkingImpl2 *iface, XNetworkingConfigurationSetting configurationParameter, UINT64 value )
{
    TRACE( "iface %p, configurationParameter %d, value %llu\n", iface, configurationParameter, value );
    return S_OK;
}

static HRESULT WINAPI x_networking_XNetworkingQueryStatistics( IXNetworkingImpl2 *iface, XNetworkingStatisticsType statisticsType, XNetworkingStatisticsBuffer *statisticsBuffer )
{
    TRACE( "iface %p, statisticsType %d, statisticsBuffer %p\n", iface, statisticsType, statisticsBuffer );
    if (statisticsBuffer) memset( statisticsBuffer, 0, sizeof(*statisticsBuffer) );
    return S_OK;
}

static const struct IXNetworkingImpl2Vtbl x_networking_vtbl =
{
    x_networking_QueryInterface,
    x_networking_AddRef,
    x_networking_Release,
    /* IXNetworkingImpl methods */
    x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPort,
    x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync,
    x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult,
    x_networking_XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged,
    x_networking_XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged,
    x_networking_XNetworkingQuerySecurityInformationForUrlAsync,
    x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResultSize,
    x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResult,
    x_networking_XNetworkingQuerySecurityInformationForUrlUtf16Async,
    x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize,
    x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult,
    x_networking_XNetworkingVerifyServerCertificate,
    x_networking_XNetworkingGetConnectivityHint,
    x_networking_XNetworkingRegisterConnectivityHintChanged,
    x_networking_XNetworkingUnregisterConnectivityHintChanged,
    /* IXNetworkingImpl2 methods */
    x_networking_XNetworkingQueryConfigurationSetting,
    x_networking_XNetworkingSetConfigurationSetting,
    x_networking_XNetworkingQueryStatistics,
};

static struct x_networking x_networking =
{
    {&x_networking_vtbl},
    0,
};

IXNetworkingImpl *x_networking_impl = (IXNetworkingImpl *)&x_networking.IXNetworkingImpl2_iface;
