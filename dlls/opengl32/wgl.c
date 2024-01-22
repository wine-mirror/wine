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

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntuser.h"
#include "malloc.h"

#include "unixlib.h"
#include "private.h"

#include "wine/glu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);
WINE_DECLARE_DEBUG_CHANNEL(fps);

static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

#ifndef _WIN64

static char **wow64_strings;
static SIZE_T wow64_strings_count;

static CRITICAL_SECTION wow64_cs;
static CRITICAL_SECTION_DEBUG wow64_cs_debug =
{
    0, 0, &wow64_cs,
    { &wow64_cs_debug.ProcessLocksList, &wow64_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wow64_cs") }
};
static CRITICAL_SECTION wow64_cs = { &wow64_cs_debug, -1, 0, 0, 0, 0 };

static void append_wow64_string( char *str )
{
    char **tmp;

    EnterCriticalSection( &wow64_cs );

    if (!(tmp = realloc( wow64_strings, (wow64_strings_count + 1) * sizeof(*wow64_strings) )))
        ERR( "Failed to allocate memory for wow64 strings\n" );
    else
    {
        wow64_strings = tmp;
        wow64_strings[wow64_strings_count] = str;
        wow64_strings_count += 1;
    }

    LeaveCriticalSection( &wow64_cs );
}

static void cleanup_wow64_strings(void)
{
    while (wow64_strings_count--) free( wow64_strings[wow64_strings_count] );
    free( wow64_strings );
}

#endif

/***********************************************************************
 *		wglGetCurrentReadDCARB
 *
 * Provided by the WGL_ARB_make_current_read extension.
 */
HDC WINAPI wglGetCurrentReadDCARB(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return 0;
    return NtCurrentTeb()->glReserved1[1];
}

/***********************************************************************
 *		wglGetCurrentDC (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentDC(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return 0;
    return NtCurrentTeb()->glReserved1[0];
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

    TRACE( "%p %p: size %u version %u flags %lu type %u color %u %u,%u,%u,%u "
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

        if ((ppfd->iPixelType == PFD_TYPE_COLORINDEX) != (format.iPixelType == PFD_TYPE_COLORINDEX))
        {
            TRACE( "pixel type mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        if ((ppfd->dwFlags & PFD_DRAW_TO_BITMAP) && !(format.dwFlags & PFD_DRAW_TO_BITMAP))
        {
            TRACE( "PFD_DRAW_TO_BITMAP required but not found for iPixelFormat=%d\n", i );
            continue;
        }
        if ((ppfd->dwFlags & PFD_DRAW_TO_WINDOW) && !(format.dwFlags & PFD_DRAW_TO_WINDOW))
        {
            TRACE( "PFD_DRAW_TO_WINDOW required but not found for iPixelFormat=%d\n", i );
            continue;
        }

        if ((ppfd->dwFlags & PFD_SUPPORT_GDI) && !(format.dwFlags & PFD_SUPPORT_GDI))
        {
            TRACE( "PFD_SUPPORT_GDI required but not found for iPixelFormat=%d\n", i );
            continue;
        }
        if ((ppfd->dwFlags & PFD_SUPPORT_OPENGL) && !(format.dwFlags & PFD_SUPPORT_OPENGL))
        {
            TRACE( "PFD_SUPPORT_OPENGL required but not found for iPixelFormat=%d\n", i );
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
        else if (!best_format)
            goto found;

        /* Stereo, see the comments above. */
        if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_STEREO) != bestStereo) &&
                ((format.dwFlags & PFD_STEREO) == (ppfd->dwFlags & PFD_STEREO)))
                goto found;

            if (bestStereo != -1 && (format.dwFlags & PFD_STEREO) != bestStereo) continue;
        }
        else if (!best_format)
            goto found;

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
        if (ppfd->cDepthBits && !(ppfd->dwFlags & PFD_DEPTH_DONTCARE))
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
        if (ppfd->dwFlags & PFD_DEPTH_DONTCARE && format.cDepthBits < best.cDepthBits)
            goto found;

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
    struct wglGetPixelFormat_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    NTSTATUS status;

    TRACE( "hdc %p\n", hdc );

    if ((status = UNIX_CALL( wglGetPixelFormat, &args )))
    {
        WARN( "wglGetPixelFormat returned %#lx\n", status );
        SetLastError( ERROR_INVALID_PIXEL_FORMAT );
    }

    return args.ret;
}

