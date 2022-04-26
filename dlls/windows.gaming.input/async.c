/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2022 Bernhard Kölbl for CodeWeavers
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(input);

#define Closed 4
#define HANDLER_NOT_SET ((void *)~(ULONG_PTR)0)

struct async_bool
{
    IAsyncOperation_boolean IAsyncOperation_boolean_iface;
    IAsyncInfo IAsyncInfo_iface;
    LONG ref;

    IAsyncOperationCompletedHandler_boolean *handler;
    BOOLEAN result;

    async_operation_boolean_callback callback;
    TP_WORK *async_run_work;
    IInspectable *invoker;

    CRITICAL_SECTION cs;
    AsyncStatus status;
    HRESULT hr;
};

DEFINE_IINSPECTABLE( async_info, IAsyncInfo, struct async_bool, IAsyncOperation_boolean_iface )

static HRESULT WINAPI async_info_get_Id( IAsyncInfo *iface, UINT32 *id )
{
    struct async_bool *impl = impl_from_IAsyncInfo( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p, id %p.\n", iface, id );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    *id = 1;
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_info_get_Status( IAsyncInfo *iface, AsyncStatus *status )
{
    struct async_bool *impl = impl_from_IAsyncInfo( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p, status %p.\n", iface, status );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    *status = impl->status;
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_info_get_ErrorCode( IAsyncInfo *iface, HRESULT *error_code )
{
    struct async_bool *impl = impl_from_IAsyncInfo( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p, error_code %p.\n", iface, error_code );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) *error_code = hr = E_ILLEGAL_METHOD_CALL;
    else *error_code = impl->hr;
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_info_Cancel( IAsyncInfo *iface )
{
    struct async_bool *impl = impl_from_IAsyncInfo( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->status == Started) impl->status = Canceled;
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_info_Close( IAsyncInfo *iface )
{
    struct async_bool *impl = impl_from_IAsyncInfo( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Started)
        hr = E_ILLEGAL_STATE_CHANGE;
    else if (impl->status != Closed)
    {
        CloseThreadpoolWork( impl->async_run_work );
        impl->async_run_work = NULL;
        impl->status = Closed;
    }
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static const struct IAsyncInfoVtbl async_info_vtbl =
{
    /* IUnknown methods */
    async_info_QueryInterface,
    async_info_AddRef,
    async_info_Release,
    /* IInspectable methods */
    async_info_GetIids,
    async_info_GetRuntimeClassName,
    async_info_GetTrustLevel,
    /* IAsyncInfo */
    async_info_get_Id,
    async_info_get_Status,
    async_info_get_ErrorCode,
    async_info_Cancel,
    async_info_Close,
};

static inline struct async_bool *impl_from_IAsyncOperation_boolean( IAsyncOperation_boolean *iface )
{
    return CONTAINING_RECORD( iface, struct async_bool, IAsyncOperation_boolean_iface );
}

static HRESULT WINAPI async_bool_QueryInterface( IAsyncOperation_boolean *iface, REFIID iid, void **out )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperation_boolean ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncOperation_boolean_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IAsyncInfo ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncInfo_iface) );
        return S_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_bool_AddRef( IAsyncOperation_boolean *iface )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI async_bool_Release( IAsyncOperation_boolean *iface )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );

    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        IAsyncInfo_Close( &impl->IAsyncInfo_iface );
        if (impl->handler && impl->handler != HANDLER_NOT_SET) IAsyncOperationCompletedHandler_boolean_Release( impl->handler );
        IInspectable_Release( impl->invoker );
        DeleteCriticalSection( &impl->cs );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI async_bool_GetIids( IAsyncOperation_boolean *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_bool_GetRuntimeClassName( IAsyncOperation_boolean *iface, HSTRING *class_name )
{
    return WindowsCreateString( L"Windows.Foundation.IAsyncOperation`1<Boolean>",
                                ARRAY_SIZE(L"Windows.Foundation.IAsyncOperation`1<Boolean>"),
                                class_name );
}

static HRESULT WINAPI async_bool_GetTrustLevel( IAsyncOperation_boolean *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_bool_put_Completed( IAsyncOperation_boolean *iface, IAsyncOperationCompletedHandler_boolean *handler )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p, handler %p.\n", iface, handler );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->handler != HANDLER_NOT_SET) hr = E_ILLEGAL_DELEGATE_ASSIGNMENT;
    else if ((impl->handler = handler))
    {
        IAsyncOperationCompletedHandler_boolean_AddRef( impl->handler );

        if (impl->status > Started)
        {
            IAsyncOperation_boolean *operation = &impl->IAsyncOperation_boolean_iface;
            AsyncStatus status = impl->status;
            impl->handler = NULL; /* Prevent concurrent invoke. */
            LeaveCriticalSection( &impl->cs );

            IAsyncOperationCompletedHandler_boolean_Invoke( handler, operation, status );
            IAsyncOperationCompletedHandler_boolean_Release( handler );

            return S_OK;
        }
    }
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_bool_get_Completed( IAsyncOperation_boolean *iface, IAsyncOperationCompletedHandler_boolean **handler )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    HRESULT hr = S_OK;

    FIXME( "iface %p, handler %p semi stub!\n", iface, handler );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    *handler = (impl->handler != HANDLER_NOT_SET) ? impl->handler : NULL;
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_bool_GetResults( IAsyncOperation_boolean *iface, BOOLEAN *results )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    HRESULT hr;

    TRACE( "iface %p, results %p.\n", iface, results );

    EnterCriticalSection( &impl->cs );
    if (impl->status != Completed && impl->status != Error) hr = E_ILLEGAL_METHOD_CALL;
    else
    {
        *results = impl->result;
        hr = impl->hr;
    }
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static const struct IAsyncOperation_booleanVtbl async_bool_vtbl =
{
    /* IUnknown methods */
    async_bool_QueryInterface,
    async_bool_AddRef,
    async_bool_Release,
    /* IInspectable methods */
    async_bool_GetIids,
    async_bool_GetRuntimeClassName,
    async_bool_GetTrustLevel,
    /* IAsyncOperation<boolean> */
    async_bool_put_Completed,
    async_bool_get_Completed,
    async_bool_GetResults,
};

static void CALLBACK async_run_cb( TP_CALLBACK_INSTANCE *instance, void *data, TP_WORK *work )
{
    IAsyncOperation_boolean *operation = data;
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( operation );
    BOOLEAN result = FALSE;
    HRESULT hr;

    hr = impl->callback( impl->invoker, &result );

    EnterCriticalSection( &impl->cs );
    if (impl->status != Closed) impl->status = FAILED(hr) ? Error : Completed;
    impl->result = result;
    impl->hr = hr;

    if (impl->handler != NULL && impl->handler != HANDLER_NOT_SET)
    {
        IAsyncOperationCompletedHandler_boolean *handler = impl->handler;
        AsyncStatus status = impl->status;
        impl->handler = NULL; /* Prevent concurrent invoke. */
        LeaveCriticalSection( &impl->cs );

        IAsyncOperationCompletedHandler_boolean_Invoke( handler, operation, status );
        IAsyncOperationCompletedHandler_boolean_Release( handler );
    }
    else LeaveCriticalSection( &impl->cs );

    IAsyncOperation_boolean_Release( operation );
}

HRESULT async_operation_boolean_create( IInspectable *invoker, async_operation_boolean_callback callback,
                                        IAsyncOperation_boolean **out )
{
    struct async_bool *impl;

    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAsyncOperation_boolean_iface.lpVtbl = &async_bool_vtbl;
    impl->IAsyncInfo_iface.lpVtbl = &async_info_vtbl;
    impl->ref = 1;

    impl->handler = HANDLER_NOT_SET;
    impl->callback = callback;
    impl->status = Started;

    if (!(impl->async_run_work = CreateThreadpoolWork( async_run_cb, &impl->IAsyncOperation_boolean_iface, NULL )))
    {
        free( impl );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    IInspectable_AddRef( (impl->invoker = invoker) );
    InitializeCriticalSection( &impl->cs );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": async_operation.cs" );

    /* keep the async alive in the callback */
    IAsyncOperation_boolean_AddRef( &impl->IAsyncOperation_boolean_iface );
    SubmitThreadpoolWork( impl->async_run_work );

    *out = &impl->IAsyncOperation_boolean_iface;
    TRACE( "created IAsyncOperation_boolean %p\n", *out );
    return S_OK;
}
