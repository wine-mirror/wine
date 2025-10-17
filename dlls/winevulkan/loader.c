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

#include <stdlib.h>
#include "vulkan_loader.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

/* For now default to 4 as it felt like a reasonable version feature wise to support.
 * Version 5 adds more extensive version checks. Something to tackle later.
 */
#define WINE_VULKAN_ICD_VERSION 4

static HINSTANCE hinstance;

VkResult WINAPI vkEnumerateInstanceLayerProperties(uint32_t *count, VkLayerProperties *properties)
{
    TRACE("%p, %p\n", count, properties);

    *count = 0;
    return VK_SUCCESS;
}

static const struct vulkan_func vk_global_dispatch_table[] =
{
    /* These functions must call wine_vk_init_once() before accessing vk_funcs. */
    {"vkCreateInstance", &vkCreateInstance},
    {"vkEnumerateInstanceExtensionProperties", &vkEnumerateInstanceExtensionProperties},
    {"vkEnumerateInstanceLayerProperties", &vkEnumerateInstanceLayerProperties},
    {"vkEnumerateInstanceVersion", &vkEnumerateInstanceVersion},
    {"vkGetInstanceProcAddr", &vkGetInstanceProcAddr},
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

static BOOL is_available_instance_function(VkInstance instance, const char *name)
{
    struct is_available_instance_function_params params = { .instance = instance, .name = name };
    return UNIX_CALL(is_available_instance_function, &params);
}

static BOOL is_available_device_function(VkDevice device, const char *name)
{
    struct is_available_device_function_params params = { .device = device, .name = name };
    return UNIX_CALL(is_available_device_function, &params);
}

static void *vulkan_client_object_create(size_t size)
{
    struct vulkan_client_object *object = calloc(1, size);
    object->loader_magic = VULKAN_ICD_MAGIC_VALUE;
    return object;
}

PFN_vkVoidFunction WINAPI vkGetInstanceProcAddr(VkInstance instance, const char *name)
{
    void *func;

    TRACE("%p, %s\n", instance, debugstr_a(name));

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
        WARN("Global function %s not found.\n", debugstr_a(name));
        return NULL;
    }

    if (!is_available_instance_function(instance, name))
        return NULL;

    func = wine_vk_get_instance_proc_addr(name);
    if (func) return func;

    func = wine_vk_get_phys_dev_proc_addr(name);
    if (func) return func;

    /* vkGetInstanceProcAddr also loads any children of instance, so device functions as well. */
    func = wine_vk_get_device_proc_addr(name);
    if (func) return func;

    WARN("Unsupported device or instance function: %s.\n", debugstr_a(name));
    return NULL;
}

PFN_vkVoidFunction WINAPI vkGetDeviceProcAddr(VkDevice device, const char *name)
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
    if (is_available_device_function(device, name))
    {
        func = wine_vk_get_device_proc_addr(name);
        if (func)
            return func;
    }

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
    if ((device->quirks & WINEVULKAN_QUIRK_GET_DEVICE_PROC_ADDR)
        && ((func = wine_vk_get_instance_proc_addr(name))
             || (func = wine_vk_get_phys_dev_proc_addr(name))))
    {
        WARN("Returning instance function %s.\n", debugstr_a(name));
        return func;
    }

    WARN("Unsupported device function: %s.\n", debugstr_a(name));
    return NULL;
}

void * WINAPI vk_icdGetPhysicalDeviceProcAddr(VkInstance instance, const char *name)
{
    TRACE("%p, %s\n", instance, debugstr_a(name));

    if (!is_available_instance_function(instance, name))
        return NULL;

    return wine_vk_get_phys_dev_proc_addr(name);
}

void * WINAPI vk_icdGetInstanceProcAddr(VkInstance instance, const char *name)
{
    TRACE("%p, %s\n", instance, debugstr_a(name));

    /* Initial version of the Vulkan ICD spec required vkGetInstanceProcAddr to be
     * exported. vk_icdGetInstanceProcAddr was added later to separate ICD calls from
     * Vulkan API. One of them in our case should forward to the other, so just forward
     * to the older vkGetInstanceProcAddr.
     */
    return vkGetInstanceProcAddr(instance, name);
}

VkResult WINAPI vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *supported_version)
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

