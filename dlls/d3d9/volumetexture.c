/*
 * IDirect3DVolumeTexture9 implementation
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

/* IDirect3DVolumeTexture9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVolumeTexture9Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IDirect3DResource9)
	|| IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
	|| IsEqualGUID(riid, &IID_IDirect3DVolumeTexture9)) {
        IDirect3DVolumeTexture9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVolumeTexture9Impl_AddRef(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DVolumeTexture9Impl_Release(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    ULONG ref = --This->ref;
    UINT  i;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        for (i = 0; i < This->levels; i++) {
            if (This->volumes[i] != NULL) {
                TRACE("(%p) : Releasing volume %p\n", This, This->volumes[i]);
                IDirect3DVolume9Impl_Release((LPDIRECT3DVOLUME9) This->volumes[i]);
            }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVolumeTexture9 IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

DWORD WINAPI IDirect3DVolumeTexture9Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DResource9Impl_SetPriority((LPDIRECT3DRESOURCE9) This, PriorityNew);
}

DWORD WINAPI IDirect3DVolumeTexture9Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DResource9Impl_GetPriority((LPDIRECT3DRESOURCE9) This);
}

void WINAPI IDirect3DVolumeTexture9Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture9Impl_GetType(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DResource9Impl_GetType((LPDIRECT3DRESOURCE9) This);
}

/* IDirect3DVolumeTexture9 IDirect3DBaseTexture9 Interface follow: */
DWORD WINAPI IDirect3DVolumeTexture9Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_SetLOD((LPDIRECT3DBASETEXTURE9) This, LODNew);
}

DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetLOD((LPDIRECT3DBASETEXTURE9) This);
}

DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetLevelCount((LPDIRECT3DBASETEXTURE9) This);
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_SetAutoGenFilterType((LPDIRECT3DBASETEXTURE9) This, FilterType);
}

D3DTEXTUREFILTERTYPE WINAPI IDirect3DVolumeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetAutoGenFilterType((LPDIRECT3DBASETEXTURE9) This);
}

void WINAPI IDirect3DVolumeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DVOLUMETEXTURE9 iface) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return ;
}

/* IDirect3DVolumeTexture9 Interface follow: */
HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DVOLUME_DESC* pDesc) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    if (Level < This->levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IDirect3DVolume9Impl_GetDesc((LPDIRECT3DVOLUME9) This->volumes[Level], pDesc);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
	return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, IDirect3DVolume9** ppVolumeLevel) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    if (Level < This->levels) {
      *ppVolumeLevel = (LPDIRECT3DVOLUME9) This->volumes[Level];
      IDirect3DVolume9Impl_AddRef((LPDIRECT3DVOLUME9) *ppVolumeLevel);
      TRACE("(%p) -> level(%d) returning volume@%p\n", This, Level, *ppVolumeLevel);
    } else {
      FIXME("(%p) Level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return D3D_OK;

}
HRESULT WINAPI IDirect3DVolumeTexture9Impl_LockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    HRESULT hr;
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    if (Level < This->levels) {
      /**
       * Not dirtified while Surfaces don't notify dirtification
       * This->Dirty = TRUE;
       */
      hr = IDirect3DVolume9Impl_LockBox((LPDIRECT3DVOLUME9) This->volumes[Level], pLockedVolume, pBox, Flags);
      TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level) {
    HRESULT hr;
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    if (Level < This->levels) {
      hr = IDirect3DVolume9Impl_UnlockBox((LPDIRECT3DVOLUME9) This->volumes[Level]);
      TRACE("(%p) -> level(%d) success(%lu)\n", This, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IDirect3DVolumeTexture9Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE9 iface, CONST D3DBOX* pDirtyBox) {
    ICOM_THIS(IDirect3DVolumeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    This->Dirty = TRUE;
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DVolumeTexture9) Direct3DVolumeTexture9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DVolumeTexture9Impl_QueryInterface,
    IDirect3DVolumeTexture9Impl_AddRef,
    IDirect3DVolumeTexture9Impl_Release,
    IDirect3DVolumeTexture9Impl_GetDevice,
    IDirect3DVolumeTexture9Impl_SetPrivateData,
    IDirect3DVolumeTexture9Impl_GetPrivateData,
    IDirect3DVolumeTexture9Impl_FreePrivateData,
    IDirect3DVolumeTexture9Impl_SetPriority,
    IDirect3DVolumeTexture9Impl_GetPriority,
    IDirect3DVolumeTexture9Impl_PreLoad,
    IDirect3DVolumeTexture9Impl_GetType,
    IDirect3DVolumeTexture9Impl_SetLOD,
    IDirect3DVolumeTexture9Impl_GetLOD,
    IDirect3DVolumeTexture9Impl_GetLevelCount,
    IDirect3DVolumeTexture9Impl_SetAutoGenFilterType,
    IDirect3DVolumeTexture9Impl_GetAutoGenFilterType,
    IDirect3DVolumeTexture9Impl_GenerateMipSubLevels,
    IDirect3DVolumeTexture9Impl_GetLevelDesc,
    IDirect3DVolumeTexture9Impl_GetVolumeLevel,
    IDirect3DVolumeTexture9Impl_LockBox,
    IDirect3DVolumeTexture9Impl_UnlockBox,
    IDirect3DVolumeTexture9Impl_AddDirtyBox
};


/* IDirect3DDevice9 IDirect3DVolumeTexture9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVolumeTexture(LPDIRECT3DDEVICE9 iface, 
                                                          UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, 
                                                          D3DFORMAT Format, D3DPOOL Pool, 
							  IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) {

    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
