/*
 * Copyright 2018 Józef Kucia for CodeWeavers
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

#include "windows.h"
#include "wine/heap.h"
#include "wine/vulkan.h"
#include "wine/test.h"

static VkResult create_instance(uint32_t extension_count,
        const char * const *enabled_extensions, VkInstance *vk_instance)
{
    VkInstanceCreateInfo create_info;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.pApplicationInfo = NULL;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = enabled_extensions;

    return vkCreateInstance(&create_info, NULL, vk_instance);
}

#define create_instance_skip(a, b, c) create_instance_skip_(__LINE__, a, b, c)
static VkResult create_instance_skip_(unsigned int line, uint32_t extension_count,
        const char * const *enabled_extensions, VkInstance *vk_instance)
{
    VkResult vr;

    if ((vr = create_instance(extension_count, enabled_extensions, vk_instance)) >= 0)
        return vr;

    switch (vr)
    {
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            if (extension_count == 1)
                skip_(__FILE__, line)("Instance extension '%s' not supported.\n", enabled_extensions[0]);
            else
                skip_(__FILE__, line)("Instance extensions not supported.\n");
            break;

        default:
            skip_(__FILE__, line)("Failed to create Vulkan instance, vr %d.\n", vr);
            break;
    }

    return vr;
}

static VkBool32 find_queue_family(VkPhysicalDevice vk_physical_device,
        VkQueueFlags flags, uint32_t *family_index)
{
    VkQueueFamilyProperties *properties;
    VkBool32 ret = VK_FALSE;
    uint32_t i, count;

    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, NULL);
    properties = heap_calloc(count, sizeof(*properties));
    ok(!!properties, "Failed to allocate memory.\n");
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, properties);

    for (i = 0; i < count; ++i)
    {
        if ((properties[i].queueFlags & flags) == flags)
        {
            ret = VK_TRUE;
            *family_index = i;
            break;
        }
    }

    heap_free(properties);
    return ret;
}

static VkResult create_device(VkPhysicalDevice vk_physical_device,
        uint32_t extension_count, const char * const *enabled_extensions,
        const void *next, VkDevice *vk_device)
{
    VkDeviceQueueCreateInfo queue_info;
    VkDeviceCreateInfo create_info;
    float priority = 0.0f;

    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.flags = 0;
    if (!find_queue_family(vk_physical_device, VK_QUEUE_GRAPHICS_BIT, &queue_info.queueFamilyIndex))
    {
        trace("Failed to find queue family.\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = next;
    create_info.flags = 0;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_info;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = enabled_extensions;
    create_info.pEnabledFeatures = NULL;

    return vkCreateDevice(vk_physical_device, &create_info, NULL, vk_device);
}

static void test_instance_version(void)
{
    PFN_vkEnumerateInstanceVersion pfn_vkEnumerateInstanceVersion;
    uint32_t version;
    VkResult vr;

    pfn_vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(
            NULL, "vkEnumerateInstanceVersion");
    if (!pfn_vkEnumerateInstanceVersion)
    {
        skip("vkEnumerateInstanceVersion() is not available.\n");
        return;
    }

    vr = pfn_vkEnumerateInstanceVersion(&version);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);
    ok(version >= VK_API_VERSION_1_0, "Invalid version %#x.\n", version);
    trace("Vulkan version %u.%u.%u.\n",
            VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
}

static void enumerate_physical_device(VkPhysicalDevice vk_physical_device)
{
    VkPhysicalDeviceProperties properties;

    vkGetPhysicalDeviceProperties(vk_physical_device, &properties);

    trace("Device '%s', %#x:%#x, driver version %u.%u.%u (%#x), api version %u.%u.%u.\n",
            properties.deviceName, properties.vendorID, properties.deviceID,
            VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion),
            VK_VERSION_PATCH(properties.driverVersion), properties.driverVersion,
            VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
            VK_VERSION_PATCH(properties.apiVersion));
}

static void test_enumerate_physical_device2(void)
{
    static const char *procs[] = {"vkGetPhysicalDeviceProperties2", "vkGetPhysicalDeviceProperties2KHR"};
    static const char *extensions[] = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
    PFN_vkGetPhysicalDeviceProperties2 pfn_vkGetPhysicalDeviceProperties2;
    VkPhysicalDeviceProperties2 properties2;
    VkPhysicalDevice *vk_physical_devices;
    VkPhysicalDeviceIDProperties id;
    VkInstance vk_instance;
    unsigned int i, j;
    const LUID *luid;
    uint32_t count;
    VkResult vr;

    if ((vr = create_instance_skip(ARRAY_SIZE(extensions), extensions, &vk_instance)) < 0)
        return;
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    vr = vkEnumeratePhysicalDevices(vk_instance, &count, NULL);
    if (vr || !count)
    {
        skip("No physical devices. VkResult %d.\n", vr);
        vkDestroyInstance(vk_instance, NULL);
        return;
    }

    vk_physical_devices = heap_calloc(count, sizeof(*vk_physical_devices));
    ok(!!vk_physical_devices, "Failed to allocate memory.\n");
    vr = vkEnumeratePhysicalDevices(vk_instance, &count, vk_physical_devices);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    for (i = 0; i < ARRAY_SIZE(procs); ++i)
    {
        pfn_vkGetPhysicalDeviceProperties2
                = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(vk_instance, procs[i]);
        if (!pfn_vkGetPhysicalDeviceProperties2)
        {
            skip("%s is not available.\n", procs[i]);
            continue;
        }

        for (j = 0; j < count; ++j)
        {
            properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            properties2.pNext = &id;

            memset(&id, 0, sizeof(id));
            id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

            pfn_vkGetPhysicalDeviceProperties2(vk_physical_devices[j], &properties2);
            luid = (const LUID *)id.deviceLUID;
            trace("Device '%s', device UUID: %s, driver UUID: %s, device LUID: %08x:%08x.\n",
                  properties2.properties.deviceName, wine_dbgstr_guid((const GUID *)id.deviceUUID),
                  wine_dbgstr_guid((const GUID *)id.driverUUID), luid->HighPart, luid->LowPart);
            ok(id.deviceLUIDValid == VK_TRUE, "Expected valid device LUID.\n");
            /* If deviceLUIDValid is VK_TRUE, deviceNodeMask must contain exactly one bit according
             * to the Vulkan specification */
            ok(id.deviceNodeMask && !(id.deviceNodeMask & (id.deviceNodeMask - 1)),
               "Expect deviceNodeMask to have only one bit set, got %#x.\n", id.deviceNodeMask);
        }
    }

    heap_free(vk_physical_devices);
    vkDestroyInstance(vk_instance, NULL);
}

