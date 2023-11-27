/* Mac Driver Vulkan implementation
 *
 * Copyright 2017 Roderick Colenbrander
 * Copyright 2018 Andrew Eikum for CodeWeavers
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

/* NOTE: If making changes here, consider whether they should be reflected in
 * the other drivers. */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>

#include "macdrv.h"
#include "wine/debug.h"

#define VK_NO_PROTOTYPES
#define WINE_VK_HOST

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#ifdef SONAME_LIBMOLTENVK

WINE_DECLARE_DEBUG_CHANNEL(fps);

typedef VkFlags VkMacOSSurfaceCreateFlagsMVK;
#define VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK 1000123000

typedef VkFlags VkMetalSurfaceCreateFlagsEXT;
#define VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT 1000217000

struct wine_vk_surface
{
    macdrv_metal_device device;
    macdrv_metal_view view;
    VkSurfaceKHR host_surface; /* host surface */
};

typedef struct VkMacOSSurfaceCreateInfoMVK
{
    VkStructureType sType;
    const void *pNext;
    VkMacOSSurfaceCreateFlagsMVK flags;
    const void *pView; /* NSView */
} VkMacOSSurfaceCreateInfoMVK;

typedef struct VkMetalSurfaceCreateInfoEXT
{
    VkStructureType sType;
    const void *pNext;
    VkMetalSurfaceCreateFlagsEXT flags;
    const void *pLayer; /* CAMetalLayer */
} VkMetalSurfaceCreateInfoEXT;

static VkResult (*pvkCreateInstance)(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *);
static VkResult (*pvkCreateSwapchainKHR)(VkDevice, const VkSwapchainCreateInfoKHR *, const VkAllocationCallbacks *, VkSwapchainKHR *);
static VkResult (*pvkCreateMacOSSurfaceMVK)(VkInstance, const VkMacOSSurfaceCreateInfoMVK*, const VkAllocationCallbacks *, VkSurfaceKHR *);
static VkResult (*pvkCreateMetalSurfaceEXT)(VkInstance, const VkMetalSurfaceCreateInfoEXT*, const VkAllocationCallbacks *, VkSurfaceKHR *);
static void (*pvkDestroyInstance)(VkInstance, const VkAllocationCallbacks *);
static void (*pvkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *);
static void (*pvkDestroySwapchainKHR)(VkDevice, VkSwapchainKHR, const VkAllocationCallbacks *);
static VkResult (*pvkEnumerateInstanceExtensionProperties)(const char *, uint32_t *, VkExtensionProperties *);
static void * (*pvkGetDeviceProcAddr)(VkDevice, const char *);
static void * (*pvkGetInstanceProcAddr)(VkInstance, const char *);
static VkResult (*pvkGetPhysicalDeviceSurfaceCapabilities2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, VkSurfaceCapabilities2KHR *);
static VkResult (*pvkGetPhysicalDeviceSurfaceCapabilitiesKHR)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR *);
static VkResult (*pvkGetPhysicalDeviceSurfaceFormats2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, uint32_t *, VkSurfaceFormat2KHR *);
static VkResult (*pvkGetPhysicalDeviceSurfaceFormatsKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkSurfaceFormatKHR *);
static VkResult (*pvkGetSwapchainImagesKHR)(VkDevice, VkSwapchainKHR, uint32_t *, VkImage *);
static VkResult (*pvkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *);

static void *macdrv_get_vk_device_proc_addr(const char *name);
static void *macdrv_get_vk_instance_proc_addr(VkInstance instance, const char *name);

static inline struct wine_vk_surface *surface_from_handle(VkSurfaceKHR handle)
{
    return (struct wine_vk_surface *)(uintptr_t)handle;
}

static void *vulkan_handle;

