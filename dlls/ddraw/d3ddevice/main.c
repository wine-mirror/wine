/* Direct3D Device
   (c) 1998 Lionel ULMER
   
   This files contains all the common stuff for D3D devices.
 */

#include <string.h>
#include "config.h"
#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"

#include "d3d_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw)

/*******************************************************************************
 *				IDirect3DDevice2
 */

HRESULT WINAPI IDirect3DDevice2Impl_QueryInterface(
    LPDIRECT3DDEVICE2 iface, REFIID riid, LPVOID* ppvObj
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%s,%p): stub\n", This, debugstr_guid(riid),ppvObj);
    return S_OK;
}

ULONG WINAPI IDirect3DDevice2Impl_AddRef(LPDIRECT3DDEVICE2 iface)
{
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );

    return ++(This->ref);
}



ULONG WINAPI IDirect3DDevice2Impl_Release(LPDIRECT3DDEVICE2 iface)
{
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }
    return This->ref;
}


/*** IDirect3DDevice2 methods ***/
HRESULT WINAPI IDirect3DDevice2Impl_GetCaps(
    LPDIRECT3DDEVICE2 iface, LPD3DDEVICEDESC lpdescsoft,
    LPD3DDEVICEDESC lpdeschard
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p,%p), stub!\n", This, lpdescsoft, lpdeschard);
    return DD_OK;
}



HRESULT WINAPI IDirect3DDevice2Impl_SwapTextureHandles(
    LPDIRECT3DDEVICE2 iface,LPDIRECT3DTEXTURE2 lptex1,LPDIRECT3DTEXTURE2 lptex2
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p,%p): stub\n", This, lptex1, lptex2);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_GetStats(
    LPDIRECT3DDEVICE2 iface, LPD3DSTATS lpstats)
{
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, lpstats);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_AddViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    FIXME("(%p)->(%p): stub\n", This, ilpvp);

    /* Adds this viewport to the viewport list */
    ilpvp->next = This->viewport_list;
    This->viewport_list = ilpvp;

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_DeleteViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp)
{
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    IDirect3DViewport2Impl *cur, *prev;
    FIXME("(%p)->(%p): stub\n", This, lpvp);

    /* Finds this viewport in the list */
    prev = NULL;
    cur = This->viewport_list;
    while ((cur != NULL) && (cur != ilpvp)) {
	prev = cur;
	cur = cur->next;
    }
    if (cur == NULL)
	return DDERR_INVALIDOBJECT;

    /* And remove it */
    if (prev == NULL)
	This->viewport_list = cur->next;
    else
	prev->next = cur->next;
    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_NextViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp,
    LPDIRECT3DVIEWPORT2* lplpvp, DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    IDirect3DViewport2Impl** ilplpvp=(IDirect3DViewport2Impl**)lplpvp;
    FIXME("(%p)->(%p,%p,%08lx): stub\n", This, lpvp, lpvp, dwFlags);

    switch (dwFlags) {
    case D3DNEXT_NEXT:
	*ilplpvp = ilpvp->next;
	break;
    case D3DNEXT_HEAD:
	*ilplpvp = This->viewport_list;
	break;
    case D3DNEXT_TAIL:
	ilpvp = This->viewport_list;
	while (ilpvp->next != NULL)
	    ilpvp = ilpvp->next;
	*ilplpvp = ilpvp;
	break;
    default:
	return DDERR_INVALIDPARAMS;
    }
    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_EnumTextureFormats(
    LPDIRECT3DDEVICE2 iface, LPD3DENUMTEXTUREFORMATSCALLBACK cb, LPVOID context
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p,%p), stub!\n", This, cb, context);

    return DD_OK; /* no texture formats in stub implementation */
}



