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

#include "config.h"

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
        for (i = 0; i < This->levels; i++) {
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
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub returning 0\n", This);    
    return 0;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub returning 0\n", This);    
    return 0;
}
void     WINAPI        IDirect3DVolumeTexture8Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE8 iface) {
    int i;
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) : About to load texture\n", This);
    for (i = 0; i < This->levels; i++) {
      if (i == 0 && This->volumes[i]->textureName != 0 && This->Dirty == FALSE) {
	glBindTexture(GL_TEXTURE_3D, This->volumes[i]->textureName);
	checkGLcall("glBindTexture");
	TRACE("Texture %p (level %d) given name %d\n", This->volumes[i], i, This->volumes[i]->textureName);
	/* No need to walk through all mip-map levels, since already all assigned */
	i = This->levels;
      } else {
	if (i == 0) {
	  if (This->volumes[i]->textureName == 0) {
	    glGenTextures(1, &This->volumes[i]->textureName);
	    checkGLcall("glGenTextures");
	    TRACE("Texture %p (level %d) given name %d\n", This->volumes[i], i, This->volumes[i]->textureName);
	  }
	  
	  glBindTexture(GL_TEXTURE_3D, This->volumes[i]->textureName);
	  checkGLcall("glBindTexture");
	  
	  TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->levels - 1);
	  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, This->levels - 1); 
	  checkGLcall("glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, This->levels - 1)");
	}

	TRACE("Calling glTexImage3D %x i=%d, intfmt=%x, w=%d, h=%d,d=%d, 0=%d, glFmt=%x, glType=%x, Mem=%p\n",
	      GL_TEXTURE_3D, 
	      i, 
	      D3DFmt2GLIntFmt(This->Device, This->format), 
	      This->volumes[i]->myDesc.Width, 
	      This->volumes[i]->myDesc.Height, 
	      This->volumes[i]->myDesc.Depth,
	      0, 
	      D3DFmt2GLFmt(This->Device, This->format), 
	      D3DFmt2GLType(This->Device, This->format),
	      This->volumes[i]->allocatedMemory);
	glTexImage3D(GL_TEXTURE_3D, 
		     i,
		     D3DFmt2GLIntFmt(This->Device, This->format),
		     This->volumes[i]->myDesc.Width,
		     This->volumes[i]->myDesc.Height,
		     This->volumes[i]->myDesc.Depth,
		     0,
		     D3DFmt2GLFmt(This->Device, This->format),
		     D3DFmt2GLType(This->Device, This->format),
		     This->volumes[i]->allocatedMemory);
	checkGLcall("glTexImage3D");

	/* Removed glTexParameterf now TextureStageStates are initialized at startup */
	This->Dirty = FALSE;
      }
    }
    return ;
}
D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture8Impl_GetType(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DVolumeTexture8 IDirect3DBaseTexture8 Interface follow: */
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub returning 0\n", This);    
    return 0;
}
DWORD    WINAPI        IDirect3DVolumeTexture8Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE8 iface) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    FIXME("(%p) : stub returning 0\n", This);    
    return 0;
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
        return IDirect3DVolume8Impl_GetDesc((LPDIRECT3DVOLUME8) This->volumes[Level], pDesc);
    } else {
        FIXME("(%p) Level (%d)\n", This, Level);
    }
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, IDirect3DVolume8** ppVolumeLevel) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    if (Level < This->levels) {
      *ppVolumeLevel = (LPDIRECT3DVOLUME8)This->volumes[Level];
      IDirect3DVolume8Impl_AddRef((LPDIRECT3DVOLUME8) *ppVolumeLevel);
      TRACE("(%p) -> level(%d) returning volume@%p\n", This, Level, *ppVolumeLevel);
    } else {
      FIXME("(%p) Level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return D3D_OK;

}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_LockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    HRESULT hr;
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    if (Level < This->levels) {
      /**
       * Not dirtified while Surfaces don't notify dirtification
       * This->Dirty = TRUE;
       */
      hr = IDirect3DVolume8Impl_LockBox((LPDIRECT3DVOLUME8) This->volumes[Level], pLockedVolume, pBox, Flags);
      TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level) {
    HRESULT hr;
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    if (Level < This->levels) {
      hr = IDirect3DVolume8Impl_UnlockBox((LPDIRECT3DVOLUME8) This->volumes[Level]);
      TRACE("(%p) -> level(%d) success(%lu)\n", This, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}
HRESULT  WINAPI        IDirect3DVolumeTexture8Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE8 iface, CONST D3DBOX* pDirtyBox) {
    ICOM_THIS(IDirect3DVolumeTexture8Impl,iface);
    This->Dirty = TRUE;
    TRACE("(%p) : dirtyfication of volume Level (0)\n", This);    
    return IDirect3DVolume8Impl_AddDirtyBox((LPDIRECT3DVOLUME8) This->volumes[0], pDirtyBox);
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
