/*
 * IDirect3DCubeTexture9 implementation
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

/* IDirect3DCubeTexture9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DCubeTexture9Impl_QueryInterface(LPDIRECT3DCUBETEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
        || IsEqualGUID(riid, &IID_IDirect3DCubeTexture9)) {
        IDirect3DCubeTexture9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DCubeTexture9Impl_AddRef(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DCubeTexture9Impl_Release(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    ULONG ref = --This->ref;
    int i;
    int j;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        for (i = 0; i < This->levels; i++) {
	  for (j = 0; j < 6; j++) { 
	    if (This->surfaces[j][i] != NULL) {
	      TRACE("(%p) : Releasing surface %p\n", This, This->surfaces[j][i]);
	      IDirect3DSurface9Impl_Release((LPDIRECT3DSURFACE9) This->surfaces[j][i]);
            }
	  }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DCubeTexture9 IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DCubeTexture9Impl_GetDevice(LPDIRECT3DCUBETEXTURE9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

DWORD WINAPI IDirect3DCubeTexture9Impl_SetPriority(LPDIRECT3DCUBETEXTURE9 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DResource9Impl_SetPriority((LPDIRECT3DRESOURCE9) This, PriorityNew);
}

DWORD WINAPI IDirect3DCubeTexture9Impl_GetPriority(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DResource9Impl_GetPriority((LPDIRECT3DRESOURCE9) This);
}

void WINAPI IDirect3DCubeTexture9Impl_PreLoad(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DCubeTexture9Impl_GetType(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DResource9Impl_GetType((LPDIRECT3DRESOURCE9) This);
}

/* IDirect3DCubeTexture9 IDirect3DBaseTexture9 Interface follow: */
DWORD WINAPI IDirect3DCubeTexture9Impl_SetLOD(LPDIRECT3DCUBETEXTURE9 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_SetLOD((LPDIRECT3DBASETEXTURE9) This, LODNew);
}

DWORD WINAPI IDirect3DCubeTexture9Impl_GetLOD(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetLOD((LPDIRECT3DBASETEXTURE9) This);
}

DWORD WINAPI IDirect3DCubeTexture9Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE9 iface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetLevelCount((LPDIRECT3DBASETEXTURE9) This);
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DCUBETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
  ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_SetAutoGenFilterType((LPDIRECT3DBASETEXTURE9) This, FilterType);
}

D3DTEXTUREFILTERTYPE WINAPI IDirect3DCubeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DCUBETEXTURE9 iface) {
  ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
  return IDirect3DBaseTexture9Impl_GetAutoGenFilterType((LPDIRECT3DBASETEXTURE9) This);
}

void WINAPI IDirect3DCubeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DCUBETEXTURE9 iface) {
  ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
  FIXME("(%p) : stub\n", This);
  return ;
}

/* IDirect3DCubeTexture9 Interface follow: */
HRESULT WINAPI IDirect3DCubeTexture9Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    if (Level < This->levels) {
        TRACE("(%p) level (%d)\n", This, Level);
        return IDirect3DSurface9Impl_GetDesc((LPDIRECT3DSURFACE9) This->surfaces[0][Level], pDesc);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
        return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    if (Level < This->levels) {
        *ppCubeMapSurface = (LPDIRECT3DSURFACE9) This->surfaces[FaceType][Level];
        IDirect3DSurface9Impl_AddRef((LPDIRECT3DSURFACE9) *ppCubeMapSurface);
	TRACE("(%p) -> faceType(%d) level(%d) returning surface@%p \n", This, FaceType, Level, This->surfaces[FaceType][Level]);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
        return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_LockRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr;
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    if (Level < This->levels) {
      /**
       * Not dirtified while Surfaces don't notify dirtification
       * This->Dirty = TRUE;
       */
      hr = IDirect3DSurface9Impl_LockRect((LPDIRECT3DSURFACE9) This->surfaces[FaceType][Level], pLockedRect, pRect, Flags);
      TRACE("(%p) -> faceType(%d) level(%d) returning memory@%p success(%lu)\n", This, FaceType, Level, pLockedRect->pBits, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IDirect3DCubeTexture9Impl_UnlockRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level) {
    HRESULT hr;
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    if (Level < This->levels) {
      hr = IDirect3DSurface9Impl_UnlockRect((LPDIRECT3DSURFACE9) This->surfaces[FaceType][Level]);
      TRACE("(%p) -> faceType(%d) level(%d) success(%lu)\n", This, FaceType, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT  WINAPI IDirect3DCubeTexture9Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) {
    ICOM_THIS(IDirect3DCubeTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    This->Dirty = TRUE;
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DCubeTexture9) Direct3DCubeTexture9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DCubeTexture9Impl_QueryInterface,
    IDirect3DCubeTexture9Impl_AddRef,
    IDirect3DCubeTexture9Impl_Release,
    IDirect3DCubeTexture9Impl_GetDevice,
    IDirect3DCubeTexture9Impl_SetPrivateData,
    IDirect3DCubeTexture9Impl_GetPrivateData,
    IDirect3DCubeTexture9Impl_FreePrivateData,
    IDirect3DCubeTexture9Impl_SetPriority,
    IDirect3DCubeTexture9Impl_GetPriority,
    IDirect3DCubeTexture9Impl_PreLoad,
    IDirect3DCubeTexture9Impl_GetType,
    IDirect3DCubeTexture9Impl_SetLOD,
    IDirect3DCubeTexture9Impl_GetLOD,
    IDirect3DCubeTexture9Impl_GetLevelCount,
    IDirect3DCubeTexture9Impl_SetAutoGenFilterType,
    IDirect3DCubeTexture9Impl_GetAutoGenFilterType,
    IDirect3DCubeTexture9Impl_GenerateMipSubLevels,
    IDirect3DCubeTexture9Impl_GetLevelDesc,
    IDirect3DCubeTexture9Impl_GetCubeMapSurface,
    IDirect3DCubeTexture9Impl_LockRect,
    IDirect3DCubeTexture9Impl_UnlockRect,
    IDirect3DCubeTexture9Impl_AddDirtyRect
};


/* IDirect3DDevice9 IDirect3DCubeTexture9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateCubeTexture(LPDIRECT3DDEVICE9 iface, 
							UINT EdgeLength, UINT Levels, DWORD Usage, 
                                                        D3DFORMAT Format, D3DPOOL Pool, 
							IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) {

    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
