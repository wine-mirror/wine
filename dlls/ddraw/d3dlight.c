/* Direct3D Light
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DLight. */


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

#ifdef HAVE_MESAGL

static ICOM_VTABLE(IDirect3DLight) light_vtable;

enum {
  D3D_1,
  D3D_2
};

/*******************************************************************************
 *				Light static functions
 */
static const float zero_value[] = {
  0.0, 0.0, 0.0, 0.0
};

static void update(IDirect3DLightImpl* This) {
  switch (This->light.dltType) {
  case D3DLIGHT_POINT:         /* 1 */
    TRACE("Activating POINT\n");
    break;
    
  case D3DLIGHT_SPOT:          /* 2 */
    TRACE("Activating SPOT\n");
    break;
    
  case D3DLIGHT_DIRECTIONAL: {  /* 3 */
    float direction[4];
    
    TRACE("Activating DIRECTIONAL\n");
    TRACE("  direction : %f %f %f\n",
	  This->light.dvDirection.x.x,
	  This->light.dvDirection.y.y,
	  This->light.dvDirection.z.z);
    _dump_colorvalue(" color    ", This->light.dcvColor);

    glLightfv(This->light_num,
	      GL_AMBIENT,
	      (float *) zero_value);

    glLightfv(This->light_num,
	      GL_DIFFUSE,
	      (float *) &(This->light.dcvColor));

    direction[0] = -This->light.dvDirection.x.x;
    direction[1] = -This->light.dvDirection.y.y;
    direction[2] = -This->light.dvDirection.z.z;
    direction[3] = 0.0; /* This is a directional light */
    glLightfv(This->light_num,
	      GL_POSITION,
	      (float *) direction);
  } break;
    
  case D3DLIGHT_PARALLELPOINT:  /* 4 */
    TRACE("Activating PARRALLEL-POINT\n");
    break;

  default:
    TRACE("Not a know Light Type\n");
    break;
  }
}

static void activate(IDirect3DLightImpl* This) {
  ENTER_GL();
  update(This);

  /* If was not active, activate it */
  if (This->is_active == 0) {
    glEnable(This->light_num);

    This->is_active = 1;
  }
  LEAVE_GL();

  return ;
}

/*******************************************************************************
 *				Light Creation functions
 */
LPDIRECT3DLIGHT d3dlight_create(IDirect3D2Impl* d3d2)
{
  IDirect3DLightImpl* light;
  
  light = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DLightImpl));
  light->ref = 1;
  ICOM_VTBL(light) = &light_vtable;
  light->d3d.d3d2 = d3d2;
  light->type = D3D_2;

  light->next = NULL;
  light->prev = NULL;
  light->activate = activate;
  light->is_active = 0;
  
  return (LPDIRECT3DLIGHT)light;
}

LPDIRECT3DLIGHT d3dlight_create_dx3(IDirect3DImpl* d3d1)
{
  IDirect3DLightImpl* light;
  
  light = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DLightImpl));
  light->ref = 1;
  ICOM_VTBL(light) = &light_vtable;
  
  light->d3d.d3d1 = d3d1;
  light->type = D3D_1;
  
  light->next = NULL;
  light->prev = NULL;
  light->activate = activate;
  light->is_active = 0;
  
  return (LPDIRECT3DLIGHT)light;
}

/*******************************************************************************
 *				IDirect3DLight methods
 */

static HRESULT WINAPI IDirect3DLightImpl_QueryInterface(LPDIRECT3DLIGHT iface,
						    REFIID riid,
						    LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DLightImpl,iface);
  
  FIXME("(%p)->(%s,%p): stub\n", This, debugstr_guid(riid),ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DLightImpl_AddRef(LPDIRECT3DLIGHT iface)
{
  ICOM_THIS(IDirect3DLightImpl,iface);
  TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DLightImpl_Release(LPDIRECT3DLIGHT iface)
{
  ICOM_THIS(IDirect3DLightImpl,iface);
  FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );
  
  if (!--(This->ref)) {
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  
  return This->ref;
}

/*** IDirect3DLight methods ***/
static void dump_light(LPD3DLIGHT light)
{
  DPRINTF("  dwSize : %ld\n", light->dwSize);
}

static HRESULT WINAPI IDirect3DLightImpl_GetLight(LPDIRECT3DLIGHT iface,
					      LPD3DLIGHT lpLight)
{
  ICOM_THIS(IDirect3DLightImpl,iface);
  TRACE("(%p)->(%p)\n", This, lpLight);
  if (TRACE_ON(ddraw))
    dump_light(lpLight);
  
  /* Copies the light structure */
  switch (This->type) {
  case D3D_1:
    *((LPD3DLIGHT)lpLight) = *((LPD3DLIGHT) &(This->light));
    break;
  case D3D_2:
    *((LPD3DLIGHT2)lpLight) = *((LPD3DLIGHT2) &(This->light));
    break;
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DLightImpl_SetLight(LPDIRECT3DLIGHT iface,
					      LPD3DLIGHT lpLight)
{
  ICOM_THIS(IDirect3DLightImpl,iface);
  TRACE("(%p)->(%p)\n", This, lpLight);
  if (TRACE_ON(ddraw))
    dump_light(lpLight);
  
  /* Stores the light */
  switch (This->type) {
  case D3D_1:
    *((LPD3DLIGHT) &(This->light)) = *((LPD3DLIGHT)lpLight);
    break;
  case D3D_2:
    *((LPD3DLIGHT2) &(This->light)) = *((LPD3DLIGHT2)lpLight);
    break;
  }
  
  ENTER_GL();
  if (This->is_active)
    update(This);
  LEAVE_GL();
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DLightImpl_Initialize(LPDIRECT3DLIGHT iface,
						LPDIRECT3D lpDirect3D)

{
  ICOM_THIS(IDirect3DLightImpl,iface);
  TRACE("(%p)->(%p)\n", This, lpDirect3D);

  return DDERR_ALREADYINITIALIZED;
}


/*******************************************************************************
 *				IDirect3DLight VTable
 */
static ICOM_VTABLE(IDirect3DLight) light_vtable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  /*** IUnknown methods ***/
  IDirect3DLightImpl_QueryInterface,
  IDirect3DLightImpl_AddRef,
  IDirect3DLightImpl_Release,
  /*** IDirect3DLight methods ***/
  IDirect3DLightImpl_Initialize,
  IDirect3DLightImpl_SetLight,
  IDirect3DLightImpl_GetLight
};

#else /* HAVE_MESAGL */

/* These function should never be called if MesaGL is not present */
LPDIRECT3DLIGHT d3dlight_create_dx3(IDirect3DImpl* d3d1) {
  ERR("Should not be called...\n");
  return NULL;
}

LPDIRECT3DLIGHT d3dlight_create(IDirect3D2Impl* d3d2) {
  ERR("Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
