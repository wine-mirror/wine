/*
 * IDirect3DTexture9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DTextureImpl_QueryInterface(IWineD3DTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DTexture)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DTextureImpl_AddRef(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->resource.ref);
    IUnknown_AddRef(This->resource.parent);
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DTextureImpl_Release(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        int i;
        for (i = 0; i < This->baseTexture.levels; i++) {
            if (This->surfaces[i] != NULL) {
                TRACE("(%p) : Releasing surface %p\n", This, This->surfaces[i]);
                IWineD3DSurface_Release((IWineD3DSurface *) This->surfaces[i]);
            }
        }
        IWineD3DDevice_Release((IWineD3DDevice *)This->resource.wineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This);
    } else {
        IUnknown_Release(This->resource.parent);  /* Released the reference to the d3dx object */
    }
    return ref;
}

/* ****************************************************
   IWineD3DTexture IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DTextureImpl_GetDevice(IWineD3DTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DTextureImpl_SetPrivateData(IWineD3DTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DTextureImpl_GetPrivateData(IWineD3DTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DTextureImpl_FreePrivateData(IWineD3DTexture *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD WINAPI IWineD3DTextureImpl_SetPriority(IWineD3DTexture *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD WINAPI IWineD3DTextureImpl_GetPriority(IWineD3DTexture *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void WINAPI IWineD3DTextureImpl_PreLoad(IWineD3DTexture *iface) {
    /* Override the IWineD3DResource Preload method */
    unsigned int i;
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    
    TRACE("(%p) : About to load texture\n", This);

    ENTER_GL();

    for (i = 0; i < This->baseTexture.levels; i++) {
      if (i == 0 && This->surfaces[i]->textureName != 0 && This->baseTexture.dirty == FALSE) {
        glBindTexture(GL_TEXTURE_2D, This->surfaces[i]->textureName);
        checkGLcall("glBindTexture");
        TRACE("Texture %p (level %d) given name %d\n", This->surfaces[i], i, This->surfaces[i]->textureName);
        /* No need to walk through all mip-map levels, since already all assigned */
        i = This->baseTexture.levels;

      } else {
        if (i == 0) {
          if (This->surfaces[i]->textureName == 0) {
            glGenTextures(1, &This->surfaces[i]->textureName);
            checkGLcall("glGenTextures");
            TRACE("Texture %p (level %d) given name %d\n", This->surfaces[i], i, This->surfaces[i]->textureName);
          }
          
          glBindTexture(GL_TEXTURE_2D, This->surfaces[i]->textureName);
          checkGLcall("glBindTexture");
        }
        IWineD3DSurface_LoadTexture((IWineD3DSurface *) This->surfaces[i], GL_TEXTURE_2D, i); 
      }
    }

    /* No longer dirty */
    This->baseTexture.dirty = FALSE;

    /* Always need to reset the number of mipmap levels when rebinding as it is
       a property of the active texture unit, and another texture may have set it
       to a different value                                                       */
    TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->baseTexture.levels - 1);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels - 1);
    checkGLcall("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->levels)");

    LEAVE_GL();

    return ;
}

