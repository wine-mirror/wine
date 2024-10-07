/*
 * Copyright 2017-2018 Roderick Colenbrander
 * Copyright 2022 Jacek Caban for CodeWeavers
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

#ifndef __WINE_VULKAN_DRIVER_H
#define __WINE_VULKAN_DRIVER_H

/* Wine internal vulkan driver version, needs to be bumped upon vulkan_funcs changes. */
#define WINE_VULKAN_DRIVER_VERSION 35

struct vulkan_funcs
{
    /* Vulkan global functions. These are the only calls at this point a graphics driver
     * needs to provide. Other function calls will be provided indirectly by dispatch
     * tables part of dispatchable Vulkan objects such as VkInstance or vkDevice.
     */
    VkResult (*p_vkCreateWin32SurfaceKHR)(VkInstance, const VkWin32SurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
    void (*p_vkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *);
    void * (*p_vkGetDeviceProcAddr)(VkDevice, const char *);
    void * (*p_vkGetInstanceProcAddr)(VkInstance, const char *);
    VkBool32 (*p_vkGetPhysicalDeviceWin32PresentationSupportKHR)(VkPhysicalDevice, uint32_t);
    VkResult (*p_vkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *, VkSurfaceKHR *surfaces);

    /* winevulkan specific functions */
    const char *(*p_get_host_surface_extension)(void);
    VkSurfaceKHR (*p_wine_get_host_surface)(VkSurfaceKHR);
    void (*p_vulkan_surface_update)(VkSurfaceKHR);
};

/* interface between win32u and the user drivers */
struct vulkan_driver_funcs
{
    VkResult (*p_vulkan_surface_create)(HWND, VkInstance, VkSurfaceKHR *, void **);
    void (*p_vulkan_surface_destroy)(HWND, void *);
    void (*p_vulkan_surface_detach)(HWND, void *);
    void (*p_vulkan_surface_update)(HWND, void *);
    void (*p_vulkan_surface_presented)(HWND, void *, VkResult);

    VkBool32 (*p_vkGetPhysicalDeviceWin32PresentationSupportKHR)(VkPhysicalDevice, uint32_t);
    const char *(*p_get_host_surface_extension)(void);
};

#endif /* __WINE_VULKAN_DRIVER_H */
