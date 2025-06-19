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

struct gl_drawable
{
    struct opengl_drawable base;
    ANativeWindow  *window;
};

static struct gl_drawable *impl_from_opengl_drawable( struct opengl_drawable *base )
{
    return CONTAINING_RECORD( base, struct gl_drawable, base );
}

static void *opengl_handle;

static inline EGLConfig egl_config_for_format(int format)
{
    assert(format > 0 && format <= 2 * egl->config_count);
    if (format <= egl->config_count) return egl->configs[format - 1];
    return egl->configs[format - egl->config_count - 1];
}

static struct gl_drawable *create_gl_drawable( HWND hwnd, HDC hdc, int format, ANativeWindow *window )
{
    static const int attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    struct gl_drawable *gl;

    if (!(gl = opengl_drawable_create( sizeof(*gl), &android_drawable_funcs, format, hwnd, hdc ))) return NULL;

    if (!window) gl->window = create_ioctl_window( hwnd, TRUE, 1.0f );
    else gl->window = grab_ioctl_window( window );

    if (!window) gl->base.surface = funcs->p_eglCreatePbufferSurface( egl->display, egl_config_for_format(gl->base.format), attribs );
    else gl->base.surface = funcs->p_eglCreateWindowSurface( egl->display, egl_config_for_format(gl->base.format), gl->window, NULL );

    TRACE( "Created drawable %s with client window %p\n", debugstr_opengl_drawable( &gl->base ), gl->window );
    return gl;
}

static void android_drawable_destroy( struct opengl_drawable *base )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );
    release_ioctl_window( gl->window );
}

static void android_drawable_detach( struct opengl_drawable *base )
{
    TRACE( "%s\n", debugstr_opengl_drawable( base ) );
}

static void android_drawable_update( struct opengl_drawable *base )
{
    TRACE( "%s\n", debugstr_opengl_drawable( base ) );
}

void update_gl_drawable( HWND hwnd )
{
    struct gl_drawable *old, *new;

    if (!(old = impl_from_opengl_drawable( get_window_opengl_drawable( hwnd ) ))) return;
    if ((new = create_gl_drawable( hwnd, 0, old->base.format, old->window )))
    {
        set_window_opengl_drawable( hwnd, &new->base );
        opengl_drawable_release( &new->base );
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
    *drawable = &gl->base;
    return TRUE;
}

static BOOL android_make_current( struct opengl_drawable *draw_base, struct opengl_drawable *read_base, void *context )
{
    struct gl_drawable *draw = impl_from_opengl_drawable( draw_base ), *read = impl_from_opengl_drawable( read_base );
    TRACE( "draw %s, read %s, context %p\n", debugstr_opengl_drawable( draw_base ), debugstr_opengl_drawable( read_base ), context );
    return funcs->p_eglMakeCurrent( egl->display, context ? draw->base.surface : EGL_NO_SURFACE, context ? read->base.surface : EGL_NO_SURFACE, context );
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

static BOOL android_drawable_swap( struct opengl_drawable *base )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "drawable %s surface %p\n", debugstr_opengl_drawable( base ), gl->base.surface );

    funcs->p_eglSwapBuffers( egl->display, gl->base.surface );
    return TRUE;
}

static void android_drawable_flush( struct opengl_drawable *base, UINT flags )
{
    struct gl_drawable *gl = impl_from_opengl_drawable( base );

    TRACE( "drawable %s, surface %p, flags %#x\n", debugstr_opengl_drawable( base ), gl->base.surface, flags );

    if (flags & GL_FLUSH_INTERVAL) funcs->p_eglSwapInterval( egl->display, abs( base->interval ) );
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
    .p_make_current = android_make_current,
};

static const struct opengl_drawable_funcs android_drawable_funcs =
{
    .destroy = android_drawable_destroy,
    .detach = android_drawable_detach,
    .update = android_drawable_update,
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
    android_driver_funcs.p_context_create = (*driver_funcs)->p_context_create;
    android_driver_funcs.p_context_destroy = (*driver_funcs)->p_context_destroy;

    *driver_funcs = &android_driver_funcs;
    return STATUS_SUCCESS;
}
