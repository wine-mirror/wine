/* Direct3D Device
   (c) 1998 Lionel ULMER
   
   This files contains all the D3D devices that Wine supports. For the moment
   only the 'OpenGL' target is supported. */

#include "config.h"
#include "wintypes.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"

#include "d3d_private.h"

/* Define this variable if you have an unpatched Mesa 3.0 (patches are available
   on Mesa's home page) or version 3.1b.

   Version 3.2b should correct this bug */
#undef HAVE_BUGGY_MESAGL

#ifdef HAVE_MESAGL

static GUID IID_D3DDEVICE2_OpenGL = {
  0x39a0da38,
  0x7e57,
  0x11d2,
  { 0x8b,0x7c,0x0e,0x4e,0xd8,0x3c,0x2b,0x3c }
};

static GUID IID_D3DDEVICE_OpenGL = {
  0x31416d44,
  0x86ae,
  0x11d2,
  { 0x82,0x2d,0xa8,0xd5,0x31,0x87,0xca,0xfa }
};


static IDirect3DDevice2_VTable OpenGL_vtable;
static IDirect3DDevice_VTable OpenGL_vtable_dx3;

/*******************************************************************************
 *				OpenGL static functions
 */
static void set_context(LPDIRECT3DDEVICE2 this) {
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this;

  OSMesaMakeCurrent(odev->ctx, odev->buffer,
		    GL_UNSIGNED_BYTE,
		    this->surface->s.surface_desc.dwWidth,
		    this->surface->s.surface_desc.dwHeight);
}

static void set_context_dx3(LPDIRECT3DDEVICE this) {
  OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) this;

  OSMesaMakeCurrent(odev->ctx, odev->buffer,
		    GL_UNSIGNED_BYTE,
		    this->surface->s.surface_desc.dwWidth,
		    this->surface->s.surface_desc.dwHeight);
}

static void fill_opengl_primcaps(D3DPRIMCAPS *pc)
{
  pc->dwSize = sizeof(*pc);
  pc->dwMiscCaps = D3DPMISCCAPS_CONFORMANT | D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW |
    D3DPMISCCAPS_LINEPATTERNREP | D3DPMISCCAPS_MASKZ;
  pc->dwRasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_FOGTABLE |
    D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_STIPPLE | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_ZTEST;
  pc->dwZCmpCaps = 0xFFFFFFFF; /* All Z test can be done */
  pc->dwSrcBlendCaps  = 0xFFFFFFFF; /* FIXME: need REAL values */
  pc->dwDestBlendCaps = 0xFFFFFFFF; /* FIXME: need REAL values */
  pc->dwAlphaCmpCaps  = 0xFFFFFFFF; /* FIXME: need REAL values */
  pc->dwShadeCaps = 0xFFFFFFFF;     /* FIXME: need REAL values */
  pc->dwTextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_BORDER | D3DPTEXTURECAPS_PERSPECTIVE |
    D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_TRANSPARENCY;
  pc->dwTextureFilterCaps = D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_LINEARMIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST |
    D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_MIPNEAREST | D3DPTFILTERCAPS_NEAREST;
  pc->dwTextureBlendCaps = 0xFFFFFFFF; /* FIXME: need REAL values */
  pc->dwTextureAddressCaps = D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_WRAP;
  pc->dwStippleWidth = 32;
  pc->dwStippleHeight = 32;
}

static void fill_opengl_caps(D3DDEVICEDESC *d1, D3DDEVICEDESC *d2)
{
  /* GLint maxlight; */
  
  d1->dwSize  = sizeof(*d1);
  d1->dwFlags = D3DDD_DEVCAPS | D3DDD_BCLIPPING | D3DDD_COLORMODEL | D3DDD_DEVICERENDERBITDEPTH | D3DDD_DEVICEZBUFFERBITDEPTH
    | D3DDD_LIGHTINGCAPS | D3DDD_LINECAPS | D3DDD_MAXBUFFERSIZE | D3DDD_MAXVERTEXCOUNT | D3DDD_TRANSFORMCAPS | D3DDD_TRICAPS;
  d1->dcmColorModel = D3DCOLOR_RGB;
  d1->dwDevCaps = D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_EXECUTESYSTEMMEMORY |
    D3DDEVCAPS_EXECUTEVIDEOMEMORY | D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTURESYSTEMMEMORY |
      D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY;
  d1->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
  d1->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
  d1->bClipping = TRUE;
  d1->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
  d1->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_DIRECTIONAL | D3DLIGHTCAPS_PARALLELPOINT | D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT;
  d1->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
  d1->dlcLightingCaps.dwNumLights = 16; /* glGetIntegerv(GL_MAX_LIGHTS, &maxlight); d1->dlcLightingCaps.dwNumLights = maxlight; */
  fill_opengl_primcaps(&(d1->dpcLineCaps));
  fill_opengl_primcaps(&(d1->dpcTriCaps));  
  d1->dwDeviceRenderBitDepth  = DDBD_16;
  d1->dwDeviceZBufferBitDepth = DDBD_16;
  d1->dwMaxBufferSize = 0;
  d1->dwMaxVertexCount = 65536;
  d1->dwMinTextureWidth  = 1;
  d1->dwMinTextureHeight = 1;
  d1->dwMaxTextureWidth  = 256; /* This is for Mesa on top of Glide (in the future :-) ) */
  d1->dwMaxTextureHeight = 256; /* This is for Mesa on top of Glide (in the future :-) ) */
  d1->dwMinStippleWidth  = 1;
  d1->dwMinStippleHeight = 1;
  d1->dwMaxStippleWidth  = 32;
  d1->dwMaxStippleHeight = 32;

  d2->dwSize  = sizeof(*d2);
  d2->dwFlags = 0;
}

int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  D3DDEVICEDESC	d1,d2;
  
  TRACE(ddraw," Enumerating OpenGL D3D device.\n");
  
  fill_opengl_caps(&d1, &d2);
  
  return cb((void*)&IID_D3DDEVICE2_OpenGL,"WINE Direct3D using OpenGL","direct3d",&d1,&d2,context);
}

