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
    unsigned int size;
    HRSRC res_info;
    void *buffer;
    HRESULT hr;

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

    TRACE("src_data %p, src_data_size %lu, pump %p, img_info %p, hresult %p.\n",
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
