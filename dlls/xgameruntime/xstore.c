/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XStore
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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

struct x_store
{
    IXStoreImpl6 IXStoreImpl6_iface;
    LONG ref;
};

static inline struct x_store *impl_from_IXStoreImpl6( IXStoreImpl6 *iface )
{
    return CONTAINING_RECORD( iface, struct x_store, IXStoreImpl6_iface );
}

static HRESULT WINAPI x_store_QueryInterface( IXStoreImpl6 *iface, REFIID iid, void **out )
{
    struct x_store *impl = impl_from_IXStoreImpl6( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown     ) ||
        IsEqualGUID( iid, &IID_IXStoreImpl  ) ||
        IsEqualGUID( iid, &IID_IXStoreImpl2 ) ||
        IsEqualGUID( iid, &IID_IXStoreImpl3 ) ||
        IsEqualGUID( iid, &IID_IXStoreImpl4 ) ||
        IsEqualGUID( iid, &IID_IXStoreImpl5 ) ||
        IsEqualGUID( iid, &IID_IXStoreImpl6 ))
    {
        IXStoreImpl6_AddRef( *out = &impl->IXStoreImpl6_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_store_AddRef( IXStoreImpl6 *iface )
{
    struct x_store *impl = impl_from_IXStoreImpl6( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_store_Release( IXStoreImpl6 *iface )
{
    struct x_store *impl = impl_from_IXStoreImpl6( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT xstore_complete_inline( XAsyncBlock *async, HRESULT hr, const void *data, SIZE_T size )
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

static HRESULT WINAPI x_store_XStoreCreateContext( IXStoreImpl6 *iface, const XUserHandle user, XStoreContextHandle *storeContextHandle )
{
    TRACE( "iface %p, user %p, storeContextHandle %p\n", iface, user, storeContextHandle );
    *storeContextHandle = (XStoreContextHandle)(ULONG_PTR)0x1;
    return S_OK;
}

static void WINAPI x_store_XStoreCloseContextHandle( IXStoreImpl6 *iface, XStoreContextHandle storeContextHandle )
{
    TRACE( "iface %p, storeContextHandle %p\n", iface, storeContextHandle );
}

static HRESULT WINAPI x_store_XStoreQueryAssociatedProductsAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, UINT32 maxItemsToRetrievePerPage, XAsyncBlock *async )
{
    XStoreProductQueryHandle sentinel = (XStoreProductQueryHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, productKinds %#x, maxItems %u, async %p\n", iface, storeContextHandle, productKinds, maxItemsToRetrievePerPage, async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreQueryAssociatedProductsResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    TRACE( "iface %p, async %p, productQueryHandle %p\n", iface, async, productQueryHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*productQueryHandle), productQueryHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryProductsAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, const char **storeIds, SIZE_T storeIdsCount, const char **actionFilters, SIZE_T actionFiltersCount, XAsyncBlock *async )
{
    XStoreProductQueryHandle sentinel = (XStoreProductQueryHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, productKinds %#x, storeIdsCount %Iu, async %p\n", iface, storeContextHandle, productKinds, storeIdsCount, async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreQueryProductsResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    TRACE( "iface %p, async %p, productQueryHandle %p\n", iface, async, productQueryHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*productQueryHandle), productQueryHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryEntitledProductsAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, UINT32 maxItemsToRetrievePerPage, XAsyncBlock *async )
{
    XStoreProductQueryHandle sentinel = (XStoreProductQueryHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, productKinds %#x, maxItems %u, async %p\n", iface, storeContextHandle, productKinds, maxItemsToRetrievePerPage, async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreQueryEntitledProductsResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    TRACE( "iface %p, async %p, productQueryHandle %p\n", iface, async, productQueryHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*productQueryHandle), productQueryHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryProductForCurrentGameAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XAsyncBlock *async )
{
    XStoreProductQueryHandle sentinel = (XStoreProductQueryHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreQueryProductForCurrentGameResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    TRACE( "iface %p, async %p, productQueryHandle %p\n", iface, async, productQueryHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*productQueryHandle), productQueryHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryProductForPackageAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XStoreProductKind productKinds, const char *packageIdentifier, XAsyncBlock *async )
{
    XStoreProductQueryHandle sentinel = (XStoreProductQueryHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, productKinds %#x, package %s, async %p\n", iface, storeContextHandle, productKinds, debugstr_a( packageIdentifier ), async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreQueryProductForPackageResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    TRACE( "iface %p, async %p, productQueryHandle %p\n", iface, async, productQueryHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*productQueryHandle), productQueryHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreEnumerateProductsQuery( IXStoreImpl6 *iface, const XStoreProductQueryHandle productQueryHandle, void *context, XStoreProductQueryCallback *callback )
{
    TRACE( "iface %p, productQueryHandle %p, context %p, callback %p — empty query\n", iface, productQueryHandle, context, callback );
    return S_OK; /* no products to enumerate */
}

static BOOLEAN WINAPI x_store_XStoreProductsQueryHasMorePages( IXStoreImpl6 *iface, const XStoreProductQueryHandle productQueryHandle )
{
    TRACE( "iface %p, productQueryHandle %p\n", iface, productQueryHandle );
    return FALSE;
}

static HRESULT WINAPI x_store_XStoreProductsQueryNextPageAsync( IXStoreImpl6 *iface, const XStoreProductQueryHandle productQueryHandle, XAsyncBlock *async )
{
    FIXME( "iface %p, productQueryHandle %p, async %p stub!\n", iface, productQueryHandle, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_store_XStoreProductsQueryNextPageResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    FIXME( "iface %p, async %p, productQueryHandle %p stub!\n", iface, async, productQueryHandle );
    return E_NOTIMPL;
}

static void WINAPI x_store_XStoreCloseProductsQueryHandle( IXStoreImpl6 *iface, XStoreProductQueryHandle productQueryHandle )
{
    TRACE( "iface %p, productQueryHandle %p\n", iface, productQueryHandle );
}

static HRESULT WINAPI x_store_XStoreAcquireLicenseForPackageAsync( IXStoreImpl6 *iface, const XStoreProductQueryHandle productQueryHandle, const char *packageIdentifier, XAsyncBlock *async )
{
    XStoreLicenseHandle sentinel = (XStoreLicenseHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, productQueryHandle %p, package %s, async %p\n", iface, productQueryHandle, debugstr_a( packageIdentifier ), async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreAcquireLicenseForPackageResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreLicenseHandle *storeLicenseHandle )
{
    TRACE( "iface %p, async %p, storeLicenseHandle %p\n", iface, async, storeLicenseHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*storeLicenseHandle), storeLicenseHandle, NULL );
}

static BOOLEAN WINAPI x_store_XStoreIsLicenseValid( IXStoreImpl6 *iface, const XStoreLicenseHandle storeLicenseHandle )
{
    TRACE( "iface %p, storeLicenseHandle %p\n", iface, storeLicenseHandle );
    return storeLicenseHandle != NULL;
}

static void WINAPI x_store_XStoreCloseLicenseHandle( IXStoreImpl6 *iface, XStoreLicenseHandle storeLicenseHandle )
{
    TRACE( "iface %p, storeLicenseHandle %p\n", iface, storeLicenseHandle );
}

static HRESULT WINAPI x_store_XStoreCanAcquireLicenseForStoreIdAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeProductId, XAsyncBlock *async )
{
    XStoreCanAcquireLicenseResult result = { "00000", XStoreCanLicenseStatus_Licensable };
    TRACE( "iface %p, storeContextHandle %p, storeProductId %s, async %p\n", iface, storeContextHandle, debugstr_a( storeProductId ), async );
    return xstore_complete_inline( async, S_OK, &result, sizeof(result) );
}

static HRESULT WINAPI x_store_XStoreCanAcquireLicenseForStoreIdResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreCanAcquireLicenseResult *storeCanAcquireLicense )
{
    TRACE( "iface %p, async %p, storeCanAcquireLicense %p\n", iface, async, storeCanAcquireLicense );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*storeCanAcquireLicense), storeCanAcquireLicense, NULL );
}

static HRESULT WINAPI x_store_XStoreCanAcquireLicenseForPackageAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *packageIdentifier, XAsyncBlock *async )
{
    XStoreCanAcquireLicenseResult result = { "00000", XStoreCanLicenseStatus_Licensable };
    TRACE( "iface %p, storeContextHandle %p, package %s, async %p\n", iface, storeContextHandle, debugstr_a( packageIdentifier ), async );
    return xstore_complete_inline( async, S_OK, &result, sizeof(result) );
}

static HRESULT WINAPI x_store_XStoreCanAcquireLicenseForPackageResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreCanAcquireLicenseResult *storeCanAcquireLicense )
{
    TRACE( "iface %p, async %p, storeCanAcquireLicense %p\n", iface, async, storeCanAcquireLicense );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*storeCanAcquireLicense), storeCanAcquireLicense, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryGameLicenseAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XAsyncBlock *async )
{
    XStoreGameLicense lic;
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    memset( &lic, 0, sizeof(lic) );
    memcpy( lic.skuStoreId, "00000WINE001", 13 );
    lic.isActive        = TRUE;
    lic.isDiscLicense   = TRUE;
    lic.isTrial         = FALSE;
    lic.expirationDate  = (time_t)0x7FFFFFFF;
    return xstore_complete_inline( async, S_OK, &lic, sizeof(lic) );
}

static HRESULT WINAPI x_store_XStoreQueryGameLicenseResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreGameLicense *license )
{
    TRACE( "iface %p, async %p, license %p\n", iface, async, license );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*license), license, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryAddOnLicensesAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XAsyncBlock *async )
{
    UINT32 zero = 0;
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    return xstore_complete_inline( async, S_OK, &zero, sizeof(zero) );
}

static HRESULT WINAPI x_store_XStoreQueryAddOnLicensesResultCount( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 *count )
{
    TRACE( "iface %p, async %p, count %p\n", iface, async, count );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*count), count, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryAddOnLicensesResult( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 count, XStoreAddonLicense *addOnLicenses )
{
    TRACE( "iface %p, async %p, count %u, addOnLicenses %p\n", iface, async, count, addOnLicenses );
    return S_OK; /* count is 0 from ResultCount; nothing to copy */
}

static HRESULT WINAPI x_store_XStoreQueryConsumableBalanceRemainingAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeProductId, XAsyncBlock *async )
{
    XStoreConsumableResult res = { 0 };
    TRACE( "iface %p, storeContextHandle %p, storeProductId %s, async %p\n", iface, storeContextHandle, debugstr_a( storeProductId ), async );
    return xstore_complete_inline( async, S_OK, &res, sizeof(res) );
}

static HRESULT WINAPI x_store_XStoreQueryConsumableBalanceRemainingResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreConsumableResult *consumableResult )
{
    TRACE( "iface %p, async %p, consumableResult %p\n", iface, async, consumableResult );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*consumableResult), consumableResult, NULL );
}

static HRESULT WINAPI x_store_XStoreReportConsumableFulfillmentAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeProductId, UINT32 quantity, GUID trackingId, XAsyncBlock *async )
{
    XStoreConsumableResult res = { 0 };
    TRACE( "iface %p, storeContextHandle %p, storeProductId %s, quantity %u, async %p\n", iface, storeContextHandle, debugstr_a( storeProductId ), quantity, async );
    return xstore_complete_inline( async, S_OK, &res, sizeof(res) );
}

static HRESULT WINAPI x_store_XStoreReportConsumableFulfillmentResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreConsumableResult *consumableResult )
{
    TRACE( "iface %p, async %p, consumableResult %p\n", iface, async, consumableResult );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*consumableResult), consumableResult, NULL );
}

