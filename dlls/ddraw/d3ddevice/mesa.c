/* Direct3D Device
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains the MESA implementation of all the D3D devices that
 * Wine supports.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>

#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "mesa_private.h"

#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

ICOM_VTABLE(IDirect3DDevice2) OpenGL_vtable;
ICOM_VTABLE(IDirect3DDevice) OpenGL_vtable_dx3;

/* Define this variable if you have an unpatched Mesa 3.0 (patches are available
   on Mesa's home page) or version 3.1b.

   Version 3.1b2 should correct this bug */
#undef HAVE_BUGGY_MESAGL

#define D3DDPRIVATE(x) mesa_d3dd_private *odev=((mesa_d3dd_private*)x->private)

#ifndef HAVE_GLEXT_PROTOTYPES
/* This is for non-OpenGL ABI compliant glext.h headers :-) */
typedef void (* PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat,
					 GLsizei width, GLenum format, GLenum type,
					 const GLvoid *table);
#endif

static const float id_mat[16] = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0
};

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;

    return display;
}


/* retrieve the X drawable to use on a given DC */
inline static Drawable get_drawable( HDC hdc )
{
    Drawable drawable;
    enum x11drv_escape_codes escape = X11DRV_GET_DRAWABLE;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(drawable), (LPSTR)&drawable )) drawable = 0;

    return drawable;
}


static BOOL opengl_flip( LPVOID display, LPVOID drawable)
{
    TRACE("(%p, %ld)\n",(Display*)display,(Drawable)drawable);
    ENTER_GL();
    glXSwapBuffers((Display*)display,(Drawable)drawable);
    LEAVE_GL();
    return TRUE;
}


/*******************************************************************************
 *				OpenGL static functions
 */
static void set_context(IDirect3DDevice2Impl* This) {
#if COMPILABLE
    D3DDPRIVATE(This);

    if (glXMakeCurrent(gdi_display,ddpriv->drawable, odev->ctx) == False) {
	ERR("Error in setting current context (context %p drawable %ld)!\n",
	    odev->ctx, ddpriv->drawable);
    }
#endif
    D3DDPRIVATE(This);

    ENTER_GL();
    TRACE("glxMakeCurrent %p, %ld, %p\n",odev->gdi_display,odev->drawable, odev->ctx);
    if (glXMakeCurrent(odev->gdi_display,odev->drawable, odev->ctx) == False) {
	ERR("Error in setting current context (context %p drawable %ld)!\n",
	    odev->ctx, odev->drawable);
    }
    LEAVE_GL();
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

static void fill_device_capabilities(IDirectDrawImpl* ddraw) {
#if COMPILABLE
  x11_dd_private *private = (x11_dd_private *) ddraw->d->private;
  const char *ext_string;
  Mesa_DeviceCapabilities *devcap;

  private->device_capabilities = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Mesa_DeviceCapabilities));
  devcap = (Mesa_DeviceCapabilities *) private->device_capabilities;

  ENTER_GL();
  ext_string = glGetString(GL_EXTENSIONS);
  /* Query for the ColorTable Extension */
  if (strstr(ext_string, "GL_EXT_paletted_texture")) {
    devcap->ptr_ColorTableEXT = (PFNGLCOLORTABLEEXTPROC) glXGetProcAddressARB("glColorTableEXT");
    TRACE("Color table extension supported (function at %p)\n", devcap->ptr_ColorTableEXT);
  } else {
    TRACE("Color table extension not found.\n");
  }
  LEAVE_GL();
#endif
}

int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  D3DDEVICEDESC	d1,d2;
  TRACE(" Enumerating OpenGL D3D2 device (IID %s).\n", debugstr_guid(&IID_D3DDEVICE2_OpenGL));
  fill_opengl_caps(&d1, &d2);
  return cb((void*)&IID_D3DDEVICE2_OpenGL,"WINE Direct3D2 using OpenGL","direct3d",&d1,&d2,context);
}

