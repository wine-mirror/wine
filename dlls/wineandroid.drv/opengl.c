/*
 * Android OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2005 Alex Woods
 * Copyright 2005 Raphael Junqueira
 * Copyright 2006-2009 Roderick Colenbrander
 * Copyright 2006 Tomas Carnecky
 * Copyright 2013 Matteo Bruni
 * Copyright 2012, 2013, 2014, 2017 Alexandre Julliard
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#ifdef HAVE_EGL_EGL_H
#include <EGL/egl.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "android.h"
#include "winternl.h"

#include "wine/opengl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

#define DECL_FUNCPTR(f) typeof(f) * p_##f = NULL
DECL_FUNCPTR( eglCreateContext );
DECL_FUNCPTR( eglCreateWindowSurface );
DECL_FUNCPTR( eglCreatePbufferSurface );
DECL_FUNCPTR( eglDestroyContext );
DECL_FUNCPTR( eglDestroySurface );
DECL_FUNCPTR( eglGetConfigAttrib );
DECL_FUNCPTR( eglGetConfigs );
DECL_FUNCPTR( eglGetDisplay );
DECL_FUNCPTR( eglGetProcAddress );
DECL_FUNCPTR( eglInitialize );
DECL_FUNCPTR( eglMakeCurrent );
DECL_FUNCPTR( eglSwapBuffers );
DECL_FUNCPTR( eglSwapInterval );
#undef DECL_FUNCPTR

static const int egl_client_version = 2;

struct egl_pixel_format
{
    EGLConfig config;
};

struct android_context
{
    struct list entry;
    EGLConfig  config;
    EGLContext context;
    EGLSurface surface;
    HWND       hwnd;
    BOOL       refresh;
};

struct gl_drawable
{
    struct list     entry;
    HWND            hwnd;
    HDC             hdc;
    int             format;
    ANativeWindow  *window;
    EGLSurface      surface;
    EGLSurface      pbuffer;
    int             swap_interval;
};

static void *egl_handle;
static void *opengl_handle;
static struct egl_pixel_format *pixel_formats;
static int nb_pixel_formats, nb_onscreen_formats;
static EGLDisplay display;
static char wgl_extensions[4096];
static struct opengl_funcs egl_funcs;

static struct list gl_contexts = LIST_INIT( gl_contexts );
static struct list gl_drawables = LIST_INIT( gl_drawables );

pthread_mutex_t drawable_mutex;

static struct gl_drawable *create_gl_drawable( HWND hwnd, HDC hdc, int format )
{
    static const int attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    struct gl_drawable *gl = malloc( sizeof(*gl) );

    gl->hwnd   = hwnd;
    gl->hdc    = hdc;
    gl->format = format;
    gl->window = create_ioctl_window( hwnd, TRUE, 1.0f );
    gl->surface = 0;
    gl->pbuffer = p_eglCreatePbufferSurface( display, pixel_formats[gl->format - 1].config, attribs );
    pthread_mutex_lock( &drawable_mutex );
    list_add_head( &gl_drawables, &gl->entry );
    return gl;
}

static struct gl_drawable *get_gl_drawable( HWND hwnd, HDC hdc )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &drawable_mutex );
    LIST_FOR_EACH_ENTRY( gl, &gl_drawables, struct gl_drawable, entry )
    {
        if (hwnd && gl->hwnd == hwnd) return gl;
        if (hdc && gl->hdc == hdc) return gl;
    }
    pthread_mutex_unlock( &drawable_mutex );
    return NULL;
}

static void release_gl_drawable( struct gl_drawable *gl )
{
    if (gl) pthread_mutex_unlock( &drawable_mutex );
}

void destroy_gl_drawable( HWND hwnd )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &drawable_mutex );
    LIST_FOR_EACH_ENTRY( gl, &gl_drawables, struct gl_drawable, entry )
    {
        if (gl->hwnd != hwnd) continue;
        list_remove( &gl->entry );
        if (gl->surface) p_eglDestroySurface( display, gl->surface );
        if (gl->pbuffer) p_eglDestroySurface( display, gl->pbuffer );
        release_ioctl_window( gl->window );
        free( gl );
        break;
    }
    pthread_mutex_unlock( &drawable_mutex );
}

static BOOL refresh_context( struct android_context *ctx )
{
    BOOL ret = InterlockedExchange( &ctx->refresh, FALSE );

    if (ret)
    {
        TRACE( "refreshing hwnd %p context %p surface %p\n", ctx->hwnd, ctx->context, ctx->surface );
        p_eglMakeCurrent( display, ctx->surface, ctx->surface, ctx->context );
        NtUserRedrawWindow( ctx->hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE );
    }
    return ret;
}

void update_gl_drawable( HWND hwnd )
{
    struct gl_drawable *gl;
    struct android_context *ctx;

    if ((gl = get_gl_drawable( hwnd, 0 )))
    {
        if (!gl->surface &&
            (gl->surface = p_eglCreateWindowSurface( display, pixel_formats[gl->format - 1].config, gl->window, NULL )))
        {
            LIST_FOR_EACH_ENTRY( ctx, &gl_contexts, struct android_context, entry )
            {
                if (ctx->hwnd != hwnd) continue;
                TRACE( "hwnd %p refreshing %p %scurrent\n", hwnd, ctx, NtCurrentTeb()->glReserved2 == ctx ? "" : "not " );
                ctx->surface = gl->surface;
                if (NtCurrentTeb()->glReserved2 == ctx)
                    p_eglMakeCurrent( display, ctx->surface, ctx->surface, ctx->context );
                else
                    InterlockedExchange( &ctx->refresh, TRUE );
            }
        }
        release_gl_drawable( gl );
        NtUserRedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE );
    }
}

static BOOL android_set_pixel_format( HWND hwnd, int old_format, int new_format, BOOL internal )
{
    struct gl_drawable *gl;

    TRACE( "hwnd %p, old_format %d, new_format %d, internal %u\n", hwnd, old_format, new_format, internal );

    if ((gl = get_gl_drawable( hwnd, 0 )))
    {
        if (internal)
        {
            EGLint pf;
            p_eglGetConfigAttrib( display, pixel_formats[new_format - 1].config, EGL_NATIVE_VISUAL_ID, &pf );
            gl->window->perform( gl->window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, pf );
            gl->format = new_format;
        }
    }
    else gl = create_gl_drawable( hwnd, 0, new_format );
    release_gl_drawable( gl );

    return TRUE;
}

static BOOL android_context_create( HDC hdc, int format, void *share, const int *attribs, void **private )
{
    struct android_context *ctx, *shared_ctx = share;
    int count = 0, egl_attribs[3];
    BOOL opengl_es = FALSE;

    if (!attribs) opengl_es = TRUE;
    while (attribs && *attribs && count < 2)
    {
        switch (*attribs)
        {
        case WGL_CONTEXT_PROFILE_MASK_ARB:
            if (attribs[1] == WGL_CONTEXT_ES2_PROFILE_BIT_EXT)
                opengl_es = TRUE;
            break;
        case WGL_CONTEXT_MAJOR_VERSION_ARB:
            egl_attribs[count++] = EGL_CONTEXT_CLIENT_VERSION;
            egl_attribs[count++] = attribs[1];
            break;
        default:
            FIXME("Unhandled attributes: %#x %#x\n", attribs[0], attribs[1]);
        }
        attribs += 2;
    }
    if (!opengl_es)
    {
        WARN("Requested creation of an OpenGL (non ES) context, that's not supported.\n");
        return FALSE;
    }
    if (!count)  /* FIXME: force version if not specified */
    {
        egl_attribs[count++] = EGL_CONTEXT_CLIENT_VERSION;
        egl_attribs[count++] = egl_client_version;
    }
    egl_attribs[count] = EGL_NONE;
    attribs = egl_attribs;

    ctx = malloc( sizeof(*ctx) );

    ctx->config  = pixel_formats[format - 1].config;
    ctx->surface = 0;
    ctx->refresh = FALSE;
    ctx->context = p_eglCreateContext( display, ctx->config, shared_ctx ? shared_ctx->context : EGL_NO_CONTEXT, attribs );
    TRACE( "%p fmt %d ctx %p\n", hdc, format, ctx->context );
    list_add_head( &gl_contexts, &ctx->entry );

    *private = ctx;
    return TRUE;
}

