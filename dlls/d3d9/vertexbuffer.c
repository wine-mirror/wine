/*
 * IDirect3DResource9 implementation
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DVertexBuffer9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVertexBuffer9Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DVertexBuffer9)) {
        IDirect3DVertexBuffer9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVertexBuffer9Impl_AddRef(LPDIRECT3DVERTEXBUFFER9 iface) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DVertexBuffer9Impl_Release(LPDIRECT3DVERTEXBUFFER9 iface) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        if (NULL != This->allocatedMemory) HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexBuffer9 IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDevice(LPDIRECT3DVERTEXBUFFER9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

HRESULT WINAPI IDirect3DVertexBuffer9Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexBuffer9Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

DWORD WINAPI IDirect3DVertexBuffer9Impl_SetPriority(LPDIRECT3DVERTEXBUFFER9 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    return IDirect3DResource9Impl_SetPriority((LPDIRECT3DRESOURCE9) This, PriorityNew);
}

DWORD WINAPI IDirect3DVertexBuffer9Impl_GetPriority(LPDIRECT3DVERTEXBUFFER9 iface) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    return IDirect3DResource9Impl_GetPriority((LPDIRECT3DRESOURCE9) This);
}

void WINAPI IDirect3DVertexBuffer9Impl_PreLoad(LPDIRECT3DVERTEXBUFFER9 iface) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer9Impl_GetType(LPDIRECT3DVERTEXBUFFER9 iface) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    return IDirect3DResource9Impl_GetType((LPDIRECT3DRESOURCE9) This);
}

/* IDirect3DVertexBuffer9 Interface follow: */
HRESULT WINAPI IDirect3DVertexBuffer9Impl_Lock(LPDIRECT3DVERTEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexBuffer9Impl_Unlock(LPDIRECT3DVERTEXBUFFER9 iface) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDesc(LPDIRECT3DVERTEXBUFFER9 iface, D3DVERTEXBUFFER_DESC* pDesc) {
    ICOM_THIS(IDirect3DVertexBuffer9Impl,iface);
    TRACE("(%p) : copying into %p\n", This, pDesc);
    memcpy(pDesc, &This->myDesc, sizeof(D3DVERTEXBUFFER_DESC));
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DVertexBuffer9) Direct3DVertexBuffer9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DVertexBuffer9Impl_QueryInterface,
    IDirect3DVertexBuffer9Impl_AddRef,
    IDirect3DVertexBuffer9Impl_Release,
    IDirect3DVertexBuffer9Impl_GetDevice,
    IDirect3DVertexBuffer9Impl_SetPrivateData,
    IDirect3DVertexBuffer9Impl_GetPrivateData,
    IDirect3DVertexBuffer9Impl_FreePrivateData,
    IDirect3DVertexBuffer9Impl_SetPriority,
    IDirect3DVertexBuffer9Impl_GetPriority,
    IDirect3DVertexBuffer9Impl_PreLoad,
    IDirect3DVertexBuffer9Impl_GetType,
    IDirect3DVertexBuffer9Impl_Lock,
    IDirect3DVertexBuffer9Impl_Unlock,
    IDirect3DVertexBuffer9Impl_GetDesc
};


/* IDirect3DDevice9 IDirect3DVertexBuffer9 Methods follow: */
HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexBuffer(LPDIRECT3DDEVICE9 iface, 
						       UINT Size, DWORD Usage, DWORD FVF, D3DPOOL Pool, 
						       IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) {
    IDirect3DVertexBuffer9Impl *object;

    ICOM_THIS(IDirect3DDevice9Impl,iface);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBuffer9Impl));
    object->lpVtbl = &Direct3DVertexBuffer9_Vtbl;
    object->ref = 1;
    object->Device = This;

    object->ResourceType = D3DRTYPE_VERTEXBUFFER;

    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    object->myDesc.Usage = Usage;
    object->myDesc.Pool  = Pool;
    object->myDesc.FVF   = FVF;
    object->myDesc.Size  = Size;

    TRACE("(%p) : Size=%d, Usage=%ld, FVF=%lx, Pool=%d - Memory@%p, Iface@%p\n", This, Size, Usage, FVF, Pool, object->allocatedMemory, object);

    *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER9) object;

    return D3D_OK;
}