static void enumerate_device_queues(VkPhysicalDevice vk_physical_device)
{
    VkPhysicalDeviceProperties device_properties;
    VkQueueFamilyProperties *properties;
    uint32_t i, count;

    vkGetPhysicalDeviceProperties(vk_physical_device, &device_properties);

    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, NULL);
    properties = heap_calloc(count, sizeof(*properties));
    ok(!!properties, "Failed to allocate memory.\n");
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, properties);

    for (i = 0; i < count; ++i)
    {
        trace("Device '%s', queue family %u: flags %#x count %u.\n",
                device_properties.deviceName, i, properties[i].queueFlags, properties[i].queueCount);
    }

    heap_free(properties);
}

static void test_physical_device_groups(void)
{
    PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;
    VkPhysicalDeviceGroupProperties *properties;
    VkDeviceGroupDeviceCreateInfo group_info;
    VkInstance vk_instance;
    uint32_t i, j, count;
    VkDevice vk_device;
    VkResult vr;

    static const char *extensions[] =
    {
        VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
    };

    if ((vr = create_instance_skip(ARRAY_SIZE(extensions), extensions, &vk_instance)) < 0)
        return;
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    vkEnumeratePhysicalDeviceGroupsKHR
            = (void *)vkGetInstanceProcAddr(vk_instance, "vkEnumeratePhysicalDeviceGroupsKHR");
    ok(!!vkEnumeratePhysicalDeviceGroupsKHR, "Failed to get proc addr.\n");

    vr = vkEnumeratePhysicalDeviceGroupsKHR(vk_instance, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);
    ok(count > 0, "Unexpected device group count %u.\n", count);

    properties = heap_calloc(count, sizeof(*properties));
    ok(!!properties, "Failed to allocate memory.\n");
    vr = vkEnumeratePhysicalDeviceGroupsKHR(vk_instance, &count, properties);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    for (i = 0; i < count; ++i)
    {
        trace("Group[%u] count %u, subset allocation %#x\n",
                i, properties[i].physicalDeviceCount, properties[i].subsetAllocation);
        for (j = 0; j < properties[i].physicalDeviceCount; ++j)
            enumerate_physical_device(properties[i].physicalDevices[j]);
    }

    if ((vr = create_device(properties->physicalDevices[0], 0, NULL, NULL, &vk_device)) < 0)
    {
        skip("Failed to create device, vr %d.\n", vr);
        return;
    }
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);
    vkDestroyDevice(vk_device, NULL);

    group_info.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;
    group_info.pNext = NULL;
    group_info.physicalDeviceCount = properties->physicalDeviceCount;
    group_info.pPhysicalDevices = properties->physicalDevices;
    vr = create_device(group_info.pPhysicalDevices[0], 0, NULL, &group_info, &vk_device);
    ok(vr == VK_SUCCESS, "Failed to create device, VkResult %d.\n", vr);
    vkDestroyDevice(vk_device, NULL);

    heap_free(properties);

    vkDestroyInstance(vk_instance, NULL);
}

