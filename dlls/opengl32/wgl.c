/* Window-specific OpenGL functions implementation.

     Copyright (c) 1999 Lionel Ulmer
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "wine/exception.h"

#include "debugtools.h"
#include "gdi.h"
#include "windef.h"
#include "winerror.h"
#include "wine_gl.h"
#include "x11drv.h"
#include "x11font.h"
#include "msvcrt/excpt.h"

#include "wgl.h"
#include "opengl_ext.h"

DEFAULT_DEBUG_CHANNEL(opengl);

static GLXContext default_cx = NULL;

typedef struct wine_glcontext {
  HDC hdc;
  GLXContext ctx;
  XVisualInfo *vis;
  struct wine_glcontext *next;
  struct wine_glcontext *prev;
} Wine_GLContext;
static Wine_GLContext *context_array;

static inline Wine_GLContext *get_context_from_GLXContext(GLXContext ctx) {
  Wine_GLContext *ret = context_array;
  while (ret != NULL) if (ctx == ret->ctx) break; else ret = ret->next;
  return ret;
}
  
static inline void free_context(Wine_GLContext *context) {
  if (context->next != NULL) context->next->prev = context->prev;
  if (context->prev != NULL) context->prev->next = context->next;
  else                       context_array = context->next;

  HeapFree(GetProcessHeap(), 0, context);
}

static inline Wine_GLContext *alloc_context(void) {
  Wine_GLContext *ret;

  ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLContext));
  ret->next = context_array;
  if (context_array != NULL) context_array->prev = ret;
  else                       context_array = ret;
  
  return ret;
}


static int XGLErrorFlag = 0;
static int XGLErrorHandler(Display *dpy, XErrorEvent *event) {
    XGLErrorFlag = 1;
    return 0;
}
/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
  return EXCEPTION_EXECUTE_HANDLER;
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

  TRACE("(%08x)\n", hdc);

  /* First, get the visual in use by the X11DRV */
  template.visualid = GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
  vis = XGetVisualInfo(gdi_display, VisualIDMask, &template, &num);

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
BOOL WINAPI wglDeleteContext(HGLRC hglrc) {
  int (*WineXHandler)(Display *, XErrorEvent *);
  Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
  BOOL ret;
  
  TRACE("(%p)\n", hglrc);
  
  ENTER_GL();
  /* A game (Half Life not to name it) deletes twice the same context. To prevent
     crashes, run with our own error function enabled */
  XSync(gdi_display, False);
  XGLErrorFlag = 0;
  WineXHandler = XSetErrorHandler(XGLErrorHandler);
  __TRY {
    glXDestroyContext(gdi_display, ctx->ctx);
    XSync(gdi_display, False);
    XFlush(gdi_display);

    if (XGLErrorHandler == 0) free_context(ctx);
  }
  __EXCEPT(page_fault) {
    XGLErrorFlag = 1;
  }
  __ENDTRY

  ret = TRUE;
  XSetErrorHandler(WineXHandler);
  if (XGLErrorFlag) {
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
  
  if (hglrc == NULL) {
    ENTER_GL();
    ret = glXMakeCurrent(gdi_display,
			 None,
			 NULL);
    LEAVE_GL();
  } else {
    DC * dc = DC_GetDCPtr( hdc );
    
    if (dc == NULL) {
      ERR("Null DC !!!\n");
      ret = FALSE;
    } else {
      X11DRV_PDEVICE *physDev;
      Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
      
      physDev =(X11DRV_PDEVICE *)dc->physDev;

      if (ctx->ctx == NULL) {
	ENTER_GL();
	ctx->ctx = glXCreateContext(gdi_display, ctx->vis, NULL, True);
	LEAVE_GL();
	TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
      }
      
      ENTER_GL();
      ret = glXMakeCurrent(gdi_display,
			   physDev->drawable,
			   ctx->ctx);
      LEAVE_GL();
      GDI_ReleaseObj( hdc );
    }
  }
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
      org->ctx = glXCreateContext(gdi_display, org->vis, NULL, True);
      LEAVE_GL();
      TRACE(" created a delayed OpenGL context (%p) for Wine context %p\n", org->ctx, org);
    }

    ENTER_GL();
    /* Create the destination context with display lists shared */
    dest->ctx = glXCreateContext(gdi_display, dest->vis, org->ctx, True);
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
			       DWORD listBase) {
  DC * dc = DC_GetDCPtr( hdc );
  X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
  fontObject* pfo = XFONT_GetFontObject( physDev->font );
  Font fid = pfo->fs->fid;

  TRACE("(%08x, %ld, %ld, %ld)\n", hdc, first, count, listBase);

  ENTER_GL();
  /* I assume that the glyphs are at the same position for X and for Windows */
  glXUseXFont(fid, first, count, listBase);
  LEAVE_GL();
  GDI_ReleaseObj( hdc );
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
static void process_attach(void) {
  XWindowAttributes win_attr;
  Visual *rootVisual;
  int num;
  XVisualInfo template;
  XVisualInfo *vis = NULL;
  Window root = (Window)GetPropA( GetDesktopWindow(), "__wine_x11_whole_window" );

  if (!root)
  {
      ERR("X11DRV not loaded. Cannot create default context.\n");
      return;
  }

  ENTER_GL();

  /* Try to get the visual from the Root Window.  We can't use the standard (presumably
     double buffered) X11DRV visual with the Root Window, since we don't know if the Root
     Window was created using the standard X11DRV visual, and glXMakeCurrent can't deal 
     with mismatched visuals.  Note that the Root Window visual may not be double 
     buffered, so apps actually attempting to render this way may flicker */
  if (XGetWindowAttributes( gdi_display, root, &win_attr ))
  {
    rootVisual = win_attr.visual; 
  }
  else
  {
    /* Get the default visual, since we can't seem to get the attributes from the 
       Root Window.  Let's hope that the Root Window Visual matches the DefaultVisual */
    rootVisual = DefaultVisual( gdi_display, DefaultScreen(gdi_display) );
  }

  template.visualid = XVisualIDFromVisual(rootVisual);
  vis = XGetVisualInfo(gdi_display, VisualIDMask, &template, &num);
  if (vis != NULL)        default_cx = glXCreateContext(gdi_display, vis, 0, GL_TRUE);
  if (default_cx != NULL) glXMakeCurrent(gdi_display, root, default_cx);
  XFree(vis);
  LEAVE_GL();

  if (default_cx == NULL) {
    ERR("Could not create default context.\n");
  }
  
  context_array = NULL;
}

static void process_detach(void) {
  glXDestroyContext(gdi_display, default_cx);
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI OpenGL32_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
  switch(reason) {
  case DLL_PROCESS_ATTACH:
    process_attach();
    break;
  case DLL_PROCESS_DETACH:
    process_detach();
    break;
  }
  return TRUE;
}