int is_OpenGL(REFCLSID rguid, LPDIRECTDRAWSURFACE surface, LPDIRECT3DDEVICE2 *device, LPDIRECT3D2 d3d)
{
  if (/* Default device */
      (rguid == NULL) ||
      /* HAL Device */
      (!memcmp(&IID_IDirect3DHALDevice,rguid,sizeof(IID_IDirect3DHALDevice))) ||
      /* OpenGL Device */
      (!memcmp(&IID_D3DDEVICE2_OpenGL,rguid,sizeof(IID_D3DDEVICE2_OpenGL)))) {
    OpenGL_IDirect3DDevice2 *odev;
    
    const float id_mat[16] = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0
    };
    
    *device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OpenGL_IDirect3DDevice2));
    odev = (OpenGL_IDirect3DDevice2 *) (*device);
    (*device)->ref = 1;
    (*device)->lpvtbl = &OpenGL_vtable;
    (*device)->d3d = d3d;
    (*device)->surface = surface;
    
    (*device)->viewport_list = NULL;
    (*device)->current_viewport = NULL;
    
    (*device)->set_context = set_context;
    
    TRACE(ddraw, "OpenGL device created \n");
    
    /* Create the OpenGL context */
    odev->ctx = OSMesaCreateContext(OSMESA_RGBA, NULL);
    odev->buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
			     surface->s.surface_desc.dwWidth * surface->s.surface_desc.dwHeight * 4);
    odev->rs.src = GL_ONE;
    odev->rs.dst = GL_ZERO;
    odev->rs.mag = GL_NEAREST;
    odev->rs.min = GL_NEAREST;
    odev->vt     = 0;
    
    memcpy(odev->world_mat, id_mat, 16 * sizeof(float));
    memcpy(odev->view_mat , id_mat, 16 * sizeof(float));
    memcpy(odev->proj_mat , id_mat, 16 * sizeof(float));

    /* Initialisation */
    (*device)->set_context(*device);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
    
    return 1;
  }
  
  /* This is not the OpenGL UID */
  return 0;
}

/*******************************************************************************
 *				Common IDirect3DDevice2
 */

static HRESULT WINAPI IDirect3DDevice2_QueryInterface(LPDIRECT3DDEVICE2 this,
						      REFIID riid,
						      LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DDevice2_AddRef(LPDIRECT3DDEVICE2 this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DDevice2_Release(LPDIRECT3DDEVICE2 this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}


/*** IDirect3DDevice2 methods ***/
static HRESULT WINAPI IDirect3DDevice2_GetCaps(LPDIRECT3DDEVICE2 this,
					       LPD3DDEVICEDESC lpdescsoft,
					       LPD3DDEVICEDESC lpdeschard)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lpdescsoft, lpdeschard);
  
  fill_opengl_caps(lpdescsoft, lpdeschard);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_SwapTextureHandles(LPDIRECT3DDEVICE2 this,
							  LPDIRECT3DTEXTURE2 lptex1,
							  LPDIRECT3DTEXTURE2 lptex2)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, lptex1, lptex2);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetStats(LPDIRECT3DDEVICE2 this,
						LPD3DSTATS lpstats)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpstats);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_AddViewport(LPDIRECT3DDEVICE2 this,
						   LPDIRECT3DVIEWPORT2 lpvp)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);
  
  /* Adds this viewport to the viewport list */
  lpvp->next = this->viewport_list;
  this->viewport_list = lpvp;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_DeleteViewport(LPDIRECT3DDEVICE2 this,
						      LPDIRECT3DVIEWPORT2 lpvp)
{
  LPDIRECT3DVIEWPORT2 cur, prev;
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);
  
  /* Finds this viewport in the list */
  prev = NULL;
  cur = this->viewport_list;
  while ((cur != NULL) && (cur != lpvp)) {
    prev = cur;
    cur = cur->next;
  }
  if (cur == NULL)
    return DDERR_INVALIDOBJECT;
  
  /* And remove it */
  if (prev == NULL)
    this->viewport_list = cur->next;
  else
    prev->next = cur->next;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_NextViewport(LPDIRECT3DDEVICE2 this,
						    LPDIRECT3DVIEWPORT2 lpvp,
						    LPDIRECT3DVIEWPORT2* lplpvp,
						    DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%p,%p,%08lx): stub\n", this, lpvp, lpvp, dwFlags);
  
  switch (dwFlags) {
  case D3DNEXT_NEXT:
    *lplpvp = lpvp->next;
    break;
    
  case D3DNEXT_HEAD:
    *lplpvp = this->viewport_list;
    break;
    
  case D3DNEXT_TAIL:
    lpvp = this->viewport_list;
    while (lpvp->next != NULL)
      lpvp = lpvp->next;
    
    *lplpvp = lpvp;
    break;
    
  default:
    return DDERR_INVALIDPARAMS;
  }
  
  return DD_OK;
}