static void test_destroy_command_pool(VkPhysicalDevice vk_physical_device)
{
    VkCommandBufferAllocateInfo allocate_info;
    VkCommandPoolCreateInfo pool_info;
    VkCommandBuffer vk_cmd_buffers[4];
    uint32_t queue_family_index;
    VkCommandPool vk_cmd_pool;
    VkDevice vk_device;
    VkResult vr;

    if ((vr = create_device(vk_physical_device, 0, NULL, NULL, &vk_device)) < 0)
    {
        skip("Failed to create device, vr %d.\n", vr);
        return;
    }

    find_queue_family(vk_physical_device, VK_QUEUE_GRAPHICS_BIT, &queue_family_index);

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.pNext = NULL;
    pool_info.flags = 0;
    pool_info.queueFamilyIndex = queue_family_index;
    vr = vkCreateCommandPool(vk_device, &pool_info, NULL, &vk_cmd_pool);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.commandPool = vk_cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = ARRAY_SIZE(vk_cmd_buffers);
    vr = vkAllocateCommandBuffers(vk_device, &allocate_info, vk_cmd_buffers);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    vkDestroyCommandPool(vk_device, vk_cmd_pool, NULL);
    vkDestroyCommandPool(vk_device, VK_NULL_HANDLE, NULL);

    vkDestroyDevice(vk_device, NULL);
}

static void test_unsupported_instance_extensions(void)
{
    VkInstance vk_instance;
    unsigned int i;
    VkResult vr;

    static const char *extensions[] =
    {
        "VK_KHR_xcb_surface",
        "VK_KHR_xlib_surface",
    };

    for (i = 0; i < ARRAY_SIZE(extensions); ++i)
    {
        vr = create_instance(1, &extensions[i], &vk_instance);
        ok(vr == VK_ERROR_EXTENSION_NOT_PRESENT,
                "Got VkResult %d for extension %s.\n", vr, extensions[i]);
    }
}

static void test_unsupported_device_extensions(VkPhysicalDevice vk_physical_device)
{
    VkDevice vk_device;
    unsigned int i;
    VkResult vr;

    static const char *extensions[] =
    {
        "VK_KHR_external_fence_fd",
        "VK_KHR_external_memory_fd",
        "VK_KHR_external_semaphore_fd",
    };

    for (i = 0; i < ARRAY_SIZE(extensions); ++i)
    {
        vr = create_device(vk_physical_device, 1, &extensions[i], NULL, &vk_device);
        ok(vr == VK_ERROR_EXTENSION_NOT_PRESENT,
                "Got VkResult %d for extension %s.\n", vr, extensions[i]);
    }
}

