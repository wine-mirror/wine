/*
 * Copyright 2018 Józef Kucia for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#define VK_NO_PROTOTYPES
#define VKD3D_NO_VULKAN_H
#define VKD3D_NO_WIN32_TYPES
#define WINE_VK_ALIGN(x)

#include "wine/debug.h"
#include "wine/vulkan.h"

#include "d3d12.h"

#include <vkd3d.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d12);

HRESULT WINAPI D3D12GetDebugInterface(REFIID iid, void **debug)
{
    FIXME("iid %s, debug %p stub!\n", debugstr_guid(iid), debug);

    return E_NOTIMPL;
}

HRESULT WINAPI D3D12CreateDevice(IUnknown *adapter, D3D_FEATURE_LEVEL minimum_feature_level,
        REFIID iid, void **device)
{
    FIXME("adapter %p, minimum_feature_level %#x, iid %s, device %p stub!\n",
            adapter, minimum_feature_level, debugstr_guid(iid), device);

    return E_NOTIMPL;
}

HRESULT WINAPI D3D12CreateRootSignatureDeserializer(const void *data, SIZE_T data_size,
        REFIID iid, void **deserializer)
{
    TRACE("data %p, data_size %lu, iid %s, deserializer %p.\n",
            data, data_size, debugstr_guid(iid), deserializer);

    return vkd3d_create_root_signature_deserializer(data, data_size, iid, deserializer);
}

HRESULT WINAPI D3D12SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC *root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION version, ID3DBlob **blob, ID3DBlob **error_blob)
{
    TRACE("root_signature_desc %p, version %#x, blob %p, error_blob %p.\n",
            root_signature_desc, version, blob, error_blob);

    return vkd3d_serialize_root_signature(root_signature_desc, version, blob, error_blob);
}
