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

#define COBJMACROS

#include "d3d10_1.h"
#include "d3dx10.h"
#include "dxhelpers.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL  0x8876086c

/*
 * These are mappings from legacy DDS header formats to DXGI formats. Some
 * don't map to a DXGI_FORMAT at all, and some only map to the default format.
 */
static DXGI_FORMAT dxgi_format_from_legacy_dds_d3dx_pixel_format_id(enum d3dx_pixel_format_id format)
{
    switch (format)
    {
        /*
         * Some of these formats do have DXGI_FORMAT equivalents, but get
         * mapped to DXGI_FORMAT_R8G8B8A8_UNORM instead.
         */
        case D3DX_PIXEL_FORMAT_P8_UINT:                  return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM:         return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B8G8R8_UNORM:             return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B5G6R5_UNORM:             return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B2G3R3_UNORM:             return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM:           return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_L8A8_UNORM:               return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_L4A4_UNORM:               return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_L8_UNORM:                 return DXGI_FORMAT_R8G8B8A8_UNORM;

        /* B10G10R10A2 doesn't exist in DXGI, both map to R10G10B10A2. */
        case D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM:
        case D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM:        return DXGI_FORMAT_R10G10B10A2_UNORM;

        case D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM:       return DXGI_FORMAT_R16G16B16A16_SNORM;
        case D3DX_PIXEL_FORMAT_L16_UNORM:                return DXGI_FORMAT_R16G16B16A16_UNORM;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM:       return DXGI_FORMAT_R16G16B16A16_UNORM;
        case D3DX_PIXEL_FORMAT_R16G16_UNORM:             return DXGI_FORMAT_R16G16_UNORM;
        case D3DX_PIXEL_FORMAT_A8_UNORM:                 return DXGI_FORMAT_A8_UNORM;
        case D3DX_PIXEL_FORMAT_R16_FLOAT:                return DXGI_FORMAT_R16_FLOAT;
        case D3DX_PIXEL_FORMAT_R16G16_FLOAT:             return DXGI_FORMAT_R16G16_FLOAT;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT:       return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case D3DX_PIXEL_FORMAT_R32_FLOAT:                return DXGI_FORMAT_R32_FLOAT;
        case D3DX_PIXEL_FORMAT_R32G32_FLOAT:             return DXGI_FORMAT_R32G32_FLOAT;
        case D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT:       return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM:          return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM:          return DXGI_FORMAT_R8G8_B8G8_UNORM;

        case D3DX_PIXEL_FORMAT_DXT1_UNORM:               return DXGI_FORMAT_BC1_UNORM;
        case D3DX_PIXEL_FORMAT_DXT2_UNORM:               return DXGI_FORMAT_BC2_UNORM;
        case D3DX_PIXEL_FORMAT_DXT3_UNORM:               return DXGI_FORMAT_BC2_UNORM;
        case D3DX_PIXEL_FORMAT_DXT4_UNORM:               return DXGI_FORMAT_BC3_UNORM;
        case D3DX_PIXEL_FORMAT_DXT5_UNORM:               return DXGI_FORMAT_BC3_UNORM;
        case D3DX_PIXEL_FORMAT_BC4_UNORM:                return DXGI_FORMAT_BC4_UNORM;
        case D3DX_PIXEL_FORMAT_BC4_SNORM:                return DXGI_FORMAT_BC4_SNORM;
        case D3DX_PIXEL_FORMAT_BC5_UNORM:                return DXGI_FORMAT_BC5_UNORM;
        case D3DX_PIXEL_FORMAT_BC5_SNORM:                return DXGI_FORMAT_BC5_SNORM;

        /* These formats are known and explicitly unsupported on d3dx10+. */
        case D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM:
        case D3DX_PIXEL_FORMAT_U8V8_SNORM:
        case D3DX_PIXEL_FORMAT_U8V8_SNORM_Cx:
        case D3DX_PIXEL_FORMAT_U16V16_SNORM:
        case D3DX_PIXEL_FORMAT_U8V8_SNORM_L8X8_UNORM:
        case D3DX_PIXEL_FORMAT_U10V10W10_SNORM_A2_UNORM:
        case D3DX_PIXEL_FORMAT_UYVY:
        case D3DX_PIXEL_FORMAT_YUY2:
            return DXGI_FORMAT_UNKNOWN;

        default:
            FIXME("Unknown d3dx_pixel_format_id %#x.\n", format);
            return DXGI_FORMAT_UNKNOWN;
    }
}

