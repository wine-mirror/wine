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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "vulkan_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

/* For now default to 4 as it felt like a reasonable version feature wise to support.
 * Don't support the optional vk_icdGetPhysicalDeviceProcAddr introduced in this version
 * as it is unlikely we will implement physical device extensions, which the loader is not
 * aware off. Version 5 adds more extensive version checks. Something to tackle later.
 */
#define WINE_VULKAN_ICD_VERSION 4

/* All Vulkan structures use this structure for the first elements. */
struct wine_vk_structure_header
{
    VkStructureType sType;
    const void *pNext;
};

static void *wine_vk_get_global_proc_addr(const char *name);

static const struct vulkan_funcs *vk_funcs;

static void wine_vk_physical_device_free(struct VkPhysicalDevice_T *phys_dev)
{
    if (!phys_dev)
        return;

    heap_free(phys_dev->extensions);
    heap_free(phys_dev);
}

static struct VkPhysicalDevice_T *wine_vk_physical_device_alloc(struct VkInstance_T *instance,
        VkPhysicalDevice phys_dev)
{
    struct VkPhysicalDevice_T *object;
    uint32_t num_host_properties, num_properties = 0;
    VkExtensionProperties *host_properties = NULL;
    VkResult res;
    unsigned int i, j;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return NULL;

    object->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
    object->instance = instance;
    object->phys_dev = phys_dev;

    res = instance->funcs.p_vkEnumerateDeviceExtensionProperties(phys_dev,
            NULL, &num_host_properties, NULL);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to enumerate device extensions, res=%d\n", res);
        goto err;
    }

    host_properties = heap_calloc(num_host_properties, sizeof(*host_properties));
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

    if (!(object->extensions = heap_calloc(num_properties, sizeof(*object->extensions))))
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

    heap_free(host_properties);
    return object;

err:
    wine_vk_physical_device_free(object);
    heap_free(host_properties);
    return NULL;
}

/* Helper function to release command buffers. */
static void wine_vk_command_buffers_free(struct VkDevice_T *device, VkCommandPool pool,
        uint32_t count, const VkCommandBuffer *buffers)
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        if (!buffers[i])
            continue;

        device->funcs.p_vkFreeCommandBuffers(device->device, pool, 1, &buffers[i]->command_buffer);
        heap_free(buffers[i]);
    }
}

/* Helper function to create queues for a given family index. */
static struct VkQueue_T *wine_vk_device_alloc_queues(struct VkDevice_T *device,
        uint32_t family_index, uint32_t queue_count)
{
    struct VkQueue_T *queues;
    unsigned int i;

    if (!(queues = heap_calloc(queue_count, sizeof(*queues))))
    {
        ERR("Failed to allocate memory for queues\n");
        return NULL;
    }

    for (i = 0; i < queue_count; i++)
    {
        struct VkQueue_T *queue = &queues[i];
        queue->device = device;

        /* The native device was already allocated with the required number of queues,
         * so just fetch them from there.
         */
        device->funcs.p_vkGetDeviceQueue(device->device, family_index, i, &queue->queue);

        /* Set special header for ICD loader. */
        queue->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
    }

    return queues;
}

/* Helper function to convert win32 VkDeviceCreateInfo to host compatible. */
static void wine_vk_device_convert_create_info(const VkDeviceCreateInfo *src,
        VkDeviceCreateInfo *dst)
{
    unsigned int i;

    *dst = *src;

    /* Application and loader can pass in a chain of extensions through pNext.
     * We can't blindly pass these through as often these contain callbacks or
     * they can even be pass structures for loader / ICD internal use. For now
     * we ignore everything in pNext chain, but we print FIXMEs.
     */
    if (src->pNext)
    {
        const struct wine_vk_structure_header *header;

        for (header = src->pNext; header; header = header->pNext)
        {
            switch (header->sType)
            {
                case VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO:
                    /* Used for loader to ICD communication. Ignore to not confuse
                     * host loader.
                     */
                    break;

                default:
                    FIXME("Application requested a linked structure of type %#x.\n", header->sType);
            }
        }
    }
    /* For now don't support anything. */
    dst->pNext = NULL;

    /* Should be filtered out by loader as ICDs don't support layers. */
    dst->enabledLayerCount = 0;
    dst->ppEnabledLayerNames = NULL;

    TRACE("Enabled extensions: %u\n", dst->enabledExtensionCount);
    for (i = 0; i < dst->enabledExtensionCount; i++)
    {
        TRACE("Extension %u: %s\n", i, debugstr_a(dst->ppEnabledExtensionNames[i]));
    }
}

