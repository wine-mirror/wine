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
#include "wine/vulkan.h"
#include "wine/test.h"

static VkResult create_instance(uint32_t extension_count,
        const char * const *enabled_extensions, VkInstance *vk_instance)
{
    VkApplicationInfo app_info = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
    VkInstanceCreateInfo create_info;

    app_info.apiVersion = VK_API_VERSION_1_1;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.pApplicationInfo = &app_info;
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
    properties = calloc(count, sizeof(*properties));
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

    free(properties);
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

static VkResult create_swapchain(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
        VkDevice device, HWND hwnd, VkSwapchainKHR *swapchain)
{
    VkSwapchainCreateInfoKHR create_info = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    VkSurfaceCapabilitiesKHR capabilities;
    RECT client_rect;
    VkResult vr;

    if (!GetClientRect(hwnd, &client_rect))
        SetRect(&client_rect, 0, 0, 0, 0);

    vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    if (!IsWindow(hwnd))
    {
        ok(vr == VK_ERROR_SURFACE_LOST_KHR || vr == VK_ERROR_UNKNOWN, "Got unexpected vr %d.\n", vr);
        memset(&capabilities, 0, sizeof(capabilities));
    }

    create_info.surface = surface;
    create_info.minImageCount = max(1, capabilities.minImageCount);
    create_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent.width = max(client_rect.right - client_rect.left, capabilities.minImageExtent.width);
    create_info.imageExtent.height = max(client_rect.bottom - client_rect.top, capabilities.minImageExtent.height);
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    create_info.clipped = VK_TRUE;

    return vkCreateSwapchainKHR(device, &create_info, NULL, swapchain);
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

    vk_physical_devices = calloc(count, sizeof(*vk_physical_devices));
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
            trace("Device '%s', device UUID: %s, driver UUID: %s, device LUID: %08lx:%08lx.\n",
                  properties2.properties.deviceName, wine_dbgstr_guid((const GUID *)id.deviceUUID),
                  wine_dbgstr_guid((const GUID *)id.driverUUID), luid->HighPart, luid->LowPart);
            todo_wine_if(!id.deviceLUIDValid && strstr(properties2.properties.deviceName, "llvmpipe"))
            ok(id.deviceLUIDValid == VK_TRUE, "Expected valid device LUID.\n");
            if (id.deviceLUIDValid == VK_TRUE)
            {
                /* If deviceLUIDValid is VK_TRUE, deviceNodeMask must contain exactly one bit
                 * according to the Vulkan specification */
                ok(id.deviceNodeMask && !(id.deviceNodeMask & (id.deviceNodeMask - 1)),
                        "Expect deviceNodeMask to have only one bit set, got %#x.\n",
                        id.deviceNodeMask);
            }
        }
    }

    free(vk_physical_devices);
    vkDestroyInstance(vk_instance, NULL);
}

