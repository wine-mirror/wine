/* Window-specific OpenGL functions implementation.
 *
 * Copyright (c) 1999 Lionel Ulmer
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

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "wine_gl.h"
#include "x11drv.h"

#include "wgl.h"
#include "opengl_ext.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

static GLXContext default_cx = NULL;
static Display *default_display;  /* display to use for default context */

typedef struct wine_glcontext {
  HDC hdc;
  Display *display;
  GLXContext ctx;
  XVisualInfo *vis;
  struct wine_glcontext *next;
  struct wine_glcontext *prev;
} Wine_GLContext;
static Wine_GLContext *context_list;

static inline Wine_GLContext *get_context_from_GLXContext(GLXContext ctx)
{
    Wine_GLContext *ret;
    for (ret = context_list; ret; ret = ret->next) if (ctx == ret->ctx) break;
    return ret;
}

static inline void free_context(Wine_GLContext *context)
{
  if (context->next != NULL) context->next->prev = context->prev;
  if (context->prev != NULL) context->prev->next = context->next;
  else context_list = context->next;

  HeapFree(GetProcessHeap(), 0, context);
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

inline static BOOL is_valid_context( Wine_GLContext *ctx )
{
    Wine_GLContext *ptr;
    for (ptr = context_list; ptr; ptr = ptr->next) if (ptr == ctx) break;
    return (ptr != NULL);
}

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


/* retrieve the X drawable to use on a given DC */
inline static Font get_font( HDC hdc )
{
    Font font;
    enum x11drv_escape_codes escape = X11DRV_GET_FONT;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(font), (LPSTR)&font )) font = 0;
    return font;
}


/***********************************************************************
 *		wglCreateContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateContext(HDC hdc)
{
  XVisualInfo *vis;
  Wine_GLContext *ret;
  int num;
  XVisualInfo template;
  Display *display = get_display( hdc );

  TRACE("(%08x)\n", hdc);

  /* First, get the visual in use by the X11DRV */
  if (!display) return 0;
  template.visualid = GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
  vis = XGetVisualInfo(display, VisualIDMask, &template, &num);

  if (vis == NULL) {
    ERR("NULL visual !!!\n");
    /* Need to set errors here */
    return NULL;
  }

  /* The context will be allocated in the wglMakeCurrent call */
  ENTER_GL();
  ret = alloc_context();
  LEAVE_GL();
  ret->hdc = hdc;
  ret->display = display;
  ret->vis = vis;

  TRACE(" creating context %p (GL context creation delayed)\n", ret);
  return (HGLRC) ret;
}

/***********************************************************************
 *		wglCreateLayerContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateLayerContext(HDC hdc,
				   int iLayerPlane) {
  FIXME("(%08x,%d): stub !\n", hdc, iLayerPlane);

  return NULL;
}

/***********************************************************************
 *		wglCopyContext (OPENGL32.@)
 */
BOOL WINAPI wglCopyContext(HGLRC hglrcSrc,
			   HGLRC hglrcDst,
			   UINT mask) {
  FIXME("(%p,%p,%d)\n", hglrcSrc, hglrcDst, mask);

  return FALSE;
}

/***********************************************************************
 *		wglDeleteContext (OPENGL32.@)
 */
BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
  Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
  BOOL ret = TRUE;

  TRACE("(%p)\n", hglrc);
  
  ENTER_GL();
  /* A game (Half Life not to name it) deletes twice the same context,
   * so make sure it is valid first */
  if (is_valid_context( ctx ))
  {
      if (ctx->ctx) glXDestroyContext(ctx->display, ctx->ctx);
      free_context(ctx);
  }
  else
  {
    WARN("Error deleting context !\n");
    SetLastError(ERROR_INVALID_HANDLE);
    ret = FALSE;
  }
  LEAVE_GL();
  
  return ret;
}

/***********************************************************************
 *		wglDescribeLayerPlane (OPENGL32.@)
 */
BOOL WINAPI wglDescribeLayerPlane(HDC hdc,
				  int iPixelFormat,
				  int iLayerPlane,
				  UINT nBytes,
				  LPLAYERPLANEDESCRIPTOR plpd) {
  FIXME("(%08x,%d,%d,%d,%p)\n", hdc, iPixelFormat, iLayerPlane, nBytes, plpd);

  return FALSE;
}

/***********************************************************************
 *		wglGetCurrentContext (OPENGL32.@)
 */
HGLRC WINAPI wglGetCurrentContext(void) {
  GLXContext gl_ctx;
  Wine_GLContext *ret;

  TRACE("()\n");

  ENTER_GL();
  gl_ctx = glXGetCurrentContext();
  ret = get_context_from_GLXContext(gl_ctx);
  LEAVE_GL();

  TRACE(" returning %p (GL context %p)\n", ret, gl_ctx);
  
  return ret;
}