int
is_OpenGL(
    REFCLSID rguid, IDirectDrawSurfaceImpl* surface,
    IDirect3DDevice2Impl** device, IDirect3D2Impl* d3d
) {
  mesa_d3dd_private *odev = NULL;
  HDC device_context;
  XVisualInfo *vis;
  int num;
  XVisualInfo template;
  IDirectDrawSurfaceImpl* surf;

  TRACE("rguid = %s, surface = %p, &device = %p, d3d = %p\n",debugstr_guid(rguid),surface,device,d3d);
  if (/* Default device */
      (rguid == NULL) ||
      /* HAL Device */
      (!memcmp(&IID_IDirect3DHALDevice,rguid,sizeof(IID_IDirect3DHALDevice))) ||
      /* OpenGL Device */
      (!memcmp(&IID_D3DDEVICE2_OpenGL,rguid,sizeof(IID_D3DDEVICE2_OpenGL)))) {

    *device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DDevice2Impl));
    (*device)->private = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(mesa_d3dd_private));
    odev = (mesa_d3dd_private*)(*device)->private;
    (*device)->ref = 1;
    ICOM_VTBL(*device) = &OpenGL_vtable;
    (*device)->d3d = d3d;
    (*device)->surface = surface;
    (*device)->viewport_list = NULL;
    (*device)->current_viewport = NULL;
    (*device)->current_texture = NULL;
    (*device)->set_context = set_context;

    TRACE("Creating OpenGL device for surface %p\n", surface);
    /* Create the OpenGL context */
    /* First get the correct visual */
    /* Create the context */
      
    device_context = GetDC((*device)->surface->ddraw_owner->window);
    odev->gdi_display = get_display(device_context);
    odev->drawable = get_drawable(device_context);
    ReleaseDC((*device)->surface->ddraw_owner->window,device_context);
    ENTER_GL();

    template.visualid = GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
    vis = XGetVisualInfo(odev->gdi_display, VisualIDMask, &template, &num);
    if (vis == NULL)
      ERR("No visual found !\n");
    else
      TRACE("Visual found\n");

    odev->ctx = glXCreateContext(odev->gdi_display, vis,
				   NULL, GL_TRUE);

    if (odev->ctx == NULL)
      ERR("Error in context creation !\n");
    else
      TRACE("Context created (%p)\n", odev->ctx);

    /* Look for the front buffer and override its surface's Flip method (if in double buffering) */ 
    for (surf = surface; surf != NULL; surf = surf->surface_owner)
        if ((surf->surface_desc.ddsCaps.dwCaps&(DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	    == (DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	{
            surface->surface_owner->aux_ctx  = (LPVOID)odev->gdi_display;
            surface->surface_owner->aux_data = (LPVOID)odev->drawable;
            surface->surface_owner->aux_flip = opengl_flip;
            break;
        }

    odev->rs.src = GL_ONE;
    odev->rs.dst = GL_ZERO;
    odev->rs.mag = GL_NEAREST;
    odev->rs.min = GL_NEAREST;
    odev->vt     = 0;

    /* Allocate memory for the matrices */
    odev->world_mat = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    odev->view_mat  = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    odev->proj_mat  = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));

    memcpy(odev->world_mat, id_mat, 16 * sizeof(float));
    memcpy(odev->view_mat , id_mat, 16 * sizeof(float));
    memcpy(odev->proj_mat , id_mat, 16 * sizeof(float));

    /* Initialisation */
    TRACE("Setting current context\n");
    LEAVE_GL();
    (*device)->set_context(*device);
    ENTER_GL();
    TRACE("Current context set\n");
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
    LEAVE_GL();

    fill_device_capabilities(d3d->ddraw);

    TRACE("OpenGL device created \n");
    return 1;
  }
  FIXME("bad IID %s\n",debugstr_guid(rguid));
  /* This is not the OpenGL UID */
  return 0;
}

/*******************************************************************************
 *				MESA IDirect3DDevice2
 */
