/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XAsync, XTaskQueue and XThread
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

struct x_threading
{
    IXThreadingImpl IXThreadingImpl_iface;
    LONG ref;
};

static inline struct x_threading *impl_from_IXThreadingImpl( IXThreadingImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_threading, IXThreadingImpl_iface );
}

static HRESULT WINAPI x_threading_QueryInterface( IXThreadingImpl *iface, REFIID iid, void **out )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown        ) ||
        IsEqualGUID( iid, &IID_IXThreadingImpl ))
    {
        IXThreadingImpl_AddRef( *out = &impl->IXThreadingImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_threading_AddRef( IXThreadingImpl *iface )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_threading_Release( IXThreadingImpl *iface )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_threading_XAsyncGetStatus( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, BOOLEAN wait )
{
    struct xasync_state *state = (struct xasync_state *)asyncBlock->internal[0];
    TRACE( "iface %p, asyncBlock %p, wait %d\n", iface, asyncBlock, wait );
    if (!state) return E_INVALIDARG;
    if (!state->completed) return 0x8000000E; /* E_PENDING */
    return state->result;
}

static HRESULT WINAPI x_threading_XAsyncGetResultSize( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T *bufferSize )
{
    struct xasync_state *state = (struct xasync_state *)asyncBlock->internal[0];
    TRACE( "iface %p, asyncBlock %p, bufferSize %p\n", iface, asyncBlock, bufferSize );
    if (!state || !state->completed) return 0x8000000E;
    *bufferSize = state->result_size;
    return S_OK;
}

static void WINAPI x_threading_XAsyncCancel( IXThreadingImpl *iface, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, asyncBlock %p stub — treating as no-op\n", iface, asyncBlock );
}

static HRESULT WINAPI x_threading_XAsyncRun( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, XAsyncWork *work )
{
    HRESULT hr;
    TRACE( "iface %p, asyncBlock %p, work %p\n", iface, asyncBlock, work );
    hr = work( asyncBlock );
    if (asyncBlock->callback) asyncBlock->callback( asyncBlock );
    return hr;
}

static HRESULT WINAPI x_threading_XAsyncBegin( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, void *context,
    const void *identity, const char *identityName, XAsyncProvider *provider )
{
    struct xasync_state *state;
    XAsyncProviderData data;
    HRESULT hr;

    TRACE( "iface %p, asyncBlock %p, context %p, identity %p, name %s, provider %p\n",
           iface, asyncBlock, context, identity, identityName, provider );

    state = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*state) );
    if (!state) return E_OUTOFMEMORY;
    state->provider  = provider;
    state->identity  = identity;
    state->context   = context;
    state->completed = FALSE;
    asyncBlock->internal[0] = state;

    data.async      = asyncBlock;
    data.bufferSize = 0;
    data.buffer     = NULL;
    data.context    = context;

    hr = provider( XAsyncOp_Begin, &data );
    if (FAILED(hr) && hr != 0x8000000E) { /* E_PENDING is OK from Begin */
        HeapFree( GetProcessHeap(), 0, state );
        asyncBlock->internal[0] = NULL;
        return hr;
    }

    /* Synchronously run the DoWork phase; provider calls XAsyncComplete when done */
    provider( XAsyncOp_DoWork, &data );
    return S_OK;
}

