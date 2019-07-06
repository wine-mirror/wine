/*
 * DIB driver OpenGL support
 *
 * Copyright 2012 Alexandre Julliard
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

#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

#ifdef SONAME_LIBOSMESA

#include "wine/wgl.h"
#include "wine/wgl_driver.h"

#define OSMESA_COLOR_INDEX	GL_COLOR_INDEX
#define OSMESA_RGBA		GL_RGBA
#define OSMESA_BGRA		0x1
#define OSMESA_ARGB		0x2
#define OSMESA_RGB		GL_RGB
#define OSMESA_BGR		0x4
#define OSMESA_RGB_565		0x5
#define OSMESA_ROW_LENGTH	0x10
#define OSMESA_Y_UP		0x11

typedef struct osmesa_context *OSMesaContext;

extern BOOL WINAPI GdiSetPixelFormat( HDC hdc, INT fmt, const PIXELFORMATDESCRIPTOR *pfd );

struct wgl_context
{
    OSMesaContext context;
    int           format;
};

static struct opengl_funcs opengl_funcs;

#define USE_GL_FUNC(name) #name,
static const char *opengl_func_names[] = { ALL_WGL_FUNCS };
#undef USE_GL_FUNC

static OSMesaContext (*pOSMesaCreateContextExt)( GLenum format, GLint depthBits, GLint stencilBits,
                                                 GLint accumBits, OSMesaContext sharelist );
static void (*pOSMesaDestroyContext)( OSMesaContext ctx );
static void * (*pOSMesaGetProcAddress)( const char *funcName );
static GLboolean (*pOSMesaMakeCurrent)( OSMesaContext ctx, void *buffer, GLenum type,
                                        GLsizei width, GLsizei height );
static void (*pOSMesaPixelStore)( GLint pname, GLint value );

static const struct
{
    UINT mesa;
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
    { OSMESA_BGRA,     32,  8, 16, 8, 8,  8, 0,  8, 24,  16, 32, 8 },
    { OSMESA_BGRA,     32,  8, 16, 8, 8,  8, 0,  8, 24,  16, 16, 8 },
    { OSMESA_RGBA,     32,  8, 0,  8, 8,  8, 16, 8, 24,  16, 32, 8 },
    { OSMESA_RGBA,     32,  8, 0,  8, 8,  8, 16, 8, 24,  16, 16, 8 },
    { OSMESA_ARGB,     32,  8, 8,  8, 16, 8, 24, 8, 0,   16, 32, 8 },
    { OSMESA_ARGB,     32,  8, 8,  8, 16, 8, 24, 8, 0,   16, 16, 8 },
    { OSMESA_RGB,      24,  8, 0,  8, 8,  8, 16, 0, 0,   16, 32, 8 },
    { OSMESA_RGB,      24,  8, 0,  8, 8,  8, 16, 0, 0,   16, 16, 8 },
    { OSMESA_BGR,      24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 32, 8 },
    { OSMESA_BGR,      24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 16, 8 },
    { OSMESA_RGB_565,  16,  5, 0,  6, 5,  5, 11, 0, 0,   16, 32, 8 },
    { OSMESA_RGB_565,  16,  5, 0,  6, 5,  5, 11, 0, 0,   16, 16, 8 },
};

static BOOL init_opengl(void)
{
    static BOOL init_done = FALSE;
    static void *osmesa_handle;
    char buffer[200];
    unsigned int i;

    if (init_done) return (osmesa_handle != NULL);
    init_done = TRUE;

    osmesa_handle = wine_dlopen( SONAME_LIBOSMESA, RTLD_NOW, buffer, sizeof(buffer) );
    if (osmesa_handle == NULL)
    {
        ERR( "Failed to load OSMesa: %s\n", buffer );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) do if (!(p##f = wine_dlsym( osmesa_handle, #f, buffer, sizeof(buffer) ))) \
    { \
        ERR( "%s not found in %s (%s), disabling.\n", #f, SONAME_LIBOSMESA, buffer ); \
        goto failed; \
    } while(0)

    LOAD_FUNCPTR(OSMesaCreateContextExt);
    LOAD_FUNCPTR(OSMesaDestroyContext);
    LOAD_FUNCPTR(OSMesaGetProcAddress);
    LOAD_FUNCPTR(OSMesaMakeCurrent);
    LOAD_FUNCPTR(OSMesaPixelStore);
#undef LOAD_FUNCPTR

    for (i = 0; i < ARRAY_SIZE( opengl_func_names ); i++)
    {
        if (!(((void **)&opengl_funcs.gl)[i] = pOSMesaGetProcAddress( opengl_func_names[i] )))
        {
            ERR( "%s not found in %s, disabling.\n", opengl_func_names[i], SONAME_LIBOSMESA );
            goto failed;
        }
    }

    return TRUE;

failed:
    wine_dlclose( osmesa_handle, NULL, 0 );
    osmesa_handle = NULL;
    return FALSE;
}

/**********************************************************************
 *	     dibdrv_wglDescribePixelFormat
 */
