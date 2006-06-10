/*
 * IDirect3DSurface8 implementation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DSurface8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DSurface8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
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

static ULONG WINAPI IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
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

/* IDirect3DSurface8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

static HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
}

/* IDirect3DSurface8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid, void **ppContainer) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT res;
    IUnknown *IWineContainer = NULL;

    TRACE("(%p) Relay\n", This);

    /* The container returned from IWineD3DSurface_GetContainer is either an IWineD3DDevice,
       one of the subclasses of IWineD3DBaseTexture or an IWineD3DSwapChain  */
    /* Get the IUnknown container. */
    res = IWineD3DSurface_GetContainer(This->wineD3DSurface, &IID_IUnknown, (void **)&IWineContainer);
    if (res == D3D_OK && IWineContainer != NULL) {

        /* Now find out what kind of container it is (so that we can get its parent)*/
        IUnknown  *IUnknownParent = NULL;
        IUnknown  *myContainer    = NULL;
        if (D3D_OK == IUnknown_QueryInterface(IWineContainer, &IID_IWineD3DDevice, (void **)&myContainer)) {
            IWineD3DDevice_GetParent((IWineD3DDevice *)IWineContainer, &IUnknownParent);
            IUnknown_Release(myContainer);
        } else 
        if (D3D_OK == IUnknown_QueryInterface(IWineContainer, &IID_IWineD3DBaseTexture, (void **)&myContainer)) {
            IWineD3DBaseTexture_GetParent((IWineD3DBaseTexture *)IWineContainer, &IUnknownParent);
            IUnknown_Release(myContainer);
        } else
        if (D3D_OK == IUnknown_QueryInterface(IWineContainer, &IID_IWineD3DSwapChain, (void **)&myContainer)) {
            IWineD3DSwapChain_GetParent((IWineD3DSwapChain *)IWineContainer, &IUnknownParent);
            IUnknown_Release(myContainer);
        } else {
            FIXME("Container is of unknown interface\n");
        }
        /* Tidy up.. */
        IUnknown_Release((IWineD3DDevice *)IWineContainer);

        /* Now, query the interface of the parent for the riid */
        if (IUnknownParent != NULL) {
            res = IUnknown_QueryInterface(IUnknownParent, riid, ppContainer);
            /* Tidy up.. */
            IUnknown_Release(IUnknownParent);
        }
    }

    TRACE("(%p) : returning %p\n", This, *ppContainer);    
    return res;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;
    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    memset(&wined3ddesc, 0, sizeof(wined3ddesc));
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &pDesc->Size;
    wined3ddesc.MultiSampleType     = (WINED3DMULTISAMPLE_TYPE *) &pDesc->MultiSampleType;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    return IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
}

static HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    TRACE("(%p) calling IWineD3DSurface_LockRect %p %p %p %ld\n", This, This->wineD3DSurface, pLockedRect, pRect, Flags);
    return IWineD3DSurface_LockRect(This->wineD3DSurface, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
}

static HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_UnlockRect(This->wineD3DSurface);
}

const IDirect3DSurface8Vtbl Direct3DSurface8_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface8Impl_QueryInterface,
    IDirect3DSurface8Impl_AddRef,
    IDirect3DSurface8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DSurface8Impl_GetDevice,
    IDirect3DSurface8Impl_SetPrivateData,
    IDirect3DSurface8Impl_GetPrivateData,
    IDirect3DSurface8Impl_FreePrivateData,
    /* IDirect3DSurface8 */
    IDirect3DSurface8Impl_GetContainer,
    IDirect3DSurface8Impl_GetDesc,
    IDirect3DSurface8Impl_LockRect,
    IDirect3DSurface8Impl_UnlockRect
};