static HRESULT enum_texture_format_OpenGL(LPD3DENUMTEXTUREFORMATSCALLBACK cb,
					  LPVOID context) {
  DDSURFACEDESC sdesc;
  LPDDPIXELFORMAT pformat;

  /* Do the texture enumeration */
  sdesc.dwSize = sizeof(DDSURFACEDESC);
  sdesc.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
  sdesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
  pformat = &(sdesc.ddpfPixelFormat);
  pformat->dwSize = sizeof(DDPIXELFORMAT);
  pformat->dwFourCC = 0;
  
  TRACE(ddraw, "Enumerating GL_RGBA unpacked (32)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->x.dwRGBBitCount = 32;
  pformat->y.dwRBitMask =         0xFF000000;
  pformat->z.dwGBitMask =         0x00FF0000;
  pformat->xx.dwBBitMask =        0x0000FF00;
  pformat->xy.dwRGBAlphaBitMask = 0x000000FF;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE(ddraw, "Enumerating GL_RGB unpacked (24)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->x.dwRGBBitCount = 24;
  pformat->y.dwRBitMask =  0x00FF0000;
  pformat->z.dwGBitMask =  0x0000FF00;
  pformat->xx.dwBBitMask = 0x000000FF;
  pformat->xy.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

#ifndef HAVE_BUGGY_MESAGL
  /* The packed texture format are buggy in Mesa. The bug was reported and corrected,
     so that future version will work great. */
  TRACE(ddraw, "Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_6_5 (16)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->x.dwRGBBitCount = 16;
  pformat->y.dwRBitMask =  0x0000F800;
  pformat->z.dwGBitMask =  0x000007E0;
  pformat->xx.dwBBitMask = 0x0000001F;
  pformat->xy.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE(ddraw, "Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_5_5_5_1 (16)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->x.dwRGBBitCount = 16;
  pformat->y.dwRBitMask =         0x0000F800;
  pformat->z.dwGBitMask =         0x000007C0;
  pformat->xx.dwBBitMask =        0x0000003E;
  pformat->xy.dwRGBAlphaBitMask = 0x00000001;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE(ddraw, "Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_4_4_4_4 (16)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->x.dwRGBBitCount = 16;
  pformat->y.dwRBitMask =         0x0000F000;
  pformat->z.dwGBitMask =         0x00000F00;
  pformat->xx.dwBBitMask =        0x000000F0;
  pformat->xy.dwRGBAlphaBitMask = 0x0000000F;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE(ddraw, "Enumerating GL_RGB packed GL_UNSIGNED_BYTE_3_3_2 (8)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->x.dwRGBBitCount = 8;
  pformat->y.dwRBitMask =         0x0000F800;
  pformat->z.dwGBitMask =         0x000007C0;
  pformat->xx.dwBBitMask =        0x0000003E;
  pformat->xy.dwRGBAlphaBitMask = 0x00000001;
  if (cb(&sdesc, context) == 0)
    return DD_OK;
#endif
  
  TRACE(ddraw, "Enumerating Paletted (8)\n");
  pformat->dwFlags = DDPF_PALETTEINDEXED8;
  pformat->x.dwRGBBitCount = 8;
  pformat->y.dwRBitMask =  0x00000000;
  pformat->z.dwGBitMask =  0x00000000;
  pformat->xx.dwBBitMask = 0x00000000;
  pformat->xy.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;
  
  TRACE(ddraw, "End of enumeration\n");
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDevice2_EnumTextureFormats(LPDIRECT3DDEVICE2 this,
							  LPD3DENUMTEXTUREFORMATSCALLBACK cb,
							  LPVOID context)
{
  FIXME(ddraw, "(%p)->(%p,%p): stub\n", this, cb, context);
  
  return enum_texture_format_OpenGL(cb, context);
}



static HRESULT WINAPI IDirect3DDevice2_BeginScene(LPDIRECT3DDEVICE2 this)
{
  /* OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this; */
  
  FIXME(ddraw, "(%p)->(): stub\n", this);
  
  /* Here, we should get the DDraw surface and 'copy it' to the
     OpenGL surface.... */
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_EndScene(LPDIRECT3DDEVICE2 this)
{
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this;
  LPDIRECTDRAWSURFACE3 surf = (LPDIRECTDRAWSURFACE3) this->surface;
  DDSURFACEDESC sdesc;
  int x,y;
  unsigned char *src;
  unsigned short *dest;
  
  FIXME(ddraw, "(%p)->(): stub\n", this);

  /* Here we copy back the OpenGL scene to the the DDraw surface */
  /* First, lock the surface */
  surf->lpvtbl->fnLock(surf,NULL,&sdesc,DDLOCK_WRITEONLY,0);

  /* The copy the OpenGL buffer to this surface */
  
  /* NOTE : this is only for debugging purpose. I KNOW it is really unoptimal.
     I am currently working on a set of patches for Mesa to have OSMesa support
     16 bpp surfaces => we will able to render directly onto the surface, no
     need to do a bpp conversion */
  dest = (unsigned short *) sdesc.y.lpSurface;
  src = ((unsigned char *) odev->buffer) + 4 * (sdesc.dwWidth * (sdesc.dwHeight - 1));
  for (y = 0; y < sdesc.dwHeight; y++) {
    unsigned char *lsrc = src;
    
    for (x = 0; x < sdesc.dwWidth ; x++) {
      unsigned char r =  *lsrc++;
      unsigned char g =  *lsrc++;
      unsigned char b =  *lsrc++;
      lsrc++; /* Alpha */
      *dest = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
      
      dest++;
    }

    src -= 4 * sdesc.dwWidth;
  }

  /* Unlock the surface */
  surf->lpvtbl->fnUnlock(surf,sdesc.y.lpSurface);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetDirect3D(LPDIRECT3DDEVICE2 this, LPDIRECT3D2 *lpd3d2)
{
  TRACE(ddraw, "(%p)->(%p): stub\n", this, lpd3d2);
  
  *lpd3d2 = this->d3d;
  
  return DD_OK;
}



/*** DrawPrimitive API ***/
static HRESULT WINAPI IDirect3DDevice2_SetCurrentViewport(LPDIRECT3DDEVICE2 this,
							  LPDIRECT3DVIEWPORT2 lpvp)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);
  
  /* Should check if the viewport was added or not */
  
  /* Set this viewport as the current viewport */
  this->current_viewport = lpvp;
  
  /* Activate this viewport */
  lpvp->device.active_device2 = this;
  lpvp->activate(lpvp);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetCurrentViewport(LPDIRECT3DDEVICE2 this,
							  LPDIRECT3DVIEWPORT2 *lplpvp)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lplpvp);
  
  /* Returns the current viewport */
  *lplpvp = this->current_viewport;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_SetRenderTarget(LPDIRECT3DDEVICE2 this,
						       LPDIRECTDRAWSURFACE lpdds,
						       DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%p,%08lx): stub\n", this, lpdds, dwFlags);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetRenderTarget(LPDIRECT3DDEVICE2 this,
						       LPDIRECTDRAWSURFACE *lplpdds)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lplpdds);
  
  /* Returns the current rendering target (the surface on wich we render) */
  *lplpdds = this->surface;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_Begin(LPDIRECT3DDEVICE2 this,
					     D3DPRIMITIVETYPE d3dp,
					     D3DVERTEXTYPE d3dv,
					     DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%d,%d,%08lx): stub\n", this, d3dp, d3dv, dwFlags);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_BeginIndexed(LPDIRECT3DDEVICE2 this,
						    D3DPRIMITIVETYPE d3dp,
						    D3DVERTEXTYPE d3dv,
						    LPVOID lpvert,
						    DWORD numvert,
						    DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%d,%d,%p,%ld,%08lx): stub\n", this, d3dp, d3dv, lpvert, numvert, dwFlags);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_Vertex(LPDIRECT3DDEVICE2 this,
					      LPVOID lpvert)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvert);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_Index(LPDIRECT3DDEVICE2 this,
					     WORD index)
{
  FIXME(ddraw, "(%p)->(%d): stub\n", this, index);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_End(LPDIRECT3DDEVICE2 this,
					   DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%08lx): stub\n", this, dwFlags);
  
  return DD_OK;
}




static HRESULT WINAPI IDirect3DDevice2_GetRenderState(LPDIRECT3DDEVICE2 this,
						      D3DRENDERSTATETYPE d3drs,
						      LPDWORD lprstate)
{
  FIXME(ddraw, "(%p)->(%d,%p): stub\n", this, d3drs, lprstate);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_SetRenderState(LPDIRECT3DDEVICE2 this,
						      D3DRENDERSTATETYPE dwRenderStateType,
						      DWORD dwRenderState)
{
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this;

  TRACE(ddraw, "(%p)->(%d,%ld)\n", this, dwRenderStateType, dwRenderState);
  
  /* Call the render state functions */
  set_render_state(dwRenderStateType, dwRenderState, &(odev->rs));
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetLightState(LPDIRECT3DDEVICE2 this,
						     D3DLIGHTSTATETYPE d3dls,
						     LPDWORD lplstate)
{
  FIXME(ddraw, "(%p)->(%d,%p): stub\n", this, d3dls, lplstate);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_SetLightState(LPDIRECT3DDEVICE2 this,
						     D3DLIGHTSTATETYPE dwLightStateType,
						     DWORD dwLightState)
{
  FIXME(ddraw, "(%p)->(%d,%08lx): stub\n", this, dwLightStateType, dwLightState);
  
  switch (dwLightStateType) {
  case D3DLIGHTSTATE_MATERIAL: {  /* 1 */
    LPDIRECT3DMATERIAL2 mat = (LPDIRECT3DMATERIAL2) dwLightState;
    
    if (mat != NULL) {
      mat->activate(mat);
    } else {
      TRACE(ddraw, "Zoups !!!\n");
    }
  } break;
    
  case D3DLIGHTSTATE_AMBIENT: {   /* 2 */
    float light[4];
    
    light[0] = ((dwLightState >> 16) & 0xFF) / 255.0;
    light[1] = ((dwLightState >>  8) & 0xFF) / 255.0;
    light[2] = ((dwLightState >>  0) & 0xFF) / 255.0;
    light[3] = 1.0;
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);
  } break;
    
  case D3DLIGHTSTATE_COLORMODEL:  /* 3 */
    break;
    
  case D3DLIGHTSTATE_FOGMODE:     /* 4 */
    break;
    
  case D3DLIGHTSTATE_FOGSTART:    /* 5 */
    break;
    
  case D3DLIGHTSTATE_FOGEND:      /* 6 */
    break;
    
  case D3DLIGHTSTATE_FOGDENSITY:  /* 7 */
    break;
    
  default:
    TRACE(ddraw, "Unexpected Light State Type\n");
    return DDERR_INVALIDPARAMS;
  }
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_SetTransform(LPDIRECT3DDEVICE2 this,
						    D3DTRANSFORMSTATETYPE d3dts,
						    LPD3DMATRIX lpmatrix)
{
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this;
  
  FIXME(ddraw, "(%p)->(%d,%p): stub\n", this, d3dts, lpmatrix);
  
  /* Using a trial and failure approach, I found that the order of 
     Direct3D transformations that works best is :

     ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord

     As OpenGL uses only two matrices, I combined PROJECTION and VIEW into
     OpenGL's GL_PROJECTION matrix and the WORLD into GL_MODELVIEW.

     If anyone has a good explanation of the three different matrices in
     the SDK online documentation, feel free to point it to me. For example,
     which matrices transform lights ? In OpenGL only the PROJECTION matrix
     transform the lights, not the MODELVIEW. Using the matrix names, I 
     supposed that PROJECTION and VIEW (all 'camera' related names) do
     transform lights, but WORLD do not. It may be wrong though... */

  /* After reading through both OpenGL and Direct3D documentations, I
     thought that D3D matrices were written in 'line major mode' transposed
     from OpenGL's 'column major mode'. But I found out that a simple memcpy
     works fine to transfer one matrix format to the other (it did not work
     when transposing)....

     So :
      1) are the documentations wrong
      2) does the matrix work even if they are not read correctly
      3) is Mesa's implementation of OpenGL not compliant regarding Matrix
         loading using glLoadMatrix ?

     Anyway, I always use 'conv_mat' to transfer the matrices from one format
     to the other so that if I ever find out that I need to transpose them, I
     will able to do it quickly, only by changing the macro conv_mat. */

  switch (d3dts) {
  case D3DTRANSFORMSTATE_WORLD: {
    conv_mat(lpmatrix, odev->world_mat);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf((float *) &(odev->world_mat));
  } break;
    
  case D3DTRANSFORMSTATE_VIEW: {
    conv_mat(lpmatrix, odev->view_mat);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float *) &(odev->proj_mat));
    glMultMatrixf((float *) &(odev->view_mat));
  } break;
    
  case D3DTRANSFORMSTATE_PROJECTION: {
    conv_mat(lpmatrix, odev->proj_mat);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float *) &(odev->proj_mat));
    glMultMatrixf((float *) &(odev->view_mat));
  } break;
    
  default:
    break;
  }
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetTransform(LPDIRECT3DDEVICE2 this,
						    D3DTRANSFORMSTATETYPE d3dts,
						    LPD3DMATRIX lpmatrix)
{
  FIXME(ddraw, "(%p)->(%d,%p): stub\n", this, d3dts, lpmatrix);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_MultiplyTransform(LPDIRECT3DDEVICE2 this,
							 D3DTRANSFORMSTATETYPE d3dts,
							 LPD3DMATRIX lpmatrix)
{
  FIXME(ddraw, "(%p)->(%d,%p): stub\n", this, d3dts, lpmatrix);
  
  return DD_OK;
}

#define DRAW_PRIMITIVE(MAXVERT,INDEX)						\
  /* Puts GL in the correct lighting mode */					\
  if (odev->vt != d3dv) {							\
    if (odev->vt == D3DVT_TLVERTEX) {						\
      /* Need to put the correct transformation again */			\
      glMatrixMode(GL_MODELVIEW);						\
      glLoadMatrixf((float *) &(odev->world_mat));				\
      glMatrixMode(GL_PROJECTION);						\
      glLoadMatrixf((float *) &(odev->proj_mat));				\
      glMultMatrixf((float *) &(odev->view_mat));				\
    }										\
										\
    switch (d3dv) {								\
    case D3DVT_VERTEX:								\
      TRACE(ddraw, "Standard Vertex\n");					\
      glEnable(GL_LIGHTING);							\
      break;									\
										\
    case D3DVT_LVERTEX:								\
      TRACE(ddraw, "Lighted Vertex\n");						\
      glDisable(GL_LIGHTING);							\
      break;									\
										\
    case D3DVT_TLVERTEX: {							\
      GLdouble height, width, minZ, maxZ;					\
										\
      TRACE(ddraw, "Transformed - Lighted Vertex\n");				\
      /* First, disable lighting */						\
      glDisable(GL_LIGHTING);							\
										\
      /* Then do not put any transformation matrixes */				\
      glMatrixMode(GL_MODELVIEW);						\
      glLoadIdentity();								\
      glMatrixMode(GL_PROJECTION);						\
      glLoadIdentity();								\
										\
      if (this->current_viewport == NULL) {					\
	ERR(ddraw, "No current viewport !\n");					\
	/* Using standard values */						\
	height = 640.0;								\
	width = 480.0;								\
	minZ = -10.0;								\
	maxZ = 10.0;								\
      } else {									\
	if (this->current_viewport->use_vp2) {					\
	  height = (GLdouble) this->current_viewport->viewport.vp2.dwHeight;	\
	  width  = (GLdouble) this->current_viewport->viewport.vp2.dwWidth;	\
	  minZ   = (GLdouble) this->current_viewport->viewport.vp2.dvMinZ;	\
	  maxZ   = (GLdouble) this->current_viewport->viewport.vp2.dvMaxZ;	\
	} else {								\
	  height = (GLdouble) this->current_viewport->viewport.vp1.dwHeight;	\
	  width  = (GLdouble) this->current_viewport->viewport.vp1.dwWidth;	\
	  minZ   = (GLdouble) this->current_viewport->viewport.vp1.dvMinZ;	\
	  maxZ   = (GLdouble) this->current_viewport->viewport.vp1.dvMaxZ;	\
	}									\
      }										\
										\
      glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);				\
    } break;									\
										\
    default:									\
      ERR(ddraw, "Unhandled vertex type\n");					\
      break;									\
    }										\
										\
    odev->vt = d3dv;								\
  }										\
										\
  switch (d3dp) {								\
  case D3DPT_POINTLIST:								\
    TRACE(ddraw, "Start POINTS\n");						\
    glBegin(GL_POINTS);								\
    break;									\
										\
  case D3DPT_LINELIST:								\
    TRACE(ddraw, "Start LINES\n");						\
    glBegin(GL_LINES);								\
    break;									\
										\
  case D3DPT_LINESTRIP:								\
    TRACE(ddraw, "Start LINE_STRIP\n");						\
    glBegin(GL_LINE_STRIP);							\
    break;									\
										\
  case D3DPT_TRIANGLELIST:							\
    TRACE(ddraw, "Start TRIANGLES\n");						\
    glBegin(GL_TRIANGLES);							\
    break;									\
										\
  case D3DPT_TRIANGLESTRIP:							\
    TRACE(ddraw, "Start TRIANGLE_STRIP\n");					\
    glBegin(GL_TRIANGLE_STRIP);							\
    break;									\
										\
  case D3DPT_TRIANGLEFAN:							\
    TRACE(ddraw, "Start TRIANGLE_FAN\n");					\
    glBegin(GL_TRIANGLE_FAN);							\
    break;									\
										\
  default:									\
    TRACE(ddraw, "Unhandled primitive\n");					\
    break;									\
  }										\
										\
  /* Draw the primitives */							\
  for (vx_index = 0; vx_index < MAXVERT; vx_index++) {				\
    switch (d3dv) {								\
    case D3DVT_VERTEX: {							\
      D3DVERTEX *vx = ((D3DVERTEX *) lpvertex) + INDEX;				\
										\
      glNormal3f(vx->nx.nx, vx->ny.ny, vx->nz.nz);				\
      glVertex3f(vx->x.x, vx->y.y, vx->z.z);					\
      TRACE(ddraw, "   V: %f %f %f\n", vx->x.x, vx->y.y, vx->z.z);		\
    } break;									\
										\
    case D3DVT_LVERTEX: {							\
      D3DLVERTEX *vx = ((D3DLVERTEX *) lpvertex) + INDEX;			\
      DWORD col = vx->c.color;							\
										\
      glColor3f(((col >> 16) & 0xFF) / 255.0,					\
		((col >>  8) & 0xFF) / 255.0,					\
		((col >>  0) & 0xFF) / 255.0);					\
      glVertex3f(vx->x.x, vx->y.y, vx->z.z);					\
      TRACE(ddraw, "  LV: %f %f %f (%02lx %02lx %02lx)\n",			\
	    vx->x.x, vx->y.y, vx->z.z,						\
	    ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF));	\
    } break;									\
										\
    case D3DVT_TLVERTEX: {							\
      D3DTLVERTEX *vx = ((D3DTLVERTEX *) lpvertex) + INDEX;			\
      DWORD col = vx->c.color;							\
										\
      glColor3f(((col >> 16) & 0xFF) / 255.0,					\
		((col >>  8) & 0xFF) / 255.0,					\
		((col >>  0) & 0xFF) / 255.0);					\
      glTexCoord2f(vx->u.tu, vx->v.tv);						\
      if (vx->r.rhw < 0.01)							\
	glVertex3f(vx->x.sx,							\
		   vx->y.sy,							\
		   vx->z.sz);							\
      else									\
	glVertex4f(vx->x.sx / vx->r.rhw,					\
		   vx->y.sy / vx->r.rhw,					\
		   vx->z.sz / vx->r.rhw,					\
		   1.0 / vx->r.rhw);						\
      TRACE(ddraw, " TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",		\
	    vx->x.sx, vx->y.sy, vx->z.sz,					\
	    ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),	\
	    vx->u.tu, vx->v.tv, vx->r.rhw);					\
    } break;									\
										\
    default:									\
      TRACE(ddraw, "Unhandled vertex type\n");					\
      break;									\
    }										\
  }										\
										\
  glEnd();									\
  TRACE(ddraw, "End\n");