static void enumerate_device_queues(VkPhysicalDevice vk_physical_device)
{
    VkPhysicalDeviceProperties device_properties;
    VkQueueFamilyProperties *properties;
    uint32_t i, count;

    vkGetPhysicalDeviceProperties(vk_physical_device, &device_properties);

    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, NULL);
    properties = calloc(count, sizeof(*properties));
    ok(!!properties, "Failed to allocate memory.\n");
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &count, properties);

    for (i = 0; i < count; ++i)
    {
        trace("Device '%s', queue family %u: flags %#x count %u.\n",
                device_properties.deviceName, i, properties[i].queueFlags, properties[i].queueCount);
    }

    free(properties);
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

    properties = calloc(count, sizeof(*properties));
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

    free(properties);

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
        ok(vr == VK_ERROR_EXTENSION_NOT_PRESENT || vr == VK_ERROR_INCOMPATIBLE_DRIVER,
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
    VkPhysicalDevicePrivateDataFeaturesEXT data_features;
    VkPrivateDataSlotCreateInfoEXT data_create_info;
    PFN_vkGetPrivateDataEXT pfn_vkGetPrivateDataEXT;
    PFN_vkSetPrivateDataEXT pfn_vkSetPrivateDataEXT;
    VkPrivateDataSlotEXT data_slot;
    VkDevice vk_device;
    uint64_t data;
    VkResult vr;

    static const uint64_t data_value = 0x70AD;

    static const char *ext_name = "VK_EXT_private_data";

    data_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES_EXT;
    data_features.pNext = NULL;
    data_features.privateData = VK_TRUE;

    if ((vr = create_device(vk_physical_device, 1, &ext_name, &data_features, &vk_device)) < 0)
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

static const char *test_win32_surface_extensions[] =
{
    "VK_KHR_surface",
    "VK_KHR_win32_surface",
    "VK_KHR_device_group_creation",
    "VK_KHR_get_surface_capabilities2",
};

static void test_win32_surface_hwnd(VkInstance vk_instance, VkPhysicalDevice vk_physical_device,
        VkDevice device, VkSurfaceKHR surface, HWND hwnd)
{
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR pvkGetPhysicalDeviceSurfaceCapabilities2KHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pvkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDevicePresentRectanglesKHR pvkGetPhysicalDevicePresentRectanglesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR pvkGetPhysicalDeviceSurfaceFormats2KHR;
    VkDeviceGroupPresentModeFlagsKHR present_mode_flags;
    VkSurfaceCapabilitiesKHR surf_caps;
    VkSurfaceFormatKHR *formats;
    uint32_t queue_family_index;
    VkPresentModeKHR *modes;
    RECT client_rect;
    uint32_t count;
    VkRect2D rect;
    VkBool32 bval;
    VkResult vr;

    if (!GetClientRect(hwnd, &client_rect))
        SetRect(&client_rect, 0, 0, 0, 0);

    pvkGetPhysicalDeviceSurfaceCapabilities2KHR = (void *)vkGetInstanceProcAddr(vk_instance,
            "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
    pvkGetPhysicalDeviceSurfacePresentModesKHR = (void *)vkGetInstanceProcAddr(vk_instance,
            "vkGetPhysicalDeviceSurfacePresentModesKHR");
    pvkGetPhysicalDevicePresentRectanglesKHR = (void *)vkGetInstanceProcAddr(vk_instance,
            "vkGetPhysicalDevicePresentRectanglesKHR");
    pvkGetPhysicalDeviceSurfaceFormats2KHR = (void *)vkGetInstanceProcAddr(vk_instance,
            "vkGetPhysicalDeviceSurfaceFormats2KHR");

    bval = find_queue_family(vk_physical_device, VK_QUEUE_GRAPHICS_BIT, &queue_family_index);
    ok(bval, "Could not find presentation queue.\n");

    count = 0;
    vr = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, surface, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(count, "Got zero count.\n");
    formats = malloc(sizeof(*formats) * count);
    vr = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, surface, &count, formats);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    todo_wine
    ok(formats[0].format == VK_FORMAT_B8G8R8A8_UNORM, "Got formats[0].format %#x\n", formats[0].format);
    ok(formats[0].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            "Got formats[0].colorSpace %#x\n", formats[0].colorSpace);
    todo_wine
    ok(formats[1].format == VK_FORMAT_B8G8R8A8_SRGB, "Got formats[1].format %#x\n", formats[1].format);
    ok(formats[1].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            "Got formats[1].colorSpace %#x\n", formats[1].colorSpace);

    if (!pvkGetPhysicalDeviceSurfaceFormats2KHR)
        win_skip("vkGetPhysicalDeviceSurfaceFormats2KHR not found, skipping tests\n");
    else
    {
        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        VkSurfaceFormat2KHR *formats2;
        UINT i;

        surface_info.surface = surface;

        vr = pvkGetPhysicalDeviceSurfaceFormats2KHR(vk_physical_device, &surface_info, &count, NULL);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
        ok(count, "Got zero count.\n");

        formats2 = calloc(count, sizeof(*formats2));
        for (i = 0; i < count; i++) formats2[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        vr = pvkGetPhysicalDeviceSurfaceFormats2KHR(vk_physical_device, &surface_info, &count, formats2);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
        ok(count, "Got zero count.\n");

        while (count--)
        {
            ok(formats2[count].surfaceFormat.format == formats[count].format,
                    "Got formats2[%u].surfaceFormat.format %#x\n", count,
                    formats2[count].surfaceFormat.format);
            ok(formats2[count].surfaceFormat.colorSpace == formats[count].colorSpace,
                    "Got formats2[%u].surfaceFormat.colorSpace %#x\n", count,
                    formats2[count].surfaceFormat.colorSpace);
        }

        free(formats2);
    }

    free(formats);

    vr = vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, queue_family_index, surface, &bval);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, surface, &surf_caps);
    if (!IsWindow(hwnd))
    {
        ok(vr == VK_ERROR_SURFACE_LOST_KHR /* Nvidia */ || vr == VK_ERROR_UNKNOWN /* AMD */,
                "Got unexpected vr %d.\n", vr);
        memset(&surf_caps, 0, sizeof(surf_caps));
    }
    else
    {
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

        ok(surf_caps.minImageCount > 0, "Got minImageCount %u\n", surf_caps.minImageCount);
        ok(surf_caps.maxImageCount > 2, "Got minImageCount %u\n", surf_caps.maxImageCount);
        ok(surf_caps.minImageCount <= surf_caps.maxImageCount, "Got maxImageCount %u\n", surf_caps.maxImageCount);

        ok(surf_caps.currentExtent.width == client_rect.right - client_rect.left,
                "Got currentExtent.width %d\n", surf_caps.currentExtent.width);
        ok(surf_caps.currentExtent.height == client_rect.bottom - client_rect.top,
                "Got currentExtent.height %d\n", surf_caps.currentExtent.height);

        ok(surf_caps.minImageExtent.width == surf_caps.currentExtent.width,
                "Got minImageExtent.width %d\n", surf_caps.minImageExtent.width);
        ok(surf_caps.minImageExtent.height == surf_caps.currentExtent.height,
                "Got minImageExtent.height %d\n", surf_caps.minImageExtent.height);
        ok(surf_caps.maxImageExtent.width == surf_caps.currentExtent.width,
                "Got maxImageExtent.width %d\n", surf_caps.maxImageExtent.width);
        ok(surf_caps.maxImageExtent.height == surf_caps.currentExtent.height,
                "Got maxImageExtent.height %d\n", surf_caps.maxImageExtent.height);

        ok(surf_caps.maxImageArrayLayers == 1, "Got maxImageArrayLayers %u\n", surf_caps.maxImageArrayLayers);
        ok(surf_caps.supportedTransforms == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                "Got supportedTransforms %#x\n", surf_caps.supportedTransforms);
        ok(surf_caps.currentTransform == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                "Got currentTransform %#x\n", surf_caps.currentTransform);
        todo_wine
        ok(surf_caps.supportedCompositeAlpha == VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                "Got supportedCompositeAlpha %#x\n", surf_caps.supportedCompositeAlpha);
        todo_wine
        ok(surf_caps.supportedUsageFlags == 0x9f, "Got supportedUsageFlags %#x\n", surf_caps.supportedUsageFlags);
    }

    if (!pvkGetPhysicalDeviceSurfaceCapabilities2KHR)
        win_skip("vkGetPhysicalDeviceSurfaceCapabilities2KHR not found, skipping tests\n");
    else
    {
        VkPhysicalDeviceSurfaceInfo2KHR surface_info = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        VkSurfaceCapabilities2KHR surface_capabilities = {.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
        surface_info.surface = surface;

        vr = pvkGetPhysicalDeviceSurfaceCapabilities2KHR(vk_physical_device, &surface_info, &surface_capabilities);
        if (IsWindow(hwnd))
            ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
        else
            ok(vr == VK_ERROR_SURFACE_LOST_KHR /* Nvidia */ || vr == VK_ERROR_UNKNOWN /* AMD */,
                    "Got unexpected vr %d.\n", vr);
    }

    count = 0;
    vr = pvkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, surface, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(count, "Got zero count.\n");
    modes = malloc(sizeof(*modes) * count);
    vr = pvkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, surface, &count, modes);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    free(modes);

    count = 0;
    vr = pvkGetPhysicalDevicePresentRectanglesKHR(vk_physical_device, surface, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(count == 1, "Got unexpected count %u.\n", count);

    memset(&rect, 0xcc, sizeof(rect));
    vr = pvkGetPhysicalDevicePresentRectanglesKHR(vk_physical_device, surface, &count, &rect);
    if (IsWindow(hwnd))
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    else
        ok(vr == VK_SUCCESS /* Nvidia */ || vr == VK_ERROR_UNKNOWN /* AMD */, "Got unexpected vr %d.\n", vr);

    memset(&rect, 0xcc, sizeof(rect));
    vr = pvkGetPhysicalDevicePresentRectanglesKHR(vk_physical_device, surface, &count, &rect);
    ok(vr == VK_SUCCESS /* Nvidia */ || vr == VK_ERROR_UNKNOWN /* AMD */, "Got unexpected vr %d.\n", vr);
    if (vr == VK_SUCCESS)
    {
        RECT tmp_rect =
        {
            rect.offset.x,
            rect.offset.y,
            rect.offset.x + rect.extent.width,
            rect.offset.y + rect.extent.height,
        };

        ok(count == 1, "Got unexpected count %u.\n", count);
        todo_wine_if(IsWindow(hwnd) && IsRectEmpty(&client_rect))
        ok(EqualRect(&tmp_rect, &client_rect), "Got unexpected rect %s.\n", wine_dbgstr_rect(&tmp_rect));
    }

    vr = vkGetDeviceGroupSurfacePresentModesKHR(device, surface, &present_mode_flags);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
}

static void test_win32_surface_swapchain_hwnd(VkDevice device, VkSwapchainKHR swapchain,
        VkQueue queue, VkCommandBuffer cmd, HWND hwnd, BOOL expect_suboptimal)
{
    VkAcquireNextImageInfoKHR acquire_info = {.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR};
    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VkImageMemoryBarrier image_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    VkFenceCreateInfo fence_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkPresentInfoKHR present_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    uint32_t image_count, image_index;
    VkResult vr, present_result;
    VkImage *images;
    VkFence fence;

    vr = vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    images = malloc(image_count * sizeof(*images));
    vr = vkGetSwapchainImagesKHR(device, swapchain, &image_count, images);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    vr = vkCreateFence(device, &fence_info, NULL, &fence);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    vr = vkResetFences(device, 1, &fence);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    image_index = 0xdeadbeef;
    vr = vkAcquireNextImageKHR(device, swapchain, -1, VK_NULL_HANDLE, fence, &image_index);
    if (expect_suboptimal)
    {
        todo_wine_if(vr == VK_SUCCESS)
        ok(vr == VK_SUBOPTIMAL_KHR || broken(vr == VK_SUCCESS) /* Nvidia */, "Got unexpected vr %d.\n", vr);
    }
    else
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(image_index != 0xdeadbeef, "Got image_index %d.\n", image_index);

    vr = vkWaitForFences(device, 1, &fence, VK_FALSE, -1);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    vr = vkResetFences(device, 1, &fence);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    /* transition swapchain image from whatever to PRESENT_SRC */
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = images[image_index];
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstAccessMask = 0;

    vr = vkResetCommandBuffer(cmd, 0);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    vr = vkBeginCommandBuffer(cmd, &begin_info);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_barrier);
    vr = vkEndCommandBuffer(cmd);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    vr = vkQueueSubmit(queue, 1, &submit_info, fence);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    vr = vkWaitForFences(device, 1, &fence, VK_FALSE, -1);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    vr = vkResetFences(device, 1, &fence);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pResults = &present_result;

    vr = vkQueuePresentKHR(queue, &present_info);
    if (expect_suboptimal)
        ok(vr == VK_SUBOPTIMAL_KHR || broken(vr == VK_ERROR_OUT_OF_DATE_KHR) /* Nvidia */,
                "Got unexpected vr %d.\n", vr);
    else if (IsWindow(hwnd))
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    else
    {
        ok(vr == VK_SUCCESS /* AMD */ || vr == VK_ERROR_DEVICE_LOST /* AMD */ ||
                        vr == VK_ERROR_OUT_OF_DATE_KHR /* Nvidia */,
                "Got unexpected vr %d.\n", vr);
    }

    image_index = 0xdeadbeef;
    acquire_info.swapchain = swapchain;
    acquire_info.timeout = -1;
    acquire_info.fence = fence;
    acquire_info.deviceMask = 1;
    vr = vkAcquireNextImage2KHR(device, &acquire_info, &image_index);
    if (expect_suboptimal)
    {
        todo_wine_if(vr == VK_SUCCESS)
        ok(vr == VK_SUBOPTIMAL_KHR || broken(vr == VK_ERROR_OUT_OF_DATE_KHR) /* Nvidia */,
                "Got unexpected vr %d.\n", vr);
    }
    else if (IsWindow(hwnd))
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    else
        ok(vr == VK_SUCCESS /* AMD */ || vr == VK_ERROR_OUT_OF_DATE_KHR /* Nvidia */,
                "Got unexpected vr %d.\n", vr);

    if (vr >= VK_SUCCESS)
    {
        vr = vkWaitForFences(device, 1, &fence, VK_FALSE, -1);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    }

    vkDestroyFence(device, fence, NULL);

    free(images);
}

