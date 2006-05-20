/*
 * IDirect3DSwapChain8 implementation
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

/* IDirect3DSwapChain IUnknown parts follow: */
HRESULT WINAPI IDirect3DSwapChain8Impl_QueryInterface(LPDIRECT3DSWAPCHAIN8 iface, REFIID riid, LPVOID* ppobj)
{
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DSwapChain8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DSwapChain8Impl_AddRef(LPDIRECT3DSWAPCHAIN8 iface) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DSwapChain8Impl_Release(LPDIRECT3DSWAPCHAIN8 iface) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DSwapChain_Release(This->wineD3DSwapChain);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DSwapChain8 parts follow: */
HRESULT WINAPI IDirect3DSwapChain8Impl_Present(LPDIRECT3DSWAPCHAIN8 iface, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSwapChain_Present(This->wineD3DSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
}

HRESULT WINAPI IDirect3DSwapChain8Impl_GetBackBuffer(LPDIRECT3DSWAPCHAIN8 iface, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSurface *mySurface = NULL;

    TRACE("(%p) Relay\n", This);

    hrc = IWineD3DSwapChain_GetBackBuffer(This->wineD3DSwapChain, iBackBuffer, (WINED3DBACKBUFFER_TYPE )Type, &mySurface);
    if (hrc == D3D_OK && NULL != mySurface) {
       IWineD3DSurface_GetParent(mySurface, (IUnknown **)ppBackBuffer);
       IWineD3DSurface_Release(mySurface);
    }
    return hrc;
}

const IDirect3DSwapChain8Vtbl Direct3DSwapChain8_Vtbl =
{
    IDirect3DSwapChain8Impl_QueryInterface,
    IDirect3DSwapChain8Impl_AddRef,
    IDirect3DSwapChain8Impl_Release,
    IDirect3DSwapChain8Impl_Present,
    IDirect3DSwapChain8Impl_GetBackBuffer
};