/***********************************************************************
 *              wglSwapBuffers (OPENGL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH wglSwapBuffers( HDC hdc )
{
    struct wglSwapBuffers_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    NTSTATUS status;

    if ((status = UNIX_CALL( wglSwapBuffers, &args ))) WARN( "wglSwapBuffers returned %#lx\n", status );
    else if (TRACE_ON(fps))
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

    return args.ret;
}

/***********************************************************************
 *		wglCreateLayerContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateLayerContext( HDC hdc, int iLayerPlane )
{
    TRACE("(%p,%d)\n", hdc, iLayerPlane);

    if (iLayerPlane == 0) return wglCreateContext( hdc );

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

/***********************************************************************
 *		wglGetProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetProcAddress( LPCSTR name )
{
    struct wglGetProcAddress_params args = { .teb = NtCurrentTeb(), .lpszProc = name };
    const void *proc;
    NTSTATUS status;

    if (!name) return NULL;
    if ((status = UNIX_CALL( wglGetProcAddress, &args )))
        WARN( "wglGetProcAddress %s returned %#lx\n", debugstr_a(name), status );
    if (args.ret == (void *)-1) return NULL;

    proc = extension_procs[(UINT_PTR)args.ret];
    TRACE( "returning %s -> %p\n", name, proc );
    return proc;
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
 *		wglGetDefaultProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetDefaultProcAddress( LPCSTR name )
{
    FIXME( "%s: stub\n", debugstr_a(name));
    return NULL;
}

/***********************************************************************
 *		wglSwapLayerBuffers (OPENGL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH wglSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
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
 *		wglUseFontBitmaps_common
 */
