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

#include "opengl_ext.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wingdi.h"
#include "winternl.h"
#include "winnt.h"

#define WGL_WGLEXT_PROTOTYPES
#include "wine/wglext.h"
#include "wine/gdi_driver.h"
#include "wine/wgl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(fps);

extern struct opengl_funcs null_opengl_funcs;

/* handle management */

#define MAX_WGL_HANDLES 1024

enum wgl_handle_type
{
    HANDLE_PBUFFER = 0 << 12,
    HANDLE_CONTEXT = 1 << 12,
    HANDLE_CONTEXT_V3 = 3 << 12,
    HANDLE_TYPE_MASK = 15 << 12
};

struct opengl_context
{
    DWORD               tid;           /* thread that the context is current in */
    HDC                 draw_dc;       /* current drawing DC */
    HDC                 read_dc;       /* current reading DC */
    GLubyte            *extensions;    /* extension string */
    GLuint             *disabled_exts; /* indices of disabled extensions */
    struct wgl_context *drv_ctx;       /* driver context */
};

struct wgl_handle
{
    UINT                 handle;
    struct opengl_funcs *funcs;
    union
    {
        struct opengl_context *context;  /* for HANDLE_CONTEXT */
        struct wgl_pbuffer    *pbuffer;  /* for HANDLE_PBUFFER */
        struct wgl_handle     *next;     /* for free handles */
    } u;
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

static inline struct opengl_funcs *get_dc_funcs( HDC hdc )
{
    struct opengl_funcs *funcs = __wine_get_wgl_driver( hdc, WINE_WGL_DRIVER_VERSION );
    if (funcs == (void *)-1) funcs = &null_opengl_funcs;
    return funcs;
}

static inline HANDLE next_handle( struct wgl_handle *ptr, enum wgl_handle_type type )
{
    WORD generation = HIWORD( ptr->handle ) + 1;
    if (!generation) generation++;
    ptr->handle = MAKELONG( ptr - wgl_handles, generation ) | type;
    return ULongToHandle( ptr->handle );
}

/* the current context is assumed valid and doesn't need locking */
static inline struct wgl_handle *get_current_context_ptr(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return NULL;
    return &wgl_handles[LOWORD(NtCurrentTeb()->glCurrentRC) & ~HANDLE_TYPE_MASK];
}

static struct wgl_handle *get_handle_ptr( HANDLE handle, enum wgl_handle_type type )
{
    unsigned int index = LOWORD( handle ) & ~HANDLE_TYPE_MASK;

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

static HANDLE alloc_handle( enum wgl_handle_type type, struct opengl_funcs *funcs, void *user_ptr )
{
    HANDLE handle = 0;
    struct wgl_handle *ptr = NULL;

    EnterCriticalSection( &wgl_section );
    if ((ptr = next_free))
        next_free = next_free->u.next;
    else if (handle_count < MAX_WGL_HANDLES)
        ptr = &wgl_handles[handle_count++];

    if (ptr)
    {
        ptr->funcs = funcs;
        ptr->u.context = user_ptr;
        handle = next_handle( ptr, type );
    }
    else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    LeaveCriticalSection( &wgl_section );
    return handle;
}

static void free_handle_ptr( struct wgl_handle *ptr )
{
    ptr->handle |= 0xffff;
    ptr->u.next = next_free;
    ptr->funcs = NULL;
    next_free = ptr;
    LeaveCriticalSection( &wgl_section );
}

static inline enum wgl_handle_type get_current_context_type(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return HANDLE_CONTEXT;
    return LOWORD(NtCurrentTeb()->glCurrentRC) & HANDLE_TYPE_MASK;
}

/***********************************************************************
 *		wglCopyContext (OPENGL32.@)
 */
BOOL WINAPI wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    struct wgl_handle *src, *dst;
    BOOL ret = FALSE;

    if (!(src = get_handle_ptr( hglrcSrc, HANDLE_CONTEXT ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst, HANDLE_CONTEXT )))
    {
        if (src->funcs != dst->funcs) SetLastError( ERROR_INVALID_HANDLE );
        else ret = src->funcs->wgl.p_wglCopyContext( src->u.context->drv_ctx,
                                                     dst->u.context->drv_ctx, mask );
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
    struct wgl_handle *ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT );

    if (!ptr) return FALSE;

