/*
 * Copyright 2012 Alexandre Julliard
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

#include <assert.h>
#include <pthread.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "win32u_private.h"
#include "ntuser_private.h"

#include "wine/opengl_driver.h"

#include "dibdrv/dibdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

struct wgl_context
{
    const struct opengl_driver_funcs *driver_funcs;
    const struct opengl_funcs *funcs;
    void *driver_private;
    int pixel_format;

    /* hooked host function pointers */
    PFN_glFinish p_glFinish;
    PFN_glFlush p_glFlush;
};

struct wgl_pbuffer
{
    const struct opengl_driver_funcs *driver_funcs;
    const struct opengl_funcs *funcs;
    void *driver_private;

    HDC hdc;
    GLsizei width;
    GLsizei height;
    GLenum texture_format;
    GLenum texture_target;
    GLint mipmap_level;
    GLenum cube_face;

    struct wgl_context *tmp_context;
    struct wgl_context *prev_context;
};

static const struct opengl_funcs *get_dc_funcs( HDC hdc, const struct opengl_funcs *null_funcs );

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

static BOOL has_extension( const char *list, const char *ext )
{
    size_t len = strlen( ext );
    const char *cur = list;

    while (cur && (cur = strstr( cur, ext )))
    {
        if ((!cur[len] || cur[len] == ' ') && (cur == list || cur[-1] == ' ')) return TRUE;
        cur = strchr( cur, ' ' );
    }

    return FALSE;
}

static void dump_extensions( const char *list )
{
    const char *start, *end, *ptr;

    for (start = end = ptr = list; ptr; ptr = strchr( ptr + 1, ' ' ))
    {
        if (ptr - start <= 128) end = ptr;
        else
        {
            TRACE( "%.*s\n", (int)(end - start), start );
            start = end + 1;
        }
    }

    TRACE( "%s\n", start );
}

static void register_extension( char *list, size_t size, const char *name )
{
    if (!has_extension( list, name ))
    {
        size_t len = strlen( list );
        assert( size - len >= strlen( name ) + 1 );
        if (*list) strcat( list + len, " " );
        strcat( list + len, name );
    }
}

#ifdef SONAME_LIBOSMESA

#define OSMESA_COLOR_INDEX  GL_COLOR_INDEX
#define OSMESA_RGBA     GL_RGBA
#define OSMESA_BGRA     0x1
#define OSMESA_ARGB     0x2
#define OSMESA_RGB      GL_RGB
#define OSMESA_BGR      0x4
#define OSMESA_RGB_565      0x5
#define OSMESA_ROW_LENGTH   0x10
#define OSMESA_Y_UP     0x11

typedef struct osmesa_context *OSMesaContext;

struct osmesa_context
{
    OSMesaContext context;
    UINT          format;
};

static struct opengl_funcs osmesa_opengl_funcs;
static const struct opengl_driver_funcs osmesa_driver_funcs;

static OSMesaContext (*pOSMesaCreateContextExt)( GLenum format, GLint depthBits, GLint stencilBits,
                                                 GLint accumBits, OSMesaContext sharelist );
static void (*pOSMesaDestroyContext)( OSMesaContext ctx );
static void * (*pOSMesaGetProcAddress)( const char *funcName );
static GLboolean (*pOSMesaMakeCurrent)( OSMesaContext ctx, void *buffer, GLenum type,
                                        GLsizei width, GLsizei height );
static void (*pOSMesaPixelStore)( GLint pname, GLint value );
static void describe_pixel_format( int fmt, PIXELFORMATDESCRIPTOR *descr );

static struct opengl_funcs *osmesa_get_wgl_driver( const struct opengl_driver_funcs **driver_funcs )
{
    static void *osmesa_handle;

    osmesa_handle = dlopen( SONAME_LIBOSMESA, RTLD_NOW );
    if (osmesa_handle == NULL)
    {
        ERR( "Failed to load OSMesa: %s\n", dlerror() );
        return NULL;
    }

#define LOAD_FUNCPTR(f) do if (!(p##f = dlsym( osmesa_handle, #f ))) \
    { \
        ERR( "%s not found in %s (%s), disabling.\n", #f, SONAME_LIBOSMESA, dlerror() ); \
        goto failed; \
    } while(0)

    LOAD_FUNCPTR(OSMesaCreateContextExt);
    LOAD_FUNCPTR(OSMesaDestroyContext);
    LOAD_FUNCPTR(OSMesaGetProcAddress);
    LOAD_FUNCPTR(OSMesaMakeCurrent);
    LOAD_FUNCPTR(OSMesaPixelStore);
#undef LOAD_FUNCPTR

    *driver_funcs = &osmesa_driver_funcs;
    return &osmesa_opengl_funcs;

failed:
    dlclose( osmesa_handle );
    osmesa_handle = NULL;
    return NULL;
}

static const char *osmesa_init_wgl_extensions(void)
{
    return "";
}

static UINT osmesa_init_pixel_formats( UINT *onscreen_count )
{
    *onscreen_count = ARRAY_SIZE(pixel_formats);
    return ARRAY_SIZE(pixel_formats);
}

static BOOL osmesa_describe_pixel_format( int format, struct wgl_pixel_format *desc )
{
    if (format <= 0 || format > ARRAY_SIZE(pixel_formats)) return FALSE;
    describe_pixel_format( format, &desc->pfd );
    return TRUE;
}

static BOOL osmesa_set_pixel_format( HWND hwnd, int old_format, int new_format, BOOL internal )
{
    return TRUE;
}

static BOOL osmesa_context_create( HDC hdc, int format, void *shared, const int *attribs, void **private )
{
    struct osmesa_context *context;
    PIXELFORMATDESCRIPTOR descr;
    UINT gl_format;

    if (attribs)
    {
        ERR( "attribs not supported\n" );
        return FALSE;
    }

    describe_pixel_format( format, &descr );

    switch (descr.cColorBits)
    {
    case 32:
        if (descr.cRedShift == 8) gl_format = OSMESA_ARGB;
        else if (descr.cRedShift == 16) gl_format = OSMESA_BGRA;
        else gl_format = OSMESA_RGBA;
        break;
    case 24:
        gl_format = descr.cRedShift == 16 ? OSMESA_BGR : OSMESA_RGB;
        break;
    case 16:
        gl_format = OSMESA_RGB_565;
        break;
    default:
        return FALSE;
    }

    if (!(context = malloc( sizeof(*context) ))) return FALSE;
    context->format = gl_format;
    if (!(context->context = pOSMesaCreateContextExt( gl_format, descr.cDepthBits, descr.cStencilBits,
                                                      descr.cAccumBits, 0 )))
    {
        free( context );
        return FALSE;
    }

    *private = context;
    return TRUE;
}

static BOOL osmesa_context_destroy( void *private )
{
    struct osmesa_context *context = private;
    pOSMesaDestroyContext( context->context );
    free( context );
    return TRUE;
}

static void *osmesa_get_proc_address( const char *proc )
{
    if (!strncmp( proc, "wgl", 3 )) return NULL;
    return (PROC)pOSMesaGetProcAddress( proc );
}

static BOOL osmesa_swap_buffers( void *private, HWND hwnd, HDC hdc, int interval )
{
    return TRUE;
}

static BOOL osmesa_context_copy( void *src_private, void *dst_private, UINT mask )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

static BOOL osmesa_context_share( void *src_private, void *dst_private )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

static BOOL osmesa_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    struct osmesa_context *context = private;
    HBITMAP bitmap;
    BITMAPOBJ *bmp;
    dib_info dib;
    BOOL ret = FALSE;

