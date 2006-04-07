/*
 * IWineD3DVolume implementation
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
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DVolume IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVolumeImpl_QueryInterface(IWineD3DVolume *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DVolume)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return WINED3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVolumeImpl_AddRef(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DVolumeImpl_Release(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVolume IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DVolumeImpl_GetParent(IWineD3DVolume *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

HRESULT WINAPI IWineD3DVolumeImpl_GetDevice(IWineD3DVolume *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DVolumeImpl_SetPrivateData(IWineD3DVolume *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DVolumeImpl_GetPrivateData(IWineD3DVolume *iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DVolumeImpl_FreePrivateData(IWineD3DVolume *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD WINAPI IWineD3DVolumeImpl_SetPriority(IWineD3DVolume *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD WINAPI IWineD3DVolumeImpl_GetPriority(IWineD3DVolume *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void WINAPI IWineD3DVolumeImpl_PreLoad(IWineD3DVolume *iface) {
    return IWineD3DResourceImpl_PreLoad((IWineD3DResource *)iface);
}

WINED3DRESOURCETYPE WINAPI IWineD3DVolumeImpl_GetType(IWineD3DVolume *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

/* *******************************************
   IWineD3DVolume parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVolumeImpl_GetContainerParent(IWineD3DVolume *iface, IUnknown **ppContainerParent) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;

    TRACE("(%p) : ppContainerParent %p\n", This, ppContainerParent);

    if (!ppContainerParent) {
        ERR("(%p) : Called without a valid ppContainerParent\n", This);
    }

    if (This->container) {
        IWineD3DBase_GetParent(This->container, ppContainerParent);
        if (!ppContainerParent) {
            /* WineD3D objects should always have a parent */
            ERR("(%p) : GetParent returned NULL\n", This);
        }
        IUnknown_Release(*ppContainerParent); /* GetParent adds a reference; we want just the pointer */
    } else {
        *ppContainerParent = NULL;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_GetContainer(IWineD3DVolume *iface, REFIID riid, void** ppContainer) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!ppContainer) {
        ERR("Called without a valid ppContainer.\n");
    }

    /* Although surfaces can be standalone, volumes can't */
    if (!This->container) {
        ERR("Volume without an container. Should not happen.\n");
    }

    TRACE("Relaying to QueryInterface\n");
    return IUnknown_QueryInterface(This->container, riid, ppContainer);
}

HRESULT WINAPI IWineD3DVolumeImpl_GetDesc(IWineD3DVolume *iface, WINED3DVOLUME_DESC* pDesc) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : copying into %p\n", This, pDesc);

    *(pDesc->Format)  = This->resource.format;
    *(pDesc->Type)    = This->resource.resourceType;
    *(pDesc->Usage)   = This->resource.usage;
    *(pDesc->Pool)    = This->resource.pool;
    *(pDesc->Size)    = This->resource.size; /* dx8 only */
    *(pDesc->Width)   = This->currentDesc.Width;
    *(pDesc->Height)  = This->currentDesc.Height;
    *(pDesc->Depth)   = This->currentDesc.Depth;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_LockBox(IWineD3DVolume *iface, WINED3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : pBox=%p stub\n", This, pBox);

    /* fixme: should we really lock as such? */
    TRACE("(%p) : box=%p, output pbox=%p, allMem=%p\n", This, pBox, pLockedVolume, This->resource.allocatedMemory);

    pLockedVolume->RowPitch   = This->bytesPerPixel * This->currentDesc.Width;                        /* Bytes / row   */
    pLockedVolume->SlicePitch = This->bytesPerPixel * This->currentDesc.Width * This->currentDesc.Height;  /* Bytes / slice */
    if (!pBox) {
        TRACE("No box supplied - all is ok\n");
        pLockedVolume->pBits = This->resource.allocatedMemory;
        This->lockedBox.Left   = 0;
        This->lockedBox.Top    = 0;
        This->lockedBox.Front  = 0;
        This->lockedBox.Right  = This->currentDesc.Width;
        This->lockedBox.Bottom = This->currentDesc.Height;
        This->lockedBox.Back   = This->currentDesc.Depth;
    } else {
        TRACE("Lock Box (%p) = l %d, t %d, r %d, b %d, fr %d, ba %d\n", pBox, pBox->Left, pBox->Top, pBox->Right, pBox->Bottom, pBox->Front, pBox->Back);
        pLockedVolume->pBits = This->resource.allocatedMemory +
          (pLockedVolume->SlicePitch * pBox->Front) + /* FIXME: is front < back or vica versa? */
          (pLockedVolume->RowPitch * pBox->Top) +
          (pBox->Left * This->bytesPerPixel);
        This->lockedBox.Left   = pBox->Left;
        This->lockedBox.Top    = pBox->Top;
        This->lockedBox.Front  = pBox->Front;
        This->lockedBox.Right  = pBox->Right;
        This->lockedBox.Bottom = pBox->Bottom;
        This->lockedBox.Back   = pBox->Back;
    }

    if (Flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)) {
      /* Don't dirtify */
    } else {
      /**
       * Dirtify on lock
       * as seen in msdn docs
       */
      IWineD3DVolume_AddDirtyBox(iface, &This->lockedBox);

      /**  Dirtify Container if needed */
      if (NULL != This->container) {

        IWineD3DVolumeTexture *cont = (IWineD3DVolumeTexture*) This->container;
        WINED3DRESOURCETYPE containerType = IWineD3DBaseTexture_GetType((IWineD3DBaseTexture *) cont);

        if (containerType == WINED3DRTYPE_VOLUMETEXTURE) {
          IWineD3DBaseTextureImpl* pTexture = (IWineD3DBaseTextureImpl*) cont;
          pTexture->baseTexture.dirty = TRUE;
        } else {
          FIXME("Set dirty on container type %d\n", containerType);
        }
      }
    }

    This->locked = TRUE;
    TRACE("returning memory@%p rpitch(%d) spitch(%d)\n", pLockedVolume->pBits, pLockedVolume->RowPitch, pLockedVolume->SlicePitch);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_UnlockBox(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    if (FALSE == This->locked) {
      ERR("trying to lock unlocked volume@%p\n", This);
      return WINED3DERR_INVALIDCALL;
    }
    TRACE("(%p) : unlocking volume\n", This);
    This->locked = FALSE;
    memset(&This->lockedBox, 0, sizeof(RECT));
    return WINED3D_OK;
}

