/* Wine Vulkan ICD implementation
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winternl.h"

#include "vulkan_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

#define wine_vk_find_struct(s, t) wine_vk_find_struct_((void *)s, VK_STRUCTURE_TYPE_##t)
static void *wine_vk_find_struct_(void *s, VkStructureType t)
{
    VkBaseOutStructure *header;

    for (header = s; header; header = header->pNext)
    {
        if (header->sType == t)
            return header;
    }

    return NULL;
}

#define wine_vk_count_struct(s, t) wine_vk_count_struct_((void *)s, VK_STRUCTURE_TYPE_##t)
static uint32_t wine_vk_count_struct_(void *s, VkStructureType t)
{
    const VkBaseInStructure *header;
    uint32_t result = 0;

    for (header = s; header; header = header->pNext)
    {
        if (header->sType == t)
            result++;
    }

    return result;
}

static const struct vulkan_funcs *vk_funcs;
static VkResult (*p_vkEnumerateInstanceVersion)(uint32_t *version);

void WINAPI wine_vkGetPhysicalDeviceProperties(VkPhysicalDevice physical_device,
        VkPhysicalDeviceProperties *properties);

#define WINE_VK_ADD_DISPATCHABLE_MAPPING(instance, object, native_handle) \
    wine_vk_add_handle_mapping((instance), (uint64_t) (uintptr_t) (object), (uint64_t) (uintptr_t) (native_handle), &(object)->mapping)
#define WINE_VK_ADD_NON_DISPATCHABLE_MAPPING(instance, object, native_handle) \
    wine_vk_add_handle_mapping((instance), (uint64_t) (uintptr_t) (object), (uint64_t) (native_handle), &(object)->mapping)
static void  wine_vk_add_handle_mapping(struct VkInstance_T *instance, uint64_t wrapped_handle,
        uint64_t native_handle, struct wine_vk_mapping *mapping)
{
    if (instance->enable_wrapper_list)
    {
        mapping->native_handle = native_handle;
        mapping->wine_wrapped_handle = wrapped_handle;
        pthread_rwlock_wrlock(&instance->wrapper_lock);
        list_add_tail(&instance->wrappers, &mapping->link);
        pthread_rwlock_unlock(&instance->wrapper_lock);
    }
}

#define WINE_VK_REMOVE_HANDLE_MAPPING(instance, object) \
    wine_vk_remove_handle_mapping((instance), &(object)->mapping)
static void wine_vk_remove_handle_mapping(struct VkInstance_T *instance, struct wine_vk_mapping *mapping)
{
    if (instance->enable_wrapper_list)
    {
        pthread_rwlock_wrlock(&instance->wrapper_lock);
        list_remove(&mapping->link);
        pthread_rwlock_unlock(&instance->wrapper_lock);
    }
}

static uint64_t wine_vk_get_wrapper(struct VkInstance_T *instance, uint64_t native_handle)
{
    struct wine_vk_mapping *mapping;
    uint64_t result = 0;

    pthread_rwlock_rdlock(&instance->wrapper_lock);
    LIST_FOR_EACH_ENTRY(mapping, &instance->wrappers, struct wine_vk_mapping, link)
    {
        if (mapping->native_handle == native_handle)
        {
            result = mapping->wine_wrapped_handle;
            break;
        }
    }
    pthread_rwlock_unlock(&instance->wrapper_lock);
    return result;
}

static VkBool32 debug_utils_callback_conversion(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
#if defined(USE_STRUCT_CONVERSION)
    const VkDebugUtilsMessengerCallbackDataEXT_host *callback_data,
#else
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
#endif
    void *user_data)
{
    struct VkDebugUtilsMessengerCallbackDataEXT wine_callback_data;
    VkDebugUtilsObjectNameInfoEXT *object_name_infos;
    struct wine_debug_utils_messenger *object;
    VkBool32 result;
    unsigned int i;

    TRACE("%i, %u, %p, %p\n", severity, message_types, callback_data, user_data);

    object = user_data;

    if (!object->instance->instance)
    {
        /* instance wasn't yet created, this is a message from the native loader */
        return VK_FALSE;
    }

    wine_callback_data = *((VkDebugUtilsMessengerCallbackDataEXT *) callback_data);

    object_name_infos = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        wine_callback_data.objectCount * sizeof(*object_name_infos));

    for (i = 0; i < wine_callback_data.objectCount; i++)
    {
        object_name_infos[i].sType = callback_data->pObjects[i].sType;
        object_name_infos[i].pNext = callback_data->pObjects[i].pNext;
        object_name_infos[i].objectType = callback_data->pObjects[i].objectType;
        object_name_infos[i].pObjectName = callback_data->pObjects[i].pObjectName;

        if (wine_vk_is_type_wrapped(callback_data->pObjects[i].objectType))
        {
            object_name_infos[i].objectHandle = wine_vk_get_wrapper(object->instance, callback_data->pObjects[i].objectHandle);
            if (!object_name_infos[i].objectHandle)
            {
                WARN("handle conversion failed 0x%s\n", wine_dbgstr_longlong(callback_data->pObjects[i].objectHandle));
                RtlFreeHeap(GetProcessHeap(), 0, object_name_infos);
                return VK_FALSE;
            }
        }
        else
        {
            object_name_infos[i].objectHandle = callback_data->pObjects[i].objectHandle;
        }
    }

    wine_callback_data.pObjects = object_name_infos;

    /* applications should always return VK_FALSE */
    result = object->user_callback(severity, message_types, &wine_callback_data, object->user_data);

    RtlFreeHeap(GetProcessHeap(), 0, object_name_infos);

    return result;
}

static VkBool32 debug_report_callback_conversion(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
    uint64_t object_handle, size_t location, int32_t code, const char *layer_prefix, const char *message, void *user_data)
{
    struct wine_debug_report_callback *object;

    TRACE("%#x, %#x, 0x%s, 0x%s, %d, %p, %p, %p\n", flags, object_type, wine_dbgstr_longlong(object_handle),
        wine_dbgstr_longlong(location), code, layer_prefix, message, user_data);

    object = user_data;

    if (!object->instance->instance)
    {
        /* instance wasn't yet created, this is a message from the native loader */
        return VK_FALSE;
    }

    object_handle = wine_vk_get_wrapper(object->instance, object_handle);
    if (!object_handle)
        object_type = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;

    return object->user_callback(
        flags, object_type, object_handle, location, code, layer_prefix, message, object->user_data);
}

static void wine_vk_physical_device_free(struct VkPhysicalDevice_T *phys_dev)
{
    if (!phys_dev)
        return;

    WINE_VK_REMOVE_HANDLE_MAPPING(phys_dev->instance, phys_dev);
    free(phys_dev->extensions);
    free(phys_dev);
}