    if (!private)
    {
        pOSMesaMakeCurrent( NULL, NULL, GL_UNSIGNED_BYTE, 0, 0 );
        return TRUE;
    }

    if (draw_hdc != read_hdc)
    {
        ERR( "read != draw not supported\n" );
        return FALSE;
    }

    bitmap = NtGdiGetDCObject( draw_hdc, NTGDI_OBJ_SURF );
    bmp = GDI_GetObjPtr( bitmap, NTGDI_OBJ_BITMAP );
    if (!bmp) return FALSE;

    if (init_dib_info_from_bitmapobj( &dib, bmp ))
    {
        char *bits;
        int width = dib.rect.right - dib.rect.left;
        int height = dib.rect.bottom - dib.rect.top;
        GLenum type;

        if (dib.stride < 0) bits = (char *)dib.bits.ptr + (dib.rect.bottom - 1) * dib.stride;
        else bits = (char *)dib.bits.ptr + dib.rect.top * dib.stride;
        bits += dib.rect.left * dib.bit_count / 8;

        TRACE( "context %p bits %p size %ux%u\n", context, bits, width, height );

        type = context->format == OSMESA_RGB_565 ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;
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

static BOOL osmesa_context_flush( void *private, HWND hwnd, HDC hdc, int interval, BOOL finish )
{
    return FALSE;
}

static const struct opengl_driver_funcs osmesa_driver_funcs =
{
    .p_get_proc_address = osmesa_get_proc_address,
    .p_init_pixel_formats = osmesa_init_pixel_formats,
    .p_describe_pixel_format = osmesa_describe_pixel_format,
    .p_init_wgl_extensions = osmesa_init_wgl_extensions,
    .p_set_pixel_format = osmesa_set_pixel_format,
    .p_swap_buffers = osmesa_swap_buffers,
    .p_context_create = osmesa_context_create,
    .p_context_destroy = osmesa_context_destroy,
    .p_context_copy = osmesa_context_copy,
    .p_context_share = osmesa_context_share,
    .p_context_flush = osmesa_context_flush,
    .p_context_make_current = osmesa_context_make_current,
};

#else  /* SONAME_LIBOSMESA */

static struct opengl_funcs *osmesa_get_wgl_driver( const struct opengl_driver_funcs **driver_funcs )
{
    return NULL;
}

#endif  /* SONAME_LIBOSMESA */

static void *nulldrv_get_proc_address( const char *name )
{
    return NULL;
}

static UINT nulldrv_init_pixel_formats( UINT *onscreen_count )
{
    *onscreen_count = ARRAY_SIZE(pixel_formats);
    return ARRAY_SIZE(pixel_formats);
}

static BOOL nulldrv_describe_pixel_format( int format, struct wgl_pixel_format *desc )
{
    if (format <= 0 || format > ARRAY_SIZE(pixel_formats)) return FALSE;
    describe_pixel_format( format, &desc->pfd );
    return TRUE;
}

static const char *nulldrv_init_wgl_extensions(void)
{
    return "";
}

static BOOL nulldrv_set_pixel_format( HWND hwnd, int old_format, int new_format, BOOL internal )
{
    return TRUE;
}

static BOOL nulldrv_pbuffer_create( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                    GLint max_level, GLsizei *width, GLsizei *height, void **private )
{
    return FALSE;
}

static BOOL nulldrv_pbuffer_destroy( HDC hdc, void *private )
{
    return FALSE;
}

static BOOL nulldrv_pbuffer_updated( HDC hdc, void *private, GLenum cube_face, GLint mipmap_level )
{
    return GL_TRUE;
}

static UINT nulldrv_pbuffer_bind( HDC hdc, void *private, GLenum buffer )
{
    return -1; /* use default implementation */
}

static BOOL nulldrv_context_create( HDC hdc, int format, void *share, const int *attribs, void **private )
{
    return FALSE;
}

static BOOL nulldrv_context_destroy( void *private )
{
    return FALSE;
}

static BOOL nulldrv_context_copy( void *src_private, void *dst_private, UINT mask )
{
    return FALSE;
}

static BOOL nulldrv_context_share( void *src_private, void *dst_private )
{
    return FALSE;
}

static BOOL nulldrv_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    return FALSE;
}

static const struct opengl_driver_funcs nulldrv_funcs =
{
    .p_get_proc_address = nulldrv_get_proc_address,
    .p_init_pixel_formats = nulldrv_init_pixel_formats,
    .p_describe_pixel_format = nulldrv_describe_pixel_format,
    .p_init_wgl_extensions = nulldrv_init_wgl_extensions,
    .p_set_pixel_format = nulldrv_set_pixel_format,
    .p_pbuffer_create = nulldrv_pbuffer_create,
    .p_pbuffer_destroy = nulldrv_pbuffer_destroy,
    .p_pbuffer_updated = nulldrv_pbuffer_updated,
    .p_pbuffer_bind = nulldrv_pbuffer_bind,
    .p_context_create = nulldrv_context_create,
    .p_context_destroy = nulldrv_context_destroy,
    .p_context_copy = nulldrv_context_copy,
    .p_context_share = nulldrv_context_share,
    .p_context_make_current = nulldrv_context_make_current,
};

static const struct opengl_driver_funcs *memory_driver_funcs = &nulldrv_funcs;
static const struct opengl_driver_funcs *display_driver_funcs = &nulldrv_funcs;
static UINT display_formats_count, display_onscreen_count;
static UINT memory_formats_count, memory_onscreen_count;

static char wgl_extensions[4096];

static const char *win32u_wglGetExtensionsStringARB( HDC hdc )
{
    if (TRACE_ON(wgl)) dump_extensions( wgl_extensions );
    return wgl_extensions;
}

static const char *win32u_wglGetExtensionsStringEXT(void)
{
    if (TRACE_ON(wgl)) dump_extensions( wgl_extensions );
    return wgl_extensions;
}

static const struct opengl_funcs *default_funcs; /* default GL function table from opengl32 */
static struct opengl_funcs *display_funcs;
static struct opengl_funcs *memory_funcs;

static PFN_glFinish p_memory_glFinish, p_display_glFinish;
static PFN_glFlush p_memory_glFlush, p_display_glFlush;

static int get_dc_pixel_format( HDC hdc, BOOL internal )
{
    int ret = 0;
    HWND hwnd;
    DC *dc;

    if ((hwnd = NtUserWindowFromDC( hdc )))
        ret = get_window_pixel_format( hwnd, internal );
    else if ((dc = get_dc_ptr( hdc )))
    {
        BOOL is_display = dc->is_display;
        ret = dc->pixel_format;
        release_dc_ptr( dc );

        /* Offscreen formats can't be used with traditional WGL calls. As has been
         * verified on Windows GetPixelFormat doesn't fail but returns 1.
         */
        if (is_display && ret >= 0 && ret > display_onscreen_count) ret = 1;
    }
    else
    {
        WARN( "Invalid DC handle %p\n", hdc );
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return -1;
    }

    TRACE( "%p/%p -> %d\n", hdc, hwnd, ret );
    return ret;
}

static int win32u_wglGetPixelFormat( HDC hdc )
{
    int format = get_dc_pixel_format( hdc, FALSE );
    return format > 0 ? format : 0;
}

static BOOL set_dc_pixel_format( HDC hdc, int new_format, BOOL internal )
{
    const struct opengl_funcs *funcs;
    UINT total, onscreen;
    HWND hwnd;

    if (!(funcs = get_dc_funcs( hdc, NULL ))) return FALSE;
    funcs->p_get_pixel_formats( NULL, 0, &total, &onscreen );
    if (new_format <= 0 || new_format > total) return FALSE;

    if ((hwnd = NtUserWindowFromDC( hdc )))
    {
        int old_format;

        if (new_format > onscreen)
        {
            WARN( "Invalid format %d for %p/%p\n", new_format, hdc, hwnd );
            return FALSE;
        }

        TRACE( "%p/%p format %d, internal %u\n", hdc, hwnd, new_format, internal );

        if ((old_format = get_window_pixel_format( hwnd, FALSE )) && !internal) return old_format == new_format;
        if (!display_driver_funcs->p_set_pixel_format( hwnd, old_format, new_format, internal )) return FALSE;
        return set_window_pixel_format( hwnd, new_format, internal );
    }

    TRACE( "%p/%p format %d, internal %u\n", hdc, hwnd, new_format, internal );
    return NtGdiSetPixelFormat( hdc, new_format );
}

static BOOL win32u_wglSetPixelFormat( HDC hdc, int format, const PIXELFORMATDESCRIPTOR *pfd )
{
    return set_dc_pixel_format( hdc, format, FALSE );
}

static BOOL win32u_wglSetPixelFormatWINE( HDC hdc, int format )
{
    return set_dc_pixel_format( hdc, format, TRUE );
}

static PROC win32u_memory_wglGetProcAddress( const char *name )
{
    PROC ret;
    if (!strncmp( name, "wgl", 3 )) return NULL;
    ret = memory_driver_funcs->p_get_proc_address( name );
    TRACE( "%s -> %p\n", debugstr_a(name), ret );
    return ret;
}

static PROC win32u_display_wglGetProcAddress( const char *name )
{
    PROC ret;
    if (!strncmp( name, "wgl", 3 )) return NULL;
    ret = display_driver_funcs->p_get_proc_address( name );
    TRACE( "%s -> %p\n", debugstr_a(name), ret );
    return ret;
}

static void win32u_display_get_pixel_formats( struct wgl_pixel_format *formats, UINT max_formats,
                                              UINT *num_formats, UINT *num_onscreen_formats )
{
    UINT i = 0;

    if (formats) while (i < max_formats && display_driver_funcs->p_describe_pixel_format( i + 1, &formats[i] )) i++;
    *num_formats = display_formats_count;
    *num_onscreen_formats = display_onscreen_count;
}

static void win32u_memory_get_pixel_formats( struct wgl_pixel_format *formats, UINT max_formats,
                                             UINT *num_formats, UINT *num_onscreen_formats )
{
    UINT i = 0;

    if (formats) while (i < max_formats && memory_driver_funcs->p_describe_pixel_format( i + 1, &formats[i] )) i++;
    *num_formats = memory_formats_count;
    *num_onscreen_formats = memory_onscreen_count;
}

static struct wgl_context *context_create( HDC hdc, struct wgl_context *shared, const int *attribs )
{
    void *shared_private = shared ? shared->driver_private : NULL;
    const struct opengl_driver_funcs *driver_funcs;
    const struct opengl_funcs *funcs;
    struct wgl_context *context;
    int format;

