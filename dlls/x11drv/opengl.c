/*
 * X11DRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
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
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>

#include "x11drv.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

#if defined(HAVE_GL_GL_H) && defined(HAVE_GL_GLX_H)

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#endif
#ifdef HAVE_GL_GLX_H
# include <GL/glx.h>
#endif
#ifdef HAVE_GL_GLEXT_H
# include <GL/glext.h>
#endif

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI


static void dump_PIXELFORMATDESCRIPTOR(const PIXELFORMATDESCRIPTOR *ppfd) {
  DPRINTF("  - size / version : %d / %d\n", ppfd->nSize, ppfd->nVersion);
  DPRINTF("  - dwFlags : ");
#define TEST_AND_DUMP(t,tv) if ((t) & (tv)) DPRINTF(#tv " ")
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DEPTH_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DOUBLEBUFFER);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DOUBLEBUFFER_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DRAW_TO_WINDOW);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DRAW_TO_BITMAP);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_GENERIC_ACCELERATED);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_GENERIC_FORMAT);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_NEED_PALETTE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_NEED_SYSTEM_PALETTE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_STEREO);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_STEREO_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_GDI);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_OPENGL);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_COPY);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_EXCHANGE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_LAYER_BUFFERS);
#undef TEST_AND_DUMP
  DPRINTF("\n");

  DPRINTF("  - iPixelType : ");
  switch (ppfd->iPixelType) {
  case PFD_TYPE_RGBA: DPRINTF("PFD_TYPE_RGBA"); break;
  case PFD_TYPE_COLORINDEX: DPRINTF("PFD_TYPE_COLORINDEX"); break;
  }
  DPRINTF("\n");

  DPRINTF("  - Color   : %d\n", ppfd->cColorBits);
  DPRINTF("  - Red     : %d\n", ppfd->cRedBits);
  DPRINTF("  - Green   : %d\n", ppfd->cGreenBits);
  DPRINTF("  - Blue    : %d\n", ppfd->cBlueBits);
  DPRINTF("  - Alpha   : %d\n", ppfd->cAlphaBits);
  DPRINTF("  - Accum   : %d\n", ppfd->cAccumBits);
  DPRINTF("  - Depth   : %d\n", ppfd->cDepthBits);
  DPRINTF("  - Stencil : %d\n", ppfd->cStencilBits);
  DPRINTF("  - Aux     : %d\n", ppfd->cAuxBuffers);

  DPRINTF("  - iLayerType : ");
  switch (ppfd->iLayerType) {
  case PFD_MAIN_PLANE: DPRINTF("PFD_MAIN_PLANE"); break;
  case PFD_OVERLAY_PLANE: DPRINTF("PFD_OVERLAY_PLANE"); break;
  case PFD_UNDERLAY_PLANE: DPRINTF("PFD_UNDERLAY_PLANE"); break;
  }
  DPRINTF("\n");
}

/* No need to load any other libraries as according to the ABI, libGL should be self-sufficient and
   include all dependencies
*/
#ifndef SONAME_LIBGL
#define SONAME_LIBGL "libGL.so"
#endif

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(glGetError)
MAKE_FUNCPTR(glXChooseVisual)
MAKE_FUNCPTR(glXGetConfig)
MAKE_FUNCPTR(glXSwapBuffers)
MAKE_FUNCPTR(glXQueryExtension)

MAKE_FUNCPTR(glXGetFBConfigs)
MAKE_FUNCPTR(glXChooseFBConfig)
MAKE_FUNCPTR(glXGetFBConfigAttrib)
#undef MAKE_FUNCPTR

