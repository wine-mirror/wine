/*
 * IWineD3DResource Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* IDirect3DResource IUnknown parts follow: */
HRESULT WINAPI IWineD3DResourceImpl_QueryInterface(IWineD3DResource *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DResource)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DResourceImpl_AddRef(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %ld\n", This, ref - 1);
    return ref; 
}

ULONG WINAPI IWineD3DResourceImpl_Release(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %ld\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DResourceImpl_CleanUp(iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* class static (not in vtable) */
void IWineD3DResourceImpl_CleanUp(IWineD3DResource *iface){
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) Cleaning up resource\n", This);
    if (This->resource.pool == D3DPOOL_DEFAULT) {
        TRACE("Decrementing device memory pool by %u\n", This->resource.size);
        globalChangeGlRam(-This->resource.size);
    }

    HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
    This->resource.allocatedMemory = 0;
}

/* IDirect3DResource Interface follow: */
HRESULT WINAPI IWineD3DResourceImpl_GetDevice(IWineD3DResource *iface, IWineD3DDevice** ppDevice) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->resource.wineD3DDevice);
    *ppDevice = (IWineD3DDevice *) This->resource.wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);
    return D3D_OK;
}

/* Private Date is not implemented yet */
HRESULT WINAPI IWineD3DResourceImpl_SetPrivateData(IWineD3DResource *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT WINAPI IWineD3DResourceImpl_GetPrivateData(IWineD3DResource *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT WINAPI IWineD3DResourceImpl_FreePrivateData(IWineD3DResource *iface, REFGUID refguid) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}

/* Priority support is not implemented yet */
DWORD    WINAPI        IWineD3DResourceImpl_SetPriority(IWineD3DResource *iface, DWORD PriorityNew) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IWineD3DResourceImpl_GetPriority(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}

/* Preloading of resources is not supported yet */
void     WINAPI        IWineD3DResourceImpl_PreLoad(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);
}

D3DRESOURCETYPE WINAPI IWineD3DResourceImpl_GetType(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->resource.resourceType);
    return This->resource.resourceType;
}

HRESULT WINAPI IWineD3DResourceImpl_GetParent(IWineD3DResource *iface, IUnknown **pParent) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    IUnknown_AddRef(This->resource.parent);
    *pParent = This->resource.parent;
    return D3D_OK;
}


static const IWineD3DResourceVtbl IWineD3DResource_Vtbl =
{
    IWineD3DResourceImpl_QueryInterface,
    IWineD3DResourceImpl_AddRef,
    IWineD3DResourceImpl_Release,
    IWineD3DResourceImpl_GetParent,
    IWineD3DResourceImpl_GetDevice,
    IWineD3DResourceImpl_SetPrivateData,
    IWineD3DResourceImpl_GetPrivateData,
    IWineD3DResourceImpl_FreePrivateData,
    IWineD3DResourceImpl_SetPriority,
    IWineD3DResourceImpl_GetPriority,
    IWineD3DResourceImpl_PreLoad,
    IWineD3DResourceImpl_GetType
};
