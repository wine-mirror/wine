/*
 * Copyright 2020 Ziqing Hui
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

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#define DDS_MAGIC 0x20534444
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#define DDPF_ALPHAPIXELS     0x00000001
#define DDPF_ALPHA           0x00000002
#define DDPF_FOURCC          0x00000004
#define DDPF_PALETTEINDEXED8 0x00000020
#define DDPF_RGB             0x00000040
#define DDPF_LUMINANCE       0x00020000
#define DDPF_BUMPDUDV        0x00080000

#define DDSCAPS2_CUBEMAP 0x00000200
#define DDSCAPS2_VOLUME  0x00200000

#define DDS_DIMENSION_TEXTURE1D 2
#define DDS_DIMENSION_TEXTURE2D 3
#define DDS_DIMENSION_TEXTURE3D 4

#define DDS_RESOURCE_MISC_TEXTURECUBE 0x00000004

typedef struct {
    DWORD size;
    DWORD flags;
    DWORD fourCC;
    DWORD rgbBitCount;
    DWORD rBitMask;
    DWORD gBitMask;
    DWORD bBitMask;
    DWORD aBitMask;
} DDS_PIXELFORMAT;

typedef struct {
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitchOrLinearSize;
    DWORD depth;
    DWORD mipMapCount;
    DWORD reserved1[11];
    DDS_PIXELFORMAT ddspf;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
} DDS_HEADER;

typedef struct {
    DWORD dxgiFormat;
    DWORD resourceDimension;
    DWORD miscFlag;
    DWORD arraySize;
    DWORD miscFlags2;
} DDS_HEADER_DXT10;

typedef struct DdsDecoder {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICDdsDecoder IWICDdsDecoder_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    CRITICAL_SECTION lock;
    DDS_HEADER header;
    DDS_HEADER_DXT10 header_dxt10;
} DdsDecoder;

typedef struct DdsFrameDecode {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICDdsFrameDecode IWICDdsFrameDecode_iface;
    LONG ref;
} DdsFrameDecode;

static inline BOOL has_extended_header(DDS_HEADER *header)
{
    return (header->ddspf.flags & DDPF_FOURCC) &&
           (header->ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0'));
}

static WICDdsDimension get_dimension(DDS_HEADER *header, DDS_HEADER_DXT10 *header_dxt10)
{
    if (header_dxt10) {
        if (header_dxt10->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) return WICDdsTextureCube;
        switch (header_dxt10->resourceDimension)
        {
        case DDS_DIMENSION_TEXTURE1D: return WICDdsTexture1D;
        case DDS_DIMENSION_TEXTURE2D: return WICDdsTexture2D;
        case DDS_DIMENSION_TEXTURE3D: return WICDdsTexture3D;
        default: return WICDdsTexture2D;
        }
    } else {
        if (header->caps2 & DDSCAPS2_CUBEMAP) {
            return WICDdsTextureCube;
        } else if (header->caps2 & DDSCAPS2_VOLUME) {
            return WICDdsTexture3D;
        } else {
            return WICDdsTexture2D;
        }
    }
}

static DXGI_FORMAT get_format_from_fourcc(DWORD fourcc)
{
    switch (fourcc)
    {
    case MAKEFOURCC('D', 'X', 'T', '1'):
        return DXGI_FORMAT_BC1_UNORM;
    case MAKEFOURCC('D', 'X', 'T', '2'):
    case MAKEFOURCC('D', 'X', 'T', '3'):
        return DXGI_FORMAT_BC2_UNORM;
    case MAKEFOURCC('D', 'X', 'T', '4'):
    case MAKEFOURCC('D', 'X', 'T', '5'):
        return DXGI_FORMAT_BC3_UNORM;
    case MAKEFOURCC('D', 'X', '1', '0'):
        /* format is indicated in extended header */
        return DXGI_FORMAT_UNKNOWN;
    default:
        /* there are DDS files where fourCC is set directly to DXGI_FORMAT enumeration value */
        return fourcc;
    }
}

