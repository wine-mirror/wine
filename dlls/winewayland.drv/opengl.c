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

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "waylanddrv.h"
#include "wine/debug.h"

#if defined(SONAME_LIBEGL)

WINE_DEFAULT_DEBUG_CHANNEL(waylanddrv);

#define WL_EGL_PLATFORM 1
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
DECL_FUNCPTR(eglChooseConfig);
DECL_FUNCPTR(eglGetConfigAttrib);
DECL_FUNCPTR(eglGetError);
DECL_FUNCPTR(eglGetPlatformDisplay);
DECL_FUNCPTR(eglGetProcAddress);
DECL_FUNCPTR(eglInitialize);
DECL_FUNCPTR(eglQueryString);
#undef DECL_FUNCPTR

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

    register_extension("WGL_ARB_extensions_string");
    opengl_funcs.ext.p_wglGetExtensionsStringARB = wayland_wglGetExtensionsStringARB;

    register_extension("WGL_EXT_extensions_string");
    opengl_funcs.ext.p_wglGetExtensionsStringEXT = wayland_wglGetExtensionsStringEXT;

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
    const char *egl_client_exts;

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
    LOAD_FUNCPTR_EGL(eglChooseConfig);
    LOAD_FUNCPTR_EGL(eglGetConfigAttrib);
    LOAD_FUNCPTR_EGL(eglGetError);
    LOAD_FUNCPTR_EGL(eglGetPlatformDisplay);
    LOAD_FUNCPTR_EGL(eglInitialize);
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
        .p_wglDescribePixelFormat = wayland_wglDescribePixelFormat,
        .p_wglGetProcAddress = wayland_wglGetProcAddress,
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

#else /* No GL */

struct opengl_funcs *WAYLAND_wine_get_wgl_driver(UINT version)
{
    return NULL;
}

#endif