D3DRESOURCETYPE WINAPI IWineD3DTextureImpl_GetType(IWineD3DTexture *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DTextureImpl_GetParent(IWineD3DTexture *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
DWORD WINAPI IWineD3DTextureImpl_SetLOD(IWineD3DTexture *iface, DWORD LODNew) {
    return IWineD3DBaseTextureImpl_SetLOD((IWineD3DBaseTexture *)iface, LODNew);
}

DWORD WINAPI IWineD3DTextureImpl_GetLOD(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLOD((IWineD3DBaseTexture *)iface);
}

DWORD WINAPI IWineD3DTextureImpl_GetLevelCount(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLevelCount((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DTextureImpl_SetAutoGenFilterType(IWineD3DTexture *iface, D3DTEXTUREFILTERTYPE FilterType) {
  return IWineD3DBaseTextureImpl_SetAutoGenFilterType((IWineD3DBaseTexture *)iface, FilterType);
}

D3DTEXTUREFILTERTYPE WINAPI IWineD3DTextureImpl_GetAutoGenFilterType(IWineD3DTexture *iface) {
  return IWineD3DBaseTextureImpl_GetAutoGenFilterType((IWineD3DBaseTexture *)iface);
}

void WINAPI IWineD3DTextureImpl_GenerateMipSubLevels(IWineD3DTexture *iface) {
  return IWineD3DBaseTextureImpl_GenerateMipSubLevels((IWineD3DBaseTexture *)iface);
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DTextureImpl_SetDirty(IWineD3DTexture *iface, BOOL dirty) {
    return IWineD3DBaseTextureImpl_SetDirty((IWineD3DBaseTexture *)iface, TRUE);
}

BOOL WINAPI IWineD3DTextureImpl_GetDirty(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetDirty((IWineD3DBaseTexture *)iface);
}

/* *******************************************
   IWineD3DTexture IWineD3DTexture parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DTextureImpl_GetLevelDesc(IWineD3DTexture *iface, UINT Level, WINED3DSURFACE_DESC* pDesc) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IWineD3DSurface_GetDesc((IWineD3DSurface *) This->surfaces[Level], pDesc);
    }
    FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI IWineD3DTextureImpl_GetSurfaceLevel(IWineD3DTexture *iface, UINT Level, IWineD3DSurface** ppSurfaceLevel) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    *ppSurfaceLevel = (IWineD3DSurface *) This->surfaces[Level];
    IWineD3DSurface_AddRef((IWineD3DSurface *) This->surfaces[Level]);
    TRACE("(%p) : returning %p for level %d\n", This, *ppSurfaceLevel, Level);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DTextureImpl_LockRect(IWineD3DTexture *iface, UINT Level, D3DLOCKED_RECT *pLockedRect, 
                                            CONST RECT *pRect, DWORD Flags) {
    HRESULT hr;
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {

        hr = IWineD3DSurface_LockRect((IWineD3DSurface *) This->surfaces[Level], pLockedRect, pRect, Flags);
        TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
        return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IWineD3DTextureImpl_UnlockRect(IWineD3DTexture *iface, UINT Level) {
    HRESULT hr;
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
        hr = IWineD3DSurface_UnlockRect((IWineD3DSurface *) This->surfaces[Level]);
        TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
        return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IWineD3DTextureImpl_AddDirtyRect(IWineD3DTexture *iface, CONST RECT* pDirtyRect) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    TRACE("(%p) : dirtyfication of surface Level (0)\n", This);    
    return IWineD3DSurface_AddDirtyRect((IWineD3DSurface *)This->surfaces[0], pDirtyRect);
}

IWineD3DTextureVtbl IWineD3DTexture_Vtbl =
{
    IWineD3DTextureImpl_QueryInterface,
    IWineD3DTextureImpl_AddRef,
    IWineD3DTextureImpl_Release,
    IWineD3DTextureImpl_GetParent,
    IWineD3DTextureImpl_GetDevice,
    IWineD3DTextureImpl_SetPrivateData,
    IWineD3DTextureImpl_GetPrivateData,
    IWineD3DTextureImpl_FreePrivateData,
    IWineD3DTextureImpl_SetPriority,
    IWineD3DTextureImpl_GetPriority,
    IWineD3DTextureImpl_PreLoad,
    IWineD3DTextureImpl_GetType,
    IWineD3DTextureImpl_SetLOD,
    IWineD3DTextureImpl_GetLOD,
    IWineD3DTextureImpl_GetLevelCount,
    IWineD3DTextureImpl_SetAutoGenFilterType,
    IWineD3DTextureImpl_GetAutoGenFilterType,
    IWineD3DTextureImpl_GenerateMipSubLevels,
    IWineD3DTextureImpl_SetDirty,
    IWineD3DTextureImpl_GetDirty,
    IWineD3DTextureImpl_GetLevelDesc,
    IWineD3DTextureImpl_GetSurfaceLevel,
    IWineD3DTextureImpl_LockRect,
    IWineD3DTextureImpl_UnlockRect,
    IWineD3DTextureImpl_AddDirtyRect
};
