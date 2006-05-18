/*
 * Copyright 2000 Marcus Meissner
 * Copyright 2000 Peter Hunnisett
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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "d3d.h"
#include "ddraw.h"
#include "winerror.h"

#include "ddraw_private.h"
#include "d3d_private.h"
#include "opengl_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

HRESULT WINAPI
GL_IDirect3DImpl_1_EnumDevices(LPDIRECT3D iface,
			       LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
			       LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpEnumDevicesCallback, lpUserArg);

    /* Call functions defined in d3ddevices.c */
    if (d3ddevice_enumerate(lpEnumDevicesCallback, lpUserArg, 1) != D3DENUMRET_OK)
	return D3D_OK;

    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DImpl_2_EnumDevices(LPDIRECT3D2 iface,
				  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
				  LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D2, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpEnumDevicesCallback, lpUserArg);

    /* Call functions defined in d3ddevices.c */
    if (d3ddevice_enumerate(lpEnumDevicesCallback, lpUserArg, 2) != D3DENUMRET_OK)
	return D3D_OK;

    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DImpl_3_EnumDevices(LPDIRECT3D3 iface,
				  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
				  LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpEnumDevicesCallback, lpUserArg);

    /* Call functions defined in d3ddevices.c */
    if (d3ddevice_enumerate(lpEnumDevicesCallback, lpUserArg, 3) != D3DENUMRET_OK)
	return D3D_OK;

    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_CreateLight(LPDIRECT3D3 iface,
				     LPDIRECT3DLIGHT* lplpDirect3DLight,
				     IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    IDirect3DLightImpl *d3dlimpl;
    HRESULT ret_value;
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lplpDirect3DLight, pUnkOuter);

    ret_value = d3dlight_create(&d3dlimpl, This);
    *lplpDirect3DLight = ICOM_INTERFACE(d3dlimpl, IDirect3DLight);

    return ret_value;
}

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_CreateMaterial(LPDIRECT3D3 iface,
					LPDIRECT3DMATERIAL3* lplpDirect3DMaterial3,
					IUnknown* pUnkOuter)
{
    IDirect3DMaterialImpl *D3Dmat_impl;
    HRESULT ret_value;
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lplpDirect3DMaterial3, pUnkOuter);
    ret_value = d3dmaterial_create(&D3Dmat_impl, This);

    *lplpDirect3DMaterial3 = ICOM_INTERFACE(D3Dmat_impl, IDirect3DMaterial3);

    return ret_value;
}

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_CreateViewport(LPDIRECT3D3 iface,
					LPDIRECT3DVIEWPORT3* lplpD3DViewport3,
					IUnknown* pUnkOuter)
{
    IDirect3DViewportImpl *D3Dvp_impl;
    HRESULT ret_value;
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lplpD3DViewport3, pUnkOuter);
    ret_value = d3dviewport_create(&D3Dvp_impl, This);

    *lplpD3DViewport3 = ICOM_INTERFACE(D3Dvp_impl, IDirect3DViewport3);

    return ret_value;
}

static HRESULT
create_device_helper(IDirectDrawImpl *This,
		     REFCLSID iid,
		     IDirectDrawSurfaceImpl *lpDDS,
		     void **obj,
		     int version) {
    IDirect3DDeviceImpl *lpd3ddev;
    HRESULT ret_value;

    ret_value = d3ddevice_create(&lpd3ddev, This, lpDDS, version);
    if (FAILED(ret_value)) return ret_value;
    
    if ((iid == NULL) ||
	(IsEqualGUID(&IID_D3DDEVICE_OpenGL, iid)) ||
	(IsEqualGUID(&IID_IDirect3DHALDevice, iid)) ||
	(IsEqualGUID(&IID_IDirect3DTnLHalDevice, iid)) ||
	(IsEqualGUID(&IID_IDirect3DRGBDevice, iid)) ||
	(IsEqualGUID(&IID_IDirect3DRefDevice, iid))) {
        switch (version) {
	    case 1:
		*obj = ICOM_INTERFACE(lpd3ddev, IDirect3DDevice);
	        TRACE(" returning OpenGL D3DDevice %p.\n", *obj);
		return D3D_OK;

	    case 2:
		*obj = ICOM_INTERFACE(lpd3ddev, IDirect3DDevice2);
	        TRACE(" returning OpenGL D3DDevice2 %p.\n", *obj);
		return D3D_OK;

	    case 3:
		*obj = ICOM_INTERFACE(lpd3ddev, IDirect3DDevice3);
	        TRACE(" returning OpenGL D3DDevice3 %p.\n", *obj);
		return D3D_OK;

	    case 7:
		*obj = ICOM_INTERFACE(lpd3ddev, IDirect3DDevice7);
	        TRACE(" returning OpenGL D3DDevice7 %p.\n", *obj);
		return D3D_OK;
        }
    }

    *obj = NULL;
    ERR(" Interface unknown when creating D3DDevice (%s)\n", debugstr_guid(iid));
    IDirect3DDevice7_Release(ICOM_INTERFACE(lpd3ddev, IDirect3DDevice7));
    return DDERR_INVALIDPARAMS;
}
     

