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

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"
#include "vulkan_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

/* For now default to 4 as it felt like a reasonable version feature wise to support.
 * Don't support the optional vk_icdGetPhysicalDeviceProcAddr introduced in this version
 * as it is unlikely we will implement physical device extensions, which the loader is not
 * aware off. Version 5 adds more extensive version checks. Something to tackle later.
 */
#define WINE_VULKAN_ICD_VERSION 4

static void *wine_vk_get_global_proc_addr(const char *name);

static const struct vulkan_funcs *vk_funcs = NULL;

static BOOL wine_vk_init(void)
{
    HDC hdc = GetDC(0);

    vk_funcs =  __wine_get_vulkan_driver(hdc, WINE_VULKAN_DRIVER_VERSION);
    if (!vk_funcs)
    {
        ERR("Failed to load Wine graphics driver supporting Vulkan.\n");
        ReleaseDC(0, hdc);
        return FALSE;
    }

    ReleaseDC(0, hdc);
    return TRUE;
}

/* Helper function which stores wrapped physical devices in the instance object. */
static VkResult wine_vk_instance_load_physical_devices(struct VkInstance_T *instance)
{
    VkResult res;
    struct VkPhysicalDevice_T **tmp_phys_devs = NULL;
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
        goto err;

    instance->phys_devs = heap_calloc(num_phys_devs, sizeof(*instance->phys_devs));
    if (!instance->phys_devs)
    {
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }

    /* Wrap each native physical device handle into a dispatchable object for the ICD loader. */
    for (i = 0; i < num_phys_devs; i++)
    {
        struct VkPhysicalDevice_T *phys_dev = heap_alloc(sizeof(*phys_dev));
        if (!phys_dev)
        {
            ERR("Unable to allocate memory for physical device!\n");
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            goto err;
        }

        phys_dev->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;
        phys_dev->instance = instance;
        phys_dev->phys_dev = tmp_phys_devs[i];

        instance->phys_devs[i] = phys_dev;
        instance->num_phys_devs = i + 1;
    }
    instance->num_phys_devs = num_phys_devs;

    heap_free(tmp_phys_devs);
    return VK_SUCCESS;

err:
    heap_free(tmp_phys_devs);

    return res;
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
            heap_free(&instance->phys_devs[i]);
        }
        heap_free(instance->phys_devs);
    }

    if (instance->instance)
        vk_funcs->p_vkDestroyInstance(instance->instance, NULL /* allocator */);

    heap_free(instance);
}

static VkResult WINAPI wine_vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    struct VkInstance_T *object = NULL;
    VkResult res;

    TRACE("create_info %p, allocator %p, instance %p\n", create_info, allocator, instance);

    if (allocator)
        FIXME("Support for allocation callbacks not implemented yet\n");

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate memory for instance\n");
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto err;
    }
    object->base.loader_magic = VULKAN_ICD_MAGIC_VALUE;

    res = vk_funcs->p_vkCreateInstance(create_info, NULL /* allocator */, &object->instance);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to create instance, res=%d\n", res);
        goto err;
    }

    /* Load all instance functions we are aware of. Note the loader takes care
     * of any filtering for extensions which were not requested, but which the
     * ICD may support.
     */
#define USE_VK_FUNC(name) \
    object->funcs.p_##name = (void*)vk_funcs->p_vkGetInstanceProcAddr(object->instance, #name);
    ALL_VK_INSTANCE_FUNCS()
#undef USE_VK_FUNC

    /* Cache physical devices for vkEnumeratePhysicalDevices within the instance as
     * each vkPhysicalDevice is a dispatchable object, which means we need to wrap
     * the native physical device and present those the application.
     * Cleanup happens as part of wine_vkDestroyInstance.
     */
    res = wine_vk_instance_load_physical_devices(object);
    if (res != VK_SUCCESS)
    {
        ERR("Failed to cache physical devices, res=%d\n", res);
        goto err;
    }

    *instance = object;
    TRACE("Done, instance=%p native_instance=%p\n", object, object->instance);
    return VK_SUCCESS;

err:
    wine_vk_instance_free(object);
    return res;
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
        *count = 0; /* No extensions yet. */
        return VK_SUCCESS;
    }

    /* When properties is not NULL, we copy the extensions over and set count to
     * the number of copied extensions. For now we don't have much to do as we don't support
     * any extensions yet.
     */
    *count = 0;
    return VK_SUCCESS;
}

static VkResult WINAPI wine_vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties *properties)
{
    TRACE("%p %p %p\n", layer_name, count, properties);
    return vk_funcs->p_vkEnumerateInstanceExtensionProperties(layer_name, count, properties);
}

VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *device_count,
        VkPhysicalDevice *devices)
{
    VkResult res;
    unsigned int i, num_copies;

    TRACE("%p %p %p\n", instance, device_count, devices);

    if (!devices)
    {
        *device_count = instance->num_phys_devs;
        return VK_SUCCESS;
    }

    if (*device_count < instance->num_phys_devs)
    {
        /* Incomplete is a type of success used to signal the application
         * that not all devices got copied.
         */
        num_copies = *device_count;
        res = VK_INCOMPLETE;
    }
    else
    {
        num_copies = instance->num_phys_devs;
        res = VK_SUCCESS;
    }

    for (i = 0; i < num_copies; i++)
    {
        devices[i] = instance->phys_devs[i];
    }
    *device_count = num_copies;

    TRACE("Returning %u devices\n", *device_count);
    return res;
}

static PFN_vkVoidFunction WINAPI wine_vkGetInstanceProcAddr(VkInstance instance, const char *name)
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
        FIXME("Global function '%s' not found\n", debugstr_a(name));
        return NULL;
    }

    func = wine_vk_get_instance_proc_addr(name);
    if (func) return func;

    FIXME("Unsupported device or instance function: '%s'\n", debugstr_a(name));
    return NULL;
}

void * WINAPI wine_vk_icdGetInstanceProcAddr(VkInstance instance, const char *name)
{
    TRACE("%p %s\n", instance, debugstr_a(name));

    /* Initial version of the Vulkan ICD spec required vkGetInstanceProcAddr to be
     * exported. vk_icdGetInstanceProcAddr was added later to separete ICD calls from
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