static HRESULT WINAPI IDirect3DDevice2_DrawPrimitive(LPDIRECT3DDEVICE2 this,
						     D3DPRIMITIVETYPE d3dp,
						     D3DVERTEXTYPE d3dv,
						     LPVOID lpvertex,
						     DWORD vertcount,
						     DWORD dwFlags)
{
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this;
  int vx_index;
  
  TRACE(ddraw, "(%p)->(%d,%d,%p,%ld,%08lx): stub\n", this, d3dp, d3dv, lpvertex, vertcount, dwFlags);

  DRAW_PRIMITIVE(vertcount, vx_index);
    
  return D3D_OK;
      }
    

      
static HRESULT WINAPI IDirect3DDevice2_DrawIndexedPrimitive(LPDIRECT3DDEVICE2 this,
							    D3DPRIMITIVETYPE d3dp,
							    D3DVERTEXTYPE d3dv,
							    LPVOID lpvertex,
							    DWORD vertcount,
							    LPWORD lpindexes,
							    DWORD indexcount,
							    DWORD dwFlags)
{
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) this;
  int vx_index;
  
  TRACE(ddraw, "(%p)->(%d,%d,%p,%ld,%p,%ld,%08lx): stub\n", this, d3dp, d3dv, lpvertex, vertcount, lpindexes, indexcount, dwFlags);
  
  DRAW_PRIMITIVE(indexcount, lpindexes[vx_index]);
  
  return D3D_OK;
}



