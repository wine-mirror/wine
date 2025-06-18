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

struct async_user_consent_verifier_availability
{
    IAsyncOperation_UserConsentVerifierAvailability IAsyncOperation_UserConsentVerifierAvailability_iface;
    IAsyncInfoImpl *IAsyncInfoImpl_inner;
    LONG ref;
};

static inline struct async_user_consent_verifier_availability *impl_from_IAsyncOperation_UserConsentVerifierAvailability( IAsyncOperation_UserConsentVerifierAvailability *iface )
{
    return CONTAINING_RECORD( iface, struct async_user_consent_verifier_availability, IAsyncOperation_UserConsentVerifierAvailability_iface );
}

static HRESULT WINAPI async_user_consent_verifier_availability_QueryInterface( IAsyncOperation_UserConsentVerifierAvailability *iface, REFIID iid, void **out )
{
    struct async_user_consent_verifier_availability *impl = impl_from_IAsyncOperation_UserConsentVerifierAvailability( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperation_UserConsentVerifierAvailability ))
    {
        IInspectable_AddRef( (*out = &impl->IAsyncOperation_UserConsentVerifierAvailability_iface) );
        return S_OK;
    }

    return IAsyncInfoImpl_QueryInterface( impl->IAsyncInfoImpl_inner, iid, out );
}

static ULONG WINAPI async_user_consent_verifier_availability_AddRef( IAsyncOperation_UserConsentVerifierAvailability *iface )
{
    struct async_user_consent_verifier_availability *impl = impl_from_IAsyncOperation_UserConsentVerifierAvailability( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI async_user_consent_verifier_availability_Release( IAsyncOperation_UserConsentVerifierAvailability *iface )
{
    struct async_user_consent_verifier_availability *impl = impl_from_IAsyncOperation_UserConsentVerifierAvailability( iface );
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

static HRESULT WINAPI async_user_consent_verifier_availability_GetIids( IAsyncOperation_UserConsentVerifierAvailability *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_user_consent_verifier_availability_GetRuntimeClassName( IAsyncOperation_UserConsentVerifierAvailability *iface, HSTRING *class_name )
{
    return WindowsCreateString( L"Windows.Foundation.IAsyncOperation`1<Windows.Security.Credentials.UI.UserConsentVerifierAvailability>",
                                ARRAY_SIZE(L"Windows.Foundation.IAsyncOperation`1<Windows.Security.Credentials.UI.UserConsentVerifierAvailability>"),
                                class_name );
}

static HRESULT WINAPI async_user_consent_verifier_availability_GetTrustLevel( IAsyncOperation_UserConsentVerifierAvailability *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI async_user_consent_verifier_availability_put_Completed( IAsyncOperation_UserConsentVerifierAvailability *iface,
                                                                              IAsyncOperationCompletedHandler_UserConsentVerifierAvailability *handler )
{
    struct async_user_consent_verifier_availability *impl = impl_from_IAsyncOperation_UserConsentVerifierAvailability( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_put_Completed( impl->IAsyncInfoImpl_inner, (IAsyncOperationCompletedHandlerImpl *)handler );
}

static HRESULT WINAPI async_user_consent_verifier_availability_get_Completed( IAsyncOperation_UserConsentVerifierAvailability *iface,
                                                                              IAsyncOperationCompletedHandler_UserConsentVerifierAvailability **handler )
{
    struct async_user_consent_verifier_availability *impl = impl_from_IAsyncOperation_UserConsentVerifierAvailability( iface );
    TRACE( "iface %p, handler %p.\n", iface, handler );
    return IAsyncInfoImpl_get_Completed( impl->IAsyncInfoImpl_inner, (IAsyncOperationCompletedHandlerImpl **)handler );
}

static HRESULT WINAPI async_user_consent_verifier_availability_GetResults( IAsyncOperation_UserConsentVerifierAvailability *iface, UserConsentVerifierAvailability *results )
{
    struct async_user_consent_verifier_availability *impl = impl_from_IAsyncOperation_UserConsentVerifierAvailability( iface );
    PROPVARIANT result = {.vt = VT_UI4};
    HRESULT hr;

    TRACE( "iface %p, results %p.\n", iface, results );

    hr = IAsyncInfoImpl_get_Result( impl->IAsyncInfoImpl_inner, &result );

    *results = result.ulVal;
    PropVariantClear( &result );
    return hr;
}

static const struct IAsyncOperation_UserConsentVerifierAvailabilityVtbl async_user_consent_verifier_availability_vtbl =
{
    /* IUnknown methods */
    async_user_consent_verifier_availability_QueryInterface,
    async_user_consent_verifier_availability_AddRef,
    async_user_consent_verifier_availability_Release,
    /* IInspectable methods */
    async_user_consent_verifier_availability_GetIids,
    async_user_consent_verifier_availability_GetRuntimeClassName,
    async_user_consent_verifier_availability_GetTrustLevel,
    /* IAsyncOperation<UserConsentVerifierAvailability> */
    async_user_consent_verifier_availability_put_Completed,
    async_user_consent_verifier_availability_get_Completed,
    async_user_consent_verifier_availability_GetResults,
};

HRESULT async_operation_user_consent_verifier_availability_create( IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                                                   IAsyncOperation_UserConsentVerifierAvailability **out )
{
    struct async_user_consent_verifier_availability *impl;
    HRESULT hr;

    *out = NULL;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IAsyncOperation_UserConsentVerifierAvailability_iface.lpVtbl = &async_user_consent_verifier_availability_vtbl;
    impl->ref = 1;

    if (FAILED(hr = async_info_create( invoker, param, callback, (IInspectable *)&impl->IAsyncOperation_UserConsentVerifierAvailability_iface, &impl->IAsyncInfoImpl_inner )) ||
        FAILED(hr = IAsyncInfoImpl_Start( impl->IAsyncInfoImpl_inner )))
    {
        if (impl->IAsyncInfoImpl_inner) IAsyncInfoImpl_Release( impl->IAsyncInfoImpl_inner );
        free( impl );
        return hr;
    }

    *out = &impl->IAsyncOperation_UserConsentVerifierAvailability_iface;
    TRACE( "created IAsyncOperation_UserConsentVerifierAvailability %p\n", *out );
    return S_OK;
}
