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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "android.h"
#include "winternl.h"

#include "wine/opengl_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

static const struct egl_platform *egl;
static const struct opengl_funcs *funcs;
static const struct opengl_drawable_funcs android_drawable_funcs;
static const int egl_client_version = 2;

struct egl_pixel_format
{
    EGLConfig config;
};

struct android_context
{
    EGLConfig  config;
    EGLContext context;
};

struct gl_drawable
{
    struct opengl_drawable base;
    struct list     entry;
    ANativeWindow  *window;
    EGLSurface      surface;
    int             swap_interval;
};

static struct gl_drawable *impl_from_opengl_drawable( struct opengl_drawable *base )
{
    return CONTAINING_RECORD( base, struct gl_drawable, base );
}

static void *opengl_handle;
static struct list gl_drawables = LIST_INIT( gl_drawables );

pthread_mutex_t drawable_mutex;

static inline EGLConfig egl_config_for_format(int format)
{
    assert(format > 0 && format <= 2 * egl->config_count);
    if (format <= egl->config_count) return egl->configs[format - 1];
    return egl->configs[format - egl->config_count - 1];
}

static struct gl_drawable *gl_drawable_grab( struct gl_drawable *gl )
{
    opengl_drawable_add_ref( &gl->base );
    return gl;
}

static struct gl_drawable *create_gl_drawable( HWND hwnd, HDC hdc, int format, ANativeWindow *window )
{
    static const int attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    struct gl_drawable *gl;

    if (!(gl = opengl_drawable_create( sizeof(*gl), &android_drawable_funcs, format, hwnd, hdc ))) return NULL;

    if (!window) gl->window = create_ioctl_window( hwnd, TRUE, 1.0f );
    else gl->window = grab_ioctl_window( window );

    if (!window) gl->surface = funcs->p_eglCreatePbufferSurface( egl->display, egl_config_for_format(gl->base.format), attribs );
    else gl->surface = funcs->p_eglCreateWindowSurface( egl->display, egl_config_for_format(gl->base.format), gl->window, NULL );

    pthread_mutex_lock( &drawable_mutex );
    list_add_head( &gl_drawables, &gl->entry );
    opengl_drawable_add_ref( &gl->base );

    TRACE( "Created drawable %s with client window %p\n", debugstr_opengl_drawable( &gl->base ), gl->window );
    return gl;
}

static struct gl_drawable *get_gl_drawable( HWND hwnd, HDC hdc )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &drawable_mutex );
    LIST_FOR_EACH_ENTRY( gl, &gl_drawables, struct gl_drawable, entry )
    {
        if (hwnd && gl->base.hwnd == hwnd) return gl_drawable_grab( gl );
        if (hdc && gl->base.hdc == hdc) return gl_drawable_grab( gl );
    }
    pthread_mutex_unlock( &drawable_mutex );
    return NULL;
}

static void android_drawable_destroy( struct opengl_drawable *base )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );
    if (gl->surface) funcs->p_eglDestroySurface( egl->display, gl->surface );
    release_ioctl_window( gl->window );
}

static void release_gl_drawable( struct gl_drawable *gl )
{
    opengl_drawable_release( &gl->base );
    pthread_mutex_unlock( &drawable_mutex );
}

void destroy_gl_drawable( HWND hwnd )
{
    struct gl_drawable *gl;

    pthread_mutex_lock( &drawable_mutex );
    LIST_FOR_EACH_ENTRY( gl, &gl_drawables, struct gl_drawable, entry )
    {
        if (gl->base.hwnd != hwnd) continue;
        list_remove( &gl->entry );
        return release_gl_drawable( gl );
    }
    pthread_mutex_unlock( &drawable_mutex );
}

void update_gl_drawable( HWND hwnd )
{
    struct gl_drawable *old, *new;

    if (!(old = impl_from_opengl_drawable( get_window_opengl_drawable( hwnd ) ))) return;
    if ((new = create_gl_drawable( hwnd, 0, old->base.format, old->window )))
    {
        set_window_opengl_drawable( hwnd, &new->base );
        release_gl_drawable( new );
    }
    opengl_drawable_release( &old->base );

    NtUserRedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE );
}

static BOOL android_surface_create( HWND hwnd, HDC hdc, int format, struct opengl_drawable **drawable )
{
    struct gl_drawable *gl;

    TRACE( "hwnd %p, hdc %p, format %d, drawable %p\n", hwnd, hdc, format, drawable );

    if (*drawable)
    {
        EGLint pf;

        FIXME( "Updating drawable %s, multiple surfaces not implemented\n", debugstr_opengl_drawable( *drawable ) );

        gl = impl_from_opengl_drawable( *drawable );
        funcs->p_eglGetConfigAttrib( egl->display, egl_config_for_format(format), EGL_NATIVE_VISUAL_ID, &pf );
        gl->window->perform( gl->window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, pf );
        gl->base.hwnd = hwnd;
        gl->base.hdc = hdc;
        gl->base.format = format;

        TRACE( "Updated drawable %s\n", debugstr_opengl_drawable( *drawable ) );
        return TRUE;
    }

    if (!(gl = create_gl_drawable( hwnd, hdc, format, NULL ))) return FALSE;
    opengl_drawable_add_ref( (*drawable = &gl->base) );
    release_gl_drawable( gl );

    return TRUE;
}