static ULONG WINAPI MESA_IDirect3DDevice2Impl_Release(LPDIRECT3DDEVICE2 iface)
{
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

  if (!--(This->ref)) {
    D3DDPRIVATE(This);

    ENTER_GL();
    glXDestroyContext(odev->gdi_display, odev->ctx);
    LEAVE_GL();
    HeapFree(GetProcessHeap(),0,This->private);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  return This->ref;
}

/*** IDirect3DDevice2 methods ***/
static HRESULT WINAPI MESA_IDirect3DDevice2Impl_GetCaps(
    LPDIRECT3DDEVICE2 iface, LPD3DDEVICEDESC lpdescsoft,
    LPD3DDEVICEDESC lpdeschard
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, lpdescsoft, lpdeschard);
  fill_opengl_caps(lpdescsoft, lpdeschard);
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
  pformat->u1.dwRGBBitCount = 32;
  pformat->u2.dwRBitMask =         0xFF000000;
  pformat->u3.dwGBitMask =         0x00FF0000;
  pformat->u4.dwBBitMask =        0x0000FF00;
  pformat->u5.dwRGBAlphaBitMask = 0x000000FF;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGB unpacked (24)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->u1.dwRGBBitCount = 24;
  pformat->u2.dwRBitMask =  0x00FF0000;
  pformat->u3.dwGBitMask =  0x0000FF00;
  pformat->u4.dwBBitMask = 0x000000FF;
  pformat->u5.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

#ifndef HAVE_BUGGY_MESAGL
  /* The packed texture format are buggy in Mesa. The bug was reported and corrected,
     so that future version will work great. */
  TRACE("Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_6_5 (16)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->u1.dwRGBBitCount = 16;
  pformat->u2.dwRBitMask =  0x0000F800;
  pformat->u3.dwGBitMask =  0x000007E0;
  pformat->u4.dwBBitMask = 0x0000001F;
  pformat->u5.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_5_5_5_1 (16)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->u1.dwRGBBitCount = 16;
  pformat->u2.dwRBitMask =         0x0000F800;
  pformat->u3.dwGBitMask =         0x000007C0;
  pformat->u4.dwBBitMask =        0x0000003E;
  pformat->u5.dwRGBAlphaBitMask = 0x00000001;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_4_4_4_4 (16)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->u1.dwRGBBitCount = 16;
  pformat->u2.dwRBitMask =         0x0000F000;
  pformat->u3.dwGBitMask =         0x00000F00;
  pformat->u4.dwBBitMask =        0x000000F0;
  pformat->u5.dwRGBAlphaBitMask = 0x0000000F;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating GL_RGB packed GL_UNSIGNED_BYTE_3_3_2 (8)\n");
  pformat->dwFlags = DDPF_RGB;
  pformat->u1.dwRGBBitCount = 8;
  pformat->u2.dwRBitMask =        0x000000E0;
  pformat->u3.dwGBitMask =        0x0000001C;
  pformat->u4.dwBBitMask =        0x00000003;
  pformat->u5.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;
#endif

  TRACE("Enumerating GL_ARGB (no direct OpenGL equivalent - conversion needed)\n");
  pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
  pformat->u1.dwRGBBitCount = 16;
  pformat->u2.dwRBitMask =         0x00007C00;
  pformat->u3.dwGBitMask =         0x000003E0;
  pformat->u4.dwBBitMask =         0x0000001F;
  pformat->u5.dwRGBAlphaBitMask =  0x00008000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("Enumerating Paletted (8)\n");
  pformat->dwFlags = DDPF_PALETTEINDEXED8;
  pformat->u1.dwRGBBitCount = 8;
  pformat->u2.dwRBitMask =  0x00000000;
  pformat->u3.dwGBitMask =  0x00000000;
  pformat->u4.dwBBitMask = 0x00000000;
  pformat->u5.dwRGBAlphaBitMask = 0x00000000;
  if (cb(&sdesc, context) == 0)
    return DD_OK;

  TRACE("End of enumeration\n");

  return DD_OK;
}

static HRESULT WINAPI MESA_IDirect3DDevice2Impl_EnumTextureFormats(
    LPDIRECT3DDEVICE2 iface, LPD3DENUMTEXTUREFORMATSCALLBACK cb, LPVOID context
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  FIXME("(%p)->(%p,%p): stub\n", This, cb, context);

  return enum_texture_format_OpenGL(cb, context);
}

static HRESULT WINAPI MESA_IDirect3DDevice2Impl_BeginScene(
    LPDIRECT3DDEVICE2 iface
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);

  FIXME("(%p)->(): stub\n", This);

  /* Here, we should get the DDraw surface and 'copy it' to the
     OpenGL surface.... */

  return DD_OK;
}

