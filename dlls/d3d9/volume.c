/*
 * IDirect3DVolume9 implementation
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DVolume9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVolume9Impl_QueryInterface(LPDIRECT3DVOLUME9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVolume9)) {
        IDirect3DVolume9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVolume9Impl_AddRef(LPDIRECT3DVOLUME9 iface) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DVolume9Impl_Release(LPDIRECT3DVOLUME9 iface) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVolume9 Interface follow: */
HRESULT WINAPI IDirect3DVolume9Impl_GetDevice(LPDIRECT3DVOLUME9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE9) This->Device;

    /* Note  Calling this method will increase the internal reference count 
       on the IDirect3DDevice9 interface. */
    IDirect3DDevice9Impl_AddRef(*ppDevice);

    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_SetPrivateData(LPDIRECT3DVOLUME9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_GetPrivateData(LPDIRECT3DVOLUME9 iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_FreePrivateData(LPDIRECT3DVOLUME9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_GetContainer(LPDIRECT3DVOLUME9 iface, REFIID riid, void** ppContainer) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Container);
    *ppContainer = This->Container;
    IUnknown_AddRef(This->Container);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_GetDesc(LPDIRECT3DVOLUME9 iface, D3DVOLUME_DESC* pDesc) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    TRACE("(%p) : copying into %p\n", This, pDesc);
    memcpy(pDesc, &This->myDesc, sizeof(D3DVOLUME_DESC));
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_LockBox(LPDIRECT3DVOLUME9 iface, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolume9Impl_UnlockBox(LPDIRECT3DVOLUME9 iface) {
    ICOM_THIS(IDirect3DVolume9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DVolume9) Direct3DVolume9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DVolume9Impl_QueryInterface,
    IDirect3DVolume9Impl_AddRef,
    IDirect3DVolume9Impl_Release,
    IDirect3DVolume9Impl_GetDevice,
    IDirect3DVolume9Impl_SetPrivateData,
    IDirect3DVolume9Impl_GetPrivateData,
    IDirect3DVolume9Impl_FreePrivateData,
    IDirect3DVolume9Impl_GetContainer,
    IDirect3DVolume9Impl_GetDesc,
    IDirect3DVolume9Impl_LockBox,
    IDirect3DVolume9Impl_UnlockBox
};
