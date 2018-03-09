/* Automatically generated from Vulkan vk.xml; DO NOT EDIT! */

#ifndef __WINE_VULKAN_THUNKS_H
#define __WINE_VULKAN_THUNKS_H

/* Perform vulkan struct conversion on 32-bit x86 platforms. */
#if defined(__i386__)
#define USE_STRUCT_CONVERSION
#endif

/* For use by vk_icdGetInstanceProcAddr / vkGetInstanceProcAddr */
void *wine_vk_get_device_proc_addr(const char *name) DECLSPEC_HIDDEN;
void *wine_vk_get_instance_proc_addr(const char *name) DECLSPEC_HIDDEN;

/* Functions for which we have custom implementations outside of the thunks. */
void WINAPI wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) DECLSPEC_HIDDEN;

typedef struct VkImageFormatProperties_host
{
    VkExtent3D maxExtent;
    uint32_t maxMipLevels;
    uint32_t maxArrayLayers;
    VkSampleCountFlags sampleCounts;
    VkDeviceSize maxResourceSize;
} VkImageFormatProperties_host;

typedef struct VkMemoryHeap_host
{
    VkDeviceSize size;
    VkMemoryHeapFlags flags;
} VkMemoryHeap_host;

typedef struct VkPhysicalDeviceMemoryProperties_host
{
    uint32_t memoryTypeCount;
    VkMemoryType memoryTypes[VK_MAX_MEMORY_TYPES];
    uint32_t memoryHeapCount;
    VkMemoryHeap_host memoryHeaps[VK_MAX_MEMORY_HEAPS];
} VkPhysicalDeviceMemoryProperties_host;