HRESULT WINAPI IDirect3DDevice2Impl_BeginScene(LPDIRECT3DDEVICE2 iface)
{
    ICOM_THIS(IDirect3DDevice2Impl,iface);

    FIXME("(%p)->(), stub!\n", This);

    /* Here, we should get the DDraw surface and 'copy it' to the
     OpenGL surface.... */
    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_EndScene(LPDIRECT3DDEVICE2 iface)
{
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(): stub\n", This);
    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_GetDirect3D(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3D2 *lpd3d2
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    TRACE("(%p)->(%p): stub\n", This, lpd3d2);
    *lpd3d2 = (LPDIRECT3D2)This->d3d;
    return DD_OK;
}

/*** DrawPrimitive API ***/
HRESULT WINAPI IDirect3DDevice2Impl_SetCurrentViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    FIXME("(%p)->(%p): stub\n", This, ilpvp);

    /* Should check if the viewport was added or not */

    /* Set this viewport as the current viewport */
    This->current_viewport = ilpvp;

    /* Activate this viewport */
    ilpvp->device.active_device2 = This;
    ilpvp->activate(ilpvp);

    return DD_OK;
}



HRESULT WINAPI IDirect3DDevice2Impl_GetCurrentViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 *lplpvp
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, lplpvp);

    /* Returns the current viewport */
    *lplpvp = (LPDIRECT3DVIEWPORT2)This->current_viewport;

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_SetRenderTarget(
    LPDIRECT3DDEVICE2 iface, LPDIRECTDRAWSURFACE lpdds, DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p,%08lx): stub\n", This, lpdds, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_GetRenderTarget(
    LPDIRECT3DDEVICE2 iface, LPDIRECTDRAWSURFACE *lplpdds
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, lplpdds);

    /* Returns the current rendering target (the surface on wich we render) */
    *lplpdds = (LPDIRECTDRAWSURFACE)This->surface;

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_Begin(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%d,%08lx): stub\n", This, d3dp, d3dv, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_BeginIndexed(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvert, DWORD numvert, DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%d,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvert, numvert, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_Vertex(
    LPDIRECT3DDEVICE2 iface,LPVOID lpvert
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, lpvert);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_Index(LPDIRECT3DDEVICE2 iface, WORD index) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d): stub\n", This, index);

    return DD_OK;
}



HRESULT WINAPI IDirect3DDevice2Impl_End(LPDIRECT3DDEVICE2 iface,DWORD dwFlags) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%08lx): stub\n", This, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_GetRenderState(
    LPDIRECT3DDEVICE2 iface, D3DRENDERSTATETYPE d3drs, LPDWORD lprstate
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%p): stub\n", This, d3drs, lprstate);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_SetRenderState(
    LPDIRECT3DDEVICE2 iface, D3DRENDERSTATETYPE dwRenderStateType,
    DWORD dwRenderState
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);

    FIXME("(%p)->(%d,%ld)\n", This, dwRenderStateType, dwRenderState);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_GetLightState(
    LPDIRECT3DDEVICE2 iface, D3DLIGHTSTATETYPE d3dls, LPDWORD lplstate
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%p): stub\n", This, d3dls, lplstate);

    return DD_OK;
}



HRESULT WINAPI IDirect3DDevice2Impl_SetLightState(
    LPDIRECT3DDEVICE2 iface, D3DLIGHTSTATETYPE dwLightStateType,
    DWORD dwLightState
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%08lx): stub\n", This, dwLightStateType, dwLightState);
    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_SetTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts, LPD3DMATRIX lpmatrix
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%p),stub!\n",This,d3dts,lpmatrix);
    return DD_OK;
}



HRESULT WINAPI IDirect3DDevice2Impl_GetTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts, LPD3DMATRIX lpmatrix
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%p): stub\n", This, d3dts, lpmatrix);

    return DD_OK;
}



HRESULT WINAPI IDirect3DDevice2Impl_MultiplyTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts, LPD3DMATRIX lpmatrix
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%d,%p): stub\n", This, d3dts, lpmatrix);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_DrawPrimitive(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvertex, DWORD vertcount, DWORD dwFlags
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  
  TRACE("(%p)->(%d,%d,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvertex, vertcount, dwFlags);

  return D3D_OK;
}
    
HRESULT WINAPI IDirect3DDevice2Impl_DrawIndexedPrimitive(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvertex, DWORD vertcount, LPWORD lpindexes, DWORD indexcount,
    DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    TRACE("(%p)->(%d,%d,%p,%ld,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvertex, vertcount, lpindexes, indexcount, dwFlags);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_SetClipStatus(
    LPDIRECT3DDEVICE2 iface, LPD3DCLIPSTATUS lpcs
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, lpcs);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDevice2Impl_GetClipStatus(
    LPDIRECT3DDEVICE2 iface, LPD3DCLIPSTATUS lpcs
) {
    ICOM_THIS(IDirect3DDevice2Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, lpcs);

    return DD_OK;
}

/*******************************************************************************
 *				Direct3DDevice
 */
HRESULT WINAPI IDirect3DDeviceImpl_QueryInterface(
    LPDIRECT3DDEVICE iface, REFIID riid, LPVOID* ppvObj
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    FIXME("(%p)->(%s,%p): stub\n", This, debugstr_guid(riid),ppvObj);
    return S_OK;
}

ULONG WINAPI IDirect3DDeviceImpl_AddRef(LPDIRECT3DDEVICE iface)
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );

    return ++(This->ref);
}