static HRESULT WINAPI x_store_XStoreGetUserCollectionsIdAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *serviceTicket, const char *publisherUserId, XAsyncBlock *async )
{
    /* No Xbox Live — return an empty string */
    char empty = '\0';
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    return xstore_complete_inline( async, S_OK, &empty, sizeof(empty) );
}

static HRESULT WINAPI x_store_XStoreGetUserCollectionsIdResultSize( IXStoreImpl6 *iface, XAsyncBlock *async, SIZE_T *size )
{
    struct xasync_state *state = (struct xasync_state *)async->internal[0];
    TRACE( "iface %p, async %p, size %p\n", iface, async, size );
    if (!state || !state->completed) return 0x8000000E;
    *size = state->result_size;
    return S_OK;
}

static HRESULT WINAPI x_store_XStoreGetUserCollectionsIdResult( IXStoreImpl6 *iface, XAsyncBlock *async, SIZE_T size, char *result )
{
    TRACE( "iface %p, async %p, size %Iu, result %p\n", iface, async, size, result );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, size, result, NULL );
}

static HRESULT WINAPI x_store_XStoreGetUserPurchaseIdAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *serviceTicket, const char *publisherUserId, XAsyncBlock *async )
{
    char empty = '\0';
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    return xstore_complete_inline( async, S_OK, &empty, sizeof(empty) );
}

