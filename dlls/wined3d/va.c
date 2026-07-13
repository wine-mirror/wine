/*
 * Copyright 2024 Elizabeth Figura for CodeWeavers
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

#if defined(SONAME_LIBVA) && defined(SONAME_LIBVA_DRM)

#include "initguid.h"
#include "unixlib.h"
#include "dxva.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
#include "ntgdi.h"
#include "wine/vulkan_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

struct va_context
{
    void *libva_handle, *libva_drm_handle;
#define MAKE_FUNCPTR(f) typeof(f) *f
    MAKE_FUNCPTR(vaGetDisplayDRM);
    MAKE_FUNCPTR(vaInitialize);
    MAKE_FUNCPTR(vaMaxNumProfiles);
    MAKE_FUNCPTR(vaQueryConfigProfiles);
    MAKE_FUNCPTR(vaTerminate);
#undef MAKE_FUNCPTR

    VADisplay display;
    int fd;
};

static void close_va_display(struct va_context *ctx)
{
    if (ctx->fd != -1)
        close(ctx->fd);
    ctx->vaTerminate(ctx->display);
    dlclose(ctx->libva_drm_handle);
    dlclose(ctx->libva_handle);
}

static NTSTATUS open_va_display(UINT64 handle, struct va_context *ctx)
{
    VkPhysicalDeviceDrmPropertiesEXT drm_properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT};
    VkPhysicalDeviceProperties2 properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    const struct vulkan_physical_device *device = vulkan_physical_device_from_handle((VkPhysicalDevice)handle);
    int major, minor;
    VAStatus status;
    char *path;

    ctx->display = 0;
    ctx->fd = -1;

    if (!device->extensions.has_VK_EXT_physical_device_drm)
        return E_NOTIMPL;

    if (!(ctx->libva_handle = dlopen(SONAME_LIBVA, RTLD_NOW)))
        return E_NOTIMPL;
    if (!(ctx->libva_drm_handle = dlopen(SONAME_LIBVA_DRM, RTLD_NOW)))
    {
        dlclose(ctx->libva_handle);
        return E_NOTIMPL;
    }

#define LOAD_FUNCPTR(f) \
    if (!(ctx->f = dlsym(ctx->libva_handle, #f))) \
    { \
        ERR("Failed to load function %s.\n", #f); \
        goto fail; \
    }
    LOAD_FUNCPTR(vaInitialize);
    LOAD_FUNCPTR(vaMaxNumProfiles);
    LOAD_FUNCPTR(vaQueryConfigProfiles);
    LOAD_FUNCPTR(vaTerminate);
#undef LOAD_FUNCPTR

    if (!(ctx->vaGetDisplayDRM = dlsym(ctx->libva_drm_handle, "vaGetDisplayDRM")))
    {
        ERR("Failed to load function vaGetDisplayDRM.\n");
        goto fail;
    }

    properties.pNext = &drm_properties;
    device->instance->p_vkGetPhysicalDeviceProperties2KHR(device->host.physical_device, &properties);

    if (drm_properties.hasPrimary)
    {
        asprintf(&path, "/dev/dri/card%"PRIu64, drm_properties.primaryMinor);
    }
    else if (drm_properties.hasRender)
    {
        asprintf(&path, "/dev/dri/renderD%"PRIu64, drm_properties.primaryMinor);
    }
    else
    {
        ERR("Vulkan device has neither a primary nor render node.\n");
        goto fail;
    }

    if ((ctx->fd = open(path, O_RDWR | O_CLOEXEC)) < 0)
    {
        ERR("Failed to open %s: %s\n", path, strerror(errno));
        free(path);
        goto fail;
    }
    free(path);

    ctx->display = ctx->vaGetDisplayDRM(ctx->fd);
    if ((status = ctx->vaInitialize(ctx->display, &major, &minor)) != VA_STATUS_SUCCESS)
    {
        ERR("Failed to initialize VA, error %#x.\n", status);
        goto fail;
    }

    return S_OK;

fail:
    close_va_display(ctx);
    return E_NOTIMPL;
}

static NTSTATUS va_get_profiles_vk(void *args)
{
    struct va_get_profiles_vk_params *params = args;
    unsigned int *count = (unsigned int *)(uintptr_t)params->count;
    GUID *profiles = (GUID *)(uintptr_t)params->profiles;
    VAProfile *va_profiles;
    struct va_context ctx;
    NTSTATUS status;
    int max_count;

    if ((status = open_va_display(params->physical_device, &ctx)))
        return status;

    max_count = ctx.vaMaxNumProfiles(ctx.display);
    va_profiles = malloc(max_count * sizeof(*va_profiles));
    ctx.vaQueryConfigProfiles(ctx.display, va_profiles, &max_count);

    for (int i = 0; i < max_count; ++i)
    {
        if (va_profiles[i] == VAProfileH264High)
        {
            profiles[(*count)++] = DXVA_ModeH264_VLD_NoFGT;
            /* FIXME: Native GPUs also support DXVA2_ModeH264_VLD_Stereo_NoFGT
             * and DXVA2_ModeH264_VLD_Stereo_Progressive_NoFGT. */
        }
    }

    free(va_profiles);
    close_va_display(&ctx);
    return S_OK;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    [unix_va_get_profiles_vk] = va_get_profiles_vk,
};

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    [unix_va_get_profiles_vk] = va_get_profiles_vk,
};

#endif
