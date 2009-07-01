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

struct BmpFrameDecode;
typedef HRESULT (*ReadDataFunc)(struct BmpFrameDecode* This);

typedef struct BmpFrameDecode {
    const IWICBitmapFrameDecodeVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    BITMAPFILEHEADER bfh;
    BITMAPV5HEADER bih;
    const WICPixelFormatGUID *pixelformat;
    int bitsperpixel;
    ReadDataFunc read_data_func;
    INT stride;
    BYTE *imagedata;
    BYTE *imagedatastart;
} BmpFrameDecode;

static HRESULT WINAPI BmpFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
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

static ULONG WINAPI BmpFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This->imagedata);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        *puiWidth = bch->bcWidth;
        *puiHeight = bch->bcHeight;
    }
    else
    {
        *puiWidth = This->bih.bV5Width;
        *puiHeight = abs(This->bih.bV5Height);
    }
    return S_OK;
}

static HRESULT WINAPI BmpFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    memcpy(pPixelFormat, This->pixelformat, sizeof(GUID));

    return S_OK;
}

static HRESULT BmpHeader_GetResolution(BITMAPV5HEADER *bih, double *pDpiX, double *pDpiY)
{
    switch (bih->bV5Size)
    {
    case sizeof(BITMAPCOREHEADER):
        *pDpiX = 96.0;
        *pDpiY = 96.0;
        return S_OK;
    case sizeof(BITMAPCOREHEADER2):
    case sizeof(BITMAPINFOHEADER):
    case sizeof(BITMAPV4HEADER):
    case sizeof(BITMAPV5HEADER):
        *pDpiX = bih->bV5XPelsPerMeter * 0.0254;
        *pDpiY = bih->bV5YPelsPerMeter * 0.0254;
        return S_OK;
    default:
        return E_FAIL;
    }
}

static HRESULT WINAPI BmpFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    return BmpHeader_GetResolution(&This->bih, pDpiX, pDpiY);
}

static HRESULT WINAPI BmpFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    BmpFrameDecode *This = (BmpFrameDecode*)iface;
    HRESULT hr;
    UINT width, height;
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (!This->imagedata)
    {
        hr = This->read_data_func(This);
        if (FAILED(hr)) return hr;
    }

    hr = BmpFrameDecode_GetSize(iface, &width, &height);
    if (FAILED(hr)) return hr;

    return copy_pixels(This->bitsperpixel, This->imagedatastart,
        width, height, This->stride,
        prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI BmpFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return E_NOTIMPL;
}

static HRESULT BmpFrameDecode_ReadUncompressed(BmpFrameDecode* This)
{
    UINT bytesperrow;
    UINT width, height;
    UINT datasize;
    int bottomup;
    HRESULT hr;
    LARGE_INTEGER offbits;
    ULONG bytesread;

    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        width = bch->bcWidth;
        height = bch->bcHeight;
        bottomup = 1;
    }
    else
    {
        width = This->bih.bV5Width;
        height = abs(This->bih.bV5Height);
        bottomup = (This->bih.bV5Height > 0);
    }

    /* row sizes in BMP files must be divisible by 4 bytes */
    bytesperrow = (((width * This->bitsperpixel)+31)/32)*4;
    datasize = bytesperrow * height;

    This->imagedata = HeapAlloc(GetProcessHeap(), 0, datasize);
    if (!This->imagedata) return E_OUTOFMEMORY;

    offbits.QuadPart = This->bfh.bfOffBits;
    hr = IStream_Seek(This->stream, offbits, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto fail;

    hr = IStream_Read(This->stream, This->imagedata, datasize, &bytesread);
    if (FAILED(hr) || bytesread != datasize) goto fail;

    if (bottomup)
    {
        This->imagedatastart = This->imagedata + (height-1) * bytesperrow;
        This->stride = -bytesperrow;
    }
    else
    {
        This->imagedatastart = This->imagedata;
        This->stride = bytesperrow;
    }
    return S_OK;

fail:
    HeapFree(GetProcessHeap(), 0, This->imagedata);
    This->imagedata = NULL;
    if (SUCCEEDED(hr)) hr = E_FAIL;
    return hr;
}

static HRESULT BmpFrameDecode_ReadUnsupported(BmpFrameDecode* This)
{
    return E_FAIL;
}

static const IWICBitmapFrameDecodeVtbl BmpFrameDecode_Vtbl = {
    BmpFrameDecode_QueryInterface,
    BmpFrameDecode_AddRef,
    BmpFrameDecode_Release,
    BmpFrameDecode_GetSize,
    BmpFrameDecode_GetPixelFormat,
    BmpFrameDecode_GetResolution,
    BmpFrameDecode_CopyPalette,
    BmpFrameDecode_CopyPixels,
    BmpFrameDecode_GetMetadataQueryReader,
    BmpFrameDecode_GetColorContexts,
    BmpFrameDecode_GetThumbnail
};

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    BITMAPFILEHEADER bfh;
    BITMAPV5HEADER bih;
    BmpFrameDecode *framedecode;
    const WICPixelFormatGUID *pixelformat;
    int bitsperpixel;
    ReadDataFunc read_data_func;
} BmpDecoder;

