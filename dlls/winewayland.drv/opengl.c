/*
 * Wayland OpenGL functions
 *
 * Copyright 2020 Alexandros Frantzis for Collabora Ltd.
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
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "waylanddrv.h"
#include "wine/debug.h"

#ifdef HAVE_LIBWAYLAND_EGL

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

#include <wayland-egl.h>

#include "wine/opengl_driver.h"

static const struct egl_platform *egl;
static const struct opengl_funcs *funcs;
static const struct opengl_drawable_funcs wayland_drawable_funcs;

static pthread_mutex_t gl_object_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct list gl_drawables = LIST_INIT(gl_drawables);
static struct list gl_contexts = LIST_INIT(gl_contexts);

struct wayland_gl_drawable
{
    struct opengl_drawable base;
    struct list entry;
    struct wayland_client_surface *client;
    struct wl_egl_window *wl_egl_window;
    EGLSurface surface;
    LONG resized;
    BOOL double_buffered;
};

static struct wayland_gl_drawable *impl_from_opengl_drawable(struct opengl_drawable *base)
{
    return CONTAINING_RECORD(base, struct wayland_gl_drawable, base);
}

struct wayland_context
{
    struct list entry;
    EGLConfig config;
    EGLContext context;
    struct wayland_gl_drawable *draw, *read;
};

struct wgl_pbuffer
{
    struct list entry;
    struct wayland_gl_drawable *gl;
    int width, height, pixel_format;
    int texture_format, texture_target, texture_binding;
    EGLContext tmp_context, prev_context;
};

struct wayland_pbuffer_dc
{
    struct list entry;
    HDC hdc;
    struct wayland_gl_drawable *gl;
};

/* lookup the existing drawable for a window, gl_object_mutex must be held */
static struct wayland_gl_drawable *find_drawable(HWND hwnd, HDC hdc)
{
    struct wayland_gl_drawable *gl;
    LIST_FOR_EACH_ENTRY(gl, &gl_drawables, struct wayland_gl_drawable, entry)
    {
        if (hwnd && gl->base.hwnd == hwnd) return gl;
        if (hdc && gl->base.hdc == hdc) return gl;
    }
    return NULL;
}

static struct wayland_gl_drawable *wayland_gl_drawable_acquire(struct wayland_gl_drawable *gl)
{
    opengl_drawable_add_ref(&gl->base);
    return gl;
}

static struct wayland_gl_drawable *wayland_gl_drawable_get(HWND hwnd, HDC hdc)
{
    struct wayland_gl_drawable *ret;

    pthread_mutex_lock(&gl_object_mutex);
    if ((ret = find_drawable(hwnd, hdc)))
        ret = wayland_gl_drawable_acquire(ret);
    pthread_mutex_unlock(&gl_object_mutex);

    return ret;
}

static void wayland_drawable_destroy(struct opengl_drawable *base)
{
    struct wayland_gl_drawable *gl = impl_from_opengl_drawable(base);

    if (!gl->base.hwnd)
    {
        pthread_mutex_lock(&gl_object_mutex);
        list_remove(&gl->entry);
        pthread_mutex_unlock(&gl_object_mutex);
    }
    if (gl->surface) funcs->p_eglDestroySurface(egl->display, gl->surface);
    if (gl->wl_egl_window) wl_egl_window_destroy(gl->wl_egl_window);
    if (gl->client)
    {
        HWND hwnd = wl_surface_get_user_data(gl->client->wl_surface);
        struct wayland_win_data *data = wayland_win_data_get(hwnd);

        if (wayland_client_surface_release(gl->client) && data)
            data->client_surface = NULL;

        if (data) wayland_win_data_release(data);
    }
}

static inline BOOL is_onscreen_format(int format)
{
    return format > 0 && format <= egl->config_count;
}

static inline EGLConfig egl_config_for_format(int format)
{
    assert(format > 0 && format <= 2 * egl->config_count);
    if (format <= egl->config_count) return egl->configs[format - 1];
    return egl->configs[format - egl->config_count - 1];
}

static struct wayland_gl_drawable *wayland_gl_drawable_create(HWND hwnd, HDC hdc, int format, int width, int height)
{
    struct wayland_gl_drawable *gl;
    EGLint attribs[4], *attrib = attribs;

    TRACE("hwnd=%p format=%d\n", hwnd, format);