static NTSTATUS WINAPI call_vulkan_debug_report_callback(void *args, ULONG size)
{
    struct wine_vk_debug_report_params *params = args;
    PFN_vkDebugReportCallbackEXT callback = (void *)(UINT_PTR)params->user_callback;
    const char *strings = (char *)(params + 1), *layer = NULL, *message = NULL;
    void *user_data = (void *)(UINT_PTR)params->user_data;
    VkBool32 ret;

    if (params->layer_len) layer = strings;
    strings += params->layer_len;
    if (params->message_len) message = strings;

    ret = callback(params->flags, params->object_type, params->object_handle, params->location,
                   params->code, layer, message, user_data);
    return NtCallbackReturn(&ret, sizeof(ret), STATUS_SUCCESS);
}

static NTSTATUS WINAPI call_vulkan_debug_utils_callback(void *args, ULONG size)
{
    struct wine_vk_debug_utils_params *params = args;
    PFN_vkDebugUtilsMessengerCallbackEXT callback = (void *)(UINT_PTR)params->user_callback;
    void *user_data = (void *)(UINT_PTR)params->user_data;
    VkDeviceAddressBindingCallbackDataEXT address_binding =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_ADDRESS_BINDING_CALLBACK_DATA_EXT,
        .flags = params->address_binding.flags,
        .baseAddress = params->address_binding.base_address,
        .size = params->address_binding.size,
        .bindingType = params->address_binding.binding_type,
    };
    VkDebugUtilsMessengerCallbackDataEXT data =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT,
        .flags = params->flags,
        .messageIdNumber = params->message_id_number,
        .queueLabelCount = params->queue_label_count,
        .cmdBufLabelCount = params->cmd_buf_label_count,
        .objectCount = params->object_count,
    };
    VkDebugUtilsObjectNameInfoEXT *objects;
    VkDebugUtilsLabelEXT *labels;
    const char *ptr, *strings;
    VkBool32 ret = VK_FALSE;
    UINT i;

    size = sizeof(*params);
    size += sizeof(struct debug_utils_label) * (data.queueLabelCount + data.cmdBufLabelCount);
    size += sizeof(struct debug_utils_object) * data.objectCount;

    ptr = (char *)(params + 1);
    strings = (char *)params + size;

    if (params->has_address_binding) data.pNext = &address_binding;
    if (params->message_id_name_len) data.pMessageIdName = strings;
    strings += params->message_id_name_len;
    if (params->message_len) data.pMessage = strings;
    strings += params->message_len;

    if ((labels = calloc(data.queueLabelCount + data.cmdBufLabelCount + 1, sizeof(*data.pQueueLabels))) &&
        (objects = calloc(data.objectCount + 1, sizeof(*data.pObjects))))
    {
        for (i = 0; i < data.queueLabelCount + data.cmdBufLabelCount; i++)
        {
            struct debug_utils_label *label = (struct debug_utils_label *)ptr;
            labels[i].sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            memcpy(labels[i].color, label->color, sizeof(label->color));
            if (label->label_name_len) labels[i].pLabelName = strings;
            strings += label->label_name_len;
            ptr += sizeof(*label);
        }
        if (data.queueLabelCount) data.pQueueLabels = labels;
        labels += data.queueLabelCount;
        if (data.cmdBufLabelCount) data.pCmdBufLabels = labels;

        for (i = 0; i < data.objectCount; i++)
        {
            struct debug_utils_object *object = (struct debug_utils_object *)ptr;
            objects[i].sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objects[i].objectType = object->object_type;
            objects[i].objectHandle = object->object_handle;
            if (object->object_name_len) objects[i].pObjectName = strings;
            strings += object->object_name_len;
            ptr += sizeof(*object);
        }
        data.pObjects = objects;

        ret = callback(params->severity, params->message_types, &data, user_data);

        free(objects);
    }
    free(labels);

    return NtCallbackReturn(&ret, sizeof(ret), STATUS_SUCCESS);
}

static BOOL WINAPI wine_vk_init(INIT_ONCE *once, void *param, void **context)
{
    struct vk_callback_funcs callback_funcs =
    {
        .call_vulkan_debug_report_callback = (ULONG_PTR)call_vulkan_debug_report_callback,
        .call_vulkan_debug_utils_callback = (ULONG_PTR)call_vulkan_debug_utils_callback,
    };

    return !__wine_init_unix_call() && !UNIX_CALL(init, &callback_funcs);
}

static BOOL  wine_vk_init_once(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

    return InitOnceExecuteOnce(&init_once, wine_vk_init, NULL, NULL);
}

