#include "config.h"

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "winerror.h"
#include "ddraw.h"
#include "d3d.h"
#include "message.h"
#include "options.h"
#include "monitor.h"
#include "debugtools.h"

#include "mesa_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/*******************************************************************************
 *				IDirect3D
 */
static HRESULT WINAPI MESA_IDirect3DImpl_QueryInterface(
    LPDIRECT3D iface,REFIID refiid,LPVOID *obj
) {
    ICOM_THIS(IDirect3DImpl,iface);
    /* FIXME: Not sure if this is correct */

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
    if (( IsEqualGUID( &IID_IDirectDraw,  refiid ) ) ||
	( IsEqualGUID (&IID_IDirectDraw2, refiid ) ) ||
	( IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    ) {
	*obj = This->ddraw;
	IDirect3D_AddRef(iface);
	TRACE("  Creating IDirectDrawX interface (%p)\n", *obj);
	return S_OK;
    }
    if (( IsEqualGUID( &IID_IDirect3D, refiid ) ) ||
	( IsEqualGUID( &IID_IUnknown,  refiid ) ) ) {
	*obj = This;
	IDirect3D_AddRef(iface);
	TRACE("  Creating IDirect3D interface (%p)\n", *obj);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3D2, refiid ) ) {
	IDirect3D2Impl*  d3d;

	d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
	d3d->ref = 1;
	d3d->ddraw = This->ddraw;
	IDirect3D_AddRef(iface);
	ICOM_VTBL(d3d) = &mesa_d3d2vt;
	*obj = d3d;
	TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);
	return S_OK;
    }
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI MESA_IDirect3DImpl_Release(LPDIRECT3D iface)
{
    ICOM_THIS(IDirect3DImpl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    return This->ref;
}

static HRESULT WINAPI MESA_IDirect3DImpl_EnumDevices(
    LPDIRECT3D iface, LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p),stub!\n",This,cb,context);

    /* Call functions defined in d3ddevices.c */
    if (!d3d_OpenGL_dx3(cb, context))
	return D3D_OK;

    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3DImpl_CreateLight(
    LPDIRECT3D iface, LPDIRECT3DLIGHT *lplight, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3DImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lplight, lpunk);

    /* Call the creation function that is located in d3dlight.c */
    *lplight = d3dlight_create_dx3(This);

    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3DImpl_CreateMaterial(
    LPDIRECT3D iface, LPDIRECT3DMATERIAL *lpmaterial, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3DImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);
    /* Call the creation function that is located in d3dviewport.c */
    *lpmaterial = d3dmaterial_create(This);
    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3DImpl_CreateViewport(
    LPDIRECT3D iface, LPDIRECT3DVIEWPORT *lpviewport, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3DImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);

    /* Call the creation function that is located in d3dviewport.c */
    *lpviewport = d3dviewport_create(This);

    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3DImpl_FindDevice(
    LPDIRECT3D iface, LPD3DFINDDEVICESEARCH lpfinddevsrc,
    LPD3DFINDDEVICERESULT lpfinddevrst)
{
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);

    return D3D_OK;
}

ICOM_VTABLE(IDirect3D) mesa_d3dvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    MESA_IDirect3DImpl_QueryInterface,
    IDirect3DImpl_AddRef,
    MESA_IDirect3DImpl_Release,
    IDirect3DImpl_Initialize,
    MESA_IDirect3DImpl_EnumDevices,
    MESA_IDirect3DImpl_CreateLight,
    MESA_IDirect3DImpl_CreateMaterial,
    MESA_IDirect3DImpl_CreateViewport,
    MESA_IDirect3DImpl_FindDevice
};

/*******************************************************************************
 *				IDirect3D2
 */
