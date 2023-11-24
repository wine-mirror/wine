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

#ifndef __WINE_VULKAN_LOADER_H
#define __WINE_VULKAN_LOADER_H

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/vulkan.h"
#include "wine/unixlib.h"
#include "wine/list.h"

#include "loader_thunks.h"

/* Magic value defined by Vulkan ICD / Loader spec */
#define VULKAN_ICD_MAGIC_VALUE 0x01CDC0DE

#define WINEVULKAN_QUIRK_GET_DEVICE_PROC_ADDR 0x00000001
#define WINEVULKAN_QUIRK_ADJUST_MAX_IMAGE_COUNT 0x00000002

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
    UINT64 loader_magic;
    UINT64 unix_handle;
};

struct VkPhysicalDevice_T
{
    struct wine_vk_base base;
};

struct VkInstance_T
{
    struct wine_vk_base base;
    uint32_t phys_dev_count;
    struct VkPhysicalDevice_T phys_devs[1];
};

struct VkQueue_T
{
    struct wine_vk_base base;
};

struct VkDevice_T
{
    struct wine_vk_base base;
    unsigned int quirks;
    struct VkQueue_T queues[1];
};

struct vk_command_pool
{
    UINT64 unix_handle;
    struct list command_buffers;
};

static inline struct vk_command_pool *command_pool_from_handle(VkCommandPool handle)
{
    return (struct vk_command_pool *)(uintptr_t)handle;
}

struct VkCommandBuffer_T
{
    struct wine_vk_base base;
    struct list pool_link;
};

struct vulkan_func
{
    const char *name;
    void *func;
};

void *wine_vk_get_device_proc_addr(const char *name);
void *wine_vk_get_phys_dev_proc_addr(const char *name);
void *wine_vk_get_instance_proc_addr(const char *name);

/* debug callbacks params */

struct wine_vk_debug_utils_params
{
    PFN_vkDebugUtilsMessengerCallbackEXT user_callback;
    void *user_data;

    VkDebugUtilsMessageSeverityFlagBitsEXT severity;
    VkDebugUtilsMessageTypeFlagsEXT message_types;
    VkDebugUtilsMessengerCallbackDataEXT data;
};

struct wine_vk_debug_report_params
{
    PFN_vkDebugReportCallbackEXT user_callback;
    void *user_data;

    VkDebugReportFlagsEXT flags;
    VkDebugReportObjectTypeEXT object_type;
    uint64_t object_handle;
    size_t location;
    int32_t code;
    const char *layer_prefix;
    const char *message;
};

struct is_available_instance_function_params
{
    VkInstance instance;
    const char *name;
};

struct is_available_device_function_params
{
    VkDevice device;
    const char *name;
};

#define UNIX_CALL(code, params) WINE_UNIX_CALL(unix_ ## code, params)

#endif /* __WINE_VULKAN_LOADER_H */
