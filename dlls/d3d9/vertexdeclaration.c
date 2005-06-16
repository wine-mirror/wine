/*
 * IDirect3DVertexDeclaration9 implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
 *                     Jason Edmeades
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DVertexDeclaration9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVertexDeclaration9Impl_QueryInterface(LPDIRECT3DVERTEXDECLARATION9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexDeclaration9)) {
        IDirect3DVertexDeclaration9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVertexDeclaration9Impl_AddRef(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DVertexDeclaration9Impl_Release(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DVertexDeclaration_Release(This->wineD3DVertexDeclaration);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexDeclaration9 Interface follow: */
HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDevice(LPDIRECT3DVERTEXDECLARATION9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    IWineD3DDevice *myDevice = NULL;
    IWineD3DVertexDeclaration_GetDevice(This->wineD3DVertexDeclaration, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDeclaration(LPDIRECT3DVERTEXDECLARATION9 iface, D3DVERTEXELEMENT9* pDecl, UINT* pNumElements) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    return IWineD3DVertexDeclaration_GetDeclaration(This->wineD3DVertexDeclaration, 9, pDecl, (DWORD*) pNumElements);
}


const IDirect3DVertexDeclaration9Vtbl Direct3DVertexDeclaration9_Vtbl =
{
    IDirect3DVertexDeclaration9Impl_QueryInterface,
    IDirect3DVertexDeclaration9Impl_AddRef,
    IDirect3DVertexDeclaration9Impl_Release,
    IDirect3DVertexDeclaration9Impl_GetDevice,
    IDirect3DVertexDeclaration9Impl_GetDeclaration
};


/* IDirect3DDevice9 IDirect3DVertexDeclaration9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9 iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) {
    
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *object = NULL;
    HRESULT hr = D3D_OK;
    
    if (NULL == ppDecl) {
      return D3DERR_INVALIDCALL;
    }
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexDeclaration9Impl));
    if (NULL == object) {
      FIXME("Allocation of memory failed\n");
      *ppDecl = NULL;
      return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexDeclaration9_Vtbl;
    object->ref = 1;
    hr = IWineD3DDevice_CreateVertexDeclaration(This->WineD3DDevice, pVertexElements, &object->wineD3DVertexDeclaration, (IUnknown *)object);

    if (FAILED(hr)) {
      /* free up object */ 
      FIXME("(%p) call to IWineD3DDevice_CreateVertexDeclaration failed\n", This);
      HeapFree(GetProcessHeap(), 0, object);
      *ppDecl = NULL;
    } else {
      *ppDecl = (LPDIRECT3DVERTEXDECLARATION9) object;
    }
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9* pDecl) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *pDeclImpl = (IDirect3DVertexDeclaration9Impl *)pDecl;
    HRESULT hr = S_OK;
    /* TODO: implement stateblocks */
    FIXME("Disabled\n");
    return D3DERR_INVALIDCALL;
    if (NULL != pDecl) {
      hr = IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice, pDeclImpl->wineD3DVertexDeclaration);
    }
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9** ppDecl) {
    IDirect3DDevice9Impl* This = (IDirect3DDevice9Impl*) iface;
    IWineD3DVertexDeclaration* pTest = NULL;
    HRESULT hr = S_OK;
    FIXME("Disabled\n");
    return D3DERR_INVALIDCALL;

    if (NULL == ppDecl) {
      return D3DERR_INVALIDCALL;
    }
    *ppDecl = NULL;
    hr = IWineD3DDevice_GetVertexDeclaration(This->WineD3DDevice, &pTest);
    if (SUCCEEDED(hr)) {
      *ppDecl = NULL;
    }
    return hr;
}
