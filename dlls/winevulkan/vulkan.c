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
WINE_DECLARE_DEBUG_CHANNEL(fps);

static PFN_vkCreateInstance p_vkCreateInstance;
static PFN_vkEnumerateInstanceVersion p_vkEnumerateInstanceVersion;
static PFN_vkEnumerateInstanceExtensionProperties p_vkEnumerateInstanceExtensionProperties;


static int window_surface_compare(const void *key, const struct rb_entry *entry)
{
    const struct wine_surface *surface = RB_ENTRY_VALUE(entry, struct wine_surface, window_entry);
    HWND key_hwnd = (HWND)key;

    if (key_hwnd < surface->hwnd) return -1;
    if (key_hwnd > surface->hwnd) return 1;
    return 0;
}

static pthread_mutex_t window_surfaces_lock = PTHREAD_MUTEX_INITIALIZER;
static struct rb_tree window_surfaces = {.compare = window_surface_compare};

static void window_surfaces_insert(struct wine_surface *surface)
{
    struct wine_surface *previous;
    struct rb_entry *ptr;

    pthread_mutex_lock(&window_surfaces_lock);

    if (!(ptr = rb_get(&window_surfaces, surface->hwnd)))
        rb_put(&window_surfaces, surface->hwnd, &surface->window_entry);
    else
    {
        previous = RB_ENTRY_VALUE(ptr, struct wine_surface, window_entry);
        rb_replace(&window_surfaces, &previous->window_entry, &surface->window_entry);
        previous->hwnd = 0; /* make sure previous surface becomes invalid */
    }

    pthread_mutex_unlock(&window_surfaces_lock);
}

static void window_surfaces_remove(struct wine_surface *surface)
{
    pthread_mutex_lock(&window_surfaces_lock);
    if (surface->hwnd) rb_remove(&window_surfaces, &surface->window_entry);
    pthread_mutex_unlock(&window_surfaces_lock);
}

static BOOL is_wow64(void)
{
    return sizeof(void *) == sizeof(UINT64) && NtCurrentTeb()->WowTebOffset;
}

static BOOL use_external_memory(void)
{
    return is_wow64();
}

static ULONG_PTR zero_bits = 0;

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

static int wrapper_entry_compare(const void *key, const struct rb_entry *entry)
{
    struct wrapper_entry *wrapper = RB_ENTRY_VALUE(entry, struct wrapper_entry, entry);
    const uint64_t *host_handle = key;
    if (*host_handle < wrapper->host_handle) return -1;
    if (*host_handle > wrapper->host_handle) return 1;
    return 0;
}

static void add_handle_mapping(struct wine_instance *instance, uint64_t client_handle,
                               uint64_t host_handle, struct wrapper_entry *entry)
{
    if (instance->enable_wrapper_list)
    {
        entry->host_handle = host_handle;
        entry->client_handle = client_handle;

        pthread_rwlock_wrlock(&instance->wrapper_lock);
        rb_put(&instance->wrappers, &host_handle, &entry->entry);
        pthread_rwlock_unlock(&instance->wrapper_lock);
    }
}

static void add_handle_mapping_ptr(struct wine_instance *instance, void *client_handle,
                                   void *host_handle, struct wrapper_entry *entry)
{
    add_handle_mapping(instance, (uintptr_t)client_handle, (uintptr_t)host_handle, entry);
}

static void remove_handle_mapping(struct wine_instance *instance, struct wrapper_entry *entry)
{
    if (instance->enable_wrapper_list)
    {
        pthread_rwlock_wrlock(&instance->wrapper_lock);
        rb_remove(&instance->wrappers, &entry->entry);
        pthread_rwlock_unlock(&instance->wrapper_lock);
    }
}

static uint64_t client_handle_from_host(struct wine_instance *instance, uint64_t host_handle)
{
    struct rb_entry *entry;
    uint64_t result = 0;

    pthread_rwlock_rdlock(&instance->wrapper_lock);
    if ((entry = rb_get(&instance->wrappers, &host_handle)))
    {
        struct wrapper_entry *wrapper = RB_ENTRY_VALUE(entry, struct wrapper_entry, entry);
        result = wrapper->client_handle;
    }
    pthread_rwlock_unlock(&instance->wrapper_lock);
    return result;
}

struct vk_callback_funcs callback_funcs;

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
    struct wine_debug_utils_messenger *object;
    struct debug_utils_object dummy_object, *objects;
    struct debug_utils_label dummy_label, *labels;
    UINT size, strings_len;
    char *ptr, *strings;
    ULONG ret_len;
    void *ret_ptr;
    unsigned int i;

    TRACE("%i, %u, %p, %p\n", severity, message_types, callback_data, user_data);

    object = user_data;

    if (!object->instance->host_instance)
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

    params->dispatch.callback = callback_funcs.call_vulkan_debug_utils_callback;
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
            objects[i].object_handle = client_handle_from_host(object->instance, objects[i].object_handle);
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
    struct wine_debug_report_callback *object;
    UINT strings_len;
    ULONG ret_len;
    void *ret_ptr;
    char *strings;

    TRACE("%#x, %#x, 0x%s, 0x%s, %d, %p, %p, %p\n", flags, object_type, wine_dbgstr_longlong(object_handle),
        wine_dbgstr_longlong(location), code, layer_prefix, message, user_data);

    object = user_data;

    if (!object->instance->host_instance)
    {
        /* instance wasn't yet created, this is a message from the host loader */
        return VK_FALSE;
    }

    strings_len = 0;
    append_string(layer_prefix, NULL, &strings_len);
    append_string(message, NULL, &strings_len);

    if (!(params = malloc(sizeof(*params) + strings_len))) return VK_FALSE;
    strings = (char *)(params + 1);

    params->dispatch.callback = callback_funcs.call_vulkan_debug_report_callback;
    params->user_callback = object->user_callback;
    params->user_data = object->user_data;
    params->flags = flags;
    params->object_type = object_type;
    params->location = location;
    params->code = code;
    params->object_handle = client_handle_from_host(object->instance, object_handle);
    if (!params->object_handle) params->object_type = VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;

    strings_len = 0;
    params->layer_len = append_string(layer_prefix, strings, &strings_len);
    params->message_len = append_string(message, strings, &strings_len);

    KeUserDispatchCallback(&params->dispatch, sizeof(*params) + strings_len, &ret_ptr, &ret_len);
    free(params);

    if (ret_len == sizeof(VkBool32)) return *(VkBool32 *)ret_ptr;
    return VK_FALSE;
}

static void wine_phys_dev_cleanup(struct wine_phys_dev *phys_dev)
{
    free(phys_dev->extensions);
}

static VkResult wine_vk_physical_device_init(struct wine_phys_dev *object, VkPhysicalDevice host_handle,
        VkPhysicalDevice client_handle, struct wine_instance *instance)
{
    BOOL have_memory_placed = FALSE, have_map_memory2 = FALSE;
    uint32_t num_host_properties, num_properties = 0;
    VkExtensionProperties *host_properties = NULL;
    BOOL have_external_memory_host = FALSE;
    VkResult res;
    unsigned int i, j;

    object->instance = instance;
    object->handle = client_handle;
    object->host_physical_device = host_handle;

    client_handle->base.unix_handle = (uintptr_t)object;

    instance->funcs.p_vkGetPhysicalDeviceMemoryProperties(host_handle, &object->memory_properties);

    res = instance->funcs.p_vkEnumerateDeviceExtensionProperties(host_handle,
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

    res = instance->funcs.p_vkEnumerateDeviceExtensionProperties(host_handle,
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
        if (!strcmp(host_properties[i].extensionName, "VK_EXT_external_memory_host"))
            have_external_memory_host = TRUE;
        else if (!strcmp(host_properties[i].extensionName, "VK_EXT_map_memory_placed"))
            have_memory_placed = TRUE;
        else if (!strcmp(host_properties[i].extensionName, "VK_KHR_map_memory2"))
            have_map_memory2 = TRUE;
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

    if (zero_bits && have_memory_placed && have_map_memory2)
    {
        VkPhysicalDeviceMapMemoryPlacedFeaturesEXT map_placed_feature =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_FEATURES_EXT,
        };
        VkPhysicalDeviceFeatures2 features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &map_placed_feature,
        };

        instance->funcs.p_vkGetPhysicalDeviceFeatures2KHR(host_handle, &features);
        if (map_placed_feature.memoryMapPlaced && map_placed_feature.memoryUnmapReserve)
        {
            VkPhysicalDeviceMapMemoryPlacedPropertiesEXT map_placed_props =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_PROPERTIES_EXT,
            };
            VkPhysicalDeviceProperties2 props =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &map_placed_props,
            };

            instance->funcs.p_vkGetPhysicalDeviceProperties2(host_handle, &props);
            object->map_placed_align = map_placed_props.minPlacedMemoryMapAlignment;
            TRACE( "Using placed map with alignment %u\n", object->map_placed_align );
        }
    }

    if (zero_bits && have_external_memory_host && !object->map_placed_align)
    {
        VkPhysicalDeviceExternalMemoryHostPropertiesEXT host_mem_props =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT,
        };
        VkPhysicalDeviceProperties2 props =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &host_mem_props,
        };
        instance->funcs.p_vkGetPhysicalDeviceProperties2KHR(host_handle, &props);
        object->external_memory_align = host_mem_props.minImportedHostPointerAlignment;
        if (object->external_memory_align)
            TRACE("Using VK_EXT_external_memory_host for memory mapping with alignment: %u\n",
                  object->external_memory_align);
    }

    free(host_properties);
    return VK_SUCCESS;

err:
    wine_phys_dev_cleanup(object);
    free(host_properties);
    return res;
}