static BOOL wglUseFontBitmaps_common( HDC hdc, DWORD first, DWORD count, DWORD listBase, BOOL unicode )
{
     GLYPHMETRICS gm;
     unsigned int glyph, size = 0;
     void *bitmap = NULL, *gl_bitmap = NULL;
     int org_alignment;
     BOOL ret = TRUE;

     glGetIntegerv( GL_UNPACK_ALIGNMENT, &org_alignment );
     glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

     for (glyph = first; glyph < first + count; glyph++) {
         unsigned int needed_size, height, width, width_int;

         if (unicode)
             needed_size = GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);
         else
             needed_size = GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);

         TRACE("Glyph: %3d / List: %ld size %d\n", glyph, listBase, needed_size);
         if (needed_size == GDI_ERROR) {
             ret = FALSE;
             break;
         }

         if (needed_size > size) {
             size = needed_size;
             free( bitmap );
             free( gl_bitmap );
             bitmap = calloc( 1, size );
             gl_bitmap = calloc( 1, size );
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

         if (TRACE_ON(opengl))
         {
             unsigned int bitmask;
             unsigned char *bitmap_ = bitmap;

             TRACE("  - bbox: %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
             TRACE("  - origin: (%ld, %ld)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
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

         glNewList( listBase++, GL_COMPILE );
         if (needed_size != 0) {
             glBitmap( gm.gmBlackBoxX, gm.gmBlackBoxY, 0 - gm.gmptGlyphOrigin.x,
                       (int)gm.gmBlackBoxY - gm.gmptGlyphOrigin.y, gm.gmCellIncX, gm.gmCellIncY, gl_bitmap );
         } else {
             /* This is the case of 'empty' glyphs like the space character */
             glBitmap( 0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL );
         }
         glEndList();
     }

     glPixelStorei( GL_UNPACK_ALIGNMENT, org_alignment );
     free( bitmap );
     free( gl_bitmap );
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

static void fixed_to_double(POINTFX fixed, UINT em_size, GLdouble vertex[3])
{
    vertex[0] = (fixed.x.value + (GLdouble)fixed.x.fract / (1 << 16)) / em_size;
    vertex[1] = (fixed.y.value + (GLdouble)fixed.y.fract / (1 << 16)) / em_size;
    vertex[2] = 0.0;
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
    UINT glyph;
    GLUtesselator *tess = NULL;
    LOGFONTW lf;
    HFONT old_font, unscaled_font;
    UINT em_size = 1024;
    RECT rc;

    TRACE("(%p, %ld, %ld, %ld, %f, %f, %d, %p, %s)\n", hdc, first, count,
          listBase, deviation, extrusion, format, lpgmf, unicode ? "W" : "A");

    if(deviation <= 0.0)
        deviation = 1.0/em_size;

    if(format == WGL_FONT_POLYGONS)
    {
        tess = gluNewTess();
        if(!tess)
        {
            ERR("glu32 is required for this function but isn't available\n");
            return FALSE;
        }
        gluTessCallback( tess, GLU_TESS_VERTEX, (void *)glVertex3dv );
        gluTessCallback( tess, GLU_TESS_BEGIN, (void *)glBegin );
        gluTessCallback( tess, GLU_TESS_END, glEnd );
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

        buf = malloc( needed );

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

        glNewList( listBase++, GL_COMPILE );
        glFrontFace( GL_CCW );
        if(format == WGL_FONT_POLYGONS)
        {
            glNormal3d( 0.0, 0.0, 1.0 );
            gluTessNormal(tess, 0, 0, 1);
            gluTessBeginPolygon(tess, NULL);
        }

        while(!vertices)
        {
            if (vertex_total != -1) vertices = malloc( vertex_total * 3 * sizeof(GLdouble) );
            vertex_total = 0;

            pph = (TTPOLYGONHEADER*)buf;
            while((BYTE*)pph < buf + needed)
            {
                GLdouble previous[3];
                fixed_to_double(pph->pfxStart, em_size, previous);

                if(vertices)
                    TRACE("\tstart %d, %d\n", pph->pfxStart.x.value, pph->pfxStart.y.value);

                if (format == WGL_FONT_POLYGONS) gluTessBeginContour( tess );
                else glBegin( GL_LINE_LOOP );

                if(vertices)
                {
                    fixed_to_double(pph->pfxStart, em_size, vertices);
                    if (format == WGL_FONT_POLYGONS) gluTessVertex( tess, vertices, vertices );
                    else glVertex3d( vertices[0], vertices[1], vertices[2] );
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
                                if (format == WGL_FONT_POLYGONS) gluTessVertex( tess, vertices, vertices );
                                else glVertex3d( vertices[0], vertices[1], vertices[2] );
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
                            points = malloc( num * sizeof(bezier_vector) );
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
                                    if (format == WGL_FONT_POLYGONS) gluTessVertex( tess, vertices, vertices );
                                    else glVertex3d( vertices[0], vertices[1], vertices[2] );
                                    vertices += 3;
                                }
                            }
                            free( points );
                            previous[0] = curve[2].x;
                            previous[1] = curve[2].y;
                        }
                        break;
                    default:
                        ERR("\t\tcurve type = %d\n", ppc->wType);
                        if (format == WGL_FONT_POLYGONS) gluTessEndContour( tess );
                        else glEnd();
                        goto error_in_list;
                    }

                    ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
                                         (ppc->cpfx - 1) * sizeof(POINTFX));
                }
                if (format == WGL_FONT_POLYGONS) gluTessEndContour( tess );
                else glEnd();
                pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
            }
        }

error_in_list:
        if (format == WGL_FONT_POLYGONS) gluTessEndPolygon( tess );
        glTranslated( (GLdouble)gm.gmCellIncX / em_size, (GLdouble)gm.gmCellIncY / em_size, 0.0 );
        glEndList();
        free( buf );
        free( vertices );
    }

 error:
    DeleteObject(SelectObject(hdc, old_font));
    if(format == WGL_FONT_POLYGONS)
        gluDeleteTess(tess);
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

