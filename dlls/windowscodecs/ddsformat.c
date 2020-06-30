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

#define DDS_BLOCK_WIDTH  4
#define DDS_BLOCK_HEIGHT 4

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

typedef struct dds_info {
    UINT width;
    UINT height;
    UINT depth;
    UINT mip_levels;
    UINT array_size;
    UINT frame_count;
    DXGI_FORMAT format;
    WICDdsDimension dimension;
    WICDdsAlphaMode alpha_mode;
    const GUID *pixel_format;
} dds_info;

typedef struct dds_frame_info {
    UINT width;
    UINT height;
    DXGI_FORMAT format;
    UINT bytes_per_block;
    UINT block_width;
    UINT block_height;
    UINT width_in_blocks;
    UINT height_in_blocks;
    const GUID *pixel_format;
} dds_frame_info;

typedef struct DdsDecoder {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICDdsDecoder IWICDdsDecoder_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    CRITICAL_SECTION lock;
    DDS_HEADER header;
    DDS_HEADER_DXT10 header_dxt10;
    dds_info info;
} DdsDecoder;

typedef struct DdsFrameDecode {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICDdsFrameDecode IWICDdsFrameDecode_iface;
    LONG ref;
    BYTE *data;
    dds_frame_info info;
} DdsFrameDecode;

static HRESULT WINAPI DdsDecoder_Dds_GetFrame(IWICDdsDecoder *, UINT, UINT, UINT, IWICBitmapFrameDecode **);

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

static void get_dds_info(dds_info* info, DDS_HEADER *header, DDS_HEADER_DXT10 *header_dxt10)
{
    int i;
    UINT depth;

    info->width = header->width;
    info->height = header->height;
    info->depth = 1;
    info->mip_levels = 1;
    info->array_size = 1;
    if (header->depth) info->depth = header->depth;
    if (header->mipMapCount) info->mip_levels = header->mipMapCount;

    if (has_extended_header(header)) {
        if (header_dxt10->arraySize) info->array_size = header_dxt10->arraySize;
        info->format = header_dxt10->dxgiFormat;
        info->dimension = get_dimension(NULL, header_dxt10);
        info->alpha_mode = header_dxt10->miscFlags2 & 0x00000008;
    } else {
        info->format = get_format_from_fourcc(header->ddspf.fourCC);
        info->dimension = get_dimension(header, NULL);
        info->alpha_mode = get_alpha_mode_from_fourcc(header->ddspf.fourCC);
    }
    info->pixel_format = (info->alpha_mode == WICDdsAlphaModePremultiplied) ?
                         &GUID_WICPixelFormat32bppPBGRA : &GUID_WICPixelFormat32bppBGRA;

    /* get frame count */
    if (info->depth == 1) {
        info->frame_count = info->array_size * info->mip_levels;
    } else {
        info->frame_count = 0;
        depth = info->depth;
        for (i = 0; i < info->mip_levels; i++)
        {
            info->frame_count += depth;
            if (depth > 1) depth /= 2;
        }
        info->frame_count *= info->array_size;
    }
}