static HRESULT WINAPI MESA_IDirect3D2Impl_QueryInterface(
    LPDIRECT3D2 iface,REFIID refiid,LPVOID *obj) {  
    ICOM_THIS(IDirect3D2Impl,iface);

    /* FIXME: Not sure if this is correct */

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
    if ( ( IsEqualGUID( &IID_IDirectDraw,  refiid ) ) ||
	 ( IsEqualGUID( &IID_IDirectDraw2, refiid ) ) ||
	 ( IsEqualGUID( &IID_IDirectDraw4, refiid ) ) ) {
	*obj = This->ddraw;
	IDirect3D2_AddRef(iface);

	TRACE("  Creating IDirectDrawX interface (%p)\n", *obj);
	
	return S_OK;
    }
    if ( ( IsEqualGUID( &IID_IDirect3D2, refiid ) ) ||
	 ( IsEqualGUID( &IID_IUnknown,   refiid ) ) ) {
	*obj = This;
	IDirect3D2_AddRef(iface);

	TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);

	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3D, refiid ) ) {
	IDirect3DImpl*  d3d;

	d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
	d3d->ref = 1;
	d3d->ddraw = This->ddraw;
	IDirect3D2_AddRef(iface);
	ICOM_VTBL(d3d) = &mesa_d3dvt;
	*obj = d3d;
	TRACE("  Creating IDirect3D interface (%p)\n", *obj);
	return S_OK;
    }
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI MESA_IDirect3D2Impl_Release(LPDIRECT3D2 iface) {
    ICOM_THIS(IDirect3D2Impl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    return This->ref;
}

static HRESULT WINAPI MESA_IDirect3D2Impl_EnumDevices(
    LPDIRECT3D2 iface,LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p),stub!\n",This,cb,context);

    /* Call functions defined in d3ddevices.c */
    if (!d3d_OpenGL(cb, context))
	return D3D_OK;
    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3D2Impl_CreateLight(
    LPDIRECT3D2 iface, LPDIRECT3DLIGHT *lplight, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lplight, lpunk);

    /* Call the creation function that is located in d3dlight.c */
    *lplight = d3dlight_create(This);

    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3D2Impl_CreateMaterial(
    LPDIRECT3D2 iface, LPDIRECT3DMATERIAL2 *lpmaterial, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);

    /* Call the creation function that is located in d3dviewport.c */
    *lpmaterial = d3dmaterial2_create(This);

    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3D2Impl_CreateViewport(
    LPDIRECT3D2 iface, LPDIRECT3DVIEWPORT2 *lpviewport, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);

    /* Call the creation function that is located in d3dviewport.c */
    *lpviewport = d3dviewport2_create(This);

    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3D2Impl_FindDevice(
    LPDIRECT3D2 iface, LPD3DFINDDEVICESEARCH lpfinddevsrc,
    LPD3DFINDDEVICERESULT lpfinddevrst)
{
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);
    return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3D2Impl_CreateDevice(
    LPDIRECT3D2 iface, REFCLSID rguid, LPDIRECTDRAWSURFACE surface,
    LPDIRECT3DDEVICE2 *device)
{
    ICOM_THIS(IDirect3D2Impl,iface);

    FIXME("(%p)->(%s,%p,%p): stub\n",This,debugstr_guid(rguid),surface,device);

    if (is_OpenGL(rguid, (IDirectDrawSurfaceImpl*)surface, (IDirect3DDevice2Impl**)device, This)) {
	IDirect3D2_AddRef(iface);
	return D3D_OK;
    }

    return DDERR_INVALIDPARAMS;
}

ICOM_VTABLE(IDirect3D2) mesa_d3d2vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    MESA_IDirect3D2Impl_QueryInterface,
    IDirect3D2Impl_AddRef,
    MESA_IDirect3D2Impl_Release,
    MESA_IDirect3D2Impl_EnumDevices,
    MESA_IDirect3D2Impl_CreateLight,
    MESA_IDirect3D2Impl_CreateMaterial,
    MESA_IDirect3D2Impl_CreateViewport,
    MESA_IDirect3D2Impl_FindDevice,
    MESA_IDirect3D2Impl_CreateDevice
};