static void wine_vk_free_command_buffers(struct wine_device *device,
        struct wine_cmd_pool *pool, uint32_t count, const VkCommandBuffer *buffers)
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        struct wine_cmd_buffer *buffer = wine_cmd_buffer_from_handle(buffers[i]);

        if (!buffer)
            continue;

        device->funcs.p_vkFreeCommandBuffers(device->host_device, pool->host_command_pool, 1,
                                             &buffer->host_command_buffer);
        remove_handle_mapping(device->phys_dev->instance, &buffer->wrapper_entry);
        buffer->handle->base.unix_handle = 0;
        free(buffer);
    }
}

static void wine_vk_device_init_queues(struct wine_device *device, const VkDeviceQueueCreateInfo *info,
        VkQueue *handles)
{
    VkDeviceQueueInfo2 queue_info;
    UINT i;

    TRACE("Queue family index %u, queue count %u.\n", info->queueFamilyIndex, info->queueCount);

    for (i = 0; i < info->queueCount; i++)
    {
        struct wine_queue *queue = device->queues + device->queue_count + i;

        queue->device = device;
        queue->handle = (*handles)++;
        queue->family_index = info->queueFamilyIndex;
        queue->queue_index = i;
        queue->flags = info->flags;

        /* The Vulkan spec says:
         *
         * "vkGetDeviceQueue must only be used to get queues that were created
         * with the flags parameter of VkDeviceQueueCreateInfo set to zero."
         */
        if (info->flags && device->funcs.p_vkGetDeviceQueue2)
        {
            queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
            queue_info.pNext = NULL;
            queue_info.flags = info->flags;
            queue_info.queueFamilyIndex = info->queueFamilyIndex;
            queue_info.queueIndex = i;
            device->funcs.p_vkGetDeviceQueue2(device->host_device, &queue_info, &queue->host_queue);
        }
        else
        {
            device->funcs.p_vkGetDeviceQueue(device->host_device, info->queueFamilyIndex, i, &queue->host_queue);
        }

        queue->handle->base.unix_handle = (uintptr_t)queue;
        TRACE("Got device %p queue %p, host_queue %p.\n", device, queue, queue->host_queue);
    }

    device->queue_count += info->queueCount;
}

static const char *find_extension(const char *const *extensions, uint32_t count, const char *ext)
{
    while (count--)
    {
        if (!strcmp(extensions[count], ext))
            return extensions[count];
    }
    return NULL;
}

static VkResult wine_vk_device_convert_create_info(struct wine_phys_dev *phys_dev,
        struct conversion_context *ctx, const VkDeviceCreateInfo *src, VkDeviceCreateInfo *dst)
{
    const char *extra_extensions[2], * const*extensions = src->ppEnabledExtensionNames;
    unsigned int i, extra_count = 0, extensions_count = src->enabledExtensionCount;

    *dst = *src;

    /* Should be filtered out by loader as ICDs don't support layers. */
    dst->enabledLayerCount = 0;
    dst->ppEnabledLayerNames = NULL;

