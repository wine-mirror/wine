/*
 * Skin Info operations specific to D3DX9.
 *
 * Copyright (C) 2011 Dylan Smith
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

HRESULT WINAPI D3DXCreateSkinInfo(DWORD num_vertices, CONST D3DVERTEXELEMENT9 *declaration,
                                  DWORD num_bones, LPD3DXSKININFO *skin_info)
{
    FIXME("(%u, %p, %u, %p): stub\n", num_vertices, declaration, num_bones, skin_info);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXCreateSkinInfoFVF(DWORD num_vertices, DWORD fvf, DWORD num_bones, LPD3DXSKININFO *skin_info)
{
    FIXME("(%u, %x, %u, %p): stub\n", num_vertices, fvf, num_bones, skin_info);

    return E_NOTIMPL;
}
