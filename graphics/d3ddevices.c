/* Direct3D Device
   (c) 1998 Lionel ULMER
   
   This files contains all the D3D devices that Wine supports. For the moment
   only the 'OpenGL' target is supported. */

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

/* Define this variable if you have an unpatched Mesa 3.0 (patches are available
   on Mesa's home page) or version 3.1b.

   Version 3.1b2 should correct this bug */
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


static ICOM_VTABLE(IDirect3DDevice2) OpenGL_vtable;
static ICOM_VTABLE(IDirect3DDevice) OpenGL_vtable_dx3;

/*******************************************************************************
 *				OpenGL static functions
 */
static void set_context(IDirect3DDevice2Impl* This) {
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;

#ifdef USE_OSMESA
  OSMesaMakeCurrent(odev->ctx, odev->buffer,
		    GL_UNSIGNED_BYTE,
		    This->surface->s.surface_desc.dwWidth,
		    This->surface->s.surface_desc.dwHeight);
#else
  if (glXMakeCurrent(display,
		     odev->common.surface->s.ddraw->d.drawable,
		     odev->ctx) == False) {
    ERR("Error in setting current context (context %p drawable %ld)!\n",
	odev->ctx, odev->common.surface->s.ddraw->d.drawable);
}
#endif
}

static void set_context_dx3(IDirect3DDeviceImpl* This) {
  OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) This;

#ifdef USE_OSMESA
  OSMesaMakeCurrent(odev->ctx, odev->buffer,
		    GL_UNSIGNED_BYTE,
		    This->surface->s.surface_desc.dwWidth,
		    This->surface->s.surface_desc.dwHeight);
#else
  if (glXMakeCurrent(display,
		     odev->common.surface->s.ddraw->d.drawable,
		     odev->ctx) == False) {
    ERR("Error in setting current context !\n");
  }
#endif
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
  
  TRACE(" Enumerating OpenGL D3D device.\n");
  
  fill_opengl_caps(&d1, &d2);
  
  return cb((void*)&IID_D3DDEVICE2_OpenGL,"WINE Direct3D using OpenGL","direct3d",&d1,&d2,context);
}

int is_OpenGL(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDevice2Impl** device, IDirect3D2Impl* d3d)
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
#ifndef USE_OSMESA
    int attributeList[]={ GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    XVisualInfo *xvis;
#endif
    
    *device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OpenGL_IDirect3DDevice2));
    odev = (OpenGL_IDirect3DDevice2 *) (*device);
    (*device)->ref = 1;
    ICOM_VTBL(*device) = &OpenGL_vtable;
    (*device)->d3d = d3d;
    (*device)->surface = surface;
    
    (*device)->viewport_list = NULL;
    (*device)->current_viewport = NULL;
    
    (*device)->set_context = set_context;
    
    TRACE("Creating OpenGL device for surface %p\n", surface);
    
    /* Create the OpenGL context */
#ifdef USE_OSMESA
    odev->ctx = OSMesaCreateContext(OSMESA_RGBA, NULL);
    odev->buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
			     surface->s.surface_desc.dwWidth * surface->s.surface_desc.dwHeight * 4);
#else
    /* First get the correct visual */
    /* if (surface->s.backbuffer == NULL)
       attributeList[3] = None; */
    ENTER_GL();
    xvis = glXChooseVisual(display,
			   DefaultScreen(display),
			   attributeList);
    if (xvis == NULL)
      ERR("No visual found !\n");
    else
      TRACE("Visual found\n");
    /* Create the context */
    odev->ctx = glXCreateContext(display,
				 xvis,
				 NULL,
				 GL_TRUE);
    if (odev->ctx == NULL)
      ERR("Error in context creation !\n");
    else
      TRACE("Context created (%p)\n", odev->ctx);
    
    /* Now override the surface's Flip method (if in double buffering) */
    surface->s.d3d_device = (void *) odev;
    {
	int i;
	struct _surface_chain *chain = surface->s.chain;
	for (i=0;i<chain->nrofsurfaces;i++)
	  if (chain->surfaces[i]->s.surface_desc.ddsCaps.dwCaps & DDSCAPS_FLIP)
	      chain->surfaces[i]->s.d3d_device = (void *) odev;
    }
