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

#include "wine/debug.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static VkResult X11DRV_vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    FIXME("stub: %p %p %p\n", create_info, allocator, instance);
    return VK_ERROR_INCOMPATIBLE_DRIVER;
}

static void X11DRV_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *allocator)
{
    FIXME("stub: %p %p\n", instance, allocator);
}

static VkResult X11DRV_vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties* properties)
{
    TRACE("layer_name %s, count %p, properties %p\n", debugstr_a(layer_name), count, properties);

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
        *count = 0;
        return VK_SUCCESS;
    }

    /* When properties is not NULL, we copy the extensions over and set count
     * to the number of copied extensions. For now we don't have much to do as
     * we don't support any extensions yet.
     */
    *count = 0;
    return VK_SUCCESS;
}

static void * X11DRV_vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    FIXME("stub: %p, %s\n", instance, debugstr_a(name));
    return NULL;
}

static struct vulkan_funcs vulkan_funcs =
{
    X11DRV_vkCreateInstance,
    X11DRV_vkDestroyInstance,
    X11DRV_vkEnumerateInstanceExtensionProperties,
    X11DRV_vkGetInstanceProcAddr
};

const struct vulkan_funcs *get_vulkan_driver(UINT version)
{
    if (version != WINE_VULKAN_DRIVER_VERSION)
    {
        ERR("version mismatch, vulkan wants %u but driver has %u\n", version, WINE_VULKAN_DRIVER_VERSION);
        return NULL;
    }

    return &vulkan_funcs;
}
