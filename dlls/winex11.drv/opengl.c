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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>

#include "x11drv.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(opengl);

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
  TRACE("  - size / version : %d / %d\n", ppfd->nSize, ppfd->nVersion);
  TRACE("  - dwFlags : ");
#define TEST_AND_DUMP(t,tv) if ((t) & (tv)) TRACE(#tv " ")
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
  TRACE("\n");

  TRACE("  - iPixelType : ");
  switch (ppfd->iPixelType) {
  case PFD_TYPE_RGBA: TRACE("PFD_TYPE_RGBA"); break;
  case PFD_TYPE_COLORINDEX: TRACE("PFD_TYPE_COLORINDEX"); break;
  }
  TRACE("\n");

  TRACE("  - Color   : %d\n", ppfd->cColorBits);
  TRACE("  - Red     : %d\n", ppfd->cRedBits);
  TRACE("  - Green   : %d\n", ppfd->cGreenBits);
  TRACE("  - Blue    : %d\n", ppfd->cBlueBits);
  TRACE("  - Alpha   : %d\n", ppfd->cAlphaBits);
  TRACE("  - Accum   : %d\n", ppfd->cAccumBits);
  TRACE("  - Depth   : %d\n", ppfd->cDepthBits);
  TRACE("  - Stencil : %d\n", ppfd->cStencilBits);
  TRACE("  - Aux     : %d\n", ppfd->cAuxBuffers);

  TRACE("  - iLayerType : ");
  switch (ppfd->iLayerType) {
  case PFD_MAIN_PLANE: TRACE("PFD_MAIN_PLANE"); break;
  case PFD_OVERLAY_PLANE: TRACE("PFD_OVERLAY_PLANE"); break;
  case (BYTE)PFD_UNDERLAY_PLANE: TRACE("PFD_UNDERLAY_PLANE"); break;
  }
  TRACE("\n");
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
MAKE_FUNCPTR(glXCreateGLXPixmap)
MAKE_FUNCPTR(glXDestroyGLXPixmap)
/* MAKE_FUNCPTR(glXQueryDrawable) */
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

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(RTLD_DEFAULT, #f, NULL, 0)) == NULL) goto sym_not_found;
LOAD_FUNCPTR(glGetError)
LOAD_FUNCPTR(glXChooseVisual)
LOAD_FUNCPTR(glXGetConfig)
LOAD_FUNCPTR(glXSwapBuffers)
LOAD_FUNCPTR(glXQueryExtension)

LOAD_FUNCPTR(glXGetFBConfigs)
LOAD_FUNCPTR(glXChooseFBConfig)
LOAD_FUNCPTR(glXGetFBConfigAttrib)
LOAD_FUNCPTR(glXCreateGLXPixmap)
LOAD_FUNCPTR(glXDestroyGLXPixmap)
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
  int att_pos_fac = 0;
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

  /* Now, build the request to GLX */
  
  if (ppfd->iPixelType == PFD_TYPE_COLORINDEX) {
    ADD2(GLX_BUFFER_SIZE, ppfd->cColorBits);
  }  
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    ADD2(GLX_RENDER_TYPE, GLX_RGBA_BIT);
    if (ppfd->dwFlags & PFD_DEPTH_DONTCARE) {
      ADD2(GLX_DEPTH_SIZE, GLX_DONT_CARE);
    } else {
      if (32 == ppfd->cDepthBits) {
	/**
	 * for 32 bpp depth buffers force to use 24.
	 * needed as some drivers don't support 32bpp
	 */
	TEST_AND_ADD2(ppfd->cDepthBits, GLX_DEPTH_SIZE, 24);
      } else {
	TEST_AND_ADD2(ppfd->cDepthBits, GLX_DEPTH_SIZE, ppfd->cDepthBits);
      }
    }
    if (32 == ppfd->cColorBits) {
      ADD2(GLX_RED_SIZE,   8);
      ADD2(GLX_GREEN_SIZE, 8);
      ADD2(GLX_BLUE_SIZE,  8);
      ADD2(GLX_ALPHA_SIZE, 8);
    } else {
      ADD2(GLX_BUFFER_SIZE, ppfd->cColorBits);
      /* Some broken apps try to ask for more than 8 bits of alpha */
      TEST_AND_ADD2(ppfd->cAlphaBits, GLX_ALPHA_SIZE, min(ppfd->cAlphaBits,8));
    }
  }
  TEST_AND_ADD2(ppfd->cStencilBits, GLX_STENCIL_SIZE, ppfd->cStencilBits);
  TEST_AND_ADD2(ppfd->cAuxBuffers,  GLX_AUX_BUFFERS,  ppfd->cAuxBuffers);
   
  /* These flags are not supported yet...
  ADD2(GLX_ACCUM_SIZE, ppfd->cAccumBits);
  */

  /** facultative flags now */
  att_pos_fac = att_pos;
  if (ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) {
    ADD2(GLX_DOUBLEBUFFER, GLX_DONT_CARE);
  } else {
    ADD2(GLX_DOUBLEBUFFER, (ppfd->dwFlags & PFD_DOUBLEBUFFER) ? TRUE : FALSE);
  }
  if (ppfd->dwFlags & PFD_STEREO_DONTCARE) {
    ADD2(GLX_STEREO, GLX_DONT_CARE);
  } else {
    ADD2(GLX_STEREO, (ppfd->dwFlags & PFD_STEREO) ? TRUE : FALSE);
  }
  
  /** Attributes List End */
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
    /** 
     * if we have facultative flags and we failed, try without 
     *  as MSDN said
     *
     * see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/opengl/ntopnglr_2qb8.asp
     */
    if ((NULL == cfgs || 0 == nCfgs) && att_pos > att_pos_fac) {
      att_list[att_pos_fac] = None;
      cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), att_list, &nCfgs); 
    }

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

  if (nCfgs < iPixelFormat || 1 > iPixelFormat) {
    WARN("unexpected iPixelFormat(%d): not >=1 and <=nFormats(%d), returning NULL\n", iPixelFormat, nCfgs);
    return 0;
  }

  ret = nCfgs;
  cur = cfgs[iPixelFormat - 1];

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;

  /* These flags are always the same... */
  ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  /* Now the flags extracted from the Visual */

  wine_tsx11_lock();

  pglXGetFBConfigAttrib(gdi_display, cur, GLX_CONFIG_CAVEAT, &value);
  if(value == GLX_SLOW_CONFIG)
      ppfd->dwFlags |= PFD_GENERIC_ACCELERATED;

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
  
  if (TRACE_ON(opengl)) {
    int nCfgs_fmt = 0;
    GLXFBConfig* cfgs_fmt = NULL;
    GLXFBConfig cur_cfg;
    int value;
    int gl_test = 0;

    /*
     * How to test if hdc current drawable is compatible (visual/FBConfig) ?
     *
     * in case of root window created HDCs we crash here :(
     *
    Drawable drawable =  get_drawable( physDev->hdc );
    TRACE(" drawable (%p,%p) have :\n", drawable, root_window);
    pglXQueryDrawable(gdi_display, drawable, GLX_FBCONFIG_ID, (unsigned int*) &value);
    TRACE(" - FBCONFIG_ID as 0x%x\n", tmp);
    pglXQueryDrawable(gdi_display, drawable, GLX_VISUAL_ID, (unsigned int*) &value);
    TRACE(" - VISUAL_ID as 0x%x\n", tmp);
    pglXQueryDrawable(gdi_display, drawable, GLX_WIDTH, (unsigned int*) &value);
    TRACE(" - WIDTH as %d\n", tmp);
    pglXQueryDrawable(gdi_display, drawable, GLX_HEIGHT, (unsigned int*) &value);
    TRACE(" - HEIGHT as %d\n", tmp);
    */
    cfgs_fmt = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs_fmt);
    cur_cfg = cfgs_fmt[iPixelFormat - 1];
    gl_test = pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_FBCONFIG_ID, &value);
    if (gl_test) {
      ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
    } else {
      TRACE(" FBConfig have :\n");
      TRACE(" - FBCONFIG_ID   0x%x\n", value);
      pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_VISUAL_ID, &value);
      TRACE(" - VISUAL_ID     0x%x\n", value);
      pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_DRAWABLE_TYPE, &value);
      TRACE(" - DRAWABLE_TYPE 0x%x\n", value);
    }
    XFree(cfgs_fmt);
  }
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
  
  TRACE_(opengl)("(%p)\n", physDev);

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
    int dblBuf[] = {GLX_RGBA,GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DOUBLEBUFFER, None};
    if (!has_opengl()) return NULL;

    wine_tsx11_lock();
    visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf);
    wine_tsx11_unlock();
    if (visual == NULL) {
        /* fallback to 16 bits depth, no alpha */
        int dblBuf2[] = {GLX_RGBA,GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None};
        WARN("Failed to get a visual with at least 24 bits depth\n");

        wine_tsx11_lock();
        visual = pglXChooseVisual(display, DefaultScreen(display), dblBuf2);
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
                return visual;
            }
        }
    }
    TRACE("Visual ID %lx Chosen\n",visual->visualid);
    return visual;
}