    if (ptr->u.context->tid && ptr->u.context->tid != GetCurrentThreadId())
    {
        SetLastError( ERROR_BUSY );
        release_handle_ptr( ptr );
        return FALSE;
    }
    if (hglrc == NtCurrentTeb()->glCurrentRC) wglMakeCurrent( 0, 0 );
    ptr->funcs->wgl.p_wglDeleteContext( ptr->u.context->drv_ctx );
    HeapFree( GetProcessHeap(), 0, ptr->u.context->disabled_exts );
    HeapFree( GetProcessHeap(), 0, ptr->u.context->extensions );
    HeapFree( GetProcessHeap(), 0, ptr->u.context );
    free_handle_ptr( ptr );
    return TRUE;
}

/***********************************************************************
 *		wglMakeCurrent (OPENGL32.@)
 */
BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_context_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
        if (!ptr->u.context->tid || ptr->u.context->tid == GetCurrentThreadId())
        {
            ret = ptr->funcs->wgl.p_wglMakeCurrent( hdc, ptr->u.context->drv_ctx );
            if (ret)
            {
                if (prev) prev->u.context->tid = 0;
                ptr->u.context->tid = GetCurrentThreadId();
                ptr->u.context->draw_dc = hdc;
                ptr->u.context->read_dc = hdc;
                NtCurrentTeb()->glCurrentRC = hglrc;
                NtCurrentTeb()->glTable = ptr->funcs;
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
        if (!prev->funcs->wgl.p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->u.context->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
        NtCurrentTeb()->glTable = &null_opengl_funcs;
    }
    else if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        ret = FALSE;
    }
    return ret;
}

/***********************************************************************
 *		wglCreateContextAttribsARB
 *
 * Provided by the WGL_ARB_create_context extension.
 */
HGLRC WINAPI wglCreateContextAttribsARB( HDC hdc, HGLRC share, const int *attribs )
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct wgl_handle *share_ptr = NULL;
    struct opengl_context *context;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglCreateContextAttribsARB) return 0;
    if (share && !(share_ptr = get_handle_ptr( share, HANDLE_CONTEXT ))) return 0;
    if ((drv_ctx = funcs->ext.p_wglCreateContextAttribsARB( hdc,
                                              share_ptr ? share_ptr->u.context->drv_ctx : NULL, attribs )))
    {
        if ((context = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context) )))
        {
            enum wgl_handle_type type = HANDLE_CONTEXT;

            if (attribs)
            {
                while (*attribs)
                {
                    if (attribs[0] == WGL_CONTEXT_MAJOR_VERSION_ARB)
                    {
                        if (attribs[1] >= 3)
                            type = HANDLE_CONTEXT_V3;
                        break;
                    }
                    attribs += 2;
                }
            }

            context->drv_ctx = drv_ctx;
            if (!(ret = alloc_handle( type, funcs, context )))
                HeapFree( GetProcessHeap(), 0, context );
        }
        if (!ret) funcs->wgl.p_wglDeleteContext( drv_ctx );
    }
    release_handle_ptr( share_ptr );
    return ret;

}

/***********************************************************************
 *		wglMakeContextCurrentARB
 *
 * Provided by the WGL_ARB_make_current_read extension.
 */
BOOL WINAPI wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, HGLRC hglrc )
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_context_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
        if (!ptr->u.context->tid || ptr->u.context->tid == GetCurrentThreadId())
        {
            ret = (ptr->funcs->ext.p_wglMakeContextCurrentARB &&
                   ptr->funcs->ext.p_wglMakeContextCurrentARB( draw_hdc, read_hdc,
                                                               ptr->u.context->drv_ctx ));
            if (ret)
            {
                if (prev) prev->u.context->tid = 0;
                ptr->u.context->tid = GetCurrentThreadId();
                ptr->u.context->draw_dc = draw_hdc;
                ptr->u.context->read_dc = read_hdc;
                NtCurrentTeb()->glCurrentRC = hglrc;
                NtCurrentTeb()->glTable = ptr->funcs;
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
        if (!prev->funcs->wgl.p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->u.context->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
        NtCurrentTeb()->glTable = &null_opengl_funcs;
    }
    return ret;
}

/***********************************************************************
 *		wglGetCurrentReadDCARB
 *
 * Provided by the WGL_ARB_make_current_read extension.
 */
HDC WINAPI wglGetCurrentReadDCARB(void)
{
    struct wgl_handle *ptr = get_current_context_ptr();

    if (!ptr) return 0;
    return ptr->u.context->read_dc;
}

/***********************************************************************
 *		wglShareLists (OPENGL32.@)
 */
BOOL WINAPI wglShareLists(HGLRC hglrcSrc, HGLRC hglrcDst)
{
    BOOL ret = FALSE;
    struct wgl_handle *src, *dst;

    if (!(src = get_handle_ptr( hglrcSrc, HANDLE_CONTEXT ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst, HANDLE_CONTEXT )))
    {
        if (src->funcs != dst->funcs) SetLastError( ERROR_INVALID_HANDLE );
        else ret = src->funcs->wgl.p_wglShareLists( src->u.context->drv_ctx, dst->u.context->drv_ctx );
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
    struct wgl_handle *ptr = get_current_context_ptr();

    if (!ptr) return 0;
    return ptr->u.context->draw_dc;
}

/***********************************************************************
 *		wglCreateContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateContext(HDC hdc)
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct opengl_context *context;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs) return 0;
    if (!(drv_ctx = funcs->wgl.p_wglCreateContext( hdc ))) return 0;
    if ((context = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context) )))
    {
        context->drv_ctx = drv_ctx;
        if (!(ret = alloc_handle( HANDLE_CONTEXT, funcs, context )))
            HeapFree( GetProcessHeap(), 0, context );
    }
    if (!ret) funcs->wgl.p_wglDeleteContext( drv_ctx );
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
 *		wglDescribePixelFormat (OPENGL32.@)
 */
INT WINAPI wglDescribePixelFormat(HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    struct opengl_funcs *funcs = get_dc_funcs( hdc );
    if (!funcs) return 0;
    return funcs->wgl.p_wglDescribePixelFormat( hdc, format, size, descr );
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

    count = wglDescribePixelFormat( hdc, 0, 0, NULL );
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
        if (!wglDescribePixelFormat( hdc, i, sizeof(format), &format )) continue;

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
 *		wglGetPixelFormat (OPENGL32.@)
 */
INT WINAPI wglGetPixelFormat(HDC hdc)
{
    struct opengl_funcs *funcs = get_dc_funcs( hdc );
    if (!funcs) return 0;
    return funcs->wgl.p_wglGetPixelFormat( hdc );
}

/***********************************************************************
 *		 wglSetPixelFormat(OPENGL32.@)
 */
BOOL WINAPI wglSetPixelFormat( HDC hdc, INT format, const PIXELFORMATDESCRIPTOR *descr )
{
    struct opengl_funcs *funcs = get_dc_funcs( hdc );
    if (!funcs) return FALSE;
    return funcs->wgl.p_wglSetPixelFormat( hdc, format, descr );
}

/***********************************************************************
 *              wglSwapBuffers (OPENGL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH wglSwapBuffers( HDC hdc )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->wgl.p_wglSwapBuffers) return FALSE;
    if (!funcs->wgl.p_wglSwapBuffers( hdc )) return FALSE;

    if (TRACE_ON(fps))
    {
        static long prev_time, start_time;
        static unsigned long frames, frames_total;

        DWORD time = GetTickCount();
        frames++;
        frames_total++;
        /* every 1.5 seconds */
        if (time - prev_time > 1500)
        {
            TRACE_(fps)("@ approx %.2ffps, total %.2ffps\n",
                        1000.0*frames/(time - prev_time), 1000.0*frames_total/(time - start_time));
            prev_time = time;
            frames = 0;
            if (start_time == 0) start_time = time;
        }
    }
    return TRUE;
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

static BOOL filter_extensions(const char *extensions, GLubyte **exts_list, GLuint **disabled_exts);

void WINAPI glGetIntegerv(GLenum pname, GLint *data)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    TRACE("(%d, %p)\n", pname, data);
    if (pname == GL_NUM_EXTENSIONS)
    {
        struct wgl_handle *ptr = get_current_context_ptr();

        if (ptr->u.context->disabled_exts ||
            filter_extensions(NULL, NULL, &ptr->u.context->disabled_exts))
        {
            const GLuint *disabled_exts = ptr->u.context->disabled_exts;
            GLint count, disabled_count = 0;

            funcs->gl.p_glGetIntegerv(pname, &count);
            while (*disabled_exts++ != ~0u)
                disabled_count++;
            *data = count - disabled_count;
            return;
        }
    }
    funcs->gl.p_glGetIntegerv(pname, data);
}

const GLubyte * WINAPI glGetStringi(GLenum name, GLuint index)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    TRACE("(%d, %d)\n", name, index);
    if (!funcs->ext.p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->ext.p_glGetStringi;

        *func_ptr = funcs->wgl.p_wglGetProcAddress("glGetStringi");
    }

    if (name == GL_EXTENSIONS)
    {
        struct wgl_handle *ptr = get_current_context_ptr();

        if (ptr->u.context->disabled_exts ||
            filter_extensions(NULL, NULL, &ptr->u.context->disabled_exts))
        {
            const GLuint *disabled_exts = ptr->u.context->disabled_exts;
            unsigned int disabled_count = 0;

            while (index + disabled_count >= *disabled_exts++)
                disabled_count++;
            return funcs->ext.p_glGetStringi(name, index + disabled_count);
        }
    }
    return funcs->ext.p_glGetStringi(name, index);
}