HRESULT WINAPI MESA_IDirect3DDevice2Impl_EndScene(LPDIRECT3DDEVICE2 iface) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);

  FIXME("(%p)->(): stub\n", This);

  /* No need to do anything here... */

  return DD_OK;
}

static HRESULT WINAPI MESA_IDirect3DDevice2Impl_SetRenderState(
    LPDIRECT3DDEVICE2 iface, D3DRENDERSTATETYPE dwRenderStateType,
    DWORD dwRenderState
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  D3DDPRIVATE(This);

  TRACE("(%p)->(%d,%ld)\n", This, dwRenderStateType, dwRenderState);

  /* Call the render state functions */
  set_render_state(dwRenderStateType, dwRenderState, &(odev->rs));

  return DD_OK;
}

static HRESULT WINAPI MESA_IDirect3DDevice2Impl_SetLightState(
    LPDIRECT3DDEVICE2 iface, D3DLIGHTSTATETYPE dwLightStateType,
    DWORD dwLightState
) {
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

#define UNSUP(x) case D3DLIGHTSTATE_##x: FIXME("unsupported D3DLIGHTSTATE_" #x "!\n");break;
  UNSUP(COLORMODEL);
  UNSUP(FOGMODE);
  UNSUP(FOGSTART);
  UNSUP(FOGEND);
  UNSUP(FOGDENSITY);
#undef UNSUP
  default:
    TRACE("Unexpected Light State Type\n");
    return DDERR_INVALIDPARAMS;
  }

  return DD_OK;
}

static HRESULT WINAPI MESA_IDirect3DDevice2Impl_SetTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts,
    LPD3DMATRIX lpmatrix
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  D3DDPRIVATE(This);

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
    glLoadMatrixf((float *) odev->world_mat);
  } break;

  case D3DTRANSFORMSTATE_VIEW: {
    conv_mat(lpmatrix, odev->view_mat);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float *) odev->proj_mat);
    glMultMatrixf((float *) odev->view_mat);
  } break;

  case D3DTRANSFORMSTATE_PROJECTION: {
    conv_mat(lpmatrix, odev->proj_mat);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float *) odev->proj_mat);
    glMultMatrixf((float *) odev->view_mat);
  } break;

  default:
    break;
  }
  LEAVE_GL();
  return DD_OK;
}

