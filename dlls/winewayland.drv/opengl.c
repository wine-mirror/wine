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

#include "waylanddrv.h"
#include "wine/debug.h"

#if defined(SONAME_LIBEGL) && defined(HAVE_LIBWAYLAND_EGL)

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wine/wgl.h"
#include "wine/wgl_driver.h"

static void *egl_handle;
static struct opengl_funcs opengl_funcs;
static EGLDisplay egl_display;
static char wgl_extensions[4096];
static EGLConfig *egl_configs;
static int num_egl_configs;

#define USE_GL_FUNC(name) #name,
static const char *opengl_func_names[] = { ALL_WGL_FUNCS };
#undef USE_GL_FUNC

#define DECL_FUNCPTR(f) static typeof(f) * p_##f
DECL_FUNCPTR(eglBindAPI);
DECL_FUNCPTR(eglChooseConfig);
DECL_FUNCPTR(eglCreateContext);
DECL_FUNCPTR(eglCreateWindowSurface);
DECL_FUNCPTR(eglDestroyContext);
DECL_FUNCPTR(eglDestroySurface);
DECL_FUNCPTR(eglGetConfigAttrib);
DECL_FUNCPTR(eglGetError);
DECL_FUNCPTR(eglGetPlatformDisplay);
DECL_FUNCPTR(eglGetProcAddress);
DECL_FUNCPTR(eglInitialize);
DECL_FUNCPTR(eglMakeCurrent);
DECL_FUNCPTR(eglQueryString);
DECL_FUNCPTR(eglSwapBuffers);
DECL_FUNCPTR(eglSwapInterval);
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
    struct wayland_client_surface *client;
    struct wl_egl_window *wl_egl_window;
    EGLSurface surface;
    LONG resized;
    int swap_interval;
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

/* lookup the existing drawable for a window, gl_object_mutex must be held */
static struct wayland_gl_drawable *find_drawable_for_hwnd(HWND hwnd)
{
    struct wayland_gl_drawable *gl;
    LIST_FOR_EACH_ENTRY(gl, &gl_drawables, struct wayland_gl_drawable, entry)
        if (gl->hwnd == hwnd) return gl;
    return NULL;
}

static struct wayland_gl_drawable *wayland_gl_drawable_acquire(struct wayland_gl_drawable *gl)
{
    InterlockedIncrement(&gl->ref);
    return gl;
}

static struct wayland_gl_drawable *wayland_gl_drawable_get(HWND hwnd)
{
    struct wayland_gl_drawable *ret;

    pthread_mutex_lock(&gl_object_mutex);
    if ((ret = find_drawable_for_hwnd(hwnd)))
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
        struct wayland_surface *wayland_surface = wayland_surface_lock_hwnd(hwnd);

        if (wayland_client_surface_release(gl->client) && wayland_surface)
            wayland_surface->client = NULL;

        if (wayland_surface) pthread_mutex_unlock(&wayland_surface->mutex);
    }

    free(gl);
}

static struct wayland_gl_drawable *wayland_gl_drawable_create(HWND hwnd, int format)
{
    struct wayland_gl_drawable *gl;
    struct wayland_surface *wayland_surface;
    int client_width = 0, client_height = 0;

    TRACE("hwnd=%p format=%d\n", hwnd, format);

    gl = calloc(1, sizeof(*gl));
    if (!gl) return NULL;

    gl->ref = 1;
    gl->hwnd = hwnd;
    gl->swap_interval = 1;

    /* Get the client surface for the HWND. If don't have a wayland surface
     * (e.g., HWND_MESSAGE windows) just create a dummy surface to act as the
     * target render surface. */
    if ((wayland_surface = wayland_surface_lock_hwnd(hwnd)))
    {
        gl->client = wayland_surface_get_client(wayland_surface);
        client_width = wayland_surface->window.client_rect.right -
                       wayland_surface->window.client_rect.left;
        client_height = wayland_surface->window.client_rect.bottom -
                        wayland_surface->window.client_rect.top;
        if (client_width == 0 || client_height == 0)
            client_width = client_height = 1;
        pthread_mutex_unlock(&wayland_surface->mutex);
    }
    else if ((wayland_surface = wayland_surface_create(0)))
    {
        gl->client = wayland_surface_get_client(wayland_surface);
        client_width = client_height = 1;
        /* It's fine to destroy the wayland surface, the client surface
         * can safely outlive it. */
        wayland_surface_destroy(wayland_surface);
    }
    if (!gl->client) goto err;