/* check if the extension is present in the list */
static BOOL has_extension( const char *list, const char *ext, size_t len )
{
    if (!list)
    {
        const char *gl_ext;
        unsigned int i;
        GLint extensions_count;

        glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);
        for (i = 0; i < extensions_count; ++i)
        {
            gl_ext = (const char *)glGetStringi(GL_EXTENSIONS, i);
            if (!strncmp(gl_ext, ext, len) && !gl_ext[len])
                return TRUE;
        }
        return FALSE;
    }

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
    enum wgl_handle_type type = get_current_context_type();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const char *gl_ext_string = NULL;
    size_t len;

    TRACE("Checking for extension '%s'\n", extension);

    if (type == HANDLE_CONTEXT)
    {
        gl_ext_string = (const char*)glGetString(GL_EXTENSIONS);
        if (!gl_ext_string)
        {
            ERR("No OpenGL extensions found, check if your OpenGL setup is correct!\n");
            return FALSE;
        }
    }

    /* We use the GetProcAddress function from the display driver to retrieve function pointers
     * for OpenGL and WGL extensions. In case of winex11.drv the OpenGL extension lookup is done
     * using glXGetProcAddress. This function is quite unreliable in the sense that its specs don't
     * require the function to return NULL when an extension isn't found. For this reason we check
     * if the OpenGL extension required for the function we are looking up is supported. */

    while ((len = strcspn(extension, " ")) != 0)
    {
        /* Check if the extension is part of the GL extension string to see if it is supported. */
        if (has_extension(gl_ext_string, extension, len))
            return TRUE;

        /* In general an OpenGL function starts as an ARB/EXT extension and at some stage
         * it becomes part of the core OpenGL library and can be reached without the ARB/EXT
         * suffix as well. In the extension table, these functions contain GL_VERSION_major_minor.
         * Check if we are searching for a core GL function */
        if(strncmp(extension, "GL_VERSION_", 11) == 0)
        {
            const GLubyte *gl_version = funcs->gl.p_glGetString(GL_VERSION);
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

        if (extension[len] == ' ') len++;
        extension += len;
    }

    return FALSE;
}

