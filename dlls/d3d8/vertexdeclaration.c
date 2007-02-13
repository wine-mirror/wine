/*
 * IDirect3DVertexDeclaration8 implementation
 *
 * Copyright 2007 Henri Verbeet
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

/* IDirect3DVertexDeclaration8 is internal to our implementation.
 * It's not visible in the API. */

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IUnknown */
static HRESULT WINAPI IDirect3DVertexDeclaration8Impl_QueryInterface(IDirect3DVertexDeclaration8 *iface, REFIID riid, void **obj_ptr)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), obj_ptr);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirect3DVertexDeclaration8))
    {
        IUnknown_AddRef(iface);
        *obj_ptr = iface;
        return S_OK;
    }

    *obj_ptr = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexDeclaration8Impl_AddRef(IDirect3DVertexDeclaration8 *iface)
{
    IDirect3DVertexDeclaration8Impl *This = (IDirect3DVertexDeclaration8Impl *)iface;

    ULONG ref_count = InterlockedIncrement(&This->ref_count);
    TRACE("(%p) : AddRef increasing to %d\n", This, ref_count);

    return ref_count;
}

static ULONG WINAPI IDirect3DVertexDeclaration8Impl_Release(IDirect3DVertexDeclaration8 *iface)
{
    IDirect3DVertexDeclaration8Impl *This = (IDirect3DVertexDeclaration8Impl *)iface;

    ULONG ref_count = InterlockedDecrement(&This->ref_count);
    TRACE("(%p) : Releasing to %d\n", This, ref_count);

    if (!ref_count) {
        IWineD3DVertexDeclaration_Release(This->wined3d_vertex_declaration);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref_count;
}

const IDirect3DVertexDeclaration8Vtbl Direct3DVertexDeclaration8_Vtbl =
{
    IDirect3DVertexDeclaration8Impl_QueryInterface,
    IDirect3DVertexDeclaration8Impl_AddRef,
    IDirect3DVertexDeclaration8Impl_Release
};