static struct VkPhysicalDevice_T *wine_vk_physical_device_alloc(struct VkInstance_T *instance,
        VkPhysicalDevice phys_dev)
{
    struct VkPhysicalDevice_T *object;
    uint32_t num_host_properties, num_properties = 0;
    VkExtensionProperties *host_properties = NULL;
    VkResult res;
    unsigned int i, j;

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;

    object->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
    object->instance = instance;
    object->phys_dev = phys_dev;

    WINE_VK_ADD_DISPATCHABLE_MAPPING(instance, object, phys_dev);

    res = instance->funcs.p_vkEnumerateDeviceExtensionProperties(phys_dev,
            NULL, &num_host_properties, NULL);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to enumerate device extensions, res=%d\n", res);
        goto err;
    }

    host_properties = calloc(num_host_properties, sizeof(*host_properties));
    if (!host_properties)
    {
        ERR("Failed to allocate memory for device properties!\n");
        goto err;
    }

    res = instance->funcs.p_vkEnumerateDeviceExtensionProperties(phys_dev,
            NULL, &num_host_properties, host_properties);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to enumerate device extensions, res=%d\n", res);
        goto err;
    }

    /* Count list of extensions for which we have an implementation.
     * TODO: perform translation for platform specific extensions.
     */
    for (i = 0; i < num_host_properties; i++)
    {
        if (wine_vk_device_extension_supported(host_properties[i].extensionName))
        {
            TRACE("Enabling extension '%s' for physical device %p\n", host_properties[i].extensionName, object);
            num_properties++;
        }
        else
        {
            TRACE("Skipping extension '%s', no implementation found in winevulkan.\n", host_properties[i].extensionName);
        }
    }

    TRACE("Host supported extensions %u, Wine supported extensions %u\n", num_host_properties, num_properties);

    if (!(object->extensions = calloc(num_properties, sizeof(*object->extensions))))
    {
        ERR("Failed to allocate memory for device extensions!\n");
        goto err;
    }

    for (i = 0, j = 0; i < num_host_properties; i++)
    {
        if (wine_vk_device_extension_supported(host_properties[i].extensionName))
        {
            object->extensions[j] = host_properties[i];
            j++;
        }
    }
    object->extension_count = num_properties;

    free(host_properties);
    return object;

err:
    wine_vk_physical_device_free(object);
    free(host_properties);
    return NULL;
}

static void wine_vk_free_command_buffers(struct VkDevice_T *device,
        struct wine_cmd_pool *pool, uint32_t count, const VkCommandBuffer *buffers)
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if (!buffers[i])
            continue;

        device->funcs.p_vkFreeCommandBuffers(device->device, pool->command_pool, 1, &buffers[i]->command_buffer);
        list_remove(&buffers[i]->pool_link);
        WINE_VK_REMOVE_HANDLE_MAPPING(device->phys_dev->instance, buffers[i]);
        free(buffers[i]);
    }
}

static void wine_vk_device_get_queues(struct VkDevice_T *device,
        uint32_t family_index, uint32_t queue_count, VkDeviceQueueCreateFlags flags,
        struct VkQueue_T* queues)
{
    VkDeviceQueueInfo2 queue_info;
    unsigned int i;

    for (i = 0; i < queue_count; i++)
    {
        struct VkQueue_T *queue = &queues[i];

        queue->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
        queue->device = device;
        queue->family_index = family_index;
        queue->queue_index = i;
        queue->flags = flags;

        /* The Vulkan spec says:
         *
         * "vkGetDeviceQueue must only be used to get queues that were created
         * with the flags parameter of VkDeviceQueueCreateInfo set to zero."
         */
        if (flags && device->funcs.p_vkGetDeviceQueue2)
        {
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
            queue_info.pNext = NULL;
            queue_info.flags = flags;
            queue_info.queueFamilyIndex = family_index;
            queue_info.queueIndex = i;
            device->funcs.p_vkGetDeviceQueue2(device->device, &queue_info, &queue->queue);
        }
        else
        {
            device->funcs.p_vkGetDeviceQueue(device->device, family_index, i, &queue->queue);
        }

        WINE_VK_ADD_DISPATCHABLE_MAPPING(device->phys_dev->instance, queue, queue->queue);
    }
}

static void wine_vk_device_free_create_info(VkDeviceCreateInfo *create_info)
{
    VkDeviceGroupDeviceCreateInfo *group_info;

    if ((group_info = wine_vk_find_struct(create_info, DEVICE_GROUP_DEVICE_CREATE_INFO)))
    {
        free((void *)group_info->pPhysicalDevices);
    }

    free_VkDeviceCreateInfo_struct_chain(create_info);
}

