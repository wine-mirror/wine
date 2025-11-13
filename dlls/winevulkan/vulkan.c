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

#include "vulkan_private.h"
#include "wine/vulkan_driver.h"
#include "wine/rbtree.h"
#include "ntgdi.h"
#include "ntuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static PFN_vkCreateInstance p_vkCreateInstance;
static PFN_vkEnumerateInstanceVersion p_vkEnumerateInstanceVersion;
static PFN_vkEnumerateInstanceExtensionProperties p_vkEnumerateInstanceExtensionProperties;

const struct vulkan_funcs *vk_funcs;

static UINT64 call_vulkan_debug_report_callback;
static UINT64 call_vulkan_debug_utils_callback;

static UINT append_string(const char *name, char *strings, UINT *strings_len)
{
    UINT len = name ? strlen(name) + 1 : 0;
    if (strings && len) memcpy(strings + *strings_len, name, len);
    *strings_len += len;
    return len;
}

static void append_debug_utils_label(const VkDebugUtilsLabelEXT *label, struct debug_utils_label *dst,
        char *strings, UINT *strings_len)
{
    if (label->pNext) FIXME("Unsupported VkDebugUtilsLabelEXT pNext chain\n");
    memcpy(dst->color, label->color, sizeof(dst->color));
    dst->label_name_len = append_string(label->pLabelName, strings, strings_len);
}

static void append_debug_utils_object(const VkDebugUtilsObjectNameInfoEXT *object, struct debug_utils_object *dst,
        char *strings, UINT *strings_len)
{
    if (object->pNext) FIXME("Unsupported VkDebugUtilsObjectNameInfoEXT pNext chain\n");
    dst->object_type = object->objectType;
    dst->object_handle = object->objectHandle;
    dst->object_name_len = append_string(object->pObjectName, strings, strings_len);
}

