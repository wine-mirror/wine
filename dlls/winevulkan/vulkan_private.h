/* Wine Vulkan ICD private data structures
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

#ifndef __WINE_VULKAN_PRIVATE_H
#define __WINE_VULKAN_PRIVATE_H

#include "vulkan_thunks.h"

/* Magic value defined by Vulkan ICD / Loader spec */
#define VULKAN_ICD_MAGIC_VALUE 0x01CDC0DE

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

struct vulkan_func
{
    const char *name;
    void *func;
};

/* Base 'class' for our Vulkan dispatchable objects such as VkDevice and VkInstance.
 * This structure MUST be the first element of a dispatchable object as the ICD
 * loader depends on it. For now only contains loader_magic, but over time more common
 * functionality is expected.
 */
struct wine_vk_base
{
    /* Special section in each dispatchable object for use by the ICD loader for
     * storing dispatch tables. The start contains a magical value '0x01CDC0DE'.
     */
    UINT_PTR loader_magic;
};

struct VkDevice_T
{
    struct wine_vk_base base;
    struct vulkan_device_funcs funcs;
    struct VkPhysicalDevice_T *phys_dev; /* parent */
    VkDevice device; /* native device */
};

struct VkInstance_T
{
    struct wine_vk_base base;
    struct vulkan_instance_funcs funcs;

    /* We cache devices as we need to wrap them as they are
     * dispatchable objects.
     */
    uint32_t num_phys_devs;
    struct VkPhysicalDevice_T **phys_devs;

    VkInstance instance; /* native instance */
};

struct VkPhysicalDevice_T
{
    struct wine_vk_base base;
    struct VkInstance_T *instance; /* parent */

    uint32_t num_properties;
    VkExtensionProperties *properties;

    VkPhysicalDevice phys_dev; /* native physical device */
};

#endif /* __WINE_VULKAN_PRIVATE_H */
