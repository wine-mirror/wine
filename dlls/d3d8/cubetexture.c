/*
 * IDirect3DCubeTexture8 implementation
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

/* IDirect3DCubeTexture8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DCubeTexture8Impl_QueryInterface(LPDIRECT3DCUBETEXTURE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    TRACE("(%p) : QueryInterface\n", This);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory)) {
        IDirect3DCubeTexture8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DCubeTexture8Impl_AddRef(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DCubeTexture8Impl_Release(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    ULONG ref = --This->ref;
    int i;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        for (i=0; i<This->levels; i++) {
            if (This->surfaces[i] != NULL) {
                TRACE("(%p) : Releasing surface %p\n", This, This->surfaces[i]);
                IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) This->surfaces[i]);
            }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DCubeTexture8 IDirect3DResource8 Interface follow: */
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetDevice(LPDIRECT3DCUBETEXTURE8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetPriority(LPDIRECT3DCUBETEXTURE8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetPriority(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
void     WINAPI        IDirect3DCubeTexture8Impl_PreLoad(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
}

D3DRESOURCETYPE WINAPI IDirect3DCubeTexture8Impl_GetType(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DCubeTexture8 (Inherited from IDirect3DBaseTexture8) */
DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetLOD(LPDIRECT3DCUBETEXTURE8 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLOD(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}

DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE8 iface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->levels);
    return This->levels;
}

/* IDirect3DCubeTexture8 */
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE8 iface,UINT Level,D3DSURFACE_DESC *pDesc) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    if (Level < This->levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IDirect3DSurface8Impl_GetDesc((LPDIRECT3DSURFACE8)This->surfaces[Level], pDesc);
    } else {
        FIXME("(%p) Level (%d)\n", This, Level);
    }
    return D3D_OK;
}

HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface8** ppCubeMapSurface) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->surfaces[FaceType][Level]);
    *ppCubeMapSurface = (LPDIRECT3DSURFACE8) This->surfaces[FaceType][Level];
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8)This->surfaces[FaceType][Level]);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_LockRect(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_UnlockRect(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,UINT Level) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    This->Dirty = TRUE;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE8 iface,D3DCUBEMAP_FACES FaceType,CONST RECT* pDirtyRect) {
    ICOM_THIS(IDirect3DCubeTexture8Impl,iface);
    This->Dirty = TRUE;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}


ICOM_VTABLE(IDirect3DCubeTexture8) Direct3DCubeTexture8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DCubeTexture8Impl_QueryInterface,
    IDirect3DCubeTexture8Impl_AddRef,
    IDirect3DCubeTexture8Impl_Release,
    IDirect3DCubeTexture8Impl_GetDevice,
    IDirect3DCubeTexture8Impl_SetPrivateData,
    IDirect3DCubeTexture8Impl_GetPrivateData,
    IDirect3DCubeTexture8Impl_FreePrivateData,
    IDirect3DCubeTexture8Impl_SetPriority,
    IDirect3DCubeTexture8Impl_GetPriority,
    IDirect3DCubeTexture8Impl_PreLoad,
    IDirect3DCubeTexture8Impl_GetType,
    IDirect3DCubeTexture8Impl_SetLOD,
    IDirect3DCubeTexture8Impl_GetLOD,
    IDirect3DCubeTexture8Impl_GetLevelCount,
    IDirect3DCubeTexture8Impl_GetLevelDesc,
    IDirect3DCubeTexture8Impl_GetCubeMapSurface,
    IDirect3DCubeTexture8Impl_LockRect,
    IDirect3DCubeTexture8Impl_UnlockRect,
    IDirect3DCubeTexture8Impl_AddDirtyRect
};
