/*
 * IDirect3DPixelShader9 implementation
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

/* IDirect3DPixelShader9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DPixelShader9Impl_QueryInterface(LPDIRECT3DPIXELSHADER9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DPixelShader9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DPixelShader9)) {
        IDirect3DPixelShader9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DPixelShader9Impl_AddRef(LPDIRECT3DPIXELSHADER9 iface) {
    ICOM_THIS(IDirect3DPixelShader9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DPixelShader9Impl_Release(LPDIRECT3DPIXELSHADER9 iface) {
    ICOM_THIS(IDirect3DPixelShader9Impl,iface);
    ULONG ref = --This->ref;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DPixelShader9 Interface follow: */
HRESULT WINAPI IDirect3DPixelShader9Impl_GetDevice(LPDIRECT3DPIXELSHADER9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DPixelShader9Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE9) This->Device;
    IDirect3DDevice9Impl_AddRef(*ppDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DPixelShader9Impl_GetFunction(LPDIRECT3DPIXELSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    ICOM_THIS(IDirect3DPixelShader9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DPixelShader9) Direct3DPixelShader9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DPixelShader9Impl_QueryInterface,
    IDirect3DPixelShader9Impl_AddRef,
    IDirect3DPixelShader9Impl_Release,
    IDirect3DPixelShader9Impl_GetDevice,
    IDirect3DPixelShader9Impl_GetFunction
};


/* IDirect3DDevice9 IDirect3DPixelShader9 Methods follow:  */
HRESULT WINAPI IDirect3DDevice9Impl_CreatePixelShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);

    This->UpdateStateBlock->PixelShader = pShader;
    This->UpdateStateBlock->Changed.pixelShader = TRUE;
    This->UpdateStateBlock->Set.pixelShader = TRUE;
    
    /* Handle recording of state blocks */
    if (This->isRecordingState) {
      TRACE("Recording... not performing anything\n");
      return D3D_OK;
    }
    /**
     * TODO: merge HAL shaders context switching from prototype
     */
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9** ppShader) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    TRACE("(%p) : GetPixelShader returning %p\n", This, This->StateBlock->PixelShader);
    *ppShader = This->StateBlock->PixelShader;
    IDirect3DPixelShader9Impl_AddRef(*ppShader);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT Register, CONST float* pConstantData, UINT Vector4fCount) {
  ICOM_THIS(IDirect3DDevice9Impl,iface);

  if (Register + Vector4fCount > D3D_VSHADER_MAX_CONSTANTS) {
    ERR("(%p) : SetPixelShaderConstant C[%u] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (Vector4fCount > 1) {
    CONST FLOAT* f = pConstantData;
    UINT i;
    TRACE("(%p) : SetPixelShaderConstant C[%u..%u]=\n", This, Register, Register + Vector4fCount - 1);
    for (i = 0; i < Vector4fCount; ++i) {
      TRACE("{%f, %f, %f, %f}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    FLOAT* f = (FLOAT*) pConstantData;
    TRACE("(%p) : SetPixelShaderConstant, C[%u]={%f, %f, %f, %f}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.pixelShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->pixelShaderConstantF[Register], pConstantData, Vector4fCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT Register, float* pConstantData, UINT Vector4fCount) {
  ICOM_THIS(IDirect3DDevice9Impl,iface);

  TRACE("(%p) : C[%u] count=%u\n", This, Register, Vector4fCount);
  if (Register + Vector4fCount > D3D_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->UpdateStateBlock->pixelShaderConstantF[Register], Vector4fCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT Register, CONST int* pConstantData, UINT Vector4iCount) {
  ICOM_THIS(IDirect3DDevice9Impl,iface);

  if (Register + Vector4iCount > D3D_VSHADER_MAX_CONSTANTS) {
    ERR("(%p) : SetPixelShaderConstantI C[%u] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (Vector4iCount > 1) {
    CONST int* f = pConstantData;
    UINT i;
    TRACE("(%p) : SetPixelShaderConstantI C[%u..%u]=\n", This, Register, Register + Vector4iCount - 1);
    for (i = 0; i < Vector4iCount; ++i) {
      TRACE("{%d, %d, %d, %d}\n", f[0], f[1], f[2], f[3]);
      f += 4;
    }
  } else { 
    CONST int* f = pConstantData;
    TRACE("(%p) : SetPixelShaderConstantI, C[%u]={%i, %i, %i, %i}\n", This, Register, f[0], f[1], f[2], f[3]);
  }
  This->UpdateStateBlock->Changed.pixelShaderConstant = TRUE;
  memcpy(&This->UpdateStateBlock->pixelShaderConstantI[Register], pConstantData, Vector4iCount * 4 * sizeof(int));
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT Register, int* pConstantData, UINT Vector4iCount) {
  ICOM_THIS(IDirect3DDevice9Impl,iface);

  TRACE("(%p) : C[%u] count=%u\n", This, Register, Vector4iCount);
  if (Register + Vector4iCount > D3D_VSHADER_MAX_CONSTANTS) {
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  memcpy(pConstantData, &This->UpdateStateBlock->pixelShaderConstantI[Register], Vector4iCount * 4 * sizeof(FLOAT));
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT Register, CONST BOOL* pConstantData, UINT BoolCount) {
  ICOM_THIS(IDirect3DDevice9Impl,iface);
  UINT i;

  if (Register + BoolCount > D3D_VSHADER_MAX_CONSTANTS) {
    ERR("(%p) : SetPixelShaderConstantB C[%u] invalid\n", This, Register);
    return D3DERR_INVALIDCALL;
  }
  if (NULL == pConstantData) {
    return D3DERR_INVALIDCALL;
  }
  if (BoolCount > 1) {
    CONST BOOL* f = pConstantData;
    TRACE("(%p) : SetPixelShaderConstantB C[%u..%u]=\n", This, Register, Register + BoolCount - 1);
    for (i = 0; i < BoolCount; ++i) {
      TRACE("{%u}\n", f[i]);
    }
  } else { 
    CONST BOOL* f = pConstantData;
    TRACE("(%p) : SetPixelShaderConstantB, C[%u]={%u}\n", This, Register, f[0]);
  }
  This->UpdateStateBlock->Changed.pixelShaderConstant = TRUE;
  for (i = 0; i < BoolCount; ++i) {
    This->UpdateStateBlock->pixelShaderConstantB[Register] = pConstantData[i];
  }
  return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT Register, BOOL* pConstantData, UINT BoolCount) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