static VkResult wine_vk_device_convert_create_info(const VkDeviceCreateInfo *src,
        VkDeviceCreateInfo *dst)
{
    VkDeviceGroupDeviceCreateInfo *group_info;
    unsigned int i;
    VkResult res;

    *dst = *src;

    if ((res = convert_VkDeviceCreateInfo_struct_chain(src->pNext, dst)) < 0)
    {
        WARN("Failed to convert VkDeviceCreateInfo pNext chain, res=%d.\n", res);
        return res;
    }

    /* FIXME: convert_VkDeviceCreateInfo_struct_chain() should unwrap handles for us. */
    if ((group_info = wine_vk_find_struct(dst, DEVICE_GROUP_DEVICE_CREATE_INFO)))
    {
        VkPhysicalDevice *physical_devices;

        if (!(physical_devices = calloc(group_info->physicalDeviceCount, sizeof(*physical_devices))))
        {
            free_VkDeviceCreateInfo_struct_chain(dst);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        for (i = 0; i < group_info->physicalDeviceCount; ++i)
        {
            physical_devices[i] = group_info->pPhysicalDevices[i]->phys_dev;
        }
        group_info->pPhysicalDevices = physical_devices;
    }

    /* Should be filtered out by loader as ICDs don't support layers. */
    dst->enabledLayerCount = 0;
    dst->ppEnabledLayerNames = NULL;

    TRACE("Enabled %u extensions.\n", dst->enabledExtensionCount);
    for (i = 0; i < dst->enabledExtensionCount; i++)
    {
        const char *extension_name = dst->ppEnabledExtensionNames[i];
        TRACE("Extension %u: %s.\n", i, debugstr_a(extension_name));
        if (!wine_vk_device_extension_supported(extension_name))
        {
            WARN("Extension %s is not supported.\n", debugstr_a(extension_name));
            wine_vk_device_free_create_info(dst);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    return VK_SUCCESS;
}

/* Helper function used for freeing a device structure. This function supports full
 * and partial object cleanups and can thus be used for vkCreateDevice failures.
 */
static void wine_vk_device_free(struct VkDevice_T *device)
{
    struct VkQueue_T *queue;

    if (!device)
        return;

    if (device->queues)
    {
        unsigned int i;
        for (i = 0; i < device->queue_count; i++)
        {
            queue = &device->queues[i];
            if (queue && queue->queue)
                WINE_VK_REMOVE_HANDLE_MAPPING(device->phys_dev->instance, queue);
        }
        free(device->queues);
        device->queues = NULL;
    }

    if (device->device && device->funcs.p_vkDestroyDevice)
    {
        WINE_VK_REMOVE_HANDLE_MAPPING(device->phys_dev->instance, device);
        device->funcs.p_vkDestroyDevice(device->device, NULL /* pAllocator */);
    }

    free(device);
}

NTSTATUS CDECL __wine_init_unix_lib(HMODULE module, DWORD reason, const void *driver, void *ptr_out)
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;

    vk_funcs = driver;
    p_vkEnumerateInstanceVersion = vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
    *(const struct unix_funcs **)ptr_out = &loader_funcs;
    return STATUS_SUCCESS;
}

/* Helper function for converting between win32 and host compatible VkInstanceCreateInfo.
 * This function takes care of extensions handled at winevulkan layer, a Wine graphics
 * driver is responsible for handling e.g. surface extensions.
 */
static VkResult wine_vk_instance_convert_create_info(const VkInstanceCreateInfo *src,
        VkInstanceCreateInfo *dst, struct VkInstance_T *object)
{
    VkDebugUtilsMessengerCreateInfoEXT *debug_utils_messenger;
    VkDebugReportCallbackCreateInfoEXT *debug_report_callback;
    VkBaseInStructure *header;
    unsigned int i;
    VkResult res;

    *dst = *src;

    if ((res = convert_VkInstanceCreateInfo_struct_chain(src->pNext, dst)) < 0)
    {
        WARN("Failed to convert VkInstanceCreateInfo pNext chain, res=%d.\n", res);
        return res;
    }

    object->utils_messenger_count = wine_vk_count_struct(dst, DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    object->utils_messengers =  calloc(object->utils_messenger_count, sizeof(*object->utils_messengers));
    header = (VkBaseInStructure *) dst;
    for (i = 0; i < object->utils_messenger_count; i++)
    {
        header = wine_vk_find_struct(header->pNext, DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
        debug_utils_messenger = (VkDebugUtilsMessengerCreateInfoEXT *) header;

        object->utils_messengers[i].instance = object;
        object->utils_messengers[i].debug_messenger = VK_NULL_HANDLE;
        object->utils_messengers[i].user_callback = debug_utils_messenger->pfnUserCallback;
        object->utils_messengers[i].user_data = debug_utils_messenger->pUserData;

        /* convert_VkInstanceCreateInfo_struct_chain already copied the chain,
         * so we can modify it in-place.
         */
        debug_utils_messenger->pfnUserCallback = (void *) &debug_utils_callback_conversion;
        debug_utils_messenger->pUserData = &object->utils_messengers[i];
    }

    debug_report_callback = wine_vk_find_struct(header->pNext, DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT);
    if (debug_report_callback)
    {
        object->default_callback.instance = object;
        object->default_callback.debug_callback = VK_NULL_HANDLE;
        object->default_callback.user_callback = debug_report_callback->pfnCallback;
        object->default_callback.user_data = debug_report_callback->pUserData;

        debug_report_callback->pfnCallback = (void *) &debug_report_callback_conversion;
        debug_report_callback->pUserData = &object->default_callback;
    }

    /* ICDs don't support any layers, so nothing to copy. Modern versions of the loader
     * filter this data out as well.
     */
    if (object->quirks & WINEVULKAN_QUIRK_IGNORE_EXPLICIT_LAYERS) {
        dst->enabledLayerCount = 0;
        dst->ppEnabledLayerNames = NULL;
        WARN("Ignoring explicit layers!\n");
    } else if (dst->enabledLayerCount) {
        FIXME("Loading explicit layers is not supported by winevulkan!\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    TRACE("Enabled %u instance extensions.\n", dst->enabledExtensionCount);
    for (i = 0; i < dst->enabledExtensionCount; i++)
    {
        const char *extension_name = dst->ppEnabledExtensionNames[i];
        TRACE("Extension %u: %s.\n", i, debugstr_a(extension_name));
        if (!wine_vk_instance_extension_supported(extension_name))
        {
            WARN("Extension %s is not supported.\n", debugstr_a(extension_name));
            free_VkInstanceCreateInfo_struct_chain(dst);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        if (!strcmp(extension_name, "VK_EXT_debug_utils") || !strcmp(extension_name, "VK_EXT_debug_report"))
        {
            object->enable_wrapper_list = VK_TRUE;
        }
    }

    return VK_SUCCESS;
}

/* Helper function which stores wrapped physical devices in the instance object. */
static VkResult wine_vk_instance_load_physical_devices(struct VkInstance_T *instance)
{
    VkPhysicalDevice *tmp_phys_devs;
    uint32_t phys_dev_count;
    unsigned int i;
    VkResult res;

    res = instance->funcs.p_vkEnumeratePhysicalDevices(instance->instance, &phys_dev_count, NULL);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to enumerate physical devices, res=%d\n", res);
        return res;
    }
    if (!phys_dev_count)
        return res;

    if (!(tmp_phys_devs = calloc(phys_dev_count, sizeof(*tmp_phys_devs))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = instance->funcs.p_vkEnumeratePhysicalDevices(instance->instance, &phys_dev_count, tmp_phys_devs);
    if (res != VK_SUCCESS)
    {
        free(tmp_phys_devs);
        return res;
    }

    instance->phys_devs = calloc(phys_dev_count, sizeof(*instance->phys_devs));
    if (!instance->phys_devs)
    {
        free(tmp_phys_devs);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Wrap each native physical device handle into a dispatchable object for the ICD loader. */
    for (i = 0; i < phys_dev_count; i++)
    {
        struct VkPhysicalDevice_T *phys_dev = wine_vk_physical_device_alloc(instance, tmp_phys_devs[i]);
        if (!phys_dev)
        {
            ERR("Unable to allocate memory for physical device!\n");
            free(tmp_phys_devs);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        instance->phys_devs[i] = phys_dev;
        instance->phys_dev_count = i + 1;
    }
    instance->phys_dev_count = phys_dev_count;

    free(tmp_phys_devs);
    return VK_SUCCESS;
}

static struct VkPhysicalDevice_T *wine_vk_instance_wrap_physical_device(struct VkInstance_T *instance,
        VkPhysicalDevice physical_device)
{
    unsigned int i;

    for (i = 0; i < instance->phys_dev_count; ++i)
    {
        struct VkPhysicalDevice_T *current = instance->phys_devs[i];
        if (current->phys_dev == physical_device)
            return current;
    }

    ERR("Unrecognized physical device %p.\n", physical_device);
    return NULL;
}

/* Helper function used for freeing an instance structure. This function supports full
 * and partial object cleanups and can thus be used for vkCreateInstance failures.
 */
static void wine_vk_instance_free(struct VkInstance_T *instance)
{
    if (!instance)
        return;

    if (instance->phys_devs)
    {
        unsigned int i;

        for (i = 0; i < instance->phys_dev_count; i++)
        {
            wine_vk_physical_device_free(instance->phys_devs[i]);
        }
        free(instance->phys_devs);
    }

    if (instance->instance)
    {
        vk_funcs->p_vkDestroyInstance(instance->instance, NULL /* allocator */);
        WINE_VK_REMOVE_HANDLE_MAPPING(instance, instance);
    }

    pthread_rwlock_destroy(&instance->wrapper_lock);
    free(instance->utils_messengers);

    free(instance);
}

VkResult WINAPI wine_vkAllocateCommandBuffers(VkDevice device,
        const VkCommandBufferAllocateInfo *allocate_info, VkCommandBuffer *buffers)
{
    struct wine_cmd_pool *pool;
    VkResult res = VK_SUCCESS;
    unsigned int i;

    TRACE("%p, %p, %p\n", device, allocate_info, buffers);

    pool = wine_cmd_pool_from_handle(allocate_info->commandPool);

    memset(buffers, 0, allocate_info->commandBufferCount * sizeof(*buffers));

    for (i = 0; i < allocate_info->commandBufferCount; i++)
    {
#if defined(USE_STRUCT_CONVERSION)
        VkCommandBufferAllocateInfo_host allocate_info_host;
#else
        VkCommandBufferAllocateInfo allocate_info_host;
#endif
        /* TODO: future extensions (none yet) may require pNext conversion. */
        allocate_info_host.pNext = allocate_info->pNext;
        allocate_info_host.sType = allocate_info->sType;
        allocate_info_host.commandPool = pool->command_pool;
        allocate_info_host.level = allocate_info->level;
        allocate_info_host.commandBufferCount = 1;

        TRACE("Allocating command buffer %u from pool 0x%s.\n",
                i, wine_dbgstr_longlong(allocate_info_host.commandPool));

        if (!(buffers[i] = calloc(1, sizeof(**buffers))))
        {
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            break;
        }

        buffers[i]->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
        buffers[i]->device = device;
        list_add_tail(&pool->command_buffers, &buffers[i]->pool_link);
        res = device->funcs.p_vkAllocateCommandBuffers(device->device,
                &allocate_info_host, &buffers[i]->command_buffer);
        WINE_VK_ADD_DISPATCHABLE_MAPPING(device->phys_dev->instance, buffers[i], buffers[i]->command_buffer);
        if (res != VK_SUCCESS)
        {
            ERR("Failed to allocate command buffer, res=%d.\n", res);
            buffers[i]->command_buffer = VK_NULL_HANDLE;
            break;
        }
    }

    if (res != VK_SUCCESS)
    {
        wine_vk_free_command_buffers(device, pool, i + 1, buffers);
        memset(buffers, 0, allocate_info->commandBufferCount * sizeof(*buffers));
    }

    return res;
}

void WINAPI wine_vkCmdExecuteCommands(VkCommandBuffer buffer, uint32_t count,
        const VkCommandBuffer *buffers)
{
    VkCommandBuffer *tmp_buffers;
    unsigned int i;

    TRACE("%p %u %p\n", buffer, count, buffers);

    if (!buffers || !count)
        return;

    /* Unfortunately we need a temporary buffer as our command buffers are wrapped.
     * This call is called often and if a performance concern, we may want to use
     * alloca as we shouldn't need much memory and it needs to be cleaned up after
     * the call anyway.
     */
    if (!(tmp_buffers = malloc(count * sizeof(*tmp_buffers))))
    {
        ERR("Failed to allocate memory for temporary command buffers\n");
        return;
    }

    for (i = 0; i < count; i++)
        tmp_buffers[i] = buffers[i]->command_buffer;

    buffer->device->funcs.p_vkCmdExecuteCommands(buffer->command_buffer, count, tmp_buffers);

    free(tmp_buffers);
}

VkResult WINAPI wine_vkCreateDevice(VkPhysicalDevice phys_dev,
        const VkDeviceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkDevice *device)
{
    VkDeviceCreateInfo create_info_host;
    struct VkQueue_T *next_queue;
    struct VkDevice_T *object;
    unsigned int i;
    VkResult res;

    TRACE("%p, %p, %p, %p\n", phys_dev, create_info, allocator, device);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (TRACE_ON(vulkan))
    {
        VkPhysicalDeviceProperties properties;

        wine_vkGetPhysicalDeviceProperties(phys_dev, &properties);

        TRACE("Device name: %s.\n", debugstr_a(properties.deviceName));
        TRACE("Vendor ID: %#x, Device ID: %#x.\n", properties.vendorID, properties.deviceID);
        TRACE("Driver version: %#x.\n", properties.driverVersion);
    }

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->base.base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
    object->phys_dev = phys_dev;

    res = wine_vk_device_convert_create_info(create_info, &create_info_host);
    if (res != VK_SUCCESS)
        goto fail;

    res = phys_dev->instance->funcs.p_vkCreateDevice(phys_dev->phys_dev,
            &create_info_host, NULL /* allocator */, &object->device);
    wine_vk_device_free_create_info(&create_info_host);
    WINE_VK_ADD_DISPATCHABLE_MAPPING(phys_dev->instance, object, object->device);
    if (res != VK_SUCCESS)
    {
        WARN("Failed to create device, res=%d.\n", res);
        goto fail;
    }

    /* Just load all function pointers we are aware off. The loader takes care of filtering.
     * We use vkGetDeviceProcAddr as opposed to vkGetInstanceProcAddr for efficiency reasons
     * as functions pass through fewer dispatch tables within the loader.
     */
#define USE_VK_FUNC(name) \
    object->funcs.p_##name = (void *)vk_funcs->p_vkGetDeviceProcAddr(object->device, #name); \
    if (object->funcs.p_##name == NULL) \
        TRACE("Not found '%s'.\n", #name);
    ALL_VK_DEVICE_FUNCS()
#undef USE_VK_FUNC

    /* We need to cache all queues within the device as each requires wrapping since queues are
     * dispatchable objects.
     */
    for (i = 0; i < create_info_host.queueCreateInfoCount; i++)
    {
        object->queue_count += create_info_host.pQueueCreateInfos[i].queueCount;
    }

    if (!(object->queues = calloc(object->queue_count, sizeof(*object->queues))))
    {
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto fail;
    }

    next_queue = object->queues;
    for (i = 0; i < create_info_host.queueCreateInfoCount; i++)
    {
        uint32_t flags = create_info_host.pQueueCreateInfos[i].flags;
        uint32_t family_index = create_info_host.pQueueCreateInfos[i].queueFamilyIndex;
        uint32_t queue_count = create_info_host.pQueueCreateInfos[i].queueCount;

        TRACE("Queue family index %u, queue count %u.\n", family_index, queue_count);

        wine_vk_device_get_queues(object, family_index, queue_count, flags, next_queue);
        next_queue += queue_count;
    }

    object->base.quirks = phys_dev->instance->quirks;

    *device = object;
    TRACE("Created device %p (native device %p).\n", object, object->device);
    return VK_SUCCESS;

fail:
    wine_vk_device_free(object);
    return res;
}

VkResult WINAPI wine_vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    VkInstanceCreateInfo create_info_host;
    const VkApplicationInfo *app_info;
    struct VkInstance_T *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
    {
        ERR("Failed to allocate memory for instance\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    object->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
    list_init(&object->wrappers);
    pthread_rwlock_init(&object->wrapper_lock, NULL);

    res = wine_vk_instance_convert_create_info(create_info, &create_info_host, object);
    if (res != VK_SUCCESS)
    {
        wine_vk_instance_free(object);
        return res;
    }

    res = vk_funcs->p_vkCreateInstance(&create_info_host, NULL /* allocator */, &object->instance);
    free_VkInstanceCreateInfo_struct_chain(&create_info_host);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create instance, res=%d\n", res);
        wine_vk_instance_free(object);
        return res;
    }

    WINE_VK_ADD_DISPATCHABLE_MAPPING(object, object, object->instance);

    /* Load all instance functions we are aware of. Note the loader takes care
     * of any filtering for extensions which were not requested, but which the
     * ICD may support.
     */
#define USE_VK_FUNC(name) \
    object->funcs.p_##name = (void *)vk_funcs->p_vkGetInstanceProcAddr(object->instance, #name);
    ALL_VK_INSTANCE_FUNCS()
#undef USE_VK_FUNC

    /* Cache physical devices for vkEnumeratePhysicalDevices within the instance as
     * each vkPhysicalDevice is a dispatchable object, which means we need to wrap
     * the native physical devices and present those to the application.
     * Cleanup happens as part of wine_vkDestroyInstance.
     */
    res = wine_vk_instance_load_physical_devices(object);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to load physical devices, res=%d\n", res);
        wine_vk_instance_free(object);
        return res;
    }

    if ((app_info = create_info->pApplicationInfo))
    {
        TRACE("Application name %s, application version %#x.\n",
                debugstr_a(app_info->pApplicationName), app_info->applicationVersion);
        TRACE("Engine name %s, engine version %#x.\n", debugstr_a(app_info->pEngineName),
                app_info->engineVersion);
        TRACE("API version %#x.\n", app_info->apiVersion);

        if (app_info->pEngineName && !strcmp(app_info->pEngineName, "idTech"))
            object->quirks |= WINEVULKAN_QUIRK_GET_DEVICE_PROC_ADDR;
    }

    object->quirks |= WINEVULKAN_QUIRK_ADJUST_MAX_IMAGE_COUNT;

    *instance = object;
    TRACE("Created instance %p (native instance %p).\n", object, object->instance);
    return VK_SUCCESS;
}

void WINAPI wine_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *allocator)
{
    TRACE("%p %p\n", device, allocator);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    wine_vk_device_free(device);
}

void WINAPI wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *allocator)
{
    TRACE("%p, %p\n", instance, allocator);

    if (allocator)
        FIXME("Support allocation allocators\n");

    wine_vk_instance_free(instance);
}

VkResult WINAPI wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice phys_dev,
        const char *layer_name, uint32_t *count, VkExtensionProperties *properties)
{
    TRACE("%p, %p, %p, %p\n", phys_dev, layer_name, count, properties);

    /* This shouldn't get called with layer_name set, the ICD loader prevents it. */
    if (layer_name)
    {
        ERR("Layer enumeration not supported from ICD.\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    if (!properties)
    {
        *count = phys_dev->extension_count;
        return VK_SUCCESS;
    }

    *count = min(*count, phys_dev->extension_count);
    memcpy(properties, phys_dev->extensions, *count * sizeof(*properties));

    TRACE("Returning %u extensions.\n", *count);
    return *count < phys_dev->extension_count ? VK_INCOMPLETE : VK_SUCCESS;
}

VkResult WINAPI wine_vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties *properties)
{
    uint32_t num_properties = 0, num_host_properties;
    VkExtensionProperties *host_properties;
    unsigned int i, j;
    VkResult res;

    res = vk_funcs->p_vkEnumerateInstanceExtensionProperties(NULL, &num_host_properties, NULL);
    if (res != VK_SUCCESS)
        return res;

    if (!(host_properties = calloc(num_host_properties, sizeof(*host_properties))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = vk_funcs->p_vkEnumerateInstanceExtensionProperties(NULL, &num_host_properties, host_properties);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to retrieve host properties, res=%d.\n", res);
        free(host_properties);
        return res;
    }

    /* The Wine graphics driver provides us with all extensions supported by the host side
     * including extension fixup (e.g. VK_KHR_xlib_surface -> VK_KHR_win32_surface). It is
     * up to us here to filter the list down to extensions for which we have thunks.
     */
    for (i = 0; i < num_host_properties; i++)
    {
        if (wine_vk_instance_extension_supported(host_properties[i].extensionName))
            num_properties++;
        else
            TRACE("Instance extension '%s' is not supported.\n", host_properties[i].extensionName);
    }

    if (!properties)
    {
        TRACE("Returning %u extensions.\n", num_properties);
        *count = num_properties;
        free(host_properties);
        return VK_SUCCESS;
    }

    for (i = 0, j = 0; i < num_host_properties && j < *count; i++)
    {
        if (wine_vk_instance_extension_supported(host_properties[i].extensionName))
        {
            TRACE("Enabling extension '%s'.\n", host_properties[i].extensionName);
            properties[j++] = host_properties[i];
        }
    }
    *count = min(*count, num_properties);

    free(host_properties);
    return *count < num_properties ? VK_INCOMPLETE : VK_SUCCESS;
}

VkResult WINAPI wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice phys_dev, uint32_t *count, VkLayerProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, count, properties);

    *count = 0;
    return VK_SUCCESS;
}

VkResult WINAPI wine_vkEnumerateInstanceVersion(uint32_t *version)
{
    VkResult res;

    if (p_vkEnumerateInstanceVersion)
    {
        res = p_vkEnumerateInstanceVersion(version);
    }
    else
    {
        *version = VK_API_VERSION_1_0;
        res = VK_SUCCESS;
    }

    TRACE("API version %u.%u.%u.\n",
            VK_VERSION_MAJOR(*version), VK_VERSION_MINOR(*version), VK_VERSION_PATCH(*version));
    *version = min(WINE_VK_VERSION, *version);
    return res;
}

VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *count,
        VkPhysicalDevice *devices)
{
    unsigned int i;

    TRACE("%p %p %p\n", instance, count, devices);

    if (!devices)
    {
        *count = instance->phys_dev_count;
        return VK_SUCCESS;
    }

    *count = min(*count, instance->phys_dev_count);
    for (i = 0; i < *count; i++)
    {
        devices[i] = instance->phys_devs[i];
    }

    TRACE("Returning %u devices.\n", *count);
    return *count < instance->phys_dev_count ? VK_INCOMPLETE : VK_SUCCESS;
}

void WINAPI wine_vkFreeCommandBuffers(VkDevice device, VkCommandPool pool_handle,
        uint32_t count, const VkCommandBuffer *buffers)
{
    struct wine_cmd_pool *pool = wine_cmd_pool_from_handle(pool_handle);

    TRACE("%p, 0x%s, %u, %p\n", device, wine_dbgstr_longlong(pool_handle), count, buffers);

    wine_vk_free_command_buffers(device, pool, count, buffers);
}

static VkQueue wine_vk_device_find_queue(VkDevice device, const VkDeviceQueueInfo2 *info)
{
    struct VkQueue_T* queue;
    uint32_t i;

    for (i = 0; i < device->queue_count; i++)
    {
        queue = &device->queues[i];
        if (queue->family_index == info->queueFamilyIndex
                && queue->queue_index == info->queueIndex
                && queue->flags == info->flags)
        {
            return queue;
        }
    }

    return VK_NULL_HANDLE;
}

void WINAPI wine_vkGetDeviceQueue(VkDevice device, uint32_t family_index,
        uint32_t queue_index, VkQueue *queue)
{
    VkDeviceQueueInfo2 queue_info;
    TRACE("%p, %u, %u, %p\n", device, family_index, queue_index, queue);

    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    queue_info.pNext = NULL;
    queue_info.flags = 0;
    queue_info.queueFamilyIndex = family_index;
    queue_info.queueIndex = queue_index;

    *queue = wine_vk_device_find_queue(device, &queue_info);
}

void WINAPI wine_vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *info, VkQueue *queue)
{
    const VkBaseInStructure *chain;

    TRACE("%p, %p, %p\n", device, info, queue);

    if ((chain = info->pNext))
        FIXME("Ignoring a linked structure of type %u.\n", chain->sType);

    *queue = wine_vk_device_find_queue(device, info);
}

VkResult WINAPI wine_vkQueueSubmit(VkQueue queue, uint32_t count,
        const VkSubmitInfo *submits, VkFence fence)
{
    VkSubmitInfo *submits_host;
    VkResult res;
    VkCommandBuffer *command_buffers;
    unsigned int i, j, num_command_buffers;

    TRACE("%p %u %p 0x%s\n", queue, count, submits, wine_dbgstr_longlong(fence));

    if (count == 0)
    {
        return queue->device->funcs.p_vkQueueSubmit(queue->queue, 0, NULL, fence);
    }

    submits_host = calloc(count, sizeof(*submits_host));
    if (!submits_host)
    {
        ERR("Unable to allocate memory for submit buffers!\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    for (i = 0; i < count; i++)
    {
        memcpy(&submits_host[i], &submits[i], sizeof(*submits_host));

        num_command_buffers = submits[i].commandBufferCount;
        command_buffers = calloc(num_command_buffers, sizeof(*command_buffers));
        if (!command_buffers)
        {
            ERR("Unable to allocate memory for command buffers!\n");
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            goto done;
        }

        for (j = 0; j < num_command_buffers; j++)
        {
            command_buffers[j] = submits[i].pCommandBuffers[j]->command_buffer;
        }
        submits_host[i].pCommandBuffers = command_buffers;
    }

    res = queue->device->funcs.p_vkQueueSubmit(queue->queue, count, submits_host, fence);

done:
    for (i = 0; i < count; i++)
    {
        free((void *)submits_host[i].pCommandBuffers);
    }
    free(submits_host);

    TRACE("Returning %d\n", res);
    return res;
}

VkResult WINAPI wine_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *info,
        const VkAllocationCallbacks *allocator, VkCommandPool *command_pool)
{
    struct wine_cmd_pool *object;
    VkResult res;

    TRACE("%p, %p, %p, %p\n", device, info, allocator, command_pool);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    list_init(&object->command_buffers);

    res = device->funcs.p_vkCreateCommandPool(device->device, info, NULL, &object->command_pool);

    if (res == VK_SUCCESS)
    {
        WINE_VK_ADD_NON_DISPATCHABLE_MAPPING(device->phys_dev->instance, object, object->command_pool);
        *command_pool = wine_cmd_pool_to_handle(object);
    }
    else
    {
        free(object);
    }

    return res;
}

void WINAPI wine_vkDestroyCommandPool(VkDevice device, VkCommandPool handle,
        const VkAllocationCallbacks *allocator)
{
    struct wine_cmd_pool *pool = wine_cmd_pool_from_handle(handle);
    struct VkCommandBuffer_T *buffer, *cursor;

    TRACE("%p, 0x%s, %p\n", device, wine_dbgstr_longlong(handle), allocator);

    if (!handle)
        return;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    /* The Vulkan spec says:
     *
     * "When a pool is destroyed, all command buffers allocated from the pool are freed."
     */
    LIST_FOR_EACH_ENTRY_SAFE(buffer, cursor, &pool->command_buffers, struct VkCommandBuffer_T, pool_link)
    {
        WINE_VK_REMOVE_HANDLE_MAPPING(device->phys_dev->instance, buffer);
        free(buffer);
    }

    WINE_VK_REMOVE_HANDLE_MAPPING(device->phys_dev->instance, pool);

    device->funcs.p_vkDestroyCommandPool(device->device, pool->command_pool, NULL);
    free(pool);
}

static VkResult wine_vk_enumerate_physical_device_groups(struct VkInstance_T *instance,
        VkResult (*p_vkEnumeratePhysicalDeviceGroups)(VkInstance, uint32_t *, VkPhysicalDeviceGroupProperties *),
        uint32_t *count, VkPhysicalDeviceGroupProperties *properties)
{
    unsigned int i, j;
    VkResult res;

    res = p_vkEnumeratePhysicalDeviceGroups(instance->instance, count, properties);
    if (res < 0 || !properties)
        return res;

    for (i = 0; i < *count; ++i)
    {
        VkPhysicalDeviceGroupProperties *current = &properties[i];
        for (j = 0; j < current->physicalDeviceCount; ++j)
        {
            VkPhysicalDevice dev = current->physicalDevices[j];
            if (!(current->physicalDevices[j] = wine_vk_instance_wrap_physical_device(instance, dev)))
                return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    return res;
}

VkResult WINAPI wine_vkEnumeratePhysicalDeviceGroups(VkInstance instance,
        uint32_t *count, VkPhysicalDeviceGroupProperties *properties)
{
    TRACE("%p, %p, %p\n", instance, count, properties);
    return wine_vk_enumerate_physical_device_groups(instance,
            instance->funcs.p_vkEnumeratePhysicalDeviceGroups, count, properties);
}

VkResult WINAPI wine_vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance,
        uint32_t *count, VkPhysicalDeviceGroupProperties *properties)
{
    TRACE("%p, %p, %p\n", instance, count, properties);
    return wine_vk_enumerate_physical_device_groups(instance,
            instance->funcs.p_vkEnumeratePhysicalDeviceGroupsKHR, count, properties);
}

void WINAPI wine_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceExternalFenceInfo *fence_info, VkExternalFenceProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, fence_info, properties);
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalFenceFeatures = 0;
}

void WINAPI wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceExternalFenceInfo *fence_info, VkExternalFenceProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, fence_info, properties);
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalFenceFeatures = 0;
}

void WINAPI wine_vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceExternalBufferInfo *buffer_info, VkExternalBufferProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, buffer_info, properties);
    memset(&properties->externalMemoryProperties, 0, sizeof(properties->externalMemoryProperties));
}

void WINAPI wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceExternalBufferInfo *buffer_info, VkExternalBufferProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, buffer_info, properties);
    memset(&properties->externalMemoryProperties, 0, sizeof(properties->externalMemoryProperties));
}

VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceImageFormatInfo2 *format_info, VkImageFormatProperties2 *properties)
{
    VkExternalImageFormatProperties *external_image_properties;
    VkResult res;

    TRACE("%p, %p, %p\n", phys_dev, format_info, properties);

    res = thunk_vkGetPhysicalDeviceImageFormatProperties2(phys_dev, format_info, properties);

    if ((external_image_properties = wine_vk_find_struct(properties, EXTERNAL_IMAGE_FORMAT_PROPERTIES)))
    {
        VkExternalMemoryProperties *p = &external_image_properties->externalMemoryProperties;
        p->externalMemoryFeatures = 0;
        p->exportFromImportedHandleTypes = 0;
        p->compatibleHandleTypes = 0;
    }

    return res;
}

VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceImageFormatInfo2 *format_info, VkImageFormatProperties2 *properties)
{
    VkExternalImageFormatProperties *external_image_properties;
    VkResult res;

    TRACE("%p, %p, %p\n", phys_dev, format_info, properties);

    res = thunk_vkGetPhysicalDeviceImageFormatProperties2KHR(phys_dev, format_info, properties);

    if ((external_image_properties = wine_vk_find_struct(properties, EXTERNAL_IMAGE_FORMAT_PROPERTIES)))
    {
        VkExternalMemoryProperties *p = &external_image_properties->externalMemoryProperties;
        p->externalMemoryFeatures = 0;
        p->exportFromImportedHandleTypes = 0;
        p->compatibleHandleTypes = 0;
    }

    return res;
}

