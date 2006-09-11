/*
 * X11DRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2006 Roderick Colenbrander
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
#include "winternl.h"
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

WINE_DECLARE_DEBUG_CHANNEL(fps);

typedef struct wine_glcontext {
  HDC hdc;
  Display *display;
  XVisualInfo *vis;
  GLXFBConfig fb_conf;
  GLXContext ctx;
  BOOL do_escape;
  struct wine_glcontext *next;
  struct wine_glcontext *prev;
} Wine_GLContext;
static Wine_GLContext *context_list;

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
MAKE_FUNCPTR(glXQueryServerString)

MAKE_FUNCPTR(glXGetFBConfigs)
MAKE_FUNCPTR(glXChooseFBConfig)
MAKE_FUNCPTR(glXGetFBConfigAttrib)
MAKE_FUNCPTR(glXGetVisualFromFBConfig)
MAKE_FUNCPTR(glXCreateGLXPixmap)
MAKE_FUNCPTR(glXDestroyGLXPixmap)
MAKE_FUNCPTR(glXCreateContext)
MAKE_FUNCPTR(glXDestroyContext)
MAKE_FUNCPTR(glXGetCurrentContext)
MAKE_FUNCPTR(glXGetCurrentReadDrawable)
MAKE_FUNCPTR(glXMakeCurrent)
MAKE_FUNCPTR(glXMakeContextCurrent)
MAKE_FUNCPTR(glXQueryDrawable)

MAKE_FUNCPTR(glDrawBuffer)
#undef MAKE_FUNCPTR

static BOOL has_opengl(void)
{
    static int init_done;
    static void *opengl_handle;
    const char *server_glx_version;

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
LOAD_FUNCPTR(glXQueryServerString)

LOAD_FUNCPTR(glXGetFBConfigs)
LOAD_FUNCPTR(glXChooseFBConfig)
LOAD_FUNCPTR(glXGetFBConfigAttrib)
LOAD_FUNCPTR(glXGetVisualFromFBConfig)
LOAD_FUNCPTR(glXCreateGLXPixmap)
LOAD_FUNCPTR(glXDestroyGLXPixmap)
LOAD_FUNCPTR(glXCreateContext)
LOAD_FUNCPTR(glXDestroyContext)
LOAD_FUNCPTR(glXGetCurrentContext)
LOAD_FUNCPTR(glXGetCurrentReadDrawable)
LOAD_FUNCPTR(glXMakeCurrent)
LOAD_FUNCPTR(glXMakeContextCurrent)

/* Standard OpenGL call, need in wglMakeCurrent */
LOAD_FUNCPTR(glDrawBuffer)
#undef LOAD_FUNCPTR

    wine_tsx11_lock();
    if (pglXQueryExtension(gdi_display, &error_base, &event_base) == True) {
        TRACE("GLX is up and running error_base = %d\n", error_base);
    } else {
        wine_dlclose(opengl_handle, NULL, 0);
        opengl_handle = NULL;
    }

    server_glx_version = pglXQueryServerString(gdi_display, DefaultScreen(gdi_display), GLX_VERSION);
    TRACE("Server GLX version: %s\n", server_glx_version);
    /* The mesa libGL client library seems to forward glXQueryDrawable to the Xserver, so only
    * enable this function when the Xserver understand GLX 1.3 or newer
    */ 
    if (!strcmp("1.2", server_glx_version))
        pglXQueryDrawable = NULL;
    else
        pglXQueryDrawable = wine_dlsym(RTLD_DEFAULT, "glXQueryDrawable", NULL, 0);

    wine_tsx11_unlock();
    return (opengl_handle != NULL);

sym_not_found:
    wine_dlclose(opengl_handle, NULL, 0);
    opengl_handle = NULL;
    return FALSE;
}

static inline Wine_GLContext *alloc_context(void)
{
    Wine_GLContext *ret;

    if ((ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLContext))))
    {
        ret->next = context_list;
        if (context_list) context_list->prev = ret;
        context_list = ret;
    }
    return ret;
}

static inline void free_context(Wine_GLContext *context)
{
    if (context->next != NULL) context->next->prev = context->prev;
    if (context->prev != NULL) context->prev->next = context->next;
    else context_list = context->next;

    HeapFree(GetProcessHeap(), 0, context);
}

