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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "d3d.h"
#include "ddraw.h"
#include "winerror.h"

#include "ddraw_private.h"
#include "d3d_private.h"
#include "mesa_private.h"
#include "main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

#define MAX_LIGHTS 8

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_EnumDevices(LPDIRECT3D3 iface,
				     LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
				     LPVOID lpUserArg)
{
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D3, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpEnumDevicesCallback, lpUserArg);

    /* Call functions defined in d3ddevices.c */
    if (d3ddevice_enumerate(lpEnumDevicesCallback, lpUserArg) != D3DENUMRET_OK)
	return D3D_OK;

    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_CreateLight(LPDIRECT3D3 iface,
				     LPDIRECT3DLIGHT* lplpDirect3DLight,
				     IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D3, iface);
    IDirect3DGLImpl *glThis = (IDirect3DGLImpl *) This;
    int fl;
    IDirect3DLightImpl *d3dlimpl;
    HRESULT ret_value;
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lplpDirect3DLight, pUnkOuter);
    for (fl = 0; fl < MAX_LIGHTS; fl++) {
        if ((glThis->free_lights & (0x01 << fl)) != 0) {
	    glThis->free_lights &= ~(0x01 << fl);
	    break;
	}
    }
    if (fl == MAX_LIGHTS) {
        return DDERR_INVALIDPARAMS; /* No way to say 'max lights reached' ... */
    }
    ret_value = d3dlight_create(&d3dlimpl, This, GL_LIGHT0 + fl);
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D3, iface);
    
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D3, iface);
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lplpD3DViewport3, pUnkOuter);
    ret_value = d3dviewport_create(&D3Dvp_impl, This);

    *lplpD3DViewport3 = ICOM_INTERFACE(D3Dvp_impl, IDirect3DViewport3);

    return ret_value;
}

static HRESULT
create_device_helper(IDirect3DImpl *This,
		     REFCLSID iid,
		     IDirectDrawSurfaceImpl *lpDDS,
		     void **obj,
		     int interface) {
    IDirect3DDeviceImpl *lpd3ddev;
    HRESULT ret_value;

    ret_value = d3ddevice_create(&lpd3ddev, This, lpDDS);
    if (FAILED(ret_value)) return ret_value;
    
    if ((iid == NULL) ||
	(IsEqualGUID(&IID_D3DDEVICE_OpenGL, iid)) ||
	(IsEqualGUID(&IID_IDirect3DHALDevice, iid)) ||
	(IsEqualGUID(&IID_IDirect3DTnLHalDevice, iid))) {
        switch (interface) {
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D2, iface);
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D3, iface);
    IDirectDrawSurfaceImpl *ddsurfaceimpl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, lpDDS);
    TRACE("(%p/%p)->(%s,%p,%p)\n", This, iface, debugstr_guid(rclsid), lpDDS, lplpD3DDevice3);
    return create_device_helper(This, rclsid, ddsurfaceimpl, (void **) lplpD3DDevice3, 3);
}

HRESULT WINAPI
GL_IDirect3DImpl_3_2T_1T_FindDevice(LPDIRECT3D3 iface,
				    LPD3DFINDDEVICESEARCH lpD3DDFS,
				    LPD3DFINDDEVICERESULT lpD3DFDR)
{
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D3, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpD3DDFS, lpD3DFDR);
    return d3ddevice_find(This, lpD3DDFS, lpD3DFDR);
}

HRESULT WINAPI
GL_IDirect3DImpl_7_3T_EnumZBufferFormats(LPDIRECT3D7 iface,
					 REFCLSID riidDevice,
					 LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
					 LPVOID lpContext)
{
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D7, iface);
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D7, iface);
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D7, iface);
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
    ICOM_THIS_FROM(IDirect3DImpl, IDirect3D7, iface);
    IDirect3DVertexBufferImpl *vbimpl;
    HRESULT res;
    
    TRACE("(%p/%p)->(%p,%p,%08lx)\n", This, iface, lpD3DVertBufDesc, lplpD3DVertBuf, dwFlags);

    res = d3dvertexbuffer_create(&vbimpl, This, lpD3DVertBufDesc, dwFlags);

    *lplpD3DVertBuf = ICOM_INTERFACE(vbimpl, IDirect3DVertexBuffer7);
    
    return res;
}

static void light_released(IDirect3DImpl *This, GLenum light_num)
{
    IDirect3DGLImpl *glThis = (IDirect3DGLImpl *) This;
    glThis->free_lights |= (light_num - GL_LIGHT0);
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3D7.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3D7) VTABLE_IDirect3D7 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Main_IDirect3DImpl_7_3T_2T_1T_QueryInterface,
    XCAST(AddRef) Main_IDirect3DImpl_7_3T_2T_1T_AddRef,
    XCAST(Release) Main_IDirect3DImpl_7_3T_2T_1T_Release,
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

ICOM_VTABLE(IDirect3D3) VTABLE_IDirect3D3 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DImpl_3_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_3_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_3_Release,
    XCAST(EnumDevices) GL_IDirect3DImpl_3_2T_1T_EnumDevices,
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

ICOM_VTABLE(IDirect3D2) VTABLE_IDirect3D2 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DImpl_2_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_2_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_2_Release,
    XCAST(EnumDevices) Thunk_IDirect3DImpl_2_EnumDevices,
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

ICOM_VTABLE(IDirect3D) VTABLE_IDirect3D =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DImpl_1_Initialize,
    XCAST(EnumDevices) Thunk_IDirect3DImpl_1_EnumDevices,
    XCAST(CreateLight) Thunk_IDirect3DImpl_1_CreateLight,
    XCAST(CreateMaterial) Thunk_IDirect3DImpl_1_CreateMaterial,
    XCAST(CreateViewport) Thunk_IDirect3DImpl_1_CreateViewport,
    XCAST(FindDevice) Thunk_IDirect3DImpl_1_FindDevice,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

HRESULT direct3d_create(IDirect3DImpl **obj, IDirectDrawImpl *ddraw)
{
    IDirect3DImpl *object;
    IDirect3DGLImpl *globject;
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    object->ref = 1;
    object->ddraw = ddraw;
    
    ICOM_INIT_INTERFACE(object, IDirect3D,  VTABLE_IDirect3D);
    ICOM_INIT_INTERFACE(object, IDirect3D2, VTABLE_IDirect3D2);
    ICOM_INIT_INTERFACE(object, IDirect3D3, VTABLE_IDirect3D3);
    ICOM_INIT_INTERFACE(object, IDirect3D7, VTABLE_IDirect3D7);

    globject = (IDirect3DGLImpl *) object;
    globject->free_lights = (0x01 << MAX_LIGHTS) - 1; /* There are, in total, 8 lights in OpenGL */
    globject->light_released = light_released;

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);
    
    return D3D_OK;
}
