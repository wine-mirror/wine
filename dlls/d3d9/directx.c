/*
 * IDirect3D9 implementation
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3D9 IUnknown parts follow: */
HRESULT WINAPI IDirect3D9Impl_QueryInterface(LPDIRECT3D9 iface, REFIID riid, LPVOID* ppobj)
{
    ICOM_THIS(IDirect3D9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3D9)) {
        IDirect3D9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3D9Impl_AddRef(LPDIRECT3D9 iface) {
    ICOM_THIS(IDirect3D9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3D9Impl_Release(LPDIRECT3D9 iface) {
    ICOM_THIS(IDirect3D9Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

/* IDirect3D9 Interface follow: */
HRESULT  WINAPI  IDirect3D9Impl_RegisterSoftwareDevice(LPDIRECT3D9 iface, void* pInitializeFunction) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, pInitializeFunction);
    return D3D_OK;
}

UINT     WINAPI  IDirect3D9Impl_GetAdapterCount(LPDIRECT3D9 iface) {
    ICOM_THIS(IDirect3D9Impl,iface);
    /* FIXME: Set to one for now to imply the display */
    TRACE("(%p): Mostly stub, only returns primary display\n", This);
    return 1;
}

HRESULT WINAPI IDirect3D9Impl_GetAdapterIdentifier(LPDIRECT3D9 iface, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

UINT WINAPI IDirect3D9Impl_GetAdapterModeCount(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return 0;
}

HRESULT WINAPI IDirect3D9Impl_EnumAdapterModes(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3D9Impl_GetAdapterDisplayMode(LPDIRECT3D9 iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3D9Impl_CheckDeviceType(LPDIRECT3D9 iface,
					      UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
					      D3DFORMAT BackBufferFormat, BOOL Windowed) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceFormat(LPDIRECT3D9 iface,
						  UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						  DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceMultiSampleType(LPDIRECT3D9 iface,
							   UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
							   BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D9Impl_CheckDepthStencilMatch(LPDIRECT3D9 iface, 
						       UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						       D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D9Impl_CheckDeviceFormatConversion(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}


HRESULT  WINAPI  IDirect3D9Impl_GetDeviceCaps(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p): stub\n", This);
    return D3D_OK;
}

HMONITOR WINAPI  IDirect3D9Impl_GetAdapterMonitor(LPDIRECT3D9 iface, UINT Adapter) {
    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return NULL;
}

HRESULT  WINAPI  IDirect3D9Impl_CreateDevice(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
					     DWORD BehaviourFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, 
					     IDirect3DDevice9** ppReturnedDeviceInterface) {

    ICOM_THIS(IDirect3D9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

ICOM_VTABLE(IDirect3D9) Direct3D9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3D9Impl_QueryInterface,
    IDirect3D9Impl_AddRef,
    IDirect3D9Impl_Release,
    IDirect3D9Impl_RegisterSoftwareDevice,
    IDirect3D9Impl_GetAdapterCount,
    IDirect3D9Impl_GetAdapterIdentifier,
    IDirect3D9Impl_GetAdapterModeCount,
    IDirect3D9Impl_EnumAdapterModes,
    IDirect3D9Impl_GetAdapterDisplayMode,
    IDirect3D9Impl_CheckDeviceType,
    IDirect3D9Impl_CheckDeviceFormat,
    IDirect3D9Impl_CheckDeviceMultiSampleType,
    IDirect3D9Impl_CheckDepthStencilMatch,
    IDirect3D9Impl_CheckDeviceFormatConversion,
    IDirect3D9Impl_GetDeviceCaps,
    IDirect3D9Impl_GetAdapterMonitor,
    IDirect3D9Impl_CreateDevice
};
