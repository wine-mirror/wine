/*
 * IDirect3DStateBlock9 implementation
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

#include <math.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DStateBlock9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DStateBlock9Impl_QueryInterface(LPDIRECT3DSTATEBLOCK9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DStateBlock9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DStateBlock9)) {
        IDirect3DStateBlock9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DStateBlock9Impl_AddRef(LPDIRECT3DSTATEBLOCK9 iface) {
    ICOM_THIS(IDirect3DStateBlock9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DStateBlock9Impl_Release(LPDIRECT3DSTATEBLOCK9 iface) {
    ICOM_THIS(IDirect3DStateBlock9Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DStateBlock9 Interface follow: */
HRESULT WINAPI IDirect3DStateBlock9Impl_GetDevice(LPDIRECT3DSTATEBLOCK9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DStateBlock9Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE9) This->Device;
    IDirect3DDevice9Impl_AddRef(*ppDevice);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DStateBlock9Impl_Capture(LPDIRECT3DSTATEBLOCK9 iface) {
    ICOM_THIS(IDirect3DStateBlock9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DStateBlock9Impl_Apply(LPDIRECT3DSTATEBLOCK9 iface) {
    ICOM_THIS(IDirect3DStateBlock9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DStateBlock9) Direct3DStateBlock9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DStateBlock9Impl_QueryInterface,
    IDirect3DStateBlock9Impl_AddRef,
    IDirect3DStateBlock9Impl_Release,
    IDirect3DStateBlock9Impl_GetDevice,
    IDirect3DStateBlock9Impl_Capture,
    IDirect3DStateBlock9Impl_Apply
};


/* IDirect3DDevice9 IDirect3DStateBlock9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateStateBlock(LPDIRECT3DDEVICE9 iface, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_BeginStateBlock(LPDIRECT3DDEVICE9 iface) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_EndStateBlock(LPDIRECT3DDEVICE9 iface, IDirect3DStateBlock9** ppSB) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
