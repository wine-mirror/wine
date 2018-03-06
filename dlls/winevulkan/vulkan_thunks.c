/* Automatically generated from Vulkan vk.xml; DO NOT EDIT! */

#include "config.h"
#include "wine/port.h"

#include "wine/debug.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"
#include "vulkan_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vulkan);

static VkResult WINAPI wine_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
    FIXME("stub: %p, %p, %p, %p\n", physicalDevice, pCreateInfo, pAllocator, pDevice);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static VkResult WINAPI wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    FIXME("stub: %p, %p, %p\n", physicalDevice, pPropertyCount, pProperties);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static void WINAPI wine_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures)
{
    FIXME("stub: %p, %p\n", physicalDevice, pFeatures);
}

static void WINAPI wine_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties)
{
    FIXME("stub: %p, %d, %p\n", physicalDevice, format, pFormatProperties);
}

static VkResult WINAPI wine_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties)
{
    FIXME("stub: %p, %d, %d, %d, %#x, %#x, %p\n", physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
}

static void WINAPI wine_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    FIXME("stub: %p, %p\n", physicalDevice, pMemoryProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties)
{
    FIXME("stub: %p, %p\n", physicalDevice, pProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties)
{
    FIXME("stub: %p, %p, %p\n", physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

static void WINAPI wine_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties)
{
    FIXME("stub: %p, %d, %d, %d, %#x, %d, %p, %p\n", physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

static const struct vulkan_func vk_instance_dispatch_table[] =
{
    {"vkCreateDevice", &wine_vkCreateDevice},
    {"vkDestroyInstance", &wine_vkDestroyInstance},
    {"vkEnumerateDeviceExtensionProperties", &wine_vkEnumerateDeviceExtensionProperties},
    {"vkEnumerateDeviceLayerProperties", &wine_vkEnumerateDeviceLayerProperties},
    {"vkEnumeratePhysicalDevices", &wine_vkEnumeratePhysicalDevices},
    {"vkGetPhysicalDeviceFeatures", &wine_vkGetPhysicalDeviceFeatures},
    {"vkGetPhysicalDeviceFormatProperties", &wine_vkGetPhysicalDeviceFormatProperties},
    {"vkGetPhysicalDeviceImageFormatProperties", &wine_vkGetPhysicalDeviceImageFormatProperties},
    {"vkGetPhysicalDeviceMemoryProperties", &wine_vkGetPhysicalDeviceMemoryProperties},
    {"vkGetPhysicalDeviceProperties", &wine_vkGetPhysicalDeviceProperties},
    {"vkGetPhysicalDeviceQueueFamilyProperties", &wine_vkGetPhysicalDeviceQueueFamilyProperties},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", &wine_vkGetPhysicalDeviceSparseImageFormatProperties},
};

void *wine_vk_get_instance_proc_addr(const char *name)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(vk_instance_dispatch_table); i++)
    {
        if (strcmp(vk_instance_dispatch_table[i].name, name) == 0)
        {
            TRACE("Found pName=%s in instance table\n", name);
            return vk_instance_dispatch_table[i].func;
        }
    }
    return NULL;
}
