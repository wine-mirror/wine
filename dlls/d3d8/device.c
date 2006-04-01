/*
 * IDirect3DDevice8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2004 Christian Costa
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

#include <math.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DDevice8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        TRACE("Releasing wined3d device %p\n", This->WineD3DDevice);
        IWineD3DDevice_Release(This->WineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
HRESULT WINAPI IDirect3DDevice8Impl_TestCooperativeLevel(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) : Relay\n", This);
    return IWineD3DDevice_TestCooperativeLevel(This->WineD3DDevice);
}

UINT WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) Relay\n", This);
    return IWineD3DDevice_GetAvailableTextureMem(This->WineD3DDevice);
}

HRESULT WINAPI IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) : Relay bytes(%ld)\n", This, Bytes);
    return IWineD3DDevice_EvictManagedResources(This->WineD3DDevice);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3D* pWineD3D;

    TRACE("(%p) Relay\n", This);

    if (NULL == ppD3D8) {
        return D3DERR_INVALIDCALL;
    }
    hr = IWineD3DDevice_GetDirect3D(This->WineD3DDevice, &pWineD3D);
    if (hr == D3D_OK && pWineD3D != NULL)
    {
        IWineD3DResource_GetParent((IWineD3DResource *)pWineD3D,(IUnknown **)ppD3D8);
        IWineD3DResource_Release((IWineD3DResource *)pWineD3D);
    } else {
        FIXME("Call to IWineD3DDevice_GetDirect3D failed\n");
        *ppD3D8 = NULL;
    }
    TRACE("(%p) returning %p\b",This , *ppD3D8);
    return hr;
}

HRESULT WINAPI IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) : Relay pCaps %p\n", This, pCaps);
    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /* well this is what MSDN says to return */
    }

    D3D8CAPSTOWINECAPS(pCaps, pWineCaps)
    hrc = IWineD3DDevice_GetDeviceCaps(This->WineD3DDevice, pWineCaps);
    HeapFree(GetProcessHeap(), 0, pWineCaps);
    TRACE("Returning %p %p\n", This, pCaps);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DDevice_GetDisplayMode(This->WineD3DDevice, 0, (WINED3DDISPLAYMODE *) pMode);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DDevice_GetCreationParameters(This->WineD3DDevice, pParameters);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *pSurface = (IDirect3DSurface8Impl*)pCursorBitmap;
    TRACE("(%p) Relay\n", This);
    return IWineD3DDevice_SetCursorProperties(This->WineD3DDevice,XHotSpot,YHotSpot,(IWineD3DSurface*)pSurface->wineD3DSurface);
}

void WINAPI IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DDevice_SetCursorPosition(This->WineD3DDevice, XScreenSpace, YScreenSpace, Flags);
}

BOOL WINAPI IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_ShowCursor(This->WineD3DDevice, bShow);
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSwapChain8Impl* object;
    HRESULT hrc = D3D_OK;
    WINED3DPRESENT_PARAMETERS localParameters;

    TRACE("(%p) Relay\n", This);

    object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *pSwapChain = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->ref = 1;
    object->lpVtbl = &Direct3DSwapChain8_Vtbl;

    /* Allocate an associated WineD3DDevice object */
    localParameters.BackBufferWidth                = &pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight               = &pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat               = (WINED3DFORMAT *)&pPresentationParameters->BackBufferFormat;
    localParameters.BackBufferCount                = &pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                = (WINED3DMULTISAMPLE_TYPE *) &pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality             = NULL; /* d3d9 only */
    localParameters.SwapEffect                     = (WINED3DSWAPEFFECT *) &pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                  = &pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                       = &pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil         = &pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat         = (WINED3DFORMAT *)&pPresentationParameters->AutoDepthStencilFormat;
    localParameters.Flags                          = &pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz     = &pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval           = &pPresentationParameters->FullScreen_PresentationInterval;


    hrc = IWineD3DDevice_CreateAdditionalSwapChain(This->WineD3DDevice, &localParameters, &object->wineD3DSwapChain, (IUnknown*)object, D3D8CB_CreateRenderTarget, D3D8CB_CreateDepthStencilSurface);
    if (hrc != D3D_OK) {
        FIXME("(%p) call to IWineD3DDevice_CreateAdditionalSwapChain failed\n", This);
        HeapFree(GetProcessHeap(), 0 , object);
        *pSwapChain = NULL;
    }else{
        *pSwapChain = (IDirect3DSwapChain8 *)object;
    }
    TRACE("(%p) returning %p\n", This, *pSwapChain);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    WINED3DPRESENT_PARAMETERS localParameters;
    TRACE("(%p) Relay pPresentationParameters(%p)\n", This, pPresentationParameters);
/* FINDME: FIXME: */
    localParameters.BackBufferWidth                = &pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight               = &pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat               = (WINED3DFORMAT *)&pPresentationParameters->BackBufferFormat;
    localParameters.BackBufferCount                = &pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                = (WINED3DMULTISAMPLE_TYPE *) &pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality             = NULL; /* D3d9 only */
    localParameters.SwapEffect                     = (WINED3DSWAPEFFECT *) &pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                  = &pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                       = &pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil         = &pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat         = (WINED3DFORMAT *)&pPresentationParameters->AutoDepthStencilFormat;
    localParameters.Flags                          = &pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz     = &pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval           = &pPresentationParameters->FullScreen_PresentationInterval;
    return IWineD3DDevice_Reset(This->WineD3DDevice, &localParameters);
}