    TRACE("Enabled %u extensions.\n", extensions_count);
    for (i = 0; i < extensions_count; i++)
    {
        const char *extension_name = extensions[i];
        TRACE("Extension %u: %s.\n", i, debugstr_a(extension_name));
        if (!wine_vk_device_extension_supported(extension_name))
        {
            WARN("Extension %s is not supported.\n", debugstr_a(extension_name));
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    if (phys_dev->map_placed_align)
    {
        VkPhysicalDeviceMapMemoryPlacedFeaturesEXT *map_placed_features;
        map_placed_features = conversion_context_alloc(ctx, sizeof(*map_placed_features));
        map_placed_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_FEATURES_EXT;
        map_placed_features->pNext = (void *)dst->pNext;
        map_placed_features->memoryMapPlaced = VK_TRUE;
        map_placed_features->memoryMapRangePlaced = VK_FALSE;
        map_placed_features->memoryUnmapReserve = VK_TRUE;
        dst->pNext = map_placed_features;

        if (!find_extension(extensions, extensions_count, "VK_EXT_map_memory_placed"))
            extra_extensions[extra_count++] = "VK_EXT_map_memory_placed";
        if (!find_extension(extensions, extensions_count, "VK_KHR_map_memory2"))
            extra_extensions[extra_count++] = "VK_KHR_map_memory2";
    }
    else if (phys_dev->external_memory_align)
    {
        if (!find_extension(extensions, extensions_count, "VK_KHR_external_memory"))
            extra_extensions[extra_count++] = "VK_KHR_external_memory";
        if (!find_extension(extensions, extensions_count, "VK_EXT_external_memory_host"))
            extra_extensions[extra_count++] = "VK_EXT_external_memory_host";
    }

    if (extra_count)
    {
        const char **new_extensions;

        dst->enabledExtensionCount += extra_count;
        new_extensions = conversion_context_alloc(ctx, dst->enabledExtensionCount * sizeof(*new_extensions));
        memcpy(new_extensions, extensions, extensions_count * sizeof(*new_extensions));
        memcpy(new_extensions + extensions_count, extra_extensions, extra_count * sizeof(*new_extensions));
        dst->ppEnabledExtensionNames = new_extensions;
    }

    return VK_SUCCESS;
}

NTSTATUS init_vulkan(void *arg)
{
    const struct vk_callback_funcs *funcs = arg;

    vk_funcs = __wine_get_vulkan_driver(WINE_VULKAN_DRIVER_VERSION);
    if (!vk_funcs)
    {
        ERR("Failed to load Wine graphics driver supporting Vulkan.\n");
        return STATUS_UNSUCCESSFUL;
    }

    callback_funcs = *funcs;
    p_vkCreateInstance = vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    p_vkEnumerateInstanceVersion = vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
    p_vkEnumerateInstanceExtensionProperties = vk_funcs->p_vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties");

    if (is_wow64())
    {
        SYSTEM_BASIC_INFORMATION info;

        NtQuerySystemInformation(SystemEmulationBasicInformation, &info, sizeof(info), NULL);
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }

    return STATUS_SUCCESS;
}

/* Helper function for converting between win32 and host compatible VkInstanceCreateInfo.
 * This function takes care of extensions handled at winevulkan layer, a Wine graphics
 * driver is responsible for handling e.g. surface extensions.
 */
static VkResult wine_vk_instance_convert_create_info(struct conversion_context *ctx,
        const VkInstanceCreateInfo *src, VkInstanceCreateInfo *dst, struct wine_instance *object)
{
    VkDebugUtilsMessengerCreateInfoEXT *debug_utils_messenger;
    VkDebugReportCallbackCreateInfoEXT *debug_report_callback;
    const char **new_extensions;
    VkBaseInStructure *header;
    unsigned int i;

    *dst = *src;

    object->utils_messenger_count = wine_vk_count_struct(dst, DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    object->utils_messengers =  calloc(object->utils_messenger_count, sizeof(*object->utils_messengers));
    header = (VkBaseInStructure *) dst;
    for (i = 0; i < object->utils_messenger_count; i++)
    {
        header = find_next_struct(header->pNext, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
        debug_utils_messenger = (VkDebugUtilsMessengerCreateInfoEXT *) header;

        object->utils_messengers[i].instance = object;
        object->utils_messengers[i].host_debug_messenger = VK_NULL_HANDLE;
        object->utils_messengers[i].user_callback = (UINT_PTR)debug_utils_messenger->pfnUserCallback;
        object->utils_messengers[i].user_data = (UINT_PTR)debug_utils_messenger->pUserData;

        /* convert_VkInstanceCreateInfo_* already copied the chain, so we can modify it in-place. */
        debug_utils_messenger->pfnUserCallback = (void *) &debug_utils_callback_conversion;
        debug_utils_messenger->pUserData = &object->utils_messengers[i];
    }

    if ((debug_report_callback = find_next_struct(dst->pNext, VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT)))
    {
        object->default_callback.instance = object;
        object->default_callback.host_debug_callback = VK_NULL_HANDLE;
        object->default_callback.user_callback = (UINT_PTR)debug_report_callback->pfnCallback;
        object->default_callback.user_data = (UINT_PTR)debug_report_callback->pUserData;

        debug_report_callback->pfnCallback = (void *) &debug_report_callback_conversion;
        debug_report_callback->pUserData = &object->default_callback;
    }

    /* ICDs don't support any layers, so nothing to copy. Modern versions of the loader
     * filter this data out as well.
     */
    if (dst->enabledLayerCount)
    {
        FIXME("Loading explicit layers is not supported by winevulkan!\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    for (i = 0; i < src->enabledExtensionCount; i++)
    {
        const char *extension_name = src->ppEnabledExtensionNames[i];
        TRACE("Extension %u: %s.\n", i, debugstr_a(extension_name));
        if (!wine_vk_instance_extension_supported(extension_name))
        {
            WARN("Extension %s is not supported.\n", debugstr_a(extension_name));
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    new_extensions = conversion_context_alloc(ctx, (src->enabledExtensionCount + 2) *
                                              sizeof(*src->ppEnabledExtensionNames));
    memcpy(new_extensions, src->ppEnabledExtensionNames,
           dst->enabledExtensionCount * sizeof(*dst->ppEnabledExtensionNames));
    dst->ppEnabledExtensionNames = new_extensions;
    dst->enabledExtensionCount = src->enabledExtensionCount;

    for (i = 0; i < dst->enabledExtensionCount; i++)
    {
        const char *extension_name = dst->ppEnabledExtensionNames[i];
        if (!strcmp(extension_name, "VK_EXT_debug_utils") || !strcmp(extension_name, "VK_EXT_debug_report"))
        {
            object->enable_wrapper_list = VK_TRUE;
        }
        if (!strcmp(extension_name, "VK_KHR_win32_surface"))
        {
            new_extensions[i] = vk_funcs->p_get_host_surface_extension();
            object->enable_win32_surface = VK_TRUE;
        }
    }

    if (use_external_memory())
    {
        new_extensions[dst->enabledExtensionCount++] = "VK_KHR_get_physical_device_properties2";
        new_extensions[dst->enabledExtensionCount++] = "VK_KHR_external_memory_capabilities";
    }

    TRACE("Enabled %u instance extensions.\n", dst->enabledExtensionCount);

    return VK_SUCCESS;
}

/* Helper function which stores wrapped physical devices in the instance object. */
static VkResult wine_vk_instance_init_physical_devices(struct wine_instance *instance)
{
    VkPhysicalDevice *host_handles;
    uint32_t phys_dev_count;
    unsigned int i;
    VkResult res;

    res = instance->funcs.p_vkEnumeratePhysicalDevices(instance->host_instance, &phys_dev_count, NULL);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to enumerate physical devices, res=%d\n", res);
        return res;
    }
    if (!phys_dev_count)
        return res;

    if (phys_dev_count > instance->handle->phys_dev_count)
    {
        instance->handle->phys_dev_count = phys_dev_count;
        return VK_ERROR_OUT_OF_POOL_MEMORY;
    }
    instance->handle->phys_dev_count = phys_dev_count;

    if (!(host_handles = calloc(phys_dev_count, sizeof(*host_handles))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = instance->funcs.p_vkEnumeratePhysicalDevices(instance->host_instance, &phys_dev_count, host_handles);
    if (res != VK_SUCCESS)
    {
        free(host_handles);
        return res;
    }

    /* Wrap each host physical device handle into a dispatchable object for the ICD loader. */
    for (i = 0; i < phys_dev_count; i++)
    {
        struct wine_phys_dev *phys_dev = instance->phys_devs + i;
        res = wine_vk_physical_device_init(phys_dev, host_handles[i], &instance->handle->phys_devs[i], instance);
        if (res != VK_SUCCESS)
            goto err;
    }
    instance->phys_dev_count = phys_dev_count;

    free(host_handles);
    return VK_SUCCESS;

err:
    while (i) wine_phys_dev_cleanup(&instance->phys_devs[--i]);
    free(host_handles);
    return res;
}

static struct wine_phys_dev *wine_vk_instance_wrap_physical_device(struct wine_instance *instance,
        VkPhysicalDevice host_handle)
{
    unsigned int i;

    for (i = 0; i < instance->phys_dev_count; ++i)
    {
        struct wine_phys_dev *current = instance->phys_devs + i;
        if (current->host_physical_device == host_handle) return current;
    }

    ERR("Unrecognized physical device %p.\n", host_handle);
    return NULL;
}

VkResult wine_vkAllocateCommandBuffers(VkDevice handle, const VkCommandBufferAllocateInfo *allocate_info,
                                       VkCommandBuffer *buffers )
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_cmd_buffer *buffer;
    struct wine_cmd_pool *pool;
    VkResult res = VK_SUCCESS;
    unsigned int i;

    pool = wine_cmd_pool_from_handle(allocate_info->commandPool);

    for (i = 0; i < allocate_info->commandBufferCount; i++)
    {
        VkCommandBufferAllocateInfo allocate_info_host;

        /* TODO: future extensions (none yet) may require pNext conversion. */
        allocate_info_host.pNext = allocate_info->pNext;
        allocate_info_host.sType = allocate_info->sType;
        allocate_info_host.commandPool = pool->host_command_pool;
        allocate_info_host.level = allocate_info->level;
        allocate_info_host.commandBufferCount = 1;

        TRACE("Allocating command buffer %u from pool 0x%s.\n",
                i, wine_dbgstr_longlong(allocate_info_host.commandPool));

        if (!(buffer = calloc(1, sizeof(*buffer))))
        {
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            break;
        }

        buffer->handle = buffers[i];
        buffer->device = device;
        res = device->funcs.p_vkAllocateCommandBuffers(device->host_device, &allocate_info_host,
                                                       &buffer->host_command_buffer);
        buffer->handle->base.unix_handle = (uintptr_t)buffer;
        add_handle_mapping_ptr(device->phys_dev->instance, buffer->handle, buffer->host_command_buffer, &buffer->wrapper_entry);
        if (res != VK_SUCCESS)
        {
            ERR("Failed to allocate command buffer, res=%d.\n", res);
            buffer->host_command_buffer = VK_NULL_HANDLE;
            break;
        }
    }

    if (res != VK_SUCCESS)
        wine_vk_free_command_buffers(device, pool, i + 1, buffers);

    return res;
}

VkResult wine_vkCreateDevice(VkPhysicalDevice phys_dev_handle, const VkDeviceCreateInfo *create_info,
                             const VkAllocationCallbacks *allocator, VkDevice *ret_device,
                             void *client_ptr)
{
    struct wine_phys_dev *phys_dev = wine_phys_dev_from_handle(phys_dev_handle);
    struct wine_instance *instance = phys_dev->instance;
    VkDevice device_handle = client_ptr;
    VkDeviceCreateInfo create_info_host;
    struct VkQueue_T *queue_handles;
    struct conversion_context ctx;
    struct wine_device *object;
    unsigned int queue_count, i;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (TRACE_ON(vulkan))
    {
        VkPhysicalDeviceProperties properties;

        instance->funcs.p_vkGetPhysicalDeviceProperties(phys_dev->host_physical_device, &properties);

        TRACE("Device name: %s.\n", debugstr_a(properties.deviceName));
        TRACE("Vendor ID: %#x, Device ID: %#x.\n", properties.vendorID, properties.deviceID);
        TRACE("Driver version: %#x.\n", properties.driverVersion);
    }

    /* We need to cache all queues within the device as each requires wrapping since queues are dispatchable objects. */
    for (queue_count = 0, i = 0; i < create_info->queueCreateInfoCount; i++)
        queue_count += create_info->pQueueCreateInfos[i].queueCount;

    if (!(object = calloc(1, offsetof(struct wine_device, queues[queue_count]))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->phys_dev = phys_dev;

    init_conversion_context(&ctx);
    res = wine_vk_device_convert_create_info(phys_dev, &ctx, create_info, &create_info_host);
    if (res == VK_SUCCESS)
        res = instance->funcs.p_vkCreateDevice(phys_dev->host_physical_device, &create_info_host,
                                               NULL /* allocator */, &object->host_device);
    free_conversion_context(&ctx);
    if (res != VK_SUCCESS)
    {
        WARN("Failed to create device, res=%d.\n", res);
        free(object);
        return res;
    }

    /* Just load all function pointers we are aware off. The loader takes care of filtering.
     * We use vkGetDeviceProcAddr as opposed to vkGetInstanceProcAddr for efficiency reasons
     * as functions pass through fewer dispatch tables within the loader.
     */
#define USE_VK_FUNC(name)                                                                          \
    object->funcs.p_##name = (void *)vk_funcs->p_vkGetDeviceProcAddr(object->host_device, #name);  \
    if (object->funcs.p_##name == NULL) TRACE("Not found '%s'.\n", #name);
    ALL_VK_DEVICE_FUNCS()
#undef USE_VK_FUNC

    queue_handles = device_handle->queues;
    for (i = 0; i < create_info_host.queueCreateInfoCount; i++)
        wine_vk_device_init_queues(object, create_info_host.pQueueCreateInfos + i, &queue_handles);

    device_handle->quirks = instance->quirks;
    device_handle->base.unix_handle = (uintptr_t)object;

    TRACE("Created device %p, host_device %p.\n", object, object->host_device);
    for (i = 0; i < object->queue_count; i++)
    {
        struct wine_queue *queue = object->queues + i;
        add_handle_mapping_ptr(instance, queue->handle, queue->host_queue, &queue->wrapper_entry);
    }

    *ret_device = device_handle;
    add_handle_mapping_ptr(instance, *ret_device, object->host_device, &object->wrapper_entry);
    return VK_SUCCESS;
}

VkResult wine_vkCreateInstance(const VkInstanceCreateInfo *create_info,
                               const VkAllocationCallbacks *allocator, VkInstance *instance,
                               void *client_ptr)
{
    VkInstance client_instance = client_ptr;
    VkInstanceCreateInfo create_info_host;
    const VkApplicationInfo *app_info;
    struct conversion_context ctx;
    struct wine_instance *object;
    unsigned int i;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, offsetof(struct wine_instance, phys_devs[client_instance->phys_dev_count]))))
    {
        ERR("Failed to allocate memory for instance\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    init_conversion_context(&ctx);
    res = wine_vk_instance_convert_create_info(&ctx, create_info, &create_info_host, object);
    if (res == VK_SUCCESS)
        res = p_vkCreateInstance(&create_info_host, NULL /* allocator */, &object->host_instance);
    free_conversion_context(&ctx);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create instance, res=%d\n", res);
        free(object->utils_messengers);
        free(object);
        return res;
    }

    object->handle = client_instance;

    /* Load all instance functions we are aware of. Note the loader takes care
     * of any filtering for extensions which were not requested, but which the
     * ICD may support.
     */
#define USE_VK_FUNC(name) \
    object->funcs.p_##name = (void *)vk_funcs->p_vkGetInstanceProcAddr(object->host_instance, #name);
    ALL_VK_INSTANCE_FUNCS()
#undef USE_VK_FUNC

    /* Cache physical devices for vkEnumeratePhysicalDevices within the instance as
     * each vkPhysicalDevice is a dispatchable object, which means we need to wrap
     * the host physical devices and present those to the application.
     * Cleanup happens as part of wine_vkDestroyInstance.
     */
    res = wine_vk_instance_init_physical_devices(object);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to load physical devices, res=%d\n", res);
        object->funcs.p_vkDestroyInstance(object->host_instance, NULL /* allocator */);
        free(object->utils_messengers);
        free(object);
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

    client_instance->base.unix_handle = (uintptr_t)object;

    TRACE("Created instance %p, host_instance %p.\n", object, object->host_instance);

    rb_init(&object->wrappers, wrapper_entry_compare);
    pthread_rwlock_init(&object->wrapper_lock, NULL);

    for (i = 0; i < object->phys_dev_count; i++)
    {
        struct wine_phys_dev *phys_dev = &object->phys_devs[i];
        add_handle_mapping_ptr(object, phys_dev->handle, phys_dev->host_physical_device, &phys_dev->wrapper_entry);
    }

    *instance = client_instance;
    add_handle_mapping_ptr(object, *instance, object->host_instance, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroyDevice(VkDevice handle, const VkAllocationCallbacks *allocator)
{
    struct wine_device *device = wine_device_from_handle(handle);
    unsigned int i;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");
    if (!device)
        return;

    device->funcs.p_vkDestroyDevice(device->host_device, NULL /* pAllocator */);
    for (i = 0; i < device->queue_count; i++)
        remove_handle_mapping(device->phys_dev->instance, &device->queues[i].wrapper_entry);
    remove_handle_mapping(device->phys_dev->instance, &device->wrapper_entry);

    free(device);
}

void wine_vkDestroyInstance(VkInstance handle, const VkAllocationCallbacks *allocator)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    unsigned int i;

    if (allocator)
        FIXME("Support allocation allocators\n");
    if (!instance)
        return;

    instance->funcs.p_vkDestroyInstance(instance->host_instance, NULL /* allocator */);
    for (i = 0; i < instance->phys_dev_count; i++)
    {
        remove_handle_mapping(instance, &instance->phys_devs[i].wrapper_entry);
        wine_phys_dev_cleanup(&instance->phys_devs[i]);
    }
    remove_handle_mapping(instance, &instance->wrapper_entry);

    pthread_rwlock_destroy(&instance->wrapper_lock);
    free(instance->utils_messengers);
    free(instance);
}

VkResult wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice phys_dev_handle, const char *layer_name,
                                                   uint32_t *count, VkExtensionProperties *properties)
{
    struct wine_phys_dev *phys_dev = wine_phys_dev_from_handle(phys_dev_handle);

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

VkResult wine_vkEnumerateInstanceExtensionProperties(const char *name, uint32_t *count,
                                                     VkExtensionProperties *properties)
{
    uint32_t num_properties = 0, num_host_properties;
    VkExtensionProperties *host_properties;
    unsigned int i, j, surface;
    VkResult res;

    res = p_vkEnumerateInstanceExtensionProperties(NULL, &num_host_properties, NULL);
    if (res != VK_SUCCESS)
        return res;

    if (!(host_properties = calloc(num_host_properties, sizeof(*host_properties))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = p_vkEnumerateInstanceExtensionProperties(NULL, &num_host_properties, host_properties);
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
    for (i = 0, surface = 0; i < num_host_properties; i++)
    {
        if (wine_vk_instance_extension_supported(host_properties[i].extensionName)
                || (wine_vk_is_host_surface_extension(host_properties[i].extensionName) && !surface++))
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

    for (i = 0, j = 0, surface = 0; i < num_host_properties && j < *count; i++)
    {
        if (wine_vk_instance_extension_supported(host_properties[i].extensionName))
        {
            TRACE("Enabling extension '%s'.\n", host_properties[i].extensionName);
            properties[j++] = host_properties[i];
        }
        else if (wine_vk_is_host_surface_extension(host_properties[i].extensionName) && !surface++)
        {
            VkExtensionProperties win32_surface = {VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_SPEC_VERSION};
            TRACE("Enabling VK_KHR_win32_surface.\n");
            properties[j++] = win32_surface;
        }
    }
    *count = min(*count, num_properties);

    free(host_properties);
    return *count < num_properties ? VK_INCOMPLETE : VK_SUCCESS;
}

VkResult wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice phys_dev, uint32_t *count,
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

VkResult wine_vkEnumeratePhysicalDevices(VkInstance handle, uint32_t *count, VkPhysicalDevice *devices)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    unsigned int i;

    if (!devices)
    {
        *count = instance->phys_dev_count;
        return VK_SUCCESS;
    }

    *count = min(*count, instance->phys_dev_count);
    for (i = 0; i < *count; i++)
    {
        devices[i] = instance->phys_devs[i].handle;
    }

    TRACE("Returning %u devices.\n", *count);
    return *count < instance->phys_dev_count ? VK_INCOMPLETE : VK_SUCCESS;
}

void wine_vkFreeCommandBuffers(VkDevice handle, VkCommandPool command_pool, uint32_t count,
                               const VkCommandBuffer *buffers)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_cmd_pool *pool = wine_cmd_pool_from_handle(command_pool);

    wine_vk_free_command_buffers(device, pool, count, buffers);
}

static VkQueue wine_vk_device_find_queue(VkDevice handle, const VkDeviceQueueInfo2 *info)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_queue *queue;
    uint32_t i;

    for (i = 0; i < device->queue_count; i++)
    {
        queue = &device->queues[i];
        if (queue->family_index == info->queueFamilyIndex
                && queue->queue_index == info->queueIndex
                && queue->flags == info->flags)
        {
            return queue->handle;
        }
    }

    return VK_NULL_HANDLE;
}

void wine_vkGetDeviceQueue(VkDevice device, uint32_t family_index, uint32_t queue_index, VkQueue *queue)
{
    VkDeviceQueueInfo2 queue_info;

    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    queue_info.pNext = NULL;
    queue_info.flags = 0;
    queue_info.queueFamilyIndex = family_index;
    queue_info.queueIndex = queue_index;

    *queue = wine_vk_device_find_queue(device, &queue_info);
}

void wine_vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *info, VkQueue *queue)
{
    const VkBaseInStructure *chain;

    if ((chain = info->pNext))
        FIXME("Ignoring a linked structure of type %u.\n", chain->sType);

    *queue = wine_vk_device_find_queue(device, info);
}

VkResult wine_vkCreateCommandPool(VkDevice device_handle, const VkCommandPoolCreateInfo *info,
                                  const VkAllocationCallbacks *allocator, VkCommandPool *command_pool,
                                  void *client_ptr)
{
    struct wine_device *device = wine_device_from_handle(device_handle);
    struct vk_command_pool *handle = client_ptr;
    struct wine_cmd_pool *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = device->funcs.p_vkCreateCommandPool(device->host_device, info, NULL, &object->host_command_pool);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    object->handle = (uintptr_t)handle;
    handle->unix_handle = (uintptr_t)object;

    *command_pool = object->handle;
    add_handle_mapping(device->phys_dev->instance, *command_pool, object->host_command_pool, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroyCommandPool(VkDevice device_handle, VkCommandPool handle,
                               const VkAllocationCallbacks *allocator)
{
    struct wine_device *device = wine_device_from_handle(device_handle);
    struct wine_cmd_pool *pool = wine_cmd_pool_from_handle(handle);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    device->funcs.p_vkDestroyCommandPool(device->host_device, pool->host_command_pool, NULL);
    remove_handle_mapping(device->phys_dev->instance, &pool->wrapper_entry);
    free(pool);
}

static VkResult wine_vk_enumerate_physical_device_groups(struct wine_instance *instance,
        VkResult (*p_vkEnumeratePhysicalDeviceGroups)(VkInstance, uint32_t *, VkPhysicalDeviceGroupProperties *),
        uint32_t *count, VkPhysicalDeviceGroupProperties *properties)
{
    unsigned int i, j;
    VkResult res;

    res = p_vkEnumeratePhysicalDeviceGroups(instance->host_instance, count, properties);
    if (res < 0 || !properties)
        return res;

    for (i = 0; i < *count; ++i)
    {
        VkPhysicalDeviceGroupProperties *current = &properties[i];
        for (j = 0; j < current->physicalDeviceCount; ++j)
        {
            VkPhysicalDevice host_handle = current->physicalDevices[j];
            struct wine_phys_dev *phys_dev = wine_vk_instance_wrap_physical_device(instance, host_handle);
            if (!phys_dev)
                return VK_ERROR_INITIALIZATION_FAILED;
            current->physicalDevices[j] = phys_dev->handle;
        }
    }

    return res;
}

VkResult wine_vkEnumeratePhysicalDeviceGroups(VkInstance handle, uint32_t *count,
                                              VkPhysicalDeviceGroupProperties *properties)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);

    return wine_vk_enumerate_physical_device_groups(instance,
            instance->funcs.p_vkEnumeratePhysicalDeviceGroups, count, properties);
}

VkResult wine_vkEnumeratePhysicalDeviceGroupsKHR(VkInstance handle, uint32_t *count,
                                                 VkPhysicalDeviceGroupProperties *properties)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);

    return wine_vk_enumerate_physical_device_groups(instance,
            instance->funcs.p_vkEnumeratePhysicalDeviceGroupsKHR, count, properties);
}

void wine_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice phys_dev,
                                                     const VkPhysicalDeviceExternalFenceInfo *fence_info,
                                                     VkExternalFenceProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalFenceFeatures = 0;
}

void wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice phys_dev,
                                                        const VkPhysicalDeviceExternalFenceInfo *fence_info,
                                                        VkExternalFenceProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalFenceFeatures = 0;
}

void wine_vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice phys_dev,
                                                      const VkPhysicalDeviceExternalBufferInfo *buffer_info,
                                                      VkExternalBufferProperties *properties)
{
    memset(&properties->externalMemoryProperties, 0, sizeof(properties->externalMemoryProperties));
}

void wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice phys_dev,
                                                         const VkPhysicalDeviceExternalBufferInfo *buffer_info,
                                                         VkExternalBufferProperties *properties)
{
    memset(&properties->externalMemoryProperties, 0, sizeof(properties->externalMemoryProperties));
}

VkResult wine_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice phys_dev_handle,
                                                        const VkPhysicalDeviceImageFormatInfo2 *format_info,
                                                        VkImageFormatProperties2 *properties)
{
    struct wine_phys_dev *phys_dev = wine_phys_dev_from_handle(phys_dev_handle);
    VkExternalImageFormatProperties *external_image_properties;
    VkResult res;

    res = phys_dev->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2(phys_dev->host_physical_device,
                                                                                format_info, properties);

    if ((external_image_properties = find_next_struct(properties,
                                                      VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES)))
    {
        VkExternalMemoryProperties *p = &external_image_properties->externalMemoryProperties;
        p->externalMemoryFeatures = 0;
        p->exportFromImportedHandleTypes = 0;
        p->compatibleHandleTypes = 0;
    }

    return res;
}

VkResult wine_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice phys_dev_handle,
                                                           const VkPhysicalDeviceImageFormatInfo2 *format_info,
                                                           VkImageFormatProperties2 *properties)
{
    struct wine_phys_dev *phys_dev = wine_phys_dev_from_handle(phys_dev_handle);
    VkExternalImageFormatProperties *external_image_properties;
    VkResult res;

    res = phys_dev->instance->funcs.p_vkGetPhysicalDeviceImageFormatProperties2KHR(phys_dev->host_physical_device,
                                                                                   format_info, properties);

    if ((external_image_properties = find_next_struct(properties,
                                                      VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES)))
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

static VkResult wine_vk_get_timestamps(struct wine_device *device, uint32_t timestamp_count,
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

    res = get_timestamps(device->host_device, timestamp_count, host_timestamp_infos, timestamps, max_deviation);
    if (res == VK_SUCCESS)
    {
        for (i = 0; i < timestamp_count; i++)
            timestamps[i] = convert_timestamp(host_timestamp_infos[i].timeDomain, timestamp_infos[i].timeDomain, timestamps[i]);
    }

    free(host_timestamp_infos);

    return res;
}

static VkResult wine_vk_get_time_domains(struct wine_phys_dev *phys_dev,
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
    res = get_domains(phys_dev->host_physical_device, &host_time_domain_count, NULL);
    if (res != VK_SUCCESS)
        return res;

    if (!(host_time_domains = malloc(sizeof(VkTimeDomainEXT) * host_time_domain_count)))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = get_domains(phys_dev->host_physical_device, &host_time_domain_count, host_time_domains);
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

VkResult wine_vkGetCalibratedTimestampsEXT(VkDevice handle, uint32_t timestamp_count,
                                           const VkCalibratedTimestampInfoEXT *timestamp_infos,
                                           uint64_t *timestamps, uint64_t *max_deviation)
{
    struct wine_device *device = wine_device_from_handle(handle);

    TRACE("%p, %u, %p, %p, %p\n", device, timestamp_count, timestamp_infos, timestamps, max_deviation);

    return wine_vk_get_timestamps(device, timestamp_count, timestamp_infos, timestamps, max_deviation,
                                  device->funcs.p_vkGetCalibratedTimestampsEXT);
}

VkResult wine_vkGetCalibratedTimestampsKHR(VkDevice handle, uint32_t timestamp_count,
                                           const VkCalibratedTimestampInfoKHR *timestamp_infos,
                                           uint64_t *timestamps, uint64_t *max_deviation)
{
    struct wine_device *device = wine_device_from_handle(handle);

    TRACE("%p, %u, %p, %p, %p\n", device, timestamp_count, timestamp_infos, timestamps, max_deviation);

    return wine_vk_get_timestamps(device, timestamp_count, timestamp_infos, timestamps, max_deviation,
                                  device->funcs.p_vkGetCalibratedTimestampsKHR);
}

VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice handle,
                                                             uint32_t *time_domain_count,
                                                             VkTimeDomainEXT *time_domains)
{
    struct wine_phys_dev *phys_dev = wine_phys_dev_from_handle(handle);

    TRACE("%p, %p, %p\n", phys_dev, time_domain_count, time_domains);

    return wine_vk_get_time_domains(phys_dev, time_domain_count, time_domains,
                                    phys_dev->instance->funcs.p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT);
}

VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR(VkPhysicalDevice handle,
                                                             uint32_t *time_domain_count,
                                                             VkTimeDomainKHR *time_domains)
{
    struct wine_phys_dev *phys_dev = wine_phys_dev_from_handle(handle);

    TRACE("%p, %p, %p\n", phys_dev, time_domain_count, time_domains);

    return wine_vk_get_time_domains(phys_dev, time_domain_count, time_domains,
                                    phys_dev->instance->funcs.p_vkGetPhysicalDeviceCalibrateableTimeDomainsKHR);
}



void wine_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice phys_dev,
                                                         const VkPhysicalDeviceExternalSemaphoreInfo *info,
                                                         VkExternalSemaphoreProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalSemaphoreFeatures = 0;
}

void wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice phys_dev,
                                                            const VkPhysicalDeviceExternalSemaphoreInfo *info,
                                                            VkExternalSemaphoreProperties *properties)
{
    properties->exportFromImportedHandleTypes = 0;
    properties->compatibleHandleTypes = 0;
    properties->externalSemaphoreFeatures = 0;
}

