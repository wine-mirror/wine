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

BOOL wine_vk_device_extension_supported(const char *name) DECLSPEC_HIDDEN;

/* Functions for which we have custom implementations outside of the thunks. */
VkResult WINAPI wine_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers) DECLSPEC_HIDDEN;
void WINAPI wine_vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) DECLSPEC_HIDDEN;
void WINAPI wine_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void WINAPI wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void WINAPI wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void WINAPI wine_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) DECLSPEC_HIDDEN;
void WINAPI wine_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) DECLSPEC_HIDDEN;
PFN_vkVoidFunction WINAPI wine_vkGetDeviceProcAddr(VkDevice device, const char *pName) DECLSPEC_HIDDEN;
void WINAPI wine_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported) DECLSPEC_HIDDEN;
VkBool32 WINAPI wine_vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages) DECLSPEC_HIDDEN;
VkResult WINAPI wine_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) DECLSPEC_HIDDEN;

typedef struct VkCommandBufferAllocateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkCommandPool commandPool;
    VkCommandBufferLevel level;
    uint32_t commandBufferCount;
} VkCommandBufferAllocateInfo_host;

typedef struct VkDescriptorSetAllocateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDescriptorPool descriptorPool;
    uint32_t descriptorSetCount;
    const VkDescriptorSetLayout *pSetLayouts;
} VkDescriptorSetAllocateInfo_host;

typedef struct VkMemoryAllocateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize allocationSize;
    uint32_t memoryTypeIndex;
} VkMemoryAllocateInfo_host;

typedef struct VkCommandBufferInheritanceInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkFramebuffer framebuffer;
    VkBool32 occlusionQueryEnable;
    VkQueryControlFlags queryFlags;
    VkQueryPipelineStatisticFlags pipelineStatistics;
} VkCommandBufferInheritanceInfo_host;

typedef struct VkCommandBufferBeginInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkCommandBufferUsageFlags flags;
    const VkCommandBufferInheritanceInfo_host *pInheritanceInfo;
} VkCommandBufferBeginInfo_host;

typedef struct VkRenderPassBeginInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    VkRect2D renderArea;
    uint32_t clearValueCount;
    const VkClearValue *pClearValues;
} VkRenderPassBeginInfo_host;

typedef struct VkBufferCopy_host
{
    VkDeviceSize srcOffset;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
} VkBufferCopy_host;

typedef struct VkBufferImageCopy_host
{
    VkDeviceSize bufferOffset;
    uint32_t bufferRowLength;
    uint32_t bufferImageHeight;
    VkImageSubresourceLayers imageSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VkBufferImageCopy_host;

typedef struct VkBufferMemoryBarrier_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
} VkBufferMemoryBarrier_host;

typedef struct VkImageMemoryBarrier_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    VkImage image;
    VkImageSubresourceRange subresourceRange;
} VkImageMemoryBarrier_host;

typedef struct VkBufferCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkBufferCreateFlags flags;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
} VkBufferCreateInfo_host;

typedef struct VkBufferViewCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkBufferViewCreateFlags flags;
    VkBuffer buffer;
    VkFormat format;
    VkDeviceSize offset;
    VkDeviceSize range;
} VkBufferViewCreateInfo_host;

typedef struct VkPipelineShaderStageCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineShaderStageCreateFlags flags;
    VkShaderStageFlagBits stage;
    VkShaderModule module;
    const char *pName;
    const VkSpecializationInfo *pSpecializationInfo;
} VkPipelineShaderStageCreateInfo_host;

typedef struct VkComputePipelineCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineCreateFlags flags;
    VkPipelineShaderStageCreateInfo_host stage;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
} VkComputePipelineCreateInfo_host;

typedef struct VkFramebufferCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkFramebufferCreateFlags flags;
    VkRenderPass renderPass;
    uint32_t attachmentCount;
    const VkImageView *pAttachments;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
} VkFramebufferCreateInfo_host;