static void test_win32_surface(VkInstance instance, VkPhysicalDevice physical_device)
{
    static const char *const device_extensions[] = {"VK_KHR_swapchain", "VK_KHR_device_group"};

    VkCommandBufferAllocateInfo allocate_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    VkWin32SurfaceCreateInfoKHR create_info = {.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    VkCommandPoolCreateInfo pool_create_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    VkCommandBuffer command_buffer;
    uint32_t queue_family_index;
    VkCommandPool command_pool;
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue queue;
    VkResult vr;
    HWND hwnd;

    vr = create_device(physical_device, ARRAY_SIZE(device_extensions), device_extensions, NULL, &device);
    if (vr != VK_SUCCESS) /* Wine testbot is missing VK_KHR_device_group */
        vr = create_device(physical_device, ARRAY_SIZE(device_extensions) - 1, device_extensions, NULL, &device);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    find_queue_family(physical_device, VK_QUEUE_GRAPHICS_BIT, &queue_family_index);
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);

    pool_create_info.queueFamilyIndex = queue_family_index;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vr = vkCreateCommandPool(device, &pool_create_info, NULL, &command_pool);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    vr = vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);

    /* test NULL window */

    winetest_push_context("null");

    surface = 0xdeadbeef;
    vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    test_win32_surface_hwnd(instance, physical_device, device, surface, NULL);

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, NULL, &swapchain);
    ok(vr == VK_ERROR_INITIALIZATION_FAILED /* Nvidia */ || vr == VK_SUCCESS /* AMD */,
            "Got unexpected vr %d.\n", vr);
    if (vr == VK_SUCCESS)
    {
        ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
        test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, NULL, FALSE);
        vkDestroySwapchainKHR(device, swapchain, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    winetest_pop_context();

    /* test normal window */

    winetest_push_context("created");

    hwnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200,
            0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExW failed, error %lu\n", GetLastError());

    surface = 0xdeadbeef;
    create_info.hwnd = hwnd;
    vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    test_win32_surface_hwnd(instance, physical_device, device, surface, hwnd);

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    vkDestroySurfaceKHR(instance, surface, NULL);
    DestroyWindow(hwnd);
    winetest_pop_context();

    /* test destroyed window */

    winetest_push_context("destroyed");

    hwnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200,
            0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExW failed, error %lu\n", GetLastError());

    surface = 0xdeadbeef;
    create_info.hwnd = hwnd;
    vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    /* test a swapchain outliving the window */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");

    DestroyWindow(hwnd);

    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    test_win32_surface_hwnd(instance, physical_device, device, surface, hwnd);

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_ERROR_INITIALIZATION_FAILED /* Nvidia */ || vr == VK_SUCCESS /* AMD */,
            "Got unexpected vr %d.\n", vr);
    if (vr == VK_SUCCESS)
    {
        ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
        test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
        vkDestroySwapchainKHR(device, swapchain, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    winetest_pop_context();

    /* test resized window */

    winetest_push_context("resized");

    hwnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200,
            0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExW failed, error %lu\n", GetLastError());

    surface = 0xdeadbeef;
    create_info.hwnd = hwnd;
    vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    /* test a swapchain created before the window is resized */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");

    SetWindowPos(hwnd, 0, 0, 0, 50, 50, SWP_NOMOVE);

    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, TRUE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    test_win32_surface_hwnd(instance, physical_device, device, surface, hwnd);

    /* test a swapchain created after the window has been resized */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    vkDestroySurfaceKHR(instance, surface, NULL);
    DestroyWindow(hwnd);
    winetest_pop_context();

    /* test hidden window */

    winetest_push_context("hidden");

    hwnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200,
            0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExW failed, error %lu\n", GetLastError());

    surface = 0xdeadbeef;
    create_info.hwnd = hwnd;
    vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    /* test a swapchain created before the window is hidden */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");

    ShowWindow(hwnd, SW_HIDE);

    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    test_win32_surface_hwnd(instance, physical_device, device, surface, hwnd);

    /* test a swapchain created after the window has been hidden */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    vkDestroySurfaceKHR(instance, surface, NULL);
    DestroyWindow(hwnd);
    winetest_pop_context();

    /* tests minimized window */

    winetest_push_context("minimized");

    hwnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200,
            0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExW failed, error %lu\n", GetLastError());

    surface = 0xdeadbeef;
    create_info.hwnd = hwnd;
    vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(surface != 0xdeadbeef, "Surface not created.\n");

    /* test a swapchain created before the window is minimized */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");

    ShowWindow(hwnd, SW_MINIMIZE);

    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, TRUE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    test_win32_surface_hwnd(instance, physical_device, device, surface, hwnd);

    /* test a swapchain created after the window has been minimized */

    swapchain = 0xdeadbeef;
    vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
    ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
    ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
    test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
    vkDestroySwapchainKHR(device, swapchain, NULL);

    vkDestroySurfaceKHR(instance, surface, NULL);
    DestroyWindow(hwnd);
    winetest_pop_context();

    /* works on Windows, crashes on Wine */
    if (0)
    {
        /* test desktop window */

        winetest_push_context("desktop");

        hwnd = GetDesktopWindow();

        surface = 0xdeadbeef;
        create_info.hwnd = hwnd;
        vr = vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &surface);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
        ok(surface != 0xdeadbeef, "Surface not created.\n");

        test_win32_surface_hwnd(instance, physical_device, device, surface, hwnd);

        swapchain = 0xdeadbeef;
        vr = create_swapchain(physical_device, surface, device, hwnd, &swapchain);
        ok(vr == VK_SUCCESS, "Got unexpected vr %d.\n", vr);
        ok(swapchain != 0xdeadbeef, "Swapchain not created.\n");
        test_win32_surface_swapchain_hwnd(device, swapchain, queue, command_buffer, hwnd, FALSE);
        vkDestroySwapchainKHR(device, swapchain, NULL);

        vkDestroySurfaceKHR(instance, surface, NULL);
        winetest_pop_context();
    }

    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroyDevice(device, NULL);
}