const GLubyte * WINAPI glGetStringi( GLenum name, GLuint index )
{
    struct glGetStringi_params args =
    {
        .teb = NtCurrentTeb(),
        .name = name,
        .index = index,
    };
    NTSTATUS status;
#ifndef _WIN64
    GLubyte *wow64_str = NULL;
#endif

    TRACE( "name %d, index %d\n", name, index );

#ifndef _WIN64
    if (UNIX_CALL( glGetStringi, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( glGetStringi, &args ))) WARN( "glGetStringi returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( (char *)args.ret );
#endif
    return args.ret;
}

/***********************************************************************
 *              glGetString (OPENGL32.@)
 */
const GLubyte * WINAPI glGetString( GLenum name )
{
    struct glGetString_params args = { .teb = NtCurrentTeb(), .name = name };
    NTSTATUS status;
#ifndef _WIN64
    GLubyte *wow64_str = NULL;
#endif

    TRACE( "name %d\n", name );

#ifndef _WIN64
    if (UNIX_CALL( glGetString, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( glGetString, &args ))) WARN( "glGetString returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( (char *)args.ret );
#endif
    return args.ret;
}

const char * WINAPI wglGetExtensionsStringARB( HDC hdc )
{
    struct wglGetExtensionsStringARB_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    NTSTATUS status;
#ifndef _WIN64
    char *wow64_str = NULL;
#endif

    TRACE( "hdc %p\n", hdc );

#ifndef _WIN64
    if (UNIX_CALL( wglGetExtensionsStringARB, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( wglGetExtensionsStringARB, &args ))) WARN( "wglGetExtensionsStringARB returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( wow64_str );
#endif
    return args.ret;
}

const char * WINAPI wglGetExtensionsStringEXT(void)
{
    struct wglGetExtensionsStringEXT_params args = { .teb = NtCurrentTeb() };
    NTSTATUS status;
#ifndef _WIN64
    char *wow64_str = NULL;
#endif

    TRACE( "\n" );

#ifndef _WIN64
    if (UNIX_CALL( wglGetExtensionsStringEXT, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( wglGetExtensionsStringEXT, &args ))) WARN( "wglGetExtensionsStringEXT returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( wow64_str );
#endif
    return args.ret;
}

const GLchar * WINAPI wglQueryCurrentRendererStringWINE( GLenum attribute )
{
    struct wglQueryCurrentRendererStringWINE_params args = { .teb = NtCurrentTeb(), .attribute = attribute };
    NTSTATUS status;
#ifndef _WIN64
    char *wow64_str = NULL;
#endif

    TRACE( "attribute %d\n", attribute );

#ifndef _WIN64
    if (UNIX_CALL( wglQueryCurrentRendererStringWINE, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( wglQueryCurrentRendererStringWINE, &args ))) WARN( "wglQueryCurrentRendererStringWINE returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( wow64_str );
#endif
    return args.ret;
}

const GLchar * WINAPI wglQueryRendererStringWINE( HDC dc, GLint renderer, GLenum attribute )
{
    struct wglQueryRendererStringWINE_params args =
    {
        .teb = NtCurrentTeb(),
        .dc = dc,
        .renderer = renderer,
        .attribute = attribute,
    };
    NTSTATUS status;
#ifndef _WIN64
    char *wow64_str = NULL;
#endif

    TRACE( "dc %p, renderer %d, attribute %d\n", dc, renderer, attribute );

#ifndef _WIN64
    if (UNIX_CALL( wglQueryCurrentRendererStringWINE, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( wglQueryRendererStringWINE, &args ))) WARN( "wglQueryRendererStringWINE returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( wow64_str );
#endif
    return args.ret;
}

#ifndef _WIN64
static void *get_buffer_pointer( GLenum target )
{
    void (WINAPI *p_glGetBufferPointerv)( GLenum target, GLenum pname, void **params );
    void *ptr;
    if (!(p_glGetBufferPointerv = (void *)wglGetProcAddress( "glGetBufferPointerv" ))) return 0;
    p_glGetBufferPointerv( target, GL_BUFFER_MAP_POINTER, &ptr );
    return ptr;
}

static void *get_named_buffer_pointer( GLint buffer )
{
    void (WINAPI *p_glGetNamedBufferPointerv)( GLuint buffer, GLenum pname, void **params );
    void *ptr;
    if (!(p_glGetNamedBufferPointerv = (void *)wglGetProcAddress( "glGetNamedBufferPointerv" ))) return 0;
    p_glGetNamedBufferPointerv( buffer, GL_BUFFER_MAP_POINTER, &ptr );
    return ptr;
}
#endif

static void *gl_map_buffer( enum unix_funcs code, GLenum target, GLenum access )
{
    struct glMapBuffer_params args =
    {
        .teb = NtCurrentTeb(),
        .target = target,
        .access = access,
    };
    NTSTATUS status;

    TRACE( "target %d, access %d\n", target, access );

    if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
#ifndef _WIN64
    if (status == STATUS_INVALID_ADDRESS)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.ret = _aligned_malloc( (size_t)args.ret, 16 ))) status = STATUS_NO_MEMORY;
        else if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
        _aligned_free( args.ret );
    }
#endif
    WARN( "glMapBuffer returned %#lx\n", status );
    return args.ret;
}

void * WINAPI glMapBuffer( GLenum target, GLenum access )
{
    return gl_map_buffer( unix_glMapBuffer, target, access );
}

void * WINAPI glMapBufferARB( GLenum target, GLenum access )
{
    return gl_map_buffer( unix_glMapBufferARB, target, access );
}

void * WINAPI glMapBufferRange( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    struct glMapBufferRange_params args =
    {
        .teb = NtCurrentTeb(),
        .target = target,
        .offset = offset,
        .length = length,
        .access = access,
    };
    NTSTATUS status;

    TRACE( "target %d, offset %Id, length %Id, access %d\n", target, offset, length, access );

    if (!(status = UNIX_CALL( glMapBufferRange, &args ))) return args.ret;
#ifndef _WIN64
    if (status == STATUS_INVALID_ADDRESS)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.ret = _aligned_malloc( length, 16 ))) status = STATUS_NO_MEMORY;
        else if (!(status = UNIX_CALL( glMapBufferRange, &args ))) return args.ret;
        _aligned_free( args.ret );
    }
#endif
    WARN( "glMapBufferRange returned %#lx\n", status );
    return args.ret;
}

