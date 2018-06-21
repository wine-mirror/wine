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

static void test_enumerate_physical_devices(void)
{
    VkPhysicalDevice *vk_physical_devices;
    VkPhysicalDeviceProperties properties;
    VkInstance vk_instance;
    unsigned int i;
    uint32_t count;
    VkResult vr;

    if ((vr = create_instance(0, NULL, &vk_instance)) < 0)
    {
        skip("Failed to create Vulkan instance, vr %d.\n", vr);
        return;
    }
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    vr = vkEnumeratePhysicalDevices(vk_instance, &count, NULL);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);
    if (!count)
    {
        skip("No physical devices.\n");
        vkDestroyInstance(vk_instance, NULL);
        return;
    }

    trace("Got %u physical device(s).\n", count);
    vk_physical_devices = heap_calloc(count, sizeof(*vk_physical_devices));
    ok(!!vk_physical_devices, "Failed to allocate memory.\n");
    vr = vkEnumeratePhysicalDevices(vk_instance, &count, vk_physical_devices);
    ok(vr == VK_SUCCESS, "Got unexpected VkResult %d.\n", vr);

    for (i = 0; i < count; ++i)
    {
        vkGetPhysicalDeviceProperties(vk_physical_devices[i], &properties);

        trace("Device '%s', %#x:%#x, driver version %u.%u.%u (%#x), api version %u.%u.%u.\n",
                properties.deviceName, properties.vendorID, properties.deviceID,
                VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion), properties.driverVersion,
                VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));
    }

    heap_free(vk_physical_devices);

    vkDestroyInstance(vk_instance, NULL);
}

START_TEST(vulkan)
{
    test_enumerate_physical_devices();
}
