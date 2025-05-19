/*
 * Copyright 2024 Elizabeth Figura for CodeWeavers
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

#include "d3d11_private.h"
#include "initguid.h"
#include "dxva.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static struct d3d_video_decoder *impl_from_ID3D11VideoDecoder(ID3D11VideoDecoder *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_video_decoder, ID3D11VideoDecoder_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_decoder_QueryInterface(ID3D11VideoDecoder *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11VideoDecoder)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11VideoDecoder_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_video_decoder_AddRef(ID3D11VideoDecoder *iface)
{
    struct d3d_video_decoder *decoder = impl_from_ID3D11VideoDecoder(iface);
    ULONG refcount = InterlockedIncrement(&decoder->refcount);

    TRACE("%p increasing refcount to %lu.\n", decoder, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_video_decoder_Release(ID3D11VideoDecoder *iface)
{
    struct d3d_video_decoder *decoder = impl_from_ID3D11VideoDecoder(iface);
    ULONG refcount = InterlockedDecrement(&decoder->refcount);

    TRACE("%p decreasing refcount to %lu.\n", decoder, refcount);

    if (!refcount)
    {
        wined3d_decoder_decref(decoder->wined3d_decoder);
        ID3D11Device5_Release(&decoder->device->ID3D11Device5_iface);
        wined3d_private_store_cleanup(&decoder->private_store);
        free(decoder);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_video_decoder_GetDevice(ID3D11VideoDecoder *iface, ID3D11Device **device)
{
    struct d3d_video_decoder *decoder = impl_from_ID3D11VideoDecoder(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)decoder->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_decoder_GetPrivateData(ID3D11VideoDecoder *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_video_decoder *decoder = impl_from_ID3D11VideoDecoder(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&decoder->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_decoder_SetPrivateData(ID3D11VideoDecoder *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_video_decoder *decoder = impl_from_ID3D11VideoDecoder(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&decoder->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_decoder_SetPrivateDataInterface(ID3D11VideoDecoder *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_video_decoder *decoder = impl_from_ID3D11VideoDecoder(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&decoder->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_video_decoder_GetCreationParameters(ID3D11VideoDecoder *iface,
        D3D11_VIDEO_DECODER_DESC *desc, D3D11_VIDEO_DECODER_CONFIG *config)
{
    FIXME("iface %p, desc %p, config %p, stub!\n", iface, desc, config);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_video_decoder_GetDriverHandle(ID3D11VideoDecoder *iface, HANDLE *handle)
{
    FIXME("iface %p, handle %p, stub!\n", iface, handle);
    return E_NOTIMPL;
}

static const struct ID3D11VideoDecoderVtbl d3d11_video_decoder_vtbl =
{
    d3d11_video_decoder_QueryInterface,
    d3d11_video_decoder_AddRef,
    d3d11_video_decoder_Release,
    d3d11_video_decoder_GetDevice,
    d3d11_video_decoder_GetPrivateData,
    d3d11_video_decoder_SetPrivateData,
    d3d11_video_decoder_SetPrivateDataInterface,
    d3d11_video_decoder_GetCreationParameters,
    d3d11_video_decoder_GetDriverHandle,
};

HRESULT d3d_video_decoder_create(struct d3d_device *device, const D3D11_VIDEO_DECODER_DESC *desc,
        const D3D11_VIDEO_DECODER_CONFIG *config, struct d3d_video_decoder **decoder)
{
    struct wined3d_decoder_desc wined3d_desc;
    struct d3d_video_decoder *object;
    HRESULT hr;

    TRACE("profile %s, size %ux%u, output format %#x.\n",
            debugstr_guid(&desc->Guid), desc->SampleWidth, desc->SampleHeight, desc->OutputFormat);

    TRACE("    guidConfigBitstreamEncryption: %s\n", debugstr_guid(&config->guidConfigBitstreamEncryption));
    TRACE("    guidConfigMBcontrolEncryption: %s\n", debugstr_guid(&config->guidConfigMBcontrolEncryption));
    TRACE("    guidConfigResidDiffEncryption: %s\n", debugstr_guid(&config->guidConfigResidDiffEncryption));
#define X(field) TRACE("    " #field ": %u\n", config->field)
    X(ConfigBitstreamRaw);
    X(ConfigMBcontrolRasterOrder);
    X(ConfigResidDiffHost);
    X(ConfigSpatialResid8);
    X(ConfigResid8Subtraction);
    X(ConfigSpatialHost8or9Clipping);
    X(ConfigSpatialResidInterleaved);
    X(ConfigIntraResidUnsigned);
    X(ConfigResidDiffAccelerator);
    X(ConfigHostInverseScan);
    X(ConfigSpecificIDCT);
    X(Config4GroupedCoefs);
    X(ConfigMinRenderTargetBuffCount);
    X(ConfigDecoderSpecific);
#undef X

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3D11VideoDecoder_iface.lpVtbl = &d3d11_video_decoder_vtbl;
    object->refcount = 1;

    wined3d_desc.codec = desc->Guid;
    wined3d_desc.width = desc->SampleWidth;
    wined3d_desc.height = desc->SampleHeight;
    wined3d_desc.output_format = wined3dformat_from_dxgi_format(desc->OutputFormat);
    wined3d_desc.long_slice_info = false;

    if (IsEqualGUID(&wined3d_desc.codec, &DXVA_ModeH264_VLD_NoFGT))
    {
        if (config->ConfigBitstreamRaw == 1)
        {
            wined3d_desc.long_slice_info = true;
        }
        else if (config->ConfigBitstreamRaw != 2)
        {
            FIXME("Unsupported ConfigBitstreamRaw value %u.\n", config->ConfigBitstreamRaw);
            return E_NOTIMPL;
        }
    }

    if (FAILED(hr = wined3d_decoder_create(device->wined3d_device, &wined3d_desc, &object->wined3d_decoder)))
    {
        free(object);
        return hr;
    }

    wined3d_private_store_init(&object->private_store);
    object->device = device;
    ID3D11Device5_AddRef(&device->ID3D11Device5_iface);

    TRACE("Created video decoder %p.\n", object);
    *decoder = object;
    return S_OK;
}

struct d3d_video_decoder *unsafe_impl_from_ID3D11VideoDecoder(ID3D11VideoDecoder *iface)
{
    assert(iface->lpVtbl == &d3d11_video_decoder_vtbl);
    return impl_from_ID3D11VideoDecoder(iface);
}