static void wine_vk_init(void)
{
    if (!(vulkan_handle = dlopen(SONAME_LIBMOLTENVK, RTLD_NOW)))
    {
        ERR("Failed to load %s\n", SONAME_LIBMOLTENVK);
        return;
    }

#define LOAD_FUNCPTR(f) if ((p##f = dlsym(vulkan_handle, #f)) == NULL) goto fail;
    LOAD_FUNCPTR(vkCreateInstance)
    LOAD_FUNCPTR(vkCreateSwapchainKHR)
    LOAD_FUNCPTR(vkCreateMacOSSurfaceMVK)
    LOAD_FUNCPTR(vkCreateMetalSurfaceEXT)
    LOAD_FUNCPTR(vkDestroyInstance)
    LOAD_FUNCPTR(vkDestroySurfaceKHR)
    LOAD_FUNCPTR(vkDestroySwapchainKHR)
    LOAD_FUNCPTR(vkEnumerateInstanceExtensionProperties)
    LOAD_FUNCPTR(vkGetDeviceProcAddr)
    LOAD_FUNCPTR(vkGetInstanceProcAddr)
    LOAD_FUNCPTR(vkGetPhysicalDeviceSurfaceCapabilities2KHR)
    LOAD_FUNCPTR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    LOAD_FUNCPTR(vkGetPhysicalDeviceSurfaceFormats2KHR)
    LOAD_FUNCPTR(vkGetPhysicalDeviceSurfaceFormatsKHR)
    LOAD_FUNCPTR(vkGetSwapchainImagesKHR)
    LOAD_FUNCPTR(vkQueuePresentKHR)
#undef LOAD_FUNCPTR

    return;

fail:
    dlclose(vulkan_handle);
    vulkan_handle = NULL;
}

/* Helper function for converting between win32 and MoltenVK compatible VkInstanceCreateInfo.
 * Caller is responsible for allocation and cleanup of 'dst'.
 */
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
        enabled_extensions = calloc(src->enabledExtensionCount, sizeof(*src->ppEnabledExtensionNames));
        if (!enabled_extensions)
        {
            ERR("Failed to allocate memory for enabled extensions\n");
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        for (i = 0; i < src->enabledExtensionCount; i++)
        {
            /* Substitute extension with MoltenVK ones else copy. Long-term, when we
             * support more extensions, we should store these in a list.
             */
            if (!strcmp(src->ppEnabledExtensionNames[i], "VK_KHR_win32_surface"))
            {
                enabled_extensions[i] = pvkCreateMetalSurfaceEXT ? "VK_EXT_metal_surface" : "VK_MVK_macos_surface";
            }
            else
            {
                enabled_extensions[i] = src->ppEnabledExtensionNames[i];
            }
        }
        dst->ppEnabledExtensionNames = enabled_extensions;
        dst->enabledExtensionCount = src->enabledExtensionCount;
    }

    return VK_SUCCESS;
}

static void wine_vk_surface_destroy(VkInstance instance, struct wine_vk_surface *surface)
{
    /* vkDestroySurfaceKHR must handle VK_NULL_HANDLE (0) for surface. */
    if (!surface)
        return;

    pvkDestroySurfaceKHR(instance, surface->host_surface, NULL /* allocator */);

    if (surface->view)
        macdrv_view_release_metal_view(surface->view);

    if (surface->device)
        macdrv_release_metal_device(surface->device);

    free(surface);
}

static VkResult macdrv_vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    VkInstanceCreateInfo create_info_host;
    VkResult res;
    TRACE("create_info %p, allocator %p, instance %p\n", create_info, allocator, instance);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    /* Perform a second pass on converting VkInstanceCreateInfo. Winevulkan
     * performed a first pass in which it handles everything except for WSI
     * functionality such as VK_KHR_win32_surface. Handle this now.
     */
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

static VkResult macdrv_vkCreateSwapchainKHR(VkDevice device,
        const VkSwapchainCreateInfoKHR *create_info,
        const VkAllocationCallbacks *allocator, VkSwapchainKHR *swapchain)
{
    VkSwapchainCreateInfoKHR create_info_host;
    TRACE("%p %p %p %p\n", device, create_info, allocator, swapchain);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    create_info_host = *create_info;
    create_info_host.surface = surface_from_handle(create_info->surface)->host_surface;

    return pvkCreateSwapchainKHR(device, &create_info_host, NULL /* allocator */,
            swapchain);
}

