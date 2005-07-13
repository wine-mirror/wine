/*
 * IWineD3DCubeTexture implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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

/* *******************************************
   IWineD3DCubeTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DCubeTextureImpl_QueryInterface(IWineD3DCubeTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DTexture)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DCubeTextureImpl_AddRef(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->resource.ref);
    IUnknown_AddRef(This->resource.parent);
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DCubeTextureImpl_Release(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        int i,j;
      TRACE("(%p) : Cleaning up\n",This);
        for (i = 0; i < This->baseTexture.levels; i++) {
          for (j = 0; j < 6; j++) {
            if (This->surfaces[j][i] != NULL) {
                /* Because the surfaces were created using a callback we need to release there parent otehrwise we leave the parent hanging */
                IUnknown* surfaceParent;
                /* Clean out the texture name we gave to the suface so that the surface doesn't try and release it */
                IWineD3DSurface_SetGlTextureDesc(This->surfaces[j][i], 0, 0);
                TRACE("(%p) : Releasing surface%d %d  %p\n", This, j, i, This->surfaces[j][i]);
                IWineD3DSurface_GetParent(This->surfaces[j][i], &surfaceParent);
                IUnknown_Release(surfaceParent);
                IUnknown_Release(surfaceParent);

            }
          }
        }
        IWineD3DBaseTextureImpl_CleanUp((IWineD3DBaseTexture *) iface);
        HeapFree(GetProcessHeap(), 0, This);
    } else {
        IUnknown_Release(This->resource.parent);  /* Released the reference to the d3dx object */
    }
    return ref;
}

/* ****************************************************
   IWineD3DCubeTexture IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DCubeTextureImpl_GetDevice(IWineD3DCubeTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_SetPrivateData(IWineD3DCubeTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_GetPrivateData(IWineD3DCubeTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_FreePrivateData(IWineD3DCubeTexture *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD WINAPI IWineD3DCubeTextureImpl_SetPriority(IWineD3DCubeTexture *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD WINAPI IWineD3DCubeTextureImpl_GetPriority(IWineD3DCubeTexture *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void WINAPI IWineD3DCubeTextureImpl_PreLoad(IWineD3DCubeTexture *iface) {
    /* Override the IWineD3DResource Preload method */
    unsigned int i,j;
    BOOL setGlTextureDesc = FALSE;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    TRACE("(%p) : About to load texture: dirtified(%d)\n", This, This->baseTexture.dirty);

    if (This->baseTexture.textureName == 0)  setGlTextureDesc = TRUE;

    IWineD3DCubeTexture_BindTexture(iface);

    ENTER_GL();
    /* If were dirty then reload the surfaces */
    if (This->baseTexture.dirty != FALSE) {
        for (i = 0; i < This->baseTexture.levels; i++) {
          for (j = 0; j < 6; j++)
              if(setGlTextureDesc)
                  IWineD3DSurface_SetGlTextureDesc(This->surfaces[j][i], This->baseTexture.textureName, cube_targets[j]);
                  IWineD3DSurface_LoadTexture((IWineD3DSurface *) This->surfaces[j][i]);
        }
        /* No longer dirty */
        This->baseTexture.dirty = FALSE;
    }
    LEAVE_GL();
    return ;
}

