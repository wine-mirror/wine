/* Direct3D Material
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of Direct3DMaterial2. */

#include "config.h"
#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"

#include "mesa_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw)

static ICOM_VTABLE(IDirect3DMaterial2) material2_vtable;
static ICOM_VTABLE(IDirect3DMaterial) material_vtable;

/*******************************************************************************
 *				Matrial2 static functions
 */
static void activate(IDirect3DMaterial2Impl* This) {
  TRACE("Activating material %p\n", This);
  
  ENTER_GL();
  /* First, set the rendering context */
  if (This->use_d3d2)
    This->device.active_device2->set_context(This->device.active_device2);
  else
    This->device.active_device1->set_context(This->device.active_device1);

  /* Set the current Material */
  _dump_colorvalue("Diffuse", This->mat.a.diffuse);
  glMaterialfv(GL_FRONT,
	       GL_DIFFUSE,
	       (float *) &(This->mat.a.diffuse));
  _dump_colorvalue("Ambient", This->mat.b.ambient);
  glMaterialfv(GL_FRONT,
	       GL_AMBIENT,
	       (float *) &(This->mat.b.ambient));
  _dump_colorvalue("Specular", This->mat.c.specular);
  glMaterialfv(GL_FRONT,
	       GL_SPECULAR,
	       (float *) &(This->mat.c.specular));
  _dump_colorvalue("Emissive", This->mat.d.emissive);
  glMaterialfv(GL_FRONT,
	       GL_EMISSION,
	       (float *) &(This->mat.d.emissive));
  
  TRACE("Size  : %ld\n", This->mat.dwSize);
  TRACE("Power : %f\n", This->mat.e.power);

  TRACE("Texture handle : %08lx\n", (DWORD)This->mat.hTexture);
  LEAVE_GL();
  
  return ;
}

/*******************************************************************************
 *				Matrial2 Creation functions
 */
LPDIRECT3DMATERIAL2 d3dmaterial2_create(IDirect3D2Impl* d3d2)
{
  IDirect3DMaterial2Impl* mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DMaterial2Impl));
  mat->ref = 1;
  ICOM_VTBL(mat) = &material2_vtable;

  mat->use_d3d2 = 1;
  mat->d3d.d3d2 = d3d2;

  mat->activate = activate;
  
  return (LPDIRECT3DMATERIAL2)mat;
}

LPDIRECT3DMATERIAL d3dmaterial_create(IDirect3DImpl* d3d1)
{
  IDirect3DMaterial2Impl* mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DMaterial2Impl));
  mat->ref = 1;
  ICOM_VTBL(mat) = (ICOM_VTABLE(IDirect3DMaterial2)*)&material_vtable;

  mat->use_d3d2 = 0;
  mat->d3d.d3d1 = d3d1;

  mat->activate = activate;
  
  return (LPDIRECT3DMATERIAL) mat;
}

/*******************************************************************************
 *				IDirect3DMaterial2 methods
 */

static HRESULT WINAPI IDirect3DMaterial2Impl_QueryInterface(LPDIRECT3DMATERIAL2 iface,
							REFIID riid,
							LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  
  FIXME("(%p)->(%s,%p): stub\n", This, debugstr_guid(riid),ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DMaterial2Impl_AddRef(LPDIRECT3DMATERIAL2 iface)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DMaterial2Impl_Release(LPDIRECT3DMATERIAL2 iface)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );
  
  if (!--(This->ref)) {
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  
  return This->ref;
}

/*** IDirect3DMaterial2 methods ***/
static void dump_material(LPD3DMATERIAL mat)
{
  DPRINTF("  dwSize : %ld\n", mat->dwSize);
}

static HRESULT WINAPI IDirect3DMaterial2Impl_GetMaterial(LPDIRECT3DMATERIAL2 iface,
						     LPD3DMATERIAL lpMat)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  TRACE("(%p)->(%p)\n", This, lpMat);
  if (TRACE_ON(ddraw))
    dump_material(lpMat);
  
  /* Copies the material structure */
  *lpMat = This->mat;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DMaterial2Impl_SetMaterial(LPDIRECT3DMATERIAL2 iface,
						     LPD3DMATERIAL lpMat)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  TRACE("(%p)->(%p)\n", This, lpMat);
  if (TRACE_ON(ddraw))
    dump_material(lpMat);
  
  /* Stores the material */
  This->mat = *lpMat;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DMaterial2Impl_GetHandle(LPDIRECT3DMATERIAL2 iface,
						   LPDIRECT3DDEVICE2 lpD3DDevice2,
						   LPD3DMATERIALHANDLE lpMatHandle)

{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lpD3DDevice2, lpMatHandle);

  if (This->use_d3d2)
    This->device.active_device2 = (IDirect3DDevice2Impl*)lpD3DDevice2;
  else
    This->device.active_device1 = (IDirect3DDeviceImpl*)lpD3DDevice2;
  
  *lpMatHandle = (DWORD) This; /* lpD3DDevice2->store_material(This); */
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DMaterialImpl_Reserve(LPDIRECT3DMATERIAL iface)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  FIXME("(%p)->(): stub\n", This);

  return DDERR_INVALIDPARAMS;
}
						  
static HRESULT WINAPI IDirect3DMaterialImpl_Unreserve(LPDIRECT3DMATERIAL iface)
{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  FIXME("(%p)->(): stub\n", This);

  return DDERR_INVALIDPARAMS;
}

static HRESULT WINAPI IDirect3DMaterialImpl_Initialize(LPDIRECT3DMATERIAL iface,
						   LPDIRECT3D lpDirect3D)

{
  ICOM_THIS(IDirect3DMaterial2Impl,iface);
  TRACE("(%p)->(%p)\n", This, lpDirect3D);
  
  return DDERR_ALREADYINITIALIZED;
}


/*******************************************************************************
 *				IDirect3DMaterial VTable
 */
static ICOM_VTABLE(IDirect3DMaterial) material_vtable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  /*** IUnknown methods ***/
  IDirect3DMaterial2Impl_QueryInterface,
  IDirect3DMaterial2Impl_AddRef,
  IDirect3DMaterial2Impl_Release,
  /*** IDirect3DMaterial methods ***/
  IDirect3DMaterialImpl_Initialize,
  IDirect3DMaterial2Impl_SetMaterial,
  IDirect3DMaterial2Impl_GetMaterial,
  IDirect3DMaterial2Impl_GetHandle,
  IDirect3DMaterialImpl_Reserve,
  IDirect3DMaterialImpl_Unreserve
};


/*******************************************************************************
 *				IDirect3DMaterial2 VTable
 */
static ICOM_VTABLE(IDirect3DMaterial2) material2_vtable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  /*** IUnknown methods ***/
  IDirect3DMaterial2Impl_QueryInterface,
  IDirect3DMaterial2Impl_AddRef,
  IDirect3DMaterial2Impl_Release,
  /*** IDirect3DMaterial methods ***/
  IDirect3DMaterial2Impl_SetMaterial,
  IDirect3DMaterial2Impl_GetMaterial,
  IDirect3DMaterial2Impl_GetHandle
};