#endif
    odev->rs.src = GL_ONE;
    odev->rs.dst = GL_ZERO;
    odev->rs.mag = GL_NEAREST;
    odev->rs.min = GL_NEAREST;
    odev->vt     = 0;
    
    memcpy(odev->world_mat, id_mat, 16 * sizeof(float));
    memcpy(odev->view_mat , id_mat, 16 * sizeof(float));
    memcpy(odev->proj_mat , id_mat, 16 * sizeof(float));

    /* Initialisation */
    TRACE("Setting current context\n");
    (*device)->set_context(*device);
    TRACE("Current context set\n");
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
    LEAVE_GL();
    
    TRACE("OpenGL device created \n");
    
    return 1;
  }
  
  /* This is not the OpenGL UID */
  return 0;
}

/*******************************************************************************
 *				Common IDirect3DDevice2
 */

static HRESULT WINAPI IDirect3DDevice2Impl_QueryInterface(LPDIRECT3DDEVICE2 iface,
						      REFIID riid,
						      LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME("(%p)->(%s,%p): stub\n", This, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DDevice2Impl_AddRef(LPDIRECT3DDEVICE2 iface)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DDevice2Impl_Release(LPDIRECT3DDEVICE2 iface)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );
  
  if (!--(This->ref)) {
    OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;

#ifdef USE_OSMESA
    OSMesaDestroyContext(odev->ctx);
#else
    ENTER_GL();
    glXDestroyContext(display,
		      odev->ctx);
    LEAVE_GL();
#endif
    
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  
  return This->ref;
}


/*** IDirect3DDevice2 methods ***/
static HRESULT WINAPI IDirect3DDevice2Impl_GetCaps(LPDIRECT3DDEVICE2 iface,
					       LPD3DDEVICEDESC lpdescsoft,
					       LPD3DDEVICEDESC lpdeschard)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lpdescsoft, lpdeschard);
  
  fill_opengl_caps(lpdescsoft, lpdeschard);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_SwapTextureHandles(LPDIRECT3DDEVICE2 iface,
							  LPDIRECT3DTEXTURE2 lptex1,
							  LPDIRECT3DTEXTURE2 lptex2)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lptex1, lptex2);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_GetStats(LPDIRECT3DDEVICE2 iface,
						LPD3DSTATS lpstats)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p): stub\n", This, lpstats);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_AddViewport(LPDIRECT3DDEVICE2 iface,
						   LPDIRECT3DVIEWPORT2 lpvp)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
  FIXME("(%p)->(%p): stub\n", This, ilpvp);
  
  /* Adds this viewport to the viewport list */
  ilpvp->next = This->viewport_list;
  This->viewport_list = ilpvp;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_DeleteViewport(LPDIRECT3DDEVICE2 iface,
						      LPDIRECT3DVIEWPORT2 lpvp)
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



