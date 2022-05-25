/* WinRT Windows.Media.Speech implementation
 *
 * Copyright 2022 Bernhard Kölbl for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(speech);

#define Closed 4
#define HANDLER_NOT_SET ((void *)~(ULONG_PTR)0)

/*
 *
 * IAsyncAction
 *
 */

struct async_void
{
    IAsyncAction IAsyncAction_iface;
    IAsyncInfo IAsyncInfo_iface;
    LONG ref;

    IAsyncActionCompletedHandler *handler;

    async_action_callback callback;
    TP_WORK *async_run_work;
    IInspectable *invoker;

    CRITICAL_SECTION cs;
    AsyncStatus status;
    HRESULT hr;
};

static inline struct async_void *impl_from_IAsyncAction( IAsyncAction *iface )
{
    return CONTAINING_RECORD(iface, struct async_void, IAsyncAction_iface);
}

HRESULT WINAPI async_void_QueryInterface( IAsyncAction *iface, REFIID iid, void **out )
{
    struct async_void *impl = impl_from_IAsyncAction(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IAsyncAction))
    {
        IInspectable_AddRef((*out = &impl->IAsyncAction_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IAsyncInfo))
    {
        IInspectable_AddRef((*out = &impl->IAsyncInfo_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI async_void_AddRef( IAsyncAction *iface )
{
    struct async_void *impl = impl_from_IAsyncAction(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

ULONG WINAPI async_void_Release( IAsyncAction *iface )
{
    struct async_void *impl = impl_from_IAsyncAction(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        IAsyncInfo_Close(&impl->IAsyncInfo_iface);

        if (impl->invoker)
            IInspectable_Release(impl->invoker);
        if (impl->handler && impl->handler != HANDLER_NOT_SET)
            IAsyncActionCompletedHandler_Release(impl->handler);

        impl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&impl->cs);
        free(impl);
    }

    return ref;
}

HRESULT WINAPI async_void_GetIids( IAsyncAction *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

HRESULT WINAPI async_void_GetRuntimeClassName( IAsyncAction *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

HRESULT WINAPI async_void_GetTrustLevel( IAsyncAction *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

HRESULT WINAPI async_void_put_Completed( IAsyncAction *iface, IAsyncActionCompletedHandler *handler )
{
    struct async_void *impl = impl_from_IAsyncAction(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, handler %p.\n", iface, handler);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->handler != HANDLER_NOT_SET)
        hr = E_ILLEGAL_DELEGATE_ASSIGNMENT;
    /*
        impl->handler can only be set once with async_void_put_Completed,
        so by default we set a non HANDLER_NOT_SET value, in this case handler.
    */
    else if ((impl->handler = handler))
    {
        IAsyncActionCompletedHandler_AddRef(impl->handler);

        if (impl->status > Started)
        {
            IAsyncAction *action = &impl->IAsyncAction_iface;
            AsyncStatus status = impl->status;
            impl->handler = NULL; /* Prevent concurrent invoke. */
            LeaveCriticalSection(&impl->cs);

            IAsyncActionCompletedHandler_Invoke(handler, action, status);
            IAsyncActionCompletedHandler_Release(handler);

            return S_OK;
        }
    }
    LeaveCriticalSection(&impl->cs);

    return hr;
}

HRESULT WINAPI async_void_get_Completed( IAsyncAction *iface, IAsyncActionCompletedHandler **handler )
{
    struct async_void *impl = impl_from_IAsyncAction(iface);
    HRESULT hr = S_OK;

    FIXME("iface %p, handler %p semi stub!\n", iface, handler);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    *handler = (impl->handler != HANDLER_NOT_SET) ? impl->handler : NULL;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

HRESULT WINAPI async_void_GetResults( IAsyncAction *iface )
{
    /* According to the docs, this function doesn't return anything, so it's left empty. */
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static const struct IAsyncActionVtbl async_void_vtbl =
{
    /* IUnknown methods */
    async_void_QueryInterface,
    async_void_AddRef,
    async_void_Release,
    /* IInspectable methods */
    async_void_GetIids,
    async_void_GetRuntimeClassName,
    async_void_GetTrustLevel,
    /* IAsyncAction methods */
    async_void_put_Completed,
    async_void_get_Completed,
    async_void_GetResults
};

/*
 *
 * IAsyncInfo for IAsyncAction
 *
 */

DEFINE_IINSPECTABLE_(async_void_info, IAsyncInfo, struct async_void, impl_from_async_void_IAsyncInfo, IAsyncInfo_iface, &impl->IAsyncAction_iface)

static HRESULT WINAPI async_void_info_get_Id( IAsyncInfo *iface, UINT32 *id )
{
    FIXME("iface %p, id %p stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_void_info_get_Status( IAsyncInfo *iface, AsyncStatus *status )
{
    struct async_void *impl = impl_from_async_void_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, status %p.\n", iface, status);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    *status = impl->status;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_void_info_get_ErrorCode( IAsyncInfo *iface, HRESULT *error_code )
{
    struct async_void *impl = impl_from_async_void_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, error_code %p.\n", iface, error_code);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        *error_code = hr = E_ILLEGAL_METHOD_CALL;
    else
        *error_code = impl->hr;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_void_info_Cancel( IAsyncInfo *iface )
{
    struct async_void *impl = impl_from_async_void_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->status == Started)
        impl->status = Canceled;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_void_info_Close( IAsyncInfo *iface )
{
    struct async_void *impl = impl_from_async_void_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Started)
        hr = E_ILLEGAL_STATE_CHANGE;
    else if (impl->status != Closed)
    {
        CloseThreadpoolWork(impl->async_run_work);
        impl->async_run_work = NULL;
        impl->status = Closed;
    }
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static const struct IAsyncInfoVtbl async_void_info_vtbl =
{
    /* IUnknown methods */
    async_void_info_QueryInterface,
    async_void_info_AddRef,
    async_void_info_Release,
    /* IInspectable methods */
    async_void_info_GetIids,
    async_void_info_GetRuntimeClassName,
    async_void_info_GetTrustLevel,
    /* IAsyncInfo */
    async_void_info_get_Id,
    async_void_info_get_Status,
    async_void_info_get_ErrorCode,
    async_void_info_Cancel,
    async_void_info_Close
};

static void CALLBACK async_void_run_cb(TP_CALLBACK_INSTANCE *instance, void *data, TP_WORK *work)
{
    IAsyncAction *action = data;
    struct async_void *impl = impl_from_IAsyncAction(action);
    HRESULT hr;

    hr = impl->callback(impl->invoker);

    EnterCriticalSection(&impl->cs);
    if (impl->status < Closed)
        impl->status = FAILED(hr) ? Error : Completed;

    impl->hr = hr;

    if (impl->handler != NULL && impl->handler != HANDLER_NOT_SET)
    {
        IAsyncActionCompletedHandler*handler = impl->handler;
        AsyncStatus status = impl->status;
        impl->handler = NULL; /* Prevent concurrent invoke. */
        LeaveCriticalSection(&impl->cs);

        IAsyncActionCompletedHandler_Invoke(handler, action, status);
        IAsyncActionCompletedHandler_Release(handler);
    }
    else LeaveCriticalSection(&impl->cs);

    IAsyncAction_Release(action);
}

HRESULT async_action_create( IInspectable *invoker, async_action_callback callback, IAsyncAction **out )
{
    struct async_void *impl;

    TRACE("invoker %p, callback %p, out %p.\n", invoker, callback, out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IAsyncAction_iface.lpVtbl = &async_void_vtbl;
    impl->IAsyncInfo_iface.lpVtbl = &async_void_info_vtbl;
    impl->ref = 1;

    impl->handler = HANDLER_NOT_SET;
    impl->callback = callback;
    impl->status = Started;

    if (!(impl->async_run_work = CreateThreadpoolWork(async_void_run_cb, &impl->IAsyncAction_iface, NULL)))
    {
        free(impl);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (invoker) IInspectable_AddRef((impl->invoker = invoker));

    InitializeCriticalSection(&impl->cs);
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": async_action.cs");

    /* AddRef to keep the obj alive in the callback. */
    IAsyncAction_AddRef(&impl->IAsyncAction_iface);
    SubmitThreadpoolWork(impl->async_run_work);

    *out = &impl->IAsyncAction_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}

/*
 *
 * IAsyncOperation<IInspectable*>
 *
 */

struct async_inspectable
{
    IAsyncOperation_IInspectable IAsyncOperation_IInspectable_iface;
    IAsyncInfo IAsyncInfo_iface;
    const GUID *iid;
    LONG ref;

    IAsyncOperationCompletedHandler_IInspectable *handler;
    IInspectable *result;

    async_operation_inspectable_callback callback;
    TP_WORK *async_run_work;
    IInspectable *invoker;

    CRITICAL_SECTION cs;
    AsyncStatus status;
    HRESULT hr;
};

static inline struct async_inspectable *impl_from_IAsyncOperation_IInspectable(IAsyncOperation_IInspectable *iface)
{
    return CONTAINING_RECORD(iface, struct async_inspectable, IAsyncOperation_IInspectable_iface);
}

static HRESULT WINAPI async_inspectable_QueryInterface( IAsyncOperation_IInspectable *iface, REFIID iid, void **out )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, impl->iid))
    {
        IInspectable_AddRef((*out = &impl->IAsyncOperation_IInspectable_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IAsyncInfo))
    {
        IInspectable_AddRef((*out = &impl->IAsyncInfo_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_inspectable_AddRef( IAsyncOperation_IInspectable *iface )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI async_inspectable_Release( IAsyncOperation_IInspectable *iface )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        IAsyncInfo_Close(&impl->IAsyncInfo_iface);

        if (impl->invoker)
            IInspectable_Release(impl->invoker);
        if (impl->handler && impl->handler != HANDLER_NOT_SET)
            IAsyncOperationCompletedHandler_IInspectable_Release(impl->handler);
        if (impl->result)
            IInspectable_Release(impl->result);

        impl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&impl->cs);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI async_inspectable_GetIids( IAsyncOperation_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_inspectable_GetRuntimeClassName( IAsyncOperation_IInspectable *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_inspectable_GetTrustLevel( IAsyncOperation_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_inspectable_put_Completed( IAsyncOperation_IInspectable *iface,
                                                       IAsyncOperationCompletedHandler_IInspectable *handler )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, handler %p.\n", iface, handler);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->handler != HANDLER_NOT_SET)
        hr = E_ILLEGAL_DELEGATE_ASSIGNMENT;
    /*
        impl->handler can only be set once with async_inspectable_put_Completed,
        so by default we set a non HANDLER_NOT_SET value, in this case handler.
    */
    else if ((impl->handler = handler))
    {
        IAsyncOperationCompletedHandler_IInspectable_AddRef(impl->handler);

        if (impl->status > Started)
        {
            IAsyncOperation_IInspectable *operation = &impl->IAsyncOperation_IInspectable_iface;
            AsyncStatus status = impl->status;
            impl->handler = NULL; /* Prevent concurrent invoke. */
            LeaveCriticalSection(&impl->cs);

            IAsyncOperationCompletedHandler_IInspectable_Invoke(handler, operation, status);
            IAsyncOperationCompletedHandler_IInspectable_Release(handler);

            return S_OK;
        }
    }
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_inspectable_get_Completed( IAsyncOperation_IInspectable *iface,
                                                       IAsyncOperationCompletedHandler_IInspectable **handler )
{
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(iface);
    HRESULT hr = S_OK;

    FIXME("iface %p, handler %p semi stub!\n", iface, handler);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    *handler = (impl->handler != HANDLER_NOT_SET) ? impl->handler : NULL;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_inspectable_GetResults( IAsyncOperation_IInspectable *iface, IInspectable **results )
{
    /* NOTE: Despite the name, this function only returns one result! */
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(iface);
    HRESULT hr;

    TRACE("iface %p, results %p.\n", iface, results);

    EnterCriticalSection(&impl->cs);
    if (impl->status != Completed && impl->status != Error)
        hr = E_ILLEGAL_METHOD_CALL;
    else if (!impl->result)
        hr = E_UNEXPECTED;
    else
    {
        *results = impl->result;
        impl->result = NULL; /* NOTE: AsyncOperation gives up it's reference to result here! */
        hr = impl->hr;
    }
    LeaveCriticalSection(&impl->cs);

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
    /* IAsyncOperation<IInspectable*> */
    async_inspectable_put_Completed,
    async_inspectable_get_Completed,
    async_inspectable_GetResults
};

/*
 *
 * IAsyncInfo for IAsyncOperation<IInspectable*>
 *
 */

DEFINE_IINSPECTABLE_(async_inspectable_info, IAsyncInfo, struct async_inspectable,
                     async_inspectable_impl_from_IAsyncInfo, IAsyncInfo_iface, &impl->IAsyncOperation_IInspectable_iface)

static HRESULT WINAPI async_inspectable_info_get_Id( IAsyncInfo *iface, UINT32 *id )
{
    FIXME("iface %p, id %p stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_inspectable_info_get_Status( IAsyncInfo *iface, AsyncStatus *status )
{
    struct async_inspectable *impl = async_inspectable_impl_from_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, status %p.\n", iface, status);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    *status = impl->status;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_inspectable_info_get_ErrorCode( IAsyncInfo *iface, HRESULT *error_code )
{
    struct async_inspectable *impl = async_inspectable_impl_from_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, error_code %p.\n", iface, error_code);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        *error_code = hr = E_ILLEGAL_METHOD_CALL;
    else
        *error_code = impl->hr;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_inspectable_info_Cancel( IAsyncInfo *iface )
{
    struct async_inspectable *impl = async_inspectable_impl_from_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Closed)
        hr = E_ILLEGAL_METHOD_CALL;
    else if (impl->status == Started)
        impl->status = Canceled;
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static HRESULT WINAPI async_inspectable_info_Close( IAsyncInfo *iface )
{
    struct async_inspectable *impl = async_inspectable_impl_from_IAsyncInfo(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&impl->cs);
    if (impl->status == Started)
        hr = E_ILLEGAL_STATE_CHANGE;
    else if (impl->status != Closed)
    {
        CloseThreadpoolWork(impl->async_run_work);
        impl->async_run_work = NULL;
        impl->status = Closed;
    }
    LeaveCriticalSection(&impl->cs);

    return hr;
}

static const struct IAsyncInfoVtbl async_inspectable_info_vtbl =
{
    /* IUnknown methods */
    async_inspectable_info_QueryInterface,
    async_inspectable_info_AddRef,
    async_inspectable_info_Release,
    /* IInspectable methods */
    async_inspectable_info_GetIids,
    async_inspectable_info_GetRuntimeClassName,
    async_inspectable_info_GetTrustLevel,
    /* IAsyncInfo */
    async_inspectable_info_get_Id,
    async_inspectable_info_get_Status,
    async_inspectable_info_get_ErrorCode,
    async_inspectable_info_Cancel,
    async_inspectable_info_Close
};

static void CALLBACK async_inspectable_run_cb(TP_CALLBACK_INSTANCE *instance, void *data, TP_WORK *work)
{
    IAsyncOperation_IInspectable *operation = data;
    IInspectable *result = NULL;
    struct async_inspectable *impl = impl_from_IAsyncOperation_IInspectable(operation);
    HRESULT hr;

    hr = impl->callback(impl->invoker, &result);

    EnterCriticalSection(&impl->cs);
    if (impl->status < Closed)
        impl->status = FAILED(hr) ? Error : Completed;

    impl->result = result;
    impl->hr = hr;

    if (impl->handler != NULL && impl->handler != HANDLER_NOT_SET)
    {
        IAsyncOperationCompletedHandler_IInspectable *handler = impl->handler;
        AsyncStatus status = impl->status;
        impl->handler = NULL; /* Prevent concurrent invoke. */
        LeaveCriticalSection(&impl->cs);

        IAsyncOperationCompletedHandler_IInspectable_Invoke(handler, operation, status);
        IAsyncOperationCompletedHandler_IInspectable_Release(handler);
    }
    else LeaveCriticalSection(&impl->cs);

    IAsyncOperation_IInspectable_Release(operation);
}

HRESULT async_operation_inspectable_create( const GUID *iid,
                                            IInspectable *invoker,
                                            async_operation_inspectable_callback callback,
                                            IAsyncOperation_IInspectable **out )
{
    struct async_inspectable *impl;

    TRACE("iid %s, invoker %p, callback %p, out %p.\n", debugstr_guid(iid), invoker, callback, out);

    *out = NULL;
    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    impl->IAsyncOperation_IInspectable_iface.lpVtbl = &async_inspectable_vtbl;
    impl->IAsyncInfo_iface.lpVtbl = &async_inspectable_info_vtbl;
    impl->iid = iid;
    impl->ref = 1;

    impl->handler = HANDLER_NOT_SET;
    impl->callback = callback;
    impl->status = Started;

    if (!(impl->async_run_work = CreateThreadpoolWork(async_inspectable_run_cb, &impl->IAsyncOperation_IInspectable_iface, NULL)))
    {
        free(impl);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (invoker) IInspectable_AddRef((impl->invoker = invoker));

    InitializeCriticalSection(&impl->cs);
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": async_operation.cs");

    /* AddRef to keep the obj alive in the callback. */
    IAsyncOperation_IInspectable_AddRef(&impl->IAsyncOperation_IInspectable_iface);
    SubmitThreadpoolWork(impl->async_run_work);

    *out = &impl->IAsyncOperation_IInspectable_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}
