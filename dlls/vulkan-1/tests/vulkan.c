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
    VkResult vr;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.pApplicationInfo = NULL;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = enabled_extensions;

    if ((vr = vkCreateInstance(&create_info, NULL, vk_instance)) >= 0)
        return vr;

    switch (vr)
    {
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            if (extension_count == 1)
                skip("Instance extension '%s' not supported.\n", enabled_extensions[0]);
            else
                skip("Instance extensions not supported.\n");
            break;

        default:
            skip("Failed to create Vulkan instance, vr %d.\n", vr);
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

    if ((vr = create_instance(ARRAY_SIZE(extensions), extensions, &vk_instance)) < 0)
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

static void for_each_device(void (*test_func)(VkPhysicalDevice))
{
    VkPhysicalDevice *vk_physical_devices;
    VkInstance vk_instance;
    unsigned int i;
    uint32_t count;
    VkResult vr;

    if ((vr = create_instance(0, NULL, &vk_instance)) < 0)
        return;
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    vr = vkEnumeratePhysicalDevices(vk_instance, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);
    if (!count)
    {
        skip("No physical devices.\n");
        vkDestroyInstance(vk_instance, NULL);
        return;
    }

    vk_physical_devices = heap_calloc(count, sizeof(*vk_physical_devices));
    ok(!!vk_physical_devices, "Failed to allocate memory.\n");
    vr = vkEnumeratePhysicalDevices(vk_instance, &count, vk_physical_devices);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    for (i = 0; i < count; ++i)
        test_func(vk_physical_devices[i]);

    heap_free(vk_physical_devices);

    vkDestroyInstance(vk_instance, NULL);
}

START_TEST(vulkan)
{
    test_instance_version();
    for_each_device(enumerate_physical_device);
    for_each_device(enumerate_device_queues);
    test_physical_device_groups();
}
