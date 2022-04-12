/*
 * Copyright 2020 Ziqing Hui for CodeWeavers
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

#include "wine/debug.h"
#include "wine/heap.h"

#define COBJMACROS

#include "d3d10_1.h"
#include "d3dx10.h"
#include "wincodec.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT sdk_version, IWICImagingFactory **imaging_factory);

static const struct
{
    const GUID *wic_container_guid;
    D3DX10_IMAGE_FILE_FORMAT d3dx_file_format;
}
file_formats[] =
{
    { &GUID_ContainerFormatBmp,  D3DX10_IFF_BMP },
    { &GUID_ContainerFormatJpeg, D3DX10_IFF_JPG },
    { &GUID_ContainerFormatPng,  D3DX10_IFF_PNG },
    { &GUID_ContainerFormatDds,  D3DX10_IFF_DDS },
    { &GUID_ContainerFormatTiff, D3DX10_IFF_TIFF },
    { &GUID_ContainerFormatGif,  D3DX10_IFF_GIF },
    { &GUID_ContainerFormatWmp,  D3DX10_IFF_WMP },
};

static const struct
{
    const GUID *wic_guid;
    DXGI_FORMAT dxgi_format;
}
wic_pixel_formats[] =
{
    { &GUID_WICPixelFormatBlackWhite,         DXGI_FORMAT_R1_UNORM },
    { &GUID_WICPixelFormat8bppAlpha,          DXGI_FORMAT_A8_UNORM },
    { &GUID_WICPixelFormat8bppGray,           DXGI_FORMAT_R8_UNORM },
    { &GUID_WICPixelFormat16bppGray,          DXGI_FORMAT_R16_UNORM },
    { &GUID_WICPixelFormat16bppGrayHalf,      DXGI_FORMAT_R16_FLOAT },
    { &GUID_WICPixelFormat32bppGrayFloat,     DXGI_FORMAT_R32_FLOAT },
    { &GUID_WICPixelFormat16bppBGR565,        DXGI_FORMAT_B5G6R5_UNORM },
    { &GUID_WICPixelFormat16bppBGRA5551,      DXGI_FORMAT_B5G5R5A1_UNORM },
    { &GUID_WICPixelFormat32bppBGR,           DXGI_FORMAT_B8G8R8X8_UNORM },
    { &GUID_WICPixelFormat32bppBGRA,          DXGI_FORMAT_B8G8R8A8_UNORM },
    { &GUID_WICPixelFormat32bppRGBA,          DXGI_FORMAT_R8G8B8A8_UNORM },
    { &GUID_WICPixelFormat32bppRGBA1010102,   DXGI_FORMAT_R10G10B10A2_UNORM },
    { &GUID_WICPixelFormat32bppRGBA1010102XR, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
    { &GUID_WICPixelFormat64bppRGBA,          DXGI_FORMAT_R16G16B16A16_UNORM },
    { &GUID_WICPixelFormat64bppRGBAHalf,      DXGI_FORMAT_R16G16B16A16_FLOAT },
    { &GUID_WICPixelFormat96bppRGBFloat,      DXGI_FORMAT_R32G32B32_FLOAT },
    { &GUID_WICPixelFormat128bppRGBAFloat,    DXGI_FORMAT_R32G32B32A32_FLOAT }
};

static const DXGI_FORMAT block_compressed_formats[] =
{
    DXGI_FORMAT_BC1_TYPELESS,  DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,  DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,  DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,  DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,  DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,  DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
};

static const DXGI_FORMAT to_be_converted_format[] =
{
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_B5G6R5_UNORM,
    DXGI_FORMAT_B4G4R4A4_UNORM,
    DXGI_FORMAT_B5G5R5A1_UNORM,
    DXGI_FORMAT_B8G8R8X8_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM
};

static D3DX10_IMAGE_FILE_FORMAT wic_container_guid_to_file_format(GUID *container_format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(file_formats); ++i)
    {
        if (IsEqualGUID(file_formats[i].wic_container_guid, container_format))
            return file_formats[i].d3dx_file_format;
    }
    return D3DX10_IFF_FORCE_DWORD;
}

static const GUID *dxgi_format_to_wic_guid(DXGI_FORMAT format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(wic_pixel_formats); ++i)
    {
        if (wic_pixel_formats[i].dxgi_format == format)
            return wic_pixel_formats[i].wic_guid;
    }

    return NULL;
}

static D3D10_RESOURCE_DIMENSION wic_dimension_to_d3dx10_dimension(WICDdsDimension wic_dimension)
{
    switch (wic_dimension)
    {
        case WICDdsTexture1D:
            return D3D10_RESOURCE_DIMENSION_TEXTURE1D;
        case WICDdsTexture2D:
        case WICDdsTextureCube:
            return D3D10_RESOURCE_DIMENSION_TEXTURE2D;
        case WICDdsTexture3D:
            return D3D10_RESOURCE_DIMENSION_TEXTURE3D;
        default:
            return D3D10_RESOURCE_DIMENSION_UNKNOWN;
    }
}

static BOOL is_block_compressed(DXGI_FORMAT format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(block_compressed_formats); ++i)
        if (format == block_compressed_formats[i])
            return TRUE;

    return FALSE;
}

static unsigned int get_bpp_from_format(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return 24;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;
        case DXGI_FORMAT_R1_UNORM:
            return 1;
        default:
            return 0;
    }
}

static DXGI_FORMAT get_d3dx10_dds_format(DXGI_FORMAT format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(to_be_converted_format); ++i)
    {
        if (format == to_be_converted_format[i])
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    return format;
}

static HRESULT load_file(const WCHAR *filename, void **buffer, DWORD *size)
{
    HRESULT hr = S_OK;
    DWORD bytes_read;
    HANDLE file;
    BOOL ret;

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    *size = GetFileSize(file, NULL);
    if (*size == INVALID_FILE_SIZE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    *buffer = heap_alloc(*size);
    if (!*buffer)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ret = ReadFile(file, *buffer, *size, &bytes_read, NULL);
    if (!ret)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
    if (bytes_read != *size)
    {
        hr = E_FAIL;
        goto done;
    }

done:
    if (FAILED(hr))
    {
        heap_free(*buffer);
        *buffer = NULL;
    }
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
    return hr;
}

static HRESULT load_resource(HMODULE module, HRSRC res_info, void **buffer, DWORD *size)
{
    HGLOBAL resource;

    if (!(*size = SizeofResource(module, res_info)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(resource = LoadResource(module, res_info)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(*buffer = LockResource(resource)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

HRESULT WINAPI D3DX10GetImageInfoFromFileA(const char *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result)
{
    WCHAR *buffer;
    int str_len;
    HRESULT hr;

    TRACE("src_file %s, pump %p, info %p, result %p.\n", debugstr_a(src_file), pump, info, result);

    if (!src_file || !info)
        return E_FAIL;

    str_len = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0);
    if (!str_len)
        return HRESULT_FROM_WIN32(GetLastError());

    buffer = heap_alloc(str_len * sizeof(*buffer));
    if (!buffer)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, src_file, -1, buffer, str_len);
    hr = D3DX10GetImageInfoFromFileW(buffer, pump, info, result);

    heap_free(buffer);

    return hr;
}

HRESULT WINAPI D3DX10GetImageInfoFromFileW(const WCHAR *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result)
{
    void *buffer = NULL;
    DWORD size = 0;
    HRESULT hr;

    TRACE("src_file %s, pump %p, info %p, result %p.\n", debugstr_w(src_file), pump, info, result);

    if (!src_file || !info)
        return E_FAIL;

    if (FAILED(load_file(src_file, &buffer, &size)))
        return D3D10_ERROR_FILE_NOT_FOUND;

    hr = D3DX10GetImageInfoFromMemory(buffer, size, pump, info, result);

    heap_free(buffer);

    return hr;
}

HRESULT WINAPI D3DX10GetImageInfoFromResourceA(HMODULE module, const char *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result)
{
    HRSRC res_info;
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("module %p, resource %s, pump %p, info %p, result %p.\n",
            module, debugstr_a(resource), pump, info, result);

    if (!resource || !info)
        return D3DX10_ERR_INVALID_DATA;

    res_info = FindResourceA(module, resource, (const char *)RT_RCDATA);
    if (!res_info)
    {
        /* Try loading the resource as bitmap data */
        res_info = FindResourceA(module, resource, (const char *)RT_BITMAP);
        if (!res_info)
            return D3DX10_ERR_INVALID_DATA;
    }

    hr = load_resource(module, res_info, &buffer, &size);
    if (FAILED(hr))
        return D3DX10_ERR_INVALID_DATA;

    return D3DX10GetImageInfoFromMemory(buffer, size, pump, info, result);
}

