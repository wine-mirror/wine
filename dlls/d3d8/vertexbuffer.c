/*
 * IDirect3DResource8 implementation
 *
 * Copyright 2002 Jason Edmeades
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DResource IUnknown parts follow: */
HRESULT WINAPI IDirect3DVertexBuffer8Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER8 iface, REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DVertexBuffer8)) {
        IDirect3DVertexBuffer8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVertexBuffer8Impl_AddRef(LPDIRECT3DVERTEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DVertexBuffer8Impl_Release(LPDIRECT3DVERTEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        if (NULL != This->allocatedMemory) HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DResource Interface follow: */
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetDevice(LPDIRECT3DVERTEXBUFFER8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVertexBuffer8Impl_SetPriority(LPDIRECT3DVERTEXBUFFER8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVertexBuffer8Impl_GetPriority(LPDIRECT3DVERTEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
void     WINAPI        IDirect3DVertexBuffer8Impl_PreLoad(LPDIRECT3DVERTEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);
}
D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer8Impl_GetType(LPDIRECT3DVERTEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DVertexBuffer8 */
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_Lock(LPDIRECT3DVERTEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    TRACE("(%p) : returning memory of %p (base:%p,offset:%u)\n", This, This->allocatedMemory + OffsetToLock, This->allocatedMemory, OffsetToLock);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */
    *ppbData = This->allocatedMemory + OffsetToLock;
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_Unlock(LPDIRECT3DVERTEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVertexBuffer8Impl_GetDesc(LPDIRECT3DVERTEXBUFFER8 iface, D3DVERTEXBUFFER_DESC *pDesc) {
    ICOM_THIS(IDirect3DVertexBuffer8Impl,iface);

    TRACE("(%p)\n", This);
    pDesc->Format = This->currentDesc.Format;
    pDesc->Type   = This->currentDesc.Type;
    pDesc->Usage  = This->currentDesc.Usage;
    pDesc->Pool   = This->currentDesc.Pool;
    pDesc->Size   = This->currentDesc.Size;
    pDesc->FVF    = This->currentDesc.FVF;
    return D3D_OK;
}

ICOM_VTABLE(IDirect3DVertexBuffer8) Direct3DVertexBuffer8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DVertexBuffer8Impl_QueryInterface,
    IDirect3DVertexBuffer8Impl_AddRef,
    IDirect3DVertexBuffer8Impl_Release,
    IDirect3DVertexBuffer8Impl_GetDevice,
    IDirect3DVertexBuffer8Impl_SetPrivateData,
    IDirect3DVertexBuffer8Impl_GetPrivateData,
    IDirect3DVertexBuffer8Impl_FreePrivateData,
    IDirect3DVertexBuffer8Impl_SetPriority,
    IDirect3DVertexBuffer8Impl_GetPriority,
    IDirect3DVertexBuffer8Impl_PreLoad,
    IDirect3DVertexBuffer8Impl_GetType,
    IDirect3DVertexBuffer8Impl_Lock,
    IDirect3DVertexBuffer8Impl_Unlock,
    IDirect3DVertexBuffer8Impl_GetDesc
};
