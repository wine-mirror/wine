/* Automatically generated from Vulkan vk.xml; DO NOT EDIT! */

#ifndef __WINE_VULKAN_THUNKS_H
#define __WINE_VULKAN_THUNKS_H

/* For use by vk_icdGetInstanceProcAddr / vkGetInstanceProcAddr */
void *wine_vk_get_instance_proc_addr(const char *name) DECLSPEC_HIDDEN;

/* Functions for which we have custom implementations outside of the thunks. */
void WINAPI wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) DECLSPEC_HIDDEN;

/* For use by vkInstance and children */
struct vulkan_instance_funcs
{
    VkResult (*p_vkCreateDevice)(VkPhysicalDevice, const VkDeviceCreateInfo *, const VkAllocationCallbacks *, VkDevice *);
    VkResult (*p_vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice, const char *, uint32_t *, VkExtensionProperties *);
    VkResult (*p_vkEnumerateDeviceLayerProperties)(VkPhysicalDevice, uint32_t *, VkLayerProperties *);
    VkResult (*p_vkEnumeratePhysicalDevices)(VkInstance, uint32_t *, VkPhysicalDevice *);
    void (*p_vkGetPhysicalDeviceFeatures)(VkPhysicalDevice, VkPhysicalDeviceFeatures *);
    void (*p_vkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice, VkFormat, VkFormatProperties *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties *);
    void (*p_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *);
    void (*p_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t *, VkSparseImageFormatProperties *);
};

#define ALL_VK_INSTANCE_FUNCS() \
    USE_VK_FUNC(vkCreateDevice)\
    USE_VK_FUNC(vkEnumerateDeviceExtensionProperties)\
    USE_VK_FUNC(vkEnumerateDeviceLayerProperties)\
    USE_VK_FUNC(vkEnumeratePhysicalDevices)\
    USE_VK_FUNC(vkGetPhysicalDeviceFeatures)\
    USE_VK_FUNC(vkGetPhysicalDeviceFormatProperties)\
    USE_VK_FUNC(vkGetPhysicalDeviceImageFormatProperties)\
    USE_VK_FUNC(vkGetPhysicalDeviceMemoryProperties)\
    USE_VK_FUNC(vkGetPhysicalDeviceProperties)\
    USE_VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties)\
    USE_VK_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties)

#endif /* __WINE_VULKAN_THUNKS_H */