static void test_private_data(VkPhysicalDevice vk_physical_device)
{
    PFN_vkDestroyPrivateDataSlotEXT pfn_vkDestroyPrivateDataSlotEXT;
    PFN_vkCreatePrivateDataSlotEXT pfn_vkCreatePrivateDataSlotEXT;
    VkPrivateDataSlotCreateInfoEXT data_create_info;
    PFN_vkGetPrivateDataEXT pfn_vkGetPrivateDataEXT;
    PFN_vkSetPrivateDataEXT pfn_vkSetPrivateDataEXT;
    VkPrivateDataSlotEXT data_slot;
    VkDevice vk_device;
    uint64_t data;
    VkResult vr;

    static const uint64_t data_value = 0x70AD;

    static const char *ext_name = "VK_EXT_private_data";

    if ((vr = create_device(vk_physical_device, 1, &ext_name, NULL, &vk_device)) < 0)
    {
        skip("Failed to create device with VK_EXT_private_data, VkResult %d.\n", vr);
        return;
    }

    pfn_vkDestroyPrivateDataSlotEXT =
            (void*) vkGetDeviceProcAddr(vk_device, "vkDestroyPrivateDataSlotEXT");
    pfn_vkCreatePrivateDataSlotEXT =
            (void*) vkGetDeviceProcAddr(vk_device, "vkCreatePrivateDataSlotEXT");
    pfn_vkGetPrivateDataEXT =
            (void*) vkGetDeviceProcAddr(vk_device, "vkGetPrivateDataEXT");
    pfn_vkSetPrivateDataEXT =
            (void*) vkGetDeviceProcAddr(vk_device, "vkSetPrivateDataEXT");

    data_create_info.sType = VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO_EXT;
    data_create_info.pNext = NULL;
    data_create_info.flags = 0;
    vr = pfn_vkCreatePrivateDataSlotEXT(vk_device, &data_create_info, NULL, &data_slot);
    ok(vr == VK_SUCCESS, "Failed to create private data slot, VkResult %d.\n", vr);

    vr = pfn_vkSetPrivateDataEXT(vk_device, VK_OBJECT_TYPE_DEVICE,
            (uint64_t) (uintptr_t) vk_device, data_slot, data_value);
    ok(vr == VK_SUCCESS, "Failed to set private data, VkResult %d.\n", vr);

    pfn_vkGetPrivateDataEXT(vk_device, VK_OBJECT_TYPE_DEVICE,
            (uint64_t) (uintptr_t) vk_device, data_slot, &data);
    ok(data == data_value, "Got unexpected private data, %s.\n",
            wine_dbgstr_longlong(data));

    pfn_vkDestroyPrivateDataSlotEXT(vk_device, data_slot, NULL);
    vkDestroyDevice(vk_device, NULL);
}

static const char *test_null_hwnd_extensions[] =
{
    "VK_KHR_surface",
    "VK_KHR_win32_surface",
    "VK_KHR_device_group_creation",
};

