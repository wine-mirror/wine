/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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

#include "d2d1_private.h"
#include "wincodec.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_bitmap *impl_from_ID2D1Bitmap1(ID2D1Bitmap1 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_bitmap, ID2D1Bitmap1_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_QueryInterface(ID2D1Bitmap1 *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Bitmap1)
            || IsEqualGUID(iid, &IID_ID2D1Bitmap)
            || IsEqualGUID(iid, &IID_ID2D1Image)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Bitmap1_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_bitmap_AddRef(ID2D1Bitmap1 *iface)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);
    ULONG refcount = InterlockedIncrement(&bitmap->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_bitmap_Release(ID2D1Bitmap1 *iface)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);
    ULONG refcount = InterlockedDecrement(&bitmap->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        ID3D10ShaderResourceView_Release(bitmap->view);
        ID2D1Factory_Release(bitmap->factory);
        heap_free(bitmap);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_bitmap_GetFactory(ID2D1Bitmap1 *iface, ID2D1Factory **factory)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = bitmap->factory);
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_bitmap_GetSize(ID2D1Bitmap1 *iface, D2D1_SIZE_F *size)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    size->width = bitmap->pixel_size.width / (bitmap->dpi_x / 96.0f);
    size->height = bitmap->pixel_size.height / (bitmap->dpi_y / 96.0f);
    return size;
}

static D2D1_SIZE_U * STDMETHODCALLTYPE d2d_bitmap_GetPixelSize(ID2D1Bitmap1 *iface, D2D1_SIZE_U *pixel_size)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);

    TRACE("iface %p, pixel_size %p.\n", iface, pixel_size);

    *pixel_size = bitmap->pixel_size;
    return pixel_size;
}

static D2D1_PIXEL_FORMAT * STDMETHODCALLTYPE d2d_bitmap_GetPixelFormat(ID2D1Bitmap1 *iface, D2D1_PIXEL_FORMAT *format)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);

    TRACE("iface %p, format %p.\n", iface, format);

    *format = bitmap->format;
    return format;
}

