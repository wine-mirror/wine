/*
 * Copyright 2009 Tony Wasserka
 * Copyright 2010 Christian Costa
 * Copyright 2010 Owen Rudge for CodeWeavers
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

#include "wine/unicode.h"
#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/* Returns TRUE if num is a power of 2, FALSE if not, or if 0 */
static BOOL is_pow2(UINT num)
{
    return !(num & (num - 1));
}

/* Returns the smallest power of 2 which is greater than or equal to num */
static UINT make_pow2(UINT num)
{
    UINT result = 1;

    /* In the unlikely event somebody passes a large value, make sure we don't enter an infinite loop */
    if (num >= 0x80000000)
        return 0x80000000;

    while (result < num)
        result <<= 1;

    return result;
}

HRESULT WINAPI D3DXFilterTexture(LPDIRECT3DBASETEXTURE9 texture,
                                 CONST PALETTEENTRY *palette,
                                 UINT srclevel,
                                 DWORD filter)
{
    UINT level = srclevel + 1;
    HRESULT hr;

    TRACE("(%p, %p, %d, %d)\n", texture, palette, srclevel, filter);

    if (!texture)
        return D3DERR_INVALIDCALL;

    if ((filter & 0xFFFF) > D3DX_FILTER_BOX && filter != D3DX_DEFAULT)
        return D3DERR_INVALIDCALL;

    if (srclevel >= IDirect3DBaseTexture9_GetLevelCount(texture))
        return D3DERR_INVALIDCALL;

    switch (IDirect3DBaseTexture9_GetType(texture))
    {
        case D3DRTYPE_TEXTURE:
        {
            IDirect3DSurface9 *topsurf, *mipsurf;
            D3DSURFACE_DESC desc;

            if (filter == D3DX_DEFAULT)
            {
                IDirect3DTexture9_GetLevelDesc((IDirect3DTexture9*) texture, srclevel, &desc);

                if (is_pow2(desc.Width) && is_pow2(desc.Height))
                    filter = D3DX_FILTER_BOX;
                else
                    filter = D3DX_FILTER_BOX | D3DX_FILTER_DITHER;
            }

            hr = IDirect3DTexture9_GetSurfaceLevel((IDirect3DTexture9*) texture, srclevel, &topsurf);

            if (FAILED(hr))
                return D3DERR_INVALIDCALL;

            while (IDirect3DTexture9_GetSurfaceLevel((IDirect3DTexture9*) texture, level, &mipsurf) == D3D_OK)
            {
                hr = D3DXLoadSurfaceFromSurface(mipsurf, palette, NULL, topsurf, palette, NULL, filter, 0);
                IDirect3DSurface9_Release(mipsurf);

                if (FAILED(hr))
                    break;

                level++;
            }

            IDirect3DSurface9_Release(topsurf);

            if (level == srclevel + 1)
                return D3DERR_INVALIDCALL;

            return D3D_OK;
        }

        default:
            FIXME("Implement volume and cube texture filtering\n");
            return E_NOTIMPL;
    }
}

