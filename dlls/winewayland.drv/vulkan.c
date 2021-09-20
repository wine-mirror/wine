/* WAYLANDDRV Vulkan implementation
 *
 * Copyright 2017 Roderick Colenbrander
 * Copyright 2021 Alexandros Frantzis
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

#include "waylanddrv.h"
#include "wine/debug.h"

#define VK_NO_PROTOTYPES
#define WINE_VK_HOST

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#ifdef SONAME_LIBVULKAN

static VkResult (*pvkCreateInstance)(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *);
static void (*pvkDestroyInstance)(VkInstance, const VkAllocationCallbacks *);
static VkResult (*pvkEnumerateInstanceExtensionProperties)(const char *, uint32_t *, VkExtensionProperties *);
static void * (*pvkGetDeviceProcAddr)(VkDevice, const char *);
static void * (*pvkGetInstanceProcAddr)(VkInstance, const char *);

static void *vulkan_handle;
static const struct vulkan_funcs vulkan_funcs;

/* Helper function for converting between win32 and Wayland compatible VkInstanceCreateInfo.
 * Caller is responsible for allocation and cleanup of 'dst'. */
static VkResult wine_vk_instance_convert_create_info(const VkInstanceCreateInfo *src,
                                                     VkInstanceCreateInfo *dst)
{
    unsigned int i;
    const char **enabled_extensions = NULL;

    dst->sType = src->sType;
    dst->flags = src->flags;
    dst->pApplicationInfo = src->pApplicationInfo;
    dst->pNext = src->pNext;
    dst->enabledLayerCount = 0;
    dst->ppEnabledLayerNames = NULL;
    dst->enabledExtensionCount = 0;
    dst->ppEnabledExtensionNames = NULL;

    if (src->enabledExtensionCount > 0)
    {
        enabled_extensions = calloc(src->enabledExtensionCount,
                                    sizeof(*src->ppEnabledExtensionNames));
        if (!enabled_extensions)
        {
            ERR("Failed to allocate memory for enabled extensions\n");
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        for (i = 0; i < src->enabledExtensionCount; i++)
        {
            /* Substitute extension with Wayland ones else copy. Long-term, when we
             * support more extensions, we should store these in a list. */
            if (!strcmp(src->ppEnabledExtensionNames[i], "VK_KHR_win32_surface"))
                enabled_extensions[i] = "VK_KHR_wayland_surface";
            else
                enabled_extensions[i] = src->ppEnabledExtensionNames[i];
        }
        dst->ppEnabledExtensionNames = enabled_extensions;
        dst->enabledExtensionCount = src->enabledExtensionCount;
    }

    return VK_SUCCESS;
}

static VkResult wayland_vkCreateInstance(const VkInstanceCreateInfo *create_info,
                                         const VkAllocationCallbacks *allocator,
                                         VkInstance *instance)
{
    VkInstanceCreateInfo create_info_host;
    VkResult res;

    TRACE("create_info %p, allocator %p, instance %p\n", create_info, allocator, instance);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    /* Perform a second pass on converting VkInstanceCreateInfo. Winevulkan
     * performed a first pass in which it handles everything except for WSI
     * functionality such as VK_KHR_win32_surface. Handle this now. */
    res = wine_vk_instance_convert_create_info(create_info, &create_info_host);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to convert instance create info, res=%d\n", res);
        return res;
    }

    res = pvkCreateInstance(&create_info_host, NULL /* allocator */, instance);

    free((void *)create_info_host.ppEnabledExtensionNames);
    return res;
}

