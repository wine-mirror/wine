/* Window-specific OpenGL functions implementation.

     Copyright (c) 1999 Lionel Ulmer
*/

#include <stdlib.h>

#include "wine/exception.h"

#include "config.h"
#include "debugtools.h"
#include "gdi.h"
#include "dc.h"
#include "windef.h"
#include "winerror.h"
#include "wine_gl.h"
#include "x11drv.h"
#include "x11font.h"

#include "wgl.h"
#include "opengl_ext.h"

DEFAULT_DEBUG_CHANNEL(opengl);

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
 *		wglCreateContext
 */
HGLRC WINAPI wglCreateContext(HDC hdc) {
  DC * dc = DC_GetDCPtr( hdc );
  X11DRV_PDEVICE *physDev;
  XVisualInfo *vis;
  Wine_GLContext *ret;

  TRACE("(%08x)\n", hdc);

  if (dc == NULL) {
    ERR("Null DC !!!\n");
    return NULL;
  }
  
  physDev = (X11DRV_PDEVICE *)dc->physDev;

  /* First, get the visual for the choosen pixel format */
  vis = physDev->visuals[physDev->current_pf - 1];

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
 *		wglCreateLayerContext
 */
HGLRC WINAPI wglCreateLayerContext(HDC hdc,
				   int iLayerPlane) {
  FIXME("(%08x,%d): stub !\n", hdc, iLayerPlane);

  return NULL;
}

/***********************************************************************
 *		wglCopyContext
 */
BOOL WINAPI wglCopyContext(HGLRC hglrcSrc,
			   HGLRC hglrcDst,
			   UINT mask) {
  FIXME("(%p,%p,%d)\n", hglrcSrc, hglrcDst, mask);

  return FALSE;
}

/***********************************************************************
 *		wglDeleteContext
 */
BOOL WINAPI wglDeleteContext(HGLRC hglrc) {
  int (*WineXHandler)(Display *, XErrorEvent *);
  Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
  BOOL ret;
  
  TRACE("(%p)\n", hglrc);
  
  ENTER_GL();
  /* A game (Half Life not to name it) deletes twice the same context. To prevent
     crashes, run with our own error function enabled */
  XSync(display, False);
  XGLErrorFlag = 0;
  WineXHandler = XSetErrorHandler(XGLErrorHandler);
  __TRY {
    glXDestroyContext(display, ctx->ctx);
    XSync(display, False);
    XFlush(display);

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
 *		wglDescribeLayerPlane
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
 *		wglGetCurrentContext
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
 *		wglGetCurrentDC
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
 *		wglGetLayerPaletteEntries
 */
int WINAPI wglGetLayerPaletteEntries(HDC hdc,
				     int iLayerPlane,
				     int iStart,
				     int cEntries,
				     const COLORREF *pcr) {
  FIXME("(): stub !\n");

  return 0;
}

static int compar(const void *elt_a, const void *elt_b) {
  return strcmp(((OpenGL_extension *) elt_a)->name,
		((OpenGL_extension *) elt_b)->name);
}

/***********************************************************************
 *		wglGetProcAddress
 */
void* WINAPI wglGetProcAddress(LPCSTR  lpszProc) {
  void *local_func;
  static HMODULE hm = 0;

  TRACE("(%s)\n", lpszProc);

  if (hm == 0)
      hm = GetModuleHandleA("opengl32");

  /* First, look if it's not already defined in the 'standard' OpenGL functions */
  if ((local_func = GetProcAddress(hm, lpszProc)) != NULL) {
    TRACE(" found function in 'standard' OpenGL functions (%p)\n", local_func);
    return local_func;
  }

  /* After that, look at the extensions defined in the Linux OpenGL library */
  if ((local_func = glXGetProcAddressARB(lpszProc)) == NULL) {
    char buf[256];
    void *ret = NULL;
    
    /* Remove the 3 last letters (EXT, ARB, ...).
       
       I know that some extensions have more than 3 letters (MESA, NV,
       INTEL, ...), but this is only a stop-gap measure to fix buggy
       OpenGL drivers (moreover, it is only useful for old 1.0 apps
       that query the glBindTextureEXT extension).
    */
    strncpy(buf, lpszProc, strlen(lpszProc) - 3);
    buf[strlen(lpszProc) - 3] = '\0';
    TRACE(" extension not found in the Linux OpenGL library, checking against libGL bug with %s..\n", buf);
    
    ret = GetProcAddress(hm, buf);
    if (ret != NULL) {
      TRACE(" found function in main OpenGL library (%p) !\n", ret);
    }

    return ret;
  } else {
    OpenGL_extension  ext;
    OpenGL_extension *ret;

    ext.name = (char *) lpszProc;
    ret = (OpenGL_extension *) bsearch(&ext, extension_registry,
				       extension_registry_size, sizeof(OpenGL_extension), compar);

    if (ret != NULL) {
      TRACE(" returning function  (%p)\n", ret->func);
      *(ret->func_ptr) = local_func;

      return ret->func;
    } else {
      ERR("Extension defined in the OpenGL library but NOT in opengl_ext.c... Please report (lionel.ulmer@free.fr) !\n");
      return NULL;
    }
  }
}

/***********************************************************************
 *		wglMakeCurrent
 */
BOOL WINAPI wglMakeCurrent(HDC hdc,
			   HGLRC hglrc) {
  BOOL ret;

  TRACE("(%08x,%p)\n", hdc, hglrc);
  
  if (hglrc == NULL) {
    ENTER_GL();
    ret = glXMakeCurrent(display,
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
	ctx->ctx = glXCreateContext(display, ctx->vis, NULL, True);
	LEAVE_GL();
	TRACE(" created a delayed OpenGL context (%p)\n", ctx->ctx);
      }
      
      ENTER_GL();
      ret = glXMakeCurrent(display,
			   physDev->drawable,
			   ctx->ctx);
      LEAVE_GL();
    }
  }
  TRACE(" returning %s\n", (ret ? "True" : "False"));
  return ret;
}

/***********************************************************************
 *		wglRealizeLayerPalette
 */
BOOL WINAPI wglRealizeLayerPalette(HDC hdc,
				   int iLayerPlane,
				   BOOL bRealize) {
  FIXME("()\n");

  return FALSE;
}

/***********************************************************************
 *		wglSetLayerPaletteEntries
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
 *		wglShareLists
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
      org->ctx = glXCreateContext(display, org->vis, NULL, True);
      LEAVE_GL();
      TRACE(" created a delayed OpenGL context (%p) for Wine context %p\n", org->ctx, org);
    }

    ENTER_GL();
    /* Create the destination context with display lists shared */
    dest->ctx = glXCreateContext(display, dest->vis, org->ctx, True);
    LEAVE_GL();
    TRACE(" created a delayed OpenGL context (%p) for Wine context %p sharing lists with OpenGL ctx %p\n", dest->ctx, dest, org->ctx);
  }
  
  return TRUE;
}

/***********************************************************************
 *		wglSwapLayerBuffers
 */
BOOL WINAPI wglSwapLayerBuffers(HDC hdc,
				UINT fuPlanes) {
  FIXME("(): stub !\n");

  return FALSE;
}

/***********************************************************************
 *		wglUseFontBitmapsA
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
  
  return TRUE;
}
 
/***********************************************************************
 *		wglUseFontOutlinesA
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
DECL_GLOBAL_CONSTRUCTOR(OpenGL_create_default_context) {
  int num;
  XVisualInfo template;
  XVisualInfo *vis;
  GLXContext cx;

  ENTER_GL();
  template.visualid = XVisualIDFromVisual(visual);
  vis = XGetVisualInfo(display, VisualIDMask, &template, &num);
  cx=glXCreateContext(display, vis, 0, GL_TRUE);
  glXMakeCurrent(display, X11DRV_GetXRootWindow(), cx);
  LEAVE_GL();

  context_array = NULL;
}
