/*
 * IDirect3DSwapChain9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DSwapChain IUnknown parts follow: */
HRESULT WINAPI IDirect3DSwapChain9Impl_QueryInterface(LPDIRECT3DSWAPCHAIN9 iface, REFIID riid, LPVOID* ppobj)
{
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DSwapChain9)) {
        IDirect3DSwapChain9Impl_AddRef(iface);
	*ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DSwapChain9Impl_AddRef(LPDIRECT3DSWAPCHAIN9 iface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DSwapChain9Impl_Release(LPDIRECT3DSWAPCHAIN9 iface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DSwapChain9 parts follow: */
HRESULT WINAPI IDirect3DSwapChain9Impl_Present(LPDIRECT3DSWAPCHAIN9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSwapChain9Impl_GetFrontBufferData(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DSurface9* pDestSurface) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSwapChain9Impl_GetBackBuffer(LPDIRECT3DSWAPCHAIN9 iface, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSwapChain9Impl_GetRasterStatus(LPDIRECT3DSWAPCHAIN9 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSwapChain9Impl_GetDisplayMode(LPDIRECT3DSWAPCHAIN9 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSwapChain9Impl_GetDevice(LPDIRECT3DSWAPCHAIN9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;  
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE9) This->Device;

    /* Note  Calling this method will increase the internal reference count 
       on the IDirect3DDevice9 interface. */
    IDirect3DDevice9Impl_AddRef(*ppDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSwapChain9Impl_GetPresentParameters(LPDIRECT3DSWAPCHAIN9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DSwapChain9Impl *This = (IDirect3DSwapChain9Impl *)iface;
    FIXME("(%p) : copy\n", This); 
    memcpy(pPresentationParameters, &This->PresentParms, sizeof(D3DPRESENT_PARAMETERS));
    return D3D_OK;
}


const IDirect3DSwapChain9Vtbl Direct3DSwapChain9_Vtbl =
{
    IDirect3DSwapChain9Impl_QueryInterface,
    IDirect3DSwapChain9Impl_AddRef,
    IDirect3DSwapChain9Impl_Release,
    IDirect3DSwapChain9Impl_Present,
    IDirect3DSwapChain9Impl_GetFrontBufferData,
    IDirect3DSwapChain9Impl_GetBackBuffer,
    IDirect3DSwapChain9Impl_GetRasterStatus,
    IDirect3DSwapChain9Impl_GetDisplayMode,
    IDirect3DSwapChain9Impl_GetDevice,
    IDirect3DSwapChain9Impl_GetPresentParameters
};


/* IDirect3DDevice9 IDirect3DSwapChain9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetSwapChain(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

UINT     WINAPI  IDirect3DDevice9Impl_GetNumberOfSwapChains(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return 1;
}