VkResult wine_vkCreateWin32SurfaceKHR(VkInstance handle, const VkWin32SurfaceCreateInfoKHR *create_info,
                                      const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    VkWin32SurfaceCreateInfoKHR create_info_host = *create_info;
    struct wine_surface *object;
    HWND dummy = NULL;
    VkResult res;

    if (allocator) FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object)))) return VK_ERROR_OUT_OF_HOST_MEMORY;

    /* Windows allows surfaces to be created with no HWND, they return VK_ERROR_SURFACE_LOST_KHR later */
    if (!(object->hwnd = create_info->hwnd))
    {
        static const WCHAR staticW[] = {'s','t','a','t','i','c',0};
        UNICODE_STRING static_us = RTL_CONSTANT_STRING(staticW);
        dummy = NtUserCreateWindowEx(0, &static_us, &static_us, &static_us, WS_POPUP,
                                     0, 0, 0, 0, NULL, NULL, NULL, NULL, 0, NULL, 0, FALSE);
        WARN("Created dummy window %p for null surface window\n", dummy);
        create_info_host.hwnd = object->hwnd = dummy;
    }

    res = instance->funcs.p_vkCreateWin32SurfaceKHR(instance->host_instance, &create_info_host,
                                                    NULL /* allocator */, &object->driver_surface);
    if (res != VK_SUCCESS)
    {
        if (dummy) NtUserDestroyWindow(dummy);
        free(object);
        return res;
    }

    object->host_surface = vk_funcs->p_wine_get_host_surface(object->driver_surface);
    if (dummy) NtUserDestroyWindow(dummy);
    window_surfaces_insert(object);

    *surface = wine_surface_to_handle(object);
    add_handle_mapping(instance, *surface, object->host_surface, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroySurfaceKHR(VkInstance handle, VkSurfaceKHR surface,
                              const VkAllocationCallbacks *allocator)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    struct wine_surface *object = wine_surface_from_handle(surface);

    if (!object)
        return;

    instance->funcs.p_vkDestroySurfaceKHR(instance->host_instance, object->driver_surface, NULL);
    remove_handle_mapping(instance, &object->wrapper_entry);
    window_surfaces_remove(object);

    free(object);
}