/* Internal use functions follow : */

HRESULT WINAPI IWineD3DVolumeImpl_CleanDirtyBox(IWineD3DVolume *iface) {
  IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
  This->dirty = FALSE;
  This->lockedBox.Left   = This->currentDesc.Width;
  This->lockedBox.Top    = This->currentDesc.Height;
  This->lockedBox.Front  = This->currentDesc.Depth;
  This->lockedBox.Right  = 0;
  This->lockedBox.Bottom = 0;
  This->lockedBox.Back   = 0;
  return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_AddDirtyBox(IWineD3DVolume *iface, CONST D3DBOX* pDirtyBox) {
  IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
  This->dirty = TRUE;
   if (NULL != pDirtyBox) {
    This->lockedBox.Left   = min(This->lockedBox.Left,   pDirtyBox->Left);
    This->lockedBox.Top    = min(This->lockedBox.Top,    pDirtyBox->Top);
    This->lockedBox.Front  = min(This->lockedBox.Front,  pDirtyBox->Front);
    This->lockedBox.Right  = max(This->lockedBox.Right,  pDirtyBox->Right);
    This->lockedBox.Bottom = max(This->lockedBox.Bottom, pDirtyBox->Bottom);
    This->lockedBox.Back   = max(This->lockedBox.Back,   pDirtyBox->Back);
  } else {
    This->lockedBox.Left   = 0;
    This->lockedBox.Top    = 0;
    This->lockedBox.Front  = 0;
    This->lockedBox.Right  = This->currentDesc.Width;
    This->lockedBox.Bottom = This->currentDesc.Height;
    This->lockedBox.Back   = This->currentDesc.Depth;
  }
  return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_SetContainer(IWineD3DVolume *iface, IWineD3DBase* container) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;

    TRACE("This %p, container %p\n", This, container);

    /* We can't keep a reference to the container, since the container already keeps a reference to us. */

    TRACE("Setting container to %p from %p\n", container, This->container);
    This->container = container;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_LoadTexture(IWineD3DVolume *iface, GLenum gl_level) {
    IWineD3DVolumeImpl *This     = (IWineD3DVolumeImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;

    TRACE("Calling glTexImage3D %x level=%d, intfmt=%x, w=%d, h=%d,d=%d, 0=%d, glFmt=%x, glType=%x, Mem=%p\n",
            GL_TEXTURE_3D,
            gl_level,
            D3DFmt2GLIntFmt(myDevice, This->currentDesc.Format),
            This->currentDesc.Width,
            This->currentDesc.Height,
            This->currentDesc.Depth,
            0,
            D3DFmt2GLFmt(myDevice, This->currentDesc.Format),
            D3DFmt2GLType(myDevice, This->currentDesc.Format),
            This->resource.allocatedMemory);
    glTexImage3D(GL_TEXTURE_3D,
                    gl_level,
                    D3DFmt2GLIntFmt(myDevice, This->currentDesc.Format),
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    This->currentDesc.Depth,
                    0,
                    D3DFmt2GLFmt(myDevice, This->currentDesc.Format),
                    D3DFmt2GLType(myDevice, This->currentDesc.Format),
                    This->resource.allocatedMemory);
    checkGLcall("glTexImage3D");
    return WINED3D_OK;

}

const IWineD3DVolumeVtbl IWineD3DVolume_Vtbl =
{
    /* IUnknown */
    IWineD3DVolumeImpl_QueryInterface,
    IWineD3DVolumeImpl_AddRef,
    IWineD3DVolumeImpl_Release,
    /* IWineD3DResource */
    IWineD3DVolumeImpl_GetParent,
    IWineD3DVolumeImpl_GetDevice,
    IWineD3DVolumeImpl_SetPrivateData,
    IWineD3DVolumeImpl_GetPrivateData,
    IWineD3DVolumeImpl_FreePrivateData,
    IWineD3DVolumeImpl_SetPriority,
    IWineD3DVolumeImpl_GetPriority,
    IWineD3DVolumeImpl_PreLoad,
    IWineD3DVolumeImpl_GetType,
    /* IWineD3DVolume */
    IWineD3DVolumeImpl_GetContainerParent,
    IWineD3DVolumeImpl_GetContainer,
    IWineD3DVolumeImpl_GetDesc,
    IWineD3DVolumeImpl_LockBox,
    IWineD3DVolumeImpl_UnlockBox,
    /* Internal interface */
    IWineD3DVolumeImpl_AddDirtyBox,
    IWineD3DVolumeImpl_CleanDirtyBox,
    IWineD3DVolumeImpl_LoadTexture,
    IWineD3DVolumeImpl_SetContainer
};