static HRESULT WINAPI IDirect3DDevice2Impl_NextViewport(LPDIRECT3DDEVICE2 iface,
						    LPDIRECT3DVIEWPORT2 lpvp,
						    LPDIRECT3DVIEWPORT2* lplpvp,
						    DWORD dwFlags)
{
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
  
  TRACE("Enumerating GL_RGBA unpacked (32)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->u.dwRGBBitCount = 32;
  pformat->u1.dwRBitMask =         0xFF000000;
  pformat->u2.dwGBitMask =         0x00FF0000;
  pformat->u3.dwBBitMask =        0x0000FF00;
  pformat->u4.dwRGBAlphaBitMask = 0x000000FF;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGB unpacked (24)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->u.dwRGBBitCount = 24;
  pformat->u1.dwRBitMask =  0x00FF0000;
  pformat->u2.dwGBitMask =  0x0000FF00;
  pformat->u3.dwBBitMask = 0x000000FF;
  pformat->u4.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

#ifndef HAVE_BUGGY_MESAGL
  /* The packed texture format are buggy in Mesa. The bug was reported and corrected,
     so that future version will work great. */
  TRACE("Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_6_5 (16)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->u.dwRGBBitCount = 16;
  pformat->u1.dwRBitMask =  0x0000F800;
  pformat->u2.dwGBitMask =  0x000007E0;
  pformat->u3.dwBBitMask = 0x0000001F;
  pformat->u4.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_5_5_5_1 (16)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->u.dwRGBBitCount = 16;
  pformat->u1.dwRBitMask =         0x0000F800;
  pformat->u2.dwGBitMask =         0x000007C0;
  pformat->u3.dwBBitMask =        0x0000003E;
  pformat->u4.dwRGBAlphaBitMask = 0x00000001;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_4_4_4_4 (16)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->u.dwRGBBitCount = 16;
  pformat->u1.dwRBitMask =         0x0000F000;
  pformat->u2.dwGBitMask =         0x00000F00;
  pformat->u3.dwBBitMask =        0x000000F0;
  pformat->u4.dwRGBAlphaBitMask = 0x0000000F;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGB packed GL_UNSIGNED_BYTE_3_3_2 (8)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->u.dwRGBBitCount = 8;
  pformat->u1.dwRBitMask =         0x0000F800;
  pformat->u2.dwGBitMask =         0x000007C0;
  pformat->u3.dwBBitMask =        0x0000003E;
  pformat->u4.dwRGBAlphaBitMask = 0x00000001;
  if (cb(&sdesc, context) == 0)
    return DD_OK;
#endif

#if defined(HAVE_GL_COLOR_TABLE) && defined(HAVE_GL_PALETTED_TEXTURE)
  TRACE("Enumerating Paletted (8)\n");
  pformat->dwFlags = DDPF_PALETTEINDEXED8;
  pformat->u.dwRGBBitCount = 8;
  pformat->u1.dwRBitMask =  0x00000000;
  pformat->u2.dwGBitMask =  0x00000000;
  pformat->u3.dwBBitMask = 0x00000000;
  pformat->u4.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;
#endif
  
  TRACE("End of enumeration\n");
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDevice2Impl_EnumTextureFormats(LPDIRECT3DDEVICE2 iface,
							  LPD3DENUMTEXTUREFORMATSCALLBACK cb,
							  LPVOID context)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, cb, context);
  
  return enum_texture_format_OpenGL(cb, context);
}



static HRESULT WINAPI IDirect3DDevice2Impl_BeginScene(LPDIRECT3DDEVICE2 iface)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  /* OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This; */
  
  FIXME("(%p)->(): stub\n", This);
  
  /* Here, we should get the DDraw surface and 'copy it' to the
     OpenGL surface.... */
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_EndScene(LPDIRECT3DDEVICE2 iface)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
#ifdef USE_OSMESA
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;
  LPDIRECTDRAWSURFACE3 surf = (LPDIRECTDRAWSURFACE3) This->surface;
  DDSURFACEDESC sdesc;
  int x,y;
  unsigned char *src;
  unsigned short *dest;
#endif
  
  FIXME("(%p)->(): stub\n", This);

#ifdef USE_OSMESA
  /* Here we copy back the OpenGL scene to the the DDraw surface */
  /* First, lock the surface */
  IDirectDrawSurface3_Lock(surf,NULL,&sdesc,DDLOCK_WRITEONLY,0);

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
  IDirectDrawSurface3_Unlock(surf,sdesc.y.lpSurface);
#else
  /* No need to do anything here... */
#endif
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_GetDirect3D(LPDIRECT3DDEVICE2 iface, LPDIRECT3D2 *lpd3d2)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  TRACE("(%p)->(%p): stub\n", This, lpd3d2);
  
  *lpd3d2 = (LPDIRECT3D2)This->d3d;
  
  return DD_OK;
}



/*** DrawPrimitive API ***/
static HRESULT WINAPI IDirect3DDevice2Impl_SetCurrentViewport(LPDIRECT3DDEVICE2 iface,
							  LPDIRECT3DVIEWPORT2 lpvp)
{
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



static HRESULT WINAPI IDirect3DDevice2Impl_GetCurrentViewport(LPDIRECT3DDEVICE2 iface,
							  LPDIRECT3DVIEWPORT2 *lplpvp)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p): stub\n", This, lplpvp);
  
  /* Returns the current viewport */
  *lplpvp = (LPDIRECT3DVIEWPORT2)This->current_viewport;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_SetRenderTarget(LPDIRECT3DDEVICE2 iface,
						       LPDIRECTDRAWSURFACE lpdds,
						       DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p,%08lx): stub\n", This, lpdds, dwFlags);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_GetRenderTarget(LPDIRECT3DDEVICE2 iface,
						       LPDIRECTDRAWSURFACE *lplpdds)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p): stub\n", This, lplpdds);
  
  /* Returns the current rendering target (the surface on wich we render) */
  *lplpdds = (LPDIRECTDRAWSURFACE)This->surface;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_Begin(LPDIRECT3DDEVICE2 iface,
					     D3DPRIMITIVETYPE d3dp,
					     D3DVERTEXTYPE d3dv,
					     DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%d,%08lx): stub\n", This, d3dp, d3dv, dwFlags);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_BeginIndexed(LPDIRECT3DDEVICE2 iface,
						    D3DPRIMITIVETYPE d3dp,
						    D3DVERTEXTYPE d3dv,
						    LPVOID lpvert,
						    DWORD numvert,
						    DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%d,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvert, numvert, dwFlags);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_Vertex(LPDIRECT3DDEVICE2 iface,
					      LPVOID lpvert)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p): stub\n", This, lpvert);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_Index(LPDIRECT3DDEVICE2 iface,
					     WORD index)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d): stub\n", This, index);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_End(LPDIRECT3DDEVICE2 iface,
					   DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%08lx): stub\n", This, dwFlags);
  
  return DD_OK;
}




