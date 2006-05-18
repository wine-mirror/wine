/* Direct3D Material
 * Copyright (c) 2002 Lionel ULMER
 *
 * This file contains the implementation of Direct3DMaterial.
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
#include "opengl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static void dump_material(LPD3DMATERIAL mat)
{
    DPRINTF("  dwSize : %ld\n", mat->dwSize);
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_3_2T_1T_QueryInterface(LPDIRECT3DMATERIAL3 iface,
                                                  REFIID riid,
                                                  LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial, riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
        *obp = ICOM_INTERFACE(This, IDirect3DMaterial);
	TRACE("  Creating IDirect3DMaterial interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial2, riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
        *obp = ICOM_INTERFACE(This, IDirect3DMaterial2);
	TRACE("  Creating IDirect3DMaterial2 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial3, riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
        *obp = ICOM_INTERFACE(This, IDirect3DMaterial3);
	TRACE("  Creating IDirect3DMaterial3 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DMaterialImpl_3_2T_1T_AddRef(LPDIRECT3DMATERIAL3 iface)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, ref - 1);

    return ref;
}

ULONG WINAPI
Main_IDirect3DMaterialImpl_3_2T_1T_Release(LPDIRECT3DMATERIAL3 iface)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, ref + 1);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_1_Initialize(LPDIRECT3DMATERIAL iface,
                                        LPDIRECT3D lpDirect3D)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial, iface);
    TRACE("(%p/%p)->(%p) no-op...!\n", This, iface, lpDirect3D);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_1_Reserve(LPDIRECT3DMATERIAL iface)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial, iface);
    TRACE("(%p/%p)->() not implemented.\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_1_Unreserve(LPDIRECT3DMATERIAL iface)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial, iface);
    FIXME("(%p/%p)->() not implemented.\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_3_2T_1T_SetMaterial(LPDIRECT3DMATERIAL3 iface,
                                               LPD3DMATERIAL lpMat)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpMat);
    if (TRACE_ON(ddraw))
        dump_material(lpMat);

    /* Stores the material */
    memset(&This->mat, 0, sizeof(This->mat));
    memcpy(&This->mat, lpMat, lpMat->dwSize);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_3_2T_1T_GetMaterial(LPDIRECT3DMATERIAL3 iface,
                                               LPD3DMATERIAL lpMat)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpMat);
    if (TRACE_ON(ddraw)) {
        TRACE("  Returning material : ");
        dump_material(&This->mat);
    }

    /* Copies the material structure */
    dwSize = lpMat->dwSize;
    memset(lpMat, 0, dwSize);
    memcpy(lpMat, &This->mat, dwSize);

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DMaterialImpl_3_2T_1T_GetHandle(LPDIRECT3DMATERIAL3 iface,
					     LPDIRECT3DDEVICE3 lpDirect3DDevice3,
					     LPD3DMATERIALHANDLE lpHandle)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpDirect3DDevice3, lpHandle);

    This->active_device = ICOM_OBJECT(IDirect3DDeviceImpl, IDirect3DDevice3, lpDirect3DDevice3);
    *lpHandle = (DWORD) This; /* Warning: this is not 64 bit clean.
			         Maybe also we need to store this material somewhere in the device ? */

    TRACE(" returning handle %08lx.\n", *lpHandle);
    
    return DD_OK;
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_GetHandle(LPDIRECT3DMATERIAL2 iface,
					LPDIRECT3DDEVICE2 lpDirect3DDevice2,
					LPD3DMATERIALHANDLE lpHandle)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpDirect3DDevice2, lpHandle);
    return IDirect3DMaterial3_GetHandle(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
					COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, lpDirect3DDevice2),
					lpHandle);
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_GetHandle(LPDIRECT3DMATERIAL iface,
					LPDIRECT3DDEVICE lpDirect3DDevice,
					LPD3DMATERIALHANDLE lpHandle)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpDirect3DDevice, lpHandle);
    return IDirect3DMaterial3_GetHandle(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
					COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, lpDirect3DDevice),
					lpHandle);
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_QueryInterface(LPDIRECT3DMATERIAL2 iface,
                                             REFIID riid,
                                             LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DMaterial3 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DMaterial3_QueryInterface(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
                                             riid,
                                             obp);
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_QueryInterface(LPDIRECT3DMATERIAL iface,
                                             REFIID riid,
                                             LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DMaterial3 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DMaterial3_QueryInterface(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
                                             riid,
                                             obp);
}

ULONG WINAPI
Thunk_IDirect3DMaterialImpl_2_AddRef(LPDIRECT3DMATERIAL2 iface)
{
    TRACE("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_AddRef(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface));
}

ULONG WINAPI
Thunk_IDirect3DMaterialImpl_1_AddRef(LPDIRECT3DMATERIAL iface)
{
    TRACE("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_AddRef(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface));
}

ULONG WINAPI
Thunk_IDirect3DMaterialImpl_2_Release(LPDIRECT3DMATERIAL2 iface)
{
    TRACE("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_Release(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface));
}

ULONG WINAPI
Thunk_IDirect3DMaterialImpl_1_Release(LPDIRECT3DMATERIAL iface)
{
    TRACE("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_Release(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface));
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_SetMaterial(LPDIRECT3DMATERIAL2 iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_SetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
                                          lpMat);
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_SetMaterial(LPDIRECT3DMATERIAL iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_SetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
                                          lpMat);
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_GetMaterial(LPDIRECT3DMATERIAL2 iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_GetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
                                          lpMat);
}

HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_GetMaterial(LPDIRECT3DMATERIAL iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_GetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
                                          lpMat);
}

/*******************************************************************************
 *				Matrial2 static functions
 */
static void activate(IDirect3DMaterialImpl* This) {
    TRACE("Activating material %p\n", This);

    /* Set the current Material */
    ENTER_GL();
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_DIFFUSE,
		 (float *) &(This->mat.u.diffuse));
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_AMBIENT,
		 (float *) &(This->mat.u1.ambient));
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_SPECULAR,
		 (float *) &(This->mat.u2.specular));
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_EMISSION,
		 (float *) &(This->mat.u3.emissive));
    LEAVE_GL();

    if (TRACE_ON(ddraw)) {
	DPRINTF(" - size  : %ld\n", This->mat.dwSize);
	DPRINTF(" - diffuse : "); dump_D3DCOLORVALUE(&(This->mat.u.diffuse)); DPRINTF("\n");
	DPRINTF(" - ambient : "); dump_D3DCOLORVALUE(&(This->mat.u1.ambient)); DPRINTF("\n");
	DPRINTF(" - specular: "); dump_D3DCOLORVALUE(&(This->mat.u2.specular)); DPRINTF("\n");
	DPRINTF(" - emissive: "); dump_D3DCOLORVALUE(&(This->mat.u3.emissive)); DPRINTF("\n");
	DPRINTF(" - power : %f\n", This->mat.u4.power);
	DPRINTF(" - texture handle : %08lx\n", (DWORD)This->mat.hTexture);
    }
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DMaterial3.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DMaterial3Vtbl VTABLE_IDirect3DMaterial3 =
{
    XCAST(QueryInterface) Main_IDirect3DMaterialImpl_3_2T_1T_QueryInterface,
    XCAST(AddRef) Main_IDirect3DMaterialImpl_3_2T_1T_AddRef,
    XCAST(Release) Main_IDirect3DMaterialImpl_3_2T_1T_Release,
    XCAST(SetMaterial) Main_IDirect3DMaterialImpl_3_2T_1T_SetMaterial,
    XCAST(GetMaterial) Main_IDirect3DMaterialImpl_3_2T_1T_GetMaterial,
    XCAST(GetHandle) Main_IDirect3DMaterialImpl_3_2T_1T_GetHandle,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DMaterial2.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DMaterial2Vtbl VTABLE_IDirect3DMaterial2 =
{
    XCAST(QueryInterface) Thunk_IDirect3DMaterialImpl_2_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DMaterialImpl_2_AddRef,
    XCAST(Release) Thunk_IDirect3DMaterialImpl_2_Release,
    XCAST(SetMaterial) Thunk_IDirect3DMaterialImpl_2_SetMaterial,
    XCAST(GetMaterial) Thunk_IDirect3DMaterialImpl_2_GetMaterial,
    XCAST(GetHandle) Thunk_IDirect3DMaterialImpl_2_GetHandle,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DMaterial.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DMaterialVtbl VTABLE_IDirect3DMaterial =
{
    XCAST(QueryInterface) Thunk_IDirect3DMaterialImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DMaterialImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DMaterialImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DMaterialImpl_1_Initialize,
    XCAST(SetMaterial) Thunk_IDirect3DMaterialImpl_1_SetMaterial,
    XCAST(GetMaterial) Thunk_IDirect3DMaterialImpl_1_GetMaterial,
    XCAST(GetHandle) Thunk_IDirect3DMaterialImpl_1_GetHandle,
    XCAST(Reserve) Main_IDirect3DMaterialImpl_1_Reserve,
    XCAST(Unreserve) Main_IDirect3DMaterialImpl_1_Unreserve,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif




HRESULT d3dmaterial_create(IDirect3DMaterialImpl **obj, IDirectDrawImpl *d3d)
{
    IDirect3DMaterialImpl *object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DMaterialImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    object->ref = 1;
    object->d3d = d3d;
    object->activate = activate;
    
    ICOM_INIT_INTERFACE(object, IDirect3DMaterial,  VTABLE_IDirect3DMaterial);
    ICOM_INIT_INTERFACE(object, IDirect3DMaterial2, VTABLE_IDirect3DMaterial2);
    ICOM_INIT_INTERFACE(object, IDirect3DMaterial3, VTABLE_IDirect3DMaterial3);

    *obj = object;
    
    TRACE(" creating implementation at %p.\n", *obj);
    
    return D3D_OK;
}
