/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
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

#ifdef SONAME_LIBTIFF

static CRITICAL_SECTION init_tiff_cs;
static CRITICAL_SECTION_DEBUG init_tiff_cs_debug =
{
    0, 0, &init_tiff_cs,
    { &init_tiff_cs_debug.ProcessLocksList,
      &init_tiff_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_tiff_cs") }
};
static CRITICAL_SECTION init_tiff_cs = { &init_tiff_cs_debug, -1, 0, 0, 0, 0 };

static void *libtiff_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(TIFFClientOpen);
#undef MAKE_FUNCPTR

static void *load_libtiff(void)
{
    void *result;

    EnterCriticalSection(&init_tiff_cs);

    if (!libtiff_handle &&
        (libtiff_handle = wine_dlopen(SONAME_LIBTIFF, RTLD_NOW, NULL, 0)) != NULL)
    {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libtiff_handle, #f, NULL, 0)) == NULL) { \
        ERR("failed to load symbol %s\n", #f); \
        libtiff_handle = NULL; \
        LeaveCriticalSection(&init_tiff_cs); \
        return NULL; \
    }
        LOAD_FUNCPTR(TIFFClientOpen);
#undef LOAD_FUNCPTR

    }

    result = libtiff_handle;

    LeaveCriticalSection(&init_tiff_cs);
    return result;
}

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
} TiffDecoder;

static HRESULT WINAPI TiffDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    TiffDecoder *This = (TiffDecoder*)iface;
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

static ULONG WINAPI TiffDecoder_AddRef(IWICBitmapDecoder *iface)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffDecoder_Release(IWICBitmapDecoder *iface)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TiffDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    FIXME("(%p,%p,%x): stub\n", iface, pIStream, cacheOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TiffDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI TiffDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    FIXME("(%p,%p)\n", iface, pCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    FIXME("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);
    return E_NOTIMPL;
}

static const IWICBitmapDecoderVtbl TiffDecoder_Vtbl = {
    TiffDecoder_QueryInterface,
    TiffDecoder_AddRef,
    TiffDecoder_Release,
    TiffDecoder_QueryCapability,
    TiffDecoder_Initialize,
    TiffDecoder_GetContainerFormat,
    TiffDecoder_GetDecoderInfo,
    TiffDecoder_CopyPalette,
    TiffDecoder_GetMetadataQueryReader,
    TiffDecoder_GetPreview,
    TiffDecoder_GetColorContexts,
    TiffDecoder_GetThumbnail,
    TiffDecoder_GetFrameCount,
    TiffDecoder_GetFrame
};

HRESULT TiffDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    HRESULT ret;
    TiffDecoder *This;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!load_libtiff())
    {
        ERR("Failed reading TIFF because unable to load %s\n",SONAME_LIBTIFF);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TiffDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &TiffDecoder_Vtbl;
    This->ref = 1;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

#else /* !SONAME_LIBTIFF */

HRESULT TiffDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to load TIFF picture, but Wine was compiled without TIFF support.\n");
    return E_FAIL;
}

#endif