static BOOL android_describe_pixel_format( int format, struct wgl_pixel_format *desc )
{
    struct egl_pixel_format *fmt = pixel_formats + format - 1;
    PIXELFORMATDESCRIPTOR *pfd = &desc->pfd;
    EGLint val;
    EGLConfig config = fmt->config;

    if (format <= 0 || format > nb_pixel_formats) return FALSE;

    memset( pfd, 0, sizeof(*pfd) );
    pfd->nSize = sizeof(*pfd);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->iLayerType = PFD_MAIN_PLANE;

    p_eglGetConfigAttrib( display, config, EGL_BUFFER_SIZE, &val );
    pfd->cColorBits = val;
    p_eglGetConfigAttrib( display, config, EGL_RED_SIZE, &val );
    pfd->cRedBits = val;
    p_eglGetConfigAttrib( display, config, EGL_GREEN_SIZE, &val );
    pfd->cGreenBits = val;
    p_eglGetConfigAttrib( display, config, EGL_BLUE_SIZE, &val );
    pfd->cBlueBits = val;
    p_eglGetConfigAttrib( display, config, EGL_ALPHA_SIZE, &val );
    pfd->cAlphaBits = val;
    p_eglGetConfigAttrib( display, config, EGL_DEPTH_SIZE, &val );
    pfd->cDepthBits = val;
    p_eglGetConfigAttrib( display, config, EGL_STENCIL_SIZE, &val );
    pfd->cStencilBits = val;

    pfd->cAlphaShift = 0;
    pfd->cBlueShift = pfd->cAlphaShift + pfd->cAlphaBits;
    pfd->cGreenShift = pfd->cBlueShift + pfd->cBlueBits;
    pfd->cRedShift = pfd->cGreenShift + pfd->cGreenBits;
    return TRUE;
}