HRESULT WINAPI IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DDevice_Present(This->WineD3DDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DSurface *retSurface = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    rc = IWineD3DDevice_GetBackBuffer(This->WineD3DDevice, 0, BackBuffer, Type, (IWineD3DSurface **)&retSurface);
    if (rc == D3D_OK && NULL != retSurface && NULL != ppBackBuffer) {
        IWineD3DSurface_GetParent(retSurface, (IUnknown **)ppBackBuffer);
        IWineD3DSurface_Release(retSurface);
    }
    return rc;
}

HRESULT WINAPI IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_GetRasterStatus(This->WineD3DDevice, 0, pRasterStatus);
}

void WINAPI IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_SetGammaRamp(This->WineD3DDevice, 0, Flags, pRamp);
}

void WINAPI IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_GetGammaRamp(This->WineD3DDevice, 0, pRamp);
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8 **ppTexture) {
    IDirect3DTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : W(%d) H(%d), Lvl(%d) d(%ld), Fmt(%u), Pool(%d)\n", This, Width, Height, Levels, Usage, Format,  Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTexture8Impl));

    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
/*        *ppTexture = NULL; */
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DTexture8_Vtbl;
    object->ref = 1;
    hrc = IWineD3DDevice_CreateTexture(This->WineD3DDevice, Width, Height, Levels, Usage,
                                 (WINED3DFORMAT)Format, (WINED3DPOOL) Pool, &object->wineD3DTexture, NULL, (IUnknown *)object, D3D8CB_CreateSurface);

    if (FAILED(hrc)) {
        /* free up object */ 
        FIXME("(%p) call to IWineD3DDevice_CreateTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
/*      *ppTexture = NULL; */
   } else {
        *ppTexture = (LPDIRECT3DTEXTURE8) object;
   }

   TRACE("(%p) Created Texture %p, %p\n",This,object,object->wineD3DTexture);
   return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, 
                                                          UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, 
                                                          D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) {

    IDirect3DVolumeTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolumeTexture8Impl));
    if (NULL == object) {
        FIXME("(%p) allocation of memory failed\n", This);
        *ppVolumeTexture = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVolumeTexture8_Vtbl;
    object->ref = 1;
    hrc = IWineD3DDevice_CreateVolumeTexture(This->WineD3DDevice, Width, Height, Depth, Levels, Usage,
                                 (WINED3DFORMAT)Format, (WINED3DPOOL) Pool, &object->wineD3DVolumeTexture, NULL,
                                 (IUnknown *)object, D3D8CB_CreateVolume);

    if (hrc != D3D_OK) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateVolumeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVolumeTexture = NULL;
    } else {
        *ppVolumeTexture = (LPDIRECT3DVOLUMETEXTURE8) object;
    }
    TRACE("(%p)  returning %p\n", This , *ppVolumeTexture);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength, UINT Levels, DWORD Usage, 
                                                        D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {

    IDirect3DCubeTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : ELen(%d) Lvl(%d) Usage(%ld) fmt(%u), Pool(%d)\n" , This, EdgeLength, Levels, Usage, Format, Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));

    if (NULL == object) {
        FIXME("(%p) allocation of CubeTexture failed\n", This);
        *ppCubeTexture = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DCubeTexture8_Vtbl;
    object->ref = 1;
    hr = IWineD3DDevice_CreateCubeTexture(This->WineD3DDevice, EdgeLength, Levels, Usage,
                                 (WINED3DFORMAT)Format, (WINED3DPOOL) Pool, &object->wineD3DCubeTexture, NULL, (IUnknown*)object,
                                 D3D8CB_CreateSurface);

    if (hr != D3D_OK){

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateCubeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppCubeTexture = NULL;
    } else {
        *ppCubeTexture = (LPDIRECT3DCUBETEXTURE8) object;
    }

    TRACE("(%p) returning %p\n",This, *ppCubeTexture);
    return hr;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Size, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
    IDirect3DVertexBuffer8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBuffer8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppVertexBuffer = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexBuffer8_Vtbl;
    object->ref = 1;
    hrc = IWineD3DDevice_CreateVertexBuffer(This->WineD3DDevice, Size, Usage, FVF, (WINED3DPOOL) Pool, &(object->wineD3DVertexBuffer), NULL, (IUnknown *)object);

    if (D3D_OK != hrc) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateVertexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVertexBuffer = NULL;
    } else {
        *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER8) object;
    }
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    IDirect3DIndexBuffer8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppIndexBuffer = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DIndexBuffer8_Vtbl;
    object->ref = 1;
    TRACE("Calling wined3d create index buffer\n");
    hrc = IWineD3DDevice_CreateIndexBuffer(This->WineD3DDevice, Length, Usage, Format, (WINED3DPOOL) Pool, &object->wineD3DIndexBuffer, NULL, (IUnknown *)object);

    if (D3D_OK != hrc) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateIndexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppIndexBuffer = NULL;
    } else {
        *ppIndexBuffer = (LPDIRECT3DINDEXBUFFER8)object;
    }
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IDirect3DSurface8 **ppSurface,D3DRESOURCETYPE Type, UINT Usage,D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality)  {
    HRESULT hrc;
    IDirect3DSurface8Impl *object;
    IDirect3DDevice8Impl  *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    if(MultisampleQuality < 0) { 
        FIXME("MultisampleQuality out of range %ld, substituting 0\n", MultisampleQuality);
        /*FIXME: Find out what windows does with a MultisampleQuality < 0 */
        MultisampleQuality=0;
    }

    if(MultisampleQuality > 0){
        FIXME("MultisampleQuality set to %ld, substituting 0\n" , MultisampleQuality);
        /*
        MultisampleQuality
        [in] Quality level. The valid range is between zero and one less than the level returned by pQualityLevels used by IDirect3D8::CheckDeviceMultiSampleType. Passing a larger value returns the error D3DERR_INVALIDCALL. The MultisampleQuality values of paired render targets, depth stencil surfaces, and the MultiSample type must all match.
        */
        MultisampleQuality=0;
    }
    /*FIXME: Check MAX bounds of MultisampleQuality*/

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppSurface = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->ref = 1;

    TRACE("(%p) : w(%d) h(%d) fmt(%d) surf@%p\n", This, Width, Height, Format, *ppSurface);

    hrc = IWineD3DDevice_CreateSurface(This->WineD3DDevice, Width, Height, Format, Lockable, Discard, Level,  &object->wineD3DSurface, Type, Usage, (WINED3DPOOL) Pool,MultiSample,MultisampleQuality, NULL,(IUnknown *)object);
    if (hrc != D3D_OK || NULL == object->wineD3DSurface) {
       /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateSurface failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppSurface = NULL;
    } else {
        *ppSurface = (LPDIRECT3DSURFACE8) object;
    }
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    TRACE("Relay\n");

    return IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, Lockable, FALSE /* Discard */, 0 /* Level */ , ppSurface, D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT, MultiSample, 0);
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    TRACE("Relay\n");
    /* TODO: Verify that Discard is false */
    return IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, FALSE, 0 /* Level */
                                               ,ppSurface, D3DRTYPE_SURFACE, D3DUSAGE_DEPTHSTENCIL,
                                                D3DPOOL_DEFAULT, MultiSample, 0);
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    TRACE("Relay\n");

    return IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Loackable */ , FALSE /*Discard*/ , 0 /* Level */ , ppSurface, D3DRTYPE_SURFACE, 0 /* Usage (undefined/none) */ , D3DPOOL_SCRATCH, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);
}