static uint32_t find_memory_type(VkPhysicalDevice vk_physical_device, VkMemoryPropertyFlagBits flags, uint32_t mask)
{
    VkPhysicalDeviceMemoryProperties properties = {0};
    unsigned int i;

    vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &properties);

    for(i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((1u << i) & mask && properties.memoryTypes[i].propertyFlags & flags)
            return i;
    }
    return -1;
}

static void test_cross_process_resource(VkPhysicalDeviceIDPropertiesKHR *device_id_properties, BOOL kmt, HANDLE handle)
{
    char driver_uuid[VK_UUID_SIZE * 2 + 1], device_uuid[VK_UUID_SIZE * 2 + 1];
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char **argv, buf[MAX_PATH];
    unsigned int i;
    BOOL res;

    for (i = 0; i < VK_UUID_SIZE; i++)
    {
        sprintf(&driver_uuid[i * 2], "%02X",  device_id_properties->driverUUID[i]);
        sprintf(&device_uuid[i * 2], "%02X",  device_id_properties->deviceUUID[i]);
    }
    driver_uuid[i * 2] = 0;
    device_uuid[i * 2] = 0;

    winetest_get_mainargs(&argv);
    sprintf(buf, "\"%s\" vulkan resource %s %s %s %p", argv[0], driver_uuid, device_uuid,
                                                        kmt ? "kmt" : "nt", handle);
    res = CreateProcessA(NULL, buf, NULL, NULL, TRUE, 0L, NULL, NULL, &si, &info);
    ok(res, "CreateProcess failed: %lu\n", GetLastError());
    CloseHandle(info.hThread);

    wait_child_process(info.hProcess);
}