typedef struct VkPhysicalDeviceLimits_host
{
    uint32_t maxImageDimension1D;
    uint32_t maxImageDimension2D;
    uint32_t maxImageDimension3D;
    uint32_t maxImageDimensionCube;
    uint32_t maxImageArrayLayers;
    uint32_t maxTexelBufferElements;
    uint32_t maxUniformBufferRange;
    uint32_t maxStorageBufferRange;
    uint32_t maxPushConstantsSize;
    uint32_t maxMemoryAllocationCount;
    uint32_t maxSamplerAllocationCount;
    VkDeviceSize bufferImageGranularity;
    VkDeviceSize sparseAddressSpaceSize;
    uint32_t maxBoundDescriptorSets;
    uint32_t maxPerStageDescriptorSamplers;
    uint32_t maxPerStageDescriptorUniformBuffers;
    uint32_t maxPerStageDescriptorStorageBuffers;
    uint32_t maxPerStageDescriptorSampledImages;
    uint32_t maxPerStageDescriptorStorageImages;
    uint32_t maxPerStageDescriptorInputAttachments;
    uint32_t maxPerStageResources;
    uint32_t maxDescriptorSetSamplers;
    uint32_t maxDescriptorSetUniformBuffers;
    uint32_t maxDescriptorSetUniformBuffersDynamic;
    uint32_t maxDescriptorSetStorageBuffers;
    uint32_t maxDescriptorSetStorageBuffersDynamic;
    uint32_t maxDescriptorSetSampledImages;
    uint32_t maxDescriptorSetStorageImages;
    uint32_t maxDescriptorSetInputAttachments;
    uint32_t maxVertexInputAttributes;
    uint32_t maxVertexInputBindings;
    uint32_t maxVertexInputAttributeOffset;
    uint32_t maxVertexInputBindingStride;
    uint32_t maxVertexOutputComponents;
    uint32_t maxTessellationGenerationLevel;
    uint32_t maxTessellationPatchSize;
    uint32_t maxTessellationControlPerVertexInputComponents;
    uint32_t maxTessellationControlPerVertexOutputComponents;
    uint32_t maxTessellationControlPerPatchOutputComponents;
    uint32_t maxTessellationControlTotalOutputComponents;
    uint32_t maxTessellationEvaluationInputComponents;
    uint32_t maxTessellationEvaluationOutputComponents;
    uint32_t maxGeometryShaderInvocations;
    uint32_t maxGeometryInputComponents;
    uint32_t maxGeometryOutputComponents;
    uint32_t maxGeometryOutputVertices;
    uint32_t maxGeometryTotalOutputComponents;
    uint32_t maxFragmentInputComponents;
    uint32_t maxFragmentOutputAttachments;
    uint32_t maxFragmentDualSrcAttachments;
    uint32_t maxFragmentCombinedOutputResources;
    uint32_t maxComputeSharedMemorySize;
    uint32_t maxComputeWorkGroupCount[3];
    uint32_t maxComputeWorkGroupInvocations;
    uint32_t maxComputeWorkGroupSize[3];
    uint32_t subPixelPrecisionBits;
    uint32_t subTexelPrecisionBits;
    uint32_t mipmapPrecisionBits;
    uint32_t maxDrawIndexedIndexValue;
    uint32_t maxDrawIndirectCount;
    float maxSamplerLodBias;
    float maxSamplerAnisotropy;
    uint32_t maxViewports;
    uint32_t maxViewportDimensions[2];
    float viewportBoundsRange[2];
    uint32_t viewportSubPixelBits;
    size_t minMemoryMapAlignment;
    VkDeviceSize minTexelBufferOffsetAlignment;
    VkDeviceSize minUniformBufferOffsetAlignment;
    VkDeviceSize minStorageBufferOffsetAlignment;
    int32_t minTexelOffset;
    uint32_t maxTexelOffset;
    int32_t minTexelGatherOffset;
    uint32_t maxTexelGatherOffset;
    float minInterpolationOffset;
    float maxInterpolationOffset;
    uint32_t subPixelInterpolationOffsetBits;
    uint32_t maxFramebufferWidth;
    uint32_t maxFramebufferHeight;
    uint32_t maxFramebufferLayers;
    VkSampleCountFlags framebufferColorSampleCounts;
    VkSampleCountFlags framebufferDepthSampleCounts;
    VkSampleCountFlags framebufferStencilSampleCounts;
    VkSampleCountFlags framebufferNoAttachmentsSampleCounts;
    uint32_t maxColorAttachments;
    VkSampleCountFlags sampledImageColorSampleCounts;
    VkSampleCountFlags sampledImageIntegerSampleCounts;
    VkSampleCountFlags sampledImageDepthSampleCounts;
    VkSampleCountFlags sampledImageStencilSampleCounts;
    VkSampleCountFlags storageImageSampleCounts;
    uint32_t maxSampleMaskWords;
    VkBool32 timestampComputeAndGraphics;
    float timestampPeriod;
    uint32_t maxClipDistances;
    uint32_t maxCullDistances;
    uint32_t maxCombinedClipAndCullDistances;
    uint32_t discreteQueuePriorities;
    float pointSizeRange[2];
    float lineWidthRange[2];
    float pointSizeGranularity;
    float lineWidthGranularity;
    VkBool32 strictLines;
    VkBool32 standardSampleLocations;
    VkDeviceSize optimalBufferCopyOffsetAlignment;
    VkDeviceSize optimalBufferCopyRowPitchAlignment;
    VkDeviceSize nonCoherentAtomSize;
} VkPhysicalDeviceLimits_host;

typedef struct VkPhysicalDeviceProperties_host
{
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    VkPhysicalDeviceType deviceType;
    char deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    uint8_t pipelineCacheUUID[VK_UUID_SIZE];
    VkPhysicalDeviceLimits_host limits;
    VkPhysicalDeviceSparseProperties sparseProperties;
} VkPhysicalDeviceProperties_host;


/* For use by vkInstance and children */
struct vulkan_instance_funcs
{
    VkResult (*p_vkCreateDevice)(VkPhysicalDevice, const VkDeviceCreateInfo *, const VkAllocationCallbacks *, VkDevice *);
    VkResult (*p_vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice, const char *, uint32_t *, VkExtensionProperties *);
    VkResult (*p_vkEnumerateDeviceLayerProperties)(VkPhysicalDevice, uint32_t *, VkLayerProperties *);
    VkResult (*p_vkEnumeratePhysicalDevices)(VkInstance, uint32_t *, VkPhysicalDevice *);
    void (*p_vkGetPhysicalDeviceFeatures)(VkPhysicalDevice, VkPhysicalDeviceFeatures *);
    void (*p_vkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice, VkFormat, VkFormatProperties *);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties_host *);
#else
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties_host *);
#else
    void (*p_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties_host *);
#else
    void (*p_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties *);
#endif
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
