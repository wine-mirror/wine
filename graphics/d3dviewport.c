/* Direct3D Viewport
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DViewport2. */

#include "config.h"
#include "windows.h"
#include "wintypes.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"
#include "objbase.h"

#include "d3d_private.h"


#ifdef HAVE_MESAGL

static IDirect3DViewport2_VTable viewport2_vtable;

/*******************************************************************************
 *				Viewport1/2 static functions
 */
static void activate(LPDIRECT3DVIEWPORT2 this) {
  LPDIRECT3DLIGHT l;
  
  /* Activate all the lights associated with this context */
  l = this->lights;

  while (l != NULL) {
    l->activate(l);
    l = l->next;
  }
}

/*******************************************************************************
 *				Viewport1/2 Creation functions
 */
LPDIRECT3DVIEWPORT2 d3dviewport2_create(LPDIRECT3D2 d3d)
{
  LPDIRECT3DVIEWPORT2 vp;
  
  vp = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DViewport2));
  vp->ref = 1;
  vp->lpvtbl = &viewport2_vtable;
  vp->d3d.d3d2 = d3d;
  vp->use_d3d2 = 1;

  vp->device.active_device2 = NULL;
  vp->activate = activate;

  vp->lights = NULL;

  vp->nextlight = GL_LIGHT0;
  
  return vp;
}

LPDIRECT3DVIEWPORT d3dviewport_create(LPDIRECT3D d3d)
{
  LPDIRECT3DVIEWPORT2 vp;
  
  vp = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DViewport2));
  vp->ref = 1;
  vp->lpvtbl = &viewport2_vtable;
  vp->d3d.d3d1 = d3d;
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

static HRESULT WINAPI IDirect3DViewport2_QueryInterface(LPDIRECT3DVIEWPORT2 this,
							REFIID riid,
							LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DViewport2_AddRef(LPDIRECT3DVIEWPORT2 this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DViewport2_Release(LPDIRECT3DVIEWPORT2 this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

/*** IDirect3DViewport methods ***/
static HRESULT WINAPI IDirect3DViewport2_Initialize(LPDIRECT3DVIEWPORT2 this,
						    LPDIRECT3D d3d)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, d3d);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_GetViewport(LPDIRECT3DVIEWPORT2 this,
						     LPD3DVIEWPORT lpvp)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);
  
  if (this->use_vp2 != 0)
    return DDERR_INVALIDPARAMS;

  *lpvp = this->viewport.vp1;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_SetViewport(LPDIRECT3DVIEWPORT2 this,
						     LPD3DVIEWPORT lpvp)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);

  this->use_vp2 = 0;
  this->viewport.vp1 = *lpvp;
  
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

