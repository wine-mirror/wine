/*
 * IDirect3DIndexBuffer8 implementation
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

/* IDirect3DIndexBuffer8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DIndexBuffer8Impl_QueryInterface(LPDIRECT3DINDEXBUFFER8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DIndexBuffer8)) {
        IDirect3DIndexBuffer8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DIndexBuffer8Impl_AddRef(LPDIRECT3DINDEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DIndexBuffer8Impl_Release(LPDIRECT3DINDEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DResource Interface follow: */
HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetDevice(LPDIRECT3DINDEXBUFFER8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DIndexBuffer8Impl_SetPriority(LPDIRECT3DINDEXBUFFER8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DIndexBuffer8Impl_GetPriority(LPDIRECT3DINDEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
void     WINAPI        IDirect3DIndexBuffer8Impl_PreLoad(LPDIRECT3DINDEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    FIXME("(%p) : stub\n", This);
}
D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer8Impl_GetType(LPDIRECT3DINDEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DIndexBuffer8 */
HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_Lock(LPDIRECT3DINDEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    TRACE("(%p) : no locking yet, offset %d, size %d, Flags=%lx\n", This, OffsetToLock, SizeToLock, Flags);
    *ppbData = (BYTE *)This->allocatedMemory + OffsetToLock;
    return D3D_OK;
}

HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_Unlock(LPDIRECT3DINDEXBUFFER8 iface) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DIndexBuffer8Impl_GetDesc(LPDIRECT3DINDEXBUFFER8 iface, D3DINDEXBUFFER_DESC *pDesc) {
    ICOM_THIS(IDirect3DIndexBuffer8Impl,iface);

    TRACE("(%p) : copying into %p\n", This, pDesc);
    memcpy(pDesc, &This->currentDesc, sizeof(D3DINDEXBUFFER_DESC));
    return D3D_OK;
}

ICOM_VTABLE(IDirect3DIndexBuffer8) Direct3DIndexBuffer8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DIndexBuffer8Impl_QueryInterface,
    IDirect3DIndexBuffer8Impl_AddRef,
    IDirect3DIndexBuffer8Impl_Release,
    IDirect3DIndexBuffer8Impl_GetDevice,
    IDirect3DIndexBuffer8Impl_SetPrivateData,
    IDirect3DIndexBuffer8Impl_GetPrivateData,
    IDirect3DIndexBuffer8Impl_FreePrivateData,
    IDirect3DIndexBuffer8Impl_SetPriority,
    IDirect3DIndexBuffer8Impl_GetPriority,
    IDirect3DIndexBuffer8Impl_PreLoad,
    IDirect3DIndexBuffer8Impl_GetType,
    IDirect3DIndexBuffer8Impl_Lock,
    IDirect3DIndexBuffer8Impl_Unlock,
    IDirect3DIndexBuffer8Impl_GetDesc
};