static WICDdsAlphaMode get_alpha_mode_from_fourcc(DWORD fourcc)
{
    switch (fourcc)
    {
    case MAKEFOURCC('D', 'X', 'T', '1'):
    case MAKEFOURCC('D', 'X', 'T', '2'):
    case MAKEFOURCC('D', 'X', 'T', '4'):
        return WICDdsAlphaModePremultiplied;
    case MAKEFOURCC('D', 'X', 'T', '3'):
    case MAKEFOURCC('D', 'X', 'T', '5'):
        return WICDdsAlphaModeStraight;
    default:
        return WICDdsAlphaModeUnknown;
    }
}

static inline DdsDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, DdsDecoder, IWICBitmapDecoder_iface);
}

static inline DdsDecoder *impl_from_IWICDdsDecoder(IWICDdsDecoder *iface)
{
    return CONTAINING_RECORD(iface, DdsDecoder, IWICDdsDecoder_iface);
}

static inline DdsFrameDecode *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, DdsFrameDecode, IWICBitmapFrameDecode_iface);
}

static inline DdsFrameDecode *impl_from_IWICDdsFrameDecode(IWICDdsFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, DdsFrameDecode, IWICDdsFrameDecode_iface);
}

static HRESULT WINAPI DdsFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
                                                    void **ppv)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid)) {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    } else if (IsEqualGUID(&IID_IWICDdsFrameDecode, iid)) {
        *ppv = &This->IWICDdsFrameDecode_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0) HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI DdsFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
                                             UINT *puiWidth, UINT *puiHeight)
{
    FIXME("(%p,%p,%p): stub.\n", iface, puiWidth, puiHeight);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
                                                    WICPixelFormatGUID *pPixelFormat)
{
    FIXME("(%p,%p): stub.\n", iface, pPixelFormat);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
                                                   double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub.\n", iface, pDpiX, pDpiY);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
                                                 IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub.\n", iface, pIPalette);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
                                                const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FIXME("(%p,%s,%u,%u,%p): stub.\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
                                                            IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub.\n", iface, ppIMetadataQueryReader);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
                                                      UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub.\n", iface, cCount, ppIColorContexts, pcActualCount);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
                                                  IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub.\n", iface, ppIThumbnail);

    return E_NOTIMPL;
}

static const IWICBitmapFrameDecodeVtbl DdsFrameDecode_Vtbl = {
    DdsFrameDecode_QueryInterface,
    DdsFrameDecode_AddRef,
    DdsFrameDecode_Release,
    DdsFrameDecode_GetSize,
    DdsFrameDecode_GetPixelFormat,
    DdsFrameDecode_GetResolution,
    DdsFrameDecode_CopyPalette,
    DdsFrameDecode_CopyPixels,
    DdsFrameDecode_GetMetadataQueryReader,
    DdsFrameDecode_GetColorContexts,
    DdsFrameDecode_GetThumbnail
};

static HRESULT WINAPI DdsFrameDecode_Dds_QueryInterface(IWICDdsFrameDecode *iface,
                                                        REFIID iid, void **ppv)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    return DdsFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI DdsFrameDecode_Dds_AddRef(IWICDdsFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    return DdsFrameDecode_AddRef(&This->IWICBitmapFrameDecode_iface);
}

static ULONG WINAPI DdsFrameDecode_Dds_Release(IWICDdsFrameDecode *iface)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    return DdsFrameDecode_Release(&This->IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI DdsFrameDecode_Dds_GetSizeInBlocks(IWICDdsFrameDecode *iface,
                                                         UINT *widthInBlocks, UINT *heightInBlocks)
{
    FIXME("(%p,%p,%p): stub.\n", iface, widthInBlocks, heightInBlocks);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_Dds_GetFormatInfo(IWICDdsFrameDecode *iface,
                                                       WICDdsFormatInfo *formatInfo)
{
    FIXME("(%p,%p): stub.\n", iface, formatInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsFrameDecode_Dds_CopyBlocks(IWICDdsFrameDecode *iface,
                                                    const WICRect *boundsInBlocks, UINT stride, UINT bufferSize,
                                                    BYTE *buffer)
{
    FIXME("(%p,%p,%u,%u,%p): stub.\n", iface, boundsInBlocks, stride, bufferSize, buffer);

    return E_NOTIMPL;
}

static const IWICDdsFrameDecodeVtbl DdsFrameDecode_Dds_Vtbl = {
    DdsFrameDecode_Dds_QueryInterface,
    DdsFrameDecode_Dds_AddRef,
    DdsFrameDecode_Dds_Release,
    DdsFrameDecode_Dds_GetSizeInBlocks,
    DdsFrameDecode_Dds_GetFormatInfo,
    DdsFrameDecode_Dds_CopyBlocks
};

static HRESULT DdsFrameDecode_CreateInstance(DdsFrameDecode **frame_decode)
{
    DdsFrameDecode *result;

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(*result));
    if (!result) return E_OUTOFMEMORY;

    result->IWICBitmapFrameDecode_iface.lpVtbl = &DdsFrameDecode_Vtbl;
    result->IWICDdsFrameDecode_iface.lpVtbl = &DdsFrameDecode_Dds_Vtbl;
    result->ref = 1;

    *frame_decode = result;
    return S_OK;
}

static HRESULT WINAPI DdsDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
                                                void **ppv)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoder, iid)) {
        *ppv = &This->IWICBitmapDecoder_iface;
    } else if (IsEqualIID(&IID_IWICDdsDecoder, iid)) {
        *ppv = &This->IWICDdsDecoder_iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DdsDecoder_AddRef(IWICBitmapDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI DdsDecoder_Release(IWICBitmapDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI DdsDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
                                                 DWORD *capability)
{
    FIXME("(%p,%p,%p): stub.\n", iface, stream, capability);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
                                            WICDecodeOptions cacheOptions)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr;
    LARGE_INTEGER seek;
    DWORD magic;
    ULONG bytesread;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

    hr = IStream_Read(pIStream, &magic, sizeof(magic), &bytesread);
    if (FAILED(hr)) goto end;
    if (bytesread != sizeof(magic)) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    if (magic != DDS_MAGIC) {
        hr = WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
        goto end;
    }

    hr = IStream_Read(pIStream, &This->header, sizeof(This->header), &bytesread);
    if (FAILED(hr)) goto end;
    if (bytesread != sizeof(This->header)) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    if (This->header.size != sizeof(This->header)) {
        hr = WINCODEC_ERR_BADHEADER;
        goto end;
    }

    if (has_extended_header(&This->header)) {
        hr = IStream_Read(pIStream, &This->header_dxt10, sizeof(This->header_dxt10), &bytesread);
        if (FAILED(hr)) goto end;
        if (bytesread != sizeof(This->header_dxt10)) {
            hr = WINCODEC_ERR_STREAMREAD;
            goto end;
        }
    }

    This->initialized = TRUE;
    This->stream = pIStream;
    IStream_AddRef(pIStream);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
                                                    GUID *pguidContainerFormat)
{
    TRACE("(%p,%p)\n", iface, pguidContainerFormat);

    memcpy(pguidContainerFormat, &GUID_ContainerFormatDds, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI DdsDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
                                                IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WICDdsDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI DdsDecoder_CopyPalette(IWICBitmapDecoder *iface,
                                             IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);

    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI DdsDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
                                                        IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    if (!ppIMetadataQueryReader) return E_INVALIDARG;

    FIXME("(%p,%p)\n", iface, ppIMetadataQueryReader);

    return E_NOTIMPL;
}

static HRESULT WINAPI DdsDecoder_GetPreview(IWICBitmapDecoder *iface,
                                            IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsDecoder_GetColorContexts(IWICBitmapDecoder *iface,
                                                  UINT cCount, IWICColorContext **ppDdslorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppDdslorContexts, pcActualCount);

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI DdsDecoder_GetThumbnail(IWICBitmapDecoder *iface,
                                              IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI DdsDecoder_GetFrameCount(IWICBitmapDecoder *iface,
                                               UINT *pCount)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    UINT arraySize = 1, mipMapCount = 1, depth = 1;
    int i;

    if (!pCount) return E_INVALIDARG;
    if (!This->initialized) return WINCODEC_ERR_WRONGSTATE;

    EnterCriticalSection(&This->lock);

    if (This->header.mipMapCount) mipMapCount = This->header.mipMapCount;
    if (This->header.depth) depth = This->header.depth;
    if (has_extended_header(&This->header) && This->header_dxt10.arraySize) arraySize = This->header_dxt10.arraySize;

    if (depth == 1) {
        *pCount = arraySize * mipMapCount;
    } else {
        *pCount = 0;
        for (i = 0; i < mipMapCount; i++)
        {
            *pCount += depth;
            if (depth > 1) depth /= 2;
        }
        *pCount *= arraySize;
    }

    LeaveCriticalSection(&This->lock);

    TRACE("(%p) <-- %d\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI DdsDecoder_GetFrame(IWICBitmapDecoder *iface,
                                          UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    HRESULT hr;
    DdsFrameDecode *frame_decode;

    FIXME("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    hr = DdsFrameDecode_CreateInstance(&frame_decode);
    if (hr == S_OK) *ppIBitmapFrame = &frame_decode->IWICBitmapFrameDecode_iface;

    return hr;
}

static const IWICBitmapDecoderVtbl DdsDecoder_Vtbl = {
        DdsDecoder_QueryInterface,
        DdsDecoder_AddRef,
        DdsDecoder_Release,
        DdsDecoder_QueryCapability,
        DdsDecoder_Initialize,
        DdsDecoder_GetContainerFormat,
        DdsDecoder_GetDecoderInfo,
        DdsDecoder_CopyPalette,
        DdsDecoder_GetMetadataQueryReader,
        DdsDecoder_GetPreview,
        DdsDecoder_GetColorContexts,
        DdsDecoder_GetThumbnail,
        DdsDecoder_GetFrameCount,
        DdsDecoder_GetFrame
};

static HRESULT WINAPI DdsDecoder_Dds_QueryInterface(IWICDdsDecoder *iface,
                                                    REFIID iid, void **ppv)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    return DdsDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
}

static ULONG WINAPI DdsDecoder_Dds_AddRef(IWICDdsDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    return DdsDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI DdsDecoder_Dds_Release(IWICDdsDecoder *iface)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    return DdsDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI DdsDecoder_Dds_GetParameters(IWICDdsDecoder *iface,
                                                   WICDdsParameters *parameters)
{
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    HRESULT hr;

    if (!parameters) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    parameters->Width = This->header.width;
    parameters->Height = This->header.height;
    parameters->Depth = 1;
    parameters->MipLevels = 1;
    parameters->ArraySize = 1;
    if (This->header.depth) parameters->Depth = This->header.depth;
    if (This->header.mipMapCount) parameters->MipLevels = This->header.mipMapCount;

    if (has_extended_header(&This->header)) {
        if (This->header_dxt10.arraySize) parameters->ArraySize = This->header_dxt10.arraySize;
        parameters->DxgiFormat = This->header_dxt10.dxgiFormat;
        parameters->Dimension = get_dimension(NULL, &This->header_dxt10);
        parameters->AlphaMode = This->header_dxt10.miscFlags2 & 0x00000008;
    } else {
        parameters->DxgiFormat = get_format_from_fourcc(This->header.ddspf.fourCC);
        parameters->Dimension = get_dimension(&This->header, NULL);
        parameters->AlphaMode = get_alpha_mode_from_fourcc(This->header.ddspf.fourCC);
    }

    TRACE("(%p) -> (%dx%d depth=%d mipLevels=%d arraySize=%d dxgiFormat=0x%x dimension=0x%x alphaMode=0x%x)\n",
          iface, parameters->Width, parameters->Height, parameters->Depth, parameters->MipLevels,
          parameters->ArraySize, parameters->DxgiFormat, parameters->Dimension, parameters->AlphaMode);

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI DdsDecoder_Dds_GetFrame(IWICDdsDecoder *iface,
                                              UINT arrayIndex, UINT mipLevel, UINT sliceIndex,
                                              IWICBitmapFrameDecode **bitmapFrame)
{
    TRACE("(%p,%u,%u,%u,%p): Stub.\n", iface, arrayIndex, mipLevel, sliceIndex, bitmapFrame);

    return E_NOTIMPL;
}

static const IWICDdsDecoderVtbl DdsDecoder_Dds_Vtbl = {
    DdsDecoder_Dds_QueryInterface,
    DdsDecoder_Dds_AddRef,
    DdsDecoder_Dds_Release,
    DdsDecoder_Dds_GetParameters,
    DdsDecoder_Dds_GetFrame
};

HRESULT DdsDecoder_CreateInstance(REFIID iid, void** ppv)
{
    DdsDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(DdsDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &DdsDecoder_Vtbl;
    This->IWICDdsDecoder_iface.lpVtbl = &DdsDecoder_Dds_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DdsDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}