VkResult WINAPI vkCreateInstance(const VkInstanceCreateInfo *create_info,
        const VkAllocationCallbacks *allocator, VkInstance *ret)
{
    struct vkCreateInstance_params params;
    struct VkInstance_T *instance;
    uint32_t phys_dev_count = 8, i;
    NTSTATUS status;

    TRACE("create_info %p, allocator %p, instance %p\n", create_info, allocator, ret);

    if (!wine_vk_init_once())
        return VK_ERROR_INITIALIZATION_FAILED;

    for (;;)
    {
        if (!(instance = vulkan_client_object_create(FIELD_OFFSET(struct VkInstance_T, phys_devs[phys_dev_count]))))
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        instance->phys_dev_count = phys_dev_count;
        for (i = 0; i < phys_dev_count; i++)
            instance->phys_devs[i].obj.loader_magic = VULKAN_ICD_MAGIC_VALUE;

        params.pCreateInfo = create_info;
        params.pAllocator = allocator;
        params.pInstance = ret;
        params.client_ptr = instance;
        status = UNIX_CALL(vkCreateInstance, &params);
        assert(!status);
        if (instance->phys_dev_count <= phys_dev_count)
            break;
        phys_dev_count = instance->phys_dev_count;
        free(instance);
    }

    if (params.result)
        free(instance);
    return params.result;
}

void WINAPI vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    struct vkDestroyInstance_params params;
    NTSTATUS status;

    params.instance = instance;
    params.pAllocator = pAllocator;
    status = UNIX_CALL(vkDestroyInstance, &params);
    assert(!status);
    free(instance);
}

VkResult WINAPI vkEnumerateInstanceExtensionProperties(const char *layer_name,
        uint32_t *count, VkExtensionProperties *properties)
{
    struct vkEnumerateInstanceExtensionProperties_params params;
    NTSTATUS status;

    TRACE("%p, %p, %p\n", layer_name, count, properties);

    if (layer_name)
    {
        WARN("Layer enumeration not supported from ICD.\n");
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    if (!wine_vk_init_once())
    {
        *count = 0;
        return VK_SUCCESS;
    }

    params.pLayerName = layer_name;
    params.pPropertyCount = count;
    params.pProperties = properties;
    status = UNIX_CALL(vkEnumerateInstanceExtensionProperties, &params);
    assert(!status);
    return params.result;
}

VkResult WINAPI vkEnumerateInstanceVersion(uint32_t *version)
{
    struct vkEnumerateInstanceVersion_params params;
    NTSTATUS status;

    TRACE("%p\n", version);

    if (!wine_vk_init_once())
    {
        *version = VK_API_VERSION_1_0;
        return VK_SUCCESS;
    }

    params.pApiVersion = version;
    status = UNIX_CALL(vkEnumerateInstanceVersion, &params);
    assert(!status);
    return params.result;
}

VkResult WINAPI vkCreateDevice(VkPhysicalDevice phys_dev, const VkDeviceCreateInfo *create_info,
                               const VkAllocationCallbacks *allocator, VkDevice *ret)
{
    struct vkCreateDevice_params params;
    uint32_t queue_count = 0, i;
    VkDevice device;
    NTSTATUS status;

    for (i = 0; i < create_info->queueCreateInfoCount; i++)
        queue_count += create_info->pQueueCreateInfos[i].queueCount;
    if (!(device = vulkan_client_object_create(FIELD_OFFSET(struct VkDevice_T, queues[queue_count]))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    for (i = 0; i < queue_count; i++)
        device->queues[i].obj.loader_magic = VULKAN_ICD_MAGIC_VALUE;

    params.physicalDevice = phys_dev;
    params.pCreateInfo = create_info;
    params.pAllocator = allocator;
    params.pDevice = ret;
    params.client_ptr = device;
    status = UNIX_CALL(vkCreateDevice, &params);
    assert(!status);
    if (params.result)
        free(device);
    return params.result;
}

void WINAPI vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *allocator)
{
    struct vkDestroyDevice_params params;
    NTSTATUS status;

    params.device = device;
    params.pAllocator = allocator;
    status = UNIX_CALL(vkDestroyDevice, &params);
    assert(!status);
    free(device);
}

VkResult WINAPI vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *create_info,
                                    const VkAllocationCallbacks *allocator, VkCommandPool *ret)
{
    struct vkCreateCommandPool_params params;
    struct vk_command_pool *cmd_pool;
    NTSTATUS status;

    if (!(cmd_pool = vulkan_client_object_create(sizeof(*cmd_pool))))
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    list_init(&cmd_pool->command_buffers);

    params.device = device;
    params.pCreateInfo = create_info;
    params.pAllocator = allocator;
    params.pCommandPool = ret;
    params.client_ptr = cmd_pool;
    status = UNIX_CALL(vkCreateCommandPool, &params);
    assert(!status);
    if (params.result)
        free(cmd_pool);
    return params.result;
}

void WINAPI vkDestroyCommandPool(VkDevice device, VkCommandPool handle, const VkAllocationCallbacks *allocator)
{
    struct vk_command_pool *cmd_pool = command_pool_from_handle(handle);
    struct vkDestroyCommandPool_params params;
    VkCommandBuffer buffer, cursor;
    NTSTATUS status;

    if (!cmd_pool)
        return;

    /* The Vulkan spec says:
     *
     * "When a pool is destroyed, all command buffers allocated from the pool are freed."
     */
    LIST_FOR_EACH_ENTRY_SAFE(buffer, cursor, &cmd_pool->command_buffers, struct VkCommandBuffer_T, pool_link)
    {
        vkFreeCommandBuffers(device, handle, 1, &buffer);
    }

    params.device = device;
    params.commandPool = handle;
    params.pAllocator = allocator;
    status = UNIX_CALL(vkDestroyCommandPool, &params);
    assert(!status);
    free(cmd_pool);
}

VkResult WINAPI vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *allocate_info,
                                         VkCommandBuffer *buffers)
{
    struct vk_command_pool *pool = command_pool_from_handle(allocate_info->commandPool);
    struct vkAllocateCommandBuffers_params params;
    NTSTATUS status;
    uint32_t i;

    for (i = 0; i < allocate_info->commandBufferCount; i++)
        buffers[i] = vulkan_client_object_create(sizeof(*buffers[i]));

    params.device = device;
    params.pAllocateInfo = allocate_info;
    params.pCommandBuffers = buffers;
    status = UNIX_CALL(vkAllocateCommandBuffers, &params);
    assert(!status);
    if (params.result == VK_SUCCESS)
    {
        for (i = 0; i < allocate_info->commandBufferCount; i++)
            list_add_tail(&pool->command_buffers, &buffers[i]->pool_link);
    }
    else
    {
        for (i = 0; i < allocate_info->commandBufferCount; i++)
        {
            free(buffers[i]);
            buffers[i] = NULL;
        }
    }
    return params.result;
}