/***********************************************************************
 *		wglGetProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetProcAddress( LPCSTR name )
{
    struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    void **func_ptr;
    OpenGL_extension  ext;
    const OpenGL_extension *ext_ret;

    if (!name) return NULL;

    /* Without an active context opengl32 doesn't know to what
     * driver it has to dispatch wglGetProcAddress.
     */
    if (!get_current_context_ptr())
    {
        WARN("No active WGL context found\n");
        return NULL;
    }

    ext.name = name;
    ext_ret = bsearch(&ext, extension_registry, extension_registry_size, sizeof(ext), compar);
    if (!ext_ret)
    {
        WARN("Function %s unknown\n", name);
        return NULL;
    }

    func_ptr = (void **)&funcs->ext + (ext_ret - extension_registry);
    if (!*func_ptr)
    {
        void *driver_func = funcs->wgl.p_wglGetProcAddress( name );

        if (!is_extension_supported(ext_ret->extension))
            WARN("Extension %s required for %s not supported\n", ext_ret->extension, name);

        if (driver_func == NULL)
        {
            WARN("Function %s not supported by driver\n", name);
            return NULL;
        }
        *func_ptr = driver_func;
    }

    TRACE("returning %s -> %p\n", name, ext_ret->func);
    return ext_ret->func;
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
  TRACE("(%p, %08x)\n", hdc, fuPlanes);

  if (fuPlanes & WGL_SWAP_MAIN_PLANE) {
    if (!wglSwapBuffers( hdc )) return FALSE;
    fuPlanes &= ~WGL_SWAP_MAIN_PLANE;
  }

  if (fuPlanes) {
    WARN("Following layers unhandled: %08x\n", fuPlanes);
  }

  return TRUE;
}

/***********************************************************************
 *		wglAllocateMemoryNV
 *
 * Provided by the WGL_NV_vertex_array_range extension.
 */
void * WINAPI wglAllocateMemoryNV( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_wglAllocateMemoryNV) return NULL;
    return funcs->ext.p_wglAllocateMemoryNV( size, readfreq, writefreq, priority );
}

/***********************************************************************
 *		wglFreeMemoryNV
 *
 * Provided by the WGL_NV_vertex_array_range extension.
 */
void WINAPI wglFreeMemoryNV( void *pointer )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (funcs->ext.p_wglFreeMemoryNV) funcs->ext.p_wglFreeMemoryNV( pointer );
}

/***********************************************************************
 *		wglBindTexImageARB
 *
 * Provided by the WGL_ARB_render_texture extension.
 */
BOOL WINAPI wglBindTexImageARB( HPBUFFERARB handle, int buffer )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglBindTexImageARB( ptr->u.pbuffer, buffer );
    release_handle_ptr( ptr );
    return ret;
}

/***********************************************************************
 *		wglReleaseTexImageARB
 *
 * Provided by the WGL_ARB_render_texture extension.
 */
BOOL WINAPI wglReleaseTexImageARB( HPBUFFERARB handle, int buffer )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglReleaseTexImageARB( ptr->u.pbuffer, buffer );
    release_handle_ptr( ptr );
    return ret;
}

/***********************************************************************
 *		wglSetPbufferAttribARB
 *
 * Provided by the WGL_ARB_render_texture extension.
 */
BOOL WINAPI wglSetPbufferAttribARB( HPBUFFERARB handle, const int *attribs )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglSetPbufferAttribARB( ptr->u.pbuffer, attribs );
    release_handle_ptr( ptr );
    return ret;
}

/***********************************************************************
 *		wglChoosePixelFormatARB
 *
 * Provided by the WGL_ARB_pixel_format extension.
 */
BOOL WINAPI wglChoosePixelFormatARB( HDC hdc, const int *iattribs, const FLOAT *fattribs,
                                     UINT max, int *formats, UINT *count )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglChoosePixelFormatARB) return FALSE;
    return funcs->ext.p_wglChoosePixelFormatARB( hdc, iattribs, fattribs, max, formats, count );
}

/***********************************************************************
 *		wglGetPixelFormatAttribivARB
 *
 * Provided by the WGL_ARB_pixel_format extension.
 */
BOOL WINAPI wglGetPixelFormatAttribivARB( HDC hdc, int format, int layer, UINT count, const int *attribs,
                                          int *values )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglGetPixelFormatAttribivARB) return FALSE;
    return funcs->ext.p_wglGetPixelFormatAttribivARB( hdc, format, layer, count, attribs, values );
}

/***********************************************************************
 *		wglGetPixelFormatAttribfvARB
 *
 * Provided by the WGL_ARB_pixel_format extension.
 */
BOOL WINAPI wglGetPixelFormatAttribfvARB( HDC hdc, int format, int layer, UINT count, const int *attribs,
                                          FLOAT *values )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglGetPixelFormatAttribfvARB) return FALSE;
    return funcs->ext.p_wglGetPixelFormatAttribfvARB( hdc, format, layer, count, attribs, values );
}

/***********************************************************************
 *		wglCreatePbufferARB
 *
 * Provided by the WGL_ARB_pbuffer extension.
 */
HPBUFFERARB WINAPI wglCreatePbufferARB( HDC hdc, int format, int width, int height, const int *attribs )
{
    HPBUFFERARB ret = 0;
    struct wgl_pbuffer *pbuffer;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglCreatePbufferARB) return 0;
    if (!(pbuffer = funcs->ext.p_wglCreatePbufferARB( hdc, format, width, height, attribs ))) return 0;
    ret = alloc_handle( HANDLE_PBUFFER, funcs, pbuffer );
    if (!ret) funcs->ext.p_wglDestroyPbufferARB( pbuffer );
    return ret;
}

/***********************************************************************
 *		wglGetPbufferDCARB
 *
 * Provided by the WGL_ARB_pbuffer extension.
 */