static HRESULT WINAPI IDirect3DDevice2Impl_GetRenderState(LPDIRECT3DDEVICE2 iface,
						      D3DRENDERSTATETYPE d3drs,
						      LPDWORD lprstate)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%p): stub\n", This, d3drs, lprstate);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_SetRenderState(LPDIRECT3DDEVICE2 iface,
						      D3DRENDERSTATETYPE dwRenderStateType,
						      DWORD dwRenderState)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;

  TRACE("(%p)->(%d,%ld)\n", This, dwRenderStateType, dwRenderState);
  
  /* Call the render state functions */
  set_render_state(dwRenderStateType, dwRenderState, &(odev->rs));
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_GetLightState(LPDIRECT3DDEVICE2 iface,
						     D3DLIGHTSTATETYPE d3dls,
						     LPDWORD lplstate)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%p): stub\n", This, d3dls, lplstate);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_SetLightState(LPDIRECT3DDEVICE2 iface,
						     D3DLIGHTSTATETYPE dwLightStateType,
						     DWORD dwLightState)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%08lx): stub\n", This, dwLightStateType, dwLightState);
  
  switch (dwLightStateType) {
  case D3DLIGHTSTATE_MATERIAL: {  /* 1 */
    IDirect3DMaterial2Impl* mat = (IDirect3DMaterial2Impl*) dwLightState;
    
    if (mat != NULL) {
      ENTER_GL();
      mat->activate(mat);
      LEAVE_GL();
    } else {
      TRACE("Zoups !!!\n");
    }
  } break;
    
  case D3DLIGHTSTATE_AMBIENT: {   /* 2 */
    float light[4];
    
    light[0] = ((dwLightState >> 16) & 0xFF) / 255.0;
    light[1] = ((dwLightState >>  8) & 0xFF) / 255.0;
    light[2] = ((dwLightState >>  0) & 0xFF) / 255.0;
    light[3] = 1.0;
    ENTER_GL();
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);
    LEAVE_GL();
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
    TRACE("Unexpected Light State Type\n");
    return DDERR_INVALIDPARAMS;
  }
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_SetTransform(LPDIRECT3DDEVICE2 iface,
						    D3DTRANSFORMSTATETYPE d3dts,
						    LPD3DMATRIX lpmatrix)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;
  
  FIXME("(%p)->(%d,%p): stub\n", This, d3dts, lpmatrix);
  
  ENTER_GL();
  
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
  
  LEAVE_GL();
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_GetTransform(LPDIRECT3DDEVICE2 iface,
						    D3DTRANSFORMSTATETYPE d3dts,
						    LPD3DMATRIX lpmatrix)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%p): stub\n", This, d3dts, lpmatrix);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_MultiplyTransform(LPDIRECT3DDEVICE2 iface,
							 D3DTRANSFORMSTATETYPE d3dts,
							 LPD3DMATRIX lpmatrix)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%d,%p): stub\n", This, d3dts, lpmatrix);
  
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
      TRACE("Standard Vertex\n");					\
      glEnable(GL_LIGHTING);							\
      break;									\
										\
    case D3DVT_LVERTEX:								\
      TRACE("Lighted Vertex\n");						\
      glDisable(GL_LIGHTING);							\
      break;									\
										\
    case D3DVT_TLVERTEX: {							\
      GLdouble height, width, minZ, maxZ;					\
										\
      TRACE("Transformed - Lighted Vertex\n");				\
      /* First, disable lighting */						\
      glDisable(GL_LIGHTING);							\
										\
      /* Then do not put any transformation matrixes */				\
      glMatrixMode(GL_MODELVIEW);						\
      glLoadIdentity();								\
      glMatrixMode(GL_PROJECTION);						\
      glLoadIdentity();								\
										\
      if (This->current_viewport == NULL) {					\
	ERR("No current viewport !\n");					\
	/* Using standard values */						\
	height = 640.0;								\
	width = 480.0;								\
	minZ = -10.0;								\
	maxZ = 10.0;								\
      } else {									\
	if (This->current_viewport->use_vp2) {					\
	  height = (GLdouble) This->current_viewport->viewport.vp2.dwHeight;	\
	  width  = (GLdouble) This->current_viewport->viewport.vp2.dwWidth;	\
	  minZ   = (GLdouble) This->current_viewport->viewport.vp2.dvMinZ;	\
	  maxZ   = (GLdouble) This->current_viewport->viewport.vp2.dvMaxZ;	\
	} else {								\
	  height = (GLdouble) This->current_viewport->viewport.vp1.dwHeight;	\
	  width  = (GLdouble) This->current_viewport->viewport.vp1.dwWidth;	\
	  minZ   = (GLdouble) This->current_viewport->viewport.vp1.dvMinZ;	\
	  maxZ   = (GLdouble) This->current_viewport->viewport.vp1.dvMaxZ;	\
	}									\
      }										\
										\
      glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);				\
    } break;									\
										\
    default:									\
      ERR("Unhandled vertex type\n");					\
      break;									\
    }										\
										\
    odev->vt = d3dv;								\
  }										\
										\
  switch (d3dp) {								\
  case D3DPT_POINTLIST:								\
    TRACE("Start POINTS\n");						\
    glBegin(GL_POINTS);								\
    break;									\
										\
  case D3DPT_LINELIST:								\
    TRACE("Start LINES\n");						\
    glBegin(GL_LINES);								\
    break;									\
										\
  case D3DPT_LINESTRIP:								\
    TRACE("Start LINE_STRIP\n");						\
    glBegin(GL_LINE_STRIP);							\
    break;									\
										\
  case D3DPT_TRIANGLELIST:							\
    TRACE("Start TRIANGLES\n");						\
    glBegin(GL_TRIANGLES);							\
    break;									\
										\
  case D3DPT_TRIANGLESTRIP:							\
    TRACE("Start TRIANGLE_STRIP\n");					\
    glBegin(GL_TRIANGLE_STRIP);							\
    break;									\
										\
  case D3DPT_TRIANGLEFAN:							\
    TRACE("Start TRIANGLE_FAN\n");					\
    glBegin(GL_TRIANGLE_FAN);							\
    break;									\
										\
  default:									\
    TRACE("Unhandled primitive\n");					\
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
      TRACE("   V: %f %f %f\n", vx->x.x, vx->y.y, vx->z.z);		\
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
      TRACE("  LV: %f %f %f (%02lx %02lx %02lx)\n",			\
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
      TRACE(" TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",		\
	    vx->x.sx, vx->y.sy, vx->z.sz,					\
	    ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),	\
	    vx->u.tu, vx->v.tv, vx->r.rhw);					\
    } break;									\
										\
    default:									\
      TRACE("Unhandled vertex type\n");					\
      break;									\
    }										\
  }										\
										\
  glEnd();									\
  TRACE("End\n");


