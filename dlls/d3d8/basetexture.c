/*
 * IDirect3DBaseTexture8 implementation
 *
 * Copyright 2005 Oliver Stieber
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DBaseTexture8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DBaseTexture8Impl_QueryInterface(LPDIRECT3DBASETEXTURE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);

    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DBaseTexture8Impl_AddRef(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DBaseTexture8Impl_Release(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DBaseTexture_Release(This->wineD3DBaseTexture);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* IDirect3DBaseTexture8 IDirect3DResource8 Interface follow: */
HRESULT WINAPI IDirect3DBaseTexture8Impl_GetDevice(LPDIRECT3DBASETEXTURE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

HRESULT WINAPI IDirect3DBaseTexture8Impl_SetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_SetPrivateData(This->wineD3DBaseTexture, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IDirect3DBaseTexture8Impl_GetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_GetPrivateData(This->wineD3DBaseTexture, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IDirect3DBaseTexture8Impl_FreePrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_FreePrivateData(This->wineD3DBaseTexture, refguid);
}

DWORD WINAPI IDirect3DBaseTexture8Impl_SetPriority(LPDIRECT3DBASETEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_SetPriority(This->wineD3DBaseTexture, PriorityNew);
}

DWORD WINAPI IDirect3DBaseTexture8Impl_GetPriority(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_GetPriority(This->wineD3DBaseTexture);
}

void WINAPI IDirect3DBaseTexture8Impl_PreLoad(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    IWineD3DBaseTexture_PreLoad(This->wineD3DBaseTexture);
    return;
}

D3DRESOURCETYPE WINAPI IDirect3DBaseTexture8Impl_GetType(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_GetType(This->wineD3DBaseTexture);
}

/* IDirect3DBaseTexture8 Interface follow: */
DWORD  WINAPI IDirect3DBaseTexture8Impl_SetLOD(LPDIRECT3DBASETEXTURE8 iface, DWORD LODNew) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_SetLOD(This->wineD3DBaseTexture, LODNew);
}

DWORD WINAPI IDirect3DBaseTexture8Impl_GetLOD(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_GetLOD(This->wineD3DBaseTexture);
}

DWORD WINAPI IDirect3DBaseTexture8Impl_GetLevelCount(LPDIRECT3DBASETEXTURE8 iface) {
    IDirect3DBaseTexture8Impl *This = (IDirect3DBaseTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DBaseTexture_GetLevelCount(This->wineD3DBaseTexture);
}

const IDirect3DBaseTexture8Vtbl Direct3DBaseTexture8_Vtbl =
{
    IDirect3DBaseTexture8Impl_QueryInterface,
    IDirect3DBaseTexture8Impl_AddRef,
    IDirect3DBaseTexture8Impl_Release,
    IDirect3DBaseTexture8Impl_GetDevice,
    IDirect3DBaseTexture8Impl_SetPrivateData,
    IDirect3DBaseTexture8Impl_GetPrivateData,
    IDirect3DBaseTexture8Impl_FreePrivateData,
    IDirect3DBaseTexture8Impl_SetPriority,
    IDirect3DBaseTexture8Impl_GetPriority,
    IDirect3DBaseTexture8Impl_PreLoad,
    IDirect3DBaseTexture8Impl_GetType,

    IDirect3DBaseTexture8Impl_SetLOD,
    IDirect3DBaseTexture8Impl_GetLOD,
    IDirect3DBaseTexture8Impl_GetLevelCount
};