static inline Wine_GLContext *get_context_from_GLXContext(GLXContext ctx)
{
    Wine_GLContext *ret;
    if (!ctx) return NULL;
    for (ret = context_list; ret; ret = ret->next) if (ctx == ret->ctx) break;
    return ret;
}

/* retrieve the GLX drawable to use on a given DC */
inline static Drawable get_drawable( HDC hdc )
{
    GLXDrawable drawable;
    enum x11drv_escape_codes escape = X11DRV_GET_GLX_DRAWABLE;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(drawable), (LPSTR)&drawable )) drawable = 0;
    return drawable;
}

/** for use of wglGetCurrentReadDCARB */
inline static HDC get_hdc_from_Drawable(GLXDrawable d)
{
    Wine_GLContext *ret;
    for (ret = context_list; ret; ret = ret->next) {
        if (d == get_drawable( ret->hdc )) {
            return ret->hdc;
        }
    }
    return NULL;
}

inline static BOOL is_valid_context( Wine_GLContext *ctx )
{
    Wine_GLContext *ptr;
    for (ptr = context_list; ptr; ptr = ptr->next) if (ptr == ctx) break;
    return (ptr != NULL);
}

static int describeContext(Wine_GLContext* ctx) {
    int tmp;
    int ctx_vis_id;
    TRACE(" Context %p have (vis:%p):\n", ctx, ctx->vis);
    pglXGetFBConfigAttrib(ctx->display, ctx->fb_conf, GLX_FBCONFIG_ID, &tmp);
    TRACE(" - FBCONFIG_ID 0x%x\n", tmp);
    pglXGetFBConfigAttrib(ctx->display, ctx->fb_conf, GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID 0x%x\n", tmp);
    ctx_vis_id = tmp;
    return ctx_vis_id;
}

static int describeDrawable(Wine_GLContext* ctx, Drawable drawable) {
    int tmp;
    int nElements;
    int attribList[3] = { GLX_FBCONFIG_ID, 0, None };
    GLXFBConfig *fbCfgs;

    if (pglXQueryDrawable == NULL)  {
        /** glXQueryDrawable not available so returns not supported */
        return -1;
    }

    TRACE(" Drawable %p have :\n", (void*) drawable);
    pglXQueryDrawable(ctx->display, drawable, GLX_WIDTH, (unsigned int*) &tmp);
    TRACE(" - WIDTH as %d\n", tmp);
    pglXQueryDrawable(ctx->display, drawable, GLX_HEIGHT, (unsigned int*) &tmp);
    TRACE(" - HEIGHT as %d\n", tmp);
    pglXQueryDrawable(ctx->display, drawable, GLX_FBCONFIG_ID, (unsigned int*) &tmp);
    TRACE(" - FBCONFIG_ID as 0x%x\n", tmp);

    attribList[1] = tmp;
    fbCfgs = pglXChooseFBConfig(ctx->display, DefaultScreen(ctx->display), attribList, &nElements);
    if (fbCfgs == NULL) {
        return -1;
    }

    pglXGetFBConfigAttrib(ctx->display, fbCfgs[0], GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID as 0x%x\n", tmp);

    XFree(fbCfgs);

    return tmp;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one. It returns the index (zero-based)
 * into the GLX FB config table and it returns the number of supported WGL formats in fmt_count.
 */
static BOOL ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, int *fmt_index, int *fmt_count)
{
  int res = FALSE;
  int i = 0;
  GLXFBConfig* cfgs = NULL;
  int nCfgs = 0;
  int tmp_fmt_id = 0;
  int tmp_vis_id = 0;
  int nFormats = 1; /* Start at 1 as we always have a main visual */
  VisualID visualid = 0;

  /* Request to look up the format of the main visual when iPixelFormat = 1 */
  if(iPixelFormat == 1) visualid = XVisualIDFromVisual(visual);

  /* As mentioned in various parts of the code only the format of the main visual can be used for onscreen rendering.
   * Next to this format there are also so called offscreen rendering formats (used for pbuffers) which can be supported
   * because they don't need a visual. Below we use glXGetFBConfigs instead of glXChooseFBConfig to enumerate the fb configurations
   * bas this call lists both types of formats instead of only onscreen ones. */
  cfgs = pglXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
  if (NULL == cfgs || 0 == nCfgs) {
    ERR("glXChooseFBConfig returns NULL\n");
    if(cfgs != NULL) XFree(cfgs);
    return FALSE;
  }

  /* Find the requested offscreen format and count the number of offscreen formats */
  for(i=0; i<nCfgs; i++) {
    pglXGetFBConfigAttrib(display, cfgs[i], GLX_VISUAL_ID, &tmp_vis_id);
    pglXGetFBConfigAttrib(display, cfgs[i], GLX_FBCONFIG_ID, &tmp_fmt_id);

    /* We are looking up the GLX index of our main visual and have found it :) */
    if(iPixelFormat == 1 && visualid == tmp_vis_id) {
      *fmt_index = i;
      TRACE("Found FBCONFIG_ID 0x%x at index %d for VISUAL_ID 0x%x\n", tmp_fmt_id, *fmt_index, tmp_vis_id);
      res = TRUE;
    }
    /* We found an offscreen rendering format :) */
    else if(tmp_vis_id == 0) {
      nFormats++;
      TRACE("Checking offscreen format FBCONFIG_ID 0x%x at index %d\n", tmp_fmt_id, i);

      if(iPixelFormat == nFormats) {
        *fmt_index = i;
        TRACE("Found offscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", tmp_fmt_id, iPixelFormat, i);
        res = TRUE;
      }
    }
  }
  *fmt_count = nFormats;
  TRACE("Number of offscreen formats: %d; returning index: %d\n", *fmt_count, *fmt_index);

  if(cfgs != NULL) XFree(cfgs);

  if(res == FALSE && iPixelFormat == 1)
    ERR("Can't find a matching FBCONFIG_ID for VISUAL_ID 0x%lx!\n", visualid);

  return res;
}

/**
 * X11DRV_ChoosePixelFormat
 *
 * Equivalent of glXChooseVisual
 */
int X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev, 
			     const PIXELFORMATDESCRIPTOR *ppfd) {
  GLXFBConfig* cfgs = NULL;
  int ret = 0;
  int nCfgs = 0;
  int value = 0;
  int fmt_index = 0;

  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }

  if (TRACE_ON(opengl)) {
    TRACE("(%p,%p)\n", physDev, ppfd);

    dump_PIXELFORMATDESCRIPTOR((const PIXELFORMATDESCRIPTOR *) ppfd);
  }

  wine_tsx11_lock(); 
  if(!visual) {
    ERR("Can't get an opengl visual!\n");
    goto choose_exit;
  }

  /* Get a list containing all supported FB configurations */
  cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), NULL, &nCfgs);
  if (NULL == cfgs || 0 == nCfgs) {
    ERR("glXChooseFBConfig returns NULL (glError: %d)\n", pglGetError());
    goto choose_exit;
  }

  /* In case an fbconfig was found, check if it matches to the requirements of the ppfd */
  if(!ConvertPixelFormatWGLtoGLX(gdi_display, 1 /* main visual */, &fmt_index, &value)) {
    ERR("Can't find a matching FBCONFIG_ID for VISUAL_ID 0x%lx!\n", visual->visualid);
  } else {
    int dwFlags = 0;
    int iPixelType = 0;
    int value = 0;

    /* Pixel type */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_RENDER_TYPE, &value);
    if (value & GLX_RGBA_BIT)
      iPixelType = PFD_TYPE_RGBA;
    else
      iPixelType = PFD_TYPE_COLORINDEX;

    if (ppfd->iPixelType != iPixelType) {
      goto choose_exit;
    }

    /* Doublebuffer */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_DOUBLEBUFFER, &value); if (value) dwFlags |= PFD_DOUBLEBUFFER;
    if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE)) {
      if ((ppfd->dwFlags & PFD_DOUBLEBUFFER) != (dwFlags & PFD_DOUBLEBUFFER)) {
        goto choose_exit;
      }
    }

    /* Stereo */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_STEREO, &value); if (value) dwFlags |= PFD_STEREO;
    if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE)) {
      if ((ppfd->dwFlags & PFD_STEREO) != (dwFlags & PFD_STEREO)) {
        goto choose_exit;
      }
    }

    /* Alpha bits */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_ALPHA_SIZE, &value);
    if (ppfd->iPixelType==PFD_TYPE_RGBA && ppfd->cAlphaBits && !value) {
      goto choose_exit;
    }

    /* Depth bits */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_DEPTH_SIZE, &value);
    if (ppfd->cDepthBits && !value) {
      goto choose_exit;
    }

    /* Stencil bits */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_STENCIL_SIZE, &value);
    if (ppfd->cStencilBits && !value) {
      goto choose_exit;
    }

    /* Aux buffers */
    pglXGetFBConfigAttrib(gdi_display, cfgs[fmt_index], GLX_AUX_BUFFERS, &value);
    if (ppfd->cAuxBuffers && !value) {
      goto choose_exit;
    }

    /* When we pass all the checks we have found a matching format :) */
    ret = 1;
    TRACE("Successfully found a matching mode, returning index: %d\n", ret);
  }