HDC WINAPI wglGetPbufferDCARB( HPBUFFERARB handle )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    HDC ret;

    if (!ptr) return 0;
    ret = ptr->funcs->ext.p_wglGetPbufferDCARB( ptr->u.pbuffer );
    release_handle_ptr( ptr );
    return ret;
}

/***********************************************************************
 *		wglReleasePbufferDCARB
 *
 * Provided by the WGL_ARB_pbuffer extension.
 */
int WINAPI wglReleasePbufferDCARB( HPBUFFERARB handle, HDC hdc )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglReleasePbufferDCARB( ptr->u.pbuffer, hdc );
    release_handle_ptr( ptr );
    return ret;
}

/***********************************************************************
 *		wglDestroyPbufferARB
 *
 * Provided by the WGL_ARB_pbuffer extension.
 */
BOOL WINAPI wglDestroyPbufferARB( HPBUFFERARB handle )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );

    if (!ptr) return FALSE;
    ptr->funcs->ext.p_wglDestroyPbufferARB( ptr->u.pbuffer );
    free_handle_ptr( ptr );
    return TRUE;
}

/***********************************************************************
 *		wglQueryPbufferARB
 *
 * Provided by the WGL_ARB_pbuffer extension.
 */
BOOL WINAPI wglQueryPbufferARB( HPBUFFERARB handle, int attrib, int *value )
{
    struct wgl_handle *ptr = get_handle_ptr( handle, HANDLE_PBUFFER );
    BOOL ret;

    if (!ptr) return FALSE;
    ret = ptr->funcs->ext.p_wglQueryPbufferARB( ptr->u.pbuffer, attrib, value );
    release_handle_ptr( ptr );
    return ret;
}

/***********************************************************************
 *		wglGetExtensionsStringARB
 *
 * Provided by the WGL_ARB_extensions_string extension.
 */
const char * WINAPI wglGetExtensionsStringARB( HDC hdc )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglGetExtensionsStringARB) return NULL;
    return (const char *)funcs->ext.p_wglGetExtensionsStringARB( hdc );
}

/***********************************************************************
 *		wglGetExtensionsStringEXT
 *
 * Provided by the WGL_EXT_extensions_string extension.
 */
const char * WINAPI wglGetExtensionsStringEXT(void)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_wglGetExtensionsStringEXT) return NULL;
    return (const char *)funcs->ext.p_wglGetExtensionsStringEXT();
}

/***********************************************************************
 *		wglSwapIntervalEXT
 *
 * Provided by the WGL_EXT_swap_control extension.
 */
BOOL WINAPI wglSwapIntervalEXT( int interval )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_wglSwapIntervalEXT) return FALSE;
    return funcs->ext.p_wglSwapIntervalEXT( interval );
}

/***********************************************************************
 *		wglGetSwapIntervalEXT
 *
 * Provided by the WGL_EXT_swap_control extension.
 */
int WINAPI wglGetSwapIntervalEXT(void)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_wglGetSwapIntervalEXT) return FALSE;
    return funcs->ext.p_wglGetSwapIntervalEXT();
}

/***********************************************************************
 *		wglSetPixelFormatWINE
 *
 * Provided by the WGL_WINE_pixel_format_passthrough extension.
 */
BOOL WINAPI wglSetPixelFormatWINE( HDC hdc, int format )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglSetPixelFormatWINE) return FALSE;
    return funcs->ext.p_wglSetPixelFormatWINE( hdc, format );
}

/***********************************************************************
 *		wglUseFontBitmaps_common
 */
static BOOL wglUseFontBitmaps_common( HDC hdc, DWORD first, DWORD count, DWORD listBase, BOOL unicode )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
     GLYPHMETRICS gm;
     unsigned int glyph, size = 0;
     void *bitmap = NULL, *gl_bitmap = NULL;
     int org_alignment;
     BOOL ret = TRUE;

     funcs->gl.p_glGetIntegerv(GL_UNPACK_ALIGNMENT, &org_alignment);
     funcs->gl.p_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

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
         if (needed_size != 0) {
             if (unicode)
                 ret = (GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm,
                                         size, bitmap, &identity) != GDI_ERROR);
             else
                 ret = (GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm,
                                         size, bitmap, &identity) != GDI_ERROR);
             if (!ret) break;
         }

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

         funcs->gl.p_glNewList(listBase++, GL_COMPILE);
         if (needed_size != 0) {
             funcs->gl.p_glBitmap(gm.gmBlackBoxX, gm.gmBlackBoxY,
                     0 - gm.gmptGlyphOrigin.x, (int) gm.gmBlackBoxY - gm.gmptGlyphOrigin.y,
                     gm.gmCellIncX, gm.gmCellIncY,
                     gl_bitmap);
         } else {
             /* This is the case of 'empty' glyphs like the space character */
             funcs->gl.p_glBitmap(0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL);
         }
         funcs->gl.p_glEndList();
     }

     funcs->gl.p_glPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
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

/* FIXME: should probably have a glu.h header */

typedef struct GLUtesselator GLUtesselator;
typedef void (WINAPI *_GLUfuncptr)(void);

#define GLU_TESS_BEGIN  100100
#define GLU_TESS_VERTEX 100101
#define GLU_TESS_END    100102