static VkBool32 debug_utils_callback_conversion(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    const VkDeviceAddressBindingCallbackDataEXT *address = NULL;
    struct wine_vk_debug_utils_params *params;
    struct vulkan_debug_utils_messenger *object;
    struct debug_utils_object dummy_object, *objects;
    struct debug_utils_label dummy_label, *labels;
    UINT size, strings_len;
    char *ptr, *strings;
    ULONG ret_len;
    void *ret_ptr;
    unsigned int i;

    TRACE("%i, %u, %p, %p\n", severity, message_types, callback_data, user_data);

    object = user_data;

    if (!object->instance->host.instance)
    {
        /* instance wasn't yet created, this is a message from the host loader */
        return VK_FALSE;
    }

    if ((address = callback_data->pNext))
    {
        if (address->sType != VK_STRUCTURE_TYPE_DEVICE_ADDRESS_BINDING_CALLBACK_DATA_EXT) address = NULL;
        if (!address || address->pNext) FIXME("Unsupported VkDebugUtilsMessengerCallbackDataEXT pNext chain\n");
    }

    strings_len = 0;
    append_string(callback_data->pMessageIdName, NULL, &strings_len);
    append_string(callback_data->pMessage, NULL, &strings_len);
    for (i = 0; i < callback_data->queueLabelCount; i++)
        append_debug_utils_label(callback_data->pQueueLabels + i, &dummy_label, NULL, &strings_len);
    for (i = 0; i < callback_data->cmdBufLabelCount; i++)
        append_debug_utils_label(callback_data->pCmdBufLabels + i, &dummy_label, NULL, &strings_len);
    for (i = 0; i < callback_data->objectCount; i++)
        append_debug_utils_object(callback_data->pObjects + i, &dummy_object, NULL, &strings_len);

    size = sizeof(*params);
    size += sizeof(*labels) * (callback_data->queueLabelCount + callback_data->cmdBufLabelCount);
    size += sizeof(*objects) * callback_data->objectCount;

    if (!(params = malloc(size + strings_len))) return VK_FALSE;
    ptr = (char *)(params + 1);
    strings = (char *)params + size;

    params->dispatch.callback = call_vulkan_debug_utils_callback;
    params->user_callback = object->user_callback;
    params->user_data = object->user_data;
    params->severity = severity;
    params->message_types = message_types;
    params->flags = callback_data->flags;
    params->message_id_number = callback_data->messageIdNumber;

    strings_len = 0;
    params->message_id_name_len = append_string(callback_data->pMessageIdName, strings, &strings_len);
    params->message_len = append_string(callback_data->pMessage, strings, &strings_len);

    labels = (void *)ptr;
    for (i = 0; i < callback_data->queueLabelCount; i++)
        append_debug_utils_label(callback_data->pQueueLabels + i, labels + i, strings, &strings_len);
    params->queue_label_count = callback_data->queueLabelCount;
    ptr += callback_data->queueLabelCount * sizeof(*labels);

    labels = (void *)ptr;
    for (i = 0; i < callback_data->cmdBufLabelCount; i++)
        append_debug_utils_label(callback_data->pCmdBufLabels + i, labels + i, strings, &strings_len);
    params->cmd_buf_label_count = callback_data->cmdBufLabelCount;
    ptr += callback_data->cmdBufLabelCount * sizeof(*labels);

    objects = (void *)ptr;
    for (i = 0; i < callback_data->objectCount; i++)
    {
        append_debug_utils_object(callback_data->pObjects + i, objects + i, strings, &strings_len);

        if (wine_vk_is_type_wrapped(objects[i].object_type))
        {
            objects[i].object_handle = object->instance->p_client_handle_from_host(object->instance, objects[i].object_handle);
            if (!objects[i].object_handle)
            {
                WARN("handle conversion failed 0x%s\n", wine_dbgstr_longlong(callback_data->pObjects[i].objectHandle));
                free(params);
                return VK_FALSE;
            }
        }
    }
    params->object_count = callback_data->objectCount;
    ptr += callback_data->objectCount * sizeof(*objects);

    if (address)
    {
        params->has_address_binding = TRUE;
        params->address_binding.flags = address->flags;
        params->address_binding.base_address = address->baseAddress;
        params->address_binding.size = address->size;
        params->address_binding.binding_type = address->bindingType;
    }

    /* applications should always return VK_FALSE */
    KeUserDispatchCallback(&params->dispatch, size + strings_len, &ret_ptr, &ret_len);
    free(params);

    if (ret_len == sizeof(VkBool32)) return *(VkBool32 *)ret_ptr;
    return VK_FALSE;
}

static VkBool32 debug_report_callback_conversion(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
    uint64_t object_handle, size_t location, int32_t code, const char *layer_prefix, const char *message, void *user_data)
{
    struct wine_vk_debug_report_params *params;
    struct vulkan_debug_report_callback *object;
    UINT strings_len;
    ULONG ret_len;
    void *ret_ptr;
    char *strings;

    TRACE("%#x, %#x, 0x%s, 0x%s, %d, %p, %p, %p\n", flags, object_type, wine_dbgstr_longlong(object_handle),
        wine_dbgstr_longlong(location), code, layer_prefix, message, user_data);

    object = user_data;

    if (!object->instance->host.instance)
    {
        /* instance wasn't yet created, this is a message from the host loader */
        return VK_FALSE;
    }

    strings_len = 0;
    append_string(layer_prefix, NULL, &strings_len);
    append_string(message, NULL, &strings_len);

    if (!(params = malloc(sizeof(*params) + strings_len))) return VK_FALSE;
    strings = (char *)(params + 1);

    params->dispatch.callback = call_vulkan_debug_report_callback;
    params->user_callback = object->user_callback;
    params->user_data = object->user_data;
    params->flags = flags;
    params->object_type = object_type;
    params->location = location;
    params->code = code;
    params->object_handle = object->instance->p_client_handle_from_host(object->instance, object_handle);
    if (!params->object_handle) params->object_type = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;

    strings_len = 0;
    params->layer_len = append_string(layer_prefix, strings, &strings_len);
    params->message_len = append_string(message, strings, &strings_len);

    KeUserDispatchCallback(&params->dispatch, sizeof(*params) + strings_len, &ret_ptr, &ret_len);
    free(params);

    if (ret_len == sizeof(VkBool32)) return *(VkBool32 *)ret_ptr;
    return VK_FALSE;
}

