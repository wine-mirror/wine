/*
 * IWineD3DVolumeTexture implementation
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

/* *******************************************
   IWineD3DTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVolumeTextureImpl_QueryInterface(IWineD3DVolumeTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DVolumeTexture)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVolumeTextureImpl_AddRef(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DVolumeTextureImpl_Release(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    ULONG ref;
    int i;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        for (i = 0; i < This->baseTexture.levels; i++) {
            if (This->volumes[i] != NULL) {
                /* Since the volumes were created by callback, the texture is
                 * keeping the reference to the parent, so the texture should
                 * release it. */
                IUnknown *volumeParent = NULL;

                TRACE("(%p) : Releasing volume %p\n", This, This->volumes[i]);

                /* Cleanup the container */
                IWineD3DVolume_SetContainer(This->volumes[i], 0);
                /* Now, release the parent, which will take care of cleaning up the volume for us */
                IWineD3DVolume_GetParent(This->volumes[i], &volumeParent);
                IUnknown_Release(volumeParent); /* Once for the reference GetParent added */
                IUnknown_Release(volumeParent); /* Once for the reference we're keeping */
            }
        }
        IWineD3DBaseTextureImpl_CleanUp((IWineD3DBaseTexture *) iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVolumeTexture IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DVolumeTextureImpl_GetDevice(IWineD3DVolumeTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_SetPrivateData(IWineD3DVolumeTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_GetPrivateData(IWineD3DVolumeTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_FreePrivateData(IWineD3DVolumeTexture *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD WINAPI IWineD3DVolumeTextureImpl_SetPriority(IWineD3DVolumeTexture *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD WINAPI IWineD3DVolumeTextureImpl_GetPriority(IWineD3DVolumeTexture *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void WINAPI IWineD3DVolumeTextureImpl_PreLoad(IWineD3DVolumeTexture *iface) {
    /* Overrider the IWineD3DResource Preload method */
    UINT i;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;

    TRACE("(%p) : About to load texture\n", This);

    IWineD3DVolumeTexture_BindTexture(iface);

    ENTER_GL();
    /* If were dirty then reload the volumes */
    if(This->baseTexture.dirty != FALSE) {
        for (i = 0; i < This->baseTexture.levels; i++) {
            IWineD3DVolume_LoadTexture(This->volumes[i], i);
        }

        /* No longer dirty */
        This->baseTexture.dirty = FALSE;
    }
    LEAVE_GL();

    return ;
}

WINED3DRESOURCETYPE WINAPI IWineD3DVolumeTextureImpl_GetType(IWineD3DVolumeTexture *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_GetParent(IWineD3DVolumeTexture *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DVolumeTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
DWORD WINAPI IWineD3DVolumeTextureImpl_SetLOD(IWineD3DVolumeTexture *iface, DWORD LODNew) {
    return IWineD3DBaseTextureImpl_SetLOD((IWineD3DBaseTexture *)iface, LODNew);
}

DWORD WINAPI IWineD3DVolumeTextureImpl_GetLOD(IWineD3DVolumeTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLOD((IWineD3DBaseTexture *)iface);
}

DWORD WINAPI IWineD3DVolumeTextureImpl_GetLevelCount(IWineD3DVolumeTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLevelCount((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_SetAutoGenFilterType(IWineD3DVolumeTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType) {
  return IWineD3DBaseTextureImpl_SetAutoGenFilterType((IWineD3DBaseTexture *)iface, FilterType);
}

WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DVolumeTextureImpl_GetAutoGenFilterType(IWineD3DVolumeTexture *iface) {
  return IWineD3DBaseTextureImpl_GetAutoGenFilterType((IWineD3DBaseTexture *)iface);
}

void WINAPI IWineD3DVolumeTextureImpl_GenerateMipSubLevels(IWineD3DVolumeTexture *iface) {
  return IWineD3DBaseTextureImpl_GenerateMipSubLevels((IWineD3DBaseTexture *)iface);
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DVolumeTextureImpl_SetDirty(IWineD3DVolumeTexture *iface, BOOL dirty) {
    return IWineD3DBaseTextureImpl_SetDirty((IWineD3DBaseTexture *)iface, dirty);
}

BOOL WINAPI IWineD3DVolumeTextureImpl_GetDirty(IWineD3DVolumeTexture *iface) {
    return IWineD3DBaseTextureImpl_GetDirty((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_BindTexture(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture\n", This);
    return IWineD3DBaseTextureImpl_BindTexture((IWineD3DBaseTexture *)iface);
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_UnBindTexture(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture\n", This);
    return IWineD3DBaseTextureImpl_UnBindTexture((IWineD3DBaseTexture *)iface);
}

UINT WINAPI IWineD3DVolumeTextureImpl_GetTextureDimensions(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p)\n", This);
    return GL_TEXTURE_3D;
}

void WINAPI IWineD3DVolumeTextureImpl_ApplyStateChanges(IWineD3DVolumeTexture *iface,
                                                        const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
                                                        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : nothing to do, passing to base texture\n", This);
    IWineD3DBaseTextureImpl_ApplyStateChanges((IWineD3DBaseTexture *)iface, textureStates, samplerStates);
}


/* *******************************************
   IWineD3DVolumeTexture IWineD3DVolumeTexture parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVolumeTextureImpl_GetLevelDesc(IWineD3DVolumeTexture *iface, UINT Level,WINED3DVOLUME_DESC *pDesc) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    if (Level < This->baseTexture.levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IWineD3DVolume_GetDesc((IWineD3DVolume *) This->volumes[Level], pDesc);
    } else {
        FIXME("(%p) Level (%d)\n", This, Level);
    }
    return D3D_OK;
}
HRESULT WINAPI IWineD3DVolumeTextureImpl_GetVolumeLevel(IWineD3DVolumeTexture *iface, UINT Level, IWineD3DVolume** ppVolumeLevel) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    if (Level < This->baseTexture.levels) {
      *ppVolumeLevel = (IWineD3DVolume *)This->volumes[Level];
      IWineD3DVolume_AddRef((IWineD3DVolume *) *ppVolumeLevel);
      TRACE("(%p) -> level(%d) returning volume@%p\n", This, Level, *ppVolumeLevel);
    } else {
      FIXME("(%p) Level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return D3DERR_INVALIDCALL;
    }
    return D3D_OK;

}
HRESULT WINAPI IWineD3DVolumeTextureImpl_LockBox(IWineD3DVolumeTexture *iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    HRESULT hr;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
      hr = IWineD3DVolume_LockBox((IWineD3DVolume *)This->volumes[Level], pLockedVolume, pBox, Flags);
      TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);

    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_UnlockBox(IWineD3DVolumeTexture *iface, UINT Level) {
    HRESULT hr;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
      hr = IWineD3DVolume_UnlockBox((IWineD3DVolume*) This->volumes[Level]);
      TRACE("(%p) -> level(%d) success(%lu)\n", This, Level, hr);

    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IWineD3DVolumeTextureImpl_AddDirtyBox(IWineD3DVolumeTexture *iface, CONST D3DBOX* pDirtyBox) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    TRACE("(%p) : dirtyfication of volume Level (0)\n", This);
    return IWineD3DVolume_AddDirtyBox((IWineD3DVolume *) This->volumes[0], pDirtyBox);
}

const IWineD3DVolumeTextureVtbl IWineD3DVolumeTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DVolumeTextureImpl_QueryInterface,
    IWineD3DVolumeTextureImpl_AddRef,
    IWineD3DVolumeTextureImpl_Release,
    /* resource */
    IWineD3DVolumeTextureImpl_GetParent,
    IWineD3DVolumeTextureImpl_GetDevice,
    IWineD3DVolumeTextureImpl_SetPrivateData,
    IWineD3DVolumeTextureImpl_GetPrivateData,
    IWineD3DVolumeTextureImpl_FreePrivateData,
    IWineD3DVolumeTextureImpl_SetPriority,
    IWineD3DVolumeTextureImpl_GetPriority,
    IWineD3DVolumeTextureImpl_PreLoad,
    IWineD3DVolumeTextureImpl_GetType,
    /* BaseTexture */
    IWineD3DVolumeTextureImpl_SetLOD,
    IWineD3DVolumeTextureImpl_GetLOD,
    IWineD3DVolumeTextureImpl_GetLevelCount,
    IWineD3DVolumeTextureImpl_SetAutoGenFilterType,
    IWineD3DVolumeTextureImpl_GetAutoGenFilterType,
    IWineD3DVolumeTextureImpl_GenerateMipSubLevels,
    IWineD3DVolumeTextureImpl_SetDirty,
    IWineD3DVolumeTextureImpl_GetDirty,
    /* not in d3d */
    IWineD3DVolumeTextureImpl_BindTexture,
    IWineD3DVolumeTextureImpl_UnBindTexture,
    IWineD3DVolumeTextureImpl_GetTextureDimensions,
    IWineD3DVolumeTextureImpl_ApplyStateChanges,
    /* volume texture */
    IWineD3DVolumeTextureImpl_GetLevelDesc,
    IWineD3DVolumeTextureImpl_GetVolumeLevel,
    IWineD3DVolumeTextureImpl_LockBox,
    IWineD3DVolumeTextureImpl_UnlockBox,
    IWineD3DVolumeTextureImpl_AddDirtyBox
};