/* From ntdll/unix/sync.c */
#define NANOSECONDS_IN_A_SECOND 1000000000
#define TICKSPERSEC             10000000

static inline VkTimeDomainEXT get_performance_counter_time_domain(void)
{
#if !defined(__APPLE__) && defined(HAVE_CLOCK_GETTIME)
# ifdef CLOCK_MONOTONIC_RAW
    return VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT;
# else
    return VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT;
# endif
#else
    FIXME("No mapping for VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT on this platform.\n");
    return VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
#endif
}

static VkTimeDomainEXT map_to_host_time_domain(VkTimeDomainEXT domain)
{
    /* Matches ntdll/unix/sync.c's performance counter implementation. */
    if (domain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT)
        return get_performance_counter_time_domain();

    return domain;
}

static inline uint64_t convert_monotonic_timestamp(uint64_t value)
{
    return value / (NANOSECONDS_IN_A_SECOND / TICKSPERSEC);
}

static inline uint64_t convert_timestamp(VkTimeDomainEXT host_domain, VkTimeDomainEXT target_domain, uint64_t value)
{
    if (host_domain == target_domain)
        return value;

    /* Convert between MONOTONIC time in ns -> QueryPerformanceCounter */
    if ((host_domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT || host_domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT)
            && target_domain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT)
        return convert_monotonic_timestamp(value);

    FIXME("Couldn't translate between host domain %d and target domain %d\n", host_domain, target_domain);
    return value;
}