static HRESULT WINAPI x_store_XStoreGetUserPurchaseIdResultSize( IXStoreImpl6 *iface, XAsyncBlock *async, SIZE_T *size )
{
    struct xasync_state *state = (struct xasync_state *)async->internal[0];
    TRACE( "iface %p, async %p, size %p\n", iface, async, size );
    if (!state || !state->completed) return 0x8000000E;
    *size = state->result_size;
    return S_OK;
}

static HRESULT WINAPI x_store_XStoreGetUserPurchaseIdResult( IXStoreImpl6 *iface, XAsyncBlock *async, SIZE_T size, char *result )
{
    TRACE( "iface %p, async %p, size %Iu, result %p\n", iface, async, size, result );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, size, result, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryLicenseTokenAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char **productIds, SIZE_T productIdsCount, const char *customDeveloperString, XAsyncBlock *async )
{
    char empty = '\0';
    TRACE( "iface %p, storeContextHandle %p, idsCount %Iu, async %p\n", iface, storeContextHandle, productIdsCount, async );
    return xstore_complete_inline( async, S_OK, &empty, sizeof(empty) );
}

static HRESULT WINAPI x_store_XStoreQueryLicenseTokenResultSize( IXStoreImpl6 *iface, XAsyncBlock *async, SIZE_T *size )
{
    struct xasync_state *state = (struct xasync_state *)async->internal[0];
    TRACE( "iface %p, async %p, size %p\n", iface, async, size );
    if (!state || !state->completed) return 0x8000000E;
    *size = state->result_size;
    return S_OK;
}