static int dibdrv_wglDescribePixelFormat( HDC hdc, int fmt, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    int ret = ARRAY_SIZE( pixel_formats );

    if (!descr) return ret;
    if (fmt <= 0 || fmt > ret) return 0;
    if (size < sizeof(*descr)) return 0;

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
    return ret;
}

/***********************************************************************
 *		dibdrv_wglCopyContext
 */
static BOOL dibdrv_wglCopyContext( struct wgl_context *src, struct wgl_context *dst, UINT mask )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

/***********************************************************************
 *		dibdrv_wglCreateContext
 */
static struct wgl_context *dibdrv_wglCreateContext( HDC hdc )
{
    struct wgl_context *context;

    if (!(context = HeapAlloc( GetProcessHeap(), 0, sizeof( *context )))) return NULL;
    context->format = GetPixelFormat( hdc );
    if (!context->format || context->format > ARRAY_SIZE( pixel_formats )) context->format = 1;

    if (!(context->context = pOSMesaCreateContextExt( pixel_formats[context->format - 1].mesa,
                                                      pixel_formats[context->format - 1].depth_bits,
                                                      pixel_formats[context->format - 1].stencil_bits,
                                                      pixel_formats[context->format - 1].accum_bits, 0 )))
    {
        HeapFree( GetProcessHeap(), 0, context );
        return NULL;
    }
    return context;
}

/***********************************************************************
 *		dibdrv_wglDeleteContext
 */
static BOOL dibdrv_wglDeleteContext( struct wgl_context *context )
{
    pOSMesaDestroyContext( context->context );
    HeapFree( GetProcessHeap(), 0, context );
    return TRUE;
}

/***********************************************************************
 *		dibdrv_wglGetPixelFormat
 */
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

/***********************************************************************
 *		dibdrv_wglGetProcAddress
 */
static PROC dibdrv_wglGetProcAddress( const char *proc )
{
    if (!strncmp( proc, "wgl", 3 )) return NULL;
    return (PROC)pOSMesaGetProcAddress( proc );
}

/***********************************************************************
 *		dibdrv_wglMakeCurrent
 */