choose_exit:
  if(!ret)
    TRACE("No matching mode was found returning 0\n");

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
  int fmt_index = 0;

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

  /* This function always reports the total number of supported pixel formats.
   * At the moment we only support the pixel format corresponding to the main
   * visual which got created at x11drv initialization. More formats could be
   * supported if there was a way to recreate x11 windows in x11drv. 
   * Because we only support one format nCfgs needs to be set to 1.
   */
  nCfgs = 1;

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

  /* Retrieve the index in the FBConfig table corresponding to the visual ID from the main visual */
  if(!ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, &fmt_index, &value)) {
      ERR("Can't find a valid pixel format index from the main visual, expect problems!\n");
      return 0;
  }

  ret = nCfgs;
  cur = cfgs[fmt_index];

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

  /* At the moment we only support the pixelformat corresponding to the main
   * x11drv visual which got created at x11drv initialization. More formats
   * can be supported if there was a way to recreate x11 windows in x11drv
   */
  if(iPixelFormat != 1) {
    TRACE("Invalid iPixelFormat: %d\n", iPixelFormat);
    return 0;
  }

  physDev->current_pf = iPixelFormat;

  if (TRACE_ON(opengl)) {
    int nCfgs_fmt = 0;
    GLXFBConfig* cfgs_fmt = NULL;
    GLXFBConfig cur_cfg;
    int value;
    int gl_test = 0;
    int fmt_index = 0;

    if(!ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, &fmt_index, &value)) {
      ERR("Can't find a valid pixel format index from the main visual, expect problems!\n");
      return TRUE; /* Return true because the SetPixelFormat stuff itself passed */
    }

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
    cur_cfg = cfgs_fmt[fmt_index];
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