static HRESULT WINAPI IDirect3DDevice2_SetClipStatus(LPDIRECT3DDEVICE2 this,
						     LPD3DCLIPSTATUS lpcs)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpcs);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2_GetClipStatus(LPDIRECT3DDEVICE2 this,
						     LPD3DCLIPSTATUS lpcs)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpcs);
  
  return DD_OK;
}



/*******************************************************************************
 *				OpenGL-specific IDirect3DDevice2
 */

/*******************************************************************************
 *				OpenGL-specific VTable
 */

static IDirect3DDevice2_VTable OpenGL_vtable = {
  IDirect3DDevice2_QueryInterface,
  IDirect3DDevice2_AddRef,
  IDirect3DDevice2_Release,
  /*** IDirect3DDevice2 methods ***/
  IDirect3DDevice2_GetCaps,
  IDirect3DDevice2_SwapTextureHandles,
  IDirect3DDevice2_GetStats,
  IDirect3DDevice2_AddViewport,
  IDirect3DDevice2_DeleteViewport,
  IDirect3DDevice2_NextViewport,
  IDirect3DDevice2_EnumTextureFormats,
  IDirect3DDevice2_BeginScene,
  IDirect3DDevice2_EndScene,
  IDirect3DDevice2_GetDirect3D,
  
  /*** DrawPrimitive API ***/
  IDirect3DDevice2_SetCurrentViewport,
  IDirect3DDevice2_GetCurrentViewport,
  
  IDirect3DDevice2_SetRenderTarget,
  IDirect3DDevice2_GetRenderTarget,
  
  IDirect3DDevice2_Begin,
  IDirect3DDevice2_BeginIndexed,
  IDirect3DDevice2_Vertex,
  IDirect3DDevice2_Index,
  IDirect3DDevice2_End,
  
  IDirect3DDevice2_GetRenderState,
  IDirect3DDevice2_SetRenderState,
  IDirect3DDevice2_GetLightState,
  IDirect3DDevice2_SetLightState,
  IDirect3DDevice2_SetTransform,
  IDirect3DDevice2_GetTransform,
  IDirect3DDevice2_MultiplyTransform,
  
  IDirect3DDevice2_DrawPrimitive,
  IDirect3DDevice2_DrawIndexedPrimitive,
  
  IDirect3DDevice2_SetClipStatus,
  IDirect3DDevice2_GetClipStatus,
};

