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

#include "pshpack1.h"

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} ICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
} ICONHEADER;

#include "poppack.h"

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    ICONHEADER header;
} IcoDecoder;

typedef struct {
    const IWICBitmapFrameDecodeVtbl *lpVtbl;
    LONG ref;
    ICONDIRENTRY entry;
    IcoDecoder *parent;
} IcoFrameDecode;

static HRESULT WINAPI IcoFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    IcoFrameDecode *This = (IcoFrameDecode*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
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

static ULONG WINAPI IcoFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    IcoFrameDecode *This = (IcoFrameDecode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcoFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    IcoFrameDecode *This = (IcoFrameDecode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        IUnknown_Release((IUnknown*)This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcoFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    IcoFrameDecode *This = (IcoFrameDecode*)iface;

    *puiWidth = This->entry.bWidth ? This->entry.bWidth : 256;
    *puiHeight = This->entry.bHeight ? This->entry.bHeight : 256;

    TRACE("(%p) -> (%i,%i)\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI IcoFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    memcpy(pPixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI IcoFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcoFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI IcoFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%p,%u,%u,%p): stub\n", iface, prc, cbStride, cbBufferSize, pbBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcoFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl IcoFrameDecode_Vtbl = {
    IcoFrameDecode_QueryInterface,
    IcoFrameDecode_AddRef,
    IcoFrameDecode_Release,
    IcoFrameDecode_GetSize,
    IcoFrameDecode_GetPixelFormat,
    IcoFrameDecode_GetResolution,
    IcoFrameDecode_CopyPalette,
    IcoFrameDecode_CopyPixels,
    IcoFrameDecode_GetMetadataQueryReader,
    IcoFrameDecode_GetColorContexts,
    IcoFrameDecode_GetThumbnail
};

static HRESULT WINAPI IcoDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    IcoDecoder *This = (IcoDecoder*)iface;
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

static ULONG WINAPI IcoDecoder_AddRef(IWICBitmapDecoder *iface)
{
    IcoDecoder *This = (IcoDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcoDecoder_Release(IWICBitmapDecoder *iface)
{
    IcoDecoder *This = (IcoDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcoDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcoDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    IcoDecoder *This = (IcoDecoder*)iface;
    LARGE_INTEGER seek;
    HRESULT hr;
    ULONG bytesread;
    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;

    hr = IStream_Read(pIStream, &This->header, sizeof(ICONHEADER), &bytesread);
    if (FAILED(hr)) return hr;
    if (bytesread != sizeof(ICONHEADER) ||
        This->header.idReserved != 0 ||
        This->header.idType != 1) return E_FAIL;

    This->initialized = TRUE;
    This->stream = pIStream;
    IStream_AddRef(pIStream);

    return S_OK;
}

static HRESULT WINAPI IcoDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%p): stub\n", iface, pguidContainerFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcoDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcoDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI IcoDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI IcoDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    IcoDecoder *This = (IcoDecoder*)iface;
    TRACE("(%p,%p)\n", iface, pCount);

    if (!This->initialized) return WINCODEC_ERR_NOTINITIALIZED;

    *pCount = This->header.idCount;
    TRACE("<-- %u\n", *pCount);

    return S_OK;
}

static HRESULT WINAPI IcoDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    IcoDecoder *This = (IcoDecoder*)iface;
    IcoFrameDecode *result;
    LARGE_INTEGER seek;
    HRESULT hr;
    ULONG bytesread;
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_NOTINITIALIZED;

    if (This->header.idCount < index) return E_INVALIDARG;

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(IcoFrameDecode));
    if (!result) return E_OUTOFMEMORY;

    result->lpVtbl = &IcoFrameDecode_Vtbl;
    result->ref = 1;
    result->parent = This;

    /* read the icon entry */
    seek.QuadPart = sizeof(ICONHEADER) + sizeof(ICONDIRENTRY) * index;
    hr = IStream_Seek(This->stream, seek, STREAM_SEEK_SET, 0);
    if (FAILED(hr)) goto fail;

    hr = IStream_Read(This->stream, &result->entry, sizeof(ICONDIRENTRY), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(ICONDIRENTRY)) goto fail;

    IWICBitmapDecoder_AddRef(iface);

    *ppIBitmapFrame = (IWICBitmapFrameDecode*)result;

    return S_OK;

fail:
    HeapFree(GetProcessHeap(), 0, result);
    if (SUCCEEDED(hr)) hr = E_FAIL;
    TRACE("<-- %x\n", hr);
    return hr;
}

static const IWICBitmapDecoderVtbl IcoDecoder_Vtbl = {
    IcoDecoder_QueryInterface,
    IcoDecoder_AddRef,
    IcoDecoder_Release,
    IcoDecoder_QueryCapability,
    IcoDecoder_Initialize,
    IcoDecoder_GetContainerFormat,
    IcoDecoder_GetDecoderInfo,
    IcoDecoder_CopyPalette,
    IcoDecoder_GetMetadataQueryReader,
    IcoDecoder_GetPreview,
    IcoDecoder_GetColorContexts,
    IcoDecoder_GetThumbnail,
    IcoDecoder_GetFrameCount,
    IcoDecoder_GetFrame
};

HRESULT IcoDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    IcoDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(IcoDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &IcoDecoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->initialized = FALSE;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