static HRESULT BmpDecoder_ReadHeaders(BmpDecoder* This, IStream *stream)
{
    HRESULT hr;
    ULONG bytestoread, bytesread;
    LARGE_INTEGER seek;

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    seek.QuadPart = 0;
    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;

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

    /* decide what kind of bitmap this is and how/if we can read it */
    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        TRACE("BITMAPCOREHEADER with depth=%i\n", bch->bcBitCount);
        This->bitsperpixel = bch->bcBitCount;
        This->read_data_func = BmpFrameDecode_ReadUncompressed;
        switch(bch->bcBitCount)
        {
        case 1:
            This->pixelformat = &GUID_WICPixelFormat1bppIndexed;
            break;
        case 2:
            This->pixelformat = &GUID_WICPixelFormat2bppIndexed;
            break;
        case 4:
            This->pixelformat = &GUID_WICPixelFormat4bppIndexed;
            break;
        case 8:
            This->pixelformat = &GUID_WICPixelFormat8bppIndexed;
            break;
        case 24:
            This->pixelformat = &GUID_WICPixelFormat24bppBGR;
            break;
        default:
            This->pixelformat = &GUID_WICPixelFormatUndefined;
            WARN("unsupported bit depth %i for BITMAPCOREHEADER\n", bch->bcBitCount);
            break;
        }
    }
    else /* struct is compatible with BITMAPINFOHEADER */
    {
        TRACE("bitmap header=%i compression=%i depth=%i\n", This->bih.bV5Size, This->bih.bV5Compression, This->bih.bV5BitCount);
        switch(This->bih.bV5Compression)
        {
        case BI_RGB:
            This->bitsperpixel = This->bih.bV5BitCount;
            This->read_data_func = BmpFrameDecode_ReadUncompressed;
            switch(This->bih.bV5BitCount)
            {
            case 1:
                This->pixelformat = &GUID_WICPixelFormat1bppIndexed;
                break;
            case 2:
                This->pixelformat = &GUID_WICPixelFormat2bppIndexed;
                break;
            case 4:
                This->pixelformat = &GUID_WICPixelFormat4bppIndexed;
                break;
            case 8:
                This->pixelformat = &GUID_WICPixelFormat8bppIndexed;
                break;
            case 16:
                This->pixelformat = &GUID_WICPixelFormat16bppBGR555;
                break;
            case 24:
                This->pixelformat = &GUID_WICPixelFormat24bppBGR;
                break;
            case 32:
                This->pixelformat = &GUID_WICPixelFormat32bppBGR;
                break;
            default:
                This->pixelformat = &GUID_WICPixelFormatUndefined;
                FIXME("unsupported bit depth %i for uncompressed RGB\n", This->bih.bV5BitCount);
            }
            break;
        default:
            This->bitsperpixel = 0;
            This->read_data_func = BmpFrameDecode_ReadUnsupported;
            This->pixelformat = &GUID_WICPixelFormatUndefined;
            FIXME("unsupported bitmap type header=%i compression=%i depth=%i\n", This->bih.bV5Size, This->bih.bV5Compression, This->bih.bV5BitCount);
            break;
        }
    }

    This->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
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
        if (This->framedecode) IUnknown_Release((IUnknown*)This->framedecode);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    HRESULT hr;
    BmpDecoder *This = (BmpDecoder*)iface;

    hr = BmpDecoder_ReadHeaders(This, pIStream);
    if (FAILED(hr)) return hr;

    if (This->read_data_func == BmpFrameDecode_ReadUnsupported)
        *pdwCapability = 0;
    else
        *pdwCapability = WICBitmapDecoderCapabilityCanDecodeAllImages;

    return S_OK;
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
    BmpDecoder *This = (BmpDecoder*)iface;

    if (index != 0) return E_INVALIDARG;

    if (!This->stream) return WINCODEC_ERR_WRONGSTATE;

    if (!This->framedecode)
    {
        This->framedecode = HeapAlloc(GetProcessHeap(), 0, sizeof(BmpFrameDecode));
        if (!This->framedecode) return E_OUTOFMEMORY;

        This->framedecode->lpVtbl = &BmpFrameDecode_Vtbl;
        This->framedecode->ref = 1;
        This->framedecode->stream = This->stream;
        IStream_AddRef(This->stream);
        This->framedecode->bfh = This->bfh;
        This->framedecode->bih = This->bih;
        This->framedecode->pixelformat = This->pixelformat;
        This->framedecode->bitsperpixel = This->bitsperpixel;
        This->framedecode->read_data_func = This->read_data_func;
        This->framedecode->imagedata = NULL;
    }

    *ppIBitmapFrame = (IWICBitmapFrameDecode*)This->framedecode;
    IWICBitmapFrameDecode_AddRef((IWICBitmapFrameDecode*)This->framedecode);

    return S_OK;
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
    This->framedecode = NULL;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