    TRACE( "hdc %p, shared %p, attribs %p\n", hdc, shared, attribs );

    if ((format = get_dc_pixel_format( hdc, TRUE )) <= 0)
    {
        if (!format) RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return NULL;
    }

    if (!(funcs = get_dc_funcs( hdc, NULL ))) return NULL;
    driver_funcs = funcs == display_funcs ? display_driver_funcs : memory_driver_funcs;

    if (!(context = calloc( 1, sizeof(*context) ))) return NULL;
    context->driver_funcs = driver_funcs;
    context->funcs = funcs;
    context->pixel_format = format;
    context->p_glFinish = funcs == display_funcs ? p_display_glFinish : p_memory_glFinish;
    context->p_glFlush = funcs == display_funcs ? p_display_glFlush : p_memory_glFlush;

    if (!driver_funcs->p_context_create( hdc, format, shared_private, attribs, &context->driver_private ))
    {
        free( context );
        return NULL;
    }

    TRACE( "created context %p, format %u for driver context %p\n", context, format, context->driver_private );
    return context;
}

static struct wgl_context *win32u_wglCreateContextAttribsARB( HDC hdc, struct wgl_context *shared, const int *attribs )
{
    static const int empty_attribs = {0};

    TRACE( "hdc %p, shared %p, attribs %p\n", hdc, shared, attribs );