/***********************************************************************
 *		wglGetCurrentDC (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentDC(void) {
  GLXContext gl_ctx;
  Wine_GLContext *ret;

  TRACE("()\n");
  
  ENTER_GL();
  gl_ctx = glXGetCurrentContext();
  ret = get_context_from_GLXContext(gl_ctx);
  LEAVE_GL();

  if (ret) {
    TRACE(" returning %08x (GL context %p - Wine context %p)\n", ret->hdc, gl_ctx, ret);
    return ret->hdc;
  } else {
    TRACE(" no Wine context found for GLX context %p\n", gl_ctx);
    return 0;
  }
}

/***********************************************************************
 *		wglGetLayerPaletteEntries (OPENGL32.@)
 */
int WINAPI wglGetLayerPaletteEntries(HDC hdc,
				     int iLayerPlane,
				     int iStart,
				     int cEntries,
				     const COLORREF *pcr) {
  FIXME("(): stub !\n");

  return 0;
}

/***********************************************************************
 *		wglGetProcAddress (OPENGL32.@)
 */
static int compar(const void *elt_a, const void *elt_b) {
  return strcmp(((OpenGL_extension *) elt_a)->name,
		((OpenGL_extension *) elt_b)->name);
}

void* WINAPI wglGetProcAddress(LPCSTR  lpszProc) {
  void *local_func;
  static HMODULE hm = 0;
  OpenGL_extension  ext;
  OpenGL_extension *ext_ret;


  TRACE("(%s)\n", lpszProc);

  if (hm == 0)
      hm = GetModuleHandleA("opengl32");

  /* First, look if it's not already defined in the 'standard' OpenGL functions */
  if ((local_func = GetProcAddress(hm, lpszProc)) != NULL) {
    TRACE(" found function in 'standard' OpenGL functions (%p)\n", local_func);
    return local_func;
  }

  /* After that, search in the thunks to find the real name of the extension */
  ext.name = (char *) lpszProc;
  ext_ret = (OpenGL_extension *) bsearch(&ext, extension_registry,
					 extension_registry_size, sizeof(OpenGL_extension), compar);

  if (ext_ret == NULL) {
    /* Some sanity checks :-) */
    if (glXGetProcAddressARB(lpszProc) != NULL) {
      ERR("Extension %s defined in the OpenGL library but NOT in opengl_ext.c... Please report (lionel.ulmer@free.fr) !\n", lpszProc);
      return NULL;
    }

    WARN("Did not find extension %s in either Wine or your OpenGL library.\n", lpszProc);
    return NULL;
  } else {
    /* After that, look at the extensions defined in the Linux OpenGL library */
    if ((local_func = glXGetProcAddressARB(ext_ret->glx_name)) == NULL) {
      char buf[256];
      void *ret = NULL;
      
      /* Remove the 3 last letters (EXT, ARB, ...).
	 
	 I know that some extensions have more than 3 letters (MESA, NV,
	 INTEL, ...), but this is only a stop-gap measure to fix buggy
	 OpenGL drivers (moreover, it is only useful for old 1.0 apps
	 that query the glBindTextureEXT extension).
      */
      strncpy(buf, ext_ret->glx_name, strlen(ext_ret->glx_name) - 3);
      buf[strlen(ext_ret->glx_name) - 3] = '\0';
      TRACE(" extension not found in the Linux OpenGL library, checking against libGL bug with %s..\n", buf);
      
      ret = GetProcAddress(hm, buf);
      if (ret != NULL) {
	TRACE(" found function in main OpenGL library (%p) !\n", ret);
      } else {
	WARN("Did not find function %s (%s) in your OpenGL library !\n", lpszProc, ext_ret->glx_name);
      }	
      
      return ret;
    } else {
      TRACE(" returning function  (%p)\n", ext_ret->func);
      *(ext_ret->func_ptr) = local_func;
      
      return ext_ret->func;
    }
  }
}

/***********************************************************************
 *		wglMakeCurrent (OPENGL32.@)
 */
BOOL WINAPI wglMakeCurrent(HDC hdc,
			   HGLRC hglrc) {
  BOOL ret;

  TRACE("(%08x,%p)\n", hdc, hglrc);
  
  ENTER_GL();
  if (hglrc == NULL) {
      ret = glXMakeCurrent(default_display, None, NULL);
  } else {
      Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
      Drawable drawable = get_drawable( hdc );

      if (ctx->ctx == NULL) {
	ctx->ctx = glXCreateContext(ctx->display, ctx->vis, NULL, True);
	TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
      }
      ret = glXMakeCurrent(ctx->display, drawable, ctx->ctx);
  }
  LEAVE_GL();
  TRACE(" returning %s\n", (ret ? "True" : "False"));
  return ret;
}

/***********************************************************************
 *		wglRealizeLayerPalette (OPENGL32.@)
 */
BOOL WINAPI wglRealizeLayerPalette(HDC hdc,
				   int iLayerPlane,
				   BOOL bRealize) {
  FIXME("()\n");

  return FALSE;
}