typedef struct VkGraphicsPipelineCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineCreateFlags flags;
    uint32_t stageCount;
    const VkPipelineShaderStageCreateInfo_host *pStages;
    const VkPipelineVertexInputStateCreateInfo *pVertexInputState;
    const VkPipelineInputAssemblyStateCreateInfo *pInputAssemblyState;
    const VkPipelineTessellationStateCreateInfo *pTessellationState;
    const VkPipelineViewportStateCreateInfo *pViewportState;
    const VkPipelineRasterizationStateCreateInfo *pRasterizationState;
    const VkPipelineMultisampleStateCreateInfo *pMultisampleState;
    const VkPipelineDepthStencilStateCreateInfo *pDepthStencilState;
    const VkPipelineColorBlendStateCreateInfo *pColorBlendState;
    const VkPipelineDynamicStateCreateInfo *pDynamicState;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
} VkGraphicsPipelineCreateInfo_host;

typedef struct VkImageViewCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkImageViewCreateFlags flags;
    VkImage image;
    VkImageViewType viewType;
    VkFormat format;
    VkComponentMapping components;
    VkImageSubresourceRange subresourceRange;
} VkImageViewCreateInfo_host;

typedef struct VkSwapchainCreateInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkSwapchainCreateFlagsKHR flags;
    VkSurfaceKHR surface;
    uint32_t minImageCount;
    VkFormat imageFormat;
    VkColorSpaceKHR imageColorSpace;
    VkExtent2D imageExtent;
    uint32_t imageArrayLayers;
    VkImageUsageFlags imageUsage;
    VkSharingMode imageSharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t *pQueueFamilyIndices;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkCompositeAlphaFlagBitsKHR compositeAlpha;
    VkPresentModeKHR presentMode;
    VkBool32 clipped;
    VkSwapchainKHR oldSwapchain;
} VkSwapchainCreateInfoKHR_host;

typedef struct VkMappedMemoryRange_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
} VkMappedMemoryRange_host;

typedef struct VkMemoryRequirements_host
{
    VkDeviceSize size;
    VkDeviceSize alignment;
    uint32_t memoryTypeBits;
} VkMemoryRequirements_host;

typedef struct VkSubresourceLayout_host
{
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceSize rowPitch;
    VkDeviceSize arrayPitch;
    VkDeviceSize depthPitch;
} VkSubresourceLayout_host;

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

typedef struct VkDescriptorImageInfo_host
{
    VkSampler sampler;
    VkImageView imageView;
    VkImageLayout imageLayout;
} VkDescriptorImageInfo_host;

typedef struct VkDescriptorBufferInfo_host
{
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
} VkDescriptorBufferInfo_host;

typedef struct VkWriteDescriptorSet_host
{
    VkStructureType sType;
    const void *pNext;
    VkDescriptorSet dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
    VkDescriptorType descriptorType;
    const VkDescriptorImageInfo_host *pImageInfo;
    const VkDescriptorBufferInfo_host *pBufferInfo;
    const VkBufferView *pTexelBufferView;
} VkWriteDescriptorSet_host;

typedef struct VkCopyDescriptorSet_host
{
    VkStructureType sType;
    const void *pNext;
    VkDescriptorSet srcSet;
    uint32_t srcBinding;
    uint32_t srcArrayElement;
    VkDescriptorSet dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    uint32_t descriptorCount;
} VkCopyDescriptorSet_host;


/* For use by vkDevice and children */
struct vulkan_device_funcs
{
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkAllocateCommandBuffers)(VkDevice, const VkCommandBufferAllocateInfo_host *, VkCommandBuffer *);
#else
    VkResult (*p_vkAllocateCommandBuffers)(VkDevice, const VkCommandBufferAllocateInfo *, VkCommandBuffer *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkAllocateDescriptorSets)(VkDevice, const VkDescriptorSetAllocateInfo_host *, VkDescriptorSet *);
#else
    VkResult (*p_vkAllocateDescriptorSets)(VkDevice, const VkDescriptorSetAllocateInfo *, VkDescriptorSet *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkAllocateMemory)(VkDevice, const VkMemoryAllocateInfo_host *, const VkAllocationCallbacks *, VkDeviceMemory *);
#else
    VkResult (*p_vkAllocateMemory)(VkDevice, const VkMemoryAllocateInfo *, const VkAllocationCallbacks *, VkDeviceMemory *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkBeginCommandBuffer)(VkCommandBuffer, const VkCommandBufferBeginInfo_host *);
#else
    VkResult (*p_vkBeginCommandBuffer)(VkCommandBuffer, const VkCommandBufferBeginInfo *);
