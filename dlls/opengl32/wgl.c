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
    PROC WINAPI  (*p_wglGetProcAddress)(LPCSTR  lpszProc);

    void WINAPI  (*p_wglGetIntegerv)(GLenum pname, GLint* params);
    void WINAPI  (*p_wglFinish)(void);
    void WINAPI  (*p_wglFlush)(void);
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

static HMODULE opengl32_handle;

static char  internal_gl_disabled_extensions[512];
static char* internal_gl_extensions = NULL;

typedef struct wine_glcontext {
  HDC hdc;
  XVisualInfo *vis;
  GLXFBConfig fb_conf;
  GLXContext ctx;
  BOOL do_escape;
  /* ... more stuff here */
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

const GLubyte * WINAPI wine_glGetString( GLenum name );

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

static int compar(const void *elt_a, const void *elt_b) {
  return strcmp(((const OpenGL_extension *) elt_a)->name,
		((const OpenGL_extension *) elt_b)->name);
}

/* Check if a GL extension is supported */
static BOOL is_extension_supported(const char* extension)
{
    const char *gl_ext_string = (const char*)wine_glGetString(GL_EXTENSIONS);

    TRACE("Checking for extension '%s'\n", extension);

    if(!gl_ext_string) {
        ERR("No OpenGL extensions found, check if your OpenGL setup is correct!\n");
        return FALSE;
    }

    /* We use the GetProcAddress function from the display driver to retrieve function pointers
     * for OpenGL and WGL extensions. In case of winex11.drv the OpenGL extension lookup is done
     * using glXGetProcAddress. This function is quite unreliable in the sense that its specs don't
     * require the function to return NULL when a extension isn't found. For this reason we check
     * if the OpenGL extension required for the function we are looking up is supported. */

    /* Check if the extension is part of the GL extension string to see if it is supported. */
    if(strstr(gl_ext_string, extension) != NULL)
        return TRUE;

    /* In general an OpenGL function starts as an ARB/EXT extension and at some stage
     * it becomes part of the core OpenGL library and can be reached without the ARB/EXT
     * suffix as well. In the extension table, these functions contain GL_VERSION_major_minor.
     * Check if we are searching for a core GL function */
    if(strncmp(extension, "GL_VERSION_", 11) == 0)
    {
        const GLubyte *gl_version = glGetString(GL_VERSION);
        const char *version = extension + 11; /* Move past 'GL_VERSION_' */

        if(!gl_version) {
            ERR("Error no OpenGL version found,\n");
            return FALSE;
        }

        /* Compare the major/minor version numbers of the native OpenGL library and what is required by the function.
         * The gl_version string is guaranteed to have at least a major/minor and sometimes it has a release number as well. */
        if( (gl_version[0] >= version[0]) || ((gl_version[0] == version[0]) && (gl_version[2] >= version[2])) ) {
            return TRUE;
        }
        WARN("The function requires OpenGL version '%c.%c' while your drivers only provide '%c.%c'\n", version[0], version[2], gl_version[0], gl_version[2]);
    }

    return FALSE;
}

/***********************************************************************
 *		wglGetProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetProcAddress(LPCSTR  lpszProc) {
  void *local_func;
  OpenGL_extension  ext;
  const OpenGL_extension *ext_ret;

  TRACE("(%s)\n", lpszProc);

  if(lpszProc == NULL)
    return NULL;

  /* First, look if it's not already defined in the 'standard' OpenGL functions */
  if ((local_func = GetProcAddress(opengl32_handle, lpszProc)) != NULL) {
    TRACE(" found function in 'standard' OpenGL functions (%p)\n", local_func);
    return local_func;
  }

  /* After that, search in the thunks to find the real name of the extension */
  ext.name = lpszProc;
  ext_ret = (const OpenGL_extension *) bsearch(&ext, extension_registry,
					 extension_registry_size, sizeof(OpenGL_extension), compar);

  /* If nothing was found, we are looking for a WGL extension or an unknown GL extension. */
  if (ext_ret == NULL) {
    /* If the function name starts with a w it is a WGL extension */
    if(lpszProc[0] == 'w')
      return wine_wgl.p_wglGetProcAddress(lpszProc);

    /* We are dealing with an unknown GL extension. */
    WARN("Extension '%s' not defined in opengl32.dll's function table!\n", lpszProc);
    return NULL;
  } else { /* We are looking for an OpenGL extension */

    /* Check if the GL extension required by the function is available */
    if(!is_extension_supported(ext_ret->extension)) {
        WARN("Extension '%s' required by function '%s' not supported!\n", ext_ret->extension, lpszProc);
    }

    local_func = wine_wgl.p_wglGetProcAddress(ext_ret->name);

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
      memcpy(buf, ext_ret->name, strlen(ext_ret->name) - 3);
      buf[strlen(ext_ret->name) - 3] = '\0';
      TRACE(" extension not found in the Linux OpenGL library, checking against libGL bug with %s..\n", buf);

      ret = GetProcAddress(opengl32_handle, buf);
      if (ret != NULL) {
        TRACE(" found function in main OpenGL library (%p) !\n", ret);
      } else {
        WARN("Did not find function %s (%s) in your OpenGL library !\n", lpszProc, ext_ret->name);
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
static BOOL WINAPI wglUseFontOutlines_common(HDC hdc,
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

    TRACE("(%p, %d, %d, %d, %f, %f, %d, %p, %s)\n", hdc, first, count,
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

static BOOL WINAPI wglUseFontOutlines_common(HDC hdc,
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

/***********************************************************************
 *              glFinish (OPENGL32.@)
 */
void WINAPI wine_glFinish( void )
{
    TRACE("()\n");
    wine_wgl.p_wglFinish();
}

/***********************************************************************
 *              glFlush (OPENGL32.@)
 */
void WINAPI wine_glFlush( void )
{
    TRACE("()\n");
    wine_wgl.p_wglFlush();
}

/***********************************************************************
 *              glGetString (OPENGL32.@)
 */
const GLubyte * WINAPI wine_glGetString( GLenum name )
{
  const GLubyte *ret;
  const char* GL_Extensions = NULL;

  if (GL_EXTENSIONS != name) {
    ENTER_GL();
    ret = glGetString(name);
    LEAVE_GL();
    return ret;
  }

  if (NULL == internal_gl_extensions) {
    ENTER_GL();
    GL_Extensions = (const char *) glGetString(GL_EXTENSIONS);

    if (GL_Extensions)
    {
      size_t len = strlen(GL_Extensions);
      internal_gl_extensions = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 2);

      TRACE("GL_EXTENSIONS reported:\n");
      while (*GL_Extensions != 0x00) {
	const char* Start = GL_Extensions;
	char        ThisExtn[256];

	while (*GL_Extensions != ' ' && *GL_Extensions != 0x00) {
	  GL_Extensions++;
	}
	memcpy(ThisExtn, Start, (GL_Extensions - Start));
        ThisExtn[GL_Extensions - Start] = 0;
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
    LEAVE_GL();
  }
  return (const GLubyte *) internal_gl_extensions;
}

/***********************************************************************
 *              glGetIntegerv (OPENGL32.@)
 */
void WINAPI wine_glGetIntegerv( GLenum pname, GLint* params )
{
    wine_wgl.p_wglGetIntegerv(pname, params);
}


/* This is for brain-dead applications that use OpenGL functions before even
   creating a rendering context.... */
static BOOL process_attach(void)
{
  HMODULE mod_x11, mod_gdi32;
  DWORD size = sizeof(internal_gl_disabled_extensions);
  HKEY hkey = 0;

  GetDesktopWindow();  /* make sure winex11 is loaded (FIXME) */
  mod_x11 = GetModuleHandleA( "winex11.drv" );
  mod_gdi32 = GetModuleHandleA( "gdi32.dll" );

  if (!mod_x11 || !mod_gdi32)
  {
      ERR("X11DRV or GDI32 not loaded. Cannot create default context.\n");
      return FALSE;
  }

  wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod_x11, "wine_tsx11_lock" );
  wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod_x11, "wine_tsx11_unlock" );

  wine_wgl.p_wglGetProcAddress = (void *)GetProcAddress(mod_gdi32, "wglGetProcAddress");

  /* Interal WGL function */
  wine_wgl.p_wglGetIntegerv = (void *)wine_wgl.p_wglGetProcAddress("wglGetIntegerv");
  wine_wgl.p_wglFinish = (void *)wine_wgl.p_wglGetProcAddress("wglFinish");
  wine_wgl.p_wglFlush = (void *)wine_wgl.p_wglGetProcAddress("wglFlush");

  internal_gl_disabled_extensions[0] = 0;
  if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\OpenGL", &hkey)) {
    if (!RegQueryValueExA( hkey, "DisabledExtensions", 0, NULL, (LPBYTE)internal_gl_disabled_extensions, &size)) {
      TRACE("found DisabledExtensions=\"%s\"\n", internal_gl_disabled_extensions);
    }
    RegCloseKey(hkey);
  }

  return TRUE;
}


/**********************************************************************/

static void process_detach(void)
{
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
