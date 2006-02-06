/*
 * IWineD3DVertexBuffer Implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
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

/* *******************************************
   IWineD3DVertexBuffer IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVertexBufferImpl_QueryInterface(IWineD3DVertexBuffer *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DVertexBuffer)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVertexBufferImpl_AddRef(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %ld\n", This, ref - 1);
    return ref;
}

ULONG WINAPI IWineD3DVertexBufferImpl_Release(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %ld\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVertexBuffer IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DVertexBufferImpl_GetDevice(IWineD3DVertexBuffer *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DVertexBufferImpl_SetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DVertexBufferImpl_GetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DVertexBufferImpl_FreePrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD    WINAPI        IWineD3DVertexBufferImpl_SetPriority(IWineD3DVertexBuffer *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD    WINAPI        IWineD3DVertexBufferImpl_GetPriority(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void     WINAPI        IWineD3DVertexBufferImpl_PreLoad(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_PreLoad((IWineD3DResource *)iface);
}

D3DRESOURCETYPE WINAPI IWineD3DVertexBufferImpl_GetType(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DVertexBufferImpl_GetParent(IWineD3DVertexBuffer *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DVertexBuffer IWineD3DVertexBuffer parts follow
   ****************************************************** */
HRESULT  WINAPI        IWineD3DVertexBufferImpl_Lock(IWineD3DVertexBuffer *iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    TRACE("(%p) : returning memory of %p (base:%p,offset:%u)\n", This, This->resource.allocatedMemory + OffsetToLock, This->resource.allocatedMemory, OffsetToLock);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */
    *ppbData = This->resource.allocatedMemory + OffsetToLock;
    return D3D_OK;
}
HRESULT  WINAPI        IWineD3DVertexBufferImpl_Unlock(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IWineD3DVertexBufferImpl_GetDesc(IWineD3DVertexBuffer *iface, D3DVERTEXBUFFER_DESC *pDesc) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    TRACE("(%p)\n", This);
    pDesc->Format = This->resource.format;
    pDesc->Type   = This->resource.resourceType;
    pDesc->Usage  = This->resource.usage;
    pDesc->Pool   = This->resource.pool;
    pDesc->Size   = This->resource.size;
    pDesc->FVF    = This->fvf;
    return D3D_OK;
}

const IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexBufferImpl_QueryInterface,
    IWineD3DVertexBufferImpl_AddRef,
    IWineD3DVertexBufferImpl_Release,
    /* IWineD3DResource */
    IWineD3DVertexBufferImpl_GetParent,
    IWineD3DVertexBufferImpl_GetDevice,
    IWineD3DVertexBufferImpl_SetPrivateData,
    IWineD3DVertexBufferImpl_GetPrivateData,
    IWineD3DVertexBufferImpl_FreePrivateData,
    IWineD3DVertexBufferImpl_SetPriority,
    IWineD3DVertexBufferImpl_GetPriority,
    IWineD3DVertexBufferImpl_PreLoad,
    IWineD3DVertexBufferImpl_GetType,
    /* IWineD3DVertexBuffer */
    IWineD3DVertexBufferImpl_Lock,
    IWineD3DVertexBufferImpl_Unlock,
    IWineD3DVertexBufferImpl_GetDesc
};

BYTE* WINAPI IWineD3DVertexBufferImpl_GetMemory(IWineD3DVertexBuffer* iface, DWORD iOffset) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    return This->resource.allocatedMemory + iOffset;
}

HRESULT WINAPI IWineD3DVertexBufferImpl_ReleaseMemory(IWineD3DVertexBuffer* iface) {
  return D3D_OK;
}