/* Helper function used for freeing a device structure. This function supports full
 * and partial object cleanups and can thus be used for vkCreateDevice failures.
 */
static void wine_vk_device_free(struct VkDevice_T *device)
{
    if (!device)
        return;

    if (device->queues)
    {
        unsigned int i;
        for (i = 0; i < device->max_queue_families; i++)
        {
            heap_free(device->queues[i]);
        }
        heap_free(device->queues);
        device->queues = NULL;
    }

    if (device->device && device->funcs.p_vkDestroyDevice)
    {
        device->funcs.p_vkDestroyDevice(device->device, NULL /* pAllocator */);
    }

    heap_free(device);
}

static BOOL wine_vk_init(void)
{
    HDC hdc;

    hdc = GetDC(0);
    vk_funcs = __wine_get_vulkan_driver(hdc, WINE_VULKAN_DRIVER_VERSION);
    ReleaseDC(0, hdc);
    if (!vk_funcs)
    {
        ERR("Failed to load Wine graphics driver supporting Vulkan.\n");
        return FALSE;
    }

    return TRUE;
}

/* Helper function for converting between win32 and host compatible VkInstanceCreateInfo.
 * This function takes care of extensions handled at winevulkan layer, a Wine graphics
 * driver is responsible for handling e.g. surface extensions.
 */
static void wine_vk_instance_convert_create_info(const VkInstanceCreateInfo *src,
        VkInstanceCreateInfo *dst)
{
    unsigned int i;

    *dst = *src;

    if (dst->pApplicationInfo)
    {
        const VkApplicationInfo *app_info = dst->pApplicationInfo;
        TRACE("Application name %s, application version %#x\n",
                debugstr_a(app_info->pApplicationName), app_info->applicationVersion);
        TRACE("Engine name %s, engine version %#x\n", debugstr_a(app_info->pEngineName),
                 app_info->engineVersion);
        TRACE("API version %#x\n", app_info->apiVersion);
    }

    /* Application and loader can pass in a chain of extensions through pNext.
     * We can't blindly pass these through as often these contain callbacks or
     * they can even be pass structures for loader / ICD internal use. For now
     * we ignore everything in pNext chain, but we print FIXMEs.
     */
    if (src->pNext)
    {
        const struct wine_vk_structure_header *header;

        for (header = src->pNext; header; header = header->pNext)
        {
            switch (header->sType)
            {
                case VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO:
                    /* Can be used to register new dispatchable object types
                     * to the loader. We should ignore it as it will confuse the
                     * host its loader.
                     */
                    break;

                default:
                    FIXME("Application requested a linked structure of type %#x.\n", header->sType);
            }
        }
    }
    /* For now don't support anything. */
    dst->pNext = NULL;

    /* ICDs don't support any layers, so nothing to copy. Modern versions of the loader
     * filter this data out as well.
     */
    dst->enabledLayerCount = 0;
    dst->ppEnabledLayerNames = NULL;

    TRACE("Enabled extensions: %u\n", dst->enabledExtensionCount);
    for (i = 0; i < dst->enabledExtensionCount; i++)
    {
        TRACE("Extension %u: %s\n", i, debugstr_a(dst->ppEnabledExtensionNames[i]));
    }
}

