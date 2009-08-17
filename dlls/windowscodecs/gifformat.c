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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
} GifDecoder;

static HRESULT WINAPI GifDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    GifDecoder *This = (GifDecoder*)iface;
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

static ULONG WINAPI GifDecoder_AddRef(IWICBitmapDecoder *iface)
{
    GifDecoder *This = (GifDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifDecoder_Release(IWICBitmapDecoder *iface)
{
    GifDecoder *This = (GifDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI GifDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    FIXME("(%p,%p,%x): stub\n", iface, pIStream, cacheOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI GifDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI GifDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI GifDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    FIXME("(%p,%p): stub\n", iface, pCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    FIXME("(%p,%u,%p): stub\n", iface, index, ppIBitmapFrame);
    return E_NOTIMPL;
}

static const IWICBitmapDecoderVtbl GifDecoder_Vtbl = {
    GifDecoder_QueryInterface,
    GifDecoder_AddRef,
    GifDecoder_Release,
    GifDecoder_QueryCapability,
    GifDecoder_Initialize,
    GifDecoder_GetContainerFormat,
    GifDecoder_GetDecoderInfo,
    GifDecoder_CopyPalette,
    GifDecoder_GetMetadataQueryReader,
    GifDecoder_GetPreview,
    GifDecoder_GetColorContexts,
    GifDecoder_GetThumbnail,
    GifDecoder_GetFrameCount,
    GifDecoder_GetFrame
};

HRESULT GifDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    GifDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(GifDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &GifDecoder_Vtbl;
    This->ref = 1;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