static HRESULT WINAPI IDirect3DDevice2Impl_DrawPrimitive(LPDIRECT3DDEVICE2 iface,
						     D3DPRIMITIVETYPE d3dp,
						     D3DVERTEXTYPE d3dv,
						     LPVOID lpvertex,
						     DWORD vertcount,
						     DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;
  int vx_index;
  
  TRACE("(%p)->(%d,%d,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvertex, vertcount, dwFlags);

  ENTER_GL();
  DRAW_PRIMITIVE(vertcount, vx_index);
  LEAVE_GL();
    
  return D3D_OK;
      }
    

      
static HRESULT WINAPI IDirect3DDevice2Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE2 iface,
							    D3DPRIMITIVETYPE d3dp,
							    D3DVERTEXTYPE d3dv,
							    LPVOID lpvertex,
							    DWORD vertcount,
							    LPWORD lpindexes,
							    DWORD indexcount,
							    DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  OpenGL_IDirect3DDevice2 *odev = (OpenGL_IDirect3DDevice2 *) This;
  int vx_index;
  
  TRACE("(%p)->(%d,%d,%p,%ld,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvertex, vertcount, lpindexes, indexcount, dwFlags);
  
  ENTER_GL();
  DRAW_PRIMITIVE(indexcount, lpindexes[vx_index]);
  LEAVE_GL();
  
  return D3D_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_SetClipStatus(LPDIRECT3DDEVICE2 iface,
						     LPD3DCLIPSTATUS lpcs)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p): stub\n", This, lpcs);
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDevice2Impl_GetClipStatus(LPDIRECT3DDEVICE2 iface,
						     LPD3DCLIPSTATUS lpcs)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p): stub\n", This, lpcs);
  
  return DD_OK;
}



