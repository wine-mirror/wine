/*
 * IDirect3DSurface9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DSurface9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DSurface9Impl_QueryInterface(LPDIRECT3DSURFACE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DSurface9)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DSurface9Impl_AddRef(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    IUnknown *containerParent = NULL;

    TRACE("(%p)\n", This);

    IWineD3DSurface_GetContainerParent(This->wineD3DSurface, &containerParent);
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

ULONG WINAPI IDirect3DSurface9Impl_Release(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    IUnknown *containerParent = NULL;

    TRACE("(%p)\n", This);

    IWineD3DSurface_GetContainerParent(This->wineD3DSurface, &containerParent);
    if (containerParent) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, containerParent);
        return IUnknown_Release(containerParent);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

        if (ref == 0) {
            IWineD3DSurface_Release(This->wineD3DSurface);
            HeapFree(GetProcessHeap(), 0, This);
        }

        return ref;
    }
}

/* IDirect3DSurface9 IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DSurface9Impl_GetDevice(LPDIRECT3DSURFACE9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

HRESULT WINAPI IDirect3DSurface9Impl_SetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IDirect3DSurface9Impl_GetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IDirect3DSurface9Impl_FreePrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
}

DWORD WINAPI IDirect3DSurface9Impl_SetPriority(LPDIRECT3DSURFACE9 iface, DWORD PriorityNew) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_SetPriority(This->wineD3DSurface, PriorityNew);
}

DWORD WINAPI IDirect3DSurface9Impl_GetPriority(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_GetPriority(This->wineD3DSurface);
}

void WINAPI IDirect3DSurface9Impl_PreLoad(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DSurface_PreLoad(This->wineD3DSurface);
    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DSurface9Impl_GetType(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_GetType(This->wineD3DSurface);
}

/* IDirect3DSurface9 Interface follow: */
HRESULT WINAPI IDirect3DSurface9Impl_GetContainer(LPDIRECT3DSURFACE9 iface, REFIID riid, void** ppContainer) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    IWineD3DBase *wineD3DContainer = NULL;
    IUnknown *wineD3DContainerParent = NULL;
    HRESULT res;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!ppContainer) {
        ERR("Called without a valid ppContainer\n");
    }

    /* Get the WineD3D container. */
    res = IWineD3DSurface_GetContainer(This->wineD3DSurface, &IID_IWineD3DBase, (void **)&wineD3DContainer);
    if (res != D3D_OK) return res;

    if (!wineD3DContainer) {
        ERR("IWineD3DSurface_GetContainer should never return NULL\n");
    }

    /* Get the parent */
    IWineD3DBase_GetParent(wineD3DContainer, &wineD3DContainerParent);
    IUnknown_Release(wineD3DContainer);

    if (!wineD3DContainerParent) {
        ERR("IWineD3DBase_GetParent should never return NULL\n");
    }

    /* Now, query the interface of the parent for the riid */
    res = IUnknown_QueryInterface(wineD3DContainerParent, riid, ppContainer);
    IUnknown_Release(wineD3DContainerParent);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

HRESULT WINAPI IDirect3DSurface9Impl_GetDesc(LPDIRECT3DSURFACE9 iface, D3DSURFACE_DESC* pDesc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;
    UINT                   tmpInt = -1;
    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt;
    wined3ddesc.MultiSampleType     = (WINED3DMULTISAMPLE_TYPE *) &pDesc->MultiSampleType;
    wined3ddesc.MultiSampleQuality  = &pDesc->MultiSampleQuality;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    return IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
}

HRESULT WINAPI IDirect3DSurface9Impl_LockRect(LPDIRECT3DSURFACE9 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    TRACE("(%p) calling IWineD3DSurface_LockRect %p %p %p %ld\n", This, This->wineD3DSurface, pLockedRect, pRect, Flags);
    return IWineD3DSurface_LockRect(This->wineD3DSurface, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
}

HRESULT WINAPI IDirect3DSurface9Impl_UnlockRect(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_UnlockRect(This->wineD3DSurface);
}

HRESULT WINAPI IDirect3DSurface9Impl_GetDC(LPDIRECT3DSURFACE9 iface, HDC* phdc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_GetDC(This->wineD3DSurface, phdc);
}

HRESULT WINAPI IDirect3DSurface9Impl_ReleaseDC(LPDIRECT3DSURFACE9 iface, HDC hdc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_ReleaseDC(This->wineD3DSurface, hdc);
}


const IDirect3DSurface9Vtbl Direct3DSurface9_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface9Impl_QueryInterface,
    IDirect3DSurface9Impl_AddRef,
    IDirect3DSurface9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DSurface9Impl_GetDevice,
    IDirect3DSurface9Impl_SetPrivateData,
    IDirect3DSurface9Impl_GetPrivateData,
    IDirect3DSurface9Impl_FreePrivateData,
    IDirect3DSurface9Impl_SetPriority,
    IDirect3DSurface9Impl_GetPriority,
    IDirect3DSurface9Impl_PreLoad,
    IDirect3DSurface9Impl_GetType,
    /* IDirect3DSurface9 */
    IDirect3DSurface9Impl_GetContainer,
    IDirect3DSurface9Impl_GetDesc,
    IDirect3DSurface9Impl_LockRect,
    IDirect3DSurface9Impl_UnlockRect,
    IDirect3DSurface9Impl_GetDC,
    IDirect3DSurface9Impl_ReleaseDC
};