static void STDMETHODCALLTYPE d2d_bitmap_GetDpi(ID2D1Bitmap1 *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    *dpi_x = bitmap->dpi_x;
    *dpi_y = bitmap->dpi_y;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_CopyFromBitmap(ID2D1Bitmap1 *iface,
        const D2D1_POINT_2U *dst_point, ID2D1Bitmap *bitmap, const D2D1_RECT_U *src_rect)
{
    FIXME("iface %p, dst_point %p, bitmap %p, src_rect %p stub!\n", iface, dst_point, bitmap, src_rect);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_CopyFromRenderTarget(ID2D1Bitmap1 *iface,
        const D2D1_POINT_2U *dst_point, ID2D1RenderTarget *render_target, const D2D1_RECT_U *src_rect)
{
    FIXME("iface %p, dst_point %p, render_target %p, src_rect %p stub!\n", iface, dst_point, render_target, src_rect);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_CopyFromMemory(ID2D1Bitmap1 *iface,
        const D2D1_RECT_U *dst_rect, const void *src_data, UINT32 pitch)
{
    struct d2d_bitmap *bitmap = impl_from_ID2D1Bitmap1(iface);
    ID3D10Device *device;
    ID3D10Resource *dst;
    D3D10_BOX box;

    TRACE("iface %p, dst_rect %p, src_data %p, pitch %u.\n", iface, dst_rect, src_data, pitch);

    if (dst_rect)
    {
        box.left = dst_rect->left;
        box.top = dst_rect->top;
        box.front = 0;
        box.right = dst_rect->right;
        box.bottom = dst_rect->bottom;
        box.back = 1;
    }

    ID3D10ShaderResourceView_GetResource(bitmap->view, &dst);
    ID3D10ShaderResourceView_GetDevice(bitmap->view, &device);
    ID3D10Device_UpdateSubresource(device, dst, 0, dst_rect ? &box : NULL, src_data, pitch, 0);
    ID3D10Device_Release(device);
    ID3D10Resource_Release(dst);

    return S_OK;
}

static void STDMETHODCALLTYPE d2d_bitmap_GetColorContext(ID2D1Bitmap1 *iface, ID2D1ColorContext **context)
{
    FIXME("iface %p, context %p stub!\n", iface, context);
}

static D2D1_BITMAP_OPTIONS STDMETHODCALLTYPE d2d_bitmap_GetOptions(ID2D1Bitmap1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_BITMAP_OPTIONS_NONE;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_GetSurface(ID2D1Bitmap1 *iface, IDXGISurface **surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_Map(ID2D1Bitmap1 *iface, D2D1_MAP_OPTIONS options,
        D2D1_MAPPED_RECT *mapped_rect)
{
    FIXME("iface %p, options %#x, mapped_rect %p stub!\n", iface, options, mapped_rect);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_Unmap(ID2D1Bitmap1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const struct ID2D1Bitmap1Vtbl d2d_bitmap_vtbl =
{
    d2d_bitmap_QueryInterface,
    d2d_bitmap_AddRef,
    d2d_bitmap_Release,
    d2d_bitmap_GetFactory,
    d2d_bitmap_GetSize,
    d2d_bitmap_GetPixelSize,
    d2d_bitmap_GetPixelFormat,
    d2d_bitmap_GetDpi,
    d2d_bitmap_CopyFromBitmap,
    d2d_bitmap_CopyFromRenderTarget,
    d2d_bitmap_CopyFromMemory,
    d2d_bitmap_GetColorContext,
    d2d_bitmap_GetOptions,
    d2d_bitmap_GetSurface,
    d2d_bitmap_Map,
    d2d_bitmap_Unmap,
};

static BOOL format_supported(const D2D1_PIXEL_FORMAT *format)
{
    unsigned int i;

    static const D2D1_PIXEL_FORMAT supported_formats[] =
    {
        {DXGI_FORMAT_R32G32B32A32_FLOAT,    D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_R32G32B32A32_FLOAT,    D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_R16G16B16A16_FLOAT,    D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,    D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_R16G16B16A16_UNORM,    D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_R16G16B16A16_UNORM,    D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_R8G8B8A8_UNORM,        D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_R8G8B8A8_UNORM,        D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_A8_UNORM,              D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_A8_UNORM,              D2D1_ALPHA_MODE_STRAIGHT     },
        {DXGI_FORMAT_B8G8R8A8_UNORM,        D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_B8G8R8A8_UNORM,        D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_B8G8R8X8_UNORM,        D2D1_ALPHA_MODE_IGNORE       },
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,   D2D1_ALPHA_MODE_PREMULTIPLIED},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,   D2D1_ALPHA_MODE_IGNORE       },
    };

    for (i = 0; i < ARRAY_SIZE(supported_formats); ++i)
    {
        if (supported_formats[i].format == format->format
                && supported_formats[i].alphaMode == format->alphaMode)
            return TRUE;
    }

    return FALSE;
}

static void d2d_bitmap_init(struct d2d_bitmap *bitmap, ID2D1Factory *factory,
        ID3D10ShaderResourceView *view, D2D1_SIZE_U size, const D2D1_BITMAP_PROPERTIES *desc)
{
    bitmap->ID2D1Bitmap1_iface.lpVtbl = &d2d_bitmap_vtbl;
    bitmap->refcount = 1;
    ID2D1Factory_AddRef(bitmap->factory = factory);
    ID3D10ShaderResourceView_AddRef(bitmap->view = view);
    bitmap->pixel_size = size;
    bitmap->format = desc->pixelFormat;
    bitmap->dpi_x = desc->dpiX;
    bitmap->dpi_y = desc->dpiY;

    if (bitmap->dpi_x == 0.0f && bitmap->dpi_y == 0.0f)
    {
        bitmap->dpi_x = 96.0f;
        bitmap->dpi_y = 96.0f;
    }
}

HRESULT d2d_bitmap_create(ID2D1Factory *factory, ID3D10Device *device, D2D1_SIZE_U size, const void *src_data,
        UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc, struct d2d_bitmap **bitmap)
{
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10ShaderResourceView *view;
    ID3D10Texture2D *texture;
    HRESULT hr;

    if (!format_supported(&desc->pixelFormat))
    {
        WARN("Tried to create bitmap with unsupported format {%#x / %#x}.\n",
                desc->pixelFormat.format, desc->pixelFormat.alphaMode);
        return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    texture_desc.Width = size.width;
    texture_desc.Height = size.height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = desc->pixelFormat.format;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    resource_data.pSysMem = src_data;
    resource_data.SysMemPitch = pitch;

    if (FAILED(hr = ID3D10Device_CreateTexture2D(device, &texture_desc,
            src_data ? &resource_data : NULL, &texture)))
    {
        ERR("Failed to create texture, hr %#x.\n", hr);
        return hr;
    }

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture, NULL, &view);
    ID3D10Texture2D_Release(texture);
    if (FAILED(hr))
    {
        ERR("Failed to create view, hr %#x.\n", hr);
        return hr;
    }

    if ((*bitmap = heap_alloc_zero(sizeof(**bitmap))))
    {
        d2d_bitmap_init(*bitmap, factory, view, size, desc);
        TRACE("Created bitmap %p.\n", *bitmap);
    }

    ID3D10ShaderResourceView_Release(view);

    return *bitmap ? S_OK : E_OUTOFMEMORY;
}

HRESULT d2d_bitmap_create_shared(ID2D1RenderTarget *render_target, ID3D10Device *target_device,
        REFIID iid, void *data, const D2D1_BITMAP_PROPERTIES *desc, struct d2d_bitmap **bitmap)
{
    D2D1_BITMAP_PROPERTIES d;
    ID2D1Factory *factory;

    if (IsEqualGUID(iid, &IID_ID2D1Bitmap))
    {
        struct d2d_bitmap *src_impl = unsafe_impl_from_ID2D1Bitmap(data);
        ID3D10Device *device;
        HRESULT hr = S_OK;

        ID2D1RenderTarget_GetFactory(render_target, &factory);
        if (src_impl->factory != factory)
        {
            hr = D2DERR_WRONG_FACTORY;
            goto failed;
        }

        ID3D10ShaderResourceView_GetDevice(src_impl->view, &device);
        ID3D10Device_Release(device);
        if (device != target_device)
        {
            hr = D2DERR_UNSUPPORTED_OPERATION;
            goto failed;
        }

        if (!desc)
        {
            d.pixelFormat = src_impl->format;
            d.dpiX = src_impl->dpi_x;
            d.dpiY = src_impl->dpi_y;
            desc = &d;
        }

        if (!format_supported(&desc->pixelFormat))
        {
            WARN("Tried to create bitmap with unsupported format {%#x / %#x}.\n",
                    desc->pixelFormat.format, desc->pixelFormat.alphaMode);
            hr = D2DERR_UNSUPPORTED_PIXEL_FORMAT;
            goto failed;
        }

        if (!(*bitmap = heap_alloc_zero(sizeof(**bitmap))))
        {
            hr = E_OUTOFMEMORY;
            goto failed;
        }

        d2d_bitmap_init(*bitmap, factory, src_impl->view, src_impl->pixel_size, desc);
        TRACE("Created bitmap %p.\n", *bitmap);

    failed:
        ID2D1Factory_Release(factory);
        return hr;
    }

    if (IsEqualGUID(iid, &IID_IDXGISurface) || IsEqualGUID(iid, &IID_IDXGISurface1))
    {
        ID3D10ShaderResourceView *view;
        DXGI_SURFACE_DESC surface_desc;
        IDXGISurface *surface = data;
        ID3D10Resource *resource;
        D2D1_SIZE_U pixel_size;
        ID3D10Device *device;
        HRESULT hr;

        if (FAILED(IDXGISurface_QueryInterface(surface, &IID_ID3D10Resource, (void **)&resource)))
        {
            WARN("Failed to get d3d resource from dxgi surface.\n");
            return E_FAIL;
        }

        ID3D10Resource_GetDevice(resource, &device);
        ID3D10Device_Release(device);
        if (device != target_device)
        {
            ID3D10Resource_Release(resource);
            return D2DERR_UNSUPPORTED_OPERATION;
        }

        hr = ID3D10Device_CreateShaderResourceView(target_device, resource, NULL, &view);
        ID3D10Resource_Release(resource);
        if (FAILED(hr))
        {
            WARN("Failed to create shader resource view, hr %#x.\n", hr);
            return hr;
        }

        if (!(*bitmap = heap_alloc_zero(sizeof(**bitmap))))
        {
            ID3D10ShaderResourceView_Release(view);
            return E_OUTOFMEMORY;
        }

        d = *desc;
        if (d.dpiX == 0.0f || d.dpiY == 0.0f)
        {
            float dpi_x, dpi_y;

            ID2D1RenderTarget_GetDpi(render_target, &dpi_x, &dpi_y);
            if (d.dpiX == 0.0f)
                d.dpiX = dpi_x;
            if (d.dpiY == 0.0f)
                d.dpiY = dpi_y;
        }

        if (FAILED(hr = IDXGISurface_GetDesc(surface, &surface_desc)))
        {
            WARN("Failed to get surface desc, hr %#x.\n", hr);
            ID3D10ShaderResourceView_Release(view);
            return hr;
        }

        pixel_size.width = surface_desc.Width;
        pixel_size.height = surface_desc.Height;

        ID2D1RenderTarget_GetFactory(render_target, &factory);
        d2d_bitmap_init(*bitmap, factory, view, pixel_size, &d);
        ID3D10ShaderResourceView_Release(view);
        ID2D1Factory_Release(factory);
        TRACE("Created bitmap %p.\n", *bitmap);

        return S_OK;
    }

    WARN("Unhandled interface %s.\n", debugstr_guid(iid));

    return E_INVALIDARG;
}

HRESULT d2d_bitmap_create_from_wic_bitmap(ID2D1Factory *factory, ID3D10Device *device, IWICBitmapSource *bitmap_source,
        const D2D1_BITMAP_PROPERTIES *desc, struct d2d_bitmap **bitmap)
{
    const D2D1_PIXEL_FORMAT *d2d_format;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    WICPixelFormatGUID wic_format;
    unsigned int bpp, data_size;
    D2D1_SIZE_U size;
    unsigned int i;
    WICRect rect;
    UINT32 pitch;
    HRESULT hr;
    void *data;

    static const struct
    {
        const WICPixelFormatGUID *wic;
        D2D1_PIXEL_FORMAT d2d;
    }
    format_lookup[] =
    {
        {&GUID_WICPixelFormat32bppPBGRA, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {&GUID_WICPixelFormat32bppBGR,   {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
    };

    if (FAILED(hr = IWICBitmapSource_GetSize(bitmap_source, &size.width, &size.height)))
    {
        WARN("Failed to get bitmap size, hr %#x.\n", hr);
        return hr;
    }

    if (!desc)
    {
        bitmap_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
        bitmap_desc.dpiX = 0.0f;
        bitmap_desc.dpiY = 0.0f;
    }
    else
    {
        bitmap_desc = *desc;
    }

    if (FAILED(hr = IWICBitmapSource_GetPixelFormat(bitmap_source, &wic_format)))
    {
        WARN("Failed to get bitmap format, hr %#x.\n", hr);
        return hr;
    }

    for (i = 0, d2d_format = NULL; i < ARRAY_SIZE(format_lookup); ++i)
    {
        if (IsEqualGUID(&wic_format, format_lookup[i].wic))
        {
            d2d_format = &format_lookup[i].d2d;
            break;
        }
    }

    if (!d2d_format)
    {
        WARN("Unsupported WIC bitmap format %s.\n", debugstr_guid(&wic_format));
        return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    if (bitmap_desc.pixelFormat.format == DXGI_FORMAT_UNKNOWN)
        bitmap_desc.pixelFormat.format = d2d_format->format;
    if (bitmap_desc.pixelFormat.alphaMode == D2D1_ALPHA_MODE_UNKNOWN)
        bitmap_desc.pixelFormat.alphaMode = d2d_format->alphaMode;

    switch (bitmap_desc.pixelFormat.format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            bpp = 4;
            break;

        default:
            FIXME("Unhandled format %#x.\n", bitmap_desc.pixelFormat.format);
            return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    pitch = ((bpp * size.width) + 15) & ~15;
    if (pitch / bpp < size.width)
        return E_OUTOFMEMORY;
    if (!(data = heap_calloc(size.height, pitch)))
        return E_OUTOFMEMORY;
    data_size = size.height * pitch;

    rect.X = 0;
    rect.Y = 0;
    rect.Width = size.width;
    rect.Height = size.height;
    if (FAILED(hr = IWICBitmapSource_CopyPixels(bitmap_source, &rect, pitch, data_size, data)))
    {
        WARN("Failed to copy bitmap pixels, hr %#x.\n", hr);
        heap_free(data);
        return hr;
    }

    hr = d2d_bitmap_create(factory, device, size, data, pitch, &bitmap_desc, bitmap);

    heap_free(data);

    return hr;
}

struct d2d_bitmap *unsafe_impl_from_ID2D1Bitmap(ID2D1Bitmap *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID2D1BitmapVtbl *)&d2d_bitmap_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_bitmap, ID2D1Bitmap1_iface);
}
