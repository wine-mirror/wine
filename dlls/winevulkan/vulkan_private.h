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

/* Perform vulkan struct conversion on 32-bit x86 platforms. */
#if defined(__i386__)
#define USE_STRUCT_CONVERSION
#endif

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"
#define VK_NO_PROTOTYPES
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"

#include "vulkan_thunks.h"

/* Magic value defined by Vulkan ICD / Loader spec */
#define VULKAN_ICD_MAGIC_VALUE 0x01CDC0DE

#define WINEVULKAN_QUIRK_GET_DEVICE_PROC_ADDR 0x00000001
#define WINEVULKAN_QUIRK_ADJUST_MAX_IMAGE_COUNT 0x00000002
#define WINEVULKAN_QUIRK_IGNORE_EXPLICIT_LAYERS 0x00000004

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

struct VkCommandBuffer_T
{
    struct wine_vk_base base;
    struct VkDevice_T *device; /* parent */
    VkCommandBuffer command_buffer; /* native command buffer */

    struct list pool_link;
};

struct VkDevice_T
{
    struct wine_vk_base base;
    struct vulkan_device_funcs funcs;
    VkDevice device; /* native device */

    struct VkQueue_T **queues;
    uint32_t max_queue_families;

    unsigned int quirks;
};

struct VkInstance_T
{
    struct wine_vk_base base;
    struct vulkan_instance_funcs funcs;
    VkInstance instance; /* native instance */

    /* We cache devices as we need to wrap them as they are
     * dispatchable objects.
     */
    struct VkPhysicalDevice_T **phys_devs;
    uint32_t phys_dev_count;

    unsigned int quirks;
};

struct VkPhysicalDevice_T
{
    struct wine_vk_base base;
    struct VkInstance_T *instance; /* parent */
    VkPhysicalDevice phys_dev; /* native physical device */

    VkExtensionProperties *extensions;
    uint32_t extension_count;
};

struct VkQueue_T
{
    struct wine_vk_base base;
    struct VkDevice_T *device; /* parent */
    VkQueue queue; /* native queue */

    VkDeviceQueueCreateFlags flags;
};

struct wine_cmd_pool
{
    VkCommandPool command_pool;

    struct list command_buffers;
};

static inline struct wine_cmd_pool *wine_cmd_pool_from_handle(VkCommandPool handle)
{
    return (struct wine_cmd_pool *)(uintptr_t)handle;
}

static inline VkCommandPool wine_cmd_pool_to_handle(struct wine_cmd_pool *cmd_pool)
{
    return (VkCommandPool)(uintptr_t)cmd_pool;
}

void *wine_vk_get_device_proc_addr(const char *name) DECLSPEC_HIDDEN;
void *wine_vk_get_instance_proc_addr(const char *name) DECLSPEC_HIDDEN;

BOOL wine_vk_device_extension_supported(const char *name) DECLSPEC_HIDDEN;
BOOL wine_vk_instance_extension_supported(const char *name) DECLSPEC_HIDDEN;

#endif /* __WINE_VULKAN_PRIVATE_H */
