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
    BOOL WINAPI  (*p_wglUseFontBitmapsA)(HDC hdc, DWORD first, DWORD count, DWORD listBase);
    BOOL WINAPI  (*p_wglUseFontBitmapsW)(HDC hdc, DWORD first, DWORD count, DWORD listBase);

    void WINAPI  (*p_wglGetIntegerv)(GLenum pname, GLint* params);
} wine_wgl_t;

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

static void* (*p_glXGetProcAddressARB)(const GLubyte *);

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

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc,
			       DWORD first,
			       DWORD count,
			       DWORD listBase)
{
    TRACE("(%p, %ld, %ld, %ld)\n", hdc, first, count, listBase);
    return wine_wgl.p_wglUseFontBitmapsA(hdc, first, count, listBase);
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc,
			       DWORD first,
			       DWORD count,
			       DWORD listBase)
{
    TRACE("(%p, %ld, %ld, %ld)\n", hdc, first, count, listBase);
    return wine_wgl.p_wglUseFontBitmapsW(hdc, first, count, listBase);
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
  wine_wgl.p_wglUseFontBitmapsA = (void*)GetProcAddress(mod, "wglUseFontBitmapsA");
  wine_wgl.p_wglUseFontBitmapsW = (void*)GetProcAddress(mod, "wglUseFontBitmapsW");

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