HRESULT WINAPI D3DX10GetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result)
{
    HRSRC res_info;
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("module %p, resource %s, pump %p, info %p, result %p.\n",
            module, debugstr_w(resource), pump, info, result);

    if (!resource || !info)
        return D3DX10_ERR_INVALID_DATA;

    res_info = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA);
    if (!res_info)
    {
        /* Try loading the resource as bitmap data */
        res_info = FindResourceW(module, resource, (const WCHAR *)RT_BITMAP);
        if (!res_info)
            return D3DX10_ERR_INVALID_DATA;
    }

    hr = load_resource(module, res_info, &buffer, &size);
    if (FAILED(hr))
        return D3DX10_ERR_INVALID_DATA;

    return D3DX10GetImageInfoFromMemory(buffer, size, pump, info, result);
}

HRESULT WINAPI D3DX10GetImageInfoFromMemory(const void *src_data, SIZE_T src_data_size, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *img_info, HRESULT *hresult)
{
    IWICBitmapFrameDecode *frame = NULL;
    IWICImagingFactory *factory = NULL;
    IWICDdsDecoder *dds_decoder = NULL;
    IWICBitmapDecoder *decoder = NULL;
    WICDdsParameters dds_params;
    IWICStream *stream = NULL;
    unsigned int frame_count;
    GUID container_format;
    HRESULT hr;

    TRACE("src_data %p, src_data_size %Iu, pump %p, img_info %p, hresult %p.\n",
            src_data, src_data_size, pump, img_info, hresult);

    if (!src_data || !src_data_size || !img_info)
        return E_FAIL;
    if (pump)
        FIXME("Thread pump is not supported yet.\n");

    WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    IWICImagingFactory_CreateStream(factory, &stream);
    hr = IWICStream_InitializeFromMemory(stream, (BYTE *)src_data, src_data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stream.\n");
        goto end;
    }
    hr = IWICImagingFactory_CreateDecoderFromStream(factory, (IStream *)stream, NULL, 0, &decoder);
    if (FAILED(hr))
        goto end;

    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &container_format);
    if (FAILED(hr))
        goto end;
    img_info->ImageFileFormat = wic_container_guid_to_file_format(&container_format);
    if (img_info->ImageFileFormat == D3DX10_IFF_FORCE_DWORD)
    {
        hr = E_FAIL;
        WARN("Unsupported image file format %s.\n", debugstr_guid(&container_format));
        goto end;
    }

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    if (FAILED(hr) || !frame_count)
        goto end;
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (FAILED(hr))
        goto end;
    hr = IWICBitmapFrameDecode_GetSize(frame, &img_info->Width, &img_info->Height);
    if (FAILED(hr))
        goto end;

    if (img_info->ImageFileFormat == D3DX10_IFF_DDS)
    {
        hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICDdsDecoder, (void **)&dds_decoder);
        if (FAILED(hr))
            goto end;
        hr = IWICDdsDecoder_GetParameters(dds_decoder, &dds_params);
        if (FAILED(hr))
            goto end;
        img_info->ArraySize = dds_params.ArraySize;
        img_info->Depth = dds_params.Depth;
        img_info->MipLevels = dds_params.MipLevels;
        img_info->ResourceDimension = wic_dimension_to_d3dx10_dimension(dds_params.Dimension);
        img_info->Format = get_d3dx10_dds_format(dds_params.DxgiFormat);
        img_info->MiscFlags = 0;
        if (dds_params.Dimension == WICDdsTextureCube)
        {
            img_info->MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
            img_info->ArraySize *= 6;
        }
    }
    else
    {
        img_info->ArraySize = 1;
        img_info->Depth = 1;
        img_info->MipLevels = 1;
        img_info->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
        img_info->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        img_info->MiscFlags = 0;
    }

