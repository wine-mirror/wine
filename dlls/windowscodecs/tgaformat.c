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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#include "pshpack1.h"

typedef struct {
    BYTE id_length;
    BYTE colormap_type;
    BYTE image_type;
    /* Colormap Specification */
    WORD colormap_firstentry;
    WORD colormap_length;
    BYTE colormap_entrysize;
    /* Image Specification */
    WORD xorigin;
    WORD yorigin;
    WORD width;
    WORD height;
    BYTE depth;
    BYTE image_descriptor;
} tga_header;

#define IMAGETYPE_COLORMAPPED 1
#define IMAGETYPE_TRUECOLOR 2
#define IMAGETYPE_GRAYSCALE 3
#define IMAGETYPE_RLE 8

#define IMAGE_ATTRIBUTE_BITCOUNT_MASK 0xf
#define IMAGE_RIGHTTOLEFT 0x10
#define IMAGE_TOPTOBOTTOM 0x20

#include "poppack.h"

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    const IWICBitmapFrameDecodeVtbl *lpFrameVtbl;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    tga_header header;
    CRITICAL_SECTION lock;
} TgaDecoder;

static inline TgaDecoder *decoder_from_frame(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, TgaDecoder, lpFrameVtbl);
}

static HRESULT WINAPI TgaDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    TgaDecoder *This = (TgaDecoder*)iface;
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