static HRESULT WINAPI x_store_XStoreQueryLicenseTokenResult( IXStoreImpl6 *iface, XAsyncBlock *async, SIZE_T size, char *result )
{
    TRACE( "iface %p, async %p, size %Iu, result %p\n", iface, async, size, result );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, size, result, NULL );
}

static HRESULT WINAPI __PADDING__( IXStoreImpl6 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING_2__( IXStoreImpl6 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING_3__( IXStoreImpl6 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_store_XStoreShowPurchaseUIAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeId, const char *name, const char *extendedJsonData, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, storeId %s, name %s, async %p — no-op\n", iface, storeContextHandle, debugstr_a( storeId ), debugstr_a( name ), async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreShowPurchaseUIResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static HRESULT WINAPI x_store_XStoreShowRateAndReviewUIAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XAsyncBlock *async )
{
    XStoreRateAndReviewResult res = { TRUE };
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    return xstore_complete_inline( async, S_OK, &res, sizeof(res) );
}

static HRESULT WINAPI x_store_XStoreShowRateAndReviewUIResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreRateAndReviewResult *result )
{
    TRACE( "iface %p, async %p, result %p\n", iface, async, result );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*result), result, NULL );
}

static HRESULT WINAPI x_store_XStoreShowRedeemTokenUIAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *token, const char **allowedStoreIds, SIZE_T allowedStoreIdsCount, BOOLEAN disallowCsvRedemption, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, token %s, async %p — no-op\n", iface, storeContextHandle, debugstr_a( token ), async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreShowRedeemTokenUIResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryGameAndDlcPackageUpdatesAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, XAsyncBlock *async )
{
    UINT32 zero = 0;
    TRACE( "iface %p, storeContextHandle %p, async %p\n", iface, storeContextHandle, async );
    return xstore_complete_inline( async, S_OK, &zero, sizeof(zero) );
}

static HRESULT WINAPI x_store_XStoreQueryGameAndDlcPackageUpdatesResultCount( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 *count )
{
    TRACE( "iface %p, async %p, count %p\n", iface, async, count );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*count), count, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryGameAndDlcPackageUpdatesResult( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 count, XStorePackageUpdate *packageUpdates )
{
    TRACE( "iface %p, async %p, count %u, packageUpdates %p\n", iface, async, count, packageUpdates );
    return S_OK; /* count 0 — nothing to copy */
}

