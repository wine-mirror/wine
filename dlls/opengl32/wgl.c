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
#include "wine/gdi_driver.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(opengl);

static struct
{
    /* internal WGL functions */
    void  (WINAPI *p_wglFinish)(void);
    void  (WINAPI *p_wglFlush)(void);
    void  (WINAPI *p_wglGetIntegerv)(GLenum pname, GLint* params);
} wine_wgl;

#ifdef SONAME_LIBGLU
#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(gluNewTess)
MAKE_FUNCPTR(gluDeleteTess)
MAKE_FUNCPTR(gluTessBeginContour)
MAKE_FUNCPTR(gluTessBeginPolygon)
MAKE_FUNCPTR(gluTessCallback)
MAKE_FUNCPTR(gluTessEndContour)
MAKE_FUNCPTR(gluTessEndPolygon)
MAKE_FUNCPTR(gluTessVertex)
#undef MAKE_FUNCPTR
#endif /* SONAME_LIBGLU */

static HMODULE opengl32_handle;
static void* libglu_handle = NULL;

const GLubyte * WINAPI wine_glGetString( GLenum name );

/* internal GDI functions */
extern INT WINAPI GdiDescribePixelFormat( HDC hdc, INT fmt, UINT size, PIXELFORMATDESCRIPTOR *pfd );
extern BOOL WINAPI GdiSetPixelFormat( HDC hdc, INT fmt, const PIXELFORMATDESCRIPTOR *pfd );
extern BOOL WINAPI GdiSwapBuffers( HDC hdc );

/* handle management */

#define MAX_WGL_HANDLES 1024

struct wgl_handle
{
    UINT                    handle;
    DWORD                   tid;
    struct wgl_context *    context;
    const struct wgl_funcs *funcs;
};

static struct wgl_handle wgl_handles[MAX_WGL_HANDLES];
static struct wgl_handle *next_free;
static unsigned int handle_count;

static CRITICAL_SECTION wgl_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &wgl_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wgl_section") }
};
static CRITICAL_SECTION wgl_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static inline const struct wgl_funcs *get_dc_funcs( HDC hdc )
{
    return __wine_get_wgl_driver( hdc, WINE_GDI_DRIVER_VERSION );
}

static inline HGLRC next_handle( struct wgl_handle *ptr )
{
    WORD generation = HIWORD( ptr->handle ) + 1;
    if (!generation) generation++;
    ptr->handle = MAKELONG( ptr - wgl_handles, generation );
    return ULongToHandle( ptr->handle );
}

/* the current handle is assumed valid and doesn't need locking */
static inline struct wgl_handle *get_current_handle_ptr(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return NULL;
    return &wgl_handles[LOWORD(NtCurrentTeb()->glCurrentRC)];
}

static struct wgl_handle *get_handle_ptr( HGLRC handle )
{
    unsigned int index = LOWORD( handle );

    EnterCriticalSection( &wgl_section );
    if (index < handle_count && ULongToHandle(wgl_handles[index].handle) == handle)
        return &wgl_handles[index];

    LeaveCriticalSection( &wgl_section );
    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}

static void release_handle_ptr( struct wgl_handle *ptr )
{
    if (ptr) LeaveCriticalSection( &wgl_section );
}