static BOOL has_opengl(void)
{
    static int init_done;
    static void *opengl_handle;

    int error_base, event_base;

    if (init_done) return (opengl_handle != NULL);
    init_done = 1;

    opengl_handle = wine_dlopen(SONAME_LIBGL, RTLD_NOW|RTLD_GLOBAL, NULL, 0);
    if (opengl_handle == NULL) return FALSE;

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(opengl_handle, #f, NULL, 0)) == NULL) goto sym_not_found;
LOAD_FUNCPTR(glGetError)
LOAD_FUNCPTR(glXChooseVisual)
LOAD_FUNCPTR(glXGetConfig)
LOAD_FUNCPTR(glXSwapBuffers)
LOAD_FUNCPTR(glXQueryExtension)

LOAD_FUNCPTR(glXGetFBConfigs)
LOAD_FUNCPTR(glXChooseFBConfig)
LOAD_FUNCPTR(glXGetFBConfigAttrib)
#undef LOAD_FUNCPTR

    wine_tsx11_lock();
    if (pglXQueryExtension(gdi_display, &event_base, &error_base) == True) {
	TRACE("GLX is up and running error_base = %d\n", error_base);
    } else {
        wine_dlclose(opengl_handle, NULL, 0);
	opengl_handle = NULL;
    }
    wine_tsx11_unlock();
    return (opengl_handle != NULL);

sym_not_found:
    wine_dlclose(opengl_handle, NULL, 0);
    opengl_handle = NULL;
    return FALSE;
}

#define TEST_AND_ADD1(t,a) if (t) att_list[att_pos++] = (a)
#define TEST_AND_ADD2(t,a,b) if (t) { att_list[att_pos++] = (a); att_list[att_pos++] = (b); }
#define NULL_TEST_AND_ADD2(tv,a,b) att_list[att_pos++] = (a); att_list[att_pos++] = ((tv) == 0 ? 0 : (b))
#define ADD2(a,b) att_list[att_pos++] = (a); att_list[att_pos++] = (b)

/**
 * X11DRV_ChoosePixelFormat
 *
 * Equivalent of glXChooseVisual
 */
int X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev, 
			     const PIXELFORMATDESCRIPTOR *ppfd) {
  int att_list[64];
  int att_pos = 0;
  GLXFBConfig* cfgs = NULL;
  int ret = 0;

  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }
  
  if (TRACE_ON(opengl)) {
    TRACE("(%p,%p)\n", physDev, ppfd);

    dump_PIXELFORMATDESCRIPTOR((const PIXELFORMATDESCRIPTOR *) ppfd);
  }

  if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) {
    ERR("Flag not supported !\n");
    /* Should SetError here... */
    return 0;
  }

  /* Now, build the request to GLX */
  TEST_AND_ADD2(ppfd->dwFlags & PFD_DOUBLEBUFFER, GLX_DOUBLEBUFFER, TRUE);
  TEST_AND_ADD1(ppfd->dwFlags & PFD_STEREO, GLX_STEREO);
  
  if (ppfd->iPixelType == PFD_TYPE_COLORINDEX) {
    ADD2(GLX_BUFFER_SIZE, ppfd->cColorBits);
  }  
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    ADD2(GLX_RENDER_TYPE, GLX_RGBA_BIT);
    ADD2(GLX_BUFFER_SIZE, ppfd->cColorBits);
    if (32 == ppfd->cDepthBits) {
      /**
       * for 32 bpp depth buffers force to use 24.
       * needed as some drivers don't support 32bpp
       */
      TEST_AND_ADD2(ppfd->cDepthBits, GLX_DEPTH_SIZE, 24);
    } else {
      TEST_AND_ADD2(ppfd->cDepthBits, GLX_DEPTH_SIZE, ppfd->cDepthBits);
    }
    TEST_AND_ADD2(ppfd->cAlphaBits, GLX_ALPHA_SIZE, ppfd->cAlphaBits);
  }
  TEST_AND_ADD2(ppfd->cStencilBits, GLX_STENCIL_SIZE, ppfd->cStencilBits);
  TEST_AND_ADD2(ppfd->cAuxBuffers,  GLX_AUX_BUFFERS,  ppfd->cAuxBuffers);
   
  /* These flags are not supported yet...
  ADD2(GLX_ACCUM_SIZE, ppfd->cAccumBits);
  */
  att_list[att_pos] = None;

  wine_tsx11_lock(); 
  {
    int nCfgs = 0;
    int gl_test = 0;
    int fmt_id;

    GLXFBConfig* cfgs_fmt = NULL;
    int nCfgs_fmt = 0;
    int tmp_fmt_id;
    UINT it_fmt;

    cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), att_list, &nCfgs); 
    if (NULL == cfgs || 0 == nCfgs) {
      ERR("glXChooseFBConfig returns NULL (glError: %d)\n", pglGetError());
      ret = 0;
      goto choose_exit;
    }
    
    gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs[0], GLX_FBCONFIG_ID, &fmt_id);
    if (gl_test) {
      ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
      ret = 0;
      goto choose_exit;
    }
    
    cfgs_fmt = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs_fmt);
    if (NULL == cfgs_fmt) {
      ERR("Failed to get All FB Configs\n");
      ret = 0;
      goto choose_exit;
    }

    for (it_fmt = 0; it_fmt < nCfgs_fmt; ++it_fmt) {
      gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs_fmt[it_fmt], GLX_FBCONFIG_ID, &tmp_fmt_id);
      if (gl_test) {
	ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
	XFree(cfgs_fmt);
	ret = 0;
	goto choose_exit;
      }
      if (fmt_id == tmp_fmt_id) {
	ret = it_fmt + 1;
	break ;
      }
    }
    if (it_fmt == nCfgs_fmt) {
      ERR("Failed to get valid fmt for FBCONFIG_ID(%d)\n", fmt_id);
      ret = 0;
    }
    XFree(cfgs_fmt);
  }

