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

#define USE_GL_FUNC(name) #name,
static const char *opengl_func_names[] = { ALL_WGL_FUNCS };
#undef USE_GL_FUNC

#define DECL_FUNCPTR(f) static typeof(f) * p_##f
DECL_FUNCPTR(eglGetError);
DECL_FUNCPTR(eglGetPlatformDisplay);
DECL_FUNCPTR(eglGetProcAddress);
DECL_FUNCPTR(eglInitialize);
DECL_FUNCPTR(eglQueryString);
#undef DECL_FUNCPTR

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