HRESULT WINAPI IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8 *pSourceSurface, CONST RECT *pSourceRects, UINT cRects, IDirect3DSurface8 *pDestinationSurface, CONST POINT *pDestPoints) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_CopyRects(This->WineD3DDevice, pSourceSurface == NULL ? NULL : ((IDirect3DSurface8Impl *)pSourceSurface)->wineD3DSurface,
            pSourceRects, cRects, pDestinationSurface == NULL ? NULL : ((IDirect3DSurface8Impl *)pDestinationSurface)->wineD3DSurface, pDestPoints);
}

HRESULT WINAPI IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_UpdateTexture(This->WineD3DDevice,  ((IDirect3DBaseTexture8Impl *)pSourceTexture)->wineD3DBaseTexture, ((IDirect3DBaseTexture8Impl *)pDestinationTexture)->wineD3DBaseTexture);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *destSurface = (IDirect3DSurface8Impl *)pDestSurface;

    TRACE("(%p) Relay\n" , This);

    if (pDestSurface == NULL) {
        WARN("(%p) : Caller passed NULL as pDestSurface returning D3DERR_INVALIDCALL\n", This);
        return D3DERR_INVALIDCALL;
    }

    return IWineD3DDevice_GetFrontBufferData(This->WineD3DDevice, 0, destSurface->wineD3DSurface);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *pSurface = (IDirect3DSurface8Impl *)pRenderTarget;
    IDirect3DSurface8Impl *pZSurface = (IDirect3DSurface8Impl *)pNewZStencil;
    TRACE("(%p) Relay\n" , This);

    IWineD3DDevice_SetDepthStencilSurface(This->WineD3DDevice, NULL == pZSurface ? NULL : (IWineD3DSurface *)pZSurface->wineD3DSurface);

    return IWineD3DDevice_SetRenderTarget(This->WineD3DDevice, 0, (IWineD3DSurface *)pSurface->wineD3DSurface);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pRenderTarget;

    TRACE("(%p) Relay\n" , This);

    if (ppRenderTarget == NULL) {
        return D3DERR_INVALIDCALL;
    }
    hr = IWineD3DDevice_GetRenderTarget(This->WineD3DDevice, 0, &pRenderTarget);

    if (hr == D3D_OK && pRenderTarget != NULL) {
        IWineD3DResource_GetParent((IWineD3DResource *)pRenderTarget,(IUnknown**)ppRenderTarget);
        IWineD3DResource_Release((IWineD3DResource *)pRenderTarget);
    } else {
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed\n");
        *ppRenderTarget = NULL;
    }

    return hr;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pZStencilSurface;

    TRACE("(%p) Relay\n" , This);
    if(ppZStencilSurface == NULL){
        return D3DERR_INVALIDCALL;
    }

    hr=IWineD3DDevice_GetDepthStencilSurface(This->WineD3DDevice,&pZStencilSurface);
    if(hr == D3D_OK && pZStencilSurface != NULL){
        IWineD3DResource_GetParent((IWineD3DResource *)pZStencilSurface,(IUnknown**)ppZStencilSurface);
        IWineD3DResource_Release((IWineD3DResource *)pZStencilSurface);
    }else{
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed\n");
        *ppZStencilSurface = NULL;
    }

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    return IWineD3DDevice_BeginScene(This->WineD3DDevice);
}

HRESULT WINAPI IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_EndScene(This->WineD3DDevice);
}

HRESULT WINAPI IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_Clear(This->WineD3DDevice, Count, pRects, Flags, Color, Z, Stencil);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* lpMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetTransform(This->WineD3DDevice, State, lpMatrix);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetTransform(This->WineD3DDevice, State, pMatrix);
}

