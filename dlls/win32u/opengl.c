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
    struct opengl_drawable *drawable;

    HDC hdc;
    GLsizei width;
    GLsizei height;
    GLenum texture_format;
    GLenum texture_target;
    GLint mipmap_level;
    GLenum cube_face;
};

static const struct opengl_driver_funcs nulldrv_funcs, *driver_funcs = &nulldrv_funcs;
static const struct opengl_funcs *default_funcs; /* default GL function table from opengl32 */
static struct egl_platform display_egl;
static struct opengl_funcs display_funcs;

static struct wgl_pixel_format *pixel_formats;
static UINT formats_count, onscreen_count;
static char wgl_extensions[4096];

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

void *opengl_drawable_create( UINT size, const struct opengl_drawable_funcs *funcs, int format, struct client_surface *client )
{
    struct opengl_drawable *drawable;

    if (!(drawable = calloc( 1, size ))) return NULL;
    drawable->funcs = funcs;
    drawable->ref = 1;

    drawable->format = format;
    drawable->interval = INT_MIN;
    drawable->doublebuffer = !!(pixel_formats[format - 1].pfd.dwFlags & PFD_DOUBLEBUFFER);
    drawable->stereo = !!(pixel_formats[format - 1].pfd.dwFlags & PFD_STEREO);
    if ((drawable->client = client)) client_surface_add_ref( client );

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
        const struct opengl_funcs *funcs = &display_funcs;
        const struct egl_platform *egl = &display_egl;

        drawable->funcs->destroy( drawable );
        if (drawable->surface) funcs->p_eglDestroySurface( egl->display, drawable->surface );
        if (drawable->client) client_surface_release( drawable->client );
        free( drawable );
    }
}

static void opengl_drawable_flush( struct opengl_drawable *drawable, int interval, UINT flags )
{
    if (!drawable->client) return;

    if (InterlockedCompareExchange( &drawable->client->updated, 0, 1 )) flags |= GL_FLUSH_UPDATED;
    if (interval != drawable->interval)
    {
        drawable->interval = interval;
        flags = GL_FLUSH_INTERVAL;
    }

    if (flags || InterlockedCompareExchange( &drawable->client->offscreen, 0, 0 ))
        drawable->funcs->flush( drawable, flags );
}

#ifdef SONAME_LIBEGL

struct framebuffer_surface
{
    struct opengl_drawable base;
};

static GLenum color_format_from_pfd( const struct wgl_pixel_format *desc )
{
    TRACE( "format type %u bits %u/%u/%u/%u\n", desc->pixel_type, desc->pfd.cRedBits,
           desc->pfd.cGreenBits, desc->pfd.cBlueBits, desc->pfd.cAlphaBits );

    if (desc->pixel_type == WGL_TYPE_RGBA_FLOAT_ARB)
    {
        if (desc->pfd.cAlphaBits == 32) return GL_RGBA32F;
        if (desc->pfd.cAlphaBits == 16) return GL_RGBA16F;
        if (desc->pfd.cBlueBits == 32) return GL_RGB32F;
        if (desc->pfd.cBlueBits == 16) return GL_RGB16F;
        if (desc->pfd.cGreenBits == 32) return GL_RG32F;
        if (desc->pfd.cGreenBits == 16) return GL_RG16F;
        if (desc->pfd.cRedBits == 32) return GL_R32F;
        if (desc->pfd.cRedBits == 16) return GL_R16F;
    }
    else
    {
        if (desc->pfd.cBlueBits == 10 && desc->pfd.cGreenBits == 10 &&
            desc->pfd.cRedBits == 10 && desc->pfd.cAlphaBits == 2)
            return GL_RGB10_A2;
        if (desc->pfd.cAlphaBits == 32) return GL_RGBA32UI;
        if (desc->pfd.cAlphaBits == 16) return GL_RGBA16;
        if (desc->pfd.cAlphaBits == 8) return GL_RGBA8;
        if (desc->pfd.cAlphaBits == 4) return GL_RGBA4;
        if (desc->pfd.cBlueBits == 32) return GL_RGB32UI;
        if (desc->pfd.cBlueBits == 16) return GL_RGB16;
        if (desc->pfd.cBlueBits == 8) return GL_RGB8;
        if (desc->pfd.cBlueBits == 4) return GL_RGB4;
        if (desc->pfd.cGreenBits == 32) return GL_RG32UI;
        if (desc->pfd.cGreenBits == 16) return GL_RG16;
        if (desc->pfd.cGreenBits == 8) return GL_RG8;
        if (desc->pfd.cRedBits == 32) return GL_R32UI;
        if (desc->pfd.cRedBits == 16) return GL_R16;
        if (desc->pfd.cRedBits == 8) return GL_R8;
    }

    FIXME( "Unsupported format type %u bits %u/%u/%u/%u\n", desc->pixel_type, desc->pfd.cRedBits,
           desc->pfd.cGreenBits, desc->pfd.cBlueBits, desc->pfd.cAlphaBits );
    return 0;
}

static GLenum depth_format_from_pfd( const struct wgl_pixel_format *desc )
{
    TRACE( "format bits %u/%u\n", desc->pfd.cStencilBits, desc->pfd.cDepthBits );

    if (desc->pfd.cStencilBits)
    {
        if (desc->pfd.cDepthBits == 32) return GL_DEPTH32F_STENCIL8;
        if (desc->pfd.cDepthBits == 24) return GL_DEPTH24_STENCIL8;
    }
    else
    {
        if (desc->pfd.cDepthBits == 32) return GL_DEPTH_COMPONENT32;
        if (desc->pfd.cDepthBits == 24) return GL_DEPTH_COMPONENT24;
        if (desc->pfd.cDepthBits == 16) return GL_DEPTH_COMPONENT16;
    }

    FIXME( "Unsupported format bits %u/%u\n", desc->pfd.cStencilBits, desc->pfd.cDepthBits );
    return 0;
}

