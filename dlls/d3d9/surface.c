/*
 * IDirect3DSurface9 implementation
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

#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);

/* IDirect3DSurface9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DSurface9Impl_QueryInterface(LPDIRECT3DSURFACE9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DSurface9)) {
        IDirect3DSurface9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DSurface9Impl_AddRef(LPDIRECT3DSURFACE9 iface) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DSurface9Impl_Release(LPDIRECT3DSURFACE9 iface) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
      HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
      HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DSurface9 IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DSurface9Impl_GetDevice(LPDIRECT3DSURFACE9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

HRESULT WINAPI IDirect3DSurface9Impl_SetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface9Impl_GetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface9Impl_FreePrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

DWORD   WINAPI IDirect3DSurface9Impl_SetPriority(LPDIRECT3DSURFACE9 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    return IDirect3DResource9Impl_SetPriority((LPDIRECT3DRESOURCE9) This, PriorityNew);
}

DWORD   WINAPI IDirect3DSurface9Impl_GetPriority(LPDIRECT3DSURFACE9 iface) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    return IDirect3DResource9Impl_GetPriority((LPDIRECT3DRESOURCE9) This);
}

void    WINAPI IDirect3DSurface9Impl_PreLoad(LPDIRECT3DSURFACE9 iface) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DSurface9Impl_GetType(LPDIRECT3DSURFACE9 iface) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    return IDirect3DResource9Impl_GetType((LPDIRECT3DRESOURCE9) This);
}

/* IDirect3DSurface9 Interface follow: */
HRESULT WINAPI IDirect3DSurface9Impl_GetContainer(LPDIRECT3DSURFACE9 iface, REFIID riid, void** ppContainer) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    HRESULT res;
    res = IUnknown_QueryInterface(This->Container, riid, ppContainer);
    if (E_NOINTERFACE == res) { 
      /**
       * If the surface is created using CreateImageSurface, CreateRenderTarget, 
       * or CreateDepthStencilSurface, the surface is considered stand alone. In this case, 
       * GetContainer will return the Direct3D device used to create the surface. 
       */
      res = IUnknown_QueryInterface(This->Container, &IID_IDirect3DDevice9, ppContainer);
    }
    TRACE("(%p) : returning %p\n", This, *ppContainer);
    return res;
}

HRESULT WINAPI IDirect3DSurface9Impl_GetDesc(LPDIRECT3DSURFACE9 iface, D3DSURFACE_DESC* pDesc) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    TRACE("(%p) : copying into %p\n", This, pDesc);
    memcpy(pDesc, &This->myDesc, sizeof(D3DSURFACE_DESC));
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface9Impl_LockRect(LPDIRECT3DSURFACE9 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface9Impl_UnlockRect(LPDIRECT3DSURFACE9 iface) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface9Impl_GetDC(LPDIRECT3DSURFACE9 iface, HDC* phdc) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface9Impl_ReleaseDC(LPDIRECT3DSURFACE9 iface, HDC hdc) {
    ICOM_THIS(IDirect3DSurface9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DSurface9) Direct3DSurface9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DSurface9Impl_QueryInterface,
    IDirect3DSurface9Impl_AddRef,
    IDirect3DSurface9Impl_Release,
    IDirect3DSurface9Impl_GetDevice,
    IDirect3DSurface9Impl_SetPrivateData,
    IDirect3DSurface9Impl_GetPrivateData,
    IDirect3DSurface9Impl_FreePrivateData,
    IDirect3DSurface9Impl_SetPriority,
    IDirect3DSurface9Impl_GetPriority,
    IDirect3DSurface9Impl_PreLoad,
    IDirect3DSurface9Impl_GetType,
    IDirect3DSurface9Impl_GetContainer,
    IDirect3DSurface9Impl_GetDesc,
    IDirect3DSurface9Impl_LockRect,
    IDirect3DSurface9Impl_UnlockRect,
    IDirect3DSurface9Impl_GetDC,
    IDirect3DSurface9Impl_ReleaseDC
};
