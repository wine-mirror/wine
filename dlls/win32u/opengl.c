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

struct wgl_pbuffer
{
    const struct opengl_driver_funcs *driver_funcs;
    struct opengl_funcs *funcs;
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

static struct opengl_funcs *get_dc_funcs( HDC hdc, void *null_funcs );

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

static const struct opengl_driver_funcs nulldrv_funcs =
{
    .p_init_pixel_formats = nulldrv_init_pixel_formats,
    .p_describe_pixel_format = nulldrv_describe_pixel_format,
    .p_init_wgl_extensions = nulldrv_init_wgl_extensions,
    .p_set_pixel_format = nulldrv_set_pixel_format,
    .p_pbuffer_create = nulldrv_pbuffer_create,
    .p_pbuffer_destroy = nulldrv_pbuffer_destroy,
    .p_pbuffer_updated = nulldrv_pbuffer_updated,
    .p_pbuffer_bind = nulldrv_pbuffer_bind,
};
static const struct opengl_driver_funcs *driver_funcs = &nulldrv_funcs;
static UINT formats_count, onscreen_count;

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
        ret = get_window_pixel_format( hwnd );
    else if ((dc = get_dc_ptr( hdc )))
    {
        BOOL is_display = dc->is_display;
        ret = dc->pixel_format;
        release_dc_ptr( dc );

        /* Offscreen formats can't be used with traditional WGL calls. As has been
         * verified on Windows GetPixelFormat doesn't fail but returns 1.
         */
        if (is_display && ret >= 0 && ret > onscreen_count) ret = 1;
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

        if ((old_format = get_window_pixel_format( hwnd )) && !internal) return old_format == new_format;
        if (!driver_funcs->p_set_pixel_format( hwnd, old_format, new_format, internal )) return FALSE;
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

static void win32u_get_pixel_formats( struct wgl_pixel_format *formats, UINT max_formats,
                                      UINT *num_formats, UINT *num_onscreen_formats )
{
    UINT i = 0;

    if (formats) while (i < max_formats && driver_funcs->p_describe_pixel_format( i + 1, &formats[i] )) i++;
    *num_formats = formats_count;
    *num_onscreen_formats = onscreen_count;
}

static struct wgl_pbuffer *win32u_wglCreatePbufferARB( HDC hdc, int format, int width, int height,
                                                       const int *attribs )
{
    UINT total, onscreen, size, max_level = 0;
    struct wgl_pbuffer *pbuffer;
    struct opengl_funcs *funcs;
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
    pbuffer->driver_funcs = driver_funcs;
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
    if (display_funcs && !(formats_count = driver_funcs->p_init_pixel_formats( &onscreen_count ))) display_funcs = NULL;
    if (!display_funcs) return;

    display_funcs->p_get_pixel_formats = win32u_get_pixel_formats;

    strcpy( wgl_extensions, driver_funcs->p_init_wgl_extensions() );
    display_funcs->p_wglGetPixelFormat = win32u_wglGetPixelFormat;
    display_funcs->p_wglSetPixelFormat = win32u_wglSetPixelFormat;

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