static HRESULT WINAPI x_store_XStoreDownloadPackageUpdatesAsync( IXStoreImpl6 *iface, XStoreContextHandle storeContextHandle, const char **packageIdentifiers, SIZE_T packageIdentifiersCount, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, packageIdentifiersCount %Iu, async %p — no-op\n", iface, storeContextHandle, packageIdentifiersCount, async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreDownloadPackageUpdatesResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static HRESULT WINAPI x_store_XStoreDownloadAndInstallPackageUpdatesAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char **packageIdentifiers, SIZE_T packageIdentifiersCount, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, packageIdentifiersCount %Iu, async %p — no-op\n", iface, storeContextHandle, packageIdentifiersCount, async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreDownloadAndInstallPackageUpdatesResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static HRESULT WINAPI x_store_XStoreDownloadAndInstallPackagesAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char **storeIds, SIZE_T storeIdsCount, XAsyncBlock *async )
{
    UINT32 zero = 0;
    FIXME( "iface %p, storeContextHandle %p, storeIdsCount %Iu, async %p — no-op\n", iface, storeContextHandle, storeIdsCount, async );
    return xstore_complete_inline( async, S_OK, &zero, sizeof(zero) );
}

static HRESULT WINAPI x_store_XStoreDownloadAndInstallPackagesResultCount( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 *count )
{
    TRACE( "iface %p, async %p, count %p\n", iface, async, count );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*count), count, NULL );
}

static HRESULT WINAPI x_store_XStoreDownloadAndInstallPackagesResult( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 count, char **packageIdentifiers )
{
    TRACE( "iface %p, async %p, count %u\n", iface, async, count );
    return S_OK; /* count 0 */
}