choose_exit:
  if (NULL != cfgs) XFree(cfgs);
  wine_tsx11_unlock();
  return ret;
}

/**
 * X11DRV_DescribePixelFormat
 *
 * Get the pixel-format descriptor associated to the given id
 */
int X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  /*XVisualInfo *vis;*/
  int value;
  int rb,gb,bb,ab;

  GLXFBConfig* cfgs = NULL;
  GLXFBConfig cur;
  int nCfgs = 0;
  int ret = 0;

  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }
  
  TRACE("(%p,%d,%d,%p)\n", physDev, iPixelFormat, nBytes, ppfd);

  wine_tsx11_lock();
  cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs);
  wine_tsx11_unlock();

  if (NULL == cfgs || 0 == nCfgs) {
    ERR("unexpected iPixelFormat(%d), returns NULL\n", iPixelFormat);
    return 0; /* unespected error */
  }

  if (ppfd == NULL) {
    /* The application is only querying the number of visuals */
    wine_tsx11_lock();
    if (NULL != cfgs) XFree(cfgs);
    wine_tsx11_unlock();
    return nCfgs;
  }

  if (nBytes < sizeof(PIXELFORMATDESCRIPTOR)) {
    ERR("Wrong structure size !\n");
    /* Should set error */
    return 0;
  }

  if (nCfgs < iPixelFormat) {
    ERR("unexpected iPixelFormat(%d) > nFormats(%d), returns NULL\n", iPixelFormat, nCfgs);
    return 0; /* unespected error */
  }

  ret = nCfgs;
  cur = cfgs[iPixelFormat - 1];

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;

  /* These flags are always the same... */
  ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED;
  /* Now the flags extraced from the Visual */

  wine_tsx11_lock();

  pglXGetFBConfigAttrib(gdi_display, cur, GLX_DOUBLEBUFFER, &value); if (value) ppfd->dwFlags |= PFD_DOUBLEBUFFER;
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_STEREO, &value); if (value) ppfd->dwFlags |= PFD_STEREO;

  /* Pixel type */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_RENDER_TYPE, &value);
  if (value & GLX_RGBA_BIT)
    ppfd->iPixelType = PFD_TYPE_RGBA;
  else
    ppfd->iPixelType = PFD_TYPE_COLORINDEX;

  /* Color bits */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_BUFFER_SIZE, &value);
  ppfd->cColorBits = value;

  /* Red, green, blue and alpha bits / shifts */
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_RED_SIZE, &rb);
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_GREEN_SIZE, &gb);
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_BLUE_SIZE, &bb);
    pglXGetFBConfigAttrib(gdi_display, cur, GLX_ALPHA_SIZE, &ab);

    ppfd->cRedBits = rb;
    ppfd->cRedShift = gb + bb + ab;
    ppfd->cBlueBits = bb;
    ppfd->cBlueShift = ab;
    ppfd->cGreenBits = gb;
    ppfd->cGreenShift = bb + ab;
    ppfd->cAlphaBits = ab;
    ppfd->cAlphaShift = 0;
  } else {
    ppfd->cRedBits = 0;
    ppfd->cRedShift = 0;
    ppfd->cBlueBits = 0;
    ppfd->cBlueShift = 0;
    ppfd->cGreenBits = 0;
    ppfd->cGreenShift = 0;
    ppfd->cAlphaBits = 0;
    ppfd->cAlphaShift = 0;
  }
  /* Accums : to do ... */

  /* Depth bits */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_DEPTH_SIZE, &value);
  ppfd->cDepthBits = value;

  /* stencil bits */
  pglXGetFBConfigAttrib(gdi_display, cur, GLX_STENCIL_SIZE, &value);
  ppfd->cStencilBits = value;

  wine_tsx11_unlock();

  /* Aux : to do ... */

  ppfd->iLayerType = PFD_MAIN_PLANE;

  if (TRACE_ON(opengl)) {
    dump_PIXELFORMATDESCRIPTOR(ppfd);
  }

  wine_tsx11_lock();
  if (NULL != cfgs) XFree(cfgs);
  wine_tsx11_unlock();

  return ret;
}