static GLuint create_framebuffer( struct opengl_drawable *drawable, const struct wgl_pixel_format *desc )
{
    const struct opengl_funcs *funcs = &display_funcs;
    GLuint count = 1, fbo, name;

    if (drawable->doublebuffer) count *= 2;
    if (drawable->stereo) count *= 2;

    funcs->p_glCreateFramebuffers( 1, &fbo );

    for (GLuint i = 0; i < count; i++)
    {
        funcs->p_glCreateRenderbuffers( 1, &name );
        funcs->p_glNamedFramebufferRenderbuffer( fbo, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, name );
        TRACE( "drawable %p/%u created color buffer %#x/%u\n", drawable, fbo, GL_COLOR_ATTACHMENT0 + i, name );
    }

    if (desc->pfd.cDepthBits)
    {
        funcs->p_glCreateRenderbuffers( 1, &name );
        funcs->p_glNamedFramebufferRenderbuffer( fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, name );
        if (desc->pfd.cStencilBits) funcs->p_glNamedFramebufferRenderbuffer( fbo, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, name );
        TRACE( "drawable %p/%u created depth buffer %u\n", drawable, fbo, name );
    }

    funcs->p_glNamedFramebufferDrawBuffer( fbo, GL_COLOR_ATTACHMENT0 );
    funcs->p_glNamedFramebufferReadBuffer( fbo, drawable->doublebuffer ? GL_COLOR_ATTACHMENT1 : GL_COLOR_ATTACHMENT0 );
    TRACE( "drawable %p created framebuffer %u\n", drawable, fbo );

    return fbo;
}

static void resize_framebuffer( struct opengl_drawable *drawable, const struct wgl_pixel_format *desc, GLuint fbo,
                                int width, int height )
{
    const struct opengl_funcs *funcs = &display_funcs;
    GLuint count = 1, name;
    GLenum ret;

    if (drawable->doublebuffer) count *= 2;
    if (drawable->stereo) count *= 2;

    for (GLuint i = 0; i < count; i++)
    {
        funcs->p_glGetNamedFramebufferAttachmentParameteriv( fbo, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&name );
        funcs->p_glNamedRenderbufferStorageMultisample( name, desc->samples, color_format_from_pfd( desc ), width, height );
        TRACE( "drawable %p/%u resized color buffer %#x/%u to %d,%d\n", drawable, fbo, GL_COLOR_ATTACHMENT0 + i, name, width, height );
    }

    if (desc->pfd.cDepthBits)
    {
        funcs->p_glGetNamedFramebufferAttachmentParameteriv( fbo, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&name );
        funcs->p_glNamedRenderbufferStorageMultisample( name, desc->samples, depth_format_from_pfd( desc ), width, height );
        TRACE( "drawable %p/%u resized depth buffer %u to %d,%d\n", drawable, fbo, name, width, height );
    }

    ret = funcs->p_glCheckNamedFramebufferStatus( fbo, GL_FRAMEBUFFER );
    if (ret != GL_FRAMEBUFFER_COMPLETE) WARN( "glCheckNamedFramebufferStatus returned %#x\n", ret );
    TRACE( "drawable %p/%u resized buffers to %d,%d\n", drawable, fbo, width, height );
}

static void destroy_framebuffer( struct opengl_drawable *drawable, const struct wgl_pixel_format *desc, GLuint fbo )
{
    const struct opengl_funcs *funcs = &display_funcs;
    GLuint count = 1, name;

    if (drawable->doublebuffer) count *= 2;
    if (drawable->stereo) count *= 2;

    for (GLuint i = 0; i < count; i++)
    {
        funcs->p_glGetNamedFramebufferAttachmentParameteriv( fbo, GL_COLOR_ATTACHMENT0 + i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&name );
        funcs->p_glDeleteRenderbuffers( 1, &name );
        TRACE( "drawable %p/%u destroyed color buffer %#x/%u\n", drawable, fbo, GL_COLOR_ATTACHMENT0 + i, name );
    }

    if (desc->pfd.cDepthBits)
    {
        funcs->p_glGetNamedFramebufferAttachmentParameteriv( fbo, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&name );
        funcs->p_glDeleteRenderbuffers( 1, &name );
        TRACE( "drawable %p/%u destroyed depth buffer %u\n", drawable, fbo, name );
    }

    funcs->p_glDeleteFramebuffers( 1, &fbo );
    TRACE( "drawable %p destroyed framebuffer %u\n", drawable, fbo );
}

static void framebuffer_surface_destroy( struct opengl_drawable *drawable )
{
    TRACE( "%s\n", debugstr_opengl_drawable( drawable ) );
}

static void framebuffer_surface_flush( struct opengl_drawable *drawable, UINT flags )
{
    struct wgl_pixel_format draw_desc = pixel_formats[drawable->format - 1], read_desc = draw_desc;
    RECT rect;

    TRACE( "%s, flags %#x\n", debugstr_opengl_drawable( drawable ), flags );

    NtUserGetClientRect( drawable->client->hwnd, &rect, NtUserGetDpiForWindow( drawable->client->hwnd ) );
    if (!rect.right) rect.right = 1;
    if (!rect.bottom) rect.bottom = 1;

    read_desc.samples = read_desc.sample_buffers = 0;

    if (flags & GL_FLUSH_WAS_CURRENT)
    {
        if (drawable->draw_fbo != drawable->read_fbo)
        {
            destroy_framebuffer( drawable, &draw_desc, drawable->draw_fbo );
            drawable->draw_fbo = 0;
        }
        destroy_framebuffer( drawable, &read_desc, drawable->read_fbo );
        drawable->read_fbo = 0;
    }

    if (flags & GL_FLUSH_SET_CURRENT)
    {
        drawable->read_fbo = create_framebuffer( drawable, &read_desc );
        if (!drawable->read_fbo) ERR( "Failed to create read framebuffer object\n" );

        if (!draw_desc.sample_buffers) drawable->draw_fbo = drawable->read_fbo;
        else drawable->draw_fbo = create_framebuffer( drawable, &draw_desc );
        if (!drawable->draw_fbo) ERR( "Failed to create draw framebuffer object\n" );
    }

    if ((flags & (GL_FLUSH_UPDATED | GL_FLUSH_SET_CURRENT)) && drawable->read_fbo)
    {
        TRACE( "Resizing drawable %p/%u to %ux%u\n", drawable, drawable->read_fbo, rect.right, rect.bottom );
        resize_framebuffer( drawable, &read_desc, drawable->read_fbo, rect.right, rect.bottom );

        if (drawable->draw_fbo != drawable->read_fbo)
        {
            TRACE( "Resizing drawable %p/%u to %ux%u\n", drawable, drawable->draw_fbo, rect.right, rect.bottom );
            resize_framebuffer( drawable, &draw_desc, drawable->draw_fbo, rect.right, rect.bottom );
        }
    }
}