end:
    if (dds_decoder)
        IWICDdsDecoder_Release(dds_decoder);
    if (frame)
        IWICBitmapFrameDecode_Release(frame);
    if (decoder)
        IWICBitmapDecoder_Release(decoder);
    if (stream)
        IWICStream_Release(stream);
    if (factory)
        IWICImagingFactory_Release(factory);

    if (hr != S_OK)
    {
        WARN("Invalid or unsupported image file.\n");
        return E_FAIL;
    }
    return S_OK;
}

HRESULT WINAPI D3DX10CreateTextureFromFileA(ID3D10Device *device, const char *src_file,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    WCHAR *buffer;
    int str_len;
    HRESULT hr;

    TRACE("device %p, src_file %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, debugstr_a(src_file), load_info, pump, texture, hresult);

    if (!src_file || !texture)
        return E_FAIL;

    if (!(str_len = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(buffer = heap_alloc(str_len * sizeof(*buffer))))
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, src_file, -1, buffer, str_len);
    hr = D3DX10CreateTextureFromFileW(device, buffer, load_info, pump, texture, hresult);

    heap_free(buffer);

    return hr;
}

HRESULT WINAPI D3DX10CreateTextureFromFileW(ID3D10Device *device, const WCHAR *src_file,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    void *buffer = NULL;
    DWORD size = 0;
    HRESULT hr;

    TRACE("device %p, src_file %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, debugstr_w(src_file), load_info, pump, texture, hresult);

    if (!src_file || !texture)
        return E_FAIL;

    if (FAILED(load_file(src_file, &buffer, &size)))
        return D3D10_ERROR_FILE_NOT_FOUND;

    hr = D3DX10CreateTextureFromMemory(device, buffer, size, load_info, pump, texture, hresult);

    heap_free(buffer);

    return hr;
}

HRESULT WINAPI D3DX10CreateTextureFromResourceA(ID3D10Device *device, HMODULE module, const char *resource,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    HRSRC res_info;
    void *buffer;
    DWORD size;
    HRESULT hr;

    TRACE("device %p, module %p, resource %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, module, debugstr_a(resource), load_info, pump, texture, hresult);

    if (!resource || !texture)
        return D3DX10_ERR_INVALID_DATA;

    if (!(res_info = FindResourceA(module, resource, (const char *)RT_RCDATA)))
    {
        /* Try loading the resource as bitmap data */
        if (!(res_info = FindResourceA(module, resource, (const char *)RT_BITMAP)))
            return D3DX10_ERR_INVALID_DATA;
    }

    if (FAILED(hr = load_resource(module, res_info, &buffer, &size)))
        return D3DX10_ERR_INVALID_DATA;

    return D3DX10CreateTextureFromMemory(device, buffer, size, load_info, pump, texture, hresult);
}

HRESULT WINAPI D3DX10CreateTextureFromResourceW(ID3D10Device *device, HMODULE module, const WCHAR *resource,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    HRSRC res_info;
    void *buffer;
    DWORD size;
    HRESULT hr;

    TRACE("device %p, module %p, resource %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, module, debugstr_w(resource), load_info, pump, texture, hresult);

    if (!resource || !texture)
        return D3DX10_ERR_INVALID_DATA;

    if (!(res_info = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
    {
        /* Try loading the resource as bitmap data */
        if (!(res_info = FindResourceW(module, resource, (const WCHAR *)RT_BITMAP)))
            return D3DX10_ERR_INVALID_DATA;
    }

    if (FAILED(hr = load_resource(module, res_info, &buffer, &size)))
        return D3DX10_ERR_INVALID_DATA;

    return D3DX10CreateTextureFromMemory(device, buffer, size, load_info, pump, texture, hresult);
}

HRESULT WINAPI D3DX10CreateTextureFromMemory(ID3D10Device *device, const void *src_data, SIZE_T src_data_size,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    unsigned int frame_count, width, height, stride, frame_size;
    IWICFormatConverter *converter = NULL;
    IWICDdsFrameDecode *dds_frame = NULL;
    D3D10_TEXTURE2D_DESC texture_2d_desc;
    D3D10_SUBRESOURCE_DATA resource_data;
    IWICBitmapFrameDecode *frame = NULL;
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    ID3D10Texture2D *texture_2d;
    D3DX10_IMAGE_INFO img_info;
    IWICStream *stream = NULL;
    const GUID *dst_format;
    BYTE *buffer = NULL;
    BOOL can_convert;
    GUID src_format;
    HRESULT hr;

    TRACE("device %p, src_data %p, src_data_size %Iu, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, src_data, src_data_size, load_info, pump, texture, hresult);

    if (!src_data || !src_data_size || !texture)
        return E_FAIL;
    if (load_info)
        FIXME("load_info is ignored.\n");
    if (pump)
        FIXME("Thread pump is not supported yet.\n");

    if (FAILED(D3DX10GetImageInfoFromMemory(src_data, src_data_size, NULL, &img_info, NULL)))
        return E_FAIL;
    if (img_info.MiscFlags & D3D10_RESOURCE_MISC_TEXTURECUBE)
    {
        FIXME("Cube map is not supported.\n");
        return E_FAIL;
    }

    if (FAILED(hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory)))
        goto end;
    if (FAILED(hr = IWICImagingFactory_CreateStream(factory, &stream)))
        goto end;
    if (FAILED(hr = IWICStream_InitializeFromMemory(stream, (BYTE *)src_data, src_data_size)))
        goto end;
    if (FAILED(hr = IWICImagingFactory_CreateDecoderFromStream(factory, (IStream *)stream, NULL, 0, &decoder)))
        goto end;
    if (FAILED(hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count)) || !frame_count)
        goto end;
    if (FAILED(hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame)))
        goto end;
    if (FAILED(hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &src_format)))
        goto end;

    width = img_info.Width;
    height = img_info.Height;
    if (is_block_compressed(img_info.Format))
    {
        width = (width + 3) & ~3;
        height = (height + 3) & ~3;
    }
    stride = (width * get_bpp_from_format(img_info.Format) + 7) / 8;
    frame_size = stride * height;

    if (!(buffer = heap_alloc(frame_size)))
    {
        hr = E_FAIL;
        goto end;
    }

    if (is_block_compressed(img_info.Format))
    {
        if (FAILED(hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICDdsFrameDecode, (void **)&dds_frame)))
            goto end;
        if (FAILED(hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, stride * 4, frame_size, buffer)))
            goto end;
    }
    else
    {
        if (!(dst_format = dxgi_format_to_wic_guid(img_info.Format)))
        {
            hr = E_FAIL;
            FIXME("Unsupported DXGI format %#x.\n", img_info.Format);
            goto end;
        }

        if (IsEqualGUID(&src_format, dst_format))
        {
            if (FAILED(hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, stride, frame_size, buffer)))
                goto end;
        }
        else
        {
            if (FAILED(hr = IWICImagingFactory_CreateFormatConverter(factory, &converter)))
                goto end;
            if (FAILED(hr = IWICFormatConverter_CanConvert(converter, &src_format, dst_format, &can_convert)))
                goto end;
            if (!can_convert)
            {
                WARN("Format converting %s to %s is not supported by WIC.\n",
                        debugstr_guid(&src_format), debugstr_guid(dst_format));
                goto end;
            }
            if (FAILED(hr = IWICFormatConverter_Initialize(converter, (IWICBitmapSource *)frame, dst_format,
                    WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom)))
                goto end;
            if (FAILED(hr = IWICFormatConverter_CopyPixels(converter, NULL, stride, frame_size, buffer)))
                goto end;
        }
    }

    memset(&texture_2d_desc, 0, sizeof(texture_2d_desc));
    texture_2d_desc.Width = width;
    texture_2d_desc.Height = height;
    texture_2d_desc.MipLevels = 1;
    texture_2d_desc.ArraySize = img_info.ArraySize;
    texture_2d_desc.Format = img_info.Format;
    texture_2d_desc.SampleDesc.Count = 1;
    texture_2d_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_2d_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_2d_desc.MiscFlags = img_info.MiscFlags;

    resource_data.pSysMem = buffer;
    resource_data.SysMemPitch = stride;
    resource_data.SysMemSlicePitch = frame_size;

    if (FAILED(hr = ID3D10Device_CreateTexture2D(device, &texture_2d_desc, &resource_data, &texture_2d)))
        goto end;

    *texture = (ID3D10Resource *)texture_2d;
    hr = S_OK;

end:
    if (converter)
        IWICFormatConverter_Release(converter);
    if (dds_frame)
        IWICDdsFrameDecode_Release(dds_frame);
    heap_free(buffer);
    if (frame)
        IWICBitmapFrameDecode_Release(frame);
    if (decoder)
        IWICBitmapDecoder_Release(decoder);
    if (stream)
        IWICStream_Release(stream);
    if (factory)
        IWICImagingFactory_Release(factory);

    return hr;
}
