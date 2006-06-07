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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DVertexDeclaration9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVertexDeclaration9Impl_QueryInterface(LPDIRECT3DVERTEXDECLARATION9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexDeclaration9)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
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
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexDeclaration9 Interface follow: */
HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDevice(LPDIRECT3DVERTEXDECLARATION9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    IWineD3DDevice *myDevice = NULL;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay\n", iface);

    hr = IWineD3DVertexDeclaration_GetDevice(This->wineD3DVertexDeclaration, &myDevice);
    if (hr == D3D_OK && myDevice != NULL) {
        hr = IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(myDevice);
    }
    return hr;
}

HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDeclaration(LPDIRECT3DVERTEXDECLARATION9 iface, D3DVERTEXELEMENT9* pDecl, UINT* pNumElements) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    DWORD NumElements;
    HRESULT hr;
    TRACE("(%p) : Relay\n", iface);
    hr = IWineD3DVertexDeclaration_GetDeclaration(This->wineD3DVertexDeclaration, pDecl, &NumElements);

    *pNumElements = NumElements;
    return hr;
}

const IDirect3DVertexDeclaration9Vtbl Direct3DVertexDeclaration9_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexDeclaration9Impl_QueryInterface,
    IDirect3DVertexDeclaration9Impl_AddRef,
    IDirect3DVertexDeclaration9Impl_Release,
    /* IDirect3DVertexDeclaration9 */
    IDirect3DVertexDeclaration9Impl_GetDevice,
    IDirect3DVertexDeclaration9Impl_GetDeclaration
};


/* IDirect3DDevice9 IDirect3DVertexDeclaration9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9 iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) {
    
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *object = NULL;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay\n", iface);
    if (NULL == ppDecl) {
        WARN("(%p) : Caller passed NULL As ppDecl, returning D3DERR_INVALIDCALL",This);
        return D3DERR_INVALIDCALL;
    }
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexDeclaration9Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexDeclaration9_Vtbl;
    object->ref = 1;
    hr = IWineD3DDevice_CreateVertexDeclaration(This->WineD3DDevice, pVertexElements, &object->wineD3DVertexDeclaration, (IUnknown *)object);

    if (FAILED(hr)) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateVertexDeclaration failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppDecl = (LPDIRECT3DVERTEXDECLARATION9) object;
         TRACE("(%p) : Created vertex declatanio %p\n", This, object);
    }
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9* pDecl) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *pDeclImpl = (IDirect3DVertexDeclaration9Impl *)pDecl;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay\n", iface);

    hr = IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice, pDeclImpl == NULL ? NULL : pDeclImpl->wineD3DVertexDeclaration);

    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9** ppDecl) {
    IDirect3DDevice9Impl* This = (IDirect3DDevice9Impl*) iface;
    IWineD3DVertexDeclaration* pTest = NULL;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay+\n", iface);

    if (NULL == ppDecl) {
      return D3DERR_INVALIDCALL;
    }

    *ppDecl = NULL;
    hr = IWineD3DDevice_GetVertexDeclaration(This->WineD3DDevice, &pTest);
    if (hr == D3D_OK && NULL != pTest) {
        IWineD3DResource_GetParent(pTest, (IUnknown **)ppDecl);
        IWineD3DResource_Release(pTest);
    } else {
        *ppDecl = NULL;
    }
    TRACE("(%p) : returning %p\n", This, *ppDecl);
    return hr;
}
