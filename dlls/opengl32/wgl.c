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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntuser.h"
#include "ntgdi.h"
#include "malloc.h"

#include "unixlib.h"
#include "private.h"

#include "wine/glu.h"
#include "wine/debug.h"
#include "wine/opengl_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);
WINE_DECLARE_DEBUG_CHANNEL(fps);

static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

#define WINE_GL_RESERVED_FORMATS_HDC      2
#define WINE_GL_RESERVED_FORMATS_PTR      3
#define WINE_GL_RESERVED_FORMATS_NUM      4
#define WINE_GL_RESERVED_FORMATS_ONSCREEN 5

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
    DWORD is_memdc;

    TRACE( "%p %p: size %u version %u flags %lu type %u color %u %u,%u,%u,%u "
           "accum %u depth %u stencil %u aux %u\n",
           hdc, ppfd, ppfd->nSize, ppfd->nVersion, ppfd->dwFlags, ppfd->iPixelType,
           ppfd->cColorBits, ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits, ppfd->cAlphaBits,
           ppfd->cAccumBits, ppfd->cDepthBits, ppfd->cStencilBits, ppfd->cAuxBuffers );

    count = wglDescribePixelFormat( hdc, 0, 0, NULL );
    if (!count) return 0;

    if (!NtGdiGetDCDword( hdc, NtGdiIsMemDC, &is_memdc )) is_memdc = 0;

    best_format = 0;
    best.dwFlags = 0;
    best.cAlphaBits = -1;
    best.cColorBits = -1;
    best.cDepthBits = -1;
    best.cStencilBits = -1;
    best.cAuxBuffers = -1;
    best.cAccumBits = -1;

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
        if (is_memdc && ppfd->cColorBits == 16 && ppfd->cAlphaBits != 1)
        {
            TRACE( "Ignoring iPixelFormat=%d RGB565 format on memory DC\n", i );
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
        if (ppfd->cAccumBits)
        {
            if (((ppfd->cAccumBits > best.cAccumBits) && (format.cAccumBits > best.cAccumBits)) ||
                ((format.cAccumBits >= ppfd->cAccumBits) && (format.cAccumBits < best.cAccumBits)))
                goto found;

            if (best.cAccumBits != format.cAccumBits)
            {
                TRACE( "cAccumBits mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->dwFlags & PFD_DEPTH_DONTCARE)
        {
            if (format.cDepthBits < best.cDepthBits)
                goto found;
            continue;
        }

        if (!ppfd->cDepthBits && format.cDepthBits > best.cDepthBits)
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

static struct wgl_pixel_format *get_pixel_formats( HDC hdc, UINT *num_formats,
                                                   UINT *num_onscreen_formats )
{
    struct get_pixel_formats_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    PVOID *glReserved = NtCurrentTeb()->glReserved1;
    NTSTATUS status;
    DWORD is_memdc;

    if (glReserved[WINE_GL_RESERVED_FORMATS_HDC] == hdc)
    {
        *num_formats = PtrToUlong( glReserved[WINE_GL_RESERVED_FORMATS_NUM] );
        *num_onscreen_formats = PtrToUlong( glReserved[WINE_GL_RESERVED_FORMATS_ONSCREEN] );
        return glReserved[WINE_GL_RESERVED_FORMATS_PTR];
    }

    if ((status = UNIX_CALL( get_pixel_formats, &args ))) goto error;
    /* Clear formats memory since not all drivers deal with all wgl_pixel_format
     * fields at the moment. */
    if (!(args.formats = calloc( args.num_formats, sizeof(*args.formats) ))) goto error;
    args.max_formats = args.num_formats;
    if ((status = UNIX_CALL( get_pixel_formats, &args ))) goto error;

    if (NtGdiGetDCDword( hdc, NtGdiIsMemDC, &is_memdc ) && is_memdc)
        args.num_onscreen_formats = args.num_formats;

    *num_formats = args.num_formats;
    *num_onscreen_formats = args.num_onscreen_formats;

    free( glReserved[WINE_GL_RESERVED_FORMATS_PTR] );
    glReserved[WINE_GL_RESERVED_FORMATS_HDC] = hdc;
    glReserved[WINE_GL_RESERVED_FORMATS_PTR] = args.formats;
    glReserved[WINE_GL_RESERVED_FORMATS_NUM] = ULongToPtr( args.num_formats );
    glReserved[WINE_GL_RESERVED_FORMATS_ONSCREEN] = ULongToPtr( args.num_onscreen_formats );

    return args.formats;

error:
    *num_formats = *num_onscreen_formats = 0;
    free( args.formats );
    return NULL;
}

static BOOL wgl_attrib_uses_layer( int attrib )
{
    switch (attrib)
    {
    case WGL_ACCELERATION_ARB:
    case WGL_TRANSPARENT_ARB:
    case WGL_SHARE_DEPTH_ARB:
    case WGL_SHARE_STENCIL_ARB:
    case WGL_SHARE_ACCUM_ARB:
    case WGL_TRANSPARENT_RED_VALUE_ARB:
    case WGL_TRANSPARENT_GREEN_VALUE_ARB:
    case WGL_TRANSPARENT_BLUE_VALUE_ARB:
    case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
    case WGL_TRANSPARENT_INDEX_VALUE_ARB:
    case WGL_SUPPORT_GDI_ARB:
    case WGL_SUPPORT_OPENGL_ARB:
    case WGL_DOUBLE_BUFFER_ARB:
    case WGL_STEREO_ARB:
    case WGL_PIXEL_TYPE_ARB:
    case WGL_COLOR_BITS_ARB:
    case WGL_RED_BITS_ARB:
    case WGL_RED_SHIFT_ARB:
    case WGL_GREEN_BITS_ARB:
    case WGL_GREEN_SHIFT_ARB:
    case WGL_BLUE_BITS_ARB:
    case WGL_BLUE_SHIFT_ARB:
    case WGL_ALPHA_BITS_ARB:
    case WGL_ALPHA_SHIFT_ARB:
    case WGL_ACCUM_BITS_ARB:
    case WGL_ACCUM_RED_BITS_ARB:
    case WGL_ACCUM_GREEN_BITS_ARB:
    case WGL_ACCUM_BLUE_BITS_ARB:
    case WGL_ACCUM_ALPHA_BITS_ARB:
    case WGL_DEPTH_BITS_ARB:
    case WGL_STENCIL_BITS_ARB:
    case WGL_AUX_BUFFERS_ARB:
    case WGL_SAMPLE_BUFFERS_ARB:
    case WGL_SAMPLES_ARB:
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB:
    case WGL_FLOAT_COMPONENTS_NV:
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOL wgl_pixel_format_get_attrib( const struct wgl_pixel_format *fmt, int attrib, int *value )
{
    int val = 0;
    int valid = -1;

    switch (attrib)
    {
    case WGL_DRAW_TO_WINDOW_ARB: val = !!(fmt->pfd.dwFlags & PFD_DRAW_TO_WINDOW); break;
    case WGL_DRAW_TO_BITMAP_ARB: val = !!(fmt->pfd.dwFlags & PFD_DRAW_TO_BITMAP); break;
    case WGL_ACCELERATION_ARB:
        if (fmt->pfd.dwFlags & PFD_GENERIC_ACCELERATED)
            val = WGL_GENERIC_ACCELERATION_ARB;
        else if (fmt->pfd.dwFlags & PFD_GENERIC_FORMAT)
            val = WGL_NO_ACCELERATION_ARB;
        else
            val = WGL_FULL_ACCELERATION_ARB;
        break;
    case WGL_NEED_PALETTE_ARB: val = !!(fmt->pfd.dwFlags & PFD_NEED_PALETTE); break;
    case WGL_NEED_SYSTEM_PALETTE_ARB: val = !!(fmt->pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE); break;
    case WGL_SWAP_LAYER_BUFFERS_ARB: val = !!(fmt->pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS); break;
    case WGL_SWAP_METHOD_ARB: val = fmt->swap_method; break;
    case WGL_NUMBER_OVERLAYS_ARB:
    case WGL_NUMBER_UNDERLAYS_ARB:
        /* We don't support any overlays/underlays. */
        val = 0;
        break;
    case WGL_TRANSPARENT_ARB: val = fmt->transparent; break;
    case WGL_SHARE_DEPTH_ARB:
    case WGL_SHARE_STENCIL_ARB:
    case WGL_SHARE_ACCUM_ARB:
        /* We support only a main plane at the moment which by definition
         * shares the depth/stencil/accum buffers with itself. */
        val = GL_TRUE;
        break;
    case WGL_SUPPORT_GDI_ARB: val = !!(fmt->pfd.dwFlags & PFD_SUPPORT_GDI); break;
    case WGL_SUPPORT_OPENGL_ARB: val = !!(fmt->pfd.dwFlags & PFD_SUPPORT_OPENGL); break;
    case WGL_DOUBLE_BUFFER_ARB: val = !!(fmt->pfd.dwFlags & PFD_DOUBLEBUFFER); break;
    case WGL_STEREO_ARB: val = !!(fmt->pfd.dwFlags & PFD_STEREO); break;
    case WGL_PIXEL_TYPE_ARB: val = fmt->pixel_type; break;
    case WGL_COLOR_BITS_ARB: val = fmt->pfd.cColorBits; break;
    case WGL_RED_BITS_ARB: val = fmt->pfd.cRedBits; break;
    case WGL_RED_SHIFT_ARB: val = fmt->pfd.cRedShift; break;
    case WGL_GREEN_BITS_ARB: val = fmt->pfd.cGreenBits; break;
    case WGL_GREEN_SHIFT_ARB: val = fmt->pfd.cGreenShift; break;
    case WGL_BLUE_BITS_ARB: val = fmt->pfd.cBlueBits; break;
    case WGL_BLUE_SHIFT_ARB: val = fmt->pfd.cBlueShift; break;
    case WGL_ALPHA_BITS_ARB: val = fmt->pfd.cAlphaBits; break;
    case WGL_ALPHA_SHIFT_ARB: val = fmt->pfd.cAlphaShift; break;
    case WGL_ACCUM_BITS_ARB: val = fmt->pfd.cAccumBits; break;
    case WGL_ACCUM_RED_BITS_ARB: val = fmt->pfd.cAccumRedBits; break;
    case WGL_ACCUM_GREEN_BITS_ARB: val = fmt->pfd.cAccumGreenBits; break;
    case WGL_ACCUM_BLUE_BITS_ARB: val = fmt->pfd.cAccumBlueBits; break;
    case WGL_ACCUM_ALPHA_BITS_ARB: val = fmt->pfd.cAccumAlphaBits; break;
    case WGL_DEPTH_BITS_ARB: val = fmt->pfd.cDepthBits; break;
    case WGL_STENCIL_BITS_ARB: val = fmt->pfd.cStencilBits; break;
    case WGL_AUX_BUFFERS_ARB: val = fmt->pfd.cAuxBuffers; break;
    case WGL_DRAW_TO_PBUFFER_ARB: val = fmt->draw_to_pbuffer; break;
    case WGL_MAX_PBUFFER_PIXELS_ARB: val = fmt->max_pbuffer_pixels; break;
    case WGL_MAX_PBUFFER_WIDTH_ARB: val = fmt->max_pbuffer_width; break;
    case WGL_MAX_PBUFFER_HEIGHT_ARB: val = fmt->max_pbuffer_height; break;
    case WGL_TRANSPARENT_RED_VALUE_ARB:
        val = fmt->transparent_red_value;
        valid = !!fmt->transparent_red_value_valid;
        break;
    case WGL_TRANSPARENT_GREEN_VALUE_ARB:
        val = fmt->transparent_green_value;
        valid = !!fmt->transparent_green_value_valid;
        break;
    case WGL_TRANSPARENT_BLUE_VALUE_ARB:
        val = fmt->transparent_blue_value;
        valid = !!fmt->transparent_blue_value_valid;
        break;
    case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
        val = fmt->transparent_alpha_value;
        valid = !!fmt->transparent_alpha_value_valid;
        break;
    case WGL_TRANSPARENT_INDEX_VALUE_ARB:
        val = fmt->transparent_index_value;
        valid = !!fmt->transparent_index_value_valid;
        break;
    case WGL_SAMPLE_BUFFERS_ARB: val = fmt->sample_buffers; break;
    case WGL_SAMPLES_ARB: val = fmt->samples; break;
    case WGL_BIND_TO_TEXTURE_RGB_ARB: val = fmt->bind_to_texture_rgb; break;
    case WGL_BIND_TO_TEXTURE_RGBA_ARB: val = fmt->bind_to_texture_rgba; break;
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV: val = fmt->bind_to_texture_rectangle_rgb; break;
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV: val = fmt->bind_to_texture_rectangle_rgba; break;
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB: val = fmt->framebuffer_srgb_capable; break;
    case WGL_FLOAT_COMPONENTS_NV: val = fmt->float_components; break;
    default:
        FIXME( "unsupported 0x%x WGL attribute\n", attrib );
        valid = 0;
        break;
    }

    /* If we haven't already determined validity, use the default check */
    if (valid == -1) valid = val != -1;
    if (valid) *value = val;

    return valid;
}

enum attrib_match
{
    ATTRIB_MATCH_INVALID = -1,
    ATTRIB_MATCH_IGNORE,
    ATTRIB_MATCH_EXACT,
    ATTRIB_MATCH_MINIMUM,
};

static enum attrib_match wgl_attrib_match_criteria( int attrib )
{
    switch (attrib)
    {
    case WGL_DRAW_TO_WINDOW_ARB:
    case WGL_DRAW_TO_BITMAP_ARB:
    case WGL_ACCELERATION_ARB:
    case WGL_NEED_PALETTE_ARB:
    case WGL_NEED_SYSTEM_PALETTE_ARB:
    case WGL_SWAP_LAYER_BUFFERS_ARB:
    case WGL_SHARE_DEPTH_ARB:
    case WGL_SHARE_STENCIL_ARB:
    case WGL_SHARE_ACCUM_ARB:
    case WGL_SUPPORT_GDI_ARB:
    case WGL_SUPPORT_OPENGL_ARB:
    case WGL_DOUBLE_BUFFER_ARB:
    case WGL_STEREO_ARB:
    case WGL_PIXEL_TYPE_ARB:
    case WGL_DRAW_TO_PBUFFER_ARB:
    case WGL_BIND_TO_TEXTURE_RGB_ARB:
    case WGL_BIND_TO_TEXTURE_RGBA_ARB:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV:
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB:
    case WGL_FLOAT_COMPONENTS_NV:
        return ATTRIB_MATCH_EXACT;
    case WGL_NUMBER_OVERLAYS_ARB:
    case WGL_NUMBER_UNDERLAYS_ARB:
    case WGL_COLOR_BITS_ARB:
    case WGL_RED_BITS_ARB:
    case WGL_GREEN_BITS_ARB:
    case WGL_BLUE_BITS_ARB:
    case WGL_ALPHA_BITS_ARB:
    case WGL_ACCUM_BITS_ARB:
    case WGL_ACCUM_RED_BITS_ARB:
    case WGL_ACCUM_GREEN_BITS_ARB:
    case WGL_ACCUM_BLUE_BITS_ARB:
    case WGL_ACCUM_ALPHA_BITS_ARB:
    case WGL_DEPTH_BITS_ARB:
    case WGL_STENCIL_BITS_ARB:
    case WGL_AUX_BUFFERS_ARB:
    case WGL_SAMPLE_BUFFERS_ARB:
    case WGL_SAMPLES_ARB:
        return ATTRIB_MATCH_MINIMUM;
    case WGL_NUMBER_PIXEL_FORMATS_ARB:
    case WGL_RED_SHIFT_ARB:
    case WGL_GREEN_SHIFT_ARB:
    case WGL_BLUE_SHIFT_ARB:
    case WGL_ALPHA_SHIFT_ARB:
    case WGL_TRANSPARENT_ARB:
    case WGL_TRANSPARENT_RED_VALUE_ARB:
    case WGL_TRANSPARENT_GREEN_VALUE_ARB:
    case WGL_TRANSPARENT_BLUE_VALUE_ARB:
    case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
    case WGL_TRANSPARENT_INDEX_VALUE_ARB:
    case WGL_SWAP_METHOD_ARB:
        return ATTRIB_MATCH_IGNORE;
    default:
        return ATTRIB_MATCH_INVALID;
    }
}

static void filter_format_array( const struct wgl_pixel_format **array,
                                 UINT num_formats, int attrib, int value )
{
    enum attrib_match match = wgl_attrib_match_criteria( attrib );
    int fmt_value;
    UINT i;

    assert(match != ATTRIB_MATCH_INVALID);

    if (match == ATTRIB_MATCH_IGNORE && attrib != WGL_SWAP_METHOD_ARB) return;

    for (i = 0; i < num_formats; ++i)
    {
        if (!array[i]) continue;
        if (!wgl_pixel_format_get_attrib( array[i], attrib, &fmt_value ) ||
            (match == ATTRIB_MATCH_EXACT && fmt_value != value) ||
            (match == ATTRIB_MATCH_MINIMUM && fmt_value < value) ||
            (attrib == WGL_SWAP_METHOD_ARB && ((fmt_value == WGL_SWAP_COPY_ARB) ^ (value == WGL_SWAP_COPY_ARB))))
        {
            array[i] = NULL;
        }
    }
}

static int wgl_attrib_sort_priority( int attrib )
{
    switch (attrib)
    {
    case WGL_DRAW_TO_WINDOW_ARB: return 1;
    case WGL_DRAW_TO_BITMAP_ARB: return 2;
    case WGL_ACCELERATION_ARB: return 3;
    case WGL_COLOR_BITS_ARB: return 4;
    case WGL_ACCUM_BITS_ARB: return 5;
    case WGL_PIXEL_TYPE_ARB: return 6;
    case WGL_ALPHA_BITS_ARB: return 7;
    case WGL_AUX_BUFFERS_ARB: return 8;
    case WGL_DEPTH_BITS_ARB: return 9;
    case WGL_STENCIL_BITS_ARB: return 10;
    case WGL_DOUBLE_BUFFER_ARB: return 11;
    case WGL_SWAP_METHOD_ARB: return 12;
    default: return 100;
    }
}

static int compare_attribs( const void *a, const void *b )
{
    return wgl_attrib_sort_priority( *(int *)a ) - wgl_attrib_sort_priority( *(int *)b );
}

static int wgl_attrib_value_priority( int value )
{
    switch (value)
    {
    case WGL_SWAP_UNDEFINED_ARB: return 1;
    case WGL_SWAP_EXCHANGE_ARB: return 2;
    case WGL_SWAP_COPY_ARB: return 3;

    case WGL_FULL_ACCELERATION_ARB: return 1;
    case WGL_GENERIC_ACCELERATION_ARB: return 2;
    case WGL_NO_ACCELERATION_ARB: return 3;

    case WGL_TYPE_RGBA_ARB: return 1;
    case WGL_TYPE_RGBA_FLOAT_ATI: return 2;
    case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT: return 3;
    case WGL_TYPE_COLORINDEX_ARB: return 4;

    default: return 100;
    }
}

struct compare_formats_ctx
{
    int attribs[256];
    UINT num_attribs;
};

static int compare_formats( void *arg, const void *a, const void *b )
{
    const struct wgl_pixel_format *fmt_a = *(void **)a, *fmt_b = *(void **)b;
    struct compare_formats_ctx *ctx = arg;
    int attrib, val_a, val_b;
    UINT i;

    if (!fmt_a) return 1;
    if (!fmt_b) return -1;

    for (i = 0; i < ctx->num_attribs; ++i)
    {
        attrib = ctx->attribs[2 * i];
        if (wgl_pixel_format_get_attrib( fmt_a, attrib, &val_a ) &&
            wgl_pixel_format_get_attrib( fmt_b, attrib, &val_b ) &&
            val_a != val_b)
        {
            switch (attrib)
            {
            case WGL_ACCELERATION_ARB:
            case WGL_SWAP_METHOD_ARB:
            case WGL_PIXEL_TYPE_ARB:
                return wgl_attrib_value_priority( val_a ) -
                       wgl_attrib_value_priority( val_b );
            case WGL_COLOR_BITS_ARB:
                /* Prefer 32bpp over other values */
                if (val_a >= 32 && val_b >= 32) return val_a - val_b;
                else return val_b - val_a;
            default:
                /* Smaller values first */
                return val_a - val_b;
            }
        }
    }

    /* Maintain pixel format id order */
    return fmt_a - fmt_b;
}

static void compare_formats_ctx_set_attrib( struct compare_formats_ctx *ctx,
                                            int attrib, int value )
{
    UINT i;

    /* Overwrite attribute if it exists already */
    for (i = 0; i < ctx->num_attribs; ++i)
        if (ctx->attribs[2 * i] == attrib) break;

    assert(i < ARRAY_SIZE(ctx->attribs) / 2);

    ctx->attribs[2 * i] = attrib;
    ctx->attribs[2 * i + 1] = value;
    if (i == ctx->num_attribs) ++ctx->num_attribs;
}

/***********************************************************************
 *		wglChoosePixelFormatARB (OPENGL32.@)
 */
BOOL WINAPI wglChoosePixelFormatARB( HDC hdc, const int *attribs_int, const FLOAT *attribs_float,
                                     UINT max_formats, int *formats, UINT *num_formats )
{
    struct wgl_pixel_format *wgl_formats;
    UINT i, num_wgl_formats, num_wgl_onscreen_formats;
    const struct wgl_pixel_format **format_array;
    struct compare_formats_ctx ctx = { 0 };
    DWORD is_memdc;

    TRACE( "hdc %p, attribs_int %p, attribs_float %p, max_formats %u, formats %p, num_formats %p\n",
           hdc, attribs_int, attribs_float, max_formats, formats, num_formats );

    if (!NtGdiGetDCDword( hdc, NtGdiIsMemDC, &is_memdc )) is_memdc = 0;

    wgl_formats = get_pixel_formats( hdc, &num_wgl_formats, &num_wgl_onscreen_formats );

    /* If the driver doesn't yet provide ARB attrib information in
     * wgl_pixel_format, fall back to an explicit call. */
    if (num_wgl_formats && !wgl_formats[0].pixel_type)
    {
        struct wglChoosePixelFormatARB_params args =
        {
            .teb = NtCurrentTeb(),
            .hdc = hdc,
            .piAttribIList = attribs_int,
            .pfAttribFList = attribs_float,
            .nMaxFormats = max_formats,
            .piFormats = formats,
            .nNumFormats = num_formats
        };
        NTSTATUS status;

        if ((status = UNIX_CALL( wglChoosePixelFormatARB, &args )))
            WARN( "wglChoosePixelFormatARB returned %#lx\n", status );

        return args.ret;
    }

    /* Gather, validate and deduplicate all attributes */
    for (i = 0; attribs_int && attribs_int[i]; i += 2)
    {
        if (wgl_attrib_match_criteria( attribs_int[i] ) == ATTRIB_MATCH_INVALID) return FALSE;
        compare_formats_ctx_set_attrib( &ctx, attribs_int[i], attribs_int[i + 1] );
    }
    for (i = 0; attribs_float && attribs_float[i]; i += 2)
    {
        if (wgl_attrib_match_criteria( attribs_float[i] ) == ATTRIB_MATCH_INVALID) return FALSE;
        compare_formats_ctx_set_attrib( &ctx, attribs_float[i], attribs_float[i + 1] );
    }

    /* Initialize the format_array with (pointers to) all wgl formats */
    format_array = malloc( num_wgl_formats * sizeof(*format_array) );
    if (!format_array) return FALSE;
    for (i = 0; i < num_wgl_formats; ++i)
    {
        struct wgl_pixel_format *format = wgl_formats + i;
        if (is_memdc && format->pfd.cColorBits == 16 && format->pfd.cAlphaBits != 1) format_array[i] = NULL;
        else format_array[i] = &wgl_formats[i];
    }

    /* Remove formats that are not acceptable */
    for (i = 0; i < ctx.num_attribs; ++i)
        filter_format_array( format_array, num_wgl_formats, ctx.attribs[2 * i],
                             ctx.attribs[2 * i + 1] );

    /* Some attributes we always want to sort by (values don't matter for sorting) */
    compare_formats_ctx_set_attrib( &ctx, WGL_ACCELERATION_ARB, 0 );
    compare_formats_ctx_set_attrib( &ctx, WGL_COLOR_BITS_ARB, 0 );
    compare_formats_ctx_set_attrib( &ctx, WGL_ACCUM_BITS_ARB, 0 );

    /* Arrange attributes in the order which we want to check them */
    qsort( ctx.attribs, ctx.num_attribs, 2 * sizeof(*ctx.attribs), compare_attribs );

    /* Sort pixel formats based on the specified attributes */
    qsort_s( format_array, num_wgl_formats, sizeof(*format_array), compare_formats, &ctx );

    /* Return the best max_formats format ids */
    *num_formats = 0;
    for (i = 0; i < num_wgl_formats && i < max_formats && format_array[i]; ++i)
    {
        ++*num_formats;
        formats[i] = format_array[i] - wgl_formats + 1;
    }

    free( format_array );
    return TRUE;
}

INT WINAPI wglDescribePixelFormat( HDC hdc, int index, UINT size, PIXELFORMATDESCRIPTOR *ppfd )
{
    struct wgl_pixel_format *formats;
    UINT num_formats, num_onscreen_formats;

    TRACE( "hdc %p, index %d, size %u, ppfd %p\n", hdc, index, index, ppfd );

    if (!(formats = get_pixel_formats( hdc, &num_formats, &num_onscreen_formats ))) return 0;
    if (!ppfd) return num_onscreen_formats;
    if (size < sizeof(*ppfd)) return 0;
    if (index <= 0 || index > num_onscreen_formats) return 0;

    *ppfd = formats[index - 1].pfd;

    return num_onscreen_formats;
}

/***********************************************************************
 *		wglGetPixelFormatAttribivARB (OPENGL32.@)
 */
BOOL WINAPI wglGetPixelFormatAttribivARB( HDC hdc, int index, int plane, UINT count,
                                          const int *attributes, int *values )
{
    static const DWORD invalid_data_error = 0xC007000D;
    struct wgl_pixel_format *formats;
    UINT i, num_formats, num_onscreen_formats;

    TRACE( "hdc %p, index %d, plane %d, count %u, attributes %p, values %p\n",
           hdc, index, plane, count, attributes, values );

    formats = get_pixel_formats( hdc, &num_formats, &num_onscreen_formats );

    /* If the driver doesn't yet provide ARB attrib information in
     * wgl_pixel_format, fall back to an explicit call. */
    if (num_formats && !formats[0].pixel_type)
    {
        struct wglGetPixelFormatAttribivARB_params args =
        {
            .teb = NtCurrentTeb(),
            .hdc = hdc,
            .iPixelFormat = index,
            .iLayerPlane = plane,
            .nAttributes = count,
            .piAttributes = attributes,
            .piValues = values
        };
        NTSTATUS status;

        if ((status = UNIX_CALL( wglGetPixelFormatAttribivARB, &args )))
            WARN( "wglGetPixelFormatAttribivARB returned %#lx\n", status );

        return args.ret;
    }

    if (!count) return TRUE;
    if (count == 1 && attributes[0] == WGL_NUMBER_PIXEL_FORMATS_ARB)
    {
        values[0] = num_formats;
        return TRUE;
    }
    if (index <= 0 || index > num_formats)
    {
        SetLastError( invalid_data_error );
        return FALSE;
    }

    for (i = 0; i < count; ++i)
    {
        int attrib = attributes[i];

        if (attrib == WGL_NUMBER_PIXEL_FORMATS_ARB)
        {
            values[i] = num_formats;
        }
        else if ((plane != 0 && wgl_attrib_uses_layer( attrib )) ||
                 !wgl_pixel_format_get_attrib( &formats[index - 1], attrib, &values[i] ))
        {
            SetLastError( invalid_data_error );
            return FALSE;
        }
    }

    return TRUE;
}

/***********************************************************************
 *		wglGetPixelFormatAttribfvARB (OPENGL32.@)
 */
BOOL WINAPI wglGetPixelFormatAttribfvARB( HDC hdc, int index, int plane, UINT count,
                                          const int *attributes, FLOAT *values )
{
    int *ivalues;
    BOOL ret;
    UINT i;

    TRACE( "hdc %p, index %d, plane %d, count %u, attributes %p, values %p\n",
           hdc, index, plane, count, attributes, values );

    if (!(ivalues = malloc( count * sizeof(int) ))) return FALSE;

    /* For now we can piggy-back on wglGetPixelFormatAttribivARB, since we don't support
     * any non-integer attributes. */
    ret = wglGetPixelFormatAttribivARB( hdc, index, plane, count, attributes, ivalues );
    if (ret)
    {
        for (i = 0; i < count; i++)
            values[i] = ivalues[i];
    }

    free( ivalues );
    return ret;
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
int WINAPI wglGetLayerPaletteEntries( HDC hdc, int plane, int start, int count, COLORREF *colors )
{
    FIXME( "hdc %p, plane %d, start %d, count %d, colors %p, stub!\n", hdc, plane, start, count, colors );
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

static BOOL bezier_fits_deviation(const bezier_vector *p, FLOAT max_deviation)
{
    bezier_vector deviation;
    bezier_vector vertex;
    bezier_vector base;
    double base_length;
    double dot;

    max_deviation *= max_deviation;

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4 - p[0].x;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4 - p[0].y;

    base.x = p[2].x - p[0].x;
    base.y = p[2].y - p[0].y;

    base_length = base.x * base.x + base.y * base.y;
    if (base_length <= max_deviation)
    {
        base.x = 0.0;
        base.y = 0.0;
    }
    else
    {
        base_length = sqrt(base_length);
        base.x /= base_length;
        base.y /= base_length;

        dot = base.x*vertex.x + base.y*vertex.y;
        dot = min(max(dot, 0.0), base_length);
        base.x *= dot;
        base.y *= dot;
    }

    deviation.x = vertex.x-base.x;
    deviation.y = vertex.y-base.y;

    return deviation.x*deviation.x + deviation.y*deviation.y <= max_deviation;
}

static int bezier_approximate(const bezier_vector *p, bezier_vector *points, FLOAT deviation)
{
    bezier_vector first_curve[3];
    bezier_vector second_curve[3];
    bezier_vector vertex;
    int total_vertices;

    if (bezier_fits_deviation(p, deviation))
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

    status = WINE_UNIX_CALL( code, &args );
#ifndef _WIN64
    if (args.client_ptr)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.client_ptr = _aligned_malloc( (size_t)args.client_ptr, 16 ))) return NULL;
        status = WINE_UNIX_CALL( code, &args );
        if (args.ret != args.client_ptr) _aligned_free( args.client_ptr );
    }
#endif
    if (status) WARN( "glMapBuffer returned %#lx\n", status );
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

    status = UNIX_CALL( glMapBufferRange, &args );
#ifndef _WIN64
    if (args.client_ptr)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.client_ptr = _aligned_malloc( length, 16 ))) return NULL;
        status = UNIX_CALL( glMapBufferRange, &args );
        if (args.ret != args.client_ptr) _aligned_free( args.client_ptr );
    }
#endif
    if (status) WARN( "glMapBufferRange returned %#lx\n", status );
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

    status = WINE_UNIX_CALL( code, &args );
#ifndef _WIN64
    if (args.client_ptr)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.client_ptr = _aligned_malloc( (size_t)args.client_ptr, 16 ))) return NULL;
        status = WINE_UNIX_CALL( code, &args );
        if (args.ret != args.client_ptr) _aligned_free( args.client_ptr );
    }
