/*
 * IDirect3DVolume8 implementation
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

/* IDirect3DVolume8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVolume8Impl_QueryInterface(LPDIRECT3DVOLUME8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVolume8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVolume8Impl_AddRef(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    IUnknown *containerParent = NULL;

    TRACE("(%p)\n", This);

    IWineD3DVolume_GetContainerParent(This->wineD3DVolume, &containerParent);
    if (containerParent) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, containerParent);
        return IUnknown_AddRef(containerParent);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);
        TRACE("(%p) : AddRef from %ld\n", This, ref - 1);
        return ref;
    }
}

ULONG WINAPI IDirect3DVolume8Impl_Release(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    IUnknown *containerParent = NULL;

    TRACE("(%p)\n", This);

    IWineD3DVolume_GetContainerParent(This->wineD3DVolume, &containerParent);
    if (containerParent) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, containerParent);
        return IUnknown_Release(containerParent);
    }
    else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

        if (ref == 0) {
            IWineD3DVolume_Release(This->wineD3DVolume);
            HeapFree(GetProcessHeap(), 0, This);
        }

        return ref;
    }
}

/* IDirect3DVolume8 Interface follow: */
HRESULT WINAPI IDirect3DVolume8Impl_GetDevice(LPDIRECT3DVOLUME8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    IWineD3DDevice       *myDevice = NULL;

    IWineD3DVolume_GetDevice(This->wineD3DVolume, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume8Impl_SetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolume_SetPrivateData(This->wineD3DVolume, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IDirect3DVolume8Impl_GetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID  refguid, void *pData, DWORD* pSizeOfData) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolume_GetPrivateData(This->wineD3DVolume, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IDirect3DVolume8Impl_FreePrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolume_FreePrivateData(This->wineD3DVolume, refguid);
}

HRESULT WINAPI IDirect3DVolume8Impl_GetContainer(LPDIRECT3DVOLUME8 iface, REFIID riid, void **ppContainer) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT res;
    IUnknown *IWineContainer = NULL;

    TRACE("(%p) Relay\n", This);
    res = IWineD3DVolume_GetContainer(This->wineD3DVolume, riid, (void **)&IWineContainer);

    /* If this works, the only valid container is a child of resource (volumetexture) */
    if (res == D3D_OK && NULL != ppContainer) {
        IWineD3DResource_GetParent((IWineD3DResource *)IWineContainer, (IUnknown **)ppContainer);
        IWineD3DResource_Release((IWineD3DResource *)IWineContainer);
    }

    return res;
}

HRESULT WINAPI IDirect3DVolume8Impl_GetDesc(LPDIRECT3DVOLUME8 iface, D3DVOLUME_DESC *pDesc) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    WINED3DVOLUME_DESC     wined3ddesc;
    UINT                   tmpInt = -1;

    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d8 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;
    wined3ddesc.Depth               = &pDesc->Depth;

    return IWineD3DVolume_GetDesc(This->wineD3DVolume, &wined3ddesc);
}

HRESULT WINAPI IDirect3DVolume8Impl_LockBox(LPDIRECT3DVOLUME8 iface, D3DLOCKED_BOX *pLockedVolume, CONST D3DBOX *pBox, DWORD Flags) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    TRACE("(%p) relay %p %p %p %ld\n", This, This->wineD3DVolume, pLockedVolume, pBox, Flags);
    return IWineD3DVolume_LockBox(This->wineD3DVolume, (WINED3DLOCKED_BOX *) pLockedVolume, (WINED3DBOX *) pBox, Flags);
}

HRESULT WINAPI IDirect3DVolume8Impl_UnlockBox(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    TRACE("(%p) relay %p\n", This, This->wineD3DVolume);
    return IWineD3DVolume_UnlockBox(This->wineD3DVolume);
}

const IDirect3DVolume8Vtbl Direct3DVolume8_Vtbl =
{
    /* IUnknown */
    IDirect3DVolume8Impl_QueryInterface,
    IDirect3DVolume8Impl_AddRef,
    IDirect3DVolume8Impl_Release,
    /* IDirect3DVolume8 */
    IDirect3DVolume8Impl_GetDevice,
    IDirect3DVolume8Impl_SetPrivateData,
    IDirect3DVolume8Impl_GetPrivateData,
    IDirect3DVolume8Impl_FreePrivateData,
    IDirect3DVolume8Impl_GetContainer,
    IDirect3DVolume8Impl_GetDesc,
    IDirect3DVolume8Impl_LockBox,
    IDirect3DVolume8Impl_UnlockBox
};


/* Internal function called back during the CreateVolumeTexture */
HRESULT WINAPI D3D8CB_CreateVolume(IUnknown  *pDevice, UINT Width, UINT Height, UINT Depth, 
                                   WINED3DFORMAT  Format, WINED3DPOOL Pool, DWORD Usage,
                                   IWineD3DVolume **ppVolume,
                                   HANDLE   * pSharedHandle) {
    IDirect3DVolume8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)pDevice;
    HRESULT hrc = D3D_OK;

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolume8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppVolume = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVolume8_Vtbl;
    object->ref = 1;
    hrc = IWineD3DDevice_CreateVolume(This->WineD3DDevice, Width, Height, Depth, Usage, Format, 
                                       Pool, &object->wineD3DVolume, pSharedHandle, (IUnknown *)object);
    if (hrc != D3D_OK) {
        /* free up object */ 
        FIXME("(%p) call to IWineD3DDevice_CreateVolume failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVolume = NULL;
    } else {
        *ppVolume = (IWineD3DVolume *)object->wineD3DVolume;
    }
    TRACE("(%p) Created volume %p\n", This, *ppVolume);
    return hrc;
}