/*******************************************************************************
 *				Direct3DDevice
 */
int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  D3DDEVICEDESC	d1,d2;
  
  TRACE(ddraw," Enumerating OpenGL D3D device.\n");
  
  fill_opengl_caps(&d1, &d2);
  
  return cb((void*)&IID_D3DDEVICE_OpenGL,"WINE Direct3D using OpenGL","direct3d",&d1,&d2,context);
}

float id_mat[16] = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0
};

int is_OpenGL_dx3(REFCLSID rguid, LPDIRECTDRAWSURFACE surface, LPDIRECT3DDEVICE *device)
{
  if (!memcmp(&IID_D3DDEVICE_OpenGL,rguid,sizeof(IID_D3DDEVICE_OpenGL))) {
    OpenGL_IDirect3DDevice *odev;
       
    *device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OpenGL_IDirect3DDevice));
    odev = (OpenGL_IDirect3DDevice *) (*device);
    (*device)->ref = 1;
    (*device)->lpvtbl = &OpenGL_vtable_dx3;
    (*device)->d3d = NULL;
    (*device)->surface = surface;
    
    (*device)->viewport_list = NULL;
    (*device)->current_viewport = NULL;
    
    (*device)->set_context = set_context_dx3;
    
    TRACE(ddraw, "OpenGL device created \n");
    
    /* Create the OpenGL context */
    odev->ctx = OSMesaCreateContext(OSMESA_RGBA, NULL);
    odev->buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
			     surface->s.surface_desc.dwWidth * surface->s.surface_desc.dwHeight * 4);
    odev->rs.src = GL_ONE;
    odev->rs.dst = GL_ZERO;
    odev->rs.mag = GL_NEAREST;
    odev->rs.min = GL_NEAREST;

    odev->world_mat = (LPD3DMATRIX) &id_mat;
    odev->view_mat  = (LPD3DMATRIX) &id_mat;
    odev->proj_mat  = (LPD3DMATRIX) &id_mat;

    /* Initialisation */
    (*device)->set_context(*device);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
    
    return 1;
  }
  
  /* This is not the OpenGL UID */
  return DD_OK;
}


