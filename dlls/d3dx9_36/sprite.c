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
        if(This->sprites) {
            int i;
            for(i=0;i<This->sprite_count;i++)
                if(This->sprites[i].texture)
                    IDirect3DTexture9_Release(This->sprites[i].texture);

            HeapFree(GetProcessHeap(), 0, This->sprites);
        }
        if(This->stateblock) IDirect3DStateBlock9_Release(This->stateblock);
        if(This->vdecl) IDirect3DVertexDeclaration9_Release(This->vdecl);
        if(This->device) IDirect3DDevice9_Release(This->device);
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
    D3DSURFACE_DESC texdesc;
    TRACE("(%p): relay\n", This);

    if(texture==NULL) return D3DERR_INVALIDCALL;

    if(This->allocated_sprites==0) {
        This->sprites=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 32*sizeof(SPRITE));
        This->allocated_sprites=32;
    } else if(This->allocated_sprites<=This->sprite_count) {
        This->allocated_sprites=This->allocated_sprites*3/2;
        This->sprites=HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->sprites, This->allocated_sprites*sizeof(SPRITE));
    }
    This->sprites[This->sprite_count].texture=texture;
    IUnknown_AddRef(texture);

    /* Reuse the texture desc if possible */
    if(This->sprite_count) {
        if(This->sprites[This->sprite_count-1].texture!=texture) {
            IDirect3DTexture9_GetLevelDesc(texture, 0, &texdesc);
        } else {
            texdesc.Width=This->sprites[This->sprite_count-1].texw;
            texdesc.Height=This->sprites[This->sprite_count-1].texh;
        }
    } else IDirect3DTexture9_GetLevelDesc(texture, 0, &texdesc);

    This->sprites[This->sprite_count].texw=texdesc.Width;
    This->sprites[This->sprite_count].texh=texdesc.Height;

    if(rect==NULL) {
        This->sprites[This->sprite_count].rect.left=0;
        This->sprites[This->sprite_count].rect.top=0;
        This->sprites[This->sprite_count].rect.right=texdesc.Width;
        This->sprites[This->sprite_count].rect.bottom=texdesc.Height;
    } else This->sprites[This->sprite_count].rect=*rect;

    if(center==NULL) {
        This->sprites[This->sprite_count].center.x=0.0f;
        This->sprites[This->sprite_count].center.y=0.0f;
        This->sprites[This->sprite_count].center.z=0.0f;
    } else This->sprites[This->sprite_count].center=*center;

    if(position==NULL) {
        This->sprites[This->sprite_count].pos.x=0.0f;
        This->sprites[This->sprite_count].pos.y=0.0f;
        This->sprites[This->sprite_count].pos.z=0.0f;
    } else This->sprites[This->sprite_count].pos=*position;

    This->sprites[This->sprite_count].color=color;
    This->sprite_count++;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_Flush(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    int i;
    FIXME("(%p): stub\n", This);

    for(i=0;i<This->sprite_count;i++)
        if(This->sprites[i].texture)
            IDirect3DTexture9_Release(This->sprites[i].texture);

    This->sprite_count=0;

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_End(LPD3DXSPRITE iface)
{
    ID3DXSpriteImpl *This=(ID3DXSpriteImpl*)iface;
    FIXME("(%p): stub\n", This);

    ID3DXSprite_Flush(iface);

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
    D3DCAPS9 caps;
    static const D3DVERTEXELEMENT9 elements[] =
        {
            { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
            { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };

    TRACE("(void): relay\n");

    if(device==NULL || sprite==NULL) return D3DERR_INVALIDCALL;

    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXSpriteImpl));
    if(object==NULL) {
        *sprite=NULL;
        return E_OUTOFMEMORY;
    }
    object->lpVtbl=&D3DXSprite_Vtbl;
    object->ref=1;
    object->device=device;
    IUnknown_AddRef(device);

    IDirect3DDevice9_CreateVertexDeclaration(object->device, elements, &object->vdecl);
    object->stateblock=NULL;

    D3DXMatrixIdentity(&object->transform);
    D3DXMatrixIdentity(&object->view);

    object->flags=0;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);
    object->texfilter_caps=caps.TextureFilterCaps;
    object->maxanisotropy=caps.MaxAnisotropy;
    object->alphacmp_caps=caps.AlphaCmpCaps;

    object->sprites=NULL;
    object->sprite_count=0;
    object->allocated_sprites=0;

    *sprite=(ID3DXSprite*)object;

    return D3D_OK;
}
