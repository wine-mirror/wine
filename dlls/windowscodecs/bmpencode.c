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
#include "winreg.h"
#include "wingdi.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct BmpFrameEncode {
    const IWICBitmapFrameEncodeVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    BOOL initialized;
} BmpFrameEncode;

static HRESULT WINAPI BmpFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
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

static ULONG WINAPI BmpFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    This->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    FIXME("(%p,%u,%u): stub\n", iface, uiWidth, uiHeight);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    FIXME("(%p,%0.2f,%0.2f): stub\n", iface, dpiX, dpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pPixelFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    FIXME("(%p,%u,%u,%u,%p): stub\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIBitmapSource, prc);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl BmpFrameEncode_Vtbl = {
    BmpFrameEncode_QueryInterface,
    BmpFrameEncode_AddRef,
    BmpFrameEncode_Release,
    BmpFrameEncode_Initialize,
    BmpFrameEncode_SetSize,
    BmpFrameEncode_SetResolution,
    BmpFrameEncode_SetPixelFormat,
    BmpFrameEncode_SetColorContexts,
    BmpFrameEncode_SetPalette,
    BmpFrameEncode_SetThumbnail,
    BmpFrameEncode_WritePixels,
    BmpFrameEncode_WriteSource,
    BmpFrameEncode_Commit,
    BmpFrameEncode_GetMetadataQueryWriter
};

typedef struct BmpEncoder {
    const IWICBitmapEncoderVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    IWICBitmapFrameEncode *frame;
} BmpEncoder;

static HRESULT WINAPI BmpEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    BmpEncoder *This = (BmpEncoder*)iface;
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

static ULONG WINAPI BmpEncoder_AddRef(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpEncoder_Release(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        if (This->frame) IWICBitmapFrameEncode_Release(This->frame);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    BmpEncoder *This = (BmpEncoder*)iface;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    BmpFrameEncode *encode;
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    if (This->frame) return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    if (!This->stream) return WINCODEC_ERR_NOTINITIALIZED;

    hr = CreatePropertyBag2(ppIEncoderOptions);
    if (FAILED(hr)) return hr;

    encode = HeapAlloc(GetProcessHeap(), 0, sizeof(BmpFrameEncode));
    if (!encode)
    {
        IPropertyBag2_Release(*ppIEncoderOptions);
        *ppIEncoderOptions = NULL;
        return E_OUTOFMEMORY;
    }
    encode->lpVtbl = &BmpFrameEncode_Vtbl;
    encode->ref = 2;
    IStream_AddRef(This->stream);
    encode->stream = This->stream;
    encode->initialized = FALSE;

    *ppIFrameEncode = (IWICBitmapFrameEncode*)encode;
    This->frame = (IWICBitmapFrameEncode*)encode;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_Commit(IWICBitmapEncoder *iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl BmpEncoder_Vtbl = {
    BmpEncoder_QueryInterface,
    BmpEncoder_AddRef,
    BmpEncoder_Release,
    BmpEncoder_Initialize,
    BmpEncoder_GetContainerFormat,
    BmpEncoder_GetEncoderInfo,
    BmpEncoder_SetColorContexts,
    BmpEncoder_SetPalette,
    BmpEncoder_SetThumbnail,
    BmpEncoder_SetPreview,
    BmpEncoder_CreateNewFrame,
    BmpEncoder_Commit,
    BmpEncoder_GetMetadataQueryWriter
};

HRESULT BmpEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    BmpEncoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BmpEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &BmpEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->frame = NULL;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
