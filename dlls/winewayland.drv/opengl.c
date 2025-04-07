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

#if defined(SONAME_LIBEGL) && defined(HAVE_LIBWAYLAND_EGL)

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wine/opengl_driver.h"

/* Support building on systems with older EGL headers, which may not include
 * the EGL_EXT_present_opaque extension. */
#ifndef EGL_PRESENT_OPAQUE_EXT
#define EGL_PRESENT_OPAQUE_EXT 0x31DF
#endif

static void *egl_handle;
static struct opengl_funcs opengl_funcs;
static EGLDisplay egl_display;
static char wgl_extensions[4096];
static EGLConfig *egl_configs;
static int num_egl_configs;
static BOOL has_egl_ext_pixel_format_float;

#define DECL_FUNCPTR(f) static typeof(f) * p_##f
DECL_FUNCPTR(eglBindAPI);
DECL_FUNCPTR(eglChooseConfig);
DECL_FUNCPTR(eglCreateContext);
DECL_FUNCPTR(eglCreateWindowSurface);
DECL_FUNCPTR(eglDestroyContext);
DECL_FUNCPTR(eglDestroySurface);
DECL_FUNCPTR(eglGetConfigAttrib);
DECL_FUNCPTR(eglGetCurrentContext);
DECL_FUNCPTR(eglGetCurrentSurface);
DECL_FUNCPTR(eglGetError);
DECL_FUNCPTR(eglGetPlatformDisplay);
DECL_FUNCPTR(eglGetProcAddress);
DECL_FUNCPTR(eglInitialize);
DECL_FUNCPTR(eglMakeCurrent);
DECL_FUNCPTR(eglQueryString);
DECL_FUNCPTR(eglSwapBuffers);
DECL_FUNCPTR(eglSwapInterval);
#undef DECL_FUNCPTR

#define DECL_FUNCPTR(f) static PFN_##f p_##f
DECL_FUNCPTR(glClear);
#undef DECL_FUNCPTR

static pthread_mutex_t gl_object_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct list gl_drawables = LIST_INIT(gl_drawables);
static struct list gl_contexts = LIST_INIT(gl_contexts);

struct wayland_gl_drawable
{
    struct list entry;
    LONG ref;
    HWND hwnd;
    HDC hdc;
    struct wayland_client_surface *client;
    struct wl_egl_window *wl_egl_window;
    EGLSurface surface;
    LONG resized;
    int swap_interval;
    BOOL double_buffered;
};

struct wayland_context
{
    struct list entry;
    EGLConfig config;
    EGLContext context;
    struct wayland_gl_drawable *draw, *read, *new_draw, *new_read;
    EGLint attribs[16];
    BOOL has_been_current;
    BOOL sharing;
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
        if (hwnd && gl->hwnd == hwnd) return gl;
        if (hdc && gl->hdc == hdc) return gl;
    }
    return NULL;
}

static struct wayland_gl_drawable *wayland_gl_drawable_acquire(struct wayland_gl_drawable *gl)
{
    InterlockedIncrement(&gl->ref);
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

static void wayland_gl_drawable_release(struct wayland_gl_drawable *gl)
{
    if (InterlockedDecrement(&gl->ref)) return;
    if (gl->surface) p_eglDestroySurface(egl_display, gl->surface);
    if (gl->wl_egl_window) wl_egl_window_destroy(gl->wl_egl_window);
    if (gl->client)
    {
        HWND hwnd = wl_surface_get_user_data(gl->client->wl_surface);
        struct wayland_win_data *data = wayland_win_data_get(hwnd);

        if (wayland_client_surface_release(gl->client) && data)
            data->client_surface = NULL;

        if (data) wayland_win_data_release(data);
    }

    free(gl);
}

static inline BOOL is_onscreen_format(int format)
{
    return format > 0 && format <= num_egl_configs;
}

static inline EGLConfig egl_config_for_format(int format)
{
    assert(format > 0 && format <= 2 * num_egl_configs);
    if (format <= num_egl_configs) return egl_configs[format - 1];
    return egl_configs[format - num_egl_configs - 1];
}

static struct wayland_gl_drawable *wayland_gl_drawable_create(HWND hwnd, HDC hdc, int format, int width, int height)
{
    struct wayland_gl_drawable *gl;
    const EGLint attribs[] = {EGL_PRESENT_OPAQUE_EXT, EGL_TRUE, EGL_NONE};

