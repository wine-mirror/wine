/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XPersistentLocalStorage
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

struct x_persistent_local_storage
{
    IXPersistentLocalStorageImpl3 IXPersistentLocalStorageImpl3_iface;
    LONG ref;
};

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_persistent_local_storage *impl_from_IXPersistentLocalStorageImpl3( IXPersistentLocalStorageImpl3 *iface )
{
    return CONTAINING_RECORD( iface, struct x_persistent_local_storage, IXPersistentLocalStorageImpl3_iface );
}

static HRESULT WINAPI x_persistent_local_storage_QueryInterface( IXPersistentLocalStorageImpl3 *iface, REFIID iid, void **out )
{
    struct x_persistent_local_storage *impl = impl_from_IXPersistentLocalStorageImpl3( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown                      ) ||
        IsEqualGUID( iid, &IID_IXPersistentLocalStorageImpl  ) ||
        IsEqualGUID( iid, &IID_IXPersistentLocalStorageImpl2 ) ||
        IsEqualGUID( iid, &IID_IXPersistentLocalStorageImpl3 ))
    {
        IXPersistentLocalStorageImpl_AddRef( *out = &impl->IXPersistentLocalStorageImpl3_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_persistent_local_storage_AddRef( IXPersistentLocalStorageImpl3 *iface )
{
    struct x_persistent_local_storage *impl = impl_from_IXPersistentLocalStorageImpl3( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_persistent_local_storage_Release( IXPersistentLocalStorageImpl3 *iface )
{
    struct x_persistent_local_storage *impl = impl_from_IXPersistentLocalStorageImpl3( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_persistent_local_storage_XPersistentLocalStorageGetPath( IXPersistentLocalStorageImpl3 *iface, SIZE_T pathSize, char *path, SIZE_T *pathUsed )
{
    FIXME( "iface %p, pathSize %Iu, path %p, pathUsed %p stub!\n", iface, pathSize, path, pathUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_persistent_local_storage_XPersistentLocalStorageGetPathSize( IXPersistentLocalStorageImpl3 *iface, SIZE_T *pathSize )
{
    FIXME( "iface %p, pathSize %p stub!\n", iface, pathSize );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_persistent_local_storage_XPersistentLocalStorageGetSpaceInfo( IXPersistentLocalStorageImpl3 *iface, XPersistentLocalStorageSpaceInfo *info )
{
    FIXME( "iface %p, info %p stub!\n", iface, info );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_persistent_local_storage_XPersistentLocalStorageMountForPackage( IXPersistentLocalStorageImpl3 *iface, const char *packageIdentifier, XPackageMountHandle *mountHandle )
{
    FIXME( "iface %p, packageIdentifier %s, mountHandle %p stub!\n", iface, debugstr_a( packageIdentifier ), mountHandle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_persistent_local_storage_XPersistentLocalStoragePromptUserForSpaceAsync( IXPersistentLocalStorageImpl3 *iface, UINT64 requestedBytes, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, requestedBytes %llu, asyncBlock %p stub!\n", iface, requestedBytes, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_persistent_local_storage_XPersistentLocalStoragePromptUserForSpaceResult( IXPersistentLocalStorageImpl3 *iface, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, asyncBlock %p stub!\n", iface, asyncBlock );
    return E_NOTIMPL;
}

static const struct IXPersistentLocalStorageImpl3Vtbl x_persistent_local_storage_vtbl =
{
    x_persistent_local_storage_QueryInterface,
    x_persistent_local_storage_AddRef,
    x_persistent_local_storage_Release,
    /* IXPersistentLocalStorageImpl/IXPersistentLocalStorageImpl2 methods */
    x_persistent_local_storage_XPersistentLocalStorageGetPathSize,
    x_persistent_local_storage_XPersistentLocalStorageGetPath,
    x_persistent_local_storage_XPersistentLocalStorageGetSpaceInfo,
    x_persistent_local_storage_XPersistentLocalStoragePromptUserForSpaceAsync,
    x_persistent_local_storage_XPersistentLocalStoragePromptUserForSpaceResult,
    /* IXPersistentLocalStorageImpl3 methods */
    x_persistent_local_storage_XPersistentLocalStorageMountForPackage,
};

static struct x_persistent_local_storage x_persistent_local_storage =
{
    {&x_persistent_local_storage_vtbl},
    0,
};

IXPersistentLocalStorageImpl *x_persistent_local_storage_impl = (IXPersistentLocalStorageImpl *)&x_persistent_local_storage.IXPersistentLocalStorageImpl3_iface;