static BOOL android_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    struct android_context *ctx = private;
    BOOL ret = FALSE;
    struct gl_drawable *draw_gl, *read_gl = NULL;
    EGLSurface draw_surface, read_surface;
    HWND draw_hwnd;

    TRACE( "%p %p %p\n", draw_hdc, read_hdc, ctx );

    if (!private)
    {
        p_eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        NtCurrentTeb()->glReserved2 = NULL;
        return TRUE;
    }

    draw_hwnd = NtUserWindowFromDC( draw_hdc );
    if ((draw_gl = get_gl_drawable( draw_hwnd, draw_hdc )))
    {
        read_gl = get_gl_drawable( NtUserWindowFromDC( read_hdc ), read_hdc );
        draw_surface = draw_gl->surface ? draw_gl->surface : draw_gl->pbuffer;
        read_surface = read_gl->surface ? read_gl->surface : read_gl->pbuffer;
        TRACE( "%p/%p context %p surface %p/%p\n",
               draw_hdc, read_hdc, ctx->context, draw_surface, read_surface );
        ret = p_eglMakeCurrent( display, draw_surface, read_surface, ctx->context );
        if (ret)
        {
            ctx->surface = draw_gl->surface;
            ctx->hwnd    = draw_hwnd;
            ctx->refresh = FALSE;
            NtCurrentTeb()->glReserved2 = ctx;
            goto done;
        }
    }
    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );

done:
    release_gl_drawable( read_gl );
    release_gl_drawable( draw_gl );
    return ret;
}

/***********************************************************************
 *		android_wglCopyContext
 */
static BOOL android_context_copy( void *src, void *dst, UINT mask )
{
    FIXME( "%p -> %p mask %#x unsupported\n", src, dst, mask );
    return FALSE;
}