static void *gl_map_named_buffer( enum unix_funcs code, GLuint buffer, GLenum access )
{
    struct glMapNamedBuffer_params args =
    {
        .teb = NtCurrentTeb(),
        .buffer = buffer,
        .access = access,
    };
    NTSTATUS status;

    TRACE( "(%d, %d)\n", buffer, access );

    if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
#ifndef _WIN64
    if (status == STATUS_INVALID_ADDRESS)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.ret = _aligned_malloc( (size_t)args.ret, 16 ))) status = STATUS_NO_MEMORY;
        else if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
        _aligned_free( args.ret );
    }
#endif
    WARN( "glMapNamedBuffer returned %#lx\n", status );
    return args.ret;
}

void * WINAPI glMapNamedBuffer( GLuint buffer, GLenum access )
{
    return gl_map_named_buffer( unix_glMapNamedBuffer, buffer, access );
}

void * WINAPI glMapNamedBufferEXT( GLuint buffer, GLenum access )
{
    return gl_map_named_buffer( unix_glMapNamedBufferEXT, buffer, access );
}

static void *gl_map_named_buffer_range( enum unix_funcs code, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    struct glMapNamedBufferRange_params args =
    {
        .teb = NtCurrentTeb(),
        .buffer = buffer,
        .offset = offset,
        .length = length,
        .access = access,
    };
    NTSTATUS status;

    TRACE( "buffer %d, offset %Id, length %Id, access %d\n", buffer, offset, length, access );

    if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
#ifndef _WIN64
    if (status == STATUS_INVALID_ADDRESS)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.ret = _aligned_malloc( length, 16 ))) status = STATUS_NO_MEMORY;
        else if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
        _aligned_free( args.ret );
    }
