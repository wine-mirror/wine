/* Direct3D Light
 * Copyright (c) 1998 / 2002 Lionel ULMER
 *
 * This file contains the implementation of Direct3DLight.
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "config.h"
#include "windef.h"
#include "winerror.h"
#include "objbase.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"
#include "mesa_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* First, the 'main' interface... */
HRESULT WINAPI
Main_IDirect3DLightImpl_1_QueryInterface(LPDIRECT3DLIGHT iface,
                                         REFIID riid,
                                         LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    FIXME("(%p/%p)->(%s,%p): stub!\n", This, iface, debugstr_guid(riid), obp);
    return DD_OK;
}

ULONG WINAPI
Main_IDirect3DLightImpl_1_AddRef(LPDIRECT3DLIGHT iface)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, This->ref);
    return ++(This->ref);
}

ULONG WINAPI
Main_IDirect3DLightImpl_1_Release(LPDIRECT3DLIGHT iface)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    if (!--(This->ref)) {
        HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
}

HRESULT WINAPI
Main_IDirect3DLightImpl_1_Initialize(LPDIRECT3DLIGHT iface,
                                     LPDIRECT3D lpDirect3D)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    TRACE("(%p/%p)->(%p) no-op...\n", This, iface, lpDirect3D);
    return DD_OK;
}

/*** IDirect3DLight methods ***/
static void dump_light(LPD3DLIGHT2 light)
{
    DPRINTF("    - dwSize : %ld\n", light->dwSize);
}

static const float zero_value[] = {
    0.0, 0.0, 0.0, 0.0
};

HRESULT WINAPI
Main_IDirect3DLightImpl_1_SetLight(LPDIRECT3DLIGHT iface,
                                   LPD3DLIGHT lpLight)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    LPD3DLIGHT7 light7 = &(This->light7);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpLight);
    if (TRACE_ON(ddraw)) {
        TRACE("  Light definition : \n");
	dump_light((LPD3DLIGHT2) lpLight);
    }

    if ( (lpLight->dltType == 0) || (lpLight->dltType > D3DLIGHT_PARALLELPOINT) )
         return DDERR_INVALIDPARAMS;
    
    if ( lpLight->dltType == D3DLIGHT_PARALLELPOINT )
	 FIXME("D3DLIGHT_PARALLELPOINT no supported\n");
    
    /* Translate D3DLIGH2 structure to D3DLIGHT7 */
    light7->dltType        = lpLight->dltType;
    light7->dcvDiffuse     = lpLight->dcvColor;
    if ((((LPD3DLIGHT2)lpLight)->dwFlags & D3DLIGHT_NO_SPECULAR) != 0)	    
      light7->dcvSpecular    = lpLight->dcvColor;
    else
      light7->dcvSpecular    = *(D3DCOLORVALUE*)zero_value;	    
    light7->dcvAmbient     = lpLight->dcvColor;
    light7->dvPosition     = lpLight->dvPosition;
    light7->dvDirection    = lpLight->dvDirection;
    light7->dvRange        = lpLight->dvRange;
    light7->dvFalloff      = lpLight->dvFalloff;
    light7->dvAttenuation0 = lpLight->dvAttenuation0;
    light7->dvAttenuation1 = lpLight->dvAttenuation1;
    light7->dvAttenuation2 = lpLight->dvAttenuation2;
    light7->dvTheta        = lpLight->dvTheta;
    light7->dvPhi          = lpLight->dvPhi;

    memcpy(&This->light, lpLight, lpLight->dwSize);
    if ((This->light.dwFlags & D3DLIGHT_ACTIVE) != 0) {
        This->update(This);        
    }
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DLightImpl_1_GetLight(LPDIRECT3DLIGHT iface,
                                   LPD3DLIGHT lpLight)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpLight);
    if (TRACE_ON(ddraw)) {
        TRACE("  Returning light definition : \n");
	dump_light(&This->light);
    }
    memcpy(lpLight, &This->light, lpLight->dwSize);
    return DD_OK;
}

/*******************************************************************************
 *				Light static functions
 */

static void update(IDirect3DLightImpl* This) {
    IDirect3DDeviceImpl* device;
    if (!This->active_viewport||!This->active_viewport->active_device)
        return;
    device =  This->active_viewport->active_device;
    IDirect3DDevice7_SetLight(ICOM_INTERFACE(device,IDirect3DDevice7),This->dwLightIndex,&(This->light7));
}

static void activate(IDirect3DLightImpl* This) {
    IDirect3DLightGLImpl *glThis = (IDirect3DLightGLImpl *) This;
    ENTER_GL();
    update(This);
    /* If was not active, activate it */
    if ((glThis->parent.light.dwFlags & D3DLIGHT_ACTIVE) == 0) {
        glEnable(glThis->light_num);
	glThis->parent.light.dwFlags |= D3DLIGHT_ACTIVE;
    }
    LEAVE_GL();
}

static void desactivate(IDirect3DLightImpl* This) {
    IDirect3DLightGLImpl *glThis = (IDirect3DLightGLImpl *) This;
    ENTER_GL();
    /* If was not active, activate it */
    if ((glThis->parent.light.dwFlags & D3DLIGHT_ACTIVE) != 0) {
        glDisable(glThis->light_num);
	glThis->parent.light.dwFlags &= ~D3DLIGHT_ACTIVE;
    }
    LEAVE_GL();
}

ULONG WINAPI
GL_IDirect3DLightImpl_1_Release(LPDIRECT3DLIGHT iface)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    IDirect3DLightGLImpl *glThis = (IDirect3DLightGLImpl *) This;
    
    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    if (!--(This->ref)) {
        ((IDirect3DGLImpl *) This->d3d)->light_released(This->d3d, glThis->light_num);
        HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DLight.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DLight) VTABLE_IDirect3DLight =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Main_IDirect3DLightImpl_1_QueryInterface,
    XCAST(AddRef) Main_IDirect3DLightImpl_1_AddRef,
    XCAST(Release) GL_IDirect3DLightImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DLightImpl_1_Initialize,
    XCAST(SetLight) Main_IDirect3DLightImpl_1_SetLight,
    XCAST(GetLight) Main_IDirect3DLightImpl_1_GetLight,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif




HRESULT d3dlight_create(IDirect3DLightImpl **obj, IDirectDrawImpl *d3d, GLenum light_num)
{
    IDirect3DLightImpl *object;
    IDirect3DLightGLImpl *gl_object;
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DLightGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;
    gl_object = (IDirect3DLightGLImpl *) object;
    
    object->ref = 1;
    object->d3d = d3d;
    object->next = NULL;
    object->activate = activate;
    object->desactivate = desactivate;
    object->update = update;
    object->active_viewport = NULL;
    gl_object->light_num = light_num;
    
    ICOM_INIT_INTERFACE(object, IDirect3DLight, VTABLE_IDirect3DLight);

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);
    
    return D3D_OK;
}