static BOOL android_context_create( int format, void *share, const int *attribs, void **private )
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

    ctx->config  = egl_config_for_format(format);
    ctx->context = funcs->p_eglCreateContext( egl->display, ctx->config, shared_ctx ? shared_ctx->context : EGL_NO_CONTEXT, attribs );
    TRACE( "fmt %d ctx %p\n", format, ctx->context );

    *private = ctx;
    return TRUE;
}

static BOOL android_context_make_current( HDC draw_hdc, HDC read_hdc, void *private )
{
    struct android_context *ctx = private;
    BOOL ret = FALSE;
    struct gl_drawable *draw_gl, *read_gl = NULL;
    HWND draw_hwnd;

    TRACE( "%p %p %p\n", draw_hdc, read_hdc, ctx );

    if (!private)
    {
        funcs->p_eglMakeCurrent( egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
        return TRUE;
    }

    draw_hwnd = NtUserWindowFromDC( draw_hdc );
    if ((draw_gl = get_gl_drawable( draw_hwnd, draw_hdc )))
    {
        read_gl = get_gl_drawable( NtUserWindowFromDC( read_hdc ), read_hdc );
        TRACE( "%p/%p context %p surface %p/%p\n",
               draw_hdc, read_hdc, ctx->context, draw_gl->surface, read_gl->surface );
        ret = funcs->p_eglMakeCurrent( egl->display, draw_gl->surface, read_gl->surface, ctx->context );
        if (ret) goto done;
    }
    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );

done:
    if (read_gl) release_gl_drawable( read_gl );
    if (draw_gl) release_gl_drawable( draw_gl );
    return ret;
}

/***********************************************************************
 *		android_wglDeleteContext
 */
static BOOL android_context_destroy( void *private )
{
    struct android_context *ctx = private;
    funcs->p_eglDestroyContext( egl->display, ctx->context );
    free( ctx );
    return TRUE;
}

static EGLenum android_init_egl_platform( const struct egl_platform *platform, EGLNativeDisplayType *platform_display )
{
    egl = platform;
    *platform_display = EGL_DEFAULT_DISPLAY;
    return EGL_PLATFORM_ANDROID_KHR;
}

static void *android_get_proc_address( const char *name )
{
    void *ptr;
    if ((ptr = dlsym( opengl_handle, name ))) return ptr;
    return funcs->p_eglGetProcAddress( name );
}

static void set_swap_interval( struct gl_drawable *gl, int interval )
{
    if (interval < 0) interval = -interval;
    if (gl->swap_interval == interval) return;
    funcs->p_eglSwapInterval( egl->display, interval );
    gl->swap_interval = interval;
}

static BOOL android_drawable_swap( struct opengl_drawable *base, int interval )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "drawable %s surface %p\n", debugstr_opengl_drawable( base ), gl->surface );

    set_swap_interval( gl, interval );
    funcs->p_eglSwapBuffers( egl->display, gl->surface );
    return TRUE;
}

static BOOL android_drawable_flush( struct opengl_drawable *base, int interval, void (*flush)(void) )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "drawable %s surface %p\n", debugstr_opengl_drawable( base ), gl->surface );

    set_swap_interval( gl, interval );
    if (flush) flush();
    return TRUE;
}

static const char *android_init_wgl_extensions( struct opengl_funcs *funcs )
{
    return "WGL_EXT_framebuffer_sRGB";
}

static struct opengl_driver_funcs android_driver_funcs =
{
    .p_init_egl_platform = android_init_egl_platform,
    .p_get_proc_address = android_get_proc_address,
    .p_init_wgl_extensions = android_init_wgl_extensions,
    .p_surface_create = android_surface_create,
    .p_context_create = android_context_create,
    .p_context_destroy = android_context_destroy,
    .p_context_make_current = android_context_make_current,
};

static const struct opengl_drawable_funcs android_drawable_funcs =
{
    .destroy = android_drawable_destroy,
    .flush = android_drawable_flush,
    .swap = android_drawable_swap,
};

/**********************************************************************
 *           ANDROID_OpenGLInit
 */
UINT ANDROID_OpenGLInit( UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs )
{
    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR( "version mismatch, opengl32 wants %u but driver has %u\n", version, WINE_OPENGL_DRIVER_VERSION );
        return STATUS_INVALID_PARAMETER;
    }
    if (!opengl_funcs->egl_handle) return STATUS_NOT_SUPPORTED;
    if (!(opengl_handle = dlopen( SONAME_LIBGLESV2, RTLD_NOW|RTLD_GLOBAL )))
    {
        ERR( "failed to load %s: %s\n", SONAME_LIBGLESV2, dlerror() );
        return STATUS_NOT_SUPPORTED;
    }
    funcs = opengl_funcs;

    android_driver_funcs.p_init_pixel_formats = (*driver_funcs)->p_init_pixel_formats;
    android_driver_funcs.p_describe_pixel_format = (*driver_funcs)->p_describe_pixel_format;

    *driver_funcs = &android_driver_funcs;
    return STATUS_SUCCESS;
}