    if (!attribs) attribs = &empty_attribs;
    return context_create( hdc, shared, attribs );
}

static struct wgl_context *win32u_wglCreateContext( HDC hdc )
{
    TRACE( "hdc %p\n", hdc );
    return context_create( hdc, NULL, NULL );
}

static BOOL win32u_wglDeleteContext( struct wgl_context *context )
{
    const struct opengl_driver_funcs *funcs = context->driver_funcs;
    BOOL ret;

    TRACE( "context %p\n", context );

    ret = funcs->p_context_destroy( context->driver_private );
    free( context );

    return ret;
}

static BOOL win32u_wglCopyContext( struct wgl_context *src, struct wgl_context *dst, UINT mask )
{
    const struct opengl_driver_funcs *funcs = src->driver_funcs;

    TRACE( "src %p, dst %p, mask %#x\n", src, dst, mask );

    if (funcs != dst->driver_funcs) return FALSE;
    return funcs->p_context_copy( src->driver_private, dst->driver_private, mask );
}

static BOOL win32u_wglShareLists( struct wgl_context *src, struct wgl_context *dst )
{
    const struct opengl_driver_funcs *funcs = src->driver_funcs;

    TRACE( "src %p, dst %p\n", src, dst );

    if (funcs != dst->driver_funcs) return FALSE;
    return funcs->p_context_share( src->driver_private, dst->driver_private );
}

static BOOL win32u_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, struct wgl_context *context )
{
    const struct opengl_driver_funcs *funcs;
    int format;

    TRACE( "draw_hdc %p, read_hdc %p, context %p\n", draw_hdc, read_hdc, context );

    if (!context)
    {
        if (!(context = NtCurrentTeb()->glContext)) return TRUE;
        funcs = context->driver_funcs;
        if (!funcs->p_context_make_current( NULL, NULL, NULL )) return FALSE;
        NtCurrentTeb()->glContext = NULL;
        return TRUE;
    }

    if ((format = get_dc_pixel_format( draw_hdc, TRUE )) <= 0)
    {
        WARN( "Invalid draw_hdc %p format %u\n", draw_hdc, format );
        if (!format) RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }
    if (context->pixel_format != format)
    {
        WARN( "Mismatched draw_hdc %p format %u, context %p format %u\n", draw_hdc, format, context, context->pixel_format );
        RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }

    funcs = context->driver_funcs;
    if (!funcs->p_context_make_current( draw_hdc, read_hdc, context->driver_private )) return FALSE;
    NtCurrentTeb()->glContext = context;
    return TRUE;
}

static BOOL win32u_wglMakeCurrent( HDC hdc, struct wgl_context *context )
{
    return win32u_wglMakeContextCurrentARB( hdc, hdc, context );
}