static void wine_vk_free_command_buffers(struct vulkan_device *device,
        struct wine_cmd_pool *pool, uint32_t count, const VkCommandBuffer *buffers)
{
    struct vulkan_instance *instance = device->physical_device->instance;
    struct vulkan_command_buffer *buffer;
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if (!buffers[i])
            continue;
        buffer = vulkan_command_buffer_from_handle(buffers[i]);
        if (!buffer)
            continue;

        device->p_vkFreeCommandBuffers(device->host.device, pool->host.command_pool, 1,
                                             &buffer->host.command_buffer);
        instance->p_remove_object(instance, &buffer->obj);
        buffer->client.command_buffer->obj.unix_handle = 0;
        free(buffer);
    }
}

NTSTATUS init_vulkan(void *arg)
{
    const struct init_params *params = arg;

    vk_funcs = __wine_get_vulkan_driver(WINE_VULKAN_DRIVER_VERSION);
    if (!vk_funcs)
    {
        ERR("Failed to load Wine graphics driver supporting Vulkan.\n");
        return STATUS_UNSUCCESSFUL;
    }

    call_vulkan_debug_report_callback = params->call_vulkan_debug_report_callback;
    call_vulkan_debug_utils_callback = params->call_vulkan_debug_utils_callback;

    p_vkCreateInstance = (PFN_vkCreateInstance)vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    p_vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
    p_vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties");

    *params->extensions = vk_funcs->client_extensions;
    return STATUS_SUCCESS;
}

static void free_debug_utils_messengers(struct list *messengers)
{
    struct vulkan_debug_utils_messenger *messenger, *next;

    LIST_FOR_EACH_ENTRY_SAFE(messenger, next, messengers, struct vulkan_debug_utils_messenger, entry)
    {
        list_remove(&messenger->entry);
        free(messenger);
    }
}

static void free_debug_report_callbacks(struct list *callbacks)
{
    struct vulkan_debug_report_callback *callback, *next;

    LIST_FOR_EACH_ENTRY_SAFE(callback, next, callbacks, struct vulkan_debug_report_callback, entry)
    {
        list_remove(&callback->entry);
        free(callback);
    }
}

static struct vulkan_physical_device *vulkan_instance_get_physical_device(struct vulkan_instance *instance,
        VkPhysicalDevice host_physical_device)
{
    struct vulkan_physical_device *physical_devices = instance->physical_devices;
    uint32_t physical_device_count = instance->physical_device_count;
    unsigned int i;

    for (i = 0; i < physical_device_count; ++i)
    {
        struct vulkan_physical_device *current = physical_devices + i;
        if (current->host.physical_device == host_physical_device) return current;
    }

    ERR("Unrecognized physical device %p.\n", host_physical_device);
    return NULL;
}

VkResult wine_vkAllocateCommandBuffers(VkDevice client_device, const VkCommandBufferAllocateInfo *allocate_info,
                                       VkCommandBuffer *buffers )
{
    struct vulkan_device *device = vulkan_device_from_handle(client_device);
    struct vulkan_instance *instance = device->physical_device->instance;
    struct vulkan_command_buffer *buffer;
    struct wine_cmd_pool *pool;
    VkResult res = VK_SUCCESS;
    unsigned int i;

    pool = wine_cmd_pool_from_handle(allocate_info->commandPool);

    for (i = 0; i < allocate_info->commandBufferCount; i++)
    {
        VkCommandBufferAllocateInfo allocate_info_host;
        VkCommandBuffer host_command_buffer, client_command_buffer = buffers[i];

        /* TODO: future extensions (none yet) may require pNext conversion. */
        allocate_info_host.pNext = allocate_info->pNext;
        allocate_info_host.sType = allocate_info->sType;
        allocate_info_host.commandPool = pool->host.command_pool;
        allocate_info_host.level = allocate_info->level;
        allocate_info_host.commandBufferCount = 1;

        TRACE("Allocating command buffer %u from pool 0x%s.\n",
                i, wine_dbgstr_longlong(allocate_info_host.commandPool));

        if (!(buffer = calloc(1, sizeof(*buffer))))
        {
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            break;
        }

        res = device->p_vkAllocateCommandBuffers(device->host.device, &allocate_info_host,
                                                       &host_command_buffer);
        if (res != VK_SUCCESS)
        {
            ERR("Failed to allocate command buffer, res=%d.\n", res);
            free(buffer);
            break;
        }

        vulkan_object_init_ptr(&buffer->obj, (UINT_PTR)host_command_buffer, &client_command_buffer->obj);
        buffer->device = device;
        instance->p_insert_object(instance, &buffer->obj);
    }

    if (res != VK_SUCCESS)
        wine_vk_free_command_buffers(device, pool, i, buffers);

    return res;
}