    if (!egl->has_EGL_EXT_present_opaque)
        WARN("Missing EGL_EXT_present_opaque extension\n");
    else
    {
        *attrib++ = EGL_PRESENT_OPAQUE_EXT;
        *attrib++ = EGL_TRUE;
    }
    *attrib++ = EGL_NONE;

    if (!(gl = opengl_drawable_create(sizeof(*gl), &wayland_drawable_funcs, format, hwnd, hdc))) return NULL;

    /* Get the client surface for the HWND. If don't have a wayland surface
     * (e.g., HWND_MESSAGE windows) just create a dummy surface to act as the
     * target render surface. */
    if (!(gl->client = wayland_client_surface_create(hwnd))) goto err;
    set_client_surface(hwnd, gl->client);

    gl->wl_egl_window = wl_egl_window_create(gl->client->wl_surface, width, height);
    if (!gl->wl_egl_window)
    {
        ERR("Failed to create wl_egl_window\n");
        goto err;
    }

    gl->surface = funcs->p_eglCreateWindowSurface(egl->display, egl_config_for_format(format),
                                                  gl->wl_egl_window, attribs);
    if (!gl->surface)
    {
        ERR("Failed to create EGL surface\n");
        goto err;
    }

    gl->double_buffered = is_onscreen_format(format);

    TRACE("Created drawable %s with egl_surface %p\n", debugstr_opengl_drawable(&gl->base), gl->surface);

    return gl;

err:
    opengl_drawable_release(&gl->base);
    return NULL;
}

static void wayland_update_gl_drawable(HWND hwnd, struct wayland_gl_drawable *new)
{
    struct wayland_gl_drawable *old;

    pthread_mutex_lock(&gl_object_mutex);

    if ((old = find_drawable(hwnd, 0))) list_remove(&old->entry);
    if (new) list_add_head(&gl_drawables, &new->entry);

    pthread_mutex_unlock(&gl_object_mutex);

    if (old) opengl_drawable_release(&old->base);
}

static void wayland_gl_drawable_sync_size(struct wayland_gl_drawable *gl)
{
    int client_width, client_height;
    RECT client_rect = {0};

    if (InterlockedCompareExchange(&gl->resized, FALSE, TRUE))
    {
        NtUserGetClientRect(gl->base.hwnd, &client_rect, NtUserGetDpiForWindow(gl->base.hwnd));
        client_width = client_rect.right - client_rect.left;
        client_height = client_rect.bottom - client_rect.top;
        if (client_width == 0 || client_height == 0) client_width = client_height = 1;

        wl_egl_window_resize(gl->wl_egl_window, client_width, client_height, 0, 0);
    }
}

static BOOL wayland_make_current(struct opengl_drawable *draw_base, struct opengl_drawable *read_base, void *private)
{
    struct wayland_gl_drawable *draw = impl_from_opengl_drawable(draw_base), *read = impl_from_opengl_drawable(read_base);
    BOOL ret;
    struct wayland_context *ctx = private;
    struct wayland_gl_drawable *old_draw = NULL, *old_read = NULL;

    TRACE("draw %s, read %s, context %p\n", debugstr_opengl_drawable(draw_base), debugstr_opengl_drawable(read_base), private);

    if (!private)
    {
        funcs->p_eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return TRUE;
    }

    /* Since making an EGL surface current may latch the native size,
     * perform any pending resizes before calling it. */
    if (draw) wayland_gl_drawable_sync_size(draw);

    pthread_mutex_lock(&gl_object_mutex);

    ret = funcs->p_eglMakeCurrent(egl->display,
                                  draw ? draw->surface : EGL_NO_SURFACE,
                                  read ? read->surface : EGL_NO_SURFACE,
                                  ctx->context);
    if (ret)
    {
        old_draw = ctx->draw;
        old_read = ctx->read;
        if ((ctx->draw = draw)) opengl_drawable_add_ref(&draw->base);
        if ((ctx->read = read)) opengl_drawable_add_ref(&read->base);
    }

    pthread_mutex_unlock(&gl_object_mutex);

    if (old_draw) opengl_drawable_release(&old_draw->base);
    if (old_read) opengl_drawable_release(&old_read->base);

    return ret;
}