static BOOL extents_equals(const VkExtent2D *extents, const RECT *rect)
{
    return extents->width == rect->right - rect->left &&
           extents->height == rect->bottom - rect->top;
}

VkResult wine_vkAcquireNextImage2KHR(VkDevice device_handle, const VkAcquireNextImageInfoKHR *acquire_info,
                                     uint32_t *image_index)
{
    struct wine_swapchain *swapchain = wine_swapchain_from_handle(acquire_info->swapchain);
    struct wine_device *device = wine_device_from_handle(device_handle);
    VkAcquireNextImageInfoKHR acquire_info_host = *acquire_info;
    struct wine_surface *surface = swapchain->surface;
    RECT client_rect;
    VkResult res;

    acquire_info_host.swapchain = swapchain->host_swapchain;
    res = device->funcs.p_vkAcquireNextImage2KHR(device->host_device, &acquire_info_host, image_index);

    if (res == VK_SUCCESS && NtUserGetClientRect(surface->hwnd, &client_rect, NtUserGetDpiForWindow(surface->hwnd)) &&
        !extents_equals(&swapchain->extents, &client_rect))
    {
        WARN("Swapchain size %dx%d does not match client rect %s, returning VK_SUBOPTIMAL_KHR\n",
             swapchain->extents.width, swapchain->extents.height, wine_dbgstr_rect(&client_rect));
        return VK_SUBOPTIMAL_KHR;
    }

    return res;
}

VkResult wine_vkAcquireNextImageKHR(VkDevice device_handle, VkSwapchainKHR swapchain_handle, uint64_t timeout,
                                    VkSemaphore semaphore, VkFence fence, uint32_t *image_index)
{
    struct wine_swapchain *swapchain = wine_swapchain_from_handle(swapchain_handle);
    struct wine_device *device = wine_device_from_handle(device_handle);
    struct wine_surface *surface = swapchain->surface;
    RECT client_rect;
    VkResult res;

    res = device->funcs.p_vkAcquireNextImageKHR(device->host_device, swapchain->host_swapchain, timeout,
                                                semaphore, fence, image_index);

    if (res == VK_SUCCESS && NtUserGetClientRect(surface->hwnd, &client_rect, NtUserGetDpiForWindow(surface->hwnd)) &&
        !extents_equals(&swapchain->extents, &client_rect))
    {
        WARN("Swapchain size %dx%d does not match client rect %s, returning VK_SUBOPTIMAL_KHR\n",
             swapchain->extents.width, swapchain->extents.height, wine_dbgstr_rect(&client_rect));
        return VK_SUBOPTIMAL_KHR;
    }

    return res;
}

