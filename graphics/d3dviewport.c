/* Direct3D Viewport
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DViewport2. */

#include "config.h"
#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"
#include "x11drv.h"

#include "d3d_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw)

#ifdef HAVE_MESAGL

static ICOM_VTABLE(IDirect3DViewport2) viewport2_vtable;

/*******************************************************************************
 *				Viewport1/2 static functions
 */
static void activate(IDirect3DViewport2Impl* This) {
  IDirect3DLightImpl* l;
  
  /* Activate all the lights associated with this context */
  l = This->lights;

  while (l != NULL) {
    l->activate(l);
    l = l->next;
  }
}

/*******************************************************************************
 *				Viewport1/2 Creation functions
 */
LPDIRECT3DVIEWPORT2 d3dviewport2_create(IDirect3D2Impl* d3d2)
{
  IDirect3DViewport2Impl* vp;
  
  vp = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DViewport2Impl));
  vp->ref = 1;
  vp->lpvtbl = &viewport2_vtable;
  vp->d3d.d3d2 = d3d2;
  vp->use_d3d2 = 1;

  vp->device.active_device2 = NULL;
  vp->activate = activate;

  vp->lights = NULL;

  vp->nextlight = GL_LIGHT0;
  
  return (LPDIRECT3DVIEWPORT2)vp;
}

LPDIRECT3DVIEWPORT d3dviewport_create(IDirect3DImpl* d3d1)
{
  IDirect3DViewport2Impl* vp;
  
  vp = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DViewport2Impl));
  vp->ref = 1;
  vp->lpvtbl = &viewport2_vtable;
  vp->d3d.d3d1 = d3d1;
  vp->use_d3d2 = 0;

  vp->device.active_device1 = NULL;
  vp->activate = activate;

  vp->lights = NULL;

  vp->nextlight = GL_LIGHT0;
  
  return (LPDIRECT3DVIEWPORT) vp;
}

/*******************************************************************************
 *				IDirect3DViewport2 methods
 */

static HRESULT WINAPI IDirect3DViewport2Impl_QueryInterface(LPDIRECT3DVIEWPORT2 iface,
							REFIID riid,
							LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", This, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DViewport2Impl_AddRef(LPDIRECT3DVIEWPORT2 iface)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DViewport2Impl_Release(LPDIRECT3DVIEWPORT2 iface)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );
  
  if (!--(This->ref)) {
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  
  return This->ref;
}

/*** IDirect3DViewport methods ***/
static HRESULT WINAPI IDirect3DViewport2Impl_Initialize(LPDIRECT3DVIEWPORT2 iface,
						    LPDIRECT3D d3d)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p): stub\n", This, d3d);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_GetViewport(LPDIRECT3DVIEWPORT2 iface,
						     LPD3DVIEWPORT lpvp)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p): stub\n", This, lpvp);
  
  if (This->use_vp2 != 0)
    return DDERR_INVALIDPARAMS;

  *lpvp = This->viewport.vp1;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_SetViewport(LPDIRECT3DVIEWPORT2 iface,
						     LPD3DVIEWPORT lpvp)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p): stub\n", This, lpvp);

  This->use_vp2 = 0;
  This->viewport.vp1 = *lpvp;
  
  TRACE(ddraw, "dwSize = %ld   dwX = %ld   dwY = %ld\n",
	lpvp->dwSize, lpvp->dwX, lpvp->dwY);
  TRACE(ddraw, "dwWidth = %ld   dwHeight = %ld\n",
	lpvp->dwWidth, lpvp->dwHeight);
  TRACE(ddraw, "dvScaleX = %f   dvScaleY = %f\n",
	lpvp->dvScaleX, lpvp->dvScaleY);
  TRACE(ddraw, "dvMaxX = %f   dvMaxY = %f\n",
	lpvp->dvMaxX, lpvp->dvMaxY);
  TRACE(ddraw, "dvMinZ = %f   dvMaxZ = %f\n",
	lpvp->dvMinZ, lpvp->dvMaxZ);

  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_TransformVertices(LPDIRECT3DVIEWPORT2 iface,
							   DWORD dwVertexCount,
							   LPD3DTRANSFORMDATA lpData,
							   DWORD dwFlags,
							   LPDWORD lpOffScreen)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%8ld,%p,%08lx,%p): stub\n",
	This, dwVertexCount, lpData, dwFlags, lpOffScreen);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_LightElements(LPDIRECT3DVIEWPORT2 iface,
						       DWORD dwElementCount,
						       LPD3DLIGHTDATA lpData)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%8ld,%p): stub\n", This, dwElementCount, lpData);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_SetBackground(LPDIRECT3DVIEWPORT2 iface,
						       D3DMATERIALHANDLE hMat)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%08lx): stub\n", This, (DWORD) hMat);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_GetBackground(LPDIRECT3DVIEWPORT2 iface,
						       LPD3DMATERIALHANDLE lphMat,
						       LPBOOL lpValid)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", This, lphMat, lpValid);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_SetBackgroundDepth(LPDIRECT3DVIEWPORT2 iface,
							    LPDIRECTDRAWSURFACE lpDDSurface)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p): stub\n", This, lpDDSurface);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_GetBackgroundDepth(LPDIRECT3DVIEWPORT2 iface,
							    LPDIRECTDRAWSURFACE* lplpDDSurface,
							    LPBOOL lpValid)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", This, lplpDDSurface, lpValid);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_Clear(LPDIRECT3DVIEWPORT2 iface,
					       DWORD dwCount,
					       LPD3DRECT lpRects,
					       DWORD dwFlags)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  GLboolean ztest;
  FIXME(ddraw, "(%p)->(%8ld,%p,%08lx): stub\n", This, dwCount, lpRects, dwFlags);

  /* For the moment, ignore the rectangles */
  if (This->device.active_device1 != NULL) {
    /* Get the rendering context */
    if (This->use_d3d2)
      This->device.active_device2->set_context(This->device.active_device2);
    else
      This->device.active_device1->set_context(This->device.active_device1);
  }

    /* Clears the screen */
    ENTER_GL();
    glGetBooleanv(GL_DEPTH_TEST, &ztest);
    glDepthMask(GL_TRUE); /* Enables Z writing to be sure to delete also the Z buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask(ztest);
    LEAVE_GL();
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_AddLight(LPDIRECT3DVIEWPORT2 iface,
						  LPDIRECT3DLIGHT lpLight)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  IDirect3DLightImpl* ilpLight=(IDirect3DLightImpl*)lpLight;
  FIXME(ddraw, "(%p)->(%p): stub\n", This, ilpLight);

  /* Add the light in the 'linked' chain */
  ilpLight->next = This->lights;
  This->lights = ilpLight;

  /* If active, activate the light */
  if (This->device.active_device1 != NULL) {
    /* Get the rendering context */
    if (This->use_d3d2)
      This->device.active_device2->set_context(This->device.active_device2);
    else
      This->device.active_device1->set_context(This->device.active_device1);
    
    /* Activate the light */
    ilpLight->light_num = This->nextlight++;
    ilpLight->activate(ilpLight);
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_DeleteLight(LPDIRECT3DVIEWPORT2 iface,
						     LPDIRECT3DLIGHT lpLight)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p): stub\n", This, lpLight);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_NextLight(LPDIRECT3DVIEWPORT2 iface,
						   LPDIRECT3DLIGHT lpLight,
						   LPDIRECT3DLIGHT* lplpLight,
						   DWORD dwFlags)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  FIXME(ddraw, "(%p)->(%p,%p,%08lx): stub\n", This, lpLight, lplpLight, dwFlags);
  
  return DD_OK;
}

