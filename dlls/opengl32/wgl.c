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
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"

#include "wgl.h"
#include "wgl_ext.h"
#include "opengl_ext.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;

static GLXContext default_cx = NULL;
static Display *default_display;  /* display to use for default context */

static HMODULE opengl32_handle;

static void *(*p_glXGetProcAddressARB)(const GLubyte *);

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

  TRACE("(%p)\n", hdc);

  /* First, get the visual in use by the X11DRV */
  if (!display) return 0;
  template.visualid = (VisualID)GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
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
  TRACE("(%p,%d)\n", hdc, iLayerPlane);

  if (iLayerPlane == 0) {
      return wglCreateContext(hdc);
  }
  FIXME(" no handler for layer %d\n", iLayerPlane);

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
  FIXME("(%p,%d,%d,%d,%p)\n", hdc, iPixelFormat, iLayerPlane, nBytes, plpd);

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
    TRACE(" returning %p (GL context %p - Wine context %p)\n", ret->hdc, gl_ctx, ret);
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

static int wgl_compar(const void *elt_a, const void *elt_b) {
  return strcmp(((WGL_extension *) elt_a)->func_name,
		((WGL_extension *) elt_b)->func_name);
}

void* WINAPI wglGetProcAddress(LPCSTR  lpszProc) {
  void *local_func;
  OpenGL_extension  ext;
  OpenGL_extension *ext_ret;

  TRACE("(%s)\n", lpszProc);

  /* First, look if it's not already defined in the 'standard' OpenGL functions */
  if ((local_func = GetProcAddress(opengl32_handle, lpszProc)) != NULL) {
    TRACE(" found function in 'standard' OpenGL functions (%p)\n", local_func);
    return local_func;
  }

  if (p_glXGetProcAddressARB == NULL) {
    ERR("Warning : dynamic GL extension loading not supported by native GL library.\n");
    return NULL;
  }
  
  /* After that, search in the thunks to find the real name of the extension */
  ext.name = (char *) lpszProc;
  ext_ret = (OpenGL_extension *) bsearch(&ext, extension_registry,
					 extension_registry_size, sizeof(OpenGL_extension), compar);

  if (ext_ret == NULL) {
    WGL_extension wgl_ext, *wgl_ext_ret;

    /* Try to find the function in the WGL extensions ... */
    wgl_ext.func_name = (char *) lpszProc;
    wgl_ext_ret = (WGL_extension *) bsearch(&wgl_ext, wgl_extension_registry,
					    wgl_extension_registry_size, sizeof(WGL_extension), wgl_compar);

    if (wgl_ext_ret == NULL) {
      /* Some sanity checks :-) */
      ENTER_GL();
      local_func = p_glXGetProcAddressARB(lpszProc);
      LEAVE_GL();
      if (local_func != NULL) {
	ERR("Extension %s defined in the OpenGL library but NOT in opengl_ext.c... Please report (lionel.ulmer@free.fr) !\n", lpszProc);
	return NULL;
      }
      
      WARN("Did not find extension %s in either Wine or your OpenGL library.\n", lpszProc);
      return NULL;
    } else {
	void *ret = NULL;

	if (wgl_ext_ret->func_init != NULL) {
	    const char *err_msg;
	    if ((err_msg = wgl_ext_ret->func_init(p_glXGetProcAddressARB,
						  wgl_ext_ret->context)) == NULL) {
		ret = wgl_ext_ret->func_address;
	    } else {
		WARN("Error when getting WGL extension '%s' : %s.\n", debugstr_a(lpszProc), err_msg);
		return NULL;
	    }
	} else {
	  ret = wgl_ext_ret->func_address;
	}

	if (ret)
	    TRACE(" returning WGL function  (%p)\n", ret);
	return ret;
    }
  } else {
    ENTER_GL();
    local_func = p_glXGetProcAddressARB(ext_ret->glx_name);
    LEAVE_GL();
    
    /* After that, look at the extensions defined in the Linux OpenGL library */
    if (local_func == NULL) {
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

      ret = GetProcAddress(opengl32_handle, buf);
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

  TRACE("(%p,%p)\n", hdc, hglrc);

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
  TRACE("(%p, %08x)\n", hdc, fuPlanes);

  if (fuPlanes & WGL_SWAP_MAIN_PLANE) {
    if (!SwapBuffers(hdc)) return FALSE;
    fuPlanes &= ~WGL_SWAP_MAIN_PLANE;
  }

  if (fuPlanes) {
    WARN("Following layers unhandled : %08x\n", fuPlanes);
  }

  return TRUE;
}

static BOOL internal_wglUseFontBitmaps(HDC hdc,
				       DWORD first,
				       DWORD count,
				       DWORD listBase,
				       DWORD WINAPI (*GetGlyphOutline_ptr)(HDC,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*))
{
    /* We are running using client-side rendering fonts... */
    GLYPHMETRICS gm;
    static const MAT2 id = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    int glyph;
    int size = 0;
    void *bitmap = NULL, *gl_bitmap = NULL;
    int org_alignment;

    ENTER_GL();
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &org_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    LEAVE_GL();

    for (glyph = first; glyph < first + count; glyph++) {
	int needed_size = GetGlyphOutline_ptr(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &id);
	int height, width_int;
	
	if (needed_size == GDI_ERROR) goto error;
	if (needed_size > size) {
	    size = needed_size;
	    if (bitmap) HeapFree(GetProcessHeap(), 0, bitmap);
	    if (gl_bitmap) HeapFree(GetProcessHeap(), 0, gl_bitmap);
	    bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	    gl_bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	}
	if (GetGlyphOutline_ptr(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, &id) == GDI_ERROR) goto error;
	if (TRACE_ON(opengl)) {
	    unsigned int height, width, bitmask;
	    unsigned char *bitmap_ = (unsigned char *) bitmap;
	    
	    DPRINTF("Glyph : %d\n", glyph);
	    DPRINTF("  - bbox : %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
	    DPRINTF("  - origin : (%ld , %ld)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
	    DPRINTF("  - increment : %d - %d\n", gm.gmCellIncX, gm.gmCellIncY);
	    DPRINTF("  - size : %d\n", needed_size);
	    DPRINTF("  - bitmap : \n");
	    for (height = 0; height < gm.gmBlackBoxY; height++) {
		DPRINTF("      ");
		for (width = 0, bitmask = 0x80; width < gm.gmBlackBoxX; width++, bitmask >>= 1) {
		    if (bitmask == 0) {
			bitmap_ += 1;
			bitmask = 0x80;
		    }
		    if (*bitmap_ & bitmask)
			DPRINTF("*");
		    else
			DPRINTF(" ");
		}
		bitmap_ += (4 - (((unsigned int) bitmap_) & 0x03));
		DPRINTF("\n");
	    }
	}
	
	/* For some obscure reasons, I seem to need to rotate the glyph for OpenGL to be happy.
	   As Wine does not seem to support the MAT2 field, I need to do it myself.... */
	width_int = (gm.gmBlackBoxX + 31) / 32;
	for (height = 0; height < gm.gmBlackBoxY; height++) {
	    int width;
	    for (width = 0; width < width_int; width++) {
		((int *) gl_bitmap)[(gm.gmBlackBoxY - height - 1) * width_int + width] =
		    ((int *) bitmap)[height * width_int + width];
	    }
	}
	
	ENTER_GL();
	glNewList(listBase++, GL_COMPILE);
	glBitmap(gm.gmBlackBoxX, gm.gmBlackBoxY, gm.gmptGlyphOrigin.x,
		 gm.gmBlackBoxY - gm.gmptGlyphOrigin.y, gm.gmCellIncX, gm.gmCellIncY, gl_bitmap);
	glEndList();
	LEAVE_GL();
    }
    
    ENTER_GL();
    glPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
    LEAVE_GL();
    
    if (bitmap) HeapFree(GetProcessHeap(), 0, bitmap);
    if (gl_bitmap) HeapFree(GetProcessHeap(), 0, gl_bitmap);
    return TRUE;

  error:
    ENTER_GL();
    glPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
    LEAVE_GL();

    if (bitmap) HeapFree(GetProcessHeap(), 0, bitmap);
    if (gl_bitmap) HeapFree(GetProcessHeap(), 0, gl_bitmap);
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

  TRACE("(%p, %ld, %ld, %ld) using font %ld\n", hdc, first, count, listBase, fid);

  if (fid == 0) {
      return internal_wglUseFontBitmaps(hdc, first, count, listBase, GetGlyphOutlineA);
  }

  ENTER_GL();
  /* I assume that the glyphs are at the same position for X and for Windows */
  glXUseXFont(fid, first, count, listBase);
  LEAVE_GL();
  return TRUE;
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc,
			       DWORD first,
			       DWORD count,
			       DWORD listBase)
{
  Font fid = get_font( hdc );

  TRACE("(%p, %ld, %ld, %ld) using font %ld\n", hdc, first, count, listBase, fid);

  if (fid == 0) {
      return internal_wglUseFontBitmaps(hdc, first, count, listBase, GetGlyphOutlineW);
  }

  WARN("Using the glX API for the WCHAR variant - some characters may come out incorrectly !\n");
  
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

/* No need to load any other libraries as according to the ABI, libGL should be self-sufficient and
   include all dependencies
*/
#ifndef SONAME_LIBGL
#define SONAME_LIBGL "libGL.so"
#endif

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
  HMODULE mod = GetModuleHandleA( "x11drv.dll" );
  void *opengl_handle;

  if (!root || !mod)
  {
      ERR("X11DRV not loaded. Cannot create default context.\n");
      return FALSE;
  }

  wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
  wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );

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

  opengl_handle = wine_dlopen(SONAME_LIBGL, RTLD_NOW|RTLD_GLOBAL, NULL, 0);
  if (opengl_handle != NULL) {
   p_glXGetProcAddressARB = wine_dlsym(opengl_handle, "glXGetProcAddressARB", NULL, 0);
   wine_dlclose(opengl_handle, NULL, 0);
   if (p_glXGetProcAddressARB == NULL)
	   TRACE("could not find glXGetProcAddressARB in libGL.\n");
  }

  /* Initialize also the list of supported WGL extensions. */
  wgl_ext_initialize_extensions(default_display, DefaultScreen(default_display));
  
  if (default_cx == NULL) {
    ERR("Could not create default context.\n");
  }
  return TRUE;
}


/**********************************************************************/

static void process_detach(void)
{
  glXDestroyContext(default_display, default_cx);

  /* Do not leak memory... */
  wgl_ext_finalize_extensions();
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        opengl32_handle = hinst;
        DisableThreadLibraryCalls(hinst);
        return process_attach();
    case DLL_PROCESS_DETACH:
        process_detach();
        break;
    }
    return TRUE;
}