static BOOL wayland_opengl_surface_create(HWND hwnd, HDC hdc, int format, struct opengl_drawable **drawable)
{
    struct opengl_drawable *previous;
    struct wayland_gl_drawable *gl;
    RECT rect;

    if ((previous = *drawable) && previous->format == format) return TRUE;

    NtUserGetClientRect(hwnd, &rect, NtUserGetDpiForWindow(hwnd));
    if (rect.right == rect.left) rect.right = rect.left + 1;
    if (rect.bottom == rect.top) rect.bottom = rect.top + 1;

    if (!(gl = wayland_gl_drawable_create(hwnd, 0, format, rect.right - rect.left, rect.bottom - rect.top))) return FALSE;
    wayland_update_gl_drawable(hwnd, gl);

    if (previous) opengl_drawable_release( previous );
    opengl_drawable_add_ref( (*drawable = &gl->base) );
    return TRUE;
}

static BOOL wayland_context_create(int format, void *share_private, const int *attribs, void **private)
{
    struct wayland_context *share = share_private, *ctx;
    EGLint egl_attribs[16], *attribs_end = egl_attribs;

    TRACE("format=%d share=%p attribs=%p\n", format, share, attribs);

    for (; attribs && attribs[0] != 0; attribs += 2)
    {
        EGLint name;

        TRACE("%#x %#x\n", attribs[0], attribs[1]);

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
                ERR("OpenGL ES contexts are not supported\n");
                return FALSE;
            }
            name = EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR;
            break;
        default:
            name = EGL_NONE;
            FIXME("Unhandled attributes: %#x %#x\n", attribs[0], attribs[1]);
        }

        if (name != EGL_NONE)
        {
            EGLint *dst = egl_attribs;
            /* Check if we have already set the same attribute and replace it. */
            for (; dst != attribs_end && *dst != name; dst += 2) continue;
            /* Our context attribute array should have enough space for all the
             * attributes we support (we merge repetitions), plus EGL_NONE. */
            assert(dst - egl_attribs <= ARRAY_SIZE(egl_attribs) - 3);
            dst[0] = name;
            dst[1] = attribs[1];
            if (dst == attribs_end) attribs_end += 2;
        }
    }
    *attribs_end = EGL_NONE;

    if (!(ctx = calloc(1, sizeof(*ctx))))
    {
        ERR("Failed to allocate memory for GL context\n");
        return FALSE;
    }

    /* For now only OpenGL is supported. It's enough to set the API only for
     * context creation, since:
     * 1. the default API is EGL_OPENGL_ES_API
     * 2. the EGL specification says in section 3.7:
     *    > EGL_OPENGL_API and EGL_OPENGL_ES_API are interchangeable for all
     *    > purposes except eglCreateContext.
     */
    funcs->p_eglBindAPI(EGL_OPENGL_API);
    ctx->context = funcs->p_eglCreateContext(egl->display, EGL_NO_CONFIG_KHR,
                                             share ? share->context : EGL_NO_CONTEXT,
                                             attribs ? egl_attribs : NULL);

    pthread_mutex_lock(&gl_object_mutex);
    list_add_head(&gl_contexts, &ctx->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    TRACE("ctx=%p egl_context=%p\n", ctx, ctx->context);

    *private = ctx;
    return TRUE;
}

static BOOL wayland_context_destroy(void *private)
{
    struct wayland_context *ctx = private;

    pthread_mutex_lock(&gl_object_mutex);
    list_remove(&ctx->entry);
    pthread_mutex_unlock(&gl_object_mutex);
    funcs->p_eglDestroyContext(egl->display, ctx->context);
    if (ctx->draw) opengl_drawable_release(&ctx->draw->base);
    if (ctx->read) opengl_drawable_release(&ctx->read->base);
    free(ctx);
    return TRUE;
}

static EGLenum wayland_init_egl_platform(const struct egl_platform *platform, EGLNativeDisplayType *platform_display)
{
    egl = platform;
    *platform_display = process_wayland.wl_display;
    return EGL_PLATFORM_WAYLAND_KHR;
}

static void wayland_drawable_flush(struct opengl_drawable *base, UINT flags)
{
    struct wayland_gl_drawable *gl = impl_from_opengl_drawable(base);

    TRACE("drawable %s, flags %#x\n", debugstr_opengl_drawable(base), flags);

    if (flags & GL_FLUSH_INTERVAL) funcs->p_eglSwapInterval(egl->display, abs(base->interval));

    /* Since context_flush is called from operations that may latch the native size,
     * perform any pending resizes before calling them. */
    wayland_gl_drawable_sync_size(gl);
}