    TRACE("hwnd=%p format=%d\n", hwnd, format);

    gl = calloc(1, sizeof(*gl));
    if (!gl) return NULL;

    gl->ref = 1;
    gl->hwnd = hwnd;
    gl->hdc = hdc;
    gl->swap_interval = 1;

    /* Get the client surface for the HWND. If don't have a wayland surface
     * (e.g., HWND_MESSAGE windows) just create a dummy surface to act as the
     * target render surface. */
    if (!(gl->client = get_client_surface(hwnd))) goto err;

    gl->wl_egl_window = wl_egl_window_create(gl->client->wl_surface, width, height);
    if (!gl->wl_egl_window)
    {
        ERR("Failed to create wl_egl_window\n");
        goto err;
    }

    gl->surface = p_eglCreateWindowSurface(egl_display, egl_config_for_format(format),
                                           gl->wl_egl_window, attribs);
    if (!gl->surface)
    {
        ERR("Failed to create EGL surface\n");
        goto err;
    }

    gl->double_buffered = is_onscreen_format(format);

    TRACE("hwnd=%p egl_surface=%p\n", gl->hwnd, gl->surface);

    return gl;

err:
    wayland_gl_drawable_release(gl);
    return NULL;
}

static void update_context_drawables(struct wayland_gl_drawable *new,
                                     struct wayland_gl_drawable *old)
{
    struct wayland_context *ctx;

    LIST_FOR_EACH_ENTRY(ctx, &gl_contexts, struct wayland_context, entry)
    {
        if (ctx->draw == old || ctx->new_draw == old) ctx->new_draw = new;
        if (ctx->read == old || ctx->new_read == old) ctx->new_read = new;
    }
}

static void wayland_update_gl_drawable(HWND hwnd, struct wayland_gl_drawable *new)
{
    struct wayland_gl_drawable *old;

    pthread_mutex_lock(&gl_object_mutex);

    if ((old = find_drawable(hwnd, 0))) list_remove(&old->entry);
    if (new) list_add_head(&gl_drawables, &new->entry);
    if (old && new) update_context_drawables(new, old);

    pthread_mutex_unlock(&gl_object_mutex);

    if (old) wayland_gl_drawable_release(old);
}

static void wayland_gl_drawable_sync_size(struct wayland_gl_drawable *gl)
{
    int client_width, client_height;
    RECT client_rect = {0};

    if (InterlockedCompareExchange(&gl->resized, FALSE, TRUE))
    {
        NtUserGetClientRect(gl->hwnd, &client_rect, NtUserGetDpiForWindow(gl->hwnd));
        client_width = client_rect.right - client_rect.left;
        client_height = client_rect.bottom - client_rect.top;
        if (client_width == 0 || client_height == 0) client_width = client_height = 1;

        wl_egl_window_resize(gl->wl_egl_window, client_width, client_height, 0, 0);
    }
}

static BOOL wayland_context_make_current(HDC draw_hdc, HDC read_hdc, void *private)
{
    BOOL ret;
    struct wayland_context *ctx = private;
    struct wayland_gl_drawable *draw, *read;
    struct wayland_gl_drawable *old_draw = NULL, *old_read = NULL;

    TRACE("draw_hdc=%p read_hdc=%p ctx=%p\n", draw_hdc, read_hdc, ctx);

    if (!private)
    {
        p_eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        NtCurrentTeb()->glReserved2 = NULL;
        return TRUE;
    }

    draw = wayland_gl_drawable_get(NtUserWindowFromDC(draw_hdc), draw_hdc);
    read = wayland_gl_drawable_get(NtUserWindowFromDC(read_hdc), read_hdc);

    TRACE("%p/%p context %p surface %p/%p\n",
          draw_hdc, read_hdc, ctx->context,
          draw ? draw->surface : EGL_NO_SURFACE,
          read ? read->surface : EGL_NO_SURFACE);

    /* Since making an EGL surface current may latch the native size,
     * perform any pending resizes before calling it. */
    if (draw) wayland_gl_drawable_sync_size(draw);

    pthread_mutex_lock(&gl_object_mutex);

    ret = p_eglMakeCurrent(egl_display,
                           draw ? draw->surface : EGL_NO_SURFACE,
                           read ? read->surface : EGL_NO_SURFACE,
                           ctx->context);
    if (ret)
    {
        old_draw = ctx->draw;
        old_read = ctx->read;
        ctx->draw = draw;
        ctx->read = read;
        ctx->new_draw = ctx->new_read = NULL;
        ctx->has_been_current = TRUE;
        NtCurrentTeb()->glReserved2 = ctx;
    }
    else
    {
        old_draw = draw;
        old_read = read;
    }

    pthread_mutex_unlock(&gl_object_mutex);

    if (old_draw) wayland_gl_drawable_release(old_draw);
    if (old_read) wayland_gl_drawable_release(old_read);

    return ret;
}

static BOOL wayland_context_populate_attribs(struct wayland_context *ctx, const int *wgl_attribs)
{
    EGLint *attribs_end = ctx->attribs;

    if (!wgl_attribs) goto out;

    for (; wgl_attribs[0] != 0; wgl_attribs += 2)
    {
        EGLint name;

        TRACE("%#x %#x\n", wgl_attribs[0], wgl_attribs[1]);

        /* Find the EGL attribute names corresponding to the WGL names.
         * For all of the attributes below, the values match between the two
         * systems, so we can use them directly. */
        switch (wgl_attribs[0])
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
            if (wgl_attribs[1] & WGL_CONTEXT_ES2_PROFILE_BIT_EXT)
            {
                ERR("OpenGL ES contexts are not supported\n");
                return FALSE;
            }
            name = EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR;
            break;
        default:
            name = EGL_NONE;
            FIXME("Unhandled attributes: %#x %#x\n", wgl_attribs[0], wgl_attribs[1]);
        }

        if (name != EGL_NONE)
        {
            EGLint *dst = ctx->attribs;
            /* Check if we have already set the same attribute and replace it. */
            for (; dst != attribs_end && *dst != name; dst += 2) continue;
            /* Our context attribute array should have enough space for all the
             * attributes we support (we merge repetitions), plus EGL_NONE. */
            assert(dst - ctx->attribs <= ARRAY_SIZE(ctx->attribs) - 3);
            dst[0] = name;
            dst[1] = wgl_attribs[1];
            if (dst == attribs_end) attribs_end += 2;
        }
    }

out:
    *attribs_end = EGL_NONE;
    return TRUE;
}


