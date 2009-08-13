/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#ifdef HAVE_PNG_H
#include <png.h>
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

#ifdef SONAME_LIBPNG

static void *libpng_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(png_create_read_struct);
MAKE_FUNCPTR(png_create_info_struct);
#undef MAKE_FUNCPTR

static void *load_libpng(void)
{
    if((libpng_handle = wine_dlopen(SONAME_LIBPNG, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libpng_handle, #f, NULL, 0)) == NULL) { \
        libpng_handle = NULL; \
        return NULL; \
    }
        LOAD_FUNCPTR(png_create_read_struct);
        LOAD_FUNCPTR(png_create_info_struct);

#undef LOAD_FUNCPTR
    }
    return libpng_handle;
}

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
} PngDecoder;

static HRESULT WINAPI PngDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    PngDecoder *This = (PngDecoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICBitmapDecoder, iid))
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

static ULONG WINAPI PngDecoder_AddRef(IWICBitmapDecoder *iface)
{
    PngDecoder *This = (PngDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PngDecoder_Release(IWICBitmapDecoder *iface)
{
    PngDecoder *This = (PngDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PngDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    FIXME("(%p,%p,%x): stub\n", iface, pIStream, cacheOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI PngDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI PngDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    FIXME("(%p,%p): stub\n", iface, pCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    FIXME("(%p,%u,%p): stub\n", iface, index, ppIBitmapFrame);
    return E_NOTIMPL;
}

static const IWICBitmapDecoderVtbl PngDecoder_Vtbl = {
    PngDecoder_QueryInterface,
    PngDecoder_AddRef,
    PngDecoder_Release,
    PngDecoder_QueryCapability,
    PngDecoder_Initialize,
    PngDecoder_GetContainerFormat,
    PngDecoder_GetDecoderInfo,
    PngDecoder_CopyPalette,
    PngDecoder_GetMetadataQueryReader,
    PngDecoder_GetPreview,
    PngDecoder_GetColorContexts,
    PngDecoder_GetThumbnail,
    PngDecoder_GetFrameCount,
    PngDecoder_GetFrame
};

HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    PngDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!libpng_handle && !load_libpng())
    {
        ERR("Failed reading PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PngDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &PngDecoder_Vtbl;
    This->ref = 1;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

#else /* !HAVE_PNG_H */

HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to load PNG picture, but PNG supported not compiled in.\n");
    return E_FAIL;
}

#endif