static ULONG WINAPI TgaDecoder_AddRef(IWICBitmapDecoder *iface)
{
    TgaDecoder *This = (TgaDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TgaDecoder_Release(IWICBitmapDecoder *iface)
{
    TgaDecoder *This = (TgaDecoder*)iface;
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

static HRESULT WINAPI TgaDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    TgaDecoder *This = (TgaDecoder*)iface;
    HRESULT hr=S_OK;
    DWORD bytesread;
    LARGE_INTEGER seek;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

    hr = IStream_Read(pIStream, &This->header, sizeof(tga_header), &bytesread);
    if (SUCCEEDED(hr) && bytesread != sizeof(tga_header))
    {
        TRACE("got only %u bytes\n", bytesread);
        hr = E_FAIL;
    }
    if (FAILED(hr)) goto end;

    TRACE("imagetype=%u, colormap type=%u, depth=%u, image descriptor=0x%x\n",
        This->header.image_type, This->header.colormap_type,
        This->header.depth, This->header.image_descriptor);

    /* Sanity checking. Since TGA has no clear identifying markers, we need
     * to be careful to not load a non-TGA image. */
    switch (This->header.image_type)
    {
    case IMAGETYPE_COLORMAPPED:
    case IMAGETYPE_COLORMAPPED|IMAGETYPE_RLE:
        if (This->header.colormap_type != 1)
            hr = E_FAIL;
        break;
    case IMAGETYPE_TRUECOLOR:
    case IMAGETYPE_TRUECOLOR|IMAGETYPE_RLE:
        if (This->header.colormap_type != 0 && This->header.colormap_type != 1)
            hr = E_FAIL;
        break;
    case IMAGETYPE_GRAYSCALE:
    case IMAGETYPE_GRAYSCALE|IMAGETYPE_RLE:
        if (This->header.colormap_type != 0)
            hr = E_FAIL;
        break;
    default:
        hr = E_FAIL;
    }

    if (This->header.depth != 8 && This->header.depth != 16 &&
        This->header.depth != 24 && This->header.depth != 32)
        hr = E_FAIL;

    if ((This->header.image_descriptor & IMAGE_ATTRIBUTE_BITCOUNT_MASK) > 8 ||
        (This->header.image_descriptor & 0xc0) != 0)
        hr = E_FAIL;

    if (FAILED(hr))
    {
        WARN("bad tga header\n");
        goto end;
    }

    /* FIXME: Read footer if there is one. */

    IStream_AddRef(pIStream);
    This->stream = pIStream;
    This->initialized = TRUE;

end:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI TgaDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_WineContainerFormatTga, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TgaDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI TgaDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI TgaDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    TgaDecoder *This = (TgaDecoder*)iface;
    TRACE("(%p,%p)\n", iface, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_NOTINITIALIZED;

    if (index != 0) return E_INVALIDARG;

    IWICBitmapDecoder_AddRef(iface);
    *ppIBitmapFrame = (IWICBitmapFrameDecode*)&This->lpFrameVtbl;

    return S_OK;
}

static const IWICBitmapDecoderVtbl TgaDecoder_Vtbl = {
    TgaDecoder_QueryInterface,
    TgaDecoder_AddRef,
    TgaDecoder_Release,
    TgaDecoder_QueryCapability,
    TgaDecoder_Initialize,
    TgaDecoder_GetContainerFormat,
    TgaDecoder_GetDecoderInfo,
    TgaDecoder_CopyPalette,
    TgaDecoder_GetMetadataQueryReader,
    TgaDecoder_GetPreview,
    TgaDecoder_GetColorContexts,
    TgaDecoder_GetThumbnail,
    TgaDecoder_GetFrameCount,
    TgaDecoder_GetFrame
};

static HRESULT WINAPI TgaDecoder_Frame_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TgaDecoder_Frame_AddRef(IWICBitmapFrameDecode *iface)
{
    TgaDecoder *This = decoder_from_frame(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI TgaDecoder_Frame_Release(IWICBitmapFrameDecode *iface)
{
    TgaDecoder *This = decoder_from_frame(iface);
    return IUnknown_Release((IUnknown*)This);
}

static HRESULT WINAPI TgaDecoder_Frame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_Frame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    TgaDecoder *This = decoder_from_frame(iface);
    int attribute_bitcount;

    TRACE("(%p,%p)\n", iface, pPixelFormat);

    attribute_bitcount = This->header.image_descriptor & IMAGE_ATTRIBUTE_BITCOUNT_MASK;

    if (attribute_bitcount)
    {
        FIXME("Need to read footer to find meaning of attribute bits\n");
        return E_NOTIMPL;
    }

    switch (This->header.image_type & ~IMAGETYPE_RLE)
    {
    case IMAGETYPE_COLORMAPPED:
        switch (This->header.depth)
        {
        case 8:
            memcpy(pPixelFormat, &GUID_WICPixelFormat8bppIndexed, sizeof(GUID));
            break;
        default:
            FIXME("Unhandled indexed color depth %u\n", This->header.depth);
            return E_NOTIMPL;
        }
        break;
    case IMAGETYPE_TRUECOLOR:
        switch (This->header.depth)
        {
        case 16:
            memcpy(pPixelFormat, &GUID_WICPixelFormat16bppBGR555, sizeof(GUID));
            break;
        case 24:
            memcpy(pPixelFormat, &GUID_WICPixelFormat24bppBGR, sizeof(GUID));
            break;
        default:
            FIXME("Unhandled truecolor depth %u\n", This->header.depth);
            return E_NOTIMPL;
        }
        break;
    case IMAGETYPE_GRAYSCALE:
        switch (This->header.depth)
        {
        case 8:
            memcpy(pPixelFormat, &GUID_WICPixelFormat8bppGray, sizeof(GUID));
            break;
        case 16:
            memcpy(pPixelFormat, &GUID_WICPixelFormat16bppGray, sizeof(GUID));
            break;
        default:
            FIXME("Unhandled grayscale depth %u\n", This->header.depth);
            return E_NOTIMPL;
        }
        break;
    default:
        ERR("Unknown image type %u\n", This->header.image_type);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI TgaDecoder_Frame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_Frame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_Frame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%p,%u,%u,%p):stub\n", iface, prc, cbStride, cbBufferSize, pbBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_Frame_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_Frame_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_Frame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl TgaDecoder_Frame_Vtbl = {
    TgaDecoder_Frame_QueryInterface,
    TgaDecoder_Frame_AddRef,
    TgaDecoder_Frame_Release,
    TgaDecoder_Frame_GetSize,
    TgaDecoder_Frame_GetPixelFormat,
    TgaDecoder_Frame_GetResolution,
    TgaDecoder_Frame_CopyPalette,
    TgaDecoder_Frame_CopyPixels,
    TgaDecoder_Frame_GetMetadataQueryReader,
    TgaDecoder_Frame_GetColorContexts,
    TgaDecoder_Frame_GetThumbnail
};

HRESULT TgaDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    TgaDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TgaDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &TgaDecoder_Vtbl;
    This->lpFrameVtbl = &TgaDecoder_Frame_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TgaDecoder.lock");

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