static UINT get_bytes_per_block(DXGI_FORMAT format)
{
    switch(format)
    {
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return 16;
        default:
            WARN("DXGI format 0x%x is not supported in DDS decoder\n", format);
            return 0;
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

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This->data);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI DdsFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
                                             UINT *puiWidth, UINT *puiHeight)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    if (!puiWidth || !puiHeight) return E_INVALIDARG;

    *puiWidth = This->info.width;
    *puiHeight = This->info.height;

    TRACE("(%p) -> (%d,%d)\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
                                                    WICPixelFormatGUID *pPixelFormat)
{
    DdsFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    if (!pPixelFormat) return E_INVALIDARG;

    *pPixelFormat = *This->info.pixel_format;

    TRACE("(%p) -> %s\n", iface, debugstr_guid(pPixelFormat));

    return S_OK;
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
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);

    if (!widthInBlocks || !heightInBlocks) return E_INVALIDARG;

    *widthInBlocks = This->info.width_in_blocks;
    *heightInBlocks = This->info.height_in_blocks;

    TRACE("(%p,%p,%p) -> (%d,%d)\n", iface, widthInBlocks, heightInBlocks, *widthInBlocks, *heightInBlocks);

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_Dds_GetFormatInfo(IWICDdsFrameDecode *iface,
                                                       WICDdsFormatInfo *formatInfo)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);

    if (!formatInfo) return E_INVALIDARG;

    formatInfo->DxgiFormat = This->info.format;
    formatInfo->BytesPerBlock = This->info.bytes_per_block;
    formatInfo->BlockWidth = This->info.block_width;
    formatInfo->BlockHeight = This->info.block_height;

    TRACE("(%p,%p) -> (0x%x,%d,%d,%d)\n", iface, formatInfo,
          formatInfo->DxgiFormat, formatInfo->BytesPerBlock, formatInfo->BlockWidth, formatInfo->BlockHeight);

    return S_OK;
}