HRESULT WINAPI IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_MultiplyTransform(This->WineD3DDevice, State, pMatrix);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetViewport(This->WineD3DDevice, (const WINED3DVIEWPORT *)pViewport);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetViewport(This->WineD3DDevice, (WINED3DVIEWPORT *)pViewport);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
/* FIXME: Verify that D3DMATERIAL8 ~= WINED3DMATERIAL */
    return IWineD3DDevice_SetMaterial(This->WineD3DDevice, (const WINED3DMATERIAL *)pMaterial);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
/* FIXME: Verify that D3DMATERIAL8 ~= WINED3DMATERIAL */
    return IWineD3DDevice_GetMaterial(This->WineD3DDevice, (WINED3DMATERIAL *)pMaterial);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST D3DLIGHT8* pLight) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
/* FIXME: Verify that D3DLIGHT8 ~= WINED3DLIGHT */
    return IWineD3DDevice_SetLight(This->WineD3DDevice, Index, (const WINED3DLIGHT *)pLight);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,D3DLIGHT8* pLight) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
/* FIXME: Verify that D3DLIGHT8 ~= WINED3DLIGHT */
    return IWineD3DDevice_GetLight(This->WineD3DDevice, Index, (WINED3DLIGHT *)pLight);
}

HRESULT WINAPI IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetLightEnable(This->WineD3DDevice, Index, Enable);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetLightEnable(This->WineD3DDevice, Index, pEnable);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetClipPlane(This->WineD3DDevice, Index, pPlane);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetClipPlane(This->WineD3DDevice, Index, pPlane);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetRenderState(This->WineD3DDevice, State, Value);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetRenderState(This->WineD3DDevice, State, pValue);
}

HRESULT WINAPI IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface) {
  IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

  TRACE("(%p)\n", This);

  return IWineD3DDevice_BeginStateBlock(This->WineD3DDevice);
}

HRESULT WINAPI IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    IWineD3DStateBlock* wineD3DStateBlock;
    IDirect3DStateBlock8Impl* object;

    TRACE("(%p) Relay\n", This);

    /* Tell wineD3D to endstatablock before anything else (in case we run out
     * of memory later and cause locking problems)
     */
    hr = IWineD3DDevice_EndStateBlock(This->WineD3DDevice , &wineD3DStateBlock);
    if (hr != D3D_OK) {
       FIXME("IWineD3DDevice_EndStateBlock returned an error\n");
       return hr;
    }

    /* allocate a new IDirectD3DStateBlock */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY ,sizeof(IDirect3DStateBlock8Impl));
    object->ref = 1;
    object->lpVtbl = &Direct3DStateBlock8_Vtbl;

    object->wineD3DStateBlock = wineD3DStateBlock;

    *pToken = (DWORD)object;
    TRACE("(%p)Returning %p %p\n", This, object, wineD3DStateBlock);

    return hr;
}

HRESULT WINAPI IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DStateBlock8Impl *pSB  = (IDirect3DStateBlock8Impl*) Token;
    IDirect3DDevice8Impl     *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) %p Relay\n", This, pSB);

    return  IWineD3DStateBlock_Apply(pSB->wineD3DStateBlock);
}

HRESULT WINAPI IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DStateBlock8Impl* pSB = (IDirect3DStateBlock8Impl *)Token;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) %p Relay\n", This, pSB);

    return IWineD3DStateBlock_Capture(pSB->wineD3DStateBlock);
}

HRESULT WINAPI IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DStateBlock8Impl* pSB = (IDirect3DStateBlock8Impl *)Token;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) Relay\n", This);

    while(IUnknown_Release((IUnknown *)pSB));

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
   IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
   IDirect3DStateBlock8Impl *object;
   HRESULT hrc = D3D_OK;

   TRACE("(%p) Relay\n", This);

   object  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DStateBlock8Impl));
   if (NULL == object) {
      FIXME("(%p)  Failed to allocate %d bytes\n", This, sizeof(IDirect3DStateBlock8Impl));
      *pToken = 0;
      return E_OUTOFMEMORY;
   }
   object->lpVtbl = &Direct3DStateBlock8_Vtbl;
   object->ref = 1;

   hrc = IWineD3DDevice_CreateStateBlock(This->WineD3DDevice, (WINED3DSTATEBLOCKTYPE)Type, &object->wineD3DStateBlock, (IUnknown *)object);
   if(D3D_OK != hrc){
       FIXME("(%p) Call to IWineD3DDevice_CreateStateBlock failed.\n", This);
       HeapFree(GetProcessHeap(), 0, object);
       *pToken = 0;
   } else {
       *pToken = (DWORD)object;
   }
   TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, object);

   return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
/* FIXME: Verify that D3DCLIPSTATUS8 ~= WINED3DCLIPSTATUS */
    return IWineD3DDevice_SetClipStatus(This->WineD3DDevice, (const WINED3DCLIPSTATUS *)pClipStatus);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetClipStatus(This->WineD3DDevice, (WINED3DCLIPSTATUS *)pClipStatus);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8** ppTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DBaseTexture *retTexture = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n" , This);

    if(ppTexture == NULL){
        return D3DERR_INVALIDCALL;
    }

    rc = IWineD3DDevice_GetTexture(This->WineD3DDevice, Stage, (IWineD3DBaseTexture **)&retTexture);
    if (rc == D3D_OK && NULL != retTexture) {
        IWineD3DBaseTexture_GetParent(retTexture, (IUnknown **)ppTexture);
        IWineD3DBaseTexture_Release(retTexture);
    } else {
        FIXME("Call to get texture  (%ld) failed (%p)\n", Stage, retTexture);
        *ppTexture = NULL;
    }

    return rc;
}

