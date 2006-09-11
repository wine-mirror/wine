/* Window-specific OpenGL functions implementation.
 *
 * Copyright (c) 1999 Lionel Ulmer
 * Copyright (c) 2005 Raphael Junqueira
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "winternl.h"
#include "winnt.h"

#include "wgl_ext.h"
#include "opengl_ext.h"
#ifdef HAVE_GL_GLU_H
#undef far
#undef near
#include <GL/glu.h>
#endif
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(opengl);

typedef struct wine_glx_s {
  unsigned     version;
  /** SGIX / 1.3 */
  GLXFBConfig* (*p_glXChooseFBConfig) (Display *dpy, int screen, const int *attrib_list, int *nelements);
  int          (*p_glXGetFBConfigAttrib) (Display *dpy, GLXFBConfig config, int attribute, int *value);
  XVisualInfo* (*p_glXGetVisualFromFBConfig) (Display *dpy, GLXFBConfig config); 
  /** 1.3 */
  GLXFBConfig* (*p_glXGetFBConfigs) (Display *dpy, int screen, int *nelements);
  void         (*p_glXQueryDrawable) (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
  Bool         (*p_glXMakeContextCurrent) (Display *, GLXDrawable, GLXDrawable, GLXContext);
} wine_glx_t;

typedef struct wine_wgl_s {
    HGLRC WINAPI (*p_wglCreateContext)(HDC hdc);
    BOOL WINAPI  (*p_wglDeleteContext)(HGLRC hglrc);
    HGLRC WINAPI (*p_wglGetCurrentContext)(void);
    HDC WINAPI   (*p_wglGetCurrentDC)(void);
    HDC WINAPI   (*p_wglGetCurrentReadDCARB)(void);
    PROC WINAPI  (*p_wglGetProcAddress)(LPCSTR  lpszProc);
    BOOL WINAPI  (*p_wglMakeCurrent)(HDC hdc, HGLRC hglrc);
    BOOL WINAPI  (*p_wglMakeContextCurrentARB)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc); 
    BOOL WINAPI  (*p_wglShareLists)(HGLRC hglrc1, HGLRC hglrc2);

    void WINAPI  (*p_wglGetIntegerv)(GLenum pname, GLint* params);
} wine_wgl_t;

/** global glx object */
static wine_glx_t wine_glx;
/** global wgl object */
static wine_wgl_t wine_wgl;

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,         /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,        /* get current drawable for a DC */
    X11DRV_GET_FONT,            /* get current X font for a DC */
    X11DRV_SET_DRAWABLE,        /* set current drawable for a DC */
    X11DRV_START_EXPOSURES,     /* start graphics exposures */
    X11DRV_END_EXPOSURES,       /* end graphics exposures */
    X11DRV_GET_DCE,             /* get the DCE pointer */
    X11DRV_SET_DCE,             /* set the DCE pointer */
    X11DRV_GET_GLX_DRAWABLE,    /* get current glx drawable for a DC */
    X11DRV_SYNC_PIXMAP          /* sync the dibsection to its pixmap */
};

void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;

static GLXContext default_cx = NULL;
static Display *default_display;  /* display to use for default context */

static HMODULE opengl32_handle;

static glXGetProcAddressARB_t p_glXGetProcAddressARB = NULL;

static char  internal_gl_disabled_extensions[512];
static char* internal_gl_extensions = NULL;

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

void enter_gl(void)
{
    Wine_GLContext *curctx = (Wine_GLContext *) NtCurrentTeb()->glContext;
    
    if (curctx && curctx->do_escape)
    {
        enum x11drv_escape_codes escape = X11DRV_SYNC_PIXMAP;
        ExtEscape(curctx->hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape, 0, NULL);
    }

    wine_tsx11_lock_ptr();
    return;
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

/* retrieve the X font to use on a given DC */
inline static Font get_font( HDC hdc )
{
    Font font;
    enum x11drv_escape_codes escape = X11DRV_GET_FONT;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(font), (LPSTR)&font )) font = 0;
    return font;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one. It returns the index (zero-based)
 * into the GLX FB config table and it returns the number of supported WGL formats in fmt_count.
 */
