/*
 * IDirect3DVolumeTexture8 implementation
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


#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DVolumeTexture8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVolumeTexture8Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);

    TRACE("(%p) : QueryInterface\n", This);
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IDirect3DResource8)
	|| IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
	|| IsEqualGUID(riid, &IID_IDirect3DVolumeTexture8)) {
        IDirect3DVolumeTexture8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVolumeTexture8Impl_AddRef(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DVolumeTexture8Impl_Release(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    ULONG ref = --This->ref;
    UINT  i;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        for (i=0; i<This->levels; i++) {
            if (This->volumes[i] != NULL) {
                TRACE("(%p) : Releasing volume %p\n", This, This->volumes[i]);
                IDirect3DVolume8Impl_Release((LPDIRECT3DVOLUME8) This->volumes[i]);
            }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVolumeTexture8 IDirect3DResource8 Interface follow: */
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    /**
     * Note  Calling this method will increase the internal reference count 
     * on the IDirect3DDevice8 interface. 
     */
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
void     WINAPI        IDirect3DVolumeTexture8Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
}
D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture8Impl_GetType(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DVolumeTexture8 IDirect3DBaseTexture8 Interface follow: */
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->levels);
    return This->levels;
}

/* IDirect3DVolumeTexture8 */
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level,D3DVOLUME_DESC *pDesc) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    if (Level < This->levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IDirect3DVolume8Impl_GetDesc((LPDIRECT3DVOLUME8)This->volumes[Level], pDesc);
    } else {
        FIXME("(%p) Level (%d)\n", This, Level);
    }
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level,IDirect3DVolume8** ppVolumeLevel) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    IDirect3DVolume8Impl_AddRef((LPDIRECT3DVOLUME8)This->volumes[Level]);
    *ppVolumeLevel = (LPDIRECT3DVOLUME8)This->volumes[Level];
    TRACE("(%p) : returning %p for level %d\n", This, *ppVolumeLevel, Level);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_LockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level,D3DLOCKED_BOX* pLockedVolume,CONST D3DBOX* pBox,DWORD Flags) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) Level (%d)\n", This, Level);
    if (Level < This->levels) {
        return IDirect3DVolume8Impl_LockBox((LPDIRECT3DVOLUME8)This->volumes[Level], pLockedVolume, pBox, Flags);
    } else {
        FIXME("Volume Levels seems too high?!!\n");
    }
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    This->Dirty = TRUE;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE8 iface, CONST D3DBOX* pDirtyBox) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    This->Dirty = TRUE;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}


ICOM_VTABLE(IDirect3DVolumeTexture8) Direct3DVolumeTexture8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DVolumeTexture8Impl_QueryInterface,
    IDirect3DVolumeTexture8Impl_AddRef,
    IDirect3DVolumeTexture8Impl_Release,
    IDirect3DVolumeTexture8Impl_GetDevice,
    IDirect3DVolumeTexture8Impl_SetPrivateData,
    IDirect3DVolumeTexture8Impl_GetPrivateData,
    IDirect3DVolumeTexture8Impl_FreePrivateData,
    IDirect3DVolumeTexture8Impl_SetPriority,
    IDirect3DVolumeTexture8Impl_GetPriority,
    IDirect3DVolumeTexture8Impl_PreLoad,
    IDirect3DVolumeTexture8Impl_GetType,
    IDirect3DVolumeTexture8Impl_SetLOD,
    IDirect3DVolumeTexture8Impl_GetLOD,
    IDirect3DVolumeTexture8Impl_GetLevelCount,
    IDirect3DVolumeTexture8Impl_GetLevelDesc,
    IDirect3DVolumeTexture8Impl_GetVolumeLevel,
    IDirect3DVolumeTexture8Impl_LockBox,
    IDirect3DVolumeTexture8Impl_UnlockBox,
    IDirect3DVolumeTexture8Impl_AddDirtyBox
};