HRESULT WINAPI IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8* pTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay %ld %p\n" , This, Stage, pTexture);

    return IWineD3DDevice_SetTexture(This->WineD3DDevice, Stage,
                                     pTexture==NULL ? NULL : ((IDirect3DBaseTexture8Impl *)pTexture)->wineD3DBaseTexture);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    switch(Type) {
    case D3DTSS_ADDRESSU:
        Type = WINED3DSAMP_ADDRESSU;
        break;
    case D3DTSS_ADDRESSV:
        Type = WINED3DSAMP_ADDRESSV;
        break;
    case D3DTSS_ADDRESSW:
        Type = WINED3DSAMP_ADDRESSW;
        break;
    case D3DTSS_BORDERCOLOR:
        Type = WINED3DSAMP_BORDERCOLOR;
        break;
    case D3DTSS_MAGFILTER:
        Type = WINED3DSAMP_MAGFILTER;
        break;
    case D3DTSS_MAXANISOTROPY:
        Type = WINED3DSAMP_MAXANISOTROPY;
        break;
    case D3DTSS_MAXMIPLEVEL:
        Type = WINED3DSAMP_MAXMIPLEVEL;
        break;
    case D3DTSS_MINFILTER:
        Type = WINED3DSAMP_MINFILTER;
        break;
    case D3DTSS_MIPFILTER:
        Type = WINED3DSAMP_MIPFILTER;
        break;
    case D3DTSS_MIPMAPLODBIAS:
        Type = WINED3DSAMP_MIPMAPLODBIAS;
        break;
    default:
        return IWineD3DDevice_GetTextureStageState(This->WineD3DDevice, Stage, Type, pValue);
    }

    return IWineD3DDevice_GetSamplerState(This->WineD3DDevice, Stage, Type, pValue);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    switch(Type) {
    case D3DTSS_ADDRESSU:
        Type = WINED3DSAMP_ADDRESSU;
        break;
    case D3DTSS_ADDRESSV:
        Type = WINED3DSAMP_ADDRESSV;
        break;
    case D3DTSS_ADDRESSW:
        Type = WINED3DSAMP_ADDRESSW;
        break;
    case D3DTSS_BORDERCOLOR:
        Type = WINED3DSAMP_BORDERCOLOR;
        break;
    case D3DTSS_MAGFILTER:
        Type = WINED3DSAMP_MAGFILTER;
        break;
    case D3DTSS_MAXANISOTROPY:
        Type = WINED3DSAMP_MAXANISOTROPY;
        break;
    case D3DTSS_MAXMIPLEVEL:
        Type = WINED3DSAMP_MAXMIPLEVEL;
        break;
    case D3DTSS_MINFILTER:
        Type = WINED3DSAMP_MINFILTER;
        break;
    case D3DTSS_MIPFILTER:
        Type = WINED3DSAMP_MIPFILTER;
        break;
    case D3DTSS_MIPMAPLODBIAS:
        Type = WINED3DSAMP_MIPMAPLODBIAS;
        break;
    default:
        return IWineD3DDevice_SetTextureStageState(This->WineD3DDevice, Stage, Type, Value);
    }

    return IWineD3DDevice_SetSamplerState(This->WineD3DDevice, Stage, Type, Value);
}

HRESULT WINAPI IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_ValidateDevice(This->WineD3DDevice, pNumPasses);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
}

HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_GetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
}

HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface; 
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_DrawPrimitive(This->WineD3DDevice, PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_DrawIndexedPrimitive(This->WineD3DDevice, PrimitiveType, This->baseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_DrawPrimitiveUP(This->WineD3DDevice, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,
                                                             UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_DrawIndexedPrimitiveUP(This->WineD3DDevice, PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount,
                                                 pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT WINAPI IDirect3DDevice8Impl_ProcessVertices(LPDIRECT3DDEVICE8 iface, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer8* pDestBuffer,DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_ProcessVertices(This->WineD3DDevice,SrcStartIndex, DestIndex, VertexCount, ((IDirect3DVertexBuffer8Impl *)pDestBuffer)->wineD3DVertexBuffer, NULL, Flags);
}

HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* ppShader, DWORD Usage) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IDirect3DVertexShader8Impl *object;

    /* Setup a stub object for now */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    TRACE("(%p) : pFunction(%p), ppShader(%p)\n", This, pFunction, ppShader);
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppShader = 0;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->ref = 1;
    object->lpVtbl = &Direct3DVertexShader8_Vtbl;
    /* Usage is missing ..*/
    hrc = IWineD3DDevice_CreateVertexShader(This->WineD3DDevice, pDeclaration, pFunction, &object->wineD3DVertexShader, (IUnknown *)object);

    if (FAILED(hrc)) {
        /* free up object */
        FIXME("Call to IWineD3DDevice_CreateVertexShader failed\n");
        HeapFree(GetProcessHeap(), 0, object);
        *ppShader = 0;
    } else {
        /* TODO: Store the VS declarations locally so that they can be derefferenced with a value higher than VS_HIGHESTFIXEDFXF */
        DWORD i = 0;
        while(This->vShaders[i] != NULL && i < MAX_SHADERS) ++i;
        if (MAX_SHADERS == i) {
            FIXME("(%p) : Number of shaders exceeds the maximum number of possible shaders\n", This);
            hrc = E_OUTOFMEMORY;
        } else {
            This->vShaders[i] = object;
            *ppShader = i + VS_HIGHESTFIXEDFXF + 1;
        }
    }
    TRACE("(%p) : returning %p\n", This, object);

    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : Relay\n", This);
    if (VS_HIGHESTFIXEDFXF >= pShader) {
        TRACE("Setting FVF, %d %ld\n", VS_HIGHESTFIXEDFXF, pShader);
        IWineD3DDevice_SetFVF(This->WineD3DDevice, pShader);

	/* Call SetVertexShader with a NULL shader to set the vertexshader in the stateblock to NULL. */
        IWineD3DDevice_SetVertexShader(This->WineD3DDevice, NULL);
    } else {
        TRACE("Setting shader\n");
        if (MAX_SHADERS <= pShader - (VS_HIGHESTFIXEDFXF + 1)) {
            FIXME("(%p) : Number of shaders exceeds the maximum number of possible shaders\n", This);
            hrc = D3DERR_INVALIDCALL;
        } else {
            IDirect3DVertexShader8Impl *shader = This->vShaders[pShader - (VS_HIGHESTFIXEDFXF + 1)];
            hrc =  IWineD3DDevice_SetVertexShader(This->WineD3DDevice, 0 == shader ? NULL : shader->wineD3DVertexShader);
        }
    }
    TRACE("(%p) : returning hr(%lu)\n", This, hrc);

    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DVertexShader *pShader;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : Relay  device@%p\n", This, This->WineD3DDevice);
    hrc = IWineD3DDevice_GetVertexShader(This->WineD3DDevice, &pShader);
    if (D3D_OK == hrc) {
        if(0 != pShader) {
            DWORD i = 0;
            hrc = IWineD3DVertexShader_GetParent(pShader, (IUnknown **)ppShader);
            IWineD3DVertexShader_Release(pShader);
            while(This->vShaders[i] != (IDirect3DVertexShader8Impl *)ppShader && i < MAX_SHADERS) ++i;
            if (i < MAX_SHADERS) {
                *ppShader = i + VS_HIGHESTFIXEDFXF + 1;
            } else {
                WARN("(%p) : Couldn't find math for shader %p in d3d7 shadres list\n", This, (IDirect3DVertexShader8Impl *)ppShader);
                *ppShader = 0;
            }
        } else {
            WARN("(%p) : The shader has been set to NULL\n", This);

            /* TODO: Find out what should be returned, e.g. the FVF */
            *ppShader = 0;
            hrc = D3DERR_INVALIDCALL;
        }
    } else {
        WARN("(%p) : Call to IWineD3DDevice_GetVertexShader failed %lu (device %p)\n", This, hrc, This->WineD3DDevice);
    }
    TRACE("(%p) : returning %p\n", This, (IDirect3DVertexShader8 *)*ppShader);

    return hrc;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;
    TRACE("(%p) Relay\n", This);
    if (pShader <= VS_HIGHESTFIXEDFXF) {
        WARN("(%p) : Caller passed a shader below the value of VS_HIGHESTFIXEDFXF\n", This);
        hrc = D3DERR_INVALIDCALL;
    } else if (MAX_SHADERS <= pShader - (VS_HIGHESTFIXEDFXF + 1)) {
        FIXME("(%p) : Caller passed a shader greater than the maximum number of shaders\n", This);
        hrc = D3DERR_INVALIDCALL;
    } else {
        IDirect3DVertexShader8Impl *shader = This->vShaders[pShader - (VS_HIGHESTFIXEDFXF + 1)];
        while(IUnknown_Release((IUnknown *)shader));
        This->vShaders[pShader - (VS_HIGHESTFIXEDFXF + 1)] = NULL;
        hrc = D3D_OK;
    }

    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Relay\n", This);

    return IWineD3DDevice_SetVertexShaderConstantF(This->WineD3DDevice, Register, (CONST float *)pConstantData, ConstantCount);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) : Relay\n", This);

    return IWineD3DDevice_GetVertexShaderConstantF(This->WineD3DDevice, Register, (float *)pConstantData, ConstantCount);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderDeclaration(LPDIRECT3DDEVICE8 iface, DWORD pVertexShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)pVertexShader;

    TRACE("(%p) : Relay\n", This);
/*    return IWineD3DVertexShader_GetDeclaration(This->wineD3DVertexShader, pData, (UINT *)pSizeOfData); */
    return D3DERR_INVALIDCALL;
}
HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD pVertexShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)pVertexShader;

    TRACE("(%p) : Relay\n", This);
    return IWineD3DVertexShader_GetFunction(This->wineD3DVertexShader, pData, (UINT *)pSizeOfData);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData, UINT baseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
/* FIXME: store base vertex index properly */
    This->baseVertexIndex = baseVertexIndex;
    return IWineD3DDevice_SetIndices(This->WineD3DDevice,
                                     NULL == pIndexData ? NULL : ((IDirect3DIndexBuffer8Impl *)pIndexData)->wineD3DIndexBuffer,
                                     0);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DIndexBuffer *retIndexData = NULL;
    HRESULT rc = D3D_OK;
    UINT tmp;

    TRACE("(%p) Relay\n", This);

    if(ppIndexData == NULL){
        return D3DERR_INVALIDCALL;
    }

    rc = IWineD3DDevice_GetIndices(This->WineD3DDevice, &retIndexData, &tmp);
    if (D3D_OK == rc && NULL != retIndexData) {
        IWineD3DVertexBuffer_GetParent(retIndexData, (IUnknown **)ppIndexData);
        IWineD3DVertexBuffer_Release(retIndexData);
    } else {
        if(rc != D3D_OK)  FIXME("Call to GetIndices failed\n");
        *ppIndexData = NULL;
    }