BOOL ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, int *fmt_index, int *fmt_count)
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
  if(iPixelFormat == 1) visualid = (VisualID)GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );

  /* As mentioned in various parts of the code only the format of the main visual can be used for onscreen rendering.
   * Next to this format there are also so called offscreen rendering formats (used for pbuffers) which can be supported
   * because they don't need a visual. Below we use glXGetFBConfigs instead of glXChooseFBConfig to enumerate the fb configurations
   * bas this call lists both types of formats instead of only onscreen ones. */
  cfgs = wine_glx.p_glXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
  if (NULL == cfgs || 0 == nCfgs) {
    ERR("glXChooseFBConfig returns NULL\n");
    if(cfgs != NULL) XFree(cfgs);
    return FALSE;
  }

  /* Find the requested offscreen format and count the number of offscreen formats */
  for(i=0; i<nCfgs; i++) {
    wine_glx.p_glXGetFBConfigAttrib(display, cfgs[i], GLX_VISUAL_ID, &tmp_vis_id);
    wine_glx.p_glXGetFBConfigAttrib(display, cfgs[i], GLX_FBCONFIG_ID, &tmp_fmt_id);

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

  return res;
}

/***********************************************************************
 *		wglCreateContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateContext(HDC hdc)
{
    TRACE("(%p)\n", hdc);
    return wine_wgl.p_wglCreateContext(hdc);
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
    TRACE("(%p)\n", hglrc);
    return wine_wgl.p_wglDeleteContext(hglrc);
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
    TRACE("\n");
    return wine_wgl.p_wglGetCurrentContext();
}

/***********************************************************************
 *		wglGetCurrentDC (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentDC(void) {
    TRACE("\n");
    return wine_wgl.p_wglGetCurrentDC();
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
  return strcmp(((const OpenGL_extension *) elt_a)->name,
		((const OpenGL_extension *) elt_b)->name);
}

PROC WINAPI wglGetProcAddress(LPCSTR  lpszProc) {
  void *local_func;
  OpenGL_extension  ext;
  const OpenGL_extension *ext_ret;

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
  ext.name = lpszProc;
  ext_ret = (const OpenGL_extension *) bsearch(&ext, extension_registry,
					 extension_registry_size, sizeof(OpenGL_extension), compar);

  /* If nothing was found, we are looking for a WGL extension */
  if (ext_ret == NULL) {
    return wine_wgl.p_wglGetProcAddress(lpszProc);
  } else { /* We are looking for an OpenGL extension */
    const char *glx_name = ext_ret->glx_name ? ext_ret->glx_name : ext_ret->name;
    ENTER_GL();
    local_func = p_glXGetProcAddressARB( (const GLubyte*)glx_name);
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
      memcpy(buf, glx_name, strlen(glx_name) - 3);
      buf[strlen(glx_name) - 3] = '\0';
      TRACE(" extension not found in the Linux OpenGL library, checking against libGL bug with %s..\n", buf);

      ret = GetProcAddress(opengl32_handle, buf);
      if (ret != NULL) {
	TRACE(" found function in main OpenGL library (%p) !\n", ret);
      } else {
	WARN("Did not find function %s (%s) in your OpenGL library !\n", lpszProc, glx_name);
      }

      return ret;
    } else {
      TRACE(" returning function  (%p)\n", ext_ret->func);
      extension_funcs[ext_ret - extension_registry] = local_func;

      return ext_ret->func;
    }
  }
}

/***********************************************************************
 *		wglMakeCurrent (OPENGL32.@)
 */
BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc) {
    TRACE("hdc: (%p), hglrc: (%p)\n", hdc, hglrc);
    return wine_wgl.p_wglMakeCurrent(hdc, hglrc);
}

/***********************************************************************
 *		wglMakeContextCurrentARB (OPENGL32.@)
 */
BOOL WINAPI wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) 
{
    TRACE("hDrawDC: (%p), hReadDC: (%p), hglrc: (%p)\n", hDrawDC, hReadDC, hglrc);
    return wine_wgl.p_wglMakeContextCurrentARB(hDrawDC, hReadDC, hglrc);
}

/***********************************************************************
 *		wglGetCurrentReadDCARB (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentReadDCARB(void) 
{
    TRACE("\n");
    return wine_wgl.p_wglGetCurrentReadDCARB();
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
BOOL WINAPI wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    TRACE("(%p, %p)\n", hglrc1, hglrc2);
    return wine_wgl.p_wglShareLists(hglrc1, hglrc2);
}

/***********************************************************************
 *		wglSwapLayerBuffers (OPENGL32.@)
 */