#endif
    if (status) WARN( "glMapNamedBuffer returned %#lx\n", status );
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

    status = WINE_UNIX_CALL( code, &args );
#ifndef _WIN64
    if (args.client_ptr)
    {
        TRACE( "Unable to map wow64 buffer directly, using copy buffer!\n" );
        if (!(args.client_ptr = _aligned_malloc( length, 16 ))) return NULL;
        status = WINE_UNIX_CALL( code, &args );
        if (args.ret != args.client_ptr) _aligned_free( args.ret );
    }
#endif
    if (status) WARN( "glMapNamedBufferRange returned %#lx\n", status );
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

    TRACE( "target %d\n", target );

    status = WINE_UNIX_CALL( code, &args );
#ifndef _WIN64
    if (args.client_ptr)
    {
        TRACE( "Releasing wow64 copy buffer %p\n", args.client_ptr );
        _aligned_free( args.client_ptr );
    }
#endif
    if (status) WARN( "glUnmapBuffer returned %#lx\n", status );
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

    TRACE( "buffer %d\n", buffer );

    status = WINE_UNIX_CALL( code, &args );
#ifndef _WIN64
    if (args.client_ptr)
    {
        TRACE( "Releasing wow64 copy buffer %p\n", args.client_ptr );
        _aligned_free( args.client_ptr );
    }
#endif
    if (status) WARN( "glUnmapNamedBuffer returned %#lx\n", status );
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

typedef void (WINAPI *gl_debug_message)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *);

static NTSTATUS WINAPI call_gl_debug_message_callback( void *args, ULONG size )
{
    struct gl_debug_message_callback_params *params = args;
    gl_debug_message callback = (void *)(UINT_PTR)params->debug_callback;
    const void *user = (void *)(UINT_PTR)params->debug_user;
    callback( params->source, params->type, params->id, params->severity,
              params->length, params->message, user );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    struct process_attach_params params =
    {
        .call_gl_debug_message_callback = (UINT_PTR)call_gl_debug_message_callback,
    };
    NTSTATUS status;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if ((status = __wine_init_unix_call()) ||
            (status = UNIX_CALL( process_attach, &params )))
        {
            ERR( "Failed to load unixlib, status %#lx\n", status );
            return FALSE;
        }

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
        /* fallthrough */
    case DLL_THREAD_DETACH:
        free( NtCurrentTeb()->glReserved1[WINE_GL_RESERVED_FORMATS_PTR] );
        return TRUE;
    }
    return TRUE;
}