static GLUtesselator * (WINAPI *pgluNewTess)(void);
static void (WINAPI *pgluDeleteTess)(GLUtesselator *tess);
static void (WINAPI *pgluTessNormal)(GLUtesselator *tess, GLdouble x, GLdouble y, GLdouble z);
static void (WINAPI *pgluTessBeginPolygon)(GLUtesselator *tess, void *polygon_data);
static void (WINAPI *pgluTessEndPolygon)(GLUtesselator *tess);
static void (WINAPI *pgluTessCallback)(GLUtesselator *tess, GLenum which, _GLUfuncptr fn);
static void (WINAPI *pgluTessBeginContour)(GLUtesselator *tess);
static void (WINAPI *pgluTessEndContour)(GLUtesselator *tess);
static void (WINAPI *pgluTessVertex)(GLUtesselator *tess, GLdouble *location, GLvoid* data);

static HMODULE load_libglu(void)
{
    static const WCHAR glu32W[] = {'g','l','u','3','2','.','d','l','l',0};
    static BOOL already_loaded;
    static HMODULE module;

    if (already_loaded) return module;
    already_loaded = TRUE;

    TRACE("Trying to load GLU library\n");
    module = LoadLibraryW( glu32W );
    if (!module)
    {
        WARN("Failed to load glu32\n");
        return NULL;
    }
#define LOAD_FUNCPTR(f) p##f = (void *)GetProcAddress( module, #f )
    LOAD_FUNCPTR(gluNewTess);
    LOAD_FUNCPTR(gluDeleteTess);
    LOAD_FUNCPTR(gluTessBeginContour);
    LOAD_FUNCPTR(gluTessNormal);
    LOAD_FUNCPTR(gluTessBeginPolygon);
    LOAD_FUNCPTR(gluTessCallback);
    LOAD_FUNCPTR(gluTessEndContour);
    LOAD_FUNCPTR(gluTessEndPolygon);
    LOAD_FUNCPTR(gluTessVertex);
#undef LOAD_FUNCPTR
    return module;
}

static void fixed_to_double(POINTFX fixed, UINT em_size, GLdouble vertex[3])
{
    vertex[0] = (fixed.x.value + (GLdouble)fixed.x.fract / (1 << 16)) / em_size;  
    vertex[1] = (fixed.y.value + (GLdouble)fixed.y.fract / (1 << 16)) / em_size;  
    vertex[2] = 0.0;
}

static void WINAPI tess_callback_vertex(GLvoid *vertex)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    GLdouble *dbl = vertex;
    TRACE("%f, %f, %f\n", dbl[0], dbl[1], dbl[2]);
    funcs->gl.p_glVertex3dv(vertex);
}

static void WINAPI tess_callback_begin(GLenum which)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    TRACE("%d\n", which);
    funcs->gl.p_glBegin(which);
}

static void WINAPI tess_callback_end(void)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    TRACE("\n");
    funcs->gl.p_glEnd();
}

typedef struct _bezier_vector {
    GLdouble x;
    GLdouble y;
} bezier_vector;

static double bezier_deviation_squared(const bezier_vector *p)
{
    bezier_vector deviation;
    bezier_vector vertex;
    bezier_vector base;
    double base_length;
    double dot;

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4 - p[0].x;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4 - p[0].y;

    base.x = p[2].x - p[0].x;
    base.y = p[2].y - p[0].y;

    base_length = sqrt(base.x*base.x + base.y*base.y);
    base.x /= base_length;
    base.y /= base_length;

    dot = base.x*vertex.x + base.y*vertex.y;
    dot = min(max(dot, 0.0), base_length);
    base.x *= dot;
    base.y *= dot;

    deviation.x = vertex.x-base.x;
    deviation.y = vertex.y-base.y;

    return deviation.x*deviation.x + deviation.y*deviation.y;
}

