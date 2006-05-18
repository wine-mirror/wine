/*
 * Copyright 2000 Marcus Meissner
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

#include <assert.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);


HRESULT WINAPI
Main_IDirect3DImpl_1_Initialize(LPDIRECT3D iface,
                                REFIID riid)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D, iface);
    TRACE("(%p/%p)->(%s) no-op...\n", This, iface, debugstr_guid(riid));
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_EnumDevices(LPDIRECT3D3 iface,
                                       LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                       LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpEnumDevicesCallback, lpUserArg);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateLight(LPDIRECT3D3 iface,
                                       LPDIRECT3DLIGHT* lplpDirect3DLight,
                                       IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lplpDirect3DLight, pUnkOuter);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateMaterial(LPDIRECT3D3 iface,
					  LPDIRECT3DMATERIAL3* lplpDirect3DMaterial3,
					  IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lplpDirect3DMaterial3, pUnkOuter);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateViewport(LPDIRECT3D3 iface,
					  LPDIRECT3DVIEWPORT3* lplpD3DViewport3,
					  IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lplpD3DViewport3, pUnkOuter);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_FindDevice(LPDIRECT3D3 iface,
				      LPD3DFINDDEVICESEARCH lpD3DDFS,
				      LPD3DFINDDEVICERESULT lpD3DFDR)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpD3DDFS, lpD3DFDR);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_2_CreateDevice(LPDIRECT3D2 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE lpDDS,
                                  LPDIRECT3DDEVICE2* lplpD3DDevice2)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D2, iface);
    FIXME("(%p/%p)->(%s,%p,%p): stub!\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice2);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_3_CreateDevice(LPDIRECT3D3 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE4 lpDDS,
                                  LPDIRECT3DDEVICE3* lplpD3DDevice3,
                                  LPUNKNOWN lpUnk)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    FIXME("(%p/%p)->(%s,%p,%p,%p): stub!\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice3, lpUnk);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_EnumZBufferFormats(LPDIRECT3D7 iface,
                                           REFCLSID riidDevice,
                                           LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                           LPVOID lpContext)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    FIXME("(%p/%p)->(%s,%p,%p): stub!\n", This, iface, debugstr_guid(riidDevice), lpEnumCallback, lpContext);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_EvictManagedTextures(LPDIRECT3D7 iface)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    FIXME("(%p/%p)->(): stub!\n", This, iface);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_7_EnumDevices(LPDIRECT3D7 iface,
                                 LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback,
                                 LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpEnumDevicesCallback, lpUserArg);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_7_CreateDevice(LPDIRECT3D7 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE7 lpDDS,
                                  LPDIRECT3DDEVICE7* lplpD3DDevice)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    FIXME("(%p/%p)->(%s,%p,%p): stub!\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice);
    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_CreateVertexBuffer(LPDIRECT3D7 iface,
					   LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					   LPDIRECT3DVERTEXBUFFER7* lplpD3DVertBuf,
					   DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    FIXME("(%p/%p)->(%p,%p,%08lx): stub!\n", This, iface, lpD3DVertBufDesc, lplpD3DVertBuf, dwFlags);
    return D3D_OK;
}

HRESULT WINAPI
Thunk_IDirect3DImpl_7_QueryInterface(LPDIRECT3D7 iface,
                                     REFIID riid,
                                     LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirectDraw7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirectDraw7_QueryInterface(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirectDraw7, iface),
				       riid,
				       obp);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_3_QueryInterface(LPDIRECT3D3 iface,
                                     REFIID riid,
                                     LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirectDraw7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirectDraw7_QueryInterface(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D3, IDirectDraw7, iface),
				       riid,
				       obp);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_2_QueryInterface(LPDIRECT3D2 iface,
                                     REFIID riid,
                                     LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirectDraw7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirectDraw7_QueryInterface(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirectDraw7, iface),
				       riid,
				       obp);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_1_QueryInterface(LPDIRECT3D iface,
                                     REFIID riid,
                                     LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirectDraw7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirectDraw7_QueryInterface(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirectDraw7, iface),
				       riid,
				       obp);
}

ULONG WINAPI
Thunk_IDirect3DImpl_7_AddRef(LPDIRECT3D7 iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_AddRef(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_3_AddRef(LPDIRECT3D3 iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_AddRef(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D3, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_2_AddRef(LPDIRECT3D2 iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_AddRef(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_1_AddRef(LPDIRECT3D iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_AddRef(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_7_Release(LPDIRECT3D7 iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_Release(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_3_Release(LPDIRECT3D3 iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_Release(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D3, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_2_Release(LPDIRECT3D2 iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_Release(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirectDraw7, iface));
}

ULONG WINAPI
Thunk_IDirect3DImpl_1_Release(LPDIRECT3D iface)
{
    TRACE("(%p)->() thunking to IDirectDraw7 interface.\n", iface);
    return IDirectDraw7_Release(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirectDraw7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DImpl_3_EnumZBufferFormats(LPDIRECT3D3 iface,
                                         REFCLSID riidDevice,
                                         LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                         LPVOID lpContext)
{
    TRACE("(%p)->(%s,%p,%p) thunking to IDirect3D7 interface.\n", iface, debugstr_guid(riidDevice), lpEnumCallback, lpContext);
    return IDirect3D7_EnumZBufferFormats(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D3, IDirect3D7, iface),
                                         riidDevice,
                                         lpEnumCallback,
                                         lpContext);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_3_EvictManagedTextures(LPDIRECT3D3 iface)
{
    TRACE("(%p)->() thunking to IDirect3D7 interface.\n", iface);
    return IDirect3D7_EvictManagedTextures(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D3, IDirect3D7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DImpl_2_EnumDevices(LPDIRECT3D2 iface,
                                  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                  LPVOID lpUserArg)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lpEnumDevicesCallback, lpUserArg);
    return IDirect3D3_EnumDevices(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirect3D3, iface),
                                  lpEnumDevicesCallback,
                                  lpUserArg);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateLight(LPDIRECT3D2 iface,
                                  LPDIRECT3DLIGHT* lplpDirect3DLight,
                                  IUnknown* pUnkOuter)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lplpDirect3DLight, pUnkOuter);
    return IDirect3D3_CreateLight(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirect3D3, iface),
                                  lplpDirect3DLight,
                                  pUnkOuter);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateLight(LPDIRECT3D iface,
                                  LPDIRECT3DLIGHT* lplpDirect3DLight,
                                  IUnknown* pUnkOuter)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lplpDirect3DLight, pUnkOuter);
    return IDirect3D3_CreateLight(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirect3D3, iface),
                                  lplpDirect3DLight,
                                  pUnkOuter);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateMaterial(LPDIRECT3D iface,
				     LPDIRECT3DMATERIAL* lplpDirect3DMaterial,
				     IUnknown* pUnkOuter)
{
    HRESULT ret;
    LPDIRECT3DMATERIAL3 ret_val;

    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lplpDirect3DMaterial, pUnkOuter);
    ret = IDirect3D3_CreateMaterial(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirect3D3, iface),
				    &ret_val,
				    pUnkOuter);

    *lplpDirect3DMaterial = COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial3, IDirect3DMaterial, ret_val);

    TRACE(" returning interface %p.\n", *lplpDirect3DMaterial);
    
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateViewport(LPDIRECT3D iface,
				     LPDIRECT3DVIEWPORT* lplpD3DViewport,
				     IUnknown* pUnkOuter)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lplpD3DViewport, pUnkOuter);
    return IDirect3D3_CreateViewport(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirect3D3, iface),
				    (LPDIRECT3DVIEWPORT3 *) lplpD3DViewport /* No need to cast here */,
				    pUnkOuter);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateMaterial(LPDIRECT3D2 iface,
				     LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2,
				     IUnknown* pUnkOuter)
{
    HRESULT ret;
    LPDIRECT3DMATERIAL3 ret_val;

    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lplpDirect3DMaterial2, pUnkOuter);
    ret = IDirect3D3_CreateMaterial(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirect3D3, iface),
				    &ret_val,
				    pUnkOuter);

    *lplpDirect3DMaterial2 = COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial3, IDirect3DMaterial2, ret_val);

    TRACE(" returning interface %p.\n", *lplpDirect3DMaterial2);
    
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateViewport(LPDIRECT3D2 iface,
				     LPDIRECT3DVIEWPORT2* lplpD3DViewport2,
				     IUnknown* pUnkOuter)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lplpD3DViewport2, pUnkOuter);
    return IDirect3D3_CreateViewport(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirect3D3, iface),
				     (LPDIRECT3DVIEWPORT3 *) lplpD3DViewport2 /* No need to cast here */,
				     pUnkOuter);
}


