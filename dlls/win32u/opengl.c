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

static struct opengl_funcs *get_dc_funcs( HDC hdc, void *null_funcs );

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

struct wgl_context
{
    OSMesaContext context;
    UINT          format;
};

static struct opengl_funcs osmesa_opengl_funcs;

static OSMesaContext (*pOSMesaCreateContextExt)( GLenum format, GLint depthBits, GLint stencilBits,
                                                 GLint accumBits, OSMesaContext sharelist );
static void (*pOSMesaDestroyContext)( OSMesaContext ctx );
static void * (*pOSMesaGetProcAddress)( const char *funcName );
static GLboolean (*pOSMesaMakeCurrent)( OSMesaContext ctx, void *buffer, GLenum type,
                                        GLsizei width, GLsizei height );
static void (*pOSMesaPixelStore)( GLint pname, GLint value );

static struct opengl_funcs *osmesa_get_wgl_driver(void)
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

#define USE_GL_FUNC(func) \
        if (!(osmesa_opengl_funcs.p_##func = pOSMesaGetProcAddress( #func ))) \
        { \
            ERR( "%s not found in %s, disabling.\n", #func, SONAME_LIBOSMESA ); \
            goto failed; \
        }
    ALL_GL_FUNCS
#undef USE_GL_FUNC

    return &osmesa_opengl_funcs;

failed:
    dlclose( osmesa_handle );
    osmesa_handle = NULL;
    return NULL;
}

static struct wgl_context *osmesa_create_context( HDC hdc, const PIXELFORMATDESCRIPTOR *descr )
{
    struct wgl_context *context;
    UINT gl_format;

    switch (descr->cColorBits)
    {
    case 32:
        if (descr->cRedShift == 8) gl_format = OSMESA_ARGB;
        else if (descr->cRedShift == 16) gl_format = OSMESA_BGRA;
        else gl_format = OSMESA_RGBA;
        break;
    case 24:
        gl_format = descr->cRedShift == 16 ? OSMESA_BGR : OSMESA_RGB;
        break;
    case 16:
        gl_format = OSMESA_RGB_565;
        break;
    default:
        return NULL;
    }
    if (!(context = malloc( sizeof(*context) ))) return NULL;
    context->format = gl_format;
    if (!(context->context = pOSMesaCreateContextExt( gl_format, descr->cDepthBits, descr->cStencilBits,
                                                      descr->cAccumBits, 0 )))
    {
        free( context );
        return NULL;
    }
    return context;
}

static BOOL osmesa_delete_context( struct wgl_context *context )
{
    pOSMesaDestroyContext( context->context );
    free( context );
    return TRUE;
}

static PROC osmesa_get_proc_address( const char *proc )
{
    return (PROC)pOSMesaGetProcAddress( proc );
}

static BOOL osmesa_make_current( struct wgl_context *context, void *bits,
                                 int width, int height, int bpp, int stride )
{
    BOOL ret;
    GLenum type;

    if (!context)
    {
        pOSMesaMakeCurrent( NULL, NULL, GL_UNSIGNED_BYTE, 0, 0 );
        return TRUE;
    }

    type = context->format == OSMESA_RGB_565 ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;
    ret = pOSMesaMakeCurrent( context->context, bits, type, width, height );
    if (ret)
    {
        pOSMesaPixelStore( OSMESA_ROW_LENGTH, abs( stride ) * 8 / bpp );
        pOSMesaPixelStore( OSMESA_Y_UP, 1 );  /* Windows seems to assume bottom-up */
    }
    return ret;
}

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

static BOOL osmesa_wglCopyContext( struct wgl_context *src, struct wgl_context *dst, UINT mask )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

static BOOL osmesa_wglDeleteContext( struct wgl_context *context )
{
    return osmesa_delete_context( context );
}

static struct wgl_context *osmesa_wglCreateContext( HDC hdc )
{
    PIXELFORMATDESCRIPTOR descr;
    struct opengl_funcs *funcs;
    int format;

    if (!(funcs = get_dc_funcs( hdc, NULL ))) return NULL;
    if (!(format = funcs->p_wglGetPixelFormat( hdc ))) format = 1;
    describe_pixel_format( format, &descr );

    return osmesa_create_context( hdc, &descr );
}

static PROC osmesa_wglGetProcAddress( const char *proc )
{
    if (!strncmp( proc, "wgl", 3 )) return NULL;
    return osmesa_get_proc_address( proc );
}

static BOOL osmesa_wglMakeCurrent( HDC hdc, struct wgl_context *context )
{
    HBITMAP bitmap;
    BITMAPOBJ *bmp;
    dib_info dib;
    BOOL ret = FALSE;

    if (!context) return osmesa_make_current( NULL, NULL, 0, 0, 0, 0 );

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

        ret = osmesa_make_current( context, bits, width, height, dib.bit_count, dib.stride );
    }
    GDI_ReleaseObj( bitmap );
    return ret;
}

static BOOL osmesa_wglShareLists( struct wgl_context *org, struct wgl_context *dest )
{
    FIXME( "not supported yet\n" );
    return FALSE;
}

static BOOL osmesa_wglSwapBuffers( HDC hdc )
{
    return TRUE;
}

static void osmesa_get_pixel_formats( struct wgl_pixel_format *formats, UINT max_formats,
                                      UINT *num_formats, UINT *num_onscreen_formats )
{
    UINT i, num_pixel_formats = ARRAY_SIZE( pixel_formats );

    for (i = 0; formats && i < min( max_formats, num_pixel_formats ); ++i)
        describe_pixel_format( i + 1, &formats[i].pfd );

    *num_formats = *num_onscreen_formats = num_pixel_formats;
}

static struct opengl_funcs osmesa_opengl_funcs =
{
    .p_wglCopyContext = osmesa_wglCopyContext,
    .p_wglCreateContext = osmesa_wglCreateContext,
    .p_wglDeleteContext = osmesa_wglDeleteContext,
    .p_wglGetProcAddress = osmesa_wglGetProcAddress,
    .p_wglMakeCurrent = osmesa_wglMakeCurrent,
    .p_wglShareLists = osmesa_wglShareLists,
    .p_wglSwapBuffers = osmesa_wglSwapBuffers,
    .p_get_pixel_formats = osmesa_get_pixel_formats,
};

#else  /* SONAME_LIBOSMESA */

static struct opengl_funcs *osmesa_get_wgl_driver(void)
{
    return NULL;
}

#endif  /* SONAME_LIBOSMESA */

static const char *nulldrv_init_wgl_extensions(void)
{
    return "";
}

static const struct opengl_driver_funcs nulldrv_funcs =
{
    .p_init_wgl_extensions = nulldrv_init_wgl_extensions,
};
static const struct opengl_driver_funcs *driver_funcs = &nulldrv_funcs;

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

static struct opengl_funcs *display_funcs;
static struct opengl_funcs *memory_funcs;

static int win32u_wglGetPixelFormat( HDC hdc )
{
    int ret = 0;
    HWND hwnd;
    DC *dc;

    if ((hwnd = NtUserWindowFromDC( hdc )))
        ret = win32u_get_window_pixel_format( hwnd );
    else if ((dc = get_dc_ptr( hdc )))
    {
        BOOL is_display = dc->is_display;
        UINT total, onscreen;
        ret = dc->pixel_format;
        release_dc_ptr( dc );

        if (is_display && ret >= 0)
        {
            /* Offscreen formats can't be used with traditional WGL calls. As has been
             * verified on Windows GetPixelFormat doesn't fail but returns 1.
             */
            display_funcs->p_get_pixel_formats( NULL, 0, &total, &onscreen );
            if (ret > onscreen) ret = 1;
        }
    }

    TRACE( "%p/%p -> %d\n", hdc, hwnd, ret );
    return ret;
}

static BOOL set_dc_pixel_format( HDC hdc, int new_format, BOOL internal )
{
    struct opengl_funcs *funcs;
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

        if ((old_format = win32u_get_window_pixel_format( hwnd )) && !internal) return old_format == new_format;
        if (!driver_funcs->p_set_pixel_format( hwnd, old_format, new_format, internal )) return FALSE;
        return win32u_set_window_pixel_format( hwnd, new_format, internal );
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

static void memory_funcs_init(void)
{
    memory_funcs = osmesa_get_wgl_driver();
    if (!memory_funcs) return;

    memory_funcs->p_wglGetPixelFormat = win32u_wglGetPixelFormat;
    memory_funcs->p_wglSetPixelFormat = win32u_wglSetPixelFormat;
}

static void display_funcs_init(void)
{
    UINT status;

    if ((status = user_driver->pOpenGLInit( WINE_OPENGL_DRIVER_VERSION, &display_funcs, &driver_funcs )) &&
        status != STATUS_NOT_IMPLEMENTED)
    {
        ERR( "Failed to initialize the driver opengl functions, status %#x\n", status );
        return;
    }
    if (!display_funcs) return;

    strcpy( wgl_extensions, driver_funcs->p_init_wgl_extensions() );

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_extensions_string" );
    display_funcs->p_wglGetExtensionsStringARB = win32u_wglGetExtensionsStringARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_extensions_string" );
    display_funcs->p_wglGetExtensionsStringEXT = win32u_wglGetExtensionsStringEXT;

    if (driver_funcs->p_set_pixel_format)
    {
        display_funcs->p_wglGetPixelFormat = win32u_wglGetPixelFormat;
        display_funcs->p_wglSetPixelFormat = win32u_wglSetPixelFormat;

        /* In WineD3D we need the ability to set the pixel format more than once (e.g. after a device reset).
         * The default wglSetPixelFormat doesn't allow this, so add our own which allows it.
         */
        register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_WINE_pixel_format_passthrough" );
        display_funcs->p_wglSetPixelFormatWINE = win32u_wglSetPixelFormatWINE;
    }
}

static struct opengl_funcs *get_dc_funcs( HDC hdc, void *null_funcs )
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
const struct opengl_funcs *__wine_get_wgl_driver( HDC hdc, UINT version )
{
    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but dibdrv has %u\n",
             version, WINE_OPENGL_DRIVER_VERSION );
        return NULL;
    }

    return get_dc_funcs( hdc, (void *)-1 );
}
