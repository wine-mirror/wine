/*
 * IWineD3DTexture implementation
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d_texture);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DTextureImpl_QueryInterface(IWineD3DTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
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
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DTextureImpl_Release(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        int i;

        TRACE("(%p) : Cleaning up\n",This);
        for (i = 0; i < This->baseTexture.levels; i++) {
            if (This->surfaces[i] != NULL) {
                /* Because the surfaces were created using a callback we need to release there parent otehrwise we leave the parent hanging */
                IUnknown* surfaceParent;
                /* Clean out the texture name we gave to the suface so that the surface doesn't try and release it */
                IWineD3DSurface_SetGlTextureDesc(This->surfaces[i], 0, 0);
                /* Cleanup the container */
                IWineD3DSurface_SetContainer(This->surfaces[i], 0);
                /* Now, release the parent, which will take care of cleaning up the surface for us */
                IWineD3DSurface_GetParent(This->surfaces[i], &surfaceParent);
                IUnknown_Release(surfaceParent);
                IUnknown_Release(surfaceParent);
            }
        }
        TRACE("(%p) : cleaning up base texture\n", This);
        IWineD3DBaseTextureImpl_CleanUp((IWineD3DBaseTexture *)iface);
        /* free the object */
        HeapFree(GetProcessHeap(), 0, This);
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

    /* Override the IWineD3DResource PreLoad method */
    unsigned int i;
    BOOL setGlTextureDesc = FALSE;
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;

    TRACE("(%p) : About to load texture\n", This);

    if (This->baseTexture.textureName == 0)  setGlTextureDesc = TRUE;

    IWineD3DTexture_BindTexture(iface);
    ENTER_GL();
        /* If were dirty then reload the surfaces */
    if(This->baseTexture.dirty != FALSE) {

        for (i = 0; i < This->baseTexture.levels; i++) {
            if(setGlTextureDesc)
                IWineD3DSurface_SetGlTextureDesc(This->surfaces[i], This->baseTexture.textureName, IWineD3DTexture_GetTextureDimensions(iface));
            IWineD3DSurface_LoadTexture(This->surfaces[i]);
        }

        /* No longer dirty */
        This->baseTexture.dirty = FALSE;
    } else {
        TRACE("(%p) Texture not dirty, nothing to do\n" , iface);
    }
    LEAVE_GL();

    return ;
}

WINED3DRESOURCETYPE WINAPI IWineD3DTextureImpl_GetType(IWineD3DTexture *iface) {
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
    return IWineD3DBaseTextureImpl_SetDirty((IWineD3DBaseTexture *)iface, dirty);
}