static void wayland_context_refresh(struct wayland_context *ctx)
{
    BOOL refresh = FALSE;
    struct wayland_gl_drawable *old_draw = NULL, *old_read = NULL;

    pthread_mutex_lock(&gl_object_mutex);

    if (ctx->new_draw)
    {
        old_draw = ctx->draw;
        ctx->draw = wayland_gl_drawable_acquire(ctx->new_draw);
        ctx->new_draw = NULL;
        refresh = TRUE;
    }
    if (ctx->new_read)
    {
        old_read = ctx->read;
        ctx->read = wayland_gl_drawable_acquire(ctx->new_read);
        ctx->new_read = NULL;
        refresh = TRUE;
    }
    if (refresh) p_eglMakeCurrent(egl_display, ctx->draw, ctx->read, ctx->context);

    pthread_mutex_unlock(&gl_object_mutex);

    if (old_draw) wayland_gl_drawable_release(old_draw);
    if (old_read) wayland_gl_drawable_release(old_read);
}

static BOOL wayland_set_pixel_format(HWND hwnd, int old_format, int new_format, BOOL internal)
{
    struct wayland_gl_drawable *gl;
    RECT rect;

    /* Even for internal pixel format fail setting it if the app has already set a
     * different pixel format. Let wined3d create a backup GL context instead.
     * Switching pixel format involves drawable recreation and is much more expensive
     * than blitting from backup context. */
    if (old_format) return old_format == new_format;

    NtUserGetClientRect(hwnd, &rect, NtUserGetDpiForWindow(hwnd));
    if (rect.right == rect.left) rect.right = rect.left + 1;
    if (rect.bottom == rect.top) rect.bottom = rect.top + 1;

    if (!(gl = wayland_gl_drawable_create(hwnd, 0, new_format, rect.right - rect.left, rect.bottom - rect.top))) return FALSE;
    wayland_update_gl_drawable(hwnd, gl);
    return TRUE;
}