HRESULT WINAPI
GL_IDirect3DImpl_2_CreateDevice(LPDIRECT3D2 iface,
				REFCLSID rclsid,
				LPDIRECTDRAWSURFACE lpDDS,
				LPDIRECT3DDEVICE2* lplpD3DDevice2)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D2, iface);
    IDirectDrawSurfaceImpl *ddsurfaceimpl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface3, lpDDS);
    TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice2);
    return create_device_helper(This, rclsid, ddsurfaceimpl, (void **) lplpD3DDevice2, 2);
}

HRESULT WINAPI
GL_IDirect3DImpl_3_CreateDevice(LPDIRECT3D3 iface,
				REFCLSID rclsid,
				LPDIRECTDRAWSURFACE4 lpDDS,
				LPDIRECT3DDEVICE3* lplpD3DDevice3,
				LPUNKNOWN lpUnk)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    IDirectDrawSurfaceImpl *ddsurfaceimpl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, lpDDS);
    TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice3);
    return create_device_helper(This, rclsid, ddsurfaceimpl, (void **) lplpD3DDevice3, 3);
}

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_FindDevice(LPDIRECT3D3 iface,
				    LPD3DFINDDEVICESEARCH lpD3DDFS,
				    LPD3DFINDDEVICERESULT lpD3DFDR)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D3, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpD3DDFS, lpD3DFDR);
    return d3ddevice_find(This, lpD3DDFS, lpD3DFDR);
}

HRESULT WINAPI
GL_IDirect3DImpl_7_3T_EnumZBufferFormats(LPDIRECT3D7 iface,
					 REFCLSID riidDevice,
					 LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
					 LPVOID lpContext)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    DDPIXELFORMAT pformat;
    
    TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, debugstr_guid(riidDevice), lpEnumCallback, lpContext);

    memset(&pformat, 0, sizeof(pformat));
    pformat.dwSize = sizeof(DDPIXELFORMAT);
    pformat.dwFourCC = 0;   
    TRACE("Enumerating dummy ZBuffer format (16 bits)\n");
    pformat.dwFlags = DDPF_ZBUFFER;
    pformat.u1.dwZBufferBitDepth = 16;
    pformat.u3.dwZBitMask =    0x0000FFFF;
    pformat.u5.dwRGBZBitMask = 0x0000FFFF;

    /* Whatever the return value, stop here.. */
    lpEnumCallback(&pformat, lpContext);
    
    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DImpl_7_EnumDevices(LPDIRECT3D7 iface,
			       LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback,
			       LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpEnumDevicesCallback, lpUserArg);

    if (d3ddevice_enumerate7(lpEnumDevicesCallback, lpUserArg) != D3DENUMRET_OK)
	return D3D_OK;
    
    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DImpl_7_CreateDevice(LPDIRECT3D7 iface,
				REFCLSID rclsid,
				LPDIRECTDRAWSURFACE7 lpDDS,
				LPDIRECT3DDEVICE7* lplpD3DDevice)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    IDirectDrawSurfaceImpl *ddsurfaceimpl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, lpDDS);
    TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice);
    return create_device_helper(This, rclsid, ddsurfaceimpl, (void **) lplpD3DDevice, 7);
}

