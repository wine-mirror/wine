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

HRESULT WINAPI D3DXLoadVolumeFromMemory(LPDIRECT3DVOLUME9 destvolume,
                                        CONST PALETTEENTRY* destpalette,
                                        CONST D3DBOX* destbox,
                                        LPCVOID srcmemory,
                                        D3DFORMAT srcformat,
                                        UINT srcrowpitch,
                                        UINT srcslicepitch,
                                        CONST PALETTEENTRY* srcpalette,
                                        CONST D3DBOX* srcbox,
                                        DWORD filter,
                                        D3DCOLOR colorkey)
{
    FIXME("(%p, %p, %p, %p, %x, %u, %u, %p, %p, %x, %x): stub\n", destvolume, destpalette, destbox, srcmemory, srcformat,
        srcrowpitch, srcslicepitch, srcpalette, srcbox, filter, colorkey);

    return E_NOTIMPL;
}
