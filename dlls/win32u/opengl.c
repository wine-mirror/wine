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
    void *driver_private;
    int pixel_format;

    HBITMAP memory_bitmap;
    struct wgl_pbuffer *memory_pbuffer;
    struct opengl_drawable *draw;
    struct opengl_drawable *read;
};

struct wgl_pbuffer
{
    struct opengl_drawable *drawable;

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

static const struct opengl_funcs *default_funcs; /* default GL function table from opengl32 */
static struct egl_platform display_egl;
static struct opengl_funcs display_funcs;

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

void *opengl_drawable_create( UINT size, const struct opengl_drawable_funcs *funcs, int format, HWND hwnd, HDC hdc )
{
    struct opengl_drawable *drawable;

    if (!(drawable = calloc( 1, size ))) return NULL;
    drawable->funcs = funcs;
    drawable->ref = 1;

    drawable->format = format;
    drawable->hwnd = hwnd;
    drawable->hdc = hdc;

    TRACE( "created %s\n", debugstr_opengl_drawable( drawable ) );
    return drawable;
}

void opengl_drawable_add_ref( struct opengl_drawable *drawable )
{
    ULONG ref = InterlockedIncrement( &drawable->ref );
    TRACE( "%s increasing refcount to %u\n", debugstr_opengl_drawable( drawable ), ref );
}

void opengl_drawable_release( struct opengl_drawable *drawable )
{
    ULONG ref = InterlockedDecrement( &drawable->ref );
    TRACE( "%s decreasing refcount to %u\n", debugstr_opengl_drawable( drawable ), ref );

    if (!ref)
    {
        drawable->funcs->destroy( drawable );
        free( drawable );
    }
}

#ifdef SONAME_LIBEGL

static void *egldrv_get_proc_address( const char *name )
{
    return display_funcs.p_eglGetProcAddress( name );
}

static UINT egldrv_init_pixel_formats( UINT *onscreen_count )
{
    const struct opengl_funcs *funcs = &display_funcs;
    struct egl_platform *egl = &display_egl;
    const EGLint attribs[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLConfig *configs;
    EGLint i, count;

    funcs->p_eglChooseConfig( egl->display, attribs, NULL, 0, &count );
    if (!(configs = malloc( count * sizeof(*configs) ))) return 0;
    if (!funcs->p_eglChooseConfig( egl->display, attribs, configs, count, &count ) || !count)
    {
        ERR( "Failed to get any configs from eglChooseConfig\n" );
        free( configs );
        return 0;
    }

    if (TRACE_ON(wgl)) for (i = 0; i < count; i++)
    {
        EGLint id, type, visual_id, native, render, color, r, g, b, a, d, s;
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_NATIVE_VISUAL_ID, &visual_id );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_SURFACE_TYPE, &type );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_RENDERABLE_TYPE, &render );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_CONFIG_ID, &id );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_NATIVE_RENDERABLE, &native );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_COLOR_BUFFER_TYPE, &color );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_RED_SIZE, &r );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_GREEN_SIZE, &g );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_BLUE_SIZE, &b );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_ALPHA_SIZE, &a );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_DEPTH_SIZE, &d );
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_STENCIL_SIZE, &s );
        TRACE( "%u: config %d id %d type %x visual %d native %d render %x colortype %d rgba %d,%d,%d,%d depth %u stencil %d\n",
               count, i, id, type, visual_id, native, render, color, r, g, b, a, d, s );
    }

    egl->configs = configs;
    egl->config_count = count;
    *onscreen_count = count;
    return 2 * count;
}

