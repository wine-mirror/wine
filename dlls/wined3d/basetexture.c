/*
 * IWineD3DBaseTexture Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
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
   IWineD3DBaseTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DBaseTextureImpl_QueryInterface(IWineD3DBaseTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    WARN("(%p)->(%s,%p) should not be called\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DBaseTextureImpl_AddRef(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->resource.ref);
    IUnknown_AddRef(This->resource.parent);
    return InterlockedIncrement(&This->resource.ref);
}

ULONG WINAPI IWineD3DBaseTextureImpl_Release(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        IWineD3DDevice_Release((IWineD3DDevice *)This->resource.wineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This);
    } else {
        IUnknown_Release(This->resource.parent);  /* Released the reference to the d3dx object */
    }
    return ref;
}

/* ****************************************************
   IWineD3DBaseTexture IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DBaseTextureImpl_GetDevice(IWineD3DBaseTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResource_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_SetPrivateData(IWineD3DBaseTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResource_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_GetPrivateData(IWineD3DBaseTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResource_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_FreePrivateData(IWineD3DBaseTexture *iface, REFGUID refguid) {
    return IWineD3DResource_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD    WINAPI        IWineD3DBaseTextureImpl_SetPriority(IWineD3DBaseTexture *iface, DWORD PriorityNew) {
    return IWineD3DResource_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD    WINAPI        IWineD3DBaseTextureImpl_GetPriority(IWineD3DBaseTexture *iface) {
    return IWineD3DResource_GetPriority((IWineD3DResource *)iface);
}

void     WINAPI        IWineD3DBaseTextureImpl_PreLoad(IWineD3DBaseTexture *iface) {
    return IWineD3DResource_PreLoad((IWineD3DResource *)iface);
}

D3DRESOURCETYPE WINAPI IWineD3DBaseTextureImpl_GetType(IWineD3DBaseTexture *iface) {
    return IWineD3DResource_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_GetParent(IWineD3DBaseTexture *iface, IUnknown **pParent) {
    return IWineD3DResource_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DBaseTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
DWORD WINAPI IWineD3DBaseTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}

DWORD WINAPI IWineD3DBaseTextureImpl_GetLOD(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}

DWORD WINAPI IWineD3DBaseTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->baseTexture.levels);    
    return This->baseTexture.levels;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface, D3DTEXTUREFILTERTYPE FilterType) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  FIXME("(%p) : stub\n", This);
  return D3D_OK;
}

D3DTEXTUREFILTERTYPE WINAPI IWineD3DBaseTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  FIXME("(%p) : stub\n", This);
  return D3DTEXF_NONE;
}

void WINAPI IWineD3DBaseTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  FIXME("(%p) : stub\n", This);
  return ;
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DBaseTextureImpl_SetDirty(IWineD3DBaseTexture *iface, BOOL dirty) {
    BOOL old;
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    old = This->baseTexture.dirty;
    This->baseTexture.dirty = dirty;
    return old;
}

IWineD3DBaseTextureVtbl IWineD3DBaseTexture_Vtbl =
{
    IWineD3DBaseTextureImpl_QueryInterface,
    IWineD3DBaseTextureImpl_AddRef,
    IWineD3DBaseTextureImpl_Release,
    IWineD3DBaseTextureImpl_GetParent,
    IWineD3DBaseTextureImpl_GetDevice,
    IWineD3DBaseTextureImpl_SetPrivateData,
    IWineD3DBaseTextureImpl_GetPrivateData,
    IWineD3DBaseTextureImpl_FreePrivateData,
    IWineD3DBaseTextureImpl_SetPriority,
    IWineD3DBaseTextureImpl_GetPriority,
    IWineD3DBaseTextureImpl_PreLoad,
    IWineD3DBaseTextureImpl_GetType,
    IWineD3DBaseTextureImpl_SetLOD,
    IWineD3DBaseTextureImpl_GetLOD,
    IWineD3DBaseTextureImpl_GetLevelCount,
    IWineD3DBaseTextureImpl_SetAutoGenFilterType,
    IWineD3DBaseTextureImpl_GetAutoGenFilterType,
    IWineD3DBaseTextureImpl_GenerateMipSubLevels,
    IWineD3DBaseTextureImpl_SetDirty
};