#define DRAW_PRIMITIVE(MAXVERT,INDEX)					\
  /* Puts GL in the correct lighting mode */				\
  if (odev->vt != d3dv) {						\
    if (odev->vt == D3DVT_TLVERTEX) {					\
      /* Need to put the correct transformation again */		\
      glMatrixMode(GL_MODELVIEW);					\
      glLoadMatrixf((float *) odev->world_mat);                      \
      glMatrixMode(GL_PROJECTION);					\
      glLoadMatrixf((float *) odev->proj_mat);	                \
      glMultMatrixf((float *) odev->view_mat);	                \
    }									\
									\
    switch (d3dv) {							\
    case D3DVT_VERTEX:							\
      TRACE("Standard Vertex\n");					\
      glEnable(GL_LIGHTING);						\
      break;								\
									\
    case D3DVT_LVERTEX:							\
      TRACE("Lighted Vertex\n");					\
      glDisable(GL_LIGHTING);						\
      break;								\
									\
    case D3DVT_TLVERTEX: {						\
      GLdouble height, width, minZ, maxZ;				\
									\
      TRACE("Transformed - Lighted Vertex\n");				\
      /* First, disable lighting */					\
      glDisable(GL_LIGHTING);						\
									\
      /* Then do not put any transformation matrixes */			\
      glMatrixMode(GL_MODELVIEW);					\
      glLoadIdentity();							\
      glMatrixMode(GL_PROJECTION);					\
      glLoadIdentity();							\
									\
      if (This->current_viewport == NULL) {				\
	ERR("No current viewport !\n");					\
	/* Using standard values */					\
	height = 640.0;							\
	width = 480.0;							\
	minZ = -10.0;							\
	maxZ = 10.0;							\
      } else {								\
	if (This->current_viewport->use_vp2) {				\
	  height = (GLdouble) This->current_viewport->viewport.vp2.dwHeight;\
	  width  = (GLdouble) This->current_viewport->viewport.vp2.dwWidth;\
	  minZ   = (GLdouble) This->current_viewport->viewport.vp2.dvMinZ;\
	  maxZ   = (GLdouble) This->current_viewport->viewport.vp2.dvMaxZ;\
	} else {							\
	  height = (GLdouble) This->current_viewport->viewport.vp1.dwHeight;\
	  width  = (GLdouble) This->current_viewport->viewport.vp1.dwWidth;\
	  minZ   = (GLdouble) This->current_viewport->viewport.vp1.dvMinZ;\
	  maxZ   = (GLdouble) This->current_viewport->viewport.vp1.dvMaxZ;\
	}								\
      }									\
									\
      glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);			\
    } break;								\
									\
    default:								\
      ERR("Unhandled vertex type\n");					\
      break;								\
    }									\
									\
    odev->vt = d3dv;							\
  }									\
									\
  switch (d3dp) {							\
  case D3DPT_POINTLIST:							\
    TRACE("Start POINTS\n");						\
    glBegin(GL_POINTS);							\
    break;								\
									\
  case D3DPT_LINELIST:							\
    TRACE("Start LINES\n");						\
    glBegin(GL_LINES);							\
    break;								\
									\
  case D3DPT_LINESTRIP:							\
    TRACE("Start LINE_STRIP\n");					\
    glBegin(GL_LINE_STRIP);						\
    break;								\
									\
  case D3DPT_TRIANGLELIST:						\
    TRACE("Start TRIANGLES\n");						\
    glBegin(GL_TRIANGLES);						\
    break;								\
									\
  case D3DPT_TRIANGLESTRIP:						\
    TRACE("Start TRIANGLE_STRIP\n");					\
    glBegin(GL_TRIANGLE_STRIP);						\
    break;								\
									\
  case D3DPT_TRIANGLEFAN:						\
    TRACE("Start TRIANGLE_FAN\n");					\
    glBegin(GL_TRIANGLE_FAN);						\
    break;								\
									\
  default:								\
    TRACE("Unhandled primitive\n");					\
    break;								\
  }									\
									\
  /* Draw the primitives */						\
  for (vx_index = 0; vx_index < MAXVERT; vx_index++) {			\
    switch (d3dv) {							\
    case D3DVT_VERTEX: {						\
      D3DVERTEX *vx = ((D3DVERTEX *) lpvertex) + INDEX;			\
									\
      glNormal3f(vx->u4.nx, vx->u5.ny, vx->u6.nz);			\
      glVertex3f(vx->u1.x, vx->u2.y, vx->u3.z);				\
      TRACE("   V: %f %f %f\n", vx->u1.x, vx->u2.y, vx->u3.z);		\
    } break;								\
									\
    case D3DVT_LVERTEX: {						\
      D3DLVERTEX *vx = ((D3DLVERTEX *) lpvertex) + INDEX;		\
      DWORD col = vx->u4.color;						\
									\
      glColor3f(((col >> 16) & 0xFF) / 255.0,				\
		((col >>  8) & 0xFF) / 255.0,				\
		((col >>  0) & 0xFF) / 255.0);				\
      glVertex3f(vx->u1.x, vx->u2.y, vx->u3.z);				\
      TRACE("  LV: %f %f %f (%02lx %02lx %02lx)\n",			\
	    vx->u1.x, vx->u2.y, vx->u3.z,				\
	    ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF));\
    } break;								\
									\
    case D3DVT_TLVERTEX: {						\
      D3DTLVERTEX *vx = ((D3DTLVERTEX *) lpvertex) + INDEX;		\
      DWORD col = vx->u5.color;						\
									\
      glColor3f(((col >> 16) & 0xFF) / 255.0,				\
		((col >>  8) & 0xFF) / 255.0,				\
		((col >>  0) & 0xFF) / 255.0);				\
      glTexCoord2f(vx->u7.tu, vx->u8.tv);				\
      if (vx->u4.rhw < 0.01)						\
	glVertex3f(vx->u1.sx,						\
		   vx->u2.sy,						\
		   vx->u3.sz);						\
      else								\
	glVertex4f(vx->u1.sx / vx->u4.rhw,				\
		   vx->u2.sy / vx->u4.rhw,				\
		   vx->u3.sz / vx->u4.rhw,				\
		   1.0 / vx->u4.rhw);					\
      TRACE(" TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",	\
	    vx->u1.sx, vx->u2.sy, vx->u3.sz,				\
	    ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),\
	    vx->u7.tu, vx->u8.tv, vx->u4.rhw);				\
    } break;								\
									\
    default:								\
      FIXME("Unhandled vertex type\n");					\
      break;								\
    }									\
  }									\
									\
  glEnd();								\
  TRACE("End\n");