static DXGI_FORMAT dxgi_format_from_d3dx_pixel_format_id(enum d3dx_pixel_format_id format)
{
    switch (format)
    {
        case D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM:          return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM:          return DXGI_FORMAT_B8G8R8A8_UNORM;
        case D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM:          return DXGI_FORMAT_B8G8R8X8_UNORM;
        case D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM:       return DXGI_FORMAT_R10G10B10A2_UNORM;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM:      return DXGI_FORMAT_R16G16B16A16_UNORM;
        case D3DX_PIXEL_FORMAT_R8_UNORM:                return DXGI_FORMAT_R8_UNORM;
        case D3DX_PIXEL_FORMAT_R8_SNORM:                return DXGI_FORMAT_R8_SNORM;
        case D3DX_PIXEL_FORMAT_R8G8_UNORM:              return DXGI_FORMAT_R8G8_UNORM;
        case D3DX_PIXEL_FORMAT_R16_UNORM:               return DXGI_FORMAT_R16_UNORM;
        case D3DX_PIXEL_FORMAT_R16G16_UNORM:            return DXGI_FORMAT_R16G16_UNORM;
        case D3DX_PIXEL_FORMAT_A8_UNORM:                return DXGI_FORMAT_A8_UNORM;
        case D3DX_PIXEL_FORMAT_R16_FLOAT:               return DXGI_FORMAT_R16_FLOAT;
        case D3DX_PIXEL_FORMAT_R16G16_FLOAT:            return DXGI_FORMAT_R16G16_FLOAT;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case D3DX_PIXEL_FORMAT_R32_FLOAT:               return DXGI_FORMAT_R32_FLOAT;
        case D3DX_PIXEL_FORMAT_R32G32_FLOAT:            return DXGI_FORMAT_R32G32_FLOAT;
        case D3DX_PIXEL_FORMAT_R32G32B32_FLOAT:         return DXGI_FORMAT_R32G32B32_FLOAT;
        case D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT:      return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM:         return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM:         return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case D3DX_PIXEL_FORMAT_BC1_UNORM:               return DXGI_FORMAT_BC1_UNORM;
        case D3DX_PIXEL_FORMAT_BC2_UNORM:               return DXGI_FORMAT_BC2_UNORM;
        case D3DX_PIXEL_FORMAT_BC3_UNORM:               return DXGI_FORMAT_BC3_UNORM;
        case D3DX_PIXEL_FORMAT_BC4_UNORM:               return DXGI_FORMAT_BC4_UNORM;
        case D3DX_PIXEL_FORMAT_BC4_SNORM:               return DXGI_FORMAT_BC4_SNORM;
        case D3DX_PIXEL_FORMAT_BC5_UNORM:               return DXGI_FORMAT_BC5_UNORM;
        case D3DX_PIXEL_FORMAT_BC5_SNORM:               return DXGI_FORMAT_BC5_SNORM;
        case D3DX_PIXEL_FORMAT_R16G16B16A16_SNORM:      return DXGI_FORMAT_R16G16B16A16_SNORM;
        case D3DX_PIXEL_FORMAT_R8G8B8A8_SNORM:          return DXGI_FORMAT_R8G8B8A8_SNORM;
        case D3DX_PIXEL_FORMAT_R8G8_SNORM:              return DXGI_FORMAT_R8G8_SNORM;
        case D3DX_PIXEL_FORMAT_R16G16_SNORM:            return DXGI_FORMAT_R16G16_SNORM;

        /*
         * These have DXGI_FORMAT equivalents, but are explicitly unsupported on
         * d3dx10.
         */
        case D3DX_PIXEL_FORMAT_B5G6R5_UNORM:
        case D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM:
        case D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM:
            return DXGI_FORMAT_UNKNOWN;

        default:
            FIXME("Unhandled d3dx_pixel_format_id %#x.\n", format);
            return DXGI_FORMAT_UNKNOWN;
    }
}

static D3DX10_IMAGE_FILE_FORMAT d3dx10_image_file_format_from_d3dx_image_file_format(enum d3dx_image_file_format iff)
{
    switch (iff)
    {
        case D3DX_IMAGE_FILE_FORMAT_BMP: return D3DX10_IFF_BMP;
        case D3DX_IMAGE_FILE_FORMAT_JPG: return D3DX10_IFF_JPG;
        case D3DX_IMAGE_FILE_FORMAT_PNG: return D3DX10_IFF_PNG;
        case D3DX_IMAGE_FILE_FORMAT_DDS: return D3DX10_IFF_DDS;
        case D3DX_IMAGE_FILE_FORMAT_TIFF: return D3DX10_IFF_TIFF;
        case D3DX_IMAGE_FILE_FORMAT_GIF: return D3DX10_IFF_GIF;
        case D3DX_IMAGE_FILE_FORMAT_WMP: return D3DX10_IFF_WMP;
        case D3DX_IMAGE_FILE_FORMAT_DDS_DXT10: return D3DX10_IFF_DDS;
        default:
            FIXME("No D3DX10_IMAGE_FILE_FORMAT for d3dx_image_file_format %d.\n", iff);
            return D3DX10_IFF_FORCE_DWORD;
    }
}

HRESULT WINAPI D3DX10GetImageInfoFromFileA(const char *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result)
{
    WCHAR *buffer;
    int str_len;
    HRESULT hr;

    TRACE("src_file %s, pump %p, info %p, result %p.\n", debugstr_a(src_file), pump, info, result);

    if (!src_file)
        return E_FAIL;

    str_len = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0);
    if (!str_len)
        return HRESULT_FROM_WIN32(GetLastError());

    buffer = malloc(str_len * sizeof(*buffer));
    if (!buffer)
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, src_file, -1, buffer, str_len);
    hr = D3DX10GetImageInfoFromFileW(buffer, pump, info, result);

    free(buffer);

    return hr;
}

HRESULT WINAPI D3DX10GetImageInfoFromFileW(const WCHAR *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result)
{
    void *buffer = NULL;
    DWORD size = 0;
    HRESULT hr;

    TRACE("src_file %s, pump %p, info %p, result %p.\n", debugstr_w(src_file), pump, info, result);

    if (!src_file)
        return E_FAIL;

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncFileLoaderW(src_file, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureInfoProcessor(info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, result, NULL);
        if (FAILED(hr))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    if (SUCCEEDED((hr = load_file(src_file, &buffer, &size))))
    {
        hr = get_image_info(buffer, size, info);
        free(buffer);
    }
    if (result)
        *result = hr;
    return hr;
}

HRESULT WINAPI D3DX10GetImageInfoFromResourceA(HMODULE module, const char *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result)
{
    uint32_t size;
    void *buffer;
    HRESULT hr;

    TRACE("module %p, resource %s, pump %p, info %p, result %p.\n",
            module, debugstr_a(resource), pump, info, result);

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncResourceLoaderA(module, resource, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureInfoProcessor(info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, result, NULL))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    if (FAILED((hr = d3dx_load_resource_a(module, resource, &buffer, &size))))
        return hr;
    hr = get_image_info(buffer, size, info);
    if (result)
        *result = hr;
    return hr;
}

HRESULT WINAPI D3DX10GetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result)
{
    uint32_t size;
    void *buffer;
    HRESULT hr;

    TRACE("module %p, resource %s, pump %p, info %p, result %p.\n",
            module, debugstr_w(resource), pump, info, result);

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncResourceLoaderW(module, resource, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureInfoProcessor(info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, result, NULL))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    if (FAILED((hr = d3dx_load_resource_w(module, resource, &buffer, &size))))
        return hr;
    hr = get_image_info(buffer, size, info);
    if (result)
        *result = hr;
    return hr;
}