ULONG WINAPI IDirect3DDeviceImpl_Release(LPDIRECT3DDEVICE iface)
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }
    return This->ref;
}

HRESULT WINAPI IDirect3DDeviceImpl_Initialize(
    LPDIRECT3DDEVICE iface, LPDIRECT3D lpd3d, LPGUID lpGUID,
    LPD3DDEVICEDESC lpd3ddvdesc
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p,%p): stub\n", This, lpd3d,lpGUID, lpd3ddvdesc);

    return DDERR_ALREADYINITIALIZED;
}


HRESULT WINAPI IDirect3DDeviceImpl_GetCaps(
    LPDIRECT3DDEVICE iface, LPD3DDEVICEDESC lpD3DHWDevDesc,
    LPD3DDEVICEDESC lpD3DSWDevDesc
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpD3DHWDevDesc, lpD3DSWDevDesc);

    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_SwapTextureHandles(
    LPDIRECT3DDEVICE iface, LPDIRECT3DTEXTURE lpD3DTex1,
    LPDIRECT3DTEXTURE lpD3DTex2
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpD3DTex1, lpD3DTex2);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_CreateExecuteBuffer(
    LPDIRECT3DDEVICE iface, LPD3DEXECUTEBUFFERDESC lpDesc,
    LPDIRECT3DEXECUTEBUFFER *lplpDirect3DExecuteBuffer, IUnknown *pUnkOuter
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    FIXME("(%p)->(%p,%p,%p): stub\n", This, lpDesc, lplpDirect3DExecuteBuffer, pUnkOuter);
    return DD_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_GetStats(
    LPDIRECT3DDEVICE iface, LPD3DSTATS lpD3DStats
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p): stub\n", This, lpD3DStats);

    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_Execute(
    LPDIRECT3DDEVICE iface, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
    LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p,%08ld): stub\n", This, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags);

    /* Put this as the default context */

    /* Execute... */
    ((IDirect3DExecuteBufferImpl*)lpDirect3DExecuteBuffer)->execute(lpDirect3DExecuteBuffer, iface, (IDirect3DViewport2*)lpDirect3DViewport);

    return DD_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_AddViewport(
    LPDIRECT3DDEVICE iface, LPDIRECT3DVIEWPORT lpvp
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    FIXME("(%p)->(%p): stub\n", This, ilpvp);

    /* Adds this viewport to the viewport list */
    ilpvp->next = This->viewport_list;
    This->viewport_list = ilpvp;

    return DD_OK;
}



HRESULT WINAPI IDirect3DDeviceImpl_DeleteViewport(
    LPDIRECT3DDEVICE iface, LPDIRECT3DVIEWPORT lpvp
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    IDirect3DViewport2Impl *cur, *prev;
    FIXME("(%p)->(%p): stub\n", This, lpvp);

    /* Finds this viewport in the list */
    prev = NULL;
    cur = This->viewport_list;
    while ((cur != NULL) && (cur != ilpvp)) {
	prev = cur;
	cur = cur->next;
    }
    if (cur == NULL)
	return DDERR_INVALIDOBJECT;

    /* And remove it */
    if (prev == NULL)
	This->viewport_list = cur->next;
    else
	prev->next = cur->next;
    return DD_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_NextViewport(
    LPDIRECT3DDEVICE iface, LPDIRECT3DVIEWPORT lpvp,
    LPDIRECT3DVIEWPORT* lplpvp, DWORD dwFlags
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
    IDirect3DViewport2Impl** ilplpvp=(IDirect3DViewport2Impl**)lplpvp;
    FIXME("(%p)->(%p,%p,%08lx): stub\n", This, ilpvp, ilplpvp, dwFlags);

    switch (dwFlags) {
    case D3DNEXT_NEXT:
	*ilplpvp = ilpvp->next;
	break;
    case D3DNEXT_HEAD:
	*ilplpvp = This->viewport_list;
	break;
    case D3DNEXT_TAIL:
	ilpvp = This->viewport_list;
	while (ilpvp->next != NULL)
	    ilpvp = ilpvp->next;
	*ilplpvp = ilpvp;
	break;
    default:
	return DDERR_INVALIDPARAMS;
    }
    return DD_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_Pick(
    LPDIRECT3DDEVICE iface, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
    LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags, LPD3DRECT lpRect
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p,%08lx,%p): stub\n", This, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags, lpRect);
    return DD_OK;
}

HRESULT WINAPI IDirect3DDeviceImpl_GetPickRecords(
    LPDIRECT3DDEVICE iface, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpCount, lpD3DPickRec);

    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_EnumTextureFormats(
    LPDIRECT3DDEVICE iface,LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc,
    LPVOID lpArg
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p): stub\n", This, lpd3dEnumTextureProc, lpArg);
    return D3D_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_CreateMatrix(
    LPDIRECT3DDEVICE iface, LPD3DMATRIXHANDLE lpD3DMatHandle
)
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p)\n", This, lpD3DMatHandle);

    *lpD3DMatHandle = (D3DMATRIXHANDLE) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(D3DMATRIX));

    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_SetMatrix(
    LPDIRECT3DDEVICE iface, D3DMATRIXHANDLE d3dMatHandle,
    const LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%08lx,%p)\n", This, d3dMatHandle, lpD3DMatrix);

    dump_mat(lpD3DMatrix);
    *((D3DMATRIX *) d3dMatHandle) = *lpD3DMatrix;
    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_GetMatrix(
    LPDIRECT3DDEVICE iface,D3DMATRIXHANDLE D3DMatHandle,LPD3DMATRIX lpD3DMatrix
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%08lx,%p)\n", This, D3DMatHandle, lpD3DMatrix);

    *lpD3DMatrix = *((D3DMATRIX *) D3DMatHandle);

    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_DeleteMatrix(
    LPDIRECT3DDEVICE iface, D3DMATRIXHANDLE d3dMatHandle
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%08lx)\n", This, d3dMatHandle);
    HeapFree(GetProcessHeap(),0, (void *) d3dMatHandle);
    return DD_OK;
}


