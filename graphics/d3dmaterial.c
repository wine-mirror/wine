/* Direct3D Material
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DMaterial2. */

#include "config.h"
#include "wintypes.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"

#include "d3d_private.h"

#ifdef HAVE_MESAGL

static IDirect3DMaterial2_VTable material2_vtable;
static IDirect3DMaterial_VTable material_vtable;

/*******************************************************************************
 *				Matrial2 static functions
 */
static void activate(LPDIRECT3DMATERIAL2 this) {
  TRACE(ddraw, "Activating material %p\n", this);
  
  /* First, set the rendering context */
  if (this->use_d3d2)
    this->device.active_device2->set_context(this->device.active_device2);
  else
    this->device.active_device1->set_context(this->device.active_device1);

  /* Set the current Material */
  _dump_colorvalue("Diffuse", this->mat.a.diffuse);
  glMaterialfv(GL_FRONT,
	       GL_DIFFUSE,
	       (float *) &(this->mat.a.diffuse));
  _dump_colorvalue("Ambient", this->mat.b.ambient);
  glMaterialfv(GL_FRONT,
	       GL_AMBIENT,
	       (float *) &(this->mat.b.ambient));
  _dump_colorvalue("Specular", this->mat.c.specular);
  glMaterialfv(GL_FRONT,
	       GL_SPECULAR,
	       (float *) &(this->mat.c.specular));
  _dump_colorvalue("Emissive", this->mat.d.emissive);
  glMaterialfv(GL_FRONT,
	       GL_EMISSION,
	       (float *) &(this->mat.d.emissive));
  
  TRACE(ddraw, "Size  : %ld\n", this->mat.dwSize);
  TRACE(ddraw, "Power : %f\n", this->mat.e.power);

  TRACE(ddraw, "Texture handle : %08lx\n", (DWORD)this->mat.hTexture);
  
  return ;
}

/*******************************************************************************
 *				Matrial2 Creation functions
 */
LPDIRECT3DMATERIAL2 d3dmaterial2_create(LPDIRECT3D2 d3d)
{
  LPDIRECT3DMATERIAL2 mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DMaterial2));
  mat->ref = 1;
  mat->lpvtbl = &material2_vtable;

  mat->use_d3d2 = 1;
  mat->d3d.d3d2 = d3d;

  mat->activate = activate;
  
  return mat;
}

LPDIRECT3DMATERIAL d3dmaterial_create(LPDIRECT3D d3d)
{
  LPDIRECT3DMATERIAL2 mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DMaterial2));
  mat->ref = 1;
  mat->lpvtbl = (LPDIRECT3DMATERIAL2_VTABLE) &material_vtable;

  mat->use_d3d2 = 0;
  mat->d3d.d3d1 = d3d;

  mat->activate = activate;
  
  return (LPDIRECT3DMATERIAL) mat;
}

/*******************************************************************************
 *				IDirect3DMaterial2 methods
 */

static HRESULT WINAPI IDirect3DMaterial2_QueryInterface(LPDIRECT3DMATERIAL2 this,
							REFIID riid,
							LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DMaterial2_AddRef(LPDIRECT3DMATERIAL2 this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DMaterial2_Release(LPDIRECT3DMATERIAL2 this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

/*** IDirect3DMaterial2 methods ***/
static void dump_material(LPD3DMATERIAL mat)
{
  fprintf(stderr, "  dwSize : %ld\n", mat->dwSize);
}

static HRESULT WINAPI IDirect3DMaterial2_GetMaterial(LPDIRECT3DMATERIAL2 this,
						     LPD3DMATERIAL lpMat)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpMat);
  if (TRACE_ON(ddraw))
    dump_material(lpMat);
  
  /* Copies the material structure */
  *lpMat = this->mat;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DMaterial2_SetMaterial(LPDIRECT3DMATERIAL2 this,
						     LPD3DMATERIAL lpMat)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpMat);
  if (TRACE_ON(ddraw))
    dump_material(lpMat);
  
  /* Stores the material */
  this->mat = *lpMat;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DMaterial2_GetHandle(LPDIRECT3DMATERIAL2 this,
						   LPDIRECT3DDEVICE2 lpD3DDevice2,
						   LPD3DMATERIALHANDLE lpMatHandle)

{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lpD3DDevice2, lpMatHandle);

  if (this->use_d3d2)
    this->device.active_device2 = lpD3DDevice2;
  else
    this->device.active_device1 = (LPDIRECT3DDEVICE) lpD3DDevice2;
  
  *lpMatHandle = (DWORD) this; /* lpD3DDevice2->store_material(this); */
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DMaterial_Reserve(LPDIRECT3DMATERIAL this)
{
  FIXME(ddraw, "(%p)->(): stub\n", this);

  return DDERR_INVALIDPARAMS;
}
						  
static HRESULT WINAPI IDirect3DMaterial_Unreserve(LPDIRECT3DMATERIAL this)
{
  FIXME(ddraw, "(%p)->(): stub\n", this);

  return DDERR_INVALIDPARAMS;
}

static HRESULT WINAPI IDirect3DMaterial_Initialize(LPDIRECT3DMATERIAL this,
						   LPDIRECT3D lpDirect3D)

{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpDirect3D);
  
  return DDERR_ALREADYINITIALIZED;
}


/*******************************************************************************
 *				IDirect3DMaterial VTable
 */
static IDirect3DMaterial_VTable material_vtable = {
  /*** IUnknown methods ***/
  IDirect3DMaterial2_QueryInterface,
  IDirect3DMaterial2_AddRef,
  IDirect3DMaterial2_Release,
  /*** IDirect3DMaterial methods ***/
  IDirect3DMaterial_Initialize,
  IDirect3DMaterial2_SetMaterial,
  IDirect3DMaterial2_GetMaterial,
  IDirect3DMaterial2_GetHandle,
  IDirect3DMaterial_Reserve,
  IDirect3DMaterial_Unreserve
};


/*******************************************************************************
 *				IDirect3DMaterial2 VTable
 */
static IDirect3DMaterial2_VTable material2_vtable = {
  /*** IUnknown methods ***/
  IDirect3DMaterial2_QueryInterface,
  IDirect3DMaterial2_AddRef,
  IDirect3DMaterial2_Release,
  /*** IDirect3DMaterial methods ***/
  IDirect3DMaterial2_SetMaterial,
  IDirect3DMaterial2_GetMaterial,
  IDirect3DMaterial2_GetHandle
};

#else /* HAVE_MESAGL */

LPDIRECT3DMATERIAL d3dmaterial_create(LPDIRECT3D d3d) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DMATERIAL2 d3dmaterial2_create(LPDIRECT3D2 d3d) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}


#endif /* HAVE_MESAGL */