VkResult wine_vkCreateInstance(const VkInstanceCreateInfo *client_create_info, const VkAllocationCallbacks *allocator,
                               VkInstance *client_instance_ptr)
{
    VkInstanceCreateInfo *create_info = (VkInstanceCreateInfo *)client_create_info; /* cast away const, chain has been copied in the thunks */
    struct list utils_messengers = LIST_INIT(utils_messengers), report_callbacks = LIST_INIT(report_callbacks);
    VkBaseInStructure *header = (VkBaseInStructure *)create_info;
    VkDebugReportCallbackCreateInfoEXT *debug_report_callback;

    while ((header = find_next_struct(header->pNext, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT)))
    {
        VkDebugUtilsMessengerCreateInfoEXT *debug_utils_messenger = (VkDebugUtilsMessengerCreateInfoEXT *)header;
        struct vulkan_debug_utils_messenger *utils_messenger;

        if (!(utils_messenger = calloc(1, sizeof(*utils_messenger)))) goto failed;
        utils_messenger->host.debug_messenger = VK_NULL_HANDLE;
        utils_messenger->user_callback = (UINT_PTR)debug_utils_messenger->pfnUserCallback;
        utils_messenger->user_data = (UINT_PTR)debug_utils_messenger->pUserData;

        debug_utils_messenger->pfnUserCallback = (void *)&debug_utils_callback_conversion;
        debug_utils_messenger->pUserData = utils_messenger;
        list_add_tail(&utils_messengers, &utils_messenger->entry);
    }

    if ((debug_report_callback = find_next_struct(create_info->pNext, VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT)))
    {
        struct vulkan_debug_report_callback *report_callback;

        if (!(report_callback = calloc(1, sizeof(*report_callback)))) goto failed;
        report_callback->host.debug_callback = VK_NULL_HANDLE;
        report_callback->user_callback = (UINT_PTR)debug_report_callback->pfnCallback;
        report_callback->user_data = (UINT_PTR)debug_report_callback->pUserData;

        debug_report_callback->pfnCallback = (void *)&debug_report_callback_conversion;
        debug_report_callback->pUserData = report_callback;
        list_add_tail(&report_callbacks, &report_callback->entry);
    }

    return vk_funcs->p_vkCreateInstance(create_info, NULL /* allocator */, client_instance_ptr);

failed:
    free_debug_utils_messengers(&utils_messengers);
    free_debug_report_callbacks(&report_callbacks);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

VkResult wine_vkEnumerateInstanceExtensionProperties(const char *name, uint32_t *count,
                                                     VkExtensionProperties *properties)
{
    return p_vkEnumerateInstanceExtensionProperties(name, count, properties);
}

VkResult wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice client_physical_device, uint32_t *count,
                                               VkLayerProperties *properties)
{
    *count = 0;
    return VK_SUCCESS;
}

VkResult wine_vkEnumerateInstanceVersion(uint32_t *version)
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

VkResult wine_vkEnumeratePhysicalDevices(VkInstance client_instance, uint32_t *count, VkPhysicalDevice *client_physical_devices)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);
    unsigned int i;

    if (!client_physical_devices)
    {
        *count = instance->physical_device_count;
        return VK_SUCCESS;
    }

    *count = min(*count, instance->physical_device_count);
    for (i = 0; i < *count; i++)
    {
        client_physical_devices[i] = instance->physical_devices[i].client.physical_device;
    }

    TRACE("Returning %u devices.\n", *count);
    return *count < instance->physical_device_count ? VK_INCOMPLETE : VK_SUCCESS;
}

