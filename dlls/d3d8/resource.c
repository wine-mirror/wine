/*
 * IDirect3DResource8 implementation
 *
 * Copyright 2002 Jason Edmeades
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DResource IUnknown parts follow: */
HRESULT WINAPI IDirect3DResource8Impl_QueryInterface(LPDIRECT3DRESOURCE8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)) {
        IDirect3DResource8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DResource8Impl_AddRef(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DResource8Impl_Release(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

/* IDirect3DResource Interface follow: */
HRESULT  WINAPI        IDirect3DResource8Impl_GetDevice(LPDIRECT3DRESOURCE8 iface, IDirect3DDevice8** ppDevice) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DResource8Impl_SetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DResource8Impl_GetPrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT  WINAPI        IDirect3DResource8Impl_FreePrivateData(LPDIRECT3DRESOURCE8 iface, REFGUID refguid) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
DWORD    WINAPI        IDirect3DResource8Impl_SetPriority(LPDIRECT3DRESOURCE8 iface, DWORD PriorityNew) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IDirect3DResource8Impl_GetPriority(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}
void     WINAPI        IDirect3DResource8Impl_PreLoad(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    FIXME("(%p) : stub\n", This);
}
D3DRESOURCETYPE WINAPI IDirect3DResource8Impl_GetType(LPDIRECT3DRESOURCE8 iface) {
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;
    TRACE("(%p) : returning %d\n", This, This->ResourceType);
    return This->ResourceType;
}

D3DPOOL WINAPI IDirect3DResource8Impl_GetPool(LPDIRECT3DRESOURCE8 iface) {    
    IDirect3DResource8Impl *This = (IDirect3DResource8Impl *)iface;

    switch (This->ResourceType) { 
    case D3DRTYPE_SURFACE:
      return D3D8_SURFACE(((IDirect3DSurface8Impl*) This))->resource.pool;
    case D3DRTYPE_TEXTURE:
      return D3D8_SURFACE(((IDirect3DTexture8Impl*) This)->surfaces[0])->resource.pool;
    case D3DRTYPE_VOLUME:
      return ((IDirect3DVolume8Impl*) This)->myDesc.Pool;
    case D3DRTYPE_VOLUMETEXTURE:
      return ((IDirect3DVolumeTexture8Impl*) This)->volumes[0]->myDesc.Pool;
    case D3DRTYPE_CUBETEXTURE:
      return D3D8_SURFACE(((IDirect3DCubeTexture8Impl*) This)->surfaces[0][0])->resource.pool;
    case D3DRTYPE_VERTEXBUFFER:
      return ((IDirect3DVertexBuffer8Impl*) This)->currentDesc.Pool;
    case D3DRTYPE_INDEXBUFFER:
      return ((IDirect3DIndexBuffer8Impl*) This)->currentDesc.Pool;
    default:
      FIXME("(%p) Unrecognized type(%d,%s)\n", This, This->ResourceType, debug_d3dressourcetype(This->ResourceType));
      return D3DPOOL_DEFAULT;
    }
}

const IDirect3DResource8Vtbl Direct3DResource8_Vtbl =
{
    IDirect3DResource8Impl_QueryInterface,
    IDirect3DResource8Impl_AddRef,
    IDirect3DResource8Impl_Release,
    IDirect3DResource8Impl_GetDevice,
    IDirect3DResource8Impl_SetPrivateData,
    IDirect3DResource8Impl_GetPrivateData,
    IDirect3DResource8Impl_FreePrivateData,
    IDirect3DResource8Impl_SetPriority,
    IDirect3DResource8Impl_GetPriority,
    IDirect3DResource8Impl_PreLoad,
    IDirect3DResource8Impl_GetType
};