XID create_glxpixmap(X11DRV_PDEVICE *physDev)
{
    GLXPixmap ret;
    XVisualInfo *vis;
    XVisualInfo template;
    int num;
    GLXFBConfig *cfgs;

    wine_tsx11_lock();
    cfgs = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &num);
    pglXGetFBConfigAttrib(gdi_display, cfgs[physDev->current_pf - 1], GLX_VISUAL_ID, (int *)&template.visualid);

    vis = XGetVisualInfo(gdi_display, VisualIDMask, &template, &num);

    ret = pglXCreateGLXPixmap(gdi_display, vis, physDev->bitmap->pixmap);
    XFree(vis);
    XFree(cfgs);
    wine_tsx11_unlock(); 
    TRACE("return %lx\n", ret);
    return ret;
}

BOOL destroy_glxpixmap(XID glxpixmap)
{
    wine_tsx11_lock(); 
    pglXDestroyGLXPixmap(gdi_display, glxpixmap);
    wine_tsx11_unlock(); 
    return TRUE;
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
  ERR_(opengl)("No OpenGL support compiled in.\n");

  return FALSE;
}

XVisualInfo *X11DRV_setup_opengl_visual( Display *display )
{
  return NULL;
}

XID create_glxpixmap(X11DRV_PDEVICE *physDev)
{
    return 0;
}

BOOL destroy_glxpixmap(XID glxpixmap)
{
    return FALSE;
}

#endif /* defined(HAVE_OPENGL) */
