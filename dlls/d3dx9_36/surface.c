/*
 * Copyright (C) 2009 Tony Wasserka
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
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);


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
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(LPCVOID data, UINT datasize, D3DXIMAGE_INFO *info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI D3DXGetImageInfoFromFileA(LPCSTR file, D3DXIMAGE_INFO *info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI D3DXGetImageInfoFromFileW(LPCWSTR file, D3DXIMAGE_INFO *info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, LPCSTR resource, D3DXIMAGE_INFO *info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, LPCWSTR resource, D3DXIMAGE_INFO *info)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}