BOOL WINAPI wglSwapLayerBuffers(HDC hdc,
				UINT fuPlanes) {
  TRACE_(opengl)("(%p, %08x)\n", hdc, fuPlanes);

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
				       DWORD (WINAPI *GetGlyphOutline_ptr)(HDC,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*))
{
    /* We are running using client-side rendering fonts... */
    GLYPHMETRICS gm;
    unsigned int glyph;
    int size = 0;
    void *bitmap = NULL, *gl_bitmap = NULL;
    int org_alignment;

    ENTER_GL();
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &org_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    LEAVE_GL();

    for (glyph = first; glyph < first + count; glyph++) {
	unsigned int needed_size = GetGlyphOutline_ptr(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, NULL);
	int height, width_int;

	TRACE("Glyph : %3d / List : %ld\n", glyph, listBase);
	if (needed_size == GDI_ERROR) {
	    TRACE("  - needed size : %d (GDI_ERROR)\n", needed_size);
	    goto error;
	} else {
	    TRACE("  - needed size : %d\n", needed_size);
	}

	if (needed_size > size) {
	    size = needed_size;
            HeapFree(GetProcessHeap(), 0, bitmap);
            HeapFree(GetProcessHeap(), 0, gl_bitmap);
	    bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	    gl_bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	}
	if (GetGlyphOutline_ptr(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, NULL) == GDI_ERROR) goto error;
	if (TRACE_ON(opengl)) {
	    unsigned int height, width, bitmask;
	    unsigned char *bitmap_ = (unsigned char *) bitmap;
	    
	    TRACE("  - bbox : %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
	    TRACE("  - origin : (%ld , %ld)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
	    TRACE("  - increment : %d - %d\n", gm.gmCellIncX, gm.gmCellIncY);
	    if (needed_size != 0) {
		TRACE("  - bitmap :\n");
		for (height = 0; height < gm.gmBlackBoxY; height++) {
		    TRACE("      ");
		    for (width = 0, bitmask = 0x80; width < gm.gmBlackBoxX; width++, bitmask >>= 1) {
			if (bitmask == 0) {
			    bitmap_ += 1;
			    bitmask = 0x80;
			}
			if (*bitmap_ & bitmask)
			    TRACE("*");
			else
			    TRACE(" ");
		    }
		    bitmap_ += (4 - ((UINT_PTR)bitmap_ & 0x03));
		    TRACE("\n");
		}
	    }
	}
	
	/* In OpenGL, the bitmap is drawn from the bottom to the top... So we need to invert the
	 * glyph for it to be drawn properly.
	 */
	if (needed_size != 0) {
	    width_int = (gm.gmBlackBoxX + 31) / 32;
	    for (height = 0; height < gm.gmBlackBoxY; height++) {
		int width;
		for (width = 0; width < width_int; width++) {
		    ((int *) gl_bitmap)[(gm.gmBlackBoxY - height - 1) * width_int + width] =
			((int *) bitmap)[height * width_int + width];
		}
	    }
	}
	
	ENTER_GL();
	glNewList(listBase++, GL_COMPILE);
	if (needed_size != 0) {
	    glBitmap(gm.gmBlackBoxX, gm.gmBlackBoxY,
		     0 - (int) gm.gmptGlyphOrigin.x, (int) gm.gmBlackBoxY - (int) gm.gmptGlyphOrigin.y,
		     gm.gmCellIncX, gm.gmCellIncY,
		     gl_bitmap);
	} else {
	    /* This is the case of 'empty' glyphs like the space character */
	    glBitmap(0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL);
	}
	glEndList();
	LEAVE_GL();
    }
    
    ENTER_GL();
    glPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
    LEAVE_GL();
    
    HeapFree(GetProcessHeap(), 0, bitmap);
    HeapFree(GetProcessHeap(), 0, gl_bitmap);
    return TRUE;

  error:
    ENTER_GL();
    glPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
    LEAVE_GL();

    HeapFree(GetProcessHeap(), 0, bitmap);
    HeapFree(GetProcessHeap(), 0, gl_bitmap);
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

#ifdef HAVE_GL_GLU_H

static void fixed_to_double(POINTFX fixed, UINT em_size, GLdouble vertex[3])
{
    vertex[0] = (fixed.x.value + (GLdouble)fixed.x.fract / (1 << 16)) / em_size;  
    vertex[1] = (fixed.y.value + (GLdouble)fixed.y.fract / (1 << 16)) / em_size;  
    vertex[2] = 0.0;
}

static void tess_callback_vertex(GLvoid *vertex)
{
    GLdouble *dbl = vertex;
    TRACE("%f, %f, %f\n", dbl[0], dbl[1], dbl[2]);
    glVertex3dv(vertex);
}

static void tess_callback_begin(GLenum which)
{
    TRACE("%d\n", which);
    glBegin(which);
}

static void tess_callback_end(void)
{
    TRACE("\n");
    glEnd();
}

/***********************************************************************
 *		wglUseFontOutlines_common
 */
BOOL WINAPI wglUseFontOutlines_common(HDC hdc,
                                      DWORD first,
                                      DWORD count,
                                      DWORD listBase,
                                      FLOAT deviation,
                                      FLOAT extrusion,
                                      int format,
                                      LPGLYPHMETRICSFLOAT lpgmf,
                                      BOOL unicode)
{
    UINT glyph;
    const MAT2 identity = {{0,1},{0,0},{0,0},{0,1}};
    GLUtesselator *tess;
    LOGFONTW lf;
    HFONT old_font, unscaled_font;
    UINT em_size = 1024;
    RECT rc;

    TRACE("(%p, %ld, %ld, %ld, %f, %f, %d, %p, %s)\n", hdc, first, count,
          listBase, deviation, extrusion, format, lpgmf, unicode ? "W" : "A");


    ENTER_GL();
    tess = gluNewTess();
    if(tess)
    {
        gluTessCallback(tess, GLU_TESS_VERTEX, (_GLUfuncptr)tess_callback_vertex);
        gluTessCallback(tess, GLU_TESS_BEGIN, (_GLUfuncptr)tess_callback_begin);
        gluTessCallback(tess, GLU_TESS_END, tess_callback_end);
    }
    LEAVE_GL();

    if(!tess) return FALSE;

    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);
    rc.left = rc.right = rc.bottom = 0;
    rc.top = em_size;
    DPtoLP(hdc, (POINT*)&rc, 2);
    lf.lfHeight = -abs(rc.top - rc.bottom);
    lf.lfOrientation = lf.lfEscapement = 0;
    unscaled_font = CreateFontIndirectW(&lf);
    old_font = SelectObject(hdc, unscaled_font);

    for (glyph = first; glyph < first + count; glyph++)
    {
        DWORD needed;
        GLYPHMETRICS gm;
        BYTE *buf;
        TTPOLYGONHEADER *pph;
        TTPOLYCURVE *ppc;
        GLdouble *vertices;

        if(unicode)
            needed = GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);
        else
            needed = GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);

        if(needed == GDI_ERROR)
            goto error;

        buf = HeapAlloc(GetProcessHeap(), 0, needed);
        vertices = HeapAlloc(GetProcessHeap(), 0, needed / sizeof(POINTFX) * 3 * sizeof(GLdouble));

        if(unicode)
            GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, needed, buf, &identity);
        else
            GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, needed, buf, &identity);

        TRACE("glyph %d\n", glyph);

        if(lpgmf)
        {
            lpgmf->gmfBlackBoxX = (float)gm.gmBlackBoxX / em_size;
            lpgmf->gmfBlackBoxY = (float)gm.gmBlackBoxY / em_size;
            lpgmf->gmfptGlyphOrigin.x = (float)gm.gmptGlyphOrigin.x / em_size;
            lpgmf->gmfptGlyphOrigin.y = (float)gm.gmptGlyphOrigin.y / em_size;
            lpgmf->gmfCellIncX = (float)gm.gmCellIncX / em_size;
            lpgmf->gmfCellIncY = (float)gm.gmCellIncY / em_size;

            TRACE("%fx%f at %f,%f inc %f,%f\n", lpgmf->gmfBlackBoxX, lpgmf->gmfBlackBoxY,
                  lpgmf->gmfptGlyphOrigin.x, lpgmf->gmfptGlyphOrigin.y, lpgmf->gmfCellIncX, lpgmf->gmfCellIncY); 
            lpgmf++;
        }

	ENTER_GL();
	glNewList(listBase++, GL_COMPILE);
        gluTessBeginPolygon(tess, NULL);

        pph = (TTPOLYGONHEADER*)buf;
        while((BYTE*)pph < buf + needed)
        {
            TRACE("\tstart %d, %d\n", pph->pfxStart.x.value, pph->pfxStart.y.value);

            gluTessBeginContour(tess);

            fixed_to_double(pph->pfxStart, em_size, vertices);
            gluTessVertex(tess, vertices, vertices);
            vertices += 3;

            ppc = (TTPOLYCURVE*)((char*)pph + sizeof(*pph));
            while((char*)ppc < (char*)pph + pph->cb)
            {
                int i;

                switch(ppc->wType) {
                case TT_PRIM_LINE:
                    for(i = 0; i < ppc->cpfx; i++)
                    {
                        TRACE("\t\tline to %d, %d\n", ppc->apfx[i].x.value, ppc->apfx[i].y.value);
                        fixed_to_double(ppc->apfx[i], em_size, vertices); 
                        gluTessVertex(tess, vertices, vertices);
                        vertices += 3;
                    }
                    break;

                case TT_PRIM_QSPLINE:
                    for(i = 0; i < ppc->cpfx/2; i++)
                    {
                        /* FIXME just connecting the control points for now */
                        TRACE("\t\tcurve  %d,%d %d,%d\n",
                              ppc->apfx[i * 2].x.value,     ppc->apfx[i * 3].y.value,
                              ppc->apfx[i * 2 + 1].x.value, ppc->apfx[i * 3 + 1].y.value);
                        fixed_to_double(ppc->apfx[i * 2], em_size, vertices); 
                        gluTessVertex(tess, vertices, vertices);
                        vertices += 3;
                        fixed_to_double(ppc->apfx[i * 2 + 1], em_size, vertices); 
                        gluTessVertex(tess, vertices, vertices);
                        vertices += 3;
                    }
                    break;
                default:
                    ERR("\t\tcurve type = %d\n", ppc->wType);
                    gluTessEndContour(tess);
                    goto error_in_list;
                }

                ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
                                     (ppc->cpfx - 1) * sizeof(POINTFX));
            }
            gluTessEndContour(tess);
            pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
        }