VkResult WINAPI wine_vkGetCalibratedTimestampsEXT(VkDevice device,
    uint32_t timestamp_count, const VkCalibratedTimestampInfoEXT *timestamp_infos,
    uint64_t *timestamps, uint64_t *max_deviation)
{
    VkCalibratedTimestampInfoEXT* host_timestamp_infos;
    unsigned int i;
    VkResult res;
    TRACE("%p, %u, %p, %p, %p\n", device, timestamp_count, timestamp_infos, timestamps, max_deviation);

    if (!(host_timestamp_infos = malloc(sizeof(VkCalibratedTimestampInfoEXT) * timestamp_count)))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    for (i = 0; i < timestamp_count; i++)
    {
        host_timestamp_infos[i].sType = timestamp_infos[i].sType;
        host_timestamp_infos[i].pNext = timestamp_infos[i].pNext;
        host_timestamp_infos[i].timeDomain = map_to_host_time_domain(timestamp_infos[i].timeDomain);
    }

    res = device->funcs.p_vkGetCalibratedTimestampsEXT(device->device, timestamp_count, host_timestamp_infos, timestamps, max_deviation);
    if (res != VK_SUCCESS)
        return res;

    for (i = 0; i < timestamp_count; i++)
        timestamps[i] = convert_timestamp(host_timestamp_infos[i].timeDomain, timestamp_infos[i].timeDomain, timestamps[i]);

    free(host_timestamp_infos);

    return res;
}