static void import_memory(VkDevice vk_device, VkMemoryAllocateInfo alloc_info, VkExternalMemoryHandleTypeFlagBits handle_type, HANDLE handle)
{
    VkImportMemoryWin32HandleInfoKHR import_handle_info;
    VkDeviceMemory memory;
    VkResult vr;

    import_handle_info.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
    import_handle_info.pNext = alloc_info.pNext;
    import_handle_info.handleType = handle_type;
    import_handle_info.handle = handle;
    import_handle_info.name = NULL;

    alloc_info.pNext = &import_handle_info;

    vr = vkAllocateMemory(vk_device, &alloc_info, NULL, &memory);
    ok(vr == VK_SUCCESS, "vkAllocateMemory failed, VkResult %d. type=%#x\n", vr, handle_type);
    vkFreeMemory(vk_device, memory, NULL);

    /* KMT-exportable objects can't be named */
    if (handle_type != VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR)
    {
        import_handle_info.handle = NULL;
        import_handle_info.name = L"wine_test_buffer_export_name";

        vr = vkAllocateMemory(vk_device, &alloc_info, NULL, &memory);
        ok(vr == VK_SUCCESS, "vkAllocateMemory failed, VkResult %d.\n", vr);
        vkFreeMemory(vk_device, memory, NULL);
    }
}

