/*
 * IDirect3DBaseTexture8 implementation
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

/* IDirect3DBaseTexture8 (Inherited from IUnknown) */
HRESULT WINAPI IDirect3DBaseTexture8Impl_QueryInterface(LPDIRECT3DBASETEXTURE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    TRACE("(%p) : QueryInterface\n", This);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)) {
        IDirect3DBaseTexture8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DBaseTexture8Impl_AddRef(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DBaseTexture8Impl_Release(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

/* IDirect3DBaseTexture8 (Inherited from IDirect3DResource8) */
HRESULT  WINAPI        IDirect3DBaseTexture8Impl_GetDevice(LPDIRECT3DBASETEXTURE8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    /**
     * Note  Calling this method will increase the internal reference count 
     * on the IDirect3DDevice8 interface. 
     */
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DBaseTexture8Impl_SetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DBaseTexture8Impl_GetPrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DBaseTexture8Impl_FreePrivateData(LPDIRECT3DBASETEXTURE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DBaseTexture8Impl_SetPriority(LPDIRECT3DBASETEXTURE8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetPriority(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
void     WINAPI        IDirect3DBaseTexture8Impl_PreLoad(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
}
D3DRESOURCETYPE WINAPI IDirect3DBaseTexture8Impl_GetType(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    /*TRACE("(%p) : returning %d\n", This, This->ResourceType);*/
    return This->ResourceType;
}

/* IDirect3DBaseTexture8 */
DWORD    WINAPI        IDirect3DBaseTexture8Impl_SetLOD(LPDIRECT3DBASETEXTURE8 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetLOD(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DBaseTexture8Impl_GetLevelCount(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return 0;
}

ICOM_VTABLE(IDirect3DBaseTexture8) Direct3DBaseTexture8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DBaseTexture8Impl_QueryInterface,
    IDirect3DBaseTexture8Impl_AddRef,
    IDirect3DBaseTexture8Impl_Release,
    IDirect3DBaseTexture8Impl_GetDevice,
    IDirect3DBaseTexture8Impl_SetPrivateData,
    IDirect3DBaseTexture8Impl_GetPrivateData,
    IDirect3DBaseTexture8Impl_FreePrivateData,
    IDirect3DBaseTexture8Impl_SetPriority,
    IDirect3DBaseTexture8Impl_GetPriority,
    IDirect3DBaseTexture8Impl_PreLoad,
    IDirect3DBaseTexture8Impl_GetType,
    IDirect3DBaseTexture8Impl_SetLOD,
    IDirect3DBaseTexture8Impl_GetLOD,
    IDirect3DBaseTexture8Impl_GetLevelCount,
};

BOOL WINAPI IDirect3DBaseTexture8Impl_IsDirty(LPDIRECT3DBASETEXTURE8 iface) {
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    return This->Dirty;
}

BOOL WINAPI IDirect3DBaseTexture8Impl_SetDirty(LPDIRECT3DBASETEXTURE8 iface, BOOL dirty) {
    BOOL old;
    ICOM_THIS(IDirect3DBaseTexture8Impl,iface);
    
    old = This->Dirty;
    This->Dirty = dirty;
    return old;
}