static struct wgl_pbuffer *win32u_wglCreatePbufferARB( HDC hdc, int format, int width, int height,
                                                       const int *attribs )
{
    UINT total, onscreen, size, max_level = 0;
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    BOOL largest = FALSE;

    TRACE( "(%p, %d, %d, %d, %p)\n", hdc, format, width, height, attribs );

    if (!(funcs = get_dc_funcs( hdc, NULL ))) return FALSE;
    funcs->p_get_pixel_formats( NULL, 0, &total, &onscreen );
    if (format <= 0 || format > total)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return NULL;
    }
    if (width <= 0 || height <= 0)
    {
        RtlSetLastWin32Error( ERROR_INVALID_DATA );
        return NULL;
    }

    if (!(pbuffer = calloc( 1, sizeof(*pbuffer) )) || !(pbuffer->hdc = NtGdiOpenDCW( NULL, NULL, NULL, 0, TRUE, NULL, NULL, NULL )))
    {
        RtlSetLastWin32Error( ERROR_NO_SYSTEM_RESOURCES );
        free( pbuffer );
        return NULL;
    }
    NtGdiSetPixelFormat( pbuffer->hdc, format );
    pbuffer->driver_funcs = funcs == memory_funcs ? memory_driver_funcs : display_driver_funcs;
    pbuffer->funcs = funcs;
    pbuffer->width = width;
    pbuffer->height = height;
    pbuffer->mipmap_level = -1;

    for (; attribs && attribs[0]; attribs += 2)
    {
        switch (attribs[0])
        {
        case WGL_PBUFFER_LARGEST_ARB:
            TRACE( "WGL_PBUFFER_LARGEST_ARB %#x\n", attribs[1] );
            largest = !!attribs[1];
            break;

        case WGL_TEXTURE_FORMAT_ARB:
            TRACE( "WGL_TEXTURE_FORMAT_ARB %#x\n", attribs[1] );
            switch (attribs[1])
            {
            case WGL_NO_TEXTURE_ARB:
                pbuffer->texture_format = 0;
                break;
            case WGL_TEXTURE_RGB_ARB:
                pbuffer->texture_format = GL_RGB;
                break;
            case WGL_TEXTURE_RGBA_ARB:
                pbuffer->texture_format = GL_RGBA;
                break;
            /* WGL_FLOAT_COMPONENTS_NV */
            case WGL_TEXTURE_FLOAT_R_NV:
                pbuffer->texture_format = GL_FLOAT_R_NV;
                break;
            case WGL_TEXTURE_FLOAT_RG_NV:
                pbuffer->texture_format = GL_FLOAT_RG_NV;
                break;
            case WGL_TEXTURE_FLOAT_RGB_NV:
                pbuffer->texture_format = GL_FLOAT_RGB_NV;
                break;
            case WGL_TEXTURE_FLOAT_RGBA_NV:
                pbuffer->texture_format = GL_FLOAT_RGBA_NV;
                break;
            default:
                FIXME( "Unknown texture format: %x\n", attribs[1] );
                goto failed;
            }
            break;

        case WGL_TEXTURE_TARGET_ARB:
            TRACE( "WGL_TEXTURE_TARGET_ARB %#x\n", attribs[1] );
            switch (attribs[1])
            {
            case WGL_NO_TEXTURE_ARB:
                pbuffer->texture_target = 0;
                break;
            case WGL_TEXTURE_CUBE_MAP_ARB:
                if (width != height) goto failed;
                pbuffer->texture_target = GL_TEXTURE_CUBE_MAP;
                break;
            case WGL_TEXTURE_1D_ARB:
                if (height != 1) goto failed;
                pbuffer->texture_target = GL_TEXTURE_1D;
                break;
            case WGL_TEXTURE_2D_ARB:
                pbuffer->texture_target = GL_TEXTURE_2D;
                break;
            case WGL_TEXTURE_RECTANGLE_NV:
                pbuffer->texture_target = GL_TEXTURE_RECTANGLE_NV;
                break;
            default:
                FIXME( "Unknown texture target: %x\n", attribs[1] );
                goto failed;
            }
            break;

        case WGL_MIPMAP_TEXTURE_ARB:
            TRACE( "WGL_MIPMAP_TEXTURE_ARB %#x\n", attribs[1] );
            if (attribs[1])
            {
                pbuffer->mipmap_level = max_level = 0;
                for (size = min( width, height ) / 2; size; size /= 2) max_level++;
            }
            break;

        default:
            WARN( "attribute %#x %#x not handled\n", attribs[0], attribs[1] );
            break;
        }
    }

    if (pbuffer->driver_funcs->p_pbuffer_create( pbuffer->hdc, format, largest, pbuffer->texture_format,
                                                 pbuffer->texture_target, max_level, &pbuffer->width,
                                                 &pbuffer->height, &pbuffer->driver_private ))
        return pbuffer;

failed:
    RtlSetLastWin32Error( ERROR_INVALID_DATA );
    NtGdiDeleteObjectApp( pbuffer->hdc );
    free( pbuffer );
    return NULL;
}

static BOOL win32u_wglDestroyPbufferARB( struct wgl_pbuffer *pbuffer )
{
    TRACE( "pbuffer %p\n", pbuffer );

    pbuffer->driver_funcs->p_pbuffer_destroy( pbuffer->hdc, pbuffer->driver_private );
    if (pbuffer->tmp_context) pbuffer->funcs->p_wglDeleteContext( pbuffer->tmp_context );
    NtGdiDeleteObjectApp( pbuffer->hdc );
    free( pbuffer );

    return GL_TRUE;
}

static HDC win32u_wglGetPbufferDCARB( struct wgl_pbuffer *pbuffer )
{
    TRACE( "pbuffer %p\n", pbuffer );
    return pbuffer->hdc;
}

static int win32u_wglReleasePbufferDCARB( struct wgl_pbuffer *pbuffer, HDC hdc )
{
    TRACE( "pbuffer %p, hdc %p\n", pbuffer, hdc );

    if (hdc != pbuffer->hdc)
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return FALSE;
    }

    return TRUE;
}

static BOOL win32u_wglQueryPbufferARB( struct wgl_pbuffer *pbuffer, int attrib, int *value )
{
    TRACE( "pbuffer %p, attrib %#x, value %p\n", pbuffer, attrib, value );

    switch (attrib)
    {
    case WGL_PBUFFER_WIDTH_ARB:
        *value = pbuffer->width;
        break;
    case WGL_PBUFFER_HEIGHT_ARB:
        *value = pbuffer->height;
        break;
    case WGL_PBUFFER_LOST_ARB:
        *value = GL_FALSE;
        break;

    case WGL_TEXTURE_FORMAT_ARB:
        switch (pbuffer->texture_format)
        {
        case 0: *value = WGL_NO_TEXTURE_ARB; break;
        case GL_RGB: *value = WGL_TEXTURE_RGB_ARB; break;
        case GL_RGBA: *value = WGL_TEXTURE_RGBA_ARB; break;
        /* WGL_FLOAT_COMPONENTS_NV */
        case GL_FLOAT_R_NV: *value = WGL_TEXTURE_FLOAT_R_NV; break;
        case GL_FLOAT_RG_NV: *value = WGL_TEXTURE_FLOAT_RG_NV; break;
        case GL_FLOAT_RGB_NV: *value = WGL_TEXTURE_FLOAT_RGB_NV; break;
        case GL_FLOAT_RGBA_NV: *value = WGL_TEXTURE_FLOAT_RGBA_NV; break;
        default: ERR( "Unknown texture format: %x\n", pbuffer->texture_format );
        }
        break;

    case WGL_TEXTURE_TARGET_ARB:
        switch (pbuffer->texture_target)
        {
        case 0: *value = WGL_NO_TEXTURE_ARB; break;
        case GL_TEXTURE_1D: *value = WGL_TEXTURE_1D_ARB; break;
        case GL_TEXTURE_2D: *value = WGL_TEXTURE_2D_ARB; break;
        case GL_TEXTURE_CUBE_MAP: *value = WGL_TEXTURE_CUBE_MAP_ARB; break;
        case GL_TEXTURE_RECTANGLE_NV: *value = WGL_TEXTURE_RECTANGLE_NV; break;
        }
        break;

    case WGL_MIPMAP_TEXTURE_ARB:
        *value = pbuffer->mipmap_level >= 0;
        break;
    case WGL_MIPMAP_LEVEL_ARB:
        *value = max( pbuffer->mipmap_level, 0 );
        break;
    case WGL_CUBE_MAP_FACE_ARB:
        switch (pbuffer->cube_face)
        {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        default:
            *value = WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            *value = WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            *value = WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            *value = WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            *value = WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            *value = WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
            break;
        }
        break;

    default:
        FIXME( "unexpected attribute %x\n", attrib );
        break;
    }

    return GL_TRUE;
}