HRESULT WINAPI
GL_IDirect3DImpl_7_3T_CreateVertexBuffer(LPDIRECT3D7 iface,
					 LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					 LPDIRECT3DVERTEXBUFFER7* lplpD3DVertBuf,
					 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirectDrawImpl, IDirect3D7, iface);
    IDirect3DVertexBufferImpl *vbimpl;
    HRESULT res;
    
    TRACE("(%p/%p)->(%p,%p,%08lx)\n", This, iface, lpD3DVertBufDesc, lplpD3DVertBuf, dwFlags);

    res = d3dvertexbuffer_create(&vbimpl, This, lpD3DVertBufDesc, dwFlags);

    *lplpD3DVertBuf = ICOM_INTERFACE(vbimpl, IDirect3DVertexBuffer7);
    
    return res;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3D7.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3D7Vtbl VTABLE_IDirect3D7 =
{
    XCAST(QueryInterface) Thunk_IDirect3DImpl_7_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_7_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_7_Release,
    XCAST(EnumDevices) GL_IDirect3DImpl_7_EnumDevices,
    XCAST(CreateDevice) GL_IDirect3DImpl_7_CreateDevice,
    XCAST(CreateVertexBuffer) GL_IDirect3DImpl_7_3T_CreateVertexBuffer,
    XCAST(EnumZBufferFormats) GL_IDirect3DImpl_7_3T_EnumZBufferFormats,
    XCAST(EvictManagedTextures) Main_IDirect3DImpl_7_3T_EvictManagedTextures,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3D3.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3D3Vtbl VTABLE_IDirect3D3 =
{
    XCAST(QueryInterface) Thunk_IDirect3DImpl_3_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_3_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_3_Release,
    XCAST(EnumDevices) GL_IDirect3DImpl_3_EnumDevices,
    XCAST(CreateLight) GL_IDirect3DImpl_3_2T_1T_CreateLight,
    XCAST(CreateMaterial) GL_IDirect3DImpl_3_2T_1T_CreateMaterial,
    XCAST(CreateViewport) GL_IDirect3DImpl_3_2T_1T_CreateViewport,
    XCAST(FindDevice) GL_IDirect3DImpl_3_2T_1T_FindDevice,
    XCAST(CreateDevice) GL_IDirect3DImpl_3_CreateDevice,
    XCAST(CreateVertexBuffer) Thunk_IDirect3DImpl_3_CreateVertexBuffer,
    XCAST(EnumZBufferFormats) Thunk_IDirect3DImpl_3_EnumZBufferFormats,
    XCAST(EvictManagedTextures) Thunk_IDirect3DImpl_3_EvictManagedTextures,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3D2.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3D2Vtbl VTABLE_IDirect3D2 =
{
    XCAST(QueryInterface) Thunk_IDirect3DImpl_2_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_2_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_2_Release,
    XCAST(EnumDevices) GL_IDirect3DImpl_2_EnumDevices,
    XCAST(CreateLight) Thunk_IDirect3DImpl_2_CreateLight,
    XCAST(CreateMaterial) Thunk_IDirect3DImpl_2_CreateMaterial,
    XCAST(CreateViewport) Thunk_IDirect3DImpl_2_CreateViewport,
    XCAST(FindDevice) Thunk_IDirect3DImpl_2_FindDevice,
    XCAST(CreateDevice) GL_IDirect3DImpl_2_CreateDevice,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3D.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DVtbl VTABLE_IDirect3D =
{
    XCAST(QueryInterface) Thunk_IDirect3DImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DImpl_1_Initialize,
    XCAST(EnumDevices) GL_IDirect3DImpl_1_EnumDevices,
    XCAST(CreateLight) Thunk_IDirect3DImpl_1_CreateLight,
    XCAST(CreateMaterial) Thunk_IDirect3DImpl_1_CreateMaterial,
    XCAST(CreateViewport) Thunk_IDirect3DImpl_1_CreateViewport,
    XCAST(FindDevice) Thunk_IDirect3DImpl_1_FindDevice,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

static HRESULT d3d_add_device(IDirectDrawImpl *This, IDirect3DDeviceImpl *device)
{
    if  (This->current_device == NULL) {
        /* Create delayed textures now that we have an OpenGL context...
	   For that, go through all surface attached to our DDraw object and create
	   OpenGL textures for all textures.. */
        IDirectDrawSurfaceImpl *surf = This->surfaces;

	while (surf != NULL) {
	    if (surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE) {
	        /* Found a texture.. Now create the OpenGL part */
	        d3dtexture_create(This, surf, FALSE, surf->mip_main);
	    }
	    surf = surf->next_ddraw;
	}
    }
    /* For the moment, only one device 'supported'... */
    This->current_device = device;

    return DD_OK;
}

static HRESULT d3d_remove_device(IDirectDrawImpl *This, IDirect3DDeviceImpl *device)
{
    This->current_device = NULL;
    return DD_OK;
}

HRESULT direct3d_create(IDirectDrawImpl *This)
{
    IDirect3DGLImpl *globject;
    
    globject = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DGLImpl));
    if (globject == NULL) return DDERR_OUTOFMEMORY;

    This->d3d_create_texture = d3dtexture_create;
    This->d3d_added_device = d3d_add_device;
    This->d3d_removed_device = d3d_remove_device;

    ICOM_INIT_INTERFACE(This, IDirect3D,  VTABLE_IDirect3D);
    ICOM_INIT_INTERFACE(This, IDirect3D2, VTABLE_IDirect3D2);
    ICOM_INIT_INTERFACE(This, IDirect3D3, VTABLE_IDirect3D3);
    ICOM_INIT_INTERFACE(This, IDirect3D7, VTABLE_IDirect3D7);

    This->d3d_private = globject;

    TRACE(" creating OpenGL private storage at %p.\n", globject);
    
    return D3D_OK;
}
