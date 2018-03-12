/* X11DRV Vulkan implementation
 *
 * Copyright 2017 Roderick Colenbrander
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"
#include "wine/library.h"

/* We only want host compatible structures and don't need alignment. */
#define WINE_VK_ALIGN(x)

#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

#ifdef SONAME_LIBVULKAN

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

static VkResult (*pvkCreateInstance)(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *);
static void (*pvkDestroyInstance)(VkInstance, const VkAllocationCallbacks *);
static void * (*pvkGetDeviceProcAddr)(VkDevice, const char *);
static void * (*pvkGetInstanceProcAddr)(VkInstance, const char *);

/* TODO: dynamically generate based on host driver capabilities. */
static const struct VkExtensionProperties winex11_vk_instance_extensions[] =
{
    { "VK_KHR_surface", 1 },
    { "VK_KHR_win32_surface", 1},
};

static BOOL wine_vk_init(void)
{
    static BOOL init_done = FALSE;
    static void *vulkan_handle;

    if (init_done) return (vulkan_handle != NULL);
    init_done = TRUE;

    if (!(vulkan_handle = wine_dlopen(SONAME_LIBVULKAN, RTLD_NOW, NULL, 0))) return FALSE;

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(vulkan_handle, #f, NULL, 0)) == NULL) return FALSE;
LOAD_FUNCPTR(vkCreateInstance)
LOAD_FUNCPTR(vkDestroyInstance)
LOAD_FUNCPTR(vkGetDeviceProcAddr)
LOAD_FUNCPTR(vkGetInstanceProcAddr)
#undef LOAD_FUNCPTR

    return TRUE;
}

static VkResult X11DRV_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain,
        uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *index)
{
    FIXME("stub: %p, 0x%s, 0x%s, 0x%s, 0x%s, %p\n", device,
            wine_dbgstr_longlong(swapchain), wine_dbgstr_longlong(timeout),
            wine_dbgstr_longlong(semaphore), wine_dbgstr_longlong(fence), index);

    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult X11DRV_vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    TRACE("create_info %p, allocator %p, instance %p\n", create_info, allocator, instance);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    /* TODO: convert win32 to x11 extensions here. */
    if (create_info->enabledExtensionCount > 0)
    {
        FIXME("Extensions are not supported yet, aborting!\n");
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    return pvkCreateInstance(create_info, NULL /* allocator */, instance);
}

static VkResult X11DRV_vkCreateSwapchainKHR(VkDevice device,
        const VkSwapchainCreateInfoKHR *create_info,
        const VkAllocationCallbacks *allocator, VkSwapchainKHR *swapchain)
{
    FIXME("stub: %p %p %p %p\n", device, create_info, allocator, swapchain);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult X11DRV_vkCreateWin32SurfaceKHR(VkInstance instance,
        const VkWin32SurfaceCreateInfoKHR *create_info,
        const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    FIXME("stub: %p %p %p %p\n", instance, create_info, allocator, surface);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static void X11DRV_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *allocator)
{
    TRACE("%p %p\n", instance, allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    pvkDestroyInstance(instance, NULL /* allocator */);
}

static void X11DRV_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
        const VkAllocationCallbacks *allocator)
{
    FIXME("stub: %p 0x%s %p\n", instance, wine_dbgstr_longlong(surface), allocator);
}

static void X11DRV_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
         const VkAllocationCallbacks *allocator)
{
    FIXME("stub: %p, 0x%s %p\n", device, wine_dbgstr_longlong(swapchain), allocator);
}

static VkResult X11DRV_vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties* properties)
{
    VkResult res;
    unsigned int i, num_copies;

    TRACE("layer_name %p, count %p, properties %p\n", debugstr_a(layer_name), count, properties);

    /* This shouldn't get called with layer_name set, the ICD loader prevents it. */
    if (layer_name)
    {
        ERR("Layer enumeration not supported from ICD.\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    if (!properties)
    {
        /* When properties is NULL, we need to return the number of extensions
         * supported. For now report 0 until we add some e.g.
         * VK_KHR_win32_surface. Long-term this needs to be an intersection
         * between what the native library supports and what thunks we have.
         */
        *count = ARRAY_SIZE(winex11_vk_instance_extensions);
        return VK_SUCCESS;
    }

    if (*count < ARRAY_SIZE(winex11_vk_instance_extensions))
    {
        /* Incomplete is a type of success used to signal the application
         * that not all devices got copied.
         */
        num_copies = *count;
        res = VK_INCOMPLETE;
    }
    else
    {
        num_copies = ARRAY_SIZE(winex11_vk_instance_extensions);
        res = VK_SUCCESS;
    }

    for (i = 0; i < num_copies; i++)
    {
        memcpy(&properties[i], &winex11_vk_instance_extensions[i], sizeof(winex11_vk_instance_extensions[i]));
    }
    *count = num_copies;

    TRACE("Result %d, extensions copied %u\n", res, num_copies);
    return res;
}

static void * X11DRV_vkGetDeviceProcAddr(VkDevice device, const char *name)
{
    TRACE("%p, %s\n", device, debugstr_a(name));
    return pvkGetDeviceProcAddr(device, name);
}

static void * X11DRV_vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    TRACE("%p, %s\n", instance, debugstr_a(name));
    return pvkGetInstanceProcAddr(instance, name);
}

static VkResult X11DRV_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice phys_dev,
        VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *capabilities)
{
    FIXME("stub: %p, 0x%s, %p\n", phys_dev, wine_dbgstr_longlong(surface), capabilities);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult X11DRV_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice phys_dev,
        VkSurfaceKHR surface, uint32_t *count, VkSurfaceFormatKHR *formats)
{
    FIXME("stub: %p, 0x%s, %p, %p\n", phys_dev, wine_dbgstr_longlong(surface), count, formats);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult X11DRV_vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice phys_dev,
        VkSurfaceKHR surface, uint32_t *count, VkPresentModeKHR *modes)
{
    FIXME("stub: %p, 0x%s, %p, %p\n", phys_dev, wine_dbgstr_longlong(surface), count, modes);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult X11DRV_vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice phys_dev,
        uint32_t index, VkSurfaceKHR surface, VkBool32 *supported)
{
    FIXME("stub: %p, %u, 0x%s, %p\n", phys_dev, index, wine_dbgstr_longlong(surface), supported);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkBool32 X11DRV_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice phys_dev,
        uint32_t index)
{
    FIXME("stub %p %u\n", phys_dev, index);
    return VK_FALSE;
}