static BOOL wayland_context_create(HDC hdc, int format, void *share_private, const int *attribs, void **private)
{
    struct wayland_context *share = share_private, *ctx;

    TRACE("hdc=%p format=%d share=%p attribs=%p\n", hdc, format, share, attribs);

    if (!(ctx = calloc(1, sizeof(*ctx))))
    {
        ERR("Failed to allocate memory for GL context\n");
        return FALSE;
    }

    if (!wayland_context_populate_attribs(ctx, attribs))
    {
        ctx->attribs[0] = EGL_NONE;
        return FALSE;
    }

    /* For now only OpenGL is supported. It's enough to set the API only for
     * context creation, since:
     * 1. the default API is EGL_OPENGL_ES_API
     * 2. the EGL specification says in section 3.7:
     *    > EGL_OPENGL_API and EGL_OPENGL_ES_API are interchangeable for all
     *    > purposes except eglCreateContext.
     */
    p_eglBindAPI(EGL_OPENGL_API);
    ctx->context = p_eglCreateContext(egl_display, EGL_NO_CONFIG_KHR,
                                      share ? share->context : EGL_NO_CONTEXT,
                                      ctx->attribs);

    pthread_mutex_lock(&gl_object_mutex);
    list_add_head(&gl_contexts, &ctx->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    TRACE("ctx=%p egl_context=%p\n", ctx, ctx->context);

    *private = ctx;
    return TRUE;
}

void wayland_glClear(GLbitfield mask)
{
    struct wayland_context *ctx = NtCurrentTeb()->glReserved2;
    /* Since glClear is one of the operations that may latch the native size,
     * perform any pending resizes before calling it. */
    if (ctx && ctx->draw) wayland_gl_drawable_sync_size(ctx->draw);
    p_glClear(mask);
}

static BOOL wayland_context_copy(void *src, void *dst, UINT mask)
{
    FIXME("%p -> %p mask %#x unsupported\n", src, dst, mask);
    return FALSE;
}

static BOOL wayland_context_destroy(void *private)
{
    struct wayland_context *ctx = private;

    pthread_mutex_lock(&gl_object_mutex);
    list_remove(&ctx->entry);
    pthread_mutex_unlock(&gl_object_mutex);
    p_eglDestroyContext(egl_display, ctx->context);
    if (ctx->draw) wayland_gl_drawable_release(ctx->draw);
    if (ctx->read) wayland_gl_drawable_release(ctx->read);
    free(ctx);
    return TRUE;
}

static void *wayland_get_proc_address(const char *name)
{
    return p_eglGetProcAddress(name);
}

static BOOL wayland_context_share(void *src_private, void *dst_private)
{
    struct wayland_context *orig = src_private, *dest = dst_private;
    struct wayland_context *keep, *clobber;

    TRACE("(%p, %p)\n", orig, dest);

    /* Sharing of display lists works differently in EGL and WGL. In case of EGL
     * it is done at context creation time but in case of WGL it is done using
     * wglShareLists. We create an EGL context in wglCreateContext /
     * wglCreateContextAttribsARB and when a program requests sharing we
     * recreate the destination or source context if it hasn't been made current
     * and it hasn't shared display lists before. */

    if (!dest->has_been_current && !dest->sharing)
    {
        keep = orig;
        clobber = dest;
    }
    else if (!orig->has_been_current && !orig->sharing)
    {
        keep = dest;
        clobber = orig;
    }
    else
    {
        ERR("Could not share display lists because both of the contexts have "
            "already been current or shared\n");
        return FALSE;
    }

    p_eglDestroyContext(egl_display, clobber->context);
    clobber->context = p_eglCreateContext(egl_display, EGL_NO_CONFIG_KHR,
                                          keep->context, clobber->attribs);
    TRACE("re-created context (%p) for Wine context %p (%p) "
          "sharing lists with ctx %p (%p)\n",
          clobber->context, clobber, clobber->config,
          keep->context, keep->config);

    orig->sharing = TRUE;
    dest->sharing = TRUE;
    return TRUE;
}

static BOOL wayland_context_flush( void *private, HWND hwnd, HDC hdc, int interval, BOOL finish )
{
    return FALSE;
}

static BOOL wayland_swap_buffers(void *private, HWND hwnd, HDC hdc, int interval)
{
    struct wayland_context *ctx = private;
    HWND toplevel = NtUserGetAncestor(hwnd, GA_ROOT);
    struct wayland_gl_drawable *gl;

    if (!(gl = wayland_gl_drawable_get(NtUserWindowFromDC(hdc), hdc))) return FALSE;

    if (interval < 0) interval = -interval;
    if (gl->swap_interval != interval)
    {
        p_eglSwapInterval(egl_display, interval);
        gl->swap_interval = interval;
    }

    if (ctx) wayland_context_refresh(ctx);
    ensure_window_surface_contents(toplevel);
    /* Although all the EGL surfaces we create are double-buffered, we want to
     * use some as single-buffered, so avoid swapping those. */
    if (gl->double_buffered) p_eglSwapBuffers(egl_display, gl->surface);
    wayland_gl_drawable_sync_size(gl);

    wayland_gl_drawable_release(gl);

    return TRUE;
}

static BOOL wayland_pbuffer_create(HDC hdc, int format, BOOL largest, GLenum texture_format, GLenum texture_target,
                                   GLint max_level, GLsizei *width, GLsizei *height, void **private)
{
    struct wayland_gl_drawable *drawable;

    TRACE("hdc %p, format %d, largest %u, texture_format %#x, texture_target %#x, max_level %#x, width %d, height %d, private %p\n",
          hdc, format, largest, texture_format, texture_target, max_level, *width, *height, private);

    /* Use an unmapped wayland surface as our offscreen "pbuffer" surface. */
    if (!(drawable = wayland_gl_drawable_create(0, hdc, format, *width, *height))) return FALSE;

    pthread_mutex_lock(&gl_object_mutex);
    list_add_head(&gl_drawables, &drawable->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    *private = drawable;
    return TRUE;
}

static BOOL wayland_pbuffer_destroy(HDC hdc, void *private)
{
    struct wayland_gl_drawable *drawable = private;

    TRACE("hdc %p, private %p\n", hdc, private);

    pthread_mutex_lock(&gl_object_mutex);
    list_remove(&drawable->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    wayland_gl_drawable_release(drawable);

    return GL_TRUE;
}

static BOOL wayland_pbuffer_updated(HDC hdc, void *private, GLenum cube_face, GLint mipmap_level)
{
    TRACE("hdc %p, private %p, cube_face %#x, mipmap_level %d\n", hdc, private, cube_face, mipmap_level);
    return GL_TRUE;
}

static UINT wayland_pbuffer_bind(HDC hdc, void *private, GLenum buffer)
{
    TRACE("hdc %p, private %p, buffer %#x\n", hdc, private, buffer);
    return -1; /* use default implementation */
}

static BOOL describe_pixel_format(EGLConfig config, struct wgl_pixel_format *fmt, BOOL pbuffer_single)
{
    EGLint value, surface_type;
    PIXELFORMATDESCRIPTOR *pfd = &fmt->pfd;

    /* If we can't get basic information, there is no point continuing */
    if (!p_eglGetConfigAttrib(egl_display, config, EGL_SURFACE_TYPE, &surface_type)) return FALSE;

    memset(fmt, 0, sizeof(*fmt));
    pfd->nSize = sizeof(*pfd);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION;
    if (!pbuffer_single)
    {
        pfd->dwFlags |= PFD_DOUBLEBUFFER;
        if (surface_type & EGL_WINDOW_BIT) pfd->dwFlags |= PFD_DRAW_TO_WINDOW;
    }
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->iLayerType = PFD_MAIN_PLANE;

#define SET_ATTRIB(field, attrib) \
    value = 0; \
    p_eglGetConfigAttrib(egl_display, config, attrib, &value); \
    pfd->field = value;
#define SET_ATTRIB_ARB(field, attrib) \
    if (!p_eglGetConfigAttrib(egl_display, config, attrib, &value)) value = -1; \
    fmt->field = value;

    /* Although the documentation describes cColorBits as excluding alpha, real
     * drivers tend to return the full pixel size, so do the same. */
    SET_ATTRIB(cColorBits, EGL_BUFFER_SIZE);
    SET_ATTRIB(cRedBits, EGL_RED_SIZE);
    SET_ATTRIB(cGreenBits, EGL_GREEN_SIZE);
    SET_ATTRIB(cBlueBits, EGL_BLUE_SIZE);
    SET_ATTRIB(cAlphaBits, EGL_ALPHA_SIZE);
    /* Although we don't get information from EGL about the component shifts
     * or the native format, the 0xARGB order is the most common. */
    pfd->cBlueShift = 0;
    pfd->cGreenShift = pfd->cBlueBits;
    pfd->cRedShift = pfd->cGreenBits + pfd->cBlueBits;
    if (pfd->cAlphaBits)
        pfd->cAlphaShift = pfd->cRedBits + pfd->cGreenBits + pfd->cBlueBits;
    else
        pfd->cAlphaShift = 0;

    SET_ATTRIB(cDepthBits, EGL_DEPTH_SIZE);
    SET_ATTRIB(cStencilBits, EGL_STENCIL_SIZE);

    fmt->swap_method = WGL_SWAP_UNDEFINED_ARB;

    if (p_eglGetConfigAttrib(egl_display, config, EGL_TRANSPARENT_TYPE, &value))
    {
        switch (value)
        {
        case EGL_TRANSPARENT_RGB: fmt->transparent = GL_TRUE; break;
        case EGL_NONE: fmt->transparent = GL_FALSE; break;
        default:
            ERR("unexpected transparency type 0x%x\n", value);
            fmt->transparent = -1;
            break;
        }
    }
    else fmt->transparent = -1;

    if (!has_egl_ext_pixel_format_float) fmt->pixel_type = WGL_TYPE_RGBA_ARB;
    else if (p_eglGetConfigAttrib(egl_display, config, EGL_COLOR_COMPONENT_TYPE_EXT, &value))
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
            ERR("unexpected color component type 0x%x\n", value);
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

    if (p_eglGetConfigAttrib(egl_display, config, EGL_TRANSPARENT_RED_VALUE, &value))
    {
        fmt->transparent_red_value_valid = GL_TRUE;
        fmt->transparent_red_value = value;
    }
    if (p_eglGetConfigAttrib(egl_display, config, EGL_TRANSPARENT_GREEN_VALUE, &value))
    {
        fmt->transparent_green_value_valid = GL_TRUE;
        fmt->transparent_green_value = value;
    }
    if (p_eglGetConfigAttrib(egl_display, config, EGL_TRANSPARENT_BLUE_VALUE, &value))
    {
        fmt->transparent_blue_value_valid = GL_TRUE;
        fmt->transparent_blue_value = value;
    }
    fmt->transparent_alpha_value_valid = GL_TRUE;
    fmt->transparent_alpha_value = 0;
    fmt->transparent_index_value_valid = GL_TRUE;
    fmt->transparent_index_value = 0;

    SET_ATTRIB_ARB(sample_buffers, EGL_SAMPLE_BUFFERS);
    SET_ATTRIB_ARB(samples, EGL_SAMPLES);

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

static BOOL wayland_describe_pixel_format(int format, struct wgl_pixel_format *desc)
{
    if (format <= 0)
        return FALSE;
    if (format <= num_egl_configs)
        return describe_pixel_format(egl_configs[format - 1], desc, FALSE);
    /* Add single-buffered pbuffer capable configs. */
    if (format <= 2 * num_egl_configs)
        return describe_pixel_format(egl_configs[format - 1 - num_egl_configs], desc, TRUE);
    return FALSE;
}

static BOOL has_extension(const char *list, const char *ext)
{
    size_t len = strlen(ext);
    const char *cur = list;

    while (cur && (cur = strstr(cur, ext)))
    {
        if ((!cur[len] || cur[len] == ' ') && (cur == list || cur[-1] == ' '))
            return TRUE;
        cur = strchr(cur, ' ');
    }

    return FALSE;
}

static void register_extension(const char *ext)
{
    if (wgl_extensions[0]) strcat(wgl_extensions, " ");
    strcat(wgl_extensions, ext);
    TRACE("%s\n", ext);
}

static BOOL init_opengl_funcs(void)
{
#define USE_GL_FUNC(func) \
        if (!(opengl_funcs.p_##func = (void *)p_eglGetProcAddress(#func))) \
        { \
            ERR("%s not found, disabling OpenGL.\n", #func); \
            return FALSE; \
        }
    ALL_GL_FUNCS
#undef USE_GL_FUNC

    p_glClear = opengl_funcs.p_glClear;
    opengl_funcs.p_glClear = wayland_glClear;

    return TRUE;
}

static const char *wayland_init_wgl_extensions(void)
{
    if (has_egl_ext_pixel_format_float)
    {
        register_extension("WGL_ARB_pixel_format_float");
        register_extension("WGL_ATI_pixel_format_float");
    }

    return wgl_extensions;
}

static UINT wayland_init_pixel_formats(UINT *onscreen_count)
{
    EGLint i;
    const EGLint attribs[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    p_eglChooseConfig(egl_display, attribs, NULL, 0, &num_egl_configs);
    if (!(egl_configs = malloc(num_egl_configs * sizeof(*egl_configs))))
    {
        ERR("Failed to allocate memory for EGL configs\n");
        return 0;
    }
    if (!p_eglChooseConfig(egl_display, attribs, egl_configs, num_egl_configs,
                           &num_egl_configs) ||
        !num_egl_configs)
    {
        free(egl_configs);
        egl_configs = NULL;
        num_egl_configs = 0;
        ERR("Failed to get any configs from eglChooseConfig\n");
        return 0;
    }

    if (TRACE_ON(waylanddrv))
    {
        for (i = 0; i < num_egl_configs; i++)
        {
            EGLint id, type, visual_id, native, render, color, r, g, b, a, d, s;
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_NATIVE_VISUAL_ID, &visual_id);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_SURFACE_TYPE, &type);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_RENDERABLE_TYPE, &render);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_CONFIG_ID, &id);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_NATIVE_RENDERABLE, &native);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_COLOR_BUFFER_TYPE, &color);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_RED_SIZE, &r);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_GREEN_SIZE, &g);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_BLUE_SIZE, &b);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_ALPHA_SIZE, &a);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_DEPTH_SIZE, &d);
            p_eglGetConfigAttrib(egl_display, egl_configs[i], EGL_STENCIL_SIZE, &s);
            TRACE("%u: config %d id %d type %x visual %d native %d render %x "
                  "colortype %d rgba %d,%d,%d,%d depth %u stencil %d\n",
                  num_egl_configs, i, id, type, visual_id, native, render,
                  color, r, g, b, a, d, s);
        }
    }

    *onscreen_count = num_egl_configs;
    return 2 * num_egl_configs;
}