static HRESULT WINAPI __PADDING__( IXThreadingImpl *iface )
{
    WARN( "iface %p padding function called!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XAsyncSchedule( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, UINT32 delayInMs )
{
    /* Treat as immediate; synchronous model ignores the delay */
    TRACE( "iface %p, asyncBlock %p, delay %u (treating as 0)\n", iface, asyncBlock, delayInMs );
    return S_OK;
}

static void WINAPI x_threading_XAsyncComplete( IXThreadingImpl *iface, XAsyncBlock *asyncBlock,
    HRESULT result, SIZE_T requiredBufferSize )
{
    struct xasync_state *state = (struct xasync_state *)asyncBlock->internal[0];
    XAsyncProviderData data;

    TRACE( "iface %p, asyncBlock %p, result %#lx, bufSize %Iu\n", iface, asyncBlock, result, requiredBufferSize );

    if (!state) return;
    state->result    = result;
    state->completed = TRUE;

    if (requiredBufferSize > 0 && state->provider) {
        state->result_buf = HeapAlloc( GetProcessHeap(), 0, requiredBufferSize );
        state->result_size = requiredBufferSize;
        if (state->result_buf) {
            data.async      = asyncBlock;
            data.bufferSize = requiredBufferSize;
            data.buffer     = state->result_buf;
            data.context    = state->context;
            state->provider( XAsyncOp_GetResult, &data );
        }
    }

    if (asyncBlock->callback) asyncBlock->callback( asyncBlock );

    if (state->provider) {
        data.async = asyncBlock; data.bufferSize = 0; data.buffer = NULL; data.context = state->context;
        state->provider( XAsyncOp_Cleanup, &data );
    }
}

static HRESULT WINAPI x_threading_XAsyncGetResult( IXThreadingImpl *iface, XAsyncBlock *asyncBlock,
    const void *identity, SIZE_T bufferSize, void *buffer, SIZE_T *bufferUsed )
{
    struct xasync_state *state = (struct xasync_state *)asyncBlock->internal[0];
    SIZE_T copy;

    TRACE( "iface %p, asyncBlock %p, identity %p, bufferSize %Iu, buffer %p\n",
           iface, asyncBlock, identity, bufferSize, buffer );

    if (!state) return E_INVALIDARG;
    if (!state->completed) return 0x8000000E;
    if (FAILED(state->result)) {
        HRESULT hr = state->result;
        HeapFree( GetProcessHeap(), 0, state->result_buf );
        HeapFree( GetProcessHeap(), 0, state );
        asyncBlock->internal[0] = NULL;
        return hr;
    }

    copy = state->result_size < bufferSize ? state->result_size : bufferSize;
    if (buffer && state->result_buf && copy) memcpy( buffer, state->result_buf, copy );
    if (bufferUsed) *bufferUsed = state->result_size;

    HeapFree( GetProcessHeap(), 0, state->result_buf );
    HeapFree( GetProcessHeap(), 0, state );
    asyncBlock->internal[0] = NULL;
    return S_OK;
}

static HRESULT WINAPI x_threading_XTaskQueueCreate( IXThreadingImpl *iface, XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle *queue )
{
    TRACE( "iface %p, workMode %d, completionMode %d, queue %p\n", iface, workDispatchMode, completionDispatchMode, queue );
    /* Return a non-NULL sentinel; we don't implement real task queues — async ops complete inline */
    *queue = (XTaskQueueHandle)(ULONG_PTR)0x1;
    return S_OK;
}

static HRESULT WINAPI x_threading_XTaskQueueCreateComposite( IXThreadingImpl *iface, XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort, XTaskQueueHandle *queue )
{
    FIXME( "iface %p, workPort %p, completionPort %p, queue %p stub!\n", iface, workPort, completionPort, queue );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueGetPort( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueuePort port, XTaskQueuePortHandle *portHandle )
{
    FIXME( "iface %p, queue %p, port %d, portHandle %p stub!\n", iface, queue, port, portHandle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueDuplicateHandle( IXThreadingImpl *iface, XTaskQueueHandle queueHandle, XTaskQueueHandle *duplicatedHandle )
{
    FIXME( "iface %p, queueHandle %p, duplicatedHandle %p stub!\n", iface, queueHandle, duplicatedHandle );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_threading_XTaskQueueDispatch( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueuePort port, UINT32 timeoutInMs )
{
    TRACE( "iface %p, queue %p, port %d, timeout %u — inline model, nothing queued\n", iface, queue, port, timeoutInMs );
    return FALSE; /* no callbacks pending */
}

static void WINAPI x_threading_XTaskQueueCloseHandle( IXThreadingImpl *iface, XTaskQueueHandle queue )
{
    TRACE( "iface %p, queue %p\n", iface, queue );
    /* sentinel handles are not freed */
}

static HRESULT WINAPI x_threading_XTaskQueueSubmitCallback( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueuePort port, void *callbackContext, XTaskQueueCallback *callback )
{
    TRACE( "iface %p, queue %p, port %d, ctx %p, cb %p — firing inline\n", iface, queue, port, callbackContext, callback );
    if (callback) callback( callbackContext, FALSE );
    return S_OK;
}

static HRESULT WINAPI x_threading_XTaskQueueSubmitDelayedCallback( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueuePort port, UINT32 delayMs, void *callbackContext, XTaskQueueCallback *callback )
{
    FIXME( "iface %p, queue %p, port %d, delayMs %d, callbackContext %p, callback %p stub!\n", iface, queue, port, delayMs, callbackContext, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueRegisterWaiter( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueuePort port, HANDLE waitHandle, void *callbackContext, XTaskQueueCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, port %d, waitHandle %p, callbackContext %p, callback %p, token %p stub!\n", iface, queue, port, waitHandle, callbackContext, callback, token );
    return E_NOTIMPL;
}

static void WINAPI x_threading_XTaskQueueUnregisterWaiter( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueueRegistrationToken token )
{
    FIXME( "iface %p, queue %p, token %p stub!\n", iface, queue, &token );
}

static HRESULT WINAPI x_threading_XTaskQueueTerminate( IXThreadingImpl *iface, XTaskQueueHandle queue, BOOLEAN wait, void *callbackContext, XTaskQueueTerminatedCallback *callback )
{
    FIXME( "iface %p, queue %p, wait %d, callbackContext %p, callback %p stub!\n", iface, queue, wait, callbackContext, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueRegisterMonitor( IXThreadingImpl *iface, XTaskQueueHandle queue, void *callbackContext, XTaskQueueMonitorCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, callbackContext %p, callback %p, token %p stub!\n", iface, queue, callbackContext, callback, token );
    return E_NOTIMPL;
}

static void WINAPI x_threading_XTaskQueueUnregisterMonitor( IXThreadingImpl *iface, XTaskQueueHandle queue, XTaskQueueRegistrationToken token )
{
    FIXME( "iface %p, queue %p, token %p stub!\n", iface, queue, &token );
}

static BOOLEAN WINAPI x_threading_XTaskQueueGetCurrentProcessTaskQueue( IXThreadingImpl *iface, XTaskQueueHandle *queue )
{
    FIXME( "iface %p, queue %p stub!\n", iface, queue );
    return FALSE;
}

static void WINAPI x_threading_XTaskQueueSetCurrentProcessTaskQueue( IXThreadingImpl *iface, XTaskQueueHandle queue )
{
    FIXME( "iface %p, queue %p stub!\n", iface, queue );
}

static HRESULT WINAPI x_threading_XThreadSetTimeSensitive( IXThreadingImpl *iface, BOOLEAN isTimeSensitiveThread )
{
    TRACE( "iface %p, isTimeSensitiveThread %d.\n", iface, isTimeSensitiveThread );
    if (!TlsSetValue( tlsIndex, (void *)(UINT_PTR)isTimeSensitiveThread )) return HRESULT_FROM_WIN32( GetLastError() );
    return S_OK;
}

static void WINAPI x_threading_XThreadAssertNotTimeSensitive( IXThreadingImpl *iface )
{
    TRACE( "iface %p.\n", iface );
    if (TlsGetValue( tlsIndex )) DebugBreak();
}

static BOOLEAN WINAPI x_threading_XThreadIsTimeSensitive( IXThreadingImpl *iface )
{
    TRACE( "iface %p.\n", iface );
    return TlsGetValue( tlsIndex ) ? 1 : 0;
}

static const struct IXThreadingImplVtbl x_threading_vtbl =
{
    x_threading_QueryInterface,
    x_threading_AddRef,
    x_threading_Release,
    /* IXThreadingImpl methods */
    x_threading_XAsyncGetStatus,
    x_threading_XAsyncGetResultSize,
    x_threading_XAsyncCancel,
    x_threading_XAsyncRun,
    x_threading_XAsyncBegin,
    __PADDING__,
    x_threading_XAsyncSchedule,
    x_threading_XAsyncComplete,
    x_threading_XAsyncGetResult,
    x_threading_XTaskQueueCreate,
    x_threading_XTaskQueueCreateComposite,
    x_threading_XTaskQueueGetPort,
    x_threading_XTaskQueueDuplicateHandle,
    x_threading_XTaskQueueDispatch,
    x_threading_XTaskQueueCloseHandle,
    x_threading_XTaskQueueSubmitCallback,
    x_threading_XTaskQueueSubmitDelayedCallback,
    x_threading_XTaskQueueRegisterWaiter,
    x_threading_XTaskQueueUnregisterWaiter,
    x_threading_XTaskQueueTerminate,
    x_threading_XTaskQueueRegisterMonitor,
    x_threading_XTaskQueueUnregisterMonitor,
    x_threading_XTaskQueueGetCurrentProcessTaskQueue,
    x_threading_XTaskQueueSetCurrentProcessTaskQueue,
    x_threading_XThreadSetTimeSensitive,
    __PADDING__,
    x_threading_XThreadAssertNotTimeSensitive,
    x_threading_XThreadIsTimeSensitive
};

static struct x_threading x_threading =
{
    {&x_threading_vtbl},
    0,
};

IXThreadingImpl *x_threading_impl = &x_threading.IXThreadingImpl_iface;