static HRESULT WINAPI IDirect3DViewport2_TransformVertices(LPDIRECT3DVIEWPORT2 this,
							   DWORD dwVertexCount,
							   LPD3DTRANSFORMDATA lpData,
							   DWORD dwFlags,
							   LPDWORD lpOffScreen)
{
  FIXME(ddraw, "(%p)->(%8ld,%p,%08lx,%p): stub\n",
	this, dwVertexCount, lpData, dwFlags, lpOffScreen);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_LightElements(LPDIRECT3DVIEWPORT2 this,
						       DWORD dwElementCount,
						       LPD3DLIGHTDATA lpData)
{
  FIXME(ddraw, "(%p)->(%8ld,%p): stub\n", this, dwElementCount, lpData);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_SetBackground(LPDIRECT3DVIEWPORT2 this,
						       D3DMATERIALHANDLE hMat)
{
  FIXME(ddraw, "(%p)->(%08lx): stub\n", this, (DWORD) hMat);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_GetBackground(LPDIRECT3DVIEWPORT2 this,
						       LPD3DMATERIALHANDLE lphMat,
						       LPBOOL32 lpValid)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lphMat, lpValid);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_SetBackgroundDepth(LPDIRECT3DVIEWPORT2 this,
							    LPDIRECTDRAWSURFACE lpDDSurface)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpDDSurface);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_GetBackgroundDepth(LPDIRECT3DVIEWPORT2 this,
							    LPDIRECTDRAWSURFACE* lplpDDSurface,
							    LPBOOL32 lpValid)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lplpDDSurface, lpValid);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_Clear(LPDIRECT3DVIEWPORT2 this,
					       DWORD dwCount,
					       LPD3DRECT lpRects,
					       DWORD dwFlags)
{
  GLboolean ztest;
  FIXME(ddraw, "(%p)->(%8ld,%p,%08lx): stub\n", this, dwCount, lpRects, dwFlags);

  /* For the moment, ignore the rectangles */
  if (this->device.active_device1 != NULL) {
    /* Get the rendering context */
    if (this->use_d3d2)
      this->device.active_device2->set_context(this->device.active_device2);
    else
      this->device.active_device1->set_context(this->device.active_device1);
  }

    /* Clears the screen */
    glGetBooleanv(GL_DEPTH_TEST, &ztest);
    glDepthMask(GL_TRUE); /* Enables Z writing to be sure to delete also the Z buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask(ztest);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_AddLight(LPDIRECT3DVIEWPORT2 this,
						  LPDIRECT3DLIGHT lpLight)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpLight);

  /* Add the light in the 'linked' chain */
  lpLight->next = this->lights;
  this->lights = lpLight;

  /* If active, activate the light */
  if (this->device.active_device1 != NULL) {
    /* Get the rendering context */
    if (this->use_d3d2)
      this->device.active_device2->set_context(this->device.active_device2);
    else
      this->device.active_device1->set_context(this->device.active_device1);
    
    /* Activate the light */
    lpLight->light_num = this->nextlight++;
    lpLight->activate(lpLight);
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_DeleteLight(LPDIRECT3DVIEWPORT2 this,
						     LPDIRECT3DLIGHT lpLight)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpLight);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_NextLight(LPDIRECT3DVIEWPORT2 this,
						   LPDIRECT3DLIGHT lpLight,
						   LPDIRECT3DLIGHT* lplpLight,
						   DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%p,%p,%08lx): stub\n", this, lpLight, lplpLight, dwFlags);
  
  return DD_OK;
}

/*** IDirect3DViewport2 methods ***/
static HRESULT WINAPI IDirect3DViewport2_GetViewport2(LPDIRECT3DVIEWPORT2 this,
						      LPD3DVIEWPORT2 lpViewport2)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpViewport2);

  if (this->use_vp2 != 1)
    return DDERR_INVALIDPARAMS;

  *lpViewport2 = this->viewport.vp2;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DViewport2_SetViewport2(LPDIRECT3DVIEWPORT2 this,
						      LPD3DVIEWPORT2 lpViewport2)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpViewport2);

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

  this->viewport.vp2 = *lpViewport2;
  this->use_vp2 = 1;
  
  return DD_OK;
}


/*******************************************************************************
 *				IDirect3DViewport1/2 VTable
 */
static IDirect3DViewport2_VTable viewport2_vtable = {
  /*** IUnknown methods ***/
  IDirect3DViewport2_QueryInterface,
  IDirect3DViewport2_AddRef,
  IDirect3DViewport2_Release,
  /*** IDirect3DViewport methods ***/
  IDirect3DViewport2_Initialize,
  IDirect3DViewport2_GetViewport,
  IDirect3DViewport2_SetViewport,
  IDirect3DViewport2_TransformVertices,
  IDirect3DViewport2_LightElements,
  IDirect3DViewport2_SetBackground,
  IDirect3DViewport2_GetBackground,
  IDirect3DViewport2_SetBackgroundDepth,
  IDirect3DViewport2_GetBackgroundDepth,
  IDirect3DViewport2_Clear,
  IDirect3DViewport2_AddLight,
  IDirect3DViewport2_DeleteLight,
  IDirect3DViewport2_NextLight,
  /*** IDirect3DViewport2 methods ***/
  IDirect3DViewport2_GetViewport2,
  IDirect3DViewport2_SetViewport2
};

#else /* HAVE_MESAGL */

LPDIRECT3DVIEWPORT d3dviewport_create(LPDIRECT3D d3d) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DVIEWPORT2 d3dviewport2_create(LPDIRECT3D2 d3d) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