static HRESULT d3dx10_image_info_from_d3dx_image(D3DX10_IMAGE_INFO *info, struct d3dx_image *image)
{
    D3DX10_IMAGE_FILE_FORMAT iff = d3dx10_image_file_format_from_d3dx_image_file_format(image->image_file_format);
    DXGI_FORMAT format;

    memset(info, 0, sizeof(*info));
    switch (image->image_file_format)
    {
        case D3DX_IMAGE_FILE_FORMAT_DDS_DXT10:
            format = dxgi_format_from_d3dx_pixel_format_id(image->format);
            break;

        case D3DX_IMAGE_FILE_FORMAT_DDS:
            format = dxgi_format_from_legacy_dds_d3dx_pixel_format_id(image->format);
            break;

        default:
            if (iff == D3DX10_IFF_FORCE_DWORD)
                return E_FAIL;

            /* All other image file formats use the default format. */
            format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
    }

    if (format == DXGI_FORMAT_UNKNOWN)
    {
        WARN("Tried to load file with unsupported pixel format %#x.\n", image->format);
        return E_FAIL;
    }

    switch (image->resource_type)
    {
        case D3DX_RESOURCE_TYPE_TEXTURE_2D:
            info->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
            break;

        case D3DX_RESOURCE_TYPE_CUBE_TEXTURE:
            info->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
            info->MiscFlags |= D3D10_RESOURCE_MISC_TEXTURECUBE;
            break;

        case D3DX_RESOURCE_TYPE_TEXTURE_3D:
            info->ResourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
            break;

        default:
            ERR("Unhandled resource type %d.\n", image->resource_type);
            return E_FAIL;
    }

    info->ImageFileFormat = iff;
    info->Width = image->size.width;
    info->Height = image->size.height;
    info->Depth = image->size.depth;
    info->ArraySize = image->layer_count;
    info->MipLevels = image->mip_levels;
    info->Format = format;
    return S_OK;
}

HRESULT get_image_info(const void *data, SIZE_T size, D3DX10_IMAGE_INFO *img_info)
{
    struct d3dx_image image;
    HRESULT hr;

    if (!data || !size)
        return E_FAIL;

    hr = d3dx_image_init(data, size, &image, 0, D3DX_IMAGE_INFO_ONLY | D3DX_IMAGE_SUPPORT_DXT10);
    if (SUCCEEDED(hr))
        hr = d3dx10_image_info_from_d3dx_image(img_info, &image);

    if (hr != S_OK)
    {
        WARN("Invalid or unsupported image file.\n");
        return E_FAIL;
    }
    return S_OK;
}

HRESULT WINAPI D3DX10GetImageInfoFromMemory(const void *src_data, SIZE_T src_data_size, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *img_info, HRESULT *result)
{
    HRESULT hr;

    TRACE("src_data %p, src_data_size %Iu, pump %p, img_info %p, hresult %p.\n",
            src_data, src_data_size, pump, img_info, result);

    if (!src_data)
        return E_FAIL;

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncMemoryLoader(src_data, src_data_size, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureInfoProcessor(img_info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, result, NULL))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    hr = get_image_info(src_data, src_data_size, img_info);
    if (result)
        *result = hr;
    return hr;
}

static HRESULT create_texture(ID3D10Device *device, const void *data, SIZE_T size,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3D10Resource **texture)
{
    D3D10_SUBRESOURCE_DATA *resource_data;
    D3DX10_IMAGE_LOAD_INFO load_info_copy;
    D3DX10_IMAGE_INFO img_info;
    HRESULT hr;

    init_load_info(load_info, &load_info_copy);
    if (!load_info_copy.pSrcInfo)
        load_info_copy.pSrcInfo = &img_info;

    if (FAILED((hr = load_texture_data(data, size, &load_info_copy, &resource_data))))
        return hr;
    hr = create_d3d_texture(device, &load_info_copy, resource_data, texture);
    free(resource_data);
    return hr;
}