    gl->wl_egl_window = wl_egl_window_create(gl->client->wl_surface,
                                             client_width, client_height);
    if (!gl->wl_egl_window)
    {
        ERR("Failed to create wl_egl_window\n");
        goto err;
    }

    gl->surface = p_eglCreateWindowSurface(egl_display, egl_configs[format - 1],
                                           gl->wl_egl_window, NULL);
    if (!gl->surface)
    {
        ERR("Failed to create EGL surface\n");
        goto err;
    }

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

    if ((old = find_drawable_for_hwnd(hwnd))) list_remove(&old->entry);
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
    struct wayland_surface *wayland_surface;

    if (InterlockedCompareExchange(&gl->resized, FALSE, TRUE))
    {
        if (!(wayland_surface = wayland_surface_lock_hwnd(gl->hwnd))) return;

        client_width = wayland_surface->window.client_rect.right -
                       wayland_surface->window.client_rect.left;
        client_height = wayland_surface->window.client_rect.bottom -
                        wayland_surface->window.client_rect.top;
        if (client_width == 0 || client_height == 0)
            client_width = client_height = 1;

        wl_egl_window_resize(gl->wl_egl_window, client_width, client_height, 0, 0);

        pthread_mutex_unlock(&wayland_surface->mutex);
    }
}

static void wayland_gl_drawable_sync_surface_state(struct wayland_gl_drawable *gl)
{
    struct wayland_surface *wayland_surface;

    if (!(wayland_surface = wayland_surface_lock_hwnd(gl->hwnd))) return;

    wayland_surface_ensure_contents(wayland_surface);

    /* Handle any processed configure request, to ensure the related
     * surface state is applied by the compositor. */
    if (wayland_surface->processing.serial &&
        wayland_surface->processing.processed &&
        wayland_surface_reconfigure(wayland_surface))
    {
        wl_surface_commit(wayland_surface->wl_surface);
    }

    pthread_mutex_unlock(&wayland_surface->mutex);
}