/* Helper function which stores wrapped physical devices in the instance object. */
static VkResult wine_vk_instance_load_physical_devices(struct VkInstance_T *instance)
{
    VkResult res;
    struct VkPhysicalDevice_T **tmp_phys_devs;
    uint32_t num_phys_devs = 0;
    unsigned int i;

    res = instance->funcs.p_vkEnumeratePhysicalDevices(instance->instance, &num_phys_devs, NULL);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to enumerate physical devices, res=%d\n", res);
        return res;
    }

    /* Don't bother with any of the rest if the system just lacks devices. */
    if (num_phys_devs == 0)
        return VK_SUCCESS;

    tmp_phys_devs = heap_calloc(num_phys_devs, sizeof(*tmp_phys_devs));
    if (!tmp_phys_devs)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = instance->funcs.p_vkEnumeratePhysicalDevices(instance->instance, &num_phys_devs, tmp_phys_devs);
    if (res != VK_SUCCESS)
    {
        heap_free(tmp_phys_devs);
        return res;
    }

    instance->phys_devs = heap_calloc(num_phys_devs, sizeof(*instance->phys_devs));
    if (!instance->phys_devs)
    {
        heap_free(tmp_phys_devs);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Wrap each native physical device handle into a dispatchable object for the ICD loader. */
    for (i = 0; i < num_phys_devs; i++)
    {
        struct VkPhysicalDevice_T *phys_dev = wine_vk_physical_device_alloc(instance, tmp_phys_devs[i]);
        if (!phys_dev)
        {
            ERR("Unable to allocate memory for physical device!\n");
            heap_free(tmp_phys_devs);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        instance->phys_devs[i] = phys_dev;
        instance->num_phys_devs = i + 1;
    }
    instance->num_phys_devs = num_phys_devs;

    heap_free(tmp_phys_devs);
    return VK_SUCCESS;
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

        for (i = 0; i < instance->num_phys_devs; i++)
        {
            wine_vk_physical_device_free(instance->phys_devs[i]);
        }
        heap_free(instance->phys_devs);
    }

    if (instance->instance)
        vk_funcs->p_vkDestroyInstance(instance->instance, NULL /* allocator */);

    heap_free(instance);
}

VkResult WINAPI wine_vkAllocateCommandBuffers(VkDevice device,
        const VkCommandBufferAllocateInfo *allocate_info, VkCommandBuffer *buffers)
{
    VkResult res = VK_SUCCESS;
    unsigned int i;

    TRACE("%p %p %p\n", device, allocate_info, buffers);

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
        allocate_info_host.commandPool = allocate_info->commandPool;
        allocate_info_host.level = allocate_info->level;
        allocate_info_host.commandBufferCount = 1;

        TRACE("Creating command buffer %u, pool 0x%s, level %#x\n", i,
                wine_dbgstr_longlong(allocate_info_host.commandPool),
                allocate_info_host.level);

        if (!(buffers[i] = heap_alloc_zero(sizeof(*buffers))))
        {
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            break;
        }

        buffers[i]->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
        buffers[i]->device = device;
        res = device->funcs.p_vkAllocateCommandBuffers(device->device,
                &allocate_info_host, &buffers[i]->command_buffer);
        if (res != VK_SUCCESS)
        {
            ERR("Failed to allocate command buffer, res=%d\n", res);
            break;
        }
    }

    if (res != VK_SUCCESS)
    {
        wine_vk_command_buffers_free(device, allocate_info->commandPool, i, buffers);
        memset(buffers, 0, allocate_info->commandBufferCount * sizeof(*buffers));
        return res;
    }

    return VK_SUCCESS;
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
    if (!(tmp_buffers = heap_alloc(count * sizeof(*tmp_buffers))))
    {
        ERR("Failed to allocate memory for temporary command buffers\n");
        return;
    }

    for (i = 0; i < count; i++)
        tmp_buffers[i] = buffers[i]->command_buffer;

    buffer->device->funcs.p_vkCmdExecuteCommands(buffer->command_buffer, count, tmp_buffers);

    heap_free(tmp_buffers);
}

