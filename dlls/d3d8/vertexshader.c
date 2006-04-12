/*
 * IDirect3DVertexShader8 implementation
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DVertexShader8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DVertexShader8Impl_QueryInterface(IDirect3DVertexShader8 *iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexShader8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DVertexShader8Impl_AddRef(IDirect3DVertexShader8 *iface) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DVertexShader8Impl_Release(IDirect3DVertexShader8 *iface) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DVertexShader_Release(This->wineD3DVertexShader);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexShader8 Interface follow: */
HRESULT WINAPI IDirect3DVertexShader8Impl_GetDevice(IDirect3DVertexShader8 *iface, IDirect3DDevice8** ppDevice) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    IWineD3DDevice *myDevice = NULL;
    HRESULT hr = D3D_OK;
    TRACE("(%p) : Relay\n", This);

    if (D3D_OK == (hr = IWineD3DVertexShader_GetDevice(This->wineD3DVertexShader, &myDevice) && myDevice != NULL)) {
        hr = IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(myDevice);
    } else {
        *ppDevice = NULL;
    }
    TRACE("(%p) returing (%p)", This, *ppDevice);
    return hr;
}

HRESULT WINAPI IDirect3DVertexShader8Impl_GetFunction(IDirect3DVertexShader8 *iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;

    TRACE("(%p) : Relay\n", This);
    return IWineD3DVertexShader_GetFunction(This->wineD3DVertexShader, pData, pSizeOfData);
}


const IDirect3DVertexShader8Vtbl Direct3DVertexShader8_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexShader8Impl_QueryInterface,
    IDirect3DVertexShader8Impl_AddRef,
    IDirect3DVertexShader8Impl_Release,
    /* IDirect3DVertexShader8 */
    IDirect3DVertexShader8Impl_GetDevice,
    IDirect3DVertexShader8Impl_GetFunction
};