#endif
    VkResult (*p_vkBindBufferMemory)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize);
    VkResult (*p_vkBindImageMemory)(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize);
    void (*p_vkCmdBeginQuery)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkCmdBeginRenderPass)(VkCommandBuffer, const VkRenderPassBeginInfo_host *, VkSubpassContents);
#else
    void (*p_vkCmdBeginRenderPass)(VkCommandBuffer, const VkRenderPassBeginInfo *, VkSubpassContents);
#endif
    void (*p_vkCmdBindDescriptorSets)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet *, uint32_t, const uint32_t *);
    void (*p_vkCmdBindIndexBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType);
    void (*p_vkCmdBindPipeline)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline);
    void (*p_vkCmdBindVertexBuffers)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *);
    void (*p_vkCmdBlitImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit *, VkFilter);
    void (*p_vkCmdClearAttachments)(VkCommandBuffer, uint32_t, const VkClearAttachment *, uint32_t, const VkClearRect *);
    void (*p_vkCmdClearColorImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue *, uint32_t, const VkImageSubresourceRange *);
    void (*p_vkCmdClearDepthStencilImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue *, uint32_t, const VkImageSubresourceRange *);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkCmdCopyBuffer)(VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy_host *);
#else
    void (*p_vkCmdCopyBuffer)(VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkCmdCopyBufferToImage)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy_host *);
#else
    void (*p_vkCmdCopyBufferToImage)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy *);
#endif
    void (*p_vkCmdCopyImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy *);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkCmdCopyImageToBuffer)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy_host *);
#else
    void (*p_vkCmdCopyImageToBuffer)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy *);
#endif
    void (*p_vkCmdCopyQueryPoolResults)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags);
    void (*p_vkCmdDispatch)(VkCommandBuffer, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize);
    void (*p_vkCmdDraw)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndexed)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t);
    void (*p_vkCmdDrawIndexedIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdEndQuery)(VkCommandBuffer, VkQueryPool, uint32_t);
    void (*p_vkCmdEndRenderPass)(VkCommandBuffer);
    void (*p_vkCmdExecuteCommands)(VkCommandBuffer, uint32_t, const VkCommandBuffer *);
    void (*p_vkCmdFillBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t);
    void (*p_vkCmdNextSubpass)(VkCommandBuffer, VkSubpassContents);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkCmdPipelineBarrier)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier_host *, uint32_t, const VkImageMemoryBarrier_host *);
#else
    void (*p_vkCmdPipelineBarrier)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *);
#endif
    void (*p_vkCmdPushConstants)(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void *);
    void (*p_vkCmdResetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags);
    void (*p_vkCmdResetQueryPool)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
    void (*p_vkCmdResolveImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve *);
    void (*p_vkCmdSetBlendConstants)(VkCommandBuffer, const float[4]);
    void (*p_vkCmdSetDepthBias)(VkCommandBuffer, float, float, float);
    void (*p_vkCmdSetDepthBounds)(VkCommandBuffer, float, float);
    void (*p_vkCmdSetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags);
    void (*p_vkCmdSetLineWidth)(VkCommandBuffer, float);
    void (*p_vkCmdSetScissor)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetStencilCompareMask)(VkCommandBuffer, VkStencilFaceFlags, uint32_t);
    void (*p_vkCmdSetStencilReference)(VkCommandBuffer, VkStencilFaceFlags, uint32_t);
    void (*p_vkCmdSetStencilWriteMask)(VkCommandBuffer, VkStencilFaceFlags, uint32_t);
    void (*p_vkCmdSetViewport)(VkCommandBuffer, uint32_t, uint32_t, const VkViewport *);
    void (*p_vkCmdUpdateBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, const void *);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkCmdWaitEvents)(VkCommandBuffer, uint32_t, const VkEvent *, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier_host *, uint32_t, const VkImageMemoryBarrier_host *);
#else
    void (*p_vkCmdWaitEvents)(VkCommandBuffer, uint32_t, const VkEvent *, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *);
#endif
    void (*p_vkCmdWriteTimestamp)(VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkCreateBuffer)(VkDevice, const VkBufferCreateInfo_host *, const VkAllocationCallbacks *, VkBuffer *);