HRESULT WINAPI
Thunk_IDirect3DImpl_3_CreateVertexBuffer(LPDIRECT3D3 iface,
					 LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					 LPDIRECT3DVERTEXBUFFER* lplpD3DVertBuf,
					 DWORD dwFlags,
					 LPUNKNOWN lpUnk)
{
    HRESULT ret;
    LPDIRECT3DVERTEXBUFFER7 ret_val;

    TRACE("(%p)->(%p,%p,%08lx,%p) thunking to IDirect3D7 interface.\n", iface, lpD3DVertBufDesc, lplpD3DVertBuf, dwFlags, lpUnk);
    
    /* dwFlags is not used in the D3D7 interface, use the vertex buffer description instead */
    if (dwFlags & D3DDP_DONOTCLIP) lpD3DVertBufDesc->dwCaps |= D3DVBCAPS_DONOTCLIP;

    ret = IDirect3D7_CreateVertexBuffer(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D3, IDirect3D7, iface),
					lpD3DVertBufDesc,
					&ret_val,
					dwFlags);

    *lplpD3DVertBuf = COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, IDirect3DVertexBuffer, ret_val);

    TRACE(" returning interface %p.\n", *lplpD3DVertBuf);
    
    return ret;
}

HRESULT WINAPI
Thunk_IDirect3DImpl_1_FindDevice(LPDIRECT3D iface,
				 LPD3DFINDDEVICESEARCH lpD3DDFS,
				 LPD3DFINDDEVICERESULT lplpD3DDevice)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lpD3DDFS, lplpD3DDevice);
    return IDirect3D3_FindDevice(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D, IDirect3D3, iface),
				 lpD3DDFS,
				 lplpD3DDevice);
}

HRESULT WINAPI
Thunk_IDirect3DImpl_2_FindDevice(LPDIRECT3D2 iface,
				 LPD3DFINDDEVICESEARCH lpD3DDFS,
				 LPD3DFINDDEVICERESULT lpD3DFDR)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, lpD3DDFS, lpD3DFDR);
    return IDirect3D3_FindDevice(COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D2, IDirect3D3, iface),
				 lpD3DDFS,
				 lpD3DFDR);
}