static const struct opengl_driver_funcs wayland_driver_funcs =
{
    .p_get_proc_address = wayland_get_proc_address,
    .p_init_pixel_formats = wayland_init_pixel_formats,
    .p_describe_pixel_format = wayland_describe_pixel_format,
    .p_init_wgl_extensions = wayland_init_wgl_extensions,
    .p_set_pixel_format = wayland_set_pixel_format,
    .p_swap_buffers = wayland_swap_buffers,
    .p_context_create = wayland_context_create,
    .p_context_destroy = wayland_context_destroy,
    .p_context_copy = wayland_context_copy,
    .p_context_share = wayland_context_share,
    .p_context_flush = wayland_context_flush,
    .p_context_make_current = wayland_context_make_current,
    .p_pbuffer_create = wayland_pbuffer_create,
    .p_pbuffer_destroy = wayland_pbuffer_destroy,
    .p_pbuffer_updated = wayland_pbuffer_updated,
    .p_pbuffer_bind = wayland_pbuffer_bind,
};

/**********************************************************************
 *           WAYLAND_OpenGLInit
 */
UINT WAYLAND_OpenGLInit(UINT version, struct opengl_funcs **funcs, const struct opengl_driver_funcs **driver_funcs)
{
    EGLint egl_version[2];
    const char *egl_client_exts, *egl_exts;

    if (version != WINE_OPENGL_DRIVER_VERSION)
    {
        ERR("Version mismatch, opengl32 wants %u but driver has %u\n",
            version, WINE_OPENGL_DRIVER_VERSION);
        return STATUS_INVALID_PARAMETER;
    }

    if (!(egl_handle = dlopen(SONAME_LIBEGL, RTLD_NOW|RTLD_GLOBAL)))
    {
        ERR("Failed to load %s: %s\n", SONAME_LIBEGL, dlerror());
        return STATUS_NOT_SUPPORTED;
    }

#define LOAD_FUNCPTR_DLSYM(func) \
    do { \
        if (!(p_##func = dlsym(egl_handle, #func))) \
            { ERR("Failed to load symbol %s\n", #func); goto err; } \
    } while(0)
    LOAD_FUNCPTR_DLSYM(eglGetProcAddress);
    LOAD_FUNCPTR_DLSYM(eglQueryString);
#undef LOAD_FUNCPTR_DLSYM

    egl_client_exts = p_eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

#define REQUIRE_CLIENT_EXT(ext) \
    do { \
        if (!has_extension(egl_client_exts, #ext)) \
            { ERR("Failed to find required extension %s\n", #ext); goto err; } \
    } while(0)
    REQUIRE_CLIENT_EXT(EGL_KHR_client_get_all_proc_addresses);
    REQUIRE_CLIENT_EXT(EGL_KHR_platform_wayland);
#undef REQUIRE_CLIENT_EXT

#define LOAD_FUNCPTR_EGL(func) \
    do { \
        if (!(p_##func = (void *)p_eglGetProcAddress(#func))) \
            { ERR("Failed to load symbol %s\n", #func); goto err; } \
    } while(0)
    LOAD_FUNCPTR_EGL(eglBindAPI);
    LOAD_FUNCPTR_EGL(eglChooseConfig);
    LOAD_FUNCPTR_EGL(eglCreateContext);
    LOAD_FUNCPTR_EGL(eglCreateWindowSurface);
    LOAD_FUNCPTR_EGL(eglDestroyContext);
    LOAD_FUNCPTR_EGL(eglDestroySurface);
    LOAD_FUNCPTR_EGL(eglGetConfigAttrib);
    LOAD_FUNCPTR_EGL(eglGetCurrentContext);
    LOAD_FUNCPTR_EGL(eglGetCurrentSurface);
    LOAD_FUNCPTR_EGL(eglGetError);
    LOAD_FUNCPTR_EGL(eglGetPlatformDisplay);
    LOAD_FUNCPTR_EGL(eglInitialize);
    LOAD_FUNCPTR_EGL(eglMakeCurrent);
    LOAD_FUNCPTR_EGL(eglSwapBuffers);
    LOAD_FUNCPTR_EGL(eglSwapInterval);
#undef LOAD_FUNCPTR_EGL

    egl_display = p_eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR,
                                          process_wayland.wl_display,
                                          NULL);
    if (egl_display == EGL_NO_DISPLAY)
    {
        ERR("Failed to get EGLDisplay\n");
        goto err;
    }
    if (!p_eglInitialize(egl_display, &egl_version[0], &egl_version[1]))
    {
        ERR("Failed to initialized EGLDisplay with error %d\n", p_eglGetError());
        goto err;
    }
    TRACE("EGL version %u.%u\n", egl_version[0], egl_version[1]);

    egl_exts = p_eglQueryString(egl_display, EGL_EXTENSIONS);
#define REQUIRE_EXT(ext) \
    do { \
        if (!has_extension(egl_exts, #ext)) \
            { ERR("Failed to find required extension %s\n", #ext); goto err; } \
    } while(0)
    REQUIRE_EXT(EGL_KHR_create_context);
    REQUIRE_EXT(EGL_KHR_create_context_no_error);
    REQUIRE_EXT(EGL_KHR_no_config_context);
    REQUIRE_EXT(EGL_EXT_present_opaque);
#undef REQUIRE_EXT

    has_egl_ext_pixel_format_float = has_extension(egl_exts, "EGL_EXT_pixel_format_float");

    if (!init_opengl_funcs()) goto err;
    *funcs = &opengl_funcs;
    *driver_funcs = &wayland_driver_funcs;
    return STATUS_SUCCESS;

err:
    dlclose(egl_handle);
    egl_handle = NULL;
    return STATUS_NOT_SUPPORTED;
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
    wayland_gl_drawable_release(gl);
}

#else /* No GL */

UINT WAYLAND_OpenGLInit(UINT version, struct opengl_funcs **funcs, const struct opengl_driver_funcs **driver_funcs)
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