VkResult WINAPI wine_vkCreateDevice(VkPhysicalDevice phys_dev,
        const VkDeviceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkDevice *device)
{
    VkDeviceCreateInfo create_info_host;
    uint32_t max_queue_families;
    struct VkDevice_T *object;
    VkResult res;
    unsigned int i;

    TRACE("%p %p %p %p\n", phys_dev, create_info, allocator, device);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    object->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;

    wine_vk_device_convert_create_info(create_info, &create_info_host);

    res = phys_dev->instance->funcs.p_vkCreateDevice(phys_dev->phys_dev,
            &create_info_host, NULL /* allocator */, &object->device);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create device.\n");
        wine_vk_device_free(object);
        return res;
    }

    /* Just load all function pointers we are aware off. The loader takes care of filtering.
     * We use vkGetDeviceProcAddr as opposed to vkGetInstanceProcAddr for efficiency reasons
     * as functions pass through fewer dispatch tables within the loader.
     */
#define USE_VK_FUNC(name) \
    object->funcs.p_##name = (void *)vk_funcs->p_vkGetDeviceProcAddr(object->device, #name); \
    if (object->funcs.p_##name == NULL) \
        TRACE("Not found %s\n", #name);
    ALL_VK_DEVICE_FUNCS()
#undef USE_VK_FUNC

    /* We need to cache all queues within the device as each requires wrapping since queues are
     * dispatchable objects.
     */
    phys_dev->instance->funcs.p_vkGetPhysicalDeviceQueueFamilyProperties(phys_dev->phys_dev,
            &max_queue_families, NULL);
    object->max_queue_families = max_queue_families;
    TRACE("Max queue families: %u\n", object->max_queue_families);

    object->queues = heap_calloc(max_queue_families, sizeof(*object->queues));
    if (!object->queues)
    {
        wine_vk_device_free(object);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    for (i = 0; i < create_info_host.queueCreateInfoCount; i++)
    {
        uint32_t family_index = create_info_host.pQueueCreateInfos[i].queueFamilyIndex;
        uint32_t queue_count = create_info_host.pQueueCreateInfos[i].queueCount;

        TRACE("queueFamilyIndex %u, queueCount %u\n", family_index, queue_count);

        object->queues[family_index] = wine_vk_device_alloc_queues(object, family_index, queue_count);
        if (!object->queues[family_index])
        {
            ERR("Failed to allocate memory for queues\n");
            wine_vk_device_free(object);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
    }

    object->quirks = phys_dev->instance->quirks;

    *device = object;
    return VK_SUCCESS;
}

VkResult WINAPI wine_vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    VkInstanceCreateInfo create_info_host;
    const VkApplicationInfo *app_info;
    struct VkInstance_T *object;
    VkResult res;

    TRACE("create_info %p, allocator %p, instance %p\n", create_info, allocator, instance);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        ERR("Failed to allocate memory for instance\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    object->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;

    wine_vk_instance_convert_create_info(create_info, &create_info_host);

    res = vk_funcs->p_vkCreateInstance(&create_info_host, NULL /* allocator */, &object->instance);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create instance, res=%d\n", res);
        wine_vk_instance_free(object);
        return res;
    }

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

    if ((app_info = create_info->pApplicationInfo) && app_info->pApplicationName)
    {
        if (!strcmp(app_info->pApplicationName, "DOOM")
                || !strcmp(app_info->pApplicationName, "Wolfenstein II The New Colossus"))
            object->quirks |= WINEVULKAN_QUIRK_GET_DEVICE_PROC_ADDR;
    }

    *instance = object;
    TRACE("Done, instance=%p native_instance=%p\n", object, object->instance);
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
    VkResult res;
    uint32_t num_properties = 0, num_host_properties = 0;
    VkExtensionProperties *host_properties = NULL;
    unsigned int i, j;

    TRACE("%p %p %p\n", layer_name, count, properties);

    /* This shouldn't get called with layer_name set, the ICD loader prevents it. */
    if (layer_name)
    {
        ERR("Layer enumeration not supported from ICD.\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    res = vk_funcs->p_vkEnumerateInstanceExtensionProperties(NULL, &num_host_properties, NULL);
    if (res != VK_SUCCESS)
        return res;

    host_properties = heap_calloc(num_host_properties, sizeof(*host_properties));
    if (!host_properties)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    res = vk_funcs->p_vkEnumerateInstanceExtensionProperties(NULL, &num_host_properties, host_properties);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to retrieve host properties, res=%d\n", res);
        heap_free(host_properties);
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
        TRACE("Returning %u extensions\n", num_properties);
        *count = num_properties;
        heap_free(host_properties);
        return VK_SUCCESS;
    }

    for (i = 0, j = 0; i < num_host_properties && j < *count; i++)
    {
        if (wine_vk_instance_extension_supported(host_properties[i].extensionName))
        {
            TRACE("Enabling extension '%s'\n", host_properties[i].extensionName);
            properties[j] = host_properties[i];
            j++;
        }
    }
    *count = min(*count, num_properties);

    heap_free(host_properties);
    return *count < num_properties ? VK_INCOMPLETE : VK_SUCCESS;
}

VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *count,
        VkPhysicalDevice *devices)
{
    unsigned int i;

    TRACE("%p %p %p\n", instance, count, devices);

    if (!devices)
    {
        *count = instance->num_phys_devs;
        return VK_SUCCESS;
    }

    *count = min(*count, instance->num_phys_devs);
    for (i = 0; i < *count; i++)
    {
        devices[i] = instance->phys_devs[i];
    }

    TRACE("Returning %u devices.\n", *count);
    return *count < instance->num_phys_devs ? VK_INCOMPLETE : VK_SUCCESS;
}

void WINAPI wine_vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count,
        const VkCommandBuffer *buffers)
{
    TRACE("%p 0x%s %u %p\n", device, wine_dbgstr_longlong(pool), count, buffers);

    wine_vk_command_buffers_free(device, pool, count, buffers);
}

PFN_vkVoidFunction WINAPI wine_vkGetDeviceProcAddr(VkDevice device, const char *name)
{
    void *func;
    TRACE("%p, %s\n", device, debugstr_a(name));

    /* The spec leaves return value undefined for a NULL device, let's just return NULL. */
    if (!device || !name)
        return NULL;

    /* Per the spec, we are only supposed to return device functions as in functions
     * for which the first parameter is vkDevice or a child of vkDevice like a
     * vkCommandBuffer or vkQueue.
     * Loader takes care of filtering of extensions which are enabled or not.
     */
    func = wine_vk_get_device_proc_addr(name);
    if (func)
        return func;

    /* vkGetDeviceProcAddr was intended for loading device and subdevice functions.
     * idTech 6 titles such as Doom and Wolfenstein II, however use it also for
     * loading of instance functions. This is undefined behavior as the specification
     * disallows using any of the returned function pointers outside of device /
     * subdevice objects. The games don't actually use the function pointers and if they
     * did, they would crash as VkInstance / VkPhysicalDevice parameters need unwrapping.
     * Khronos clarified behavior in the Vulkan spec and expects drivers to get updated,
     * however it would require both driver and game fixes.
     * https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/2323
     * https://github.com/KhronosGroup/Vulkan-Docs/issues/655
     */
    if (device->quirks & WINEVULKAN_QUIRK_GET_DEVICE_PROC_ADDR
            && (func = wine_vk_get_instance_proc_addr(name)))
    {
        WARN("Returning instance function %s.\n", debugstr_a(name));
        return func;
    }

    TRACE("Function %s not found.\n", debugstr_a(name));
    return NULL;
}

void WINAPI wine_vkGetDeviceQueue(VkDevice device, uint32_t family_index,
        uint32_t queue_index, VkQueue *queue)
{
    TRACE("%p %u %u %p\n", device, family_index, queue_index, queue);

    *queue = &device->queues[family_index][queue_index];
}

PFN_vkVoidFunction WINAPI wine_vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    void *func;

    TRACE("%p %s\n", instance, debugstr_a(name));

    if (!name)
        return NULL;

    /* vkGetInstanceProcAddr can load most Vulkan functions when an instance is passed in, however
     * for a NULL instance it can only load global functions.
     */
    func = wine_vk_get_global_proc_addr(name);
    if (func)
    {
        return func;
    }
    if (!instance)
    {
        FIXME("Global function %s not found.\n", debugstr_a(name));
        return NULL;
    }

    func = wine_vk_get_instance_proc_addr(name);
    if (func) return func;

    /* vkGetInstanceProcAddr also loads any children of instance, so device functions as well. */
    func = wine_vk_get_device_proc_addr(name);
    if (func) return func;

    FIXME("Unsupported device or instance function: %s.\n", debugstr_a(name));
    return NULL;
}

