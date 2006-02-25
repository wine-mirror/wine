/*
 * IDirect3DResource8 implementation
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

/* IDirect3DResource8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DResource8Impl_QueryInterface(LPDIRECT3DRESOURCE8 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DResource8Impl_AddRef(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DResource8Impl_Release(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DResource_Release(This->wineD3DResource);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DResource8 Interface follow: */
HRESULT WINAPI IDirect3DResource8Impl_GetDevice(LPDIRECT3DRESOURCE8 iface, IDirect3DDevice8** ppDevice) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    IWineD3DDevice *myDevice = NULL;

    TRACE("(%p) Relay\n", This);

    IWineD3DResource_GetDevice(This->wineD3DResource, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DResource8Impl_SetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_SetPrivateData(This->wineD3DResource, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IDirect3DResource8Impl_GetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_GetPrivateData(This->wineD3DResource, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IDirect3DResource8Impl_FreePrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_FreePrivateData(This->wineD3DResource, refguid);
}

DWORD  WINAPI IDirect3DResource8Impl_SetPriority(LPDIRECT3DRESOURCE8 iface, DWORD PriorityNew) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_SetPriority(This->wineD3DResource, PriorityNew);
}

DWORD WINAPI IDirect3DResource8Impl_GetPriority(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_GetPriority(This->wineD3DResource);
}

void WINAPI IDirect3DResource8Impl_PreLoad(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DResource_PreLoad(This->wineD3DResource);
    return;
}

D3DRESOURCETYPE WINAPI IDirect3DResource8Impl_GetType(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_GetType(This->wineD3DResource);
}

const IDirect3DResource8Vtbl Direct3DResource8_Vtbl =
{
    IDirect3DResource8Impl_QueryInterface,
    IDirect3DResource8Impl_AddRef,
    IDirect3DResource8Impl_Release,
    IDirect3DResource8Impl_GetDevice,
    IDirect3DResource8Impl_SetPrivateData,
    IDirect3DResource8Impl_GetPrivateData,
    IDirect3DResource8Impl_FreePrivateData,
    IDirect3DResource8Impl_SetPriority,
    IDirect3DResource8Impl_GetPriority,
    IDirect3DResource8Impl_PreLoad,
    IDirect3DResource8Impl_GetType
};