D3DRESOURCETYPE WINAPI IWineD3DCubeTextureImpl_GetType(IWineD3DCubeTexture *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_GetParent(IWineD3DCubeTexture *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DCubeTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
DWORD WINAPI IWineD3DCubeTextureImpl_SetLOD(IWineD3DCubeTexture *iface, DWORD LODNew) {
    return IWineD3DBaseTextureImpl_SetLOD((IWineD3DBaseTexture *)iface, LODNew);
}

DWORD WINAPI IWineD3DCubeTextureImpl_GetLOD(IWineD3DCubeTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLOD((IWineD3DBaseTexture *)iface);
}

DWORD WINAPI IWineD3DCubeTextureImpl_GetLevelCount(IWineD3DCubeTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLevelCount((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_SetAutoGenFilterType(IWineD3DCubeTexture *iface, D3DTEXTUREFILTERTYPE FilterType) {
  return IWineD3DBaseTextureImpl_SetAutoGenFilterType((IWineD3DBaseTexture *)iface, FilterType);
}

D3DTEXTUREFILTERTYPE WINAPI IWineD3DCubeTextureImpl_GetAutoGenFilterType(IWineD3DCubeTexture *iface) {
  return IWineD3DBaseTextureImpl_GetAutoGenFilterType((IWineD3DBaseTexture *)iface);
}

void WINAPI IWineD3DCubeTextureImpl_GenerateMipSubLevels(IWineD3DCubeTexture *iface) {
  return IWineD3DBaseTextureImpl_GenerateMipSubLevels((IWineD3DBaseTexture *)iface);
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DCubeTextureImpl_SetDirty(IWineD3DCubeTexture *iface, BOOL dirty) {
    return IWineD3DBaseTextureImpl_SetDirty((IWineD3DBaseTexture *)iface, dirty);
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DCubeTextureImpl_GetDirty(IWineD3DCubeTexture *iface) {
    return IWineD3DBaseTextureImpl_GetDirty((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_BindTexture(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture \n", This);
    return IWineD3DBaseTextureImpl_BindTexture((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DCubeTextureImpl_UnBindTexture(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture \n", This);
    return IWineD3DBaseTextureImpl_UnBindTexture((IWineD3DBaseTexture *)iface);
}

UINT WINAPI IWineD3DCubeTextureImpl_GetTextureDimensions(IWineD3DCubeTexture *iface){
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p) \n", This);

    return GLTEXTURECUBEMAP;
}

/* *******************************************
   IWineD3DCubeTexture IWineD3DCubeTexture parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DCubeTextureImpl_GetLevelDesc(IWineD3DCubeTexture *iface, UINT Level, WINED3DSURFACE_DESC* pDesc) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
        TRACE("(%p) level (%d)\n", This, Level);
        return IWineD3DSurface_GetDesc((IWineD3DSurface *) This->surfaces[0][Level], pDesc);
    }
    FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI IWineD3DCubeTextureImpl_GetCubeMapSurface(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, UINT Level, IWineD3DSurface** ppCubeMapSurface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    if (Level < This->baseTexture.levels) {
        *ppCubeMapSurface = (IWineD3DSurface *) This->surfaces[FaceType][Level];
        IWineD3DSurface_AddRef((IWineD3DSurface *) *ppCubeMapSurface);
        TRACE("(%p) -> faceType(%d) level(%d) returning surface@%p \n", This, FaceType, Level, This->surfaces[FaceType][Level]);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
        return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT WINAPI IWineD3DCubeTextureImpl_LockRect(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
      hr = IWineD3DSurface_LockRect((IWineD3DSurface *) This->surfaces[FaceType][Level], pLockedRect, pRect, Flags);
      TRACE("(%p) -> faceType(%d) level(%d) returning memory@%p success(%lu)\n", This, FaceType, Level, pLockedRect->pBits, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IWineD3DCubeTextureImpl_UnlockRect(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, UINT Level) {
    HRESULT hr;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
      hr = IWineD3DSurface_UnlockRect((IWineD3DSurface *) This->surfaces[FaceType][Level]);
      TRACE("(%p) -> faceType(%d) level(%d) success(%lu)\n", This, FaceType, Level, hr);
    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT  WINAPI IWineD3DCubeTextureImpl_AddDirtyRect(IWineD3DCubeTexture *iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    TRACE("(%p) : dirtyfication of faceType(%d) Level (0)\n", This, FaceType);
    return IWineD3DSurface_AddDirtyRect((IWineD3DSurface *) This->surfaces[FaceType][0], pDirtyRect);
}


const IWineD3DCubeTextureVtbl IWineD3DCubeTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DCubeTextureImpl_QueryInterface,
    IWineD3DCubeTextureImpl_AddRef,
    IWineD3DCubeTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DCubeTextureImpl_GetParent,
    IWineD3DCubeTextureImpl_GetDevice,
    IWineD3DCubeTextureImpl_SetPrivateData,
    IWineD3DCubeTextureImpl_GetPrivateData,
    IWineD3DCubeTextureImpl_FreePrivateData,
    IWineD3DCubeTextureImpl_SetPriority,
    IWineD3DCubeTextureImpl_GetPriority,
    IWineD3DCubeTextureImpl_PreLoad,
    IWineD3DCubeTextureImpl_GetType,
    /* IWineD3DBaseTexture */
    IWineD3DCubeTextureImpl_SetLOD,
    IWineD3DCubeTextureImpl_GetLOD,
    IWineD3DCubeTextureImpl_GetLevelCount,
    IWineD3DCubeTextureImpl_SetAutoGenFilterType,
    IWineD3DCubeTextureImpl_GetAutoGenFilterType,
    IWineD3DCubeTextureImpl_GenerateMipSubLevels,
    IWineD3DCubeTextureImpl_SetDirty,
    IWineD3DCubeTextureImpl_GetDirty,
    IWineD3DCubeTextureImpl_BindTexture,
    IWineD3DCubeTextureImpl_UnBindTexture,
    IWineD3DCubeTextureImpl_GetTextureDimensions,
    /* IWineD3DCubeTexture */
    IWineD3DCubeTextureImpl_GetLevelDesc,
    IWineD3DCubeTextureImpl_GetCubeMapSurface,
    IWineD3DCubeTextureImpl_LockRect,
    IWineD3DCubeTextureImpl_UnlockRect,
    IWineD3DCubeTextureImpl_AddDirtyRect
};