static HRESULT WINAPI MESA_IDirect3D3Impl_CreateVertexBuffer(
    LPDIRECT3D3 iface,
    LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
    LPDIRECT3DVERTEXBUFFER* lplpD3DVertBuf,
    DWORD dwFlags,
    LPUNKNOWN lpUnk)
{
  FIXME(":stub\n");
  return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3D3Impl_EnumZBufferFormats(
    LPDIRECT3D3 iface,
    REFCLSID riidDevice,
    LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
    LPVOID lpContext)
{
  FIXME(":stub\n");
  return DDERR_NOZBUFFERHW; /* Pretend we don't have valid HW */
}

static HRESULT WINAPI MESA_IDirect3D3Impl_EvictManagedTextures(
    LPDIRECT3D3 iface)
{
  FIXME(":stub\n");
  return D3D_OK;
}


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(mesa_d3d3vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

ICOM_VTABLE(IDirect3D3) mesa_d3d3vt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)MESA_IDirect3D2Impl_QueryInterface,
    XCAST(AddRef)IDirect3D2Impl_AddRef,
    XCAST(Release)MESA_IDirect3D2Impl_Release,
    XCAST(EnumDevices)MESA_IDirect3D2Impl_EnumDevices,
    XCAST(CreateLight)MESA_IDirect3D2Impl_CreateLight,
    XCAST(CreateMaterial)MESA_IDirect3D2Impl_CreateMaterial,
    XCAST(CreateViewport)MESA_IDirect3D2Impl_CreateViewport,
    XCAST(FindDevice)MESA_IDirect3D2Impl_FindDevice,
    XCAST(CreateDevice)MESA_IDirect3D2Impl_CreateDevice,
    XCAST(CreateVertexBuffer)MESA_IDirect3D3Impl_CreateVertexBuffer,
    XCAST(EnumZBufferFormats)MESA_IDirect3D3Impl_EnumZBufferFormats,
    XCAST(EvictManagedTextures)MESA_IDirect3D3Impl_EvictManagedTextures
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

HRESULT create_direct3d(LPVOID *obj,IDirectDraw2Impl* ddraw) {
    IDirect3DImpl* d3d;

    d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
    d3d->ref = 1;
    d3d->ddraw = (IDirectDrawImpl *) ddraw;
    d3d->private = NULL; /* unused for now */
    IDirectDraw_AddRef((LPDIRECTDRAW)ddraw);
    ICOM_VTBL(d3d) = &mesa_d3dvt;
    *obj = (LPUNKNOWN)d3d;
    TRACE("  Created IDirect3D interface (%p)\n", *obj);

    return S_OK;
}

HRESULT create_direct3d2(LPVOID *obj,IDirectDraw2Impl* ddraw) {
    IDirect3D2Impl* d3d;

    d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
    d3d->ref = 1;
    d3d->ddraw = (IDirectDrawImpl *) ddraw;
    d3d->private = NULL; /* unused for now */
    IDirectDraw_AddRef((LPDIRECTDRAW)ddraw);
    ICOM_VTBL(d3d) = &mesa_d3d2vt;
    *obj = (LPUNKNOWN)d3d;
    TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);

    return S_OK;
}

HRESULT create_direct3d3(LPVOID *obj,IDirectDraw2Impl* ddraw) {
    IDirect3D3Impl* d3d;

    d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
    d3d->ref = 1;
    d3d->ddraw = (IDirectDrawImpl *) ddraw;
    d3d->private = NULL; /* unused for now */

    IDirectDraw_AddRef((LPDIRECTDRAW)ddraw);
    ICOM_VTBL(d3d) = &mesa_d3d3vt;
    *obj = (LPUNKNOWN)d3d;

    TRACE("  Creating IDirect3D3 interface (%p)\n", *obj);
    
    return S_OK;
}

