/*
 * Copyright 2010 Christian Costa
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
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT WINAPI D3DXCheckTextureRequirements(LPDIRECT3DDEVICE9 device,
                                            UINT* width,
                                            UINT* height,
                                            UINT* miplevels,
                                            DWORD usage,
                                            D3DFORMAT* format,
                                            D3DPOOL pool)
{
    FIXME("(%p, %p, %p, %p, %u, %p, %u): stub\n", device, width, height, miplevels, usage, format, pool);

    return E_NOTIMPL;
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
    FIXME("(%p, %u, %u, %u, %x, %x, %x, %p): semi-stub\n", pDevice, width, height, miplevels, usage, format,
        pool, ppTexture);

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
    FIXME("(%p, %p, %u, %u, %u, %u, %x, %x, %x, %u, %u, %x, %p, %p, %p): stub\n", device, srcdata, srcdatasize, width,
        height, miplevels, usage, format, pool, filter, mipfilter, colorkey, srcinfo, palette, texture);

    return E_NOTIMPL;
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

HRESULT WINAPI D3DXCreateTextureFromFileA(LPDIRECT3DDEVICE9 device,
                                          LPCSTR srcfile,
                                          LPDIRECT3DTEXTURE9 *texture)
{
    FIXME("(%p, %s, %p): stub\n", device, debugstr_a(srcfile), texture);

    return E_NOTIMPL;
}