/***********************************************************************
 *		android_wglDeleteContext
 */
static BOOL android_context_destroy( void *private )
{
    struct android_context *ctx = private;
    pthread_mutex_lock( &drawable_mutex );
    list_remove( &ctx->entry );
    pthread_mutex_unlock( &drawable_mutex );
    p_eglDestroyContext( display, ctx->context );
    free( ctx );
    return TRUE;
}

static void *android_get_proc_address( const char *name )
{
    void *ptr;
    if ((ptr = dlsym( opengl_handle, name ))) return ptr;
    return p_eglGetProcAddress( name );
}

static BOOL android_context_share( void *org, void *dest )
{
    FIXME( "%p %p\n", org, dest );
    return FALSE;
}

static void set_swap_interval( struct gl_drawable *gl, int interval )
{
    if (interval < 0) interval = -interval;
    if (gl->swap_interval == interval) return;
    p_eglSwapInterval( display, interval );
    gl->swap_interval = interval;
}

static BOOL android_swap_buffers( void *private, HWND hwnd, HDC hdc, int interval )
{
    struct android_context *ctx = private;
    struct gl_drawable *gl;

    if (!ctx) return FALSE;

    TRACE( "%p hwnd %p context %p surface %p\n", hdc, ctx->hwnd, ctx->context, ctx->surface );

    if (refresh_context( ctx )) return TRUE;

    if ((gl = get_gl_drawable( hwnd, hdc )))
    {
        set_swap_interval( gl, interval );
        release_gl_drawable( gl );
    }

    if (ctx->surface) p_eglSwapBuffers( display, ctx->surface );
    return TRUE;
}

static BOOL android_context_flush( void *private, HWND hwnd, HDC hdc, int interval, BOOL finish )
{
    struct android_context *ctx = private;
    struct gl_drawable *gl;

    TRACE( "hwnd %p context %p\n", ctx->hwnd, ctx->context );
    refresh_context( ctx );

    if ((gl = get_gl_drawable( hwnd, hdc )))
    {
        set_swap_interval( gl, interval );
        release_gl_drawable( gl );
    }
    return FALSE;
}

static void register_extension( const char *ext )
{
    if (wgl_extensions[0]) strcat( wgl_extensions, " " );
    strcat( wgl_extensions, ext );
    TRACE( "%s\n", ext );
}

static const char *android_init_wgl_extensions( struct opengl_funcs *funcs )
{
    register_extension("WGL_EXT_framebuffer_sRGB");
    return wgl_extensions;
}

static UINT android_init_pixel_formats( UINT *onscreen_count )
{
    EGLConfig *configs;
    EGLint count, i, pass;

    p_eglGetConfigs( display, NULL, 0, &count );
    configs = malloc( count * sizeof(*configs) );
    pixel_formats = malloc( count * sizeof(*pixel_formats) );
    p_eglGetConfigs( display, configs, count, &count );
    if (!count || !configs || !pixel_formats)
    {
        free( configs );
        free( pixel_formats );
        ERR( "eglGetConfigs returned no configs\n" );
        return 0;
    }

    for (pass = 0; pass < 2; pass++)
    {
        for (i = 0; i < count; i++)
        {
            EGLint id, type, visual_id, native, render, color, r, g, b, d, s;

            p_eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE, &type );
            if (!(type & EGL_WINDOW_BIT) == !pass) continue;
            p_eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &render );
            if (egl_client_version == 2 && !(render & EGL_OPENGL_ES2_BIT)) continue;

            pixel_formats[nb_pixel_formats++].config = configs[i];

            p_eglGetConfigAttrib( display, configs[i], EGL_CONFIG_ID, &id );
            p_eglGetConfigAttrib( display, configs[i], EGL_NATIVE_VISUAL_ID, &visual_id );
            p_eglGetConfigAttrib( display, configs[i], EGL_NATIVE_RENDERABLE, &native );
            p_eglGetConfigAttrib( display, configs[i], EGL_COLOR_BUFFER_TYPE, &color );
            p_eglGetConfigAttrib( display, configs[i], EGL_RED_SIZE, &r );
            p_eglGetConfigAttrib( display, configs[i], EGL_GREEN_SIZE, &g );
            p_eglGetConfigAttrib( display, configs[i], EGL_BLUE_SIZE, &b );
            p_eglGetConfigAttrib( display, configs[i], EGL_DEPTH_SIZE, &d );
            p_eglGetConfigAttrib( display, configs[i], EGL_STENCIL_SIZE, &s );
            TRACE( "%u: config %u id %u type %x visual %u native %u render %x colortype %u rgb %u,%u,%u depth %u stencil %u\n",
                   nb_pixel_formats, i, id, type, visual_id, native, render, color, r, g, b, d, s );
        }
        if (!pass) nb_onscreen_formats = nb_pixel_formats;
    }

    *onscreen_count = nb_onscreen_formats;
    return nb_pixel_formats;
}