static VkResult macdrv_vkCreateWin32SurfaceKHR(VkInstance instance,
        const VkWin32SurfaceCreateInfoKHR *create_info,
        const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    VkResult res;
    struct wine_vk_surface *mac_surface;
    struct macdrv_win_data *data;

    TRACE("%p %p %p %p\n", instance, create_info, allocator, surface);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(data = get_win_data(create_info->hwnd)))
    {
        FIXME("DC for window %p of other process: not implemented\n", create_info->hwnd);
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    mac_surface = calloc(1, sizeof(*mac_surface));
    if (!mac_surface)
    {
        release_win_data(data);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    mac_surface->device = macdrv_create_metal_device();
    if (!mac_surface->device)
    {
        ERR("Failed to allocate Metal device for hwnd=%p\n", create_info->hwnd);
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    mac_surface->view = macdrv_view_create_metal_view(data->client_cocoa_view, mac_surface->device);
    if (!mac_surface->view)
    {
        ERR("Failed to allocate Metal view for hwnd=%p\n", create_info->hwnd);

        /* VK_KHR_win32_surface only allows out of host and device memory as errors. */
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    if (pvkCreateMetalSurfaceEXT)
    {
        VkMetalSurfaceCreateInfoEXT create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pLayer = macdrv_view_get_metal_layer(mac_surface->view);

        res = pvkCreateMetalSurfaceEXT(instance, &create_info_host, NULL /* allocator */, &mac_surface->host_surface);
    }
    else
    {
        VkMacOSSurfaceCreateInfoMVK create_info_host;
        create_info_host.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        create_info_host.pNext = NULL;
        create_info_host.flags = 0; /* reserved */
        create_info_host.pView = macdrv_view_get_metal_layer(mac_surface->view);

        res = pvkCreateMacOSSurfaceMVK(instance, &create_info_host, NULL /* allocator */, &mac_surface->host_surface);
    }
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create MoltenVK surface, res=%d\n", res);
        goto err;
    }

    *surface = (uintptr_t)mac_surface;

    release_win_data(data);

    TRACE("Created surface=0x%s\n", wine_dbgstr_longlong(*surface));
    return VK_SUCCESS;

err:
    wine_vk_surface_destroy(instance, mac_surface);
    release_win_data(data);
    return res;
}

static void macdrv_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *allocator)
{
    TRACE("%p %p\n", instance, allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    pvkDestroyInstance(instance, NULL /* allocator */);
}

static void macdrv_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
        const VkAllocationCallbacks *allocator)
{
    struct wine_vk_surface *mac_surface = surface_from_handle(surface);

    TRACE("%p 0x%s %p\n", instance, wine_dbgstr_longlong(surface), allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    wine_vk_surface_destroy(instance, mac_surface);
}

static void macdrv_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
         const VkAllocationCallbacks *allocator)
{
    TRACE("%p, 0x%s %p\n", device, wine_dbgstr_longlong(swapchain), allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    pvkDestroySwapchainKHR(device, swapchain, NULL /* allocator */);
}

static VkResult macdrv_vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties* properties)
{
    unsigned int i;
    BOOL seen_surface = FALSE;
    VkResult res;

    TRACE("layer_name %s, count %p, properties %p\n", debugstr_a(layer_name), count, properties);

    /* This shouldn't get called with layer_name set, the ICD loader prevents it. */
    if (layer_name)
    {
        ERR("Layer enumeration not supported from ICD.\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    /* We will return at most the same number of instance extensions reported by the host back to
     * winevulkan. Along the way we may replace MoltenVK extensions with their win32 equivalents,
     * or remove redundant extensions outright.
     * Winevulkan will perform more detailed filtering as it knows whether it has thunks
     * for a particular extension.
     */
    res = pvkEnumerateInstanceExtensionProperties(layer_name, count, properties);
    if (!properties || res < 0)
        return res;

    for (i = 0; i < *count; i++)
    {
        /* For now the only MoltenVK extensions we need to fixup. Long-term we may need an array. */
        if (!strcmp(properties[i].extensionName, "VK_MVK_macos_surface") ||
            !strcmp(properties[i].extensionName, "VK_EXT_metal_surface"))
        {
            if (seen_surface)
            {
                /* If we've already seen a surface extension, just hide this one. */
                memmove(properties + i, properties + i + 1, (*count - i - 1) * sizeof(*properties));
                --*count;
                --i;
                continue;
            }
            TRACE("Substituting %s for VK_KHR_win32_surface\n", properties[i].extensionName);

            snprintf(properties[i].extensionName, sizeof(properties[i].extensionName),
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            properties[i].specVersion = VK_KHR_WIN32_SURFACE_SPEC_VERSION;
            seen_surface = TRUE;
        }
    }

    TRACE("Returning %u extensions.\n", *count);
    return res;
}

static const char *wine_vk_host_fn_name(const char *name)
{
    const char *create_surface_name =
        pvkCreateMetalSurfaceEXT ? "vkCreateMetalSurfaceEXT" : "vkCreateMacOSSurfaceMVK";

    if (!strcmp(name, "vkCreateWin32SurfaceKHR"))
        return create_surface_name;
    /* We just need something where non-NULL is returned if the correct extension is enabled.
     * So since there is no host equivalent of this function check for the create
     * surface function.
     */
    if (!strcmp(name, "vkGetPhysicalDeviceWin32PresentationSupportKHR"))
        return create_surface_name;

    return name;
}

static void *macdrv_vkGetDeviceProcAddr(VkDevice device, const char *name)
{
    void *proc_addr;

    TRACE("%p, %s\n", device, debugstr_a(name));

    if (!pvkGetDeviceProcAddr(device, wine_vk_host_fn_name(name)))
        return NULL;

    if ((proc_addr = macdrv_get_vk_device_proc_addr(name)))
        return proc_addr;

    return pvkGetDeviceProcAddr(device, name);
}

static void *macdrv_vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    void *proc_addr;

    TRACE("%p, %s\n", instance, debugstr_a(name));

    if (!pvkGetInstanceProcAddr(instance, wine_vk_host_fn_name(name)))
        return NULL;

    if ((proc_addr = macdrv_get_vk_instance_proc_addr(instance, name)))
        return proc_addr;

    return pvkGetInstanceProcAddr(instance, name);
}

static VkResult macdrv_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceSurfaceInfo2KHR *surface_info, VkSurfaceCapabilities2KHR *capabilities)
{
    VkPhysicalDeviceSurfaceInfo2KHR surface_info_host;

    TRACE("%p, %p, %p\n", phys_dev, surface_info, capabilities);

    surface_info_host = *surface_info;
    surface_info_host.surface = surface_from_handle(surface_info->surface)->host_surface;

    return pvkGetPhysicalDeviceSurfaceCapabilities2KHR(phys_dev, &surface_info_host, capabilities);
}

static VkResult macdrv_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice phys_dev,
        VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *capabilities)
{
    struct wine_vk_surface *mac_surface = surface_from_handle(surface);

    TRACE("%p, 0x%s, %p\n", phys_dev, wine_dbgstr_longlong(surface), capabilities);

    return pvkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, mac_surface->host_surface,
            capabilities);
}