static GLenum binding_from_target( GLenum target )
{
    switch (target)
    {
    case GL_TEXTURE_CUBE_MAP: return GL_TEXTURE_BINDING_CUBE_MAP;
    case GL_TEXTURE_1D: return GL_TEXTURE_BINDING_1D;
    case GL_TEXTURE_2D: return GL_TEXTURE_BINDING_2D;
    case GL_TEXTURE_RECTANGLE_NV: return GL_TEXTURE_BINDING_RECTANGLE_NV;
    }
    FIXME( "Unsupported target %#x\n", target );
    return 0;
}

static BOOL win32u_wglBindTexImageARB( struct wgl_pbuffer *pbuffer, int buffer )
{
    HDC prev_draw = NtCurrentTeb()->glReserved1[0], prev_read = NtCurrentTeb()->glReserved1[1];
    int prev_texture = 0, format = win32u_wglGetPixelFormat( pbuffer->hdc );
    struct wgl_context *prev_context = NtCurrentTeb()->glContext;
    struct wgl_pixel_format desc;
    GLenum source;
    UINT ret;

    TRACE( "pbuffer %p, buffer %d\n", pbuffer, buffer );

    if (!pbuffer->texture_format)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return GL_FALSE;
    }

    if (!pbuffer->driver_funcs->p_describe_pixel_format( format, &desc ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }

    switch (buffer)
    {
    case WGL_FRONT_LEFT_ARB:
        if (desc.pfd.dwFlags & PFD_STEREO) source = GL_FRONT_LEFT;
        else source = GL_FRONT;
        break;
    case WGL_FRONT_RIGHT_ARB:
        source = GL_FRONT_RIGHT;
        break;
    case WGL_BACK_LEFT_ARB:
        if (desc.pfd.dwFlags & PFD_STEREO) source = GL_BACK_LEFT;
        else source = GL_BACK;
        break;
    case WGL_BACK_RIGHT_ARB:
        source = GL_BACK_RIGHT;
        break;
    case WGL_AUX0_ARB: source = GL_AUX0; break;
    case WGL_AUX1_ARB: source = GL_AUX1; break;
    case WGL_AUX2_ARB: source = GL_AUX2; break;
    case WGL_AUX3_ARB: source = GL_AUX3; break;

    case WGL_AUX4_ARB:
    case WGL_AUX5_ARB:
    case WGL_AUX6_ARB:
    case WGL_AUX7_ARB:
    case WGL_AUX8_ARB:
    case WGL_AUX9_ARB:
        FIXME( "Unsupported source buffer %#x\n", buffer );
        RtlSetLastWin32Error( ERROR_INVALID_DATA );
        return GL_FALSE;

    default:
        WARN( "Unknown source buffer %#x\n", buffer );
        RtlSetLastWin32Error( ERROR_INVALID_DATA );
        return GL_FALSE;
    }

    if ((ret = pbuffer->driver_funcs->p_pbuffer_bind( pbuffer->hdc, pbuffer->driver_private, source )) != -1)
        return ret;

    if (!pbuffer->tmp_context || pbuffer->prev_context != prev_context)
    {
        if (pbuffer->tmp_context) pbuffer->funcs->p_wglDeleteContext( pbuffer->tmp_context );
        pbuffer->tmp_context = pbuffer->funcs->p_wglCreateContextAttribsARB( pbuffer->hdc, prev_context, NULL );
        pbuffer->prev_context = prev_context;
    }

    pbuffer->funcs->p_glGetIntegerv( binding_from_target( pbuffer->texture_target ), &prev_texture );

    /* Switch to our pbuffer */
    pbuffer->funcs->p_wglMakeCurrent( pbuffer->hdc, pbuffer->tmp_context );

    /* Make sure that the prev_texture is set as the current texture state isn't shared
     * between contexts. After that copy the pbuffer texture data. */
    pbuffer->funcs->p_glBindTexture( pbuffer->texture_target, prev_texture );
    pbuffer->funcs->p_glCopyTexImage2D( pbuffer->texture_target, 0, pbuffer->texture_format, 0, 0,
                                        pbuffer->width, pbuffer->height, 0 );

    /* Switch back to the original drawable and context */
    pbuffer->funcs->p_wglMakeContextCurrentARB( prev_draw, prev_read, prev_context );
    return GL_TRUE;
}

static BOOL win32u_wglReleaseTexImageARB( struct wgl_pbuffer *pbuffer, int buffer )
{
    TRACE( "pbuffer %p, buffer %d\n", pbuffer, buffer );

    if (!pbuffer->texture_format)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return GL_FALSE;
    }

    return !!pbuffer->driver_funcs->p_pbuffer_bind( pbuffer->hdc, pbuffer->driver_private, GL_NONE );
}

