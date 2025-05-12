/* WinRT IAsync* implementation
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

#define WIDL_using_Wine_Internal
#include "private.h"
#include "initguid.h"
#include "async_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(combase);

#define Closed 4
#define HANDLER_NOT_SET ((void *)~(ULONG_PTR)0)

struct async_info
{
    IAsyncInfoImpl IAsyncInfoImpl_iface;
    IAsyncInfo IAsyncInfo_iface;
    IInspectable *IInspectable_outer;
    LONG ref;

    async_operation_callback callback;
    TP_WORK *async_run_work;
    IUnknown *invoker;
    IUnknown *param;

    CRITICAL_SECTION cs;
    IAsyncOperationCompletedHandlerImpl *handler;
    PROPVARIANT result;
    AsyncStatus status;
    HRESULT hr;
};

static inline struct async_info *impl_from_IAsyncInfoImpl( IAsyncInfoImpl *iface )
{
    return CONTAINING_RECORD( iface, struct async_info, IAsyncInfoImpl_iface );
}

static HRESULT WINAPI async_impl_QueryInterface( IAsyncInfoImpl *iface, REFIID iid, void **out )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncInfoImpl ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncInfoImpl_iface) );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IAsyncInfo ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncInfo_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_impl_AddRef( IAsyncInfoImpl *iface )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI async_impl_Release( IAsyncInfoImpl *iface )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        if (impl->handler && impl->handler != HANDLER_NOT_SET) IAsyncOperationCompletedHandlerImpl_Release( impl->handler );
        IAsyncInfo_Close( &impl->IAsyncInfo_iface );
        if (impl->param) IUnknown_Release( impl->param );
        if (impl->invoker) IUnknown_Release( impl->invoker );
        PropVariantClear( &impl->result );
        impl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &impl->cs );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI async_impl_put_Completed( IAsyncInfoImpl *iface, IAsyncOperationCompletedHandlerImpl *handler )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p, handler %p.\n", iface, handler );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->handler != HANDLER_NOT_SET) hr = E_ILLEGAL_DELEGATE_ASSIGNMENT;
    else if ((impl->handler = handler))
    {
        IAsyncOperationCompletedHandlerImpl_AddRef( impl->handler );

        if (impl->status > Started)
        {
            IInspectable *operation = impl->IInspectable_outer;
            AsyncStatus status = impl->status;
            impl->handler = NULL; /* Prevent concurrent invoke. */
            LeaveCriticalSection( &impl->cs );

            IAsyncOperationCompletedHandlerImpl_Invoke( handler, operation, status );
            IAsyncOperationCompletedHandlerImpl_Release( handler );

            return S_OK;
        }
    }
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_impl_get_Completed( IAsyncInfoImpl *iface, IAsyncOperationCompletedHandlerImpl **handler )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );
    HRESULT hr = S_OK;

    TRACE( "iface %p, handler %p.\n", iface, handler );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Closed) hr = E_ILLEGAL_METHOD_CALL;
    if (impl->handler == NULL || impl->handler == HANDLER_NOT_SET) *handler = NULL;
    else IAsyncOperationCompletedHandlerImpl_AddRef( (*handler = impl->handler) );
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_impl_get_Result( IAsyncInfoImpl *iface, PROPVARIANT *result )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );
    HRESULT hr = E_ILLEGAL_METHOD_CALL;

    TRACE( "iface %p, result %p.\n", iface, result );

    EnterCriticalSection( &impl->cs );
    if (impl->status == Completed || impl->status == Error)
    {
        PropVariantCopy( result, &impl->result );
        hr = impl->hr;
    }
    LeaveCriticalSection( &impl->cs );

    return hr;
}

static HRESULT WINAPI async_impl_Start( IAsyncInfoImpl *iface )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );

    TRACE( "iface %p.\n", iface );

    /* keep the async alive in the callback */
    IInspectable_AddRef( impl->IInspectable_outer );
    SubmitThreadpoolWork( impl->async_run_work );

    return S_OK;
}

static const struct IAsyncInfoImplVtbl async_impl_vtbl =
{
    /* IUnknown methods */
    async_impl_QueryInterface,
    async_impl_AddRef,
    async_impl_Release,
    /* IAsyncInfoImpl */
    async_impl_put_Completed,
    async_impl_get_Completed,
    async_impl_get_Result,
    async_impl_Start,
};

DEFINE_IINSPECTABLE_OUTER( async_info, IAsyncInfo, struct async_info, IInspectable_outer )