/* OpenGL32 wglCreateContext */
HGLRC WINAPI X11DRV_wglCreateContext(HDC hdc)
{
    Wine_GLContext *ret;
    GLXFBConfig* cfgs_fmt = NULL;
    GLXFBConfig cur_cfg;
    int hdcPF = 1; /* We can only use the Wine's main visual which has an index of 1 */
    int tmp = 0;
    int fmt_index = 0;
    int nCfgs_fmt = 0;
    int value = 0;
    int gl_test = 0;

    TRACE("(%p)->(PF:%d)\n", hdc, hdcPF);

    /* First, get the visual in use by the X11DRV */
    if (!gdi_display) return 0;

    /* We can only render using the iPixelFormat (1) of Wine's Main visual, we need to get the correspondig GLX format.
    * If this fails something is very wrong on the system. */
    if(!ConvertPixelFormatWGLtoGLX(gdi_display, hdcPF, &tmp, &fmt_index)) {
        ERR("Cannot get FB Config for main iPixelFormat 1, expect problems!\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    cfgs_fmt = pglXGetFBConfigs(gdi_display, DefaultScreen(gdi_display), &nCfgs_fmt);
    if (NULL == cfgs_fmt || 0 == nCfgs_fmt) {
        ERR("Cannot get FB Configs, expect problems.\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    if (nCfgs_fmt < fmt_index) {
        ERR("(%p): unexpected pixelFormat(%d) > nFormats(%d), returns NULL\n", hdc, fmt_index, nCfgs_fmt);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    cur_cfg = cfgs_fmt[fmt_index];
    gl_test = pglXGetFBConfigAttrib(gdi_display, cur_cfg, GLX_FBCONFIG_ID, &value);
    if (gl_test) {
        ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }
    XFree(cfgs_fmt);

    /* The context will be allocated in the wglMakeCurrent call */
    wine_tsx11_lock();
    ret = alloc_context();
    wine_tsx11_unlock();
    ret->hdc = hdc;
    ret->display = gdi_display;
    ret->fb_conf = cur_cfg;
    /*ret->vis = vis;*/
    ret->vis = pglXGetVisualFromFBConfig(gdi_display, cur_cfg);

    TRACE(" creating context %p (GL context creation delayed)\n", ret);
    return (HGLRC) ret;
}

/* OpenGL32 wglDeleteContext */
BOOL WINAPI X11DRV_wglDeleteContext(HGLRC hglrc)
{
    Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
    BOOL ret = TRUE;

    TRACE("(%p)\n", hglrc);

    wine_tsx11_lock();
    /* A game (Half Life not to name it) deletes twice the same context,
    * so make sure it is valid first */
    if (is_valid_context( ctx ))
    {
        if (ctx->ctx) pglXDestroyContext(ctx->display, ctx->ctx);
        free_context(ctx);
    }
    else
    {
        WARN("Error deleting context !\n");
        SetLastError(ERROR_INVALID_HANDLE);
        ret = FALSE;
    }
    wine_tsx11_unlock();

    return ret;
}

/* OpenGL32 wglGetCurrentContext() */
HGLRC WINAPI X11DRV_wglGetCurrentContext(void) {
    GLXContext gl_ctx;
    Wine_GLContext *ret;

    TRACE("()\n");

    wine_tsx11_lock();
    gl_ctx = pglXGetCurrentContext();
    ret = get_context_from_GLXContext(gl_ctx);
    wine_tsx11_unlock();

    TRACE(" returning %p (GL context %p)\n", ret, gl_ctx);

    return (HGLRC)ret;
}

/* OpenGL32 wglGetCurrentDC */
HDC WINAPI X11DRV_wglGetCurrentDC(void) {
    GLXContext gl_ctx;
    Wine_GLContext *ret;

    TRACE("()\n");

    wine_tsx11_lock();
    gl_ctx = pglXGetCurrentContext();
    ret = get_context_from_GLXContext(gl_ctx);
    wine_tsx11_unlock();

    if (ret) {
        TRACE(" returning %p (GL context %p - Wine context %p)\n", ret->hdc, gl_ctx, ret);
        return ret->hdc;
    } else {
        TRACE(" no Wine context found for GLX context %p\n", gl_ctx);
        return 0;
    }
}

/* OpenGL32 wglGetCurrentReadDCARB */
HDC WINAPI X11DRV_wglGetCurrentReadDCARB(void) 
{
    GLXDrawable gl_d;
    HDC ret;

    TRACE("()\n");

    wine_tsx11_lock();
    gl_d = pglXGetCurrentReadDrawable();
    ret = get_hdc_from_Drawable(gl_d);
    wine_tsx11_unlock();

    TRACE(" returning %p (GL drawable %lu)\n", ret, gl_d);
    return ret;
}

/* OpenGL32 wglMakeCurrent */
BOOL WINAPI X11DRV_wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    BOOL ret;
    DWORD type = GetObjectType(hdc);

    TRACE("(%p,%p)\n", hdc, hglrc);

    wine_tsx11_lock();
    if (hglrc == NULL) {
        ret = pglXMakeCurrent(gdi_display, None, NULL);
        NtCurrentTeb()->glContext = NULL;
    } else {
        Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
        Drawable drawable = get_drawable( hdc );
        if (ctx->ctx == NULL) {
            int draw_vis_id, ctx_vis_id;
            VisualID visualid = (VisualID)GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
            TRACE(" Wine desktop VISUAL_ID is 0x%x\n", (unsigned int) visualid);
            draw_vis_id = describeDrawable(ctx, drawable);
            ctx_vis_id = describeContext(ctx);

            if (-1 == draw_vis_id || (draw_vis_id == visualid && draw_vis_id != ctx_vis_id)) {
                /**
                * Inherits from root window so reuse desktop visual
                */
                XVisualInfo template;
                XVisualInfo *vis;
                int num;
                template.visualid = visualid;
                vis = XGetVisualInfo(ctx->display, VisualIDMask, &template, &num);

                TRACE(" Creating GLX Context\n");
                ctx->ctx = pglXCreateContext(ctx->display, vis, NULL, type == OBJ_MEMDC ? False : True);
            } else {
                TRACE(" Creating GLX Context\n");
                ctx->ctx = pglXCreateContext(ctx->display, ctx->vis, NULL, type == OBJ_MEMDC ? False : True);
            }
            TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
        }
        TRACE(" make current for dis %p, drawable %p, ctx %p\n", ctx->display, (void*) drawable, ctx->ctx);
        ret = pglXMakeCurrent(ctx->display, drawable, ctx->ctx);
        NtCurrentTeb()->glContext = ctx;
        if(ret && type == OBJ_MEMDC)
        {
            ctx->do_escape = TRUE;
            pglDrawBuffer(GL_FRONT_LEFT);
        }
    }
    wine_tsx11_unlock();
    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/* OpenGL32 wglMakeContextCurrentARB */
BOOL WINAPI X11DRV_wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) 
{
    BOOL ret;
    TRACE("(%p,%p,%p)\n", hDrawDC, hReadDC, hglrc);

    wine_tsx11_lock();
    if (hglrc == NULL) {
        ret = pglXMakeCurrent(gdi_display, None, NULL);
    } else {
        if (NULL == pglXMakeContextCurrent) {
            ret = FALSE;
        } else {
            Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
            Drawable d_draw = get_drawable( hDrawDC );
            Drawable d_read = get_drawable( hReadDC );

            if (ctx->ctx == NULL) {
                ctx->ctx = pglXCreateContext(ctx->display, ctx->vis, NULL, GetObjectType(hDrawDC) == OBJ_MEMDC ? False : True);
                TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
            }
            ret = pglXMakeContextCurrent(ctx->display, d_draw, d_read, ctx->ctx);
        }
    }
    wine_tsx11_unlock();

    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/* OpenGL32 wglShaderLists */
BOOL WINAPI X11DRV_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    Wine_GLContext *org  = (Wine_GLContext *) hglrc1;
    Wine_GLContext *dest = (Wine_GLContext *) hglrc2;

    TRACE("(%p, %p)\n", org, dest);

    if (NULL != dest && dest->ctx != NULL) {
        ERR("Could not share display lists, context already created !\n");
        return FALSE;
    } else {
        if (org->ctx == NULL) {
            wine_tsx11_lock();
            describeContext(org);
            org->ctx = pglXCreateContext(org->display, org->vis, NULL, GetObjectType(org->hdc) == OBJ_MEMDC ? False : True);
            wine_tsx11_unlock();
            TRACE(" created a delayed OpenGL context (%p) for Wine context %p\n", org->ctx, org);
        }
        if (NULL != dest) {
            wine_tsx11_lock();
            describeContext(dest);
            /* Create the destination context with display lists shared */
            dest->ctx = pglXCreateContext(org->display, dest->vis, org->ctx, GetObjectType(org->hdc) == OBJ_MEMDC ? False : True);
            wine_tsx11_unlock();
            TRACE(" created a delayed OpenGL context (%p) for Wine context %p sharing lists with OpenGL ctx %p\n", dest->ctx, dest, org->ctx);
            return TRUE;
        }
    }
    return FALSE;
}


/* WGL helper function which handles differences in glGetIntegerv from WGL and GLX */ 
void X11DRV_wglGetIntegerv(GLenum pname, GLint* params) {
    TRACE("pname: 0x%x, params: %p\n", pname, params);
    if (pname == GL_DEPTH_BITS) { 
        GLXContext gl_ctx = pglXGetCurrentContext();
        Wine_GLContext* ret = get_context_from_GLXContext(gl_ctx);
        /*TRACE("returns Wine Ctx as %p\n", ret);*/
        /** 
        * if we cannot find a Wine Context
        * we only have the default wine desktop context, 
        * so if we have only a 24 depth say we have 32
        */
        if (NULL == ret && 24 == *params) { 
            *params = 32;
        }
        TRACE("returns GL_DEPTH_BITS as '%d'\n", *params);
    }
    if (pname == GL_ALPHA_BITS) {
        GLint tmp;
        GLXContext gl_ctx = pglXGetCurrentContext();
        Wine_GLContext* ret = get_context_from_GLXContext(gl_ctx);
        pglXGetFBConfigAttrib(ret->display, ret->fb_conf, GLX_ALPHA_SIZE, &tmp);
        TRACE("returns GL_ALPHA_BITS as '%d'\n", tmp);
        *params = tmp;
    }
}

static XID create_glxpixmap(X11DRV_PDEVICE *physDev)
{
    GLXPixmap ret;
    XVisualInfo *vis;
    XVisualInfo template;
    int num;

    wine_tsx11_lock();

    /* Retrieve the visualid from our main visual which is the only visual we can use */
    template.visualid =  XVisualIDFromVisual(visual);
    vis = XGetVisualInfo(gdi_display, VisualIDMask, &template, &num);

    ret = pglXCreateGLXPixmap(gdi_display, vis, physDev->bitmap->pixmap);
    XFree(vis);
    wine_tsx11_unlock(); 
    TRACE("return %lx\n", ret);
    return ret;
}

Drawable get_glxdrawable(X11DRV_PDEVICE *physDev)
{
    Drawable ret;

    if(physDev->bitmap)
    {
        if (physDev->bitmap->hbitmap == BITMAP_stock_phys_bitmap.hbitmap)
            ret = physDev->drawable; /* PBuffer */
        else
        {
            if(!physDev->bitmap->glxpixmap)
                physDev->bitmap->glxpixmap = create_glxpixmap(physDev);
            ret = physDev->bitmap->glxpixmap;
        }
    }
    else
        ret = physDev->drawable;
    return ret;
}

BOOL destroy_glxpixmap(XID glxpixmap)
{
    wine_tsx11_lock(); 
    pglXDestroyGLXPixmap(gdi_display, glxpixmap);
    wine_tsx11_unlock(); 
    return TRUE;
}

/**
 * X11DRV_SwapBuffers
 *
 * Swap the buffers of this DC
 */
BOOL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev)
{
  GLXDrawable drawable;
  if (!has_opengl()) {
    ERR("No libGL on this box - disabling OpenGL support !\n");
    return 0;
  }
  
  TRACE_(opengl)("(%p)\n", physDev);

  drawable = get_glxdrawable(physDev);
  wine_tsx11_lock();
  pglXSwapBuffers(gdi_display, drawable);
  wine_tsx11_unlock();

  /* FPS support */
  if (TRACE_ON(fps))
  {
      static long prev_time, frames;

      DWORD time = GetTickCount();
      frames++;
      /* every 1.5 seconds */
      if (time - prev_time > 1500) {
          TRACE_(fps)("@ approx %.2ffps\n", 1000.0*frames/(time - prev_time));
          prev_time = time;
          frames = 0;
      }
  }

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

/* OpenGL32 wglCreateContext */
HGLRC WINAPI X11DRV_wglCreateContext(HDC hdc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return NULL;
}

/* OpenGL32 wglDeleteContext */
BOOL WINAPI X11DRV_wglDeleteContext(HGLRC hglrc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* OpenGL32 wglGetCurrentContext() */
HGLRC WINAPI X11DRV_wglGetCurrentContext(void) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return NULL;
}

/* OpenGL32 wglGetCurrentDC */
HDC WINAPI X11DRV_wglGetCurrentDC(void) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return 0;
}

/* OpenGL32 wglGetCurrentReadDCARB */
HDC WINAPI X11DRV_wglGetCurrentReadDCARB(void)
{
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return 0;
}

/* OpenGL32 wglMakeCurrent */
BOOL WINAPI X11DRV_wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* OpenGL32 wglMakeContextCurrentARB */
BOOL WINAPI X11DRV_wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) 
{
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* OpenGL32 wglShaderLists */
BOOL WINAPI X11DRV_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
    return FALSE;
}

/* WGL helper function which handles differences in glGetIntegerv from WGL and GLX */ 
void X11DRV_wglGetIntegerv(int pname, int* params) {
    ERR_(opengl)("No OpenGL support compiled in.\n");
}

XVisualInfo *X11DRV_setup_opengl_visual( Display *display )
{
  return NULL;
}

Drawable get_glxdrawable(X11DRV_PDEVICE *physDev)
{
    return 0;
}

BOOL destroy_glxpixmap(XID glxpixmap)
{
    return FALSE;
}

#endif /* defined(HAVE_OPENGL) */