static BOOL wgl_context_make_current(struct wgl_context *ctx, HWND draw_hwnd,
                                     HWND read_hwnd)
{
    BOOL ret;
    struct wayland_gl_drawable *draw, *read;
    struct wayland_gl_drawable *old_draw = NULL, *old_read = NULL;

    draw = wayland_gl_drawable_get(draw_hwnd);
    read = wayland_gl_drawable_get(read_hwnd);

    TRACE("%p/%p context %p surface %p/%p\n",
          draw_hwnd, read_hwnd, ctx->context,
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

static BOOL set_pixel_format(HDC hdc, int format, BOOL internal)
{
    HWND hwnd = NtUserWindowFromDC(hdc);
    struct wayland_gl_drawable *gl;
    int prev = 0;

    if (!hwnd || hwnd == NtUserGetDesktopWindow())
    {
        WARN("not a proper window DC %p/%p\n", hdc, hwnd);
        return FALSE;
    }
    if (format < 0 || format >= num_egl_configs)
    {
        WARN("Invalid format %d\n", format);
        return FALSE;
    }
    TRACE("%p/%p format %d\n", hdc, hwnd, format);

    /* Even for internal pixel format fail setting it if the app has already set a
     * different pixel format. Let wined3d create a backup GL context instead.
     * Switching pixel format involves drawable recreation and is much more expensive
     * than blitting from backup context. */
    if ((prev = win32u_get_window_pixel_format(hwnd)))
        return prev == format;

    if (!(gl = wayland_gl_drawable_create(hwnd, format))) return FALSE;
    wayland_update_gl_drawable(hwnd, gl);
    win32u_set_window_pixel_format(hwnd, format, internal);

    return TRUE;
}

static struct wgl_context *create_context(HDC hdc, struct wgl_context *share,
                                          const int *attribs)
{
    struct wayland_gl_drawable *gl;
    struct wgl_context *ctx;

    if (!(gl = wayland_gl_drawable_get(NtUserWindowFromDC(hdc)))) return NULL;

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
    pthread_mutex_lock(&gl_object_mutex);
    list_remove(&ctx->entry);
    pthread_mutex_unlock(&gl_object_mutex);
    p_eglDestroyContext(egl_display, ctx->context);
    if (ctx->draw) wayland_gl_drawable_release(ctx->draw);
    if (ctx->read) wayland_gl_drawable_release(ctx->read);
    free(ctx);
    return TRUE;
}

static BOOL has_opengl(void);

static int wayland_wglDescribePixelFormat(HDC hdc, int fmt, UINT size,
                                          PIXELFORMATDESCRIPTOR *pfd)
{
    EGLint val;
    EGLConfig config;

    if (!has_opengl()) return 0;
    if (!pfd) return num_egl_configs;
    if (size < sizeof(*pfd)) return 0;
    if (fmt <= 0 || fmt > num_egl_configs) return 0;

    config = egl_configs[fmt - 1];

    memset(pfd, 0, sizeof(*pfd));
    pfd->nSize = sizeof(*pfd);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER |
                   PFD_SUPPORT_COMPOSITION;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->iLayerType = PFD_MAIN_PLANE;

    /* Although the documentation describes cColorBits as excluding alpha, real
     * drivers tend to return the full pixel size, so do the same. */
    p_eglGetConfigAttrib(egl_display, config, EGL_BUFFER_SIZE, &val);
    pfd->cColorBits = val;
    p_eglGetConfigAttrib(egl_display, config, EGL_RED_SIZE, &val);
    pfd->cRedBits = val;
    p_eglGetConfigAttrib(egl_display, config, EGL_GREEN_SIZE, &val);
    pfd->cGreenBits = val;
    p_eglGetConfigAttrib(egl_display, config, EGL_BLUE_SIZE, &val);
    pfd->cBlueBits = val;
    p_eglGetConfigAttrib(egl_display, config, EGL_ALPHA_SIZE, &val);
    pfd->cAlphaBits = val;
    p_eglGetConfigAttrib(egl_display, config, EGL_DEPTH_SIZE, &val);
    pfd->cDepthBits = val;
    p_eglGetConfigAttrib(egl_display, config, EGL_STENCIL_SIZE, &val);
    pfd->cStencilBits = val;

    /* Although we don't get information from EGL about the component shifts
     * or the native format, the 0xARGB order is the most common. */
    pfd->cBlueShift = 0;
    pfd->cGreenShift = pfd->cBlueBits;
    pfd->cRedShift = pfd->cGreenBits + pfd->cBlueBits;
    if (pfd->cAlphaBits)
        pfd->cAlphaShift = pfd->cRedBits + pfd->cGreenBits + pfd->cBlueBits;
    else
        pfd->cAlphaShift = 0;

    TRACE("fmt %u color %u %u/%u/%u/%u depth %u stencil %u\n",
          fmt, pfd->cColorBits, pfd->cRedBits, pfd->cGreenBits, pfd->cBlueBits,
          pfd->cAlphaBits, pfd->cDepthBits, pfd->cStencilBits);

    return num_egl_configs;
}

static const char *wayland_wglGetExtensionsStringARB(HDC hdc)
{
    TRACE("() returning \"%s\"\n", wgl_extensions);
    return wgl_extensions;
}

static const char *wayland_wglGetExtensionsStringEXT(void)
{
    TRACE("() returning \"%s\"\n", wgl_extensions);
    return wgl_extensions;
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

    ret = wgl_context_make_current(ctx, NtUserWindowFromDC(draw_hdc), NtUserWindowFromDC(read_hdc));
    if (!ret) RtlSetLastWin32Error(ERROR_INVALID_HANDLE);

    return ret;
}

static BOOL wayland_wglMakeCurrent(HDC hdc, struct wgl_context *ctx)
{
    return wayland_wglMakeContextCurrentARB(hdc, hdc, ctx);
}

static BOOL wayland_wglSetPixelFormat(HDC hdc, int format,
                                      const PIXELFORMATDESCRIPTOR *pfd)
{
    return set_pixel_format(hdc, format, FALSE);
}

static BOOL wayland_wglSetPixelFormatWINE(HDC hdc, int format)
{
    return set_pixel_format(hdc, format, TRUE);
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
    HWND hwnd = NtUserWindowFromDC(hdc);
    struct wayland_gl_drawable *gl;

    if (!(gl = wayland_gl_drawable_get(hwnd))) return FALSE;

    if (ctx) wgl_context_refresh(ctx);
    wayland_gl_drawable_sync_surface_state(gl);
    p_eglSwapBuffers(egl_display, gl->surface);
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
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(opengl_func_names); i++)
    {
        if (!(((void **)&opengl_funcs.gl)[i] = p_eglGetProcAddress(opengl_func_names[i])))
        {
            ERR("%s not found, disabling OpenGL.\n", opengl_func_names[i]);
            return FALSE;
        }
    }

    p_glClear = opengl_funcs.gl.p_glClear;
    opengl_funcs.gl.p_glClear = wayland_glClear;

    register_extension("WGL_ARB_extensions_string");
    opengl_funcs.ext.p_wglGetExtensionsStringARB = wayland_wglGetExtensionsStringARB;

    register_extension("WGL_EXT_extensions_string");
    opengl_funcs.ext.p_wglGetExtensionsStringEXT = wayland_wglGetExtensionsStringEXT;

    register_extension("WGL_WINE_pixel_format_passthrough");
    opengl_funcs.ext.p_wglSetPixelFormatWINE = wayland_wglSetPixelFormatWINE;

    register_extension("WGL_ARB_make_current_read");
    opengl_funcs.ext.p_wglGetCurrentReadDCARB = (void *)1;  /* never called */
    opengl_funcs.ext.p_wglMakeContextCurrentARB = wayland_wglMakeContextCurrentARB;

    register_extension("WGL_ARB_create_context");
    register_extension("WGL_ARB_create_context_no_error");
    register_extension("WGL_ARB_create_context_profile");
    opengl_funcs.ext.p_wglCreateContextAttribsARB = wayland_wglCreateContextAttribsARB;

    register_extension("WGL_EXT_swap_control");
    opengl_funcs.ext.p_wglGetSwapIntervalEXT = wayland_wglGetSwapIntervalEXT;
    opengl_funcs.ext.p_wglSwapIntervalEXT = wayland_wglSwapIntervalEXT;

    return TRUE;
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

static void init_opengl(void)
{
    EGLint egl_version[2];
    const char *egl_client_exts, *egl_exts;

    if (!(egl_handle = dlopen(SONAME_LIBEGL, RTLD_NOW|RTLD_GLOBAL)))
    {
        ERR("Failed to load %s: %s\n", SONAME_LIBEGL, dlerror());
        return;
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
#undef REQUIRE_EXT

    if (!init_opengl_funcs()) goto err;
    if (!init_egl_configs()) goto err;

    return;

err:
    dlclose(egl_handle);
    egl_handle = NULL;
}

static BOOL has_opengl(void)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;

    return !pthread_once(&init_once, init_opengl) && egl_handle;
}

static struct opengl_funcs opengl_funcs =
{
    .wgl =
    {
        .p_wglCopyContext = wayland_wglCopyContext,
        .p_wglCreateContext = wayland_wglCreateContext,
        .p_wglDeleteContext = wayland_wglDeleteContext,
        .p_wglDescribePixelFormat = wayland_wglDescribePixelFormat,
        .p_wglGetProcAddress = wayland_wglGetProcAddress,
        .p_wglMakeCurrent = wayland_wglMakeCurrent,
        .p_wglSetPixelFormat = wayland_wglSetPixelFormat,
        .p_wglShareLists = wayland_wglShareLists,
        .p_wglSwapBuffers = wayland_wglSwapBuffers,
    }
};

/**********************************************************************
 *           WAYLAND_wine_get_wgl_driver
 */
struct opengl_funcs *WAYLAND_wine_get_wgl_driver(UINT version)
{
    if (version != WINE_WGL_DRIVER_VERSION)
    {
        ERR("Version mismatch, opengl32 wants %u but driver has %u\n",
            version, WINE_WGL_DRIVER_VERSION);
        return NULL;
    }
    if (!has_opengl()) return NULL;
    return &opengl_funcs;
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

    if (!(gl = wayland_gl_drawable_get(hwnd))) return;
    /* wl_egl_window_resize is not thread safe, so we just mark the
     * drawable as resized and perform the resize in the proper thread. */
    InterlockedExchange(&gl->resized, TRUE);
    wayland_gl_drawable_release(gl);
}

#else /* No GL */

struct opengl_funcs *WAYLAND_wine_get_wgl_driver(UINT version)
{
    return NULL;
}

void wayland_destroy_gl_drawable(HWND hwnd)
{
}

void wayland_resize_gl_drawable(HWND hwnd)
{
}

#endif
