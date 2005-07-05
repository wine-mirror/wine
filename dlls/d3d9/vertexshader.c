/*
 * IDirect3DVertexShader9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

/* IDirect3DVertexShader9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVertexShader9Impl_QueryInterface(LPDIRECT3DVERTEXSHADER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexShader9)) {
        IDirect3DVertexShader9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVertexShader9Impl_AddRef(LPDIRECT3DVERTEXSHADER9 iface) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DVertexShader9Impl_Release(LPDIRECT3DVERTEXSHADER9 iface) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexShader9 Interface follow: */
HRESULT WINAPI IDirect3DVertexShader9Impl_GetDevice(LPDIRECT3DVERTEXSHADER9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE9) This->Device;
    IDirect3DDevice9Impl_AddRef(*ppDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexShader9Impl_GetFunction(LPDIRECT3DVERTEXSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}


const IDirect3DVertexShader9Vtbl Direct3DVertexShader9_Vtbl =
{
    IDirect3DVertexShader9Impl_QueryInterface,
    IDirect3DVertexShader9Impl_AddRef,
    IDirect3DVertexShader9Impl_Release,
    IDirect3DVertexShader9Impl_GetDevice,
    IDirect3DVertexShader9Impl_GetFunction
};


/* IDirect3DDevice9 IDirect3DVertexShader9 Methods follow: */
HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    /* TODO: implement stateblocks */
    
    FIXME("Disabled %p\n", This);
    /* Handle recording of state blocks */
    /**
     * TODO: merge HAL shaders context switching from prototype
     */
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    TRACE("(%p) : GetVertexShader returning\n", This);
    *ppShader = NULL; 
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT Register, CONST float* pConstantData, UINT Vector4fCount) {
  IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
#if 0
  if (Register + Vector4fCount > D3D_VSHADER_MAX_CONSTANTS) {
    ERR("(%p) : SetVertexShaderConstant C[%u] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (Vector4fCount > 1) {
    CONST FLOAT* f = pConstantData;
    UINT i;
    TRACE("(%p) : SetVertexShaderConstant C[%u..%u]=\n", This, Register, Register + Vector4fCount - 1);
    for (i = 0; i < Vector4fCount; ++i) {
      TRACE("{%f, %f, %f, %f}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    const FLOAT* f = (const FLOAT*) pConstantData;
    TRACE("(%p) : SetVertexShaderConstant, C[%u]={%f, %f, %f, %f}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.vertexShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->vertexShaderConstantF[Register], pConstantData, Vector4fCount * 4 * sizeof(FLOAT));
#endif
  FIXME("(%p) : stub\n", This);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT Register, float* pConstantData, UINT Vector4fCount) {
  IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
#if 0
  TRACE("(%p) : C[%u] count=%u\n", This, Register, Vector4fCount);
  if (Register + Vector4fCount > D3D_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->UpdateStateBlock->vertexShaderConstantF[Register], Vector4fCount * 4 * sizeof(FLOAT));
#endif
  FIXME("(%p) : stub\n", This);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT Register, CONST int* pConstantData, UINT Vector4iCount) {
  IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
#if 0
  if (Register + Vector4iCount > D3D_VSHADER_MAX_CONSTANTS) {
    ERR("(%p) : SetVertexShaderConstantI C[%u] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (Vector4iCount > 1) {
    CONST int* f = pConstantData;
    UINT i;
    TRACE("(%p) : SetVertexShaderConstantI C[%u..%u]=\n", This, Register, Register + Vector4iCount - 1);
    for (i = 0; i < Vector4iCount; ++i) {
      TRACE("{%d, %d, %d, %d}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    CONST int* f = pConstantData;
    TRACE("(%p) : SetVertexShaderConstantI, C[%u]={%i, %i, %i, %i}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.vertexShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->vertexShaderConstantI[Register], pConstantData, Vector4iCount * 4 * sizeof(int));
#endif
  FIXME("(%p) : stub\n", This);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT Register, int* pConstantData, UINT Vector4iCount) {
  IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
#if 0
  TRACE("(%p) : C[%u] count=%u\n", This, Register, Vector4iCount);
  if (Register + Vector4iCount > D3D_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->UpdateStateBlock->vertexShaderConstantI[Register], Vector4iCount * 4 * sizeof(FLOAT));
#endif
  FIXME("(%p) : stub\n", This);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT Register, CONST BOOL* pConstantData, UINT BoolCount) {
  IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
#if 0
  UINT i;

  if (Register + BoolCount > D3D_VSHADER_MAX_CONSTANTS) {
    ERR("(%p) : SetVertexShaderConstantB C[%u] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (BoolCount > 1) {
    CONST BOOL* f = pConstantData;
    TRACE("(%p) : SetVertexShaderConstantB C[%u..%u]=\n", This, Register, Register + BoolCount - 1);
    for (i = 0; i < BoolCount; ++i) {
      TRACE("{%u}\n", f[i]);
    }
  } else { 
    CONST BOOL* f = pConstantData;
    TRACE("(%p) : SetVertexShaderConstantB, C[%u]={%u}\n", This, Register, f[0]);
  }
  This->UpdateStateBlock->Changed.vertexShaderConstant = TRUE;
  for (i = 0; i < BoolCount; ++i) {
    This->UpdateStateBlock->vertexShaderConstantB[Register] = pConstantData[i];
  }
#endif
  FIXME("(%p) : stub\n", This);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT Register, BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
