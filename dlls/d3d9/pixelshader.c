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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

/* IDirect3DPixelShader9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DPixelShader9Impl_QueryInterface(LPDIRECT3DPIXELSHADER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;

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
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DPixelShader9Impl_Release(LPDIRECT3DPIXELSHADER9 iface) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DPixelShader9 Interface follow: */
HRESULT WINAPI IDirect3DPixelShader9Impl_GetDevice(LPDIRECT3DPIXELSHADER9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DPixelShader9Impl_GetFunction(LPDIRECT3DPIXELSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}


const IDirect3DPixelShader9Vtbl Direct3DPixelShader9_Vtbl =
{
    IDirect3DPixelShader9Impl_QueryInterface,
    IDirect3DPixelShader9Impl_AddRef,
    IDirect3DPixelShader9Impl_Release,
    IDirect3DPixelShader9Impl_GetDevice,
    IDirect3DPixelShader9Impl_GetFunction
};


/* IDirect3DDevice9 IDirect3DPixelShader9 Methods follow:  */
HRESULT WINAPI IDirect3DDevice9Impl_CreatePixelShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    *ppShader = NULL;
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT Register, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT Register, float* pConstantData, UINT Vector4fCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT Register, CONST int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT Register, int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT Register, CONST BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT Register, BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
