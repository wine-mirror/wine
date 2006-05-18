/* Direct3D Viewport
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains the implementation of Direct3DViewport2.
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static void activate(IDirect3DViewportImpl* This) {
    IDirect3DLightImpl* light;
    D3DVIEWPORT7 vp;
    
    /* Activate all the lights associated with this context */
    light = This->lights;

    while (light != NULL) {
        light->activate(light);
	light = light->next;
    }

    /* And copy the values in the structure used by the device */
    if (This->use_vp2) {
        vp.dwX = This->viewports.vp2.dwX;
	vp.dwY = This->viewports.vp2.dwY;
	vp.dwHeight = This->viewports.vp2.dwHeight;
	vp.dwWidth = This->viewports.vp2.dwWidth;
	vp.dvMinZ = This->viewports.vp2.dvMinZ;
	vp.dvMaxZ = This->viewports.vp2.dvMaxZ;
    } else {
        vp.dwX = This->viewports.vp1.dwX;
	vp.dwY = This->viewports.vp1.dwY;
	vp.dwHeight = This->viewports.vp1.dwHeight;
	vp.dwWidth = This->viewports.vp1.dwWidth;
	vp.dvMinZ = This->viewports.vp1.dvMinZ;
	vp.dvMaxZ = This->viewports.vp1.dvMaxZ;
    }
    
    /* And also set the viewport */
    IDirect3DDevice7_SetViewport(ICOM_INTERFACE(This->active_device, IDirect3DDevice7), &vp);
}


static void _dump_D3DVIEWPORT(D3DVIEWPORT *lpvp)
{
    TRACE("    - dwSize = %ld   dwX = %ld   dwY = %ld\n",
	  lpvp->dwSize, lpvp->dwX, lpvp->dwY);
    TRACE("    - dwWidth = %ld   dwHeight = %ld\n",
	  lpvp->dwWidth, lpvp->dwHeight);
    TRACE("    - dvScaleX = %f   dvScaleY = %f\n",
	  lpvp->dvScaleX, lpvp->dvScaleY);
    TRACE("    - dvMaxX = %f   dvMaxY = %f\n",
	  lpvp->dvMaxX, lpvp->dvMaxY);
    TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	  lpvp->dvMinZ, lpvp->dvMaxZ);
}

static void _dump_D3DVIEWPORT2(D3DVIEWPORT2 *lpvp)
{
    TRACE("    - dwSize = %ld   dwX = %ld   dwY = %ld\n",
	  lpvp->dwSize, lpvp->dwX, lpvp->dwY);
    TRACE("    - dwWidth = %ld   dwHeight = %ld\n",
	  lpvp->dwWidth, lpvp->dwHeight);
    TRACE("    - dvClipX = %f   dvClipY = %f\n",
	  lpvp->dvClipX, lpvp->dvClipY);
    TRACE("    - dvClipWidth = %f   dvClipHeight = %f\n",
	  lpvp->dvClipWidth, lpvp->dvClipHeight);
    TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	  lpvp->dvMinZ, lpvp->dvMaxZ);
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_QueryInterface(LPDIRECT3DVIEWPORT3 iface,
                                                REFIID riid,
                                                LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID(&IID_IUnknown,  riid) ||
	 IsEqualGUID(&IID_IDirect3DViewport, riid) ||
	 IsEqualGUID(&IID_IDirect3DViewport2, riid) ||
	 IsEqualGUID(&IID_IDirect3DViewport3, riid) ) {
        IDirect3DViewport3_AddRef(ICOM_INTERFACE(This, IDirect3DViewport3));
        *obp = ICOM_INTERFACE(This, IDirect3DViewport3);
	TRACE("  Creating IDirect3DViewport1/2/3 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DViewportImpl_3_2_1_AddRef(LPDIRECT3DVIEWPORT3 iface)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, ref - 1);

    return ref;
}

ULONG WINAPI
Main_IDirect3DViewportImpl_3_2_1_Release(LPDIRECT3DVIEWPORT3 iface)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, ref + 1);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}


HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_Initialize(LPDIRECT3DVIEWPORT3 iface,
                                            LPDIRECT3D lpDirect3D)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p/%p)->(%p) no-op...\n", This, iface, lpDirect3D);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_GetViewport(LPDIRECT3DVIEWPORT3 iface,
                                             LPD3DVIEWPORT lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);
    if (This->use_vp2 != 0) {
        ERR("  Requesting to get a D3DVIEWPORT struct where a D3DVIEWPORT2 was set !\n");
	return DDERR_INVALIDPARAMS;
    }
    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    memcpy(lpData, &(This->viewports.vp1), dwSize);

    if (TRACE_ON(ddraw)) {
        TRACE("  returning D3DVIEWPORT :\n");
	_dump_D3DVIEWPORT(lpData);
    }
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_SetViewport(LPDIRECT3DVIEWPORT3 iface,
                                             LPD3DVIEWPORT lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    LPDIRECT3DVIEWPORT3 current_viewport;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(ddraw)) {
        TRACE("  getting D3DVIEWPORT :\n");
	_dump_D3DVIEWPORT(lpData);
    }

    This->use_vp2 = 0;
    memset(&(This->viewports.vp1), 0, sizeof(This->viewports.vp1));
    memcpy(&(This->viewports.vp1), lpData, lpData->dwSize);

    /* Tests on two games show that these values are never used properly so override
       them with proper ones :-)
    */
    This->viewports.vp1.dvMinZ = 0.0;
    This->viewports.vp1.dvMaxZ = 1.0;

    if (This->active_device) {
      IDirect3DDevice3_GetCurrentViewport(ICOM_INTERFACE(This->active_device, IDirect3DDevice3), &current_viewport);
      if (ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, current_viewport) == This)
          This->activate(This);
      IDirect3DViewport3_Release(current_viewport);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_TransformVertices(LPDIRECT3DVIEWPORT3 iface,
                                                   DWORD dwVertexCount,
                                                   LPD3DTRANSFORMDATA lpData,
                                                   DWORD dwFlags,
                                                   LPDWORD lpOffScreen)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%08lx,%p,%08lx,%p): stub!\n", This, iface, dwVertexCount, lpData, dwFlags, lpOffScreen);
    if (lpOffScreen)
	*lpOffScreen = 0;
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_LightElements(LPDIRECT3DVIEWPORT3 iface,
                                               DWORD dwElementCount,
                                               LPD3DLIGHTDATA lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%08lx,%p): stub!\n", This, iface, dwElementCount, lpData);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_SetBackground(LPDIRECT3DVIEWPORT3 iface,
                                               D3DMATERIALHANDLE hMat)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p/%p)->(%08lx)\n", This, iface, (DWORD) hMat);
    
    This->background = (IDirect3DMaterialImpl *) hMat;
    TRACE(" setting background color : %f %f %f %f\n",
	  This->background->mat.u.diffuse.u1.r,
	  This->background->mat.u.diffuse.u2.g,
	  This->background->mat.u.diffuse.u3.b,
	  This->background->mat.u.diffuse.u4.a);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_GetBackground(LPDIRECT3DVIEWPORT3 iface,
                                               LPD3DMATERIALHANDLE lphMat,
                                               LPBOOL lpValid)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lphMat, lpValid);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_SetBackgroundDepth(LPDIRECT3DVIEWPORT3 iface,
                                                    LPDIRECTDRAWSURFACE lpDDSurface)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpDDSurface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_GetBackgroundDepth(LPDIRECT3DVIEWPORT3 iface,
                                                    LPDIRECTDRAWSURFACE* lplpDDSurface,
                                                    LPBOOL lpValid)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lplpDDSurface, lpValid);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_Clear(LPDIRECT3DVIEWPORT3 iface,
                                       DWORD dwCount,
                                       LPD3DRECT lpRects,
                                       DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    DWORD color = 0x00000000;
    
    TRACE("(%p/%p)->(%08lx,%p,%08lx)\n", This, iface, dwCount, lpRects, dwFlags);
    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device !\n");
	return D3DERR_VIEWPORTHASNODEVICE;
    }
    if (dwFlags & D3DCLEAR_TARGET) {
        if (This->background == NULL) {
	    ERR(" Trying to clear the color buffer without background material !\n");
	} else {
	    color = 
	      ((int) ((This->background->mat.u.diffuse.u1.r) * 255) << 16) |
	      ((int) ((This->background->mat.u.diffuse.u2.g) * 255) <<  8) |
	      ((int) ((This->background->mat.u.diffuse.u3.b) * 255) <<  0) |
	      ((int) ((This->background->mat.u.diffuse.u4.a) * 255) << 24);
	}
    }
    return This->active_device->clear(This->active_device, dwCount, lpRects, 
				      dwFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET),
				      color, 1.0, 0x00000000);
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_AddLight(LPDIRECT3DVIEWPORT3 iface,
                                          LPDIRECT3DLIGHT lpDirect3DLight)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    IDirect3DLightImpl *lpDirect3DLightImpl = ICOM_OBJECT(IDirect3DLightImpl, IDirect3DLight, lpDirect3DLight);
    DWORD i = 0;
    DWORD map = This->map_lights;
    
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDirect3DLight);
    
    if (This->num_lights >= 8)
        return DDERR_INVALIDPARAMS;

    /* Find a light number and update both light and viewports objects accordingly */
    while(map&1) {
        map>>=1;
	i++;
    }
    lpDirect3DLightImpl->dwLightIndex = i;
    This->num_lights++;
    This->map_lights |= 1<<i;

    /* Add the light in the 'linked' chain */
    lpDirect3DLightImpl->next = This->lights;
    This->lights = lpDirect3DLightImpl;

    /* Attach the light to the viewport */
    lpDirect3DLightImpl->active_viewport = This;
    
    /* If active, activate the light */
    if (This->active_device != NULL) {
        lpDirect3DLightImpl->activate(lpDirect3DLightImpl);
    }
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_DeleteLight(LPDIRECT3DVIEWPORT3 iface,
                                             LPDIRECT3DLIGHT lpDirect3DLight)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    IDirect3DLightImpl *lpDirect3DLightImpl = ICOM_OBJECT(IDirect3DLightImpl, IDirect3DLight, lpDirect3DLight);
    IDirect3DLightImpl *cur_light, *prev_light = NULL;
    
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDirect3DLight);
    cur_light = This->lights;
    while (cur_light != NULL) {
        if (cur_light == lpDirect3DLightImpl) {
	    lpDirect3DLightImpl->desactivate(lpDirect3DLightImpl);
	    if (prev_light == NULL) This->lights = cur_light->next;
	    else prev_light->next = cur_light->next;
	    /* Detach the light to the viewport */
	    cur_light->active_viewport = NULL;
	    This->num_lights--;
	    This->map_lights &= ~(1<<lpDirect3DLightImpl->dwLightIndex);
	    return DD_OK;
	}
	prev_light = cur_light;
	cur_light = cur_light->next;
    }
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_1_NextLight(LPDIRECT3DVIEWPORT3 iface,
                                           LPDIRECT3DLIGHT lpDirect3DLight,
                                           LPDIRECT3DLIGHT* lplpDirect3DLight,
                                           DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p,%p,%08lx): stub!\n", This, iface, lpDirect3DLight, lplpDirect3DLight, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_GetViewport2(LPDIRECT3DVIEWPORT3 iface,
                                            LPD3DVIEWPORT2 lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);
    if (This->use_vp2 != 1) {
        ERR("  Requesting to get a D3DVIEWPORT2 struct where a D3DVIEWPORT was set !\n");
	return DDERR_INVALIDPARAMS;
    }
    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    memcpy(lpData, &(This->viewports.vp2), dwSize);

    if (TRACE_ON(ddraw)) {
        TRACE("  returning D3DVIEWPORT2 :\n");
	_dump_D3DVIEWPORT2(lpData);
    }
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_2_SetViewport2(LPDIRECT3DVIEWPORT3 iface,
                                            LPD3DVIEWPORT2 lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    LPDIRECT3DVIEWPORT3 current_viewport;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(ddraw)) {
        TRACE("  getting D3DVIEWPORT2 :\n");
	_dump_D3DVIEWPORT2(lpData);
    }

    This->use_vp2 = 1;
    memset(&(This->viewports.vp2), 0, sizeof(This->viewports.vp2));
    memcpy(&(This->viewports.vp2), lpData, lpData->dwSize);

    if (This->active_device) {
      IDirect3DDevice3_GetCurrentViewport(ICOM_INTERFACE(This->active_device, IDirect3DDevice3), &current_viewport);
      if (ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, current_viewport) == This)
        This->activate(This);
      IDirect3DViewport3_Release(current_viewport);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_SetBackgroundDepth2(LPDIRECT3DVIEWPORT3 iface,
                                                 LPDIRECTDRAWSURFACE4 lpDDS)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpDDS);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_GetBackgroundDepth2(LPDIRECT3DVIEWPORT3 iface,
                                                 LPDIRECTDRAWSURFACE4* lplpDDS,
                                                 LPBOOL lpValid)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lplpDDS, lpValid);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DViewportImpl_3_Clear2(LPDIRECT3DVIEWPORT3 iface,
                                    DWORD dwCount,
                                    LPD3DRECT lpRects,
                                    DWORD dwFlags,
                                    DWORD dwColor,
                                    D3DVALUE dvZ,
                                    DWORD dwStencil)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p/%p)->(%08lx,%p,%08lx,%08lx,%f,%08lx)\n", This, iface, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device !\n");
	return D3DERR_VIEWPORTHASNODEVICE;
    }
    return This->active_device->clear(This->active_device, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DViewport3.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DViewport3Vtbl VTABLE_IDirect3DViewport3 =
{
    XCAST(QueryInterface) Main_IDirect3DViewportImpl_3_2_1_QueryInterface,
    XCAST(AddRef) Main_IDirect3DViewportImpl_3_2_1_AddRef,
    XCAST(Release) Main_IDirect3DViewportImpl_3_2_1_Release,
    XCAST(Initialize) Main_IDirect3DViewportImpl_3_2_1_Initialize,
    XCAST(GetViewport) Main_IDirect3DViewportImpl_3_2_1_GetViewport,
    XCAST(SetViewport) Main_IDirect3DViewportImpl_3_2_1_SetViewport,
    XCAST(TransformVertices) Main_IDirect3DViewportImpl_3_2_1_TransformVertices,
    XCAST(LightElements) Main_IDirect3DViewportImpl_3_2_1_LightElements,
    XCAST(SetBackground) Main_IDirect3DViewportImpl_3_2_1_SetBackground,
    XCAST(GetBackground) Main_IDirect3DViewportImpl_3_2_1_GetBackground,
    XCAST(SetBackgroundDepth) Main_IDirect3DViewportImpl_3_2_1_SetBackgroundDepth,
    XCAST(GetBackgroundDepth) Main_IDirect3DViewportImpl_3_2_1_GetBackgroundDepth,
    XCAST(Clear) Main_IDirect3DViewportImpl_3_2_1_Clear,
    XCAST(AddLight) Main_IDirect3DViewportImpl_3_2_1_AddLight,
    XCAST(DeleteLight) Main_IDirect3DViewportImpl_3_2_1_DeleteLight,
    XCAST(NextLight) Main_IDirect3DViewportImpl_3_2_1_NextLight,
    XCAST(GetViewport2) Main_IDirect3DViewportImpl_3_2_GetViewport2,
    XCAST(SetViewport2) Main_IDirect3DViewportImpl_3_2_SetViewport2,
    XCAST(SetBackgroundDepth2) Main_IDirect3DViewportImpl_3_SetBackgroundDepth2,
    XCAST(GetBackgroundDepth2) Main_IDirect3DViewportImpl_3_GetBackgroundDepth2,
    XCAST(Clear2) Main_IDirect3DViewportImpl_3_Clear2,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif




HRESULT d3dviewport_create(IDirect3DViewportImpl **obj, IDirectDrawImpl *d3d)
{
    IDirect3DViewportImpl *object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DViewportImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    object->ref = 1;
    object->d3d = d3d;
    object->activate = activate;
    object->use_vp2 = 0xFF;
    object->next = NULL;
    object->lights = NULL;
    object->num_lights = 0;
    object->map_lights = 0;
    
    ICOM_INIT_INTERFACE(object, IDirect3DViewport3, VTABLE_IDirect3DViewport3);

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);
    
    return D3D_OK;
}
