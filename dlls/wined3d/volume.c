/*
 * IWineD3DVolume implementation
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
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DVolume IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVolumeImpl_QueryInterface(IWineD3DVolume *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    WARN("(%p)->(%s,%p) should not be called\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVolumeImpl_AddRef(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    IUnknown_AddRef(This->parent);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DVolumeImpl_Release(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
        IWineD3DDevice_Release((IWineD3DDevice *)This->wineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This);
    } else {
        IUnknown_Release(This->parent);  /* Released the reference to the d3dx object */
    }
    return ref;
}

/* *******************************************
   IWineD3DVolume parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVolumeImpl_GetParent(IWineD3DVolume *iface, IUnknown **pParent) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    IUnknown_AddRef(This->parent);
    *pParent = This->parent;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_GetDevice(IWineD3DVolume *iface, IWineD3DDevice** ppDevice) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->wineD3DDevice);
    
    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_SetPrivateData(IWineD3DVolume *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_GetPrivateData(IWineD3DVolume *iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_FreePrivateData(IWineD3DVolume *iface, REFGUID refguid) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_GetContainer(IWineD3DVolume *iface, REFIID riid, void** ppContainer) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->container);
    *ppContainer = This->container;
    IUnknown_AddRef(This->container);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_GetDesc(IWineD3DVolume *iface, WINED3DVOLUME_DESC* pDesc) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : copying into %p\n", This, pDesc);
    
    *(pDesc->Format)             = This->currentDesc.Format;
    *(pDesc->Type)               = This->currentDesc.Type;
    *(pDesc->Usage)              = This->currentDesc.Usage;
    *(pDesc->Pool)               = This->currentDesc.Pool;
    *(pDesc->Size)               = This->currentDesc.Size;   /* dx8 only */
    *(pDesc->Width)              = This->currentDesc.Width;
    *(pDesc->Height)             = This->currentDesc.Height;
    *(pDesc->Depth)              = This->currentDesc.Depth;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_LockBox(IWineD3DVolume *iface, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : pBox=%p stub\n", This, pBox);    

    /* fixme: should we really lock as such? */
    TRACE("(%p) : box=%p, output pbox=%p, allMem=%p\n", This, pBox, pLockedVolume, This->allocatedMemory);

    pLockedVolume->RowPitch   = This->bytesPerPixel * This->currentDesc.Width;                        /* Bytes / row   */
    pLockedVolume->SlicePitch = This->bytesPerPixel * This->currentDesc.Width * This->currentDesc.Height;  /* Bytes / slice */
    if (!pBox) {
        TRACE("No box supplied - all is ok\n");
        pLockedVolume->pBits = This->allocatedMemory;
        This->lockedBox.Left   = 0;
        This->lockedBox.Top    = 0;
        This->lockedBox.Front  = 0;
        This->lockedBox.Right  = This->currentDesc.Width;
        This->lockedBox.Bottom = This->currentDesc.Height;
        This->lockedBox.Back   = This->currentDesc.Depth;
    } else {
        TRACE("Lock Box (%p) = l %d, t %d, r %d, b %d, fr %d, ba %d\n", pBox, pBox->Left, pBox->Top, pBox->Right, pBox->Bottom, pBox->Front, pBox->Back);
        pLockedVolume->pBits = This->allocatedMemory + 
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
    
    if (Flags & (D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY)) {
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
        D3DRESOURCETYPE containerType = IWineD3DBaseTexture_GetType((IWineD3DBaseTexture *) cont);

        if (containerType == D3DRTYPE_VOLUMETEXTURE) {
          IWineD3DBaseTextureImpl* pTexture = (IWineD3DBaseTextureImpl*) cont;
          pTexture->baseTexture.dirty = TRUE;
        } else {
          FIXME("Set dirty on container type %d\n", containerType);
        }
      }
    }

    This->locked = TRUE;
    TRACE("returning memory@%p rpitch(%d) spitch(%d)\n", pLockedVolume->pBits, pLockedVolume->RowPitch, pLockedVolume->SlicePitch);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DVolumeImpl_UnlockBox(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    if (FALSE == This->locked) {
      ERR("trying to lock unlocked volume@%p\n", This);  
      return D3DERR_INVALIDCALL;
    }
    TRACE("(%p) : unlocking volume\n", This);
    This->locked = FALSE;
    memset(&This->lockedBox, 0, sizeof(RECT));
    return D3D_OK;
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
  return D3D_OK;
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
  return D3D_OK;
}

IWineD3DVolumeVtbl IWineD3DVolume_Vtbl =
{
    IWineD3DVolumeImpl_QueryInterface,
    IWineD3DVolumeImpl_AddRef,
    IWineD3DVolumeImpl_Release,
    IWineD3DVolumeImpl_GetParent,
    IWineD3DVolumeImpl_GetDevice,
    IWineD3DVolumeImpl_SetPrivateData,
    IWineD3DVolumeImpl_GetPrivateData,
    IWineD3DVolumeImpl_FreePrivateData,
    IWineD3DVolumeImpl_GetContainer,
    IWineD3DVolumeImpl_GetDesc,
    IWineD3DVolumeImpl_LockBox,
    IWineD3DVolumeImpl_UnlockBox,
    IWineD3DVolumeImpl_AddDirtyBox,
    IWineD3DVolumeImpl_CleanDirtyBox
};
