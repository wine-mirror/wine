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

/* IDirect3DResource9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DResource9Impl_QueryInterface(LPDIRECT3DRESOURCE9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DResource9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)) {
        IDirect3DResource9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DResource9Impl_AddRef(LPDIRECT3DRESOURCE9 iface) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DResource9Impl_Release(LPDIRECT3DRESOURCE9 iface) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DResource9Impl_GetDevice(LPDIRECT3DRESOURCE9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE9) This->Device;
    IDirect3DDevice9Impl_AddRef(*ppDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DResource9Impl_SetPrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DResource9Impl_GetPrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DResource9Impl_FreePrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

DWORD  WINAPI IDirect3DResource9Impl_SetPriority(LPDIRECT3DRESOURCE9 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}

DWORD WINAPI IDirect3DResource9Impl_GetPriority(LPDIRECT3DRESOURCE9 iface) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}

void WINAPI IDirect3DResource9Impl_PreLoad(LPDIRECT3DRESOURCE9 iface) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    FIXME("(%p) : stub\n", This);
}

D3DRESOURCETYPE WINAPI IDirect3DResource9Impl_GetType(LPDIRECT3DRESOURCE9 iface) {
    ICOM_THIS(IDirect3DResource9Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}


ICOM_VTABLE(IDirect3DResource9) Direct3DResource9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DResource9Impl_QueryInterface,
    IDirect3DResource9Impl_AddRef,
    IDirect3DResource9Impl_Release,
    IDirect3DResource9Impl_GetDevice,
    IDirect3DResource9Impl_SetPrivateData,
    IDirect3DResource9Impl_GetPrivateData,
    IDirect3DResource9Impl_FreePrivateData,
    IDirect3DResource9Impl_SetPriority,
    IDirect3DResource9Impl_GetPriority,
    IDirect3DResource9Impl_PreLoad,
    IDirect3DResource9Impl_GetType
};