static BOOL win32u_wglSetPbufferAttribARB( struct wgl_pbuffer *pbuffer, const int *attribs )
{
    TRACE( "pbuffer %p, attribs %p\n", pbuffer, attribs );

    if (!pbuffer->texture_format)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return GL_FALSE;
    }

    for (; attribs && attribs[0]; attribs += 2)
    {
        switch (attribs[0])
        {
        case WGL_MIPMAP_LEVEL_ARB:
            TRACE( "WGL_MIPMAP_LEVEL_ARB %#x\n", attribs[1] );
            pbuffer->mipmap_level = attribs[1];
            break;

        case WGL_CUBE_MAP_FACE_ARB:
            TRACE( "WGL_CUBE_MAP_FACE_ARB %#x\n", attribs[1] );
            switch (attribs[1])
            {
            case WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
                pbuffer->cube_face = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                break;
            case WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
                pbuffer->cube_face = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                break;
            case WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
                pbuffer->cube_face = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                break;
            case WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
                pbuffer->cube_face = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                break;
            case WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
                pbuffer->cube_face = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                break;
            case WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
                pbuffer->cube_face = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                break;
            default:
                FIXME( "Unknown texture face: %x\n", attribs[1] );
                RtlSetLastWin32Error( ERROR_INVALID_DATA );
                return GL_FALSE;
            }
            break;

        default:
            FIXME( "Invalid attribute 0x%x\n", attribs[0] );
            RtlSetLastWin32Error( ERROR_INVALID_DATA );
            return GL_FALSE;
        }
    }

    return pbuffer->driver_funcs->p_pbuffer_updated( pbuffer->hdc, pbuffer->driver_private,
                                                     pbuffer->cube_face, max( pbuffer->mipmap_level, 0 ) );
}

static int get_window_swap_interval( HWND hwnd )
{
    int interval;
    WND *win;

    if (!(win = get_win_ptr( hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return 0;
    interval = win->swap_interval;
    release_win_ptr( win );

    return interval;
}

static void wgl_context_flush( struct wgl_context *context, BOOL finish )
{
    HDC hdc = NtCurrentTeb()->glReserved1[0];
    int interval;
    HWND hwnd;

    if (!(hwnd = NtUserWindowFromDC( hdc ))) interval = 0;
    else interval = get_window_swap_interval( hwnd );

    TRACE( "context %p, hwnd %p, hdc %p, interval %d, finish %u\n", context, hwnd, hdc, interval, finish );

    if (!context->driver_funcs->p_context_flush( context->driver_private, hwnd, hdc, interval, finish ))
    {
        /* default implementation: call the hooked functions */
        if (finish) context->p_glFinish();
        else context->p_glFlush();
    }
}

static BOOL win32u_wglSwapBuffers( HDC hdc )
{
    struct wgl_context *context = NtCurrentTeb()->glContext;
    const struct opengl_driver_funcs *driver_funcs;
    const struct opengl_funcs *funcs;
    int interval;
    HWND hwnd;

    if (!(funcs = get_dc_funcs( hdc, NULL )))
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return FALSE;
    }
    driver_funcs = funcs == display_funcs ? display_driver_funcs : memory_driver_funcs;

    if (!(hwnd = NtUserWindowFromDC( hdc ))) interval = 0;
    else interval = get_window_swap_interval( hwnd );

    if (!driver_funcs->p_swap_buffers( context ? context->driver_private : NULL, hwnd, hdc, interval ))
    {
        /* default implementation: implicitly flush the context */
        if (context) wgl_context_flush( context, FALSE );
        return FALSE;
    }

    return TRUE;
}

static BOOL win32u_wglSwapIntervalEXT( int interval )
{
    HDC hdc = NtCurrentTeb()->glReserved1[0];
    HWND hwnd;
    WND *win;

    if (!(hwnd = NtUserWindowFromDC( hdc )) || !(win = get_win_ptr( hwnd )))
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return FALSE;
    }
    if (win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        WARN( "setting swap interval on win %p not supported\n", hwnd );
        return TRUE;
    }

    TRACE( "setting window %p swap interval %d\n", hwnd, interval );
    win->swap_interval = interval;
    release_win_ptr( win );
    return TRUE;
}

static int win32u_wglGetSwapIntervalEXT(void)
{
    HDC hdc = NtCurrentTeb()->glReserved1[0];
    int interval;
    HWND hwnd;
    WND *win;

    if (!(hwnd = NtUserWindowFromDC( hdc )) || !(win = get_win_ptr( hwnd )))
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return 0;
    }
    if (win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        WARN( "setting swap interval on win %p not supported\n", hwnd );
        return TRUE;
    }
    interval = win->swap_interval;
    release_win_ptr( win );

    return interval;
}

static void win32u_glFlush(void)
{
    struct wgl_context *context = NtCurrentTeb()->glContext;
    if (context) wgl_context_flush( context, FALSE );
}

static void win32u_glFinish(void)
{
    struct wgl_context *context = NtCurrentTeb()->glContext;
    if (context) wgl_context_flush( context, TRUE );
}

