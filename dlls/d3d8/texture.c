/*
 * IDirect3DTexture8 implementation
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

/* IDirect3DTexture8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DTexture8Impl_QueryInterface(LPDIRECT3DTEXTURE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) : QueryInterface\n", This);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
        || IsEqualGUID(riid, &IID_IDirect3DTexture8)) {
        IDirect3DTexture8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DTexture8Impl_AddRef(LPDIRECT3DTEXTURE8 iface) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DTexture8Impl_Release(LPDIRECT3DTEXTURE8 iface) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    ULONG ref = --This->ref;
    int i;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        for (i = 0; i < This->levels; i++) {
            if (This->surfaces[i] != NULL) {
                TRACE("(%p) : Releasing surface %p\n", This, This->surfaces[i]);
                IDirect3DSurface8Impl_Release((LPDIRECT3DSURFACE8) This->surfaces[i]);
            }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DTexture8 IDirect3DResource8 Interface follow: */
HRESULT  WINAPI        IDirect3DTexture8Impl_GetDevice(LPDIRECT3DTEXTURE8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device; 
    /**
     * Note  Calling this method will increase the internal reference count 
     * on the IDirect3DDevice8 interface. 
     */
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_SetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_GetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_FreePrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DTexture8Impl_SetPriority(LPDIRECT3DTEXTURE8 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DTexture8Impl_GetPriority(LPDIRECT3DTEXTURE8 iface) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
void     WINAPI        IDirect3DTexture8Impl_PreLoad(LPDIRECT3DTEXTURE8 iface) {
    int i;
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) : About to load texture\n", This);
    for (i = 0; i < This->levels; i++) {
      if (i == 0 && This->surfaces[i]->textureName != 0 && This->Dirty == FALSE) {
	glBindTexture(GL_TEXTURE_2D, This->surfaces[i]->textureName);
	checkGLcall("glBindTexture");
	TRACE("Texture %p (level %d) given name %d\n", This->surfaces[i], i, This->surfaces[i]->textureName);
	/* No need to walk through all mip-map levels, since already all assigned */
	i = This->levels;
      } else {
	if (i == 0) {
	  if (This->surfaces[i]->textureName == 0) {
	    glGenTextures(1, &This->surfaces[i]->textureName);
	    checkGLcall("glGenTextures");
	    TRACE("Texture %p (level %d) given name %d\n", This->surfaces[i], i, This->surfaces[i]->textureName);
	  }
	  
	  glBindTexture(GL_TEXTURE_2D, This->surfaces[i]->textureName);
	  checkGLcall("glBindTexture");
	  
	  TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->levels - 1);   
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->levels - 1);
	  checkGLcall("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->levels)");
	  
	}

	IDirect3DSurface8Impl_CreateGLTexture((LPDIRECT3DSURFACE8) This->surfaces[i], GL_TEXTURE_2D, i); 
#if 0
	TRACE("Calling glTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
	      GL_TEXTURE_2D, 
              i, 
              D3DFmt2GLIntFmt(This->format),
	      This->surfaces[i]->myDesc.Width, 
              This->surfaces[i]->myDesc.Height, 
	      0, 
              D3DFmt2GLFmt(This->format), 
              D3DFmt2GLType(This->format),
	      This->surfaces[i]->allocatedMemory);
	glTexImage2D(GL_TEXTURE_2D, 
		     i,
		     D3DFmt2GLIntFmt(This->format),
		     This->surfaces[i]->myDesc.Width,
		     This->surfaces[i]->myDesc.Height,
		     0,
		     D3DFmt2GLFmt(This->format),
		     D3DFmt2GLType(This->format),
		     This->surfaces[i]->allocatedMemory);
	checkGLcall("glTexImage2D");
#endif
	/* Removed glTexParameterf now TextureStageStates are initialized at startup */
	This->Dirty = FALSE;
      }
    }
    return ;
}
D3DRESOURCETYPE WINAPI IDirect3DTexture8Impl_GetType(LPDIRECT3DTEXTURE8 iface) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) : is %d \n", This, This->ResourceType);
    return This->ResourceType;
}

/* IDirect3DTexture8 IDirect3DBaseTexture8 Interface follow: */
DWORD    WINAPI        IDirect3DTexture8Impl_SetLOD(LPDIRECT3DTEXTURE8 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DTexture8Impl_GetLOD(LPDIRECT3DTEXTURE8 iface) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DTexture8Impl_GetLevelCount(LPDIRECT3DTEXTURE8 iface) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) : returning %d\n", This, This->levels);
    return This->levels;
}

/* IDirect3DTexture8 */
HRESULT  WINAPI        IDirect3DTexture8Impl_GetLevelDesc(LPDIRECT3DTEXTURE8 iface, UINT Level,D3DSURFACE_DESC* pDesc) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);

    if (Level < This->levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IDirect3DSurface8Impl_GetDesc((LPDIRECT3DSURFACE8) This->surfaces[Level], pDesc);
    } else {
        FIXME("(%p) Level (%d)\n", This, Level);
    }
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE8 iface, UINT Level,IDirect3DSurface8** ppSurfaceLevel) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    *ppSurfaceLevel = (LPDIRECT3DSURFACE8)This->surfaces[Level];
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) This->surfaces[Level]);
    TRACE("(%p) : returning %p for level %d\n", This, *ppSurfaceLevel, Level);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_LockRect(LPDIRECT3DTEXTURE8 iface, UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) {
    HRESULT hr;
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) Level (%d)\n", This, Level);
    if (Level < This->levels) {
        /**
	 * Not dirtified while Surfaces don't notify dirtification
	 * This->Dirty = TRUE;
	 */
        hr = IDirect3DSurface8Impl_LockRect((LPDIRECT3DSURFACE8) This->surfaces[Level], pLockedRect, pRect, Flags);
        TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
        FIXME("Levels seems too high?!!\n");
    }
    return hr;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_UnlockRect(LPDIRECT3DTEXTURE8 iface, UINT Level) {
    HRESULT hr;
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    TRACE("(%p) Level (%d)\n", This, Level);
    if (Level < This->levels) {
        hr = IDirect3DSurface8Impl_UnlockRect((LPDIRECT3DSURFACE8) This->surfaces[Level]);
        TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
        FIXME("Levels seems too high?!!\n");
    }
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DTexture8Impl_AddDirtyRect(LPDIRECT3DTEXTURE8 iface, CONST RECT* pDirtyRect) {
    ICOM_THIS(IDirect3DTexture8Impl,iface);
    This->Dirty = TRUE;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DTexture8) Direct3DTexture8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DTexture8Impl_QueryInterface,
    IDirect3DTexture8Impl_AddRef,
    IDirect3DTexture8Impl_Release,
    IDirect3DTexture8Impl_GetDevice,
    IDirect3DTexture8Impl_SetPrivateData,
    IDirect3DTexture8Impl_GetPrivateData,
    IDirect3DTexture8Impl_FreePrivateData,
    IDirect3DTexture8Impl_SetPriority,
    IDirect3DTexture8Impl_GetPriority,
    IDirect3DTexture8Impl_PreLoad,
    IDirect3DTexture8Impl_GetType,
    IDirect3DTexture8Impl_SetLOD,
    IDirect3DTexture8Impl_GetLOD,
    IDirect3DTexture8Impl_GetLevelCount,
    IDirect3DTexture8Impl_GetLevelDesc,
    IDirect3DTexture8Impl_GetSurfaceLevel,
    IDirect3DTexture8Impl_LockRect,
    IDirect3DTexture8Impl_UnlockRect,
    IDirect3DTexture8Impl_AddDirtyRect

};