static HRESULT WINAPI async_info_get_Id( IAsyncInfo *iface, UINT32 *id )
{
    struct async_info *impl = impl_from_IAsyncInfo( iface );
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
    struct async_info *impl = impl_from_IAsyncInfo( iface );
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
    struct async_info *impl = impl_from_IAsyncInfo( iface );
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
    struct async_info *impl = impl_from_IAsyncInfo( iface );
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
    struct async_info *impl = impl_from_IAsyncInfo( iface );
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

static void CALLBACK async_info_callback( TP_CALLBACK_INSTANCE *instance, void *iface, TP_WORK *work )
{
    struct async_info *impl = impl_from_IAsyncInfoImpl( iface );
    IInspectable *operation = impl->IInspectable_outer;
    PROPVARIANT result = {0};
    HRESULT hr;

    hr = impl->callback( impl->invoker, impl->param, &result );

    EnterCriticalSection( &impl->cs );
    if (impl->status != Closed) impl->status = FAILED(hr) ? Error : Completed;
    PropVariantCopy( &impl->result, &result );
    impl->hr = hr;

    if (impl->handler != NULL && impl->handler != HANDLER_NOT_SET)
    {
        IAsyncOperationCompletedHandlerImpl *handler = impl->handler;
        AsyncStatus status = impl->status;
        impl->handler = NULL; /* Prevent concurrent invoke. */
        LeaveCriticalSection( &impl->cs );

        IAsyncOperationCompletedHandlerImpl_Invoke( handler, operation, status );
        IAsyncOperationCompletedHandlerImpl_Release( handler );
    }
    else LeaveCriticalSection( &impl->cs );

    /* release refcount acquired in Start */
    IInspectable_Release( operation );

    PropVariantClear( &result );
}

static HRESULT async_info_create( IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                  IInspectable *outer, IAsyncInfoImpl **out )
{
    struct async_info *impl;
    HRESULT hr;

    if (!(impl = calloc( 1, sizeof(struct async_info) ))) return E_OUTOFMEMORY;
    impl->IAsyncInfoImpl_iface.lpVtbl = &async_impl_vtbl;
    impl->IAsyncInfo_iface.lpVtbl = &async_info_vtbl;
    impl->IInspectable_outer = outer;
    impl->ref = 1;

    impl->callback = callback;
    impl->handler = HANDLER_NOT_SET;
    impl->status = Started;
    if (!(impl->async_run_work = CreateThreadpoolWork( async_info_callback, &impl->IAsyncInfoImpl_iface, NULL )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        free( impl );
        return hr;
    }

    if ((impl->invoker = invoker)) IUnknown_AddRef( impl->invoker );
    if ((impl->param = param)) IUnknown_AddRef( impl->param );

    InitializeCriticalSectionEx( &impl->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": async_info.cs" );

    *out = &impl->IAsyncInfoImpl_iface;
    return S_OK;
}

struct async_bool
{
    IAsyncOperation_boolean IAsyncOperation_boolean_iface;
    IAsyncInfoImpl *IAsyncInfoImpl_inner;
    LONG ref;
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

    return IAsyncInfoImpl_QueryInterface( impl->IAsyncInfoImpl_inner, iid, out );
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
        /* guard against re-entry if inner releases an outer iface */
        InterlockedIncrement( &impl->ref );
        IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
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

static HRESULT WINAPI async_bool_put_Completed( IAsyncOperation_boolean *iface, IAsyncOperationCompletedHandler_boolean *bool_handler )
{
    IAsyncOperationCompletedHandlerImpl *handler = (IAsyncOperationCompletedHandlerImpl *)bool_handler;
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_put_Completed( impl->IAsyncInfoImpl_inner, handler );
}

static HRESULT WINAPI async_bool_get_Completed( IAsyncOperation_boolean *iface, IAsyncOperationCompletedHandler_boolean **bool_handler )
{
    IAsyncOperationCompletedHandlerImpl **handler = (IAsyncOperationCompletedHandlerImpl **)bool_handler;
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_get_Completed( impl->IAsyncInfoImpl_inner, handler );
}

static HRESULT WINAPI async_bool_GetResults( IAsyncOperation_boolean *iface, BOOLEAN *results )
{
    struct async_bool *impl = impl_from_IAsyncOperation_boolean( iface );
    PROPVARIANT result = {.vt = VT_BOOL};
    HRESULT hr;

    TRACE( "iface %p, results %p.\n", iface, results );

    hr = IAsyncInfoImpl_get_Result( impl->IAsyncInfoImpl_inner, &result );

    *results = result.boolVal;
    PropVariantClear( &result );
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

HRESULT async_operation_boolean_create( IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                        IAsyncOperation_boolean **out )
{
    struct async_bool *impl;
    HRESULT hr;

    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAsyncOperation_boolean_iface.lpVtbl = &async_bool_vtbl;
    impl->ref = 1;

    if (FAILED(hr = async_info_create( invoker, param, callback, (IInspectable *)&impl->IAsyncOperation_boolean_iface, &impl->IAsyncInfoImpl_inner )) ||
        FAILED(hr = IAsyncInfoImpl_Start( impl->IAsyncInfoImpl_inner )))
    {
        if (impl->IAsyncInfoImpl_inner) IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
        free( impl );
        return hr;
    }

    *out = &impl->IAsyncOperation_boolean_iface;
    TRACE( "created IAsyncOperation_boolean %p\n", *out );
    return S_OK;
}

struct async_action
{
    IAsyncAction IAsyncAction_iface;
    IAsyncInfoImpl *IAsyncInfoImpl_inner;
    LONG ref;
};

static inline struct async_action *impl_from_IAsyncAction( IAsyncAction *iface )
{
    return CONTAINING_RECORD( iface, struct async_action, IAsyncAction_iface );
}

static HRESULT WINAPI async_action_QueryInterface( IAsyncAction *iface, REFIID iid, void **out )
{
    struct async_action *impl = impl_from_IAsyncAction( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncAction ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncAction_iface) );
        return S_OK;
    }

    return IAsyncInfoImpl_QueryInterface( impl->IAsyncInfoImpl_inner, iid, out );
}

static ULONG WINAPI async_action_AddRef( IAsyncAction *iface )
{
    struct async_action *impl = impl_from_IAsyncAction( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI async_action_Release( IAsyncAction *iface )
{
    struct async_action *impl = impl_from_IAsyncAction( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        /* guard against re-entry if inner releases an outer iface */
        InterlockedIncrement( &impl->ref );
        IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI async_action_GetIids( IAsyncAction *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_action_GetRuntimeClassName( IAsyncAction *iface, HSTRING *class_name )
{
    return WindowsCreateString( L"Windows.Foundation.IAsyncOperation`1<Boolean>",
                                ARRAY_SIZE(L"Windows.Foundation.IAsyncOperation`1<Boolean>"),
                                class_name );
}

static HRESULT WINAPI async_action_GetTrustLevel( IAsyncAction *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_action_put_Completed( IAsyncAction *iface, IAsyncActionCompletedHandler *bool_handler )
{
    IAsyncOperationCompletedHandlerImpl *handler = (IAsyncOperationCompletedHandlerImpl *)bool_handler;
    struct async_action *impl = impl_from_IAsyncAction( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_put_Completed( impl->IAsyncInfoImpl_inner, handler );
}

static HRESULT WINAPI async_action_get_Completed( IAsyncAction *iface, IAsyncActionCompletedHandler **bool_handler )
{
    IAsyncOperationCompletedHandlerImpl **handler = (IAsyncOperationCompletedHandlerImpl **)bool_handler;
    struct async_action *impl = impl_from_IAsyncAction( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_get_Completed( impl->IAsyncInfoImpl_inner, handler );
}

static HRESULT WINAPI async_action_GetResults( IAsyncAction *iface )
{
    struct async_action *impl = impl_from_IAsyncAction( iface );
    PROPVARIANT result;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    PropVariantInit( &result );
    hr = IAsyncInfoImpl_get_Result( impl->IAsyncInfoImpl_inner, &result );
    PropVariantClear( &result );
    return hr;
}

static const struct IAsyncActionVtbl async_action_vtbl =
{
    /* IUnknown methods */
    async_action_QueryInterface,
    async_action_AddRef,
    async_action_Release,
    /* IInspectable methods */
    async_action_GetIids,
    async_action_GetRuntimeClassName,
    async_action_GetTrustLevel,
    /* IAsyncOperation<boolean> */
    async_action_put_Completed,
    async_action_get_Completed,
    async_action_GetResults,
};

HRESULT async_action_create( IUnknown *invoker, async_operation_callback callback, IAsyncAction **out )
{
    struct async_action *impl;
    HRESULT hr;

    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAsyncAction_iface.lpVtbl = &async_action_vtbl;
    impl->ref = 1;

    if (FAILED(hr = async_info_create( invoker, NULL, callback, (IInspectable *)&impl->IAsyncAction_iface, &impl->IAsyncInfoImpl_inner )) ||
        FAILED(hr = IAsyncInfoImpl_Start( impl->IAsyncInfoImpl_inner )))
    {
        if (impl->IAsyncInfoImpl_inner) IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
        free( impl );
        return hr;
    }

    *out = &impl->IAsyncAction_iface;
    TRACE( "created IAsyncAction %p\n", *out );
    return S_OK;
}

struct async_inspectable
{
    IAsyncOperation_IInspectable IAsyncOperation_IInspectable_iface;
    IAsyncInfoImpl *IAsyncInfoImpl_inner;
    LONG ref;
    const GUID *iid;
};

static inline struct async_inspectable *impl_from_IAsyncOperation_IInspectable( IAsyncOperation_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct async_inspectable, IAsyncOperation_IInspectable_iface );
}

static HRESULT WINAPI async_inspectable_QueryInterface( IAsyncOperation_IInspectable *iface, REFIID iid, void **out )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, impl->iid ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncOperation_IInspectable_iface) );
        return S_OK;
    }

    return IAsyncInfoImpl_QueryInterface( impl->IAsyncInfoImpl_inner, iid, out );
}

static ULONG WINAPI async_inspectable_AddRef( IAsyncOperation_IInspectable *iface )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI async_inspectable_Release( IAsyncOperation_IInspectable *iface )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        /* guard against re-entry if inner releases an outer iface */
        InterlockedIncrement( &impl->ref );
        IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI async_inspectable_GetIids( IAsyncOperation_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_inspectable_GetRuntimeClassName( IAsyncOperation_IInspectable *iface, HSTRING *class_name )
{
    return WindowsCreateString( L"Windows.Foundation.IAsyncOperation`1<IInspectable>",
                                ARRAY_SIZE(L"Windows.Foundation.IAsyncOperation`1<IInspectable>"),
                                class_name );
}

static HRESULT WINAPI async_inspectable_GetTrustLevel( IAsyncOperation_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_inspectable_put_Completed( IAsyncOperation_IInspectable *iface, IAsyncOperationCompletedHandler_IInspectable *bool_handler )
{
    IAsyncOperationCompletedHandlerImpl *handler = (IAsyncOperationCompletedHandlerImpl *)bool_handler;
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_put_Completed( impl->IAsyncInfoImpl_inner, handler );
}

static HRESULT WINAPI async_inspectable_get_Completed( IAsyncOperation_IInspectable *iface, IAsyncOperationCompletedHandler_IInspectable **bool_handler )
{
    IAsyncOperationCompletedHandlerImpl **handler = (IAsyncOperationCompletedHandlerImpl **)bool_handler;
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_get_Completed( impl->IAsyncInfoImpl_inner, handler );
}

static HRESULT WINAPI async_inspectable_GetResults( IAsyncOperation_IInspectable *iface, IInspectable **results )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable( iface );
    PROPVARIANT result = {.vt = VT_UNKNOWN};
    HRESULT hr;

    TRACE( "iface %p, results %p.\n", iface, results );

    if (SUCCEEDED(hr = IAsyncInfoImpl_get_Result( impl->IAsyncInfoImpl_inner, &result )))
    {
        if ((*results = (IInspectable *)result.punkVal)) IInspectable_AddRef( *results );
        PropVariantClear( &result );
    }

    return hr;
}

static const struct IAsyncOperation_IInspectableVtbl async_inspectable_vtbl =
{
    /* IUnknown methods */
    async_inspectable_QueryInterface,
    async_inspectable_AddRef,
    async_inspectable_Release,
    /* IInspectable methods */
    async_inspectable_GetIids,
    async_inspectable_GetRuntimeClassName,
    async_inspectable_GetTrustLevel,
    /* IAsyncOperation<IInspectable> */
    async_inspectable_put_Completed,
    async_inspectable_get_Completed,
    async_inspectable_GetResults,
};

HRESULT async_operation_inspectable_create( const GUID *iid, IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                            IAsyncOperation_IInspectable **out )
{
    struct async_inspectable *impl;
    HRESULT hr;

    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAsyncOperation_IInspectable_iface.lpVtbl = &async_inspectable_vtbl;
    impl->ref = 1;
    impl->iid = iid;

    if (FAILED(hr = async_info_create( invoker, param, callback, (IInspectable *)&impl->IAsyncOperation_IInspectable_iface, &impl->IAsyncInfoImpl_inner )) ||
        FAILED(hr = IAsyncInfoImpl_Start( impl->IAsyncInfoImpl_inner )))
    {
        if (impl->IAsyncInfoImpl_inner) IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
        free( impl );
        return hr;
    }

    *out = &impl->IAsyncOperation_IInspectable_iface;
    TRACE( "created IAsyncOperation_IInspectable %p\n", *out );
    return S_OK;
}