/*** IDirect3DViewport2 methods ***/
static HRESULT WINAPI IDirect3DViewport2Impl_GetViewport2(LPDIRECT3DVIEWPORT2 iface,
						      LPD3DVIEWPORT2 lpViewport2)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  TRACE(ddraw, "(%p)->(%p)\n", This, lpViewport2);

  if (This->use_vp2 != 1)
    return DDERR_INVALIDPARAMS;

  *lpViewport2 = This->viewport.vp2;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2Impl_SetViewport2(LPDIRECT3DVIEWPORT2 iface,
						      LPD3DVIEWPORT2 lpViewport2)
{
  ICOM_THIS(IDirect3DViewport2Impl,iface);
  TRACE(ddraw, "(%p)->(%p)\n", This, lpViewport2);

  TRACE(ddraw, "dwSize = %ld   dwX = %ld   dwY = %ld\n",
	lpViewport2->dwSize, lpViewport2->dwX, lpViewport2->dwY);
  TRACE(ddraw, "dwWidth = %ld   dwHeight = %ld\n",
	lpViewport2->dwWidth, lpViewport2->dwHeight);
  TRACE(ddraw, "dvClipX = %f   dvClipY = %f\n",
	lpViewport2->dvClipX, lpViewport2->dvClipY);
  TRACE(ddraw, "dvClipWidth = %f   dvClipHeight = %f\n",
	lpViewport2->dvClipWidth, lpViewport2->dvClipHeight);
  TRACE(ddraw, "dvMinZ = %f   dvMaxZ = %f\n",
	lpViewport2->dvMinZ, lpViewport2->dvMaxZ);

  This->viewport.vp2 = *lpViewport2;
  This->use_vp2 = 1;
  
  return DD_OK;
}


/*******************************************************************************
 *				IDirect3DViewport1/2 VTable
 */
static ICOM_VTABLE(IDirect3DViewport2) viewport2_vtable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  /*** IUnknown methods ***/
  IDirect3DViewport2Impl_QueryInterface,
  IDirect3DViewport2Impl_AddRef,
  IDirect3DViewport2Impl_Release,
  /*** IDirect3DViewport methods ***/
  IDirect3DViewport2Impl_Initialize,
  IDirect3DViewport2Impl_GetViewport,
  IDirect3DViewport2Impl_SetViewport,
  IDirect3DViewport2Impl_TransformVertices,
  IDirect3DViewport2Impl_LightElements,
  IDirect3DViewport2Impl_SetBackground,
  IDirect3DViewport2Impl_GetBackground,
  IDirect3DViewport2Impl_SetBackgroundDepth,
  IDirect3DViewport2Impl_GetBackgroundDepth,
  IDirect3DViewport2Impl_Clear,
  IDirect3DViewport2Impl_AddLight,
  IDirect3DViewport2Impl_DeleteLight,
  IDirect3DViewport2Impl_NextLight,
  /*** IDirect3DViewport2 methods ***/
  IDirect3DViewport2Impl_GetViewport2,
  IDirect3DViewport2Impl_SetViewport2
};

#else /* HAVE_MESAGL */

LPDIRECT3DVIEWPORT d3dviewport_create(IDirect3DImpl* d3d1) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DVIEWPORT2 d3dviewport2_create(IDirect3D2Impl* d3d2) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