static HRESULT WINAPI MESA_IDirect3DDevice2Impl_DrawPrimitive(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvertex, DWORD vertcount, DWORD dwFlags
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  D3DDPRIVATE(This);
  int vx_index;

  TRACE("(%p)->(%d,%d,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvertex, vertcount, dwFlags);

  ENTER_GL();
  DRAW_PRIMITIVE(vertcount, vx_index);
  LEAVE_GL();

  return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3DDevice2Impl_DrawIndexedPrimitive(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvertex, DWORD vertcount, LPWORD lpindexes, DWORD indexcount,
    DWORD dwFlags
) {
  ICOM_THIS(IDirect3DDevice2Impl,iface);
  D3DDPRIVATE(This);
  int vx_index;

  TRACE("(%p)->(%d,%d,%p,%ld,%p,%ld,%08lx): stub\n", This, d3dp, d3dv, lpvertex, vertcount, lpindexes, indexcount, dwFlags);

  ENTER_GL();
  DRAW_PRIMITIVE(indexcount, lpindexes[vx_index]);
  LEAVE_GL();

  return D3D_OK;
}

static HRESULT WINAPI MESA_IDirect3DDeviceImpl_CreateExecuteBuffer(
    LPDIRECT3DDEVICE iface, LPD3DEXECUTEBUFFERDESC lpDesc,
    LPDIRECT3DEXECUTEBUFFER *lplpDirect3DExecuteBuffer, IUnknown *pUnkOuter
) {
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    TRACE("(%p)->(%p,%p,%p)\n", This, lpDesc, lplpDirect3DExecuteBuffer, pUnkOuter);
    *lplpDirect3DExecuteBuffer = d3dexecutebuffer_create(This, lpDesc);
    return DD_OK;
}


/*******************************************************************************
 *				OpenGL-specific IDirect3DDevice2
 */

/*******************************************************************************
 *				OpenGL-specific VTable
 */

ICOM_VTABLE(IDirect3DDevice2) OpenGL_vtable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirect3DDevice2Impl_QueryInterface,
  IDirect3DDevice2Impl_AddRef,
  MESA_IDirect3DDevice2Impl_Release,
  /*** IDirect3DDevice2 methods ***/
  MESA_IDirect3DDevice2Impl_GetCaps,
  IDirect3DDevice2Impl_SwapTextureHandles,
  IDirect3DDevice2Impl_GetStats,
  IDirect3DDevice2Impl_AddViewport,
  IDirect3DDevice2Impl_DeleteViewport,
  IDirect3DDevice2Impl_NextViewport,
  MESA_IDirect3DDevice2Impl_EnumTextureFormats,
  MESA_IDirect3DDevice2Impl_BeginScene,
  MESA_IDirect3DDevice2Impl_EndScene,
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
  MESA_IDirect3DDevice2Impl_SetRenderState,
  IDirect3DDevice2Impl_GetLightState,
  MESA_IDirect3DDevice2Impl_SetLightState,
  MESA_IDirect3DDevice2Impl_SetTransform,
  IDirect3DDevice2Impl_GetTransform,
  IDirect3DDevice2Impl_MultiplyTransform,

  MESA_IDirect3DDevice2Impl_DrawPrimitive,
  MESA_IDirect3DDevice2Impl_DrawIndexedPrimitive,

  IDirect3DDevice2Impl_SetClipStatus,
  IDirect3DDevice2Impl_GetClipStatus,
};

/*******************************************************************************
 *				Direct3DDevice
 */
int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) {
  D3DDEVICEDESC	d1,d2;

  TRACE(" Enumerating OpenGL D3D device (IID %s).\n", debugstr_guid(&IID_D3DDEVICE_OpenGL));

  fill_opengl_caps(&d1, &d2);

  return cb((void*)&IID_D3DDEVICE_OpenGL,"WINE Direct3D using OpenGL","direct3d",&d1,&d2,context);
}

