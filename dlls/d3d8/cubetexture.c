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

#include "config.h"

#include <stdarg.h>

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
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) : QueryInterface\n", This);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
        || IsEqualGUID(riid, &IID_IDirect3DCubeTexture8)) {
        IDirect3DCubeTexture8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DCubeTexture8Impl_AddRef(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DCubeTexture8Impl_Release(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    unsigned int i, j;

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);
    if (ref == 0) {
        for (i = 0; i < This->levels; i++) {
	  for (j = 0; j < 6; j++) { 
	    if (This->surfaces[j][i] != NULL) {
	      TRACE("(%p) : Releasing surface %p\n", This, This->surfaces[j][i]);
	      IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) This->surfaces[j][i]);
            }
	  }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DCubeTexture8 (Inherited from IDirect3DResource8) */
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetDevice(LPDIRECT3DCUBETEXTURE8 iface, IDirect3DDevice8** ppDevice) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    /**
     * Note  Calling this method will increase the internal reference count 
     * on the IDirect3DDevice8 interface. 
     */
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetPriority(LPDIRECT3DCUBETEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return 0;
}
DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetPriority(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return 0;
}

static const GLenum cube_targets[6] = {
#if defined(GL_VERSION_1_3)
  GL_TEXTURE_CUBE_MAP_POSITIVE_X,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
#else
  GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
#endif
};

void     WINAPI        IDirect3DCubeTexture8Impl_PreLoad(LPDIRECT3DCUBETEXTURE8 iface) {
    unsigned int i, j;
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) : About to load texture: dirtified(%d)\n", This, This->Dirty);

    ENTER_GL();

    for (i = 0; i < This->levels; i++) {
      if (i == 0 && D3D8_SURFACE(This->surfaces[0][0])->textureName != 0 && This->Dirty == FALSE) {
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
#if defined(GL_VERSION_1_3)
	glBindTexture(GL_TEXTURE_CUBE_MAP, D3D8_SURFACE(This->surfaces[0][0])->textureName);
#else
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, D3D8_SURFACE(This->surfaces[0][0])->textureName);
#endif
	checkGLcall("glBindTexture");
	TRACE("Texture %p (level %d) given name %d\n", This->surfaces[0][0], i, D3D8_SURFACE(This->surfaces[0][0])->textureName);
	/* No need to walk through all mip-map levels, since already all assigned */
	i = This->levels;
      } else {
	if (i == 0) {
	  if (D3D8_SURFACE(This->surfaces[0][0])->textureName == 0) {
	    glGenTextures(1, &(D3D8_SURFACE(This->surfaces[0][0]))->textureName);
	    checkGLcall("glGenTextures");
	    TRACE("Texture %p (level %d) given name %d\n", This->surfaces[0][i], i, D3D8_SURFACE(This->surfaces[0][0])->textureName);
	  }

#if defined(GL_VERSION_1_3)
	  glBindTexture(GL_TEXTURE_CUBE_MAP, D3D8_SURFACE(This->surfaces[0][0])->textureName);
#else
	  glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, This->surfaces[0][0]->textureName);
#endif
	  checkGLcall("glBindTexture");

	  TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->levels - 1);
#if defined(GL_VERSION_1_3)
	  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, This->levels - 1); 
#else
	  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAX_LEVEL, This->levels - 1); 
#endif
	  checkGLcall("glTexParameteri(GL_TEXTURE_CUBE, GL_TEXTURE_MAX_LEVEL, This->levels - 1)");
	}
	
	for (j = 0; j < 6; j++) {
	  IDirect3DSurface8Impl_LoadTexture((LPDIRECT3DSURFACE8) This->surfaces[j][i], cube_targets[j], i); 
#if 0
	  static int gen = 0;
	  char buffer[4096];
	  snprintf(buffer, sizeof(buffer), "/tmp/cube%d_face%d_level%d_%d.png", This->surfaces[0][0]->textureName, j, i, ++gen);
	  IDirect3DSurface8Impl_SaveSnapshot((LPDIRECT3DSURFACE8) This->surfaces[j][i], buffer);
#endif
	}
	/* Removed glTexParameterf now TextureStageStates are initialized at startup */
	This->Dirty = FALSE;
      }
    }

    LEAVE_GL();

    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DCubeTexture8Impl_GetType(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DCubeTexture8 (Inherited from IDirect3DBaseTexture8) */
DWORD    WINAPI        IDirect3DCubeTexture8Impl_SetLOD(LPDIRECT3DCUBETEXTURE8 iface, DWORD LODNew) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return 0;
}
DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLOD(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return 0;
}

DWORD    WINAPI        IDirect3DCubeTexture8Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) : returning %d\n", This, This->levels);
    return This->levels;
}

/* IDirect3DCubeTexture8 */
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE8 iface, UINT Level, D3DSURFACE_DESC* pDesc) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    if (Level < This->levels) {
        TRACE("(%p) level (%d)\n", This, Level);
        return IDirect3DSurface8Impl_GetDesc((LPDIRECT3DSURFACE8) This->surfaces[0][Level], pDesc);
    }
    FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
    return D3DERR_INVALIDCALL;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface8** ppCubeMapSurface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    if (Level < This->levels) {
        *ppCubeMapSurface = (LPDIRECT3DSURFACE8) This->surfaces[FaceType][Level];
        IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) *ppCubeMapSurface);
	TRACE("(%p) -> faceType(%d) level(%d) returning surface@%p\n", This, FaceType, Level, This->surfaces[FaceType][Level]);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
        return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_LockRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr;
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    if (Level < This->levels) {
      /**
       * Not dirtified while Surfaces don't notify dirtification
       * This->Dirty = TRUE;
       */
      hr = IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) This->surfaces[FaceType][Level], pLockedRect, pRect, Flags);
      TRACE("(%p) -> faceType(%d) level(%d) returning memory@%p success(%lu)\n", This, FaceType, Level, pLockedRect->pBits, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_UnlockRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level) {
    HRESULT hr;
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    if (Level < This->levels) {
      hr = IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) This->surfaces[FaceType][Level]);
      TRACE("(%p) -> faceType(%d) level(%d) success(%lu)\n", This, FaceType, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}
HRESULT  WINAPI        IDirect3DCubeTexture8Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    This->Dirty = TRUE;
    TRACE("(%p) : dirtyfication of faceType(%d) Level (0)\n", This, FaceType);    
    return IWineD3DSurface_AddDirtyRect( (IWineD3DSurface*)(This->surfaces[FaceType][0])->wineD3DSurface, pDirtyRect);
}


const IDirect3DCubeTexture8Vtbl Direct3DCubeTexture8_Vtbl =
{
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