static BOOL describe_egl_config( EGLConfig config, struct wgl_pixel_format *fmt, BOOL onscreen )
{
    const struct opengl_funcs *funcs = &display_funcs;
    struct egl_platform *egl = &display_egl;
    EGLint value, surface_type;
    PIXELFORMATDESCRIPTOR *pfd = &fmt->pfd;

    /* If we can't get basic information, there is no point continuing */
    if (!funcs->p_eglGetConfigAttrib( egl->display, config, EGL_SURFACE_TYPE, &surface_type )) return FALSE;

    memset( fmt, 0, sizeof(*fmt) );
    pfd->nSize = sizeof(*pfd);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION;
    if (onscreen)
    {
        pfd->dwFlags |= PFD_DOUBLEBUFFER;
        if (surface_type & EGL_WINDOW_BIT) pfd->dwFlags |= PFD_DRAW_TO_WINDOW;
    }
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->iLayerType = PFD_MAIN_PLANE;

#define SET_ATTRIB( field, attrib )                                                                \
    value = 0;                                                                                     \
    funcs->p_eglGetConfigAttrib( egl->display, config, attrib, &value );                           \
    pfd->field = value;
#define SET_ATTRIB_ARB( field, attrib )                                                            \
    if (!funcs->p_eglGetConfigAttrib( egl->display, config, attrib, &value )) value = -1;          \
    fmt->field = value;

    /* Although the documentation describes cColorBits as excluding alpha, real
     * drivers tend to return the full pixel size, so do the same. */
    SET_ATTRIB( cColorBits, EGL_BUFFER_SIZE );
    SET_ATTRIB( cRedBits, EGL_RED_SIZE );
    SET_ATTRIB( cGreenBits, EGL_GREEN_SIZE );
    SET_ATTRIB( cBlueBits, EGL_BLUE_SIZE );
    SET_ATTRIB( cAlphaBits, EGL_ALPHA_SIZE );
    /* Although we don't get information from EGL about the component shifts
     * or the native format, the 0xARGB order is the most common. */
    pfd->cBlueShift = 0;
    pfd->cGreenShift = pfd->cBlueBits;
    pfd->cRedShift = pfd->cGreenBits + pfd->cBlueBits;
    if (!pfd->cAlphaBits) pfd->cAlphaShift = 0;
    else pfd->cAlphaShift = pfd->cRedBits + pfd->cGreenBits + pfd->cBlueBits;

    SET_ATTRIB( cDepthBits, EGL_DEPTH_SIZE );
    SET_ATTRIB( cStencilBits, EGL_STENCIL_SIZE );

    fmt->swap_method = WGL_SWAP_UNDEFINED_ARB;

    if (funcs->p_eglGetConfigAttrib( egl->display, config, EGL_TRANSPARENT_TYPE, &value ))
    {
        switch (value)
        {
        case EGL_TRANSPARENT_RGB:
            fmt->transparent = GL_TRUE;
            break;
        case EGL_NONE:
            fmt->transparent = GL_FALSE;
            break;
        default:
            ERR( "unexpected transparency type 0x%x\n", value );
            fmt->transparent = -1;
            break;
        }
    }
    else fmt->transparent = -1;

    if (!egl->has_EGL_EXT_pixel_format_float) fmt->pixel_type = WGL_TYPE_RGBA_ARB;
    else if (funcs->p_eglGetConfigAttrib( egl->display, config, EGL_COLOR_COMPONENT_TYPE_EXT, &value ))
    {
        switch (value)
        {
        case EGL_COLOR_COMPONENT_TYPE_FIXED_EXT:
            fmt->pixel_type = WGL_TYPE_RGBA_ARB;
            break;
        case EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT:
            fmt->pixel_type = WGL_TYPE_RGBA_FLOAT_ARB;
            break;
        default:
            ERR( "unexpected color component type 0x%x\n", value );
            fmt->pixel_type = -1;
            break;
        }
    }
    else fmt->pixel_type = -1;

    fmt->draw_to_pbuffer = TRUE;
    /* Use some arbitrary but reasonable limits (4096 is also Mesa's default) */
    fmt->max_pbuffer_width = 4096;
    fmt->max_pbuffer_height = 4096;
    fmt->max_pbuffer_pixels = fmt->max_pbuffer_width * fmt->max_pbuffer_height;

    if (funcs->p_eglGetConfigAttrib( egl->display, config, EGL_TRANSPARENT_RED_VALUE, &value ))
    {
        fmt->transparent_red_value_valid = GL_TRUE;
        fmt->transparent_red_value = value;
    }
    if (funcs->p_eglGetConfigAttrib( egl->display, config, EGL_TRANSPARENT_GREEN_VALUE, &value ))
    {
        fmt->transparent_green_value_valid = GL_TRUE;
        fmt->transparent_green_value = value;
    }
    if (funcs->p_eglGetConfigAttrib( egl->display, config, EGL_TRANSPARENT_BLUE_VALUE, &value ))
    {
        fmt->transparent_blue_value_valid = GL_TRUE;
        fmt->transparent_blue_value = value;
    }
    fmt->transparent_alpha_value_valid = GL_TRUE;
    fmt->transparent_alpha_value = 0;
    fmt->transparent_index_value_valid = GL_TRUE;
    fmt->transparent_index_value = 0;

    SET_ATTRIB_ARB( sample_buffers, EGL_SAMPLE_BUFFERS );
    SET_ATTRIB_ARB( samples, EGL_SAMPLES );

    fmt->bind_to_texture_rgb = GL_TRUE;
    fmt->bind_to_texture_rgba = GL_TRUE;
    fmt->bind_to_texture_rectangle_rgb = GL_TRUE;
    fmt->bind_to_texture_rectangle_rgba = GL_TRUE;

    /* TODO: Support SRGB surfaces and enable the attribute */
    fmt->framebuffer_srgb_capable = GL_FALSE;

    fmt->float_components = GL_FALSE;

#undef SET_ATTRIB
#undef SET_ATTRIB_ARB
    return TRUE;
}

static BOOL egldrv_describe_pixel_format( int format, struct wgl_pixel_format *desc )
{
    struct egl_platform *egl = &display_egl;
    int count = egl->config_count;
    BOOL onscreen = TRUE;

    if (--format < 0 || format > 2 * count) return FALSE;
    if (format >= count) onscreen = FALSE;
    return describe_egl_config( egl->configs[format % count], desc, onscreen );
}

static const char *egldrv_init_wgl_extensions( struct opengl_funcs *funcs )
{
    return "";
}