HRESULT WINAPI IDirect3DDeviceImpl_BeginScene(LPDIRECT3DDEVICE iface)
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    FIXME("(%p)->(): stub\n", This);
    return DD_OK;
}

/* This is for the moment copy-pasted from IDirect3DDevice2...
   Will make a common function ... */
HRESULT WINAPI IDirect3DDeviceImpl_EndScene(LPDIRECT3DDEVICE iface)
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    FIXME("(%p)->(): stub\n", This);
    return DD_OK;  
}

HRESULT WINAPI IDirect3DDeviceImpl_GetDirect3D(
    LPDIRECT3DDEVICE iface, LPDIRECT3D *lpDirect3D
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p): stub\n", This, lpDirect3D);

    return DD_OK;
}

/*******************************************************************************
 *				Direct3DDevice VTable
 */
static ICOM_VTABLE(IDirect3DDevice) WINE_UNUSED d3d_d3ddevice_vtbl = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirect3DDeviceImpl_QueryInterface,
  IDirect3DDeviceImpl_AddRef,
  IDirect3DDeviceImpl_Release,
  IDirect3DDeviceImpl_Initialize,
  IDirect3DDeviceImpl_GetCaps,
  IDirect3DDeviceImpl_SwapTextureHandles,
  IDirect3DDeviceImpl_CreateExecuteBuffer,
  IDirect3DDeviceImpl_GetStats,
  IDirect3DDeviceImpl_Execute,
  IDirect3DDeviceImpl_AddViewport,
  IDirect3DDeviceImpl_DeleteViewport,
  IDirect3DDeviceImpl_NextViewport,
  IDirect3DDeviceImpl_Pick,
  IDirect3DDeviceImpl_GetPickRecords,
  IDirect3DDeviceImpl_EnumTextureFormats,
  IDirect3DDeviceImpl_CreateMatrix,
  IDirect3DDeviceImpl_SetMatrix,
  IDirect3DDeviceImpl_GetMatrix,
  IDirect3DDeviceImpl_DeleteMatrix,
  IDirect3DDeviceImpl_BeginScene,
  IDirect3DDeviceImpl_EndScene,
  IDirect3DDeviceImpl_GetDirect3D,
};