static void init_opengl_funcs( struct opengl_funcs *funcs, const struct opengl_driver_funcs *driver_funcs )
{
#define USE_GL_FUNC(func) \
    if (!funcs->p_##func && !(funcs->p_##func = driver_funcs->p_get_proc_address( #func ))) \
    { \
        WARN( "%s not found for memory DCs.\n", #func ); \
        funcs->p_##func = default_funcs->p_##func; \
    }
    ALL_GL_FUNCS
#undef USE_GL_FUNC
}

static void memory_funcs_init(void)
{
    memory_funcs = osmesa_get_wgl_driver( &memory_driver_funcs );
    if (memory_funcs && !(memory_formats_count = memory_driver_funcs->p_init_pixel_formats( &memory_onscreen_count ))) memory_funcs = NULL;
    if (!memory_funcs) return;

    init_opengl_funcs( memory_funcs, memory_driver_funcs );

    memory_funcs->p_wglGetProcAddress = win32u_memory_wglGetProcAddress;
    memory_funcs->p_get_pixel_formats = win32u_memory_get_pixel_formats;

    memory_funcs->p_wglGetPixelFormat = win32u_wglGetPixelFormat;
    memory_funcs->p_wglSetPixelFormat = win32u_wglSetPixelFormat;

    memory_funcs->p_wglCreateContext = win32u_wglCreateContext;
    memory_funcs->p_wglDeleteContext = win32u_wglDeleteContext;
    memory_funcs->p_wglCopyContext = win32u_wglCopyContext;
    memory_funcs->p_wglShareLists = win32u_wglShareLists;
    memory_funcs->p_wglMakeCurrent = win32u_wglMakeCurrent;

    memory_funcs->p_wglSwapBuffers = win32u_wglSwapBuffers;
    p_memory_glFinish = memory_funcs->p_glFinish;
    memory_funcs->p_glFinish = win32u_glFinish;
    p_memory_glFlush = memory_funcs->p_glFlush;
    memory_funcs->p_glFlush = win32u_glFlush;
}

static void display_funcs_init(void)
{
    UINT status;

    if ((status = user_driver->pOpenGLInit( WINE_OPENGL_DRIVER_VERSION, &display_funcs, &display_driver_funcs )) &&
        status != STATUS_NOT_IMPLEMENTED)
    {
        ERR( "Failed to initialize the driver opengl functions, status %#x\n", status );
        return;
    }
    if (display_funcs && !(display_formats_count = display_driver_funcs->p_init_pixel_formats( &display_onscreen_count ))) display_funcs = NULL;
    if (!display_funcs) return;

    init_opengl_funcs( display_funcs, display_driver_funcs );

    display_funcs->p_wglGetProcAddress = win32u_display_wglGetProcAddress;
    display_funcs->p_get_pixel_formats = win32u_display_get_pixel_formats;

    strcpy( wgl_extensions, display_driver_funcs->p_init_wgl_extensions() );
    display_funcs->p_wglGetPixelFormat = win32u_wglGetPixelFormat;
    display_funcs->p_wglSetPixelFormat = win32u_wglSetPixelFormat;

    display_funcs->p_wglCreateContext = win32u_wglCreateContext;
    display_funcs->p_wglDeleteContext = win32u_wglDeleteContext;
    display_funcs->p_wglCopyContext = win32u_wglCopyContext;
    display_funcs->p_wglShareLists = win32u_wglShareLists;
    display_funcs->p_wglMakeCurrent = win32u_wglMakeCurrent;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_extensions_string" );
    display_funcs->p_wglGetExtensionsStringARB = win32u_wglGetExtensionsStringARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_extensions_string" );
    display_funcs->p_wglGetExtensionsStringEXT = win32u_wglGetExtensionsStringEXT;

    /* In WineD3D we need the ability to set the pixel format more than once (e.g. after a device reset).
     * The default wglSetPixelFormat doesn't allow this, so add our own which allows it.
     */
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_WINE_pixel_format_passthrough" );
    display_funcs->p_wglSetPixelFormatWINE = win32u_wglSetPixelFormatWINE;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_pixel_format" );
    display_funcs->p_wglChoosePixelFormatARB      = (void *)1; /* never called */
    display_funcs->p_wglGetPixelFormatAttribfvARB = (void *)1; /* never called */
    display_funcs->p_wglGetPixelFormatAttribivARB = (void *)1; /* never called */

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_create_context" );
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_create_context_no_error" );
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_create_context_profile" );
    display_funcs->p_wglCreateContextAttribsARB = win32u_wglCreateContextAttribsARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_make_current_read" );
    display_funcs->p_wglGetCurrentReadDCARB   = (void *)1;  /* never called */
    display_funcs->p_wglMakeContextCurrentARB = win32u_wglMakeContextCurrentARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_pbuffer" );
    display_funcs->p_wglCreatePbufferARB    = win32u_wglCreatePbufferARB;
    display_funcs->p_wglDestroyPbufferARB   = win32u_wglDestroyPbufferARB;
    display_funcs->p_wglGetPbufferDCARB     = win32u_wglGetPbufferDCARB;
    display_funcs->p_wglReleasePbufferDCARB = win32u_wglReleasePbufferDCARB;
    display_funcs->p_wglQueryPbufferARB     = win32u_wglQueryPbufferARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_render_texture" );
    display_funcs->p_wglBindTexImageARB     = win32u_wglBindTexImageARB;
    display_funcs->p_wglReleaseTexImageARB  = win32u_wglReleaseTexImageARB;
    display_funcs->p_wglSetPbufferAttribARB = win32u_wglSetPbufferAttribARB;

    if (display_driver_funcs->p_swap_buffers)
    {
        display_funcs->p_wglSwapBuffers = win32u_wglSwapBuffers;
        p_display_glFinish = display_funcs->p_glFinish;
        display_funcs->p_glFinish = win32u_glFinish;
        p_display_glFlush = display_funcs->p_glFlush;
        display_funcs->p_glFlush = win32u_glFlush;

        register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_swap_control" );
        register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_swap_control_tear" );
        display_funcs->p_wglSwapIntervalEXT = win32u_wglSwapIntervalEXT;
        display_funcs->p_wglGetSwapIntervalEXT = win32u_wglGetSwapIntervalEXT;
    }
}

static const struct opengl_funcs *get_dc_funcs( HDC hdc, const struct opengl_funcs *null_funcs )
{
    DWORD is_disabled, is_display, is_memdc;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return NULL;
    is_memdc = get_gdi_object_type( hdc ) == NTGDI_OBJ_MEMDC;
    is_display = dc->is_display;
    is_disabled = dc->attr->disabled;
    release_dc_ptr( dc );

    if (is_disabled) return NULL;
    if (is_display)
    {
        static pthread_once_t display_init_once = PTHREAD_ONCE_INIT;
        pthread_once( &display_init_once, display_funcs_init );
        return display_funcs ? display_funcs : null_funcs;
    }
    if (is_memdc)
    {
        static pthread_once_t memory_init_once = PTHREAD_ONCE_INIT;
        pthread_once( &memory_init_once, memory_funcs_init );
        return memory_funcs ? memory_funcs : null_funcs;
    }
    return NULL;
}

/***********************************************************************
 *      __wine_get_wgl_driver  (win32u.@)
 */
const struct opengl_funcs *__wine_get_wgl_driver( HDC hdc, UINT version, const struct opengl_funcs *null_funcs )
{
    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but dibdrv has %u\n",
             version, WINE_OPENGL_DRIVER_VERSION );
        return NULL;
    }

    InterlockedExchangePointer( (void *)&default_funcs, (void *)null_funcs );
    return get_dc_funcs( hdc, null_funcs );
}
