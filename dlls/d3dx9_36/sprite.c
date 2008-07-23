/*
 * Copyright (C) 2008 Tony Wasserka
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

static HRESULT WINAPI ID3DXSpriteImpl_QueryInterface(LPD3DXSPRITE iface, REFIID riid, LPVOID *object)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;

    TRACE("(%p): QueryInterface from %s\n", This, debugstr_guid(riid));
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ID3DXSprite)) {
        IUnknown_AddRef(iface);
        *object=This;
        return S_OK;
    }
    WARN("(%p)->(%s, %p): not found\n", iface, debugstr_guid(riid), *object);
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXSpriteImpl_AddRef(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    ULONG ref=InterlockedIncrement(&This->ref);
    TRACE("(%p): AddRef from %d\n", This, ref-1);
    return ref;
}

static ULONG WINAPI ID3DXSpriteImpl_Release(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    ULONG ref=InterlockedDecrement(&This->ref);
    TRACE("(%p): ReleaseRef to %d\n", This, ref);

    if(ref==0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI ID3DXSpriteImpl_GetDevice(LPD3DXSPRITE iface, LPDIRECT3DDEVICE9 *device)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_GetTransform(LPD3DXSPRITE iface, D3DXMATRIX *transform)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_SetTransform(LPD3DXSPRITE iface, CONST D3DXMATRIX *transform)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_SetWorldViewRH(LPD3DXSPRITE iface, CONST D3DXMATRIX *world, CONST D3DXMATRIX *view)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_SetWorldViewLH(LPD3DXSPRITE iface, CONST D3DXMATRIX *world, CONST D3DXMATRIX *view)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_Begin(LPD3DXSPRITE iface, DWORD flags)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_Draw(LPD3DXSPRITE iface, LPDIRECT3DTEXTURE9 texture, CONST RECT *rect, CONST D3DXVECTOR3 *center,
                                           CONST D3DXVECTOR3 *position, D3DCOLOR color)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_Flush(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_End(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_OnLostDevice(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_OnResetDevice(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static const ID3DXSpriteVtbl D3DXSprite_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXSpriteImpl_QueryInterface,
    ID3DXSpriteImpl_AddRef,
    ID3DXSpriteImpl_Release,
    /*** ID3DXSprite methods ***/
    ID3DXSpriteImpl_GetDevice,
    ID3DXSpriteImpl_GetTransform,
    ID3DXSpriteImpl_SetTransform,
    ID3DXSpriteImpl_SetWorldViewRH,
    ID3DXSpriteImpl_SetWorldViewLH,
    ID3DXSpriteImpl_Begin,
    ID3DXSpriteImpl_Draw,
    ID3DXSpriteImpl_Flush,
    ID3DXSpriteImpl_End,
    ID3DXSpriteImpl_OnLostDevice,
    ID3DXSpriteImpl_OnResetDevice
};

HRESULT WINAPI D3DXCreateSprite(LPDIRECT3DDEVICE9 device, LPD3DXSPRITE *sprite)
{
    ID3DXSpriteImpl *object;

    FIXME("stub\n");

    if(device==NULL || sprite==NULL) return D3DERR_INVALIDCALL;

    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXSpriteImpl));
    if(object==NULL) {
        *sprite=NULL;
        return E_OUTOFMEMORY;
    }
    object->lpVtbl=&D3DXSprite_Vtbl;
    object->ref=1;

    *sprite=(ID3DXSprite*)object;

    return D3D_OK;
}
