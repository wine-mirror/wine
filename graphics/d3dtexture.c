/* Direct3D Texture
   (c) 1998 Lionel ULMER
   
   This files contains the implementation of interface Direct3DTexture2. */


#include "config.h"
#include "windows.h"
#include "wintypes.h"
#include "winerror.h"
#include "interfaces.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"

#include "d3d_private.h"

#ifdef HAVE_MESAGL

static IDirect3DTexture2_VTable texture2_vtable;
static IDirect3DTexture_VTable texture_vtable;

/*******************************************************************************
 *				Texture2 Creation functions
 */
LPDIRECT3DTEXTURE2 d3dtexture2_create(LPDIRECTDRAWSURFACE3 surf)
{
  LPDIRECT3DTEXTURE2 mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DTexture2));
  mat->ref = 1;
  mat->lpvtbl = &texture2_vtable;
  mat->surface = surf;
  
  return mat;
}

/*******************************************************************************
 *				Texture Creation functions
 */
LPDIRECT3DTEXTURE d3dtexture_create(LPDIRECTDRAWSURFACE3 surf)
{
  LPDIRECT3DTEXTURE mat;
  
  mat = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DTexture));
  mat->ref = 1;
  mat->lpvtbl = (IDirect3DTexture2_VTable*) &texture_vtable;
  mat->surface = surf;
  
  return mat;
}


/*******************************************************************************
 *				IDirect3DTexture2 methods
 */

static HRESULT WINAPI IDirect3DTexture2_QueryInterface(LPDIRECT3DTEXTURE2 this,
							REFIID riid,
							LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DTexture2_AddRef(LPDIRECT3DTEXTURE2 this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DTexture2_Release(LPDIRECT3DTEXTURE2 this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

/*** IDirect3DTexture methods ***/
static HRESULT WINAPI IDirect3DTexture_GetHandle(LPDIRECT3DTEXTURE this,
						 LPDIRECT3DDEVICE lpD3DDevice,
						 LPD3DTEXTUREHANDLE lpHandle)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lpD3DDevice, lpHandle);

  *lpHandle = (DWORD) this->surface;
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DTexture_Initialize(LPDIRECT3DTEXTURE this,
						  LPDIRECT3DDEVICE lpD3DDevice,
						  LPDIRECTDRAWSURFACE lpSurface)
{
  TRACE(ddraw, "(%p)->(%p,%p)\n", this, lpD3DDevice, lpSurface);

  return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirect3DTexture_Unload(LPDIRECT3DTEXTURE this)
{
  FIXME(ddraw, "(%p)->(): stub\n", this);

  return DD_OK;
}

/*** IDirect3DTexture2 methods ***/
static HRESULT WINAPI IDirect3DTexture2_GetHandle(LPDIRECT3DTEXTURE2 this,
						  LPDIRECT3DDEVICE2 lpD3DDevice2,
						  LPD3DTEXTUREHANDLE lpHandle)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lpD3DDevice2, lpHandle);

  *lpHandle = (DWORD) this->surface; /* lpD3DDevice2->store_texture(this); */
  
  return DD_OK;
}

/* Common methods */
static HRESULT WINAPI IDirect3DTexture2_PaletteChanged(LPDIRECT3DTEXTURE2 this,
						       DWORD dwStart,
						       DWORD dwCount)
{
  FIXME(ddraw, "(%p)->(%8ld,%8ld): stub\n", this, dwStart, dwCount);

  return DD_OK;
}

static HRESULT WINAPI IDirect3DTexture2_Load(LPDIRECT3DTEXTURE2 this,
					     LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpD3DTexture2);

  /* Hack ? */
  FIXME(ddraw, "Sthis %p / Sload %p\n", this->surface, lpD3DTexture2->surface);
  this->surface->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;
  
  return DD_OK;
}


/*******************************************************************************
 *				IDirect3DTexture2 VTable
 */
static IDirect3DTexture2_VTable texture2_vtable = {
  /*** IUnknown methods ***/
  IDirect3DTexture2_QueryInterface,
  IDirect3DTexture2_AddRef,
  IDirect3DTexture2_Release,
  /*** IDirect3DTexture methods ***/
  IDirect3DTexture2_GetHandle,
  IDirect3DTexture2_PaletteChanged,
  IDirect3DTexture2_Load
};

/*******************************************************************************
 *				IDirect3DTexture VTable
 */
static IDirect3DTexture_VTable texture_vtable = {
  /*** IUnknown methods ***/
  IDirect3DTexture2_QueryInterface,
  IDirect3DTexture2_AddRef,
  IDirect3DTexture2_Release,
  /*** IDirect3DTexture methods ***/
  IDirect3DTexture_Initialize,
  IDirect3DTexture_GetHandle,
  IDirect3DTexture2_PaletteChanged,
  IDirect3DTexture2_Load,
  IDirect3DTexture_Unload
};

#else /* HAVE_MESAGL */

/* These function should never be called if MesaGL is not present */
LPDIRECT3DTEXTURE2 d3dtexture2_create(LPDIRECTDRAWSURFACE3 surf) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

LPDIRECT3DTEXTURE d3dtexture_create(LPDIRECTDRAWSURFACE3 surf) {
  ERR(ddraw, "Should not be called...\n");
  return NULL;
}

#endif /* HAVE_MESAGL */