/***********************************************************************
 *		wglSetLayerPaletteEntries (OPENGL32.@)
 */
int WINAPI wglSetLayerPaletteEntries(HDC hdc,
				     int iLayerPlane,
				     int iStart,
				     int cEntries,
				     const COLORREF *pcr) {
  FIXME("(): stub !\n");

  return 0;
}

/***********************************************************************
 *		wglShareLists (OPENGL32.@)
 */
BOOL WINAPI wglShareLists(HGLRC hglrc1,
			  HGLRC hglrc2) {
  Wine_GLContext *org  = (Wine_GLContext *) hglrc1;
  Wine_GLContext *dest = (Wine_GLContext *) hglrc2;
  
  TRACE("(%p, %p)\n", org, dest);

  if (dest->ctx != NULL) {
    ERR("Could not share display lists, context already created !\n");
    return FALSE;
  } else {
    if (org->ctx == NULL) {
      ENTER_GL();
      org->ctx = glXCreateContext(org->display, org->vis, NULL, True);
      LEAVE_GL();
      TRACE(" created a delayed OpenGL context (%p) for Wine context %p\n", org->ctx, org);
    }

    ENTER_GL();
    /* Create the destination context with display lists shared */
    dest->ctx = glXCreateContext(org->display, dest->vis, org->ctx, True);
    LEAVE_GL();
    TRACE(" created a delayed OpenGL context (%p) for Wine context %p sharing lists with OpenGL ctx %p\n", dest->ctx, dest, org->ctx);
  }
  
  return TRUE;
}

/***********************************************************************
 *		wglSwapLayerBuffers (OPENGL32.@)
 */
BOOL WINAPI wglSwapLayerBuffers(HDC hdc,
				UINT fuPlanes) {
  FIXME("(): stub !\n");

  return FALSE;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc,
			       DWORD first,
			       DWORD count,
			       DWORD listBase)
{
  Font fid = get_font( hdc );

  TRACE("(%08x, %ld, %ld, %ld)\n", hdc, first, count, listBase);

  ENTER_GL();
  /* I assume that the glyphs are at the same position for X and for Windows */
  glXUseXFont(fid, first, count, listBase);
  LEAVE_GL();
  return TRUE;
}
 
/***********************************************************************
 *		wglUseFontOutlinesA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontOutlinesA(HDC hdc,
				DWORD first,
				DWORD count,
				DWORD listBase,
				FLOAT deviation,
				FLOAT extrusion,
				int format,
				LPGLYPHMETRICSFLOAT lpgmf) {
  FIXME("(): stub !\n");

  return FALSE;
}


/* This is for brain-dead applications that use OpenGL functions before even
   creating a rendering context.... */
static BOOL process_attach(void)
{
  XWindowAttributes win_attr;
  Visual *rootVisual;
  int num;
  XVisualInfo template;
  HDC hdc;
  XVisualInfo *vis = NULL;
  Window root = (Window)GetPropA( GetDesktopWindow(), "__wine_x11_whole_window" );

  if (!root)
  {
      ERR("X11DRV not loaded. Cannot create default context.\n");
      return FALSE;
  }

  hdc = GetDC(0);
  default_display = get_display( hdc );
  ReleaseDC( 0, hdc );
  if (!default_display)
  {
      ERR("X11DRV not loaded. Cannot get display for screen DC.\n");
      return FALSE;
  }

  ENTER_GL();

  /* Try to get the visual from the Root Window.  We can't use the standard (presumably
     double buffered) X11DRV visual with the Root Window, since we don't know if the Root
     Window was created using the standard X11DRV visual, and glXMakeCurrent can't deal 
     with mismatched visuals.  Note that the Root Window visual may not be double 
     buffered, so apps actually attempting to render this way may flicker */
  if (XGetWindowAttributes( default_display, root, &win_attr ))
  {
    rootVisual = win_attr.visual; 
  }
  else
  {
    /* Get the default visual, since we can't seem to get the attributes from the 
       Root Window.  Let's hope that the Root Window Visual matches the DefaultVisual */
    rootVisual = DefaultVisual( default_display, DefaultScreen(default_display) );
  }

  template.visualid = XVisualIDFromVisual(rootVisual);
  vis = XGetVisualInfo(default_display, VisualIDMask, &template, &num);
  if (vis != NULL) default_cx = glXCreateContext(default_display, vis, 0, GL_TRUE);
  if (default_cx != NULL) glXMakeCurrent(default_display, root, default_cx);
  XFree(vis);
  LEAVE_GL();

  if (default_cx == NULL) {
    ERR("Could not create default context.\n");
  }
  return TRUE;
}

static void process_detach(void)
{
  glXDestroyContext(default_display, default_cx);
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI OpenGL32_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        return process_attach();
    case DLL_PROCESS_DETACH:
        process_detach();
        break;
    }
    return TRUE;
}
