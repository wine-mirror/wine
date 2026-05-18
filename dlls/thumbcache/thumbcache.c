/*
* Copyright 2026 Alistair Leslie-Hughes
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "oleidl.h"
#include "rpcproxy.h"
#include "thumbcache.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thumbcache);

struct thumb_cache
{
    IThumbnailCache IThumbnailCache_iface;
    LONG ref;
};

static struct thumb_cache *impl_from_IThumbnailCache( IThumbnailCache *iface )
{
    return CONTAINING_RECORD( iface, struct thumb_cache, IThumbnailCache_iface );
}

static HRESULT WINAPI thumbcache_QueryInterface(IThumbnailCache *iface, REFIID iid, void **obj)
{
    struct thumb_cache *This = impl_from_IThumbnailCache(iface);

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IThumbnailCache ))
    {
        IThumbnailCache_AddRef( iface );
        *obj = &This->IThumbnailCache_iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI thumbcache_AddRef(IThumbnailCache *iface)
{
    struct thumb_cache *This = impl_from_IThumbnailCache(iface);
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %lu\n", This, ref );
    return ref;
}

static ULONG WINAPI thumbcache_Release(IThumbnailCache *iface)
{
    struct thumb_cache *This = impl_from_IThumbnailCache(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %lu\n", This, ref );

    if (!ref)
    {
        free( This );
    }

    return ref;
}

static HRESULT WINAPI thumbcache_GetThumbnail(IThumbnailCache *iface, IShellItem *pShellItem, UINT cxyRequestedThumbSize,
    WTS_FLAGS flags, ISharedBitmap **ppvThumb, WTS_CACHEFLAGS *pOutFlags, WTS_THUMBNAILID *pThumbnailID)
{
    struct thumb_cache *This = impl_from_IThumbnailCache(iface);

    FIXME("%p, %p, %d, %x, %p, %p, %p\n", This, pShellItem, cxyRequestedThumbSize, flags, ppvThumb, pOutFlags, pThumbnailID );

    return E_NOTIMPL;
}

static HRESULT WINAPI thumbcache_GetThumbnailByID(IThumbnailCache *iface, WTS_THUMBNAILID thumbnailID,
    UINT cxyRequestedThumbSize, ISharedBitmap **ppvThumb, WTS_CACHEFLAGS *pOutFlags)
{
    struct thumb_cache *This = impl_from_IThumbnailCache(iface);

    FIXME("%p, %p, %u, %p, %p\n", This, &thumbnailID, cxyRequestedThumbSize, ppvThumb, pOutFlags );

    return E_NOTIMPL;
}

static const struct IThumbnailCacheVtbl thumb_cache_vtbl =
{
    thumbcache_QueryInterface,
    thumbcache_AddRef,
    thumbcache_Release,
    thumbcache_GetThumbnail,
    thumbcache_GetThumbnailByID
};

static HRESULT thumbcache_create( IUnknown *outer, REFIID iid, void **obj )
 {
    struct thumb_cache *This = malloc( sizeof(*This) );
    HRESULT hr = E_FAIL;

    if (!This) return E_OUTOFMEMORY;
    This->IThumbnailCache_iface.lpVtbl = &thumb_cache_vtbl;
    This->ref = 1;

    hr = IThumbnailCache_QueryInterface( &This->IThumbnailCache_iface, iid, obj );
    IThumbnailCache_Release( &This->IThumbnailCache_iface );
    return hr;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *, REFIID, void **);
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI class_factory_QueryInterface( IClassFactory *iface,
                                                    REFIID iid, void **obj )
{
    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI class_factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance( IClassFactory *iface,
                                                    IUnknown *outer, REFIID iid,
                                                    void **obj )
{
    struct class_factory *This = impl_from_IClassFactory( iface );

    TRACE( "%p %s %p\n", outer, debugstr_guid( iid ), obj );

    *obj = NULL;
    return This->create_instance( outer, iid, obj );
}

static HRESULT WINAPI class_factory_LockServer( IClassFactory *iface,
                                                BOOL lock )
{
    FIXME( "%d: stub!\n", lock );
    return S_OK;
}

static const struct IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer
};

static struct class_factory thumb_cache_cf       = { { &class_factory_vtbl }, thumbcache_create };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    IClassFactory *cf = NULL;

    TRACE( "(%s %s %p)\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (IsEqualCLSID( clsid, &CLSID_LocalThumbnailCache ))
        cf = &thumb_cache_cf.IClassFactory_iface;
    else
        FIXME( "(%s %s %p) unsupported interface\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}