static VkResult X11DRV_vkGetSwapchainImagesKHR(VkDevice device,
        VkSwapchainKHR swapchain, uint32_t *count, VkImage *images)
{
    FIXME("stub: %p, 0x%s %p %p\n", device, wine_dbgstr_longlong(swapchain), count, images);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult X11DRV_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *present_info)
{
    FIXME("stub: %p, %p\n", queue, present_info);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}


static const struct vulkan_funcs vulkan_funcs =
{
    X11DRV_vkAcquireNextImageKHR,
    X11DRV_vkCreateInstance,
    X11DRV_vkCreateSwapchainKHR,
    X11DRV_vkCreateWin32SurfaceKHR,
    X11DRV_vkDestroyInstance,
    X11DRV_vkDestroySurfaceKHR,
    X11DRV_vkDestroySwapchainKHR,
    X11DRV_vkEnumerateInstanceExtensionProperties,
    X11DRV_vkGetDeviceProcAddr,
    X11DRV_vkGetInstanceProcAddr,
    X11DRV_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
    X11DRV_vkGetPhysicalDeviceSurfaceFormatsKHR,
    X11DRV_vkGetPhysicalDeviceSurfacePresentModesKHR,
    X11DRV_vkGetPhysicalDeviceSurfaceSupportKHR,
    X11DRV_vkGetPhysicalDeviceWin32PresentationSupportKHR,
    X11DRV_vkGetSwapchainImagesKHR,
    X11DRV_vkQueuePresentKHR
};

const struct vulkan_funcs *get_vulkan_driver(UINT version)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, vulkan wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return NULL;
    }

    if (wine_vk_init())
        return &vulkan_funcs;

    return NULL;
}

#else /* No vulkan */

const struct vulkan_funcs *get_vulkan_driver(UINT version)
{
    return NULL;
}

#endif /* SONAME_LIBVULKAN */
