/*
 * vertex declaration implementation
 *
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2004 Christian Costa
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d_decl);

/**
 * DirectX9 SDK download
 *  http://msdn.microsoft.com/library/default.asp?url=/downloads/list/directx.asp
 *
 * Exploring D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx07162002.asp
 *
 * Using Vertex Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx02192001.asp
 *
 * Dx9 New
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/whatsnew.asp
 *
 * Dx9 Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/VertexShader2_0.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/Instructions/Instructions.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexDeclaration/VertexDeclaration.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader3_0/VertexShader3_0.asp
 *
 * Dx9 D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/VertexPipe/matrixstack/matrixstack.asp
 *
 * FVF
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexFormats/vformats.asp
 *
 * NVIDIA: DX8 Vertex Shader to NV Vertex Program
 *  http://developer.nvidia.com/view.asp?IO=vstovp
 *
 * NVIDIA: Memory Management with VAR
 *  http://developer.nvidia.com/view.asp?IO=var_memory_management 
 */

/** Vertex Shader Declaration 8 data types tokens */
#if 0
#define MAX_VSHADER_DECL_TYPES 8
static CONST char* VertexShader8_DeclDataTypes[] = {
  "D3DVSDT_FLOAT1",
  "D3DVSDT_FLOAT2",
  "D3DVSDT_FLOAT3",
  "D3DVSDT_FLOAT4",
  "D3DVSDT_D3DCOLOR",
  "D3DVSDT_UBYTE4",
  "D3DVSDT_SHORT2",
  "D3DVSDT_SHORT4",
  NULL
};
#endif

/* *******************************************
   IWineD3DVertexDeclaration IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DVertexDeclarationImpl_QueryInterface(IWineD3DVertexDeclaration *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    WARN("(%p)->(%s,%p) should not be called\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DVertexDeclarationImpl_AddRef(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DVertexDeclarationImpl_Release(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
      HeapFree(GetProcessHeap(), 0, This->pDeclaration8);
      HeapFree(GetProcessHeap(), 0, This->pDeclaration9);
      HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DVertexDeclaration parts follow
   ******************************************* */

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDevice(IWineD3DVertexDeclaration *iface, IWineD3DDevice** ppDevice) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->wineD3DDevice);
    
    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration8(IWineD3DVertexDeclaration* iface, DWORD* pData, DWORD* pSizeOfData) {
  IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
  if (NULL == pData) {
    *pSizeOfData = This->declaration8Length;
    return D3D_OK;
  }
  if (*pSizeOfData < This->declaration8Length) {
    *pSizeOfData = This->declaration8Length;
    return D3DERR_MOREDATA;
  }
  TRACE("(%p) : GetVertexDeclaration8 copying to %p\n", This, pData);
  memcpy(pData, This->pDeclaration8, This->declaration8Length);
  return D3D_OK;
}

HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDeclaration9(IWineD3DVertexDeclaration* iface,  D3DVERTEXELEMENT9* pData, UINT* pNumElements) {
  IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
  if (NULL == pData) {
    *pNumElements = This->declaration9NumElements;
    return D3D_OK;
  }
  if (*pNumElements < This->declaration9NumElements) {
    *pNumElements = This->declaration9NumElements;
    return D3DERR_MOREDATA;
  }
  TRACE("(%p) : GetVertexDeclaration9 copying to %p\n", This, pData);
  memcpy(pData, This->pDeclaration9, This->declaration9NumElements);
  return D3D_OK;
}

IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl =
{
    IWineD3DVertexDeclarationImpl_QueryInterface,
    IWineD3DVertexDeclarationImpl_AddRef,
    IWineD3DVertexDeclarationImpl_Release,
    IWineD3DVertexDeclarationImpl_GetDevice,
    IWineD3DVertexDeclarationImpl_GetDeclaration8,
    IWineD3DVertexDeclarationImpl_GetDeclaration9
};