/**
 * X11DRV_GetPixelFormat
 *
 * Get the pixel-format id used by this DC
 */
int X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev) {
  TRACE("(%p): returns %d\n", physDev, physDev->current_pf);

  return physDev->current_pf;
}

/**
 * X11DRV_SetPixelFormat
 *
 * Set the pixel-format id used by this DC
 */
BOOL X11DRV_SetPixelFormat(X11DRV_PDEVICE *physDev,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
  TRACE("(%p,%d,%p)\n", physDev, iPixelFormat, ppfd);

  physDev->current_pf = iPixelFormat;

  return TRUE;
}

/**
 * X11DRV_SwapBuffers
 *
 * Swap the buffers of this DC
 */
BOOL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev) {
  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }
  
  TRACE("(%p)\n", physDev);

  wine_tsx11_lock();
  pglXSwapBuffers(gdi_display, physDev->drawable);
  wine_tsx11_unlock();

  return TRUE;
}

/***********************************************************************
 *		X11DRV_setup_opengl_visual
 *
 * Setup the default visual used for OpenGL and Direct3D, and the desktop
 * window (if it exists).  If OpenGL isn't available, the visual is simply
 * set to the default visual for the display
 */
XVisualInfo *X11DRV_setup_opengl_visual( Display *display )
{
    XVisualInfo *visual = NULL;
    /* In order to support OpenGL or D3D, we require a double-buffered visual and stencil buffer support, */
    int dblBuf[] = {GLX_RGBA,GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None};
    if (!has_opengl()) return NULL;

    wine_tsx11_lock();
    visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf);
    wine_tsx11_unlock();
    if (visual == NULL) {
        /* fallback to no stencil */
        int dblBuf2[] = {GLX_RGBA,GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};
        WARN("Failed to get a visual with at least 8 bits of stencil\n");

        wine_tsx11_lock();
        visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf2);
        wine_tsx11_unlock();
        if (visual == NULL) {
            /* This should only happen if we cannot find a match with a depth size 16 */
            FIXME("Failed to find a suitable visual\n");
        }
    }
    TRACE("Visual ID %lx Chosen\n",visual->visualid);
    return visual;
}

#else  /* no OpenGL includes */

void X11DRV_OpenGL_Init(Display *display)
{
}

/***********************************************************************
 *		ChoosePixelFormat (X11DRV.@)
 */
int X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev,
			     const PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

/***********************************************************************
 *		DescribePixelFormat (X11DRV.@)
 */
int X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

/***********************************************************************
 *		GetPixelFormat (X11DRV.@)
 */
int X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev) {
  ERR("No OpenGL support compiled in.\n");

  return 0;
}

/***********************************************************************
 *		SetPixelFormat (X11DRV.@)
 */
BOOL X11DRV_SetPixelFormat(X11DRV_PDEVICE *physDev,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
  ERR("No OpenGL support compiled in.\n");

  return FALSE;
}

/***********************************************************************
 *		SwapBuffers (X11DRV.@)
 */
BOOL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev) {
  ERR("No OpenGL support compiled in.\n");

  return FALSE;
}

XVisualInfo *X11DRV_setup_opengl_visual( Display *display )
{
  return NULL;
}

#endif /* defined(HAVE_OPENGL) */