HRESULT WINAPI D3DX10CreateTextureFromFileA(ID3D10Device *device, const char *src_file,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    WCHAR *buffer;
    int str_len;
    HRESULT hr;

    TRACE("device %p, src_file %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, debugstr_a(src_file), load_info, pump, texture, hresult);

    if (!device)
        return E_INVALIDARG;
    if (!src_file)
        return E_FAIL;

    if (!(str_len = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(buffer = malloc(str_len * sizeof(*buffer))))
        return E_OUTOFMEMORY;

    MultiByteToWideChar(CP_ACP, 0, src_file, -1, buffer, str_len);
    hr = D3DX10CreateTextureFromFileW(device, buffer, load_info, pump, texture, hresult);

    free(buffer);

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

    if (!device)
        return E_INVALIDARG;
    if (!src_file)
        return E_FAIL;

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncFileLoaderW(src_file, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureProcessor(device, load_info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, hresult, (void **)texture))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    if (SUCCEEDED((hr = load_file(src_file, &buffer, &size))))
    {
        hr = create_texture(device, buffer, size, load_info, texture);
        free(buffer);
    }
    if (hresult)
        *hresult = hr;
    return hr;
}

HRESULT WINAPI D3DX10CreateTextureFromResourceA(ID3D10Device *device, HMODULE module, const char *resource,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    uint32_t size;
    void *buffer;
    HRESULT hr;

    TRACE("device %p, module %p, resource %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, module, debugstr_a(resource), load_info, pump, texture, hresult);

    if (!device)
        return E_INVALIDARG;

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncResourceLoaderA(module, resource, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureProcessor(device, load_info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, hresult, (void **)texture))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    if (FAILED((hr = d3dx_load_resource_a(module, resource, &buffer, &size))))
        return hr;
    hr = create_texture(device, buffer, size, load_info, texture);
    if (hresult)
        *hresult = hr;
    return hr;
}

HRESULT WINAPI D3DX10CreateTextureFromResourceW(ID3D10Device *device, HMODULE module, const WCHAR *resource,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    uint32_t size;
    void *buffer;
    HRESULT hr;

    TRACE("device %p, module %p, resource %s, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, module, debugstr_w(resource), load_info, pump, texture, hresult);

    if (!device)
        return E_INVALIDARG;

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncResourceLoaderW(module, resource, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureProcessor(device, load_info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, hresult, (void **)texture))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    if (FAILED((hr = d3dx_load_resource_w(module, resource, &buffer, &size))))
        return hr;
    hr = create_texture(device, buffer, size, load_info, texture);
    if (hresult)
        *hresult = hr;
    return hr;
}

void init_load_info(const D3DX10_IMAGE_LOAD_INFO *load_info, D3DX10_IMAGE_LOAD_INFO *out)
{
    if (load_info)
    {
        *out = *load_info;
        return;
    }

    out->Width = D3DX10_DEFAULT;
    out->Height = D3DX10_DEFAULT;
    out->Depth = D3DX10_DEFAULT;
    out->FirstMipLevel = D3DX10_DEFAULT;
    out->MipLevels = D3DX10_DEFAULT;
    out->Usage = D3DX10_DEFAULT;
    out->BindFlags = D3DX10_DEFAULT;
    out->CpuAccessFlags = D3DX10_DEFAULT;
    out->MiscFlags = D3DX10_DEFAULT;
    out->Format = D3DX10_DEFAULT;
    out->Filter = D3DX10_DEFAULT;
    out->MipFilter = D3DX10_DEFAULT;
    out->pSrcInfo = NULL;
}

HRESULT load_texture_data(const void *data, SIZE_T size, D3DX10_IMAGE_LOAD_INFO *load_info,
        D3D10_SUBRESOURCE_DATA **resource_data)
{
    uint32_t loaded_mip_level_count, max_mip_level_count, loaded_layer_count;
    const struct pixel_format_desc *fmt_desc, *src_desc;
    struct d3dx_subresource_data *sub_rsrcs = NULL;
    D3DX10_IMAGE_INFO img_info;
    struct d3dx_image image;
    unsigned int i, j;
    HRESULT hr = S_OK;

    if (!data || !size)
        return E_FAIL;

    *resource_data = NULL;
    hr = d3dx_image_init(data, size, &image, 0, D3DX_IMAGE_SUPPORT_DXT10);
    if (FAILED(hr))
        return E_FAIL;

    hr = d3dx10_image_info_from_d3dx_image(&img_info, &image);
    if (FAILED(hr))
    {
        WARN("Invalid or unsupported image file, hr %#lx.\n", hr);
        hr = E_FAIL;
        goto end;
    }

    loaded_layer_count = img_info.ArraySize;
    if ((loaded_layer_count > 1) && (img_info.ResourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D))
    {
        TRACE("Ignoring array size variable %u for 3D texture.\n", img_info.ArraySize);
        loaded_layer_count = 1;
    }

    if (load_info->FirstMipLevel == D3DX10_DEFAULT || load_info->FirstMipLevel >= img_info.MipLevels)
        load_info->FirstMipLevel = 0;

    if (load_info->Format == D3DX10_DEFAULT || load_info->Format == DXGI_FORMAT_FROM_FILE)
        load_info->Format = img_info.Format;
    fmt_desc = get_d3dx_pixel_format_info(d3dx_pixel_format_id_from_dxgi_format(load_info->Format));
    if (fmt_desc->format == D3DX_PIXEL_FORMAT_COUNT)
    {
        FIXME("Unknown DXGI format supplied, %#x.\n", load_info->Format);
        hr = E_NOTIMPL;
        goto end;
    }

    /* Potentially round up width/height to align with block size. */
    if (!load_info->Width || load_info->Width == D3DX10_FROM_FILE || load_info->Width == D3DX10_DEFAULT)
        load_info->Width = (img_info.Width + fmt_desc->block_width - 1) & ~(fmt_desc->block_width - 1);
    if (!load_info->Height || load_info->Height == D3DX10_FROM_FILE || load_info->Height == D3DX10_DEFAULT)
        load_info->Height = (img_info.Height + fmt_desc->block_height - 1) & ~(fmt_desc->block_height - 1);
    if (!load_info->Depth || load_info->Depth == D3DX10_FROM_FILE || load_info->Depth == D3DX10_DEFAULT)
        load_info->Depth = img_info.Depth;

    if ((load_info->Depth > 1) && (img_info.ResourceDimension != D3D10_RESOURCE_DIMENSION_TEXTURE3D))
    {
        WARN("Invalid depth value %u for image with dimension %d.\n", load_info->Depth, img_info.ResourceDimension);
        hr = E_FAIL;
        goto end;
    }

    max_mip_level_count = d3dx_get_max_mip_levels_for_size(load_info->Width, load_info->Height, load_info->Depth);
    if (!load_info->MipLevels || load_info->MipLevels == D3DX10_DEFAULT || load_info->MipLevels == D3DX10_FROM_FILE)
        load_info->MipLevels = (load_info->MipLevels == D3DX10_FROM_FILE) ? img_info.MipLevels : max_mip_level_count;
    load_info->MipLevels = min(max_mip_level_count, load_info->MipLevels);

    if ((load_info->Width != image.size.width) || (load_info->Height != image.size.height)
            || (load_info->Depth != image.size.depth) || (load_info->MipLevels != image.mip_levels)
            || (fmt_desc->format != image.format))
    {
        if (!load_info->Filter || load_info->Filter == D3DX10_DEFAULT)
            load_info->Filter = D3DX10_FILTER_LINEAR;
        if (FAILED(hr = d3dx_validate_filter(load_info->Filter)))
        {
            WARN("Invalid filter argument %#x.\n", load_info->Filter);
            goto end;
        }
    }
    else
    {
        load_info->Filter = D3DX10_FILTER_NONE;
    }

    hr = d3dx_create_subresource_data_for_texture(load_info->Width, load_info->Height, load_info->Depth,
            load_info->MipLevels, loaded_layer_count, fmt_desc, &sub_rsrcs);
    if (FAILED(hr))
        goto end;

    src_desc = get_d3dx_pixel_format_info(image.format);
    loaded_mip_level_count = min(img_info.MipLevels - load_info->FirstMipLevel, load_info->MipLevels);
    for (i = 0; i < loaded_layer_count; ++i)
    {
        struct volume dst_size = { load_info->Width, load_info->Height, load_info->Depth };

        for (j = 0; j < loaded_mip_level_count; ++j)
        {
            struct d3dx_subresource_data *sub_rsrc = &sub_rsrcs[i * load_info->MipLevels + j];
            const RECT unaligned_rect = { 0, 0, dst_size.width, dst_size.height };
            struct d3dx_pixels src_pixels, dst_pixels;

            hr = d3dx_image_get_pixels(&image, i, j + load_info->FirstMipLevel, &src_pixels);
            if (FAILED(hr))
                goto end;

            set_d3dx_pixels(&dst_pixels, sub_rsrc->data, sub_rsrc->row_pitch, sub_rsrc->slice_pitch, NULL, dst_size.width,
                    dst_size.height, dst_size.depth, &unaligned_rect);

            hr = d3dx_load_pixels_from_pixels(&dst_pixels, fmt_desc, &src_pixels, src_desc, load_info->Filter, 0);
            if (FAILED(hr))
                goto end;

            d3dx_get_next_mip_level_size(&dst_size);
        }
    }

    if (loaded_mip_level_count < load_info->MipLevels)
    {
        struct volume base_level_size = { load_info->Width, load_info->Height, load_info->Depth };
        const uint32_t base_level = loaded_mip_level_count - 1;

        if (!load_info->MipFilter || load_info->MipFilter == D3DX10_DEFAULT)
            load_info->MipFilter = D3DX10_FILTER_LINEAR;
        if (FAILED(hr = d3dx_validate_filter(load_info->MipFilter)))
        {
            WARN("Invalid mip filter argument %#x.\n", load_info->MipFilter);
            goto end;
        }

        d3dx_get_mip_level_size(&base_level_size, base_level);
        for (i = 0; i < loaded_layer_count; ++i)
        {
            struct volume src_size, dst_size;

            src_size = dst_size = base_level_size;
            for (j = base_level; j < (load_info->MipLevels - 1); ++j)
            {
                struct d3dx_subresource_data *dst_data = &sub_rsrcs[i * load_info->MipLevels + j + 1];
                struct d3dx_subresource_data *src_data = &sub_rsrcs[i * load_info->MipLevels + j];
                const RECT src_unaligned_rect = { 0, 0, src_size.width, src_size.height };
                struct d3dx_pixels src_pixels, dst_pixels;
                RECT dst_unaligned_rect;

                d3dx_get_next_mip_level_size(&dst_size);
                SetRect(&dst_unaligned_rect, 0, 0, dst_size.width, dst_size.height);
                set_d3dx_pixels(&dst_pixels, dst_data->data, dst_data->row_pitch, dst_data->slice_pitch, NULL,
                        dst_size.width, dst_size.height, dst_size.depth, &dst_unaligned_rect);
                set_d3dx_pixels(&src_pixels, src_data->data, src_data->row_pitch, src_data->slice_pitch, NULL,
                        src_size.width, src_size.height, src_size.depth, &src_unaligned_rect);

                hr = d3dx_load_pixels_from_pixels(&dst_pixels, fmt_desc, &src_pixels, fmt_desc, load_info->MipFilter, 0);
                if (FAILED(hr))
                    goto end;

                src_size = dst_size;
            }
        }
    }

    *resource_data = (D3D10_SUBRESOURCE_DATA *)sub_rsrcs;
    sub_rsrcs = NULL;

    load_info->Usage = (load_info->Usage == D3DX10_DEFAULT) ? D3D10_USAGE_DEFAULT : load_info->Usage;
    load_info->BindFlags = (load_info->BindFlags == D3DX10_DEFAULT) ? D3D10_BIND_SHADER_RESOURCE : load_info->BindFlags;
    load_info->CpuAccessFlags = (load_info->CpuAccessFlags == D3DX10_DEFAULT) ? 0 : load_info->CpuAccessFlags;
    load_info->MiscFlags = (load_info->MiscFlags == D3DX10_DEFAULT) ? 0 : load_info->MiscFlags;
    load_info->MiscFlags |= img_info.MiscFlags;
    /*
     * Must be present in order to get resource dimension for texture
     * creation.
     */
    assert(load_info->pSrcInfo);
    *load_info->pSrcInfo = img_info;

end:
    d3dx_image_cleanup(&image);
    free(sub_rsrcs);
    return hr;
}

HRESULT create_d3d_texture(ID3D10Device *device, D3DX10_IMAGE_LOAD_INFO *load_info,
        D3D10_SUBRESOURCE_DATA *resource_data, ID3D10Resource **texture)
{
    HRESULT hr;

    *texture = NULL;
    switch (load_info->pSrcInfo->ResourceDimension)
    {
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D10_TEXTURE2D_DESC texture_2d_desc = { 0 };
            ID3D10Texture2D *texture_2d;

            texture_2d_desc.Width = load_info->Width;
            texture_2d_desc.Height = load_info->Height;
            texture_2d_desc.MipLevels = load_info->MipLevels;
            texture_2d_desc.ArraySize = load_info->pSrcInfo->ArraySize;
            texture_2d_desc.Format = load_info->Format;
            texture_2d_desc.SampleDesc.Count = 1;
            texture_2d_desc.Usage = load_info->Usage;
            texture_2d_desc.BindFlags = load_info->BindFlags;
            texture_2d_desc.CPUAccessFlags = load_info->CpuAccessFlags;
            texture_2d_desc.MiscFlags = load_info->MiscFlags;

            if (FAILED(hr = ID3D10Device_CreateTexture2D(device, &texture_2d_desc, resource_data, &texture_2d)))
                return hr;
            *texture = (ID3D10Resource *)texture_2d;
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D10_TEXTURE3D_DESC texture_3d_desc = { 0 };
            ID3D10Texture3D *texture_3d;

            texture_3d_desc.Width = load_info->Width;
            texture_3d_desc.Height = load_info->Height;
            texture_3d_desc.Depth = load_info->Depth;
            texture_3d_desc.MipLevels = load_info->MipLevels;
            texture_3d_desc.Format = load_info->Format;
            texture_3d_desc.Usage = load_info->Usage;
            texture_3d_desc.BindFlags = load_info->BindFlags;
            texture_3d_desc.CPUAccessFlags = load_info->CpuAccessFlags;
            texture_3d_desc.MiscFlags = load_info->MiscFlags;

            if (FAILED(hr = ID3D10Device_CreateTexture3D(device, &texture_3d_desc, resource_data, &texture_3d)))
                return hr;
            *texture = (ID3D10Resource *)texture_3d;
            break;
        }

        default:
            FIXME("Unhandled resource dimension %d.\n", load_info->pSrcInfo->ResourceDimension);
            return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT WINAPI D3DX10CreateTextureFromMemory(ID3D10Device *device, const void *src_data, SIZE_T src_data_size,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10ThreadPump *pump, ID3D10Resource **texture, HRESULT *hresult)
{
    HRESULT hr;

    TRACE("device %p, src_data %p, src_data_size %Iu, load_info %p, pump %p, texture %p, hresult %p.\n",
            device, src_data, src_data_size, load_info, pump, texture, hresult);

    if (!device)
        return E_INVALIDARG;
    if (!src_data)
        return E_FAIL;

    if (pump)
    {
        ID3DX10DataProcessor *processor;
        ID3DX10DataLoader *loader;

        if (FAILED((hr = D3DX10CreateAsyncMemoryLoader(src_data, src_data_size, &loader))))
            return hr;
        if (FAILED((hr = D3DX10CreateAsyncTextureProcessor(device, load_info, &processor))))
        {
            ID3DX10DataLoader_Destroy(loader);
            return hr;
        }
        if (FAILED((hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, hresult, (void **)texture))))
        {
            ID3DX10DataLoader_Destroy(loader);
            ID3DX10DataProcessor_Destroy(processor);
        }
        return hr;
    }

    hr = create_texture(device, src_data, src_data_size, load_info, texture);
    if (hresult)
        *hresult = hr;
    return hr;
}

static void init_d3d10_box(D3D10_BOX *box, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t front,
        uint32_t back)
{
    box->left = left;
    box->top = top;
    box->right = max(right, left + 1);
    box->bottom = max(bottom, top + 1);
    box->front = front;
    box->back = max(back, front + 1);
}

static void d3d10_box_get_level(D3D10_BOX *box, unsigned int level)
{
    init_d3d10_box(box, box->left >> level, box->top >> level, box->right >> level, box->bottom >> level,
            box->front >> level, box->back >> level);
}

static void d3d10_box_get_next_level(D3D10_BOX *box)
{
    init_d3d10_box(box, box->left >> 1, box->top >> 1, box->right >> 1, box->bottom >> 1,
            box->front >> 1, box->back >> 1);
}

static BOOL d3d10_box_clamp(D3D10_BOX *box, unsigned int level, struct volume *size)
{
    struct volume level_size = *size;
    BOOL clamped;

    if (level)
        d3dx_get_mip_level_size(&level_size, level);

    clamped = (box->right > level_size.width) || (box->bottom > level_size.height) || (box->back > level_size.depth);
    if (box->right > level_size.width)
    {
        box->right = level_size.width;
        box->left = min(box->right - 1, box->left);
    }

    if (box->bottom > level_size.height)
    {
        box->bottom = level_size.height;
        box->top = min(box->bottom - 1, box->top);
    }

    if (box->back > level_size.depth)
    {
        box->back = level_size.depth;
        box->front = min(box->back - 1, box->front);
    }

    return clamped;
}

static BOOL d3d10_box_is_valid(const D3D10_BOX *box)
{
    return (box->left <= box->right) && (box->top <= box->bottom) && (box->front <= box->back);
}

struct d3dx_texture_info
{
    enum d3dx_resource_type resource_type;
    const struct pixel_format_desc *fmt;

    struct volume size;
    uint32_t levels;
    uint32_t layers;

    D3D10_USAGE usage;
};

static HRESULT d3dx_texture_info_from_d3d10_resource(ID3D10Resource *rsrc, struct d3dx_texture_info *info)
{
    D3D10_RESOURCE_DIMENSION rsrc_dim;
    HRESULT hr;

    ID3D10Resource_GetType(rsrc, &rsrc_dim);
    switch (rsrc_dim)
    {
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D10_TEXTURE2D_DESC desc_2d;
            ID3D10Texture2D *tex_2d;

            hr = ID3D10Resource_QueryInterface(rsrc, &IID_ID3D10Texture2D, (void **)&tex_2d);
            if (FAILED(hr))
                return hr;
            ID3D10Texture2D_GetDesc(tex_2d, &desc_2d);
            ID3D10Texture2D_Release(tex_2d);

            info->resource_type = D3DX_RESOURCE_TYPE_TEXTURE_2D;
            info->fmt = get_d3dx_pixel_format_info(d3dx_pixel_format_id_from_dxgi_format(desc_2d.Format));
            if (is_unknown_format(info->fmt))
                return E_NOTIMPL;
            set_volume_struct(&info->size, desc_2d.Width, desc_2d.Height, 1);
            info->levels = desc_2d.MipLevels;
            info->layers = desc_2d.ArraySize;
            info->usage = desc_2d.Usage;
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D10_TEXTURE3D_DESC desc_3d;
            ID3D10Texture3D *tex_3d;

            hr = ID3D10Resource_QueryInterface(rsrc, &IID_ID3D10Texture3D, (void **)&tex_3d);
            if (FAILED(hr))
                return hr;
            ID3D10Texture3D_GetDesc(tex_3d, &desc_3d);
            ID3D10Texture3D_Release(tex_3d);

            info->resource_type = D3DX_RESOURCE_TYPE_TEXTURE_3D;
            info->fmt = get_d3dx_pixel_format_info(d3dx_pixel_format_id_from_dxgi_format(desc_3d.Format));
            if (is_unknown_format(info->fmt))
                return E_NOTIMPL;
            set_volume_struct(&info->size, desc_3d.Width, desc_3d.Height, desc_3d.Depth);
            info->levels = desc_3d.MipLevels;
            info->layers = 1;
            info->usage = desc_3d.Usage;
            break;
        }

        default:
            FIXME("Unhandled resource type %d.\n", rsrc_dim);
            return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT d3d10_staging_resource_from_d3dx_texture_info(ID3D10Device *device, struct d3dx_texture_info *info,
        BOOL is_dst, ID3D10Resource **rsrc)
{
    const UINT cpu_flags = is_dst ? (D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE) : D3D10_CPU_ACCESS_READ;

    *rsrc = NULL;
    switch (info->resource_type)
    {
        case D3DX_RESOURCE_TYPE_TEXTURE_2D:
        {
            D3D10_TEXTURE2D_DESC desc = { info->size.width, info->size.height, 1, 1, 0, { 1, 0 }, D3D10_USAGE_STAGING,
                                          0, cpu_flags };

            desc.Format = dxgi_format_from_d3dx_pixel_format_id(info->fmt->format);
            return ID3D10Device_CreateTexture2D(device, &desc, NULL, (ID3D10Texture2D **)rsrc);
        }

        case D3DX_RESOURCE_TYPE_TEXTURE_3D:
        {
            D3D10_TEXTURE3D_DESC desc = { info->size.width, info->size.height, info->size.depth, 1, 0,
                                          D3D10_USAGE_STAGING, 0, cpu_flags };

            desc.Format = dxgi_format_from_d3dx_pixel_format_id(info->fmt->format);
            return ID3D10Device_CreateTexture3D(device, &desc, NULL, (ID3D10Texture3D **)rsrc);
        }

        default:
            return E_NOTIMPL;
    }
}

static HRESULT d3dx_subresource_data_from_d3d10_staging_resource_map(ID3D10Resource *rsrc,
        enum d3dx_resource_type rtype, BOOL is_dst, struct d3dx_subresource_data *out_data)
{
    const D3D10_MAP map_type = is_dst ? D3D10_MAP_READ_WRITE : D3D10_MAP_READ;
    HRESULT hr;

    memset(out_data, 0, sizeof(*out_data));
    switch (rtype)
    {
        case D3DX_RESOURCE_TYPE_TEXTURE_2D:
        {
            ID3D10Texture2D *tex = (ID3D10Texture2D *)rsrc;
            D3D10_MAPPED_TEXTURE2D map;

            hr = ID3D10Texture2D_Map(tex, 0, map_type, 0, &map);
            if (SUCCEEDED(hr))
            {
                out_data->data = map.pData;
                out_data->row_pitch = map.RowPitch;
            }
            break;
        }

        case D3DX_RESOURCE_TYPE_TEXTURE_3D:
        {
            ID3D10Texture3D *tex = (ID3D10Texture3D *)rsrc;
            D3D10_MAPPED_TEXTURE3D map;

            hr = ID3D10Texture3D_Map(tex, 0, map_type, 0, &map);
            if (SUCCEEDED(hr))
            {
                out_data->data = map.pData;
                out_data->row_pitch = map.RowPitch;
                out_data->slice_pitch = map.DepthPitch;
            }
            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
    }

    return hr;
}

static void d3d10_staging_resource_unmap(ID3D10Resource *rsrc, enum d3dx_resource_type rtype)
{
    switch (rtype)
    {
        case D3DX_RESOURCE_TYPE_TEXTURE_2D:
            ID3D10Texture2D_Unmap((ID3D10Texture2D *)rsrc, 0);
            break;

        case D3DX_RESOURCE_TYPE_TEXTURE_3D:
            ID3D10Texture3D_Unmap((ID3D10Texture3D *)rsrc, 0);
            break;

        default:
            break;
    }
}

static HRESULT d3d10_load_texture_from_texture(struct d3dx_texture_info *src_info, ID3D10Resource *src_rsrc,
        const D3D10_BOX *src_box, uint32_t src_first_layer, uint32_t src_first_level,
        struct d3dx_texture_info *dst_info, ID3D10Resource *dst_rsrc, const D3D10_BOX *dst_box,
        uint32_t dst_first_layer, uint32_t dst_first_level, uint32_t layer_load_count, uint32_t level_load_count,
        uint32_t filter)
{
    ID3D10Resource *src_staging, *dst_staging;
    ID3D10Device *device = NULL;
    unsigned int i, j;
    HRESULT hr;

    if (FAILED(hr = d3dx_validate_filter(filter)))
    {
        WARN("Invalid filter argument %#x.\n", filter);
        return hr;
    }

    src_staging = dst_staging = NULL;
    ID3D10Resource_GetDevice(src_rsrc, &device);
    hr = d3d10_staging_resource_from_d3dx_texture_info(device, src_info, FALSE, &src_staging);
    if (FAILED(hr))
        goto exit;

    hr = d3d10_staging_resource_from_d3dx_texture_info(device, dst_info, TRUE, &dst_staging);
    if (FAILED(hr))
        goto exit;

    for (i = 0; i < layer_load_count; ++i)
    {
        unsigned int src_sub_rsrc_idx = (src_first_layer + i) * src_info->levels + src_first_level;
        unsigned int dst_sub_rsrc_idx = (dst_first_layer + i) * dst_info->levels + dst_first_level;
        D3D10_BOX src_level_box = *src_box;
        D3D10_BOX dst_level_box = *dst_box;

        for (j = 0; j < level_load_count; ++j)
        {
            struct d3dx_subresource_data src_sub_rsrc, dst_sub_rsrc;
            struct d3dx_pixels src_pixels, dst_pixels;

            ID3D10Device_CopySubresourceRegion(device, src_staging, 0, 0, 0, 0, src_rsrc, src_sub_rsrc_idx + j, NULL);
            hr = d3dx_subresource_data_from_d3d10_staging_resource_map(src_staging, src_info->resource_type, FALSE,
                    &src_sub_rsrc);
            if (FAILED(hr))
                goto exit;

            ID3D10Device_CopySubresourceRegion(device, dst_staging, 0, 0, 0, 0, dst_rsrc, dst_sub_rsrc_idx + j, NULL);
            hr = d3dx_subresource_data_from_d3d10_staging_resource_map(dst_staging, dst_info->resource_type, TRUE,
                    &dst_sub_rsrc);
            if (FAILED(hr))
                goto exit;

            /* Account for source/destination box offsets. */
            d3dx_pixels_init(src_sub_rsrc.data, src_sub_rsrc.row_pitch, src_sub_rsrc.slice_pitch, NULL,
                    src_info->fmt->format, src_level_box.left, src_level_box.top, src_level_box.right,
                    src_level_box.bottom, src_level_box.front, src_level_box.back, &src_pixels);
            d3dx_pixels_init(dst_sub_rsrc.data, dst_sub_rsrc.row_pitch, dst_sub_rsrc.slice_pitch, NULL,
                    dst_info->fmt->format, dst_level_box.left, dst_level_box.top, dst_level_box.right,
                    dst_level_box.bottom, dst_level_box.front, dst_level_box.back, &dst_pixels);

            hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_info->fmt, &src_pixels, src_info->fmt, filter, 0);
            d3d10_staging_resource_unmap(src_staging, src_info->resource_type);
            d3d10_staging_resource_unmap(dst_staging, dst_info->resource_type);
            if (FAILED(hr))
                goto exit;

            ID3D10Device_CopySubresourceRegion(device, dst_rsrc, dst_sub_rsrc_idx + j, 0, 0, 0, dst_staging, 0, &dst_level_box);
            d3d10_box_get_next_level(&src_level_box);
            d3d10_box_get_next_level(&dst_level_box);
        }
    }

exit:
    if (device)
        ID3D10Device_Release(device);
    if (src_staging)
        ID3D10Resource_Release(src_staging);
    if (dst_staging)
        ID3D10Resource_Release(dst_staging);
    return hr;
}

HRESULT WINAPI D3DX10LoadTextureFromTexture(ID3D10Resource *src_texture, D3DX10_TEXTURE_LOAD_INFO *load_info,
        ID3D10Resource *dst_texture)
{
    static const D3DX10_TEXTURE_LOAD_INFO default_load_info = { NULL, NULL, 0, 0, D3DX10_DEFAULT, 0, 0, D3DX10_DEFAULT,
                                                                D3DX10_DEFAULT, D3DX10_DEFAULT };
    D3DX10_TEXTURE_LOAD_INFO info = (load_info) ? *load_info : default_load_info;
    struct d3dx_texture_info src_info, dst_info;
    uint32_t loaded_level_count;
    D3D10_BOX src_box, dst_box;
    HRESULT hr;

    TRACE("src_texture %p, load_info %p, dst_texture %p.\n", src_texture, load_info, dst_texture);

    if (!src_texture || !dst_texture)
        return D3DERR_INVALIDCALL;

    if ((info.pSrcBox && !d3d10_box_is_valid(info.pSrcBox)) || (info.pDstBox && !d3d10_box_is_valid(info.pDstBox)))
        return D3DERR_INVALIDCALL;

    /*
     * If the source and destination texture are the same, we can't load into
     * the same subresource.
     */
    if (src_texture == dst_texture && info.SrcFirstMip == info.DstFirstMip
            && info.SrcFirstElement == info.DstFirstElement)
        return D3DERR_INVALIDCALL;

    hr = d3dx_texture_info_from_d3d10_resource(src_texture, &src_info);
    if (FAILED(hr))
        return hr;

    hr = d3dx_texture_info_from_d3d10_resource(dst_texture, &dst_info);
    if (FAILED(hr))
        return hr;

    /* Destination cannot be immutable. */
    if (dst_info.usage == D3D10_USAGE_IMMUTABLE)
        return D3DERR_INVALIDCALL;

    /*
     * Attempting to load beyond the number of levels/layers in the passed in
     * textures, nothing to load, return early.
     */
    if ((info.DstFirstMip >= dst_info.levels) || (info.SrcFirstElement >= src_info.layers)
            || (info.DstFirstElement >= dst_info.layers))
        return S_OK;

    /*
     * Native doesn't validate the SrcFirstMip argument, and will read OOB if
     * passed a value higher than the actual number of levels.
     */
    if (info.SrcFirstMip >= src_info.levels)
    {
        WARN("Attempted to load first mip from source beyond total number of mips, clamping.\n");
        info.SrcFirstMip = src_info.levels - 1;
    }

    if (!info.NumElements)
        info.NumElements = D3DX10_DEFAULT;
    info.NumElements = min(info.NumElements, min(src_info.layers - info.SrcFirstElement,
                dst_info.layers - info.DstFirstElement));

    if (!info.NumMips)
        info.NumMips = D3DX10_DEFAULT;
    info.NumMips = min(dst_info.levels - info.DstFirstMip, info.NumMips);
    loaded_level_count = min(src_info.levels - info.SrcFirstMip, info.NumMips);

    if (info.pSrcBox)
        init_d3d10_box(&src_box, info.pSrcBox->left, info.pSrcBox->top, info.pSrcBox->right, info.pSrcBox->bottom,
                info.pSrcBox->front, info.pSrcBox->back);
    else
        init_d3d10_box(&src_box, 0, 0, src_info.size.width, src_info.size.height, 0, src_info.size.depth);
    /* Native will do an OOB access in this case, we'll just clamp instead. */
    if (d3d10_box_clamp(&src_box, info.SrcFirstMip, &src_info.size) && info.pSrcBox)
        WARN("Clamped passed in pSrcBox values.\n");

    if (info.pDstBox)
        init_d3d10_box(&dst_box, info.pDstBox->left, info.pDstBox->top, info.pDstBox->right, info.pDstBox->bottom,
                info.pDstBox->front, info.pDstBox->back);
    else
        init_d3d10_box(&dst_box, 0, 0, dst_info.size.width, dst_info.size.height, 0, dst_info.size.depth);
    /* Native will do an OOB access in this case, we'll just clamp instead. */
    if (d3d10_box_clamp(&dst_box, info.DstFirstMip, &dst_info.size) && info.pDstBox)
        WARN("Clamped passed in pDstBox values.\n");

    if (!info.Filter || info.Filter == D3DX10_DEFAULT)
        info.Filter = D3DX10_FILTER_LINEAR;

    hr = d3d10_load_texture_from_texture(&src_info, src_texture, &src_box, info.SrcFirstElement, info.SrcFirstMip,
            &dst_info, dst_texture, &dst_box, info.DstFirstElement, info.DstFirstMip, info.NumElements,
            loaded_level_count, info.Filter);
    if (SUCCEEDED(hr) && loaded_level_count < info.NumMips)
    {
        const uint32_t base_level = loaded_level_count - 1;
        D3D10_BOX src_level_box, dst_level_box;

        src_level_box = dst_level_box = dst_box;
        d3d10_box_get_level(&src_level_box, base_level);
        d3d10_box_get_level(&dst_level_box, base_level + 1);
        if (!info.MipFilter || info.MipFilter == D3DX10_DEFAULT)
            info.MipFilter = D3DX10_FILTER_LINEAR;
        hr = d3d10_load_texture_from_texture(&dst_info, dst_texture, &src_level_box, info.DstFirstElement, base_level,
                &dst_info, dst_texture, &dst_level_box, info.DstFirstElement, base_level + 1, info.NumElements,
                (info.NumMips - loaded_level_count), info.MipFilter);
    }

    return hr;
}
