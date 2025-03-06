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
static struct list gl_pbuffer_dcs = LIST_INIT(gl_pbuffer_dcs);
static struct list gl_pbuffers = LIST_INIT(gl_pbuffers);

struct wayland_gl_drawable
{
    struct list entry;
    LONG ref;
    HWND hwnd;
    struct wayland_client_surface *client;
    struct wl_egl_window *wl_egl_window;
    EGLSurface surface;
    LONG resized;
    int swap_interval;
    BOOL double_buffered;
};

struct wgl_context
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
    struct wayland_pbuffer_dc *pb;
    if (hwnd)
    {
        LIST_FOR_EACH_ENTRY(gl, &gl_drawables, struct wayland_gl_drawable, entry)
            if (gl->hwnd == hwnd) return gl;
    }
    if (hdc)
    {
        LIST_FOR_EACH_ENTRY(pb, &gl_pbuffer_dcs, struct wayland_pbuffer_dc, entry)
            if (pb->hdc == hdc) return pb->gl;
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

static struct wayland_gl_drawable *wayland_gl_drawable_create(HWND hwnd, int format)
{
    struct wayland_gl_drawable *gl;
    int client_width, client_height;
    RECT client_rect = {0};
    const EGLint attribs[] = {EGL_PRESENT_OPAQUE_EXT, EGL_TRUE, EGL_NONE};

    TRACE("hwnd=%p format=%d\n", hwnd, format);

    gl = calloc(1, sizeof(*gl));
    if (!gl) return NULL;

    gl->ref = 1;
    gl->hwnd = hwnd;
    gl->swap_interval = 1;

    NtUserGetClientRect(gl->hwnd, &client_rect, NtUserGetDpiForWindow(gl->hwnd));
    client_width = client_rect.right - client_rect.left;
    client_height = client_rect.bottom - client_rect.top;
    if (client_width == 0 || client_height == 0) client_width = client_height = 1;

    /* Get the client surface for the HWND. If don't have a wayland surface
     * (e.g., HWND_MESSAGE windows) just create a dummy surface to act as the
     * target render surface. */
    if (!(gl->client = get_client_surface(hwnd))) goto err;

    gl->wl_egl_window = wl_egl_window_create(gl->client->wl_surface,
                                             client_width, client_height);
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
    struct wgl_context *ctx;

    LIST_FOR_EACH_ENTRY(ctx, &gl_contexts, struct wgl_context, entry)
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
    if (old && new)
    {
        update_context_drawables(new, old);
        new->swap_interval = old->swap_interval;
    }

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

static BOOL wgl_context_make_current(struct wgl_context *ctx, HDC draw_hdc, HDC read_hdc)
{
    BOOL ret;
    struct wayland_gl_drawable *draw, *read;
    struct wayland_gl_drawable *old_draw = NULL, *old_read = NULL;

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
        NtCurrentTeb()->glContext = ctx;
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

static BOOL wgl_context_populate_attribs(struct wgl_context *ctx, const int *wgl_attribs)
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


static void wgl_context_refresh(struct wgl_context *ctx)
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
    if (refresh)
    {
        p_eglMakeCurrent(egl_display, ctx->draw, ctx->read, ctx->context);
        if (ctx->draw) p_eglSwapInterval(egl_display, ctx->draw->swap_interval);
    }

    pthread_mutex_unlock(&gl_object_mutex);

    if (old_draw) wayland_gl_drawable_release(old_draw);
    if (old_read) wayland_gl_drawable_release(old_read);
}

static BOOL wayland_set_pixel_format(HWND hwnd, int old_format, int new_format, BOOL internal)
{
    struct wayland_gl_drawable *gl;

    /* Even for internal pixel format fail setting it if the app has already set a
     * different pixel format. Let wined3d create a backup GL context instead.
     * Switching pixel format involves drawable recreation and is much more expensive
     * than blitting from backup context. */
    if (old_format) return old_format == new_format;

    if (!(gl = wayland_gl_drawable_create(hwnd, new_format))) return FALSE;
    wayland_update_gl_drawable(hwnd, gl);
    return TRUE;
}

static struct wgl_context *create_context(HDC hdc, struct wgl_context *share,
                                          const int *attribs)
{
    struct wayland_gl_drawable *gl;
    struct wgl_context *ctx;

    if (!(gl = wayland_gl_drawable_get(NtUserWindowFromDC(hdc), hdc))) return NULL;

    if (!(ctx = calloc(1, sizeof(*ctx))))
    {
        ERR("Failed to allocate memory for GL context\n");
        goto out;
    }

    if (!wgl_context_populate_attribs(ctx, attribs))
    {
        ctx->attribs[0] = EGL_NONE;
        goto out;
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

out:
    wayland_gl_drawable_release(gl);
    return ctx;
}

static struct wayland_gl_drawable *clear_pbuffer_dc(HDC hdc)
{
    struct wayland_pbuffer_dc *pd;
    struct wayland_gl_drawable *gl;

    LIST_FOR_EACH_ENTRY(pd, &gl_pbuffer_dcs, struct wayland_pbuffer_dc, entry)
    {
        if (pd->hdc == hdc)
        {
            list_remove(&pd->entry);
            gl = pd->gl;
            free(pd);
            return gl;
        }
    }

    return NULL;
}

static BOOL set_pbuffer_dc(struct wgl_pbuffer *pbuffer, HDC hdc)
{
    struct wayland_pbuffer_dc *pd;

    if (!(pd = calloc(1, sizeof(*pd))))
    {
        ERR("Failed to allocate memory for pbuffer HDC mapping\n");
        return FALSE;
    }
    pd->hdc = hdc;
    pd->gl = wayland_gl_drawable_acquire(pbuffer->gl);
    list_add_head(&gl_pbuffer_dcs, &pd->entry);

    return TRUE;
}

void wayland_glClear(GLbitfield mask)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    /* Since glClear is one of the operations that may latch the native size,
     * perform any pending resizes before calling it. */
    if (ctx && ctx->draw) wayland_gl_drawable_sync_size(ctx->draw);
    p_glClear(mask);
}

static BOOL wayland_wglCopyContext(struct wgl_context *src,
                                   struct wgl_context *dst, UINT mask)
{
    FIXME("%p -> %p mask %#x unsupported\n", src, dst, mask);
    return FALSE;
}

static struct wgl_context *wayland_wglCreateContext(HDC hdc)
{
    TRACE("hdc=%p\n", hdc);
    return create_context(hdc, NULL, NULL);
}

static struct wgl_context *wayland_wglCreateContextAttribsARB(HDC hdc,
                                                              struct wgl_context *share,
                                                              const int *attribs)
{
    TRACE("hdc=%p share=%p attribs=%p\n", hdc, share, attribs);
    return create_context(hdc, share, attribs);
}

static BOOL wayland_wglDeleteContext(struct wgl_context *ctx)
{
    struct wgl_pbuffer *pb;

    pthread_mutex_lock(&gl_object_mutex);
    list_remove(&ctx->entry);
    LIST_FOR_EACH_ENTRY(pb, &gl_pbuffers, struct wgl_pbuffer, entry)
    {
        if (pb->prev_context == ctx->context)
        {
            p_eglDestroyContext(egl_display, pb->tmp_context);
            pb->prev_context = pb->tmp_context = NULL;
        }
    }
    pthread_mutex_unlock(&gl_object_mutex);
    p_eglDestroyContext(egl_display, ctx->context);
    if (ctx->draw) wayland_gl_drawable_release(ctx->draw);
    if (ctx->read) wayland_gl_drawable_release(ctx->read);
    free(ctx);
    return TRUE;
}

static PROC wayland_wglGetProcAddress(LPCSTR name)
{
    if (!strncmp(name, "wgl", 3)) return NULL;
    return (PROC)p_eglGetProcAddress(name);
}

static int wayland_wglGetSwapIntervalEXT(void)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;

    if (!ctx || !ctx->draw)
    {
        WARN("No GL drawable found, returning swap interval 0\n");
        return 0;
    }

    /* It's safe to read the value without a lock, since only
     * the current thread can write to it. */
    return ctx->draw->swap_interval;
}

static BOOL wayland_wglMakeContextCurrentARB(HDC draw_hdc, HDC read_hdc,
                                             struct wgl_context *ctx)
{
    BOOL ret;

    TRACE("draw_hdc=%p read_hdc=%p ctx=%p\n", draw_hdc, read_hdc, ctx);

    if (!ctx)
    {
        p_eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        NtCurrentTeb()->glContext = NULL;
        return TRUE;
    }

    ret = wgl_context_make_current(ctx, draw_hdc, read_hdc);
    if (!ret) RtlSetLastWin32Error(ERROR_INVALID_HANDLE);

    return ret;
}

static BOOL wayland_wglMakeCurrent(HDC hdc, struct wgl_context *ctx)
{
    return wayland_wglMakeContextCurrentARB(hdc, hdc, ctx);
}

static BOOL wayland_wglShareLists(struct wgl_context *orig, struct wgl_context *dest)
{
    struct wgl_context *keep, *clobber;

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

static BOOL wayland_wglSwapBuffers(HDC hdc)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    HWND hwnd = NtUserWindowFromDC(hdc), toplevel = NtUserGetAncestor(hwnd, GA_ROOT);
    struct wayland_gl_drawable *gl;

    if (!(gl = wayland_gl_drawable_get(NtUserWindowFromDC(hdc), hdc))) return FALSE;

    if (ctx) wgl_context_refresh(ctx);
    ensure_window_surface_contents(toplevel);
    /* Although all the EGL surfaces we create are double-buffered, we want to
     * use some as single-buffered, so avoid swapping those. */
    if (gl->double_buffered) p_eglSwapBuffers(egl_display, gl->surface);
    wayland_gl_drawable_sync_size(gl);

    wayland_gl_drawable_release(gl);

    return TRUE;
}

static BOOL wayland_wglSwapIntervalEXT(int interval)
{
    struct wgl_context *ctx = NtCurrentTeb()->glContext;
    BOOL ret;

    TRACE("(%d)\n", interval);

    if (interval < 0)
    {
        RtlSetLastWin32Error(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (!ctx || !ctx->draw)
    {
        RtlSetLastWin32Error(ERROR_DC_NOT_FOUND);
        return FALSE;
    }

    /* Lock to protect against concurrent access to drawable swap_interval
     * from wayland_update_gl_drawable */
    pthread_mutex_lock(&gl_object_mutex);
    if ((ret = p_eglSwapInterval(egl_display, interval)))
        ctx->draw->swap_interval = interval;
    else
        RtlSetLastWin32Error(ERROR_DC_NOT_FOUND);
    pthread_mutex_unlock(&gl_object_mutex);

    return ret;
}

static struct wgl_pbuffer *wayland_wglCreatePbufferARB(HDC hdc, int format,
                                                       int width, int height,
                                                       const int *attribs)
{
    struct wgl_pbuffer *pbuffer;

    TRACE("(%p, %d, %d, %d, %p)\n", hdc, format, width, height, attribs);

    if (format <= 0 || format > 2 * num_egl_configs)
    {
        RtlSetLastWin32Error(ERROR_INVALID_PIXEL_FORMAT);
        ERR("Invalid format %d\n", format);
        return NULL;
    }

    /* Use an unmapped wayland surface as our offscreen "pbuffer" surface. */
    if (!(pbuffer = calloc(1, sizeof(*pbuffer))) ||
        !(pbuffer->gl = wayland_gl_drawable_create(0, format)))
    {
        RtlSetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        free(pbuffer);
        return NULL;
    }

    pbuffer->width = width;
    pbuffer->height = height;
    pbuffer->pixel_format = format;
    wl_egl_window_resize(pbuffer->gl->wl_egl_window, width, height, 0, 0);

    for (; attribs && attribs[0]; attribs += 2)
    {
        switch (attribs[0])
        {
        case WGL_TEXTURE_FORMAT_ARB:
            TRACE("attribs[WGL_TEXTURE_FORMAT_ARB]=0x%x\n", attribs[1]);
            switch(attribs[1])
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
                ERR("Unknown texture format: %x\n", attribs[1]);
            }
            break;

        case WGL_TEXTURE_TARGET_ARB:
            TRACE("attribs[WGL_TEXTURE_TARGET_ARB]=0x%x\n", attribs[1]);
            switch (attribs[1])
            {
            case WGL_TEXTURE_CUBE_MAP_ARB:
                if (width != height) goto err;
                pbuffer->texture_target = GL_TEXTURE_CUBE_MAP;
                pbuffer->texture_binding = GL_TEXTURE_BINDING_CUBE_MAP;
                break;
            case WGL_TEXTURE_1D_ARB:
                if (height != 1) goto err;
                pbuffer->texture_target = GL_TEXTURE_1D;
                pbuffer->texture_binding = GL_TEXTURE_BINDING_1D;
                break;
            case WGL_TEXTURE_2D_ARB:
                pbuffer->texture_target = GL_TEXTURE_2D;
                pbuffer->texture_binding = GL_TEXTURE_BINDING_2D;
                break;
            case WGL_TEXTURE_RECTANGLE_NV:
                pbuffer->texture_target = GL_TEXTURE_RECTANGLE_NV;
                pbuffer->texture_binding = GL_TEXTURE_BINDING_RECTANGLE_NV;
                break;
            default:
                ERR("Unknown texture target: %x\n", attribs[1]);
                goto err;
            }
            break;

        case WGL_MIPMAP_TEXTURE_ARB:
            TRACE("attribs[WGL_MIPMAP_TEXTURE_ARB]=0x%x\n", attribs[1]);
            break;

        default:
            WARN("Unknown attribute: %x\n", attribs[0]);
            break;
        }
    }

    pthread_mutex_lock(&gl_object_mutex);
    list_add_head(&gl_pbuffers, &pbuffer->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    return pbuffer;

err:
    RtlSetLastWin32Error(ERROR_INVALID_DATA);
    wayland_gl_drawable_release(pbuffer->gl);
    free(pbuffer);
    return NULL;
}

static BOOL wayland_wglDestroyPbufferARB(struct wgl_pbuffer *pbuffer)
{
    TRACE("(%p)\n", pbuffer);

    pthread_mutex_lock(&gl_object_mutex);
    list_remove(&pbuffer->entry);
    pthread_mutex_unlock(&gl_object_mutex);

    if (pbuffer->tmp_context) p_eglDestroyContext(egl_display, pbuffer->tmp_context);
    wayland_gl_drawable_release(pbuffer->gl);
    free(pbuffer);

    return GL_TRUE;
}

static HDC wayland_wglGetPbufferDCARB(struct wgl_pbuffer *pbuffer)
{
    HDC hdc;
    BOOL set;
    struct wayland_gl_drawable *old;

    if (!(hdc = NtGdiOpenDCW(NULL, NULL, NULL, 0, TRUE, NULL, NULL, NULL))) return 0;

    pthread_mutex_lock(&gl_object_mutex);
    old = clear_pbuffer_dc(hdc);
    set = set_pbuffer_dc(pbuffer, hdc);
    pthread_mutex_unlock(&gl_object_mutex);

    if (old) wayland_gl_drawable_release(old);
    if (!set)
    {
        NtGdiDeleteObjectApp(hdc);
        return 0;
    }

    NtGdiSetPixelFormat(hdc, pbuffer->pixel_format);
    return hdc;
}

static BOOL wayland_wglQueryPbufferARB(struct wgl_pbuffer *pbuffer, int attrib, int *value)
{
    TRACE("(%p, 0x%x, %p)\n", pbuffer, attrib, value);

    switch (attrib)
    {
        case WGL_PBUFFER_WIDTH_ARB: *value = pbuffer->width; break;
        case WGL_PBUFFER_HEIGHT_ARB: *value = pbuffer->height; break;
        case WGL_PBUFFER_LOST_ARB: *value = GL_FALSE; break;
        case WGL_TEXTURE_FORMAT_ARB:
            switch(pbuffer->texture_format)
            {
            case GL_RGB: *value = WGL_TEXTURE_RGB_ARB; break;
            case GL_RGBA: *value = WGL_TEXTURE_RGBA_ARB; break;
            case GL_FLOAT_R_NV: *value = WGL_TEXTURE_FLOAT_R_NV; break;
            case GL_FLOAT_RG_NV: *value = WGL_TEXTURE_FLOAT_RG_NV; break;
            case GL_FLOAT_RGB_NV: *value = WGL_TEXTURE_FLOAT_RGB_NV; break;
            case GL_FLOAT_RGBA_NV: *value = WGL_TEXTURE_FLOAT_RGBA_NV; break;
            default:
                ERR("Unknown texture format: %x\n", pbuffer->texture_format);
                break;
            }
            break;
        case WGL_TEXTURE_TARGET_ARB:
            switch (pbuffer->texture_target)
            {
                case GL_TEXTURE_1D: *value = WGL_TEXTURE_1D_ARB; break;
                case GL_TEXTURE_2D: *value = WGL_TEXTURE_2D_ARB; break;
                case GL_TEXTURE_CUBE_MAP: *value = WGL_TEXTURE_CUBE_MAP_ARB; break;
                case GL_TEXTURE_RECTANGLE_NV: *value = WGL_TEXTURE_RECTANGLE_NV; break;
                default:
                    ERR("Unknown texture target: %x\n", pbuffer->texture_target);
                    break;
            }
            break;
        case WGL_MIPMAP_TEXTURE_ARB:
            *value = GL_FALSE;
            FIXME("unsupported attribute query for 0x%x\n", attrib);
            break;
        default:
            FIXME("unexpected attribute %x\n", attrib);
            break;
    }

    return GL_TRUE;
}

static int wayland_wglReleasePbufferDCARB(struct wgl_pbuffer *pbuffer, HDC hdc)
{
    struct wayland_gl_drawable *old;

    pthread_mutex_lock(&gl_object_mutex);
    old = clear_pbuffer_dc(hdc);
    pthread_mutex_unlock(&gl_object_mutex);

    if (old) wayland_gl_drawable_release(old);
    else hdc = 0;

    return hdc && NtGdiDeleteObjectApp(hdc);
}

static BOOL wayland_wglBindTexImageARB(struct wgl_pbuffer *pbuffer, int buffer)
{
    EGLContext prev_context = p_eglGetCurrentContext();
    EGLSurface prev_draw = p_eglGetCurrentSurface(EGL_DRAW);
    EGLSurface prev_read = p_eglGetCurrentSurface(EGL_READ);
    int prev_bound_texture = 0;

    TRACE("(%p, %d)\n", pbuffer, buffer);

    if (!pbuffer->tmp_context || pbuffer->prev_context != prev_context)
    {
        if (pbuffer->tmp_context) p_eglDestroyContext(egl_display, pbuffer->tmp_context);
        p_eglBindAPI(EGL_OPENGL_API);
        pbuffer->tmp_context =
            p_eglCreateContext(egl_display, EGL_NO_CONFIG_KHR, prev_context, NULL);
        pbuffer->prev_context = prev_context;
    }

    opengl_funcs.p_glGetIntegerv(pbuffer->texture_binding, &prev_bound_texture);

    p_eglMakeCurrent(egl_display, pbuffer->gl->surface, pbuffer->gl->surface,
                     pbuffer->tmp_context);

    /* Make sure that the prev_bound_texture is set as the current texture
     * state isn't shared between contexts. After that copy the pbuffer texture
     * data. Note that at the moment we ignore the 'buffer' argument and always
     * copy from the pbuffer back buffer. */
    opengl_funcs.p_glBindTexture(pbuffer->texture_target, prev_bound_texture);
    opengl_funcs.p_glCopyTexImage2D(pbuffer->texture_target, 0,
                                       pbuffer->texture_format, 0, 0,
                                       pbuffer->width, pbuffer->height, 0);

    /* Switch back to the original surface and context. */
    p_eglMakeCurrent(egl_display, prev_draw, prev_read, prev_context);

    return GL_TRUE;
}

static BOOL wayland_wglReleaseTexImageARB(struct wgl_pbuffer *pbuffer, int buffer)
{
    TRACE("(%p, %d)\n", pbuffer, buffer);

    if (!pbuffer->texture_format)
    {
        RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    return GL_TRUE;
}

static BOOL wayland_wglSetPbufferAttribARB(struct wgl_pbuffer *pbuffer, const int *attribs)
{
    if (!pbuffer->texture_format)
    {
        RtlSetLastWin32Error(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    return GL_TRUE;
}

static void describe_pixel_format(EGLConfig config, struct wgl_pixel_format *fmt, BOOL pbuffer_single)
{
    EGLint value, surface_type;
    PIXELFORMATDESCRIPTOR *pfd = &fmt->pfd;

    /* If we can't get basic information, there is no point continuing */
    if (!p_eglGetConfigAttrib(egl_display, config, EGL_SURFACE_TYPE, &surface_type)) return;

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
}

static void wayland_get_pixel_formats(struct wgl_pixel_format *formats,
                                      UINT max_formats, UINT *num_formats,
                                      UINT *num_onscreen_formats)
{
    UINT i;

    if (formats)
    {
        for (i = 0; i < min(max_formats, num_egl_configs); ++i)
            describe_pixel_format(egl_configs[i], &formats[i], FALSE);
        /* Add single-buffered pbuffer capable configs. */
        for (i = num_egl_configs; i < min(max_formats, 2 * num_egl_configs); ++i)
            describe_pixel_format(egl_configs[i - num_egl_configs], &formats[i], TRUE);
    }
    *num_formats = 2 * num_egl_configs;
    *num_onscreen_formats = num_egl_configs;
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
    register_extension("WGL_ARB_make_current_read");
    opengl_funcs.p_wglGetCurrentReadDCARB = (void *)1;  /* never called */
    opengl_funcs.p_wglMakeContextCurrentARB = wayland_wglMakeContextCurrentARB;

    register_extension("WGL_ARB_create_context");
    register_extension("WGL_ARB_create_context_no_error");
    register_extension("WGL_ARB_create_context_profile");
    opengl_funcs.p_wglCreateContextAttribsARB = wayland_wglCreateContextAttribsARB;

    register_extension("WGL_EXT_swap_control");
    opengl_funcs.p_wglGetSwapIntervalEXT = wayland_wglGetSwapIntervalEXT;
    opengl_funcs.p_wglSwapIntervalEXT = wayland_wglSwapIntervalEXT;

    if (has_egl_ext_pixel_format_float)
    {
        register_extension("WGL_ARB_pixel_format_float");
        register_extension("WGL_ATI_pixel_format_float");
    }

    register_extension("WGL_ARB_pbuffer");
    opengl_funcs.p_wglCreatePbufferARB = wayland_wglCreatePbufferARB;
    opengl_funcs.p_wglDestroyPbufferARB = wayland_wglDestroyPbufferARB;
    opengl_funcs.p_wglGetPbufferDCARB = wayland_wglGetPbufferDCARB;
    opengl_funcs.p_wglQueryPbufferARB = wayland_wglQueryPbufferARB;
    opengl_funcs.p_wglReleasePbufferDCARB = wayland_wglReleasePbufferDCARB;

    register_extension("WGL_ARB_render_texture");
    opengl_funcs.p_wglBindTexImageARB = wayland_wglBindTexImageARB;
    opengl_funcs.p_wglReleaseTexImageARB = wayland_wglReleaseTexImageARB;
    opengl_funcs.p_wglSetPbufferAttribARB = wayland_wglSetPbufferAttribARB;

    return wgl_extensions;
}

static BOOL init_egl_configs(void)
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
        return FALSE;
    }
    if (!p_eglChooseConfig(egl_display, attribs, egl_configs, num_egl_configs,
                           &num_egl_configs) ||
        !num_egl_configs)
    {
        free(egl_configs);
        egl_configs = NULL;
        num_egl_configs = 0;
        ERR("Failed to get any configs from eglChooseConfig\n");
        return FALSE;
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

    return TRUE;
}

static const struct opengl_driver_funcs wayland_driver_funcs =
{
    .p_init_wgl_extensions = wayland_init_wgl_extensions,
    .p_set_pixel_format = wayland_set_pixel_format,
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
    if (!init_egl_configs()) goto err;
    *funcs = &opengl_funcs;
    *driver_funcs = &wayland_driver_funcs;
    return STATUS_SUCCESS;

err:
    dlclose(egl_handle);
    egl_handle = NULL;
    return STATUS_NOT_SUPPORTED;
}

static struct opengl_funcs opengl_funcs =
{
    .p_wglCopyContext = wayland_wglCopyContext,
    .p_wglCreateContext = wayland_wglCreateContext,
    .p_wglDeleteContext = wayland_wglDeleteContext,
    .p_wglGetProcAddress = wayland_wglGetProcAddress,
    .p_wglMakeCurrent = wayland_wglMakeCurrent,
    .p_wglShareLists = wayland_wglShareLists,
    .p_wglSwapBuffers = wayland_wglSwapBuffers,
    .p_get_pixel_formats = wayland_get_pixel_formats,
};

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