static const char *test_external_memory_extensions[] =
{
    "VK_KHR_external_memory_capabilities",
    "VK_KHR_get_physical_device_properties2",
};

static void test_external_memory(VkInstance vk_instance, VkPhysicalDevice vk_physical_device)
{
    PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR;
    PFN_vkGetPhysicalDeviceProperties2 pfn_vkGetPhysicalDeviceProperties2;
    VkExternalMemoryBufferCreateInfoKHR buffer_external_memory_info;
    PFN_vkGetMemoryWin32HandleKHR pfn_vkGetMemoryWin32HandleKHR;
    VkPhysicalDeviceExternalBufferInfoKHR external_buffer_info;
    VkExternalBufferPropertiesKHR external_buffer_properties;
    VkMemoryDedicatedAllocateInfoKHR dedicated_alloc_info;
    VkPhysicalDeviceIDPropertiesKHR device_id_properties;
    VkExportMemoryWin32HandleInfoKHR export_handle_info;
    VkPhysicalDeviceProperties2KHR device_properties;
    VkExportMemoryAllocateInfoKHR export_memory_info;
    VkMemoryGetWin32HandleInfoKHR get_handle_info;
    VkExternalMemoryHandleTypeFlagBits handle_type;
    VkMemoryRequirements memory_requirements;
    VkBufferCreateInfo buffer_create_info;
    VkMemoryAllocateInfo alloc_info;
    VkDeviceMemory vk_memory;
    SECURITY_ATTRIBUTES sa;
    unsigned int val, i;
    VkBuffer vk_buffer;
    VkDevice vk_device;
    HANDLE handle;
    VkResult vr;
    char **argv;
    int argc;

    static const char *extensions[] =
    {
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_external_memory",
        "VK_KHR_external_memory_win32",
    };

    pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR =
        (void*) vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");

    pfn_vkGetPhysicalDeviceProperties2 =
        (void*) vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceProperties2KHR");

    if (pfn_vkGetPhysicalDeviceProperties2)
    {
        device_id_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;
        device_id_properties.pNext = NULL;

        device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
        device_properties.pNext = &device_id_properties;

        pfn_vkGetPhysicalDeviceProperties2(vk_physical_device, &device_properties);
    }

    argc = winetest_get_mainargs(&argv);
    if (argc > 3 && !strcmp(argv[2], "resource"))
    {
        for (i = 0; i < VK_UUID_SIZE; i++)
        {
            sscanf(&argv[3][i * 2], "%02X", &val);
            if (val != device_id_properties.driverUUID[i])
                break;

            sscanf(&argv[4][i * 2], "%02X", &val);
            if (val != device_id_properties.deviceUUID[i])
                break;
        }

        if (i != VK_UUID_SIZE)
            return;
    }

    if ((vr = create_device(vk_physical_device, ARRAY_SIZE(extensions), extensions, NULL, &vk_device)))
    {
        skip("Failed to create device with external memory extensions, VkResult %d.\n", vr);
        return;
    }

    pfn_vkGetMemoryWin32HandleKHR = (void *) vkGetDeviceProcAddr(vk_device, "vkGetMemoryWin32HandleKHR");

    buffer_external_memory_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR;
    buffer_external_memory_info.pNext = NULL;
    buffer_external_memory_info.handleTypes = 0;

    external_buffer_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO_KHR;
    external_buffer_info.pNext = NULL;
    external_buffer_info.flags = 0;
    external_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    external_buffer_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

    memset(&external_buffer_properties, 0, sizeof(external_buffer_properties));
    external_buffer_properties.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES_KHR;

    pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR(vk_physical_device, &external_buffer_info, &external_buffer_properties);

    if (!(~external_buffer_properties.externalMemoryProperties.externalMemoryFeatures &
            (VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR|VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR)))
    {
        ok(external_buffer_properties.externalMemoryProperties.compatibleHandleTypes & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            "Unexpected compatibleHandleTypes %#x.\n", external_buffer_properties.externalMemoryProperties.compatibleHandleTypes);

        buffer_external_memory_info.handleTypes |= VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
    }

    external_buffer_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;

    memset(&external_buffer_properties, 0, sizeof(external_buffer_properties));
    external_buffer_properties.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES_KHR;

    pfn_vkGetPhysicalDeviceExternalBufferPropertiesKHR(vk_physical_device, &external_buffer_info, &external_buffer_properties);

    if (!(~external_buffer_properties.externalMemoryProperties.externalMemoryFeatures &
            (VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR|VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR)))
    {
        ok(external_buffer_properties.externalMemoryProperties.compatibleHandleTypes & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
            "Unexpected compatibleHandleTypes %#x.\n", external_buffer_properties.externalMemoryProperties.compatibleHandleTypes);

        buffer_external_memory_info.handleTypes |= VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    }

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = &buffer_external_memory_info;
    buffer_create_info.flags = 0;
    buffer_create_info.size = 1;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = NULL;
    if ((vr = vkCreateBuffer(vk_device, &buffer_create_info, NULL, &vk_buffer)))
    {
        skip("Failed to create generic buffer, VkResult %d.\n", vr);
        vkDestroyDevice(vk_device, NULL);
        return;
    }

    vkGetBufferMemoryRequirements(vk_device, vk_buffer, &memory_requirements);

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(vk_physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements.memoryTypeBits);

    /* Most implementations only support exporting dedicated allocations */

    dedicated_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
    dedicated_alloc_info.pNext = NULL;
    dedicated_alloc_info.image = VK_NULL_HANDLE;
    dedicated_alloc_info.buffer = vk_buffer;

    if (argc > 3 && !strcmp(argv[2], "resource"))
    {
        handle_type = strcmp(argv[5], "kmt") ?
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR :
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;

        sscanf(argv[6], "%p", &handle);

        ok(handle_type & buffer_external_memory_info.handleTypes,
            "External memory capabilities for handleType %#x do not match on child process.\n", handle_type);

        alloc_info.pNext = &dedicated_alloc_info;
        import_memory(vk_device, alloc_info, handle_type, handle);

        vkDestroyBuffer(vk_device, vk_buffer, NULL);
        vkDestroyDevice(vk_device, NULL);

        return;
    }

    if (!(buffer_external_memory_info.handleTypes & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR))
        skip("With desired parameters, buffers are not exportable to and importable from an NT handle.\n");
    else
    {
        export_memory_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
        export_memory_info.pNext = &dedicated_alloc_info;
        export_memory_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        export_handle_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
        export_handle_info.pNext = &export_memory_info;
        export_handle_info.name = L"wine_test_buffer_export_name";
        export_handle_info.dwAccess = GENERIC_ALL;
        export_handle_info.pAttributes = &sa;

        alloc_info.pNext = &export_handle_info;

        ok(alloc_info.memoryTypeIndex != -1, "Device local memory type index was not found.\n");

        vr = vkAllocateMemory(vk_device, &alloc_info, NULL, &vk_memory);
        ok(vr == VK_SUCCESS, "vkAllocateMemory failed, VkResult %d.\n", vr);

        get_handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        get_handle_info.pNext = NULL;
        get_handle_info.memory = vk_memory;
        get_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

        vr = pfn_vkGetMemoryWin32HandleKHR(vk_device, &get_handle_info, &handle);
        todo_wine ok(vr == VK_SUCCESS, "vkGetMemoryWin32HandleKHR failed, VkResult %d.\n", vr);

        alloc_info.pNext = &dedicated_alloc_info;
        import_memory(vk_device, alloc_info, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR, handle);

        if (pfn_vkGetPhysicalDeviceProperties2)
            test_cross_process_resource(&device_id_properties, FALSE, handle);
        else
            skip("Skipping cross process shared resource test due to lack of VK_KHR_get_physical_device_properties2.\n");

        vkFreeMemory(vk_device, vk_memory, NULL);
        CloseHandle(handle);
    }

    if (!(buffer_external_memory_info.handleTypes & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR))
        skip("With desired parameters, buffers are not exportable to and importable from a KMT handle.\n");
    else
    {
        export_memory_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
        export_memory_info.pNext = &dedicated_alloc_info;
        export_memory_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;

        alloc_info.pNext = &export_memory_info;

        ok(alloc_info.memoryTypeIndex != -1, "Device local memory type index was not found.\n");

        vr = vkAllocateMemory(vk_device, &alloc_info, NULL, &vk_memory);
        ok(vr == VK_SUCCESS, "vkAllocateMemory failed, VkResult %d.\n", vr);

        get_handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        get_handle_info.pNext = NULL;
        get_handle_info.memory = vk_memory;
        get_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;

        vr = pfn_vkGetMemoryWin32HandleKHR(vk_device, &get_handle_info, &handle);
        ok(vr == VK_SUCCESS, "vkGetMemoryWin32HandleKHR failed, VkResult %d.\n", vr);

        alloc_info.pNext = &dedicated_alloc_info;
        import_memory(vk_device, alloc_info, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR, handle);

        if (pfn_vkGetPhysicalDeviceProperties2)
            test_cross_process_resource(&device_id_properties, TRUE, handle);
        else
            skip("Skipping cross process shared resource test due to lack of VK_KHR_get_physical_device_properties2.\n");

        vkFreeMemory(vk_device, vk_memory, NULL);
    }

    vkDestroyBuffer(vk_device, vk_buffer, NULL);
    vkDestroyDevice(vk_device, NULL);
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

    vk_physical_devices = calloc(count, sizeof(*vk_physical_devices));
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

    free(vk_physical_devices);

    vkDestroyInstance(vk_instance, NULL);
}

static void for_each_device(void (*test_func)(VkPhysicalDevice))
{
    for_each_device_instance(0, NULL, NULL, test_func);
}

START_TEST(vulkan)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);

    if (argc > 3 && !strcmp(argv[2], "resource"))
    {
        ok(argc >= 7, "Missing launch arguments\n");
        for_each_device_instance(ARRAY_SIZE(test_external_memory_extensions), test_external_memory_extensions, test_external_memory, NULL);
        return;
    }

    test_instance_version();
    for_each_device(enumerate_physical_device);
    test_enumerate_physical_device2();
    for_each_device(enumerate_device_queues);
    test_physical_device_groups();
    for_each_device(test_destroy_command_pool);
    test_unsupported_instance_extensions();
    for_each_device(test_unsupported_device_extensions);
    for_each_device(test_private_data);
    for_each_device_instance(ARRAY_SIZE(test_win32_surface_extensions), test_win32_surface_extensions, test_win32_surface, NULL);
    for_each_device_instance(ARRAY_SIZE(test_external_memory_extensions), test_external_memory_extensions, test_external_memory, NULL);
}
