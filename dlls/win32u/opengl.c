/*
 * Copyright 2025 Rémi Bernon for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "win32u_private.h"
#include "ntuser_private.h"

#include "wine/wgl.h"
#include "wine/wgl_driver.h"

#include "dibdrv/dibdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

static const struct osmesa_funcs *osmesa_funcs;

struct wgl_context;

static const struct
{
    BYTE color_bits;
    BYTE red_bits, red_shift;
    BYTE green_bits, green_shift;
    BYTE blue_bits, blue_shift;
    BYTE alpha_bits, alpha_shift;
    BYTE accum_bits;
    BYTE depth_bits;
    BYTE stencil_bits;
} pixel_formats[] =
{
    { 32,  8, 16, 8, 8,  8, 0,  8, 24,  16, 32, 8 },
    { 32,  8, 16, 8, 8,  8, 0,  8, 24,  16, 16, 8 },
    { 32,  8, 0,  8, 8,  8, 16, 8, 24,  16, 32, 8 },
    { 32,  8, 0,  8, 8,  8, 16, 8, 24,  16, 16, 8 },
    { 32,  8, 8,  8, 16, 8, 24, 8, 0,   16, 32, 8 },
    { 32,  8, 8,  8, 16, 8, 24, 8, 0,   16, 16, 8 },
    { 24,  8, 0,  8, 8,  8, 16, 0, 0,   16, 32, 8 },
    { 24,  8, 0,  8, 8,  8, 16, 0, 0,   16, 16, 8 },
    { 24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 32, 8 },
    { 24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 16, 8 },
    { 16,  5, 0,  6, 5,  5, 11, 0, 0,   16, 32, 8 },
    { 16,  5, 0,  6, 5,  5, 11, 0, 0,   16, 16, 8 },
};

static void describe_pixel_format( int fmt, PIXELFORMATDESCRIPTOR *descr )
{
    memset( descr, 0, sizeof(*descr) );
    descr->nSize            = sizeof(*descr);
    descr->nVersion         = 1;
    descr->dwFlags          = PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_BITMAP | PFD_GENERIC_FORMAT;
    descr->iPixelType       = PFD_TYPE_RGBA;
    descr->cColorBits       = pixel_formats[fmt - 1].color_bits;
    descr->cRedBits         = pixel_formats[fmt - 1].red_bits;
    descr->cRedShift        = pixel_formats[fmt - 1].red_shift;
    descr->cGreenBits       = pixel_formats[fmt - 1].green_bits;
    descr->cGreenShift      = pixel_formats[fmt - 1].green_shift;
    descr->cBlueBits        = pixel_formats[fmt - 1].blue_bits;
    descr->cBlueShift       = pixel_formats[fmt - 1].blue_shift;
    descr->cAlphaBits       = pixel_formats[fmt - 1].alpha_bits;
    descr->cAlphaShift      = pixel_formats[fmt - 1].alpha_shift;
    descr->cAccumBits       = pixel_formats[fmt - 1].accum_bits;
    descr->cAccumRedBits    = pixel_formats[fmt - 1].accum_bits / 4;
    descr->cAccumGreenBits  = pixel_formats[fmt - 1].accum_bits / 4;
    descr->cAccumBlueBits   = pixel_formats[fmt - 1].accum_bits / 4;
    descr->cAccumAlphaBits  = pixel_formats[fmt - 1].accum_bits / 4;
    descr->cDepthBits       = pixel_formats[fmt - 1].depth_bits;
    descr->cStencilBits     = pixel_formats[fmt - 1].stencil_bits;
    descr->cAuxBuffers      = 0;
    descr->iLayerType       = PFD_MAIN_PLANE;
}

static BOOL dibdrv_wglCopyContext( struct wgl_context *src, struct wgl_context *dst, UINT mask )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

static BOOL dibdrv_wglDeleteContext( struct wgl_context *context )
{
    if (!osmesa_funcs) return FALSE;
    return osmesa_funcs->delete_context( context );
}

static int dibdrv_wglGetPixelFormat( HDC hdc )
{
    DC *dc = get_dc_ptr( hdc );
    int ret = 0;

    if (dc)
    {
        ret = dc->pixel_format;
        release_dc_ptr( dc );
    }
    return ret;
}

static struct wgl_context *dibdrv_wglCreateContext( HDC hdc )
{
    PIXELFORMATDESCRIPTOR descr;
    int format = dibdrv_wglGetPixelFormat( hdc );

    if (!format) format = 1;
    if (format <= 0 || format > ARRAY_SIZE( pixel_formats )) return NULL;
    describe_pixel_format( format, &descr );

    if (!osmesa_funcs) return NULL;
    return osmesa_funcs->create_context( hdc, &descr );
}

static PROC dibdrv_wglGetProcAddress( const char *proc )
{
    if (!strncmp( proc, "wgl", 3 )) return NULL;
    if (!osmesa_funcs) return NULL;
    return osmesa_funcs->get_proc_address( proc );
}

static BOOL dibdrv_wglMakeCurrent( HDC hdc, struct wgl_context *context )
{
    HBITMAP bitmap;
    BITMAPOBJ *bmp;
    dib_info dib;
    BOOL ret = FALSE;

    if (!osmesa_funcs) return FALSE;
    if (!context) return osmesa_funcs->make_current( NULL, NULL, 0, 0, 0, 0 );

    bitmap = NtGdiGetDCObject( hdc, NTGDI_OBJ_SURF );
    bmp = GDI_GetObjPtr( bitmap, NTGDI_OBJ_BITMAP );
    if (!bmp) return FALSE;

    if (init_dib_info_from_bitmapobj( &dib, bmp ))
    {
        char *bits;
        int width = dib.rect.right - dib.rect.left;
        int height = dib.rect.bottom - dib.rect.top;

        if (dib.stride < 0) bits = (char *)dib.bits.ptr + (dib.rect.bottom - 1) * dib.stride;
        else bits = (char *)dib.bits.ptr + dib.rect.top * dib.stride;
        bits += dib.rect.left * dib.bit_count / 8;

        TRACE( "context %p bits %p size %ux%u\n", context, bits, width, height );

        ret = osmesa_funcs->make_current( context, bits, width, height, dib.bit_count, dib.stride );
    }
    GDI_ReleaseObj( bitmap );
    return ret;
}

static BOOL dibdrv_wglSetPixelFormat( HDC hdc, int fmt, const PIXELFORMATDESCRIPTOR *descr )
{
    if (fmt <= 0 || fmt > ARRAY_SIZE( pixel_formats )) return FALSE;
    return NtGdiSetPixelFormat( hdc, fmt );
}

static BOOL dibdrv_wglShareLists( struct wgl_context *org, struct wgl_context *dest )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

static BOOL dibdrv_wglSwapBuffers( HDC hdc )
{
    return TRUE;
}

static void dibdrv_get_pixel_formats( struct wgl_pixel_format *formats, UINT max_formats,
                                      UINT *num_formats, UINT *num_onscreen_formats )
{
    UINT i, num_pixel_formats = ARRAY_SIZE( pixel_formats );

    for (i = 0; formats && i < min( max_formats, num_pixel_formats ); ++i)
        describe_pixel_format( i + 1, &formats[i].pfd );

    *num_formats = *num_onscreen_formats = num_pixel_formats;
}

static struct opengl_funcs dibdrv_opengl_funcs =
{
    .wgl.p_wglCopyContext = dibdrv_wglCopyContext,
    .wgl.p_wglCreateContext = dibdrv_wglCreateContext,
    .wgl.p_wglDeleteContext = dibdrv_wglDeleteContext,
    .wgl.p_wglGetPixelFormat = dibdrv_wglGetPixelFormat,
    .wgl.p_wglGetProcAddress = dibdrv_wglGetProcAddress,
    .wgl.p_wglMakeCurrent = dibdrv_wglMakeCurrent,
    .wgl.p_wglSetPixelFormat = dibdrv_wglSetPixelFormat,
    .wgl.p_wglShareLists = dibdrv_wglShareLists,
    .wgl.p_wglSwapBuffers = dibdrv_wglSwapBuffers,
    .wgl.p_get_pixel_formats = dibdrv_get_pixel_formats,
};

static struct opengl_funcs *dibdrv_get_wgl_driver(void)
{
    if (!osmesa_funcs && !(osmesa_funcs = init_opengl_lib()))
    {
        static int warned;
        if (!warned++) ERR( "OSMesa not available, no OpenGL bitmap support\n" );
        return (void *)-1;
    }
    osmesa_funcs->get_gl_funcs( &dibdrv_opengl_funcs );
    return &dibdrv_opengl_funcs;
}

/***********************************************************************
 *      __wine_get_wgl_driver  (win32u.@)
 */
const struct opengl_funcs *__wine_get_wgl_driver( HDC hdc, UINT version )
{
    DWORD is_disabled, is_display, is_memdc;
    DC *dc;

    if (version != WINE_WGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but dibdrv has %u\n",
             version, WINE_WGL_DRIVER_VERSION );
        return NULL;
    }

    if (!(dc = get_dc_ptr( hdc ))) return NULL;
    is_memdc = get_gdi_object_type( hdc ) == NTGDI_OBJ_MEMDC;
    is_display = dc->is_display;
    is_disabled = dc->attr->disabled;
    release_dc_ptr( dc );

    if (is_disabled) return NULL;
    if (is_display) return user_driver->pwine_get_wgl_driver( version );
    if (is_memdc) return dibdrv_get_wgl_driver();
    return (void *)-1;
}