VkResult wine_vkCreateSwapchainKHR(VkDevice device_handle, const VkSwapchainCreateInfoKHR *create_info,
                                   const VkAllocationCallbacks *allocator, VkSwapchainKHR *swapchain_handle)
{
    struct wine_swapchain *object, *old_swapchain = wine_swapchain_from_handle(create_info->oldSwapchain);
    struct wine_surface *surface = wine_surface_from_handle(create_info->surface);
    struct wine_device *device = wine_device_from_handle(device_handle);
    struct wine_phys_dev *physical_device = device->phys_dev;
    struct wine_instance *instance = physical_device->instance;
    VkSwapchainCreateInfoKHR create_info_host = *create_info;
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult res;

    if (!NtUserIsWindow(surface->hwnd))
    {
        ERR("surface %p, hwnd %p is invalid!\n", surface, surface->hwnd);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (surface) create_info_host.surface = surface->host_surface;
    if (old_swapchain) create_info_host.oldSwapchain = old_swapchain->host_swapchain;

    /* update the host surface to commit any pending size change */
    vk_funcs->p_vulkan_surface_update( surface->driver_surface );

    /* Windows allows client rect to be empty, but host Vulkan often doesn't, adjust extents back to the host capabilities */
    res = instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->host_physical_device,
                                                                      surface->host_surface, &capabilities);
    if (res != VK_SUCCESS) return res;

    create_info_host.imageExtent.width = max(create_info_host.imageExtent.width, capabilities.minImageExtent.width);
    create_info_host.imageExtent.height = max(create_info_host.imageExtent.height, capabilities.minImageExtent.height);

    if (!(object = calloc(1, sizeof(*object)))) return VK_ERROR_OUT_OF_HOST_MEMORY;
    res = device->funcs.p_vkCreateSwapchainKHR(device->host_device, &create_info_host, NULL, &object->host_swapchain);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    object->surface = surface;
    object->extents = create_info->imageExtent;

    *swapchain_handle = wine_swapchain_to_handle(object);
    add_handle_mapping(instance, *swapchain_handle, object->host_swapchain, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroySwapchainKHR(VkDevice device_handle, VkSwapchainKHR swapchain_handle,
                                const VkAllocationCallbacks *allocator)
{
    struct wine_device *device = wine_device_from_handle(device_handle);
    struct wine_swapchain *swapchain = wine_swapchain_from_handle(swapchain_handle);

    if (allocator) FIXME("Support for allocation callbacks not implemented yet\n");
    if (!swapchain) return;

    device->funcs.p_vkDestroySwapchainKHR(device->host_device, swapchain->host_swapchain, NULL);
    remove_handle_mapping(device->phys_dev->instance, &swapchain->wrapper_entry);

    free(swapchain);
}

VkResult wine_vkQueuePresentKHR(VkQueue queue_handle, const VkPresentInfoKHR *present_info)
{
    VkSwapchainKHR swapchains_buffer[16], *swapchains = swapchains_buffer;
    VkSurfaceKHR surfaces_buffer[ARRAY_SIZE(swapchains_buffer)], *surfaces = surfaces_buffer;
    struct wine_queue *queue = wine_queue_from_handle(queue_handle);
    VkPresentInfoKHR present_info_host = *present_info;
    VkResult res;
    UINT i;

    if (present_info->swapchainCount > ARRAY_SIZE(swapchains_buffer) &&
        (!(swapchains = malloc(present_info->swapchainCount * sizeof(*swapchains))) ||
         !(surfaces = malloc(present_info->swapchainCount * sizeof(*surfaces)))))
    {
        free(swapchains);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    for (i = 0; i < present_info->swapchainCount; i++)
    {
        struct wine_swapchain *swapchain = wine_swapchain_from_handle(present_info->pSwapchains[i]);
        struct wine_surface *surface = swapchain->surface;
        swapchains[i] = swapchain->host_swapchain;
        surfaces[i] = surface->driver_surface;
    }

    present_info_host.pSwapchains = swapchains;

    res = vk_funcs->p_vkQueuePresentKHR(queue->host_queue, &present_info_host, surfaces);

    for (i = 0; i < present_info->swapchainCount; i++)
    {
        struct wine_swapchain *swapchain = wine_swapchain_from_handle(present_info->pSwapchains[i]);
        VkResult swapchain_res = present_info->pResults ? present_info->pResults[i] : res;
        struct wine_surface *surface = swapchain->surface;
        RECT client_rect;

        if (swapchain_res < VK_SUCCESS) continue;
        if (!NtUserGetClientRect(surface->hwnd, &client_rect, NtUserGetDpiForWindow(surface->hwnd)))
        {
            WARN("Swapchain window %p is invalid, returning VK_ERROR_OUT_OF_DATE_KHR\n", surface->hwnd);
            if (present_info->pResults) present_info->pResults[i] = VK_ERROR_OUT_OF_DATE_KHR;
            if (res >= VK_SUCCESS) res = VK_ERROR_OUT_OF_DATE_KHR;
        }
        else if (swapchain_res != VK_SUCCESS)
            WARN("Present returned status %d for swapchain %p\n", swapchain_res, swapchain);
        else if (!extents_equals(&swapchain->extents, &client_rect))
        {
            WARN("Swapchain size %dx%d does not match client rect %s, returning VK_SUBOPTIMAL_KHR\n",
                    swapchain->extents.width, swapchain->extents.height, wine_dbgstr_rect(&client_rect));
            if (present_info->pResults) present_info->pResults[i] = VK_SUBOPTIMAL_KHR;
            if (res == VK_SUCCESS) res = VK_SUBOPTIMAL_KHR;
        }
    }

    if (swapchains != swapchains_buffer) free(swapchains);
    if (surfaces != surfaces_buffer) free(surfaces);

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
            TRACE_(fps)("%p @ approx %.2ffps, total %.2ffps\n", queue,
                        1000.0 * frames / (time - prev_time),
                        1000.0 * frames_total / (time - start_time));
            prev_time = time;
            frames = 0;

            if (!start_time) start_time = time;
        }
    }

    return res;
}

VkResult wine_vkAllocateMemory(VkDevice handle, const VkMemoryAllocateInfo *alloc_info,
                               const VkAllocationCallbacks *allocator, VkDeviceMemory *ret)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_device_memory *memory;
    VkMemoryAllocateInfo info = *alloc_info;
    VkImportMemoryHostPointerInfoEXT host_pointer_info;
    uint32_t mem_flags;
    void *mapping = NULL;
    VkResult result;

    /* For host visible memory, we try to use VK_EXT_external_memory_host on wow64
     * to ensure that mapped pointer is 32-bit. */
    mem_flags = device->phys_dev->memory_properties.memoryTypes[alloc_info->memoryTypeIndex].propertyFlags;
    if (device->phys_dev->external_memory_align && (mem_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
        !find_next_struct(alloc_info->pNext, VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT))
    {
        VkMemoryHostPointerPropertiesEXT props =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT,
        };
        uint32_t i, align = device->phys_dev->external_memory_align - 1;
        SIZE_T alloc_size = info.allocationSize;
        static int once;

        if (!once++)
            FIXME("Using VK_EXT_external_memory_host\n");

        if (NtAllocateVirtualMemory(GetCurrentProcess(), &mapping, zero_bits, &alloc_size,
                                    MEM_COMMIT, PAGE_READWRITE))
        {
            ERR("NtAllocateVirtualMemory failed\n");
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        result = device->funcs.p_vkGetMemoryHostPointerPropertiesEXT(device->host_device,
                VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, mapping, &props);
        if (result != VK_SUCCESS)
        {
            ERR("vkGetMemoryHostPointerPropertiesEXT failed: %d\n", result);
            return result;
        }

        if (!(props.memoryTypeBits & (1u << info.memoryTypeIndex)))
        {
            /* If requested memory type is not allowed to use external memory,
             * try to find a supported compatible type. */
            uint32_t mask = mem_flags & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            for (i = 0; i < device->phys_dev->memory_properties.memoryTypeCount; i++)
            {
                if (!(props.memoryTypeBits & (1u << i)))
                    continue;
                if ((device->phys_dev->memory_properties.memoryTypes[i].propertyFlags & mask) != mask)
                    continue;

                TRACE("Memory type not compatible with host memory, using %u instead\n", i);
                info.memoryTypeIndex = i;
                break;
            }
            if (i == device->phys_dev->memory_properties.memoryTypeCount)
            {
                FIXME("Not found compatible memory type\n");
                alloc_size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), &mapping, &alloc_size, MEM_RELEASE);
            }
        }

        if (props.memoryTypeBits & (1u << info.memoryTypeIndex))
        {
            host_pointer_info.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
            host_pointer_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
            host_pointer_info.pHostPointer = mapping;
            host_pointer_info.pNext = info.pNext;
            info.pNext = &host_pointer_info;

            info.allocationSize = (info.allocationSize + align) & ~align;
        }
    }

    if (!(memory = malloc(sizeof(*memory))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    result = device->funcs.p_vkAllocateMemory(device->host_device, &info, NULL, &memory->host_memory);
    if (result != VK_SUCCESS)
    {
        free(memory);
        return result;
    }

    memory->size = info.allocationSize;
    memory->vm_map = mapping;

    *ret = (VkDeviceMemory)(uintptr_t)memory;
    add_handle_mapping(device->phys_dev->instance, *ret, memory->host_memory, &memory->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkFreeMemory(VkDevice handle, VkDeviceMemory memory_handle, const VkAllocationCallbacks *allocator)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_device_memory *memory;

    if (!memory_handle)
        return;
    memory = wine_device_memory_from_handle(memory_handle);

    if (memory->vm_map && !device->phys_dev->external_memory_align)
    {
        const VkMemoryUnmapInfoKHR info =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO_KHR,
            .memory = memory->host_memory,
            .flags = VK_MEMORY_UNMAP_RESERVE_BIT_EXT,
        };
        device->funcs.p_vkUnmapMemory2KHR(device->host_device, &info);
    }

    device->funcs.p_vkFreeMemory(device->host_device, memory->host_memory, NULL);
    remove_handle_mapping(device->phys_dev->instance, &memory->wrapper_entry);

    if (memory->vm_map)
    {
        SIZE_T alloc_size = 0;
        NtFreeVirtualMemory(GetCurrentProcess(), &memory->vm_map, &alloc_size, MEM_RELEASE);
    }

    free(memory);
}

VkResult wine_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset,
                          VkDeviceSize size, VkMemoryMapFlags flags, void **data)
{
    const VkMemoryMapInfoKHR info =
    {
      .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
      .flags = flags,
      .memory = memory,
      .offset = offset,
      .size = size,
   };

   return wine_vkMapMemory2KHR(device, &info, data);
}