void wine_vkFreeCommandBuffers(VkDevice client_device, VkCommandPool command_pool, uint32_t count,
                               const VkCommandBuffer *buffers)
{
    struct vulkan_device *device = vulkan_device_from_handle(client_device);
    struct wine_cmd_pool *pool = wine_cmd_pool_from_handle(command_pool);

    wine_vk_free_command_buffers(device, pool, count, buffers);
}

VkResult wine_vkCreateCommandPool(VkDevice client_device, const VkCommandPoolCreateInfo *info,
                                  const VkAllocationCallbacks *allocator, VkCommandPool *command_pool,
                                  void *client_ptr)
{
    struct vulkan_device *device = vulkan_device_from_handle(client_device);
    struct vulkan_instance *instance = device->physical_device->instance;
    struct vk_command_pool *client_command_pool = client_ptr;
    VkCommandPool host_command_pool;
    struct wine_cmd_pool *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = device->p_vkCreateCommandPool(device->host.device, info, NULL, &host_command_pool);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    vulkan_object_init_ptr(&object->obj, host_command_pool, &client_command_pool->obj);
    instance->p_insert_object(instance, &object->obj);

    *command_pool = object->client.command_pool;
    return VK_SUCCESS;
}

void wine_vkDestroyCommandPool(VkDevice client_device, VkCommandPool handle,
                               const VkAllocationCallbacks *allocator)
{
    struct vulkan_device *device = vulkan_device_from_handle(client_device);
    struct vulkan_instance *instance = device->physical_device->instance;
    struct wine_cmd_pool *pool = wine_cmd_pool_from_handle(handle);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    device->p_vkDestroyCommandPool(device->host.device, pool->host.command_pool, NULL);
    instance->p_remove_object(instance, &pool->obj);
    free(pool);
}

static VkResult wine_vk_enumerate_physical_device_groups(struct vulkan_instance *instance,
        VkResult (*p_vkEnumeratePhysicalDeviceGroups)(VkInstance, uint32_t *, VkPhysicalDeviceGroupProperties *),
        uint32_t *count, VkPhysicalDeviceGroupProperties *properties)
{
    unsigned int i, j;
    VkResult res;

    res = p_vkEnumeratePhysicalDeviceGroups(instance->host.instance, count, properties);
    if (res < 0 || !properties)
        return res;

    for (i = 0; i < *count; ++i)
    {
        VkPhysicalDeviceGroupProperties *current = &properties[i];
        for (j = 0; j < current->physicalDeviceCount; ++j)
        {
            VkPhysicalDevice host_physical_device = current->physicalDevices[j];
            struct vulkan_physical_device *physical_device = vulkan_instance_get_physical_device(instance, host_physical_device);
            if (!physical_device) return VK_ERROR_INITIALIZATION_FAILED;
            current->physicalDevices[j] = physical_device->client.physical_device;
        }
    }

    return res;
}

VkResult wine_vkEnumeratePhysicalDeviceGroups(VkInstance client_instance, uint32_t *count,
                                              VkPhysicalDeviceGroupProperties *properties)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);

    return wine_vk_enumerate_physical_device_groups(instance,
            instance->p_vkEnumeratePhysicalDeviceGroups, count, properties);
}

VkResult wine_vkEnumeratePhysicalDeviceGroupsKHR(VkInstance client_instance, uint32_t *count,
                                                 VkPhysicalDeviceGroupProperties *properties)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);

    return wine_vk_enumerate_physical_device_groups(instance,
            instance->p_vkEnumeratePhysicalDeviceGroupsKHR, count, properties);
}

void wine_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice client_physical_device,
                                                     const VkPhysicalDeviceExternalFenceInfo *fence_info,
                                                     VkExternalFenceProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalFenceFeatures = 0;
}

void wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice client_physical_device,
                                                        const VkPhysicalDeviceExternalFenceInfo *fence_info,
                                                        VkExternalFenceProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalFenceFeatures = 0;
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

static VkResult wine_vk_get_timestamps(struct vulkan_device *device, uint32_t timestamp_count,
                                       const VkCalibratedTimestampInfoEXT *timestamp_infos,
                                       uint64_t *timestamps, uint64_t *max_deviation,
                                       VkResult (*get_timestamps)(VkDevice, uint32_t, const VkCalibratedTimestampInfoEXT *, uint64_t *, uint64_t *))
{
    VkCalibratedTimestampInfoEXT* host_timestamp_infos;
    unsigned int i;
    VkResult res;

    if (timestamp_count == 0)
        return VK_SUCCESS;

    if (!(host_timestamp_infos = calloc(sizeof(VkCalibratedTimestampInfoEXT), timestamp_count)))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    for (i = 0; i < timestamp_count; i++)
    {
        host_timestamp_infos[i].sType = timestamp_infos[i].sType;
        host_timestamp_infos[i].pNext = timestamp_infos[i].pNext;
        host_timestamp_infos[i].timeDomain = map_to_host_time_domain(timestamp_infos[i].timeDomain);
    }

    res = get_timestamps(device->host.device, timestamp_count, host_timestamp_infos, timestamps, max_deviation);
    if (res == VK_SUCCESS)
    {
        for (i = 0; i < timestamp_count; i++)
            timestamps[i] = convert_timestamp(host_timestamp_infos[i].timeDomain, timestamp_infos[i].timeDomain, timestamps[i]);
    }

    free(host_timestamp_infos);

    return res;
}

static VkResult wine_vk_get_time_domains(struct vulkan_physical_device *physical_device,
                                         uint32_t *time_domain_count,
                                         VkTimeDomainEXT *time_domains,
                                         VkResult (*get_domains)(VkPhysicalDevice, uint32_t *, VkTimeDomainEXT *))
{
    BOOL supports_device = FALSE, supports_monotonic = FALSE, supports_monotonic_raw = FALSE;
    const VkTimeDomainEXT performance_counter_domain = get_performance_counter_time_domain();
    VkTimeDomainEXT *host_time_domains;
    uint32_t host_time_domain_count;
    VkTimeDomainEXT out_time_domains[2];
    uint32_t out_time_domain_count;
    unsigned int i;
    VkResult res;

    /* Find out the time domains supported on the host */
    res = get_domains(physical_device->host.physical_device, &host_time_domain_count, NULL);
    if (res != VK_SUCCESS)
        return res;

    if (!(host_time_domains = malloc(sizeof(VkTimeDomainEXT) * host_time_domain_count)))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = get_domains(physical_device->host.physical_device, &host_time_domain_count, host_time_domains);
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

VkResult wine_vkGetCalibratedTimestampsEXT(VkDevice client_device, uint32_t timestamp_count,
                                           const VkCalibratedTimestampInfoEXT *timestamp_infos,
                                           uint64_t *timestamps, uint64_t *max_deviation)
{
    struct vulkan_device *device = vulkan_device_from_handle(client_device);

    TRACE("%p, %u, %p, %p, %p\n", device, timestamp_count, timestamp_infos, timestamps, max_deviation);

    return wine_vk_get_timestamps(device, timestamp_count, timestamp_infos, timestamps, max_deviation,
                                  device->p_vkGetCalibratedTimestampsEXT);
}

VkResult wine_vkGetCalibratedTimestampsKHR(VkDevice client_device, uint32_t timestamp_count,
                                           const VkCalibratedTimestampInfoKHR *timestamp_infos,
                                           uint64_t *timestamps, uint64_t *max_deviation)
{
    struct vulkan_device *device = vulkan_device_from_handle(client_device);

    TRACE("%p, %u, %p, %p, %p\n", device, timestamp_count, timestamp_infos, timestamps, max_deviation);

    return wine_vk_get_timestamps(device, timestamp_count, timestamp_infos, timestamps, max_deviation,
                                  device->p_vkGetCalibratedTimestampsKHR);
}

VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice client_physical_device,
                                                             uint32_t *time_domain_count,
                                                             VkTimeDomainEXT *time_domains)
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle(client_physical_device);
    struct vulkan_instance *instance = physical_device->instance;

    TRACE("%p, %p, %p\n", physical_device, time_domain_count, time_domains);

    return wine_vk_get_time_domains(physical_device, time_domain_count, time_domains,
                                    instance->p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT);
}

VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR(VkPhysicalDevice client_physical_device,
                                                             uint32_t *time_domain_count,
                                                             VkTimeDomainKHR *time_domains)
{
    struct vulkan_physical_device *physical_device = vulkan_physical_device_from_handle(client_physical_device);
    struct vulkan_instance *instance = physical_device->instance;

    TRACE("%p, %p, %p\n", physical_device, time_domain_count, time_domains);

    return wine_vk_get_time_domains(physical_device, time_domain_count, time_domains,
                                    instance->p_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR);
}



void wine_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice client_physical_device,
                                                         const VkPhysicalDeviceExternalSemaphoreInfo *info,
                                                         VkExternalSemaphoreProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalSemaphoreFeatures = 0;
}

void wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice client_physical_device,
                                                            const VkPhysicalDeviceExternalSemaphoreInfo *info,
                                                            VkExternalSemaphoreProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalSemaphoreFeatures = 0;
}

VkResult wine_vkCreateDebugUtilsMessengerEXT(VkInstance client_instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT *create_info,
                                             const VkAllocationCallbacks *allocator,
                                             VkDebugUtilsMessengerEXT *messenger)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);
    VkDebugUtilsMessengerCreateInfoEXT wine_create_info;
    VkDebugUtilsMessengerEXT host_debug_messenger;
    struct vulkan_debug_utils_messenger *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    wine_create_info = *create_info;
    wine_create_info.pfnUserCallback = (void *) &debug_utils_callback_conversion;
    wine_create_info.pUserData = object;

    res = instance->p_vkCreateDebugUtilsMessengerEXT(instance->host.instance, &wine_create_info,
                                                           NULL, &host_debug_messenger);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    vulkan_object_init(&object->obj, host_debug_messenger);
    object->instance = instance;
    object->user_callback = (UINT_PTR)create_info->pfnUserCallback;
    object->user_data = (UINT_PTR)create_info->pUserData;
    instance->p_insert_object(instance, &object->obj);

    *messenger = object->client.debug_messenger;
    return VK_SUCCESS;
}

void wine_vkDestroyDebugUtilsMessengerEXT(VkInstance client_instance, VkDebugUtilsMessengerEXT messenger,
                                          const VkAllocationCallbacks *allocator)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);
    struct vulkan_debug_utils_messenger *object;

    object = vulkan_debug_utils_messenger_from_handle(messenger);

    if (!object)
        return;

    instance->p_vkDestroyDebugUtilsMessengerEXT(instance->host.instance, object->host.debug_messenger, NULL);
    instance->p_remove_object(instance, &object->obj);

    free(object);
}

VkResult wine_vkCreateDebugReportCallbackEXT(VkInstance client_instance,
                                             const VkDebugReportCallbackCreateInfoEXT *create_info,
                                             const VkAllocationCallbacks *allocator,
                                             VkDebugReportCallbackEXT *callback)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);
    VkDebugReportCallbackCreateInfoEXT wine_create_info;
    VkDebugReportCallbackEXT host_debug_callback;
    struct vulkan_debug_report_callback *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    wine_create_info = *create_info;
    wine_create_info.pfnCallback = (void *) debug_report_callback_conversion;
    wine_create_info.pUserData = object;

    res = instance->p_vkCreateDebugReportCallbackEXT(instance->host.instance, &wine_create_info,
                                                           NULL, &host_debug_callback);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    vulkan_object_init(&object->obj, host_debug_callback);
    object->instance = instance;
    object->user_callback = (UINT_PTR)create_info->pfnCallback;
    object->user_data = (UINT_PTR)create_info->pUserData;
    instance->p_insert_object(instance, &object->obj);

    *callback = object->client.debug_callback;
    return VK_SUCCESS;
}