/*******************************************************************************
 *				Direct3DDevice
 */
static HRESULT WINAPI IDirect3DDevice_QueryInterface(LPDIRECT3DDEVICE this,
						     REFIID riid,
						     LPVOID* ppvObj)
{
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME(ddraw, "(%p)->(%s,%p): stub\n", this, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DDevice_AddRef(LPDIRECT3DDEVICE this)
{
  TRACE(ddraw, "(%p)->()incrementing from %lu.\n", this, this->ref );
  
  return ++(this->ref);
}



static ULONG WINAPI IDirect3DDevice_Release(LPDIRECT3DDEVICE this)
{
  FIXME( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
  
  if (!--(this->ref)) {
    HeapFree(GetProcessHeap(),0,this);
    return 0;
  }
  
  return this->ref;
}

static HRESULT WINAPI IDirect3DDevice_Initialize(LPDIRECT3DDEVICE this,
						 LPDIRECT3D lpd3d,
						 LPGUID lpGUID,
						 LPD3DDEVICEDESC lpd3ddvdesc)
{
  TRACE(ddraw, "(%p)->(%p,%p,%p): stub\n", this, lpd3d,lpGUID, lpd3ddvdesc);
  
  return DDERR_ALREADYINITIALIZED;
}


static HRESULT WINAPI IDirect3DDevice_GetCaps(LPDIRECT3DDEVICE this,
					      LPD3DDEVICEDESC lpD3DHWDevDesc,
					      LPD3DDEVICEDESC lpD3DSWDevDesc)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpD3DHWDevDesc, lpD3DSWDevDesc);

  fill_opengl_caps(lpD3DHWDevDesc, lpD3DSWDevDesc);  
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_SwapTextureHandles(LPDIRECT3DDEVICE this,
							 LPDIRECT3DTEXTURE lpD3DTex1,
							 LPDIRECT3DTEXTURE lpD3DTex2)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpD3DTex1, lpD3DTex2);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDevice_CreateExecuteBuffer(LPDIRECT3DDEVICE this,
							  LPD3DEXECUTEBUFFERDESC lpDesc,
							  LPDIRECT3DEXECUTEBUFFER *lplpDirect3DExecuteBuffer,
							  IUnknown *pUnkOuter)
{
  TRACE(ddraw, "(%p)->(%p,%p,%p)\n", this, lpDesc, lplpDirect3DExecuteBuffer, pUnkOuter);

  *lplpDirect3DExecuteBuffer = d3dexecutebuffer_create(this, lpDesc);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_GetStats(LPDIRECT3DDEVICE this,
					       LPD3DSTATS lpD3DStats)
{
  TRACE(ddraw, "(%p)->(%p): stub\n", this, lpD3DStats);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_Execute(LPDIRECT3DDEVICE this,
					      LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
					      LPDIRECT3DVIEWPORT lpDirect3DViewport,
					      DWORD dwFlags)
{
  TRACE(ddraw, "(%p)->(%p,%p,%08ld): stub\n", this, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags);

  /* Put this as the default context */

  /* Execute... */
  lpDirect3DExecuteBuffer->execute(lpDirect3DExecuteBuffer, this, lpDirect3DViewport);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDevice_AddViewport(LPDIRECT3DDEVICE this,
						  LPDIRECT3DVIEWPORT lpvp)
{
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);
  
  /* Adds this viewport to the viewport list */
  lpvp->next = this->viewport_list;
  this->viewport_list = lpvp;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice_DeleteViewport(LPDIRECT3DDEVICE this,
						     LPDIRECT3DVIEWPORT lpvp)
{
  LPDIRECT3DVIEWPORT cur, prev;
  FIXME(ddraw, "(%p)->(%p): stub\n", this, lpvp);
  
  /* Finds this viewport in the list */
  prev = NULL;
  cur = this->viewport_list;
  while ((cur != NULL) && (cur != lpvp)) {
    prev = cur;
    cur = cur->next;
  }
  if (cur == NULL)
    return DDERR_INVALIDOBJECT;
  
  /* And remove it */
  if (prev == NULL)
    this->viewport_list = cur->next;
  else
    prev->next = cur->next;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice_NextViewport(LPDIRECT3DDEVICE this,
						   LPDIRECT3DVIEWPORT lpvp,
						   LPDIRECT3DVIEWPORT* lplpvp,
						   DWORD dwFlags)
{
  FIXME(ddraw, "(%p)->(%p,%p,%08lx): stub\n", this, lpvp, lpvp, dwFlags);
  
  switch (dwFlags) {
  case D3DNEXT_NEXT:
    *lplpvp = lpvp->next;
    break;
    
  case D3DNEXT_HEAD:
    *lplpvp = this->viewport_list;
    break;
    
  case D3DNEXT_TAIL:
    lpvp = this->viewport_list;
    while (lpvp->next != NULL)
      lpvp = lpvp->next;
    
    *lplpvp = lpvp;
    break;
    
  default:
    return DDERR_INVALIDPARAMS;
  }
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDevice_Pick(LPDIRECT3DDEVICE this,
					   LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
					   LPDIRECT3DVIEWPORT lpDirect3DViewport,
					   DWORD dwFlags,
					   LPD3DRECT lpRect)
{
  TRACE(ddraw, "(%p)->(%p,%p,%08lx,%p): stub\n", this, lpDirect3DExecuteBuffer, lpDirect3DViewport,
	dwFlags, lpRect);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_GetPickRecords(LPDIRECT3DDEVICE this,
						     LPDWORD lpCount,
						     LPD3DPICKRECORD lpD3DPickRec)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpCount, lpD3DPickRec);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_EnumTextureFormats(LPDIRECT3DDEVICE this,
							 LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc,
							 LPVOID lpArg)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpd3dEnumTextureProc, lpArg);
  
  return enum_texture_format_OpenGL(lpd3dEnumTextureProc, lpArg);
}


static HRESULT WINAPI IDirect3DDevice_CreateMatrix(LPDIRECT3DDEVICE this,
						   LPD3DMATRIXHANDLE lpD3DMatHandle)
{
  TRACE(ddraw, "(%p)->(%p)\n", this, lpD3DMatHandle);

  *lpD3DMatHandle = (D3DMATRIXHANDLE) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(D3DMATRIX));
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_SetMatrix(LPDIRECT3DDEVICE this,
						D3DMATRIXHANDLE d3dMatHandle,
						const LPD3DMATRIX lpD3DMatrix)
{
  TRACE(ddraw, "(%p)->(%08lx,%p)\n", this, d3dMatHandle, lpD3DMatrix);

  dump_mat(lpD3DMatrix);
  
  *((D3DMATRIX *) d3dMatHandle) = *lpD3DMatrix;
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_GetMatrix(LPDIRECT3DDEVICE this,
						D3DMATRIXHANDLE D3DMatHandle,
						LPD3DMATRIX lpD3DMatrix)
{
  TRACE(ddraw, "(%p)->(%08lx,%p)\n", this, D3DMatHandle, lpD3DMatrix);

  *lpD3DMatrix = *((D3DMATRIX *) D3DMatHandle);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_DeleteMatrix(LPDIRECT3DDEVICE this,
						   D3DMATRIXHANDLE d3dMatHandle)
{
  TRACE(ddraw, "(%p)->(%08lx)\n", this, d3dMatHandle);

  HeapFree(GetProcessHeap(),0, (void *) d3dMatHandle);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDevice_BeginScene(LPDIRECT3DDEVICE this)
{
  /* OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) this; */
  
  FIXME(ddraw, "(%p)->(): stub\n", this);
  
  /* We get the pointer to the surface (should be done on flip) */
  /* odev->zb->pbuf = this->surface->s.surface_desc.y.lpSurface; */
  
  return DD_OK;
}


/* This is for the moment copy-pasted from IDirect3DDevice2...
   Will make a common function ... */
static HRESULT WINAPI IDirect3DDevice_EndScene(LPDIRECT3DDEVICE this)
{
  OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) this;
  LPDIRECTDRAWSURFACE3 surf = (LPDIRECTDRAWSURFACE3) this->surface;
  DDSURFACEDESC sdesc;
  int x,y;
  unsigned char *src;
  unsigned short *dest;
  
  FIXME(ddraw, "(%p)->(): stub\n", this);

  /* Here we copy back the OpenGL scene to the the DDraw surface */
  /* First, lock the surface */
  surf->lpvtbl->fnLock(surf,NULL,&sdesc,DDLOCK_WRITEONLY,0);

  /* The copy the OpenGL buffer to this surface */
  
  /* NOTE : this is only for debugging purpose. I KNOW it is really unoptimal.
     I am currently working on a set of patches for Mesa to have OSMesa support
     16 bpp surfaces => we will able to render directly onto the surface, no
     need to do a bpp conversion */
  dest = (unsigned short *) sdesc.y.lpSurface;
  src = ((unsigned char *) odev->buffer) + 4 * (sdesc.dwWidth * (sdesc.dwHeight - 1));
  for (y = 0; y < sdesc.dwHeight; y++) {
    unsigned char *lsrc = src;
    
    for (x = 0; x < sdesc.dwWidth ; x++) {
      unsigned char r =  *lsrc++;
      unsigned char g =  *lsrc++;
      unsigned char b =  *lsrc++;
      lsrc++; /* Alpha */

      *dest = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
      
      dest++;
    }

    src -= 4 * sdesc.dwWidth;
  }

  /* Unlock the surface */
  surf->lpvtbl->fnUnlock(surf,sdesc.y.lpSurface);
  
  return DD_OK;  
}


static HRESULT WINAPI IDirect3DDevice_GetDirect3D(LPDIRECT3DDEVICE this,
						  LPDIRECT3D *lpDirect3D)
{
  TRACE(ddraw, "(%p)->(%p): stub\n", this, lpDirect3D);

  return DD_OK;
}



/*******************************************************************************
 *				Direct3DDevice VTable
 */
static IDirect3DDevice_VTable OpenGL_vtable_dx3 = {
  IDirect3DDevice_QueryInterface,
  IDirect3DDevice_AddRef,
  IDirect3DDevice_Release,
  IDirect3DDevice_Initialize,
  IDirect3DDevice_GetCaps,
  IDirect3DDevice_SwapTextureHandles,
  IDirect3DDevice_CreateExecuteBuffer,
  IDirect3DDevice_GetStats,
  IDirect3DDevice_Execute,
  IDirect3DDevice_AddViewport,
  IDirect3DDevice_DeleteViewport,
  IDirect3DDevice_NextViewport,
  IDirect3DDevice_Pick,
  IDirect3DDevice_GetPickRecords,
  IDirect3DDevice_EnumTextureFormats,
  IDirect3DDevice_CreateMatrix,
  IDirect3DDevice_SetMatrix,
  IDirect3DDevice_GetMatrix,
  IDirect3DDevice_DeleteMatrix,
  IDirect3DDevice_BeginScene,
  IDirect3DDevice_EndScene,
  IDirect3DDevice_GetDirect3D,
};

#else /* HAVE_MESAGL */

int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  return 0;
}

int is_OpenGL(REFCLSID rguid, LPDIRECTDRAWSURFACE surface, LPDIRECT3DDEVICE2 *device, LPDIRECT3D2 d3d)
{
  return 0;
}

int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  return 0;
}

int is_OpenGL_dx3(REFCLSID rguid, LPDIRECTDRAWSURFACE surface, LPDIRECT3DDEVICE *device)
{
  return 0;
}

#endif /* HAVE_MESAGL */