error_in_list:
        gluTessEndPolygon(tess);
        glTranslated((GLdouble)gm.gmCellIncX / em_size, (GLdouble)gm.gmCellIncY / em_size, 0.0);
        glEndList();
        LEAVE_GL();
        HeapFree(GetProcessHeap(), 0, buf);
        HeapFree(GetProcessHeap(), 0, vertices);
    }

 error:
    DeleteObject(SelectObject(hdc, old_font));
    gluDeleteTess(tess);
    return TRUE;

}

#else /* HAVE_GL_GLU_H */

BOOL WINAPI wglUseFontOutlines_common(HDC hdc,
                                      DWORD first,
                                      DWORD count,
                                      DWORD listBase,
                                      FLOAT deviation,
                                      FLOAT extrusion,
                                      int format,
                                      LPGLYPHMETRICSFLOAT lpgmf,
                                      BOOL unicode)
{
    FIXME("Unable to compile in wglUseFontOutlines support without GL/glu.h\n");
    return FALSE;
}

#endif /* HAVE_GL_GLU_H */

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
				LPGLYPHMETRICSFLOAT lpgmf)
{
    return wglUseFontOutlines_common(hdc, first, count, listBase, deviation, extrusion, format, lpgmf, FALSE);
}

/***********************************************************************
 *		wglUseFontOutlinesW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontOutlinesW(HDC hdc,
				DWORD first,
				DWORD count,
				DWORD listBase,
				FLOAT deviation,
				FLOAT extrusion,
				int format,
				LPGLYPHMETRICSFLOAT lpgmf)
{
    return wglUseFontOutlines_common(hdc, first, count, listBase, deviation, extrusion, format, lpgmf, TRUE);
}

const GLubyte * internal_glGetString(GLenum name) {
  const char* GL_Extensions = NULL;
  
  if (GL_EXTENSIONS != name) {
    return glGetString(name);
  }

  if (NULL == internal_gl_extensions) {
    GL_Extensions = (const char *) glGetString(GL_EXTENSIONS);

    TRACE("GL_EXTENSIONS reported:\n");  
    if (NULL == GL_Extensions) {
      ERR("GL_EXTENSIONS returns NULL\n");      
      return NULL;
    } else {
      size_t len = strlen(GL_Extensions);
      internal_gl_extensions = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 2);

      while (*GL_Extensions != 0x00) {
	const char* Start = GL_Extensions;
	char        ThisExtn[256];
	 
	memset(ThisExtn, 0x00, sizeof(ThisExtn));
	while (*GL_Extensions != ' ' && *GL_Extensions != 0x00) {
	  GL_Extensions++;
	}
	memcpy(ThisExtn, Start, (GL_Extensions - Start));
	TRACE("- %s:", ThisExtn);
	
	/* test if supported API is disabled by config */
	if (NULL == strstr(internal_gl_disabled_extensions, ThisExtn)) {
	  strcat(internal_gl_extensions, " ");
	  strcat(internal_gl_extensions, ThisExtn);
	  TRACE(" active\n");
	} else {
	  TRACE(" deactived (by config)\n");
	}

	if (*GL_Extensions == ' ') GL_Extensions++;
      }
    }
  }
  return (const GLubyte *) internal_gl_extensions;
}