VkResult wine_vkMapMemory2KHR(VkDevice handle, const VkMemoryMapInfoKHR *map_info, void **data)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_device_memory *memory = wine_device_memory_from_handle(map_info->memory);
    VkMemoryMapInfoKHR info = *map_info;
    VkMemoryMapPlacedInfoEXT placed_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_PLACED_INFO_EXT,
    };
    VkResult result;

    info.memory = memory->host_memory;
    if (memory->vm_map)
    {
        *data = (char *)memory->vm_map + info.offset;
        TRACE("returning %p\n", *data);
        return VK_SUCCESS;
    }

    if (device->phys_dev->map_placed_align)
    {
        SIZE_T alloc_size = memory->size;

        placed_info.pNext = info.pNext;
        info.pNext = &placed_info;
        info.offset = 0;
        info.size = VK_WHOLE_SIZE;
        info.flags |=  VK_MEMORY_MAP_PLACED_BIT_EXT;

        if (NtAllocateVirtualMemory(GetCurrentProcess(), &placed_info.pPlacedAddress, zero_bits, &alloc_size,
                                    MEM_COMMIT, PAGE_READWRITE))
        {
            ERR("NtAllocateVirtualMemory failed\n");
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
    }

    if (device->funcs.p_vkMapMemory2KHR)
    {
        result = device->funcs.p_vkMapMemory2KHR(device->host_device, &info, data);
    }
    else
    {
        assert(!info.pNext);
        result = device->funcs.p_vkMapMemory(device->host_device, info.memory, info.offset,
                                             info.size, info.flags, data);
    }

    if (placed_info.pPlacedAddress)
    {
        if (result != VK_SUCCESS)
        {
            SIZE_T alloc_size = 0;
            ERR("vkMapMemory2EXT failed: %d\n", result);
            NtFreeVirtualMemory(GetCurrentProcess(), &placed_info.pPlacedAddress, &alloc_size, MEM_RELEASE);
            return result;
        }
        memory->vm_map = placed_info.pPlacedAddress;
        *data = (char *)memory->vm_map + map_info->offset;
        TRACE("Using placed mapping %p\n", memory->vm_map);
    }

#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset && result == VK_SUCCESS && (UINT_PTR)*data >> 32)
    {
        FIXME("returned mapping %p does not fit 32-bit pointer\n", *data);
        device->funcs.p_vkUnmapMemory(device->host_device, memory->host_memory);
        *data = NULL;
        result = VK_ERROR_OUT_OF_HOST_MEMORY;
    }
#endif

    return result;
}

void wine_vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    const VkMemoryUnmapInfoKHR info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO_KHR,
        .memory = memory,
    };

    wine_vkUnmapMemory2KHR(device, &info);
}

VkResult wine_vkUnmapMemory2KHR(VkDevice handle, const VkMemoryUnmapInfoKHR *unmap_info)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_device_memory *memory = wine_device_memory_from_handle(unmap_info->memory);
    VkMemoryUnmapInfoKHR info;
    VkResult result;

    if (memory->vm_map && device->phys_dev->external_memory_align)
        return VK_SUCCESS;

    if (!device->funcs.p_vkUnmapMemory2KHR)
    {
        assert(!unmap_info->pNext && !memory->vm_map);
        device->funcs.p_vkUnmapMemory(device->host_device, memory->host_memory);
        return VK_SUCCESS;
    }

    info = *unmap_info;
    info.memory = memory->host_memory;
    if (memory->vm_map)
        info.flags |= VK_MEMORY_UNMAP_RESERVE_BIT_EXT;

    result = device->funcs.p_vkUnmapMemory2KHR(device->host_device, &info);

    if (result == VK_SUCCESS && memory->vm_map)
    {
        SIZE_T size = 0;
        NtFreeVirtualMemory(GetCurrentProcess(), &memory->vm_map, &size, MEM_RELEASE);
        memory->vm_map = NULL;
    }
    return result;
}

VkResult wine_vkCreateBuffer(VkDevice handle, const VkBufferCreateInfo *create_info,
                             const VkAllocationCallbacks *allocator, VkBuffer *buffer)
{
    struct wine_device *device = wine_device_from_handle(handle);
    VkExternalMemoryBufferCreateInfo external_memory_info;
    VkBufferCreateInfo info = *create_info;

    if (device->phys_dev->external_memory_align &&
        !find_next_struct(info.pNext, VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO))
    {
        external_memory_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        external_memory_info.pNext = info.pNext;
        external_memory_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
        info.pNext = &external_memory_info;
    }

    return device->funcs.p_vkCreateBuffer(device->host_device, &info, NULL, buffer);
}

VkResult wine_vkCreateImage(VkDevice handle, const VkImageCreateInfo *create_info,
                            const VkAllocationCallbacks *allocator, VkImage *image)
{
    struct wine_device *device = wine_device_from_handle(handle);
    VkExternalMemoryImageCreateInfo external_memory_info;
    VkImageCreateInfo info = *create_info;

    if (device->phys_dev->external_memory_align &&
        !find_next_struct(info.pNext, VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO))
    {
        external_memory_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        external_memory_info.pNext = info.pNext;
        external_memory_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
        info.pNext = &external_memory_info;
    }

    return device->funcs.p_vkCreateImage(device->host_device, &info, NULL, image);
}

static void adjust_surface_capabilities(struct wine_instance *instance, struct wine_surface *surface,
                                        VkSurfaceCapabilitiesKHR *capabilities)
{
    RECT client_rect;

    /* Many Windows games, for example Strange Brigade, No Man's Sky, Path of Exile
     * and World War Z, do not expect that maxImageCount can be set to 0.
     * A value of 0 means that there is no limit on the number of images.
     * Nvidia reports 8 on Windows, AMD 16.
     * https://vulkan.gpuinfo.org/displayreport.php?id=9122#surface
     * https://vulkan.gpuinfo.org/displayreport.php?id=9121#surface
     */
    if (!capabilities->maxImageCount)
        capabilities->maxImageCount = max(capabilities->minImageCount, 16);

    /* Update the image extents to match what the Win32 WSI would provide. */
    /* FIXME: handle DPI scaling, somehow */
    NtUserGetClientRect(surface->hwnd, &client_rect, NtUserGetDpiForWindow(surface->hwnd));
    capabilities->minImageExtent.width = client_rect.right - client_rect.left;
    capabilities->minImageExtent.height = client_rect.bottom - client_rect.top;
    capabilities->maxImageExtent.width = client_rect.right - client_rect.left;
    capabilities->maxImageExtent.height = client_rect.bottom - client_rect.top;
    capabilities->currentExtent.width = client_rect.right - client_rect.left;
    capabilities->currentExtent.height = client_rect.bottom - client_rect.top;
}

VkResult wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice device_handle, VkSurfaceKHR surface_handle,
                                                        VkSurfaceCapabilitiesKHR *capabilities)
{
    struct wine_phys_dev *physical_device = wine_phys_dev_from_handle(device_handle);
    struct wine_surface *surface = wine_surface_from_handle(surface_handle);
    struct wine_instance *instance = physical_device->instance;
    VkResult res;

    if (!NtUserIsWindow(surface->hwnd)) return VK_ERROR_SURFACE_LOST_KHR;
    res = instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->host_physical_device,
                                                                      surface->host_surface, capabilities);
    if (res == VK_SUCCESS) adjust_surface_capabilities(instance, surface, capabilities);
    return res;
}

VkResult wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice device_handle, const VkPhysicalDeviceSurfaceInfo2KHR *surface_info,
                                                         VkSurfaceCapabilities2KHR *capabilities)
{
    struct wine_phys_dev *physical_device = wine_phys_dev_from_handle(device_handle);
    struct wine_surface *surface = wine_surface_from_handle(surface_info->surface);
    VkPhysicalDeviceSurfaceInfo2KHR surface_info_host = *surface_info;
    struct wine_instance *instance = physical_device->instance;
    VkResult res;

    if (!instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR)
    {
        /* Until the loader version exporting this function is common, emulate it using the older non-2 version. */
        if (surface_info->pNext || capabilities->pNext) FIXME("Emulating vkGetPhysicalDeviceSurfaceCapabilities2KHR, ignoring pNext.\n");
        return wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_handle, surface_info->surface,
                                                              &capabilities->surfaceCapabilities);
    }

    surface_info_host.surface = surface->host_surface;

    if (!NtUserIsWindow(surface->hwnd)) return VK_ERROR_SURFACE_LOST_KHR;
    res = instance->funcs.p_vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device->host_physical_device,
                                                                       &surface_info_host, capabilities);
    if (res == VK_SUCCESS) adjust_surface_capabilities(instance, surface, &capabilities->surfaceCapabilities);
    return res;
}