static HGLRC alloc_handle( struct wgl_context *context, const struct wgl_funcs *funcs )
{
    HGLRC handle = 0;
    struct wgl_handle *ptr = NULL;

    EnterCriticalSection( &wgl_section );
    if ((ptr = next_free))
        next_free = (struct wgl_handle *)next_free->context;
    else if (handle_count < MAX_WGL_HANDLES)
        ptr = &wgl_handles[handle_count++];

    if (ptr)
    {
        ptr->context = context;
        ptr->funcs = funcs;
        handle = next_handle( ptr );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    LeaveCriticalSection( &wgl_section );
    return handle;
}

static void free_handle_ptr( struct wgl_handle *ptr )
{
    ptr->handle |= 0xffff;
    ptr->context = (struct wgl_context *)next_free;
    ptr->funcs = NULL;
    next_free = ptr;
    LeaveCriticalSection( &wgl_section );
}

/***********************************************************************
 *		 wglSetPixelFormat(OPENGL32.@)
 */
BOOL WINAPI wglSetPixelFormat( HDC hdc, INT iPixelFormat,
                               const PIXELFORMATDESCRIPTOR *ppfd)
{
    return GdiSetPixelFormat(hdc, iPixelFormat, ppfd);
}

/***********************************************************************
 *		wglCopyContext (OPENGL32.@)
 */
BOOL WINAPI wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    struct wgl_handle *src, *dst;
    BOOL ret = FALSE;

    if (!(src = get_handle_ptr( hglrcSrc ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst )))
    {
        if (src->funcs != dst->funcs) SetLastError( ERROR_INVALID_HANDLE );
        else ret = src->funcs->p_wglCopyContext( src->context, dst->context, mask );
    }
    release_handle_ptr( dst );
    release_handle_ptr( src );
    return ret;
}

/***********************************************************************
 *		wglDeleteContext (OPENGL32.@)
 */
BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
    struct wgl_handle *ptr = get_handle_ptr( hglrc );

    if (!ptr) return FALSE;

    if (ptr->tid && ptr->tid != GetCurrentThreadId())
    {
        SetLastError( ERROR_BUSY );
        release_handle_ptr( ptr );
        return FALSE;
    }
    if (hglrc == NtCurrentTeb()->glCurrentRC) wglMakeCurrent( 0, 0 );
    ptr->funcs->p_wglDeleteContext( ptr->context );
    free_handle_ptr( ptr );
    return TRUE;
}

/***********************************************************************
 *		wglMakeCurrent (OPENGL32.@)
 */
BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_handle_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc ))) return FALSE;
        if (!ptr->tid || ptr->tid == GetCurrentThreadId())
        {
            ret = ptr->funcs->p_wglMakeCurrent( hdc, ptr->context );
            if (ret)
            {
                if (prev) prev->tid = 0;
                ptr->tid = GetCurrentThreadId();
                NtCurrentTeb()->glCurrentRC = hglrc;
            }
        }
        else
        {
            SetLastError( ERROR_BUSY );
            ret = FALSE;
        }
        release_handle_ptr( ptr );
    }
    else if (prev)
    {
        if (!prev->funcs->p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
    }
    else if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        ret = FALSE;
    }
    return ret;
}

/***********************************************************************
 *		wglCreateContextAttribsARB  (wrapper for the extension function returned by the driver)
 */
static HGLRC WINAPI wglCreateContextAttribsARB( HDC hdc, HGLRC share, const int *attribs )
{
    HGLRC ret = 0;
    struct wgl_context *context;
    struct wgl_handle *share_ptr = NULL;
    const struct wgl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs) return 0;
    if (share && !(share_ptr = get_handle_ptr( share ))) return 0;
    if ((context = funcs->p_wglCreateContextAttribsARB( hdc, share_ptr ? share_ptr->context : NULL,
                                                        attribs )))
    {
        ret = alloc_handle( context, funcs );
        if (!ret) funcs->p_wglDeleteContext( context );
    }
    release_handle_ptr( share_ptr );
    return ret;

}

/***********************************************************************
 *		wglMakeContextCurrentARB  (wrapper for the extension function returned by the driver)
 */
static BOOL WINAPI wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, HGLRC hglrc )
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_handle_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc ))) return FALSE;
        if (!ptr->tid || ptr->tid == GetCurrentThreadId())
        {
            ret = ptr->funcs->p_wglMakeContextCurrentARB( draw_hdc, read_hdc, ptr->context );
            if (ret)
            {
                if (prev) prev->tid = 0;
                ptr->tid = GetCurrentThreadId();
                NtCurrentTeb()->glCurrentRC = hglrc;
            }
        }
        else
        {
            SetLastError( ERROR_BUSY );
            ret = FALSE;
        }
        release_handle_ptr( ptr );
    }
    else if (prev)
    {
        if (!prev->funcs->p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
    }
    return ret;
}