static int bezier_approximate(const bezier_vector *p, bezier_vector *points, FLOAT deviation)
{
    bezier_vector first_curve[3];
    bezier_vector second_curve[3];
    bezier_vector vertex;
    int total_vertices;

    if(bezier_deviation_squared(p) <= deviation*deviation)
    {
        if(points)
            *points = p[2];
        return 1;
    }

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4;

    first_curve[0] = p[0];
    first_curve[1].x = (p[0].x + p[1].x)/2;
    first_curve[1].y = (p[0].y + p[1].y)/2;
    first_curve[2] = vertex;

    second_curve[0] = vertex;
    second_curve[1].x = (p[2].x + p[1].x)/2;
    second_curve[1].y = (p[2].y + p[1].y)/2;
    second_curve[2] = p[2];

    total_vertices = bezier_approximate(first_curve, points, deviation);
    if(points)
        points += total_vertices;
    total_vertices += bezier_approximate(second_curve, points, deviation);
    return total_vertices;
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
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    UINT glyph;
    const MAT2 identity = {{0,1},{0,0},{0,0},{0,1}};
    GLUtesselator *tess = NULL;
    LOGFONTW lf;
    HFONT old_font, unscaled_font;
    UINT em_size = 1024;
    RECT rc;

    TRACE("(%p, %d, %d, %d, %f, %f, %d, %p, %s)\n", hdc, first, count,
          listBase, deviation, extrusion, format, lpgmf, unicode ? "W" : "A");

    if(deviation <= 0.0)
        deviation = 1.0/em_size;

    if(format == WGL_FONT_POLYGONS)
    {
        if (!load_libglu())
        {
            ERR("glu32 is required for this function but isn't available\n");
            return FALSE;
        }

        tess = pgluNewTess();
        if(!tess) return FALSE;
        pgluTessCallback(tess, GLU_TESS_VERTEX, (_GLUfuncptr)tess_callback_vertex);
        pgluTessCallback(tess, GLU_TESS_BEGIN, (_GLUfuncptr)tess_callback_begin);
        pgluTessCallback(tess, GLU_TESS_END, tess_callback_end);
    }

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
        GLdouble *vertices = NULL;
        int vertex_total = -1;

        if(unicode)
            needed = GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);
        else
            needed = GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);

        if(needed == GDI_ERROR)
            goto error;

        buf = HeapAlloc(GetProcessHeap(), 0, needed);

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

        funcs->gl.p_glNewList(listBase++, GL_COMPILE);
        funcs->gl.p_glFrontFace(GL_CCW);
        if(format == WGL_FONT_POLYGONS)
        {
            funcs->gl.p_glNormal3d(0.0, 0.0, 1.0);
            pgluTessNormal(tess, 0, 0, 1);
            pgluTessBeginPolygon(tess, NULL);
        }

        while(!vertices)
        {
            if(vertex_total != -1)
                vertices = HeapAlloc(GetProcessHeap(), 0, vertex_total * 3 * sizeof(GLdouble));
            vertex_total = 0;

            pph = (TTPOLYGONHEADER*)buf;
            while((BYTE*)pph < buf + needed)
            {
                GLdouble previous[3];
                fixed_to_double(pph->pfxStart, em_size, previous);

                if(vertices)
                    TRACE("\tstart %d, %d\n", pph->pfxStart.x.value, pph->pfxStart.y.value);

                if(format == WGL_FONT_POLYGONS)
                    pgluTessBeginContour(tess);
                else
                    funcs->gl.p_glBegin(GL_LINE_LOOP);

                if(vertices)
                {
                    fixed_to_double(pph->pfxStart, em_size, vertices);
                    if(format == WGL_FONT_POLYGONS)
                        pgluTessVertex(tess, vertices, vertices);
                    else
                        funcs->gl.p_glVertex3d(vertices[0], vertices[1], vertices[2]);
                    vertices += 3;
                }
                vertex_total++;

                ppc = (TTPOLYCURVE*)((char*)pph + sizeof(*pph));
                while((char*)ppc < (char*)pph + pph->cb)
                {
                    int i, j;
                    int num;

                    switch(ppc->wType) {
                    case TT_PRIM_LINE:
                        for(i = 0; i < ppc->cpfx; i++)
                        {
                            if(vertices)
                            {
                                TRACE("\t\tline to %d, %d\n",
                                      ppc->apfx[i].x.value, ppc->apfx[i].y.value);
                                fixed_to_double(ppc->apfx[i], em_size, vertices);
                                if(format == WGL_FONT_POLYGONS)
                                    pgluTessVertex(tess, vertices, vertices);
                                else
                                    funcs->gl.p_glVertex3d(vertices[0], vertices[1], vertices[2]);
                                vertices += 3;
                            }
                            fixed_to_double(ppc->apfx[i], em_size, previous);
                            vertex_total++;
                        }
                        break;

                    case TT_PRIM_QSPLINE:
                        for(i = 0; i < ppc->cpfx-1; i++)
                        {
                            bezier_vector curve[3];
                            bezier_vector *points;
                            GLdouble curve_vertex[3];

                            if(vertices)
                                TRACE("\t\tcurve  %d,%d %d,%d\n",
                                      ppc->apfx[i].x.value,     ppc->apfx[i].y.value,
                                      ppc->apfx[i + 1].x.value, ppc->apfx[i + 1].y.value);

                            curve[0].x = previous[0];
                            curve[0].y = previous[1];
                            fixed_to_double(ppc->apfx[i], em_size, curve_vertex);
                            curve[1].x = curve_vertex[0];
                            curve[1].y = curve_vertex[1];
                            fixed_to_double(ppc->apfx[i + 1], em_size, curve_vertex);
                            curve[2].x = curve_vertex[0];
                            curve[2].y = curve_vertex[1];
                            if(i < ppc->cpfx-2)
                            {
                                curve[2].x = (curve[1].x + curve[2].x)/2;
                                curve[2].y = (curve[1].y + curve[2].y)/2;
                            }
                            num = bezier_approximate(curve, NULL, deviation);
                            points = HeapAlloc(GetProcessHeap(), 0, num*sizeof(bezier_vector));
                            num = bezier_approximate(curve, points, deviation);
                            vertex_total += num;
                            if(vertices)
                            {
                                for(j=0; j<num; j++)
                                {
                                    TRACE("\t\t\tvertex at %f,%f\n", points[j].x, points[j].y);
                                    vertices[0] = points[j].x;
                                    vertices[1] = points[j].y;
                                    vertices[2] = 0.0;
                                    if(format == WGL_FONT_POLYGONS)
                                        pgluTessVertex(tess, vertices, vertices);
                                    else
                                        funcs->gl.p_glVertex3d(vertices[0], vertices[1], vertices[2]);
                                    vertices += 3;
                                }
                            }
                            HeapFree(GetProcessHeap(), 0, points);
                            previous[0] = curve[2].x;
                            previous[1] = curve[2].y;
                        }
                        break;
                    default:
                        ERR("\t\tcurve type = %d\n", ppc->wType);
                        if(format == WGL_FONT_POLYGONS)
                            pgluTessEndContour(tess);
                        else
                            funcs->gl.p_glEnd();
                        goto error_in_list;
                    }

                    ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
                                         (ppc->cpfx - 1) * sizeof(POINTFX));
                }
                if(format == WGL_FONT_POLYGONS)
                    pgluTessEndContour(tess);
                else
                    funcs->gl.p_glEnd();
                pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
            }
        }