static HRESULT WINAPI DdsFrameDecode_Dds_CopyBlocks(IWICDdsFrameDecode *iface,
                                                    const WICRect *boundsInBlocks, UINT stride, UINT bufferSize,
                                                    BYTE *buffer)
{
    DdsFrameDecode *This = impl_from_IWICDdsFrameDecode(iface);
    int x, y, width, height;
    UINT bytes_per_block, frame_stride, frame_size, i;
    BYTE *data, *dst_buffer;

    TRACE("(%p,%p,%u,%u,%p)\n", iface, boundsInBlocks, stride, bufferSize, buffer);

    if (!buffer) return E_INVALIDARG;

    bytes_per_block = This->info.bytes_per_block;
    frame_stride = This->info.width_in_blocks * bytes_per_block;
    frame_size = frame_stride * This->info.height_in_blocks;
    if (!boundsInBlocks) {
        if (stride < frame_stride) return E_INVALIDARG;
        if (bufferSize < frame_size) return E_INVALIDARG;
        memcpy(buffer, This->data, frame_size);
        return S_OK;
    }

    x = boundsInBlocks->X;
    y = boundsInBlocks->Y;
    width = boundsInBlocks->Width;
    height = boundsInBlocks->Height;
    if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
        x + width > This->info.width_in_blocks ||
        y + height > This->info.height_in_blocks) {
        return E_INVALIDARG;
    }
    if (stride < width * bytes_per_block) return E_INVALIDARG;
    if (bufferSize < stride * height) return E_INVALIDARG;

    data = This->data + (x + y * This->info.width_in_blocks) * bytes_per_block;
    dst_buffer = buffer;
    for (i = 0; i < height; i++)
    {
        memcpy(dst_buffer, data, (size_t)width * bytes_per_block);
        data += This->info.width_in_blocks * bytes_per_block;
        dst_buffer += stride;
    }

    return S_OK;
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

    get_dds_info(&This->info, &This->header, &This->header_dxt10);

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

    if (!pCount) return E_INVALIDARG;
    if (!This->initialized) return WINCODEC_ERR_WRONGSTATE;

    EnterCriticalSection(&This->lock);

    *pCount = This->info.frame_count;

    LeaveCriticalSection(&This->lock);

    TRACE("(%p) -> %d\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI DdsDecoder_GetFrame(IWICBitmapDecoder *iface,
                                          UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    DdsDecoder *This = impl_from_IWICBitmapDecoder(iface);
    UINT frame_per_texture, array_index, mip_level, slice_index, depth;

    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!ppIBitmapFrame) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized) {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    frame_per_texture = This->info.frame_count / This->info.array_size;
    array_index = index / frame_per_texture;
    slice_index = index % frame_per_texture;
    depth = This->info.depth;
    mip_level = 0;
    while (slice_index >= depth)
    {
        slice_index -= depth;
        mip_level++;
        if (depth > 1) depth /= 2;
    }

    LeaveCriticalSection(&This->lock);

    return DdsDecoder_Dds_GetFrame(&This->IWICDdsDecoder_iface, array_index, mip_level, slice_index, ppIBitmapFrame);
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

    parameters->Width = This->info.width;
    parameters->Height = This->info.height;
    parameters->Depth = This->info.depth;
    parameters->MipLevels = This->info.mip_levels;
    parameters->ArraySize = This->info.array_size;
    parameters->DxgiFormat = This->info.format;
    parameters->Dimension = This->info.dimension;
    parameters->AlphaMode = This->info.alpha_mode;

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
    DdsDecoder *This = impl_from_IWICDdsDecoder(iface);
    HRESULT hr;
    LARGE_INTEGER seek;
    UINT width, height, depth, width_in_blocks, height_in_blocks, size;
    UINT frame_width = 0, frame_height = 0, frame_width_in_blocks = 0, frame_height_in_blocks = 0, frame_size = 0;
    UINT bytes_per_block, bytesread, i;
    DdsFrameDecode *frame_decode = NULL;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, arrayIndex, mipLevel, sliceIndex, bitmapFrame);

    if (!bitmapFrame) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized) {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    if (arrayIndex >= This->info.array_size || mipLevel >= This->info.mip_levels || sliceIndex >= This->info.depth) {
        hr = E_INVALIDARG;
        goto end;
    }

    bytes_per_block = get_bytes_per_block(This->info.format);
    seek.QuadPart = sizeof(DWORD) + sizeof(DDS_HEADER);
    if (has_extended_header(&This->header)) seek.QuadPart += sizeof(DDS_HEADER_DXT10);

    width = This->info.width;
    height = This->info.height;
    depth = This->info.depth;
    for (i = 0; i < This->info.mip_levels; i++)
    {
        width_in_blocks = (width + DDS_BLOCK_WIDTH - 1) / DDS_BLOCK_WIDTH;
        height_in_blocks = (height + DDS_BLOCK_HEIGHT - 1) / DDS_BLOCK_HEIGHT;
        size = width_in_blocks * height_in_blocks * bytes_per_block;

        if (i < mipLevel)  {
            seek.QuadPart += size * depth;
        } else if (i == mipLevel){
            seek.QuadPart += size * sliceIndex;
            frame_width = width;
            frame_height = height;
            frame_width_in_blocks = width_in_blocks;
            frame_height_in_blocks = height_in_blocks;
            frame_size = frame_width_in_blocks * frame_height_in_blocks * bytes_per_block;
            if (arrayIndex == 0) break;
        }
        seek.QuadPart += arrayIndex * size * depth;

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
        if (depth > 1) depth /= 2;
    }

    hr = DdsFrameDecode_CreateInstance(&frame_decode);
    if (hr != S_OK) goto end;
    frame_decode->info.width = frame_width;
    frame_decode->info.height = frame_height;
    frame_decode->info.format = This->info.format;
    frame_decode->info.bytes_per_block = bytes_per_block;
    frame_decode->info.block_width = DDS_BLOCK_WIDTH;
    frame_decode->info.block_height = DDS_BLOCK_HEIGHT;
    frame_decode->info.width_in_blocks = frame_width_in_blocks;
    frame_decode->info.height_in_blocks = frame_height_in_blocks;
    frame_decode->info.pixel_format = This->info.pixel_format;
    frame_decode->data = HeapAlloc(GetProcessHeap(), 0, frame_size);
    hr = IStream_Seek(This->stream, seek, SEEK_SET, NULL);
    if (hr != S_OK) goto end;
    hr = IStream_Read(This->stream, frame_decode->data, frame_size, &bytesread);
    if (hr != S_OK || bytesread != frame_size) {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }
    *bitmapFrame = &frame_decode->IWICBitmapFrameDecode_iface;

    hr = S_OK;

end:
    LeaveCriticalSection(&This->lock);

    if (hr != S_OK && frame_decode) DdsFrameDecode_Release(&frame_decode->IWICBitmapFrameDecode_iface);

    return hr;
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