static BOOL egldrv_surface_create( HWND hwnd, HDC hdc, int format, struct opengl_drawable **drawable )
{
    FIXME( "stub!\n" );
    return TRUE;
}

static BOOL egldrv_swap_buffers( void *private, HWND hwnd, HDC hdc, int interval )
{
    FIXME( "stub!\n" );
    return TRUE;
}

static BOOL egldrv_pbuffer_create( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                   GLint max_level, GLsizei *width, GLsizei *height, struct opengl_drawable **drawable )
{
    FIXME( "stub!\n" );
    return FALSE;
}

static BOOL egldrv_pbuffer_updated( HDC hdc, struct opengl_drawable *drawable, GLenum cube_face, GLint mipmap_level )
{
    FIXME( "stub!\n" );
    return GL_TRUE;
}

static UINT egldrv_pbuffer_bind( HDC hdc, struct opengl_drawable *drawable, GLenum buffer )
{
    FIXME( "stub!\n" );
    return -1; /* use default implementation */
}

static BOOL egldrv_context_create( int format, void *share, const int *attribs, void **private )
{
    FIXME( "stub!\n" );
    return TRUE;
}

static BOOL egldrv_context_destroy( void *private )
{
    FIXME( "stub!\n" );
    return FALSE;
}

static BOOL egldrv_context_flush( void *private, HWND hwnd, HDC hdc, int interval, void (*flush)(void) )
{
    FIXME( "stub!\n" );
    return FALSE;
}

static BOOL egldrv_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    FIXME( "stub!\n" );
    return FALSE;
}

static const struct opengl_driver_funcs egldrv_funcs =
{
    .p_get_proc_address = egldrv_get_proc_address,
    .p_init_pixel_formats = egldrv_init_pixel_formats,
    .p_describe_pixel_format = egldrv_describe_pixel_format,
    .p_init_wgl_extensions = egldrv_init_wgl_extensions,
    .p_surface_create = egldrv_surface_create,
    .p_swap_buffers = egldrv_swap_buffers,
    .p_pbuffer_create = egldrv_pbuffer_create,
    .p_pbuffer_updated = egldrv_pbuffer_updated,
    .p_pbuffer_bind = egldrv_pbuffer_bind,
    .p_context_create = egldrv_context_create,
    .p_context_destroy = egldrv_context_destroy,
    .p_context_flush = egldrv_context_flush,
    .p_context_make_current = egldrv_context_make_current,
};

