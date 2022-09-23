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
#define VK_NO_PROTOTYPES

#include <pthread.h>

#include "vulkan_loader.h"
#include "vulkan_thunks.h"

/* Some extensions have callbacks for those we need to be able to
 * get the wine wrapper for a native handle
 */
struct wine_vk_mapping
{
    struct list link;
    uint64_t native_handle;
    uint64_t wine_wrapped_handle;
};

struct wine_cmd_buffer
{
    struct wine_device *device; /* parent */

    VkCommandBuffer handle; /* client command buffer */
    VkCommandBuffer command_buffer; /* native command buffer */

    struct wine_vk_mapping mapping;
};

static inline struct wine_cmd_buffer *wine_cmd_buffer_from_handle(VkCommandBuffer handle)
{
    return (struct wine_cmd_buffer *)(uintptr_t)handle->base.unix_handle;
}

struct wine_device
{
    struct vulkan_device_funcs funcs;
    struct wine_phys_dev *phys_dev; /* parent */

    VkDevice handle; /* client device */
    VkDevice device; /* native device */

    struct wine_queue *queues;
    uint32_t queue_count;

    struct wine_vk_mapping mapping;
};

static inline struct wine_device *wine_device_from_handle(VkDevice handle)
{
    return (struct wine_device *)(uintptr_t)handle->base.unix_handle;
}

struct wine_debug_utils_messenger;

struct wine_debug_report_callback
{
    struct wine_instance *instance; /* parent */
    VkDebugReportCallbackEXT debug_callback; /* native callback object */

    /* application callback + data */
    PFN_vkDebugReportCallbackEXT user_callback;
    void *user_data;

    struct wine_vk_mapping mapping;
};

struct wine_instance
{
    struct vulkan_instance_funcs funcs;

    VkInstance handle; /* client instance */
    VkInstance instance; /* native instance */

    /* We cache devices as we need to wrap them as they are
     * dispatchable objects.
     */
    struct wine_phys_dev **phys_devs;
    uint32_t phys_dev_count;

    VkBool32 enable_wrapper_list;
    struct list wrappers;
    pthread_rwlock_t wrapper_lock;

    struct wine_debug_utils_messenger *utils_messengers;
    uint32_t utils_messenger_count;

    struct wine_debug_report_callback default_callback;

    unsigned int quirks;

    struct wine_vk_mapping mapping;
};

static inline struct wine_instance *wine_instance_from_handle(VkInstance handle)
{
    return (struct wine_instance *)(uintptr_t)handle->base.unix_handle;
}

struct wine_phys_dev
{
    struct wine_instance *instance; /* parent */

    VkPhysicalDevice handle; /* client physical device */
    VkPhysicalDevice phys_dev; /* native physical device */

    VkExtensionProperties *extensions;
    uint32_t extension_count;

    struct wine_vk_mapping mapping;
};

static inline struct wine_phys_dev *wine_phys_dev_from_handle(VkPhysicalDevice handle)
{
    return (struct wine_phys_dev *)(uintptr_t)handle->base.unix_handle;
}

struct wine_queue
{
    struct wine_device *device; /* parent */

    VkQueue handle; /* client queue */
    VkQueue queue; /* native queue */

    uint32_t family_index;
    uint32_t queue_index;
    VkDeviceQueueCreateFlags flags;

    struct wine_vk_mapping mapping;
};

static inline struct wine_queue *wine_queue_from_handle(VkQueue handle)
{
    return (struct wine_queue *)(uintptr_t)handle->base.unix_handle;
}

struct wine_cmd_pool
{
    VkCommandPool handle;
    VkCommandPool command_pool;

    struct wine_vk_mapping mapping;
};

static inline struct wine_cmd_pool *wine_cmd_pool_from_handle(VkCommandPool handle)
{
    struct vk_command_pool *client_ptr = command_pool_from_handle(handle);
    return (struct wine_cmd_pool *)(uintptr_t)client_ptr->unix_handle;
}

struct wine_debug_utils_messenger
{
    struct wine_instance *instance; /* parent */
    VkDebugUtilsMessengerEXT debug_messenger; /* native messenger */

    /* application callback + data */
    PFN_vkDebugUtilsMessengerCallbackEXT user_callback;
    void *user_data;

    struct wine_vk_mapping mapping;
};

static inline struct wine_debug_utils_messenger *wine_debug_utils_messenger_from_handle(
        VkDebugUtilsMessengerEXT handle)
{
    return (struct wine_debug_utils_messenger *)(uintptr_t)handle;
}

static inline VkDebugUtilsMessengerEXT wine_debug_utils_messenger_to_handle(
        struct wine_debug_utils_messenger *debug_messenger)
{
    return (VkDebugUtilsMessengerEXT)(uintptr_t)debug_messenger;
}

static inline struct wine_debug_report_callback *wine_debug_report_callback_from_handle(
        VkDebugReportCallbackEXT handle)
{
    return (struct wine_debug_report_callback *)(uintptr_t)handle;
}

static inline VkDebugReportCallbackEXT wine_debug_report_callback_to_handle(
        struct wine_debug_report_callback *debug_messenger)
{
    return (VkDebugReportCallbackEXT)(uintptr_t)debug_messenger;
}

struct wine_surface
{
    VkSurfaceKHR surface; /* native surface */
    VkSurfaceKHR driver_surface; /* wine driver surface */

    struct wine_vk_mapping mapping;
};

static inline struct wine_surface *wine_surface_from_handle(VkSurfaceKHR handle)
{
    return (struct wine_surface *)(uintptr_t)handle;
}

static inline VkSurfaceKHR wine_surface_to_handle(struct wine_surface *surface)
{
    return (VkSurfaceKHR)(uintptr_t)surface;
}

BOOL wine_vk_device_extension_supported(const char *name) DECLSPEC_HIDDEN;
BOOL wine_vk_instance_extension_supported(const char *name) DECLSPEC_HIDDEN;

BOOL wine_vk_is_type_wrapped(VkObjectType type) DECLSPEC_HIDDEN;
uint64_t wine_vk_unwrap_handle(VkObjectType type, uint64_t handle) DECLSPEC_HIDDEN;

NTSTATUS init_vulkan(void *args) DECLSPEC_HIDDEN;

NTSTATUS WINAPI vk_direct_unix_call(unixlib_handle_t handle, unsigned int code, void *arg) DECLSPEC_HIDDEN;

NTSTATUS vk_is_available_instance_function(void *arg) DECLSPEC_HIDDEN;
NTSTATUS vk_is_available_device_function(void *arg) DECLSPEC_HIDDEN;

struct conversion_context
{
    char buffer[2048];
    uint32_t used;
    struct list alloc_entries;
};

static inline void init_conversion_context(struct conversion_context *pool)
{
    pool->used = 0;
    list_init(&pool->alloc_entries);
}

static inline void free_conversion_context(struct conversion_context *pool)
{
    struct list *entry, *next;
    LIST_FOR_EACH_SAFE(entry, next, &pool->alloc_entries)
        free(entry);
}

static inline void *conversion_context_alloc(struct conversion_context *pool, size_t size)
{
    if (pool->used + size <= sizeof(pool->buffer))
    {
        void *ret = pool->buffer + pool->used;
        pool->used += size;
        return ret;
    }
    else
    {
        struct list *entry;
        if (!(entry = malloc(sizeof(*entry) + size)))
            return NULL;
        list_add_tail(&pool->alloc_entries, entry);
        return entry + 1;
    }
}

#endif /* __WINE_VULKAN_PRIVATE_H */