void wine_vkDestroyDebugReportCallbackEXT(VkInstance client_instance, VkDebugReportCallbackEXT callback,
                                          const VkAllocationCallbacks *allocator)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(client_instance);
    struct vulkan_debug_report_callback *object;

    object = vulkan_debug_report_callback_from_handle(callback);

    if (!object)
        return;

    instance->p_vkDestroyDebugReportCallbackEXT(instance->host.instance, object->host.debug_callback, NULL);
    instance->p_remove_object(instance, &object->obj);

    free(object);
}

VkResult wine_vkCreateDeferredOperationKHR(VkDevice device_handle,
                                           const VkAllocationCallbacks* allocator,
                                           VkDeferredOperationKHR*      operation)
{
    struct vulkan_device *device = vulkan_device_from_handle(device_handle);
    struct vulkan_instance *instance = device->physical_device->instance;
    VkDeferredOperationKHR host_deferred_operation;
    struct wine_deferred_operation *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = device->p_vkCreateDeferredOperationKHR(device->host.device, NULL, &host_deferred_operation);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    vulkan_object_init(&object->obj, host_deferred_operation);
    init_conversion_context(&object->ctx);
    instance->p_insert_object(instance, &object->obj);

    *operation = object->client.deferred_operation;
    return VK_SUCCESS;
}

void wine_vkDestroyDeferredOperationKHR(VkDevice device_handle,
                                        VkDeferredOperationKHR       operation,
                                        const VkAllocationCallbacks* allocator)
{
    struct vulkan_device *device = vulkan_device_from_handle(device_handle);
    struct vulkan_instance *instance = device->physical_device->instance;
    struct wine_deferred_operation *object;

    object = wine_deferred_operation_from_handle(operation);

    if (!object)
        return;

    device->p_vkDestroyDeferredOperationKHR(device->host.device, object->host.deferred_operation, NULL);
    instance->p_remove_object(instance, &object->obj);

    free_conversion_context(&object->ctx);
    free(object);
}

static NTSTATUS is_available_instance_function(VkInstance handle, const char *name)
{
    struct vulkan_instance *instance = vulkan_instance_from_handle(handle);
    return !!vk_funcs->p_vkGetInstanceProcAddr(instance->host.instance, name);
}

static NTSTATUS is_available_device_function(VkDevice handle, const char *name)
{
    struct vulkan_device *device = vulkan_device_from_handle(handle);
    return !!vk_funcs->p_vkGetDeviceProcAddr(device->host.device, name);
}

#ifdef _WIN64

NTSTATUS vk_is_available_instance_function(void *arg)
{
    struct is_available_instance_function_params *params = arg;
    return is_available_instance_function(params->instance, params->name);
}

NTSTATUS vk_is_available_device_function(void *arg)
{
    struct is_available_device_function_params *params = arg;
    return is_available_device_function(params->device, params->name);
}

#endif /* _WIN64 */

NTSTATUS wow64_init_vulkan(void *arg)
{
    struct
    {
        UINT64 call_vulkan_debug_report_callback;
        UINT64 call_vulkan_debug_utils_callback;
        ULONG extensions;
    } *params32 = arg;
    struct init_params params;
    params.call_vulkan_debug_report_callback = params32->call_vulkan_debug_report_callback;
    params.call_vulkan_debug_utils_callback = params32->call_vulkan_debug_utils_callback;
    params.extensions = UlongToPtr(params32->extensions);
    return init_vulkan(&params);
}

NTSTATUS vk_is_available_instance_function32(void *arg)
{
    struct
    {
        UINT32 instance;
        UINT32 name;
    } *params = arg;
    return is_available_instance_function(UlongToPtr(params->instance), UlongToPtr(params->name));
}

NTSTATUS vk_is_available_device_function32(void *arg)
{
    struct
    {
        UINT32 device;
        UINT32 name;
    } *params = arg;
    return is_available_device_function(UlongToPtr(params->device), UlongToPtr(params->name));
}