static BOOL wayland_drawable_swap(struct opengl_drawable *base)
{
    HWND hwnd = base->hwnd, toplevel = NtUserGetAncestor(hwnd, GA_ROOT);
    struct wayland_gl_drawable *gl = impl_from_opengl_drawable(base);

    ensure_window_surface_contents(toplevel);
    set_client_surface(hwnd, gl->client);

    /* Although all the EGL surfaces we create are double-buffered, we want to
     * use some as single-buffered, so avoid swapping those. */
    if (gl->double_buffered) funcs->p_eglSwapBuffers(egl->display, gl->surface);
    wayland_gl_drawable_sync_size(gl);

    return TRUE;
}

static BOOL wayland_pbuffer_create(HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                   GLint max_level, GLsizei *width, GLsizei *height, struct opengl_drawable **surface)
{
    struct wayland_gl_drawable *drawable;

    TRACE("hdc %p, format %d, largest %u, texture_format %#x, texture_target %#x, max_level %#x, width %d, height %d, private %p\n",
          hdc, format, largest, texture_format, texture_target, max_level, *width, *height, surface);

    /* Use an unmapped wayland surface as our offscreen "pbuffer" surface. */
    if (!(drawable = wayland_gl_drawable_create(0, hdc, format, *width, *height))) return FALSE;

    pthread_mutex_lock(&gl_object_mutex);
    list_add_head(&gl_drawables, &drawable->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    *surface = &drawable->base;
    return TRUE;
}

static BOOL wayland_pbuffer_updated(HDC hdc, struct opengl_drawable *base, GLenum cube_face, GLint mipmap_level)
{
    return GL_TRUE;
}

static UINT wayland_pbuffer_bind(HDC hdc, struct opengl_drawable *base, GLenum buffer)
{
    return -1; /* use default implementation */
}

static struct opengl_driver_funcs wayland_driver_funcs =
{
    .p_init_egl_platform = wayland_init_egl_platform,
    .p_surface_create = wayland_opengl_surface_create,
    .p_context_create = wayland_context_create,
    .p_context_destroy = wayland_context_destroy,
    .p_make_current = wayland_make_current,
    .p_pbuffer_create = wayland_pbuffer_create,
    .p_pbuffer_updated = wayland_pbuffer_updated,
    .p_pbuffer_bind = wayland_pbuffer_bind,
};

static const struct opengl_drawable_funcs wayland_drawable_funcs =
{
    .destroy = wayland_drawable_destroy,
    .flush = wayland_drawable_flush,
    .swap = wayland_drawable_swap,
};

/**********************************************************************
 *           WAYLAND_OpenGLInit
 */
UINT WAYLAND_OpenGLInit(UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs)
{
    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR("Version mismatch, opengl32 wants %u but driver has %u\n",
            version, WINE_OPENGL_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

    if (!opengl_funcs->egl_handle) return STATUS_NOT_SUPPORTED;
    funcs = opengl_funcs;

    wayland_driver_funcs.p_get_proc_address = (*driver_funcs)->p_get_proc_address;
    wayland_driver_funcs.p_init_pixel_formats = (*driver_funcs)->p_init_pixel_formats;
    wayland_driver_funcs.p_describe_pixel_format = (*driver_funcs)->p_describe_pixel_format;
    wayland_driver_funcs.p_init_wgl_extensions = (*driver_funcs)->p_init_wgl_extensions;

    *driver_funcs = &wayland_driver_funcs;
    return STATUS_SUCCESS;
}

/**********************************************************************
 *           wayland_destroy_gl_drawable
 */
void wayland_destroy_gl_drawable(HWND hwnd)
{
    wayland_update_gl_drawable(hwnd, NULL);
}

/**********************************************************************
 *           wayland_resize_gl_drawable
 */
void wayland_resize_gl_drawable(HWND hwnd)
{
    struct wayland_gl_drawable *gl;

    if (!(gl = wayland_gl_drawable_get(hwnd, 0))) return;
    /* wl_egl_window_resize is not thread safe, so we just mark the
     * drawable as resized and perform the resize in the proper thread. */
    InterlockedExchange(&gl->resized, TRUE);
    opengl_drawable_release(&gl->base);
}

#else /* No GL */

UINT WAYLAND_OpenGLInit(UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs)
{
    return STATUS_NOT_IMPLEMENTED;
}

void wayland_destroy_gl_drawable(HWND hwnd)
{
}

void wayland_resize_gl_drawable(HWND hwnd)
{
}

#endif