/* FIXME: store base vertex index properly */
    *pBaseVertexIndex = This->baseVertexIndex;
    return rc;
}
HRESULT WINAPI IDirect3DDevice8Impl_CreatePixelShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pFunction, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *object;
    HRESULT hrc = D3D_OK;

    if (NULL == ppShader) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));

    if (NULL == object) {
        return E_OUTOFMEMORY;
    } else {

        object->ref    = 1;
        object->lpVtbl = &Direct3DPixelShader8_Vtbl;
        hrc = IWineD3DDevice_CreatePixelShader(This->WineD3DDevice, pFunction, &object->wineD3DPixelShader , (IUnknown *)object);
        if (D3D_OK != hrc) {
            FIXME("(%p) call to IWineD3DDevice_CreatePixelShader failed\n", This);
            HeapFree(GetProcessHeap(), 0 , object);
            *ppShader = 0;
        } else {
            *ppShader = (DWORD)object;
        }

    }

    TRACE("(%p) : returning %p\n", This, (void *)*ppShader);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_SetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *shader = (IDirect3DPixelShader8Impl *)pShader;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_SetPixelShader(This->WineD3DDevice, shader == NULL ? NULL :shader->wineD3DPixelShader);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DPixelShader *object;

    HRESULT hrc = D3D_OK;
    TRACE("(%p) Relay\n", This);
    if (NULL == ppShader) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    hrc = IWineD3DDevice_GetPixelShader(This->WineD3DDevice, &object);
    if (D3D_OK == hrc && NULL != object) {
       hrc = IWineD3DPixelShader_GetParent(object, (IUnknown **)ppShader);
       IWineD3DPixelShader_Release(object);
    } else {
        *ppShader = (DWORD)NULL;
    }

    TRACE("(%p) : returning %p\n", This, (void *)*ppShader);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *shader = (IDirect3DPixelShader8Impl *)pShader;
    TRACE("(%p) Relay\n", This);

    if (NULL != shader) {
        while(IUnknown_Release((IUnknown *)shader));
    }

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_SetPixelShaderConstantF(This->WineD3DDevice, Register, (CONST float *)pConstantData, ConstantCount);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_GetPixelShaderConstantF(This->WineD3DDevice, Register, (float *)pConstantData, ConstantCount);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD pPixelShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)pPixelShader;

    TRACE("(%p) : Relay\n", This);
    return IWineD3DPixelShader_GetFunction(This->wineD3DPixelShader, pData, (UINT *)pSizeOfData);
}

HRESULT WINAPI IDirect3DDevice8Impl_DrawRectPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_DrawRectPatch(This->WineD3DDevice, Handle, pNumSegs, (WINED3DRECTPATCH_INFO *)pRectPatchInfo);
}

HRESULT WINAPI IDirect3DDevice8Impl_DrawTriPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_DrawTriPatch(This->WineD3DDevice, Handle, pNumSegs, (WINED3DTRIPATCH_INFO *)pTriPatchInfo);
}

HRESULT WINAPI IDirect3DDevice8Impl_DeletePatch(LPDIRECT3DDEVICE8 iface, UINT Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    return IWineD3DDevice_DeletePatch(This->WineD3DDevice, Handle);
}

HRESULT WINAPI IDirect3DDevice8Impl_SetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8* pStreamData,UINT Stride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    return IWineD3DDevice_SetStreamSource(This->WineD3DDevice, StreamNumber,
                                          NULL == pStreamData ? NULL : ((IDirect3DVertexBuffer8Impl *)pStreamData)->wineD3DVertexBuffer,
                                          0/* Offset in bytes */, Stride);
}

HRESULT WINAPI IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** pStream,UINT* pStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DVertexBuffer *retStream = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n" , This);

    if(pStream == NULL){
        return D3DERR_INVALIDCALL;
    }

    rc = IWineD3DDevice_GetStreamSource(This->WineD3DDevice, StreamNumber, (IWineD3DVertexBuffer **)&retStream, 0 /* Offset in bytes */, pStride);
    if (rc == D3D_OK  && NULL != retStream) {
        IWineD3DVertexBuffer_GetParent(retStream, (IUnknown **)pStream);
        IWineD3DVertexBuffer_Release(retStream);
    }else{
         FIXME("Call to GetStreamSource failed %p\n",  pStride);
        *pStream = NULL;
    }

    return rc;
}