int is_OpenGL_dx3(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDeviceImpl** device)
{
  TRACE("rguid = %s, surface = %p, &device = %p\n",debugstr_guid(rguid),surface,device);
  if (!memcmp(&IID_D3DDEVICE_OpenGL,rguid,sizeof(IID_D3DDEVICE_OpenGL))) {
    mesa_d3dd_private *odev;
    HDC device_context;
    int attributeList[]={ GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };
    XVisualInfo *xvis;
    IDirectDrawSurfaceImpl* surf;

    *device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirect3DDeviceImpl));
    (*device)->private = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(mesa_d3dd_private));
    odev = (mesa_d3dd_private*)(*device)->private;
    (*device)->ref = 1;
    ICOM_VTBL(*device) = &OpenGL_vtable_dx3;
    (*device)->d3d = NULL;
    (*device)->surface = surface;

    (*device)->viewport_list = NULL;
    (*device)->current_viewport = NULL;
    (*device)->current_texture = NULL;

    (*device)->set_context = (void*)set_context;

    TRACE("OpenGL device created \n");

    /* Create the OpenGL context */
    /* First get the correct visual */
    /* if (surface->s.backbuffer == NULL)
       attributeList[3] = None; */
    device_context = GetDC((*device)->surface->ddraw_owner->window);
    odev->gdi_display = get_display(device_context);
    odev->drawable = get_drawable(device_context);
    ReleaseDC((*device)->surface->ddraw_owner->window,device_context);
    ENTER_GL();
    xvis = glXChooseVisual(odev->gdi_display,
			   DefaultScreen(odev->gdi_display),
			   attributeList);
    if (xvis == NULL)
      ERR("No visual found !\n");
    else
      TRACE("Visual found\n");
    
    /* Create the context */
    odev->ctx = glXCreateContext(odev->gdi_display,
				 xvis,
				 NULL,
				 GL_TRUE);
    TRACE("Context created\n");

    /* Look for the front buffer and override its surface's Flip method (if in double buffering) */ 
    for (surf = surface; surf != NULL; surf = surf->surface_owner)
        if ((surf->surface_desc.ddsCaps.dwCaps&(DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	    == (DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	{
            surface->surface_owner->aux_ctx  = (LPVOID)odev->gdi_display;
            surface->surface_owner->aux_data = (LPVOID)odev->drawable;
            surface->surface_owner->aux_flip = opengl_flip;
            break;
        }
	
    odev->rs.src = GL_ONE;
    odev->rs.dst = GL_ZERO;
    odev->rs.mag = GL_NEAREST;
    odev->rs.min = GL_NEAREST;

    odev->world_mat = (LPD3DMATRIX) &id_mat;
    odev->view_mat  = (LPD3DMATRIX) &id_mat;
    odev->proj_mat  = (LPD3DMATRIX) &id_mat;

    /* Initialisation */
    LEAVE_GL();
    (*device)->set_context(*device);
    ENTER_GL();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
    LEAVE_GL();
    fill_device_capabilities((IDirectDrawImpl *) surface->ddraw_owner);

    return 1;
  }

  /* This is not the OpenGL UID */
  return DD_OK;
}

static ULONG WINAPI MESA_IDirect3DDeviceImpl_Release(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

  if (!--(This->ref)) {
    D3DDPRIVATE(This);
    ENTER_GL();
    glXDestroyContext(odev->gdi_display, odev->ctx);
    LEAVE_GL();
    HeapFree(GetProcessHeap(),0,This->private);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  return This->ref;
}

static HRESULT WINAPI MESA_IDirect3DDeviceImpl_EnumTextureFormats(
    LPDIRECT3DDEVICE iface,LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc,
    LPVOID lpArg)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpd3dEnumTextureProc, lpArg);

  return enum_texture_format_OpenGL(lpd3dEnumTextureProc, lpArg);
}


static HRESULT WINAPI MESA_IDirect3DDeviceImpl_BeginScene(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);
  /* OpenGL_IDirect3DDevice *odev = (OpenGL_IDirect3DDevice *) This; */

  FIXME("(%p)->(): stub\n", This);

  /* We get the pointer to the surface (should be done on flip) */
  /* odev->zb->pbuf = This->surface->s.surface_desc.u2.lpSurface; */

  return DD_OK;
}


