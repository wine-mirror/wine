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

typedef struct {
    DWORD bc2Size;
    DWORD bc2Width;
    DWORD bc2Height;
    WORD  bc2Planes;
    WORD  bc2BitCount;
    DWORD bc2Compression;
    DWORD bc2SizeImage;
    DWORD bc2XRes;
    DWORD bc2YRes;
    DWORD bc2ClrUsed;
    DWORD bc2ClrImportant;
    /* same as BITMAPINFOHEADER until this point */
    WORD  bc2ResUnit;
    WORD  bc2Reserved;
    WORD  bc2Orientation;
    WORD  bc2Halftoning;
    DWORD bc2HalftoneSize1;
    DWORD bc2HalftoneSize2;
    DWORD bc2ColorSpace;
    DWORD bc2AppData;
} BITMAPCOREHEADER2;

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    BITMAPFILEHEADER bfh;
    BITMAPV5HEADER bih;
} BmpDecoder;

static HRESULT BmpDecoder_ReadHeaders(BmpDecoder* This, IStream *stream)
{
    HRESULT hr;
    ULONG bytestoread, bytesread;

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    hr = IStream_Read(stream, &This->bfh, sizeof(BITMAPFILEHEADER), &bytesread);
    if (FAILED(hr)) return hr;
    if (bytesread != sizeof(BITMAPFILEHEADER) ||
        This->bfh.bfType != 0x4d42 /* "BM" */) return E_FAIL;

    hr = IStream_Read(stream, &This->bih.bV5Size, sizeof(DWORD), &bytesread);
    if (FAILED(hr)) return hr;
    if (bytesread != sizeof(DWORD) ||
        (This->bih.bV5Size != sizeof(BITMAPCOREHEADER) &&
         This->bih.bV5Size != sizeof(BITMAPCOREHEADER2) &&
         This->bih.bV5Size != sizeof(BITMAPINFOHEADER) &&
         This->bih.bV5Size != sizeof(BITMAPV4HEADER) &&
         This->bih.bV5Size != sizeof(BITMAPV5HEADER))) return E_FAIL;

    bytestoread = This->bih.bV5Size-sizeof(DWORD);
    hr = IStream_Read(stream, &This->bih.bV5Width, bytestoread, &bytesread);
    if (FAILED(hr)) return hr;
    if (bytestoread != bytesread) return E_FAIL;

    This->initialized = TRUE;

    return S_OK;
}

HRESULT WINAPI BmpDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    BmpDecoder *This = (BmpDecoder*)iface;
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

static ULONG WINAPI BmpDecoder_AddRef(IWICBitmapDecoder *iface)
{
    BmpDecoder *This = (BmpDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpDecoder_Release(IWICBitmapDecoder *iface)
{
    BmpDecoder *This = (BmpDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    HRESULT hr;
    BmpDecoder *This = (BmpDecoder*)iface;

    hr = BmpDecoder_ReadHeaders(This, pIStream);

    if (SUCCEEDED(hr))
    {
        This->stream = pIStream;
        IStream_AddRef(pIStream);
    }

    return hr;
}

static HRESULT WINAPI BmpDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatBmp, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI BmpDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI BmpDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    FIXME("(%p,%u,%p): stub\n", iface, index, ppIBitmapFrame);
    return E_NOTIMPL;
}

static const IWICBitmapDecoderVtbl BmpDecoder_Vtbl = {
    BmpDecoder_QueryInterface,
    BmpDecoder_AddRef,
    BmpDecoder_Release,
    BmpDecoder_QueryCapability,
    BmpDecoder_Initialize,
    BmpDecoder_GetContainerFormat,
    BmpDecoder_GetDecoderInfo,
    BmpDecoder_CopyPalette,
    BmpDecoder_GetMetadataQueryReader,
    BmpDecoder_GetPreview,
    BmpDecoder_GetColorContexts,
    BmpDecoder_GetThumbnail,
    BmpDecoder_GetFrameCount,
    BmpDecoder_GetFrame
};

HRESULT BmpDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    BmpDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BmpDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &BmpDecoder_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