#else
    VkResult (*p_vkCreateBuffer)(VkDevice, const VkBufferCreateInfo *, const VkAllocationCallbacks *, VkBuffer *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkCreateBufferView)(VkDevice, const VkBufferViewCreateInfo_host *, const VkAllocationCallbacks *, VkBufferView *);
#else
    VkResult (*p_vkCreateBufferView)(VkDevice, const VkBufferViewCreateInfo *, const VkAllocationCallbacks *, VkBufferView *);
#endif
    VkResult (*p_vkCreateCommandPool)(VkDevice, const VkCommandPoolCreateInfo *, const VkAllocationCallbacks *, VkCommandPool *);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkCreateComputePipelines)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo_host *, const VkAllocationCallbacks *, VkPipeline *);
#else
    VkResult (*p_vkCreateComputePipelines)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline *);
#endif
    VkResult (*p_vkCreateDescriptorPool)(VkDevice, const VkDescriptorPoolCreateInfo *, const VkAllocationCallbacks *, VkDescriptorPool *);
    VkResult (*p_vkCreateDescriptorSetLayout)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, const VkAllocationCallbacks *, VkDescriptorSetLayout *);
    VkResult (*p_vkCreateEvent)(VkDevice, const VkEventCreateInfo *, const VkAllocationCallbacks *, VkEvent *);
    VkResult (*p_vkCreateFence)(VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *, VkFence *);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkCreateFramebuffer)(VkDevice, const VkFramebufferCreateInfo_host *, const VkAllocationCallbacks *, VkFramebuffer *);
#else
    VkResult (*p_vkCreateFramebuffer)(VkDevice, const VkFramebufferCreateInfo *, const VkAllocationCallbacks *, VkFramebuffer *);
#endif
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkCreateGraphicsPipelines)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo_host *, const VkAllocationCallbacks *, VkPipeline *);
#else
    VkResult (*p_vkCreateGraphicsPipelines)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline *);
#endif
    VkResult (*p_vkCreateImage)(VkDevice, const VkImageCreateInfo *, const VkAllocationCallbacks *, VkImage *);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkCreateImageView)(VkDevice, const VkImageViewCreateInfo_host *, const VkAllocationCallbacks *, VkImageView *);
#else
    VkResult (*p_vkCreateImageView)(VkDevice, const VkImageViewCreateInfo *, const VkAllocationCallbacks *, VkImageView *);
#endif
    VkResult (*p_vkCreatePipelineCache)(VkDevice, const VkPipelineCacheCreateInfo *, const VkAllocationCallbacks *, VkPipelineCache *);
    VkResult (*p_vkCreatePipelineLayout)(VkDevice, const VkPipelineLayoutCreateInfo *, const VkAllocationCallbacks *, VkPipelineLayout *);
    VkResult (*p_vkCreateQueryPool)(VkDevice, const VkQueryPoolCreateInfo *, const VkAllocationCallbacks *, VkQueryPool *);
    VkResult (*p_vkCreateRenderPass)(VkDevice, const VkRenderPassCreateInfo *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateSampler)(VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *, VkSampler *);
    VkResult (*p_vkCreateSemaphore)(VkDevice, const VkSemaphoreCreateInfo *, const VkAllocationCallbacks *, VkSemaphore *);
    VkResult (*p_vkCreateShaderModule)(VkDevice, const VkShaderModuleCreateInfo *, const VkAllocationCallbacks *, VkShaderModule *);
    void (*p_vkDestroyBuffer)(VkDevice, VkBuffer, const VkAllocationCallbacks *);
    void (*p_vkDestroyBufferView)(VkDevice, VkBufferView, const VkAllocationCallbacks *);
    void (*p_vkDestroyCommandPool)(VkDevice, VkCommandPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyDescriptorPool)(VkDevice, VkDescriptorPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyDescriptorSetLayout)(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks *);
    void (*p_vkDestroyDevice)(VkDevice, const VkAllocationCallbacks *);
    void (*p_vkDestroyEvent)(VkDevice, VkEvent, const VkAllocationCallbacks *);
    void (*p_vkDestroyFence)(VkDevice, VkFence, const VkAllocationCallbacks *);
    void (*p_vkDestroyFramebuffer)(VkDevice, VkFramebuffer, const VkAllocationCallbacks *);
    void (*p_vkDestroyImage)(VkDevice, VkImage, const VkAllocationCallbacks *);
    void (*p_vkDestroyImageView)(VkDevice, VkImageView, const VkAllocationCallbacks *);
    void (*p_vkDestroyPipeline)(VkDevice, VkPipeline, const VkAllocationCallbacks *);
    void (*p_vkDestroyPipelineCache)(VkDevice, VkPipelineCache, const VkAllocationCallbacks *);
    void (*p_vkDestroyPipelineLayout)(VkDevice, VkPipelineLayout, const VkAllocationCallbacks *);
    void (*p_vkDestroyQueryPool)(VkDevice, VkQueryPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyRenderPass)(VkDevice, VkRenderPass, const VkAllocationCallbacks *);
    void (*p_vkDestroySampler)(VkDevice, VkSampler, const VkAllocationCallbacks *);
    void (*p_vkDestroySemaphore)(VkDevice, VkSemaphore, const VkAllocationCallbacks *);
    void (*p_vkDestroyShaderModule)(VkDevice, VkShaderModule, const VkAllocationCallbacks *);
    VkResult (*p_vkDeviceWaitIdle)(VkDevice);
    VkResult (*p_vkEndCommandBuffer)(VkCommandBuffer);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkFlushMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange_host *);