static const struct opengl_driver_funcs android_driver_funcs =
{
    .p_get_proc_address = android_get_proc_address,
    .p_init_pixel_formats = android_init_pixel_formats,
    .p_describe_pixel_format = android_describe_pixel_format,
    .p_init_wgl_extensions = android_init_wgl_extensions,
    .p_set_pixel_format = android_set_pixel_format,
    .p_swap_buffers = android_swap_buffers,
    .p_context_create = android_context_create,
    .p_context_destroy = android_context_destroy,
    .p_context_copy = android_context_copy,
    .p_context_share = android_context_share,
    .p_context_flush = android_context_flush,
    .p_context_make_current = android_context_make_current,
};

/**********************************************************************
 *           ANDROID_OpenGLInit
 */
UINT ANDROID_OpenGLInit( UINT version, struct opengl_funcs **funcs, const struct opengl_driver_funcs **driver_funcs )
{
    EGLint major, minor;

    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but driver has %u\n", version, WINE_OPENGL_DRIVER_VERSION );
        return STATUS_INVALID_PARAMETER;
    }
    if (!(egl_handle = dlopen( SONAME_LIBEGL, RTLD_NOW|RTLD_GLOBAL )))
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBEGL, dlerror() );
        return STATUS_NOT_SUPPORTED;
    }
    if (!(opengl_handle = dlopen( SONAME_LIBGLESV2, RTLD_NOW|RTLD_GLOBAL )))
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBGLESV2, dlerror() );
        return STATUS_NOT_SUPPORTED;
    }

#define LOAD_FUNCPTR(func) do { \
        if (!(p_##func = dlsym( egl_handle, #func ))) \
        { ERR( "can't find symbol %s\n", #func); return FALSE; }    \
    } while(0)
    LOAD_FUNCPTR( eglCreateContext );
    LOAD_FUNCPTR( eglCreateWindowSurface );
    LOAD_FUNCPTR( eglCreatePbufferSurface );
    LOAD_FUNCPTR( eglDestroyContext );
    LOAD_FUNCPTR( eglDestroySurface );
    LOAD_FUNCPTR( eglGetConfigAttrib );
    LOAD_FUNCPTR( eglGetConfigs );
    LOAD_FUNCPTR( eglGetDisplay );
    LOAD_FUNCPTR( eglGetProcAddress );
    LOAD_FUNCPTR( eglInitialize );
    LOAD_FUNCPTR( eglMakeCurrent );
    LOAD_FUNCPTR( eglSwapBuffers );
    LOAD_FUNCPTR( eglSwapInterval );
#undef LOAD_FUNCPTR

    display = p_eglGetDisplay( EGL_DEFAULT_DISPLAY );
    if (!p_eglInitialize( display, &major, &minor )) return 0;
    TRACE( "display %p version %u.%u\n", display, major, minor );

    *funcs = &egl_funcs;
    *driver_funcs = &android_driver_funcs;
    return STATUS_SUCCESS;
}

static struct opengl_funcs egl_funcs;