static BOOL egl_init( const struct opengl_driver_funcs **driver_funcs )
{
    struct opengl_funcs *funcs = &display_funcs;
    const char *extensions;

    if (!(funcs->egl_handle = dlopen( SONAME_LIBEGL, RTLD_NOW | RTLD_GLOBAL )))
    {
        ERR( "Failed to load %s: %s\n", SONAME_LIBEGL, dlerror() );
        return FALSE;
    }

#define LOAD_FUNCPTR( name )                                    \
    if (!(funcs->p_##name = dlsym( funcs->egl_handle, #name ))) \
    {                                                           \
        ERR( "Failed to find EGL function %s\n", #name );       \
        goto failed;                                            \
    }
    LOAD_FUNCPTR( eglGetProcAddress );
    LOAD_FUNCPTR( eglQueryString );
#undef LOAD_FUNCPTR

    if (!(extensions = funcs->p_eglQueryString( EGL_NO_DISPLAY, EGL_EXTENSIONS )))
    {
        ERR( "Failed to find client extensions\n" );
        goto failed;
    }
    TRACE( "EGL client extensions:\n" );
    dump_extensions( extensions );

#define CHECK_EXTENSION( ext )                                  \
    if (!has_extension( extensions, #ext ))                     \
    {                                                           \
        ERR( "Failed to find required extension %s\n", #ext );  \
        goto failed;                                            \
    }
    CHECK_EXTENSION( EGL_KHR_client_get_all_proc_addresses );
    CHECK_EXTENSION( EGL_EXT_platform_base );
#undef CHECK_EXTENSION

#define USE_GL_FUNC( func )                                                                     \
    if (!funcs->p_##func && !(funcs->p_##func = (void *)funcs->p_eglGetProcAddress( #func )))   \
    {                                                                                           \
        ERR( "Failed to load symbol %s\n", #func );                                             \
        goto failed;                                                                            \
    }
    ALL_EGL_FUNCS
#undef USE_GL_FUNC

    *driver_funcs = &egldrv_funcs;
    return TRUE;

failed:
    dlclose( funcs->egl_handle );
    funcs->egl_handle = NULL;
    return FALSE;
}

static void init_egl_platform( struct egl_platform *egl, struct opengl_funcs *funcs,
                               const struct opengl_driver_funcs *driver_funcs )
{
    EGLNativeDisplayType platform_display;
    const char *extensions;
    EGLint major, minor;
    EGLenum platform;

    if (!funcs->egl_handle || !driver_funcs->p_init_egl_platform) return;

    platform = driver_funcs->p_init_egl_platform( egl, &platform_display );
    if (!platform) egl->display = funcs->p_eglGetDisplay( EGL_DEFAULT_DISPLAY );
    else egl->display = funcs->p_eglGetPlatformDisplay( platform, platform_display, NULL );

    if (!egl->display)
    {
        ERR( "Failed to open EGL display\n" );
        return;
    }

    if (!funcs->p_eglInitialize( egl->display, &major, &minor )) return;
    TRACE( "Initialized EGL display %p, version %d.%d\n", egl->display, major, minor );

    if (!(extensions = funcs->p_eglQueryString( egl->display, EGL_EXTENSIONS ))) return;
    TRACE( "EGL display extensions:\n" );
    dump_extensions( extensions );

#define CHECK_EXTENSION( ext )                                                                     \
    if (!has_extension( extensions, #ext ))                                                        \
    {                                                                                              \
        ERR( "Failed to find required extension %s\n", #ext );                                     \
        return;                                                                                    \
    }
    CHECK_EXTENSION( EGL_KHR_create_context );
    CHECK_EXTENSION( EGL_KHR_create_context_no_error );
    CHECK_EXTENSION( EGL_KHR_no_config_context );
#undef CHECK_EXTENSION

    egl->has_EGL_EXT_present_opaque = has_extension( extensions, "EGL_EXT_present_opaque" );
    egl->has_EGL_EXT_pixel_format_float = has_extension( extensions, "EGL_EXT_pixel_format_float" );
}

#else /* SONAME_LIBEGL */

static BOOL egl_init( const struct opengl_driver_funcs **driver_funcs )
{
    WARN( "EGL support not compiled in!\n" );
    return FALSE;
}

static void init_egl_platform( struct egl_platform *egl, struct opengl_funcs *funcs,
                               const struct opengl_driver_funcs *driver_funcs )
{
}

#endif /* SONAME_LIBEGL */

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

static const char *nulldrv_init_wgl_extensions( struct opengl_funcs *funcs )
{
    return "";
}

static BOOL nulldrv_surface_create( HWND hwnd, HDC hdc, int format, struct opengl_drawable **drawable )
{
    return TRUE;
}

static BOOL nulldrv_swap_buffers( void *private, HWND hwnd, HDC hdc, int interval )
{
    return FALSE;
}

static BOOL nulldrv_pbuffer_create( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                    GLint max_level, GLsizei *width, GLsizei *height, struct opengl_drawable **drawable )
{
    return FALSE;
}

static BOOL nulldrv_pbuffer_updated( HDC hdc, struct opengl_drawable *drawable, GLenum cube_face, GLint mipmap_level )
{
    return GL_TRUE;
}

static UINT nulldrv_pbuffer_bind( HDC hdc, struct opengl_drawable *drawable, GLenum buffer )
{
    return -1; /* use default implementation */
}

static BOOL nulldrv_context_create( int format, void *share, const int *attribs, void **private )
{
    return FALSE;
}

static BOOL nulldrv_context_destroy( void *private )
{
    return FALSE;
}

static BOOL nulldrv_context_flush( void *private, HWND hwnd, HDC hdc, int interval, void (*flush)(void) )
{
    return FALSE;
}

static BOOL nulldrv_context_make_current( HDC draw, HDC read, void *context )
{
    return FALSE;
}

static const struct opengl_driver_funcs nulldrv_funcs =
{
    .p_get_proc_address = nulldrv_get_proc_address,
    .p_init_pixel_formats = nulldrv_init_pixel_formats,
    .p_describe_pixel_format = nulldrv_describe_pixel_format,
    .p_init_wgl_extensions = nulldrv_init_wgl_extensions,
    .p_surface_create = nulldrv_surface_create,
    .p_swap_buffers = nulldrv_swap_buffers,
    .p_pbuffer_create = nulldrv_pbuffer_create,
    .p_pbuffer_updated = nulldrv_pbuffer_updated,
    .p_pbuffer_bind = nulldrv_pbuffer_bind,
    .p_context_create = nulldrv_context_create,
    .p_context_destroy = nulldrv_context_destroy,
    .p_context_flush = nulldrv_context_flush,
    .p_context_make_current = nulldrv_context_make_current,
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
        if (is_display && ret >= 0 && ret > onscreen_count) ret = 1;
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

void set_window_opengl_drawable( HWND hwnd, struct opengl_drawable *new_drawable )
{
    void *old_drawable = NULL;
    WND *win;

    TRACE( "hwnd %p, new_drawable %s\n", hwnd, debugstr_opengl_drawable( new_drawable ) );

    if ((win = get_win_ptr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        old_drawable = win->opengl_drawable;
        if ((win->opengl_drawable = new_drawable)) opengl_drawable_add_ref( new_drawable );
        release_win_ptr( win );
    }

    if (old_drawable) opengl_drawable_release( old_drawable );
}

struct opengl_drawable *get_window_opengl_drawable( HWND hwnd )
{
    void *drawable = NULL;
    WND *win;

    if ((win = get_win_ptr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        if ((drawable = win->opengl_drawable)) opengl_drawable_add_ref( drawable );
        release_win_ptr( win );
    }

    TRACE( "hwnd %p, drawable %s\n", hwnd, debugstr_opengl_drawable( drawable ) );
    return drawable;
}

static void set_dc_opengl_drawable( HDC hdc, struct opengl_drawable *new_drawable )
{
    void *old_drawable = NULL;
    DC *dc;

    TRACE( "hdc %p, new_drawable %s\n", hdc, debugstr_opengl_drawable( new_drawable ) );

    if ((dc = get_dc_ptr( hdc )))
    {
        old_drawable = dc->opengl_drawable;
        if ((dc->opengl_drawable = new_drawable)) opengl_drawable_add_ref( new_drawable );
        release_dc_ptr( dc );
    }

    if (old_drawable) opengl_drawable_release( old_drawable );
}

static struct opengl_drawable *get_dc_opengl_drawable( HDC hdc )
{
    void *drawable = NULL;
    HWND hwnd;
    DC *dc;

    if ((hwnd = NtUserWindowFromDC( hdc ))) return get_window_opengl_drawable( hwnd );

    if ((dc = get_dc_ptr( hdc )))
    {
        if ((drawable = dc->opengl_drawable)) opengl_drawable_add_ref( drawable );
        release_dc_ptr( dc );
    }

    TRACE( "hdc %p, drawable %s\n", hdc, debugstr_opengl_drawable( drawable ) );
    return drawable;
}

static struct wgl_pbuffer *create_memory_pbuffer( HDC hdc, int format )
{
    const struct opengl_funcs *funcs = &display_funcs;
    struct wgl_pbuffer *pbuffer = NULL;
    BITMAPOBJ *bmp;
    dib_info dib;
    BOOL ret;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return NULL;
    if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC) ret = FALSE;
    else if (!(bmp = GDI_GetObjPtr( dc->hBitmap, NTGDI_OBJ_BITMAP ))) ret = FALSE;
    else
    {
        ret = init_dib_info_from_bitmapobj( &dib, bmp );
        GDI_ReleaseObj( dc->hBitmap );
    }
    release_dc_ptr( dc );

    if (ret)
    {
        int width = dib.rect.right - dib.rect.left, height = dib.rect.bottom - dib.rect.top;
        pbuffer = funcs->p_wglCreatePbufferARB( hdc, format, width, height, NULL );
        if (pbuffer) set_dc_opengl_drawable( hdc, pbuffer->drawable );
    }

    if (pbuffer) TRACE( "Created pbuffer %p for memory DC %p\n", pbuffer, hdc );
    else WARN( "Failed to create pbuffer for memory DC %p\n", hdc );
    return pbuffer;
}

static BOOL flush_memory_pbuffer( struct wgl_context *context, HDC hdc, BOOL write, void (*flush)(void) )
{
    const struct opengl_funcs *funcs = &display_funcs;
    BITMAPOBJ *bmp;
    DC *dc;

    if (flush) flush();

    if (!(dc = get_dc_ptr( hdc ))) return TRUE;
    if ((bmp = GDI_GetObjPtr( dc->hBitmap, NTGDI_OBJ_BITMAP )))
    {
        char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        struct bitblt_coords src = {0};
        struct gdi_image_bits bits;

        if (dc->hBitmap != context->memory_bitmap) write = TRUE;
        context->memory_bitmap = dc->hBitmap;

        if (!get_image_from_bitmap( bmp, info, &bits, &src ))
        {
            int width = info->bmiHeader.biWidth, height = abs( info->bmiHeader.biHeight );
            if (write) funcs->p_glDrawPixels( width, height, GL_BGRA, GL_UNSIGNED_BYTE, bits.ptr );
            else funcs->p_glReadPixels( 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, bits.ptr );
        }
        GDI_ReleaseObj( dc->hBitmap );
    }
    release_dc_ptr( dc );

    return TRUE;
}

static void destroy_memory_pbuffer( struct wgl_context *context, HDC hdc )
{
    const struct opengl_funcs *funcs = &display_funcs;
    flush_memory_pbuffer( context, hdc, FALSE, funcs->p_glFinish );
    set_dc_opengl_drawable( hdc, NULL );
    funcs->p_wglDestroyPbufferARB( context->memory_pbuffer );
    context->memory_pbuffer = NULL;
}

static BOOL set_dc_pixel_format( HDC hdc, int new_format, BOOL internal )
{
    const struct opengl_funcs *funcs = &display_funcs;
    UINT total, onscreen;
    HWND hwnd;

    funcs->p_get_pixel_formats( NULL, 0, &total, &onscreen );
    if (new_format <= 0 || new_format > total) return FALSE;

    if ((hwnd = NtUserWindowFromDC( hdc )))
    {
        struct opengl_drawable *drawable;
        int old_format;
        BOOL ret;

        if (new_format > onscreen)
        {
            WARN( "Invalid format %d for %p/%p\n", new_format, hdc, hwnd );
            return FALSE;
        }

        TRACE( "%p/%p format %d, internal %u\n", hdc, hwnd, new_format, internal );

        if ((old_format = get_window_pixel_format( hwnd, FALSE )) && !internal) return old_format == new_format;

        drawable = get_window_opengl_drawable( hwnd );
        if ((ret = driver_funcs->p_surface_create( hwnd, hdc, new_format, &drawable )))
            set_window_opengl_drawable( hwnd, drawable );
        if (drawable) opengl_drawable_release( drawable );

        if (!ret) return FALSE;
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

static PROC win32u_wglGetProcAddress( const char *name )
{
    PROC ret;
    if (!strncmp( name, "wgl", 3 )) return NULL;
    ret = driver_funcs->p_get_proc_address( name );
    TRACE( "%s -> %p\n", debugstr_a(name), ret );
    return ret;
}

static void win32u_get_pixel_formats( struct wgl_pixel_format *formats, UINT max_formats,
                                      UINT *num_formats, UINT *num_onscreen_formats )
{
    UINT i = 0;

    if (formats) while (i < max_formats && driver_funcs->p_describe_pixel_format( i + 1, &formats[i] )) i++;
    *num_formats = formats_count;
    *num_onscreen_formats = onscreen_count;
}

static struct wgl_context *context_create( HDC hdc, struct wgl_context *shared, const int *attribs )
{
    void *shared_private = shared ? shared->driver_private : NULL;
    struct wgl_context *context;
    int format;

    TRACE( "hdc %p, shared %p, attribs %p\n", hdc, shared, attribs );

    if ((format = get_dc_pixel_format( hdc, TRUE )) <= 0)
    {
        if (!format) RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return NULL;
    }

    if (!(context = calloc( 1, sizeof(*context) ))) return NULL;
    context->pixel_format = format;

    if (!driver_funcs->p_context_create( format, shared_private, attribs, &context->driver_private ))
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

static BOOL context_set_drawables( struct wgl_context *context, void *private, HDC draw_hdc, HDC read_hdc, BOOL force )
{
    struct opengl_drawable *new_draw, *new_read, *old_draw = context->draw, *old_read = context->read;
    BOOL ret = FALSE;

    new_draw = get_dc_opengl_drawable( draw_hdc );
    new_read = get_dc_opengl_drawable( read_hdc );

    TRACE( "context %p, new_draw %s, new_read %s\n", context, debugstr_opengl_drawable( new_draw ), debugstr_opengl_drawable( new_read ) );

    if (private && (!new_draw || !new_read))
        WARN( "One of the drawable has been lost, ignoring\n" );
    else if (!private && (new_draw || new_read))
        WARN( "Unexpected drawables with NULL context\n" );
    else if (!force && new_draw == context->draw && new_read == context->read)
        TRACE( "Drawables didn't change, nothing to do\n" );
    else if (driver_funcs->p_context_make_current( draw_hdc, read_hdc, private ))
    {
        if ((context->draw = new_draw)) opengl_drawable_add_ref( new_draw );
        if ((context->read = new_read)) opengl_drawable_add_ref( new_read );
        if (old_draw) opengl_drawable_release( old_draw );
        if (old_read) opengl_drawable_release( old_read );
        ret = TRUE;
    }

    if (new_draw) opengl_drawable_release( new_draw );
    if (new_read) opengl_drawable_release( new_read );
    return ret;
}

static BOOL win32u_wglDeleteContext( struct wgl_context *context )
{
    BOOL ret;

    TRACE( "context %p\n", context );

    ret = driver_funcs->p_context_destroy( context->driver_private );
    free( context );

    return ret;
}

static BOOL win32u_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, struct wgl_context *context )
{
    HDC hdc = draw_hdc, prev_draw = NtCurrentTeb()->glReserved1[0];
    struct wgl_context *prev_context = NtCurrentTeb()->glContext;
    int format;

    TRACE( "draw_hdc %p, read_hdc %p, context %p\n", draw_hdc, read_hdc, context );

    if (prev_context && prev_context->memory_pbuffer) destroy_memory_pbuffer( prev_context, prev_draw );

    if (!context)
    {
        if (!(context = prev_context)) return TRUE;
        if (!context_set_drawables( context, NULL, NULL, NULL, TRUE )) return FALSE;
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

    if ((context->memory_pbuffer = create_memory_pbuffer( draw_hdc, context->pixel_format )))
    {
        if (read_hdc != draw_hdc) ERR( "read != draw not supported\n" );
        draw_hdc = read_hdc = context->memory_pbuffer->hdc;
    }

    if (!context_set_drawables( context, context->driver_private, draw_hdc, read_hdc, TRUE )) return FALSE;
    NtCurrentTeb()->glContext = context;
    if (context->memory_pbuffer) flush_memory_pbuffer( context, hdc, TRUE, NULL );
    return TRUE;
}

static BOOL win32u_wglMakeCurrent( HDC hdc, struct wgl_context *context )
{
    return win32u_wglMakeContextCurrentARB( hdc, hdc, context );
}

static struct wgl_pbuffer *win32u_wglCreatePbufferARB( HDC hdc, int format, int width, int height,
                                                       const int *attribs )
{
    const struct opengl_funcs *funcs = &display_funcs;
    UINT total, onscreen, size, max_level = 0;
    struct wgl_pbuffer *pbuffer;
    BOOL largest = FALSE;

    TRACE( "(%p, %d, %d, %d, %p)\n", hdc, format, width, height, attribs );

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

    if (driver_funcs->p_pbuffer_create( pbuffer->hdc, format, largest, pbuffer->texture_format,
                                        pbuffer->texture_target, max_level, &pbuffer->width,
                                        &pbuffer->height, &pbuffer->drawable ))
    {
        set_dc_opengl_drawable( pbuffer->hdc, pbuffer->drawable );
        return pbuffer;
    }

failed:
    RtlSetLastWin32Error( ERROR_INVALID_DATA );
    NtGdiDeleteObjectApp( pbuffer->hdc );
    free( pbuffer );
    return NULL;
}

static BOOL win32u_wglDestroyPbufferARB( struct wgl_pbuffer *pbuffer )
{
    const struct opengl_funcs *funcs = &display_funcs;

    TRACE( "pbuffer %p\n", pbuffer );

    opengl_drawable_release( pbuffer->drawable );
    if (pbuffer->tmp_context) funcs->p_wglDeleteContext( pbuffer->tmp_context );
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
    const struct opengl_funcs *funcs = &display_funcs;
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

    if (!driver_funcs->p_describe_pixel_format( format, &desc ))
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

    if ((ret = driver_funcs->p_pbuffer_bind( pbuffer->hdc, pbuffer->drawable, source )) != -1)
        return ret;

    if (!pbuffer->tmp_context || pbuffer->prev_context != prev_context)
    {
        if (pbuffer->tmp_context) funcs->p_wglDeleteContext( pbuffer->tmp_context );
        pbuffer->tmp_context = funcs->p_wglCreateContextAttribsARB( pbuffer->hdc, prev_context, NULL );
        pbuffer->prev_context = prev_context;
    }

    funcs->p_glGetIntegerv( binding_from_target( pbuffer->texture_target ), &prev_texture );

    /* Switch to our pbuffer */
    funcs->p_wglMakeCurrent( pbuffer->hdc, pbuffer->tmp_context );

    /* Make sure that the prev_texture is set as the current texture state isn't shared
     * between contexts. After that copy the pbuffer texture data. */
    funcs->p_glBindTexture( pbuffer->texture_target, prev_texture );
    funcs->p_glCopyTexImage2D( pbuffer->texture_target, 0, pbuffer->texture_format, 0, 0,
                                        pbuffer->width, pbuffer->height, 0 );

    /* Switch back to the original drawable and context */
    funcs->p_wglMakeContextCurrentARB( prev_draw, prev_read, prev_context );
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

    return !!driver_funcs->p_pbuffer_bind( pbuffer->hdc, pbuffer->drawable, GL_NONE );
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

    return driver_funcs->p_pbuffer_updated( pbuffer->hdc, pbuffer->drawable, pbuffer->cube_face,
                                            max( pbuffer->mipmap_level, 0 ) );
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

static BOOL win32u_wgl_context_flush( struct wgl_context *context, void (*flush)(void) )
{
    HDC draw_hdc = NtCurrentTeb()->glReserved1[0], read_hdc = NtCurrentTeb()->glReserved1[1];
    int interval;
    HWND hwnd;

    if (!(hwnd = NtUserWindowFromDC( draw_hdc ))) interval = 0;
    else interval = get_window_swap_interval( hwnd );

    TRACE( "context %p, hwnd %p, draw_hdc %p, interval %d, flush %p\n", context, hwnd, draw_hdc, interval, flush );

    context_set_drawables( context, context->driver_private, draw_hdc, read_hdc, FALSE );
    if (context->memory_pbuffer) return flush_memory_pbuffer( context, draw_hdc, FALSE, flush );
    return driver_funcs->p_context_flush( context->driver_private, hwnd, draw_hdc, interval, flush );
}

static BOOL win32u_wglSwapBuffers( HDC hdc )
{
    HDC draw_hdc = NtCurrentTeb()->glReserved1[0], read_hdc = NtCurrentTeb()->glReserved1[1];
    struct wgl_context *context = NtCurrentTeb()->glContext;
    const struct opengl_funcs *funcs = &display_funcs;
    int interval;
    HWND hwnd;

    if (!(hwnd = NtUserWindowFromDC( hdc ))) interval = 0;
    else interval = get_window_swap_interval( hwnd );

    context_set_drawables( context, context->driver_private, draw_hdc, read_hdc, FALSE );
    if (context->memory_pbuffer) return flush_memory_pbuffer( context, hdc, FALSE, funcs->p_glFlush );
    return driver_funcs->p_swap_buffers( context ? context->driver_private : NULL, hwnd, hdc, interval );
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

static void display_funcs_init(void)
{
    UINT status;

    if (egl_init( &driver_funcs )) TRACE( "Initialized EGL library\n" );

    if ((status = user_driver->pOpenGLInit( WINE_OPENGL_DRIVER_VERSION, &display_funcs, &driver_funcs )))
        WARN( "Failed to initialize the driver OpenGL functions, status %#x\n", status );
    init_egl_platform( &display_egl, &display_funcs, driver_funcs );

    formats_count = driver_funcs->p_init_pixel_formats( &onscreen_count );

#define USE_GL_FUNC(func) \
    if (!display_funcs.p_##func && !(display_funcs.p_##func = driver_funcs->p_get_proc_address( #func ))) \
    { \
        WARN( "%s not found for memory DCs.\n", #func ); \
        display_funcs.p_##func = default_funcs->p_##func; \
    }
    ALL_GL_FUNCS
#undef USE_GL_FUNC

    display_funcs.p_wglGetProcAddress = win32u_wglGetProcAddress;
    display_funcs.p_get_pixel_formats = win32u_get_pixel_formats;

    strcpy( wgl_extensions, driver_funcs->p_init_wgl_extensions( &display_funcs ) );
    display_funcs.p_wglGetPixelFormat = win32u_wglGetPixelFormat;
    display_funcs.p_wglSetPixelFormat = win32u_wglSetPixelFormat;

    display_funcs.p_wglCreateContext = win32u_wglCreateContext;
    display_funcs.p_wglDeleteContext = win32u_wglDeleteContext;
    display_funcs.p_wglCopyContext = (void *)1; /* never called */
    display_funcs.p_wglShareLists = (void *)1; /* never called */
    display_funcs.p_wglMakeCurrent = win32u_wglMakeCurrent;

    display_funcs.p_wglSwapBuffers = win32u_wglSwapBuffers;
    display_funcs.p_wgl_context_flush = win32u_wgl_context_flush;

    if (display_egl.has_EGL_EXT_pixel_format_float)
    {
        register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_pixel_format_float" );
        register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ATI_pixel_format_float" );
    }

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_extensions_string" );
    display_funcs.p_wglGetExtensionsStringARB = win32u_wglGetExtensionsStringARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_extensions_string" );
    display_funcs.p_wglGetExtensionsStringEXT = win32u_wglGetExtensionsStringEXT;

    /* In WineD3D we need the ability to set the pixel format more than once (e.g. after a device reset).
     * The default wglSetPixelFormat doesn't allow this, so add our own which allows it.
     */
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_WINE_pixel_format_passthrough" );
    display_funcs.p_wglSetPixelFormatWINE = win32u_wglSetPixelFormatWINE;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_pixel_format" );
    display_funcs.p_wglChoosePixelFormatARB      = (void *)1; /* never called */
    display_funcs.p_wglGetPixelFormatAttribfvARB = (void *)1; /* never called */
    display_funcs.p_wglGetPixelFormatAttribivARB = (void *)1; /* never called */

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_create_context" );
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_create_context_no_error" );
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_create_context_profile" );
    display_funcs.p_wglCreateContextAttribsARB = win32u_wglCreateContextAttribsARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_make_current_read" );
    display_funcs.p_wglGetCurrentReadDCARB   = (void *)1;  /* never called */
    display_funcs.p_wglMakeContextCurrentARB = win32u_wglMakeContextCurrentARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_pbuffer" );
    display_funcs.p_wglCreatePbufferARB    = win32u_wglCreatePbufferARB;
    display_funcs.p_wglDestroyPbufferARB   = win32u_wglDestroyPbufferARB;
    display_funcs.p_wglGetPbufferDCARB     = win32u_wglGetPbufferDCARB;
    display_funcs.p_wglReleasePbufferDCARB = win32u_wglReleasePbufferDCARB;
    display_funcs.p_wglQueryPbufferARB     = win32u_wglQueryPbufferARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_ARB_render_texture" );
    display_funcs.p_wglBindTexImageARB     = win32u_wglBindTexImageARB;
    display_funcs.p_wglReleaseTexImageARB  = win32u_wglReleaseTexImageARB;
    display_funcs.p_wglSetPbufferAttribARB = win32u_wglSetPbufferAttribARB;

    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_swap_control" );
    register_extension( wgl_extensions, ARRAY_SIZE(wgl_extensions), "WGL_EXT_swap_control_tear" );
    display_funcs.p_wglSwapIntervalEXT = win32u_wglSwapIntervalEXT;
    display_funcs.p_wglGetSwapIntervalEXT = win32u_wglGetSwapIntervalEXT;
}

/***********************************************************************
 *      __wine_get_wgl_driver  (win32u.@)
 */
const struct opengl_funcs *__wine_get_wgl_driver( HDC hdc, UINT version, const struct opengl_funcs *null_funcs )
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;
    DWORD is_disabled, is_display, is_memdc;
    DC *dc;

    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but dibdrv has %u\n",
             version, WINE_OPENGL_DRIVER_VERSION );
        return NULL;
    }

    InterlockedExchangePointer( (void *)&default_funcs, (void *)null_funcs );

    if (!(dc = get_dc_ptr( hdc ))) return NULL;
    is_memdc = get_gdi_object_type( hdc ) == NTGDI_OBJ_MEMDC;
    is_display = dc->is_display;
    is_disabled = dc->attr->disabled;
    release_dc_ptr( dc );

    if (is_disabled) return NULL;
    if (!is_display && !is_memdc) return NULL;
    pthread_once( &init_once, display_funcs_init );
    return &display_funcs;
}