#else
    VkResult (*p_vkFlushMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange *);
#endif
    void (*p_vkFreeCommandBuffers)(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *);
    VkResult (*p_vkFreeDescriptorSets)(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet *);
    void (*p_vkFreeMemory)(VkDevice, VkDeviceMemory, const VkAllocationCallbacks *);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkGetBufferMemoryRequirements)(VkDevice, VkBuffer, VkMemoryRequirements_host *);
#else
    void (*p_vkGetBufferMemoryRequirements)(VkDevice, VkBuffer, VkMemoryRequirements *);
#endif
    void (*p_vkGetDeviceMemoryCommitment)(VkDevice, VkDeviceMemory, VkDeviceSize *);
    void (*p_vkGetDeviceQueue)(VkDevice, uint32_t, uint32_t, VkQueue *);
    VkResult (*p_vkGetEventStatus)(VkDevice, VkEvent);
    VkResult (*p_vkGetFenceStatus)(VkDevice, VkFence);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkGetImageMemoryRequirements)(VkDevice, VkImage, VkMemoryRequirements_host *);
#else
    void (*p_vkGetImageMemoryRequirements)(VkDevice, VkImage, VkMemoryRequirements *);
#endif
    void (*p_vkGetImageSparseMemoryRequirements)(VkDevice, VkImage, uint32_t *, VkSparseImageMemoryRequirements *);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkGetImageSubresourceLayout)(VkDevice, VkImage, const VkImageSubresource *, VkSubresourceLayout_host *);
#else
    void (*p_vkGetImageSubresourceLayout)(VkDevice, VkImage, const VkImageSubresource *, VkSubresourceLayout *);
#endif
    VkResult (*p_vkGetPipelineCacheData)(VkDevice, VkPipelineCache, size_t *, void *);
    VkResult (*p_vkGetQueryPoolResults)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void *, VkDeviceSize, VkQueryResultFlags);
    void (*p_vkGetRenderAreaGranularity)(VkDevice, VkRenderPass, VkExtent2D *);
#if defined(USE_STRUCT_CONVERSION)
    VkResult (*p_vkInvalidateMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange_host *);
#else
    VkResult (*p_vkInvalidateMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange *);
#endif
    VkResult (*p_vkMapMemory)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void **);
    VkResult (*p_vkMergePipelineCaches)(VkDevice, VkPipelineCache, uint32_t, const VkPipelineCache *);
    VkResult (*p_vkQueueBindSparse)(VkQueue, uint32_t, const VkBindSparseInfo *, VkFence);
    VkResult (*p_vkQueueSubmit)(VkQueue, uint32_t, const VkSubmitInfo *, VkFence);
    VkResult (*p_vkQueueWaitIdle)(VkQueue);
    VkResult (*p_vkResetCommandBuffer)(VkCommandBuffer, VkCommandBufferResetFlags);
    VkResult (*p_vkResetCommandPool)(VkDevice, VkCommandPool, VkCommandPoolResetFlags);
    VkResult (*p_vkResetDescriptorPool)(VkDevice, VkDescriptorPool, VkDescriptorPoolResetFlags);
    VkResult (*p_vkResetEvent)(VkDevice, VkEvent);
    VkResult (*p_vkResetFences)(VkDevice, uint32_t, const VkFence *);
    VkResult (*p_vkSetEvent)(VkDevice, VkEvent);
    void (*p_vkUnmapMemory)(VkDevice, VkDeviceMemory);