/***********************************************************************
 *		wglShareLists (OPENGL32.@)
 */
BOOL WINAPI wglShareLists(HGLRC hglrcSrc, HGLRC hglrcDst)
{
    BOOL ret = FALSE;
    struct wgl_handle *src, *dst;

    if (!(src = get_handle_ptr( hglrcSrc ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst )))
    {
        if (src->funcs != dst->funcs) SetLastError( ERROR_INVALID_HANDLE );
        else ret = src->funcs->p_wglShareLists( src->context, dst->context );
    }
    release_handle_ptr( dst );
    release_handle_ptr( src );
    return ret;
}

/***********************************************************************
 *		wglGetCurrentDC (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentDC(void)
{
    struct wgl_handle *context = get_current_handle_ptr();
    if (!context) return 0;
    return context->funcs->p_wglGetCurrentDC( context->context );
}

/***********************************************************************
 *		wglCreateContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateContext(HDC hdc)
{
    HGLRC ret = 0;
    struct wgl_context *context;
    const struct wgl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs) return 0;
    if (!(context = funcs->p_wglCreateContext( hdc ))) return 0;
    ret = alloc_handle( context, funcs );
    if (!ret) funcs->p_wglDeleteContext( context );
    return ret;
}

/***********************************************************************
 *		wglGetCurrentContext (OPENGL32.@)
 */
HGLRC WINAPI wglGetCurrentContext(void)
{
    return NtCurrentTeb()->glCurrentRC;
}

/***********************************************************************
 *		wglChoosePixelFormat (OPENGL32.@)
 */