static VkResult macdrv_vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceSurfaceInfo2KHR *surface_info, uint32_t *count, VkSurfaceFormat2KHR *formats)
{
    VkPhysicalDeviceSurfaceInfo2KHR surface_info_host;

    TRACE("%p, %p, %p, %p\n", phys_dev, surface_info, count, formats);

    surface_info_host = *surface_info;
    surface_info_host.surface = surface_from_handle(surface_info->surface)->host_surface;

    return pvkGetPhysicalDeviceSurfaceFormats2KHR(phys_dev, &surface_info_host, count, formats);
}

static VkResult macdrv_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice phys_dev,
        VkSurfaceKHR surface, uint32_t *count, VkSurfaceFormatKHR *formats)
{
    struct wine_vk_surface *mac_surface = surface_from_handle(surface);

    TRACE("%p, 0x%s, %p, %p\n", phys_dev, wine_dbgstr_longlong(surface), count, formats);

    return pvkGetPhysicalDeviceSurfaceFormatsKHR(phys_dev, mac_surface->host_surface,
            count, formats);
}

static VkBool32 macdrv_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice phys_dev,
        uint32_t index)
{
    TRACE("%p %u\n", phys_dev, index);

    return VK_TRUE;
}

static VkResult macdrv_vkGetSwapchainImagesKHR(VkDevice device,
        VkSwapchainKHR swapchain, uint32_t *count, VkImage *images)
{
    TRACE("%p, 0x%s %p %p\n", device, wine_dbgstr_longlong(swapchain), count, images);
    return pvkGetSwapchainImagesKHR(device, swapchain, count, images);
}