void internal_glGetIntegerv(GLenum pname, GLint* params) {
  TRACE("pname: 0x%x, params %p\n", pname, params);
  glGetIntegerv(pname, params);
  /* A few parameters like GL_DEPTH_BITS differ between WGL and GLX, the wglGetIntegerv helper function handles those */
  wine_wgl.p_wglGetIntegerv(pname, params);
}


/* No need to load any other libraries as according to the ABI, libGL should be self-sufficient and
   include all dependencies
*/
#ifndef SONAME_LIBGL
#define SONAME_LIBGL "libGL.so"
#endif

static void wgl_initialize_glx(Display *display, int screen, glXGetProcAddressARB_t proc) 
{
  const char *server_glx_version = glXQueryServerString(display, screen, GLX_VERSION);
  const char *client_glx_version = glXGetClientString(display, GLX_VERSION);
  const char *glx_extensions = NULL;
  BOOL glx_direct = glXIsDirect(display, default_cx);

  memset(&wine_glx, 0, sizeof(wine_glx));

  /* In case of GLX you have direct and indirect rendering. Most of the time direct rendering is used
   * as in general only that is hardware accelerated. In some cases like in case of remote X indirect
   * rendering is used.
   *
   * The main problem for our OpenGL code is that we need certain GLX calls but their presence
   * depends on the reported GLX client / server version and on the client / server extension list.
   * Those don't have to be the same.
   *
   * In general the server GLX information should be used in case of indirect rendering. When direct
   * rendering is used, the OpenGL client library is responsible for which GLX calls are available.
   * Nvidia's OpenGL drivers are the best in terms of GLX features. At the moment of writing their
   * 8762 drivers support 1.3 for the server and 1.4 for the client and they support lots of extensions.
   * Unfortunately it is much more complicated for Mesa/DRI-based drivers and ATI's drivers.
   * Both sets of drivers report a server version of 1.2 and the client version can be 1.3 or 1.4.
   * Further in case of at least ATI's drivers one crucial extension needed for our pixel format code
   * is only available in the list of server extensions and not in the client list.
   *
   * The version checks below try to take into account the comments from above.
   */

  TRACE("Server GLX version: %s\n", server_glx_version);
  TRACE("Client GLX version: %s\n", client_glx_version);
  TRACE("Direct rendering eanbled: %s\n", glx_direct ? "True" : "False");

  /* Based on the default opengl context we decide whether direct or indirect rendering is used.
   * In case of indirect rendering we check if the GLX version of the server is 1.2 and else
   * the client version is checked.
   */
  if ( (!glx_direct && !strcmp("1.2", server_glx_version)) || (glx_direct && !strcmp("1.2", client_glx_version)) ) {
    wine_glx.version = 2;
  } else {
    wine_glx.version = 3;
  }

  /* Depending on the use of direct or indirect rendering we need either the list of extensions
   * exported by the client or by the server.
   */
  if(glx_direct)
    glx_extensions = glXGetClientString(display, GLX_EXTENSIONS);
  else
    glx_extensions = glXQueryServerString(display, screen, GLX_EXTENSIONS);

  if (2 < wine_glx.version) {
    wine_glx.p_glXChooseFBConfig = proc( (const GLubyte *) "glXChooseFBConfig");
    wine_glx.p_glXGetFBConfigAttrib = proc( (const GLubyte *) "glXGetFBConfigAttrib");
    wine_glx.p_glXGetVisualFromFBConfig = proc( (const GLubyte *) "glXGetVisualFromFBConfig");

    /*wine_glx.p_glXGetFBConfigs = proc( (const GLubyte *) "glXGetFBConfigs");*/
  } else {
    if (NULL != strstr(glx_extensions, "GLX_SGIX_fbconfig")) {
      wine_glx.p_glXChooseFBConfig = proc( (const GLubyte *) "glXChooseFBConfigSGIX");
      wine_glx.p_glXGetFBConfigAttrib = proc( (const GLubyte *) "glXGetFBConfigAttribSGIX");
      wine_glx.p_glXGetVisualFromFBConfig = proc( (const GLubyte *) "glXGetVisualFromFBConfigSGIX");
    } else {
      ERR(" glx_version as %s and GLX_SGIX_fbconfig extension is unsupported. Expect problems.\n", client_glx_version);
    }
  }

  /* The mesa libGL client library seems to forward glXQueryDrawable to the Xserver, so only
   * enable this function when the Xserver understand GLX 1.3 or newer
   */  
  if (!strcmp("1.2", server_glx_version))
    wine_glx.p_glXQueryDrawable = NULL;
  else
    wine_glx.p_glXQueryDrawable = proc( (const GLubyte *) "glXQueryDrawable");
  
  /** try anyway to retrieve that calls, maybe they works using glx client tricks */
  wine_glx.p_glXGetFBConfigs = proc( (const GLubyte *) "glXGetFBConfigs");
  wine_glx.p_glXMakeContextCurrent = proc( (const GLubyte *) "glXMakeContextCurrent");
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
  HMODULE mod = GetModuleHandleA( "winex11.drv" );
  void *opengl_handle;
  DWORD size = sizeof(internal_gl_disabled_extensions);
  HKEY hkey = 0;

  if (!root || !mod)
  {
      ERR("X11DRV not loaded. Cannot create default context.\n");
      return FALSE;
  }

  wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
  wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );

  /* Load WGL function pointers from winex11.drv */
  wine_wgl.p_wglCreateContext = (void *)GetProcAddress(mod, "wglCreateContext");
  wine_wgl.p_wglDeleteContext = (void *)GetProcAddress(mod, "wglDeleteContext");
  wine_wgl.p_wglGetCurrentContext = (void *)GetProcAddress(mod, "wglGetCurrentContext");
  wine_wgl.p_wglGetCurrentDC = (void *)GetProcAddress(mod, "wglGetCurrentDC");
  wine_wgl.p_wglGetCurrentReadDCARB = (void *)GetProcAddress(mod, "wglGetCurrentReadDCARB");
  wine_wgl.p_wglGetProcAddress = (void *)GetProcAddress(mod, "wglGetProcAddress");
  wine_wgl.p_wglMakeCurrent = (void *)GetProcAddress(mod, "wglMakeCurrent");
  wine_wgl.p_wglMakeContextCurrentARB = (void *)GetProcAddress(mod, "wglMakeContextCurrentARB");
  wine_wgl.p_wglShareLists = (void *)GetProcAddress(mod, "wglShareLists");
  /* Interal WGL function */
  wine_wgl.p_wglGetIntegerv = (void *)GetProcAddress(mod, "wglGetIntegerv");

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

  internal_gl_disabled_extensions[0] = 0;
  if (!RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\OpenGL", &hkey)) {
    if (!RegQueryValueExA( hkey, "DisabledExtensions", 0, NULL, (LPBYTE)internal_gl_disabled_extensions, &size)) {
      TRACE("found DisabledExtensions=\"%s\"\n", internal_gl_disabled_extensions);
    }
    RegCloseKey(hkey);
  }

  if (default_cx == NULL) {
    ERR("Could not create default context.\n");
  }
  else
  {
    /* After context initialize also the list of supported WGL extensions. */
    wgl_initialize_glx(default_display, DefaultScreen(default_display), p_glXGetProcAddressARB);
  }
  return TRUE;
}


/**********************************************************************/

static void process_detach(void)
{
  glXDestroyContext(default_display, default_cx);

  HeapFree(GetProcessHeap(), 0, internal_gl_extensions);
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