error_in_list:
        if(format == WGL_FONT_POLYGONS)
            pgluTessEndPolygon(tess);
        funcs->gl.p_glTranslated((GLdouble)gm.gmCellIncX / em_size, (GLdouble)gm.gmCellIncY / em_size, 0.0);
        funcs->gl.p_glEndList();
        HeapFree(GetProcessHeap(), 0, buf);
        HeapFree(GetProcessHeap(), 0, vertices);
    }

 error:
    DeleteObject(SelectObject(hdc, old_font));
    if(format == WGL_FONT_POLYGONS)
        pgluDeleteTess(tess);
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
GLint WINAPI glDebugEntry( GLint unknown1, GLint unknown2 )
{
    return 0;
}

static GLubyte *filter_extensions_list(const char *extensions, const char *disabled)
{
    char *p, *str = NULL;
    const char *end;

    p = str = HeapAlloc(GetProcessHeap(), 0, strlen(extensions) + 2);
    if (!str)
        return NULL;
    for (;;)
    {
        while (*extensions == ' ')
            extensions++;
        if (!*extensions)
            break;
        if (!(end = strchr(extensions, ' ')))
            end = extensions + strlen(extensions);
        memcpy(p, extensions, end - extensions);
        p[end - extensions] = 0;
        if (!has_extension(disabled, p, strlen(p)))
        {
            TRACE("++ %s\n", p);
            p += end - extensions;
            *p++ = ' ';
        }
        else
        {
            TRACE("-- %s (disabled by config)\n", p);
        }
        extensions = end;
    }
    *p = 0;
    return (GLubyte *)str;
}

static GLuint *filter_extensions_index(const char *disabled)
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const char *ext, *end, *gl_ext;
    GLuint *disabled_exts = NULL, *new_disabled_exts;
    unsigned int i = 0, j, disabled_size;
    GLint extensions_count;

    if (!funcs->ext.p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->ext.p_glGetStringi;

        *func_ptr = funcs->wgl.p_wglGetProcAddress("glGetStringi");
        if (!funcs->ext.p_glGetStringi)
            return NULL;
    }

    funcs->gl.p_glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);
    disabled_size = 2;
    disabled_exts = HeapAlloc(GetProcessHeap(), 0, disabled_size * sizeof(*disabled_exts));
    if (!disabled_exts)
        return NULL;
    for (j = 0; j < extensions_count; ++j)
    {
        gl_ext = (const char *)funcs->ext.p_glGetStringi(GL_EXTENSIONS, j);
        ext = disabled;
        for (;;)
        {
            while (*ext == ' ')
                ext++;
            if (!*ext)
            {
                TRACE("++ %s\n", gl_ext);
                break;
            }
            if (!(end = strchr(ext, ' ')))
                end = ext + strlen(ext);

            if (!strncmp(gl_ext, ext, end - ext) && !gl_ext[end - ext])
            {
                if (i + 1 == disabled_size)
                {
                    disabled_size *= 2;
                    new_disabled_exts = HeapReAlloc(GetProcessHeap(), 0, disabled_exts,
                                                    disabled_size * sizeof(*disabled_exts));
                    if (!new_disabled_exts)
                    {
                        disabled_exts[i] = ~0u;
                        return disabled_exts;
                    }
                    disabled_exts = new_disabled_exts;
                }
                TRACE("-- %s (disabled by config)\n", gl_ext);
                disabled_exts[i++] = j;
                break;
            }
            ext = end;
        }
    }
    disabled_exts[i] = ~0u;
    return disabled_exts;
}

/* build the extension string by filtering out the disabled extensions */
static BOOL filter_extensions(const char *extensions, GLubyte **exts_list, GLuint **disabled_exts)
{
    static const char *disabled;

    TRACE( "GL_EXTENSIONS:\n" );

    if (!disabled)
    {
        HKEY hkey;
        DWORD size;
        char *str = NULL;

        /* @@ Wine registry key: HKCU\Software\Wine\OpenGL */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\OpenGL", &hkey ))
        {
            if (!RegQueryValueExA( hkey, "DisabledExtensions", 0, NULL, NULL, &size ))
            {
                str = HeapAlloc( GetProcessHeap(), 0, size );
                if (RegQueryValueExA( hkey, "DisabledExtensions", 0, NULL, (BYTE *)str, &size )) *str = 0;
            }
            RegCloseKey( hkey );
        }
        if (str)
        {
            if (InterlockedCompareExchangePointer( (void **)&disabled, str, NULL ))
                HeapFree( GetProcessHeap(), 0, str );
        }
        else disabled = "";
    }

    if (!disabled[0])
        return FALSE;

    if (extensions && !*exts_list)
        *exts_list = filter_extensions_list(extensions, disabled);

    if (!*disabled_exts)
        *disabled_exts = filter_extensions_index(disabled);

    return (exts_list && *exts_list) || *disabled_exts;
}

/***********************************************************************
 *              glGetString (OPENGL32.@)
 */
const GLubyte * WINAPI glGetString( GLenum name )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const GLubyte *ret = funcs->gl.p_glGetString( name );

    if (name == GL_EXTENSIONS && ret)
    {
        struct wgl_handle *ptr = get_current_context_ptr();
        if (ptr->u.context->extensions ||
            filter_extensions((const char *)ret, &ptr->u.context->extensions, &ptr->u.context->disabled_exts))
            ret = ptr->u.context->extensions;
    }
    return ret;
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        NtCurrentTeb()->glTable = &null_opengl_funcs;
        break;
    case DLL_THREAD_ATTACH:
        NtCurrentTeb()->glTable = &null_opengl_funcs;
        break;
    }
    return TRUE;
}
