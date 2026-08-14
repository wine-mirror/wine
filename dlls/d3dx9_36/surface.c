/*
 * Copyright (C) 2009-2010 Tony Wasserka
 * Copyright (C) 2012 Józef Kucia
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


#include "d3dx9_private.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT lock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect, D3DLOCKED_RECT *lock,
        IDirect3DSurface9 **temp_surface, BOOL write)
{
    unsigned int width, height;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    DWORD lock_flag;
    HRESULT hr;

    lock_flag = write ? 0 : D3DLOCK_READONLY;
    *temp_surface = NULL;
    if (FAILED(hr = IDirect3DSurface9_LockRect(surface, lock, surface_rect, lock_flag)))
    {
        IDirect3DSurface9_GetDevice(surface, &device);
        IDirect3DSurface9_GetDesc(surface, &desc);

        if (surface_rect)
        {
            width = surface_rect->right - surface_rect->left;
            height = surface_rect->bottom - surface_rect->top;
        }
        else
        {
            width = desc.Width;
            height = desc.Height;
        }

        hr = write ? IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height,
                desc.Format, D3DPOOL_SYSTEMMEM, temp_surface, NULL)
                : IDirect3DDevice9_CreateRenderTarget(device, width, height,
                desc.Format, D3DMULTISAMPLE_NONE, 0, TRUE, temp_surface, NULL);
        if (FAILED(hr))
        {
            WARN("Failed to create temporary surface, surface %p, format %#x, "
                    "usage %#lx, pool %#x, write %#x, width %u, height %u.\n",
                    surface, desc.Format, desc.Usage, desc.Pool, write, width, height);
            IDirect3DDevice9_Release(device);
            return hr;
        }

        if (write || SUCCEEDED(hr = IDirect3DDevice9_StretchRect(device, surface, surface_rect,
                *temp_surface, NULL, D3DTEXF_NONE)))
            hr = IDirect3DSurface9_LockRect(*temp_surface, lock, NULL, lock_flag);

        IDirect3DDevice9_Release(device);
        if (FAILED(hr))
        {
            WARN("Failed to lock surface %p, write %#x, usage %#lx, pool %#x.\n",
                    surface, write, desc.Usage, desc.Pool);
            IDirect3DSurface9_Release(*temp_surface);
            *temp_surface = NULL;
            return hr;
        }
        TRACE("Created temporary surface %p.\n", surface);
    }
    return hr;
}

HRESULT unlock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect,
        IDirect3DSurface9 *temp_surface, BOOL update)
{
    IDirect3DDevice9 *device;
    POINT surface_point;
    HRESULT hr;

    if (!temp_surface)
    {
        hr = IDirect3DSurface9_UnlockRect(surface);
        return hr;
    }

    hr = IDirect3DSurface9_UnlockRect(temp_surface);
    if (update)
    {
        if (surface_rect)
        {
            surface_point.x = surface_rect->left;
            surface_point.y = surface_rect->top;
        }
        else
        {
            surface_point.x = 0;
            surface_point.y = 0;
        }
        IDirect3DSurface9_GetDevice(surface, &device);
        if (FAILED(hr = IDirect3DDevice9_UpdateSurface(device, temp_surface, NULL, surface, &surface_point)))
            WARN("Updating surface failed, hr %#lx, surface %p, temp_surface %p.\n",
                    hr, surface, temp_surface);
        IDirect3DDevice9_Release(device);
    }
    IDirect3DSurface9_Release(temp_surface);
    return hr;
}

/************************************************************
 * D3DXGetImageInfoFromFileInMemory
 *
 * Fills a D3DXIMAGE_INFO structure with info about an image
 *
 * PARAMS
 *   data     [I] pointer to the image file data
 *   datasize [I] size of the passed data
 *   info     [O] pointer to the destination structure
 *
 * RETURNS
 *   Success: D3D_OK, if info is not NULL and data and datasize make up a valid image file or
 *                    if info is NULL and data and datasize are not NULL
 *   Failure: D3DXERR_INVALIDDATA, if data is no valid image file and datasize and info are not NULL
 *            D3DERR_INVALIDCALL, if data is NULL or
 *                                if datasize is 0
 *
 * NOTES
 *   datasize may be bigger than the actual file size
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(const void *data, UINT datasize, D3DXIMAGE_INFO *info)
{
    struct d3dx_image image;
    HRESULT hr;

    TRACE("(%p, %d, %p)\n", data, datasize, info);

    if (!data || !datasize)
        return D3DERR_INVALIDCALL;

    if (!info)
        return D3D_OK;

    hr = d3dx_image_init(data, datasize, &image, 0, D3DX_IMAGE_INFO_ONLY);
    if (FAILED(hr)) {
        TRACE("Invalid or unsupported image file\n");
        return D3DXERR_INVALIDDATA;
    }

    if (!d3dximage_info_from_d3dx_image(info, &image))
        return D3DXERR_INVALIDDATA;
    return D3D_OK;
}

/************************************************************
 * D3DXGetImageInfoFromFile
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load a valid image file or
 *                    if we successfully load a file which is no valid image and info is NULL
 *   Failure: D3DXERR_INVALIDDATA, if we fail to load file or
 *                                 if file is not a valid image file and info is not NULL
 *            D3DERR_INVALIDCALL, if file is NULL
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileA(const char *file, D3DXIMAGE_INFO *info)
{
    WCHAR *widename;
    HRESULT hr;
    int strlength;

    TRACE("file %s, info %p.\n", debugstr_a(file), info);

    if( !file ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
    widename = malloc(strlength * sizeof(*widename));
    MultiByteToWideChar(CP_ACP, 0, file, -1, widename, strlength);

    hr = D3DXGetImageInfoFromFileW(widename, info);
    free(widename);

    return hr;
}

HRESULT WINAPI D3DXGetImageInfoFromFileW(const WCHAR *file, D3DXIMAGE_INFO *info)
{
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("file %s, info %p.\n", debugstr_w(file), info);

    if (!file)
        return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(file, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    UnmapViewOfFile(buffer);

    return hr;
}

/************************************************************
 * D3DXGetImageInfoFromResource
 *
 * RETURNS
 *   Success: D3D_OK, if resource is a valid image file
 *   Failure: D3DXERR_INVALIDDATA, if resource is no valid image file or NULL or
 *                                 if we fail to load resource
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, const char *resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, resource %s, info %p.\n", module, debugstr_a(resource), info);

    if (!(resinfo = FindResourceA(module, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(module, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(module, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
}

HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, resource %s, info %p.\n", module, debugstr_w(resource), info);

    if (!(resinfo = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(module, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(module, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
}

static HRESULT d3dx_load_surface_from_memory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        enum d3dx_pixel_format_id src_format, uint32_t src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_desc, *dst_desc;
    struct d3dx_pixels src_pixels, dst_pixels;
    RECT dst_rect_tmp, dst_rect_aligned;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT lock_rect;
    D3DSURFACE_DESC desc;
    HRESULT hr;

    IDirect3DSurface9_GetDesc(dst_surface, &desc);
    if (desc.MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        TRACE("Multisampled destination surface, doing nothing.\n");
        return D3D_OK;
    }

    dst_desc = get_format_info(desc.Format);
    if (!dst_rect)
    {
        SetRect(&dst_rect_tmp, 0, 0, desc.Width, desc.Height);
        dst_rect = &dst_rect_tmp;
    }
    else
    {
        if (dst_rect->left > dst_rect->right || dst_rect->right > desc.Width
                || dst_rect->top > dst_rect->bottom || dst_rect->bottom > desc.Height
                || dst_rect->left < 0 || dst_rect->top < 0)
        {
            WARN("Invalid dst_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
        if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
        {
            WARN("Empty dst_rect specified.\n");
            return D3D_OK;
        }
    }

    src_desc = get_d3dx_pixel_format_info(src_format);
    hr = d3dx_pixels_init(src_memory, src_pitch, 0, src_palette, src_desc->format,
            src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1, &src_pixels);
    if (FAILED(hr))
        return hr;

    get_aligned_rect(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, desc.Width, desc.Height,
        dst_desc, &dst_rect_aligned);
    if (FAILED(hr = lock_surface(dst_surface, &dst_rect_aligned, &lock_rect, &surface, TRUE)))
        return hr;

    set_d3dx_pixels(&dst_pixels, lock_rect.pBits, lock_rect.Pitch, 0, dst_palette,
            (dst_rect_aligned.right - dst_rect_aligned.left), (dst_rect_aligned.bottom - dst_rect_aligned.top), 1,
            dst_rect);
    OffsetRect(&dst_pixels.unaligned_rect, -dst_rect_aligned.left, -dst_rect_aligned.top);

    if (FAILED(hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_desc, &src_pixels, src_desc, filter, color_key)))
    {
        unlock_surface(dst_surface, &dst_rect_aligned, surface, FALSE);
        return hr;
    }

    return unlock_surface(dst_surface, &dst_rect_aligned, surface, TRUE);
}

/************************************************************
 * D3DXLoadSurfaceFromFileInMemory
 *
 * Loads data from a given buffer into a surface and fills a given
 * D3DXIMAGE_INFO structure with info about the source data.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcData     [I] pointer to the source data
 *   SrcDataSize  [I] size of the source data in bytes
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *   pSrcInfo     [O] pointer to a D3DXIMAGE_INFO structure
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, pSrcData or SrcDataSize is NULL
 *            D3DXERR_INVALIDDATA, if pSrcData is no valid image file
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileInMemory(IDirect3DSurface9 *pDestSurface,
        const PALETTEENTRY *pDestPalette, const RECT *pDestRect, const void *pSrcData, UINT SrcDataSize,
        const RECT *pSrcRect, DWORD dwFilter, D3DCOLOR Colorkey, D3DXIMAGE_INFO *pSrcInfo)
{
    struct d3dx_pixels pixels = { 0 };
    struct d3dx_image image;
    D3DXIMAGE_INFO img_info;
    RECT src_rect;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_data %p, src_data_size %u, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            pDestSurface, pDestPalette, wine_dbgstr_rect(pDestRect), pSrcData, SrcDataSize,
            wine_dbgstr_rect(pSrcRect), dwFilter, Colorkey, pSrcInfo);

    if (!pDestSurface || !pSrcData || !SrcDataSize)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&dwFilter)))
        return hr;

    hr = d3dx_image_init(pSrcData, SrcDataSize, &image, 0, 0);
    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    if (!d3dximage_info_from_d3dx_image(&img_info, &image))
    {
        hr = D3DXERR_INVALIDDATA;
        goto exit;
    }

    if (pSrcRect)
        src_rect = *pSrcRect;
    else
        SetRect(&src_rect, 0, 0, img_info.Width, img_info.Height);

    hr = d3dx_image_get_pixels(&image, 0, 0, &pixels);
    if (FAILED(hr))
        goto exit;

    hr = d3dx_load_surface_from_memory(pDestSurface, pDestPalette, pDestRect, pixels.data, image.format, pixels.row_pitch,
            pixels.palette, &src_rect, dwFilter, Colorkey);
    if (SUCCEEDED(hr) && pSrcInfo)
        *pSrcInfo = img_info;

exit:
    d3dx_image_cleanup(&image);
    return FAILED(hr) ? D3DXERR_INVALIDDATA : D3D_OK;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileA(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const char *src_file,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    WCHAR *src_file_w;
    HRESULT hr;
    int strlength;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_file %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), debugstr_a(src_file),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!src_file || !dst_surface)
        return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0);
    src_file_w = malloc(strlength * sizeof(*src_file_w));
    MultiByteToWideChar(CP_ACP, 0, src_file, -1, src_file_w, strlength);

    hr = D3DXLoadSurfaceFromFileW(dst_surface, dst_palette, dst_rect,
            src_file_w, src_rect, filter, color_key, src_info);
    free(src_file_w);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileW(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const WCHAR *src_file,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    DWORD data_size;
    void *data;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_file %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), debugstr_w(src_file),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!src_file || !dst_surface)
        return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(src_file, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
    UnmapViewOfFile(data);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceA(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, HMODULE src_module, const char *resource,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    DWORD data_size;
    HRSRC resinfo;
    void *data;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_module %p, resource %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_module, debugstr_a(resource),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!dst_surface)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceA(src_module, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(src_module, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(src_module, resinfo, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    return D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceW(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, HMODULE src_module, const WCHAR *resource,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    DWORD data_size;
    HRSRC resinfo;
    void *data;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_module %p, resource %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_module, debugstr_w(resource),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!dst_surface)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceW(src_module, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(src_module, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(src_module, resinfo, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    return D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect is NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN) or
 *                                if DestRect is invalid
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_desc;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_memory %p, src_format %#x, "
            "src_pitch %u, src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_memory, src_format,
            src_pitch, src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_memory || !src_rect)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    if (src_format == D3DFMT_UNKNOWN
            || src_rect->left >= src_rect->right
            || src_rect->top >= src_rect->bottom)
    {
        WARN("Invalid src_format or src_rect.\n");
        return E_FAIL;
    }

    src_desc = get_format_info(src_format);
    if (is_unknown_format(src_desc))
    {
        FIXME("Unsupported format %#x.\n", src_format);
        return E_NOTIMPL;
    }

    return d3dx_load_surface_from_memory(dst_surface, dst_palette, dst_rect, src_memory, src_desc->format, src_pitch,
            src_palette, src_rect, filter, color_key);
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface is NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
        const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_format_desc, *dst_format_desc;
    D3DSURFACE_DESC src_desc, dst_desc;
    struct volume src_size, dst_size;
    IDirect3DSurface9 *temp_surface;
    D3DTEXTUREFILTERTYPE d3d_filter;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lock;
    RECT dst_rect_temp;
    HRESULT hr;
    RECT s;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_surface %p, "
            "src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_surface,
            src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_surface)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    IDirect3DSurface9_GetDesc(src_surface, &src_desc);
    src_format_desc = get_format_info(src_desc.Format);
    if (!src_rect)
    {
        SetRect(&s, 0, 0, src_desc.Width, src_desc.Height);
        src_rect = &s;
    }
    else if (src_rect->left == src_rect->right || src_rect->top == src_rect->bottom)
    {
        WARN("Empty src_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (src_rect->left > src_rect->right || src_rect->right > src_desc.Width
            || src_rect->left < 0 || src_rect->left > src_desc.Width
            || src_rect->top > src_rect->bottom || src_rect->bottom > src_desc.Height
            || src_rect->top < 0 || src_rect->top > src_desc.Height)
    {
        WARN("Invalid src_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    IDirect3DSurface9_GetDesc(dst_surface, &dst_desc);
    dst_format_desc = get_format_info(dst_desc.Format);
    if (!dst_rect)
    {
        SetRect(&dst_rect_temp, 0, 0, dst_desc.Width, dst_desc.Height);
        dst_rect = &dst_rect_temp;
    }
    else if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
    {
        WARN("Empty dst_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (dst_rect->left > dst_rect->right || dst_rect->right > dst_desc.Width
            || dst_rect->left < 0 || dst_rect->left > dst_desc.Width
            || dst_rect->top > dst_rect->bottom || dst_rect->bottom > dst_desc.Height
            || dst_rect->top < 0 || dst_rect->top > dst_desc.Height)
    {
        WARN("Invalid dst_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;

    if (!dst_palette && !src_palette && !color_key)
    {
        if (src_desc.Format == dst_desc.Format
                && dst_size.width == src_size.width
                && dst_size.height == src_size.height
                && color_key == 0
                && !(src_rect->left & (src_format_desc->block_width - 1))
                && !(src_rect->top & (src_format_desc->block_height - 1))
                && !(dst_rect->left & (dst_format_desc->block_width - 1))
                && !(dst_rect->top & (dst_format_desc->block_height - 1)))
        {
            d3d_filter = D3DTEXF_NONE;
        }
        else
        {
            switch (filter)
            {
                case D3DX_FILTER_NONE:
                    d3d_filter = D3DTEXF_NONE;
                    break;

                case D3DX_FILTER_POINT:
                    d3d_filter = D3DTEXF_POINT;
                    break;

                case D3DX_FILTER_LINEAR:
                    d3d_filter = D3DTEXF_LINEAR;
                    break;

                default:
                    d3d_filter = D3DTEXF_FORCE_DWORD;
                    break;
            }
        }

        if (d3d_filter != D3DTEXF_FORCE_DWORD)
        {
            IDirect3DSurface9_GetDevice(src_surface, &device);
            hr = IDirect3DDevice9_StretchRect(device, src_surface, src_rect, dst_surface, dst_rect, d3d_filter);
            IDirect3DDevice9_Release(device);
            if (SUCCEEDED(hr))
                return D3D_OK;
        }
    }

    if (FAILED(lock_surface(src_surface, NULL, &lock, &temp_surface, FALSE)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, lock.pBits,
            src_desc.Format, lock.Pitch, src_palette, src_rect, filter, color_key);

    if (FAILED(unlock_surface(src_surface, NULL, temp_surface, FALSE)))
        return D3DXERR_INVALIDDATA;

    return hr;
}


HRESULT WINAPI D3DXSaveSurfaceToFileA(const char *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    int len;
    WCHAR *filename;
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p, %s): relay\n",
            wine_dbgstr_a(dst_filename), file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, NULL, 0);
    filename = malloc(len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, filename, len);

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, file_format, src_surface, src_palette, src_rect);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    free(filename);
    return hr;
}

HRESULT WINAPI D3DXSaveSurfaceToFileW(const WCHAR *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p, %s): relay\n",
        wine_dbgstr_w(dst_filename), file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_filename) return D3DERR_INVALIDCALL;

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, file_format, src_surface, src_palette, src_rect);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(dst_filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    return hr;
}

HRESULT WINAPI D3DXSaveSurfaceToFileInMemory(ID3DXBuffer **dst_buffer, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    const struct pixel_format_desc *src_fmt_desc;
    D3DSURFACE_DESC src_surface_desc;
    IDirect3DSurface9 *temp_surface;
    struct d3dx_pixels src_pixels;
    D3DLOCKED_RECT locked_rect;
    ID3DXBuffer *buffer;
    RECT src_rect_temp;
    HRESULT hr;

    TRACE("dst_buffer %p, file_format %#x, src_surface %p, src_palette %p, src_rect %s.\n",
        dst_buffer, file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_buffer || !src_surface || file_format > D3DXIFF_PFM)
        return D3DERR_INVALIDCALL;

    switch (file_format)
    {
        case D3DXIFF_HDR:
        case D3DXIFF_PFM:
        case D3DXIFF_PPM:
            FIXME("File format %#x is not supported yet.\n", file_format);
            return E_NOTIMPL;

        default:
            break;
    }

    IDirect3DSurface9_GetDesc(src_surface, &src_surface_desc);
    src_fmt_desc = get_format_info(src_surface_desc.Format);
    if (is_unknown_format(src_fmt_desc))
        return E_NOTIMPL;

    if (!src_palette && is_index_format(src_fmt_desc))
    {
        FIXME("Default palette unimplemented.\n");
        return E_NOTIMPL;
    }

    if (src_rect)
    {
        if (src_rect->left > src_rect->right || src_rect->right > src_surface_desc.Width
                || src_rect->top > src_rect->bottom || src_rect->bottom > src_surface_desc.Height
                || src_rect->left < 0 || src_rect->top < 0)
        {
            WARN("Invalid src_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
    }
    else
    {
        SetRect(&src_rect_temp, 0, 0, src_surface_desc.Width, src_surface_desc.Height);
        src_rect = &src_rect_temp;
    }

    hr = lock_surface(src_surface, NULL, &locked_rect, &temp_surface, FALSE);
    if (FAILED(hr))
        return hr;

    hr = d3dx_pixels_init(locked_rect.pBits, locked_rect.Pitch, 0, src_palette, src_fmt_desc->format,
            src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1, &src_pixels);
    if (FAILED(hr))
    {
        unlock_surface(src_surface, NULL, temp_surface, FALSE);
        return hr;
    }

    hr = d3dx9_save_pixels_to_memory(&src_pixels, src_fmt_desc, file_format, &buffer);
    if (FAILED(hr))
    {
        unlock_surface(src_surface, NULL, temp_surface, FALSE);
        return hr;
    }

    hr = unlock_surface(src_surface, NULL, temp_surface, FALSE);
    if (FAILED(hr))
    {
        ID3DXBuffer_Release(buffer);
        return hr;
    }

    *dst_buffer = buffer;
    return D3D_OK;
}