BOOL WINAPI IWineD3DTextureImpl_GetDirty(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetDirty((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DTextureImpl_BindTexture(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture\n", This);
    return IWineD3DBaseTextureImpl_BindTexture((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DTextureImpl_UnBindTexture(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture\n", This);
    return IWineD3DBaseTextureImpl_UnBindTexture((IWineD3DBaseTexture *)iface);
}

UINT WINAPI IWineD3DTextureImpl_GetTextureDimensions(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return GL_TEXTURE_2D;
}

void WINAPI IWineD3DTextureImpl_ApplyStateChanges(IWineD3DTexture *iface,
                                                   const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
                                                   const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    float matrix[16];
    IWineD3DBaseTextureImpl_ApplyStateChanges((IWineD3DBaseTexture *)iface, textureStates, samplerStates);

    /** non-power2 fixups using texture matrix **/
    if(This->pow2scalingFactorX != 1.0f || This->pow2scalingFactorY != 1.0f) {
        /* Apply non-power2 mappings and texture offsets so long as the texture coords aren't projected or generated */
        if(((textureStates[WINED3DTSS_TEXCOORDINDEX] & 0xFFFF0000) == D3DTSS_TCI_PASSTHRU) &&
                      (~textureStates[WINED3DTSS_TEXTURETRANSFORMFLAGS] & D3DTTFF_PROJECTED)) {
            glMatrixMode(GL_TEXTURE);
            memset(matrix, 0 , sizeof(matrix));
            matrix[0] = This->pow2scalingFactorX;
            matrix[5] = This->pow2scalingFactorY;
#if 0   /* this isn't needed any more, I changed the translation in drawprim.c to 0.9/width instead of 1/width and everything lines up ok. left here as a reminder */
            matrix[12] = -0.25f / (float)This->width;
            matrix[13] = -0.75f / (float)This->height;
#endif
            matrix[10] = 1;
            matrix[15] = 1;
            TRACE("(%p) Setup Matrix:\n", This);
            TRACE(" %f %f %f %f\n", matrix[0], matrix[1], matrix[2], matrix[3]);
            TRACE(" %f %f %f %f\n", matrix[4], matrix[5], matrix[6], matrix[7]);
            TRACE(" %f %f %f %f\n", matrix[8], matrix[9], matrix[10], matrix[11]);
            TRACE(" %f %f %f %f\n", matrix[12], matrix[13], matrix[14], matrix[15]);
            TRACE("\n");

            glMultMatrixf(matrix);
        } else {
            /* I don't expect nonpower 2 textures to be used with generated texture coordinates, but if they are present a fixme. */
            FIXME("Non-power2 texture being used with generated texture coords\n");
        }
    }

}

/* *******************************************
   IWineD3DTexture IWineD3DTexture parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DTextureImpl_GetLevelDesc(IWineD3DTexture *iface, UINT Level, WINED3DSURFACE_DESC* pDesc) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IWineD3DSurface_GetDesc(This->surfaces[Level], pDesc);
    }
    FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI IWineD3DTextureImpl_GetSurfaceLevel(IWineD3DTexture *iface, UINT Level, IWineD3DSurface** ppSurfaceLevel) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    HRESULT hr = D3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels) {
        *ppSurfaceLevel = This->surfaces[Level];
        IWineD3DSurface_AddRef((IWineD3DSurface*) This->surfaces[Level]);
        hr = D3D_OK;
        TRACE("(%p) : returning %p for level %d\n", This, *ppSurfaceLevel, Level);
    }
    if (D3D_OK != hr) {
        WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
        *ppSurfaceLevel = NULL; /* Just to be on the safe side.. */
    }
    return hr;
}

HRESULT WINAPI IWineD3DTextureImpl_LockRect(IWineD3DTexture *iface, UINT Level, D3DLOCKED_RECT *pLockedRect,
                                            CONST RECT *pRect, DWORD Flags) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    HRESULT hr = D3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels) {
        hr = IWineD3DSurface_LockRect(This->surfaces[Level], pLockedRect, pRect, Flags);
    }
    if (D3D_OK == hr) {
        TRACE("(%p) Level (%d) success\n", This, Level);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    }

    return hr;
}

HRESULT WINAPI IWineD3DTextureImpl_UnlockRect(IWineD3DTexture *iface, UINT Level) {
   IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    HRESULT hr = D3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels) {
        hr = IWineD3DSurface_UnlockRect(This->surfaces[Level]);
    }
    if ( D3D_OK == hr) {
        TRACE("(%p) Level (%d) success\n", This, Level);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    }
    return hr;
}

HRESULT WINAPI IWineD3DTextureImpl_AddDirtyRect(IWineD3DTexture *iface, CONST RECT* pDirtyRect) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    TRACE("(%p) : dirtyfication of surface Level (0)\n", This);
    return IWineD3DSurface_AddDirtyRect(This->surfaces[0], pDirtyRect);
}

const IWineD3DTextureVtbl IWineD3DTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DTextureImpl_QueryInterface,
    IWineD3DTextureImpl_AddRef,
    IWineD3DTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DTextureImpl_GetParent,
    IWineD3DTextureImpl_GetDevice,
    IWineD3DTextureImpl_SetPrivateData,
    IWineD3DTextureImpl_GetPrivateData,
    IWineD3DTextureImpl_FreePrivateData,
    IWineD3DTextureImpl_SetPriority,
    IWineD3DTextureImpl_GetPriority,
    IWineD3DTextureImpl_PreLoad,
    IWineD3DTextureImpl_GetType,
    /* IWineD3DBaseTexture */
    IWineD3DTextureImpl_SetLOD,
    IWineD3DTextureImpl_GetLOD,
    IWineD3DTextureImpl_GetLevelCount,
    IWineD3DTextureImpl_SetAutoGenFilterType,
    IWineD3DTextureImpl_GetAutoGenFilterType,
    IWineD3DTextureImpl_GenerateMipSubLevels,
    IWineD3DTextureImpl_SetDirty,
    IWineD3DTextureImpl_GetDirty,
    IWineD3DTextureImpl_BindTexture,
    IWineD3DTextureImpl_UnBindTexture,
    IWineD3DTextureImpl_GetTextureDimensions,
    IWineD3DTextureImpl_ApplyStateChanges,
    /* IWineD3DTexture */
    IWineD3DTextureImpl_GetLevelDesc,
    IWineD3DTextureImpl_GetSurfaceLevel,
    IWineD3DTextureImpl_LockRect,
    IWineD3DTextureImpl_UnlockRect,
    IWineD3DTextureImpl_AddDirtyRect
};