const IDirect3DDevice8Vtbl Direct3DDevice8_Vtbl =
{
    IDirect3DDevice8Impl_QueryInterface,
    IDirect3DDevice8Impl_AddRef,
    IDirect3DDevice8Impl_Release,
    IDirect3DDevice8Impl_TestCooperativeLevel,
    IDirect3DDevice8Impl_GetAvailableTextureMem,
    IDirect3DDevice8Impl_ResourceManagerDiscardBytes,
    IDirect3DDevice8Impl_GetDirect3D,
    IDirect3DDevice8Impl_GetDeviceCaps,
    IDirect3DDevice8Impl_GetDisplayMode,
    IDirect3DDevice8Impl_GetCreationParameters,
    IDirect3DDevice8Impl_SetCursorProperties,
    IDirect3DDevice8Impl_SetCursorPosition,
    IDirect3DDevice8Impl_ShowCursor,
    IDirect3DDevice8Impl_CreateAdditionalSwapChain,
    IDirect3DDevice8Impl_Reset,
    IDirect3DDevice8Impl_Present,
    IDirect3DDevice8Impl_GetBackBuffer,
    IDirect3DDevice8Impl_GetRasterStatus,
    IDirect3DDevice8Impl_SetGammaRamp,
    IDirect3DDevice8Impl_GetGammaRamp,
    IDirect3DDevice8Impl_CreateTexture,
    IDirect3DDevice8Impl_CreateVolumeTexture,
    IDirect3DDevice8Impl_CreateCubeTexture,
    IDirect3DDevice8Impl_CreateVertexBuffer,
    IDirect3DDevice8Impl_CreateIndexBuffer,
    IDirect3DDevice8Impl_CreateRenderTarget,
    IDirect3DDevice8Impl_CreateDepthStencilSurface,
    IDirect3DDevice8Impl_CreateImageSurface,
    IDirect3DDevice8Impl_CopyRects,
    IDirect3DDevice8Impl_UpdateTexture,
    IDirect3DDevice8Impl_GetFrontBuffer,
    IDirect3DDevice8Impl_SetRenderTarget,
    IDirect3DDevice8Impl_GetRenderTarget,
    IDirect3DDevice8Impl_GetDepthStencilSurface,
    IDirect3DDevice8Impl_BeginScene,
    IDirect3DDevice8Impl_EndScene,
    IDirect3DDevice8Impl_Clear,
    IDirect3DDevice8Impl_SetTransform,
    IDirect3DDevice8Impl_GetTransform,
    IDirect3DDevice8Impl_MultiplyTransform,
    IDirect3DDevice8Impl_SetViewport,
    IDirect3DDevice8Impl_GetViewport,
    IDirect3DDevice8Impl_SetMaterial,
    IDirect3DDevice8Impl_GetMaterial,
    IDirect3DDevice8Impl_SetLight,
    IDirect3DDevice8Impl_GetLight,
    IDirect3DDevice8Impl_LightEnable,
    IDirect3DDevice8Impl_GetLightEnable,
    IDirect3DDevice8Impl_SetClipPlane,
    IDirect3DDevice8Impl_GetClipPlane,
    IDirect3DDevice8Impl_SetRenderState,
    IDirect3DDevice8Impl_GetRenderState,
    IDirect3DDevice8Impl_BeginStateBlock,
    IDirect3DDevice8Impl_EndStateBlock,
    IDirect3DDevice8Impl_ApplyStateBlock,
    IDirect3DDevice8Impl_CaptureStateBlock,
    IDirect3DDevice8Impl_DeleteStateBlock,
    IDirect3DDevice8Impl_CreateStateBlock,
    IDirect3DDevice8Impl_SetClipStatus,
    IDirect3DDevice8Impl_GetClipStatus,
    IDirect3DDevice8Impl_GetTexture,
    IDirect3DDevice8Impl_SetTexture,
    IDirect3DDevice8Impl_GetTextureStageState,
    IDirect3DDevice8Impl_SetTextureStageState,
    IDirect3DDevice8Impl_ValidateDevice,
    IDirect3DDevice8Impl_GetInfo,
    IDirect3DDevice8Impl_SetPaletteEntries,
    IDirect3DDevice8Impl_GetPaletteEntries,
    IDirect3DDevice8Impl_SetCurrentTexturePalette,
    IDirect3DDevice8Impl_GetCurrentTexturePalette,
    IDirect3DDevice8Impl_DrawPrimitive,
    IDirect3DDevice8Impl_DrawIndexedPrimitive,
    IDirect3DDevice8Impl_DrawPrimitiveUP,
    IDirect3DDevice8Impl_DrawIndexedPrimitiveUP,
    IDirect3DDevice8Impl_ProcessVertices,
    IDirect3DDevice8Impl_CreateVertexShader,
    IDirect3DDevice8Impl_SetVertexShader,
    IDirect3DDevice8Impl_GetVertexShader,
    IDirect3DDevice8Impl_DeleteVertexShader,
    IDirect3DDevice8Impl_SetVertexShaderConstant,
    IDirect3DDevice8Impl_GetVertexShaderConstant,
    IDirect3DDevice8Impl_GetVertexShaderDeclaration,
    IDirect3DDevice8Impl_GetVertexShaderFunction,
    IDirect3DDevice8Impl_SetStreamSource,
    IDirect3DDevice8Impl_GetStreamSource,
    IDirect3DDevice8Impl_SetIndices,
    IDirect3DDevice8Impl_GetIndices,
    IDirect3DDevice8Impl_CreatePixelShader,
    IDirect3DDevice8Impl_SetPixelShader,
    IDirect3DDevice8Impl_GetPixelShader,
    IDirect3DDevice8Impl_DeletePixelShader,
    IDirect3DDevice8Impl_SetPixelShaderConstant,
    IDirect3DDevice8Impl_GetPixelShaderConstant,
    IDirect3DDevice8Impl_GetPixelShaderFunction,
    IDirect3DDevice8Impl_DrawRectPatch,
    IDirect3DDevice8Impl_DrawTriPatch,
    IDirect3DDevice8Impl_DeletePatch
};

/* Internal function called back during the CreateDevice to create a render target  */
HRESULT WINAPI D3D8CB_CreateSurface(IUnknown *device, UINT Width, UINT Height, 
                                         WINED3DFORMAT Format, DWORD Usage, WINED3DPOOL Pool, UINT Level,
                                         IWineD3DSurface **ppSurface, HANDLE *pSharedHandle) {

    HRESULT res = D3D_OK;
    IDirect3DSurface8Impl *d3dSurface = NULL;
    BOOL Lockable = TRUE;

    if((WINED3DPOOL_DEFAULT == Pool && WINED3DUSAGE_DYNAMIC != Usage))
        Lockable = FALSE;

    TRACE("relay\n");
    res = IDirect3DDevice8Impl_CreateSurface((IDirect3DDevice8 *)device, Width, Height, (D3DFORMAT)Format, Lockable, FALSE/*Discard*/, Level,  (IDirect3DSurface8 **)&d3dSurface, D3DRTYPE_SURFACE, Usage, Pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);

    if (res == D3D_OK) {
        *ppSurface = d3dSurface->wineD3DSurface;
    } else {
        FIXME("(%p) IDirect3DDevice8_CreateSurface failed\n", device);
    }
    return res;
}