VkResult WINAPI wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice phys_dev,
    uint32_t *time_domain_count, VkTimeDomainEXT *time_domains)
{
    BOOL supports_device = FALSE, supports_monotonic = FALSE, supports_monotonic_raw = FALSE;
    const VkTimeDomainEXT performance_counter_domain = get_performance_counter_time_domain();
    VkTimeDomainEXT *host_time_domains;
    uint32_t host_time_domain_count;
    VkTimeDomainEXT out_time_domains[2];
    uint32_t out_time_domain_count;
    unsigned int i;
    VkResult res;

    TRACE("%p, %p, %p\n", phys_dev, time_domain_count, time_domains);

    /* Find out the time domains supported on the host */
    res = phys_dev->instance->funcs.p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(phys_dev->phys_dev, &host_time_domain_count, NULL);
    if (res != VK_SUCCESS)
        return res;

    if (!(host_time_domains = malloc(sizeof(VkTimeDomainEXT) * host_time_domain_count)))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = phys_dev->instance->funcs.p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(phys_dev->phys_dev, &host_time_domain_count, host_time_domains);
    if (res != VK_SUCCESS)
    {
        free(host_time_domains);
        return res;
    }

    for (i = 0; i < host_time_domain_count; i++)
    {
        if (host_time_domains[i] == VK_TIME_DOMAIN_DEVICE_EXT)
            supports_device = TRUE;
        else if (host_time_domains[i] == VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT)
            supports_monotonic = TRUE;
        else if (host_time_domains[i] == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT)
            supports_monotonic_raw = TRUE;
        else
            FIXME("Unknown time domain %d\n", host_time_domains[i]);
    }

    free(host_time_domains);

    out_time_domain_count = 0;

    /* Map our monotonic times -> QPC */
    if (supports_monotonic_raw && performance_counter_domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT)
        out_time_domains[out_time_domain_count++] = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
    else if (supports_monotonic && performance_counter_domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT)
        out_time_domains[out_time_domain_count++] = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
    else
        FIXME("VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT not supported on this platform.\n");

    /* Forward the device domain time */
    if (supports_device)
        out_time_domains[out_time_domain_count++] = VK_TIME_DOMAIN_DEVICE_EXT;

    /* Send the count/domains back to the app */
    if (!time_domains)
    {
        *time_domain_count = out_time_domain_count;
        return VK_SUCCESS;
    }

    for (i = 0; i < min(*time_domain_count, out_time_domain_count); i++)
        time_domains[i] = out_time_domains[i];

    res = *time_domain_count < out_time_domain_count ? VK_INCOMPLETE : VK_SUCCESS;
    *time_domain_count = out_time_domain_count;
    return res;
}

void WINAPI wine_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceExternalSemaphoreInfo *semaphore_info, VkExternalSemaphoreProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, semaphore_info, properties);
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalSemaphoreFeatures = 0;
}

void WINAPI wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceExternalSemaphoreInfo *semaphore_info, VkExternalSemaphoreProperties *properties)
{
    TRACE("%p, %p, %p\n", phys_dev, semaphore_info, properties);
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalSemaphoreFeatures = 0;
}

VkResult WINAPI wine_vkSetPrivateDataEXT(VkDevice device, VkObjectType object_type, uint64_t object_handle,
        VkPrivateDataSlotEXT private_data_slot, uint64_t data)
{
    TRACE("%p, %#x, 0x%s, 0x%s, 0x%s\n", device, object_type, wine_dbgstr_longlong(object_handle),
            wine_dbgstr_longlong(private_data_slot), wine_dbgstr_longlong(data));

    object_handle = wine_vk_unwrap_handle(object_type, object_handle);
    return device->funcs.p_vkSetPrivateDataEXT(device->device, object_type, object_handle, private_data_slot, data);
}

void WINAPI wine_vkGetPrivateDataEXT(VkDevice device, VkObjectType object_type, uint64_t object_handle,
        VkPrivateDataSlotEXT private_data_slot, uint64_t *data)
{
    TRACE("%p, %#x, 0x%s, 0x%s, %p\n", device, object_type, wine_dbgstr_longlong(object_handle),
            wine_dbgstr_longlong(private_data_slot), data);

    object_handle = wine_vk_unwrap_handle(object_type, object_handle);
    device->funcs.p_vkGetPrivateDataEXT(device->device, object_type, object_handle, private_data_slot, data);
}

VkResult WINAPI wine_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *create_info,
        const VkAllocationCallbacks *allocator, VkSwapchainKHR *swapchain)
{
    VkSwapchainCreateInfoKHR native_info;

    TRACE("%p, %p, %p, %p\n", device, create_info, allocator, swapchain);

    native_info = *create_info;
    native_info.surface = wine_surface_from_handle(create_info->surface)->driver_surface;

    return thunk_vkCreateSwapchainKHR(device, &native_info, allocator, swapchain);
}

VkResult WINAPI wine_vkCreateWin32SurfaceKHR(VkInstance instance,
        const VkWin32SurfaceCreateInfoKHR *createInfo, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    struct wine_surface *object;
    VkResult res;

    TRACE("%p, %p, %p, %p\n", instance, createInfo, allocator, surface);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    object = calloc(1, sizeof(*object));

    if (!object)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = instance->funcs.p_vkCreateWin32SurfaceKHR(instance->instance, createInfo, NULL, &object->driver_surface);

    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    object->surface = vk_funcs->p_wine_get_native_surface(object->driver_surface);

    WINE_VK_ADD_NON_DISPATCHABLE_MAPPING(instance, object, object->surface);

    *surface = wine_surface_to_handle(object);

    return VK_SUCCESS;
}

void WINAPI wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *allocator)
{
    struct wine_surface *object = wine_surface_from_handle(surface);

    TRACE("%p, 0x%s, %p\n", instance, wine_dbgstr_longlong(surface), allocator);

    if (!object)
        return;

    instance->funcs.p_vkDestroySurfaceKHR(instance->instance, object->driver_surface, NULL);

    WINE_VK_REMOVE_HANDLE_MAPPING(instance, object);
    free(object);
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceSurfaceInfo2KHR *surface_info, uint32_t *formats_count, VkSurfaceFormat2KHR *formats)
{
    VkPhysicalDeviceSurfaceInfo2KHR native_info;

    TRACE("%p, %p, %p, %p\n", phys_dev, surface_info, formats_count, formats);

    native_info = *surface_info;
    native_info.surface = wine_surface_from_handle(surface_info->surface)->driver_surface;

    return thunk_vkGetPhysicalDeviceSurfaceFormats2KHR(phys_dev, &native_info, formats_count, formats);
}