void * WINAPI wine_vk_icdGetInstanceProcAddr(VkInstance instance, const char *name)
{
    TRACE("%p %s\n", instance, debugstr_a(name));

    /* Initial version of the Vulkan ICD spec required vkGetInstanceProcAddr to be
     * exported. vk_icdGetInstanceProcAddr was added later to separate ICD calls from
     * Vulkan API. One of them in our case should forward to the other, so just forward
     * to the older vkGetInstanceProcAddr.
     */
    return wine_vkGetInstanceProcAddr(instance, name);
}

VkResult WINAPI wine_vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *supported_version)
{
    uint32_t req_version;

    TRACE("%p\n", supported_version);

    /* The spec is not clear how to handle this. Mesa drivers don't check, but it
     * is probably best to not explode. VK_INCOMPLETE seems to be the closest value.
     */
    if (!supported_version)
        return VK_INCOMPLETE;

    req_version = *supported_version;
    *supported_version = min(req_version, WINE_VULKAN_ICD_VERSION);
    TRACE("Loader requested ICD version %u, returning %u\n", req_version, *supported_version);

    return VK_SUCCESS;
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

    submits_host = heap_calloc(count, sizeof(*submits_host));
    if (!submits_host)
    {
        ERR("Unable to allocate memory for submit buffers!\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    for (i = 0; i < count; i++)
    {
        memcpy(&submits_host[i], &submits[i], sizeof(*submits_host));

        num_command_buffers = submits[i].commandBufferCount;
        command_buffers = heap_calloc(num_command_buffers, sizeof(*submits_host));
        if (!command_buffers)
        {
            ERR("Unable to allocate memory for comman buffers!\n");
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            goto err;
        }

        for (j = 0; j < num_command_buffers; j++)
        {
            command_buffers[j] = submits[i].pCommandBuffers[j]->command_buffer;
        }
        submits_host[i].pCommandBuffers = command_buffers;
    }

    res = queue->device->funcs.p_vkQueueSubmit(queue->queue, count, submits_host, fence);

err:
    for (i = 0; i < count; i++)
    {
        heap_free((void *)submits_host[i].pCommandBuffers);
    }
    heap_free(submits_host);

    TRACE("Returning %d\n", res);
    return res;
}


BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            return wine_vk_init();
    }
    return TRUE;
}

static const struct vulkan_func vk_global_dispatch_table[] =
{
    {"vkCreateInstance", &wine_vkCreateInstance},
    {"vkEnumerateInstanceExtensionProperties", &wine_vkEnumerateInstanceExtensionProperties},
    {"vkGetInstanceProcAddr", &wine_vkGetInstanceProcAddr},
};

static void *wine_vk_get_global_proc_addr(const char *name)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vk_global_dispatch_table); i++)
    {
        if (strcmp(name, vk_global_dispatch_table[i].name) == 0)
        {
            TRACE("Found name=%s in global table\n", debugstr_a(name));
            return vk_global_dispatch_table[i].func;
        }
    }
    return NULL;
}
