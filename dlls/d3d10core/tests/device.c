/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#include <assert.h>
#define COBJMACROS
#include "initguid.h"
#include "d3d11.h"
#include "wine/test.h"
#include <limits.h>

#define BITS_NNAN 0xffc00000
#define BITS_NAN  0x7fc00000
#define BITS_NINF 0xff800000
#define BITS_INF  0x7f800000
#define BITS_N1_0 0xbf800000
#define BITS_1_0  0x3f800000

struct format_support
{
    DXGI_FORMAT format;
    BOOL optional;
};

static const struct format_support display_format_support[] =
{
    {DXGI_FORMAT_R8G8B8A8_UNORM},
    {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},
    {DXGI_FORMAT_B8G8R8A8_UNORM,             TRUE},
    {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,        TRUE},
    {DXGI_FORMAT_R16G16B16A16_FLOAT},
    {DXGI_FORMAT_R10G10B10A2_UNORM},
    {DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, TRUE},
};

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

struct uvec4
{
    unsigned int x, y, z, w;
};

static void set_box(D3D10_BOX *box, UINT left, UINT top, UINT front, UINT right, UINT bottom, UINT back)
{
    box->left = left;
    box->top = top;
    box->front = front;
    box->right = right;
    box->bottom = bottom;
    box->back = back;
}

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static BOOL compare_vec4(const struct vec4 *v1, const struct vec4 *v2, unsigned int ulps)
{
    return compare_float(v1->x, v2->x, ulps)
            && compare_float(v1->y, v2->y, ulps)
            && compare_float(v1->z, v2->z, ulps)
            && compare_float(v1->w, v2->w, ulps);
}

static BOOL compare_uvec4(const struct uvec4* v1, const struct uvec4 *v2)
{
    return v1->x == v2->x && v1->y == v2->y && v1->z == v2->z && v1->w == v2->w;
}

static BOOL compare_color(DWORD c1, DWORD c2, BYTE max_diff)
{
    if (abs((int)(c1 & 0xff) - (int)(c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((int)(c1 & 0xff) - (int)(c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((int)(c1 & 0xff) - (int)(c2 & 0xff)) > max_diff)
        return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((int)(c1 & 0xff) - (int)(c2 & 0xff)) > max_diff)
        return FALSE;
    return TRUE;
}

struct srv_desc
{
    DXGI_FORMAT format;
    D3D10_SRV_DIMENSION dimension;
    unsigned int miplevel_idx;
    unsigned int miplevel_count;
    unsigned int layer_idx;
    unsigned int layer_count;
};

static void get_srv_desc(D3D10_SHADER_RESOURCE_VIEW_DESC *d3d10_desc, const struct srv_desc *desc)
{
    d3d10_desc->Format = desc->format;
    d3d10_desc->ViewDimension = desc->dimension;
    if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURE1D)
    {
        U(*d3d10_desc).Texture1D.MostDetailedMip = desc->miplevel_idx;
        U(*d3d10_desc).Texture1D.MipLevels = desc->miplevel_count;
    }
    else if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURE1DARRAY)
    {
        U(*d3d10_desc).Texture1DArray.MostDetailedMip = desc->miplevel_idx;
        U(*d3d10_desc).Texture1DArray.MipLevels = desc->miplevel_count;
        U(*d3d10_desc).Texture1DArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture1DArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURE2D)
    {
        U(*d3d10_desc).Texture2D.MostDetailedMip = desc->miplevel_idx;
        U(*d3d10_desc).Texture2D.MipLevels = desc->miplevel_count;
    }
    else if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURE2DARRAY)
    {
        U(*d3d10_desc).Texture2DArray.MostDetailedMip = desc->miplevel_idx;
        U(*d3d10_desc).Texture2DArray.MipLevels = desc->miplevel_count;
        U(*d3d10_desc).Texture2DArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture2DArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY)
    {
        U(*d3d10_desc).Texture2DMSArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture2DMSArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURE3D)
    {
        U(*d3d10_desc).Texture3D.MostDetailedMip = desc->miplevel_idx;
        U(*d3d10_desc).Texture3D.MipLevels = desc->miplevel_count;
    }
    else if (desc->dimension == D3D10_SRV_DIMENSION_TEXTURECUBE)
    {
        U(*d3d10_desc).TextureCube.MostDetailedMip = desc->miplevel_idx;
        U(*d3d10_desc).TextureCube.MipLevels = desc->miplevel_count;
    }
    else if (desc->dimension != D3D10_SRV_DIMENSION_UNKNOWN
            && desc->dimension != D3D10_SRV_DIMENSION_TEXTURE2DMS)
    {
        trace("Unhandled view dimension %#x.\n", desc->dimension);
    }
}

#define check_srv_desc(a, b) check_srv_desc_(__LINE__, a, b)
static void check_srv_desc_(unsigned int line, const D3D10_SHADER_RESOURCE_VIEW_DESC *desc,
        const struct srv_desc *expected_desc)
{
    ok_(__FILE__, line)(desc->Format == expected_desc->format,
            "Got format %#x, expected %#x.\n", desc->Format, expected_desc->format);
    ok_(__FILE__, line)(desc->ViewDimension == expected_desc->dimension,
            "Got view dimension %#x, expected %#x.\n", desc->ViewDimension, expected_desc->dimension);

    if (desc->ViewDimension != expected_desc->dimension)
        return;

    if (desc->ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2D)
    {
        ok_(__FILE__, line)(U(*desc).Texture2D.MostDetailedMip == expected_desc->miplevel_idx,
                "Got MostDetailedMip %u, expected %u.\n",
                U(*desc).Texture2D.MostDetailedMip, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).Texture2D.MipLevels == expected_desc->miplevel_count,
                "Got MipLevels %u, expected %u.\n",
                U(*desc).Texture2D.MipLevels, expected_desc->miplevel_count);
    }
    else if (desc->ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2DARRAY)
    {
        ok_(__FILE__, line)(U(*desc).Texture2DArray.MostDetailedMip == expected_desc->miplevel_idx,
                "Got MostDetailedMip %u, expected %u.\n",
                U(*desc).Texture2DArray.MostDetailedMip, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.MipLevels == expected_desc->miplevel_count,
                "Got MipLevels %u, expected %u.\n",
                U(*desc).Texture2DArray.MipLevels, expected_desc->miplevel_count);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.FirstArraySlice == expected_desc->layer_idx,
                "Got FirstArraySlice %u, expected %u.\n",
                U(*desc).Texture2DArray.FirstArraySlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.ArraySize == expected_desc->layer_count,
                "Got ArraySize %u, expected %u.\n",
                U(*desc).Texture2DArray.ArraySize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY)
    {
        ok_(__FILE__, line)(U(*desc).Texture2DMSArray.FirstArraySlice == expected_desc->layer_idx,
                "Got FirstArraySlice %u, expected %u.\n",
                U(*desc).Texture2DMSArray.FirstArraySlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DMSArray.ArraySize == expected_desc->layer_count,
                "Got ArraySize %u, expected %u.\n",
                U(*desc).Texture2DMSArray.ArraySize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension == D3D10_SRV_DIMENSION_TEXTURE3D)
    {
        ok_(__FILE__, line)(U(*desc).Texture3D.MostDetailedMip == expected_desc->miplevel_idx,
                "Got MostDetailedMip %u, expected %u.\n",
                U(*desc).Texture3D.MostDetailedMip, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).Texture3D.MipLevels == expected_desc->miplevel_count,
                "Got MipLevels %u, expected %u.\n",
                U(*desc).Texture3D.MipLevels, expected_desc->miplevel_count);
    }
    else if (desc->ViewDimension == D3D10_SRV_DIMENSION_TEXTURECUBE)
    {
        ok_(__FILE__, line)(U(*desc).TextureCube.MostDetailedMip == expected_desc->miplevel_idx,
                "Got MostDetailedMip %u, expected %u.\n",
                U(*desc).TextureCube.MostDetailedMip, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).TextureCube.MipLevels == expected_desc->miplevel_count,
                "Got MipLevels %u, expected %u.\n",
                U(*desc).TextureCube.MipLevels, expected_desc->miplevel_count);
    }
    else if (desc->ViewDimension != D3D10_SRV_DIMENSION_TEXTURE2DMS)
    {
        trace("Unhandled view dimension %#x.\n", desc->ViewDimension);
    }
}

struct rtv_desc
{
    DXGI_FORMAT format;
    D3D10_RTV_DIMENSION dimension;
    unsigned int miplevel_idx;
    unsigned int layer_idx;
    unsigned int layer_count;
};

static void get_rtv_desc(D3D10_RENDER_TARGET_VIEW_DESC *d3d10_desc, const struct rtv_desc *desc)
{
    d3d10_desc->Format = desc->format;
    d3d10_desc->ViewDimension = desc->dimension;
    if (desc->dimension == D3D10_RTV_DIMENSION_TEXTURE1D)
    {
        U(*d3d10_desc).Texture1D.MipSlice = desc->miplevel_idx;
    }
    else if (desc->dimension == D3D10_RTV_DIMENSION_TEXTURE1DARRAY)
    {
        U(*d3d10_desc).Texture1DArray.MipSlice = desc->miplevel_idx;
        U(*d3d10_desc).Texture1DArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture1DArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_RTV_DIMENSION_TEXTURE2D)
    {
        U(*d3d10_desc).Texture2D.MipSlice = desc->miplevel_idx;
    }
    else if (desc->dimension == D3D10_RTV_DIMENSION_TEXTURE2DARRAY)
    {
        U(*d3d10_desc).Texture2DArray.MipSlice = desc->miplevel_idx;
        U(*d3d10_desc).Texture2DArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture2DArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY)
    {
        U(*d3d10_desc).Texture2DMSArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture2DMSArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_RTV_DIMENSION_TEXTURE3D)
    {
        U(*d3d10_desc).Texture3D.MipSlice = desc->miplevel_idx;
        U(*d3d10_desc).Texture3D.FirstWSlice = desc->layer_idx;
        U(*d3d10_desc).Texture3D.WSize = desc->layer_count;
    }
    else if (desc->dimension != D3D10_RTV_DIMENSION_UNKNOWN
            && desc->dimension != D3D10_RTV_DIMENSION_TEXTURE2DMS)
    {
        trace("Unhandled view dimension %#x.\n", desc->dimension);
    }
}

#define check_rtv_desc(a, b) check_rtv_desc_(__LINE__, a, b)
static void check_rtv_desc_(unsigned int line, const D3D10_RENDER_TARGET_VIEW_DESC *desc,
        const struct rtv_desc *expected_desc)
{
    ok_(__FILE__, line)(desc->Format == expected_desc->format,
            "Got format %#x, expected %#x.\n", desc->Format, expected_desc->format);
    ok_(__FILE__, line)(desc->ViewDimension == expected_desc->dimension,
            "Got view dimension %#x, expected %#x.\n", desc->ViewDimension, expected_desc->dimension);

    if (desc->ViewDimension != expected_desc->dimension)
        return;

    if (desc->ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2D)
    {
        ok_(__FILE__, line)(U(*desc).Texture2D.MipSlice == expected_desc->miplevel_idx,
                "Got MipSlice %u, expected %u.\n",
                U(*desc).Texture2D.MipSlice, expected_desc->miplevel_idx);
    }
    else if (desc->ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2DARRAY)
    {
        ok_(__FILE__, line)(U(*desc).Texture2DArray.MipSlice == expected_desc->miplevel_idx,
                "Got MipSlice %u, expected %u.\n",
                U(*desc).Texture2DArray.MipSlice, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.FirstArraySlice == expected_desc->layer_idx,
                "Got FirstArraySlice %u, expected %u.\n",
                U(*desc).Texture2DArray.FirstArraySlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.ArraySize == expected_desc->layer_count,
                "Got ArraySize %u, expected %u.\n",
                U(*desc).Texture2DArray.ArraySize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension == D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY)
    {
        ok_(__FILE__, line)(U(*desc).Texture2DMSArray.FirstArraySlice == expected_desc->layer_idx,
                "Got FirstArraySlice %u, expected %u.\n",
                U(*desc).Texture2DMSArray.FirstArraySlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DMSArray.ArraySize == expected_desc->layer_count,
                "Got ArraySize %u, expected %u.\n",
                U(*desc).Texture2DMSArray.ArraySize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension == D3D10_RTV_DIMENSION_TEXTURE3D)
    {
        ok_(__FILE__, line)(U(*desc).Texture3D.MipSlice == expected_desc->miplevel_idx,
                "Got MipSlice %u, expected %u.\n",
                U(*desc).Texture3D.MipSlice, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).Texture3D.FirstWSlice == expected_desc->layer_idx,
                "Got FirstWSlice %u, expected %u.\n",
                U(*desc).Texture3D.FirstWSlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture3D.WSize == expected_desc->layer_count,
                "Got WSize %u, expected %u.\n",
                U(*desc).Texture3D.WSize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension != D3D10_RTV_DIMENSION_TEXTURE2DMS)
    {
        trace("Unhandled view dimension %#x.\n", desc->ViewDimension);
    }
}

struct dsv_desc
{
    DXGI_FORMAT format;
    D3D10_DSV_DIMENSION dimension;
    unsigned int miplevel_idx;
    unsigned int layer_idx;
    unsigned int layer_count;
};

static void get_dsv_desc(D3D10_DEPTH_STENCIL_VIEW_DESC *d3d10_desc, const struct dsv_desc *desc)
{
    d3d10_desc->Format = desc->format;
    d3d10_desc->ViewDimension = desc->dimension;
    if (desc->dimension == D3D10_DSV_DIMENSION_TEXTURE1D)
    {
        U(*d3d10_desc).Texture1D.MipSlice = desc->miplevel_idx;
    }
    else if (desc->dimension == D3D10_DSV_DIMENSION_TEXTURE1DARRAY)
    {
        U(*d3d10_desc).Texture1DArray.MipSlice = desc->miplevel_idx;
        U(*d3d10_desc).Texture1DArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture1DArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_DSV_DIMENSION_TEXTURE2D)
    {
        U(*d3d10_desc).Texture2D.MipSlice = desc->miplevel_idx;
    }
    else if (desc->dimension == D3D10_DSV_DIMENSION_TEXTURE2DARRAY)
    {
        U(*d3d10_desc).Texture2DArray.MipSlice = desc->miplevel_idx;
        U(*d3d10_desc).Texture2DArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture2DArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension == D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY)
    {
        U(*d3d10_desc).Texture2DMSArray.FirstArraySlice = desc->layer_idx;
        U(*d3d10_desc).Texture2DMSArray.ArraySize = desc->layer_count;
    }
    else if (desc->dimension != D3D10_DSV_DIMENSION_UNKNOWN
            && desc->dimension != D3D10_DSV_DIMENSION_TEXTURE2DMS)
    {
        trace("Unhandled view dimension %#x.\n", desc->dimension);
    }
}

#define check_dsv_desc(a, b) check_dsv_desc_(__LINE__, a, b)
static void check_dsv_desc_(unsigned int line, const D3D10_DEPTH_STENCIL_VIEW_DESC *desc,
        const struct dsv_desc *expected_desc)
{
    ok_(__FILE__, line)(desc->Format == expected_desc->format,
            "Got format %#x, expected %#x.\n", desc->Format, expected_desc->format);
    ok_(__FILE__, line)(desc->ViewDimension == expected_desc->dimension,
            "Got view dimension %#x, expected %#x.\n", desc->ViewDimension, expected_desc->dimension);

    if (desc->ViewDimension != expected_desc->dimension)
        return;

    if (desc->ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2D)
    {
        ok_(__FILE__, line)(U(*desc).Texture2D.MipSlice == expected_desc->miplevel_idx,
                "Got MipSlice %u, expected %u.\n",
                U(*desc).Texture2D.MipSlice, expected_desc->miplevel_idx);
    }
    else if (desc->ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2DARRAY)
    {
        ok_(__FILE__, line)(U(*desc).Texture2DArray.MipSlice == expected_desc->miplevel_idx,
                "Got MipSlice %u, expected %u.\n",
                U(*desc).Texture2DArray.MipSlice, expected_desc->miplevel_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.FirstArraySlice == expected_desc->layer_idx,
                "Got FirstArraySlice %u, expected %u.\n",
                U(*desc).Texture2DArray.FirstArraySlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DArray.ArraySize == expected_desc->layer_count,
                "Got ArraySize %u, expected %u.\n",
                U(*desc).Texture2DArray.ArraySize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY)
    {
        ok_(__FILE__, line)(U(*desc).Texture2DMSArray.FirstArraySlice == expected_desc->layer_idx,
                "Got FirstArraySlice %u, expected %u.\n",
                U(*desc).Texture2DMSArray.FirstArraySlice, expected_desc->layer_idx);
        ok_(__FILE__, line)(U(*desc).Texture2DMSArray.ArraySize == expected_desc->layer_count,
                "Got ArraySize %u, expected %u.\n",
                U(*desc).Texture2DMSArray.ArraySize, expected_desc->layer_count);
    }
    else if (desc->ViewDimension != D3D10_DSV_DIMENSION_TEXTURE2DMS)
    {
        trace("Unhandled view dimension %#x.\n", desc->ViewDimension);
    }
}

#define create_buffer(a, b, c, d) create_buffer_(__LINE__, a, b, c, d)
static ID3D10Buffer *create_buffer_(unsigned int line, ID3D10Device *device,
        unsigned int bind_flags, unsigned int size, const void *data)
{
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10Buffer *buffer;
    HRESULT hr;

    buffer_desc.ByteWidth = size;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = bind_flags;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    resource_data.pSysMem = data;
    resource_data.SysMemPitch = 0;
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D10Device_CreateBuffer(device, &buffer_desc, data ? &resource_data : NULL, &buffer);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    return buffer;
}

struct resource_readback
{
    D3D10_RESOURCE_DIMENSION dimension;
    ID3D10Resource *resource;
    D3D10_MAPPED_TEXTURE2D map_desc;
    unsigned int width, height, sub_resource_idx;
};

static void get_buffer_readback(ID3D10Buffer *buffer, struct resource_readback *rb)
{
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));
    rb->dimension = D3D10_RESOURCE_DIMENSION_BUFFER;

    ID3D10Buffer_GetDevice(buffer, &device);

    ID3D10Buffer_GetDesc(buffer, &buffer_desc);
    buffer_desc.Usage = D3D10_USAGE_STAGING;
    buffer_desc.BindFlags = 0;
    buffer_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    buffer_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, (ID3D10Buffer **)&rb->resource)))
    {
        trace("Failed to create texture, hr %#x.\n", hr);
        ID3D10Device_Release(device);
        return;
    }

    rb->width = buffer_desc.ByteWidth;
    rb->height = 1;
    rb->sub_resource_idx = 0;

    ID3D10Device_CopyResource(device, rb->resource, (ID3D10Resource *)buffer);
    if (FAILED(hr = ID3D10Buffer_Map((ID3D10Buffer *)rb->resource, D3D10_MAP_READ, 0, &rb->map_desc.pData)))
    {
        trace("Failed to map buffer, hr %#x.\n", hr);
        ID3D10Resource_Release(rb->resource);
        rb->resource = NULL;
    }
    rb->map_desc.RowPitch = 0;

    ID3D10Device_Release(device);
}

static void get_texture_readback(ID3D10Texture2D *texture, unsigned int sub_resource_idx,
        struct resource_readback *rb)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    unsigned int miplevel;
    ID3D10Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));
    rb->dimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;

    ID3D10Texture2D_GetDevice(texture, &device);

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D10_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D10Texture2D **)&rb->resource)))
    {
        trace("Failed to create texture, hr %#x.\n", hr);
        ID3D10Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    rb->width = max(1, texture_desc.Width >> miplevel);
    rb->height = max(1, texture_desc.Height >> miplevel);
    rb->sub_resource_idx = sub_resource_idx;

    ID3D10Device_CopyResource(device, rb->resource, (ID3D10Resource *)texture);
    if (FAILED(hr = ID3D10Texture2D_Map((ID3D10Texture2D *)rb->resource, sub_resource_idx,
            D3D10_MAP_READ, 0, &rb->map_desc)))
    {
        trace("Failed to map sub-resource %u, hr %#x.\n", sub_resource_idx, hr);
        ID3D10Resource_Release(rb->resource);
        rb->resource = NULL;
    }

    ID3D10Device_Release(device);
}

static DWORD get_readback_color(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return ((DWORD *)rb->map_desc.pData)[rb->map_desc.RowPitch * y / sizeof(DWORD)  + x];
}

static float get_readback_float(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return ((float *)rb->map_desc.pData)[rb->map_desc.RowPitch * y / sizeof(float) + x];
}

static const struct vec4 *get_readback_vec4(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return &((const struct vec4 *)rb->map_desc.pData)[rb->map_desc.RowPitch * y / sizeof(struct vec4) + x];
}

static const struct uvec4 *get_readback_uvec4(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return &((const struct uvec4 *)rb->map_desc.pData)[rb->map_desc.RowPitch * y / sizeof(struct uvec4) + x];
}

static void release_resource_readback(struct resource_readback *rb)
{
    switch (rb->dimension)
    {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            ID3D10Buffer_Unmap((ID3D10Buffer *)rb->resource);
            break;
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            ID3D10Texture2D_Unmap((ID3D10Texture2D *)rb->resource, rb->sub_resource_idx);
            break;
        default:
            trace("Unhandled resource dimension %#x.\n", rb->dimension);
            break;
    }
    ID3D10Resource_Release(rb->resource);
}

static DWORD get_texture_color(ID3D10Texture2D *texture, unsigned int x, unsigned int y)
{
    struct resource_readback rb;
    DWORD color;

    get_texture_readback(texture, 0, &rb);
    color = get_readback_color(&rb, x, y);
    release_resource_readback(&rb);

    return color;
}

#define check_texture_sub_resource_color(t, s, c, d) check_texture_sub_resource_color_(__LINE__, t, s, c, d)
static void check_texture_sub_resource_color_(unsigned int line, ID3D10Texture2D *texture,
        unsigned int sub_resource_idx, DWORD expected_color, BYTE max_diff)
{
    struct resource_readback rb;
    unsigned int x = 0, y = 0;
    BOOL all_match = TRUE;
    DWORD color = 0;

    get_texture_readback(texture, sub_resource_idx, &rb);
    for (y = 0; y < rb.height; ++y)
    {
        for (x = 0; x < rb.width; ++x)
        {
            color = get_readback_color(&rb, x, y);
            if (!compare_color(color, expected_color, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    release_resource_readback(&rb);
    ok_(__FILE__, line)(all_match,
            "Got 0x%08x, expected 0x%08x at (%u, %u), sub-resource %u.\n",
            color, expected_color, x, y, sub_resource_idx);
}

#define check_texture_color(t, c, d) check_texture_color_(__LINE__, t, c, d)
static void check_texture_color_(unsigned int line, ID3D10Texture2D *texture,
        DWORD expected_color, BYTE max_diff)
{
    unsigned int sub_resource_idx, sub_resource_count;
    D3D10_TEXTURE2D_DESC texture_desc;

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    sub_resource_count = texture_desc.ArraySize * texture_desc.MipLevels;
    for (sub_resource_idx = 0; sub_resource_idx < sub_resource_count; ++sub_resource_idx)
        check_texture_sub_resource_color_(line, texture, sub_resource_idx, expected_color, max_diff);
}

#define check_texture_sub_resource_float(r, s, f, d) check_texture_sub_resource_float_(__LINE__, r, s, f, d)
static void check_texture_sub_resource_float_(unsigned int line, ID3D10Texture2D *texture,
        unsigned int sub_resource_idx, float expected_value, BYTE max_diff)
{
    struct resource_readback rb;
    unsigned int x = 0, y = 0;
    BOOL all_match = TRUE;
    float value = 0.0f;

    get_texture_readback(texture, sub_resource_idx, &rb);
    for (y = 0; y < rb.height; ++y)
    {
        for (x = 0; x < rb.width; ++x)
        {
            value = get_readback_float(&rb, x, y);
            if (!compare_float(value, expected_value, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    release_resource_readback(&rb);
    ok_(__FILE__, line)(all_match,
            "Got %.8e, expected %.8e at (%u, %u), sub-resource %u.\n",
            value, expected_value, x, y, sub_resource_idx);
}

#define check_texture_float(r, f, d) check_texture_float_(__LINE__, r, f, d)
static void check_texture_float_(unsigned int line, ID3D10Texture2D *texture,
        float expected_value, BYTE max_diff)
{
    unsigned int sub_resource_idx, sub_resource_count;
    D3D10_TEXTURE2D_DESC texture_desc;

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    sub_resource_count = texture_desc.ArraySize * texture_desc.MipLevels;
    for (sub_resource_idx = 0; sub_resource_idx < sub_resource_count; ++sub_resource_idx)
        check_texture_sub_resource_float_(line, texture, sub_resource_idx, expected_value, max_diff);
}

#define check_texture_sub_resource_vec4(a, b, c, d) check_texture_sub_resource_vec4_(__LINE__, a, b, c, d)
static void check_texture_sub_resource_vec4_(unsigned int line, ID3D10Texture2D *texture,
        unsigned int sub_resource_idx, const struct vec4 *expected_value, BYTE max_diff)
{
    struct resource_readback rb;
    unsigned int x = 0, y = 0;
    struct vec4 value = {0};
    BOOL all_match = TRUE;

    get_texture_readback(texture, sub_resource_idx, &rb);
    for (y = 0; y < rb.height; ++y)
    {
        for (x = 0; x < rb.width; ++x)
        {
            value = *get_readback_vec4(&rb, x, y);
            if (!compare_vec4(&value, expected_value, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    release_resource_readback(&rb);
    ok_(__FILE__, line)(all_match,
            "Got {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e} at (%u, %u), sub-resource %u.\n",
            value.x, value.y, value.z, value.w,
            expected_value->x, expected_value->y, expected_value->z, expected_value->w,
            x, y, sub_resource_idx);
}

#define check_texture_vec4(a, b, c) check_texture_vec4_(__LINE__, a, b, c)
static void check_texture_vec4_(unsigned int line, ID3D10Texture2D *texture,
        const struct vec4 *expected_value, BYTE max_diff)
{
    unsigned int sub_resource_idx, sub_resource_count;
    D3D10_TEXTURE2D_DESC texture_desc;

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    sub_resource_count = texture_desc.ArraySize * texture_desc.MipLevels;
    for (sub_resource_idx = 0; sub_resource_idx < sub_resource_count; ++sub_resource_idx)
        check_texture_sub_resource_vec4_(line, texture, sub_resource_idx, expected_value, max_diff);
}

#define check_texture_sub_resource_uvec4(a, b, c) check_texture_sub_resource_uvec4_(__LINE__, a, b, c)
static void check_texture_sub_resource_uvec4_(unsigned int line, ID3D10Texture2D *texture,
        unsigned int sub_resource_idx, const struct uvec4 *expected_value)
{
    struct resource_readback rb;
    unsigned int x = 0, y = 0;
    struct uvec4 value = {0};
    BOOL all_match = TRUE;

    get_texture_readback(texture, sub_resource_idx, &rb);
    for (y = 0; y < rb.height; ++y)
    {
        for (x = 0; x < rb.width; ++x)
        {
            value = *get_readback_uvec4(&rb, x, y);
            if (!compare_uvec4(&value, expected_value))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    release_resource_readback(&rb);
    ok_(__FILE__, line)(all_match,
            "Got {0x%08x, 0x%08x, 0x%08x, 0x%08x}, expected {0x%08x, 0x%08x, 0x%08x, 0x%08x} "
            "at (%u, %u), sub-resource %u.\n",
            value.x, value.y, value.z, value.w,
            expected_value->x, expected_value->y, expected_value->z, expected_value->w,
            x, y, sub_resource_idx);
}

#define check_texture_uvec4(a, b) check_texture_uvec4_(__LINE__, a, b)
static void check_texture_uvec4_(unsigned int line, ID3D10Texture2D *texture,
        const struct uvec4 *expected_value)
{
    unsigned int sub_resource_idx, sub_resource_count;
    D3D10_TEXTURE2D_DESC texture_desc;

    ID3D10Texture2D_GetDesc(texture, &texture_desc);
    sub_resource_count = texture_desc.ArraySize * texture_desc.MipLevels;
    for (sub_resource_idx = 0; sub_resource_idx < sub_resource_count; ++sub_resource_idx)
        check_texture_sub_resource_uvec4_(line, texture, sub_resource_idx, expected_value);
}

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static void get_device_adapter_desc(ID3D10Device *device, DXGI_ADAPTER_DESC *adapter_desc)
{
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    HRESULT hr;

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to query IDXGIDevice interface, hr %#x.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetDesc(adapter, adapter_desc);
    ok(SUCCEEDED(hr), "Failed to get adapter desc, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);
}

static BOOL is_warp_device(ID3D10Device *device)
{
    DXGI_ADAPTER_DESC adapter_desc;
    get_device_adapter_desc(device, &adapter_desc);
    return !adapter_desc.SubSysId && !adapter_desc.Revision
            && ((!adapter_desc.VendorId && !adapter_desc.DeviceId)
            || (adapter_desc.VendorId == 0x1414 && adapter_desc.DeviceId == 0x008c));
}

static BOOL is_amd_device(ID3D10Device *device)
{
    DXGI_ADAPTER_DESC adapter_desc;

    if (!strcmp(winetest_platform, "wine"))
        return FALSE;

    get_device_adapter_desc(device, &adapter_desc);
    return adapter_desc.VendorId == 0x1002;
}

static BOOL is_d3d11_interface_available(ID3D10Device *device)
{
    ID3D11Device *d3d11_device;
    HRESULT hr;

    if (SUCCEEDED(hr = ID3D10Device_QueryInterface(device, &IID_ID3D11Device, (void **)&d3d11_device)))
        ID3D11Device_Release(d3d11_device);

    return SUCCEEDED(hr);
}

#define SWAPCHAIN_FLAG_SHADER_INPUT             0x1

struct swapchain_desc
{
    BOOL windowed;
    UINT buffer_count;
    DXGI_SWAP_EFFECT swap_effect;
    DWORD flags;
};

static IDXGISwapChain *create_swapchain(ID3D10Device *device, HWND window, const struct swapchain_desc *swapchain_desc)
{
    IDXGISwapChain *swapchain;
    DXGI_SWAP_CHAIN_DESC dxgi_desc;
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to get factory, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    dxgi_desc.BufferDesc.Width = 640;
    dxgi_desc.BufferDesc.Height = 480;
    dxgi_desc.BufferDesc.RefreshRate.Numerator = 60;
    dxgi_desc.BufferDesc.RefreshRate.Denominator = 1;
    dxgi_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgi_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    dxgi_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    dxgi_desc.SampleDesc.Count = 1;
    dxgi_desc.SampleDesc.Quality = 0;
    dxgi_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_desc.BufferCount = 1;
    dxgi_desc.OutputWindow = window;
    dxgi_desc.Windowed = TRUE;
    dxgi_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    dxgi_desc.Flags = 0;

    if (swapchain_desc)
    {
        dxgi_desc.Windowed = swapchain_desc->windowed;
        dxgi_desc.SwapEffect = swapchain_desc->swap_effect;
        dxgi_desc.BufferCount = swapchain_desc->buffer_count;

        if (swapchain_desc->flags & SWAPCHAIN_FLAG_SHADER_INPUT)
            dxgi_desc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
    }

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &dxgi_desc, &swapchain);
    ok(SUCCEEDED(hr), "Failed to create swapchain, hr %#x.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

struct d3d10core_test_context
{
    ID3D10Device *device;
    HWND window;
    IDXGISwapChain *swapchain;
    ID3D10Texture2D *backbuffer;
    ID3D10RenderTargetView *backbuffer_rtv;

    ID3D10InputLayout *input_layout;
    ID3D10VertexShader *vs;
    ID3D10Buffer *vb;

    ID3D10PixelShader *ps;
    ID3D10Buffer *ps_cb;
};

#define init_test_context(c) init_test_context_(__LINE__, c)
static BOOL init_test_context_(unsigned int line, struct d3d10core_test_context *context)
{
    D3D10_VIEWPORT vp;
    HRESULT hr;
    RECT rect;

    memset(context, 0, sizeof(*context));

    if (!(context->device = create_device()))
    {
        skip_(__FILE__, line)("Failed to create device.\n");
        return FALSE;
    }
    SetRect(&rect, 0, 0, 640, 480);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);
    context->window = CreateWindowA("static", "d3d10core_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    context->swapchain = create_swapchain(context->device, context->window, NULL);
    hr = IDXGISwapChain_GetBuffer(context->swapchain, 0, &IID_ID3D10Texture2D, (void **)&context->backbuffer);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(context->device, (ID3D10Resource *)context->backbuffer,
            NULL, &context->backbuffer_rtv);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);

    ID3D10Device_OMSetRenderTargets(context->device, 1, &context->backbuffer_rtv, NULL);

    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D10Device_RSSetViewports(context->device, 1, &vp);

    return TRUE;
}

#define release_test_context(c) release_test_context_(__LINE__, c)
static void release_test_context_(unsigned int line, struct d3d10core_test_context *context)
{
    ULONG ref;

    if (context->input_layout)
        ID3D10InputLayout_Release(context->input_layout);
    if (context->vs)
        ID3D10VertexShader_Release(context->vs);
    if (context->vb)
        ID3D10Buffer_Release(context->vb);
    if (context->ps)
        ID3D10PixelShader_Release(context->ps);
    if (context->ps_cb)
        ID3D10Buffer_Release(context->ps_cb);

    ID3D10RenderTargetView_Release(context->backbuffer_rtv);
    ID3D10Texture2D_Release(context->backbuffer);
    IDXGISwapChain_Release(context->swapchain);
    DestroyWindow(context->window);

    ref = ID3D10Device_Release(context->device);
    ok_(__FILE__, line)(!ref, "Device has %u references left.\n", ref);
}

#define draw_quad(c) draw_quad_(__LINE__, c)
static void draw_quad_(unsigned int line, struct d3d10core_test_context *context)
{
    static const D3D10_INPUT_ELEMENT_DESC default_layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD default_vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };

    ID3D10Device *device = context->device;
    unsigned int stride, offset;
    HRESULT hr;

    if (!context->input_layout)
    {
        hr = ID3D10Device_CreateInputLayout(device, default_layout_desc,
                sizeof(default_layout_desc) / sizeof(*default_layout_desc),
                default_vs_code, sizeof(default_vs_code), &context->input_layout);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

        context->vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quad), quad);

        hr = ID3D10Device_CreateVertexShader(device, default_vs_code, sizeof(default_vs_code), &context->vs);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    }

    ID3D10Device_IASetInputLayout(context->device, context->input_layout);
    ID3D10Device_IASetPrimitiveTopology(context->device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(context->device, 0, 1, &context->vb, &stride, &offset);
    ID3D10Device_VSSetShader(context->device, context->vs);

    ID3D10Device_Draw(context->device, 4, 0);
}

#define draw_color_quad(c, color) draw_color_quad_(__LINE__, c, color)
static void draw_color_quad_(unsigned int line, struct d3d10core_test_context *context, const struct vec4 *color)
{
    static const DWORD ps_color_code[] =
    {
#if 0
        float4 color;

        float4 main() : SV_TARGET
        {
            return color;
        }
#endif
        0x43425844, 0x80f1c810, 0xdacbbc8b, 0xe07b133e, 0x3059cbfa, 0x00000001, 0x000000b8, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000040, 0x00000040, 0x00000010,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x06000036,
        0x001020f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000000, 0x0100003e,
    };

    ID3D10Device *device = context->device;
    HRESULT hr;

    if (!context->ps)
    {
        hr = ID3D10Device_CreatePixelShader(device, ps_color_code, sizeof(ps_color_code), &context->ps);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    }

    if (!context->ps_cb)
        context->ps_cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(*color), NULL);

    ID3D10Device_PSSetShader(device, context->ps);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &context->ps_cb);

    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)context->ps_cb, 0, NULL, color, 0, 0);

    draw_quad_(line, context);
}

static void test_feature_level(void)
{
    D3D_FEATURE_LEVEL feature_level;
    ID3D11Device *device11;
    ID3D10Device *device10;
    HRESULT hr;

    if (!(device10 = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_QueryInterface(device10, &IID_ID3D11Device, (void **)&device11);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Failed to query ID3D11Device interface, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        win_skip("D3D11 is not available.\n");
        ID3D10Device_Release(device10);
        return;
    }

    /* Device was created by D3D10CreateDevice. */
    feature_level = ID3D11Device_GetFeatureLevel(device11);
    ok(feature_level == D3D_FEATURE_LEVEL_10_0, "Got unexpected feature level %#x.\n", feature_level);

    ID3D11Device_Release(device11);
    ID3D10Device_Release(device10);
}

static void test_device_interfaces(void)
{
    IDXGIAdapter *dxgi_adapter;
    IDXGIDevice *dxgi_device;
    ID3D10Device *device;
    IUnknown *iface;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D10Device_QueryInterface(device, &IID_IUnknown, (void **)&iface);
    ok(SUCCEEDED(hr), "Device should implement IUnknown interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIObject, (void **)&iface);
    ok(SUCCEEDED(hr), "Device should implement IDXGIObject interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Device should implement IDXGIDevice.\n");
    hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter, (void **)&dxgi_adapter);
    ok(SUCCEEDED(hr), "Device parent should implement IDXGIAdapter.\n");
    hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory, (void **)&iface);
    ok(SUCCEEDED(hr), "Adapter parent should implement IDXGIFactory.\n");
    IUnknown_Release(iface);
    IUnknown_Release(dxgi_adapter);
    hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter1, (void **)&dxgi_adapter);
    ok(SUCCEEDED(hr), "Device parent should implement IDXGIAdapter1.\n");
    hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory1, (void **)&iface);
    ok(hr == E_NOINTERFACE, "Adapter parent should not implement IDXGIFactory1.\n");
    IUnknown_Release(dxgi_adapter);
    IUnknown_Release(dxgi_device);

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Device should implement IDXGIDevice1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Multithread, (void **)&iface);
    ok(SUCCEEDED(hr), "Device should implement ID3D10Multithread interface, hr %#x.\n", hr);
    IUnknown_Release(iface);

    hr = ID3D10Device_QueryInterface(device, &IID_ID3D10Device1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Device should implement ID3D10Device1 interface, hr %#x.\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    hr = ID3D10Device_QueryInterface(device, &IID_ID3D11Device, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Device should implement ID3D11Device interface, hr %#x.\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_texture2d(void)
{
    ULONG refcount, expected_refcount;
    D3D10_SUBRESOURCE_DATA data = {0};
    ID3D10Device *device, *tmp;
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Texture2D *texture;
    UINT quality_level_count;
    IDXGISurface *surface;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        DXGI_FORMAT format;
        UINT array_size;
        D3D10_BIND_FLAG bind_flags;
        UINT misc_flags;
        BOOL succeeds;
        BOOL todo;
    }
    tests[] =
    {
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_VERTEX_BUFFER,   0, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_INDEX_BUFFER,    0, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_CONSTANT_BUFFER, 0, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  0, D3D10_BIND_SHADER_RESOURCE, 0, FALSE, FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  2, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  3, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  3, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  5, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  6, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  7, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, 10, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, 12, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  0, D3D10_BIND_RENDER_TARGET,   0, FALSE, FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  2, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  9, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS,  1, D3D10_BIND_DEPTH_STENCIL,   0, FALSE, FALSE},
        {DXGI_FORMAT_R32G32B32A32_UINT,      1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32A32_SINT,      1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32B32_TYPELESS,     1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16B16A16_TYPELESS,  1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16B16A16_TYPELESS,  1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G32_TYPELESS,        1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32G8X24_TYPELESS,      1, D3D10_BIND_DEPTH_STENCIL,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R10G10B10A2_TYPELESS,   1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R10G10B10A2_TYPELESS,   1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16_TYPELESS,        1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16_UNORM,           1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16_SNORM,           1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           0, D3D10_BIND_SHADER_RESOURCE, 0, FALSE, FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           9, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           9, D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_DEPTH_STENCIL, 0,
                TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           9, D3D10_BIND_SHADER_RESOURCE, D3D10_RESOURCE_MISC_TEXTURECUBE,
                FALSE, TRUE},
        {DXGI_FORMAT_R32_TYPELESS,           1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R32_TYPELESS,           1, D3D10_BIND_RENDER_TARGET | D3D10_BIND_DEPTH_STENCIL, 0,
                FALSE, TRUE},
        {DXGI_FORMAT_R32_TYPELESS,           1, D3D10_BIND_DEPTH_STENCIL,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D10_BIND_VERTEX_BUFFER,   0, FALSE, TRUE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D10_BIND_INDEX_BUFFER,    0, FALSE, TRUE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D10_BIND_CONSTANT_BUFFER, 0, FALSE, TRUE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R24G8_TYPELESS,         1, D3D10_BIND_DEPTH_STENCIL,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8_TYPELESS,          1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8_TYPELESS,          1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8_UNORM,             1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8_SNORM,             1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R16_TYPELESS,           1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R16_TYPELESS,           1, D3D10_BIND_DEPTH_STENCIL,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R16_TYPELESS,           1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R16_UINT,               1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R16_SINT,               1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8_TYPELESS,            1, D3D10_BIND_SHADER_RESOURCE, 0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,         1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,         1, D3D10_BIND_DEPTH_STENCIL,   0, FALSE, FALSE},
        {DXGI_FORMAT_R8G8B8A8_UINT,          1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_SNORM,         1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_SINT,          1, D3D10_BIND_RENDER_TARGET,   0, TRUE,  FALSE},
        {DXGI_FORMAT_D24_UNORM_S8_UINT,      1, D3D10_BIND_SHADER_RESOURCE, 0, FALSE, TRUE},
        {DXGI_FORMAT_D24_UNORM_S8_UINT,      1, D3D10_BIND_RENDER_TARGET,   0, FALSE, FALSE},
        {DXGI_FORMAT_D32_FLOAT,              1, D3D10_BIND_SHADER_RESOURCE, 0, FALSE, TRUE},
        {DXGI_FORMAT_D32_FLOAT,              1, D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_DEPTH_STENCIL, 0,
                FALSE, TRUE},
        {DXGI_FORMAT_D32_FLOAT,              1, D3D10_BIND_RENDER_TARGET,   0, FALSE, FALSE},
        {DXGI_FORMAT_D32_FLOAT,              1, D3D10_BIND_DEPTH_STENCIL,   0, TRUE,  FALSE},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &desc, &data, &texture);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture2D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Texture should implement IDXGISurface\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);

    desc.MipLevels = 0;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture2D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10Texture2D_GetDesc(texture, &desc);
    ok(desc.Width == 512, "Got unexpected Width %u.\n", desc.Width);
    ok(desc.Height == 512, "Got unexpected Height %u.\n", desc.Height);
    ok(desc.MipLevels == 10, "Got unexpected MipLevels %u.\n", desc.MipLevels);
    ok(desc.ArraySize == 1, "Got unexpected ArraySize %u.\n", desc.ArraySize);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", desc.Format);
    ok(desc.SampleDesc.Count == 1, "Got unexpected SampleDesc.Count %u.\n", desc.SampleDesc.Count);
    ok(desc.SampleDesc.Quality == 0, "Got unexpected SampleDesc.Quality %u.\n", desc.SampleDesc.Quality);
    ok(desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected Usage %u.\n", desc.Usage);
    ok(desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %u.\n", desc.BindFlags);
    ok(desc.CPUAccessFlags == 0, "Got unexpected CPUAccessFlags %u.\n", desc.CPUAccessFlags);
    ok(desc.MiscFlags == 0, "Got unexpected MiscFlags %u.\n", desc.MiscFlags);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);

    desc.MipLevels = 1;
    desc.ArraySize = 2;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);

    ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &quality_level_count);
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 2;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    if (quality_level_count)
    {
        ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
        ID3D10Texture2D_Release(texture);
        desc.SampleDesc.Quality = quality_level_count;
        hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    }
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    /* We assume 15 samples multisampling is never supported in practice. */
    desc.SampleDesc.Count = 15;
    desc.SampleDesc.Quality = 0;
    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    desc.SampleDesc.Count = 1;
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        desc.ArraySize = tests[i].array_size;
        desc.Format = tests[i].format;
        desc.BindFlags = tests[i].bind_flags;
        desc.MiscFlags = tests[i].misc_flags;
        hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, (ID3D10Texture2D **)&texture);

        todo_wine_if(tests[i].todo)
        ok(hr == (tests[i].succeeds ? S_OK : E_INVALIDARG),
                "Test %u: Got unexpected hr %#x (format %#x).\n", i, hr, desc.Format);

        if (SUCCEEDED(hr))
            ID3D10Texture2D_Release(texture);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_texture2d_interfaces(void)
{
    ID3D11Texture2D *d3d11_texture;
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Texture2D *texture;
    IDXGISurface *surface;
    ID3D10Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct test
    {
        UINT bind_flags;
        UINT misc_flags;
        UINT expected_bind_flags;
        UINT expected_misc_flags;
    }
    desc_conversion_tests[] =
    {
        {
            D3D10_BIND_RENDER_TARGET, 0,
            D3D11_BIND_RENDER_TARGET, 0
        },
        {
            0, D3D10_RESOURCE_MISC_SHARED,
            0, D3D11_RESOURCE_MISC_SHARED
        },
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(hr == E_NOINTERFACE, "Texture should not implement IDXGISurface.\n");

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_ID3D11Texture2D, (void **)&d3d11_texture);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Texture should implement ID3D11Texture2D.\n");
    if (SUCCEEDED(hr)) ID3D11Texture2D_Release(d3d11_texture);
    ID3D10Texture2D_Release(texture);

    if (FAILED(hr))
    {
        win_skip("D3D11 is not available, skipping tests.\n");
        ID3D10Device_Release(device);
        return;
    }

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D11_TEXTURE2D_DESC d3d11_desc;
        ID3D11Device *d3d11_device;

        desc.Width = 512;
        desc.Height = 512;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D10_USAGE_DEFAULT;
        desc.BindFlags = current->bind_flags;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = current->misc_flags;

        hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &texture);
        /* Shared resources are not supported by REF and WARP devices. */
        ok(SUCCEEDED(hr) || broken(hr == E_OUTOFMEMORY),
                "Test %u: Failed to create a 2d texture, hr %#x.\n", i, hr);
        if (FAILED(hr))
        {
            win_skip("Failed to create ID3D10Texture2D, skipping test %u.\n", i);
            continue;
        }

        hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
        ok(SUCCEEDED(hr), "Test %u: Texture should implement IDXGISurface.\n", i);
        IDXGISurface_Release(surface);

        hr = ID3D10Texture2D_QueryInterface(texture, &IID_ID3D11Texture2D, (void **)&d3d11_texture);
        ok(SUCCEEDED(hr), "Test %u: Texture should implement ID3D11Texture2D.\n", i);
        ID3D10Texture2D_Release(texture);

        ID3D11Texture2D_GetDesc(d3d11_texture, &d3d11_desc);

        ok(d3d11_desc.Width == desc.Width,
                "Test %u: Got unexpected Width %u.\n", i, d3d11_desc.Width);
        ok(d3d11_desc.Height == desc.Height,
                "Test %u: Got unexpected Height %u.\n", i, d3d11_desc.Height);
        ok(d3d11_desc.MipLevels == desc.MipLevels,
                "Test %u: Got unexpected MipLevels %u.\n", i, d3d11_desc.MipLevels);
        ok(d3d11_desc.ArraySize == desc.ArraySize,
                "Test %u: Got unexpected ArraySize %u.\n", i, d3d11_desc.ArraySize);
        ok(d3d11_desc.Format == desc.Format,
                "Test %u: Got unexpected Format %u.\n", i, d3d11_desc.Format);
        ok(d3d11_desc.SampleDesc.Count == desc.SampleDesc.Count,
                "Test %u: Got unexpected SampleDesc.Count %u.\n", i, d3d11_desc.SampleDesc.Count);
        ok(d3d11_desc.SampleDesc.Quality == desc.SampleDesc.Quality,
                "Test %u: Got unexpected SampleDesc.Quality %u.\n", i, d3d11_desc.SampleDesc.Quality);
        ok(d3d11_desc.Usage == (D3D11_USAGE)desc.Usage,
                "Test %u: Got unexpected Usage %u.\n", i, d3d11_desc.Usage);
        ok(d3d11_desc.BindFlags == current->expected_bind_flags,
                "Test %u: Got unexpected BindFlags %#x.\n", i, d3d11_desc.BindFlags);
        ok(d3d11_desc.CPUAccessFlags == desc.CPUAccessFlags,
                "Test %u: Got unexpected CPUAccessFlags %#x.\n", i, d3d11_desc.CPUAccessFlags);
        ok(d3d11_desc.MiscFlags == current->expected_misc_flags,
                "Test %u: Got unexpected MiscFlags %#x.\n", i, d3d11_desc.MiscFlags);

        d3d11_device = NULL;
        ID3D11Texture2D_GetDevice(d3d11_texture, &d3d11_device);
        ok(!!d3d11_device, "Test %u: Got NULL, expected device pointer.\n", i);
        ID3D11Device_Release(d3d11_device);

        ID3D11Texture2D_Release(d3d11_texture);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_texture3d(void)
{
    ULONG refcount, expected_refcount;
    D3D10_SUBRESOURCE_DATA data = {0};
    ID3D10Device *device, *tmp;
    D3D10_TEXTURE3D_DESC desc;
    ID3D10Texture3D *texture;
    IDXGISurface *surface;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        DXGI_FORMAT format;
        D3D10_BIND_FLAG bind_flags;
        BOOL succeeds;
        BOOL todo;
    }
    tests[] =
    {
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D10_BIND_VERTEX_BUFFER,   FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D10_BIND_INDEX_BUFFER,    FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D10_BIND_CONSTANT_BUFFER, FALSE, TRUE},
        {DXGI_FORMAT_R32G32B32A32_TYPELESS, D3D10_BIND_SHADER_RESOURCE, TRUE,  FALSE},
        {DXGI_FORMAT_R16G16B16A16_TYPELESS, D3D10_BIND_SHADER_RESOURCE, TRUE,  FALSE},
        {DXGI_FORMAT_R10G10B10A2_TYPELESS,  D3D10_BIND_SHADER_RESOURCE, TRUE,  FALSE},
        {DXGI_FORMAT_R8G8B8A8_UNORM,        D3D10_BIND_DEPTH_STENCIL,   FALSE, FALSE},
        {DXGI_FORMAT_D24_UNORM_S8_UINT,     D3D10_BIND_RENDER_TARGET,   FALSE, FALSE},
        {DXGI_FORMAT_D32_FLOAT,             D3D10_BIND_RENDER_TARGET,   FALSE, FALSE},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    desc.Width = 64;
    desc.Height = 64;
    desc.Depth = 64;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture3D(device, &desc, &data, &texture);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture3D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture3D_Release(texture);

    desc.MipLevels = 0;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateTexture3D(device, &desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 3d texture, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Texture3D_GetDevice(texture, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10Texture3D_GetDesc(texture, &desc);
    ok(desc.Width == 64, "Got unexpected Width %u.\n", desc.Width);
    ok(desc.Height == 64, "Got unexpected Height %u.\n", desc.Height);
    ok(desc.Depth == 64, "Got unexpected Depth %u.\n", desc.Depth);
    ok(desc.MipLevels == 7, "Got unexpected MipLevels %u.\n", desc.MipLevels);
    ok(desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Got unexpected Format %#x.\n", desc.Format);
    ok(desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected Usage %u.\n", desc.Usage);
    ok(desc.BindFlags == D3D10_BIND_RENDER_TARGET, "Got unexpected BindFlags %u.\n", desc.BindFlags);
    ok(desc.CPUAccessFlags == 0, "Got unexpected CPUAccessFlags %u.\n", desc.CPUAccessFlags);
    ok(desc.MiscFlags == 0, "Got unexpected MiscFlags %u.\n", desc.MiscFlags);

    hr = ID3D10Texture3D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(FAILED(hr), "Texture should not implement IDXGISurface.\n");
    if (SUCCEEDED(hr)) IDXGISurface_Release(surface);
    ID3D10Texture3D_Release(texture);

    desc.MipLevels = 1;
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        desc.Format = tests[i].format;
        desc.BindFlags = tests[i].bind_flags;
        hr = ID3D10Device_CreateTexture3D(device, &desc, NULL, (ID3D10Texture3D **)&texture);

        todo_wine_if(tests[i].todo)
        ok(hr == (tests[i].succeeds ? S_OK : E_INVALIDARG), "Test %u: Got unexpected hr %#x.\n", i, hr);

        if (SUCCEEDED(hr))
            ID3D10Texture3D_Release(texture);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_buffer(void)
{
    ID3D11Buffer *d3d11_buffer;
    D3D10_BUFFER_DESC desc;
    ID3D10Buffer *buffer;
    ID3D10Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    static const struct test
    {
        UINT bind_flags;
        UINT misc_flags;
        UINT expected_bind_flags;
        UINT expected_misc_flags;
    }
    desc_conversion_tests[] =
    {
        {
            D3D10_BIND_VERTEX_BUFFER, 0,
            D3D11_BIND_VERTEX_BUFFER, 0
        },
        {
            D3D10_BIND_INDEX_BUFFER, 0,
            D3D11_BIND_INDEX_BUFFER, 0
        },
        {
            D3D10_BIND_CONSTANT_BUFFER, 0,
            D3D11_BIND_CONSTANT_BUFFER, 0
        },
        {
            D3D10_BIND_SHADER_RESOURCE, 0,
            D3D11_BIND_SHADER_RESOURCE, 0
        },
        {
            D3D10_BIND_STREAM_OUTPUT, 0,
            D3D11_BIND_STREAM_OUTPUT, 0
        },
        {
            D3D10_BIND_RENDER_TARGET, 0,
            D3D11_BIND_RENDER_TARGET, 0
        },
        {
            0, D3D10_RESOURCE_MISC_SHARED,
            0, D3D11_RESOURCE_MISC_SHARED
        },
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    buffer = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, 1024, NULL);
    hr = ID3D10Buffer_QueryInterface(buffer, &IID_ID3D11Buffer, (void **)&d3d11_buffer);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Buffer should implement ID3D11Buffer.\n");
    if (SUCCEEDED(hr)) ID3D11Buffer_Release(d3d11_buffer);
    ID3D10Buffer_Release(buffer);

    if (FAILED(hr))
    {
        win_skip("D3D11 is not available.\n");
        ID3D10Device_Release(device);
        return;
    }

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D11_BUFFER_DESC d3d11_desc;
        ID3D11Device *d3d11_device;

        desc.ByteWidth = 1024;
        desc.Usage = D3D10_USAGE_DEFAULT;
        desc.BindFlags = current->bind_flags;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = current->misc_flags;

        hr = ID3D10Device_CreateBuffer(device, &desc, NULL, &buffer);
        /* Shared resources are not supported by REF and WARP devices. */
        ok(SUCCEEDED(hr) || broken(hr == E_OUTOFMEMORY), "Test %u: Failed to create a buffer, hr %#x.\n", i, hr);
        if (FAILED(hr))
        {
            win_skip("Failed to create a buffer, skipping test %u.\n", i);
            continue;
        }

        hr = ID3D10Buffer_QueryInterface(buffer, &IID_ID3D11Buffer, (void **)&d3d11_buffer);
        ok(SUCCEEDED(hr), "Test %u: Buffer should implement ID3D11Buffer.\n", i);
        ID3D10Buffer_Release(buffer);

        ID3D11Buffer_GetDesc(d3d11_buffer, &d3d11_desc);

        ok(d3d11_desc.ByteWidth == desc.ByteWidth,
                "Test %u: Got unexpected ByteWidth %u.\n", i, d3d11_desc.ByteWidth);
        ok(d3d11_desc.Usage == (D3D11_USAGE)desc.Usage,
                "Test %u: Got unexpected Usage %u.\n", i, d3d11_desc.Usage);
        ok(d3d11_desc.BindFlags == current->expected_bind_flags,
                "Test %u: Got unexpected BindFlags %#x.\n", i, d3d11_desc.BindFlags);
        ok(d3d11_desc.CPUAccessFlags == desc.CPUAccessFlags,
                "Test %u: Got unexpected CPUAccessFlags %#x.\n", i, d3d11_desc.CPUAccessFlags);
        ok(d3d11_desc.MiscFlags == current->expected_misc_flags,
                "Test %u: Got unexpected MiscFlags %#x.\n", i, d3d11_desc.MiscFlags);
        ok(d3d11_desc.StructureByteStride == 0,
                "Test %u: Got unexpected StructureByteStride %u.\n", i, d3d11_desc.StructureByteStride);

        d3d11_device = NULL;
        ID3D11Buffer_GetDevice(d3d11_buffer, &d3d11_device);
        ok(!!d3d11_device, "Test %u: Got NULL, expected device pointer.\n", i);
        ID3D11Device_Release(d3d11_device);

        ID3D11Buffer_Release(d3d11_buffer);
    }

    desc.ByteWidth = 1024;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    hr = ID3D10Device_CreateBuffer(device, &desc, NULL, &buffer);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        ID3D10Buffer_Release(buffer);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_depthstencil_view(void)
{
    D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D10DepthStencilView *dsview;
    ID3D10Device *device, *tmp;
    ID3D10Texture2D *texture;
    IUnknown *iface;
    unsigned int i;
    HRESULT hr;

#define FMT_UNKNOWN  DXGI_FORMAT_UNKNOWN
#define D24S8        DXGI_FORMAT_D24_UNORM_S8_UINT
#define R24G8_TL     DXGI_FORMAT_R24G8_TYPELESS
#define DIM_UNKNOWN  D3D10_DSV_DIMENSION_UNKNOWN
#define TEX_1D       D3D10_DSV_DIMENSION_TEXTURE1D
#define TEX_1D_ARRAY D3D10_DSV_DIMENSION_TEXTURE1DARRAY
#define TEX_2D       D3D10_DSV_DIMENSION_TEXTURE2D
#define TEX_2D_ARRAY D3D10_DSV_DIMENSION_TEXTURE2DARRAY
#define TEX_2DMS     D3D10_DSV_DIMENSION_TEXTURE2DMS
#define TEX_2DMS_ARR D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY
    static const struct
    {
        struct
        {
            unsigned int miplevel_count;
            unsigned int array_size;
            DXGI_FORMAT format;
        } texture;
        struct dsv_desc dsv_desc;
        struct dsv_desc expected_dsv_desc;
    }
    tests[] =
    {
        {{ 1, 1, D24S8},    {0},                                    {D24S8, TEX_2D,       0}},
        {{10, 1, D24S8},    {0},                                    {D24S8, TEX_2D,       0}},
        {{10, 1, D24S8},    {FMT_UNKNOWN, TEX_2D,       0},         {D24S8, TEX_2D,       0}},
        {{10, 1, D24S8},    {FMT_UNKNOWN, TEX_2D,       1},         {D24S8, TEX_2D,       1}},
        {{10, 1, D24S8},    {FMT_UNKNOWN, TEX_2D,       9},         {D24S8, TEX_2D,       9}},
        {{ 1, 1, R24G8_TL}, {D24S8,       TEX_2D,       0},         {D24S8, TEX_2D,       0}},
        {{10, 1, R24G8_TL}, {D24S8,       TEX_2D,       0},         {D24S8, TEX_2D,       0}},
        {{ 1, 4, D24S8},    {0},                                    {D24S8, TEX_2D_ARRAY, 0, 0, 4}},
        {{10, 4, D24S8},    {0},                                    {D24S8, TEX_2D_ARRAY, 0, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 0, ~0u}, {D24S8, TEX_2D_ARRAY, 0, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 1, 0, ~0u}, {D24S8, TEX_2D_ARRAY, 1, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 3, 0, ~0u}, {D24S8, TEX_2D_ARRAY, 3, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 5, 0, ~0u}, {D24S8, TEX_2D_ARRAY, 5, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 9, 0, ~0u}, {D24S8, TEX_2D_ARRAY, 9, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 1, ~0u}, {D24S8, TEX_2D_ARRAY, 0, 1, 3}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 2, ~0u}, {D24S8, TEX_2D_ARRAY, 0, 2, 2}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 3, ~0u}, {D24S8, TEX_2D_ARRAY, 0, 3, 1}},
        {{ 1, 1, D24S8},    {FMT_UNKNOWN, TEX_2DMS},                {D24S8, TEX_2DMS}},
        {{ 1, 4, D24S8},    {FMT_UNKNOWN, TEX_2DMS},                {D24S8, TEX_2DMS}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2DMS},                {D24S8, TEX_2DMS}},
        {{ 1, 1, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  1},  {D24S8, TEX_2DMS_ARR, 0, 0, 1}},
        {{ 1, 1, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0, ~0u}, {D24S8, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 1, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  1},  {D24S8, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 1, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0, ~0u}, {D24S8, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  1},  {D24S8, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  4},  {D24S8, TEX_2DMS_ARR, 0, 0, 4}},
        {{10, 4, D24S8},    {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0, ~0u}, {D24S8, TEX_2DMS_ARR, 0, 0, 4}},
    };
    static const struct
    {
        struct
        {
            unsigned int miplevel_count;
            unsigned int array_size;
            DXGI_FORMAT format;
        } texture;
        struct dsv_desc dsv_desc;
    }
    invalid_desc_tests[] =
    {
        {{1, 1, D24S8},    {D24S8,       DIM_UNKNOWN}},
        {{6, 4, D24S8},    {D24S8,       DIM_UNKNOWN}},
        {{1, 1, D24S8},    {D24S8,       TEX_1D,       0}},
        {{1, 1, D24S8},    {D24S8,       TEX_1D_ARRAY, 0, 0, 1}},
        {{1, 1, D24S8},    {R24G8_TL,    TEX_2D,       0}},
        {{1, 1, R24G8_TL}, {FMT_UNKNOWN, TEX_2D,       0}},
        {{1, 1, D24S8},    {D24S8,       TEX_2D,       1}},
        {{1, 1, D24S8},    {D24S8,       TEX_2D_ARRAY, 0, 0, 0}},
        {{1, 1, D24S8},    {D24S8,       TEX_2D_ARRAY, 1, 0, 1}},
        {{1, 1, D24S8},    {D24S8,       TEX_2D_ARRAY, 0, 0, 2}},
        {{1, 1, D24S8},    {D24S8,       TEX_2D_ARRAY, 0, 1, 1}},
        {{1, 1, D24S8},    {D24S8,       TEX_2DMS_ARR, 0, 0, 2}},
        {{1, 1, D24S8},    {D24S8,       TEX_2DMS_ARR, 0, 1, 1}},
    };
#undef FMT_UNKNOWN
#undef D24S8
#undef R24S8_TL
#undef DIM_UNKNOWN
#undef TEX_1D
#undef TEX_1D_ARRAY
#undef TEX_2D
#undef TEX_2D_ARRAY
#undef TEX_2DMS
#undef TEX_2DMS_ARR

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, NULL, &dsview);
    ok(SUCCEEDED(hr), "Failed to create a depthstencil view, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10DepthStencilView_GetDevice(dsview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    memset(&dsv_desc, 0, sizeof(dsv_desc));
    ID3D10DepthStencilView_GetDesc(dsview, &dsv_desc);
    ok(dsv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", dsv_desc.Format);
    ok(dsv_desc.ViewDimension == D3D10_DSV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", dsv_desc.ViewDimension);
    ok(!U(dsv_desc).Texture2D.MipSlice, "Got unexpected mip slice %u.\n", U(dsv_desc).Texture2D.MipSlice);

    ID3D10DepthStencilView_Release(dsview);
    ID3D10Texture2D_Release(texture);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        D3D10_DEPTH_STENCIL_VIEW_DESC *current_desc;

        texture_desc.MipLevels = tests[i].texture.miplevel_count;
        texture_desc.ArraySize = tests[i].texture.array_size;
        texture_desc.Format = tests[i].texture.format;

        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
        ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);

        if (tests[i].dsv_desc.dimension == D3D10_DSV_DIMENSION_UNKNOWN)
        {
            current_desc = NULL;
        }
        else
        {
            current_desc = &dsv_desc;
            get_dsv_desc(current_desc, &tests[i].dsv_desc);
        }

        expected_refcount = get_refcount((IUnknown *)texture);
        hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, current_desc, &dsview);
        ok(SUCCEEDED(hr), "Test %u: Failed to create depth stencil view, hr %#x.\n", i, hr);
        refcount = get_refcount((IUnknown *)texture);
        ok(refcount == expected_refcount, "Got refcount %u, expected %u.\n", refcount, expected_refcount);

        hr = ID3D10DepthStencilView_QueryInterface(dsview, &IID_ID3D11DepthStencilView, (void **)&iface);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Test %u: Depth stencil view should implement ID3D11DepthStencilView.\n", i);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);

        memset(&dsv_desc, 0, sizeof(dsv_desc));
        ID3D10DepthStencilView_GetDesc(dsview, &dsv_desc);
        check_dsv_desc(&dsv_desc, &tests[i].expected_dsv_desc);

        ID3D10DepthStencilView_Release(dsview);
        ID3D10Texture2D_Release(texture);
    }

    for (i = 0; i < sizeof(invalid_desc_tests) / sizeof(*invalid_desc_tests); ++i)
    {
        texture_desc.MipLevels = invalid_desc_tests[i].texture.miplevel_count;
        texture_desc.ArraySize = invalid_desc_tests[i].texture.array_size;
        texture_desc.Format = invalid_desc_tests[i].texture.format;

        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
        ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);

        get_dsv_desc(&dsv_desc, &invalid_desc_tests[i].dsv_desc);
        hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, &dsv_desc, &dsview);
        ok(hr == E_INVALIDARG, "Test %u: Got unexpected hr %#x.\n", i, hr);

        ID3D10Texture2D_Release(texture);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_depthstencil_view_interfaces(void)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
    D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    ID3D11DepthStencilView *d3d11_dsview;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10DepthStencilView *dsview;
    ID3D10Texture2D *texture;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    dsv_desc.Format = texture_desc.Format;
    dsv_desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    U(dsv_desc).Texture2D.MipSlice = 0;

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, &dsv_desc, &dsview);
    ok(SUCCEEDED(hr), "Failed to create a depthstencil view, hr %#x.\n", hr);

    hr = ID3D10DepthStencilView_QueryInterface(dsview, &IID_ID3D11DepthStencilView, (void **)&d3d11_dsview);
    ID3D10DepthStencilView_Release(dsview);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Depth stencil view should implement ID3D11DepthStencilView.\n");

    if (SUCCEEDED(hr))
    {
        ID3D11DepthStencilView_GetDesc(d3d11_dsview, &d3d11_dsv_desc);
        ok(d3d11_dsv_desc.Format == dsv_desc.Format, "Got unexpected format %#x.\n", d3d11_dsv_desc.Format);
        ok(d3d11_dsv_desc.ViewDimension == (D3D11_DSV_DIMENSION)dsv_desc.ViewDimension,
                "Got unexpected view dimension %u.\n", d3d11_dsv_desc.ViewDimension);
        ok(!d3d11_dsv_desc.Flags, "Got unexpected flags %#x.\n", d3d11_dsv_desc.Flags);
        ok(U(d3d11_dsv_desc).Texture2D.MipSlice == U(dsv_desc).Texture2D.MipSlice,
                "Got unexpected mip slice %u.\n", U(d3d11_dsv_desc).Texture2D.MipSlice);

        ID3D11DepthStencilView_Release(d3d11_dsview);
    }
    else
    {
        win_skip("D3D11 is not available.\n");
    }

    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_rendertarget_view(void)
{
    D3D10_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D10_TEXTURE3D_DESC texture3d_desc;
    D3D10_TEXTURE2D_DESC texture2d_desc;
    D3D10_SUBRESOURCE_DATA data = {0};
    ULONG refcount, expected_refcount;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10RenderTargetView *rtview;
    ID3D10Device *device, *tmp;
    ID3D10Texture3D *texture3d;
    ID3D10Texture2D *texture2d;
    ID3D10Resource *texture;
    ID3D10Buffer *buffer;
    IUnknown *iface;
    unsigned int i;
    HRESULT hr;

#define FMT_UNKNOWN  DXGI_FORMAT_UNKNOWN
#define RGBA8_UNORM  DXGI_FORMAT_R8G8B8A8_UNORM
#define RGBA8_TL     DXGI_FORMAT_R8G8B8A8_TYPELESS
#define DIM_UNKNOWN  D3D10_RTV_DIMENSION_UNKNOWN
#define TEX_1D       D3D10_RTV_DIMENSION_TEXTURE1D
#define TEX_1D_ARRAY D3D10_RTV_DIMENSION_TEXTURE1DARRAY
#define TEX_2D       D3D10_RTV_DIMENSION_TEXTURE2D
#define TEX_2D_ARRAY D3D10_RTV_DIMENSION_TEXTURE2DARRAY
#define TEX_2DMS     D3D10_RTV_DIMENSION_TEXTURE2DMS
#define TEX_2DMS_ARR D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY
#define TEX_3D       D3D10_RTV_DIMENSION_TEXTURE3D
    static const struct
    {
        struct
        {
            unsigned int miplevel_count;
            unsigned int depth_or_array_size;
            DXGI_FORMAT format;
        } texture;
        struct rtv_desc rtv_desc;
        struct rtv_desc expected_rtv_desc;
    }
    tests[] =
    {
        {{ 1, 1, RGBA8_UNORM}, {0},                                    {RGBA8_UNORM, TEX_2D,       0}},
        {{10, 1, RGBA8_UNORM}, {0},                                    {RGBA8_UNORM, TEX_2D,       0}},
        {{10, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D,       0},         {RGBA8_UNORM, TEX_2D,       0}},
        {{10, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D,       1},         {RGBA8_UNORM, TEX_2D,       1}},
        {{10, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D,       9},         {RGBA8_UNORM, TEX_2D,       9}},
        {{ 1, 1, RGBA8_TL},    {RGBA8_UNORM, TEX_2D,       0},         {RGBA8_UNORM, TEX_2D,       0}},
        {{10, 1, RGBA8_TL},    {RGBA8_UNORM, TEX_2D,       0},         {RGBA8_UNORM, TEX_2D,       0}},
        {{ 1, 4, RGBA8_UNORM}, {0},                                    {RGBA8_UNORM, TEX_2D_ARRAY, 0, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {0},                                    {RGBA8_UNORM, TEX_2D_ARRAY, 0, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 1, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 1, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 3, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 3, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 5, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 5, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 9, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 9, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 1, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 1, 3}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 2, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 2, 2}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, 3, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 3, 1}},
        {{ 1, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS},                {RGBA8_UNORM, TEX_2DMS}},
        {{ 1, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS},                {RGBA8_UNORM, TEX_2DMS}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS},                {RGBA8_UNORM, TEX_2DMS}},
        {{ 1, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  1},  {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 1}},
        {{ 1, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0, ~0u}, {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  1},  {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0, ~0u}, {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  1},  {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 1}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0,  4},  {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 4}},
        {{10, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0, 0, ~0u}, {RGBA8_UNORM, TEX_2DMS_ARR, 0, 0, 4}},
        {{ 1, 6, RGBA8_UNORM}, {0},                                    {RGBA8_UNORM, TEX_3D,       0, 0, 6}},
        {{ 2, 6, RGBA8_UNORM}, {0},                                    {RGBA8_UNORM, TEX_3D,       0, 0, 6}},
        {{ 2, 6, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       0, 0, 6}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       1, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       1, 0, 2}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       1, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       1, 0, 2}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, 1, ~0u}, {RGBA8_UNORM, TEX_3D,       0, 1, 3}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, 2, ~0u}, {RGBA8_UNORM, TEX_3D,       0, 2, 2}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, 3, ~0u}, {RGBA8_UNORM, TEX_3D,       0, 3, 1}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, 1,  1},  {RGBA8_UNORM, TEX_3D,       0, 1, 1}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       1, 1,  1},  {RGBA8_UNORM, TEX_3D,       1, 1, 1}},
        {{ 2, 4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       1, 1, ~0u}, {RGBA8_UNORM, TEX_3D,       1, 1, 1}},
        {{ 6, 8, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       0, 0, 8}},
        {{ 6, 8, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       1, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       1, 0, 4}},
        {{ 6, 8, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       2, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       2, 0, 2}},
        {{ 6, 8, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       3, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       3, 0, 1}},
        {{ 6, 8, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       4, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       4, 0, 1}},
        {{ 6, 8, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       5, 0, ~0u}, {RGBA8_UNORM, TEX_3D,       5, 0, 1}},
    };
    static const struct
    {
        struct
        {
            D3D10_RTV_DIMENSION dimension;
            unsigned int miplevel_count;
            unsigned int depth_or_array_size;
            DXGI_FORMAT format;
        } texture;
        struct rtv_desc rtv_desc;
    }
    invalid_desc_tests[] =
    {
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, DIM_UNKNOWN}},
        {{TEX_2D, 6, 4, RGBA8_UNORM}, {RGBA8_UNORM, DIM_UNKNOWN}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D,        0}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D_ARRAY,  0, 0,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0, 0,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0, 0, ~0u}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_TL,    TEX_2D,        0}},
        {{TEX_2D, 1, 1, RGBA8_TL},    {FMT_UNKNOWN, TEX_2D,        0}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0, 0,  0}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  1, 0,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0, 0,  2}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0, 1,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2DMS_ARR,  0, 0,  2}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2DMS_ARR,  0, 1,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D,        0}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D_ARRAY,  0, 0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        0}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0, 0,  1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D,        0}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D_ARRAY,  0, 0,  1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        0}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0, 0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0, 0,  0}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        1, 0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_TL,    TEX_3D,        0, 0,  1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_TL,    TEX_3D,        0, 0,  1}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0, 0,  9}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        3, 0,  2}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        2, 0,  4}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        1, 0,  8}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0, 8, ~0u}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        1, 4, ~0u}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        2, 2, ~0u}},
        {{TEX_3D, 4, 8, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        3, 1, ~0u}},
    };
#undef FMT_UNKNOWN
#undef RGBA8_UNORM
#undef RGBA8_TL
#undef DIM_UNKNOWN
#undef TEX_1D
#undef TEX_1D_ARRAY
#undef TEX_2D
#undef TEX_2D_ARRAY
#undef TEX_2DMS
#undef TEX_2DMS_ARR
#undef TEX_3D

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateBuffer(device, &buffer_desc, &data, &buffer);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(SUCCEEDED(hr), "Failed to create a buffer, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Buffer_GetDevice(buffer, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    rtv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    rtv_desc.ViewDimension = D3D10_RTV_DIMENSION_BUFFER;
    U(rtv_desc).Buffer.ElementOffset = 0;
    U(rtv_desc).Buffer.ElementWidth = 64;

    hr = ID3D10Device_CreateRenderTargetView(device, NULL, &rtv_desc, &rtview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)buffer, &rtv_desc, &rtview);
    ok(SUCCEEDED(hr), "Failed to create a rendertarget view, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10RenderTargetView_GetDevice(rtview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10RenderTargetView_QueryInterface(rtview, &IID_ID3D11RenderTargetView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Render target view should implement ID3D11RenderTargetView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    ID3D10RenderTargetView_Release(rtview);
    ID3D10Buffer_Release(buffer);

    texture2d_desc.Width = 512;
    texture2d_desc.Height = 512;
    texture2d_desc.SampleDesc.Count = 1;
    texture2d_desc.SampleDesc.Quality = 0;
    texture2d_desc.Usage = D3D10_USAGE_DEFAULT;
    texture2d_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture2d_desc.CPUAccessFlags = 0;
    texture2d_desc.MiscFlags = 0;

    texture3d_desc.Width = 64;
    texture3d_desc.Height = 64;
    texture3d_desc.Usage = D3D10_USAGE_DEFAULT;
    texture3d_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture3d_desc.CPUAccessFlags = 0;
    texture3d_desc.MiscFlags = 0;

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        D3D10_RENDER_TARGET_VIEW_DESC *current_desc;

        if (tests[i].expected_rtv_desc.dimension != D3D10_RTV_DIMENSION_TEXTURE3D)
        {
            texture2d_desc.MipLevels = tests[i].texture.miplevel_count;
            texture2d_desc.ArraySize = tests[i].texture.depth_or_array_size;
            texture2d_desc.Format = tests[i].texture.format;

            hr = ID3D10Device_CreateTexture2D(device, &texture2d_desc, NULL, &texture2d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture2d;
        }
        else
        {
            texture3d_desc.MipLevels = tests[i].texture.miplevel_count;
            texture3d_desc.Depth = tests[i].texture.depth_or_array_size;
            texture3d_desc.Format = tests[i].texture.format;

            hr = ID3D10Device_CreateTexture3D(device, &texture3d_desc, NULL, &texture3d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 3d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture3d;
        }

        if (tests[i].rtv_desc.dimension == D3D10_RTV_DIMENSION_UNKNOWN)
        {
            current_desc = NULL;
        }
        else
        {
            current_desc = &rtv_desc;
            get_rtv_desc(current_desc, &tests[i].rtv_desc);
        }

        expected_refcount = get_refcount((IUnknown *)texture);
        hr = ID3D10Device_CreateRenderTargetView(device, texture, current_desc, &rtview);
        ok(SUCCEEDED(hr), "Test %u: Failed to create render target view, hr %#x.\n", i, hr);
        refcount = get_refcount((IUnknown *)texture);
        ok(refcount == expected_refcount, "Got refcount %u, expected %u.\n", refcount, expected_refcount);

        hr = ID3D10RenderTargetView_QueryInterface(rtview, &IID_ID3D11RenderTargetView, (void **)&iface);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Test %u: Render target view should implement ID3D11RenderTargetView.\n", i);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);

        memset(&rtv_desc, 0, sizeof(rtv_desc));
        ID3D10RenderTargetView_GetDesc(rtview, &rtv_desc);
        check_rtv_desc(&rtv_desc, &tests[i].expected_rtv_desc);

        ID3D10RenderTargetView_Release(rtview);
        ID3D10Resource_Release(texture);
    }

    for (i = 0; i < sizeof(invalid_desc_tests) / sizeof(*invalid_desc_tests); ++i)
    {
        assert(invalid_desc_tests[i].texture.dimension == D3D10_RTV_DIMENSION_TEXTURE2D
                || invalid_desc_tests[i].texture.dimension == D3D10_RTV_DIMENSION_TEXTURE3D);

        if (invalid_desc_tests[i].texture.dimension != D3D10_RTV_DIMENSION_TEXTURE3D)
        {
            texture2d_desc.MipLevels = invalid_desc_tests[i].texture.miplevel_count;
            texture2d_desc.ArraySize = invalid_desc_tests[i].texture.depth_or_array_size;
            texture2d_desc.Format = invalid_desc_tests[i].texture.format;

            hr = ID3D10Device_CreateTexture2D(device, &texture2d_desc, NULL, &texture2d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture2d;
        }
        else
        {
            texture3d_desc.MipLevels = invalid_desc_tests[i].texture.miplevel_count;
            texture3d_desc.Depth = invalid_desc_tests[i].texture.depth_or_array_size;
            texture3d_desc.Format = invalid_desc_tests[i].texture.format;

            hr = ID3D10Device_CreateTexture3D(device, &texture3d_desc, NULL, &texture3d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 3d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture3d;
        }

        get_rtv_desc(&rtv_desc, &invalid_desc_tests[i].rtv_desc);
        hr = ID3D10Device_CreateRenderTargetView(device, texture, &rtv_desc, &rtview);
        ok(hr == E_INVALIDARG, "Test %u: Got unexpected hr %#x.\n", i, hr);

        ID3D10Resource_Release(texture);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_render_target_views(void)
{
    struct texture
    {
        UINT miplevel_count;
        UINT array_size;
    };
    struct rtv
    {
        DXGI_FORMAT format;
        D3D10_RTV_DIMENSION dimension;
        unsigned int miplevel_idx;
        unsigned int layer_idx;
        unsigned int layer_count;
    };

    static const struct vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
    static struct test
    {
        struct texture texture;
        struct rtv_desc rtv;
        DWORD expected_colors[4];
    }
    tests[] =
    {
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2D, 0},
                {0xff0000ff, 0x00000000}},
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2D, 1},
                {0x00000000, 0xff0000ff}},
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 1},
                {0xff0000ff, 0x00000000}},
        {{2, 1}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 1, 0, 1},
                {0x00000000, 0xff0000ff}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2D, 0},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 1},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 1, 1},
                {0x00000000, 0xff0000ff, 0x00000000, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 2, 1},
                {0x00000000, 0x00000000, 0xff0000ff, 0x00000000}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 3, 1},
                {0x00000000, 0x00000000, 0x00000000, 0xff0000ff}},
        {{1, 4}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 4},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2D, 0},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 0, 1},
                {0xff0000ff, 0x00000000, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 0, 1, 1},
                {0x00000000, 0x00000000, 0xff0000ff, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 1, 0, 1},
                {0x00000000, 0xff0000ff, 0x00000000, 0x00000000}},
        {{2, 2}, {DXGI_FORMAT_UNKNOWN, D3D10_RTV_DIMENSION_TEXTURE2DARRAY, 1, 1, 1},
                {0x00000000, 0x00000000, 0x00000000, 0xff0000ff}},
    };

    struct d3d10core_test_context test_context;
    D3D10_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10RenderTargetView *rtv;
    ID3D10Texture2D *texture;
    ID3D10Device *device;
    unsigned int i, j, k;
    void *data;
    HRESULT hr;

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    texture_desc.Width = 32;
    texture_desc.Height = 32;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, texture_desc.Width * texture_desc.Height * 4);
    ok(!!data, "Failed to allocate memory.\n");

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        const struct test *test = &tests[i];
        unsigned int sub_resource_count;

        texture_desc.MipLevels = test->texture.miplevel_count;
        texture_desc.ArraySize = test->texture.array_size;

        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
        ok(SUCCEEDED(hr), "Test %u: Failed to create texture, hr %#x.\n", i, hr);

        get_rtv_desc(&rtv_desc, &test->rtv);
        hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, &rtv_desc, &rtv);
        ok(SUCCEEDED(hr), "Test %u: Failed to create render target view, hr %#x.\n", i, hr);

        for (j = 0; j < texture_desc.ArraySize; ++j)
        {
            for (k = 0; k < texture_desc.MipLevels; ++k)
            {
                unsigned int sub_resource_idx = j * texture_desc.MipLevels + k;
                ID3D10Device_UpdateSubresource(device,
                        (ID3D10Resource *)texture, sub_resource_idx, NULL, data, texture_desc.Width * 4, 0);
            }
        }
        check_texture_color(texture, 0, 0);

        ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);
        draw_color_quad(&test_context, &red);

        sub_resource_count = texture_desc.MipLevels * texture_desc.ArraySize;
        assert(sub_resource_count <= sizeof(test->expected_colors) / sizeof(*test->expected_colors));
        for (j = 0; j < sub_resource_count; ++j)
            check_texture_sub_resource_color(texture, j, test->expected_colors[j], 1);

        ID3D10RenderTargetView_Release(rtv);
        ID3D10Texture2D_Release(texture);
    }

    HeapFree(GetProcessHeap(), 0, data);
    release_test_context(&test_context);
}

static void test_create_shader_resource_view(void)
{
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D10_TEXTURE3D_DESC texture3d_desc;
    D3D10_TEXTURE2D_DESC texture2d_desc;
    ULONG refcount, expected_refcount;
    ID3D10ShaderResourceView *srview;
    ID3D10Device *device, *tmp;
    ID3D10Texture3D *texture3d;
    ID3D10Texture2D *texture2d;
    ID3D10Resource *texture;
    ID3D10Buffer *buffer;
    IUnknown *iface;
    unsigned int i;
    HRESULT hr;

#define FMT_UNKNOWN  DXGI_FORMAT_UNKNOWN
#define RGBA8_UNORM  DXGI_FORMAT_R8G8B8A8_UNORM
#define RGBA8_TL     DXGI_FORMAT_R8G8B8A8_TYPELESS
#define DIM_UNKNOWN  D3D10_SRV_DIMENSION_UNKNOWN
#define TEX_1D       D3D10_SRV_DIMENSION_TEXTURE1D
#define TEX_1D_ARRAY D3D10_SRV_DIMENSION_TEXTURE1DARRAY
#define TEX_2D       D3D10_SRV_DIMENSION_TEXTURE2D
#define TEX_2D_ARRAY D3D10_SRV_DIMENSION_TEXTURE2DARRAY
#define TEX_2DMS     D3D10_SRV_DIMENSION_TEXTURE2DMS
#define TEX_2DMS_ARR D3D10_SRV_DIMENSION_TEXTURE2DMSARRAY
#define TEX_3D       D3D10_SRV_DIMENSION_TEXTURE3D
#define TEX_CUBE     D3D10_SRV_DIMENSION_TEXTURECUBE
    static const struct
    {
        struct
        {
            unsigned int miplevel_count;
            unsigned int depth_or_array_size;
            DXGI_FORMAT format;
        } texture;
        struct srv_desc srv_desc;
        struct srv_desc expected_srv_desc;
    }
    tests[] =
    {
        {{10,  1, RGBA8_UNORM}, {0},                                         {RGBA8_UNORM, TEX_2D,       0, 10}},
        {{10,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D,       0, ~0u},         {RGBA8_UNORM, TEX_2D,       0, 10}},
        {{10,  1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,       0, ~0u},         {RGBA8_UNORM, TEX_2D,       0, 10}},
        {{10,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D,       0, 10},          {RGBA8_UNORM, TEX_2D,       0, 10}},
        {{ 1,  1, RGBA8_TL},    {RGBA8_UNORM, TEX_2D,       0, ~0u},         {RGBA8_UNORM, TEX_2D,       0,  1}},
        {{10,  1, RGBA8_TL},    {RGBA8_UNORM, TEX_2D,       0, ~0u},         {RGBA8_UNORM, TEX_2D,       0, 10}},
        {{10,  4, RGBA8_UNORM}, {0},                                         {RGBA8_UNORM, TEX_2D_ARRAY, 0, 10, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, ~0u, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 10, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 1, ~0u, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 1,  9, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 3, ~0u, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 3,  7, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 5, ~0u, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 5,  5, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 9, ~0u, 0, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 9,  1, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, ~0u, 1, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 10, 1, 3}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, ~0u, 2, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 10, 2, 2}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2D_ARRAY, 0, ~0u, 3, ~0u}, {RGBA8_UNORM, TEX_2D_ARRAY, 0, 10, 3, 1}},
        {{ 1,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS},                     {RGBA8_UNORM, TEX_2DMS}},
        {{ 1,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS},                     {RGBA8_UNORM, TEX_2DMS}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS},                     {RGBA8_UNORM, TEX_2DMS}},
        {{ 1,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0,  1},  {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 1}},
        {{ 1,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0, ~0u}, {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 1}},
        {{10,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0,  1},  {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 1}},
        {{10,  1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0, ~0u}, {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 1}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0,  1},  {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 1}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0,  4},  {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 4}},
        {{10,  4, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_2DMS_ARR, 0,  1,  0, ~0u}, {RGBA8_UNORM, TEX_2DMS_ARR, 0,  1, 0, 4}},
        {{ 1, 12, RGBA8_UNORM}, {0},                                         {RGBA8_UNORM, TEX_3D,       0,  1}},
        {{ 1, 12, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0,  1},          {RGBA8_UNORM, TEX_3D,       0,  1}},
        {{ 1, 12, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, ~0u},         {RGBA8_UNORM, TEX_3D,       0,  1}},
        {{ 4, 12, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,       0, ~0u},         {RGBA8_UNORM, TEX_3D,       0,  4}},
        {{ 2,  6, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_CUBE,     0, ~0u},         {RGBA8_UNORM, TEX_CUBE,     0,  2}},
        {{ 2,  6, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_CUBE,     0,  1},          {RGBA8_UNORM, TEX_CUBE ,    0,  1}},
        {{ 2,  6, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_CUBE,     1,  1},          {RGBA8_UNORM, TEX_CUBE ,    1,  1}},
    };
    static const struct
    {
        struct
        {
            D3D10_SRV_DIMENSION dimension;
            unsigned int miplevel_count;
            unsigned int depth_or_array_size;
            DXGI_FORMAT format;
        } texture;
        struct srv_desc srv_desc;
    }
    invalid_desc_tests[] =
    {
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, DIM_UNKNOWN}},
        {{TEX_2D, 6, 4, RGBA8_UNORM}, {RGBA8_UNORM, DIM_UNKNOWN}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D,        0,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D_ARRAY,  0,  1, 0, 1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_TL,    TEX_2D,        0, ~0u}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_TL,    TEX_2D,        0,  1}},
        {{TEX_2D, 1, 1, RGBA8_TL},    {FMT_UNKNOWN, TEX_2D,        0, ~0u}},
        {{TEX_2D, 1, 1, RGBA8_TL},    {FMT_UNKNOWN, TEX_2D,        0,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        0,  0}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        0,  2}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        1,  1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  0, 0, 0}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  0, 0, 1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  1, 0, 0}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  2, 0, 1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  1,  1, 0, 1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  1, 0, 2}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  1, 1, 1}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2DMS_ARR,  0,  1, 0, 2}},
        {{TEX_2D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2DMS_ARR,  0,  1, 1, 1}},
        {{TEX_2D, 1, 6, RGBA8_UNORM}, {RGBA8_UNORM, TEX_CUBE,      0,  0}},
        {{TEX_2D, 1, 6, RGBA8_UNORM}, {RGBA8_UNORM, TEX_CUBE,      0,  2}},
        {{TEX_2D, 1, 6, RGBA8_UNORM}, {RGBA8_UNORM, TEX_CUBE,      1,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D,        0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D_ARRAY,  0,  1, 0, 1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_CUBE,      0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  1, 0, 1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D,        0,  1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_1D_ARRAY,  0,  1, 0, 1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D,        0,  1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_CUBE,      0,  1}},
        {{TEX_3D, 1, 9, RGBA8_UNORM}, {RGBA8_UNORM, TEX_2D_ARRAY,  0,  1, 0, 1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0,  0}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_TL,    TEX_3D,        0,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,        0,  2}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {FMT_UNKNOWN, TEX_3D,        1,  1}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        0,  2}},
        {{TEX_3D, 1, 1, RGBA8_UNORM}, {RGBA8_UNORM, TEX_3D,        1,  1}},
    };
#undef FMT_UNKNOWN
#undef RGBA8_UNORM
#undef DIM_UNKNOWN
#undef TEX_1D
#undef TEX_1D_ARRAY
#undef TEX_2D
#undef TEX_2D_ARRAY
#undef TEX_2DMS
#undef TEX_2DMS_ARR
#undef TEX_3D
#undef TEX_CUBE

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    buffer = create_buffer(device, D3D10_BIND_SHADER_RESOURCE, 1024, NULL);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)buffer, NULL, &srview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
    U(srv_desc).Buffer.ElementOffset = 0;
    U(srv_desc).Buffer.ElementWidth = 64;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)buffer, &srv_desc, &srview);
    ok(SUCCEEDED(hr), "Failed to create a shader resource view, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10ShaderResourceView_GetDevice(srview, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10ShaderResourceView_QueryInterface(srview, &IID_ID3D10ShaderResourceView1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Shader resource view should implement ID3D10ShaderResourceView1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);
    hr = ID3D10ShaderResourceView_QueryInterface(srview, &IID_ID3D11ShaderResourceView, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Shader resource view should implement ID3D11ShaderResourceView.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    ID3D10ShaderResourceView_Release(srview);
    ID3D10Buffer_Release(buffer);

    texture2d_desc.Width = 512;
    texture2d_desc.Height = 512;
    texture2d_desc.SampleDesc.Count = 1;
    texture2d_desc.SampleDesc.Quality = 0;
    texture2d_desc.Usage = D3D10_USAGE_DEFAULT;
    texture2d_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture2d_desc.CPUAccessFlags = 0;

    texture3d_desc.Width = 64;
    texture3d_desc.Height = 64;
    texture3d_desc.Usage = D3D10_USAGE_DEFAULT;
    texture3d_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture3d_desc.CPUAccessFlags = 0;
    texture3d_desc.MiscFlags = 0;

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        D3D10_SHADER_RESOURCE_VIEW_DESC *current_desc;

        if (tests[i].expected_srv_desc.dimension != D3D10_SRV_DIMENSION_TEXTURE3D)
        {
            texture2d_desc.MipLevels = tests[i].texture.miplevel_count;
            texture2d_desc.ArraySize = tests[i].texture.depth_or_array_size;
            texture2d_desc.Format = tests[i].texture.format;
            texture2d_desc.MiscFlags = 0;

            if (tests[i].srv_desc.dimension == D3D10_SRV_DIMENSION_TEXTURECUBE)
                texture2d_desc.MiscFlags |= D3D10_RESOURCE_MISC_TEXTURECUBE;

            hr = ID3D10Device_CreateTexture2D(device, &texture2d_desc, NULL, &texture2d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture2d;
        }
        else
        {
            texture3d_desc.MipLevels = tests[i].texture.miplevel_count;
            texture3d_desc.Depth = tests[i].texture.depth_or_array_size;
            texture3d_desc.Format = tests[i].texture.format;

            hr = ID3D10Device_CreateTexture3D(device, &texture3d_desc, NULL, &texture3d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 3d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture3d;
        }

        if (tests[i].srv_desc.dimension == D3D10_SRV_DIMENSION_UNKNOWN)
        {
            current_desc = NULL;
        }
        else
        {
            current_desc = &srv_desc;
            get_srv_desc(current_desc, &tests[i].srv_desc);
        }

        expected_refcount = get_refcount((IUnknown *)texture);
        hr = ID3D10Device_CreateShaderResourceView(device, texture, current_desc, &srview);
        ok(SUCCEEDED(hr), "Test %u: Failed to create a shader resource view, hr %#x.\n", i, hr);
        refcount = get_refcount((IUnknown *)texture);
        ok(refcount == expected_refcount, "Got refcount %u, expected %u.\n", refcount, expected_refcount);

        hr = ID3D10ShaderResourceView_QueryInterface(srview, &IID_ID3D10ShaderResourceView1, (void **)&iface);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Test %u: Shader resource view should implement ID3D10ShaderResourceView1.\n", i);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);
        hr = ID3D10ShaderResourceView_QueryInterface(srview, &IID_ID3D11ShaderResourceView, (void **)&iface);
        ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
                "Test %u: Shader resource view should implement ID3D11ShaderResourceView.\n", i);
        if (SUCCEEDED(hr)) IUnknown_Release(iface);

        memset(&srv_desc, 0, sizeof(srv_desc));
        ID3D10ShaderResourceView_GetDesc(srview, &srv_desc);
        check_srv_desc(&srv_desc, &tests[i].expected_srv_desc);

        ID3D10ShaderResourceView_Release(srview);
        ID3D10Resource_Release(texture);
    }

    for (i = 0; i < sizeof(invalid_desc_tests) / sizeof(*invalid_desc_tests); ++i)
    {
        assert(invalid_desc_tests[i].texture.dimension == D3D10_SRV_DIMENSION_TEXTURE2D
                || invalid_desc_tests[i].texture.dimension == D3D10_SRV_DIMENSION_TEXTURE3D);

        if (invalid_desc_tests[i].texture.dimension == D3D10_SRV_DIMENSION_TEXTURE2D)
        {
            texture2d_desc.MipLevels = invalid_desc_tests[i].texture.miplevel_count;
            texture2d_desc.ArraySize = invalid_desc_tests[i].texture.depth_or_array_size;
            texture2d_desc.Format = invalid_desc_tests[i].texture.format;
            texture2d_desc.MiscFlags = 0;

            if (invalid_desc_tests[i].srv_desc.dimension == D3D10_SRV_DIMENSION_TEXTURECUBE)
                texture2d_desc.MiscFlags |= D3D10_RESOURCE_MISC_TEXTURECUBE;

            hr = ID3D10Device_CreateTexture2D(device, &texture2d_desc, NULL, &texture2d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture2d;
        }
        else
        {
            texture3d_desc.MipLevels = invalid_desc_tests[i].texture.miplevel_count;
            texture3d_desc.Depth = invalid_desc_tests[i].texture.depth_or_array_size;
            texture3d_desc.Format = invalid_desc_tests[i].texture.format;

            hr = ID3D10Device_CreateTexture3D(device, &texture3d_desc, NULL, &texture3d);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 3d texture, hr %#x.\n", i, hr);
            texture = (ID3D10Resource *)texture3d;
        }

        get_srv_desc(&srv_desc, &invalid_desc_tests[i].srv_desc);
        hr = ID3D10Device_CreateShaderResourceView(device, texture, &srv_desc, &srview);
        ok(hr == E_INVALIDARG, "Test %u: Got unexpected hr %#x.\n", i, hr);

        ID3D10Resource_Release(texture);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_shader(void)
{
#if 0
float4 light;
float4x4 mat;

struct input
{
    float4 position : POSITION;
    float3 normal : NORMAL;
};

struct output
{
    float4 position : POSITION;
    float4 diffuse : COLOR;
};

output main(const input v)
{
    output o;

    o.position = mul(v.position, mat);
    o.diffuse = dot((float3)light, v.normal);

    return o;
}
#endif
    static const DWORD vs_4_0[] =
    {
        0x43425844, 0x3ae813ca, 0x0f034b91, 0x790f3226, 0x6b4a718a, 0x00000001, 0x000001c0,
        0x00000003, 0x0000002c, 0x0000007c, 0x000000cc, 0x4e475349, 0x00000048, 0x00000002,
        0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f,
        0x00000041, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x00000707, 0x49534f50,
        0x4e4f4954, 0x524f4e00, 0x004c414d, 0x4e47534f, 0x00000048, 0x00000002, 0x00000008,
        0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x00000041,
        0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x49534f50, 0x4e4f4954,
        0x4c4f4300, 0xab00524f, 0x52444853, 0x000000ec, 0x00010040, 0x0000003b, 0x04000059,
        0x00208e46, 0x00000000, 0x00000005, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f,
        0x00101072, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x03000065, 0x001020f2,
        0x00000001, 0x08000011, 0x00102012, 0x00000000, 0x00101e46, 0x00000000, 0x00208e46,
        0x00000000, 0x00000001, 0x08000011, 0x00102022, 0x00000000, 0x00101e46, 0x00000000,
        0x00208e46, 0x00000000, 0x00000002, 0x08000011, 0x00102042, 0x00000000, 0x00101e46,
        0x00000000, 0x00208e46, 0x00000000, 0x00000003, 0x08000011, 0x00102082, 0x00000000,
        0x00101e46, 0x00000000, 0x00208e46, 0x00000000, 0x00000004, 0x08000010, 0x001020f2,
        0x00000001, 0x00208246, 0x00000000, 0x00000000, 0x00101246, 0x00000001, 0x0100003e,
    };

    static const DWORD vs_2_0[] =
    {
        0xfffe0200, 0x002bfffe, 0x42415443, 0x0000001c, 0x00000077, 0xfffe0200, 0x00000002,
        0x0000001c, 0x00000100, 0x00000070, 0x00000044, 0x00040002, 0x00000001, 0x0000004c,
        0x00000000, 0x0000005c, 0x00000002, 0x00000004, 0x00000060, 0x00000000, 0x6867696c,
        0xabab0074, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x0074616d, 0x00030003,
        0x00040004, 0x00000001, 0x00000000, 0x325f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
        0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072,
        0x392e3932, 0x332e3235, 0x00313131, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f,
        0x80000003, 0x900f0001, 0x03000009, 0xc0010000, 0x90e40000, 0xa0e40000, 0x03000009,
        0xc0020000, 0x90e40000, 0xa0e40001, 0x03000009, 0xc0040000, 0x90e40000, 0xa0e40002,
        0x03000009, 0xc0080000, 0x90e40000, 0xa0e40003, 0x03000008, 0xd00f0000, 0xa0e40004,
        0x90e40001, 0x0000ffff,
    };

    static const DWORD vs_3_0[] =
    {
        0xfffe0300, 0x002bfffe, 0x42415443, 0x0000001c, 0x00000077, 0xfffe0300, 0x00000002,
        0x0000001c, 0x00000100, 0x00000070, 0x00000044, 0x00040002, 0x00000001, 0x0000004c,
        0x00000000, 0x0000005c, 0x00000002, 0x00000004, 0x00000060, 0x00000000, 0x6867696c,
        0xabab0074, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x0074616d, 0x00030003,
        0x00040004, 0x00000001, 0x00000000, 0x335f7376, 0x4d00305f, 0x6f726369, 0x74666f73,
        0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072,
        0x392e3932, 0x332e3235, 0x00313131, 0x0200001f, 0x80000000, 0x900f0000, 0x0200001f,
        0x80000003, 0x900f0001, 0x0200001f, 0x80000000, 0xe00f0000, 0x0200001f, 0x8000000a,
        0xe00f0001, 0x03000009, 0xe0010000, 0x90e40000, 0xa0e40000, 0x03000009, 0xe0020000,
        0x90e40000, 0xa0e40001, 0x03000009, 0xe0040000, 0x90e40000, 0xa0e40002, 0x03000009,
        0xe0080000, 0x90e40000, 0xa0e40003, 0x03000008, 0xe00f0001, 0xa0e40004, 0x90e40001,
        0x0000ffff,
    };

#if 0
float4 main(const float4 color : COLOR) : SV_TARGET
{
    float4 o;

    o = color;

    return o;
}
#endif
    static const DWORD ps_4_0[] =
    {
        0x43425844, 0x4da9446f, 0xfbe1f259, 0x3fdb3009, 0x517521fa, 0x00000001, 0x000001ac,
        0x00000005, 0x00000034, 0x0000008c, 0x000000bc, 0x000000f0, 0x00000130, 0x46454452,
        0x00000050, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100,
        0x0000001c, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168,
        0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00,
        0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000,
        0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453,
        0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000,
    };

#if 0
struct gs_out
{
    float4 pos : SV_POSITION;
};

[maxvertexcount(4)]
void main(point float4 vin[1] : POSITION, inout TriangleStream<gs_out> vout)
{
    float offset = 0.1 * vin[0].w;
    gs_out v;

    v.pos = float4(vin[0].x - offset, vin[0].y - offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x - offset, vin[0].y + offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x + offset, vin[0].y - offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x + offset, vin[0].y + offset, vin[0].z, vin[0].w);
    vout.Append(v);
}
#endif
    static const DWORD gs_4_0[] =
    {
        0x43425844, 0x000ee786, 0xc624c269, 0x885a5cbe, 0x444b3b1f, 0x00000001, 0x0000023c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x000001a0, 0x00020040,
        0x00000068, 0x0400005f, 0x002010f2, 0x00000001, 0x00000000, 0x02000068, 0x00000001, 0x0100085d,
        0x0100285c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x0200005e, 0x00000004, 0x0f000032,
        0x00100032, 0x00000000, 0x80201ff6, 0x00000041, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd,
        0x3dcccccd, 0x00000000, 0x00000000, 0x00201046, 0x00000000, 0x00000000, 0x05000036, 0x00102032,
        0x00000000, 0x00100046, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x0e000032,
        0x00100052, 0x00000000, 0x00201ff6, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd, 0x00000000,
        0x3dcccccd, 0x00000000, 0x00201106, 0x00000000, 0x00000000, 0x05000036, 0x00102022, 0x00000000,
        0x0010002a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000,
        0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x05000036, 0x00102022,
        0x00000000, 0x0010001a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102032, 0x00000000, 0x00100086, 0x00000000, 0x06000036,
        0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000, 0x01000013, 0x0100003e,
    };

    ULONG refcount, expected_refcount;
    ID3D10Device *device, *tmp;
    ID3D10GeometryShader *gs;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    IUnknown *iface;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    /* vertex shader */
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateVertexShader(device, vs_4_0, sizeof(vs_4_0), &vs);
    ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x\n", hr);

    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10VertexShader_GetDevice(vs, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10VertexShader_QueryInterface(vs, &IID_ID3D11VertexShader, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Vertex shader should implement ID3D11VertexShader.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    refcount = ID3D10VertexShader_Release(vs);
    ok(!refcount, "Vertex shader has %u references left.\n", refcount);

    hr = ID3D10Device_CreateVertexShader(device, vs_2_0, sizeof(vs_2_0), &vs);
    ok(hr == E_INVALIDARG, "Created a SM2 vertex shader, hr %#x\n", hr);

    hr = ID3D10Device_CreateVertexShader(device, vs_3_0, sizeof(vs_3_0), &vs);
    ok(hr == E_INVALIDARG, "Created a SM3 vertex shader, hr %#x\n", hr);

    hr = ID3D10Device_CreateVertexShader(device, ps_4_0, sizeof(ps_4_0), &vs);
    ok(hr == E_INVALIDARG, "Created a SM4 vertex shader from a pixel shader source, hr %#x\n", hr);

    /* pixel shader */
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreatePixelShader(device, ps_4_0, sizeof(ps_4_0), &ps);
    ok(SUCCEEDED(hr), "Failed to create SM4 vertex shader, hr %#x\n", hr);

    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10PixelShader_GetDevice(ps, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10PixelShader_QueryInterface(ps, &IID_ID3D11PixelShader, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Pixel shader should implement ID3D11PixelShader.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    refcount = ID3D10PixelShader_Release(ps);
    ok(!refcount, "Pixel shader has %u references left.\n", refcount);

    /* geometry shader */
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateGeometryShader(device, gs_4_0, sizeof(gs_4_0), &gs);
    ok(SUCCEEDED(hr), "Failed to create SM4 geometry shader, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10GeometryShader_GetDevice(gs, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10GeometryShader_QueryInterface(gs, &IID_ID3D11GeometryShader, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Geometry shader should implement ID3D11GeometryShader.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    refcount = ID3D10GeometryShader_Release(gs);
    ok(!refcount, "Geometry shader has %u references left.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_sampler_state(void)
{
    static const struct test
    {
        D3D10_FILTER filter;
        D3D11_FILTER expected_filter;
    }
    desc_conversion_tests[] =
    {
        {D3D10_FILTER_MIN_MAG_MIP_POINT, D3D11_FILTER_MIN_MAG_MIP_POINT},
        {D3D10_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR},
        {D3D10_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT, D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT},
        {D3D10_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR},
        {D3D10_FILTER_MIN_LINEAR_MAG_MIP_POINT, D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT},
        {D3D10_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR},
        {D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT},
        {D3D10_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_MIN_MAG_MIP_LINEAR},
        {D3D10_FILTER_ANISOTROPIC, D3D11_FILTER_ANISOTROPIC},
        {D3D10_FILTER_COMPARISON_MIN_MAG_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT},
        {D3D10_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR},
        {
            D3D10_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT
        },
        {D3D10_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR},
        {D3D10_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT},
        {
            D3D10_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        },
        {D3D10_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT},
        {D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR},
        {D3D10_FILTER_COMPARISON_ANISOTROPIC, D3D11_FILTER_COMPARISON_ANISOTROPIC},
    };

    ID3D10SamplerState *sampler_state1, *sampler_state2;
    ID3D11SamplerState *d3d11_sampler_state;
    ULONG refcount, expected_refcount;
    ID3D10Device *device, *tmp;
    ID3D11Device *d3d11_device;
    D3D10_SAMPLER_DESC desc;
    unsigned int i;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateSamplerState(device, NULL, &sampler_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
    desc.BorderColor[0] = 0.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 0.0f;
    desc.BorderColor[3] = 1.0f;
    desc.MinLOD = 0.0f;
    desc.MaxLOD = 16.0f;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateSamplerState(device, &desc, &sampler_state1);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateSamplerState(device, &desc, &sampler_state2);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    ok(sampler_state1 == sampler_state2, "Got different sampler state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10SamplerState_GetDevice(sampler_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10SamplerState_GetDesc(sampler_state1, &desc);
    ok(desc.Filter == D3D10_FILTER_MIN_MAG_MIP_LINEAR, "Got unexpected filter %#x.\n", desc.Filter);
    ok(desc.AddressU == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected address u %u.\n", desc.AddressU);
    ok(desc.AddressV == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected address v %u.\n", desc.AddressV);
    ok(desc.AddressW == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected address w %u.\n", desc.AddressW);
    ok(!desc.MipLODBias, "Got unexpected mip LOD bias %f.\n", desc.MipLODBias);
    ok(!desc.MaxAnisotropy || broken(desc.MaxAnisotropy == 16) /* Not set to 0 on all Windows versions. */,
            "Got unexpected max anisotropy %u.\n", desc.MaxAnisotropy);
    ok(desc.ComparisonFunc == D3D10_COMPARISON_NEVER, "Got unexpected comparison func %u.\n", desc.ComparisonFunc);
    ok(!desc.BorderColor[0] && !desc.BorderColor[1] && !desc.BorderColor[2] && !desc.BorderColor[3],
            "Got unexpected border color {%.8e, %.8e, %.8e, %.8e}.\n",
            desc.BorderColor[0], desc.BorderColor[1], desc.BorderColor[2], desc.BorderColor[3]);
    ok(!desc.MinLOD, "Got unexpected min LOD %f.\n", desc.MinLOD);
    ok(desc.MaxLOD == 16.0f, "Got unexpected max LOD %f.\n", desc.MaxLOD);

    refcount = ID3D10SamplerState_Release(sampler_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10SamplerState_Release(sampler_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    hr = ID3D10Device_QueryInterface(device, &IID_ID3D11Device, (void **)&d3d11_device);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Device should implement ID3D11Device.\n");
    if (FAILED(hr))
    {
        win_skip("D3D11 is not available.\n");
        goto done;
    }

    for (i = 0; i < sizeof(desc_conversion_tests) / sizeof(*desc_conversion_tests); ++i)
    {
        const struct test *current = &desc_conversion_tests[i];
        D3D11_SAMPLER_DESC d3d11_desc, expected_desc;

        desc.Filter = current->filter;
        desc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D10_TEXTURE_ADDRESS_BORDER;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
        desc.BorderColor[0] = 0.0f;
        desc.BorderColor[1] = 1.0f;
        desc.BorderColor[2] = 0.0f;
        desc.BorderColor[3] = 1.0f;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = 16.0f;

        hr = ID3D10Device_CreateSamplerState(device, &desc, &sampler_state1);
        ok(SUCCEEDED(hr), "Test %u: Failed to create sampler state, hr %#x.\n", i, hr);

        hr = ID3D10SamplerState_QueryInterface(sampler_state1, &IID_ID3D11SamplerState,
                (void **)&d3d11_sampler_state);
        ok(SUCCEEDED(hr), "Test %u: Sampler state should implement ID3D11SamplerState.\n", i);

        memcpy(&expected_desc, &desc, sizeof(expected_desc));
        expected_desc.Filter = current->expected_filter;
        if (!D3D11_DECODE_IS_ANISOTROPIC_FILTER(current->filter))
            expected_desc.MaxAnisotropy = 0;
        if (!D3D11_DECODE_IS_COMPARISON_FILTER(current->filter))
            expected_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

        ID3D11SamplerState_GetDesc(d3d11_sampler_state, &d3d11_desc);
        ok(d3d11_desc.Filter == expected_desc.Filter,
                "Test %u: Got unexpected filter %#x.\n", i, d3d11_desc.Filter);
        ok(d3d11_desc.AddressU == expected_desc.AddressU,
                "Test %u: Got unexpected address u %u.\n", i, d3d11_desc.AddressU);
        ok(d3d11_desc.AddressV == expected_desc.AddressV,
                "Test %u: Got unexpected address v %u.\n", i, d3d11_desc.AddressV);
        ok(d3d11_desc.AddressW == expected_desc.AddressW,
                "Test %u: Got unexpected address w %u.\n", i, d3d11_desc.AddressW);
        ok(d3d11_desc.MipLODBias == expected_desc.MipLODBias,
                "Test %u: Got unexpected mip LOD bias %f.\n", i, d3d11_desc.MipLODBias);
        ok(d3d11_desc.MaxAnisotropy == expected_desc.MaxAnisotropy,
                "Test %u: Got unexpected max anisotropy %u.\n", i, d3d11_desc.MaxAnisotropy);
        ok(d3d11_desc.ComparisonFunc == expected_desc.ComparisonFunc,
                "Test %u: Got unexpected comparison func %u.\n", i, d3d11_desc.ComparisonFunc);
        ok(d3d11_desc.BorderColor[0] == expected_desc.BorderColor[0]
                && d3d11_desc.BorderColor[1] == expected_desc.BorderColor[1]
                && d3d11_desc.BorderColor[2] == expected_desc.BorderColor[2]
                && d3d11_desc.BorderColor[3] == expected_desc.BorderColor[3],
                "Test %u: Got unexpected border color {%.8e, %.8e, %.8e, %.8e}.\n", i,
                d3d11_desc.BorderColor[0], d3d11_desc.BorderColor[1],
                d3d11_desc.BorderColor[2], d3d11_desc.BorderColor[3]);
        ok(d3d11_desc.MinLOD == expected_desc.MinLOD,
                "Test %u: Got unexpected min LOD %f.\n", i, d3d11_desc.MinLOD);
        ok(d3d11_desc.MaxLOD == expected_desc.MaxLOD,
                "Test %u: Got unexpected max LOD %f.\n", i, d3d11_desc.MaxLOD);

        refcount = ID3D11SamplerState_Release(d3d11_sampler_state);
        ok(refcount == 1, "Test %u: Got unexpected refcount %u.\n", i, refcount);

        hr = ID3D11Device_CreateSamplerState(d3d11_device, &d3d11_desc, &d3d11_sampler_state);
        ok(SUCCEEDED(hr), "Test %u: Failed to create sampler state, hr %#x.\n", i, hr);
        hr = ID3D11SamplerState_QueryInterface(d3d11_sampler_state, &IID_ID3D10SamplerState,
                (void **)&sampler_state2);
        ok(SUCCEEDED(hr), "Test %u: Sampler state should implement ID3D10SamplerState.\n", i);
        ok(sampler_state1 == sampler_state2, "Test %u: Got different sampler state objects.\n", i);

        refcount = ID3D11SamplerState_Release(d3d11_sampler_state);
        ok(refcount == 2, "Test %u: Got unexpected refcount %u.\n", i, refcount);
        refcount = ID3D10SamplerState_Release(sampler_state2);
        ok(refcount == 1, "Test %u: Got unexpected refcount %u.\n", i, refcount);
        refcount = ID3D10SamplerState_Release(sampler_state1);
        ok(!refcount, "Test %u: Got unexpected refcount %u.\n", i, refcount);
    }

    ID3D11Device_Release(d3d11_device);

done:
    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_blend_state(void)
{
    ID3D10BlendState *blend_state1, *blend_state2;
    ID3D11BlendState *d3d11_blend_state;
    ULONG refcount, expected_refcount;
    D3D11_BLEND_DESC d3d11_blend_desc;
    D3D10_BLEND_DESC blend_desc;
    ID3D11Device *d3d11_device;
    ID3D10Device *device, *tmp;
    IUnknown *iface;
    unsigned int i;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D10Device_CreateBlendState(device, NULL, &blend_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.SrcBlend = D3D10_BLEND_ONE;
    blend_desc.DestBlend = D3D10_BLEND_ZERO;
    blend_desc.BlendOp = D3D10_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_ONE;
    blend_desc.DestBlendAlpha = D3D10_BLEND_ZERO;
    blend_desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        blend_desc.BlendEnable[i] = FALSE;
        blend_desc.RenderTargetWriteMask[i] = D3D10_COLOR_WRITE_ENABLE_ALL;
    }

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state1);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state2);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    ok(blend_state1 == blend_state2, "Got different blend state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10BlendState_GetDevice(blend_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    hr = ID3D10BlendState_QueryInterface(blend_state1, &IID_ID3D10BlendState1, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Blend state should implement ID3D10BlendState1.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);

    hr = ID3D10Device_QueryInterface(device, &IID_ID3D11Device, (void **)&d3d11_device);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Device should implement ID3D11Device.\n");
    if (FAILED(hr))
    {
        win_skip("D3D11 is not available.\n");
        goto done;
    }

    hr = ID3D10BlendState_QueryInterface(blend_state1, &IID_ID3D11BlendState, (void **)&d3d11_blend_state);
    ok(SUCCEEDED(hr), "Blend state should implement ID3D11BlendState.\n");

    ID3D11BlendState_GetDesc(d3d11_blend_state, &d3d11_blend_desc);
    ok(d3d11_blend_desc.AlphaToCoverageEnable == blend_desc.AlphaToCoverageEnable,
            "Got unexpected alpha to coverage enable %#x.\n", d3d11_blend_desc.AlphaToCoverageEnable);
    ok(d3d11_blend_desc.IndependentBlendEnable == FALSE,
            "Got unexpected independent blend enable %#x.\n", d3d11_blend_desc.IndependentBlendEnable);
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(d3d11_blend_desc.RenderTarget[i].BlendEnable == blend_desc.BlendEnable[i],
                "Got unexpected blend enable %#x for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].BlendEnable, i);
        ok(d3d11_blend_desc.RenderTarget[i].SrcBlend == (D3D11_BLEND)blend_desc.SrcBlend,
                "Got unexpected src blend %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].SrcBlend, i);
        ok(d3d11_blend_desc.RenderTarget[i].DestBlend == (D3D11_BLEND)blend_desc.DestBlend,
                "Got unexpected dest blend %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].DestBlend, i);
        ok(d3d11_blend_desc.RenderTarget[i].BlendOp == (D3D11_BLEND_OP)blend_desc.BlendOp,
                "Got unexpected blend op %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].BlendOp, i);
        ok(d3d11_blend_desc.RenderTarget[i].SrcBlendAlpha == (D3D11_BLEND)blend_desc.SrcBlendAlpha,
                "Got unexpected src blend alpha %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].SrcBlendAlpha, i);
        ok(d3d11_blend_desc.RenderTarget[i].DestBlendAlpha == (D3D11_BLEND)blend_desc.DestBlendAlpha,
                "Got unexpected dest blend alpha %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].DestBlendAlpha, i);
        ok(d3d11_blend_desc.RenderTarget[i].BlendOpAlpha == (D3D11_BLEND_OP)blend_desc.BlendOpAlpha,
                "Got unexpected blend op alpha %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].BlendOpAlpha, i);
        ok(d3d11_blend_desc.RenderTarget[i].RenderTargetWriteMask == blend_desc.RenderTargetWriteMask[i],
                "Got unexpected render target write mask %#x for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].RenderTargetWriteMask, i);
    }

    refcount = ID3D11BlendState_Release(d3d11_blend_state);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10BlendState_Release(blend_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = ID3D11Device_CreateBlendState(d3d11_device, &d3d11_blend_desc, &d3d11_blend_state);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    hr = ID3D11BlendState_QueryInterface(d3d11_blend_state, &IID_ID3D10BlendState, (void **)&blend_state2);
    ok(SUCCEEDED(hr), "Blend state should implement ID3D10BlendState.\n");
    ok(blend_state1 == blend_state2, "Got different blend state objects.\n");

    refcount = ID3D11BlendState_Release(d3d11_blend_state);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10BlendState_Release(blend_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10BlendState_Release(blend_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    blend_desc.BlendEnable[0] = TRUE;
    blend_desc.RenderTargetWriteMask[1] = D3D10_COLOR_WRITE_ENABLE_RED;
    blend_desc.RenderTargetWriteMask[2] = D3D10_COLOR_WRITE_ENABLE_GREEN;
    blend_desc.RenderTargetWriteMask[3] = D3D10_COLOR_WRITE_ENABLE_BLUE;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state1);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    hr = ID3D10BlendState_QueryInterface(blend_state1, &IID_ID3D11BlendState, (void **)&d3d11_blend_state);
    ok(SUCCEEDED(hr), "Blend state should implement ID3D11BlendState.\n");

    ID3D11BlendState_GetDesc(d3d11_blend_state, &d3d11_blend_desc);
    ok(d3d11_blend_desc.AlphaToCoverageEnable == blend_desc.AlphaToCoverageEnable,
            "Got unexpected alpha to coverage enable %#x.\n", d3d11_blend_desc.AlphaToCoverageEnable);
    ok(d3d11_blend_desc.IndependentBlendEnable == TRUE,
            "Got unexpected independent blend enable %#x.\n", d3d11_blend_desc.IndependentBlendEnable);
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(d3d11_blend_desc.RenderTarget[i].BlendEnable == blend_desc.BlendEnable[i],
                "Got unexpected blend enable %#x for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].BlendEnable, i);
        ok(d3d11_blend_desc.RenderTarget[i].SrcBlend == (D3D11_BLEND)blend_desc.SrcBlend,
                "Got unexpected src blend %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].SrcBlend, i);
        ok(d3d11_blend_desc.RenderTarget[i].DestBlend == (D3D11_BLEND)blend_desc.DestBlend,
                "Got unexpected dest blend %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].DestBlend, i);
        ok(d3d11_blend_desc.RenderTarget[i].BlendOp == (D3D11_BLEND_OP)blend_desc.BlendOp,
                "Got unexpected blend op %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].BlendOp, i);
        ok(d3d11_blend_desc.RenderTarget[i].SrcBlendAlpha == (D3D11_BLEND)blend_desc.SrcBlendAlpha,
                "Got unexpected src blend alpha %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].SrcBlendAlpha, i);
        ok(d3d11_blend_desc.RenderTarget[i].DestBlendAlpha == (D3D11_BLEND)blend_desc.DestBlendAlpha,
                "Got unexpected dest blend alpha %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].DestBlendAlpha, i);
        ok(d3d11_blend_desc.RenderTarget[i].BlendOpAlpha == (D3D11_BLEND_OP)blend_desc.BlendOpAlpha,
                "Got unexpected blend op alpha %u for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].BlendOpAlpha, i);
        ok(d3d11_blend_desc.RenderTarget[i].RenderTargetWriteMask == blend_desc.RenderTargetWriteMask[i],
                "Got unexpected render target write mask %#x for render target %u.\n",
                d3d11_blend_desc.RenderTarget[i].RenderTargetWriteMask, i);
    }

    refcount = ID3D11BlendState_Release(d3d11_blend_state);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = ID3D11Device_CreateBlendState(d3d11_device, &d3d11_blend_desc, &d3d11_blend_state);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    hr = ID3D11BlendState_QueryInterface(d3d11_blend_state, &IID_ID3D10BlendState, (void **)&blend_state2);
    ok(SUCCEEDED(hr), "Blend state should implement ID3D10BlendState.\n");
    ok(blend_state1 == blend_state2, "Got different blend state objects.\n");

    refcount = ID3D11BlendState_Release(d3d11_blend_state);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    ID3D11Device_Release(d3d11_device);

done:
    refcount = ID3D10BlendState_Release(blend_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10BlendState_Release(blend_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_depthstencil_state(void)
{
    ID3D10DepthStencilState *ds_state1, *ds_state2;
    ULONG refcount, expected_refcount;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateDepthStencilState(device, NULL, &ds_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    ds_desc.DepthEnable = TRUE;
    ds_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D10_COMPARISON_LESS;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
    ds_desc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
    ds_desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
    ds_desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state1);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state2);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);
    ok(ds_state1 == ds_state2, "Got different depthstencil state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10DepthStencilState_GetDevice(ds_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    refcount = ID3D10DepthStencilState_Release(ds_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10DepthStencilState_Release(ds_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    ds_desc.DepthEnable = FALSE;
    ds_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
    ds_desc.DepthFunc = D3D10_COMPARISON_NEVER;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = 0;
    ds_desc.StencilWriteMask = 0;
    ds_desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilFunc = D3D10_COMPARISON_NEVER;
    ds_desc.BackFace = ds_desc.FrontFace;

    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state1);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);

    memset(&ds_desc, 0, sizeof(ds_desc));
    ID3D10DepthStencilState_GetDesc(ds_state1, &ds_desc);
    ok(!ds_desc.DepthEnable, "Got unexpected depth enable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ALL
            || broken(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ZERO),
            "Got unexpected depth write mask %#x.\n", ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_LESS || broken(ds_desc.DepthFunc == D3D10_COMPARISON_NEVER),
            "Got unexpected depth func %#x.\n", ds_desc.DepthFunc);
    ok(!ds_desc.StencilEnable, "Got unexpected stencil enable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == D3D10_DEFAULT_STENCIL_READ_MASK,
            "Got unexpected stencil read mask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == D3D10_DEFAULT_STENCIL_WRITE_MASK,
            "Got unexpected stencil write mask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected front face stencil depth fail op %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected front face stencil pass op %#x.\n", ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected front face stencil fail op %#x.\n", ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_ALWAYS,
            "Got unexpected front face stencil func %#x.\n", ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected back face stencil depth fail op %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected back face stencil pass op %#x.\n", ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected back face stencil fail op %#x.\n", ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_ALWAYS,
            "Got unexpected back face stencil func %#x.\n", ds_desc.BackFace.StencilFunc);

    ID3D10DepthStencilState_Release(ds_state1);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_rasterizer_state(void)
{
    ID3D10RasterizerState *rast_state1, *rast_state2;
    ULONG refcount, expected_refcount;
    D3D10_RASTERIZER_DESC rast_desc;
    ID3D10Device *device, *tmp;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_CreateRasterizerState(device, NULL, &rast_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    rast_desc.FillMode = D3D10_FILL_SOLID;
    rast_desc.CullMode = D3D10_CULL_BACK;
    rast_desc.FrontCounterClockwise = FALSE;
    rast_desc.DepthBias = 0;
    rast_desc.DepthBiasClamp = 0.0f;
    rast_desc.SlopeScaledDepthBias = 0.0f;
    rast_desc.DepthClipEnable = TRUE;
    rast_desc.ScissorEnable = FALSE;
    rast_desc.MultisampleEnable = FALSE;
    rast_desc.AntialiasedLineEnable = FALSE;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreateRasterizerState(device, &rast_desc, &rast_state1);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    hr = ID3D10Device_CreateRasterizerState(device, &rast_desc, &rast_state2);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ok(rast_state1 == rast_state2, "Got different rasterizer state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10RasterizerState_GetDevice(rast_state1, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    refcount = ID3D10RasterizerState_Release(rast_state2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = ID3D10RasterizerState_Release(rast_state1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_create_query(void)
{
    static const struct
    {
        D3D10_QUERY query;
        BOOL is_predicate;
        BOOL todo;
    }
    tests[] =
    {
        {D3D10_QUERY_EVENT,                 FALSE, FALSE},
        {D3D10_QUERY_OCCLUSION,             FALSE, FALSE},
        {D3D10_QUERY_TIMESTAMP,             FALSE, FALSE},
        {D3D10_QUERY_TIMESTAMP_DISJOINT,    FALSE, FALSE},
        {D3D10_QUERY_PIPELINE_STATISTICS,   FALSE, TRUE},
        {D3D10_QUERY_OCCLUSION_PREDICATE,   TRUE,  FALSE},
        {D3D10_QUERY_SO_STATISTICS,         FALSE, TRUE},
        {D3D10_QUERY_SO_OVERFLOW_PREDICATE, TRUE,  TRUE},
    };

    ULONG refcount, expected_refcount;
    D3D10_QUERY_DESC query_desc;
    ID3D10Predicate *predicate;
    ID3D10Device *device, *tmp;
    HRESULT hr, expected_hr;
    ID3D10Query *query;
    IUnknown *iface;
    unsigned int i;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D10Device_CreateQuery(device, NULL, &query);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_CreatePredicate(device, NULL, &predicate);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        query_desc.Query = tests[i].query;
        query_desc.MiscFlags = 0;

        hr = ID3D10Device_CreateQuery(device, &query_desc, NULL);
        todo_wine_if(tests[i].todo)
        ok(hr == S_FALSE, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);

        query_desc.Query = tests[i].query;
        hr = ID3D10Device_CreateQuery(device, &query_desc, &query);
        todo_wine_if(tests[i].todo)
        ok(hr == S_OK, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);
        if (FAILED(hr))
            continue;

        expected_hr = tests[i].is_predicate ? S_OK : E_NOINTERFACE;
        hr = ID3D10Query_QueryInterface(query, &IID_ID3D10Predicate, (void **)&predicate);
        ID3D10Query_Release(query);
        ok(hr == expected_hr, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);
        if (SUCCEEDED(hr))
            ID3D10Predicate_Release(predicate);

        expected_hr = tests[i].is_predicate ? S_FALSE : E_INVALIDARG;
        hr = ID3D10Device_CreatePredicate(device, &query_desc, NULL);
        ok(hr == expected_hr, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);

        expected_hr = tests[i].is_predicate ? S_OK : E_INVALIDARG;
        hr = ID3D10Device_CreatePredicate(device, &query_desc, &predicate);
        ok(hr == expected_hr, "Got unexpected hr %#x for query type %u.\n", hr, query_desc.Query);
        if (SUCCEEDED(hr))
            ID3D10Predicate_Release(predicate);
    }

    query_desc.Query = D3D10_QUERY_OCCLUSION_PREDICATE;
    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device_CreatePredicate(device, &query_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %u, expected >= %u.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10Predicate_GetDevice(predicate, &tmp);
    ok(tmp == device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);
    hr = ID3D10Predicate_QueryInterface(predicate, &IID_ID3D11Predicate, (void **)&iface);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Predicate should implement ID3D11Predicate.\n");
    if (SUCCEEDED(hr)) IUnknown_Release(iface);
    ID3D10Predicate_Release(predicate);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_occlusion_query(void)
{
    static const struct vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};

    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10RenderTargetView *rtv;
    D3D10_QUERY_DESC query_desc;
    ID3D10Asynchronous *query;
    unsigned int data_size, i;
    ID3D10Texture2D *texture;
    ID3D10Device *device;
    D3D10_VIEWPORT vp;
    union
    {
        UINT64 uint;
        DWORD dword[2];
    } data;
    HRESULT hr;

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, white);

    query_desc.Query = D3D10_QUERY_OCCLUSION;
    query_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateQuery(device, &query_desc, (ID3D10Query **)&query);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    data_size = ID3D10Asynchronous_GetDataSize(query);
    ok(data_size == sizeof(data), "Got unexpected data size %u.\n", data_size);

    hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data), 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    ID3D10Asynchronous_End(query);
    ID3D10Asynchronous_Begin(query);
    ID3D10Asynchronous_Begin(query);

    hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data), 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    draw_color_quad(&test_context, &red);

    ID3D10Asynchronous_End(query);
    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    memset(&data, 0xff, sizeof(data));
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(data.uint == 640 * 480, "Got unexpected query result 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    memset(&data, 0xff, sizeof(data));
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(DWORD), 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(WORD), 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data) - 1, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data) + 1, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(data.dword[0] == 0xffffffff && data.dword[1] == 0xffffffff,
            "Data was modified 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    memset(&data, 0xff, sizeof(data));
    hr = ID3D10Asynchronous_GetData(query, &data, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(data.dword[0] == 0xffffffff && data.dword[1] == 0xffffffff,
            "Data was modified 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    hr = ID3D10Asynchronous_GetData(query, NULL, sizeof(DWORD), 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, NULL, sizeof(data), 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    ID3D10Asynchronous_Begin(query);
    ID3D10Asynchronous_End(query);
    ID3D10Asynchronous_End(query);

    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    data.dword[0] = 0x12345678;
    data.dword[1] = 0x12345678;
    hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!data.uint, "Got unexpected query result 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    texture_desc.Width = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    texture_desc.Height = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = texture_desc.Width;
    vp.Height = texture_desc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D10Device_RSSetViewports(device, 1, &vp);

    ID3D10Asynchronous_Begin(query);
    for (i = 0; i < 100; i++)
        draw_color_quad(&test_context, &red);
    ID3D10Asynchronous_End(query);

    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    memset(&data, 0xff, sizeof(data));
    hr = ID3D10Asynchronous_GetData(query, NULL, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Asynchronous_GetData(query, &data, sizeof(data), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok((data.dword[0] == 0x90000000 && data.dword[1] == 0x1)
            || (data.dword[0] == 0xffffffff && !data.dword[1])
            || broken(!data.uint),
            "Got unexpected query result 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    ID3D10Asynchronous_Release(query);
    ID3D10RenderTargetView_Release(rtv);
    ID3D10Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_timestamp_query(void)
{
    static const struct vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};

    D3D10_QUERY_DATA_TIMESTAMP_DISJOINT disjoint, prev_disjoint;
    ID3D10Query *timestamp_query, *timestamp_disjoint_query;
    struct d3d10core_test_context test_context;
    D3D10_QUERY_DESC query_desc;
    unsigned int data_size, i;
    ID3D10Device *device;
    UINT64 timestamp;
    HRESULT hr;

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    query_desc.Query = D3D10_QUERY_TIMESTAMP;
    query_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateQuery(device, &query_desc, &timestamp_query);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    data_size = ID3D10Query_GetDataSize(timestamp_query);
    ok(data_size == sizeof(UINT64), "Got unexpected data size %u.\n", data_size);

    query_desc.Query = D3D10_QUERY_TIMESTAMP_DISJOINT;
    query_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateQuery(device, &query_desc, &timestamp_disjoint_query);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    data_size = ID3D10Query_GetDataSize(timestamp_disjoint_query);
    ok(data_size == sizeof(disjoint), "Got unexpected data size %u.\n", data_size);

    /* Test a TIMESTAMP_DISJOINT query. */
    ID3D10Query_Begin(timestamp_disjoint_query);
    ID3D10Query_End(timestamp_disjoint_query);
    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Query_GetData(timestamp_disjoint_query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    disjoint.Frequency = 0xdeadbeef;
    disjoint.Disjoint = 0xff;
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(disjoint.Frequency != 0xdeadbeef, "Frequency data was not modified.\n");
    ok(disjoint.Disjoint == TRUE || disjoint.Disjoint == FALSE, "Got unexpected disjoint %#x.\n", disjoint.Disjoint);

    prev_disjoint = disjoint;

    disjoint.Frequency = 0xdeadbeef;
    disjoint.Disjoint = 0xff;
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint) - 1, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint) + 1, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint) / 2, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint) * 2, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(disjoint.Frequency == 0xdeadbeef, "Frequency data was modified.\n");
    ok(disjoint.Disjoint == 0xff, "Disjoint data was modified.\n");

    hr = ID3D10Query_GetData(timestamp_disjoint_query, NULL, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint), D3D10_ASYNC_GETDATA_DONOTFLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!memcmp(&disjoint, &prev_disjoint, sizeof(disjoint)), "Disjoint data mismatch.\n");

    hr = ID3D10Query_GetData(timestamp_query, NULL, 0, 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp), 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    /* Test a TIMESTAMP query inside a TIMESTAMP_DISJOINT query. */
    ID3D10Query_Begin(timestamp_disjoint_query);

    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp), 0);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#x.\n", hr);

    draw_color_quad(&test_context, &red);

    ID3D10Query_End(timestamp_query);
    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Query_GetData(timestamp_query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    timestamp = 0xdeadbeef;
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp) / 2, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(timestamp == 0xdeadbeef, "Timestamp was modified.\n");

    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(timestamp != 0xdeadbeef, "Timestamp was not modified.\n");

    timestamp = 0xdeadbeef;
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp) - 1, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp) + 1, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp) / 2, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp) * 2, 0);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(timestamp == 0xdeadbeef, "Timestamp was modified.\n");

    ID3D10Query_End(timestamp_disjoint_query);
    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Query_GetData(timestamp_disjoint_query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    disjoint.Frequency = 0xdeadbeef;
    disjoint.Disjoint = 0xff;
    hr = ID3D10Query_GetData(timestamp_disjoint_query, &disjoint, sizeof(disjoint), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(disjoint.Frequency != 0xdeadbeef, "Frequency data was not modified.\n");
    ok(disjoint.Disjoint == TRUE || disjoint.Disjoint == FALSE, "Got unexpected disjoint %#x.\n", disjoint.Disjoint);

    /* It's not strictly necessary for the TIMESTAMP query to be inside a TIMESTAMP_DISJOINT query. */
    ID3D10Query_Release(timestamp_query);
    query_desc.Query = D3D10_QUERY_TIMESTAMP;
    query_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateQuery(device, &query_desc, &timestamp_query);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    draw_color_quad(&test_context, &red);

    ID3D10Query_End(timestamp_query);
    for (i = 0; i < 500; ++i)
    {
        if ((hr = ID3D10Query_GetData(timestamp_query, NULL, 0, 0)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Query_GetData(timestamp_query, &timestamp, sizeof(timestamp), 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ID3D10Query_Release(timestamp_query);
    ID3D10Query_Release(timestamp_disjoint_query);
    release_test_context(&test_context);
}

static void test_device_removed_reason(void)
{
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = ID3D10Device_GetDeviceRemovedReason(device);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_GetDeviceRemovedReason(device);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_scissor(void)
{
    struct d3d10core_test_context test_context;
    D3D10_RASTERIZER_DESC rs_desc;
    ID3D10RasterizerState *rs;
    D3D10_RECT scissor_rect;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    DWORD color;
    HRESULT hr;

    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static const DWORD ps_code[] =
    {
#if 0
        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            return float4(0.0, 1.0, 0.0, 1.0);
        }
#endif
        0x43425844, 0x30240e72, 0x012f250c, 0x8673c6ea, 0x392e4cec, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002,
        0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    rs_desc.FillMode = D3D10_FILL_SOLID;
    rs_desc.CullMode = D3D10_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;
    hr = ID3D10Device_CreateRasterizerState(device, &rs_desc, &rs);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);

    ID3D10Device_PSSetShader(device, ps);

    scissor_rect.left = 160;
    scissor_rect.top = 120;
    scissor_rect.right = 480;
    scissor_rect.bottom = 360;
    ID3D10Device_RSSetScissorRects(device, 1, &scissor_rect);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
    check_texture_color(test_context.backbuffer, 0xff0000ff, 1);

    draw_quad(&test_context);
    color = get_texture_color(test_context.backbuffer, 320, 60);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 80, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 560, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 420);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
    ID3D10Device_RSSetState(device, rs);
    draw_quad(&test_context);
    color = get_texture_color(test_context.backbuffer, 320, 60);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 80, 240);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 560, 240);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 420);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10RasterizerState_Release(rs);
    ID3D10PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_clear_state(void)
{
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
#if 0
float4 main(float4 pos : POSITION) : POSITION
{
    return pos;
}
#endif
    static const DWORD simple_vs[] =
    {
        0x43425844, 0x66689e7c, 0x643f0971, 0xb7f67ff4, 0xabc48688, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x49534f50, 0x4e4f4954, 0xababab00, 0x52444853, 0x00000038, 0x00010040,
        0x0000000e, 0x0300005f, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

#if 0
struct gs_out
{
    float4 pos : SV_POSITION;
};

[maxvertexcount(4)]
void main(point float4 vin[1] : POSITION, inout TriangleStream<gs_out> vout)
{
    float offset = 0.1 * vin[0].w;
    gs_out v;

    v.pos = float4(vin[0].x - offset, vin[0].y - offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x - offset, vin[0].y + offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x + offset, vin[0].y - offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x + offset, vin[0].y + offset, vin[0].z, vin[0].w);
    vout.Append(v);
}
#endif
    static const DWORD simple_gs[] =
    {
        0x43425844, 0x000ee786, 0xc624c269, 0x885a5cbe, 0x444b3b1f, 0x00000001, 0x0000023c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x000001a0, 0x00020040,
        0x00000068, 0x0400005f, 0x002010f2, 0x00000001, 0x00000000, 0x02000068, 0x00000001, 0x0100085d,
        0x0100285c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x0200005e, 0x00000004, 0x0f000032,
        0x00100032, 0x00000000, 0x80201ff6, 0x00000041, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd,
        0x3dcccccd, 0x00000000, 0x00000000, 0x00201046, 0x00000000, 0x00000000, 0x05000036, 0x00102032,
        0x00000000, 0x00100046, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x0e000032,
        0x00100052, 0x00000000, 0x00201ff6, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd, 0x00000000,
        0x3dcccccd, 0x00000000, 0x00201106, 0x00000000, 0x00000000, 0x05000036, 0x00102022, 0x00000000,
        0x0010002a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000,
        0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x05000036, 0x00102022,
        0x00000000, 0x0010001a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102032, 0x00000000, 0x00100086, 0x00000000, 0x06000036,
        0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000, 0x01000013, 0x0100003e,
    };

#if 0
float4 main(float4 color : COLOR) : SV_TARGET
{
    return color;
}
#endif
    static const DWORD simple_ps[] =
    {
        0x43425844, 0x08c2b568, 0x17d33120, 0xb7d82948, 0x13a570fb, 0x00000001, 0x000000d0, 0x00000003,
        0x0000002c, 0x0000005c, 0x00000090, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

    D3D10_VIEWPORT tmp_viewport[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D10ShaderResourceView *tmp_srv[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10ShaderResourceView *srv[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10RenderTargetView *tmp_rtv[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    RECT tmp_rect[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D10SamplerState *tmp_sampler[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10RenderTargetView *rtv[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D10Texture2D *rt_texture[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D10Buffer *cb[D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D10Buffer *tmp_buffer[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10SamplerState *sampler[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10Buffer *buffer[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT offset[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT stride[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10Buffer *so_buffer[D3D10_SO_BUFFER_SLOT_COUNT];
    ID3D10InputLayout *tmp_input_layout, *input_layout;
    ID3D10DepthStencilState *tmp_ds_state, *ds_state;
    ID3D10BlendState *tmp_blend_state, *blend_state;
    ID3D10RasterizerState *tmp_rs_state, *rs_state;
    ID3D10Predicate *tmp_predicate, *predicate;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D10DepthStencilView *tmp_dsv, *dsv;
    D3D10_PRIMITIVE_TOPOLOGY topology;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10GeometryShader *tmp_gs, *gs;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10VertexShader *tmp_vs, *vs;
    D3D10_SAMPLER_DESC sampler_desc;
    D3D10_QUERY_DESC predicate_desc;
    ID3D10PixelShader *tmp_ps, *ps;
    D3D10_RASTERIZER_DESC rs_desc;
    D3D10_BLEND_DESC blend_desc;
    ID3D10Texture2D *ds_texture;
    float tmp_blend_factor[4];
    float blend_factor[4];
    ID3D10Device *device;
    BOOL predicate_value;
    DXGI_FORMAT format;
    UINT sample_mask;
    UINT stencil_ref;
    ULONG refcount;
    UINT count, i;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    /* Verify the initial state after device creation. */

    ID3D10Device_VSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_VSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_VSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_VSGetShader(device, &tmp_vs);
    ok(!tmp_vs, "Got unexpected vertex shader %p.\n", tmp_vs);

    ID3D10Device_GSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_GSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_GSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_GSGetShader(device, &tmp_gs);
    ok(!tmp_gs, "Got unexpected geometry shader %p.\n", tmp_gs);

    ID3D10Device_PSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_PSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_PSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_PSGetShader(device, &tmp_ps);
    ok(!tmp_ps, "Got unexpected pixel shader %p.\n", tmp_ps);

    ID3D10Device_IAGetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, tmp_buffer, stride, offset);
    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected vertex buffer %p in slot %u.\n", tmp_buffer[i], i);
        ok(!stride[i], "Got unexpected stride %u in slot %u.\n", stride[i], i);
        ok(!offset[i], "Got unexpected offset %u in slot %u.\n", offset[i], i);
    }
    ID3D10Device_IAGetIndexBuffer(device, tmp_buffer, &format, offset);
    ok(!tmp_buffer[0], "Got unexpected index buffer %p.\n", tmp_buffer[0]);
    ok(format == DXGI_FORMAT_UNKNOWN, "Got unexpected index buffer format %#x.\n", format);
    ok(!offset[0], "Got unexpected index buffer offset %u.\n", offset[0]);
    ID3D10Device_IAGetInputLayout(device, &tmp_input_layout);
    ok(!tmp_input_layout, "Got unexpected input layout %p.\n", tmp_input_layout);
    ID3D10Device_IAGetPrimitiveTopology(device, &topology);
    ok(topology == D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, "Got unexpected primitive topology %#x.\n", topology);

    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, blend_factor, &sample_mask);
    ok(!tmp_blend_state, "Got unexpected blend state %p.\n", tmp_blend_state);
    ok(blend_factor[0] == 1.0f && blend_factor[1] == 1.0f
            && blend_factor[2] == 1.0f && blend_factor[3] == 1.0f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            blend_factor[0], blend_factor[1], blend_factor[2], blend_factor[3]);
    ok(sample_mask == D3D10_DEFAULT_SAMPLE_MASK, "Got unexpected sample mask %#x.\n", sample_mask);
    ID3D10Device_OMGetDepthStencilState(device, &tmp_ds_state, &stencil_ref);
    ok(!tmp_ds_state, "Got unexpected depth stencil state %p.\n", tmp_ds_state);
    ok(!stencil_ref, "Got unexpected stencil ref %u.\n", stencil_ref);
    ID3D10Device_OMGetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, tmp_rtv, &tmp_dsv);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(!tmp_rtv[i], "Got unexpected render target view %p in slot %u.\n", tmp_rtv[i], i);
    }
    ok(!tmp_dsv, "Got unexpected depth stencil view %p.\n", tmp_dsv);

    ID3D10Device_RSGetScissorRects(device, &count, NULL);
    todo_wine ok(!count, "Got unexpected scissor rect count %u.\n", count);
    memset(tmp_rect, 0x55, sizeof(tmp_rect));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetScissorRects(device, &count, tmp_rect);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        ok(!tmp_rect[i].left && !tmp_rect[i].top && !tmp_rect[i].right && !tmp_rect[i].bottom,
                "Got unexpected scissor rect %s in slot %u.\n", wine_dbgstr_rect(&tmp_rect[i]), i);
    }
    ID3D10Device_RSGetViewports(device, &count, NULL);
    todo_wine ok(!count, "Got unexpected viewport count %u.\n", count);
    memset(tmp_viewport, 0x55, sizeof(tmp_viewport));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetViewports(device, &count, tmp_viewport);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        ok(!tmp_viewport[i].TopLeftX && !tmp_viewport[i].TopLeftY && !tmp_viewport[i].Width
                && !tmp_viewport[i].Height && !tmp_viewport[i].MinDepth && !tmp_viewport[i].MaxDepth,
                "Got unexpected viewport {%d, %d, %u, %u, %.8e, %.8e} in slot %u.\n",
                tmp_viewport[i].TopLeftX, tmp_viewport[i].TopLeftY, tmp_viewport[i].Width,
                tmp_viewport[i].Height, tmp_viewport[i].MinDepth, tmp_viewport[i].MaxDepth, i);
    }
    ID3D10Device_RSGetState(device, &tmp_rs_state);
    ok(!tmp_rs_state, "Got unexpected rasterizer state %p.\n", tmp_rs_state);

    ID3D10Device_SOGetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, tmp_buffer, offset);
    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected stream output %p in slot %u.\n", tmp_buffer[i], i);
        ok(!offset[i], "Got unexpected stream output offset %u in slot %u.\n", offset[i], i);
    }

    ID3D10Device_GetPredication(device, &tmp_predicate, &predicate_value);
    ok(!tmp_predicate, "Got unexpected predicate %p.\n", tmp_predicate);
    ok(!predicate_value, "Got unexpected predicate value %#x.\n", predicate_value);

    /* Create resources. */

    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
        cb[i] = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, 1024, NULL);

    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        buffer[i] = create_buffer(device,
                D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_INDEX_BUFFER | D3D10_BIND_SHADER_RESOURCE,
                1024, NULL);

        stride[i] = (i + 1) * 4;
        offset[i] = (i + 1) * 16;
    }

    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
        so_buffer[i] = create_buffer(device, D3D10_BIND_STREAM_OUTPUT, 1024, NULL);

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
    U(srv_desc).Buffer.ElementOffset = 0;
    U(srv_desc).Buffer.ElementWidth = 64;

    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateShaderResourceView(device,
                (ID3D10Resource *)buffer[i % D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT], &srv_desc, &srv[i]);
        ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);
    }

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 16;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 16.0f;

    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        sampler_desc.MinLOD = (float)i;

        hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler[i]);
        ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);
    }

    hr = ID3D10Device_CreateVertexShader(device, simple_vs, sizeof(simple_vs), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreateGeometryShader(device, simple_gs, sizeof(simple_gs), &gs);
    ok(SUCCEEDED(hr), "Failed to create geometry shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, simple_ps, sizeof(simple_ps), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            simple_vs, sizeof(simple_vs), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.BlendEnable[0] = FALSE;
    blend_desc.BlendEnable[1] = FALSE;
    blend_desc.BlendEnable[2] = FALSE;
    blend_desc.BlendEnable[3] = FALSE;
    blend_desc.BlendEnable[4] = FALSE;
    blend_desc.BlendEnable[5] = FALSE;
    blend_desc.BlendEnable[6] = FALSE;
    blend_desc.BlendEnable[7] = FALSE;
    blend_desc.SrcBlend = D3D10_BLEND_ONE;
    blend_desc.DestBlend = D3D10_BLEND_ZERO;
    blend_desc.BlendOp = D3D10_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_ONE;
    blend_desc.DestBlendAlpha = D3D10_BLEND_ZERO;
    blend_desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[1] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[2] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[3] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[4] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[5] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[6] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[7] = D3D10_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    ds_desc.DepthEnable = TRUE;
    ds_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D10_COMPARISON_LESS;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
    ds_desc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
    ds_desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
    ds_desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;

    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#x.\n", hr);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &rt_texture[i]);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    }

    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &ds_texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)rt_texture[i], NULL, &rtv[i]);
        ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);
    }

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)ds_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depthstencil view, hr %#x.\n", hr);

    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        SetRect(&tmp_rect[i], i, i * 2, i + 1, (i + 1) * 2);

        tmp_viewport[i].TopLeftX = i * 3;
        tmp_viewport[i].TopLeftY = i * 4;
        tmp_viewport[i].Width = 3;
        tmp_viewport[i].Height = 4;
        tmp_viewport[i].MinDepth = i * 0.01f;
        tmp_viewport[i].MaxDepth = (i + 1) * 0.01f;
    }

    rs_desc.FillMode = D3D10_FILL_SOLID;
    rs_desc.CullMode = D3D10_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = FALSE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;

    hr = ID3D10Device_CreateRasterizerState(device, &rs_desc, &rs_state);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);

    predicate_desc.Query = D3D10_QUERY_OCCLUSION_PREDICATE;
    predicate_desc.MiscFlags = 0;

    hr = ID3D10Device_CreatePredicate(device, &predicate_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#x.\n", hr);

    /* Verify the behavior of set state methods. */

    blend_factor[0] = 0.1f;
    blend_factor[1] = 0.2f;
    blend_factor[2] = 0.3f;
    blend_factor[3] = 0.4f;
    ID3D10Device_OMSetBlendState(device, blend_state, blend_factor, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, tmp_blend_factor, &sample_mask);
    ok(tmp_blend_factor[0] == 0.1f && tmp_blend_factor[1] == 0.2f
            && tmp_blend_factor[2] == 0.3f && tmp_blend_factor[3] == 0.4f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            tmp_blend_factor[0], tmp_blend_factor[1], tmp_blend_factor[2], tmp_blend_factor[3]);
    ID3D10BlendState_Release(tmp_blend_state);

    ID3D10Device_OMSetBlendState(device, blend_state, NULL, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, tmp_blend_factor, &sample_mask);
    ok(tmp_blend_factor[0] == 1.0f && tmp_blend_factor[1] == 1.0f
            && tmp_blend_factor[2] == 1.0f && tmp_blend_factor[3] == 1.0f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            tmp_blend_factor[0], tmp_blend_factor[1], tmp_blend_factor[2], tmp_blend_factor[3]);
    ID3D10BlendState_Release(tmp_blend_state);

    /* Setup state. */

    ID3D10Device_VSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_VSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_VSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_VSSetShader(device, vs);

    ID3D10Device_GSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_GSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_GSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_GSSetShader(device, gs);

    ID3D10Device_PSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_PSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_PSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_IASetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, buffer, stride, offset);
    ID3D10Device_IASetIndexBuffer(device, buffer[0], DXGI_FORMAT_R32_UINT, offset[0]);
    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    blend_factor[0] = 0.1f;
    blend_factor[1] = 0.2f;
    blend_factor[2] = 0.3f;
    blend_factor[3] = 0.4f;
    ID3D10Device_OMSetBlendState(device, blend_state, blend_factor, 0xff00ff00);
    ID3D10Device_OMSetDepthStencilState(device, ds_state, 3);
    ID3D10Device_OMSetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, dsv);

    ID3D10Device_RSSetScissorRects(device, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, tmp_rect);
    ID3D10Device_RSSetViewports(device, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, tmp_viewport);
    ID3D10Device_RSSetState(device, rs_state);

    ID3D10Device_SOSetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, so_buffer, offset);

    ID3D10Device_SetPredication(device, predicate, TRUE);

    /* Verify the set state. */

    ID3D10Device_VSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(tmp_buffer[i] == cb[i], "Got unexpected constant buffer %p in slot %u, expected %p.\n",
                tmp_buffer[i], i, cb[i]);
        ID3D10Buffer_Release(tmp_buffer[i]);
    }
    ID3D10Device_VSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(tmp_srv[i] == srv[i], "Got unexpected shader resource view %p in slot %u, expected %p.\n",
                tmp_srv[i], i, srv[i]);
        ID3D10ShaderResourceView_Release(tmp_srv[i]);
    }
    ID3D10Device_VSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(tmp_sampler[i] == sampler[i], "Got unexpected sampler %p in slot %u, expected %p.\n",
                tmp_sampler[i], i, sampler[i]);
        ID3D10SamplerState_Release(tmp_sampler[i]);
    }
    ID3D10Device_VSGetShader(device, &tmp_vs);
    ok(tmp_vs == vs, "Got unexpected vertex shader %p, expected %p.\n", tmp_vs, vs);
    ID3D10VertexShader_Release(tmp_vs);

    ID3D10Device_GSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(tmp_buffer[i] == cb[i], "Got unexpected constant buffer %p in slot %u, expected %p.\n",
                tmp_buffer[i], i, cb[i]);
        ID3D10Buffer_Release(tmp_buffer[i]);
    }
    ID3D10Device_GSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(tmp_srv[i] == srv[i], "Got unexpected shader resource view %p in slot %u, expected %p.\n",
                tmp_srv[i], i, srv[i]);
        ID3D10ShaderResourceView_Release(tmp_srv[i]);
    }
    ID3D10Device_GSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(tmp_sampler[i] == sampler[i], "Got unexpected sampler %p in slot %u, expected %p.\n",
                tmp_sampler[i], i, sampler[i]);
        ID3D10SamplerState_Release(tmp_sampler[i]);
    }
    ID3D10Device_GSGetShader(device, &tmp_gs);
    ok(tmp_gs == gs, "Got unexpected geometry shader %p, expected %p.\n", tmp_gs, gs);
    ID3D10GeometryShader_Release(tmp_gs);

    ID3D10Device_PSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(tmp_buffer[i] == cb[i], "Got unexpected constant buffer %p in slot %u, expected %p.\n",
                tmp_buffer[i], i, cb[i]);
        ID3D10Buffer_Release(tmp_buffer[i]);
    }
    ID3D10Device_PSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(tmp_srv[i] == srv[i], "Got unexpected shader resource view %p in slot %u, expected %p.\n",
                tmp_srv[i], i, srv[i]);
        ID3D10ShaderResourceView_Release(tmp_srv[i]);
    }
    ID3D10Device_PSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(tmp_sampler[i] == sampler[i], "Got unexpected sampler %p in slot %u, expected %p.\n",
                tmp_sampler[i], i, sampler[i]);
        ID3D10SamplerState_Release(tmp_sampler[i]);
    }
    ID3D10Device_PSGetShader(device, &tmp_ps);
    ok(tmp_ps == ps, "Got unexpected pixel shader %p, expected %p.\n", tmp_ps, ps);
    ID3D10PixelShader_Release(tmp_ps);

    ID3D10Device_IAGetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, tmp_buffer, stride, offset);
    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(tmp_buffer[i] == buffer[i], "Got unexpected vertex buffer %p in slot %u, expected %p.\n",
                tmp_buffer[i], i, buffer[i]);
        ok(stride[i] == (i + 1) * 4, "Got unexpected stride %u in slot %u.\n", stride[i], i);
        ok(offset[i] == (i + 1) * 16, "Got unexpected offset %u in slot %u.\n", offset[i], i);
        ID3D10Buffer_Release(tmp_buffer[i]);
    }
    ID3D10Device_IAGetIndexBuffer(device, tmp_buffer, &format, offset);
    ok(tmp_buffer[0] == buffer[0], "Got unexpected index buffer %p, expected %p.\n", tmp_buffer[0], buffer[0]);
    ID3D10Buffer_Release(tmp_buffer[0]);
    ok(format == DXGI_FORMAT_R32_UINT, "Got unexpected index buffer format %#x.\n", format);
    ok(offset[0] == 16, "Got unexpected index buffer offset %u.\n", offset[0]);
    ID3D10Device_IAGetInputLayout(device, &tmp_input_layout);
    ok(tmp_input_layout == input_layout, "Got unexpected input layout %p, expected %p.\n",
            tmp_input_layout, input_layout);
    ID3D10InputLayout_Release(tmp_input_layout);
    ID3D10Device_IAGetPrimitiveTopology(device, &topology);
    ok(topology == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, "Got unexpected primitive topology %#x.\n", topology);

    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, blend_factor, &sample_mask);
    ok(tmp_blend_state == blend_state, "Got unexpected blend state %p, expected %p.\n", tmp_blend_state, blend_state);
    ID3D10BlendState_Release(tmp_blend_state);
    ok(blend_factor[0] == 0.1f && blend_factor[1] == 0.2f
            && blend_factor[2] == 0.3f && blend_factor[3] == 0.4f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            blend_factor[0], blend_factor[1], blend_factor[2], blend_factor[3]);
    ok(sample_mask == 0xff00ff00, "Got unexpected sample mask %#x.\n", sample_mask);
    ID3D10Device_OMGetDepthStencilState(device, &tmp_ds_state, &stencil_ref);
    ok(tmp_ds_state == ds_state, "Got unexpected depth stencil state %p, expected %p.\n", tmp_ds_state, ds_state);
    ID3D10DepthStencilState_Release(tmp_ds_state);
    ok(stencil_ref == 3, "Got unexpected stencil ref %u.\n", stencil_ref);
    ID3D10Device_OMGetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, tmp_rtv, &tmp_dsv);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(tmp_rtv[i] == rtv[i], "Got unexpected render target view %p in slot %u, expected %p.\n",
                tmp_rtv[i], i, rtv[i]);
        ID3D10RenderTargetView_Release(tmp_rtv[i]);
    }
    ok(tmp_dsv == dsv, "Got unexpected depth stencil view %p, expected %p.\n", tmp_dsv, dsv);
    ID3D10DepthStencilView_Release(tmp_dsv);

    ID3D10Device_RSGetScissorRects(device, &count, NULL);
    todo_wine ok(count == D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE,
            "Got unexpected scissor rect count %u.\n", count);
    memset(tmp_rect, 0x55, sizeof(tmp_rect));
    ID3D10Device_RSGetScissorRects(device, &count, tmp_rect);
    for (i = 0; i < count; ++i)
    {
        ok(tmp_rect[i].left == i
                && tmp_rect[i].top == i * 2
                && tmp_rect[i].right == i + 1
                && tmp_rect[i].bottom == (i + 1) * 2,
                "Got unexpected scissor rect %s in slot %u.\n", wine_dbgstr_rect(&tmp_rect[i]), i);
    }
    ID3D10Device_RSGetViewports(device, &count, NULL);
    todo_wine ok(count == D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE,
            "Got unexpected viewport count %u.\n", count);
    memset(tmp_viewport, 0x55, sizeof(tmp_viewport));
    ID3D10Device_RSGetViewports(device, &count, tmp_viewport);
    for (i = 0; i < count; ++i)
    {
        ok(tmp_viewport[i].TopLeftX == i * 3
                && tmp_viewport[i].TopLeftY == i * 4
                && tmp_viewport[i].Width == 3
                && tmp_viewport[i].Height == 4
                && compare_float(tmp_viewport[i].MinDepth, i * 0.01f, 16)
                && compare_float(tmp_viewport[i].MaxDepth, (i + 1) * 0.01f, 16),
                "Got unexpected viewport {%d, %d, %u, %u, %.8e, %.8e} in slot %u.\n",
                tmp_viewport[i].TopLeftX, tmp_viewport[i].TopLeftY, tmp_viewport[i].Width,
                tmp_viewport[i].Height, tmp_viewport[i].MinDepth, tmp_viewport[i].MaxDepth, i);
    }
    ID3D10Device_RSGetState(device, &tmp_rs_state);
    ok(tmp_rs_state == rs_state, "Got unexpected rasterizer state %p, expected %p.\n", tmp_rs_state, rs_state);
    ID3D10RasterizerState_Release(tmp_rs_state);

    ID3D10Device_SOGetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, tmp_buffer, offset);
    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ok(tmp_buffer[i] == so_buffer[i], "Got unexpected stream output %p in slot %u, expected %p.\n",
                tmp_buffer[i], i, so_buffer[i]);
        ID3D10Buffer_Release(tmp_buffer[i]);
        todo_wine ok(offset[i] == ~0u, "Got unexpected stream output offset %u in slot %u.\n", offset[i], i);
    }

    ID3D10Device_GetPredication(device, &tmp_predicate, &predicate_value);
    ok(tmp_predicate == predicate, "Got unexpected predicate %p, expected %p.\n", tmp_predicate, predicate);
    ID3D10Predicate_Release(tmp_predicate);
    ok(predicate_value, "Got unexpected predicate value %#x.\n", predicate_value);

    /* Verify ClearState(). */

    ID3D10Device_ClearState(device);

    ID3D10Device_VSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_VSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_VSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_VSGetShader(device, &tmp_vs);
    ok(!tmp_vs, "Got unexpected vertex shader %p.\n", tmp_vs);

    ID3D10Device_GSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_GSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_GSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_GSGetShader(device, &tmp_gs);
    ok(!tmp_gs, "Got unexpected geometry shader %p.\n", tmp_gs);

    ID3D10Device_PSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_PSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_PSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_PSGetShader(device, &tmp_ps);
    ok(!tmp_ps, "Got unexpected pixel shader %p.\n", tmp_ps);

    ID3D10Device_IAGetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, tmp_buffer, stride, offset);
    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected vertex buffer %p in slot %u.\n", tmp_buffer[i], i);
        todo_wine ok(!stride[i], "Got unexpected stride %u in slot %u.\n", stride[i], i);
        todo_wine ok(!offset[i], "Got unexpected offset %u in slot %u.\n", offset[i], i);
    }
    ID3D10Device_IAGetIndexBuffer(device, tmp_buffer, &format, offset);
    ok(!tmp_buffer[0], "Got unexpected index buffer %p.\n", tmp_buffer[0]);
    ok(format == DXGI_FORMAT_UNKNOWN, "Got unexpected index buffer format %#x.\n", format);
    ok(!offset[0], "Got unexpected index buffer offset %u.\n", offset[0]);
    ID3D10Device_IAGetInputLayout(device, &tmp_input_layout);
    ok(!tmp_input_layout, "Got unexpected input layout %p.\n", tmp_input_layout);
    ID3D10Device_IAGetPrimitiveTopology(device, &topology);
    ok(topology == D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED, "Got unexpected primitive topology %#x.\n", topology);

    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, blend_factor, &sample_mask);
    ok(!tmp_blend_state, "Got unexpected blend state %p.\n", tmp_blend_state);
    ok(blend_factor[0] == 1.0f && blend_factor[1] == 1.0f
            && blend_factor[2] == 1.0f && blend_factor[3] == 1.0f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            blend_factor[0], blend_factor[1], blend_factor[2], blend_factor[3]);
    ok(sample_mask == D3D10_DEFAULT_SAMPLE_MASK, "Got unexpected sample mask %#x.\n", sample_mask);
    ID3D10Device_OMGetDepthStencilState(device, &tmp_ds_state, &stencil_ref);
    ok(!tmp_ds_state, "Got unexpected depth stencil state %p.\n", tmp_ds_state);
    ok(!stencil_ref, "Got unexpected stencil ref %u.\n", stencil_ref);
    ID3D10Device_OMGetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, tmp_rtv, &tmp_dsv);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(!tmp_rtv[i], "Got unexpected render target view %p in slot %u.\n", tmp_rtv[i], i);
    }
    ok(!tmp_dsv, "Got unexpected depth stencil view %p.\n", tmp_dsv);

    ID3D10Device_RSGetScissorRects(device, &count, NULL);
    todo_wine ok(!count, "Got unexpected scissor rect count %u.\n", count);
    memset(tmp_rect, 0x55, sizeof(tmp_rect));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetScissorRects(device, &count, tmp_rect);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        todo_wine_if(!i)
            ok(!tmp_rect[i].left && !tmp_rect[i].top && !tmp_rect[i].right && !tmp_rect[i].bottom,
                    "Got unexpected scissor rect %s in slot %u.\n",
                    wine_dbgstr_rect(&tmp_rect[i]), i);
    }
    ID3D10Device_RSGetViewports(device, &count, NULL);
    todo_wine ok(!count, "Got unexpected viewport count %u.\n", count);
    memset(tmp_viewport, 0x55, sizeof(tmp_viewport));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetViewports(device, &count, tmp_viewport);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        todo_wine_if(!i)
            ok(!tmp_viewport[i].TopLeftX && !tmp_viewport[i].TopLeftY && !tmp_viewport[i].Width
                    && !tmp_viewport[i].Height && !tmp_viewport[i].MinDepth && !tmp_viewport[i].MaxDepth,
                    "Got unexpected viewport {%d, %d, %u, %u, %.8e, %.8e} in slot %u.\n",
                    tmp_viewport[i].TopLeftX, tmp_viewport[i].TopLeftY, tmp_viewport[i].Width,
                    tmp_viewport[i].Height, tmp_viewport[i].MinDepth, tmp_viewport[i].MaxDepth, i);
    }
    ID3D10Device_RSGetState(device, &tmp_rs_state);
    ok(!tmp_rs_state, "Got unexpected rasterizer state %p.\n", tmp_rs_state);

    ID3D10Device_SOGetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, tmp_buffer, offset);
    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected stream output %p in slot %u.\n", tmp_buffer[i], i);
        ok(!offset[i], "Got unexpected stream output offset %u in slot %u.\n", offset[i], i);
    }

    ID3D10Device_GetPredication(device, &tmp_predicate, &predicate_value);
    ok(!tmp_predicate, "Got unexpected predicate %p.\n", tmp_predicate);
    ok(!predicate_value, "Got unexpected predicate value %#x.\n", predicate_value);

    /* Cleanup. */

    ID3D10Predicate_Release(predicate);
    ID3D10RasterizerState_Release(rs_state);
    ID3D10DepthStencilView_Release(dsv);
    ID3D10Texture2D_Release(ds_texture);

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ID3D10RenderTargetView_Release(rtv[i]);
        ID3D10Texture2D_Release(rt_texture[i]);
    }

    ID3D10DepthStencilState_Release(ds_state);
    ID3D10BlendState_Release(blend_state);
    ID3D10InputLayout_Release(input_layout);
    ID3D10VertexShader_Release(vs);
    ID3D10GeometryShader_Release(gs);
    ID3D10PixelShader_Release(ps);

    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ID3D10SamplerState_Release(sampler[i]);
    }

    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ID3D10ShaderResourceView_Release(srv[i]);
    }

    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(so_buffer[i]);
    }

    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(buffer[i]);
    }

    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(cb[i]);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_blend(void)
{
    struct d3d10core_test_context test_context;
    ID3D10BlendState *src_blend, *dst_blend;
    ID3D10RenderTargetView *offscreen_rtv;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10InputLayout *input_layout;
    D3D10_BLEND_DESC blend_desc;
    unsigned int stride, offset;
    ID3D10Texture2D *offscreen;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    D3D10_VIEWPORT vp;
    ID3D10Buffer *vb;
    DWORD color;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        struct vs_out main(float4 position : POSITION, float4 color : COLOR)
        {
            struct vs_out o;

            o.position = position;
            o.color = color;

            return o;
        }
#endif
        0x43425844, 0x5c73b061, 0x5c71125f, 0x3f8b345f, 0xce04b9ab, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x0000007c, 0x000000d0, 0x4e475349, 0x00000048, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0x4c4f4300, 0xab00524f, 0x4e47534f,
        0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003, 0x00000000,
        0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x505f5653,
        0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000068, 0x00010040, 0x0000001a,
        0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067, 0x001020f2,
        0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2, 0x00000000,
        0x00101e46, 0x00000000, 0x05000036, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(struct vs_out i) : SV_TARGET
        {
            return i.color;
        }
#endif
        0x43425844, 0xe2087fa6, 0xa35fbd95, 0x8e585b3f, 0x67890f54, 0x00000001, 0x000000f4, 0x00000003,
        0x0000002c, 0x00000080, 0x000000b4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quads[] =
    {
        /* quad1 */
        {{-1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{-1.0f,  0.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f,  0.0f, 0.1f}, 0x4000ff00},
        /* quad2 */
        {{-1.0f,  0.0f, 0.1f}, 0xc0ff0000},
        {{-1.0f,  1.0f, 0.1f}, 0xc0ff0000},
        {{ 1.0f,  0.0f, 0.1f}, 0xc0ff0000},
        {{ 1.0f,  1.0f, 0.1f}, 0xc0ff0000},
    };
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const float blend_factor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quads), quads);
    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    memset(&blend_desc, 0, sizeof(blend_desc));
    blend_desc.BlendEnable[0] = TRUE;
    blend_desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
    blend_desc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOp = D3D10_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_SRC_ALPHA;
    blend_desc.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
    blend_desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &src_blend);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    blend_desc.SrcBlend = D3D10_BLEND_DEST_ALPHA;
    blend_desc.DestBlend = D3D10_BLEND_INV_DEST_ALPHA;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_DEST_ALPHA;
    blend_desc.DestBlendAlpha = D3D10_BLEND_INV_DEST_ALPHA;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &dst_blend);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);

    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quads);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);
    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

    ID3D10Device_OMSetBlendState(device, src_blend, blend_factor, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10Device_Draw(device, 4, 0);
    ID3D10Device_OMSetBlendState(device, dst_blend, blend_factor, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10Device_Draw(device, 4, 4);

    color = get_texture_color(test_context.backbuffer, 320, 360);
    ok(compare_color(color, 0x700040bf, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 120);
    ok(compare_color(color, 0xa080007f, 1), "Got unexpected color 0x%08x.\n", color);

    texture_desc.Width = 128;
    texture_desc.Height = 128;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    /* DXGI_FORMAT_B8G8R8X8_UNORM is not supported on all implementations. */
    if (FAILED(ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &offscreen)))
    {
        skip("DXGI_FORMAT_B8G8R8X8_UNORM not supported, skipping tests.\n");
        goto done;
    }

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)offscreen, NULL, &offscreen_rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);

    ID3D10Device_OMSetRenderTargets(device, 1, &offscreen_rtv, NULL);

    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = 128;
    vp.Height = 128;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D10Device_RSSetViewports(device, 1, &vp);

    ID3D10Device_ClearRenderTargetView(device, offscreen_rtv, red);

    ID3D10Device_OMSetBlendState(device, src_blend, blend_factor, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10Device_Draw(device, 4, 0);
    ID3D10Device_OMSetBlendState(device, dst_blend, blend_factor, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10Device_Draw(device, 4, 4);

    color = get_texture_color(offscreen, 64, 96) & 0x00ffffff;
    ok(compare_color(color, 0x00bf4000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(offscreen, 64, 32) & 0x00ffffff;
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10RenderTargetView_Release(offscreen_rtv);
    ID3D10Texture2D_Release(offscreen);
done:
    ID3D10BlendState_Release(dst_blend);
    ID3D10BlendState_Release(src_blend);
    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    ID3D10Buffer_Release(vb);
    ID3D10InputLayout_Release(input_layout);
    release_test_context(&test_context);
}

static void test_texture(void)
{
    struct shader
    {
        const DWORD *code;
        size_t size;
    };
    struct texture
    {
        UINT width;
        UINT height;
        UINT miplevel_count;
        UINT array_size;
        DXGI_FORMAT format;
        D3D10_SUBRESOURCE_DATA data[3];
    };

    struct d3d10core_test_context test_context;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    const struct texture *current_texture;
    D3D10_TEXTURE2D_DESC texture_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    const struct shader *current_ps;
    ID3D10ShaderResourceView *srv;
    ID3D10SamplerState *sampler;
    struct resource_readback rb;
    ID3D10Texture2D *texture;
    struct vec4 ps_constant;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    unsigned int i, x, y;
    ID3D10Buffer *cb;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_ld_code[] =
    {
#if 0
        Texture2D t;

        float miplevel;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 p;
            t.GetDimensions(miplevel, p.x, p.y, p.z);
            p.z = miplevel;
            p *= float3(position.x / 640.0f, position.y / 480.0f, 1.0f);
            return t.Load(int3(p));
        }
#endif
        0x43425844, 0xbdda6bdf, 0xc6ffcdf1, 0xa58596b3, 0x822383f0, 0x00000001, 0x000001ac, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000110, 0x00000040,
        0x00000044, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04001858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x0600001c, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000,
        0x0700003d, 0x001000f2, 0x00000000, 0x0010000a, 0x00000000, 0x00107e46, 0x00000000, 0x07000038,
        0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x00101046, 0x00000000, 0x06000036, 0x001000c2,
        0x00000000, 0x00208006, 0x00000000, 0x00000000, 0x0a000038, 0x001000f2, 0x00000000, 0x00100e46,
        0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x3f800000, 0x3f800000, 0x0500001b, 0x001000f2,
        0x00000000, 0x00100e46, 0x00000000, 0x0700002d, 0x001020f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_ld_sint8_code[] =
    {
#if 0
        Texture2D<int4> t;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 p, s;
            int4 c;

            p = float3(position.x / 640.0f, position.y / 480.0f, 0.0f);
            t.GetDimensions(0, s.x, s.y, s.z);
            p *= s;

            c = t.Load(int3(p));
            return (max(c / (float4)127, (float4)-1) + (float4)1) / 2.0f;
        }
#endif
        0x43425844, 0xb3d0b0fc, 0x0e486f4a, 0xf67eec12, 0xfb9dd52f, 0x00000001, 0x00000240, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000001a4, 0x00000040,
        0x00000069, 0x04001858, 0x00107000, 0x00000000, 0x00003333, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x0700003d, 0x001000f2,
        0x00000000, 0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x0a000038, 0x00100032, 0x00000001,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x08000036,
        0x001000c2, 0x00000001, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000038,
        0x001000f2, 0x00000000, 0x00100f46, 0x00000000, 0x00100e46, 0x00000001, 0x0500001b, 0x001000f2,
        0x00000000, 0x00100e46, 0x00000000, 0x0700002d, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x0500002b, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x0a000038,
        0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0x3c010204, 0x3c010204, 0x3c010204,
        0x3c010204, 0x0a000034, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0xbf800000,
        0xbf800000, 0xbf800000, 0xbf800000, 0x0a000000, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00004002, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x0a000038, 0x001020f2, 0x00000000,
        0x00100e46, 0x00000000, 0x00004002, 0x3f000000, 0x3f000000, 0x3f000000, 0x3f000000, 0x0100003e,
    };
    static const DWORD ps_ld_uint8_code[] =
    {
#if 0
        Texture2D<uint4> t;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 p, s;

            p = float3(position.x / 640.0f, position.y / 480.0f, 0.0f);
            t.GetDimensions(0, s.x, s.y, s.z);
            p *= s;

            return t.Load(int3(p)) / (float4)255;
        }
#endif
        0x43425844, 0xd09917eb, 0x4508a07e, 0xb0b7250a, 0x228c1f0e, 0x00000001, 0x000001c8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000012c, 0x00000040,
        0x0000004b, 0x04001858, 0x00107000, 0x00000000, 0x00004444, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x0700003d, 0x001000f2,
        0x00000000, 0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x0a000038, 0x00100032, 0x00000001,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x08000036,
        0x001000c2, 0x00000001, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000038,
        0x001000f2, 0x00000000, 0x00100f46, 0x00000000, 0x00100e46, 0x00000001, 0x0500001b, 0x001000f2,
        0x00000000, 0x00100e46, 0x00000000, 0x0700002d, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x05000056, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x0a000038,
        0x001020f2, 0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0x3b808081, 0x3b808081, 0x3b808081,
        0x3b808081, 0x0100003e,
    };
    static const DWORD ps_sample_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_sample_b_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float bias;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.SampleBias(s, p, bias);
        }
#endif
        0x43425844, 0xc39b0686, 0x8244a7fc, 0x14c0b97a, 0x2900b3b7, 0x00000001, 0x00000150, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x000000b4, 0x00000040,
        0x0000002d, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0c00004a,
        0x001020f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_sample_l_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float level;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.SampleLevel(s, p, level);
        }
#endif
        0x43425844, 0x61e05d85, 0x2a7300fb, 0x0a83706b, 0x889d1683, 0x00000001, 0x00000150, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x000000b4, 0x00000040,
        0x0000002d, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0c000048,
        0x001020f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_sample_2d_array_code[] =
    {
#if 0
        Texture2DArray t;
        SamplerState s;

        float layer;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float3 d;
            float3 p = float3(position.x / 640.0f, position.y / 480.0f, 1.0f);
            t.GetDimensions(d.x, d.y, d.z);
            d.z = layer;
            return t.Sample(s, p * d);
        }
#endif
        0x43425844, 0xa9457e44, 0xc0b3ef8e, 0x3d751ae8, 0x23fa4807, 0x00000001, 0x00000194, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000000f8, 0x00000040,
        0x0000003e, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300005a, 0x00106000, 0x00000000,
        0x04004058, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0700003d, 0x001000f2, 0x00000000,
        0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x0a000038, 0x001000c2, 0x00000000, 0x00101406,
        0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x3acccccd, 0x3b088889, 0x07000038, 0x00100032,
        0x00000000, 0x00100046, 0x00000000, 0x00100ae6, 0x00000000, 0x06000036, 0x00100042, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100246, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const struct shader ps_ld = {ps_ld_code, sizeof(ps_ld_code)};
    static const struct shader ps_ld_sint8 = {ps_ld_sint8_code, sizeof(ps_ld_sint8_code)};
    static const struct shader ps_ld_uint8 = {ps_ld_uint8_code, sizeof(ps_ld_uint8_code)};
    static const struct shader ps_sample = {ps_sample_code, sizeof(ps_sample_code)};
    static const struct shader ps_sample_b = {ps_sample_b_code, sizeof(ps_sample_b_code)};
    static const struct shader ps_sample_l = {ps_sample_l_code, sizeof(ps_sample_l_code)};
    static const struct shader ps_sample_2d_array = {ps_sample_2d_array_code, sizeof(ps_sample_2d_array_code)};
    static const DWORD red_data[] =
    {
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0x00000000, 0x00000000,
    };
    static const DWORD green_data[] =
    {
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
        0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00,
    };
    static const DWORD blue_data[] =
    {
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0x00000000,
    };
    static const DWORD rgba_level_0[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD rgba_level_1[] =
    {
        0xffffffff, 0xff0000ff,
        0xff000000, 0xff00ff00,
    };
    static const DWORD rgba_level_2[] =
    {
        0xffff0000,
    };
    static const DWORD srgb_data[] =
    {
        0x00000000, 0xffffffff, 0xff000000, 0x7f7f7f7f,
        0xff010203, 0xff102030, 0xff0a0b0c, 0xff8090a0,
        0xffb1c4de, 0xfff0f1f2, 0xfffafdfe, 0xff5a560f,
        0xffd5ff00, 0xffc8f99f, 0xffaa00aa, 0xffdd55bb,
    };
    static const BYTE a8_data[] =
    {
        0x00, 0x10, 0x20, 0x30,
        0x40, 0x50, 0x60, 0x70,
        0x80, 0x90, 0xa0, 0xb0,
        0xc0, 0xd0, 0xe0, 0xf0,
    };
    static const BYTE bc1_data[] =
    {
        0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    };
    static const BYTE bc2_data[] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    };
    static const BYTE bc3_data[] =
    {
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    };
    static const BYTE bc4_data[] =
    {
        0x10, 0x7f, 0x77, 0x39, 0x05, 0x00, 0x00, 0x00,
        0x10, 0x7f, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x10, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x10, 0x7f, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb,
    };
    static const BYTE bc5_data[] =
    {
        0x10, 0x7f, 0x77, 0x39, 0x05, 0x00, 0x00, 0x00, 0x10, 0x7f, 0x77, 0x39, 0x05, 0x00, 0x00, 0x00,
        0x10, 0x7f, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x10, 0x7f, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x10, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x10, 0x7f, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb, 0x10, 0x7f, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb,
    };
    static const struct texture rgba_texture =
    {
        4, 4, 3, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
        {
            {rgba_level_0, 4 * sizeof(*rgba_level_0), 0},
            {rgba_level_1, 2 * sizeof(*rgba_level_1), 0},
            {rgba_level_2,     sizeof(*rgba_level_2), 0},
        }
    };
    static const struct texture srgb_texture = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            {{srgb_data, 4 * sizeof(*srgb_data)}}};
    static const struct texture srgb_typeless = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_TYPELESS,
            {{srgb_data, 4 * sizeof(*srgb_data)}}};
    static const struct texture a8_texture = {4, 4, 1, 1, DXGI_FORMAT_A8_UNORM,
            {{a8_data, 4 * sizeof(*a8_data)}}};
    static const struct texture bc1_texture = {8, 8, 1, 1, DXGI_FORMAT_BC1_UNORM, {{bc1_data, 2 * 8}}};
    static const struct texture bc2_texture = {8, 8, 1, 1, DXGI_FORMAT_BC2_UNORM, {{bc2_data, 2 * 16}}};
    static const struct texture bc3_texture = {8, 8, 1, 1, DXGI_FORMAT_BC3_UNORM, {{bc3_data, 2 * 16}}};
    static const struct texture bc4_texture = {8, 8, 1, 1, DXGI_FORMAT_BC4_UNORM, {{bc4_data, 2 * 8}}};
    static const struct texture bc5_texture = {8, 8, 1, 1, DXGI_FORMAT_BC5_UNORM, {{bc5_data, 2 * 16}}};
    static const struct texture bc1_texture_srgb = {8, 8, 1, 1, DXGI_FORMAT_BC1_UNORM_SRGB, {{bc1_data, 2 * 8}}};
    static const struct texture bc2_texture_srgb = {8, 8, 1, 1, DXGI_FORMAT_BC2_UNORM_SRGB, {{bc2_data, 2 * 16}}};
    static const struct texture bc3_texture_srgb = {8, 8, 1, 1, DXGI_FORMAT_BC3_UNORM_SRGB, {{bc3_data, 2 * 16}}};
    static const struct texture bc1_typeless = {8, 8, 1, 1, DXGI_FORMAT_BC1_TYPELESS, {{bc1_data, 2 * 8}}};
    static const struct texture bc2_typeless = {8, 8, 1, 1, DXGI_FORMAT_BC2_TYPELESS, {{bc2_data, 2 * 16}}};
    static const struct texture bc3_typeless = {8, 8, 1, 1, DXGI_FORMAT_BC3_TYPELESS, {{bc3_data, 2 * 16}}};
    static const struct texture sint8_texture = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_SINT,
            {{rgba_level_0, 4 * sizeof(*rgba_level_0)}}};
    static const struct texture uint8_texture = {4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_UINT,
            {{rgba_level_0, 4 * sizeof(*rgba_level_0)}}};
    static const struct texture array_2d_texture =
    {
        4, 4, 1, 3, DXGI_FORMAT_R8G8B8A8_UNORM,
        {
            {red_data,   6 * sizeof(*red_data)},
            {green_data, 4 * sizeof(*green_data)},
            {blue_data,  5 * sizeof(*blue_data)},
        }
    };
    static const DWORD red_colors[] =
    {
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
        0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
    };
    static const DWORD blue_colors[] =
    {
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
    };
    static const DWORD level_1_colors[] =
    {
        0xffffffff, 0xffffffff, 0xff0000ff, 0xff0000ff,
        0xffffffff, 0xffffffff, 0xff0000ff, 0xff0000ff,
        0xff000000, 0xff000000, 0xff00ff00, 0xff00ff00,
        0xff000000, 0xff000000, 0xff00ff00, 0xff00ff00,
    };
    static const DWORD lerp_1_2_colors[] =
    {
        0xffff7f7f, 0xffff7f7f, 0xff7f007f, 0xff7f007f,
        0xffff7f7f, 0xffff7f7f, 0xff7f007f, 0xff7f007f,
        0xff7f0000, 0xff7f0000, 0xff7f7f00, 0xff7f7f00,
        0xff7f0000, 0xff7f0000, 0xff7f7f00, 0xff7f7f00,
    };
    static const DWORD level_2_colors[] =
    {
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
    };
    static const DWORD srgb_colors[] =
    {
        0x00000001, 0xffffffff, 0xff000000, 0x7f363636,
        0xff000000, 0xff010408, 0xff010101, 0xff37475a,
        0xff708cba, 0xffdee0e2, 0xfff3fbfd, 0xff1a1801,
        0xffa9ff00, 0xff93f159, 0xff670067, 0xffb8177f,
    };
    static const DWORD a8_colors[] =
    {
        0x00000000, 0x10000000, 0x20000000, 0x30000000,
        0x40000000, 0x50000000, 0x60000000, 0x70000000,
        0x80000000, 0x90000000, 0xa0000000, 0xb0000000,
        0xc0000000, 0xd0000000, 0xe0000000, 0xf0000000,
    };
    static const DWORD bc_colors[] =
    {
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xff00ff00,
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xff00ff00,
        0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff,
        0xffff0000, 0xffff0000, 0xffffffff, 0xffffffff,
    };
    static const DWORD bc4_colors[] =
    {
        0xff000026, 0xff000010, 0xff00007f, 0xff00007f,
        0xff000010, 0xff000010, 0xff00007f, 0xff00007f,
        0xff0000ff, 0xff0000ff, 0xff000000, 0xff000000,
        0xff0000ff, 0xff0000ff, 0xff000000, 0xff000000,
    };
    static const DWORD bc5_colors[] =
    {
        0xff002626, 0xff001010, 0xff007f7f, 0xff007f7f,
        0xff001010, 0xff001010, 0xff007f7f, 0xff007f7f,
        0xff00ffff, 0xff00ffff, 0xff000000, 0xff000000,
        0xff00ffff, 0xff00ffff, 0xff000000, 0xff000000,
    };
    static const DWORD sint8_colors[] =
    {
        0x7e80807e, 0x7e807e7e, 0x7e807e80, 0x7e7e7e80,
        0x7e7e8080, 0x7e7e7f7f, 0x7e808080, 0x7effffff,
        0x7e7e7e7e, 0x7e7e7e7e, 0x7e7e7e7e, 0x7e808080,
        0x7e7e7e7e, 0x7e7f7f7f, 0x7e7f7f7f, 0x7e7f7f7f,
    };
    static const DWORD zero_colors[4 * 4] = {0};
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    static const struct texture_test
    {
        const struct shader *ps;
        const struct texture *texture;
        D3D10_FILTER filter;
        float lod_bias;
        float min_lod;
        float max_lod;
        float ps_constant;
        const DWORD *expected_colors;
    }
    texture_tests[] =
    {
#define POINT        D3D10_FILTER_MIN_MAG_MIP_POINT
#define POINT_LINEAR D3D10_FILTER_MIN_MAG_POINT_MIP_LINEAR
#define MIP_MAX      D3D10_FLOAT32_MAX
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, rgba_level_0},
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  1.0f, level_1_colors},
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  2.0f, level_2_colors},
        {&ps_ld,              &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  3.0f, zero_colors},
        {&ps_ld,              &srgb_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, srgb_colors},
        {&ps_ld,              &bc1_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc1_texture,      POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_ld,              &bc2_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc2_texture,      POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_ld,              &bc3_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc3_texture,      POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_ld,              &bc4_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc4_colors},
        {&ps_ld,              &bc5_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc5_colors},
        {&ps_ld,              &bc1_texture_srgb, POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc2_texture_srgb, POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              &bc3_texture_srgb, POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_ld,              NULL,              POINT,        0.0f, 0.0f,    0.0f,  0.0f, zero_colors},
        {&ps_ld,              NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, zero_colors},
        {&ps_ld_sint8,        &sint8_texture,    POINT,        0.0f, 0.0f,    0.0f,  0.0f, sint8_colors},
        {&ps_ld_uint8,        &uint8_texture,    POINT,        0.0f, 0.0f,    0.0f,  0.0f, rgba_level_0},
        {&ps_sample,          &bc1_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_sample,          &bc2_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_sample,          &bc3_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc_colors},
        {&ps_sample,          &bc4_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc4_colors},
        {&ps_sample,          &bc5_texture,      POINT,        0.0f, 0.0f,    0.0f,  0.0f, bc5_colors},
        {&ps_sample,          &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, rgba_level_0},
        {&ps_sample,          &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample,          &rgba_texture,     POINT,        2.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample,          &rgba_texture,     POINT,        8.0f, 0.0f, MIP_MAX,  0.0f, level_1_colors},
        {&ps_sample,          &srgb_texture,     POINT,        0.0f, 0.0f,    0.0f,  0.0f, srgb_colors},
        {&ps_sample,          &a8_texture,       POINT,        0.0f, 0.0f,    0.0f,  0.0f, a8_colors},
        {&ps_sample,          NULL,              POINT,        0.0f, 0.0f,    0.0f,  0.0f, zero_colors},
        {&ps_sample,          NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, zero_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample_b,        &rgba_texture,     POINT,        8.0f, 0.0f, MIP_MAX,  0.0f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  8.0f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  8.4f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  8.5f, level_2_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  9.0f, level_2_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    2.0f,  1.0f, rgba_level_0},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    2.0f,  9.0f, level_2_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    1.0f,  9.0f, level_1_colors},
        {&ps_sample_b,        &rgba_texture,     POINT,        0.0f, 0.0f,    0.0f,  9.0f, rgba_level_0},
        {&ps_sample_b,        NULL,              POINT,        0.0f, 0.0f,    0.0f,  0.0f, zero_colors},
        {&ps_sample_b,        NULL,              POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_sample_b,        NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, zero_colors},
        {&ps_sample_b,        NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  1.0f, zero_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX, -1.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.4f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  0.5f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  1.0f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  1.4f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  1.5f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  2.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  3.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        0.0f, 0.0f, MIP_MAX,  4.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 0.0f, 0.0f, MIP_MAX,  1.5f, lerp_1_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX, -2.0f, rgba_level_0},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX, -1.0f, level_1_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX,  0.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX,  1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 0.0f, MIP_MAX,  1.5f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f, -9.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f, -1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f,  0.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f,  1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT_LINEAR, 2.0f, 2.0f,    2.0f,  9.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f, -9.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f, -1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f,  0.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f,  1.0f, level_2_colors},
        {&ps_sample_l,        &rgba_texture,     POINT,        2.0f, 2.0f,    2.0f,  9.0f, level_2_colors},
        {&ps_sample_l,        NULL,              POINT,        2.0f, 2.0f,    0.0f,  0.0f, zero_colors},
        {&ps_sample_l,        NULL,              POINT,        2.0f, 2.0f,    0.0f,  1.0f, zero_colors},
        {&ps_sample_l,        NULL,              POINT,        2.0f, 2.0f, MIP_MAX,  0.0f, zero_colors},
        {&ps_sample_l,        NULL,              POINT,        2.0f, 2.0f, MIP_MAX,  1.0f, zero_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX, -9.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX, -1.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  0.4f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  0.5f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  1.0f, green_data},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  1.4f, green_data},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  2.0f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  2.1f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  3.0f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  3.1f, blue_colors},
        {&ps_sample_2d_array, &array_2d_texture, POINT,        0.0f, 0.0f, MIP_MAX,  9.0f, blue_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f,    0.0f,  1.0f, zero_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f,    0.0f,  2.0f, zero_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f,    0.0f,  0.0f, zero_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  0.0f, zero_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  1.0f, zero_colors},
        {&ps_sample_2d_array, NULL,              POINT,        0.0f, 0.0f, MIP_MAX,  2.0f, zero_colors},
#undef POINT
#undef POINT_LINEAR
#undef MIP_MAX
    };
    static const struct srv_test
    {
        const struct shader *ps;
        const struct texture *texture;
        struct srv_desc srv_desc;
        float ps_constant;
        const DWORD *expected_colors;
    }
    srv_tests[] =
    {
#define TEX_2D              D3D10_SRV_DIMENSION_TEXTURE2D
#define TEX_2D_ARRAY        D3D10_SRV_DIMENSION_TEXTURE2DARRAY
#define BC1_UNORM           DXGI_FORMAT_BC1_UNORM
#define BC1_UNORM_SRGB      DXGI_FORMAT_BC1_UNORM_SRGB
#define BC2_UNORM           DXGI_FORMAT_BC2_UNORM
#define BC2_UNORM_SRGB      DXGI_FORMAT_BC2_UNORM_SRGB
#define BC3_UNORM           DXGI_FORMAT_BC3_UNORM
#define BC3_UNORM_SRGB      DXGI_FORMAT_BC3_UNORM_SRGB
#define R8G8B8A8_UNORM_SRGB DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
#define R8G8B8A8_UNORM      DXGI_FORMAT_R8G8B8A8_UNORM
#define FMT_UNKNOWN         DXGI_FORMAT_UNKNOWN
        {&ps_sample,          &bc1_typeless,     {BC1_UNORM,           TEX_2D,       0, 1},       0.0f, bc_colors},
        {&ps_sample,          &bc1_typeless,     {BC1_UNORM_SRGB,      TEX_2D,       0, 1},       0.0f, bc_colors},
        {&ps_sample,          &bc2_typeless,     {BC2_UNORM,           TEX_2D,       0, 1},       0.0f, bc_colors},
        {&ps_sample,          &bc2_typeless,     {BC2_UNORM_SRGB,      TEX_2D,       0, 1},       0.0f, bc_colors},
        {&ps_sample,          &bc3_typeless,     {BC3_UNORM,           TEX_2D,       0, 1},       0.0f, bc_colors},
        {&ps_sample,          &bc3_typeless,     {BC3_UNORM_SRGB,      TEX_2D,       0, 1},       0.0f, bc_colors},
        {&ps_sample,          &srgb_typeless,    {R8G8B8A8_UNORM_SRGB, TEX_2D,       0, 1},       0.0f, srgb_colors},
        {&ps_sample,          &srgb_typeless,    {R8G8B8A8_UNORM,      TEX_2D,       0, 1},       0.0f, srgb_data},
        {&ps_sample,          &array_2d_texture, {FMT_UNKNOWN,         TEX_2D,       0, 1},       0.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, {FMT_UNKNOWN,         TEX_2D_ARRAY, 0, 1, 0, 1}, 0.0f, red_colors},
        {&ps_sample_2d_array, &array_2d_texture, {FMT_UNKNOWN,         TEX_2D_ARRAY, 0, 1, 1, 1}, 0.0f, green_data},
        {&ps_sample_2d_array, &array_2d_texture, {FMT_UNKNOWN,         TEX_2D_ARRAY, 0, 1, 2, 1}, 0.0f, blue_colors},
#undef TEX_2D
#undef TEX_2D_ARRAY
#undef BC1_UNORM
#undef BC1_UNORM_SRGB
#undef BC2_UNORM
#undef BC2_UNORM_SRGB
#undef BC3_UNORM
#undef BC3_UNORM_SRGB
#undef R8G8B8A8_UNORM_SRGB
#undef R8G8B8A8_UNORM
#undef FMT_UNKNOWN
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(ps_constant), NULL);

    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D10_FLOAT32_MAX;

    ps = NULL;
    srv = NULL;
    sampler = NULL;
    texture = NULL;
    current_ps = NULL;
    current_texture = NULL;
    for (i = 0; i < sizeof(texture_tests) / sizeof(*texture_tests); ++i)
    {
        const struct texture_test *test = &texture_tests[i];

        if (current_ps != test->ps)
        {
            if (ps)
                ID3D10PixelShader_Release(ps);

            current_ps = test->ps;

            hr = ID3D10Device_CreatePixelShader(device, current_ps->code, current_ps->size, &ps);
            ok(SUCCEEDED(hr), "Test %u: Failed to create pixel shader, hr %#x.\n", i, hr);

            ID3D10Device_PSSetShader(device, ps);
        }

        if (current_texture != test->texture)
        {
            if (texture)
                ID3D10Texture2D_Release(texture);
            if (srv)
                ID3D10ShaderResourceView_Release(srv);

            current_texture = test->texture;

            if (current_texture)
            {
                texture_desc.Width = current_texture->width;
                texture_desc.Height = current_texture->height;
                texture_desc.MipLevels = current_texture->miplevel_count;
                texture_desc.ArraySize = current_texture->array_size;
                texture_desc.Format = current_texture->format;

                hr = ID3D10Device_CreateTexture2D(device, &texture_desc, current_texture->data, &texture);
                ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);

                hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, NULL, &srv);
                ok(SUCCEEDED(hr), "Test %u: Failed to create shader resource view, hr %#x.\n", i, hr);
            }
            else
            {
                texture = NULL;
                srv = NULL;
            }

            ID3D10Device_PSSetShaderResources(device, 0, 1, &srv);
        }

        if (!sampler || (sampler_desc.Filter != test->filter
                || sampler_desc.MipLODBias != test->lod_bias
                || sampler_desc.MinLOD != test->min_lod
                || sampler_desc.MaxLOD != test->max_lod))
        {
            if (sampler)
                ID3D10SamplerState_Release(sampler);

            sampler_desc.Filter = test->filter;
            sampler_desc.MipLODBias = test->lod_bias;
            sampler_desc.MinLOD = test->min_lod;
            sampler_desc.MaxLOD = test->max_lod;

            hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler);
            ok(SUCCEEDED(hr), "Test %u: Failed to create sampler state, hr %#x.\n", i, hr);

            ID3D10Device_PSSetSamplers(device, 0, 1, &sampler);
        }

        ps_constant.x = test->ps_constant;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &ps_constant, 0, 0);

        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

        draw_quad(&test_context);

        get_texture_readback(test_context.backbuffer, 0, &rb);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                color = get_readback_color(&rb, 80 + x * 160, 60 + y * 120);
                ok(compare_color(color, test->expected_colors[y * 4 + x], 1),
                        "Test %u: Got unexpected color 0x%08x at (%u, %u).\n", i, color, x, y);
            }
        }
        release_resource_readback(&rb);
    }
    if (srv)
        ID3D10ShaderResourceView_Release(srv);
    ID3D10SamplerState_Release(sampler);
    if (texture)
        ID3D10Texture2D_Release(texture);
    ID3D10PixelShader_Release(ps);

    if (is_warp_device(device) && !is_d3d11_interface_available(device))
    {
        win_skip("SRV tests are broken on WARP.\n");
        ID3D10Buffer_Release(cb);
        release_test_context(&test_context);
        return;
    }

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D10_FLOAT32_MAX;

    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    ID3D10Device_PSSetSamplers(device, 0, 1, &sampler);

    ps = NULL;
    srv = NULL;
    texture = NULL;
    current_ps = NULL;
    current_texture = NULL;
    for (i = 0; i < sizeof(srv_tests) / sizeof(*srv_tests); ++i)
    {
        const struct srv_test *test = &srv_tests[i];

        if (current_ps != test->ps)
        {
            if (ps)
                ID3D10PixelShader_Release(ps);

            current_ps = test->ps;

            hr = ID3D10Device_CreatePixelShader(device, current_ps->code, current_ps->size, &ps);
            ok(SUCCEEDED(hr), "Test %u: Failed to create pixel shader, hr %#x.\n", i, hr);

            ID3D10Device_PSSetShader(device, ps);
        }

        if (current_texture != test->texture)
        {
            if (texture)
                ID3D10Texture2D_Release(texture);

            current_texture = test->texture;

            texture_desc.Width = current_texture->width;
            texture_desc.Height = current_texture->height;
            texture_desc.MipLevels = current_texture->miplevel_count;
            texture_desc.ArraySize = current_texture->array_size;
            texture_desc.Format = current_texture->format;

            hr = ID3D10Device_CreateTexture2D(device, &texture_desc, current_texture->data, &texture);
            ok(SUCCEEDED(hr), "Test %u: Failed to create 2d texture, hr %#x.\n", i, hr);
        }

        if (srv)
            ID3D10ShaderResourceView_Release(srv);

        get_srv_desc(&srv_desc, &test->srv_desc);
        hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, &srv_desc, &srv);
        ok(SUCCEEDED(hr), "Test %u: Failed to create shader resource view, hr %#x.\n", i, hr);

        ID3D10Device_PSSetShaderResources(device, 0, 1, &srv);

        ps_constant.x = test->ps_constant;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &ps_constant, 0, 0);

        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

        draw_quad(&test_context);

        get_texture_readback(test_context.backbuffer, 0, &rb);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                color = get_readback_color(&rb, 80 + x * 160, 60 + y * 120);
                ok(compare_color(color, test->expected_colors[y * 4 + x], 1),
                        "Test %u: Got unexpected color 0x%08x at (%u, %u).\n", i, color, x, y);
            }
        }
        release_resource_readback(&rb);
    }
    ID3D10PixelShader_Release(ps);
    ID3D10Texture2D_Release(texture);
    ID3D10ShaderResourceView_Release(srv);
    ID3D10SamplerState_Release(sampler);

    ID3D10Buffer_Release(cb);
    release_test_context(&test_context);
}

static void test_depth_stencil_sampling(void)
{
    ID3D10PixelShader *ps_cmp, *ps_depth, *ps_stencil, *ps_depth_stencil;
    ID3D10ShaderResourceView *depth_srv, *stencil_srv;
    struct d3d10core_test_context test_context;
    ID3D10SamplerState *cmp_sampler, *sampler;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    ID3D10Texture2D *texture, *rt_texture;
    D3D10_TEXTURE2D_DESC texture_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10DepthStencilView *dsv;
    ID3D10RenderTargetView *rtv;
    struct vec4 ps_constant;
    ID3D10Device *device;
    ID3D10Buffer *cb;
    unsigned int i;
    HRESULT hr;

    static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    static const DWORD ps_compare_code[] =
    {
#if 0
        Texture2D t;
        SamplerComparisonState s;

        float ref;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            return t.SampleCmp(s, float2(position.x / 640.0f, position.y / 480.0f), ref);
        }
#endif
        0x43425844, 0xc2e0d84e, 0x0522c395, 0x9ff41580, 0xd3ca29cc, 0x00000001, 0x00000164, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x000000c8, 0x00000040,
        0x00000032, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x0300085a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032, 0x00000000,
        0x00101046, 0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0c000046,
        0x00100012, 0x00000000, 0x00100046, 0x00000000, 0x00107006, 0x00000000, 0x00106000, 0x00000000,
        0x0020800a, 0x00000000, 0x00000000, 0x05000036, 0x001020f2, 0x00000000, 0x00100006, 0x00000000,
        0x0100003e,
    };
    static const DWORD ps_sample_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            return t.Sample(s, float2(position.x / 640.0f, position.y / 480.0f));
        }
#endif
        0x43425844, 0x7472c092, 0x5548f00e, 0xf4e007f1, 0x5970429c, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_stencil_code[] =
    {
#if 0
        Texture2D<uint4> t;

        float4 main(float4 position : SV_Position) : SV_Target
        {
            float2 s;
            t.GetDimensions(s.x, s.y);
            return t.Load(int3(float3(s.x * position.x / 640.0f, s.y * position.y / 480.0f, 0))).y;
        }
#endif
        0x43425844, 0x929fced8, 0x2cd93320, 0x0591ece3, 0xee50d04a, 0x00000001, 0x000001a0, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000104, 0x00000040,
        0x00000041, 0x04001858, 0x00107000, 0x00000000, 0x00004444, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0700003d, 0x001000f2,
        0x00000000, 0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x07000038, 0x00100032, 0x00000000,
        0x00100046, 0x00000000, 0x00101046, 0x00000000, 0x0a000038, 0x00100032, 0x00000000, 0x00100046,
        0x00000000, 0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0500001b, 0x00100032,
        0x00000000, 0x00100046, 0x00000000, 0x08000036, 0x001000c2, 0x00000000, 0x00004002, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x0700002d, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
        0x00107e46, 0x00000000, 0x05000056, 0x001020f2, 0x00000000, 0x00100556, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_depth_stencil_code[] =
    {
#if 0
        SamplerState samp;
        Texture2D depth_tex;
        Texture2D<uint4> stencil_tex;

        float main(float4 position: SV_Position) : SV_Target
        {
            float2 s, p;
            float depth, stencil;
            depth_tex.GetDimensions(s.x, s.y);
            p = float2(s.x * position.x / 640.0f, s.y * position.y / 480.0f);
            depth = depth_tex.Sample(samp, p).r;
            stencil = stencil_tex.Load(int3(float3(p.x, p.y, 0))).y;
            return depth + stencil;
        }
#endif
        0x43425844, 0x348f8377, 0x977d1ee0, 0x8cca4f35, 0xff5c5afc, 0x00000001, 0x000001fc, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x00000e01, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000160, 0x00000040,
        0x00000058, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04001858, 0x00107000, 0x00000001, 0x00004444, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x00102012, 0x00000000, 0x02000068, 0x00000002, 0x0700003d, 0x001000f2, 0x00000000,
        0x00004001, 0x00000000, 0x00107e46, 0x00000000, 0x07000038, 0x00100032, 0x00000000, 0x00100046,
        0x00000000, 0x00101046, 0x00000000, 0x0a000038, 0x00100032, 0x00000000, 0x00100046, 0x00000000,
        0x00004002, 0x3acccccd, 0x3b088889, 0x00000000, 0x00000000, 0x0500001b, 0x00100032, 0x00000001,
        0x00100046, 0x00000000, 0x09000045, 0x001000f2, 0x00000000, 0x00100046, 0x00000000, 0x00107e46,
        0x00000000, 0x00106000, 0x00000000, 0x08000036, 0x001000c2, 0x00000001, 0x00004002, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x0700002d, 0x001000f2, 0x00000001, 0x00100e46, 0x00000001,
        0x00107e46, 0x00000001, 0x05000056, 0x00100022, 0x00000000, 0x0010001a, 0x00000001, 0x07000000,
        0x00102012, 0x00000000, 0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x0100003e,
    };
    static const struct test
    {
        DXGI_FORMAT typeless_format;
        DXGI_FORMAT dsv_format;
        DXGI_FORMAT depth_view_format;
        DXGI_FORMAT stencil_view_format;
    }
    tests[] =
    {
        {DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
                DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_X32_TYPELESS_G8X24_UINT},
        {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT,
                DXGI_FORMAT_R32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT,
                DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_X24_TYPELESS_G8_UINT},
        {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM,
                DXGI_FORMAT_R16_UNORM},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    if (is_amd_device(device))
    {
        /* Reads from depth/stencil shader resource views return stale values on some AMD drivers. */
        win_skip("Some AMD drivers have a bug affecting the test.\n");
        release_test_context(&test_context);
        return;
    }

    sampler_desc.Filter = D3D10_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_GREATER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 0.0f;
    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &cmp_sampler);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &rt_texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)rt_texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);

    memset(&ps_constant, 0, sizeof(ps_constant));
    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(ps_constant), &ps_constant);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    hr = ID3D10Device_CreatePixelShader(device, ps_compare_code, sizeof(ps_compare_code), &ps_cmp);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_sample_code, sizeof(ps_sample_code), &ps_depth);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_stencil_code, sizeof(ps_stencil_code), &ps_stencil);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_depth_stencil_code, sizeof(ps_depth_stencil_code),
            &ps_depth_stencil);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        texture_desc.Format = tests[i].typeless_format;
        texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_DEPTH_STENCIL;
        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
        ok(SUCCEEDED(hr), "Failed to create texture for format %#x, hr %#x.\n",
                texture_desc.Format, hr);

        dsv_desc.Format = tests[i].dsv_format;
        dsv_desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
        U(dsv_desc).Texture2D.MipSlice = 0;
        hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, &dsv_desc, &dsv);
        ok(SUCCEEDED(hr), "Failed to create depth stencil view for format %#x, hr %#x.\n",
                dsv_desc.Format, hr);

        srv_desc.Format = tests[i].depth_view_format;
        srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        U(srv_desc).Texture2D.MostDetailedMip = 0;
        U(srv_desc).Texture2D.MipLevels = 1;
        hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, &srv_desc, &depth_srv);
        ok(SUCCEEDED(hr), "Failed to create depth shader resource view for format %#x, hr %#x.\n",
                srv_desc.Format, hr);

        ID3D10Device_PSSetShader(device, ps_cmp);
        ID3D10Device_PSSetShaderResources(device, 0, 1, &depth_srv);
        ID3D10Device_PSSetSamplers(device, 0, 1, &cmp_sampler);

        ps_constant.x = 0.5f;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0,
                NULL, &ps_constant, 0, 0);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 0.0f, 2);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 0.0f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 1.0f, 2);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 0.5f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 0.0f, 2);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 0.6f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 0.0f, 2);

        ps_constant.x = 0.7f;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0,
                NULL, &ps_constant, 0, 0);

        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 1.0f, 2);

        ID3D10Device_PSSetShader(device, ps_depth);
        ID3D10Device_PSSetSamplers(device, 0, 1, &sampler);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 1.0f, 2);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 0.2f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 0.2f, 2);

        if (!tests[i].stencil_view_format)
        {
            ID3D10DepthStencilView_Release(dsv);
            ID3D10ShaderResourceView_Release(depth_srv);
            ID3D10Texture2D_Release(texture);
            continue;
        }

        srv_desc.Format = tests[i].stencil_view_format;
        hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, &srv_desc, &stencil_srv);
        if (hr == E_OUTOFMEMORY)
        {
            skip("Could not create SRV for format %#x.\n", srv_desc.Format);
            ID3D10DepthStencilView_Release(dsv);
            ID3D10ShaderResourceView_Release(depth_srv);
            ID3D10Texture2D_Release(texture);
            continue;
        }
        ok(SUCCEEDED(hr), "Failed to create stencil shader resource view for format %#x, hr %#x.\n",
                srv_desc.Format, hr);

        ID3D10Device_PSSetShader(device, ps_stencil);
        ID3D10Device_PSSetShaderResources(device, 0, 1, &stencil_srv);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_STENCIL, 0.0f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 0.0f, 0);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_STENCIL, 0.0f, 100);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 100.0f, 0);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_STENCIL, 0.0f, 255);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 255.0f, 0);

        ID3D10Device_PSSetShader(device, ps_depth_stencil);
        ID3D10Device_PSSetShaderResources(device, 0, 1, &depth_srv);
        ID3D10Device_PSSetShaderResources(device, 1, 1, &stencil_srv);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 0.3f, 3);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 3.3f, 2);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 3);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 4.0f, 2);

        ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 0.0f, 0);
        ID3D10Device_ClearRenderTargetView(device, rtv, black);
        draw_quad(&test_context);
        check_texture_float(rt_texture, 0.0f, 2);

        ID3D10DepthStencilView_Release(dsv);
        ID3D10ShaderResourceView_Release(depth_srv);
        ID3D10ShaderResourceView_Release(stencil_srv);
        ID3D10Texture2D_Release(texture);
    }

    ID3D10Buffer_Release(cb);
    ID3D10PixelShader_Release(ps_cmp);
    ID3D10PixelShader_Release(ps_depth);
    ID3D10PixelShader_Release(ps_depth_stencil);
    ID3D10PixelShader_Release(ps_stencil);
    ID3D10RenderTargetView_Release(rtv);
    ID3D10SamplerState_Release(cmp_sampler);
    ID3D10SamplerState_Release(sampler);
    ID3D10Texture2D_Release(rt_texture);
    release_test_context(&test_context);
}

static void test_multiple_render_targets(void)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10InputLayout *input_layout;
    unsigned int stride, offset, i;
    ID3D10RenderTargetView *rtv[4];
    ID3D10Texture2D *rt[4];
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    D3D10_VIEWPORT vp;
    ID3D10Buffer *vb;
    ULONG refcount;
    HRESULT hr;

    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct output
        {
            float4 t1 : SV_TARGET0;
            float4 t2 : SV_Target1;
            float4 t3 : SV_TARGET2;
            float4 t4 : SV_Target3;
        };

        output main(float4 position : SV_POSITION)
        {
            struct output o;
            o.t1 = (float4)1.0f;
            o.t2 = (float4)0.5f;
            o.t3 = (float4)0.2f;
            o.t4 = float4(0.0f, 0.2f, 0.5f, 1.0f);
            return o;
        }
#endif
        0x43425844, 0x8701ad18, 0xe3d5291d, 0x7b4288a6, 0x01917515, 0x00000001, 0x000001a8, 0x00000003,
        0x0000002c, 0x00000060, 0x000000e4, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000007c, 0x00000004, 0x00000008, 0x00000068, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x00000072, 0x00000001, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x00000068, 0x00000002, 0x00000000, 0x00000003, 0x00000002, 0x0000000f, 0x00000072, 0x00000003,
        0x00000000, 0x00000003, 0x00000003, 0x0000000f, 0x545f5653, 0x45475241, 0x56530054, 0x7261545f,
        0x00746567, 0x52444853, 0x000000bc, 0x00000040, 0x0000002f, 0x03000065, 0x001020f2, 0x00000000,
        0x03000065, 0x001020f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000002, 0x03000065, 0x001020f2,
        0x00000003, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x3f800000, 0x3f800000, 0x3f800000,
        0x3f800000, 0x08000036, 0x001020f2, 0x00000001, 0x00004002, 0x3f000000, 0x3f000000, 0x3f000000,
        0x3f000000, 0x08000036, 0x001020f2, 0x00000002, 0x00004002, 0x3e4ccccd, 0x3e4ccccd, 0x3e4ccccd,
        0x3e4ccccd, 0x08000036, 0x001020f2, 0x00000003, 0x00004002, 0x00000000, 0x3e4ccccd, 0x3f000000,
        0x3f800000, 0x0100003e,
    };
    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    for (i = 0; i < sizeof(rt) / sizeof(*rt); ++i)
    {
        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &rt[i]);
        ok(SUCCEEDED(hr), "Failed to create texture %u, hr %#x.\n", i, hr);

        hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)rt[i], NULL, &rtv[i]);
        ok(SUCCEEDED(hr), "Failed to create rendertarget view %u, hr %#x.\n", i, hr);
    }

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_OMSetRenderTargets(device, 4, rtv, NULL);
    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);
    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_PSSetShader(device, ps);

    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D10Device_RSSetViewports(device, 1, &vp);

    for (i = 0; i < sizeof(rtv) / sizeof(*rtv); ++i)
        ID3D10Device_ClearRenderTargetView(device, rtv[i], red);

    ID3D10Device_Draw(device, 4, 0);

    check_texture_color(rt[0], 0xffffffff, 2);
    check_texture_color(rt[1], 0x7f7f7f7f, 2);
    check_texture_color(rt[2], 0x33333333, 2);
    check_texture_color(rt[3], 0xff7f3300, 2);

    ID3D10Buffer_Release(vb);
    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    ID3D10InputLayout_Release(input_layout);
    for (i = 0; i < sizeof(rtv) / sizeof(*rtv); ++i)
        ID3D10RenderTargetView_Release(rtv[i]);
    for (i = 0; i < sizeof(rt) / sizeof(*rt); ++i)
        ID3D10Texture2D_Release(rt[i]);
    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_private_data(void)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D11Texture2D *d3d11_texture;
    ID3D11Device *d3d11_device;
    ID3D10Device *test_object;
    ID3D10Texture2D *texture;
    IDXGIDevice *dxgi_device;
    IDXGISurface *surface;
    ID3D10Device *device;
    IUnknown *ptr;
    HRESULT hr;
    UINT size;

    static const GUID test_guid =
            {0xfdb37466, 0x428f, 0x4edf, {0xa3, 0x7f, 0x9b, 0x1d, 0xf4, 0x88, 0xc5, 0xfc}};
    static const GUID test_guid2 =
            {0x2e5afac2, 0x87b5, 0x4c10, {0x9b, 0x4b, 0x89, 0xd7, 0xd1, 0x12, 0xe7, 0x2b}};
    static const DWORD data[] = {1, 2, 3, 4};

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    test_object = create_device();

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get IDXGISurface, hr %#x.\n", hr);

    /* SetPrivateData() with a pointer of NULL has the purpose of
     * FreePrivateData() in previous D3D versions. A successful clear returns
     * S_OK. A redundant clear S_FALSE. Setting a NULL interface is not
     * considered a clear but as setting an interface pointer that happens to
     * be NULL. */
    hr = ID3D10Device_SetPrivateData(device, &test_guid, 0, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_SetPrivateData(device, &test_guid, ~0u, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_SetPrivateData(device, &test_guid, ~0u, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = ID3D10Device_GetPrivateData(device, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);
    size = sizeof(ptr) * 2;
    ptr = (IUnknown *)0xdeadbeef;
    hr = IDXGIDevice_GetPrivateData(dxgi_device, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(!ptr, "Got unexpected pointer %p.\n", ptr);
    ok(size == sizeof(IUnknown *), "Got unexpected size %u.\n", size);
    IDXGIDevice_Release(dxgi_device);

    refcount = get_refcount((IUnknown *)test_object);
    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    --expected_refcount;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(data);
    hr = ID3D10Device_SetPrivateData(device, &test_guid, size, data);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = ID3D10Device_SetPrivateData(device, &test_guid, 42, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_SetPrivateData(device, &test_guid, 42, NULL);
    ok(hr == S_FALSE, "Got unexpected hr %#x.\n", hr);

    hr = ID3D10Device_SetPrivateDataInterface(device, &test_guid,
            (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ++expected_refcount;
    size = 2 * sizeof(ptr);
    ptr = NULL;
    hr = ID3D10Device_GetPrivateData(device, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(test_object), "Got unexpected size %u.\n", size);
    ++expected_refcount;
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    IUnknown_Release(ptr);
    --expected_refcount;

    hr = ID3D10Device_QueryInterface(device, &IID_ID3D11Device, (void **)&d3d11_device);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Device should implement ID3D11Device.\n");
    if (SUCCEEDED(hr))
    {
        ptr = NULL;
        size = sizeof(ptr);
        hr = ID3D11Device_GetPrivateData(d3d11_device, &test_guid, &size, &ptr);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(ptr == (IUnknown *)test_object, "Got unexpected ptr %p, expected %p.\n", ptr, test_object);
        IUnknown_Release(ptr);
        ID3D11Device_Release(d3d11_device);
        refcount = get_refcount((IUnknown *)test_object);
        ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n",
                refcount, expected_refcount);
    }

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = ID3D10Device_GetPrivateData(device, &test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = ID3D10Device_GetPrivateData(device, &test_guid, &size, NULL);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount((IUnknown *)test_object);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    size = 1;
    hr = ID3D10Device_GetPrivateData(device, &test_guid, &size, &ptr);
    ok(hr == DXGI_ERROR_MORE_DATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = ID3D10Device_GetPrivateData(device, &test_guid2, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = ID3D10Device_GetPrivateData(device, &test_guid2, &size, &ptr);
    ok(hr == DXGI_ERROR_NOT_FOUND, "Got unexpected hr %#x.\n", hr);
    ok(size == 0, "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = ID3D10Device_GetPrivateData(device, &test_guid, NULL, &ptr);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);

    hr = ID3D10Texture2D_SetPrivateDataInterface(texture, &test_guid, (IUnknown *)test_object);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ptr = NULL;
    size = sizeof(ptr);
    hr = IDXGISurface_GetPrivateData(surface, &test_guid, &size, &ptr);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)test_object, "Got unexpected ptr %p, expected %p.\n", ptr, test_object);
    IUnknown_Release(ptr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_ID3D11Texture2D, (void **)&d3d11_texture);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Not available on all Windows versions. */,
            "Texture should implement ID3D11Texture2D.\n");
    if (SUCCEEDED(hr))
    {
        ptr = NULL;
        size = sizeof(ptr);
        hr = ID3D11Texture2D_GetPrivateData(d3d11_texture, &test_guid, &size, &ptr);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ok(ptr == (IUnknown *)test_object, "Got unexpected ptr %p, expected %p.\n", ptr, test_object);
        IUnknown_Release(ptr);
        ID3D11Texture2D_Release(d3d11_texture);
    }

    IDXGISurface_Release(surface);
    ID3D10Texture2D_Release(texture);
    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = ID3D10Device_Release(test_object);
    ok(!refcount, "Test object has %u references left.\n", refcount);
}

static void test_il_append_aligned(void)
{
    struct d3d10core_test_context test_context;
    ID3D10InputLayout *input_layout;
    unsigned int stride, offset;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    ID3D10Buffer *vb[3];
    DWORD color;
    HRESULT hr;

    /* Semantic names are case-insensitive. */
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"CoLoR",    2, DXGI_FORMAT_R32G32_FLOAT,       1, D3D10_APPEND_ALIGNED_ELEMENT,
                D3D10_INPUT_PER_INSTANCE_DATA, 2},
        {"ColoR",    3, DXGI_FORMAT_R32G32_FLOAT,       2, D3D10_APPEND_ALIGNED_ELEMENT,
                D3D10_INPUT_PER_INSTANCE_DATA, 1},
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,
                D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"ColoR",    0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D10_APPEND_ALIGNED_ELEMENT,
                D3D10_INPUT_PER_INSTANCE_DATA, 1},
        {"cOLOr",    1, DXGI_FORMAT_R32G32_FLOAT,       1, D3D10_APPEND_ALIGNED_ELEMENT,
                D3D10_INPUT_PER_INSTANCE_DATA, 2},
    };
    static const DWORD vs_code[] =
    {
#if 0
        struct vs_in
        {
            float4 position : POSITION;
            float2 color_xy : COLOR0;
            float2 color_zw : COLOR1;
            unsigned int instance_id : SV_INSTANCEID;
        };

        struct vs_out
        {
            float4 position : SV_POSITION;
            float2 color_xy : COLOR0;
            float2 color_zw : COLOR1;
        };

        struct vs_out main(struct vs_in i)
        {
            struct vs_out o;

            o.position = i.position;
            o.position.x += i.instance_id * 0.5;
            o.color_xy = i.color_xy;
            o.color_zw = i.color_zw;

            return o;
        }
#endif
        0x43425844, 0x52e3bf46, 0x6300403d, 0x624cffe4, 0xa4fc0013, 0x00000001, 0x00000214, 0x00000003,
        0x0000002c, 0x000000bc, 0x00000128, 0x4e475349, 0x00000088, 0x00000004, 0x00000008, 0x00000068,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000071, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000303, 0x00000071, 0x00000001, 0x00000000, 0x00000003, 0x00000002,
        0x00000303, 0x00000077, 0x00000000, 0x00000008, 0x00000001, 0x00000003, 0x00000101, 0x49534f50,
        0x4e4f4954, 0x4c4f4300, 0x5300524f, 0x4e495f56, 0x4e415453, 0x44494543, 0xababab00, 0x4e47534f,
        0x00000064, 0x00000003, 0x00000008, 0x00000050, 0x00000000, 0x00000001, 0x00000003, 0x00000000,
        0x0000000f, 0x0000005c, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x00000c03, 0x0000005c,
        0x00000001, 0x00000000, 0x00000003, 0x00000001, 0x0000030c, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4f4c4f43, 0xabab0052, 0x52444853, 0x000000e4, 0x00010040, 0x00000039, 0x0300005f, 0x001010f2,
        0x00000000, 0x0300005f, 0x00101032, 0x00000001, 0x0300005f, 0x00101032, 0x00000002, 0x04000060,
        0x00101012, 0x00000003, 0x00000008, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x03000065,
        0x00102032, 0x00000001, 0x03000065, 0x001020c2, 0x00000001, 0x02000068, 0x00000001, 0x05000056,
        0x00100012, 0x00000000, 0x0010100a, 0x00000003, 0x09000032, 0x00102012, 0x00000000, 0x0010000a,
        0x00000000, 0x00004001, 0x3f000000, 0x0010100a, 0x00000000, 0x05000036, 0x001020e2, 0x00000000,
        0x00101e56, 0x00000000, 0x05000036, 0x00102032, 0x00000001, 0x00101046, 0x00000001, 0x05000036,
        0x001020c2, 0x00000001, 0x00101406, 0x00000002, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_POSITION;
            float2 color_xy : COLOR0;
            float2 color_zw : COLOR1;
        };

        float4 main(struct vs_out i) : SV_TARGET
        {
            return float4(i.color_xy.xy, i.color_zw.xy);
        }
#endif
        0x43425844, 0x64e48a09, 0xaa484d46, 0xe40a6e78, 0x9885edf3, 0x00000001, 0x00000118, 0x00000003,
        0x0000002c, 0x00000098, 0x000000cc, 0x4e475349, 0x00000064, 0x00000003, 0x00000008, 0x00000050,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x0000005c, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000303, 0x0000005c, 0x00000001, 0x00000000, 0x00000003, 0x00000001,
        0x00000c0c, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f,
        0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000044, 0x00000040, 0x00000011, 0x03001062,
        0x00101032, 0x00000001, 0x03001062, 0x001010c2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const struct
    {
        struct vec4 position;
    }
    stream0[] =
    {
        {{-1.0f, -1.0f, 0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}},
        {{-0.5f, -1.0f, 0.0f, 1.0f}},
        {{-0.5f,  1.0f, 0.0f, 1.0f}},
    };
    static const struct
    {
        struct vec2 color2;
        struct vec2 color1;
    }
    stream1[] =
    {
        {{0.5f, 0.5f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f}, {1.0f, 1.0f}},
    };
    static const struct
    {
        struct vec2 color3;
        struct vec2 color0;
    }
    stream2[] =
    {
        {{0.5f, 0.5f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {1.0f, 0.0f}},
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb[0] = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(stream0), stream0);
    vb[1] = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(stream1), stream1);
    vb[2] = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(stream2), stream2);

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    offset = 0;
    stride = sizeof(*stream0);
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb[0], &stride, &offset);
    stride = sizeof(*stream1);
    ID3D10Device_IASetVertexBuffers(device, 1, 1, &vb[1], &stride, &offset);
    stride = sizeof(*stream2);
    ID3D10Device_IASetVertexBuffers(device, 2, 1, &vb[2], &stride, &offset);
    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

    ID3D10Device_DrawInstanced(device, 4, 4, 0, 0);

    color = get_texture_color(test_context.backbuffer,  80, 240);
    ok(compare_color(color, 0xff0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 240, 240);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 400, 240);
    ok(compare_color(color, 0xffff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 560, 240);
    ok(compare_color(color, 0xffff00ff, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    ID3D10Buffer_Release(vb[2]);
    ID3D10Buffer_Release(vb[1]);
    ID3D10Buffer_Release(vb[0]);
    ID3D10InputLayout_Release(input_layout);
    release_test_context(&test_context);
}

static void test_fragment_coords(void)
{
    struct d3d10core_test_context test_context;
    ID3D10PixelShader *ps, *ps_frac;
    ID3D10Device *device;
    ID3D10Buffer *ps_cb;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        float2 cutoff;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float4 ret = float4(0.0, 0.0, 0.0, 1.0);

            if (position.x > cutoff.x)
                ret.y = 1.0;
            if (position.y > cutoff.y)
                ret.z = 1.0;

            return ret;
        }
#endif
        0x43425844, 0x49fc9e51, 0x8068867d, 0xf20cfa39, 0xb8099e6b, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000000a8, 0x00000040,
        0x0000002a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x08000031, 0x00100032,
        0x00000000, 0x00208046, 0x00000000, 0x00000000, 0x00101046, 0x00000000, 0x0a000001, 0x00102062,
        0x00000000, 0x00100106, 0x00000000, 0x00004002, 0x00000000, 0x3f800000, 0x3f800000, 0x00000000,
        0x08000036, 0x00102092, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
        0x0100003e,
    };
    static const DWORD ps_frac_code[] =
    {
#if 0
        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            return float4(frac(position.xy), 0.0, 1.0);
        }
#endif
        0x43425844, 0x86d9d78a, 0x190b72c2, 0x50841fd6, 0xdc24022e, 0x00000001, 0x000000f8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000005c, 0x00000040,
        0x00000017, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x0500001a, 0x00102032, 0x00000000, 0x00101046, 0x00000000, 0x08000036, 0x001020c2, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    struct vec4 cutoff = {320.0f, 240.0f, 0.0f, 0.0f};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    ps_cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(cutoff), &cutoff);

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_frac_code, sizeof(ps_frac_code), &ps_frac);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &ps_cb);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

    draw_quad(&test_context);

    color = get_texture_color(test_context.backbuffer, 319, 239);
    ok(compare_color(color, 0xff000000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 239);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 319, 240);
    ok(compare_color(color, 0xffff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 320, 240);
    ok(compare_color(color, 0xffffff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10Buffer_Release(ps_cb);
    cutoff.x = 16.0f;
    cutoff.y = 16.0f;
    ps_cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(cutoff), &cutoff);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &ps_cb);

    draw_quad(&test_context);

    color = get_texture_color(test_context.backbuffer, 14, 14);
    ok(compare_color(color, 0xff000000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 18, 14);
    ok(compare_color(color, 0xff00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 14, 18);
    ok(compare_color(color, 0xffff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(test_context.backbuffer, 18, 18);
    ok(compare_color(color, 0xffffff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10Device_PSSetShader(device, ps_frac);
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

    draw_quad(&test_context);

    color = get_texture_color(test_context.backbuffer, 14, 14);
    ok(compare_color(color, 0xff008080, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10Buffer_Release(ps_cb);
    ID3D10PixelShader_Release(ps_frac);
    ID3D10PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_update_subresource(void)
{
    struct d3d10core_test_context test_context;
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10SamplerState *sampler_state;
    ID3D10ShaderResourceView *ps_srv;
    D3D10_SAMPLER_DESC sampler_desc;
    struct resource_readback rb;
    ID3D10Texture2D *texture;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    unsigned int i, j;
    D3D10_BOX box;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    static const DWORD initial_data[16] = {0};
    static const DWORD bitmap_data[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD expected_colors[] =
    {
        0xffffffff, 0xff000000, 0xffffffff, 0xff000000,
        0xff00ff00, 0xff0000ff, 0xff00ffff, 0x00000000,
        0xffffff00, 0xffff0000, 0xffff00ff, 0x00000000,
        0xff000000, 0xff7f7f7f, 0xffffffff, 0x00000000,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    texture_desc.Width = 4;
    texture_desc.Height = 4;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    resource_data.pSysMem = initial_data;
    resource_data.SysMemPitch = texture_desc.Width * sizeof(*initial_data);
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, &resource_data, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, NULL, &ps_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x\n", hr);

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 0.0f;

    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler_state);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_PSSetShaderResources(device, 0, 1, &ps_srv);
    ID3D10Device_PSSetSamplers(device, 0, 1, &sampler_state);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
    check_texture_color(test_context.backbuffer, 0x7f0000ff, 1);

    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 1, 1, 0, 3, 3, 1);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, &box,
            bitmap_data, 4 * sizeof(*bitmap_data), 0);
    set_box(&box, 0, 3, 0, 3, 4, 1);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, &box,
            &bitmap_data[6], 4 * sizeof(*bitmap_data), 0);
    set_box(&box, 0, 0, 0, 4, 1, 1);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, &box,
            &bitmap_data[10], 4 * sizeof(*bitmap_data), 0);
    set_box(&box, 0, 1, 0, 1, 3, 1);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, &box,
            &bitmap_data[2], sizeof(*bitmap_data), 0);
    set_box(&box, 4, 4, 0, 3, 1, 1);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, &box,
            bitmap_data, sizeof(*bitmap_data), 0);
    set_box(&box, 0, 0, 0, 4, 4, 0);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, &box,
            bitmap_data, 4 * sizeof(*bitmap_data), 0);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, expected_colors[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, expected_colors[j + i * 4]);
        }
    }
    release_resource_readback(&rb);

    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)texture, 0, NULL,
            bitmap_data, 4 * sizeof(*bitmap_data), 0);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, bitmap_data[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, bitmap_data[j + i * 4]);
        }
    }
    release_resource_readback(&rb);

    ID3D10PixelShader_Release(ps);
    ID3D10SamplerState_Release(sampler_state);
    ID3D10ShaderResourceView_Release(ps_srv);
    ID3D10Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_copy_subresource_region(void)
{
    struct d3d10core_test_context test_context;
    ID3D10Texture2D *dst_texture, *src_texture;
    ID3D10Buffer *dst_buffer, *src_buffer;
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10SamplerState *sampler_state;
    ID3D10ShaderResourceView *ps_srv;
    D3D10_SAMPLER_DESC sampler_desc;
    struct vec4 float_colors[16];
    struct resource_readback rb;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    unsigned int i, j;
    D3D10_BOX box;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_buffer_code[] =
    {
#if 0
        float4 buffer[16];

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float2 p = (float2)4;
            p *= float2(position.x / 640.0f, position.y / 480.0f);
            return buffer[(int)p.y * 4 + (int)p.x];
        }
#endif
        0x43425844, 0x57e7139f, 0x4f0c9e52, 0x598b77e3, 0x5a239132, 0x00000001, 0x0000016c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x000000d0, 0x00000040,
        0x00000034, 0x04000859, 0x00208e46, 0x00000000, 0x00000010, 0x04002064, 0x00101032, 0x00000000,
        0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x0a000038, 0x00100032,
        0x00000000, 0x00101516, 0x00000000, 0x00004002, 0x3c088889, 0x3bcccccd, 0x00000000, 0x00000000,
        0x0500001b, 0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x07000029, 0x00100012, 0x00000000,
        0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x0700001e, 0x00100012, 0x00000000, 0x0010000a,
        0x00000000, 0x0010001a, 0x00000000, 0x07000036, 0x001020f2, 0x00000000, 0x04208e46, 0x00000000,
        0x0010000a, 0x00000000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    static const DWORD initial_data[16] = {0};
    static const DWORD bitmap_data[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD expected_colors[] =
    {
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
        0xffffff00, 0xff0000ff, 0xff00ffff, 0x00000000,
        0xff7f7f7f, 0xffff0000, 0xffff00ff, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xff000000, 0x00000000,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    texture_desc.Width = 4;
    texture_desc.Height = 4;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    resource_data.pSysMem = initial_data;
    resource_data.SysMemPitch = texture_desc.Width * sizeof(*initial_data);
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, &resource_data, &dst_texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    texture_desc.Usage = D3D10_USAGE_IMMUTABLE;

    resource_data.pSysMem = bitmap_data;
    resource_data.SysMemPitch = texture_desc.Width * sizeof(*bitmap_data);
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, &resource_data, &src_texture);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)dst_texture, NULL, &ps_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 0.0f;

    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler_state);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_PSSetShaderResources(device, 0, 1, &ps_srv);
    ID3D10Device_PSSetSamplers(device, 0, 1, &sampler_state);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);

    set_box(&box, 0, 0, 0, 2, 2, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            1, 1, 0, (ID3D10Resource *)src_texture, 0, &box);
    set_box(&box, 1, 2, 0, 4, 3, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            0, 3, 0, (ID3D10Resource *)src_texture, 0, &box);
    set_box(&box, 0, 3, 0, 4, 4, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            0, 0, 0, (ID3D10Resource *)src_texture, 0, &box);
    set_box(&box, 3, 0, 0, 4, 2, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            0, 1, 0, (ID3D10Resource *)src_texture, 0, &box);
    set_box(&box, 3, 1, 0, 4, 2, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            3, 2, 0, (ID3D10Resource *)src_texture, 0, &box);
    set_box(&box, 0, 0, 0, 4, 4, 0);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            0, 0, 0, (ID3D10Resource *)src_texture, 0, &box);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, expected_colors[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, expected_colors[j + i * 4]);
        }
    }
    release_resource_readback(&rb);

    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_texture, 0,
            0, 0, 0, (ID3D10Resource *)src_texture, 0, NULL);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, bitmap_data[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, bitmap_data[j + i * 4]);
        }
    }
    release_resource_readback(&rb);

    ID3D10PixelShader_Release(ps);
    hr = ID3D10Device_CreatePixelShader(device, ps_buffer_code, sizeof(ps_buffer_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10ShaderResourceView_Release(ps_srv);
    ps_srv = NULL;

    ID3D10SamplerState_Release(sampler_state);
    sampler_state = NULL;

    ID3D10Texture2D_Release(dst_texture);
    ID3D10Texture2D_Release(src_texture);

    ID3D10Device_PSSetShaderResources(device, 0, 1, &ps_srv);
    ID3D10Device_PSSetSamplers(device, 0, 1, &sampler_state);
    ID3D10Device_PSSetShader(device, ps);

    memset(float_colors, 0, sizeof(float_colors));
    dst_buffer = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(float_colors), float_colors);

    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &dst_buffer);

    src_buffer = create_buffer(device, 0, 256 * sizeof(*float_colors), NULL);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            float_colors[j + i * 4].x = ((bitmap_data[j + i * 4] >>  0) & 0xff) / 255.0f;
            float_colors[j + i * 4].y = ((bitmap_data[j + i * 4] >>  8) & 0xff) / 255.0f;
            float_colors[j + i * 4].z = ((bitmap_data[j + i * 4] >> 16) & 0xff) / 255.0f;
            float_colors[j + i * 4].w = ((bitmap_data[j + i * 4] >> 24) & 0xff) / 255.0f;
        }
    }
    set_box(&box, 0, 0, 0, sizeof(float_colors), 1, 1);
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)src_buffer, 0, &box, float_colors, 0, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 0, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D10Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 1, 0);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D10Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 0, 0);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D10Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0x00000000, 0);

    set_box(&box, 0, 0, 0, sizeof(float_colors), 1, 1);
    ID3D10Device_CopySubresourceRegion(device, (ID3D10Resource *)dst_buffer, 0,
            0, 0, 0, (ID3D10Resource *)src_buffer, 0, &box);
    draw_quad(&test_context);
    get_texture_readback(test_context.backbuffer, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            color = get_readback_color(&rb, 80 + j * 160, 60 + i * 120);
            ok(compare_color(color, bitmap_data[j + i * 4], 1),
                    "Got unexpected color 0x%08x at (%u, %u), expected 0x%08x.\n",
                    color, j, i, bitmap_data[j + i * 4]);
        }
    }
    release_resource_readback(&rb);

    ID3D10Buffer_Release(dst_buffer);
    ID3D10Buffer_Release(src_buffer);
    ID3D10PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_check_multisample_quality_levels(void)
{
    ID3D10Device *device;
    UINT quality_levels;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &quality_levels);
    if (!quality_levels)
    {
        skip("Multisampling not supported for DXGI_FORMAT_R8G8B8A8_UNORM, skipping test.\n");
        goto done;
    }

    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_UNKNOWN, 2, &quality_levels);
    todo_wine ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);
    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, 65536, 2, &quality_levels);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    todo_wine ok(quality_levels == 0xdeadbeef, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 0, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &quality_levels);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 1, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels == 1, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 2, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    /* We assume 15 samples multisampling is never supported in practice. */
    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 15, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 32, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 33, &quality_levels);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);
    quality_levels = 0xdeadbeef;
    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_R8G8B8A8_UNORM, 64, &quality_levels);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    hr = ID3D10Device_CheckMultisampleQualityLevels(device, DXGI_FORMAT_BC3_UNORM, 2, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(!quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

done:
    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_cb_relative_addressing(void)
{
    struct d3d10core_test_context test_context;
    ID3D10Buffer *vb, *colors_cb, *index_cb;
    ID3D10InputLayout *input_layout;
    unsigned int i, index[4] = {0};
    unsigned int stride, offset;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    DWORD color;
    HRESULT hr;

    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD vs_code[] =
    {
#if 0
int color_index;

cbuffer colors
{
    float4 colors[8];
};

struct vs_in
{
    float4 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

vs_out main(const vs_in v)
{
    vs_out o;

    o.position = v.position;
    o.color = colors[color_index];

    return o;
}
#endif
        0x43425844, 0xcecf6d7c, 0xe418097c, 0x47902dd0, 0x9500abc2, 0x00000001, 0x00000160, 0x00000003,
        0x0000002c, 0x00000060, 0x000000b4, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x000000a4, 0x00010040,
        0x00000029, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04000859, 0x00208e46, 0x00000001,
        0x00000008, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000001, 0x02000068, 0x00000001, 0x05000036, 0x001020f2, 0x00000000,
        0x00101e46, 0x00000000, 0x06000036, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000,
        0x07000036, 0x001020f2, 0x00000001, 0x04208e46, 0x00000001, 0x0010000a, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
struct ps_in
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(const ps_in v) : SV_TARGET
{
    return v.color;
}
#endif
        0x43425844, 0xe2087fa6, 0xa35fbd95, 0x8e585b3f, 0x67890f54, 0x00000001, 0x000000f4, 0x00000003,
        0x0000002c, 0x00000080, 0x000000b4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };
    static const struct
    {
        float color[4];
    }
    colors[10] =
    {
        {{0.0f, 0.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f, 1.0f}},
        {{0.0f, 1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.0f, 1.0f, 1.0f, 0.0f}},
        {{0.0f, 1.0f, 1.0f, 1.0f}},
        {{1.0f, 0.0f, 0.0f, 0.0f}},
        {{1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 0.0f, 1.0f, 0.0f}},
    };
    static const struct
    {
        int index;
        DWORD expected;
    }
    test_data[] =
    {
        { 0, 0xff000000},
        { 1, 0x00ff0000},
        { 2, 0xffff0000},
        { 3, 0x0000ff00},
        { 4, 0xff00ff00},
        { 5, 0x00ffff00},
        { 6, 0xffffff00},
        { 7, 0x000000ff},

        { 8, 0xff0000ff},
        { 9, 0x00ff00ff},
    };
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quad), quad);
    colors_cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(colors), &colors);
    index_cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(index), NULL);

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);
    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_VSSetConstantBuffers(device, 0, 1, &index_cb);
    ID3D10Device_VSSetConstantBuffers(device, 1, 1, &colors_cb);
    ID3D10Device_PSSetShader(device, ps);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, white);

        index[0] = test_data[i].index;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)index_cb, 0, NULL, index, 0, 0);

        ID3D10Device_Draw(device, 4, 0);

        color = get_texture_color(test_context.backbuffer, 319, 239);
        ok(compare_color(color, test_data[i].expected, 1),
                "Got unexpected color 0x%08x for index %d.\n", color, test_data[i].index);
    }

    ID3D10Buffer_Release(index_cb);
    ID3D10Buffer_Release(colors_cb);

    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    ID3D10Buffer_Release(vb);
    ID3D10InputLayout_Release(input_layout);
    release_test_context(&test_context);
}

static void test_swapchain_formats(void)
{
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGISwapChain *swapchain;
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    ID3D10Device *device;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 0;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 0;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = CreateWindowA("static", "d3d10core_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = ID3D10Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to query IDXGIDevice, hr %#x.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(SUCCEEDED(hr), "GetAdapter failed, hr %#x.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "GetParent failed, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x for typeless format.\n", hr);
    if (SUCCEEDED(hr))
        IDXGISwapChain_Release(swapchain);

    for (i = 0; i < sizeof(display_format_support) / sizeof(*display_format_support); ++i)
    {
        if (display_format_support[i].optional)
            continue;

        swapchain_desc.BufferDesc.Format = display_format_support[i].format;
        hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &swapchain_desc, &swapchain);
        ok(hr == S_OK, "Got unexpected hr %#x for format %#x.\n", hr, display_format_support[i].format);
        refcount = IDXGISwapChain_Release(swapchain);
        ok(!refcount, "Swapchain has %u references left.\n", refcount);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDXGIFactory_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
    DestroyWindow(swapchain_desc.OutputWindow);
}

static void test_swapchain_views(void)
{
    struct d3d10core_test_context test_context;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D10_RENDER_TARGET_VIEW_DESC rtv_desc;
    ID3D10ShaderResourceView *srv;
    ID3D10RenderTargetView *rtv;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    refcount = get_refcount((IUnknown *)test_context.backbuffer);
    ok(refcount == 1, "Got refcount %u.\n", refcount);

    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    U(rtv_desc).Texture2D.MipSlice = 0;
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)test_context.backbuffer, &rtv_desc, &rtv);
    /* This seems to work only on Windows 7. */
    ok(hr == S_OK || broken(hr == E_INVALIDARG), "Failed to create render target view, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        ID3D10RenderTargetView_Release(rtv);

    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    U(srv_desc).Texture2D.MostDetailedMip = 0;
    U(srv_desc).Texture2D.MipLevels = 1;
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)test_context.backbuffer, &srv_desc, &srv);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        ID3D10ShaderResourceView_Release(srv);

    release_test_context(&test_context);
}

static void test_swapchain_flip(void)
{
    IDXGISwapChain *swapchain;
    ID3D10Texture2D *backbuffer_0, *backbuffer_1, *backbuffer_2, *offscreen;
    ID3D10RenderTargetView *backbuffer_0_rtv, *offscreen_rtv;
    ID3D10ShaderResourceView *backbuffer_0_srv, *backbuffer_1_srv;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10InputLayout *input_layout;
    ID3D10Buffer *vb;
    unsigned int stride, offset;
    ID3D10Device *device;
    D3D10_VIEWPORT vp;
    ULONG refcount;
    DWORD color;
    HWND window;
    HRESULT hr;
    RECT rect;

    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const DWORD vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t0, t1;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;

            p.x = 0.5;
            p.y = 0.5;
            if (position.x < 320)
                return t0.Sample(s, p);
            return t1.Sample(s, p);
        }
#endif
        0x43425844, 0x1733542c, 0xf74c6b6a, 0x0fb11eac, 0x76f6a999, 0x00000001, 0x000002cc, 0x00000005,
        0x00000034, 0x000000f4, 0x00000128, 0x0000015c, 0x00000250, 0x46454452, 0x000000b8, 0x00000000,
        0x00000000, 0x00000003, 0x0000001c, 0xffff0400, 0x00000100, 0x00000084, 0x0000007c, 0x00000003,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0000007e, 0x00000002,
        0x00000005, 0x00000004, 0xffffffff, 0x00000000, 0x00000001, 0x0000000c, 0x00000081, 0x00000002,
        0x00000005, 0x00000004, 0xffffffff, 0x00000001, 0x00000001, 0x0000000c, 0x30740073, 0x00317400,
        0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
        0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x0000002c, 0x00000001,
        0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000010f, 0x505f5653,
        0x5449534f, 0x004e4f49, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000,
        0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853,
        0x000000ec, 0x00000040, 0x0000003b, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000,
        0x00000000, 0x00005555, 0x04001858, 0x00107000, 0x00000001, 0x00005555, 0x04002064, 0x00101012,
        0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x07000031,
        0x00100012, 0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x43a00000, 0x0304001f, 0x0010000a,
        0x00000000, 0x0c000045, 0x001020f2, 0x00000000, 0x00004002, 0x3f000000, 0x3f000000, 0x00000000,
        0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e, 0x01000015, 0x0c000045,
        0x001020f2, 0x00000000, 0x00004002, 0x3f000000, 0x3f000000, 0x00000000, 0x00000000, 0x00107e46,
        0x00000001, 0x00106000, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000007, 0x00000001,
        0x00000000, 0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000
    };
    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};
    static const float green[] = {0.0f, 1.0f, 0.0f, 0.5f};
    static const float blue[] = {0.0f, 0.0f, 1.0f, 0.5f};
    struct swapchain_desc desc;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    SetRect(&rect, 0, 0, 640, 480);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);
    window = CreateWindowA("static", "d3d10core_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    desc.buffer_count = 3;
    desc.swap_effect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    desc.windowed = TRUE;
    desc.flags = SWAPCHAIN_FLAG_SHADER_INPUT;
    swapchain = create_swapchain(device, window, &desc);

    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D10Texture2D, (void **)&backbuffer_0);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 1, &IID_ID3D10Texture2D, (void **)&backbuffer_1);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    hr = IDXGISwapChain_GetBuffer(swapchain, 2, &IID_ID3D10Texture2D, (void **)&backbuffer_2);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)backbuffer_0, NULL, &backbuffer_0_rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)backbuffer_0, NULL, &backbuffer_0_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)backbuffer_1, NULL, &backbuffer_1_srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    ID3D10Texture2D_GetDesc(backbuffer_0, &texture_desc);
    todo_wine ok((texture_desc.BindFlags & (D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE))
            == (D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE),
            "Got unexpected bind flags %x.\n", texture_desc.BindFlags);
    ok(texture_desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected usage %u.\n", texture_desc.Usage);

    ID3D10Texture2D_GetDesc(backbuffer_1, &texture_desc);
    todo_wine ok((texture_desc.BindFlags & (D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE))
            == (D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE),
            "Got unexpected bind flags %x.\n", texture_desc.BindFlags);
    ok(texture_desc.Usage == D3D10_USAGE_DEFAULT, "Got unexpected usage %u.\n", texture_desc.Usage);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)backbuffer_1, NULL, &offscreen_rtv);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        ID3D10RenderTargetView_Release(offscreen_rtv);

    ID3D10Device_PSSetShaderResources(device, 0, 1, &backbuffer_0_srv);
    ID3D10Device_PSSetShaderResources(device, 1, 1, &backbuffer_1_srv);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &offscreen);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)offscreen, NULL, &offscreen_rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);
    ID3D10Device_OMSetRenderTargets(device, 1, &offscreen_rtv, NULL);
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D10Device_RSSetViewports(device, 1, &vp);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);
    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D10Device_VSSetShader(device, vs);
    stride = sizeof(*quad);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_ClearRenderTargetView(device, backbuffer_0_rtv, red);

    ID3D10Device_Draw(device, 4, 0);
    color = get_texture_color(offscreen, 120, 240);
    ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    /* DXGI moves buffers in the same direction as earlier versions. Buffer 2 becomes buffer 1,
     * buffer 1 becomes the new buffer 0, and buffer 0 becomes buffer n - 1. However, only buffer
     * 0 can be rendered to.
     *
     * What is this good for? I don't know. Ad-hoc tests suggest that Present always waits for
     * the next vsync interval, even if there are still untouched buffers. Buffer 0 is the buffer
     * that is shown on the screen, just like in <= d3d9. Present also doesn't discard buffers if
     * rendering finishes before the vsync interval is over. I haven't found any productive use
     * for more than one buffer. */
    IDXGISwapChain_Present(swapchain, 0, 0);

    ID3D10Device_ClearRenderTargetView(device, backbuffer_0_rtv, green);

    ID3D10Device_Draw(device, 4, 0);
    color = get_texture_color(offscreen, 120, 240); /* green, buf 0 */
    ok(compare_color(color, 0x7f00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    /* Buffer 1 is still untouched. */

    color = get_texture_color(backbuffer_0, 320, 240); /* green */
    ok(compare_color(color, 0x7f00ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(backbuffer_2, 320, 240); /* red */
    ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    IDXGISwapChain_Present(swapchain, 0, 0);

    ID3D10Device_ClearRenderTargetView(device, backbuffer_0_rtv, blue);

    ID3D10Device_Draw(device, 4, 0);
    color = get_texture_color(offscreen, 120, 240); /* blue, buf 0 */
    ok(compare_color(color, 0x7fff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(offscreen, 360, 240); /* red, buf 1 */
    ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);

    color = get_texture_color(backbuffer_0, 320, 240); /* blue */
    ok(compare_color(color, 0x7fff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(backbuffer_1, 320, 240); /* red */
    ok(compare_color(color, 0x7f0000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_texture_color(backbuffer_2, 320, 240); /* green */
    ok(compare_color(color, 0x7f00ff00, 1), "Got unexpected color 0x%08x.\n", color);

    ID3D10VertexShader_Release(vs);
    ID3D10PixelShader_Release(ps);
    ID3D10Buffer_Release(vb);
    ID3D10InputLayout_Release(input_layout);
    ID3D10ShaderResourceView_Release(backbuffer_0_srv);
    ID3D10ShaderResourceView_Release(backbuffer_1_srv);
    ID3D10RenderTargetView_Release(backbuffer_0_rtv);
    ID3D10RenderTargetView_Release(offscreen_rtv);
    ID3D10Texture2D_Release(offscreen);
    ID3D10Texture2D_Release(backbuffer_0);
    ID3D10Texture2D_Release(backbuffer_1);
    ID3D10Texture2D_Release(backbuffer_2);
    IDXGISwapChain_Release(swapchain);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_clear_render_target_view(void)
{
    static const DWORD expected_color = 0xbf4c7f19, expected_srgb_color = 0xbf95bc59;
    static const float color[] = {0.1f, 0.5f, 0.3f, 0.75f};
    static const float green[] = {0.0f, 1.0f, 0.0f, 0.5f};

    struct d3d10core_test_context test_context;
    ID3D10Texture2D *texture, *srgb_texture;
    ID3D10RenderTargetView *rtv, *srgb_rtv;
    D3D10_RENDER_TARGET_VIEW_DESC rtv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Device *device;
    HRESULT hr;

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &srgb_texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)srgb_texture, NULL, &srgb_rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, color);
    check_texture_color(test_context.backbuffer, expected_color, 1);

    ID3D10Device_ClearRenderTargetView(device, rtv, color);
    check_texture_color(texture, expected_color, 1);

    if (is_d3d11_interface_available(device))
    {
        ID3D10Device_ClearRenderTargetView(device, NULL, green);
        check_texture_color(texture, expected_color, 1);
    }
    else
        win_skip("D3D11 is not available, skipping test.\n");

    ID3D10Device_ClearRenderTargetView(device, srgb_rtv, color);
    check_texture_color(srgb_texture, expected_srgb_color, 1);

    ID3D10RenderTargetView_Release(srgb_rtv);
    ID3D10RenderTargetView_Release(rtv);
    ID3D10Texture2D_Release(srgb_texture);
    ID3D10Texture2D_Release(texture);

    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    U(rtv_desc).Texture2D.MipSlice = 0;
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, &rtv_desc, &srgb_rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtv_desc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    U(rtv_desc).Texture2D.MipSlice = 0;
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, &rtv_desc, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D10Device_ClearRenderTargetView(device, rtv, color);
    check_texture_color(texture, expected_color, 1);

    if (!is_warp_device(device))
    {
        ID3D10Device_ClearRenderTargetView(device, srgb_rtv, color);
        todo_wine check_texture_color(texture, expected_srgb_color, 1);
    }
    else
    {
        win_skip("sRGB clears are broken on WARP.\n");
    }

    ID3D10RenderTargetView_Release(srgb_rtv);
    ID3D10RenderTargetView_Release(rtv);
    ID3D10Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_clear_depth_stencil_view(void)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *depth_texture;
    ID3D10DepthStencilView *dsv;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &depth_texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)depth_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(depth_texture, 1.0f, 0);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 0.25f, 0);
    check_texture_float(depth_texture, 0.25f, 0);

    ID3D10Texture2D_Release(depth_texture);
    ID3D10DepthStencilView_Release(dsv);

    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &depth_texture);
    ok(SUCCEEDED(hr), "Failed to create depth texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)depth_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);
    todo_wine check_texture_color(depth_texture, 0x00ffffff, 0);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 0.0f, 0xff);
    todo_wine check_texture_color(depth_texture, 0xff000000, 0);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0xff);
    check_texture_color(depth_texture, 0xffffffff, 0);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 0.0f, 0);
    check_texture_color(depth_texture, 0x00000000, 0);

    if (is_d3d11_interface_available(device))
    {
        ID3D10Device_ClearDepthStencilView(device, NULL, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0xff);
        check_texture_color(depth_texture, 0x00000000, 0);
    }
    else
        win_skip("D3D11 is not available, skipping test.\n");

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0xff);
    todo_wine check_texture_color(depth_texture, 0x00ffffff, 0);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_STENCIL, 0.0f, 0xff);
    check_texture_color(depth_texture, 0xffffffff, 0);

    ID3D10Texture2D_Release(depth_texture);
    ID3D10DepthStencilView_Release(dsv);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_draw_depth_only(void)
{
    ID3D10DepthStencilState *depth_stencil_state;
    D3D10_DEPTH_STENCIL_DESC depth_stencil_desc;
    struct d3d10core_test_context test_context;
    ID3D10PixelShader *ps_color, *ps_depth;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10DepthStencilView *dsv;
    struct resource_readback rb;
    ID3D10Texture2D *texture;
    ID3D10Device *device;
    unsigned int i, j;
    D3D10_VIEWPORT vp;
    struct vec4 depth;
    ID3D10Buffer *cb;
    HRESULT hr;

    static const DWORD ps_color_code[] =
    {
#if 0
        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            return float4(0.0, 1.0, 0.0, 1.0);
        }
#endif
        0x43425844, 0x30240e72, 0x012f250c, 0x8673c6ea, 0x392e4cec, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002,
        0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const DWORD ps_depth_code[] =
    {
#if 0
        float depth;

        float main() : SV_Depth
        {
            return depth;
        }
#endif
        0x43425844, 0x91af6cd0, 0x7e884502, 0xcede4f54, 0x6f2c9326, 0x00000001, 0x000000b0, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
        0x00000e01, 0x445f5653, 0x68747065, 0xababab00, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x02000065, 0x0000c001, 0x05000036, 0x0000c001,
        0x0020800a, 0x00000000, 0x00000000, 0x0100003e,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(depth), NULL);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    depth_stencil_desc.DepthEnable = TRUE;
    depth_stencil_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D10_COMPARISON_LESS;
    depth_stencil_desc.StencilEnable = FALSE;

    hr = ID3D10Device_CreateDepthStencilState(device, &depth_stencil_desc, &depth_stencil_state);
    ok(SUCCEEDED(hr), "Failed to create depth stencil state, hr %#x.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, ps_color_code, sizeof(ps_color_code), &ps_color);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_depth_code, sizeof(ps_depth_code), &ps_depth);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);
    ID3D10Device_PSSetShader(device, ps_color);
    ID3D10Device_OMSetRenderTargets(device, 0, NULL, dsv);
    ID3D10Device_OMSetDepthStencilState(device, depth_stencil_state, 0);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(texture, 1.0f, 1);
    draw_quad(&test_context);
    check_texture_float(texture, 0.0f, 1);

    ID3D10Device_PSSetShader(device, ps_depth);

    depth.x = 0.7f;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &depth, 0, 0);
    draw_quad(&test_context);
    check_texture_float(texture, 0.0f, 1);
    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0);
    check_texture_float(texture, 1.0f, 1);
    draw_quad(&test_context);
    check_texture_float(texture, 0.7f, 1);
    depth.x = 0.8f;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &depth, 0, 0);
    draw_quad(&test_context);
    check_texture_float(texture, 0.7f, 1);
    depth.x = 0.5f;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &depth, 0, 0);
    draw_quad(&test_context);
    check_texture_float(texture, 0.5f, 1);

    ID3D10Device_ClearDepthStencilView(device, dsv, D3D10_CLEAR_DEPTH, 1.0f, 0);
    depth.x = 0.1f;
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            depth.x = 1.0f / 16.0f * (j + 4 * i);
            ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &depth, 0, 0);

            vp.TopLeftX = 160 * j;
            vp.TopLeftY = 120 * i;
            vp.Width = 160;
            vp.Height = 120;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            ID3D10Device_RSSetViewports(device, 1, &vp);

            draw_quad(&test_context);
        }
    }
    get_texture_readback(texture, 0, &rb);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            float obtained_depth, expected_depth;

            obtained_depth = get_readback_float(&rb, 80 + j * 160, 60 + i * 120);
            expected_depth = 1.0f / 16.0f * (j + 4 * i);
            ok(compare_float(obtained_depth, expected_depth, 1),
                    "Got unexpected depth %.8e at (%u, %u), expected %.8e.\n",
                    obtained_depth, j, i, expected_depth);
        }
    }
    release_resource_readback(&rb);

    ID3D10Buffer_Release(cb);
    ID3D10PixelShader_Release(ps_color);
    ID3D10PixelShader_Release(ps_depth);
    ID3D10DepthStencilView_Release(dsv);
    ID3D10DepthStencilState_Release(depth_stencil_state);
    ID3D10Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_shader_stage_input_output_matching(void)
{
    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *render_target;
    ID3D10RenderTargetView *rtv[2];
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        struct output
        {
            float4 position : SV_PoSiTion;
            float4 color0 : COLOR0;
            float4 color1 : COLOR1;
        };

        void main(uint id : SV_VertexID, out output o)
        {
            float2 coords = float2((id << 1) & 2, id & 2);
            o.position = float4(coords * float2(2, -2) + float2(-1, 1), 0, 1);
            o.color0 = float4(1.0f, 0.0f, 0.0f, 1.0f);
            o.color1 = float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x93c216a1, 0xbaa7e8d4, 0xd5368c6a, 0x4e889e07, 0x00000001, 0x00000224, 0x00000003,
        0x0000002c, 0x00000060, 0x000000cc, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000101, 0x565f5653, 0x65747265, 0x00444978,
        0x4e47534f, 0x00000064, 0x00000003, 0x00000008, 0x00000050, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x0000005c, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x0000005c, 0x00000001, 0x00000000, 0x00000003, 0x00000002, 0x0000000f, 0x505f5653, 0x5469536f,
        0x006e6f69, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000150, 0x00010040, 0x00000054, 0x04000060,
        0x00101012, 0x00000000, 0x00000006, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x03000065,
        0x001020f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000002, 0x02000068, 0x00000001, 0x07000029,
        0x00100012, 0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x00000001, 0x07000001, 0x00100012,
        0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x07000001, 0x00100042, 0x00000000,
        0x0010100a, 0x00000000, 0x00004001, 0x00000002, 0x05000056, 0x00100032, 0x00000000, 0x00100086,
        0x00000000, 0x0f000032, 0x00102032, 0x00000000, 0x00100046, 0x00000000, 0x00004002, 0x40000000,
        0xc0000000, 0x00000000, 0x00000000, 0x00004002, 0xbf800000, 0x3f800000, 0x00000000, 0x00000000,
        0x08000036, 0x001020c2, 0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
        0x08000036, 0x001020f2, 0x00000001, 0x00004002, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000,
        0x08000036, 0x001020f2, 0x00000002, 0x00004002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000,
        0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        struct input
        {
            float4 position : SV_PoSiTiOn;
            float4 color1 : COLOR1;
            float4 color0 : COLOR0;
        };

        struct output
        {
            float4 target0 : SV_Target0;
            float4 target1 : SV_Target1;
        };

        void main(const in input i, out output o)
        {
            o.target0 = i.color0;
            o.target1 = i.color1;
        }
#endif
        0x43425844, 0x620ef963, 0xed8f19fe, 0x7b3a0a53, 0x126ce021, 0x00000001, 0x00000150, 0x00000003,
        0x0000002c, 0x00000098, 0x000000e4, 0x4e475349, 0x00000064, 0x00000003, 0x00000008, 0x00000050,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x0000005c, 0x00000001, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x0000005c, 0x00000000, 0x00000000, 0x00000003, 0x00000002,
        0x00000f0f, 0x505f5653, 0x5469536f, 0x006e4f69, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x00000044,
        0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f,
        0x00000038, 0x00000001, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x545f5653, 0x65677261,
        0xabab0074, 0x52444853, 0x00000064, 0x00000040, 0x00000019, 0x03001062, 0x001010f2, 0x00000001,
        0x03001062, 0x001010f2, 0x00000002, 0x03000065, 0x001020f2, 0x00000000, 0x03000065, 0x001020f2,
        0x00000001, 0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000002, 0x05000036, 0x001020f2,
        0x00000001, 0x00101e46, 0x00000001, 0x0100003e,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Texture2D_GetDesc(test_context.backbuffer, &texture_desc);
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &render_target);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    rtv[0] = test_context.backbuffer_rtv;
    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)render_target, NULL, &rtv[1]);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_PSSetShader(device, ps);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D10Device_OMSetRenderTargets(device, 2, rtv, NULL);
    ID3D10Device_Draw(device, 3, 0);

    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);
    check_texture_color(render_target, 0xff0000ff, 0);

    ID3D10RenderTargetView_Release(rtv[1]);
    ID3D10Texture2D_Release(render_target);
    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    release_test_context(&test_context);
}

static void test_sm4_if_instruction(void)
{
    struct d3d10core_test_context test_context;
    ID3D10PixelShader *ps_if_nz, *ps_if_z;
    ID3D10Device *device;
    unsigned int bits[4];
    DWORD expected_color;
    ID3D10Buffer *cb;
    unsigned int i;
    HRESULT hr;

    static const DWORD ps_if_nz_code[] =
    {
#if 0
        uint bits;

        float4 main() : SV_TARGET
        {
            if (bits)
                return float4(0.0f, 1.0f, 0.0f, 1.0f);
            else
                return float4(1.0f, 0.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x2a94f6f1, 0xdbe88943, 0x3426a708, 0x09cec990, 0x00000001, 0x00000100, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000088, 0x00000040, 0x00000022,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x0404001f,
        0x0020800a, 0x00000000, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x00000000,
        0x3f800000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000012, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000015, 0x0100003e,
    };
    static const DWORD ps_if_z_code[] =
    {
#if 0
        uint bits;

        float4 main() : SV_TARGET
        {
            if (!bits)
                return float4(0.0f, 1.0f, 0.0f, 1.0f);
            else
                return float4(1.0f, 0.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x2e3030ca, 0x94c8610c, 0xdf0c1b1f, 0x80f2ca2c, 0x00000001, 0x00000100, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000088, 0x00000040, 0x00000022,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x0400001f,
        0x0020800a, 0x00000000, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x00000000,
        0x3f800000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000012, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000015, 0x0100003e,
    };
    static unsigned int bit_patterns[] =
    {
        0x00000000, 0x00000001, 0x10010001, 0x10000000, 0x80000000, 0xffff0000, 0x0000ffff, 0xffffffff,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_if_nz_code, sizeof(ps_if_nz_code), &ps_if_nz);
    ok(SUCCEEDED(hr), "Failed to create if_nz pixel shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_if_z_code, sizeof(ps_if_z_code), &ps_if_z);
    ok(SUCCEEDED(hr), "Failed to create if_z pixel shader, hr %#x.\n", hr);

    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(bits), NULL);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    for (i = 0; i < sizeof(bit_patterns) / sizeof(*bit_patterns); ++i)
    {
        *bits = bit_patterns[i];
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, bits, 0, 0);

        ID3D10Device_PSSetShader(device, ps_if_nz);
        expected_color = *bits ? 0xff00ff00 : 0xff0000ff;
        draw_quad(&test_context);
        check_texture_color(test_context.backbuffer, expected_color, 0);

        ID3D10Device_PSSetShader(device, ps_if_z);
        expected_color = *bits ? 0xff0000ff : 0xff00ff00;
        draw_quad(&test_context);
        check_texture_color(test_context.backbuffer, expected_color, 0);
    }

    ID3D10Buffer_Release(cb);
    ID3D10PixelShader_Release(ps_if_z);
    ID3D10PixelShader_Release(ps_if_nz);
    release_test_context(&test_context);
}

static void test_sm4_breakc_instruction(void)
{
    struct d3d10core_test_context test_context;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    HRESULT hr;

    static const DWORD ps_breakc_nz_code[] =
    {
#if 0
        float4 main() : SV_TARGET
        {
            uint counter = 0;

            for (uint i = 0; i < 255; ++i)
                ++counter;

            if (counter == 255)
                return float4(0.0f, 1.0f, 0.0f, 1.0f);
            else
                return float4(1.0f, 0.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x065ac80a, 0x24369e7e, 0x218d5dc1, 0x3532868c, 0x00000001, 0x00000188, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000110, 0x00000040, 0x00000044,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x08000036, 0x00100032, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000030, 0x07000050, 0x00100042,
        0x00000000, 0x0010001a, 0x00000000, 0x00004001, 0x000000ff, 0x03040003, 0x0010002a, 0x00000000,
        0x0a00001e, 0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x00004002, 0x00000001, 0x00000001,
        0x00000000, 0x00000000, 0x01000016, 0x07000020, 0x00100012, 0x00000000, 0x0010000a, 0x00000000,
        0x00004001, 0x000000ff, 0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000012, 0x08000036,
        0x001020f2, 0x00000000, 0x00004002, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e,
        0x01000015, 0x0100003e,
    };
    static const DWORD ps_breakc_z_code[] =
    {
#if 0
        float4 main() : SV_TARGET
        {
            uint counter = 0;

            for (int i = 0, j = 254; i < 255 && j >= 0; ++i, --j)
                ++counter;

            if (counter == 255)
                return float4(0.0f, 1.0f, 0.0f, 1.0f);
            else
                return float4(1.0f, 0.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x687406ef, 0x7bdeb7d1, 0xb3282292, 0x934a9101, 0x00000001, 0x000001c0, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000148, 0x00000040, 0x00000052,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x08000036, 0x00100072, 0x00000000,
        0x00004002, 0x00000000, 0x00000000, 0x000000fe, 0x00000000, 0x01000030, 0x07000022, 0x00100082,
        0x00000000, 0x0010001a, 0x00000000, 0x00004001, 0x000000ff, 0x07000021, 0x00100012, 0x00000001,
        0x0010002a, 0x00000000, 0x00004001, 0x00000000, 0x07000001, 0x00100082, 0x00000000, 0x0010003a,
        0x00000000, 0x0010000a, 0x00000001, 0x03000003, 0x0010003a, 0x00000000, 0x0a00001e, 0x00100072,
        0x00000000, 0x00100246, 0x00000000, 0x00004002, 0x00000001, 0x00000001, 0xffffffff, 0x00000000,
        0x01000016, 0x07000020, 0x00100012, 0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x000000ff,
        0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x00000000,
        0x3f800000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000012, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000015, 0x0100003e,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_breakc_nz_code, sizeof(ps_breakc_nz_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create breakc_nz pixel shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);
    ID3D10PixelShader_Release(ps);

    hr = ID3D10Device_CreatePixelShader(device, ps_breakc_z_code, sizeof(ps_breakc_z_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create breakc_z pixel shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);
    ID3D10PixelShader_Release(ps);

    release_test_context(&test_context);
}

static void test_create_input_layout(void)
{
    D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D10InputLayout *input_layout;
    ID3D10Device *device;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const DXGI_FORMAT vertex_formats[] =
    {
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT,
        DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SINT,
        DXGI_FORMAT_R8_UINT,
        DXGI_FORMAT_R8_SINT,
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    for (i = 0; i < sizeof(vertex_formats) / sizeof(*vertex_formats); ++i)
    {
        layout_desc->Format = vertex_formats[i];
        hr = ID3D10Device_CreateInputLayout(device, layout_desc,
                sizeof(layout_desc) / sizeof(*layout_desc), vs_code, sizeof(vs_code),
                &input_layout);
        ok(SUCCEEDED(hr), "Failed to create input layout for format %#x, hr %#x.\n",
                vertex_formats[i], hr);
        ID3D10InputLayout_Release(input_layout);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_input_assembler(void)
{
    enum layout_id
    {
        LAYOUT_FLOAT32,
        LAYOUT_UINT16,
        LAYOUT_SINT16,
        LAYOUT_UNORM16,
        LAYOUT_SNORM16,
        LAYOUT_UINT8,
        LAYOUT_SINT8,
        LAYOUT_UNORM8,
        LAYOUT_SNORM8,
        LAYOUT_UNORM10_2,
        LAYOUT_UINT10_2,

        LAYOUT_COUNT,
    };

    D3D10_INPUT_ELEMENT_DESC input_layout_desc[] =
    {
        {"POSITION",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"ATTRIBUTE", 0, DXGI_FORMAT_UNKNOWN,      1, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D10VertexShader *vs_float, *vs_uint, *vs_sint;
    ID3D10InputLayout *input_layout[LAYOUT_COUNT];
    struct d3d10core_test_context test_context;
    ID3D10Buffer *vb_position, *vb_attribute;
    D3D10_TEXTURE2D_DESC texture_desc;
    unsigned int i, j, stride, offset;
    ID3D10Texture2D *render_target;
    ID3D10RenderTargetView *rtv;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    HRESULT hr;

    static const DXGI_FORMAT layout_formats[LAYOUT_COUNT] =
    {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R16G16B16A16_UINT,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_SNORM,
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_R10G10B10A2_UINT,
    };
    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };
    static const DWORD ps_code[] =
    {
#if 0
        float4 main(float4 position : POSITION, float4 color: COLOR) : SV_Target
        {
            return color;
        }
#endif
        0x43425844, 0xa9150342, 0x70e18d2e, 0xf7769835, 0x4c3a7f02, 0x00000001, 0x000000f0, 0x00000003,
        0x0000002c, 0x0000007c, 0x000000b0, 0x4e475349, 0x00000048, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000000f, 0x00000041, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0x4c4f4300, 0xab00524f, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const DWORD vs_float_code[] =
    {
#if 0
        struct output
        {
            float4 position : SV_Position;
            float4 color : COLOR;
        };

        void main(float4 position : POSITION, float4 color : ATTRIBUTE, out output o)
        {
            o.position = position;
            o.color = color;
        }
#endif
        0x43425844, 0xf6051ffd, 0xd9e49503, 0x171ad197, 0x3764fe47, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000080, 0x000000d4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0x54544100, 0x55424952, 0xab004554,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x7469736f, 0x006e6f69, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000068, 0x00010040,
        0x0000001a, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067,
        0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x05000036, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001,
        0x0100003e,
    };
    static const DWORD vs_uint_code[] =
    {
#if 0
        struct output
        {
            float4 position : SV_Position;
            float4 color : COLOR;
        };

        void main(float4 position : POSITION, uint4 color : ATTRIBUTE, out output o)
        {
            o.position = position;
            o.color = color;
        }
#endif
        0x43425844, 0x0bae0bc0, 0xf6473aa5, 0x4ecf4a25, 0x414fac23, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000080, 0x000000d4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
        0x00000001, 0x00000001, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0x54544100, 0x55424952, 0xab004554,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x7469736f, 0x006e6f69, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000068, 0x00010040,
        0x0000001a, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067,
        0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x05000056, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001,
        0x0100003e,
    };
    static const DWORD vs_sint_code[] =
    {
#if 0
        struct output
        {
            float4 position : SV_Position;
            float4 color : COLOR;
        };

        void main(float4 position : POSITION, int4 color : ATTRIBUTE, out output o)
        {
            o.position = position;
            o.color = color;
        }
#endif
        0x43425844, 0xaf60aad9, 0xba91f3a4, 0x2015d384, 0xf746fdf5, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000080, 0x000000d4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
        0x00000002, 0x00000001, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0x54544100, 0x55424952, 0xab004554,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x7469736f, 0x006e6f69, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000068, 0x00010040,
        0x0000001a, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067,
        0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0500002b, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001,
        0x0100003e,
    };
    static const float float32_data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    static const unsigned short uint16_data[] = {6, 8, 55, 777};
    static const short sint16_data[] = {-1, 33, 8, -77};
    static const unsigned short unorm16_data[] = {0, 16383, 32767, 65535};
    static const short snorm16_data[] = {-32768, 0, 32767, 0};
    static const unsigned char uint8_data[] = {0, 64, 128, 255};
    static const signed char sint8_data[] = {-128, 0, 127, 64};
    static const unsigned int uint32_zero = 0;
    static const unsigned int uint32_max = 0xffffffff;
    static const unsigned int unorm10_2_data= 0xa00003ff;
    static const unsigned int g10_data = 0x000ffc00;
    static const unsigned int a2_data = 0xc0000000;
    static const struct
    {
        enum layout_id layout_id;
        unsigned int stride;
        const void *data;
        struct vec4 expected_color;
        BOOL todo;
    }
    tests[] =
    {
        {LAYOUT_FLOAT32,   sizeof(float32_data),   float32_data,
                {1.0f, 2.0f, 3.0f, 4.0f}},
        {LAYOUT_UINT16,    sizeof(uint16_data),    uint16_data,
                {6.0f, 8.0f, 55.0f, 777.0f}, TRUE},
        {LAYOUT_SINT16,    sizeof(sint16_data),    sint16_data,
                {-1.0f, 33.0f, 8.0f, -77.0f}, TRUE},
        {LAYOUT_UNORM16,   sizeof(unorm16_data),   unorm16_data,
                {0.0f, 16383.0f / 65535.0f, 32767.0f / 65535.0f, 1.0f}},
        {LAYOUT_SNORM16,   sizeof(snorm16_data),   snorm16_data,
                {-1.0f, 0.0f, 1.0f, 0.0f}},
        {LAYOUT_UINT8,     sizeof(uint32_zero),    &uint32_zero,
                {0.0f, 0.0f, 0.0f, 0.0f}},
        {LAYOUT_UINT8,     sizeof(uint32_max),     &uint32_max,
                {255.0f, 255.0f, 255.0f, 255.0f}},
        {LAYOUT_UINT8,     sizeof(uint8_data),     uint8_data,
                {0.0f, 64.0f, 128.0f, 255.0f}},
        {LAYOUT_SINT8,     sizeof(uint32_zero),    &uint32_zero,
                {0.0f, 0.0f, 0.0f, 0.0f}},
        {LAYOUT_SINT8,     sizeof(uint32_max),     &uint32_max,
                {-1.0f, -1.0f, -1.0f, -1.0f}},
        {LAYOUT_SINT8,     sizeof(sint8_data),     sint8_data,
                {-128.0f, 0.0f, 127.0f, 64.0f}},
        {LAYOUT_UNORM8,    sizeof(uint32_zero),    &uint32_zero,
                {0.0f, 0.0f, 0.0f, 0.0f}},
        {LAYOUT_UNORM8,    sizeof(uint32_max),     &uint32_max,
                {1.0f, 1.0f, 1.0f, 1.0f}},
        {LAYOUT_UNORM8,    sizeof(uint8_data),     uint8_data,
                {0.0f, 64.0f / 255.0f, 128.0f / 255.0f, 1.0f}},
        {LAYOUT_SNORM8,    sizeof(uint32_zero),    &uint32_zero,
                {0.0f, 0.0f, 0.0f, 0.0f}},
        {LAYOUT_SNORM8,    sizeof(sint8_data),     sint8_data,
                {-1.0f, 0.0f, 1.0f, 64.0f / 127.0f}},
        {LAYOUT_UNORM10_2, sizeof(uint32_zero),    &uint32_zero,
                {0.0f, 0.0f, 0.0f, 0.0f}},
        {LAYOUT_UNORM10_2, sizeof(uint32_max),     &uint32_max,
                {1.0f, 1.0f, 1.0f, 1.0f}},
        {LAYOUT_UNORM10_2, sizeof(g10_data),       &g10_data,
                {0.0f, 1.0f, 0.0f, 0.0f}},
        {LAYOUT_UNORM10_2, sizeof(a2_data),        &a2_data,
                {0.0f, 0.0f, 0.0f, 1.0f}},
        {LAYOUT_UNORM10_2, sizeof(unorm10_2_data), &unorm10_2_data,
                {1.0f, 0.0f, 512.0f / 1023.0f, 2.0f / 3.0f}},
        {LAYOUT_UINT10_2,  sizeof(uint32_zero),    &uint32_zero,
                {0.0f, 0.0f, 0.0f, 0.0f}},
        {LAYOUT_UINT10_2,  sizeof(uint32_max),     &uint32_max,
                {1023.0f, 1023.0f, 1023.0f, 3.0f}},
        {LAYOUT_UINT10_2,  sizeof(g10_data),       &g10_data,
                {0.0f, 1023.0f, 0.0f, 0.0f}},
        {LAYOUT_UINT10_2,  sizeof(a2_data),        &a2_data,
                {0.0f, 0.0f, 0.0f, 3.0f}},
        {LAYOUT_UINT10_2,  sizeof(unorm10_2_data), &unorm10_2_data,
                {1023.0f, 0.0f, 512.0f, 2.0f}},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    hr = ID3D10Device_CreateVertexShader(device, vs_float_code, sizeof(vs_float_code), &vs_float);
    ok(SUCCEEDED(hr), "Failed to create float vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreateVertexShader(device, vs_uint_code, sizeof(vs_uint_code), &vs_uint);
    ok(SUCCEEDED(hr), "Failed to create uint vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreateVertexShader(device, vs_sint_code, sizeof(vs_sint_code), &vs_sint);
    ok(SUCCEEDED(hr), "Failed to create sint vertex shader, hr %#x.\n", hr);

    for (i = 0; i < LAYOUT_COUNT; ++i)
    {
        input_layout_desc[1].Format = layout_formats[i];
        input_layout[i] = NULL;
        hr = ID3D10Device_CreateInputLayout(device, input_layout_desc,
                sizeof(input_layout_desc) / sizeof(*input_layout_desc),
                vs_float_code, sizeof(vs_float_code), &input_layout[i]);
        todo_wine_if(input_layout_desc[1].Format == DXGI_FORMAT_R10G10B10A2_UINT)
        ok(SUCCEEDED(hr), "Failed to create input layout for format %#x, hr %#x.\n", layout_formats[i], hr);
    }

    vb_position = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quad), quad);
    vb_attribute = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, 1024, NULL);

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &render_target);
    ok(SUCCEEDED(hr), "Failed to create 2d texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)render_target, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#x.\n", hr);

    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    offset = 0;
    stride = sizeof(*quad);
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb_position, &stride, &offset);
    ID3D10Device_PSSetShader(device, ps);
    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        D3D10_BOX box = {0, 0, 0, 1, 1, 1};

        if (tests[i].layout_id == LAYOUT_UINT10_2)
            continue;

        assert(tests[i].layout_id < LAYOUT_COUNT);
        ID3D10Device_IASetInputLayout(device, input_layout[tests[i].layout_id]);

        assert(4 * tests[i].stride <= 1024);
        box.right = tests[i].stride;
        for (j = 0; j < 4; ++j)
        {
            ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)vb_attribute, 0,
                    &box, tests[i].data, 0, 0);
            box.left += tests[i].stride;
            box.right += tests[i].stride;
        }

        stride = tests[i].stride;
        ID3D10Device_IASetVertexBuffers(device, 1, 1, &vb_attribute, &stride, &offset);

        switch (layout_formats[tests[i].layout_id])
        {
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R8G8B8A8_UINT:
                ID3D10Device_VSSetShader(device, vs_uint);
                break;
            case DXGI_FORMAT_R16G16B16A16_SINT:
            case DXGI_FORMAT_R8G8B8A8_SINT:
                ID3D10Device_VSSetShader(device, vs_sint);
                break;

            default:
                trace("Unhandled format %#x.\n", layout_formats[tests[i].layout_id]);
                /* Fall through. */
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
                ID3D10Device_VSSetShader(device, vs_float);
                break;
        }

        ID3D10Device_Draw(device, 4, 0);
        check_texture_vec4(render_target, &tests[i].expected_color, 2);
    }

    ID3D10Texture2D_Release(render_target);
    ID3D10RenderTargetView_Release(rtv);
    ID3D10Buffer_Release(vb_attribute);
    ID3D10Buffer_Release(vb_position);
    for (i = 0; i < LAYOUT_COUNT; ++i)
    {
        if (input_layout[i])
            ID3D10InputLayout_Release(input_layout[i]);
    }
    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs_float);
    ID3D10VertexShader_Release(vs_uint);
    ID3D10VertexShader_Release(vs_sint);
    release_test_context(&test_context);
}

static void test_null_sampler(void)
{
    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10ShaderResourceView *srv;
    ID3D10RenderTargetView *rtv;
    ID3D10SamplerState *sampler;
    ID3D10Texture2D *texture;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            return t.Sample(s, float2(position.x / 640.0f, position.y / 480.0f));
        }
#endif
        0x43425844, 0x1ce9b612, 0xc8176faa, 0xd37844af, 0xdb515605, 0x00000001, 0x00000134, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000098, 0x00000040,
        0x00000026, 0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
        0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0a000038, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x00004002, 0x3acccccd,
        0x3b088889, 0x00000000, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000000,
        0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e,
    };
    static const float blue[] = {0.0f, 0.0f, 1.0f, 1.0f};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    texture_desc.Width = 64;
    texture_desc.Height = 64;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, NULL, &srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    ID3D10Device_ClearRenderTargetView(device, rtv, blue);

    ID3D10Device_PSSetShader(device, ps);
    ID3D10Device_PSSetShaderResources(device, 0, 1, &srv);
    sampler = NULL;
    ID3D10Device_PSSetSamplers(device, 0, 1, &sampler);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xffff0000, 0);

    ID3D10ShaderResourceView_Release(srv);
    ID3D10RenderTargetView_Release(rtv);
    ID3D10Texture2D_Release(texture);
    ID3D10PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_immediate_constant_buffer(void)
{
    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10RenderTargetView *rtv;
    unsigned int index[4] = {0};
    ID3D10Texture2D *texture;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    ID3D10Buffer *cb;
    unsigned int i;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        uint index;

        static const int int_array[6] =
        {
            310, 111, 212, -513, -318, 0,
        };

        static const uint uint_array[6] =
        {
            2, 7, 0x7f800000, 0xff800000, 0x7fc00000, 0
        };

        static const float float_array[6] =
        {
            76, 83.5f, 0.5f, 0.75f, -0.5f, 0.0f,
        };

        float4 main() : SV_Target
        {
            return float4(int_array[index], uint_array[index], float_array[index], 1.0f);
        }
#endif
        0x43425844, 0xbad068da, 0xd631ea3c, 0x41648374, 0x3ccd0120, 0x00000001, 0x00000184, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x0000010c, 0x00000040, 0x00000043,
        0x00001835, 0x0000001a, 0x00000136, 0x00000002, 0x42980000, 0x00000000, 0x0000006f, 0x00000007,
        0x42a70000, 0x00000000, 0x000000d4, 0x7f800000, 0x3f000000, 0x00000000, 0xfffffdff, 0xff800000,
        0x3f400000, 0x00000000, 0xfffffec2, 0x7fc00000, 0xbf000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2,
        0x00000000, 0x02000068, 0x00000001, 0x05000036, 0x00102082, 0x00000000, 0x00004001, 0x3f800000,
        0x06000036, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x06000056, 0x00102022,
        0x00000000, 0x0090901a, 0x0010000a, 0x00000000, 0x0600002b, 0x00102012, 0x00000000, 0x0090900a,
        0x0010000a, 0x00000000, 0x06000036, 0x00102042, 0x00000000, 0x0090902a, 0x0010000a, 0x00000000,
        0x0100003e,
    };
    static struct vec4 expected_result[] =
    {
        { 310.0f,          2.0f, 76.00f, 1.0f},
        { 111.0f,          7.0f, 83.50f, 1.0f},
        { 212.0f, 2139095040.0f,  0.50f, 1.0f},
        {-513.0f, 4286578688.0f,  0.75f, 1.0f},
        {-318.0f, 2143289344.0f, -0.50f, 1.0f},
        {   0.0f,          0.0f,  0.0f,  1.0f},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);

    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(index), NULL);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    ID3D10Texture2D_GetDesc(test_context.backbuffer, &texture_desc);
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);
    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);

    for (i = 0; i < sizeof(expected_result) / sizeof(*expected_result); ++i)
    {
        *index = i;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, index, 0, 0);

        draw_quad(&test_context);
        check_texture_vec4(texture, &expected_result[i], 0);
    }

    ID3D10Buffer_Release(cb);
    ID3D10PixelShader_Release(ps);
    ID3D10Texture2D_Release(texture);
    ID3D10RenderTargetView_Release(rtv);
    release_test_context(&test_context);
}

static void test_fp_specials(void)
{
    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10RenderTargetView *rtv;
    ID3D10Texture2D *texture;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        float4 main() : SV_Target
        {
            return float4(0.0f / 0.0f, 1.0f / 0.0f, -1.0f / 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x86d7f319, 0x14cde598, 0xe7ce83a8, 0x0e06f3f0, 0x00000001, 0x000000b0, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0xffc00000,
        0x7f800000, 0xff800000, 0x3f800000, 0x0100003e,
    };
    static const struct uvec4 expected_result = {BITS_NNAN, BITS_INF, BITS_NINF, BITS_1_0};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Texture2D_GetDesc(test_context.backbuffer, &texture_desc);
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);

    draw_quad(&test_context);
    check_texture_uvec4(texture, &expected_result);

    ID3D10PixelShader_Release(ps);
    ID3D10Texture2D_Release(texture);
    ID3D10RenderTargetView_Release(rtv);
    release_test_context(&test_context);
}

static void test_uint_shader_instructions(void)
{
    struct shader
    {
        const DWORD *code;
        size_t size;
    };

    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10RenderTargetView *rtv;
    ID3D10Texture2D *texture;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    ID3D10Buffer *cb;
    unsigned int i;
    HRESULT hr;

    static const DWORD ps_ftou_code[] =
    {
#if 0
        float f;

        uint4 main() : SV_Target
        {
            return uint4(f, -f, 0, 0);
        }
#endif
        0x43425844, 0xfde0ee2d, 0x812b339a, 0xb9fc36d2, 0x5820bec6, 0x00000001, 0x000000f4, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x0000007c, 0x00000040, 0x0000001f,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x0600001c,
        0x00102012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x0700001c, 0x00102022, 0x00000000,
        0x8020800a, 0x00000041, 0x00000000, 0x00000000, 0x08000036, 0x001020c2, 0x00000000, 0x00004002,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_not_code[] =
    {
#if 0
        uint bits[2];

        uint4 main() : SV_Target
        {
            return uint4(~bits[0], ~(bits[0] ^ ~0u), ~bits[1], ~(bits[1] ^ ~0u));
        }
#endif
        0x43425844, 0x1d56b429, 0xb5f4c0e1, 0x496a0bfd, 0xfc6f8e6f, 0x00000001, 0x00000140, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x000000c8, 0x00000040, 0x00000032,
        0x04000059, 0x00208e46, 0x00000000, 0x00000002, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x08000057, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x00004001,
        0xffffffff, 0x0500003b, 0x00102022, 0x00000000, 0x0010000a, 0x00000000, 0x08000057, 0x00100012,
        0x00000000, 0x0020800a, 0x00000000, 0x00000001, 0x00004001, 0xffffffff, 0x0500003b, 0x00102082,
        0x00000000, 0x0010000a, 0x00000000, 0x0600003b, 0x00102012, 0x00000000, 0x0020800a, 0x00000000,
        0x00000000, 0x0600003b, 0x00102042, 0x00000000, 0x0020800a, 0x00000000, 0x00000001, 0x0100003e,
    };
    static const struct shader ps_ftou = {ps_ftou_code, sizeof(ps_ftou_code)};
    static const struct shader ps_not = {ps_not_code, sizeof(ps_not_code)};
    static const struct
    {
        const struct shader *ps;
        unsigned int bits[4];
        struct uvec4 expected_result;
        BOOL todo;
    }
    tests[] =
    {
        {&ps_ftou,  {BITS_NNAN}, { 0,  0}},
        {&ps_ftou,  {BITS_NAN},  { 0,  0}},
        {&ps_ftou,  {BITS_NINF}, { 0, ~0u}},
        {&ps_ftou,  {BITS_INF},  {~0u, 0}},
        {&ps_ftou,  {BITS_N1_0}, { 0,  1}},
        {&ps_ftou,  {BITS_1_0},  { 1,  0}},
        {&ps_not,   {0x00000000, 0xffffffff}, {0xffffffff, 0x00000000, 0x00000000, 0xffffffff}},
        {&ps_not,   {0xf0f0f0f0, 0x0f0f0f0f}, {0x0f0f0f0f, 0xf0f0f0f0, 0xf0f0f0f0, 0x0f0f0f0f}},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, 4 * sizeof(tests[0].bits), NULL);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    ID3D10Texture2D_GetDesc(test_context.backbuffer, &texture_desc);
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        hr = ID3D10Device_CreatePixelShader(device, tests[i].ps->code, tests[i].ps->size, &ps);
        ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
        ID3D10Device_PSSetShader(device, ps);

        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, tests[i].bits, 0, 0);

        draw_quad(&test_context);
        todo_wine_if(tests[i].todo)
        check_texture_uvec4(texture, &tests[i].expected_result);

        ID3D10PixelShader_Release(ps);
    }

    ID3D10Buffer_Release(cb);
    ID3D10Texture2D_Release(texture);
    ID3D10RenderTargetView_Release(rtv);
    release_test_context(&test_context);
}

static void test_index_buffer_offset(void)
{
    struct d3d10core_test_context test_context;
    ID3D10Buffer *vb, *ib, *so_buffer;
    ID3D10InputLayout *input_layout;
    struct resource_readback rb;
    ID3D10GeometryShader *gs;
    const struct vec4 *data;
    ID3D10VertexShader *vs;
    ID3D10Device *device;
    UINT stride, offset;
    unsigned int i;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        void main(float4 position : SV_POSITION, float4 attrib : ATTRIB,
                out float4 out_position : SV_Position, out float4 out_attrib : ATTRIB)
        {
            out_position = position;
            out_attrib = attrib;
        }
#endif
        0x43425844, 0xd7716716, 0xe23207f3, 0xc8af57c0, 0x585e2919, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000080, 0x000000d4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52545441, 0xab004249,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x7469736f, 0x006e6f69, 0x52545441, 0xab004249, 0x52444853, 0x00000068, 0x00010040,
        0x0000001a, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067,
        0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x05000036, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001,
        0x0100003e,
    };
    static const DWORD gs_code[] =
    {
#if 0
        struct vertex
        {
            float4 position : SV_POSITION;
            float4 attrib : ATTRIB;
        };

        [maxvertexcount(1)]
        void main(point vertex input[1], inout PointStream<vertex> output)
        {
            output.Append(input[0]);
            output.RestartStrip();
        }
#endif
        0x43425844, 0x3d1dc497, 0xdf450406, 0x284ab03b, 0xa4ec0fd6, 0x00000001, 0x00000170, 0x00000003,
        0x0000002c, 0x00000080, 0x000000d4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x00000f0f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52545441, 0xab004249,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x5449534f, 0x004e4f49, 0x52545441, 0xab004249, 0x52444853, 0x00000094, 0x00020040,
        0x00000025, 0x05000061, 0x002010f2, 0x00000001, 0x00000000, 0x00000001, 0x0400005f, 0x002010f2,
        0x00000001, 0x00000001, 0x0100085d, 0x0100085c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000001, 0x0200005e, 0x00000001, 0x06000036, 0x001020f2, 0x00000000,
        0x00201e46, 0x00000000, 0x00000000, 0x06000036, 0x001020f2, 0x00000001, 0x00201e46, 0x00000000,
        0x00000001, 0x01000013, 0x01000009, 0x0100003e,
    };
    static const D3D10_INPUT_ELEMENT_DESC input_desc[] =
    {
        {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"ATTRIB",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const D3D10_SO_DECLARATION_ENTRY so_declaration[] =
    {
        {"SV_Position", 0, 0, 4, 0},
        {"ATTRIB",      0, 0, 4, 0},
    };
    static const struct
    {
        struct vec4 position;
        struct vec4 attrib;
    }
    vertices[] =
    {
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {2.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {3.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {4.0f}},
    };
    static const unsigned int indices[] =
    {
        0, 1, 2, 3,
        3, 2, 1, 0,
        1, 3, 2, 0,
    };
    static const struct vec4 expected_data[] =
    {
        {-1.0f, -1.0f, 0.0f, 1.0f}, {1.0f},
        {-1.0f,  1.0f, 0.0f, 1.0f}, {2.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f}, {3.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f}, {4.0f},

        { 1.0f,  1.0f, 0.0f, 1.0f}, {4.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f}, {3.0f},
        {-1.0f,  1.0f, 0.0f, 1.0f}, {2.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f}, {1.0f},

        {-1.0f,  1.0f, 0.0f, 1.0f}, {2.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f}, {4.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f}, {3.0f},
        {-1.0f, -1.0f, 0.0f, 1.0f}, {1.0f},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreateInputLayout(device, input_desc, sizeof(input_desc) / sizeof(*input_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    stride = 32;
    hr = ID3D10Device_CreateGeometryShaderWithStreamOutput(device, gs_code, sizeof(gs_code),
            so_declaration, sizeof(so_declaration) / sizeof(*so_declaration),
            stride, &gs);
    todo_wine ok(SUCCEEDED(hr), "Failed to create geometry shader with stream output, hr %#x.\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(vertices), vertices);
    ib = create_buffer(device, D3D10_BIND_INDEX_BUFFER, sizeof(indices), indices);
    so_buffer = create_buffer(device, D3D10_BIND_STREAM_OUTPUT, 1024, NULL);

    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_GSSetShader(device, gs);

    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
    stride = sizeof(*vertices);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);

    offset = 0;
    ID3D10Device_SOSetTargets(device, 1, &so_buffer, &offset);

    ID3D10Device_IASetIndexBuffer(device, ib, DXGI_FORMAT_R32_UINT, 0);
    ID3D10Device_DrawIndexed(device, 4, 0, 0);

    ID3D10Device_IASetIndexBuffer(device, ib, DXGI_FORMAT_R32_UINT, 4 * sizeof(*indices));
    ID3D10Device_DrawIndexed(device, 4, 0, 0);

    ID3D10Device_IASetIndexBuffer(device, ib, DXGI_FORMAT_R32_UINT, 8 * sizeof(*indices));
    ID3D10Device_DrawIndexed(device, 4, 0, 0);

    get_buffer_readback(so_buffer, &rb);
    for (i = 0; i < sizeof(expected_data) / sizeof(*expected_data); ++i)
    {
        data = get_readback_vec4(&rb, i, 0);
        ok(compare_vec4(data, &expected_data[i], 0),
                "Got unexpected result {%.8e, %.8e, %.8e, %.8e} at %u.\n",
                data->x, data->y, data->z, data->w, i);
    }
    release_resource_readback(&rb);

    ID3D10Buffer_Release(so_buffer);
    ID3D10Buffer_Release(ib);
    ID3D10Buffer_Release(vb);
    ID3D10VertexShader_Release(vs);
    ID3D10GeometryShader_Release(gs);
cleanup:
    ID3D10InputLayout_Release(input_layout);
    release_test_context(&test_context);
}

static void test_face_culling(void)
{
    struct d3d10core_test_context test_context;
    D3D10_RASTERIZER_DESC rasterizer_desc;
    ID3D10RasterizerState *state;
    ID3D10Buffer *cw_vb, *ccw_vb;
    ID3D10Device *device;
    BOOL broken_warp;
    unsigned int i;
    HRESULT hr;

    static const struct vec4 red = {1.0f, 0.0f, 0.0f, 1.0f};
    static const struct vec4 green = {0.0f, 1.0f, 0.0f, 1.0f};
    static const DWORD ps_code[] =
    {
#if 0
        float4 main(uint front : SV_IsFrontFace) : SV_Target
        {
            return (front == ~0u) ? float4(0.0f, 1.0f, 0.0f, 1.0f) : float4(0.0f, 0.0f, 1.0f, 1.0f);
        }
#endif
        0x43425844, 0x92002fad, 0xc5c620b9, 0xe7a154fb, 0x78b54e63, 0x00000001, 0x00000128, 0x00000003,
        0x0000002c, 0x00000064, 0x00000098, 0x4e475349, 0x00000030, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000009, 0x00000001, 0x00000000, 0x00000101, 0x495f5653, 0x6f724673, 0x6146746e,
        0xab006563, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000,
        0x00000003, 0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000088,
        0x00000040, 0x00000022, 0x04000863, 0x00101012, 0x00000000, 0x00000009, 0x03000065, 0x001020f2,
        0x00000000, 0x02000068, 0x00000001, 0x07000020, 0x00100012, 0x00000000, 0x0010100a, 0x00000000,
        0x00004001, 0xffffffff, 0x0f000037, 0x001020f2, 0x00000000, 0x00100006, 0x00000000, 0x00004002,
        0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x00004002, 0x00000000, 0x00000000, 0x3f800000,
        0x3f800000, 0x0100003e,
    };
    static const struct vec2 ccw_quad[] =
    {
        {-1.0f,  1.0f},
        {-1.0f, -1.0f},
        { 1.0f,  1.0f},
        { 1.0f, -1.0f},
    };
    static const struct
    {
        D3D10_CULL_MODE cull_mode;
        BOOL front_ccw;
        BOOL expected_cw;
        BOOL expected_ccw;
    }
    tests[] =
    {
        {D3D10_CULL_NONE,  FALSE, TRUE,  TRUE},
        {D3D10_CULL_NONE,  TRUE,  TRUE,  TRUE},
        {D3D10_CULL_FRONT, FALSE, FALSE, TRUE},
        {D3D10_CULL_FRONT, TRUE,  TRUE,  FALSE},
        {D3D10_CULL_BACK,  FALSE, TRUE,  FALSE},
        {D3D10_CULL_BACK,  TRUE,  FALSE, TRUE},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);

    cw_vb = test_context.vb;
    ccw_vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(ccw_quad), ccw_quad);

    test_context.vb = ccw_vb;
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff0000ff, 0);

    rasterizer_desc.FillMode = D3D10_FILL_SOLID;
    rasterizer_desc.CullMode = D3D10_CULL_BACK;
    rasterizer_desc.FrontCounterClockwise = FALSE;
    rasterizer_desc.DepthBias = 0;
    rasterizer_desc.DepthBiasClamp = 0.0f;
    rasterizer_desc.SlopeScaledDepthBias = 0.0f;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ScissorEnable = FALSE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = FALSE;

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        rasterizer_desc.CullMode = tests[i].cull_mode;
        rasterizer_desc.FrontCounterClockwise = tests[i].front_ccw;
        hr = ID3D10Device_CreateRasterizerState(device, &rasterizer_desc, &state);
        ok(SUCCEEDED(hr), "Test %u: Failed to create rasterizer state, hr %#x.\n", i, hr);

        ID3D10Device_RSSetState(device, state);

        test_context.vb = cw_vb;
        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
        draw_color_quad(&test_context, &green);
        check_texture_color(test_context.backbuffer, tests[i].expected_cw ? 0xff00ff00 : 0xff0000ff, 0);

        test_context.vb = ccw_vb;
        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
        draw_color_quad(&test_context, &green);
        check_texture_color(test_context.backbuffer, tests[i].expected_ccw ? 0xff00ff00 : 0xff0000ff, 0);

        ID3D10RasterizerState_Release(state);
    }

    broken_warp = is_warp_device(device) && !is_d3d11_interface_available(device);

    /* Test SV_IsFrontFace. */
    ID3D10PixelShader_Release(test_context.ps);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &test_context.ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    rasterizer_desc.CullMode = D3D10_CULL_NONE;
    rasterizer_desc.FrontCounterClockwise = FALSE;
    hr = ID3D10Device_CreateRasterizerState(device, &rasterizer_desc, &state);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ID3D10Device_RSSetState(device, state);

    test_context.vb = cw_vb;
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);
    test_context.vb = ccw_vb;
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    if (!broken_warp)
        check_texture_color(test_context.backbuffer, 0xffff0000, 0);
    else
        win_skip("Broken WARP.\n");

    ID3D10RasterizerState_Release(state);

    rasterizer_desc.CullMode = D3D10_CULL_NONE;
    rasterizer_desc.FrontCounterClockwise = TRUE;
    hr = ID3D10Device_CreateRasterizerState(device, &rasterizer_desc, &state);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ID3D10Device_RSSetState(device, state);

    test_context.vb = cw_vb;
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    if (!broken_warp)
        check_texture_color(test_context.backbuffer, 0xffff0000 , 0);
    else
        win_skip("Broken WARP.\n");
    test_context.vb = ccw_vb;
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);

    ID3D10RasterizerState_Release(state);

    test_context.vb = cw_vb;
    ID3D10Buffer_Release(ccw_vb);
    release_test_context(&test_context);
}

static void test_line_antialiasing_blending(void)
{
    struct d3d10core_test_context test_context;
    ID3D10RasterizerState *rasterizer_state;
    D3D10_RASTERIZER_DESC rasterizer_desc;
    ID3D10BlendState *blend_state;
    D3D10_BLEND_DESC blend_desc;
    ID3D10Device *device;
    HRESULT hr;

    static const struct vec4 red = {1.0f, 0.0f, 0.0f, 0.8f};
    static const struct vec4 green = {0.0f, 1.0f, 0.0f, 0.5f};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    memset(&blend_desc, 0, sizeof(blend_desc));
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.BlendEnable[0] = TRUE;
    blend_desc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
    blend_desc.DestBlend = D3D10_BLEND_DEST_ALPHA;
    blend_desc.BlendOp = D3D10_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_SRC_ALPHA;
    blend_desc.DestBlendAlpha = D3D10_BLEND_DEST_ALPHA;
    blend_desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#x.\n", hr);
    ID3D10Device_OMSetBlendState(device, blend_state, NULL, D3D10_DEFAULT_SAMPLE_MASK);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xe2007fcc, 1);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &green.x);
    draw_color_quad(&test_context, &red);
    check_texture_color(test_context.backbuffer, 0xe2007fcc, 1);

    ID3D10Device_OMSetBlendState(device, NULL, NULL, D3D10_DEFAULT_SAMPLE_MASK);
    ID3D10BlendState_Release(blend_state);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0x7f00ff00, 1);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &green.x);
    draw_color_quad(&test_context, &red);
    check_texture_color(test_context.backbuffer, 0xcc0000ff, 1);

    rasterizer_desc.FillMode = D3D10_FILL_SOLID;
    rasterizer_desc.CullMode = D3D10_CULL_BACK;
    rasterizer_desc.FrontCounterClockwise = FALSE;
    rasterizer_desc.DepthBias = 0;
    rasterizer_desc.DepthBiasClamp = 0.0f;
    rasterizer_desc.SlopeScaledDepthBias = 0.0f;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ScissorEnable = FALSE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = TRUE;

    hr = ID3D10Device_CreateRasterizerState(device, &rasterizer_desc, &rasterizer_state);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ID3D10Device_RSSetState(device, rasterizer_state);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &red.x);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0x7f00ff00, 1);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, &green.x);
    draw_color_quad(&test_context, &red);
    check_texture_color(test_context.backbuffer, 0xcc0000ff, 1);

    ID3D10RasterizerState_Release(rasterizer_state);
    release_test_context(&test_context);
}

static void check_format_support(const unsigned int *format_support,
        const struct format_support *formats, unsigned int format_count,
        unsigned int feature_flag, const char *feature_name)
{
    unsigned int i;

    for (i = 0; i < format_count; ++i)
    {
        DXGI_FORMAT format = formats[i].format;
        unsigned int supported = format_support[format] & feature_flag;

        if (formats[i].optional)
        {
            if (supported)
                trace("Optional format %#x - %s supported.\n", format, feature_name);
            continue;
        }

        ok(supported, "Format %#x - %s supported, format support %#x.\n",
                format, feature_name, format_support[format]);
    }
}

static void test_required_format_support(void)
{
    unsigned int format_support[DXGI_FORMAT_B4G4R4A4_UNORM + 1];
    ID3D10Device *device;
    DXGI_FORMAT format;
    ULONG refcount;
    HRESULT hr;

    static const struct format_support index_buffers[] =
    {
        {DXGI_FORMAT_R32_UINT},
        {DXGI_FORMAT_R16_UINT},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    memset(format_support, 0, sizeof(format_support));
    for (format = DXGI_FORMAT_UNKNOWN; format <= DXGI_FORMAT_B4G4R4A4_UNORM; ++format)
    {
        hr = ID3D10Device_CheckFormatSupport(device, format, &format_support[format]);
        todo_wine ok(hr == S_OK || (hr == E_FAIL && !format_support[format]),
                "Got unexpected result for format %#x: hr %#x, format_support %#x.\n",
                format, hr, format_support[format]);
    }
    if (hr == E_NOTIMPL)
    {
        skip("CheckFormatSupport not implemented.\n");
        ID3D10Device_Release(device);
        return;
    }

    check_format_support(format_support, index_buffers,
            sizeof(index_buffers) / sizeof(*index_buffers),
            D3D10_FORMAT_SUPPORT_IA_INDEX_BUFFER, "index buffer");

    check_format_support(format_support, display_format_support,
            sizeof(display_format_support) / sizeof(*display_format_support),
            D3D10_FORMAT_SUPPORT_DISPLAY, "display");

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_ddy(void)
{
    static const struct
    {
        struct vec4 position;
        unsigned int color;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f, 1.0f}, 0x00ff0000},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, 0x0000ff00},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, 0x00ff0000},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, 0x0000ff00},
    };
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",       0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
#if 0
struct vs_data
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

void main(in struct vs_data vs_input, out struct vs_data vs_output)
{
    vs_output.pos = vs_input.pos;
    vs_output.color = vs_input.color;
}
#endif
    static const DWORD vs_code[] =
    {
        0x43425844, 0xd5b32785, 0x35332906, 0x4d05e031, 0xf66a58af, 0x00000001, 0x00000144, 0x00000003,
        0x0000002c, 0x00000080, 0x000000d4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052,
        0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f,
        0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052, 0x52444853, 0x00000068, 0x00010040,
        0x0000001a, 0x0300005f, 0x001010f2, 0x00000000, 0x0300005f, 0x001010f2, 0x00000001, 0x04000067,
        0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x05000036, 0x001020f2, 0x00000001, 0x00101e46, 0x00000001,
        0x0100003e,
    };
#if 0
struct ps_data
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(struct ps_data ps_input) : SV_Target
{
    return ddy(ps_input.color) * 240.0 + 0.5;
}
#endif
    static const DWORD ps_code[] =
    {
        0x43425844, 0x423712f6, 0x786c59c2, 0xa6023c60, 0xb79faad2, 0x00000001, 0x00000138, 0x00000003,
        0x0000002c, 0x00000080, 0x000000b4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x4f4c4f43, 0xabab0052,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x0000007c, 0x00000040,
        0x0000001f, 0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x0500000c, 0x001000f2, 0x00000000, 0x00101e46, 0x00000001, 0x0f000032, 0x001020f2,
        0x00000000, 0x00100e46, 0x00000000, 0x00004002, 0x43700000, 0x43700000, 0x43700000, 0x43700000,
        0x00004002, 0x3f000000, 0x3f000000, 0x3f000000, 0x3f000000, 0x0100003e,
    };
    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10InputLayout *input_layout;
    unsigned int stride, offset;
    struct resource_readback rb;
    ID3D10RenderTargetView *rtv;
    ID3D10Texture2D *texture;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    ID3D10Buffer *vb;
    DWORD color;
    HRESULT hr;

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    ID3D10Texture2D_GetDesc(test_context.backbuffer, &texture_desc);
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)texture, NULL, &rtv);
    ok(SUCCEEDED(hr), "Failed to create render target view, hr %#x.\n", hr);

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);
    ID3D10Device_VSSetShader(device, vs);

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);
    ID3D10Device_ClearRenderTargetView(device, rtv, red);
    ID3D10Device_Draw(device, 4, 0);

    get_texture_readback(texture, 0, &rb);
    color = get_readback_color(&rb, 320, 190);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 255, 240);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 320, 240);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 385, 240);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 320, 290);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    release_resource_readback(&rb);

    ID3D10Device_OMSetRenderTargets(device, 1, &test_context.backbuffer_rtv, NULL);
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
    ID3D10Device_Draw(device, 4, 0);

    get_texture_readback(test_context.backbuffer, 0, &rb);
    color = get_readback_color(&rb, 320, 190);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 255, 240);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 320, 240);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 385, 240);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_readback_color(&rb, 320, 290);
    ok(compare_color(color, 0x7fff007f, 1), "Got unexpected color 0x%08x.\n", color);
    release_resource_readback(&rb);

    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    ID3D10Buffer_Release(vb);
    ID3D10InputLayout_Release(input_layout);
    ID3D10Texture2D_Release(texture);
    ID3D10RenderTargetView_Release(rtv);
    release_test_context(&test_context);
}

static void test_shader_input_registers_limits(void)
{
    struct d3d10core_test_context test_context;
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_TEXTURE2D_DESC texture_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10ShaderResourceView *srv;
    ID3D10SamplerState *sampler;
    ID3D10Texture2D *texture;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    HRESULT hr;

    static const DWORD ps_last_register_code[] =
    {
#if 0
        Texture2D t : register(t127);
        SamplerState s : register(s15);

        void main(out float4 target : SV_Target)
        {
            target = t.Sample(s, float2(0, 0));
        }
#endif
        0x43425844, 0xd81ff2f8, 0x8c704b9c, 0x8c6f4857, 0xd02949ac, 0x00000001, 0x000000dc, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000064, 0x00000040, 0x00000019,
        0x0300005a, 0x00106000, 0x0000000f, 0x04001858, 0x00107000, 0x0000007f, 0x00005555, 0x03000065,
        0x001020f2, 0x00000000, 0x0c000045, 0x001020f2, 0x00000000, 0x00004002, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00107e46, 0x0000007f, 0x00106000, 0x0000000f, 0x0100003e,
    };
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const DWORD texture_data[] = {0xff00ff00};

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    texture_desc.Width = 1;
    texture_desc.Height = 1;
    texture_desc.MipLevels = 0;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    resource_data.pSysMem = texture_data;
    resource_data.SysMemPitch = sizeof(texture_data);
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, &resource_data, &texture);
    ok(SUCCEEDED(hr), "Failed to create a 2d texture, hr %#x.\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, NULL, &srv);
    ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#x.\n", hr);

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 0;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 0.0f;

    hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler);
    ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#x.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, ps_last_register_code, sizeof(ps_last_register_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_PSSetShaderResources(device,
            D3D10_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT - 1, 1, &srv);
    ID3D10Device_PSSetSamplers(device, D3D10_COMMONSHADER_SAMPLER_REGISTER_COUNT - 1, 1, &sampler);
    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, white);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 1);

    ID3D10PixelShader_Release(ps);
    ID3D10SamplerState_Release(sampler);
    ID3D10ShaderResourceView_Release(srv);
    ID3D10Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_stencil_separate(void)
{
    struct d3d10core_test_context test_context;
    D3D10_TEXTURE2D_DESC texture_desc;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10DepthStencilState *ds_state;
    ID3D10DepthStencilView *ds_view;
    D3D10_RASTERIZER_DESC rs_desc;
    ID3D10RasterizerState *rs;
    ID3D10Texture2D *texture;
    ID3D10Device *device;
    HRESULT hr;

    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static const struct vec4 green = {0.0f, 1.0f, 0.0f, 1.0f};
    static const struct vec2 ccw_quad[] =
    {
        {-1.0f, -1.0f},
        { 1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f,  1.0f},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    texture_desc.Width = 640;
    texture_desc.Height = 480;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)texture, NULL, &ds_view);
    ok(SUCCEEDED(hr), "Failed to create depth stencil view, hr %#x.\n", hr);

    ds_desc.DepthEnable = TRUE;
    ds_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D10_COMPARISON_LESS;
    ds_desc.StencilEnable = TRUE;
    ds_desc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
    ds_desc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
    ds_desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.FrontFace.StencilFunc = D3D10_COMPARISON_NEVER;
    ds_desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_ZERO;
    ds_desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state);
    ok(SUCCEEDED(hr), "Failed to create depth stencil state, hr %#x.\n", hr);

    rs_desc.FillMode = D3D10_FILL_SOLID;
    rs_desc.CullMode = D3D10_CULL_NONE;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = FALSE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;
    ID3D10Device_CreateRasterizerState(device, &rs_desc, &rs);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
    ID3D10Device_ClearDepthStencilView(device, ds_view, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);
    ID3D10Device_OMSetRenderTargets(device, 1, &test_context.backbuffer_rtv, ds_view);
    ID3D10Device_OMSetDepthStencilState(device, ds_state, 0);
    ID3D10Device_RSSetState(device, rs);

    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff0000ff, 1);

    ID3D10Buffer_Release(test_context.vb);
    test_context.vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(ccw_quad), ccw_quad);

    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 1);

    ID3D10RasterizerState_Release(rs);
    rs_desc.FrontCounterClockwise = TRUE;
    ID3D10Device_CreateRasterizerState(device, &rs_desc, &rs);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#x.\n", hr);
    ID3D10Device_RSSetState(device, rs);

    ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
    draw_color_quad(&test_context, &green);
    check_texture_color(test_context.backbuffer, 0xff0000ff, 1);

    ID3D10DepthStencilState_Release(ds_state);
    ID3D10DepthStencilView_Release(ds_view);
    ID3D10RasterizerState_Release(rs);
    ID3D10Texture2D_Release(texture);
    release_test_context(&test_context);
}

static void test_sm4_ret_instruction(void)
{
    struct d3d10core_test_context test_context;
    ID3D10PixelShader *ps;
    struct uvec4 constant;
    ID3D10Device *device;
    ID3D10Buffer *cb;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        uint c;

        float4 main() : SV_TARGET
        {
            if (c == 1)
                return float4(1, 0, 0, 1);
            if (c == 2)
                return float4(0, 1, 0, 1);
            if (c == 3)
                return float4(0, 0, 1, 1);
            return float4(1, 1, 1, 1);
        }
#endif
        0x43425844, 0x9ee6f808, 0xe74009f3, 0xbb1adaf2, 0x432e97b5, 0x00000001, 0x000001c4, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000014c, 0x00000040, 0x00000053,
        0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x02000068,
        0x00000001, 0x08000020, 0x00100012, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x00004001,
        0x00000001, 0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002,
        0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e, 0x01000015, 0x08000020, 0x00100012,
        0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x00004001, 0x00000002, 0x0304001f, 0x0010000a,
        0x00000000, 0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x00000000, 0x3f800000, 0x00000000,
        0x3f800000, 0x0100003e, 0x01000015, 0x08000020, 0x00100012, 0x00000000, 0x0020800a, 0x00000000,
        0x00000000, 0x00004001, 0x00000003, 0x0304001f, 0x0010000a, 0x00000000, 0x08000036, 0x001020f2,
        0x00000000, 0x00004002, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, 0x0100003e, 0x01000015,
        0x08000036, 0x001020f2, 0x00000000, 0x00004002, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x0100003e,
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create shader, hr %#x.\n", hr);
    ID3D10Device_PSSetShader(device, ps);
    memset(&constant, 0, sizeof(constant));
    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(constant), &constant);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xffffffff, 0);

    constant.x = 1;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &constant, 0, 0);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xff0000ff, 0);

    constant.x = 2;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &constant, 0, 0);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xff00ff00, 0);

    constant.x = 3;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &constant, 0, 0);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xffff0000, 0);

    constant.x = 4;
    ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &constant, 0, 0);
    draw_quad(&test_context);
    check_texture_color(test_context.backbuffer, 0xffffffff, 0);

    ID3D10Buffer_Release(cb);
    ID3D10PixelShader_Release(ps);
    release_test_context(&test_context);
}

static void test_primitive_restart(void)
{
    struct d3d10core_test_context test_context;
    unsigned int stride, offset, x, y;
    ID3D10Buffer *ib32, *ib16, *vb;
    struct resource_readback rb;
    ID3D10InputLayout *layout;
    ID3D10VertexShader *vs;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    unsigned int i;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_Position;
            float4 color : color;
        };

        float4 main(vs_out input) : SV_TARGET
        {
            return input.color;
        }
#endif
        0x43425844, 0x119e48d1, 0x468aecb3, 0x0a405be5, 0x4e203b82, 0x00000001, 0x000000f4, 0x00000003,
        0x0000002c, 0x00000080, 0x000000b4, 0x4e475349, 0x0000004c, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000,
        0x00000003, 0x00000001, 0x00000f0f, 0x505f5653, 0x7469736f, 0x006e6f69, 0x6f6c6f63, 0xabab0072,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000001, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000001, 0x0100003e,
    };
    static const DWORD vs_code[] =
    {
#if 0
        struct vs_out
        {
            float4 position : SV_Position;
            float4 color : color;
        };

        void main(float4 position : POSITION, uint vertex_id : SV_VertexID, out vs_out output)
        {
            output.position = position;
            output.color = vertex_id < 4 ? float4(0.0, 1.0, 1.0, 1.0) : float4(1.0, 0.0, 0.0, 1.0);
        }
#endif
        0x43425844, 0x2fa57573, 0xdb71c15f, 0x2641b028, 0xa8f87ccc, 0x00000001, 0x00000198, 0x00000003,
        0x0000002c, 0x00000084, 0x000000d8, 0x4e475349, 0x00000050, 0x00000002, 0x00000008, 0x00000038,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000006,
        0x00000001, 0x00000001, 0x00000101, 0x49534f50, 0x4e4f4954, 0x5f565300, 0x74726556, 0x44497865,
        0xababab00, 0x4e47534f, 0x0000004c, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000001,
        0x00000003, 0x00000000, 0x0000000f, 0x00000044, 0x00000000, 0x00000000, 0x00000003, 0x00000001,
        0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69, 0x6f6c6f63, 0xabab0072, 0x52444853, 0x000000b8,
        0x00010040, 0x0000002e, 0x0300005f, 0x001010f2, 0x00000000, 0x04000060, 0x00101012, 0x00000001,
        0x00000006, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001,
        0x02000068, 0x00000001, 0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0700004f,
        0x00100012, 0x00000000, 0x0010100a, 0x00000001, 0x00004001, 0x00000004, 0x0f000037, 0x001020f2,
        0x00000001, 0x00100006, 0x00000000, 0x00004002, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00004002, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    static const struct vec2 vertices[] =
    {
        {-1.00f, -1.0f},
        {-1.00f,  1.0f},
        {-0.25f, -1.0f},
        {-0.25f,  1.0f},
        { 0.25f, -1.0f},
        { 0.25f,  1.0f},
        { 1.00f, -1.0f},
        { 1.00f,  1.0f},
    };
    static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    static const unsigned short indices16[] =
    {
        0, 1, 2, 3, 0xffff, 4, 5, 6, 7
    };
    static const unsigned int indices32[] =
    {
        0, 1, 2, 3, 0xffffffff, 4, 5, 6, 7
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    hr = ID3D10Device_CreateVertexShader(device, vs_code, sizeof(vs_code), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = ID3D10Device_CreatePixelShader(device, ps_code, sizeof(ps_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create return pixel shader, hr %#x.\n", hr);

    ib16 = create_buffer(device, D3D10_BIND_INDEX_BUFFER, sizeof(indices16), indices16);
    ib32 = create_buffer(device, D3D10_BIND_INDEX_BUFFER, sizeof(indices32), indices32);

    hr = ID3D10Device_CreateInputLayout(device, layout_desc,
            sizeof(layout_desc) / sizeof(*layout_desc),
            vs_code, sizeof(vs_code), &layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#x.\n", hr);

    vb = create_buffer(device, D3D10_BIND_VERTEX_BUFFER, sizeof(vertices), vertices);

    ID3D10Device_VSSetShader(device, vs);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_IASetInputLayout(device, layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*vertices);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(device, 0, 1, &vb, &stride, &offset);

    for (i = 0; i < 2; ++i)
    {
        if (!i)
            ID3D10Device_IASetIndexBuffer(device, ib32, DXGI_FORMAT_R32_UINT, 0);
        else
            ID3D10Device_IASetIndexBuffer(device, ib16, DXGI_FORMAT_R16_UINT, 0);

        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, black);
        ID3D10Device_DrawIndexed(device, 9, 0, 0);
        get_texture_readback(test_context.backbuffer, 0, &rb);
        for (y = 0; y < 480; ++y)
        {
            for (x = 0; x < 640; ++x)
            {
                DWORD color = get_readback_color(&rb, x, y);
                DWORD expected_color;
                if (x < 240)
                    expected_color = 0xffffff00;
                else if (x >= 640 - 240)
                    expected_color = 0xff0000ff;
                else
                    expected_color = 0x00000000;
                ok(compare_color(color, expected_color, 1),
                        "Test %u: Got 0x%08x, expected 0x%08x at (%u, %u).\n",
                        i, color, expected_color, x, y);
            }
        }
        release_resource_readback(&rb);
    }

    ID3D10Buffer_Release(ib16);
    ID3D10Buffer_Release(ib32);
    ID3D10Buffer_Release(vb);
    ID3D10InputLayout_Release(layout);
    ID3D10PixelShader_Release(ps);
    ID3D10VertexShader_Release(vs);
    release_test_context(&test_context);
}

static void test_render_target_device_mismatch(void)
{
    struct d3d10core_test_context test_context;
    ID3D10RenderTargetView *rtv;
    ID3D10Device *device;
    ULONG refcount;

    if (!init_test_context(&test_context))
        return;

    device = create_device();
    ok(!!device, "Failed to create device.\n");

    rtv = (ID3D10RenderTargetView *)0xdeadbeef;
    ID3D10Device_OMGetRenderTargets(device, 1, &rtv, NULL);
    ok(!rtv, "Got unexpected render target view %p.\n", rtv);
    ID3D10Device_OMSetRenderTargets(device, 1, &test_context.backbuffer_rtv, NULL);
    ID3D10Device_OMGetRenderTargets(device, 1, &rtv, NULL);
    ok(rtv == test_context.backbuffer_rtv, "Got unexpected render target view %p.\n", rtv);
    ID3D10RenderTargetView_Release(rtv);

    rtv = NULL;
    ID3D10Device_OMSetRenderTargets(device, 1, &rtv, NULL);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    release_test_context(&test_context);
}

static void test_buffer_srv(void)
{
    struct buffer
    {
        UINT byte_count;
        const void *data;
    };

    struct d3d10core_test_context test_context;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D10_SUBRESOURCE_DATA resource_data;
    const struct buffer *current_buffer;
    ID3D10ShaderResourceView *srv;
    D3D10_BUFFER_DESC buffer_desc;
    struct resource_readback rb;
    ID3D10Buffer *cb, *buffer;
    ID3D10PixelShader *ps;
    ID3D10Device *device;
    unsigned int i, x, y;
    struct vec4 cb_size;
    DWORD color;
    HRESULT hr;

    static const DWORD ps_float4_code[] =
    {
#if 0
        Buffer<float4> b;

        float2 size;

        float4 main(float4 position : SV_POSITION) : SV_Target
        {
            float2 p;
            int2 coords;
            p.x = position.x / 640.0f;
            p.y = position.y / 480.0f;
            coords = int2(p.x * size.x, p.y * size.y);
            return b.Load(coords.y * size.x + coords.x);
        }
#endif
        0x43425844, 0xf10ea650, 0x311f5c38, 0x3a888b7f, 0x58230334, 0x00000001, 0x000001a0, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000104, 0x00000040,
        0x00000041, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x04000858, 0x00107000, 0x00000000,
        0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x02000068, 0x00000001, 0x08000038, 0x00100032, 0x00000000, 0x00101516, 0x00000000, 0x00208516,
        0x00000000, 0x00000000, 0x0a000038, 0x00100032, 0x00000000, 0x00100046, 0x00000000, 0x00004002,
        0x3b088889, 0x3acccccd, 0x00000000, 0x00000000, 0x05000043, 0x00100032, 0x00000000, 0x00100046,
        0x00000000, 0x0a000032, 0x00100012, 0x00000000, 0x0010000a, 0x00000000, 0x0020800a, 0x00000000,
        0x00000000, 0x0010001a, 0x00000000, 0x0500001b, 0x00100012, 0x00000000, 0x0010000a, 0x00000000,
        0x0700002d, 0x001020f2, 0x00000000, 0x00100006, 0x00000000, 0x00107e46, 0x00000000, 0x0100003e,
    };
    static const DWORD rgba16[] =
    {
        0xff0000ff, 0xff00ffff, 0xff00ff00, 0xffffff00,
        0xffff0000, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD rgba4[] =
    {
        0xffffffff, 0xff0000ff,
        0xff000000, 0xff00ff00,
    };
    static const struct buffer rgba16_buffer = {sizeof(rgba16), &rgba16};
    static const struct buffer rgba4_buffer  = {sizeof(rgba4),  &rgba4};
    static const DWORD rgba4_colors[] =
    {
        0xffffffff, 0xffffffff, 0xff0000ff, 0xff0000ff,
        0xffffffff, 0xffffffff, 0xff0000ff, 0xff0000ff,
        0xff000000, 0xff000000, 0xff00ff00, 0xff00ff00,
        0xff000000, 0xff000000, 0xff00ff00, 0xff00ff00,
    };
    static const DWORD zero_colors[16] = {0};
    static const float red[] = {1.0f, 0.0f, 0.0f, 0.5f};

    static const struct test
    {
        const struct buffer *buffer;
        DXGI_FORMAT srv_format;
        UINT srv_first_element;
        UINT srv_element_count;
        struct vec2 size;
        const DWORD *expected_colors;
    }
    tests[] =
    {
        {&rgba16_buffer, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, {4.0f, 4.0f}, rgba16},
        {&rgba4_buffer,  DXGI_FORMAT_R8G8B8A8_UNORM, 0,  4, {2.0f, 2.0f}, rgba4_colors},
        {NULL,           0,                          0,  0, {2.0f, 2.0f}, zero_colors},
    };

    if (!init_test_context(&test_context))
        return;

    device = test_context.device;

    cb = create_buffer(device, D3D10_BIND_CONSTANT_BUFFER, sizeof(cb_size), NULL);

    hr = ID3D10Device_CreatePixelShader(device, ps_float4_code, sizeof(ps_float4_code), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    ID3D10Device_PSSetShader(device, ps);
    ID3D10Device_PSSetConstantBuffers(device, 0, 1, &cb);

    srv = NULL;
    buffer = NULL;
    current_buffer = NULL;
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        const struct test *test = &tests[i];

        if (current_buffer != test->buffer)
        {
            if (buffer)
                ID3D10Buffer_Release(buffer);
            if (srv)
                ID3D10ShaderResourceView_Release(srv);

            current_buffer = test->buffer;
            if (current_buffer)
            {
                buffer_desc.ByteWidth = current_buffer->byte_count;
                buffer_desc.Usage = D3D10_USAGE_DEFAULT;
                buffer_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
                buffer_desc.CPUAccessFlags = 0;
                buffer_desc.MiscFlags = 0;
                resource_data.SysMemPitch = 0;
                resource_data.SysMemSlicePitch = 0;
                resource_data.pSysMem = current_buffer->data;
                hr = ID3D10Device_CreateBuffer(device, &buffer_desc, &resource_data, &buffer);
                ok(SUCCEEDED(hr), "Test %u: Failed to create buffer, hr %#x.\n", i, hr);

                srv_desc.Format = test->srv_format;
                srv_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
                U(srv_desc).Buffer.ElementOffset = test->srv_first_element;
                U(srv_desc).Buffer.ElementWidth = test->srv_element_count;
                hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)buffer, &srv_desc, &srv);
                ok(SUCCEEDED(hr), "Test %u: Failed to create shader resource view, hr %#x.\n", i, hr);
            }
            else
            {
                buffer = NULL;
                srv = NULL;
            }

            ID3D10Device_PSSetShaderResources(device, 0, 1, &srv);
        }

        cb_size.x = test->size.x;
        cb_size.y = test->size.y;
        ID3D10Device_UpdateSubresource(device, (ID3D10Resource *)cb, 0, NULL, &cb_size, 0, 0);

        ID3D10Device_ClearRenderTargetView(device, test_context.backbuffer_rtv, red);
        draw_quad(&test_context);

        get_texture_readback(test_context.backbuffer, 0, &rb);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                color = get_readback_color(&rb, 80 + x * 160, 60 + y * 120);
                ok(compare_color(color, test->expected_colors[y * 4 + x], 1),
                        "Test %u: Got unexpected color 0x%08x at (%u, %u).\n", i, color, x, y);
            }
        }
        release_resource_readback(&rb);
    }
    if (srv)
        ID3D10ShaderResourceView_Release(srv);
    if (buffer)
        ID3D10Buffer_Release(buffer);

    ID3D10Buffer_Release(cb);
    ID3D10PixelShader_Release(ps);
    release_test_context(&test_context);
}

START_TEST(device)
{
    test_feature_level();
    test_device_interfaces();
    test_create_texture2d();
    test_texture2d_interfaces();
    test_create_texture3d();
    test_create_buffer();
    test_create_depthstencil_view();
    test_depthstencil_view_interfaces();
    test_create_rendertarget_view();
    test_render_target_views();
    test_create_shader_resource_view();
    test_create_shader();
    test_create_sampler_state();
    test_create_blend_state();
    test_create_depthstencil_state();
    test_create_rasterizer_state();
    test_create_query();
    test_occlusion_query();
    test_timestamp_query();
    test_device_removed_reason();
    test_scissor();
    test_clear_state();
    test_blend();
    test_texture();
    test_depth_stencil_sampling();
    test_multiple_render_targets();
    test_private_data();
    test_il_append_aligned();
    test_fragment_coords();
    test_update_subresource();
    test_copy_subresource_region();
    test_check_multisample_quality_levels();
    test_cb_relative_addressing();
    test_swapchain_formats();
    test_swapchain_views();
    test_swapchain_flip();
    test_clear_render_target_view();
    test_clear_depth_stencil_view();
    test_draw_depth_only();
    test_shader_stage_input_output_matching();
    test_sm4_if_instruction();
    test_sm4_breakc_instruction();
    test_create_input_layout();
    test_input_assembler();
    test_null_sampler();
    test_immediate_constant_buffer();
    test_fp_specials();
    test_uint_shader_instructions();
    test_index_buffer_offset();
    test_face_culling();
    test_line_antialiasing_blending();
    test_required_format_support();
    test_ddy();
    test_shader_input_registers_limits();
    test_stencil_separate();
    test_sm4_ret_instruction();
    test_primitive_restart();
    test_render_target_device_mismatch();
    test_buffer_srv();
}