static inline void adjust_max_image_count(VkPhysicalDevice phys_dev, VkSurfaceCapabilitiesKHR* capabilities)
{
    /* Many Windows games, for example Strange Brigade, No Man's Sky, Path of Exile
     * and World War Z, do not expect that maxImageCount can be set to 0.
     * A value of 0 means that there is no limit on the number of images.
     * Nvidia reports 8 on Windows, AMD 16.
     * https://vulkan.gpuinfo.org/displayreport.php?id=9122#surface
     * https://vulkan.gpuinfo.org/displayreport.php?id=9121#surface
     */
    if ((phys_dev->instance->quirks & WINEVULKAN_QUIRK_ADJUST_MAX_IMAGE_COUNT) && !capabilities->maxImageCount)
    {
        capabilities->maxImageCount = max(capabilities->minImageCount, 16);
    }
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice phys_dev,
        VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *capabilities)
{
    VkResult res;

    TRACE("%p, 0x%s, %p\n", phys_dev, wine_dbgstr_longlong(surface), capabilities);

    res = thunk_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, capabilities);

    if (res == VK_SUCCESS)
        adjust_max_image_count(phys_dev, capabilities);

    return res;
}

VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice phys_dev,
        const VkPhysicalDeviceSurfaceInfo2KHR *surface_info, VkSurfaceCapabilities2KHR *capabilities)
{
    VkPhysicalDeviceSurfaceInfo2KHR native_info;
    VkResult res;

    TRACE("%p, %p, %p\n", phys_dev, surface_info, capabilities);

    native_info = *surface_info;
    native_info.surface = wine_surface_from_handle(surface_info->surface)->driver_surface;

    res = thunk_vkGetPhysicalDeviceSurfaceCapabilities2KHR(phys_dev, &native_info, capabilities);

    if (res == VK_SUCCESS)
        adjust_max_image_count(phys_dev, &capabilities->surfaceCapabilities);

    return res;
}

VkResult WINAPI wine_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *create_info,
        const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *messenger)
{
    VkDebugUtilsMessengerCreateInfoEXT wine_create_info;
    struct wine_debug_utils_messenger *object;
    VkResult res;

    TRACE("%p, %p, %p, %p\n", instance, create_info, allocator, messenger);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->instance = instance;
    object->user_callback = create_info->pfnUserCallback;
    object->user_data = create_info->pUserData;

    wine_create_info = *create_info;

    wine_create_info.pfnUserCallback = (void *) &debug_utils_callback_conversion;
    wine_create_info.pUserData = object;

    res = instance->funcs.p_vkCreateDebugUtilsMessengerEXT(instance->instance, &wine_create_info, NULL,  &object->debug_messenger);

    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    WINE_VK_ADD_NON_DISPATCHABLE_MAPPING(instance, object, object->debug_messenger);
    *messenger = wine_debug_utils_messenger_to_handle(object);

    return VK_SUCCESS;
}

void WINAPI wine_vkDestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *allocator)
{
    struct wine_debug_utils_messenger *object;

    TRACE("%p, 0x%s, %p\n", instance, wine_dbgstr_longlong(messenger), allocator);

    object = wine_debug_utils_messenger_from_handle(messenger);

    if (!object)
        return;

    instance->funcs.p_vkDestroyDebugUtilsMessengerEXT(instance->instance, object->debug_messenger, NULL);
    WINE_VK_REMOVE_HANDLE_MAPPING(instance, object);

    free(object);
}

void WINAPI wine_vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *callback_data)
{
    VkDebugUtilsMessengerCallbackDataEXT native_callback_data;
    VkDebugUtilsObjectNameInfoEXT *object_names;
    unsigned int i;

    TRACE("%p, %#x, %#x, %p\n", instance, severity, types, callback_data);

    native_callback_data = *callback_data;
    object_names = calloc(callback_data->objectCount, sizeof(*object_names));
    memcpy(object_names, callback_data->pObjects, callback_data->objectCount * sizeof(*object_names));
    native_callback_data.pObjects = object_names;

    for (i = 0; i < callback_data->objectCount; i++)
    {
        object_names[i].objectHandle =
                wine_vk_unwrap_handle(callback_data->pObjects[i].objectType, callback_data->pObjects[i].objectHandle);
    }

    thunk_vkSubmitDebugUtilsMessageEXT(instance, severity, types, &native_callback_data);

    free(object_names);
}

VkResult WINAPI wine_vkSetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *tag_info)
{
    VkDebugUtilsObjectTagInfoEXT wine_tag_info;

    TRACE("%p, %p\n", device, tag_info);

    wine_tag_info = *tag_info;
    wine_tag_info.objectHandle = wine_vk_unwrap_handle(tag_info->objectType, tag_info->objectHandle);

    return thunk_vkSetDebugUtilsObjectTagEXT(device, &wine_tag_info);
}

VkResult WINAPI wine_vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *name_info)
{
    VkDebugUtilsObjectNameInfoEXT wine_name_info;

    TRACE("%p, %p\n", device, name_info);

    wine_name_info = *name_info;
    wine_name_info.objectHandle = wine_vk_unwrap_handle(name_info->objectType, name_info->objectHandle);

    return thunk_vkSetDebugUtilsObjectNameEXT(device, &wine_name_info);
}

VkResult WINAPI wine_vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *create_info,
    const VkAllocationCallbacks *allocator, VkDebugReportCallbackEXT *callback)
{
    VkDebugReportCallbackCreateInfoEXT wine_create_info;
    struct wine_debug_report_callback *object;
    VkResult res;

    TRACE("%p, %p, %p, %p\n", instance, create_info, allocator, callback);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->instance = instance;
    object->user_callback = create_info->pfnCallback;
    object->user_data = create_info->pUserData;

    wine_create_info = *create_info;

    wine_create_info.pfnCallback = (void *) debug_report_callback_conversion;
    wine_create_info.pUserData = object;

    res = instance->funcs.p_vkCreateDebugReportCallbackEXT(instance->instance, &wine_create_info, NULL, &object->debug_callback);

    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    WINE_VK_ADD_NON_DISPATCHABLE_MAPPING(instance, object, object->debug_callback);
    *callback = wine_debug_report_callback_to_handle(object);

    return VK_SUCCESS;
}

void WINAPI wine_vkDestroyDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *allocator)
{
    struct wine_debug_report_callback *object;

    TRACE("%p, 0x%s, %p\n", instance, wine_dbgstr_longlong(callback), allocator);

    object = wine_debug_report_callback_from_handle(callback);

    if (!object)
        return;

    instance->funcs.p_vkDestroyDebugReportCallbackEXT(instance->instance, object->debug_callback, NULL);

    WINE_VK_REMOVE_HANDLE_MAPPING(instance, object);

    free(object);
}

void WINAPI wine_vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
    uint64_t object, size_t location, int32_t code, const char *layer_prefix, const char *message)
{
    TRACE("%p, %#x, %#x, 0x%s, 0x%s, %d, %p, %p\n", instance, flags, object_type, wine_dbgstr_longlong(object),
        wine_dbgstr_longlong(location), code, layer_prefix, message);

    object = wine_vk_unwrap_handle(object_type, object);

    instance->funcs.p_vkDebugReportMessageEXT(
        instance->instance, flags, object_type, object, location, code, layer_prefix, message);
}

VkResult WINAPI wine_vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *tag_info)
{
    VkDebugMarkerObjectTagInfoEXT wine_tag_info;

    TRACE("%p, %p\n", device, tag_info);

    wine_tag_info = *tag_info;
    wine_tag_info.object = wine_vk_unwrap_handle(tag_info->objectType, tag_info->object);

    return thunk_vkDebugMarkerSetObjectTagEXT(device, &wine_tag_info);
}

VkResult WINAPI wine_vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *name_info)
{
    VkDebugMarkerObjectNameInfoEXT wine_name_info;

    TRACE("%p, %p\n", device, name_info);

    wine_name_info = *name_info;
    wine_name_info.object = wine_vk_unwrap_handle(name_info->objectType, name_info->object);

    return thunk_vkDebugMarkerSetObjectNameEXT(device, &wine_name_info);
}
