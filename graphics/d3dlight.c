/* Direct3D Light
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DLight. */


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

static IDirect3DLight_VTable light_vtable;

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

static void update(LPDIRECT3DLIGHT this) {
  switch (this->light.dltType) {
  case D3DLIGHT_POINT:         /* 1 */
    TRACE(ddraw, "Activating POINT\n");
    break;
    
  case D3DLIGHT_SPOT:          /* 2 */
    TRACE(ddraw, "Activating SPOT\n");
    break;
    
  case D3DLIGHT_DIRECTIONAL: {  /* 3 */
    float direction[4];
    
    TRACE(ddraw, "Activating DIRECTIONAL\n");
    TRACE(ddraw, "  direction : %f %f %f\n",
	  this->light.dvDirection.x.x,
	  this->light.dvDirection.y.y,
	  this->light.dvDirection.z.z);
    _dump_colorvalue(" color    ", this->light.dcvColor);

    glLightfv(this->light_num,
	      GL_AMBIENT,
	      (float *) zero_value);

    glLightfv(this->light_num,
	      GL_DIFFUSE,
	      (float *) &(this->light.dcvColor));

    direction[0] = -this->light.dvDirection.x.x;
    direction[1] = -this->light.dvDirection.y.y;
    direction[2] = -this->light.dvDirection.z.z;
    direction[3] = 0.0; /* This is a directional light */
    glLightfv(this->light_num,
	      GL_POSITION,
	      (float *) direction);
  } break;
    
  case D3DLIGHT_PARALLELPOINT:  /* 4 */
    TRACE(ddraw, "Activating PARRALLEL-POINT\n");
    break;

  default:
    TRACE(ddraw, "Not a know Light Type\n");
    break;
  }
}

static void activate(LPDIRECT3DLIGHT this) {
  update(this);

  /* If was not active, activate it */
  if (this->is_active == 0) {
    glEnable(this->light_num);

    this->is_active = 1;
  }

  return ;
}

/*******************************************************************************
 *				Light Creation functions
 */
LPDIRECT3DLIGHT d3dlight_create(LPDIRECT3D2 d3d)
{
  LPDIRECT3DLIGHT mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DLight));
  mat->ref = 1;
  mat->lpvtbl = &light_vtable;
  mat->d3d.d3d2 = d3d;
  mat->type = D3D_2;

  mat->next = NULL;
  mat->prev = NULL;
  mat->activate = activate;
  mat->is_active = 0;
  
  return mat;
}

LPDIRECT3DLIGHT d3dlight_create_dx3(LPDIRECT3D d3d)
{
  LPDIRECT3DLIGHT mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DLight));
  mat->ref = 1;
  mat->lpvtbl = &light_vtable;
  
  mat->d3d.d3d = d3d;
  mat->type = D3D_1;
  
  mat->next = NULL;
  mat->prev = NULL;
  mat->activate = activate;
  mat->is_active = 0;
  
  return mat;
}

/*******************************************************************************
 *				IDirect3DLight methods
 */

static HRESULT WINAPI IDirect3DLight_QueryInterface(LPDIRECT3DLIGHT this,
						    REFIID riid,
						    LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DLight_AddRef(LPDIRECT3DLIGHT this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DLight_Release(LPDIRECT3DLIGHT this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

/*** IDirect3DLight methods ***/
static void dump_light(LPD3DLIGHT light)
{
  fprintf(stderr, "  dwSize : %ld\n", light->dwSize);
}

static HRESULT WINAPI IDirect3DLight_GetLight(LPDIRECT3DLIGHT this,
					      LPD3DLIGHT lpLight)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpLight);
  if (TRACE_ON(ddraw))
    dump_light(lpLight);
  
  /* Copies the light structure */
  switch (this->type) {
  case D3D_1:
    *((LPD3DLIGHT)lpLight) = *((LPD3DLIGHT) &(this->light));
    break;
  case D3D_2:
    *((LPD3DLIGHT2)lpLight) = *((LPD3DLIGHT2) &(this->light));
    break;
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DLight_SetLight(LPDIRECT3DLIGHT this,
					      LPD3DLIGHT lpLight)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpLight);
  if (TRACE_ON(ddraw))
    dump_light(lpLight);
  
  /* Stores the light */
  switch (this->type) {
  case D3D_1:
    *((LPD3DLIGHT) &(this->light)) = *((LPD3DLIGHT)lpLight);
    break;
  case D3D_2:
    *((LPD3DLIGHT2) &(this->light)) = *((LPD3DLIGHT2)lpLight);
    break;
  }
  
  if (this->is_active)
    update(this);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DLight_Initialize(LPDIRECT3DLIGHT this,
						LPDIRECT3D lpDirect3D)

{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpDirect3D);

  return DDERR_ALREADYINITIALIZED;
}


/*******************************************************************************
 *				IDirect3DLight VTable
 */
static IDirect3DLight_VTable light_vtable = {
  /*** IUnknown methods ***/
  IDirect3DLight_QueryInterface,
  IDirect3DLight_AddRef,
  IDirect3DLight_Release,
  /*** IDirect3DLight methods ***/
  IDirect3DLight_Initialize,
  IDirect3DLight_SetLight,
  IDirect3DLight_GetLight
};

#else /* HAVE_MESAGL */

/* These function should never be called if MesaGL is not present */
LPDIRECT3DLIGHT d3dlight_create_dx3(LPDIRECT3D d3d) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DLIGHT d3dlight_create(LPDIRECT3D2 d3d) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
