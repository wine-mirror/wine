/*
 * Copyright 2010 Damjan Jovanovic
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#ifdef HAVE_ICNS_H
#include <icns.h>
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBICNS

static void *libicns_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(icns_create_family);
MAKE_FUNCPTR(icns_export_family_data);
MAKE_FUNCPTR(icns_free_image);
MAKE_FUNCPTR(icns_get_mask_type_for_icon_type);
MAKE_FUNCPTR(icns_get_type_from_image_info);
MAKE_FUNCPTR(icns_init_image_for_type);
MAKE_FUNCPTR(icns_new_element_from_image);
MAKE_FUNCPTR(icns_set_element_in_family);
#undef MAKE_FUNCPTR

static void *load_libicns(void)
{
    if((libicns_handle = wine_dlopen(SONAME_LIBICNS, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libicns_handle, #f, NULL, 0)) == NULL) { \
        libicns_handle = NULL; \
        return NULL; \
    }
        LOAD_FUNCPTR(icns_create_family);
        LOAD_FUNCPTR(icns_export_family_data);
        LOAD_FUNCPTR(icns_free_image);
        LOAD_FUNCPTR(icns_get_mask_type_for_icon_type);
        LOAD_FUNCPTR(icns_get_type_from_image_info);
        LOAD_FUNCPTR(icns_init_image_for_type);
        LOAD_FUNCPTR(icns_new_element_from_image);
        LOAD_FUNCPTR(icns_set_element_in_family);
#undef LOAD_FUNCPTR
    }
    return libicns_handle;
}

typedef struct IcnsEncoder {
    const IWICBitmapEncoderVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock;
} IcnsEncoder;

static HRESULT WINAPI IcnsEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IcnsEncoder_AddRef(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcnsEncoder_Release(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream)
            IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcnsEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    IcnsEncoder *This = (IcnsEncoder*)iface;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI IcnsEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    FIXME("(%p,%p,%p): stub\n", iface, ppIFrameEncode, ppIEncoderOptions);

    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_Commit(IWICBitmapEncoder *iface)
{
    FIXME("(%p): stub\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl IcnsEncoder_Vtbl = {
    IcnsEncoder_QueryInterface,
    IcnsEncoder_AddRef,
    IcnsEncoder_Release,
    IcnsEncoder_Initialize,
    IcnsEncoder_GetContainerFormat,
    IcnsEncoder_GetEncoderInfo,
    IcnsEncoder_SetColorContexts,
    IcnsEncoder_SetPalette,
    IcnsEncoder_SetThumbnail,
    IcnsEncoder_SetPreview,
    IcnsEncoder_CreateNewFrame,
    IcnsEncoder_Commit,
    IcnsEncoder_GetMetadataQueryWriter
};

HRESULT IcnsEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    IcnsEncoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!libicns_handle && !load_libicns())
    {
        ERR("Failed writing ICNS because unable to find %s\n",SONAME_LIBICNS);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(IcnsEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &IcnsEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IcnsEncoder.lock");

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

#else /* !HAVE_ICNS_H */

HRESULT IcnsEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to save ICNS picture, but ICNS support is not compiled in.\n");
    return E_FAIL;
}

#endif