/* This is for the moment copy-pasted from IDirect3DDevice2...
   Will make a common function ... */
static HRESULT WINAPI MESA_IDirect3DDeviceImpl_EndScene(LPDIRECT3DDEVICE iface)
{
  ICOM_THIS(IDirect3DDeviceImpl,iface);

  FIXME("(%p)->(): stub\n", This);

  /* No need to do anything here... */

  return DD_OK;
}

/*******************************************************************************
 *				Direct3DDevice VTable
 */
ICOM_VTABLE(IDirect3DDevice) OpenGL_vtable_dx3 =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IDirect3DDeviceImpl_QueryInterface,
  IDirect3DDeviceImpl_AddRef,
  MESA_IDirect3DDeviceImpl_Release,
  IDirect3DDeviceImpl_Initialize,
  IDirect3DDeviceImpl_GetCaps,
  IDirect3DDeviceImpl_SwapTextureHandles,
  MESA_IDirect3DDeviceImpl_CreateExecuteBuffer,
  IDirect3DDeviceImpl_GetStats,
  IDirect3DDeviceImpl_Execute,
  IDirect3DDeviceImpl_AddViewport,
  IDirect3DDeviceImpl_DeleteViewport,
  IDirect3DDeviceImpl_NextViewport,
  IDirect3DDeviceImpl_Pick,
  IDirect3DDeviceImpl_GetPickRecords,
  MESA_IDirect3DDeviceImpl_EnumTextureFormats,
  IDirect3DDeviceImpl_CreateMatrix,
  IDirect3DDeviceImpl_SetMatrix,
  IDirect3DDeviceImpl_GetMatrix,
  IDirect3DDeviceImpl_DeleteMatrix,
  MESA_IDirect3DDeviceImpl_BeginScene,
  MESA_IDirect3DDeviceImpl_EndScene,
  IDirect3DDeviceImpl_GetDirect3D,
};