#endif
    WARN( "glMapNamedBufferRange returned %#lx\n", status );
    return args.ret;
}

void * WINAPI glMapNamedBufferRange( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    return gl_map_named_buffer_range( unix_glMapNamedBufferRange, buffer, offset, length, access );
}

void * WINAPI glMapNamedBufferRangeEXT( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    return gl_map_named_buffer_range( unix_glMapNamedBufferRangeEXT, buffer, offset, length, access );
}

static GLboolean gl_unmap_buffer( enum unix_funcs code, GLenum target )
{
    struct glUnmapBuffer_params args =
    {
        .teb = NtCurrentTeb(),
        .target = target,
    };
    NTSTATUS status;
#ifndef _WIN64
    void *ptr = get_buffer_pointer( target );
#endif

    TRACE( "target %d\n", target );

    if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
#ifndef _WIN64
    if (status == STATUS_INVALID_ADDRESS)
    {
        TRACE( "Releasing wow64 copy buffer %p\n", ptr );
        _aligned_free( ptr );
        return args.ret;
    }
#endif
    WARN( "glUnmapBuffer returned %#lx\n", status );
    return args.ret;
}

GLboolean WINAPI glUnmapBuffer( GLenum target )
{
    return gl_unmap_buffer( unix_glUnmapBuffer, target );
}

GLboolean WINAPI glUnmapBufferARB( GLenum target )
{
    return gl_unmap_buffer( unix_glUnmapBufferARB, target );
}

static GLboolean gl_unmap_named_buffer( enum unix_funcs code, GLuint buffer )
{
    struct glUnmapNamedBuffer_params args =
    {
        .teb = NtCurrentTeb(),
        .buffer = buffer,
    };
    NTSTATUS status;
#ifndef _WIN64
    void *ptr = get_named_buffer_pointer( buffer );
#endif

    TRACE( "buffer %d\n", buffer );

    if (!(status = WINE_UNIX_CALL( code, &args ))) return args.ret;
#ifndef _WIN64
    if (status == STATUS_INVALID_ADDRESS)
    {
        TRACE( "Releasing wow64 copy buffer %p\n", ptr );
        _aligned_free( ptr );
        return args.ret;
    }
#endif
    WARN( "glUnmapNamedBuffer returned %#lx\n", status );
    return args.ret;
}

GLboolean WINAPI glUnmapNamedBuffer( GLuint buffer )
{
    return gl_unmap_named_buffer( unix_glUnmapNamedBuffer, buffer );
}

GLboolean WINAPI glUnmapNamedBufferEXT( GLuint buffer )
{
    return gl_unmap_named_buffer( unix_glUnmapNamedBufferEXT, buffer );
}

static NTSTATUS WINAPI call_opengl_debug_message_callback( void *args, ULONG size )
{
    struct wine_gl_debug_message_params *params = args;
    params->user_callback( params->source, params->type, params->id, params->severity,
                           params->length, params->message, params->user_data );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    KERNEL_CALLBACK_PROC *kernel_callback_table;
    NTSTATUS status;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if ((status = __wine_init_unix_call()))
        {
            ERR( "Failed to load unixlib, status %#lx\n", status );
            return FALSE;
        }

        kernel_callback_table = NtCurrentTeb()->Peb->KernelCallbackTable;
        kernel_callback_table[NtUserCallOpenGLDebugMessageCallback] = call_opengl_debug_message_callback;
        /* fallthrough */
    case DLL_THREAD_ATTACH:
        if ((status = UNIX_CALL( thread_attach, NtCurrentTeb() )))
        {
            WARN( "Failed to initialize thread, status %#lx\n", status );
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        UNIX_CALL( process_detach, NULL );
#ifndef _WIN64
        cleanup_wow64_strings();
#endif
        return TRUE;
    }
    return TRUE;
}
