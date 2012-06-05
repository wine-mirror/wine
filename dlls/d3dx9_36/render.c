/*
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

#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT WINAPI D3DXCreateRenderToSurface(IDirect3DDevice9 *device,
                                         UINT width,
                                         UINT height,
                                         D3DFORMAT format,
                                         BOOL depth_stencil,
                                         D3DFORMAT depth_stencil_format,
                                         ID3DXRenderToSurface **out)
{
    FIXME("(%p, %u, %u, %#x, %d, %#x, %p): stub\n", device, width, height, format,
            depth_stencil, depth_stencil_format, out);

    if (!device || !out) return D3DERR_INVALIDCALL;

    return E_NOTIMPL;
}