static VkResult macdrv_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *present_info)
{
    TRACE("%p, %p\n", queue, present_info);
    VkResult res = pvkQueuePresentKHR(queue, present_info);

    if (TRACE_ON(fps))
    {
        static unsigned long frames, frames_total;
        static long prev_time, start_time;
        DWORD time;

        time = NtGetTickCount();
        frames++;
        frames_total++;
        if (time - prev_time > 1500)
        {
            TRACE_(fps)("%p @ approx %.2ffps, total %.2ffps\n",
                    queue, 1000.0 * frames / (time - prev_time),
                    1000.0 * frames_total / (time - start_time));
            prev_time = time;
            frames = 0;
            if (!start_time)
                start_time = time;
        }
    }

    return res;
}

static VkSurfaceKHR macdrv_wine_get_host_surface(VkSurfaceKHR surface)
{
    struct wine_vk_surface *mac_surface = surface_from_handle(surface);

    TRACE("0x%s\n", wine_dbgstr_longlong(surface));

    return mac_surface->host_surface;
}

static const struct vulkan_funcs vulkan_funcs =
{
    macdrv_vkCreateInstance,
    macdrv_vkCreateSwapchainKHR,
    macdrv_vkCreateWin32SurfaceKHR,
    macdrv_vkDestroyInstance,
    macdrv_vkDestroySurfaceKHR,
    macdrv_vkDestroySwapchainKHR,
    macdrv_vkEnumerateInstanceExtensionProperties,
    macdrv_vkGetDeviceProcAddr,
    macdrv_vkGetInstanceProcAddr,
    NULL,
    macdrv_vkGetPhysicalDeviceSurfaceCapabilities2KHR,
    macdrv_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    macdrv_vkGetPhysicalDeviceSurfaceFormats2KHR,
    macdrv_vkGetPhysicalDeviceSurfaceFormatsKHR,
    macdrv_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    macdrv_vkGetSwapchainImagesKHR,
    macdrv_vkQueuePresentKHR,

    macdrv_wine_get_host_surface,
};

static void *macdrv_get_vk_device_proc_addr(const char *name)
{
    return get_vulkan_driver_device_proc_addr(&vulkan_funcs, name);
}

static void *macdrv_get_vk_instance_proc_addr(VkInstance instance, const char *name)
{
    return get_vulkan_driver_instance_proc_addr(&vulkan_funcs, instance, name);
}

static const struct vulkan_funcs *get_vulkan_driver(UINT version)
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

static const struct vulkan_funcs *get_vulkan_driver(UINT version)
{
    ERR("Wine was built without Vulkan support.\n");
    return NULL;
}

#endif /* SONAME_LIBMOLTENVK */

const struct vulkan_funcs *macdrv_wine_get_vulkan_driver(UINT version)
{
    return get_vulkan_driver( version );
}
