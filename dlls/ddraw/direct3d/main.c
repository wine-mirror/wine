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

#include "d3d_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static ICOM_VTABLE(IDirect3D) d3dvt;
static ICOM_VTABLE(IDirect3D2) d3d2vt;

/*******************************************************************************
 *				IDirect3D
 */
HRESULT WINAPI IDirect3DImpl_QueryInterface(
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
	ICOM_VTBL(d3d) = &d3d2vt;
	*obj = d3d;
	TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);
	return S_OK;
    }
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI IDirect3DImpl_AddRef(LPDIRECT3D iface) {
    ICOM_THIS(IDirect3DImpl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );

    return ++(This->ref);
}

ULONG WINAPI IDirect3DImpl_Release(LPDIRECT3D iface)
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

HRESULT WINAPI IDirect3DImpl_Initialize(LPDIRECT3D iface,REFIID refiid) {
    ICOM_THIS(IDirect3DImpl,iface);
    /* FIXME: Not sure if this is correct */
    FIXME("(%p)->(%s):stub.\n",This,debugstr_guid(refiid));
    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI IDirect3DImpl_EnumDevices(
    LPDIRECT3D iface, LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p),stub!\n",This,cb,context);
    return DD_OK;
}

HRESULT WINAPI IDirect3DImpl_CreateLight(
    LPDIRECT3D iface, LPDIRECT3DLIGHT *lplight, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lplight, lpunk);
    return E_FAIL;
}

HRESULT WINAPI IDirect3DImpl_CreateMaterial(
    LPDIRECT3D iface, LPDIRECT3DMATERIAL *lpmaterial, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);
    return E_FAIL;
}

HRESULT WINAPI IDirect3DImpl_CreateViewport(
    LPDIRECT3D iface, LPDIRECT3DVIEWPORT *lpviewport, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);

    return E_FAIL;
}

HRESULT WINAPI IDirect3DImpl_FindDevice(
    LPDIRECT3D iface, LPD3DFINDDEVICESEARCH lpfinddevsrc,
    LPD3DFINDDEVICERESULT lpfinddevrst)
{
    ICOM_THIS(IDirect3DImpl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);
    return DD_OK;
}

/* This is for checking the correctness of the prototypes/functions.
 * Do not remove.
 */
static WINE_UNUSED ICOM_VTABLE(IDirect3D) d3dvt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DImpl_QueryInterface,
    IDirect3DImpl_AddRef,
    IDirect3DImpl_Release,
    IDirect3DImpl_Initialize,
    IDirect3DImpl_EnumDevices,
    IDirect3DImpl_CreateLight,
    IDirect3DImpl_CreateMaterial,
    IDirect3DImpl_CreateViewport,
    IDirect3DImpl_FindDevice
};

/*******************************************************************************
 *				IDirect3D2
 */
HRESULT WINAPI IDirect3D2Impl_QueryInterface(
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
	ICOM_VTBL(d3d) = &d3dvt;
	*obj = d3d;
	TRACE("  Creating IDirect3D interface (%p)\n", *obj);
	return S_OK;
    }
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI IDirect3D2Impl_AddRef(LPDIRECT3D2 iface) {
    ICOM_THIS(IDirect3D2Impl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );

    return ++(This->ref);
}

ULONG WINAPI IDirect3D2Impl_Release(LPDIRECT3D2 iface) {
    ICOM_THIS(IDirect3D2Impl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    return This->ref;
}

HRESULT WINAPI IDirect3D2Impl_EnumDevices(
    LPDIRECT3D2 iface,LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p),stub!\n",This,cb,context);
    return DD_OK;
}

HRESULT WINAPI IDirect3D2Impl_CreateLight(
    LPDIRECT3D2 iface, LPDIRECT3DLIGHT *lplight, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lplight, lpunk);
    return E_FAIL;
}

HRESULT WINAPI IDirect3D2Impl_CreateMaterial(
    LPDIRECT3D2 iface, LPDIRECT3DMATERIAL2 *lpmaterial, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);
    return E_FAIL;
}

HRESULT WINAPI IDirect3D2Impl_CreateViewport(
    LPDIRECT3D2 iface, LPDIRECT3DVIEWPORT2 *lpviewport, IUnknown *lpunk
) {
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);
    return E_FAIL;
}

HRESULT WINAPI IDirect3D2Impl_FindDevice(
    LPDIRECT3D2 iface, LPD3DFINDDEVICESEARCH lpfinddevsrc,
    LPD3DFINDDEVICERESULT lpfinddevrst)
{
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);
    return DD_OK;
}

HRESULT WINAPI IDirect3D2Impl_CreateDevice(
    LPDIRECT3D2 iface, REFCLSID rguid, LPDIRECTDRAWSURFACE surface,
    LPDIRECT3DDEVICE2 *device)
{
    ICOM_THIS(IDirect3D2Impl,iface);
    FIXME("(%p)->(%s,%p,%p): stub\n",This,debugstr_guid(rguid),surface,device);
    return DDERR_INVALIDPARAMS;
}

/* This is for checking the correctness of the prototypes/functions.
 * Do not remove.
 */
static WINE_UNUSED ICOM_VTABLE(IDirect3D2) d3d2vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3D2Impl_QueryInterface,
    IDirect3D2Impl_AddRef,
    IDirect3D2Impl_Release,
    IDirect3D2Impl_EnumDevices,
    IDirect3D2Impl_CreateLight,
    IDirect3D2Impl_CreateMaterial,
    IDirect3D2Impl_CreateViewport,
    IDirect3D2Impl_FindDevice,
    IDirect3D2Impl_CreateDevice
};