static HRESULT WINAPI x_store_XStoreQueryPackageIdentifier( IXStoreImpl6 *iface, const char *storeId, SIZE_T size, char *packageIdentifier )
{
    FIXME( "iface %p, storeId %s, size %Iu stub!\n", iface, debugstr_a( storeId ), size );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_store_XStoreRegisterGameLicenseChanged( IXStoreImpl6 *iface, XStoreContextHandle storeContextHandle, XTaskQueueHandle queue, void *context, XStoreGameLicenseChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, storeContextHandle %p, queue %p, token %p\n", iface, storeContextHandle, queue, token );
    token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_store_XStoreUnregisterGameLicenseChanged( IXStoreImpl6 *iface, XStoreContextHandle storeContextHandle, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p, storeContextHandle %p\n", iface, storeContextHandle );
    return TRUE;
}

static HRESULT WINAPI x_store_XStoreRegisterPackageLicenseLost( IXStoreImpl6 *iface, XStoreLicenseHandle storeLicenseHandle, XTaskQueueHandle queue, void *context, XStorePackageLicenseLostCallback *callback, XTaskQueueRegistrationToken *token)
{
    TRACE( "iface %p, storeLicenseHandle %p, queue %p, token %p\n", iface, storeLicenseHandle, queue, token );
    token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_store_XStoreUnregisterPackageLicenseLost( IXStoreImpl6 *iface, XStoreLicenseHandle licenseHandle, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p, licenseHandle %p\n", iface, licenseHandle );
    return TRUE;
}

static BOOLEAN WINAPI x_store_XStoreIsAvailabilityPurchasable( IXStoreImpl6 *iface, const XStoreAvailability availability )
{
    TRACE( "iface %p\n", iface );
    return FALSE; /* no network store */
}

static HRESULT WINAPI x_store_XStoreAcquireLicenseForDurablesAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeId, XAsyncBlock *async )
{
    XStoreLicenseHandle sentinel = (XStoreLicenseHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, storeId %s, async %p\n", iface, storeContextHandle, debugstr_a( storeId ), async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreAcquireLicenseForDurablesResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreLicenseHandle *storeLicenseHandle )
{
    TRACE( "iface %p, async %p, storeLicenseHandle %p\n", iface, async, storeLicenseHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*storeLicenseHandle), storeLicenseHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreShowAssociatedProductsUIAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeId, XStoreProductKind productKinds, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, storeId %s, productKinds %#x, async %p — no-op\n", iface, storeContextHandle, debugstr_a( storeId ), productKinds, async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreShowAssociatedProductsUIResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static HRESULT WINAPI x_store_XStoreShowProductPageUIAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeId, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, storeId %s, async %p — no-op\n", iface, storeContextHandle, debugstr_a( storeId ), async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreShowProductPageUIResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryAssociatedProductsForStoreIdAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeId, XStoreProductKind productKinds, UINT32 maxItemsToRetrievePerPage, XAsyncBlock *async )
{
    XStoreProductQueryHandle sentinel = (XStoreProductQueryHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, storeContextHandle %p, storeId %s, productKinds %#x, async %p\n", iface, storeContextHandle, debugstr_a( storeId ), productKinds, async );
    return xstore_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_store_XStoreQueryAssociatedProductsForStoreIdResult( IXStoreImpl6 *iface, XAsyncBlock *async, XStoreProductQueryHandle *productQueryHandle )
{
    TRACE( "iface %p, async %p, productQueryHandle %p\n", iface, async, productQueryHandle );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*productQueryHandle), productQueryHandle, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryPackageUpdatesAsync( IXStoreImpl6 *iface, XStoreContextHandle storeContextHandle, const char **packageIdentifiers, SIZE_T packageIdentifiersCount, XAsyncBlock *async )
{
    UINT32 zero = 0;
    TRACE( "iface %p, storeContextHandle %p, packageIdentifiersCount %Iu, async %p\n", iface, storeContextHandle, packageIdentifiersCount, async );
    return xstore_complete_inline( async, S_OK, &zero, sizeof(zero) );
}

static HRESULT WINAPI x_store_XStoreQueryPackageUpdatesResultCount( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 *count )
{
    TRACE( "iface %p, async %p, count %p\n", iface, async, count );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*count), count, NULL );
}

static HRESULT WINAPI x_store_XStoreQueryPackageUpdatesResult( IXStoreImpl6 *iface, XAsyncBlock *async, UINT32 count, XStorePackageUpdate *packageUpdates )
{
    TRACE( "iface %p, async %p, count %u\n", iface, async, count );
    return S_OK; /* count 0 */
}

static HRESULT WINAPI x_store_XStoreShowGiftingUIAsync( IXStoreImpl6 *iface, const XStoreContextHandle storeContextHandle, const char *storeId, const char *name, const char *extendedJsonData, XAsyncBlock *async )
{
    FIXME( "iface %p, storeContextHandle %p, storeId %s, name %s, async %p — no-op\n", iface, storeContextHandle, debugstr_a( storeId ), debugstr_a( name ), async );
    return xstore_complete_inline( async, S_OK, NULL, 0 );
}

static HRESULT WINAPI x_store_XStoreShowGiftingUIResult( IXStoreImpl6 *iface, XAsyncBlock *async )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, 0, NULL, NULL );
}

static const struct IXStoreImpl6Vtbl x_store_vtbl =
{
    x_store_QueryInterface,
    x_store_AddRef,
    x_store_Release,
    /* IXStoreImpl methods */
    x_store_XStoreCreateContext,
    x_store_XStoreCloseContextHandle,
    x_store_XStoreQueryAssociatedProductsAsync,
    x_store_XStoreQueryAssociatedProductsResult,
    x_store_XStoreQueryProductsAsync,
    x_store_XStoreQueryProductsResult,
    x_store_XStoreQueryEntitledProductsAsync,
    x_store_XStoreQueryEntitledProductsResult,
    x_store_XStoreQueryProductForCurrentGameAsync,
    x_store_XStoreQueryProductForCurrentGameResult,
    x_store_XStoreQueryProductForPackageAsync,
    x_store_XStoreQueryProductForPackageResult,
    x_store_XStoreEnumerateProductsQuery,
    x_store_XStoreProductsQueryHasMorePages,
    x_store_XStoreProductsQueryNextPageAsync,
    x_store_XStoreProductsQueryNextPageResult,
    x_store_XStoreCloseProductsQueryHandle,
    x_store_XStoreAcquireLicenseForPackageAsync,
    x_store_XStoreAcquireLicenseForPackageResult,
    x_store_XStoreIsLicenseValid,
    x_store_XStoreCloseLicenseHandle,
    x_store_XStoreCanAcquireLicenseForStoreIdAsync,
    x_store_XStoreCanAcquireLicenseForStoreIdResult,
    x_store_XStoreCanAcquireLicenseForPackageAsync,
    x_store_XStoreCanAcquireLicenseForPackageResult,
    x_store_XStoreQueryGameLicenseAsync,
    x_store_XStoreQueryGameLicenseResult,
    x_store_XStoreQueryAddOnLicensesAsync,
    x_store_XStoreQueryAddOnLicensesResultCount,
    x_store_XStoreQueryAddOnLicensesResult,
    x_store_XStoreQueryConsumableBalanceRemainingAsync,
    x_store_XStoreQueryConsumableBalanceRemainingResult,
    x_store_XStoreReportConsumableFulfillmentAsync,
    x_store_XStoreReportConsumableFulfillmentResult,
    x_store_XStoreGetUserCollectionsIdAsync,
    x_store_XStoreGetUserCollectionsIdResultSize,
    x_store_XStoreGetUserCollectionsIdResult,
    x_store_XStoreGetUserPurchaseIdAsync,
    x_store_XStoreGetUserPurchaseIdResultSize,
    x_store_XStoreGetUserPurchaseIdResult,
    x_store_XStoreQueryLicenseTokenAsync,
    x_store_XStoreQueryLicenseTokenResultSize,
    x_store_XStoreQueryLicenseTokenResult,
    __PADDING__,
    __PADDING_2__,
    __PADDING_3__,
    x_store_XStoreShowPurchaseUIAsync,
    x_store_XStoreShowPurchaseUIResult,
    x_store_XStoreShowRateAndReviewUIAsync,
    x_store_XStoreShowRateAndReviewUIResult,
    x_store_XStoreShowRedeemTokenUIAsync,
    x_store_XStoreShowRedeemTokenUIResult,
    x_store_XStoreQueryGameAndDlcPackageUpdatesAsync,
    x_store_XStoreQueryGameAndDlcPackageUpdatesResultCount,
    x_store_XStoreQueryGameAndDlcPackageUpdatesResult,
    x_store_XStoreDownloadPackageUpdatesAsync,
    x_store_XStoreDownloadPackageUpdatesResult,
    x_store_XStoreDownloadAndInstallPackageUpdatesAsync,
    x_store_XStoreDownloadAndInstallPackageUpdatesResult,
    x_store_XStoreDownloadAndInstallPackagesAsync,
    x_store_XStoreDownloadAndInstallPackagesResultCount,
    x_store_XStoreDownloadAndInstallPackagesResult,
    x_store_XStoreQueryPackageIdentifier,
    x_store_XStoreRegisterGameLicenseChanged,
    x_store_XStoreUnregisterGameLicenseChanged,
    x_store_XStoreRegisterPackageLicenseLost,
    x_store_XStoreUnregisterPackageLicenseLost,
    /* IXStoreImpl2 methods */
    x_store_XStoreIsAvailabilityPurchasable,
    /* IXStoreImpl3 methods */
    x_store_XStoreAcquireLicenseForDurablesAsync,
    x_store_XStoreAcquireLicenseForDurablesResult,
    /* IXStoreImpl4 methods */
    x_store_XStoreShowAssociatedProductsUIAsync,
    x_store_XStoreShowAssociatedProductsUIResult,
    x_store_XStoreShowProductPageUIAsync,
    x_store_XStoreShowProductPageUIResult,
    /* IXStoreImpl5 methods */
    x_store_XStoreQueryAssociatedProductsForStoreIdAsync,
    x_store_XStoreQueryAssociatedProductsForStoreIdResult,
    x_store_XStoreQueryPackageUpdatesAsync,
    x_store_XStoreQueryPackageUpdatesResultCount,
    x_store_XStoreQueryPackageUpdatesResult,
    /* IXStoreImpl6 methods */
    x_store_XStoreShowGiftingUIAsync,
    x_store_XStoreShowGiftingUIResult,
};

static struct x_store x_store =
{
    {&x_store_vtbl},
    0,
};

IXStoreImpl *x_store_impl = (IXStoreImpl *)&x_store.IXStoreImpl6_iface;