INT WINAPI wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd)
{
    PIXELFORMATDESCRIPTOR format, best;
    int i, count, best_format;
    int bestDBuffer = -1, bestStereo = -1;

    TRACE_(wgl)( "%p %p: size %u version %u flags %u type %u color %u %u,%u,%u,%u "
                 "accum %u depth %u stencil %u aux %u\n",
                 hdc, ppfd, ppfd->nSize, ppfd->nVersion, ppfd->dwFlags, ppfd->iPixelType,
                 ppfd->cColorBits, ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits, ppfd->cAlphaBits,
                 ppfd->cAccumBits, ppfd->cDepthBits, ppfd->cStencilBits, ppfd->cAuxBuffers );

    count = GdiDescribePixelFormat( hdc, 0, 0, NULL );
    if (!count) return 0;

    best_format = 0;
    best.dwFlags = 0;
    best.cAlphaBits = -1;
    best.cColorBits = -1;
    best.cDepthBits = -1;
    best.cStencilBits = -1;
    best.cAuxBuffers = -1;

    for (i = 1; i <= count; i++)
    {
        if (!GdiDescribePixelFormat( hdc, i, sizeof(format), &format )) continue;

        if (ppfd->iPixelType != format.iPixelType)
        {
            TRACE( "pixel type mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* only use bitmap capable for formats for bitmap rendering */
        if( (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) != (format.dwFlags & PFD_DRAW_TO_BITMAP))
        {
            TRACE( "PFD_DRAW_TO_BITMAP mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* The behavior of PDF_STEREO/PFD_STEREO_DONTCARE and PFD_DOUBLEBUFFER / PFD_DOUBLEBUFFER_DONTCARE
         * is not very clear on MSDN. They specify that ChoosePixelFormat tries to match pixel formats
         * with the flag (PFD_STEREO / PFD_DOUBLEBUFFERING) set. Otherwise it says that it tries to match
         * formats without the given flag set.
         * A test on Windows using a Radeon 9500pro on WinXP (the driver doesn't support Stereo)
         * has indicated that a format without stereo is returned when stereo is unavailable.
         * So in case PFD_STEREO is set, formats that support it should have priority above formats
         * without. In case PFD_STEREO_DONTCARE is set, stereo is ignored.
         *
         * To summarize the following is most likely the correct behavior:
         * stereo not set -> prefer non-stereo formats, but also accept stereo formats
         * stereo set -> prefer stereo formats, but also accept non-stereo formats
         * stereo don't care -> it doesn't matter whether we get stereo or not
         *
         * In Wine we will treat non-stereo the same way as don't care because it makes
         * format selection even more complicated and second drivers with Stereo advertise
         * each format twice anyway.
         */

        /* Doublebuffer, see the comments above */
        if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) &&
                ((format.dwFlags & PFD_DOUBLEBUFFER) == (ppfd->dwFlags & PFD_DOUBLEBUFFER)))
                goto found;

            if (bestDBuffer != -1 && (format.dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) continue;
        }

        /* Stereo, see the comments above. */
        if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_STEREO) != bestStereo) &&
                ((format.dwFlags & PFD_STEREO) == (ppfd->dwFlags & PFD_STEREO)))
                goto found;

            if (bestStereo != -1 && (format.dwFlags & PFD_STEREO) != bestStereo) continue;
        }

        /* Below we will do a number of checks to select the 'best' pixelformat.
         * We assume the precedence cColorBits > cAlphaBits > cDepthBits > cStencilBits -> cAuxBuffers.
         * The code works by trying to match the most important options as close as possible.
         * When a reasonable format is found, we will try to match more options.
         * It appears (see the opengl32 test) that Windows opengl drivers ignore options
         * like cColorBits, cAlphaBits and friends if they are set to 0, so they are considered
         * as DONTCARE. At least Serious Sam TSE relies on this behavior. */

        if (ppfd->cColorBits)
        {
            if (((ppfd->cColorBits > best.cColorBits) && (format.cColorBits > best.cColorBits)) ||
                ((format.cColorBits >= ppfd->cColorBits) && (format.cColorBits < best.cColorBits)))
                goto found;

            if (best.cColorBits != format.cColorBits)  /* Do further checks if the format is compatible */
            {
                TRACE( "color mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAlphaBits)
        {
            if (((ppfd->cAlphaBits > best.cAlphaBits) && (format.cAlphaBits > best.cAlphaBits)) ||
                ((format.cAlphaBits >= ppfd->cAlphaBits) && (format.cAlphaBits < best.cAlphaBits)))
                goto found;

            if (best.cAlphaBits != format.cAlphaBits)
            {
                TRACE( "alpha mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cDepthBits)
        {
            if (((ppfd->cDepthBits > best.cDepthBits) && (format.cDepthBits > best.cDepthBits)) ||
                ((format.cDepthBits >= ppfd->cDepthBits) && (format.cDepthBits < best.cDepthBits)))
                goto found;

            if (best.cDepthBits != format.cDepthBits)
            {
                TRACE( "depth mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cStencilBits)
        {
            if (((ppfd->cStencilBits > best.cStencilBits) && (format.cStencilBits > best.cStencilBits)) ||
                ((format.cStencilBits >= ppfd->cStencilBits) && (format.cStencilBits < best.cStencilBits)))
                goto found;

            if (best.cStencilBits != format.cStencilBits)
            {
                TRACE( "stencil mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAuxBuffers)
        {
            if (((ppfd->cAuxBuffers > best.cAuxBuffers) && (format.cAuxBuffers > best.cAuxBuffers)) ||
                ((format.cAuxBuffers >= ppfd->cAuxBuffers) && (format.cAuxBuffers < best.cAuxBuffers)))
                goto found;

            if (best.cAuxBuffers != format.cAuxBuffers)
            {
                TRACE( "aux mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        continue;

    found:
        best_format = i;
        best = format;
        bestDBuffer = format.dwFlags & PFD_DOUBLEBUFFER;
        bestStereo = format.dwFlags & PFD_STEREO;
    }

    TRACE( "returning %u\n", best_format );
    return best_format;
}

/***********************************************************************
 *		wglDescribePixelFormat (OPENGL32.@)
 */
INT WINAPI wglDescribePixelFormat(HDC hdc, INT iPixelFormat, UINT nBytes,
                                LPPIXELFORMATDESCRIPTOR ppfd)
{
  return GdiDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

/***********************************************************************
 *		wglGetPixelFormat (OPENGL32.@)
 */
INT WINAPI wglGetPixelFormat(HDC hdc)
{
    const struct wgl_funcs *funcs = get_dc_funcs( hdc );
    if (!funcs) return 0;
    return funcs->p_GetPixelFormat( hdc );
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
  FIXME("no handler for layer %d\n", iLayerPlane);

  return NULL;
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
  FIXME("(): stub!\n");

  return 0;
}

/* check if the extension is present in the list */
static BOOL has_extension( const char *list, const char *ext )
{
    size_t len = strlen( ext );

    while (list)
    {
        while (*list == ' ') list++;
        if (!strncmp( list, ext, len ) && (!list[len] || list[len] == ' ')) return TRUE;
        list = strchr( list, ' ' );
    }
    return FALSE;
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
     * require the function to return NULL when an extension isn't found. For this reason we check
     * if the OpenGL extension required for the function we are looking up is supported. */

    /* Check if the extension is part of the GL extension string to see if it is supported. */
    if (has_extension(gl_ext_string, extension))
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
            ERR("No OpenGL version found!\n");
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

static const OpenGL_extension wgl_extensions[] =
{
    { "wglCreateContextAttribsARB", "WGL_ARB_create_context", wglCreateContextAttribsARB },
    { "wglMakeContextCurrentARB", "WGL_ARB_make_current_read", wglMakeContextCurrentARB },
};

/***********************************************************************
 *		wglGetProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetProcAddress(LPCSTR  lpszProc) {
  void *local_func;
  OpenGL_extension  ext;
  const OpenGL_extension *ext_ret;
  struct wgl_handle *context = get_current_handle_ptr();

  TRACE("(%s)\n", lpszProc);

  if (lpszProc == NULL)
    return NULL;

  /* Without an active context opengl32 doesn't know to what
   * driver it has to dispatch wglGetProcAddress.
   */
  if (!context)
  {
    WARN("No active WGL context found\n");
    return NULL;
  }

  /* Search in the thunks to find the real name of the extension */
  ext.name = lpszProc;
  ext_ret = bsearch(&ext, extension_registry, extension_registry_size,
                    sizeof(OpenGL_extension), compar);

  /* If nothing was found, we are looking for a WGL extension or an unknown GL extension. */
  if (ext_ret == NULL) {
    /* If the function name starts with a 'w', it is a WGL extension */
    if(lpszProc[0] == 'w')
    {
        local_func = context->funcs->p_wglGetProcAddress( lpszProc );
        if (local_func == (void *)1)  /* special function that needs a wrapper */
        {
            ext_ret = bsearch( &ext, wgl_extensions, sizeof(wgl_extensions)/sizeof(wgl_extensions[0]),
                               sizeof(OpenGL_extension), compar );
            if (ext_ret) return ext_ret->func;

            FIXME( "wrapper missing for %s\n", lpszProc );
            return NULL;
        }
        return local_func;
    }

    /* We are dealing with an unknown GL extension */
    WARN("Extension '%s' not defined in opengl32.dll's function table!\n", lpszProc);
    return NULL;
  } else { /* We are looking for an OpenGL extension */

    /* Check if the GL extension required by the function is available */
    if(!is_extension_supported(ext_ret->extension)) {
        WARN("Extension '%s' required by function '%s' not supported!\n", ext_ret->extension, lpszProc);
    }

    local_func = context->funcs->p_wglGetProcAddress( ext_ret->name );

    /* After that, look at the extensions defined in the Linux OpenGL library */
    if (local_func == NULL) {
      char buf[256];
      void *ret = NULL;

      /* Remove the last 3 letters (EXT, ARB, ...).

	 I know that some extensions have more than 3 letters (MESA, NV,
	 INTEL, ...), but this is only a stop-gap measure to fix buggy
	 OpenGL drivers (moreover, it is only useful for old 1.0 apps
	 that query the glBindTextureEXT extension).
      */
      memcpy(buf, ext_ret->name, strlen(ext_ret->name) - 3);
      buf[strlen(ext_ret->name) - 3] = '\0';
      TRACE("Extension not found in the Linux OpenGL library, checking against libGL bug with %s..\n", buf);

      ret = GetProcAddress(opengl32_handle, buf);
      if (ret != NULL) {
        TRACE("Found function in main OpenGL library (%p)!\n", ret);
      } else {
        WARN("Did not find function %s (%s) in your OpenGL library!\n", lpszProc, ext_ret->name);
      }

      return ret;
    } else {
      TRACE("returning function (%p)\n", ext_ret->func);
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
  FIXME("(): stub!\n");

  return 0;
}

/***********************************************************************
 *		wglSwapLayerBuffers (OPENGL32.@)
 */
BOOL WINAPI wglSwapLayerBuffers(HDC hdc,
				UINT fuPlanes) {
  TRACE_(opengl)("(%p, %08x)\n", hdc, fuPlanes);

  if (fuPlanes & WGL_SWAP_MAIN_PLANE) {
    if (!GdiSwapBuffers(hdc)) return FALSE;
    fuPlanes &= ~WGL_SWAP_MAIN_PLANE;
  }

  if (fuPlanes) {
    WARN("Following layers unhandled: %08x\n", fuPlanes);
  }

  return TRUE;
}

/***********************************************************************
 *		wglUseFontBitmaps_common
 */
static BOOL wglUseFontBitmaps_common( HDC hdc, DWORD first, DWORD count, DWORD listBase, BOOL unicode )
{
     GLYPHMETRICS gm;
     unsigned int glyph, size = 0;
     void *bitmap = NULL, *gl_bitmap = NULL;
     int org_alignment;
     BOOL ret = TRUE;

     glGetIntegerv(GL_UNPACK_ALIGNMENT, &org_alignment);
     glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

     for (glyph = first; glyph < first + count; glyph++) {
         static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };
         unsigned int needed_size, height, width, width_int;

         if (unicode)
             needed_size = GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);
         else
             needed_size = GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);

         TRACE("Glyph: %3d / List: %d size %d\n", glyph, listBase, needed_size);
         if (needed_size == GDI_ERROR) {
             ret = FALSE;
             break;
         }

         if (needed_size > size) {
             size = needed_size;
             HeapFree(GetProcessHeap(), 0, bitmap);
             HeapFree(GetProcessHeap(), 0, gl_bitmap);
             bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
             gl_bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
         }
         if (unicode)
             ret = (GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, &identity) != GDI_ERROR);
         else
             ret = (GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, &identity) != GDI_ERROR);
         if (!ret) break;

         if (TRACE_ON(wgl)) {
             unsigned int bitmask;
             unsigned char *bitmap_ = bitmap;

             TRACE("  - bbox: %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
             TRACE("  - origin: (%d, %d)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
             TRACE("  - increment: %d - %d\n", gm.gmCellIncX, gm.gmCellIncY);
             if (needed_size != 0) {
                 TRACE("  - bitmap:\n");
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
                 for (width = 0; width < width_int; width++) {
                     ((int *) gl_bitmap)[(gm.gmBlackBoxY - height - 1) * width_int + width] =
                     ((int *) bitmap)[height * width_int + width];
                 }
             }
         }

         glNewList(listBase++, GL_COMPILE);
         if (needed_size != 0) {
             glBitmap(gm.gmBlackBoxX, gm.gmBlackBoxY,
                     0 - gm.gmptGlyphOrigin.x, (int) gm.gmBlackBoxY - gm.gmptGlyphOrigin.y,
                     gm.gmCellIncX, gm.gmCellIncY,
                     gl_bitmap);
         } else {
             /* This is the case of 'empty' glyphs like the space character */
             glBitmap(0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL);
         }
         glEndList();
     }

     glPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
     HeapFree(GetProcessHeap(), 0, bitmap);
     HeapFree(GetProcessHeap(), 0, gl_bitmap);
     return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmaps_common( hdc, first, count, listBase, FALSE );
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmaps_common( hdc, first, count, listBase, TRUE );
}

#ifdef SONAME_LIBGLU

static void *load_libglu(void)
{
    static int already_loaded;
    void *handle;

    if (already_loaded) return libglu_handle;
    already_loaded = 1;

    TRACE("Trying to load GLU library: %s\n", SONAME_LIBGLU);
    handle = wine_dlopen(SONAME_LIBGLU, RTLD_NOW, NULL, 0);
    if (!handle)
    {
        WARN("Failed to load %s\n", SONAME_LIBGLU);
        return NULL;
    }

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(handle, #f, NULL, 0)) == NULL) goto sym_not_found;
LOAD_FUNCPTR(gluNewTess)
LOAD_FUNCPTR(gluDeleteTess)
LOAD_FUNCPTR(gluTessBeginContour)
LOAD_FUNCPTR(gluTessBeginPolygon)
LOAD_FUNCPTR(gluTessCallback)
LOAD_FUNCPTR(gluTessEndContour)
LOAD_FUNCPTR(gluTessEndPolygon)
LOAD_FUNCPTR(gluTessVertex)
#undef LOAD_FUNCPTR
    libglu_handle = handle;
    return handle;

sym_not_found:
    WARN("Unable to load function ptrs from libGLU\n");
    /* Close the library as we won't use it */
    wine_dlclose(handle, NULL, 0);
    return NULL;
}

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
static BOOL wglUseFontOutlines_common(HDC hdc,
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

    if (!load_libglu())
    {
        ERR("libGLU is required for this function but isn't loaded\n");
        return FALSE;
    }

    tess = pgluNewTess();
    if(!tess) return FALSE;
    pgluTessCallback(tess, GLU_TESS_VERTEX, (_GLUfuncptr)tess_callback_vertex);
    pgluTessCallback(tess, GLU_TESS_BEGIN, (_GLUfuncptr)tess_callback_begin);
    pgluTessCallback(tess, GLU_TESS_END, tess_callback_end);

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

	glNewList(listBase++, GL_COMPILE);
        pgluTessBeginPolygon(tess, NULL);

        pph = (TTPOLYGONHEADER*)buf;
        while((BYTE*)pph < buf + needed)
        {
            TRACE("\tstart %d, %d\n", pph->pfxStart.x.value, pph->pfxStart.y.value);

            pgluTessBeginContour(tess);

            fixed_to_double(pph->pfxStart, em_size, vertices);
            pgluTessVertex(tess, vertices, vertices);
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
                        pgluTessVertex(tess, vertices, vertices);
                        vertices += 3;
                    }
                    break;

                case TT_PRIM_QSPLINE:
                    for(i = 0; i < ppc->cpfx/2; i++)
                    {
                        /* FIXME: just connecting the control points for now */
                        TRACE("\t\tcurve  %d,%d %d,%d\n",
                              ppc->apfx[i * 2].x.value,     ppc->apfx[i * 3].y.value,
                              ppc->apfx[i * 2 + 1].x.value, ppc->apfx[i * 3 + 1].y.value);
                        fixed_to_double(ppc->apfx[i * 2], em_size, vertices); 
                        pgluTessVertex(tess, vertices, vertices);
                        vertices += 3;
                        fixed_to_double(ppc->apfx[i * 2 + 1], em_size, vertices); 
                        pgluTessVertex(tess, vertices, vertices);
                        vertices += 3;
                    }
                    break;
                default:
                    ERR("\t\tcurve type = %d\n", ppc->wType);
                    pgluTessEndContour(tess);
                    goto error_in_list;
                }

                ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
                                     (ppc->cpfx - 1) * sizeof(POINTFX));
            }
            pgluTessEndContour(tess);
            pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
        }

error_in_list:
        pgluTessEndPolygon(tess);
        glTranslated((GLdouble)gm.gmCellIncX / em_size, (GLdouble)gm.gmCellIncY / em_size, 0.0);
        glEndList();
        HeapFree(GetProcessHeap(), 0, buf);
        HeapFree(GetProcessHeap(), 0, vertices);
    }

 error:
    DeleteObject(SelectObject(hdc, old_font));
    pgluDeleteTess(tess);
    return TRUE;

}

#else /* SONAME_LIBGLU */

static BOOL wglUseFontOutlines_common(HDC hdc,
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

#endif /* SONAME_LIBGLU */

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
 *              glDebugEntry (OPENGL32.@)
 */
GLint WINAPI wine_glDebugEntry( GLint unknown1, GLint unknown2 )
{
    return 0;
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

/* build the extension string by filtering out the disabled extensions */
static char *build_gl_extensions( const char *extensions )
{
    char *p, *str, *disabled = NULL;
    const char *end;
    HKEY hkey;

    TRACE( "GL_EXTENSIONS:\n" );

    if (!extensions) extensions = "";

    /* @@ Wine registry key: HKCU\Software\Wine\OpenGL */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\OpenGL", &hkey ))
    {
        DWORD size, ret = RegQueryValueExA( hkey, "DisabledExtensions", 0, NULL, NULL, &size );
        if (!ret && (disabled = HeapAlloc( GetProcessHeap(), 0, size )))
            ret = RegQueryValueExA( hkey, "DisabledExtensions", 0, NULL, (BYTE *)disabled, &size );
        RegCloseKey( hkey );
        if (ret) *disabled = 0;
    }

    if ((str = HeapAlloc( GetProcessHeap(), 0, strlen(extensions) + 2 )))
    {
        p = str;
        for (;;)
        {
            while (*extensions == ' ') extensions++;
            if (!*extensions) break;
            if (!(end = strchr( extensions, ' ' ))) end = extensions + strlen( extensions );
            memcpy( p, extensions, end - extensions );
            p[end - extensions] = 0;
            if (!has_extension( disabled, p ))
            {
                TRACE("++ %s\n", p );
                p += end - extensions;
                *p++ = ' ';
            }
            else TRACE("-- %s (disabled by config)\n", p );
            extensions = end;
        }
        *p = 0;
    }
    HeapFree( GetProcessHeap(), 0, disabled );
    return str;
}

/***********************************************************************
 *              glGetString (OPENGL32.@)
 */
const GLubyte * WINAPI wine_glGetString( GLenum name )
{
  static const GLubyte *gl_extensions;

  /* this is for buggy nvidia driver, crashing if called from a different
     thread with no context */
  if(wglGetCurrentContext() == NULL)
    return NULL;

  if (name != GL_EXTENSIONS) return glGetString(name);

  if (!gl_extensions)
  {
      const char *orig_ext = (const char *)glGetString(GL_EXTENSIONS);
      char *new_ext = build_gl_extensions( orig_ext );
      if (InterlockedCompareExchangePointer( (void **)&gl_extensions, new_ext, NULL ))
          HeapFree( GetProcessHeap(), 0, new_ext );
  }
  return gl_extensions;
}

/***********************************************************************
 *              glGetIntegerv (OPENGL32.@)
 */
void WINAPI wine_glGetIntegerv( GLenum pname, GLint* params )
{
    wine_wgl.p_wglGetIntegerv(pname, params);
}

/***********************************************************************
 *              wglSwapBuffers (OPENGL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH wglSwapBuffers( HDC hdc )
{
    return GdiSwapBuffers(hdc);
}

/* This is for brain-dead applications that use OpenGL functions before even
   creating a rendering context.... */
static BOOL process_attach(void)
{
  HDC hdc = GetDC( 0 );
  const struct wgl_funcs *funcs = get_dc_funcs( hdc );

  ReleaseDC( 0, hdc );

  /* internal WGL functions */
  wine_wgl.p_wglFinish = (void *)funcs->p_wglGetProcAddress("wglFinish");
  wine_wgl.p_wglFlush = (void *)funcs->p_wglGetProcAddress("wglFlush");
  wine_wgl.p_wglGetIntegerv = (void *)funcs->p_wglGetProcAddress("wglGetIntegerv");
  return TRUE;
}


/**********************************************************************/

static void process_detach(void)
{
  if (libglu_handle) wine_dlclose(libglu_handle, NULL, 0);
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
