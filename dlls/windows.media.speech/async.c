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

/*
 *
 * IAsyncOperation<IInspectable*>
 *
 */

struct async_operation
{
    IAsyncOperation_IInspectable IAsyncOperation_IInspectable_iface;
    IAsyncInfo IAsyncInfo_iface;
    const GUID *iid;
    LONG ref;
};

static inline struct async_operation *impl_from_IAsyncOperation_IInspectable(IAsyncOperation_IInspectable *iface)
{
    return CONTAINING_RECORD(iface, struct async_operation, IAsyncOperation_IInspectable_iface);
}

static HRESULT WINAPI async_operation_QueryInterface( IAsyncOperation_IInspectable *iface, REFIID iid, void **out )
{
    struct async_operation *impl = impl_from_IAsyncOperation_IInspectable(iface);

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

static ULONG WINAPI async_operation_AddRef( IAsyncOperation_IInspectable *iface )
{
    struct async_operation *impl = impl_from_IAsyncOperation_IInspectable(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI async_operation_Release( IAsyncOperation_IInspectable *iface )
{
    struct async_operation *impl = impl_from_IAsyncOperation_IInspectable(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
        free(impl);

    return ref;
}

static HRESULT WINAPI async_operation_GetIids( IAsyncOperation_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_GetRuntimeClassName( IAsyncOperation_IInspectable *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_GetTrustLevel( IAsyncOperation_IInspectable *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_put_Completed( IAsyncOperation_IInspectable *iface,
                                                     IAsyncOperationCompletedHandler_IInspectable *handler )
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_get_Completed( IAsyncOperation_IInspectable *iface,
                                                     IAsyncOperationCompletedHandler_IInspectable **handler )
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_GetResults( IAsyncOperation_IInspectable *iface, IInspectable **results )
{
    FIXME("iface %p, results %p stub!\n", iface, results);
    return E_NOTIMPL;
}

static const struct IAsyncOperation_IInspectableVtbl async_operation_vtbl =
{
    /* IUnknown methods */
    async_operation_QueryInterface,
    async_operation_AddRef,
    async_operation_Release,
    /* IInspectable methods */
    async_operation_GetIids,
    async_operation_GetRuntimeClassName,
    async_operation_GetTrustLevel,
    /* IAsyncOperation<IInspectable*> */
    async_operation_put_Completed,
    async_operation_get_Completed,
    async_operation_GetResults
};

/*
 *
 * IAsyncInfo
 *
 */

DEFINE_IINSPECTABLE(async_operation_info, IAsyncInfo, struct async_operation, IAsyncOperation_IInspectable_iface)

static HRESULT WINAPI async_operation_info_get_Id( IAsyncInfo *iface, UINT32 *id )
{
    FIXME("iface %p, id %p stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_info_get_Status( IAsyncInfo *iface, AsyncStatus *status )
{
    FIXME("iface %p, status %p stub!\n", iface, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_info_get_ErrorCode( IAsyncInfo *iface, HRESULT *error_code )
{
    FIXME("iface %p, error_code %p stub!\n", iface, error_code);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_info_Cancel( IAsyncInfo *iface )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_operation_info_Close( IAsyncInfo *iface )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static const struct IAsyncInfoVtbl async_operation_info_vtbl =
{
    /* IUnknown methods */
    async_operation_info_QueryInterface,
    async_operation_info_AddRef,
    async_operation_info_Release,
    /* IInspectable methods */
    async_operation_info_GetIids,
    async_operation_info_GetRuntimeClassName,
    async_operation_info_GetTrustLevel,
    /* IAsyncInfo */
    async_operation_info_get_Id,
    async_operation_info_get_Status,
    async_operation_info_get_ErrorCode,
    async_operation_info_Cancel,
    async_operation_info_Close
};

HRESULT async_operation_create( const GUID *iid, IAsyncOperation_IInspectable **out )
{
    struct async_operation *impl;

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IAsyncOperation_IInspectable_iface.lpVtbl = &async_operation_vtbl;
    impl->IAsyncInfo_iface.lpVtbl = &async_operation_info_vtbl;
    impl->iid = iid;
    impl->ref = 1;

    *out = &impl->IAsyncOperation_IInspectable_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}