static void wayland_vkDestroyInstance(VkInstance instance,
                                      const VkAllocationCallbacks *allocator)
{
    TRACE("%p %p\n", instance, allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    pvkDestroyInstance(instance, NULL /* allocator */);
}

static VkResult wayland_vkEnumerateInstanceExtensionProperties(const char *layer_name,
                                                               uint32_t *count,
                                                               VkExtensionProperties* properties)
{
    unsigned int i;
    VkResult res;

    TRACE("layer_name %s, count %p, properties %p\n", debugstr_a(layer_name), count, properties);

    /* This shouldn't get called with layer_name set, the ICD loader prevents it. */
    if (layer_name)
    {
        ERR("Layer enumeration not supported from ICD.\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    /* We will return the same number of instance extensions reported by the host back to
     * winevulkan. Along the way we may replace xlib extensions with their win32 equivalents.
     * Winevulkan will perform more detailed filtering as it knows whether it has thunks
     * for a particular extension.
     */
    res = pvkEnumerateInstanceExtensionProperties(layer_name, count, properties);
    if (!properties || res < 0)
        return res;

    for (i = 0; i < *count; i++)
    {
        /* For now the only wayland extension we need to fixup. Long-term we may need an array. */
        if (!strcmp(properties[i].extensionName, "VK_KHR_wayland_surface"))
        {
            TRACE("Substituting VK_KHR_wayland_surface for VK_KHR_win32_surface\n");

            snprintf(properties[i].extensionName, sizeof(properties[i].extensionName),
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            properties[i].specVersion = VK_KHR_WIN32_SURFACE_SPEC_VERSION;
        }
    }

    TRACE("Returning %u extensions.\n", *count);
    return res;
}

static void *wayland_vkGetDeviceProcAddr(VkDevice device, const char *name)
{
    void *proc_addr;

    TRACE("%p, %s\n", device, debugstr_a(name));

    if ((proc_addr = get_vulkan_driver_device_proc_addr(&vulkan_funcs, name)))
        return proc_addr;

    return pvkGetDeviceProcAddr(device, name);
}

static void *wayland_vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    void *proc_addr;

    TRACE("%p, %s\n", instance, debugstr_a(name));

    if ((proc_addr = get_vulkan_driver_instance_proc_addr(&vulkan_funcs, instance, name)))
        return proc_addr;

    return pvkGetInstanceProcAddr(instance, name);
}

static void wine_vk_init(void)
{
    if (!(vulkan_handle = dlopen(SONAME_LIBVULKAN, RTLD_NOW)))
    {
        ERR("Failed to load %s.\n", SONAME_LIBVULKAN);
        return;
    }

#define LOAD_FUNCPTR(f) if (!(p##f = dlsym(vulkan_handle, #f))) goto fail
    LOAD_FUNCPTR(vkCreateInstance);
    LOAD_FUNCPTR(vkDestroyInstance);
    LOAD_FUNCPTR(vkEnumerateInstanceExtensionProperties);
    LOAD_FUNCPTR(vkGetDeviceProcAddr);
    LOAD_FUNCPTR(vkGetInstanceProcAddr);
#undef LOAD_FUNCPTR

    return;

fail:
    dlclose(vulkan_handle);
    vulkan_handle = NULL;
}

static const struct vulkan_funcs vulkan_funcs =
{
    .p_vkCreateInstance = wayland_vkCreateInstance,
    .p_vkDestroyInstance = wayland_vkDestroyInstance,
    .p_vkEnumerateInstanceExtensionProperties = wayland_vkEnumerateInstanceExtensionProperties,
    .p_vkGetDeviceProcAddr = wayland_vkGetDeviceProcAddr,
    .p_vkGetInstanceProcAddr = wayland_vkGetInstanceProcAddr,
};

/**********************************************************************
 *           WAYLAND_wine_get_vulkan_driver
 */
const struct vulkan_funcs *WAYLAND_wine_get_vulkan_driver(UINT version)
{
    static pthread_once_t init_once = PTHREAD_ONCE_INIT;

    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, vulkan wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return NULL;
    }

    pthread_once(&init_once, wine_vk_init);
    if (vulkan_handle)
        return &vulkan_funcs;

    return NULL;
}

#else /* No vulkan */

const struct vulkan_funcs *WAYLAND_wine_get_vulkan_driver(UINT version)
{
    ERR("Wine was built without Vulkan support.\n");
    return NULL;
}

#endif /* SONAME_LIBVULKAN */