static BOOL dibdrv_wglMakeCurrent( HDC hdc, struct wgl_context *context )
{
    HBITMAP bitmap;
    BITMAPOBJ *bmp;
    dib_info dib;
    GLenum type;
    BOOL ret = FALSE;

    if (!context)
    {
        pOSMesaMakeCurrent( NULL, NULL, GL_UNSIGNED_BYTE, 0, 0 );
        return TRUE;
    }

    if (GetPixelFormat( hdc ) != context->format)
        FIXME( "mismatched pixel formats %u/%u not supported yet\n",
               GetPixelFormat( hdc ), context->format );

    bitmap = GetCurrentObject( hdc, OBJ_BITMAP );
    bmp = GDI_GetObjPtr( bitmap, OBJ_BITMAP );
    if (!bmp) return FALSE;

    if (init_dib_info_from_bitmapobj( &dib, bmp ))
    {
        char *bits;
        int width = dib.rect.right - dib.rect.left;
        int height = dib.rect.bottom - dib.rect.top;

        if (dib.stride < 0)
            bits = (char *)dib.bits.ptr + (dib.rect.bottom - 1) * dib.stride;
        else
            bits = (char *)dib.bits.ptr + dib.rect.top * dib.stride;
        bits += dib.rect.left * dib.bit_count / 8;

        TRACE( "context %p bits %p size %ux%u\n", context, bits, width, height );

        if (pixel_formats[context->format - 1].mesa == OSMESA_RGB_565)
            type = GL_UNSIGNED_SHORT_5_6_5;
        else
            type = GL_UNSIGNED_BYTE;

        ret = pOSMesaMakeCurrent( context->context, bits, type, width, height );
        if (ret)
        {
            pOSMesaPixelStore( OSMESA_ROW_LENGTH, abs( dib.stride ) * 8 / dib.bit_count );
            pOSMesaPixelStore( OSMESA_Y_UP, 1 );  /* Windows seems to assume bottom-up */
        }
    }
    GDI_ReleaseObj( bitmap );
    return ret;
}

/**********************************************************************
 *	     dibdrv_wglSetPixelFormat
 */
static BOOL dibdrv_wglSetPixelFormat( HDC hdc, int fmt, const PIXELFORMATDESCRIPTOR *descr )
{
    if (fmt <= 0 || fmt > ARRAY_SIZE( pixel_formats )) return FALSE;
    return GdiSetPixelFormat( hdc, fmt, descr );
}

/***********************************************************************
 *		dibdrv_wglShareLists
 */
static BOOL dibdrv_wglShareLists( struct wgl_context *org, struct wgl_context *dest )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

/***********************************************************************
 *		dibdrv_wglSwapBuffers
 */
static BOOL dibdrv_wglSwapBuffers( HDC hdc )
{
    return TRUE;
}

static struct opengl_funcs opengl_funcs =
{
    {
        dibdrv_wglCopyContext,        /* p_wglCopyContext */
        dibdrv_wglCreateContext,      /* p_wglCreateContext */
        dibdrv_wglDeleteContext,      /* p_wglDeleteContext */
        dibdrv_wglDescribePixelFormat,/* p_wglDescribePixelFormat */
        dibdrv_wglGetPixelFormat,     /* p_wglGetPixelFormat */
        dibdrv_wglGetProcAddress,     /* p_wglGetProcAddress */
        dibdrv_wglMakeCurrent,        /* p_wglMakeCurrent */
        dibdrv_wglSetPixelFormat,     /* p_wglSetPixelFormat */
        dibdrv_wglShareLists,         /* p_wglShareLists */
        dibdrv_wglSwapBuffers,        /* p_wglSwapBuffers */
    }
};

/**********************************************************************
 *	     dibdrv_wine_get_wgl_driver
 */
struct opengl_funcs * CDECL dibdrv_wine_get_wgl_driver( PHYSDEV dev, UINT version )
{
    if (version != WINE_WGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but dibdrv has %u\n", version, WINE_WGL_DRIVER_VERSION );
        return NULL;
    }

    if (!init_opengl()) return (void *)-1;

    return &opengl_funcs;
}

#else  /* SONAME_LIBOSMESA */

/**********************************************************************
 *	     dibdrv_wine_get_wgl_driver
 */
struct opengl_funcs * CDECL dibdrv_wine_get_wgl_driver( PHYSDEV dev, UINT version )
{
    static int warned;
    if (!warned++) ERR( "OSMesa not compiled in, no OpenGL bitmap support\n" );
    return (void *)-1;
}

#endif  /* SONAME_LIBOSMESA */