#if defined(USE_STRUCT_CONVERSION)
    void (*p_vkUpdateDescriptorSets)(VkDevice, uint32_t, const VkWriteDescriptorSet_host *, uint32_t, const VkCopyDescriptorSet_host *);
#else
    void (*p_vkUpdateDescriptorSets)(VkDevice, uint32_t, const VkWriteDescriptorSet *, uint32_t, const VkCopyDescriptorSet *);
#endif
    VkResult (*p_vkWaitForFences)(VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t);
};

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

#define ALL_VK_DEVICE_FUNCS() \
    USE_VK_FUNC(vkAllocateCommandBuffers) \
    USE_VK_FUNC(vkAllocateDescriptorSets) \
    USE_VK_FUNC(vkAllocateMemory) \
    USE_VK_FUNC(vkBeginCommandBuffer) \
    USE_VK_FUNC(vkBindBufferMemory) \
    USE_VK_FUNC(vkBindImageMemory) \
    USE_VK_FUNC(vkCmdBeginQuery) \
    USE_VK_FUNC(vkCmdBeginRenderPass) \
    USE_VK_FUNC(vkCmdBindDescriptorSets) \
    USE_VK_FUNC(vkCmdBindIndexBuffer) \
    USE_VK_FUNC(vkCmdBindPipeline) \
    USE_VK_FUNC(vkCmdBindVertexBuffers) \
    USE_VK_FUNC(vkCmdBlitImage) \
    USE_VK_FUNC(vkCmdClearAttachments) \
    USE_VK_FUNC(vkCmdClearColorImage) \
    USE_VK_FUNC(vkCmdClearDepthStencilImage) \
    USE_VK_FUNC(vkCmdCopyBuffer) \
    USE_VK_FUNC(vkCmdCopyBufferToImage) \
    USE_VK_FUNC(vkCmdCopyImage) \
    USE_VK_FUNC(vkCmdCopyImageToBuffer) \
    USE_VK_FUNC(vkCmdCopyQueryPoolResults) \
    USE_VK_FUNC(vkCmdDispatch) \
    USE_VK_FUNC(vkCmdDispatchIndirect) \
    USE_VK_FUNC(vkCmdDraw) \
    USE_VK_FUNC(vkCmdDrawIndexed) \
    USE_VK_FUNC(vkCmdDrawIndexedIndirect) \
    USE_VK_FUNC(vkCmdDrawIndirect) \
    USE_VK_FUNC(vkCmdEndQuery) \
    USE_VK_FUNC(vkCmdEndRenderPass) \
    USE_VK_FUNC(vkCmdExecuteCommands) \
    USE_VK_FUNC(vkCmdFillBuffer) \
    USE_VK_FUNC(vkCmdNextSubpass) \
    USE_VK_FUNC(vkCmdPipelineBarrier) \
    USE_VK_FUNC(vkCmdPushConstants) \
    USE_VK_FUNC(vkCmdResetEvent) \
    USE_VK_FUNC(vkCmdResetQueryPool) \
    USE_VK_FUNC(vkCmdResolveImage) \
    USE_VK_FUNC(vkCmdSetBlendConstants) \
    USE_VK_FUNC(vkCmdSetDepthBias) \
    USE_VK_FUNC(vkCmdSetDepthBounds) \
    USE_VK_FUNC(vkCmdSetEvent) \
    USE_VK_FUNC(vkCmdSetLineWidth) \
    USE_VK_FUNC(vkCmdSetScissor) \
    USE_VK_FUNC(vkCmdSetStencilCompareMask) \
    USE_VK_FUNC(vkCmdSetStencilReference) \
    USE_VK_FUNC(vkCmdSetStencilWriteMask) \
    USE_VK_FUNC(vkCmdSetViewport) \
    USE_VK_FUNC(vkCmdUpdateBuffer) \
    USE_VK_FUNC(vkCmdWaitEvents) \
    USE_VK_FUNC(vkCmdWriteTimestamp) \
    USE_VK_FUNC(vkCreateBuffer) \
    USE_VK_FUNC(vkCreateBufferView) \
    USE_VK_FUNC(vkCreateCommandPool) \
    USE_VK_FUNC(vkCreateComputePipelines) \
    USE_VK_FUNC(vkCreateDescriptorPool) \
    USE_VK_FUNC(vkCreateDescriptorSetLayout) \
    USE_VK_FUNC(vkCreateEvent) \
    USE_VK_FUNC(vkCreateFence) \
    USE_VK_FUNC(vkCreateFramebuffer) \
    USE_VK_FUNC(vkCreateGraphicsPipelines) \
    USE_VK_FUNC(vkCreateImage) \
    USE_VK_FUNC(vkCreateImageView) \
    USE_VK_FUNC(vkCreatePipelineCache) \
    USE_VK_FUNC(vkCreatePipelineLayout) \
    USE_VK_FUNC(vkCreateQueryPool) \
    USE_VK_FUNC(vkCreateRenderPass) \
    USE_VK_FUNC(vkCreateSampler) \
    USE_VK_FUNC(vkCreateSemaphore) \
    USE_VK_FUNC(vkCreateShaderModule) \
    USE_VK_FUNC(vkDestroyBuffer) \
    USE_VK_FUNC(vkDestroyBufferView) \
    USE_VK_FUNC(vkDestroyCommandPool) \
    USE_VK_FUNC(vkDestroyDescriptorPool) \
    USE_VK_FUNC(vkDestroyDescriptorSetLayout) \
    USE_VK_FUNC(vkDestroyDevice) \
    USE_VK_FUNC(vkDestroyEvent) \
    USE_VK_FUNC(vkDestroyFence) \
    USE_VK_FUNC(vkDestroyFramebuffer) \
    USE_VK_FUNC(vkDestroyImage) \
    USE_VK_FUNC(vkDestroyImageView) \
    USE_VK_FUNC(vkDestroyPipeline) \
    USE_VK_FUNC(vkDestroyPipelineCache) \
    USE_VK_FUNC(vkDestroyPipelineLayout) \
    USE_VK_FUNC(vkDestroyQueryPool) \
    USE_VK_FUNC(vkDestroyRenderPass) \
    USE_VK_FUNC(vkDestroySampler) \
    USE_VK_FUNC(vkDestroySemaphore) \
    USE_VK_FUNC(vkDestroyShaderModule) \
    USE_VK_FUNC(vkDeviceWaitIdle) \
    USE_VK_FUNC(vkEndCommandBuffer) \
    USE_VK_FUNC(vkFlushMappedMemoryRanges) \
    USE_VK_FUNC(vkFreeCommandBuffers) \
    USE_VK_FUNC(vkFreeDescriptorSets) \
    USE_VK_FUNC(vkFreeMemory) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements) \
    USE_VK_FUNC(vkGetDeviceMemoryCommitment) \
    USE_VK_FUNC(vkGetDeviceQueue) \
    USE_VK_FUNC(vkGetEventStatus) \
    USE_VK_FUNC(vkGetFenceStatus) \
    USE_VK_FUNC(vkGetImageMemoryRequirements) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements) \
    USE_VK_FUNC(vkGetImageSubresourceLayout) \
    USE_VK_FUNC(vkGetPipelineCacheData) \
    USE_VK_FUNC(vkGetQueryPoolResults) \
    USE_VK_FUNC(vkGetRenderAreaGranularity) \
    USE_VK_FUNC(vkInvalidateMappedMemoryRanges) \
    USE_VK_FUNC(vkMapMemory) \
    USE_VK_FUNC(vkMergePipelineCaches) \
    USE_VK_FUNC(vkQueueBindSparse) \
    USE_VK_FUNC(vkQueueSubmit) \
    USE_VK_FUNC(vkQueueWaitIdle) \
    USE_VK_FUNC(vkResetCommandBuffer) \
    USE_VK_FUNC(vkResetCommandPool) \
    USE_VK_FUNC(vkResetDescriptorPool) \
    USE_VK_FUNC(vkResetEvent) \
    USE_VK_FUNC(vkResetFences) \
    USE_VK_FUNC(vkSetEvent) \
    USE_VK_FUNC(vkUnmapMemory) \
    USE_VK_FUNC(vkUpdateDescriptorSets) \
    USE_VK_FUNC(vkWaitForFences)

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