HRESULT WINAPI D3DXCheckTextureRequirements(LPDIRECT3DDEVICE9 device,
                                            UINT* width,
                                            UINT* height,
                                            UINT* miplevels,
                                            DWORD usage,
                                            D3DFORMAT* format,
                                            D3DPOOL pool)
{
    UINT w = (width && *width) ? *width : 1;
    UINT h = (height && *height) ? *height : 1;
    D3DCAPS9 caps;

    TRACE("(%p, %p, %p, %p, %u, %p, %u)\n", device, width, height, miplevels, usage, format, pool);

    if (!device)
        return D3DERR_INVALIDCALL;

    /* usage */
    if ((usage != D3DX_DEFAULT) &&
        (usage & (D3DUSAGE_WRITEONLY | D3DUSAGE_DONOTCLIP | D3DUSAGE_POINTS | D3DUSAGE_RTPATCHES | D3DUSAGE_NPATCHES)))
        return D3DERR_INVALIDCALL;

    /* pool */
    if ((pool != D3DPOOL_DEFAULT) && (pool != D3DPOOL_MANAGED) && (pool != D3DPOOL_SYSTEMMEM) && (pool != D3DPOOL_SCRATCH))
        return D3DERR_INVALIDCALL;

    /* width and height */
    if (FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    /* 256 x 256 default width/height */
    if ((w == D3DX_DEFAULT) && (h == D3DX_DEFAULT))
        w = h = 256;
    else if (w == D3DX_DEFAULT)
        w = (height ? h : 256);
    else if (h == D3DX_DEFAULT)
        h = (width ? w : 256);

    /* ensure width/height is power of 2 */
    if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && (!is_pow2(w)))
        w = make_pow2(w);

    if (w > caps.MaxTextureWidth)
        w = caps.MaxTextureWidth;

    if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && (!is_pow2(h)))
        h = make_pow2(h);

    if (h > caps.MaxTextureHeight)
        h = caps.MaxTextureHeight;

    /* texture must be square? */
    if (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
    {
        if (w > h)
            h = w;
        else
            w = h;
    }

    if (width)
        *width = w;

    if (height)
        *height = h;

    /* miplevels */
    if (miplevels)
    {
        UINT max_mipmaps = 1;

        if (!width && !height)
            max_mipmaps = 9;    /* number of mipmaps in a 256x256 texture */
        else
        {
            UINT max_dimen = max(w, h);

            while (max_dimen > 1)
            {
                max_dimen >>= 1;
                max_mipmaps++;
            }
        }

        if (*miplevels == 0 || *miplevels > max_mipmaps)
            *miplevels = max_mipmaps;
    }

    /* format */
    if (format)
    {
        D3DDEVICE_CREATION_PARAMETERS params;
        IDirect3D9 *d3d = NULL;
        D3DDISPLAYMODE mode;
        HRESULT hr;

        hr = IDirect3DDevice9_GetDirect3D(device, &d3d);

        if (FAILED(hr))
            goto cleanup;

        hr = IDirect3DDevice9_GetCreationParameters(device, &params);

        if (FAILED(hr))
            goto cleanup;

        hr = IDirect3DDevice9_GetDisplayMode(device, 0, &mode);

        if (FAILED(hr))
            goto cleanup;

        if ((*format == D3DFMT_UNKNOWN) || (*format == D3DX_DEFAULT))
            *format = D3DFMT_A8R8G8B8;

        hr = IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType, mode.Format, usage,
            D3DRTYPE_TEXTURE, *format);

        if (FAILED(hr))
            FIXME("Pixel format adjustment not implemented yet\n");

cleanup:

        if (d3d)
            IDirect3D9_Release(d3d);

        if (FAILED(hr))
            return D3DERR_INVALIDCALL;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE9 pDevice,
                                 UINT width,
                                 UINT height,
                                 UINT miplevels,
                                 DWORD usage,
                                 D3DFORMAT format,
                                 D3DPOOL pool,
                                 LPDIRECT3DTEXTURE9 *ppTexture)
{
    HRESULT hr;

    TRACE("(%p, %u, %u, %u, %x, %x, %x, %p)\n", pDevice, width, height, miplevels, usage, format,
        pool, ppTexture);

    if (!pDevice || !ppTexture)
        return D3DERR_INVALIDCALL;

    hr = D3DXCheckTextureRequirements(pDevice, &width, &height, &miplevels, usage, &format, pool);

    if (FAILED(hr))
        return hr;

    return IDirect3DDevice9_CreateTexture(pDevice, width, height, miplevels, usage, format, pool, ppTexture, NULL);
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemoryEx(LPDIRECT3DDEVICE9 device,
                                                   LPCVOID srcdata,
                                                   UINT srcdatasize,
                                                   UINT width,
                                                   UINT height,
                                                   UINT miplevels,
                                                   DWORD usage,
                                                   D3DFORMAT format,
                                                   D3DPOOL pool,
                                                   DWORD filter,
                                                   DWORD mipfilter,
                                                   D3DCOLOR colorkey,
                                                   D3DXIMAGE_INFO* srcinfo,
                                                   PALETTEENTRY* palette,
                                                   LPDIRECT3DTEXTURE9* texture)
{
    IDirect3DTexture9 **texptr;
    IDirect3DTexture9 *buftex;
    IDirect3DSurface9 *surface;
    BOOL file_width = FALSE, file_height = FALSE;
    BOOL file_format = FALSE, file_miplevels = FALSE;
    D3DXIMAGE_INFO imginfo;
    D3DCAPS9 caps;
    HRESULT hr;

    TRACE("(%p, %p, %u, %u, %u, %u, %x, %x, %x, %u, %u, %x, %p, %p, %p)\n", device, srcdata, srcdatasize, width,
        height, miplevels, usage, format, pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    /* check for invalid parameters */
    if (!device || !texture || !srcdata || !srcdatasize)
        return D3DERR_INVALIDCALL;

    hr = D3DXGetImageInfoFromFileInMemory(srcdata, srcdatasize, &imginfo);

    if (FAILED(hr))
    {
        *texture = NULL;
        return hr;
    }

    /* handle default values */
    if (width == 0 || width == D3DX_DEFAULT_NONPOW2)
        width = imginfo.Width;

    if (height == 0 || height == D3DX_DEFAULT_NONPOW2)
        height = imginfo.Height;

    if (width == D3DX_DEFAULT)
        width = make_pow2(imginfo.Width);

    if (height == D3DX_DEFAULT)
        height = make_pow2(imginfo.Height);

    if (format == D3DFMT_UNKNOWN || format == D3DX_DEFAULT)
        format = imginfo.Format;

    if (width == D3DX_FROM_FILE)
    {
        file_width = TRUE;
        width = imginfo.Width;
    }

    if (height == D3DX_FROM_FILE)
    {
        file_height = TRUE;
        height = imginfo.Height;
    }

    if (format == D3DFMT_FROM_FILE)
    {
        file_format = TRUE;
        format = imginfo.Format;
    }

    if (miplevels == D3DX_FROM_FILE)
    {
        file_miplevels = TRUE;
        miplevels = imginfo.MipLevels;
    }

    /* fix texture creation parameters */
    hr = D3DXCheckTextureRequirements(device, &width, &height, &miplevels, usage, &format, pool);

    if (FAILED(hr))
    {
        *texture = NULL;
        return hr;
    }

    if (((file_width) && (width != imginfo.Width))    ||
        ((file_height) && (height != imginfo.Height)) ||
        ((file_format) && (format != imginfo.Format)) ||
        ((file_miplevels) && (miplevels != imginfo.MipLevels)))
    {
        return D3DERR_NOTAVAILABLE;
    }

    if (FAILED(IDirect3DDevice9_GetDeviceCaps(device, &caps)))
        return D3DERR_INVALIDCALL;

    /* Create the to-be-filled texture */
    if ((caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (pool != D3DPOOL_DEFAULT) && (usage != D3DUSAGE_DYNAMIC))
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, pool, texture);
        texptr = texture;
    }
    else
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, D3DPOOL_SYSTEMMEM, &buftex);
        texptr = &buftex;
    }

    if (FAILED(hr))
    {
        *texture = NULL;
        return hr;
    }

    /* Load the file */
    IDirect3DTexture9_GetSurfaceLevel(*texptr, 0, &surface);
    hr = D3DXLoadSurfaceFromFileInMemory(surface, palette, NULL, srcdata, srcdatasize, NULL, filter, colorkey, NULL);
    IDirect3DSurface9_Release(surface);

    if (FAILED(hr))
    {
        IDirect3DTexture9_Release(*texptr);
        *texture = NULL;
        return hr;
    }

    hr = D3DXFilterTexture((IDirect3DBaseTexture9*) *texptr, palette, 0, mipfilter);

    if (FAILED(hr))
    {
        IDirect3DTexture9_Release(*texptr);
        *texture = NULL;
        return hr;
    }

    /* Move the data to the actual texture if necessary */
    if (texptr == &buftex)
    {
        hr = D3DXCreateTexture(device, width, height, miplevels, usage, format, pool, texture);

        if (FAILED(hr))
        {
            IDirect3DTexture9_Release(buftex);
            *texture = NULL;
            return hr;
        }

        IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9*)buftex, (IDirect3DBaseTexture9*)(*texture));
        IDirect3DTexture9_Release(buftex);
    }

    if (srcinfo)
        *srcinfo = imginfo;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemory(LPDIRECT3DDEVICE9 device,
                                                 LPCVOID srcdata,
                                                 UINT srcdatasize,
                                                 LPDIRECT3DTEXTURE9 *texture)
{
    TRACE("(%p, %p, %d, %p)\n", device, srcdata, srcdatasize, texture);

    return D3DXCreateTextureFromFileInMemoryEx(device, srcdata, srcdatasize, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                               D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromFileExW(LPDIRECT3DDEVICE9 device,
                                            LPCWSTR srcfile,
                                            UINT width,
                                            UINT height,
                                            UINT miplevels,
                                            DWORD usage,
                                            D3DFORMAT format,
                                            D3DPOOL pool,
                                            DWORD filter,
                                            DWORD mipfilter,
                                            D3DCOLOR colorkey,
                                            D3DXIMAGE_INFO *srcinfo,
                                            PALETTEENTRY *palette,
                                            LPDIRECT3DTEXTURE9 *texture)
{
    HRESULT hr;
    DWORD size;
    LPVOID buffer;

    TRACE("(%p, %p, %u, %u, %u, %x, %x, %x, %u, %u, %x, %p, %p, %p): relay\n", device, debugstr_w(srcfile), width,
        height, miplevels, usage, format, pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!srcfile)
        return D3DERR_INVALIDCALL;

    hr = map_view_of_file(srcfile, &buffer, &size);
    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    hr = D3DXCreateTextureFromFileInMemoryEx(device, buffer, size, width, height, miplevels, usage, format, pool,
        filter, mipfilter, colorkey, srcinfo, palette, texture);

    UnmapViewOfFile(buffer);

    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE9 device,
                                            LPCSTR srcfile,
                                            UINT width,
                                            UINT height,
                                            UINT miplevels,
                                            DWORD usage,
                                            D3DFORMAT format,
                                            D3DPOOL pool,
                                            DWORD filter,
                                            DWORD mipfilter,
                                            D3DCOLOR colorkey,
                                            D3DXIMAGE_INFO *srcinfo,
                                            PALETTEENTRY *palette,
                                            LPDIRECT3DTEXTURE9 *texture)
{
    LPWSTR widename;
    HRESULT hr;
    DWORD len;

    TRACE("(%p, %p, %u, %u, %u, %x, %x, %x, %u, %u, %x, %p, %p, %p): relay\n", device, debugstr_a(srcfile), width,
        height, miplevels, usage, format, pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    if (!device || !srcfile || !texture)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    widename = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, widename, len);

    hr = D3DXCreateTextureFromFileExW(device, widename, width, height, miplevels,
                                      usage, format, pool, filter, mipfilter,
                                      colorkey, srcinfo, palette, texture);

    HeapFree(GetProcessHeap(), 0, widename);
    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileA(LPDIRECT3DDEVICE9 device,
                                          LPCSTR srcfile,
                                          LPDIRECT3DTEXTURE9 *texture)
{
    TRACE("(%p, %s, %p)\n", device, debugstr_a(srcfile), texture);

    return D3DXCreateTextureFromFileExA(device, srcfile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}

HRESULT WINAPI D3DXCreateTextureFromFileW(LPDIRECT3DDEVICE9 device,
                                          LPCWSTR srcfile,
                                          LPDIRECT3DTEXTURE9 *texture)
{
    TRACE("(%p, %s, %p)\n", device, debugstr_w(srcfile), texture);

    return D3DXCreateTextureFromFileExW(device, srcfile, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN,
                                        D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, texture);
}