/*******************************************************************************
 *				OpenGL-specific IDirect3DDevice2
 */

/*******************************************************************************
 *				OpenGL-specific VTable
 */

static ICOM_VTABLE(IDirect3DDevice2) OpenGL_vtable = 
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirect3DDevice2Impl_QueryInterface,
  IDirect3DDevice2Impl_AddRef,
  IDirect3DDevice2Impl_Release,
  /*** IDirect3DDevice2 methods ***/
  IDirect3DDevice2Impl_GetCaps,
  IDirect3DDevice2Impl_SwapTextureHandles,
  IDirect3DDevice2Impl_GetStats,
  IDirect3DDevice2Impl_AddViewport,
  IDirect3DDevice2Impl_DeleteViewport,
  IDirect3DDevice2Impl_NextViewport,
  IDirect3DDevice2Impl_EnumTextureFormats,
  IDirect3DDevice2Impl_BeginScene,
  IDirect3DDevice2Impl_EndScene,
  IDirect3DDevice2Impl_GetDirect3D,
  
  /*** DrawPrimitive API ***/
  IDirect3DDevice2Impl_SetCurrentViewport,
  IDirect3DDevice2Impl_GetCurrentViewport,
  
  IDirect3DDevice2Impl_SetRenderTarget,
  IDirect3DDevice2Impl_GetRenderTarget,
  
  IDirect3DDevice2Impl_Begin,
  IDirect3DDevice2Impl_BeginIndexed,
  IDirect3DDevice2Impl_Vertex,
  IDirect3DDevice2Impl_Index,
  IDirect3DDevice2Impl_End,
  
  IDirect3DDevice2Impl_GetRenderState,
  IDirect3DDevice2Impl_SetRenderState,
  IDirect3DDevice2Impl_GetLightState,
  IDirect3DDevice2Impl_SetLightState,
  IDirect3DDevice2Impl_SetTransform,
  IDirect3DDevice2Impl_GetTransform,
  IDirect3DDevice2Impl_MultiplyTransform,
  
  IDirect3DDevice2Impl_DrawPrimitive,
  IDirect3DDevice2Impl_DrawIndexedPrimitive,
  
  IDirect3DDevice2Impl_SetClipStatus,
  IDirect3DDevice2Impl_GetClipStatus,
};

/*******************************************************************************
 *				Direct3DDevice
 */
int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  D3DDEVICEDESC	d1,d2;
  
  TRACE(" Enumerating OpenGL D3D device.\n");
  
  fill_opengl_caps(&d1, &d2);
  
  return cb((void*)&IID_D3DDEVICE_OpenGL,"WINE Direct3D using OpenGL","direct3d",&d1,&d2,context);
}

float id_mat[16] = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0
};

int is_OpenGL_dx3(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDeviceImpl** device)
{
  if (!memcmp(&IID_D3DDEVICE_OpenGL,rguid,sizeof(IID_D3DDEVICE_OpenGL))) {
    OpenGL_IDirect3DDevice *odev;
#ifndef USE_OSMESA
    int attributeList[]={ GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    XVisualInfo *xvis;
#endif
       
    *device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OpenGL_IDirect3DDevice));
    odev = (OpenGL_IDirect3DDevice *) (*device);
    (*device)->ref = 1;
    ICOM_VTBL(*device) = &OpenGL_vtable_dx3;
    (*device)->d3d = NULL;
    (*device)->surface = surface;
    
    (*device)->viewport_list = NULL;
    (*device)->current_viewport = NULL;
    
    (*device)->set_context = set_context_dx3;
    
    TRACE("OpenGL device created \n");
    
    /* Create the OpenGL context */
#ifdef USE_OSMESA
    odev->ctx = OSMesaCreateContext(OSMESA_RGBA, NULL);
    odev->buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
			     surface->s.surface_desc.dwWidth * surface->s.surface_desc.dwHeight * 4);
#else
    /* First get the correct visual */
    /* if (surface->s.backbuffer == NULL)
       attributeList[3] = None; */
    ENTER_GL();
    xvis = glXChooseVisual(display,
			   DefaultScreen(display),
			   attributeList);
    if (xvis == NULL)
      ERR("No visual found !\n");
    else
      TRACE("Visual found\n");
    /* Create the context */
    odev->ctx = glXCreateContext(display,
				 xvis,
				 NULL,
				 GL_TRUE);
    TRACE("Context created\n");
    
    /* Now override the surface's Flip method (if in double buffering) */
    surface->s.d3d_device = (void *) odev;
    {
	int i;
	struct _surface_chain *chain = surface->s.chain;
	for (i=0;i<chain->nrofsurfaces;i++)
	  if (chain->surfaces[i]->s.surface_desc.ddsCaps.dwCaps & DDSCAPS_FLIP)
	      chain->surfaces[i]->s.d3d_device = (void *) odev;
    }
