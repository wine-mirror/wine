/*
 * IWineD3DIndexBuffer Implementation
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

/* *******************************************
   IWineD3DIndexBuffer IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DIndexBufferImpl_QueryInterface(IWineD3DIndexBuffer *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    WARN("(%p)->(%s,%p) should not be called\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DIndexBufferImpl_AddRef(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->resource.ref);
    IUnknown_AddRef(This->resource.parent);
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DIndexBufferImpl_Release(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        if (NULL != This->allocatedMemory) HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
        IWineD3DDevice_Release(This->resource.wineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This);
    } else {
        IUnknown_Release(This->resource.parent);  /* Released the reference to the d3dx VB */
    }
    return ref;
}

/* ****************************************************
   IWineD3DIndexBuffer IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DIndexBufferImpl_GetDevice(IWineD3DIndexBuffer *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResource_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DIndexBufferImpl_SetPrivateData(IWineD3DIndexBuffer *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResource_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DIndexBufferImpl_GetPrivateData(IWineD3DIndexBuffer *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResource_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DIndexBufferImpl_FreePrivateData(IWineD3DIndexBuffer *iface, REFGUID refguid) {
    return IWineD3DResource_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD    WINAPI        IWineD3DIndexBufferImpl_SetPriority(IWineD3DIndexBuffer *iface, DWORD PriorityNew) {
    return IWineD3DResource_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD    WINAPI        IWineD3DIndexBufferImpl_GetPriority(IWineD3DIndexBuffer *iface) {
    return IWineD3DResource_GetPriority((IWineD3DResource *)iface);
}

void     WINAPI        IWineD3DIndexBufferImpl_PreLoad(IWineD3DIndexBuffer *iface) {
    return IWineD3DResource_PreLoad((IWineD3DResource *)iface);
}

D3DRESOURCETYPE WINAPI IWineD3DIndexBufferImpl_GetType(IWineD3DIndexBuffer *iface) {
    return IWineD3DResource_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DIndexBufferImpl_GetParent(IWineD3DIndexBuffer *iface, IUnknown **pParent) {
    return IWineD3DResource_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DIndexBuffer IWineD3DIndexBuffer parts follow
   ****************************************************** */
HRESULT  WINAPI        IWineD3DIndexBufferImpl_Lock(IWineD3DIndexBuffer *iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    TRACE("(%p) : no real locking yet, offset %d, size %d, Flags=%lx\n", This, OffsetToLock, SizeToLock, Flags);
    *ppbData = (BYTE *)This->allocatedMemory + OffsetToLock;
    return D3D_OK;
}
HRESULT  WINAPI        IWineD3DIndexBufferImpl_Unlock(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IWineD3DIndexBufferImpl_GetDesc(IWineD3DIndexBuffer *iface, D3DINDEXBUFFER_DESC *pDesc) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;

    TRACE("(%p)\n", This);
    pDesc->Format = This->currentDesc.Format;
    pDesc->Type   = This->resource.resourceType;
    pDesc->Usage  = This->currentDesc.Usage;
    pDesc->Pool   = This->currentDesc.Pool;
    pDesc->Size   = This->currentDesc.Size;
    return D3D_OK;
}

IWineD3DIndexBufferVtbl IWineD3DIndexBuffer_Vtbl =
{
    IWineD3DIndexBufferImpl_QueryInterface,
    IWineD3DIndexBufferImpl_AddRef,
    IWineD3DIndexBufferImpl_Release,
    IWineD3DIndexBufferImpl_GetParent,
    IWineD3DIndexBufferImpl_GetDevice,
    IWineD3DIndexBufferImpl_SetPrivateData,
    IWineD3DIndexBufferImpl_GetPrivateData,
    IWineD3DIndexBufferImpl_FreePrivateData,
    IWineD3DIndexBufferImpl_SetPriority,
    IWineD3DIndexBufferImpl_GetPriority,
    IWineD3DIndexBufferImpl_PreLoad,
    IWineD3DIndexBufferImpl_GetType,
    IWineD3DIndexBufferImpl_Lock,
    IWineD3DIndexBufferImpl_Unlock,
    IWineD3DIndexBufferImpl_GetDesc
};