VkResult wine_vkGetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice device_handle, VkSurfaceKHR surface_handle,
                                                      uint32_t *rect_count, VkRect2D *rects)
{
    struct wine_phys_dev *physical_device = wine_phys_dev_from_handle(device_handle);
    struct wine_surface *surface = wine_surface_from_handle(surface_handle);
    struct wine_instance *instance = physical_device->instance;

    if (!NtUserIsWindow(surface->hwnd))
    {
        if (rects && !*rect_count) return VK_INCOMPLETE;
        if (rects) memset(rects, 0, sizeof(VkRect2D));
        *rect_count = 1;
        return VK_SUCCESS;
    }

    return instance->funcs.p_vkGetPhysicalDevicePresentRectanglesKHR(physical_device->host_physical_device,
                                                                     surface->host_surface, rect_count, rects);
}

VkResult wine_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device_handle, VkSurfaceKHR surface_handle,
                                                   uint32_t *format_count, VkSurfaceFormatKHR *formats)
{
    struct wine_phys_dev *physical_device = wine_phys_dev_from_handle(device_handle);
    struct wine_surface *surface = wine_surface_from_handle(surface_handle);
    struct wine_instance *instance = physical_device->instance;

    return instance->funcs.p_vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->host_physical_device, surface->host_surface,
                                                                  format_count, formats);
}

VkResult wine_vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice device_handle, const VkPhysicalDeviceSurfaceInfo2KHR *surface_info,
                                                    uint32_t *format_count, VkSurfaceFormat2KHR *formats)
{
    struct wine_phys_dev *physical_device = wine_phys_dev_from_handle(device_handle);
    struct wine_surface *surface = wine_surface_from_handle(surface_info->surface);
    VkPhysicalDeviceSurfaceInfo2KHR surface_info_host = *surface_info;
    struct wine_instance *instance = physical_device->instance;
    VkResult res;

    if (!physical_device->instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR)
    {
        VkSurfaceFormatKHR *surface_formats;
        UINT i;

        /* Until the loader version exporting this function is common, emulate it using the older non-2 version. */
        if (surface_info->pNext) FIXME("Emulating vkGetPhysicalDeviceSurfaceFormats2KHR, ignoring pNext.\n");

        if (!formats) return wine_vkGetPhysicalDeviceSurfaceFormatsKHR(device_handle, surface_info->surface, format_count, NULL);

        surface_formats = calloc(*format_count, sizeof(*surface_formats));
        if (!surface_formats) return VK_ERROR_OUT_OF_HOST_MEMORY;

        res = wine_vkGetPhysicalDeviceSurfaceFormatsKHR(device_handle, surface_info->surface, format_count, surface_formats);
        if (res == VK_SUCCESS || res == VK_INCOMPLETE)
        {
            for (i = 0; i < *format_count; i++)
                formats[i].surfaceFormat = surface_formats[i];
        }

        free(surface_formats);
        return res;
    }

    surface_info_host.surface = surface->host_surface;

    return instance->funcs.p_vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device->host_physical_device,
                                                                   &surface_info_host, format_count, formats);
}

VkResult wine_vkCreateDebugUtilsMessengerEXT(VkInstance handle,
                                             const VkDebugUtilsMessengerCreateInfoEXT *create_info,
                                             const VkAllocationCallbacks *allocator,
                                             VkDebugUtilsMessengerEXT *messenger)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    VkDebugUtilsMessengerCreateInfoEXT wine_create_info;
    struct wine_debug_utils_messenger *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->instance = instance;
    object->user_callback = (UINT_PTR)create_info->pfnUserCallback;
    object->user_data = (UINT_PTR)create_info->pUserData;

    wine_create_info = *create_info;

    wine_create_info.pfnUserCallback = (void *) &debug_utils_callback_conversion;
    wine_create_info.pUserData = object;

    res = instance->funcs.p_vkCreateDebugUtilsMessengerEXT(instance->host_instance, &wine_create_info,
                                                           NULL, &object->host_debug_messenger);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    *messenger = wine_debug_utils_messenger_to_handle(object);
    add_handle_mapping(instance, *messenger, object->host_debug_messenger, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroyDebugUtilsMessengerEXT(VkInstance handle, VkDebugUtilsMessengerEXT messenger,
                                          const VkAllocationCallbacks *allocator)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    struct wine_debug_utils_messenger *object;

    object = wine_debug_utils_messenger_from_handle(messenger);

    if (!object)
        return;

    instance->funcs.p_vkDestroyDebugUtilsMessengerEXT(instance->host_instance, object->host_debug_messenger, NULL);
    remove_handle_mapping(instance, &object->wrapper_entry);

    free(object);
}

VkResult wine_vkCreateDebugReportCallbackEXT(VkInstance handle,
                                             const VkDebugReportCallbackCreateInfoEXT *create_info,
                                             const VkAllocationCallbacks *allocator,
                                             VkDebugReportCallbackEXT *callback)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    VkDebugReportCallbackCreateInfoEXT wine_create_info;
    struct wine_debug_report_callback *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->instance = instance;
    object->user_callback = (UINT_PTR)create_info->pfnCallback;
    object->user_data = (UINT_PTR)create_info->pUserData;

    wine_create_info = *create_info;

    wine_create_info.pfnCallback = (void *) debug_report_callback_conversion;
    wine_create_info.pUserData = object;

    res = instance->funcs.p_vkCreateDebugReportCallbackEXT(instance->host_instance, &wine_create_info,
                                                           NULL, &object->host_debug_callback);
    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    *callback = wine_debug_report_callback_to_handle(object);
    add_handle_mapping(instance, *callback, object->host_debug_callback, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroyDebugReportCallbackEXT(VkInstance handle, VkDebugReportCallbackEXT callback,
                                          const VkAllocationCallbacks *allocator)
{
    struct wine_instance *instance = wine_instance_from_handle(handle);
    struct wine_debug_report_callback *object;

    object = wine_debug_report_callback_from_handle(callback);

    if (!object)
        return;

    instance->funcs.p_vkDestroyDebugReportCallbackEXT(instance->host_instance, object->host_debug_callback, NULL);
    remove_handle_mapping(instance, &object->wrapper_entry);

    free(object);
}

VkResult wine_vkCreateDeferredOperationKHR(VkDevice                     handle,
                                           const VkAllocationCallbacks* allocator,
                                           VkDeferredOperationKHR*      operation)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_deferred_operation *object;
    VkResult res;

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = calloc(1, sizeof(*object))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = device->funcs.p_vkCreateDeferredOperationKHR(device->host_device, NULL, &object->host_deferred_operation);

    if (res != VK_SUCCESS)
    {
        free(object);
        return res;
    }

    init_conversion_context(&object->ctx);

    *operation = wine_deferred_operation_to_handle(object);
    add_handle_mapping(device->phys_dev->instance, *operation, object->host_deferred_operation, &object->wrapper_entry);
    return VK_SUCCESS;
}

void wine_vkDestroyDeferredOperationKHR(VkDevice                     handle,
                                        VkDeferredOperationKHR       operation,
                                        const VkAllocationCallbacks* allocator)
{
    struct wine_device *device = wine_device_from_handle(handle);
    struct wine_deferred_operation *object;

    object = wine_deferred_operation_from_handle(operation);

    if (!object)
        return;

    device->funcs.p_vkDestroyDeferredOperationKHR(device->host_device, object->host_deferred_operation, NULL);
    remove_handle_mapping(device->phys_dev->instance, &object->wrapper_entry);

    free_conversion_context(&object->ctx);
    free(object);
}

#ifdef _WIN64

NTSTATUS vk_is_available_instance_function(void *arg)
{
    struct is_available_instance_function_params *params = arg;
    struct wine_instance *instance = wine_instance_from_handle(params->instance);

    if (!strcmp(params->name, "vkCreateWin32SurfaceKHR"))
        return instance->enable_win32_surface;
    if (!strcmp(params->name, "vkGetPhysicalDeviceWin32PresentationSupportKHR"))
        return instance->enable_win32_surface;

    return !!vk_funcs->p_vkGetInstanceProcAddr(instance->host_instance, params->name);
}

NTSTATUS vk_is_available_device_function(void *arg)
{
    struct is_available_device_function_params *params = arg;
    struct wine_device *device = wine_device_from_handle(params->device);
    return !!vk_funcs->p_vkGetDeviceProcAddr(device->host_device, params->name);
}

#endif /* _WIN64 */

NTSTATUS vk_is_available_instance_function32(void *arg)
{
    struct
    {
        UINT32 instance;
        UINT32 name;
    } *params = arg;
    struct wine_instance *instance = wine_instance_from_handle(UlongToPtr(params->instance));
    return !!vk_funcs->p_vkGetInstanceProcAddr(instance->host_instance, UlongToPtr(params->name));
}

NTSTATUS vk_is_available_device_function32(void *arg)
{
    struct
    {
        UINT32 device;
        UINT32 name;
    } *params = arg;
    struct wine_device *device = wine_device_from_handle(UlongToPtr(params->device));
    return !!vk_funcs->p_vkGetDeviceProcAddr(device->host_device, UlongToPtr(params->name));
}
