/*
 * shaders implementation
 *
 * Copyright 2005      Oliver Stieber
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

#include <math.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);


/* *******************************************
   IWineD3DPixelShader IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DPixelShaderImpl_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DPixelShader)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DPixelShaderImpl_AddRef(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DPixelShaderImpl_Release(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DPixelShader IWineD3DPixelShader parts follow
   ******************************************* */

HRESULT WINAPI IWineD3DPixelShaderImpl_GetParent(IWineD3DPixelShader *iface, IUnknown** parent){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;

    *parent= (IUnknown*) parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return D3D_OK;
}
   
HRESULT WINAPI IWineD3DPixelShaderImpl_GetDevice(IWineD3DPixelShader* iface, IWineD3DDevice **pDevice){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    IWineD3DDevice_AddRef((IWineD3DDevice *)This->wineD3DDevice);
    *pDevice = (IWineD3DDevice *)This->wineD3DDevice;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return D3D_OK;
}


HRESULT WINAPI IWineD3DPixelShaderImpl_GetFunction(IWineD3DPixelShader* impl, VOID* pData, UINT* pSizeOfData) {
  IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)impl;
  FIXME("(%p) : pData(%p), pSizeOfData(%p)\n", This, pData, pSizeOfData);

  if (NULL == pData) {
    *pSizeOfData = This->functionLength;
    return D3D_OK;
  }
  if (*pSizeOfData < This->functionLength) {
    *pSizeOfData = This->functionLength;
    return D3DERR_MOREDATA;
  }
  if (NULL == This->function) { /* no function defined */
    TRACE("(%p) : GetFunction no User Function defined using NULL to %p\n", This, pData);
    (*(DWORD **) pData) = NULL;
  } else {
    if(This->functionLength == 0){

    }
    TRACE("(%p) : GetFunction copying to %p\n", This, pData);
    memcpy(pData, This->function, This->functionLength);
  }
  return D3D_OK;
}



const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DPixelShaderImpl_QueryInterface,
    IWineD3DPixelShaderImpl_AddRef,
    IWineD3DPixelShaderImpl_Release,
    /*** IWineD3DPixelShader methods ***/
    IWineD3DPixelShaderImpl_GetParent,
    IWineD3DPixelShaderImpl_GetDevice,
    IWineD3DPixelShaderImpl_GetFunction
};