static void test_null_hwnd(VkInstance vk_instance, VkPhysicalDevice vk_physical_device)
{
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pvkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDevicePresentRectanglesKHR pvkGetPhysicalDevicePresentRectanglesKHR;
    VkDeviceGroupPresentModeFlagsKHR present_mode_flags;
    VkWin32SurfaceCreateInfoKHR surface_create_info;
    VkSurfaceCapabilitiesKHR surf_caps;
    VkSurfaceFormatKHR *formats;
    uint32_t queue_family_index;
    VkPresentModeKHR *modes;
    VkSurfaceKHR surface;
    VkDevice vk_device;
    uint32_t count;
    VkRect2D rect;
    VkBool32 bval;
    VkResult vr;

    pvkGetPhysicalDeviceSurfacePresentModesKHR = (void *)vkGetInstanceProcAddr(vk_instance,
            "vkGetPhysicalDeviceSurfacePresentModesKHR");
    pvkGetPhysicalDevicePresentRectanglesKHR = (void *)vkGetInstanceProcAddr(vk_instance,
            "vkGetPhysicalDevicePresentRectanglesKHR");

    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = NULL;
    surface_create_info.flags = 0;
    surface_create_info.hinstance = NULL;
    surface_create_info.hwnd = NULL;

    bval = find_queue_family(vk_physical_device, VK_QUEUE_GRAPHICS_BIT, &queue_family_index);
    ok(bval, "Could not find presentation queue.\n");

    surface = 0xdeadbeef;
    vr = vkCreateWin32SurfaceKHR(vk_instance, &surface_create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    count = 0;
    vr = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, surface, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(count, "Got zero count.\n");
    formats = heap_alloc(sizeof(*formats) * count);
    vr = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, surface, &count, formats);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    heap_free(formats);

    vr = vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, queue_family_index, surface, &bval);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, surface, &surf_caps);
    ok(vr, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR succeeded.\n");

    count = 0;
    vr = pvkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, surface, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(count, "Got zero count.\n");
    modes = heap_alloc(sizeof(*modes) * count);
    vr = pvkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, surface, &count, modes);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    heap_free(modes);

    if (pvkGetPhysicalDevicePresentRectanglesKHR)
    {
        count = 0;
        vr = pvkGetPhysicalDevicePresentRectanglesKHR(vk_physical_device, surface, &count, NULL);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
        ok(count == 1, "Got unexpected count %u.\n", count);
        memset(&rect, 0xcc, sizeof(rect));
        vr = pvkGetPhysicalDevicePresentRectanglesKHR(vk_physical_device, surface, &count, &rect);
        if (vr == VK_SUCCESS) /* Fails on AMD, succeeds on Nvidia. */
        {
            ok(count == 1, "Got unexpected count %u.\n", count);
            ok(!rect.offset.x && !rect.offset.y && !rect.extent.width && !rect.extent.height,
                    "Got unexpected rect %d, %d, %u, %u.\n",
                    rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
        }

        if ((vr = create_device(vk_physical_device, 0, NULL, NULL, &vk_device)) < 0)
        {
            skip("Failed to create device, vr %d.\n", vr);
            vkDestroySurfaceKHR(vk_instance, surface, NULL);
            return;
        }
    }
    else
    {
        /* The function should be available in practice with VK_KHR_device_group_creation, but spec lists
         * it as a part of VK_KHR_device_group device extension which we don't check, so consider the
         * absence of the function. */
        win_skip("pvkGetPhysicalDevicePresentRectanglesKHR is no available.\n");
    }

    if (0)
    {
        /* Causes access violation on Windows. */
        vr = vkGetDeviceGroupSurfacePresentModesKHR(vk_device, surface, &present_mode_flags);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    }

    vkDestroyDevice(vk_device, NULL);
    vkDestroySurfaceKHR(vk_instance, surface, NULL);
}

static void for_each_device_instance(uint32_t extension_count, const char * const *enabled_extensions,
        void (*test_func_instance)(VkInstance, VkPhysicalDevice), void (*test_func)(VkPhysicalDevice))
{
    VkPhysicalDevice *vk_physical_devices;
    VkInstance vk_instance;
    unsigned int i;
    uint32_t count;
    VkResult vr;

    if ((vr = create_instance_skip(extension_count, enabled_extensions, &vk_instance)) < 0)
        return;
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    vr = vkEnumeratePhysicalDevices(vk_instance, &count, NULL);
    if (vr || !count)
    {
        skip("No physical devices. VkResult %d.\n", vr);
        vkDestroyInstance(vk_instance, NULL);
        return;
    }

    vk_physical_devices = heap_calloc(count, sizeof(*vk_physical_devices));
    ok(!!vk_physical_devices, "Failed to allocate memory.\n");
    vr = vkEnumeratePhysicalDevices(vk_instance, &count, vk_physical_devices);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    for (i = 0; i < count; ++i)
    {
        if (test_func_instance)
            test_func_instance(vk_instance, vk_physical_devices[i]);
        else
            test_func(vk_physical_devices[i]);
    }

    heap_free(vk_physical_devices);

    vkDestroyInstance(vk_instance, NULL);
}

static void for_each_device(void (*test_func)(VkPhysicalDevice))
{
    for_each_device_instance(0, NULL, NULL, test_func);
}

START_TEST(vulkan)
{
    test_instance_version();
    for_each_device(enumerate_physical_device);
    test_enumerate_physical_device2();
    for_each_device(enumerate_device_queues);
    test_physical_device_groups();
    for_each_device(test_destroy_command_pool);
    test_unsupported_instance_extensions();
    for_each_device(test_unsupported_device_extensions);
    for_each_device(test_private_data);
    for_each_device_instance(ARRAY_SIZE(test_null_hwnd_extensions), test_null_hwnd_extensions, test_null_hwnd, NULL);
}