static BOOL framebuffer_surface_swap( struct opengl_drawable *drawable )
{
    TRACE( "%s\n", debugstr_opengl_drawable( drawable ) );
    return TRUE;
}

static const struct opengl_drawable_funcs framebuffer_surface_funcs =
{
    .destroy = framebuffer_surface_destroy,
    .flush = framebuffer_surface_flush,
    .swap = framebuffer_surface_swap,
};

static struct opengl_drawable *framebuffer_surface_create( int format, struct client_surface *client )
{
    struct framebuffer_surface *surface;
    if (!(surface = opengl_drawable_create( sizeof(*surface), &framebuffer_surface_funcs, format, client ))) return NULL;
    return &surface->base;
}

static const struct opengl_drawable_funcs egldrv_pbuffer_funcs;

static inline EGLConfig egl_config_for_format( const struct egl_platform *egl, int format )
{
    assert(format > 0 && format <= 2 * egl->config_count);
    if (format <= egl->config_count) return egl->configs[format - 1];
    return egl->configs[format - egl->config_count - 1];
}

static void egldrv_init_egl_platform( struct egl_platform *platform )
{
    platform->type = EGL_PLATFORM_SURFACELESS_MESA;
    platform->native_display = 0;
}

static void *egldrv_get_proc_address( const char *name )
{
    return display_funcs.p_eglGetProcAddress( name );
}