void WINAPI vkFreeCommandBuffers(VkDevice device, VkCommandPool cmd_pool, uint32_t count,
                                 const VkCommandBuffer *buffers)
{
    struct vkFreeCommandBuffers_params params;
    NTSTATUS status;
    uint32_t i;

    params.device = device;
    params.commandPool = cmd_pool;
    params.commandBufferCount = count;
    params.pCommandBuffers = buffers;
    status = UNIX_CALL(vkFreeCommandBuffers, &params);
    assert(!status);
    for (i = 0; i < count; i++)
    {
        if (!buffers[i])
            continue;
        list_remove(&buffers[i]->pool_link);
        free(buffers[i]);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("%p, %lu, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            hinstance = hinst;
            DisableThreadLibraryCalls(hinst);
            break;
    }
    return TRUE;
}

static const WCHAR winevulkan_json_pathW[] = L"\\winevulkan.json";
static const WCHAR vulkan_driversW[] = L"Software\\Khronos\\Vulkan\\Drivers";

HRESULT WINAPI DllRegisterServer(void)
{
    WCHAR json_path[MAX_PATH];
    HRSRC rsrc;
    const char *data;
    DWORD datalen, written, zero = 0;
    HANDLE file;
    HKEY key;

    /* Create the JSON manifest and registry key to register this ICD with the official Vulkan loader. */
    TRACE("\n");
    rsrc = FindResourceW(hinstance, L"winevulkan_json", (const WCHAR *)RT_RCDATA);
    data = LockResource(LoadResource(hinstance, rsrc));
    datalen = SizeofResource(hinstance, rsrc);

    GetSystemDirectoryW(json_path, ARRAY_SIZE(json_path));
    lstrcatW(json_path, winevulkan_json_pathW);
    file = CreateFileW(json_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create JSON manifest.\n");
        return E_UNEXPECTED;
    }
    WriteFile(file, data, datalen, &written, NULL);
    CloseHandle(file);

    if (!RegCreateKeyExW(HKEY_LOCAL_MACHINE, vulkan_driversW, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL))
    {
        RegSetValueExW(key, json_path, 0, REG_DWORD, (const BYTE *)&zero, sizeof(zero));
        RegCloseKey(key);
    }
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    WCHAR json_path[MAX_PATH];
    HKEY key;

    /* Remove the JSON manifest and registry key */
    TRACE("\n");
    GetSystemDirectoryW(json_path, ARRAY_SIZE(json_path));
    lstrcatW(json_path, winevulkan_json_pathW);
    DeleteFileW(json_path);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, vulkan_driversW, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
    {
        RegDeleteValueW(key, json_path);
        RegCloseKey(key);
    }

    return S_OK;
}