#endif
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
static HRESULT WINAPI IDirect3DDeviceImpl_QueryInterface(LPDIRECT3DDEVICE iface,
						     REFIID riid,
						     LPVOID* ppvObj)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  char xrefiid[50];
  
  WINE_StringFromCLSID((LPCLSID)riid,xrefiid);
  FIXME("(%p)->(%s,%p): stub\n", This, xrefiid,ppvObj);
  
  return S_OK;
}



static ULONG WINAPI IDirect3DDeviceImpl_AddRef(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->()incrementing from %lu.\n", This, This->ref );
  
  return ++(This->ref);
}



static ULONG WINAPI IDirect3DDeviceImpl_Release(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  FIXME("(%p)->() decrementing from %lu.\n", This, This->ref );
  
  if (!--(This->ref)) {
    OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) This;

#ifdef USE_OSMESA
    OSMesaDestroyContext(odev->ctx);
#else
    ENTER_GL();
    glXDestroyContext(display,
		      odev->ctx);
    LEAVE_GL();
#endif    
    
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  
  return This->ref;
}

static HRESULT WINAPI IDirect3DDeviceImpl_Initialize(LPDIRECT3DDEVICE iface,
						 LPDIRECT3D lpd3d,
						 LPGUID lpGUID,
						 LPD3DDEVICEDESC lpd3ddvdesc)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p,%p): stub\n", This, lpd3d,lpGUID, lpd3ddvdesc);
  
  return DDERR_ALREADYINITIALIZED;
}


static HRESULT WINAPI IDirect3DDeviceImpl_GetCaps(LPDIRECT3DDEVICE iface,
					      LPD3DDEVICEDESC lpD3DHWDevDesc,
					      LPD3DDEVICEDESC lpD3DSWDevDesc)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpD3DHWDevDesc, lpD3DSWDevDesc);

  fill_opengl_caps(lpD3DHWDevDesc, lpD3DSWDevDesc);  
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_SwapTextureHandles(LPDIRECT3DDEVICE iface,
							 LPDIRECT3DTEXTURE lpD3DTex1,
							 LPDIRECT3DTEXTURE lpD3DTex2)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpD3DTex1, lpD3DTex2);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_CreateExecuteBuffer(LPDIRECT3DDEVICE iface,
							  LPD3DEXECUTEBUFFERDESC lpDesc,
							  LPDIRECT3DEXECUTEBUFFER *lplpDirect3DExecuteBuffer,
							  IUnknown *pUnkOuter)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p,%p)\n", This, lpDesc, lplpDirect3DExecuteBuffer, pUnkOuter);

  *lplpDirect3DExecuteBuffer = d3dexecutebuffer_create(This, lpDesc);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_GetStats(LPDIRECT3DDEVICE iface,
					       LPD3DSTATS lpD3DStats)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p): stub\n", This, lpD3DStats);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_Execute(LPDIRECT3DDEVICE iface,
					      LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
					      LPDIRECT3DVIEWPORT lpDirect3DViewport,
					      DWORD dwFlags)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p,%08ld): stub\n", This, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags);

  /* Put this as the default context */

  /* Execute... */
  ((IDirect3DExecuteBufferImpl*)lpDirect3DExecuteBuffer)->execute(lpDirect3DExecuteBuffer, iface, lpDirect3DViewport);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_AddViewport(LPDIRECT3DDEVICE iface,
						  LPDIRECT3DVIEWPORT lpvp)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  IDirect3DViewport2Impl* ilpvp=(IDirect3DViewport2Impl*)lpvp;
  FIXME("(%p)->(%p): stub\n", This, ilpvp);
  
  /* Adds this viewport to the viewport list */
  ilpvp->next = This->viewport_list;
  This->viewport_list = ilpvp;
  
  return DD_OK;
}



