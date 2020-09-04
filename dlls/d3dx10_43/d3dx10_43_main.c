/*
 * D3DX10 main file
 *
 * Copyright (c) 2010 Owen Rudge for CodeWeavers
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
 *
 */

#include "wine/debug.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

   return TRUE;
}

/***********************************************************************
 * D3DX10CheckVersion
 *
 * Checks whether we are compiling against the correct d3d and d3dx library.
 */
BOOL WINAPI D3DX10CheckVersion(UINT d3dsdkvers, UINT d3dxsdkvers)
{
    if ((d3dsdkvers == D3D10_SDK_VERSION) && (d3dxsdkvers == 43))
        return TRUE;

    return FALSE;
}

HRESULT WINAPI D3DX10CreateEffectFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors,
        HRESULT *hresult)
{
    FIXME("filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, effectpool %p, pump %p, effect %p, errors %p, hresult %p\n",
            debugstr_a(filename), defines, include, debugstr_a(profile), hlslflags, fxflags,
            device, effectpool, pump, effect, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateEffectFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors,
        HRESULT *hresult)
{
    FIXME("filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, effectpool %p, pump %p, effect %p, errors %p, hresult %p\n",
            debugstr_w(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            effectpool, pump, effect, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateEffectFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile, UINT hlslflags,
        UINT fxflags, ID3D10Device *device, ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    FIXME("data %p, datasize %lu, filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, effectpool %p, pump %p, effect %p, errors %p, hresult %p\n",
            data, datasize, debugstr_a(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            effectpool, pump, effect, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateEffectPoolFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile, UINT hlslflags,
        UINT fxflags, ID3D10Device *device, ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool,
        ID3D10Blob **errors, HRESULT *hresult)
{
    FIXME("data %p, datasize %lu, filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, "
            "device %p, pump %p, effectpool %p, errors %p, hresult %p.\n",
            data, datasize, debugstr_a(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            pump, effectpool, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10UnsetAllDeviceObjects(ID3D10Device *device)
{
    static ID3D10ShaderResourceView * const views[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    static ID3D10RenderTargetView * const target_views[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    static ID3D10SamplerState * const sampler_states[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    static ID3D10Buffer * const buffers[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    static const unsigned int so_offsets[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] =
            {~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u};
    static const unsigned int strides[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    static const unsigned int offsets[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    static const float blend_factors[4];

    TRACE("device %p.\n", device);

    if (!device)
        return E_INVALIDARG;

    ID3D10Device_VSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);
    ID3D10Device_PSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);
    ID3D10Device_GSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, buffers);

    ID3D10Device_VSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler_states);
    ID3D10Device_PSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler_states);
    ID3D10Device_GSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler_states);

    ID3D10Device_VSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views);
    ID3D10Device_PSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views);
    ID3D10Device_GSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, views);

    ID3D10Device_VSSetShader(device, NULL);
    ID3D10Device_PSSetShader(device, NULL);
    ID3D10Device_GSSetShader(device, NULL);

    ID3D10Device_OMSetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, target_views, NULL);

    ID3D10Device_IASetIndexBuffer(device, NULL, DXGI_FORMAT_R32_UINT, 0);
    ID3D10Device_IASetInputLayout(device, NULL);
    ID3D10Device_IASetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, buffers, strides, offsets);

    ID3D10Device_SOSetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, buffers, so_offsets);

    ID3D10Device_OMSetBlendState(device, NULL, blend_factors, 0);
    ID3D10Device_OMSetDepthStencilState(device, NULL, 0);

    ID3D10Device_RSSetState(device, NULL);

    ID3D10Device_SetPredication(device, NULL, FALSE);

    return S_OK;
}

HRESULT WINAPI D3DX10CreateDevice(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, ID3D10Device **device)
{
    HRESULT hr;

    TRACE("adapter %p, driver_type %d, swrast %p, flags %#x, device %p.\n", adapter, driver_type,
            swrast, flags, device);

    if (SUCCEEDED(hr = D3D10CreateDevice1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_1,
            D3D10_SDK_VERSION, (ID3D10Device1 **)device)))
        return hr;

    if (SUCCEEDED(hr = D3D10CreateDevice1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_0,
            D3D10_SDK_VERSION, (ID3D10Device1 **)device)))
        return hr;

    return hr;
}

HRESULT WINAPI D3DX10CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain,
        ID3D10Device **device)
{
    HRESULT hr;

    TRACE("adapter %p, driver_type %d, swrast %p, flags %#x, desc %p, swapchain %p, device %p.\n",
            adapter, driver_type, swrast, flags, desc, swapchain, device);

    if (SUCCEEDED(hr = D3D10CreateDeviceAndSwapChain1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_1,
            D3D10_1_SDK_VERSION, desc, swapchain, (ID3D10Device1 **)device)))
        return hr;

    return D3D10CreateDeviceAndSwapChain1(adapter, driver_type, swrast, flags, D3D10_FEATURE_LEVEL_10_0,
            D3D10_1_SDK_VERSION, desc, swapchain, (ID3D10Device1 **)device);
}

HRESULT WINAPI D3DX10CreateTextureFromMemory(ID3D10Device *device, const void *src_data,
        SIZE_T src_data_size, D3DX10_IMAGE_LOAD_INFO *loadinfo, ID3DX10ThreadPump *pump,
        ID3D10Resource **texture, HRESULT *hresult)
{
    FIXME("device %p, src_data %p, src_data_size %lu, loadinfo %p, pump %p, texture %p, "
            "hresult %p, stub!\n",
            device, src_data, src_data_size, loadinfo, pump, texture, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10FilterTexture(ID3D10Resource *texture, UINT src_level, UINT filter)
{
    FIXME("texture %p, src_level %u, filter %#x stub!\n", texture, src_level, filter);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10GetFeatureLevel1(ID3D10Device *device, ID3D10Device1 **device1)
{
    TRACE("device %p, device1 %p.\n", device, device1);

    return ID3D10Device_QueryInterface(device, &IID_ID3D10Device1, (void **)device1);
}

HRESULT WINAPI D3DX10GetImageInfoFromFileA(const char *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result)
{
    FIXME("src_file %s, pump %p, info %p, result %p\n", debugstr_a(src_file), pump, info, result);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10GetImageInfoFromFileW(const WCHAR *src_file, ID3DX10ThreadPump *pump, D3DX10_IMAGE_INFO *info,
        HRESULT *result)
{
    FIXME("src_file %s, pump %p, info %p, result %p\n", debugstr_w(src_file), pump, info, result);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10GetImageInfoFromResourceA(HMODULE module, const char *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result)
{
    FIXME("module %p, resource %s, pump %p, info %p, result %p\n",
            module, debugstr_a(resource), pump, info, result);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10GetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, ID3DX10ThreadPump *pump,
        D3DX10_IMAGE_INFO *info, HRESULT *result)
{
    FIXME("module %p, resource %s, pump %p, info %p, result %p\n",
            module, debugstr_w(resource), pump, info, result);

    return E_NOTIMPL;
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

D3DX_CPU_OPTIMIZATION WINAPI D3DXCpuOptimizations(BOOL enable)
{
    FIXME("enable %#x stub.\n", enable);

    return D3DX_NOT_OPTIMIZED;
}

HRESULT WINAPI D3DX10LoadTextureFromTexture(ID3D10Resource *src_texture, D3DX10_TEXTURE_LOAD_INFO *load_info,
        ID3D10Resource *dst_texture)
{
    FIXME("src_texture %p, load_info %p, dst_texture %p stub!\n", src_texture, load_info, dst_texture);

    return E_NOTIMPL;
}