static UINT egldrv_init_pixel_formats( UINT *onscreen_count )
{
    const struct opengl_funcs *funcs = &display_funcs;
    struct egl_platform *egl = &display_egl;
    EGLConfig *configs;
    EGLint i, j, render, count;

    funcs->p_eglGetConfigs( egl->display, NULL, 0, &count );
    if (!(configs = malloc( count * sizeof(*configs) ))) return 0;
    if (!funcs->p_eglGetConfigs( egl->display, configs, count, &count ) || !count)
    {
        ERR( "Failed to get any configs from eglChooseConfig\n" );
        free( configs );
        return 0;
    }

    for (i = 0, j = 0; i < count; i++)
    {
        funcs->p_eglGetConfigAttrib( egl->display, configs[i], EGL_RENDERABLE_TYPE, &render );
        if (render & EGL_OPENGL_BIT) configs[j++] = configs[i];
    }
    count = j;

    if (TRACE_ON(wgl)) for (i = 0; i < count; i++)
    {
        EGLint id, type, visual_id, native, color, r, g, b, a, d, s;
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
    if (egl->type == EGL_PLATFORM_SURFACELESS_MESA) surface_type |= EGL_WINDOW_BIT | EGL_PBUFFER_BIT;

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

    if (egl->force_pbuffer_formats) fmt->draw_to_pbuffer = TRUE;
    else if (surface_type & EGL_PBUFFER_BIT) fmt->draw_to_pbuffer = TRUE;
    if (fmt->draw_to_pbuffer) pfd->dwFlags |= PFD_DRAW_TO_BITMAP;

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

static BOOL egldrv_surface_create( HWND hwnd, int format, struct opengl_drawable **drawable )
{
    struct client_surface *client;

    if (!(client = nulldrv_client_surface_create( hwnd ))) return FALSE;
    *drawable = framebuffer_surface_create( format, client );
    client_surface_release( client );

    return !!*drawable;
}

static BOOL egldrv_pbuffer_create( HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                   GLint max_level, GLsizei *width, GLsizei *height, struct opengl_drawable **drawable )
{
    const struct opengl_funcs *funcs = &display_funcs;
    const struct egl_platform *egl = &display_egl;
    EGLint attribs[13], *attrib = attribs;
    struct opengl_drawable *gl;

    TRACE( "hdc %p, format %d, largest %u, texture_format %#x, texture_target %#x, max_level %#x, width %d, height %d, drawable %p\n",
           hdc, format, largest, texture_format, texture_target, max_level, *width, *height, drawable );

    *attrib++ = EGL_WIDTH;
    *attrib++ = *width;
    *attrib++ = EGL_HEIGHT;
    *attrib++ = *height;
    if (largest)
    {
        *attrib++ = EGL_LARGEST_PBUFFER;
        *attrib++ = 1;
    }
    switch (texture_format)
    {
    case 0: break;
    case GL_RGB:
        *attrib++ = EGL_TEXTURE_FORMAT;
        *attrib++ = EGL_TEXTURE_RGB;
        break;
    case GL_RGBA:
        *attrib++ = EGL_TEXTURE_FORMAT;
        *attrib++ = EGL_TEXTURE_RGBA;
        break;
    default:
        FIXME( "Unsupported format %#x\n", texture_format );
        *attrib++ = EGL_TEXTURE_FORMAT;
        *attrib++ = EGL_TEXTURE_RGBA;
        break;
    }
    switch (texture_target)
    {
    case 0: break;
    case GL_TEXTURE_2D:
        *attrib++ = EGL_TEXTURE_TARGET;
        *attrib++ = EGL_TEXTURE_2D;
        break;
    default:
        FIXME( "Unsupported target %#x\n", texture_target );
        *attrib++ = EGL_TEXTURE_TARGET;
        *attrib++ = EGL_TEXTURE_2D;
        break;
    }
    if (max_level)
    {
        *attrib++ = EGL_MIPMAP_TEXTURE;
        *attrib++ = GL_TRUE;
    }
    *attrib++ = EGL_NONE;

    if (!(gl = opengl_drawable_create( sizeof(*gl), &egldrv_pbuffer_funcs, format, NULL ))) return FALSE;
    if (!(gl->surface = funcs->p_eglCreatePbufferSurface( egl->display, egl_config_for_format( egl, gl->format ), attribs )))
    {
        opengl_drawable_release( gl );
        return FALSE;
    }

    funcs->p_eglQuerySurface( egl->display, gl->surface, EGL_WIDTH, width );
    funcs->p_eglQuerySurface( egl->display, gl->surface, EGL_HEIGHT, height );

    *drawable = gl;
    return TRUE;
}

static BOOL egldrv_pbuffer_updated( HDC hdc, struct opengl_drawable *drawable, GLenum cube_face, GLint mipmap_level )
{
    return GL_TRUE;
}

static UINT egldrv_pbuffer_bind( HDC hdc, struct opengl_drawable *drawable, GLenum buffer )
{
    return -1; /* use default implementation */
}

static BOOL egldrv_context_create( int format, void *share, const int *attribs, void **context )
{
    const struct opengl_funcs *funcs = &display_funcs;
    const struct egl_platform *egl = &display_egl;
    EGLint egl_attribs[16], *attribs_end = egl_attribs;

    TRACE( "format %d, share %p, attribs %p\n", format, share, attribs );

    for (; attribs && attribs[0] != 0; attribs += 2)
    {
        EGLint name;

        TRACE( "%#x %#x\n", attribs[0], attribs[1] );

        /* Find the EGL attribute names corresponding to the WGL names.
         * For all of the attributes below, the values match between the two
         * systems, so we can use them directly. */
        switch (attribs[0])
        {
        case WGL_CONTEXT_MAJOR_VERSION_ARB:
            name = EGL_CONTEXT_MAJOR_VERSION_KHR;
            break;
        case WGL_CONTEXT_MINOR_VERSION_ARB:
            name = EGL_CONTEXT_MINOR_VERSION_KHR;
            break;
        case WGL_CONTEXT_FLAGS_ARB:
            name = EGL_CONTEXT_FLAGS_KHR;
            break;
        case WGL_CONTEXT_OPENGL_NO_ERROR_ARB:
            name = EGL_CONTEXT_OPENGL_NO_ERROR_KHR;
            break;
        case WGL_CONTEXT_PROFILE_MASK_ARB:
            if (attribs[1] & WGL_CONTEXT_ES2_PROFILE_BIT_EXT)
            {
                ERR( "OpenGL ES contexts are not supported\n" );
                return FALSE;
            }
            name = EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR;
            break;
        default:
            name = EGL_NONE;
            FIXME( "Unhandled attributes: %#x %#x\n", attribs[0], attribs[1] );
        }

        if (name != EGL_NONE)
        {
            EGLint *dst = egl_attribs;
            /* Check if we have already set the same attribute and replace it. */
            for (; dst != attribs_end && *dst != name; dst += 2) continue;
            /* Our context attribute array should have enough space for all the
             * attributes we support (we merge repetitions), plus EGL_NONE. */
            assert( dst - egl_attribs <= ARRAY_SIZE(egl_attribs) - 3 );
            dst[0] = name;
            dst[1] = attribs[1];
            if (dst == attribs_end) attribs_end += 2;
        }
    }
    *attribs_end = EGL_NONE;

    /* For now only OpenGL is supported. It's enough to set the API only for
     * context creation, since:
     * 1. the default API is EGL_OPENGL_ES_API
     * 2. the EGL specification says in section 3.7:
     *    > EGL_OPENGL_API and EGL_OPENGL_ES_API are interchangeable for all
     *    > purposes except eglCreateContext.
     */
    funcs->p_eglBindAPI( EGL_OPENGL_API );
    *context = funcs->p_eglCreateContext( egl->display, EGL_NO_CONFIG_KHR, share, attribs ? egl_attribs : NULL );
    TRACE( "Created context %p\n", *context );
    return TRUE;
}

static BOOL egldrv_context_destroy( void *context )
{
    const struct opengl_funcs *funcs = &display_funcs;
    const struct egl_platform *egl = &display_egl;

    funcs->p_eglDestroyContext( egl->display, context );
    return TRUE;
}

static BOOL egldrv_make_current( struct opengl_drawable *draw, struct opengl_drawable *read, void *context )
{
    const struct opengl_funcs *funcs = &display_funcs;
    const struct egl_platform *egl = &display_egl;

    TRACE( "draw %s, read %s, context %p\n", debugstr_opengl_drawable( draw ), debugstr_opengl_drawable( read ), context );

    return funcs->p_eglMakeCurrent( egl->display, context ? draw->surface : EGL_NO_SURFACE, context ? read->surface : EGL_NO_SURFACE, context );
}

static void egldrv_pbuffer_destroy( struct opengl_drawable *drawable )
{
    TRACE( "%s\n", debugstr_opengl_drawable( drawable ) );
}

static const struct opengl_drawable_funcs egldrv_pbuffer_funcs =
{
    .destroy = egldrv_pbuffer_destroy,
};

static const struct opengl_driver_funcs egldrv_funcs =
{
    .p_init_egl_platform = egldrv_init_egl_platform,
    .p_get_proc_address = egldrv_get_proc_address,
    .p_init_pixel_formats = egldrv_init_pixel_formats,
    .p_describe_pixel_format = egldrv_describe_pixel_format,
    .p_init_wgl_extensions = egldrv_init_wgl_extensions,
    .p_surface_create = egldrv_surface_create,
    .p_pbuffer_create = egldrv_pbuffer_create,
    .p_pbuffer_updated = egldrv_pbuffer_updated,
    .p_pbuffer_bind = egldrv_pbuffer_bind,
    .p_context_create = egldrv_context_create,
    .p_context_destroy = egldrv_context_destroy,
    .p_make_current = egldrv_make_current,
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
    const char *extensions;
    EGLint major, minor;

    if (!funcs->egl_handle || !driver_funcs->p_init_egl_platform) return;

    driver_funcs->p_init_egl_platform( egl );
    if (!egl->type) egl->display = funcs->p_eglGetDisplay( EGL_DEFAULT_DISPLAY );
    else egl->display = funcs->p_eglGetPlatformDisplay( egl->type, egl->native_display, NULL );

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
} nulldrv_pixel_formats[] =
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

static void *nulldrv_get_proc_address( const char *name )
{
    return NULL;
}

static UINT nulldrv_init_pixel_formats( UINT *onscreen_count )
{
    *onscreen_count = ARRAY_SIZE(nulldrv_pixel_formats);
    return ARRAY_SIZE(nulldrv_pixel_formats);
}

static BOOL nulldrv_describe_pixel_format( int format, struct wgl_pixel_format *descr )
{
    if (format <= 0 || format > ARRAY_SIZE(nulldrv_pixel_formats)) return FALSE;

    memset( descr, 0, sizeof(*descr) );
    descr->pfd.nSize            = sizeof(*descr);
    descr->pfd.nVersion         = 1;
    descr->pfd.dwFlags          = PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_BITMAP | PFD_GENERIC_FORMAT;
    descr->pfd.iPixelType       = PFD_TYPE_RGBA;
    descr->pfd.cColorBits       = nulldrv_pixel_formats[format - 1].color_bits;
    descr->pfd.cRedBits         = nulldrv_pixel_formats[format - 1].red_bits;
    descr->pfd.cRedShift        = nulldrv_pixel_formats[format - 1].red_shift;
    descr->pfd.cGreenBits       = nulldrv_pixel_formats[format - 1].green_bits;
    descr->pfd.cGreenShift      = nulldrv_pixel_formats[format - 1].green_shift;
    descr->pfd.cBlueBits        = nulldrv_pixel_formats[format - 1].blue_bits;
    descr->pfd.cBlueShift       = nulldrv_pixel_formats[format - 1].blue_shift;
    descr->pfd.cAlphaBits       = nulldrv_pixel_formats[format - 1].alpha_bits;
    descr->pfd.cAlphaShift      = nulldrv_pixel_formats[format - 1].alpha_shift;
    descr->pfd.cAccumBits       = nulldrv_pixel_formats[format - 1].accum_bits;
    descr->pfd.cAccumRedBits    = nulldrv_pixel_formats[format - 1].accum_bits / 4;
    descr->pfd.cAccumGreenBits  = nulldrv_pixel_formats[format - 1].accum_bits / 4;
    descr->pfd.cAccumBlueBits   = nulldrv_pixel_formats[format - 1].accum_bits / 4;
    descr->pfd.cAccumAlphaBits  = nulldrv_pixel_formats[format - 1].accum_bits / 4;
    descr->pfd.cDepthBits       = nulldrv_pixel_formats[format - 1].depth_bits;
    descr->pfd.cStencilBits     = nulldrv_pixel_formats[format - 1].stencil_bits;
    descr->pfd.cAuxBuffers      = 0;
    descr->pfd.iLayerType       = PFD_MAIN_PLANE;

    return TRUE;
}

static const char *nulldrv_init_wgl_extensions( struct opengl_funcs *funcs )
{
    return "";
}

static BOOL nulldrv_surface_create( HWND hwnd, int format, struct opengl_drawable **drawable )
{
    return TRUE;
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

static BOOL nulldrv_make_current( struct opengl_drawable *draw_base, struct opengl_drawable *read_base, void *private )
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
    .p_pbuffer_create = nulldrv_pbuffer_create,
    .p_pbuffer_updated = nulldrv_pbuffer_updated,
    .p_pbuffer_bind = nulldrv_pbuffer_bind,
    .p_context_create = nulldrv_context_create,
    .p_context_destroy = nulldrv_context_destroy,
    .p_make_current = nulldrv_make_current,
};

static const char *win32u_wglGetExtensionsStringARB( HDC hdc )
{
    TRACE( "hdc %p\n", hdc );
    if (TRACE_ON(wgl)) dump_extensions( wgl_extensions );
    return wgl_extensions;
}

static const char *win32u_wglGetExtensionsStringEXT(void)
{
    TRACE( "\n" );
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

void set_window_opengl_drawable( HWND hwnd, struct opengl_drawable *new_drawable, BOOL current )
{
    struct opengl_drawable *old_drawable = NULL;
    WND *win;

    TRACE( "hwnd %p, new_drawable %s\n", hwnd, debugstr_opengl_drawable( new_drawable ) );

    if ((win = get_win_ptr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        struct opengl_drawable **ptr = current ? &win->current_drawable : &win->unused_drawable;
        old_drawable = *ptr;
        if ((*ptr = new_drawable)) opengl_drawable_add_ref( new_drawable );
        release_win_ptr( win );
    }

    if (old_drawable) opengl_drawable_release( old_drawable );
}

static struct opengl_drawable *get_window_current_drawable( HWND hwnd )
{
    struct opengl_drawable *drawable = NULL;
    WND *win;

    if ((win = get_win_ptr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        if ((drawable = win->current_drawable)) opengl_drawable_add_ref( drawable );
        release_win_ptr( win );
    }

    TRACE( "hwnd %p, drawable %s\n", hwnd, debugstr_opengl_drawable( drawable ) );
    return drawable;
}

static struct opengl_drawable *get_window_unused_drawable( HWND hwnd, int format )
{
    struct opengl_drawable *drawable = NULL;
    WND *win;

    if ((win = get_win_ptr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        drawable = win->unused_drawable;
        win->unused_drawable = NULL;
        release_win_ptr( win );
    }

    if (drawable && drawable->format != format)
    {
        opengl_drawable_release( drawable );
        drawable = NULL;
    }

    /* No compatible window drawable found, try creating a new one. This is not what native
     * is doing, it allows multiple contexts to be current on separate threads on the same
     * window, each drawing to the same back/front buffers. We cannot do that because host
     * OpenGL usually doesn't allow multiple contexts to use the same surface at the same time.
     */
    if (!drawable) driver_funcs->p_surface_create( hwnd, format, &drawable );

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
    DC *dc;

    if ((dc = get_dc_ptr( hdc )))
    {
        if ((drawable = dc->opengl_drawable)) opengl_drawable_add_ref( drawable );
        release_dc_ptr( dc );
    }

    TRACE( "hdc %p, drawable %s\n", hdc, debugstr_opengl_drawable( drawable ) );
    return drawable;
}

static BOOL create_memory_pbuffer( HDC hdc )
{
    const struct opengl_funcs *funcs = &display_funcs;
    dib_info dib = {.rect = {0, 0, 1, 1}};
    BOOL ret = TRUE;
    BITMAPOBJ *bmp;
    int format = 0;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    else if (dc->opengl_drawable) ret = FALSE;
    else if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC) ret = FALSE;
    else if ((bmp = GDI_GetObjPtr( dc->hBitmap, NTGDI_OBJ_BITMAP )))
    {
        if (!(format = dc->pixel_format)) ret = FALSE;
        init_dib_info_from_bitmapobj( &dib, bmp );
        GDI_ReleaseObj( dc->hBitmap );
    }
    release_dc_ptr( dc );

    if (ret)
    {
        int width = dib.rect.right - dib.rect.left, height = dib.rect.bottom - dib.rect.top;
        struct wgl_pbuffer *pbuffer;

        if (!(pbuffer = funcs->p_wglCreatePbufferARB( hdc, format, width, height, NULL )))
            WARN( "Failed to create pbuffer for memory DC %p\n", hdc );
        else
        {
            TRACE( "Created pbuffer %p for memory DC %p\n", pbuffer, hdc );
            set_dc_opengl_drawable( hdc, pbuffer->drawable );
            funcs->p_wglDestroyPbufferARB( pbuffer );
        }
    }

    return ret;
}

static BOOL flush_memory_dc( struct wgl_context *context, HDC hdc, BOOL write, void (*flush)(void) )
{
    const struct opengl_funcs *funcs = &display_funcs;
    BOOL ret = TRUE;
    BITMAPOBJ *bmp;
    DC *dc;

    if (!(dc = get_dc_ptr( hdc ))) return FALSE;
    if (get_gdi_object_type( hdc ) != NTGDI_OBJ_MEMDC) ret = FALSE;
    else if (context && (bmp = GDI_GetObjPtr( dc->hBitmap, NTGDI_OBJ_BITMAP )))
    {
        char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        struct bitblt_coords src = {0};
        struct gdi_image_bits bits;

        if (flush) flush();

        if (!get_image_from_bitmap( bmp, info, &bits, &src ))
        {
            int width = info->bmiHeader.biWidth, height = info->bmiHeader.biSizeImage / 4 / width;
            if (write) funcs->p_glDrawPixels( width, height, GL_BGRA, GL_UNSIGNED_BYTE, bits.ptr );
            else funcs->p_glReadPixels( 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, bits.ptr );
        }
        GDI_ReleaseObj( dc->hBitmap );
    }
    release_dc_ptr( dc );

    return ret;
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

        if (new_format > onscreen)
        {
            WARN( "Invalid format %d for %p/%p\n", new_format, hdc, hwnd );
            return FALSE;
        }

        TRACE( "%p/%p format %d, internal %u\n", hdc, hwnd, new_format, internal );

        if ((old_format = get_window_pixel_format( hwnd, FALSE )) && !internal) return old_format == new_format;

        if ((drawable = get_window_unused_drawable( hwnd, new_format )))
        {
            set_window_opengl_drawable( hwnd, drawable, TRUE );
            set_window_opengl_drawable( hwnd, drawable, FALSE );
            opengl_drawable_release( drawable );
        }

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
    memcpy( formats, pixel_formats, min( max_formats, formats_count ) * sizeof(*pixel_formats) );
    *num_formats = formats_count;
    *num_onscreen_formats = onscreen_count;
}

static void context_exchange_drawables( struct wgl_context *context, struct opengl_drawable **draw, struct opengl_drawable **read )
{
    struct opengl_drawable *old_draw = context->draw, *old_read = context->read;
    context->draw = *draw;
    context->read = *read;
    *draw = old_draw;
    *read = old_read;
}

static BOOL context_unset_current( struct wgl_context *context )
{
    struct opengl_drawable *old_draw = context->draw, *old_read = context->read;

    TRACE( "context %p\n", context );

    opengl_drawable_flush( old_read, old_read->interval, GL_FLUSH_WAS_CURRENT );
    if (old_read != old_draw) opengl_drawable_flush( old_draw, old_draw->interval, GL_FLUSH_WAS_CURRENT );
    if (driver_funcs->p_make_current( NULL, NULL, NULL )) return TRUE;
    opengl_drawable_flush( old_read, old_read->interval, GL_FLUSH_SET_CURRENT );
    if (old_read != old_draw) opengl_drawable_flush( old_draw, old_draw->interval, GL_FLUSH_SET_CURRENT );

    return FALSE;
}

/* return an updated drawable, recreating one if the window drawables have been invalidated (mostly wineandroid) */
static struct opengl_drawable *get_updated_drawable( HDC hdc, int format, struct opengl_drawable *drawable )
{
    struct opengl_drawable *current;
    HWND hwnd = NULL;

    if (hdc && !(hwnd = NtUserWindowFromDC( hdc ))) return get_dc_opengl_drawable( hdc );
    if (!hdc && drawable && drawable->client) hwnd = drawable->client->hwnd;
    if (!hwnd) return NULL;

    /* if the window still has a drawable, keep using the one we have */
    if (drawable && (current = get_window_current_drawable( hwnd )))
    {
        opengl_drawable_release( current );
        opengl_drawable_add_ref( drawable );
        return drawable;
    }

    /* get an updated drawable with the desired format */
    return get_window_unused_drawable( hwnd, format );
}

static BOOL context_sync_drawables( struct wgl_context *context, HDC draw_hdc, HDC read_hdc )
{
    struct opengl_drawable *new_draw, *new_read, *old_draw = NULL, *old_read = NULL;
    struct wgl_context *previous = NtCurrentTeb()->glContext;
    BOOL ret = FALSE;

    new_draw = get_updated_drawable( draw_hdc, context->format, context->draw );
    if (!draw_hdc && context->draw == context->read) opengl_drawable_add_ref( (new_read = new_draw) );
    else if (draw_hdc && draw_hdc == read_hdc) opengl_drawable_add_ref( (new_read = new_draw) );
    else new_read = get_updated_drawable( read_hdc, context->format, context->read );

    TRACE( "context %p, new_draw %s, new_read %s\n", context, debugstr_opengl_drawable( new_draw ), debugstr_opengl_drawable( new_read ) );

    if (!new_draw || !new_read)
    {
        WARN( "One of the drawable has been lost, ignoring\n" );
        return FALSE;
    }

    if (previous == context && new_draw == context->draw && new_read == context->read) ret = TRUE;
    else if (previous)
    {
        context_exchange_drawables( previous, &old_draw, &old_read ); /* take ownership of the previous context drawables */
        opengl_drawable_flush( old_read, old_read->interval, GL_FLUSH_WAS_CURRENT );
        if (old_read != old_draw) opengl_drawable_flush( old_draw, old_draw->interval, GL_FLUSH_WAS_CURRENT );
    }

    if (!ret && (ret = driver_funcs->p_make_current( new_draw, new_read, context->driver_private )))
    {
        NtCurrentTeb()->glContext = context;

        if (old_draw && old_draw != new_draw && old_draw != new_read && old_draw->client)
            set_window_opengl_drawable( old_draw->client->hwnd, old_draw, FALSE );
        if (old_read && old_read != new_draw && old_read != new_read && old_read->client)
            set_window_opengl_drawable( old_read->client->hwnd, old_read, FALSE );

        /* all good, release previous context drawables if any */
        if (old_draw) opengl_drawable_release( old_draw );
        if (old_read) opengl_drawable_release( old_read );

        opengl_drawable_flush( new_read, new_read->interval, GL_FLUSH_SET_CURRENT );
        if (new_read != new_draw) opengl_drawable_flush( new_draw, new_draw->interval, GL_FLUSH_SET_CURRENT );
    }

    if (ret)
    {
        opengl_drawable_flush( new_read, new_read->interval, 0 );
        opengl_drawable_flush( new_draw, new_draw->interval, 0 );
        /* update the current window drawable to the last used draw surface */
        if (new_draw->client) set_window_opengl_drawable( new_draw->client->hwnd, new_draw, TRUE );
        context_exchange_drawables( context, &new_draw, &new_read );
    }
    else if (previous)
    {
        opengl_drawable_flush( old_read, old_read->interval, GL_FLUSH_SET_CURRENT );
        if (old_read != old_draw) opengl_drawable_flush( old_draw, old_draw->interval, GL_FLUSH_SET_CURRENT );
        context_exchange_drawables( previous, &old_draw, &old_read ); /* give back ownership of the previous drawables */
        assert( !old_draw && !old_read );
    }

    if (new_draw) opengl_drawable_release( new_draw );
    if (new_read) opengl_drawable_release( new_read );
    return ret;
}

static void push_internal_context( struct wgl_context *context, HDC hdc, int format )
{
    TRACE( "context %p, hdc %p\n", context, hdc );

    if (!context->internal_context)
    {
        driver_funcs->p_context_create( format, context->driver_private, NULL, &context->internal_context );
        if (!context->internal_context) ERR( "Failed to create internal context\n" );
    }

    driver_funcs->p_make_current( context->draw, context->read, context->internal_context );
}

static void pop_internal_context( struct wgl_context *context )
{
    TRACE( "context %p\n", context );
    driver_funcs->p_make_current( context->draw, context->read, context->driver_private );
}

static BOOL win32u_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, struct wgl_context *context )
{
    struct wgl_context *prev_context = NtCurrentTeb()->glContext;
    BOOL created;
    int format;

    TRACE( "draw_hdc %p, read_hdc %p, context %p\n", draw_hdc, read_hdc, context );

    if (!context)
    {
        struct opengl_drawable *draw = NULL, *read = NULL;

        if (!(context = prev_context)) return TRUE;
        if (!context_unset_current( context )) return FALSE;
        NtCurrentTeb()->glContext = NULL;

        context_exchange_drawables( context, &draw, &read );
        if (draw->client) set_window_opengl_drawable( draw->client->hwnd, draw, FALSE );
        opengl_drawable_release( draw );
        if (read->client) set_window_opengl_drawable( read->client->hwnd, read, FALSE );
        opengl_drawable_release( read );

        return TRUE;
    }

    if ((format = get_dc_pixel_format( draw_hdc, TRUE )) <= 0)
    {
        WARN( "Invalid draw_hdc %p format %u\n", draw_hdc, format );
        if (!format) RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }
    if (context->format != format)
    {
        WARN( "Mismatched draw_hdc %p format %u, context %p format %u\n", draw_hdc, format, context, context->format );
        RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }

    created = create_memory_pbuffer( draw_hdc );
    if (!context_sync_drawables( context, draw_hdc, read_hdc )) return FALSE;
    NtCurrentTeb()->glContext = context;
    if (created) flush_memory_dc( context, draw_hdc, TRUE, NULL );

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
    TRACE( "pbuffer %p\n", pbuffer );

    opengl_drawable_release( pbuffer->drawable );
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
    int prev_texture = 0, format = win32u_wglGetPixelFormat( pbuffer->hdc );
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

    funcs->p_glGetIntegerv( binding_from_target( pbuffer->texture_target ), &prev_texture );
    push_internal_context( NtCurrentTeb()->glContext, pbuffer->hdc, format );

    /* Make sure that the prev_texture is set as the current texture state isn't shared
     * between contexts. After that copy the pbuffer texture data. */
    funcs->p_glBindTexture( pbuffer->texture_target, prev_texture );
    funcs->p_glCopyTexImage2D( pbuffer->texture_target, 0, pbuffer->texture_format, 0, 0,
                                        pbuffer->width, pbuffer->height, 0 );

    pop_internal_context( NtCurrentTeb()->glContext );
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

static BOOL win32u_wgl_context_reset( struct wgl_context *context, HDC hdc, struct wgl_context *share, const int *attribs )
{
    void *share_private = share ? share->driver_private : NULL;
    int format;

    TRACE( "context %p, hdc %p, share %p, attribs %p\n", context, hdc, share, attribs );

    if (context->internal_context)
    {
        driver_funcs->p_context_destroy( context->internal_context );
        context->internal_context = NULL;
    }
    if (context->driver_private && !driver_funcs->p_context_destroy( context->driver_private ))
    {
        WARN( "Failed to destroy driver context %p\n", context->driver_private );
        return FALSE;
    }
    context->driver_private = NULL;
    if (!hdc) return TRUE;

    if ((format = get_dc_pixel_format( hdc, TRUE )) <= 0)
    {
        if (!format) RtlSetLastWin32Error( ERROR_INVALID_PIXEL_FORMAT );
        return FALSE;
    }
    if (!driver_funcs->p_context_create( format, share_private, attribs, &context->driver_private ))
    {
        WARN( "Failed to create driver context for context %p\n", context );
        return FALSE;
    }
    context->format = format;

    TRACE( "reset context %p, format %u for driver context %p\n", context, format, context->driver_private );
    return TRUE;
}

static BOOL flush_memory_pbuffer( void (*flush)(void) )
{
    HDC draw_hdc = NtCurrentTeb()->glReserved1[0], read_hdc = NtCurrentTeb()->glReserved1[1];
    struct wgl_context *context = NtCurrentTeb()->glContext;
    BOOL created;

    TRACE( "context %p, draw_hdc %p, read_hdc %p, flush %p\n", context, draw_hdc, read_hdc, flush );

    created = create_memory_pbuffer( draw_hdc );
    if (context) context_sync_drawables( context, draw_hdc, read_hdc );
    if (created) flush_memory_dc( context, draw_hdc, TRUE, NULL );
    return flush_memory_dc( context, draw_hdc, FALSE, flush );
}

static BOOL win32u_wgl_context_flush( struct wgl_context *context, void (*flush)(void) )
{
    const struct opengl_funcs *funcs = &display_funcs;
    struct opengl_drawable *draw = context->draw;
    UINT flags = 0;
    int interval;

    if (!draw->client) return flush_memory_pbuffer( flush );
    interval = get_window_swap_interval( draw->client->hwnd );

    TRACE( "context %p, hwnd %p, interval %d, flush %p\n", context, draw->client->hwnd, interval, flush );

    context_sync_drawables( context, 0, 0 );

    if (flush) flush();
    if (flush == funcs->p_glFinish) flags |= GL_FLUSH_FINISHED;
    opengl_drawable_flush( context->draw, interval, flags );

    return TRUE;
}

static BOOL win32u_wglSwapBuffers( HDC hdc )
{
    struct wgl_context *context = NtCurrentTeb()->glContext;
    const struct opengl_funcs *funcs = &display_funcs;
    struct opengl_drawable *draw;
    int interval;
    HWND hwnd;
    BOOL ret;

    if (!(hwnd = NtUserWindowFromDC( hdc ))) return flush_memory_pbuffer( funcs->p_glFlush );

    interval = get_window_swap_interval( hwnd );

    TRACE( "context %p, hwnd %p, hdc %p, interval %d\n", context, hwnd, hdc, interval );

    if (context) context_sync_drawables( context, 0, 0 );

    if (context) opengl_drawable_add_ref( (draw = context->draw) );
    else if (!(draw = get_window_current_drawable( hwnd ))) return FALSE;

    opengl_drawable_flush( draw, interval, 0 );
    ret = draw->funcs->swap( draw );
    opengl_drawable_release( draw );

    return ret;
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
    if (!(pixel_formats = malloc( formats_count * sizeof(*pixel_formats) ))) ERR( "Failed to allocate memory for pixel formats\n" );
    else for (int i = 0; i < formats_count; i++) driver_funcs->p_describe_pixel_format( i + 1, pixel_formats + i );

#define USE_GL_FUNC(func) \
    if (!display_funcs.p_##func && !(display_funcs.p_##func = driver_funcs->p_get_proc_address( #func ))) \
    { \
        WARN( "%s not found for memory DCs.\n", #func ); \
        display_funcs.p_##func = default_funcs->p_##func; \
    }
    ALL_GL_FUNCS
    USE_GL_FUNC(glBindFramebuffer)
    USE_GL_FUNC(glCheckNamedFramebufferStatus)
    USE_GL_FUNC(glCreateFramebuffers)
    USE_GL_FUNC(glCreateRenderbuffers)
    USE_GL_FUNC(glDeleteFramebuffers)
    USE_GL_FUNC(glDeleteRenderbuffers)
    USE_GL_FUNC(glGetNamedFramebufferAttachmentParameteriv)
    USE_GL_FUNC(glNamedFramebufferDrawBuffer)
    USE_GL_FUNC(glNamedFramebufferReadBuffer)
    USE_GL_FUNC(glNamedFramebufferRenderbuffer)
    USE_GL_FUNC(glNamedRenderbufferStorageMultisample)
#undef USE_GL_FUNC

    display_funcs.p_wglGetProcAddress = win32u_wglGetProcAddress;
    display_funcs.p_get_pixel_formats = win32u_get_pixel_formats;

    strcpy( wgl_extensions, driver_funcs->p_init_wgl_extensions( &display_funcs ) );
    display_funcs.p_wglGetPixelFormat = win32u_wglGetPixelFormat;
    display_funcs.p_wglSetPixelFormat = win32u_wglSetPixelFormat;

    display_funcs.p_wglCreateContext = (void *)1; /* never called */
    display_funcs.p_wglDeleteContext = (void *)1; /* never called */
    display_funcs.p_wglCopyContext = (void *)1; /* never called */
    display_funcs.p_wglShareLists = (void *)1; /* never called */
    display_funcs.p_wglMakeCurrent = win32u_wglMakeCurrent;

    display_funcs.p_wglSwapBuffers = win32u_wglSwapBuffers;
    display_funcs.p_wgl_context_reset = win32u_wgl_context_reset;
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
    display_funcs.p_wglCreateContextAttribsARB = (void *)1; /* never called */

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