static HRESULT WINAPI IDirect3DDeviceImpl_DeleteViewport(LPDIRECT3DDEVICE iface,
						     LPDIRECT3DVIEWPORT lpvp)
{
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



static HRESULT WINAPI IDirect3DDeviceImpl_NextViewport(LPDIRECT3DDEVICE iface,
						   LPDIRECT3DVIEWPORT lpvp,
						   LPDIRECT3DVIEWPORT* lplpvp,
						   DWORD dwFlags)
{
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

static HRESULT WINAPI IDirect3DDeviceImpl_Pick(LPDIRECT3DDEVICE iface,
					   LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
					   LPDIRECT3DVIEWPORT lpDirect3DViewport,
					   DWORD dwFlags,
					   LPD3DRECT lpRect)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p,%08lx,%p): stub\n", This, lpDirect3DExecuteBuffer, lpDirect3DViewport,
	dwFlags, lpRect);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_GetPickRecords(LPDIRECT3DDEVICE iface,
						     LPDWORD lpCount,
						     LPD3DPICKRECORD lpD3DPickRec)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpCount, lpD3DPickRec);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_EnumTextureFormats(LPDIRECT3DDEVICE iface,
							 LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc,
							 LPVOID lpArg)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpd3dEnumTextureProc, lpArg);
  
  return enum_texture_format_OpenGL(lpd3dEnumTextureProc, lpArg);
}


static HRESULT WINAPI IDirect3DDeviceImpl_CreateMatrix(LPDIRECT3DDEVICE iface,
						   LPD3DMATRIXHANDLE lpD3DMatHandle)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p)\n", This, lpD3DMatHandle);

  *lpD3DMatHandle = (D3DMATRIXHANDLE) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(D3DMATRIX));
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_SetMatrix(LPDIRECT3DDEVICE iface,
						D3DMATRIXHANDLE d3dMatHandle,
						const LPD3DMATRIX lpD3DMatrix)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%08lx,%p)\n", This, d3dMatHandle, lpD3DMatrix);

  dump_mat(lpD3DMatrix);
  
  *((D3DMATRIX *) d3dMatHandle) = *lpD3DMatrix;
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_GetMatrix(LPDIRECT3DDEVICE iface,
						D3DMATRIXHANDLE D3DMatHandle,
						LPD3DMATRIX lpD3DMatrix)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%08lx,%p)\n", This, D3DMatHandle, lpD3DMatrix);

  *lpD3DMatrix = *((D3DMATRIX *) D3DMatHandle);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_DeleteMatrix(LPDIRECT3DDEVICE iface,
						   D3DMATRIXHANDLE d3dMatHandle)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%08lx)\n", This, d3dMatHandle);

  HeapFree(GetProcessHeap(),0, (void *) d3dMatHandle);
  
  return DD_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_BeginScene(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  /* OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) This; */
  
  FIXME("(%p)->(): stub\n", This);
  
  /* We get the pointer to the surface (should be done on flip) */
  /* odev->zb->pbuf = This->surface->s.surface_desc.y.lpSurface; */
  
  return DD_OK;
}


/* This is for the moment copy-pasted from IDirect3DDevice2...
   Will make a common function ... */
static HRESULT WINAPI IDirect3DDeviceImpl_EndScene(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
#ifdef USE_OSMESA
  OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) This;
  LPDIRECTDRAWSURFACE3 surf = (LPDIRECTDRAWSURFACE3) This->surface;
  DDSURFACEDESC sdesc;
  int x,y;
  unsigned char *src;
  unsigned short *dest;
#endif
  
  FIXME("(%p)->(): stub\n", This);

#ifdef USE_OSMESA
  /* Here we copy back the OpenGL scene to the the DDraw surface */
  /* First, lock the surface */
  IDirectDrawSurface3_Lock(surf,NULL,&sdesc,DDLOCK_WRITEONLY,0);

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
  IDirectDrawSurface3_Unlock(surf,sdesc.y.lpSurface);
#else
  /* No need to do anything here... */
#endif
  
  return DD_OK;  
}


static HRESULT WINAPI IDirect3DDeviceImpl_GetDirect3D(LPDIRECT3DDEVICE iface,
						  LPDIRECT3D *lpDirect3D)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p): stub\n", This, lpDirect3D);

  return DD_OK;
}



/*******************************************************************************
 *				Direct3DDevice VTable
 */
static ICOM_VTABLE(IDirect3DDevice) OpenGL_vtable_dx3 = 
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

#else /* HAVE_MESAGL */

int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  return 0;
}

int is_OpenGL(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDevice2Impl** device, IDirect3D2Impl* d3d)
{
  return 0;
}

int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  return 0;
}

int is_OpenGL_dx3(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDeviceImpl** device)
{
  return 0;
}

#endif /* HAVE_MESAGL */
