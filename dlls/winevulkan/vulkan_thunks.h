/* Automatically generated from Vulkan vk.xml; DO NOT EDIT!
 *
 * This file is generated from Vulkan vk.xml file covered
 * by the following copyright and permission notice:
 *
 * Copyright 2015-2022 The Khronos Group Inc.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __WINE_VULKAN_THUNKS_H
#define __WINE_VULKAN_THUNKS_H

#define WINE_VK_VERSION VK_API_VERSION_1_3

/* Functions for which we have custom implementations outside of the thunks. */
VkResult wine_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers) DECLSPEC_HIDDEN;
VkResult wine_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool, void *client_ptr) DECLSPEC_HIDDEN;
VkResult wine_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult wine_vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback) DECLSPEC_HIDDEN;
VkResult wine_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger) DECLSPEC_HIDDEN;
VkResult wine_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice, void *client_ptr) DECLSPEC_HIDDEN;
VkResult wine_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult wine_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance, void *client_ptr) DECLSPEC_HIDDEN;
VkResult wine_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult wine_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult wine_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) DECLSPEC_HIDDEN;
void wine_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void wine_vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void wine_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void wine_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void wine_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
void wine_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) DECLSPEC_HIDDEN;
VkResult wine_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) DECLSPEC_HIDDEN;
VkResult wine_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties) DECLSPEC_HIDDEN;
VkResult wine_vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) DECLSPEC_HIDDEN;
VkResult wine_vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties) DECLSPEC_HIDDEN;
VkResult wine_vkEnumerateInstanceVersion(uint32_t *pApiVersion) DECLSPEC_HIDDEN;
VkResult wine_vkEnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties) DECLSPEC_HIDDEN;
VkResult wine_vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties) DECLSPEC_HIDDEN;
VkResult wine_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) DECLSPEC_HIDDEN;
void wine_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) DECLSPEC_HIDDEN;
VkResult wine_vkGetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation) DECLSPEC_HIDDEN;
PFN_vkVoidFunction wine_vkGetDeviceProcAddr(VkDevice device, const char *pName) DECLSPEC_HIDDEN;
void wine_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) DECLSPEC_HIDDEN;
void wine_vkGetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue) DECLSPEC_HIDDEN;
PFN_vkVoidFunction wine_vkGetInstanceProcAddr(VkInstance instance, const char *pName) DECLSPEC_HIDDEN;
VkResult wine_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainEXT *pTimeDomains) DECLSPEC_HIDDEN;
void wine_vkGetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties) DECLSPEC_HIDDEN;
void wine_vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties) DECLSPEC_HIDDEN;
void wine_vkGetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties) DECLSPEC_HIDDEN;
void wine_vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties) DECLSPEC_HIDDEN;
void wine_vkGetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties) DECLSPEC_HIDDEN;
void wine_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties) DECLSPEC_HIDDEN;
VkResult wine_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties) DECLSPEC_HIDDEN;
VkResult wine_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties) DECLSPEC_HIDDEN;
VkResult wine_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities) DECLSPEC_HIDDEN;
VkResult wine_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) DECLSPEC_HIDDEN;

/* Private thunks */
VkResult thunk_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult thunk_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult thunk_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult thunk_vkCreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) DECLSPEC_HIDDEN;
VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties) DECLSPEC_HIDDEN;
VkResult thunk_vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties) DECLSPEC_HIDDEN;
VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities) DECLSPEC_HIDDEN;
VkResult thunk_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) DECLSPEC_HIDDEN;

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAcquireNextImageInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkSwapchainKHR swapchain;
    uint64_t timeout;
    VkSemaphore semaphore;
    VkFence fence;
    uint32_t deviceMask;
} VkAcquireNextImageInfoKHR_host;
#else
typedef VkAcquireNextImageInfoKHR VkAcquireNextImageInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAcquireProfilingLockInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkAcquireProfilingLockFlagsKHR flags;
    uint64_t timeout;
} VkAcquireProfilingLockInfoKHR_host;
#else
typedef VkAcquireProfilingLockInfoKHR VkAcquireProfilingLockInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCommandBufferAllocateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkCommandPool commandPool;
    VkCommandBufferLevel level;
    uint32_t commandBufferCount;
} VkCommandBufferAllocateInfo_host;
#else
typedef VkCommandBufferAllocateInfo VkCommandBufferAllocateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDescriptorSetAllocateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDescriptorPool descriptorPool;
    uint32_t descriptorSetCount;
    const VkDescriptorSetLayout *pSetLayouts;
} VkDescriptorSetAllocateInfo_host;
#else
typedef VkDescriptorSetAllocateInfo VkDescriptorSetAllocateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMemoryAllocateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize allocationSize;
    uint32_t memoryTypeIndex;
} VkMemoryAllocateInfo_host;
#else
typedef VkMemoryAllocateInfo VkMemoryAllocateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkCommandBufferInheritanceInfo VkCommandBufferInheritanceInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCommandBufferBeginInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkCommandBufferUsageFlags flags;
    const VkCommandBufferInheritanceInfo_host *pInheritanceInfo;
} VkCommandBufferBeginInfo_host;
#else
typedef VkCommandBufferBeginInfo VkCommandBufferBeginInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBindAccelerationStructureMemoryInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureNV accelerationStructure;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
    uint32_t deviceIndexCount;
    const uint32_t *pDeviceIndices;
} VkBindAccelerationStructureMemoryInfoNV_host;
#else
typedef VkBindAccelerationStructureMemoryInfoNV VkBindAccelerationStructureMemoryInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBindBufferMemoryInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
} VkBindBufferMemoryInfo_host;
typedef VkBindBufferMemoryInfo VkBindBufferMemoryInfoKHR;
#else
typedef VkBindBufferMemoryInfo VkBindBufferMemoryInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBindImageMemoryInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage image;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
} VkBindImageMemoryInfo_host;
typedef VkBindImageMemoryInfo VkBindImageMemoryInfoKHR;
#else
typedef VkBindImageMemoryInfo VkBindImageMemoryInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureBuildGeometryInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureTypeKHR type;
    VkBuildAccelerationStructureFlagsKHR flags;
    VkBuildAccelerationStructureModeKHR mode;
    VkAccelerationStructureKHR srcAccelerationStructure;
    VkAccelerationStructureKHR dstAccelerationStructure;
    uint32_t geometryCount;
    const VkAccelerationStructureGeometryKHR *pGeometries;
    const VkAccelerationStructureGeometryKHR * const*ppGeometries;
    VkDeviceOrHostAddressKHR scratchData;
} VkAccelerationStructureBuildGeometryInfoKHR_host;
#else
typedef VkAccelerationStructureBuildGeometryInfoKHR VkAccelerationStructureBuildGeometryInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMicromapBuildInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkMicromapTypeEXT type;
    VkBuildMicromapFlagsEXT flags;
    VkBuildMicromapModeEXT mode;
    VkMicromapEXT dstMicromap;
    uint32_t usageCountsCount;
    const VkMicromapUsageEXT *pUsageCounts;
    const VkMicromapUsageEXT * const*ppUsageCounts;
    VkDeviceOrHostAddressConstKHR data;
    VkDeviceOrHostAddressKHR scratchData;
    VkDeviceOrHostAddressConstKHR triangleArray;
    VkDeviceSize triangleArrayStride;
} VkMicromapBuildInfoEXT_host;
#else
typedef VkMicromapBuildInfoEXT VkMicromapBuildInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkConditionalRenderingBeginInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkConditionalRenderingFlagsEXT flags;
} VkConditionalRenderingBeginInfoEXT_host;
#else
typedef VkConditionalRenderingBeginInfoEXT VkConditionalRenderingBeginInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkRenderPassBeginInfo VkRenderPassBeginInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkRenderingAttachmentInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkImageView imageView;
    VkImageLayout imageLayout;
    VkResolveModeFlagBits resolveMode;
    VkImageView resolveImageView;
    VkImageLayout resolveImageLayout;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
    VkClearValue clearValue;
} VkRenderingAttachmentInfo_host;
typedef VkRenderingAttachmentInfo VkRenderingAttachmentInfoKHR;
#else
typedef VkRenderingAttachmentInfo VkRenderingAttachmentInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkRenderingInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkRenderingFlags flags;
    VkRect2D renderArea;
    uint32_t layerCount;
    uint32_t viewMask;
    uint32_t colorAttachmentCount;
    const VkRenderingAttachmentInfo_host *pColorAttachments;
    const VkRenderingAttachmentInfo_host *pDepthAttachment;
    const VkRenderingAttachmentInfo_host *pStencilAttachment;
} VkRenderingInfo_host;
typedef VkRenderingInfo VkRenderingInfoKHR;
#else
typedef VkRenderingInfo VkRenderingInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBlitImageInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageBlit2 *pRegions;
    VkFilter filter;
} VkBlitImageInfo2_host;
typedef VkBlitImageInfo2 VkBlitImageInfo2KHR;
#else
typedef VkBlitImageInfo2 VkBlitImageInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkGeometryTrianglesNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer vertexData;
    VkDeviceSize vertexOffset;
    uint32_t vertexCount;
    VkDeviceSize vertexStride;
    VkFormat vertexFormat;
    VkBuffer indexData;
    VkDeviceSize indexOffset;
    uint32_t indexCount;
    VkIndexType indexType;
    VkBuffer transformData;
    VkDeviceSize transformOffset;
} VkGeometryTrianglesNV_host;
#else
typedef VkGeometryTrianglesNV VkGeometryTrianglesNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkGeometryAABBNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer aabbData;
    uint32_t numAABBs;
    uint32_t stride;
    VkDeviceSize offset;
} VkGeometryAABBNV_host;
#else
typedef VkGeometryAABBNV VkGeometryAABBNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkGeometryDataNV_host
{
    VkGeometryTrianglesNV_host triangles;
    VkGeometryAABBNV_host aabbs;
} VkGeometryDataNV_host;
#else
typedef VkGeometryDataNV VkGeometryDataNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkGeometryNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkGeometryTypeKHR geometryType;
    VkGeometryDataNV_host geometry;
    VkGeometryFlagsKHR flags;
} VkGeometryNV_host;
#else
typedef VkGeometryNV VkGeometryNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureTypeNV type;
    VkBuildAccelerationStructureFlagsNV flags;
    uint32_t instanceCount;
    uint32_t geometryCount;
    const VkGeometryNV_host *pGeometries;
} VkAccelerationStructureInfoNV_host;
#else
typedef VkAccelerationStructureInfoNV VkAccelerationStructureInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyAccelerationStructureInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureKHR src;
    VkAccelerationStructureKHR dst;
    VkCopyAccelerationStructureModeKHR mode;
} VkCopyAccelerationStructureInfoKHR_host;
#else
typedef VkCopyAccelerationStructureInfoKHR VkCopyAccelerationStructureInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyAccelerationStructureToMemoryInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureKHR src;
    VkDeviceOrHostAddressKHR dst;
    VkCopyAccelerationStructureModeKHR mode;
} VkCopyAccelerationStructureToMemoryInfoKHR_host;
#else
typedef VkCopyAccelerationStructureToMemoryInfoKHR VkCopyAccelerationStructureToMemoryInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferCopy_host
{
    VkDeviceSize srcOffset;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
} VkBufferCopy_host;
#else
typedef VkBufferCopy VkBufferCopy_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferCopy2_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize srcOffset;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
} VkBufferCopy2_host;
typedef VkBufferCopy2 VkBufferCopy2KHR;
#else
typedef VkBufferCopy2 VkBufferCopy2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyBufferInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer srcBuffer;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferCopy2_host *pRegions;
} VkCopyBufferInfo2_host;
typedef VkCopyBufferInfo2 VkCopyBufferInfo2KHR;
#else
typedef VkCopyBufferInfo2 VkCopyBufferInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferImageCopy_host
{
    VkDeviceSize bufferOffset;
    uint32_t bufferRowLength;
    uint32_t bufferImageHeight;
    VkImageSubresourceLayers imageSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VkBufferImageCopy_host;
#else
typedef VkBufferImageCopy VkBufferImageCopy_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferImageCopy2_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize bufferOffset;
    uint32_t bufferRowLength;
    uint32_t bufferImageHeight;
    VkImageSubresourceLayers imageSubresource;
    VkOffset3D imageOffset;
    VkExtent3D imageExtent;
} VkBufferImageCopy2_host;
typedef VkBufferImageCopy2 VkBufferImageCopy2KHR;
#else
typedef VkBufferImageCopy2 VkBufferImageCopy2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyBufferToImageInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer srcBuffer;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkBufferImageCopy2_host *pRegions;
} VkCopyBufferToImageInfo2_host;
typedef VkCopyBufferToImageInfo2 VkCopyBufferToImageInfo2KHR;
#else
typedef VkCopyBufferToImageInfo2 VkCopyBufferToImageInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyImageInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageCopy2 *pRegions;
} VkCopyImageInfo2_host;
typedef VkCopyImageInfo2 VkCopyImageInfo2KHR;
#else
typedef VkCopyImageInfo2 VkCopyImageInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyImageToBufferInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferImageCopy2_host *pRegions;
} VkCopyImageToBufferInfo2_host;
typedef VkCopyImageToBufferInfo2 VkCopyImageToBufferInfo2KHR;
#else
typedef VkCopyImageToBufferInfo2 VkCopyImageToBufferInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyMemoryToAccelerationStructureInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceOrHostAddressConstKHR src;
    VkAccelerationStructureKHR dst;
    VkCopyAccelerationStructureModeKHR mode;
} VkCopyMemoryToAccelerationStructureInfoKHR_host;
#else
typedef VkCopyMemoryToAccelerationStructureInfoKHR VkCopyMemoryToAccelerationStructureInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyMemoryToMicromapInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceOrHostAddressConstKHR src;
    VkMicromapEXT dst;
    VkCopyMicromapModeEXT mode;
} VkCopyMemoryToMicromapInfoEXT_host;
#else
typedef VkCopyMemoryToMicromapInfoEXT VkCopyMemoryToMicromapInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyMicromapInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkMicromapEXT src;
    VkMicromapEXT dst;
    VkCopyMicromapModeEXT mode;
} VkCopyMicromapInfoEXT_host;
#else
typedef VkCopyMicromapInfoEXT VkCopyMicromapInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCopyMicromapToMemoryInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkMicromapEXT src;
    VkDeviceOrHostAddressKHR dst;
    VkCopyMicromapModeEXT mode;
} VkCopyMicromapToMemoryInfoEXT_host;
#else
typedef VkCopyMicromapToMemoryInfoEXT VkCopyMicromapToMemoryInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCuLaunchInfoNVX_host
{
    VkStructureType sType;
    const void *pNext;
    VkCuFunctionNVX function;
    uint32_t gridDimX;
    uint32_t gridDimY;
    uint32_t gridDimZ;
    uint32_t blockDimX;
    uint32_t blockDimY;
    uint32_t blockDimZ;
    uint32_t sharedMemBytes;
    size_t paramCount;
    const void * const *pParams;
    size_t extraCount;
    const void * const *pExtras;
} VkCuLaunchInfoNVX_host;
#else
typedef VkCuLaunchInfoNVX VkCuLaunchInfoNVX_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkIndirectCommandsStreamNV_host
{
    VkBuffer buffer;
    VkDeviceSize offset;
} VkIndirectCommandsStreamNV_host;
#else
typedef VkIndirectCommandsStreamNV VkIndirectCommandsStreamNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkGeneratedCommandsInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline pipeline;
    VkIndirectCommandsLayoutNV indirectCommandsLayout;
    uint32_t streamCount;
    const VkIndirectCommandsStreamNV_host *pStreams;
    uint32_t sequencesCount;
    VkBuffer preprocessBuffer;
    VkDeviceSize preprocessOffset;
    VkDeviceSize preprocessSize;
    VkBuffer sequencesCountBuffer;
    VkDeviceSize sequencesCountOffset;
    VkBuffer sequencesIndexBuffer;
    VkDeviceSize sequencesIndexOffset;
} VkGeneratedCommandsInfoNV_host;
#else
typedef VkGeneratedCommandsInfoNV VkGeneratedCommandsInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkBufferMemoryBarrier VkBufferMemoryBarrier_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkImageMemoryBarrier VkImageMemoryBarrier_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferMemoryBarrier2_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
} VkBufferMemoryBarrier2_host;
typedef VkBufferMemoryBarrier2 VkBufferMemoryBarrier2KHR;
#else
typedef VkBufferMemoryBarrier2 VkBufferMemoryBarrier2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageMemoryBarrier2_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    VkImage image;
    VkImageSubresourceRange subresourceRange;
} VkImageMemoryBarrier2_host;
typedef VkImageMemoryBarrier2 VkImageMemoryBarrier2KHR;
#else
typedef VkImageMemoryBarrier2 VkImageMemoryBarrier2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDependencyInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDependencyFlags dependencyFlags;
    uint32_t memoryBarrierCount;
    const VkMemoryBarrier2 *pMemoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier2_host *pBufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    const VkImageMemoryBarrier2_host *pImageMemoryBarriers;
} VkDependencyInfo_host;
typedef VkDependencyInfo VkDependencyInfoKHR;
#else
typedef VkDependencyInfo VkDependencyInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDescriptorImageInfo_host
{
    VkSampler sampler;
    VkImageView imageView;
    VkImageLayout imageLayout;
} VkDescriptorImageInfo_host;
#else
typedef VkDescriptorImageInfo VkDescriptorImageInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDescriptorBufferInfo_host
{
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
} VkDescriptorBufferInfo_host;
#else
typedef VkDescriptorBufferInfo VkDescriptorBufferInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkWriteDescriptorSet VkWriteDescriptorSet_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkResolveImageInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageResolve2 *pRegions;
} VkResolveImageInfo2_host;
typedef VkResolveImageInfo2 VkResolveImageInfo2KHR;
#else
typedef VkResolveImageInfo2 VkResolveImageInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPerformanceMarkerInfoINTEL_host
{
    VkStructureType sType;
    const void *pNext;
    uint64_t marker;
} VkPerformanceMarkerInfoINTEL_host;
#else
typedef VkPerformanceMarkerInfoINTEL VkPerformanceMarkerInfoINTEL_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPerformanceOverrideInfoINTEL_host
{
    VkStructureType sType;
    const void *pNext;
    VkPerformanceOverrideTypeINTEL type;
    VkBool32 enable;
    uint64_t parameter;
} VkPerformanceOverrideInfoINTEL_host;
#else
typedef VkPerformanceOverrideInfoINTEL VkPerformanceOverrideInfoINTEL_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkStridedDeviceAddressRegionKHR_host
{
    VkDeviceAddress deviceAddress;
    VkDeviceSize stride;
    VkDeviceSize size;
} VkStridedDeviceAddressRegionKHR_host;
#else
typedef VkStridedDeviceAddressRegionKHR VkStridedDeviceAddressRegionKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureCreateInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureCreateFlagsKHR createFlags;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkAccelerationStructureTypeKHR type;
    VkDeviceAddress deviceAddress;
} VkAccelerationStructureCreateInfoKHR_host;
#else
typedef VkAccelerationStructureCreateInfoKHR VkAccelerationStructureCreateInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureCreateInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize compactedSize;
    VkAccelerationStructureInfoNV_host info;
} VkAccelerationStructureCreateInfoNV_host;
#else
typedef VkAccelerationStructureCreateInfoNV VkAccelerationStructureCreateInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkBufferCreateInfo VkBufferCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkBufferViewCreateInfo VkBufferViewCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkPipelineShaderStageCreateInfo VkPipelineShaderStageCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkComputePipelineCreateInfo VkComputePipelineCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCuFunctionCreateInfoNVX_host
{
    VkStructureType sType;
    const void *pNext;
    VkCuModuleNVX module;
    const char *pName;
} VkCuFunctionCreateInfoNVX_host;
#else
typedef VkCuFunctionCreateInfoNVX VkCuFunctionCreateInfoNVX_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDescriptorUpdateTemplateCreateInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDescriptorUpdateTemplateCreateFlags flags;
    uint32_t descriptorUpdateEntryCount;
    const VkDescriptorUpdateTemplateEntry *pDescriptorUpdateEntries;
    VkDescriptorUpdateTemplateType templateType;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipelineLayout pipelineLayout;
    uint32_t set;
} VkDescriptorUpdateTemplateCreateInfo_host;
typedef VkDescriptorUpdateTemplateCreateInfo VkDescriptorUpdateTemplateCreateInfoKHR;
#else
typedef VkDescriptorUpdateTemplateCreateInfo VkDescriptorUpdateTemplateCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkFramebufferCreateInfo VkFramebufferCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkGraphicsPipelineCreateInfo VkGraphicsPipelineCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkImageViewCreateInfo VkImageViewCreateInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkIndirectCommandsLayoutTokenNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkIndirectCommandsTokenTypeNV tokenType;
    uint32_t stream;
    uint32_t offset;
    uint32_t vertexBindingUnit;
    VkBool32 vertexDynamicStride;
    VkPipelineLayout pushconstantPipelineLayout;
    VkShaderStageFlags pushconstantShaderStageFlags;
    uint32_t pushconstantOffset;
    uint32_t pushconstantSize;
    VkIndirectStateFlagsNV indirectStateFlags;
    uint32_t indexTypeCount;
    const VkIndexType *pIndexTypes;
    const uint32_t *pIndexTypeValues;
} VkIndirectCommandsLayoutTokenNV_host;
#else
typedef VkIndirectCommandsLayoutTokenNV VkIndirectCommandsLayoutTokenNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkIndirectCommandsLayoutCreateInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkIndirectCommandsLayoutUsageFlagsNV flags;
    VkPipelineBindPoint pipelineBindPoint;
    uint32_t tokenCount;
    const VkIndirectCommandsLayoutTokenNV_host *pTokens;
    uint32_t streamCount;
    const uint32_t *pStreamStrides;
} VkIndirectCommandsLayoutCreateInfoNV_host;
#else
typedef VkIndirectCommandsLayoutCreateInfoNV VkIndirectCommandsLayoutCreateInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMicromapCreateInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkMicromapCreateFlagsEXT createFlags;
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkMicromapTypeEXT type;
    VkDeviceAddress deviceAddress;
} VkMicromapCreateInfoEXT_host;
#else
typedef VkMicromapCreateInfoEXT VkMicromapCreateInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkRayTracingPipelineCreateInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineCreateFlags flags;
    uint32_t stageCount;
    const VkPipelineShaderStageCreateInfo_host *pStages;
    uint32_t groupCount;
    const VkRayTracingShaderGroupCreateInfoKHR *pGroups;
    uint32_t maxPipelineRayRecursionDepth;
    const VkPipelineLibraryCreateInfoKHR *pLibraryInfo;
    const VkRayTracingPipelineInterfaceCreateInfoKHR *pLibraryInterface;
    const VkPipelineDynamicStateCreateInfo *pDynamicState;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
} VkRayTracingPipelineCreateInfoKHR_host;
#else
typedef VkRayTracingPipelineCreateInfoKHR VkRayTracingPipelineCreateInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkRayTracingPipelineCreateInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineCreateFlags flags;
    uint32_t stageCount;
    const VkPipelineShaderStageCreateInfo_host *pStages;
    uint32_t groupCount;
    const VkRayTracingShaderGroupCreateInfoNV *pGroups;
    uint32_t maxRecursionDepth;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    int32_t basePipelineIndex;
} VkRayTracingPipelineCreateInfoNV_host;
#else
typedef VkRayTracingPipelineCreateInfoNV VkRayTracingPipelineCreateInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkSwapchainCreateInfoKHR VkSwapchainCreateInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDebugMarkerObjectNameInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkDebugReportObjectTypeEXT objectType;
    uint64_t object;
    const char *pObjectName;
} VkDebugMarkerObjectNameInfoEXT_host;
#else
typedef VkDebugMarkerObjectNameInfoEXT VkDebugMarkerObjectNameInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDebugMarkerObjectTagInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkDebugReportObjectTypeEXT objectType;
    uint64_t object;
    uint64_t tagName;
    size_t tagSize;
    const void *pTag;
} VkDebugMarkerObjectTagInfoEXT_host;
#else
typedef VkDebugMarkerObjectTagInfoEXT VkDebugMarkerObjectTagInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPhysicalDeviceGroupProperties_host
{
    VkStructureType sType;
    void *pNext;
    uint32_t physicalDeviceCount;
    VkPhysicalDevice physicalDevices[VK_MAX_DEVICE_GROUP_SIZE];
    VkBool32 subsetAllocation;
} VkPhysicalDeviceGroupProperties_host;
typedef VkPhysicalDeviceGroupProperties VkPhysicalDeviceGroupPropertiesKHR;
#else
typedef VkPhysicalDeviceGroupProperties VkPhysicalDeviceGroupProperties_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMappedMemoryRange_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
} VkMappedMemoryRange_host;
#else
typedef VkMappedMemoryRange VkMappedMemoryRange_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureBuildSizesInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize accelerationStructureSize;
    VkDeviceSize updateScratchSize;
    VkDeviceSize buildScratchSize;
} VkAccelerationStructureBuildSizesInfoKHR_host;
#else
typedef VkAccelerationStructureBuildSizesInfoKHR VkAccelerationStructureBuildSizesInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureDeviceAddressInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureKHR accelerationStructure;
} VkAccelerationStructureDeviceAddressInfoKHR_host;
#else
typedef VkAccelerationStructureDeviceAddressInfoKHR VkAccelerationStructureDeviceAddressInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkAccelerationStructureMemoryRequirementsInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkAccelerationStructureMemoryRequirementsTypeNV type;
    VkAccelerationStructureNV accelerationStructure;
} VkAccelerationStructureMemoryRequirementsInfoNV_host;
#else
typedef VkAccelerationStructureMemoryRequirementsInfoNV VkAccelerationStructureMemoryRequirementsInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMemoryRequirements_host
{
    VkDeviceSize size;
    VkDeviceSize alignment;
    uint32_t memoryTypeBits;
} VkMemoryRequirements_host;
#else
typedef VkMemoryRequirements VkMemoryRequirements_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMemoryRequirements2KHR_host
{
    VkStructureType sType;
    void *pNext;
    VkMemoryRequirements_host memoryRequirements;
} VkMemoryRequirements2KHR_host;
#else
typedef VkMemoryRequirements2KHR VkMemoryRequirements2KHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferDeviceAddressInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer buffer;
} VkBufferDeviceAddressInfo_host;
typedef VkBufferDeviceAddressInfo VkBufferDeviceAddressInfoKHR;
typedef VkBufferDeviceAddressInfo VkBufferDeviceAddressInfoEXT;
#else
typedef VkBufferDeviceAddressInfo VkBufferDeviceAddressInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBufferMemoryRequirementsInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkBuffer buffer;
} VkBufferMemoryRequirementsInfo2_host;
typedef VkBufferMemoryRequirementsInfo2 VkBufferMemoryRequirementsInfo2KHR;
#else
typedef VkBufferMemoryRequirementsInfo2 VkBufferMemoryRequirementsInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMemoryRequirements2_host
{
    VkStructureType sType;
    void *pNext;
    VkMemoryRequirements_host memoryRequirements;
} VkMemoryRequirements2_host;
typedef VkMemoryRequirements2 VkMemoryRequirements2KHR;
#else
typedef VkMemoryRequirements2 VkMemoryRequirements2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDescriptorSetBindingReferenceVALVE_host
{
    VkStructureType sType;
    const void *pNext;
    VkDescriptorSetLayout descriptorSetLayout;
    uint32_t binding;
} VkDescriptorSetBindingReferenceVALVE_host;
#else
typedef VkDescriptorSetBindingReferenceVALVE VkDescriptorSetBindingReferenceVALVE_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDeviceBufferMemoryRequirements_host
{
    VkStructureType sType;
    const void *pNext;
    const VkBufferCreateInfo_host *pCreateInfo;
} VkDeviceBufferMemoryRequirements_host;
typedef VkDeviceBufferMemoryRequirements VkDeviceBufferMemoryRequirementsKHR;
#else
typedef VkDeviceBufferMemoryRequirements VkDeviceBufferMemoryRequirements_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDeviceFaultCountsEXT_host
{
    VkStructureType sType;
    void *pNext;
    uint32_t addressInfoCount;
    uint32_t vendorInfoCount;
    VkDeviceSize vendorBinarySize;
} VkDeviceFaultCountsEXT_host;
#else
typedef VkDeviceFaultCountsEXT VkDeviceFaultCountsEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDeviceFaultAddressInfoEXT_host
{
    VkDeviceFaultAddressTypeEXT addressType;
    VkDeviceAddress reportedAddress;
    VkDeviceSize addressPrecision;
} VkDeviceFaultAddressInfoEXT_host;
#else
typedef VkDeviceFaultAddressInfoEXT VkDeviceFaultAddressInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDeviceFaultVendorInfoEXT_host
{
    char description[VK_MAX_DESCRIPTION_SIZE];
    uint64_t vendorFaultCode;
    uint64_t vendorFaultData;
} VkDeviceFaultVendorInfoEXT_host;
#else
typedef VkDeviceFaultVendorInfoEXT VkDeviceFaultVendorInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDeviceFaultInfoEXT_host
{
    VkStructureType sType;
    void *pNext;
    char description[VK_MAX_DESCRIPTION_SIZE];
    VkDeviceFaultAddressInfoEXT_host *pAddressInfos;
    VkDeviceFaultVendorInfoEXT_host *pVendorInfos;
    void *pVendorBinaryData;
} VkDeviceFaultInfoEXT_host;
#else
typedef VkDeviceFaultInfoEXT VkDeviceFaultInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDeviceMemoryOpaqueCaptureAddressInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceMemory memory;
} VkDeviceMemoryOpaqueCaptureAddressInfo_host;
typedef VkDeviceMemoryOpaqueCaptureAddressInfo VkDeviceMemoryOpaqueCaptureAddressInfoKHR;
#else
typedef VkDeviceMemoryOpaqueCaptureAddressInfo VkDeviceMemoryOpaqueCaptureAddressInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkGeneratedCommandsMemoryRequirementsInfoNV_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline pipeline;
    VkIndirectCommandsLayoutNV indirectCommandsLayout;
    uint32_t maxSequencesCount;
} VkGeneratedCommandsMemoryRequirementsInfoNV_host;
#else
typedef VkGeneratedCommandsMemoryRequirementsInfoNV VkGeneratedCommandsMemoryRequirementsInfoNV_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageMemoryRequirementsInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage image;
} VkImageMemoryRequirementsInfo2_host;
typedef VkImageMemoryRequirementsInfo2 VkImageMemoryRequirementsInfo2KHR;
#else
typedef VkImageMemoryRequirementsInfo2 VkImageMemoryRequirementsInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageSparseMemoryRequirementsInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkImage image;
} VkImageSparseMemoryRequirementsInfo2_host;
typedef VkImageSparseMemoryRequirementsInfo2 VkImageSparseMemoryRequirementsInfo2KHR;
#else
typedef VkImageSparseMemoryRequirementsInfo2 VkImageSparseMemoryRequirementsInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSubresourceLayout_host
{
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceSize rowPitch;
    VkDeviceSize arrayPitch;
    VkDeviceSize depthPitch;
} VkSubresourceLayout_host;
#else
typedef VkSubresourceLayout VkSubresourceLayout_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSubresourceLayout2EXT_host
{
    VkStructureType sType;
    void *pNext;
    VkSubresourceLayout_host subresourceLayout;
} VkSubresourceLayout2EXT_host;
#else
typedef VkSubresourceLayout2EXT VkSubresourceLayout2EXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageViewAddressPropertiesNVX_host
{
    VkStructureType sType;
    void *pNext;
    VkDeviceAddress deviceAddress;
    VkDeviceSize size;
} VkImageViewAddressPropertiesNVX_host;
#else
typedef VkImageViewAddressPropertiesNVX VkImageViewAddressPropertiesNVX_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageViewHandleInfoNVX_host
{
    VkStructureType sType;
    const void *pNext;
    VkImageView imageView;
    VkDescriptorType descriptorType;
    VkSampler sampler;
} VkImageViewHandleInfoNVX_host;
#else
typedef VkImageViewHandleInfoNVX VkImageViewHandleInfoNVX_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMemoryGetWin32HandleInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceMemory memory;
    VkExternalMemoryHandleTypeFlagBits handleType;
} VkMemoryGetWin32HandleInfoKHR_host;
#else
typedef VkMemoryGetWin32HandleInfoKHR VkMemoryGetWin32HandleInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMicromapBuildSizesInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkDeviceSize micromapSize;
    VkDeviceSize buildScratchSize;
    VkBool32 discardable;
} VkMicromapBuildSizesInfoEXT_host;
#else
typedef VkMicromapBuildSizesInfoEXT VkMicromapBuildSizesInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageFormatProperties_host
{
    VkExtent3D maxExtent;
    uint32_t maxMipLevels;
    uint32_t maxArrayLayers;
    VkSampleCountFlags sampleCounts;
    VkDeviceSize maxResourceSize;
} VkImageFormatProperties_host;
#else
typedef VkImageFormatProperties VkImageFormatProperties_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkImageFormatProperties2_host
{
    VkStructureType sType;
    void *pNext;
    VkImageFormatProperties_host imageFormatProperties;
} VkImageFormatProperties2_host;
typedef VkImageFormatProperties2 VkImageFormatProperties2KHR;
#else
typedef VkImageFormatProperties2 VkImageFormatProperties2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkMemoryHeap_host
{
    VkDeviceSize size;
    VkMemoryHeapFlags flags;
} VkMemoryHeap_host;
#else
typedef VkMemoryHeap VkMemoryHeap_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPhysicalDeviceMemoryProperties_host
{
    uint32_t memoryTypeCount;
    VkMemoryType memoryTypes[VK_MAX_MEMORY_TYPES];
    uint32_t memoryHeapCount;
    VkMemoryHeap_host memoryHeaps[VK_MAX_MEMORY_HEAPS];
} VkPhysicalDeviceMemoryProperties_host;
#else
typedef VkPhysicalDeviceMemoryProperties VkPhysicalDeviceMemoryProperties_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPhysicalDeviceMemoryProperties2_host
{
    VkStructureType sType;
    void *pNext;
    VkPhysicalDeviceMemoryProperties_host memoryProperties;
} VkPhysicalDeviceMemoryProperties2_host;
typedef VkPhysicalDeviceMemoryProperties2 VkPhysicalDeviceMemoryProperties2KHR;
#else
typedef VkPhysicalDeviceMemoryProperties2 VkPhysicalDeviceMemoryProperties2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkPhysicalDeviceLimits VkPhysicalDeviceLimits_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkPhysicalDeviceProperties VkPhysicalDeviceProperties_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPhysicalDeviceProperties2_host
{
    VkStructureType sType;
    void *pNext;
    VkPhysicalDeviceProperties_host properties;
} VkPhysicalDeviceProperties2_host;
typedef VkPhysicalDeviceProperties2 VkPhysicalDeviceProperties2KHR;
#else
typedef VkPhysicalDeviceProperties2 VkPhysicalDeviceProperties2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPhysicalDeviceSurfaceInfo2KHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkSurfaceKHR surface;
} VkPhysicalDeviceSurfaceInfo2KHR_host;
#else
typedef VkPhysicalDeviceSurfaceInfo2KHR VkPhysicalDeviceSurfaceInfo2KHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPipelineExecutableInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipeline pipeline;
    uint32_t executableIndex;
} VkPipelineExecutableInfoKHR_host;
#else
typedef VkPipelineExecutableInfoKHR VkPipelineExecutableInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPipelineInfoKHR_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipeline pipeline;
} VkPipelineInfoKHR_host;
typedef VkPipelineInfoKHR VkPipelineInfoEXT;
#else
typedef VkPipelineInfoKHR VkPipelineInfoKHR_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkPipelineInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkPipeline pipeline;
} VkPipelineInfoEXT_host;
#else
typedef VkPipelineInfoEXT VkPipelineInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSparseMemoryBind_host
{
    VkDeviceSize resourceOffset;
    VkDeviceSize size;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
    VkSparseMemoryBindFlags flags;
} VkSparseMemoryBind_host;
#else
typedef VkSparseMemoryBind VkSparseMemoryBind_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSparseBufferMemoryBindInfo_host
{
    VkBuffer buffer;
    uint32_t bindCount;
    const VkSparseMemoryBind_host *pBinds;
} VkSparseBufferMemoryBindInfo_host;
#else
typedef VkSparseBufferMemoryBindInfo VkSparseBufferMemoryBindInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSparseImageOpaqueMemoryBindInfo_host
{
    VkImage image;
    uint32_t bindCount;
    const VkSparseMemoryBind_host *pBinds;
} VkSparseImageOpaqueMemoryBindInfo_host;
#else
typedef VkSparseImageOpaqueMemoryBindInfo VkSparseImageOpaqueMemoryBindInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSparseImageMemoryBind_host
{
    VkImageSubresource subresource;
    VkOffset3D offset;
    VkExtent3D extent;
    VkDeviceMemory memory;
    VkDeviceSize memoryOffset;
    VkSparseMemoryBindFlags flags;
} VkSparseImageMemoryBind_host;
#else
typedef VkSparseImageMemoryBind VkSparseImageMemoryBind_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSparseImageMemoryBindInfo_host
{
    VkImage image;
    uint32_t bindCount;
    const VkSparseImageMemoryBind_host *pBinds;
} VkSparseImageMemoryBindInfo_host;
#else
typedef VkSparseImageMemoryBindInfo VkSparseImageMemoryBindInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkBindSparseInfo_host
{
    VkStructureType sType;
    const void *pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore *pWaitSemaphores;
    uint32_t bufferBindCount;
    const VkSparseBufferMemoryBindInfo_host *pBufferBinds;
    uint32_t imageOpaqueBindCount;
    const VkSparseImageOpaqueMemoryBindInfo_host *pImageOpaqueBinds;
    uint32_t imageBindCount;
    const VkSparseImageMemoryBindInfo_host *pImageBinds;
    uint32_t signalSemaphoreCount;
    const VkSemaphore *pSignalSemaphores;
} VkBindSparseInfo_host;
#else
typedef VkBindSparseInfo VkBindSparseInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSubmitInfo_host
{
    VkStructureType sType;
    const void *pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore *pWaitSemaphores;
    const VkPipelineStageFlags *pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer *pCommandBuffers;
    uint32_t signalSemaphoreCount;
    const VkSemaphore *pSignalSemaphores;
} VkSubmitInfo_host;
#else
typedef VkSubmitInfo VkSubmitInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSemaphoreSubmitInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkSemaphore semaphore;
    uint64_t value;
    VkPipelineStageFlags2 stageMask;
    uint32_t deviceIndex;
} VkSemaphoreSubmitInfo_host;
typedef VkSemaphoreSubmitInfo VkSemaphoreSubmitInfoKHR;
#else
typedef VkSemaphoreSubmitInfo VkSemaphoreSubmitInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkCommandBufferSubmitInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkCommandBuffer commandBuffer;
    uint32_t deviceMask;
} VkCommandBufferSubmitInfo_host;
typedef VkCommandBufferSubmitInfo VkCommandBufferSubmitInfoKHR;
#else
typedef VkCommandBufferSubmitInfo VkCommandBufferSubmitInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSubmitInfo2_host
{
    VkStructureType sType;
    const void *pNext;
    VkSubmitFlags flags;
    uint32_t waitSemaphoreInfoCount;
    const VkSemaphoreSubmitInfo_host *pWaitSemaphoreInfos;
    uint32_t commandBufferInfoCount;
    const VkCommandBufferSubmitInfo *pCommandBufferInfos;
    uint32_t signalSemaphoreInfoCount;
    const VkSemaphoreSubmitInfo_host *pSignalSemaphoreInfos;
} VkSubmitInfo2_host;
typedef VkSubmitInfo2 VkSubmitInfo2KHR;
#else
typedef VkSubmitInfo2 VkSubmitInfo2_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDebugUtilsObjectNameInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkObjectType objectType;
    uint64_t objectHandle;
    const char *pObjectName;
} VkDebugUtilsObjectNameInfoEXT_host;
#else
typedef VkDebugUtilsObjectNameInfoEXT VkDebugUtilsObjectNameInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDebugUtilsObjectTagInfoEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkObjectType objectType;
    uint64_t objectHandle;
    uint64_t tagName;
    size_t tagSize;
    const void *pTag;
} VkDebugUtilsObjectTagInfoEXT_host;
#else
typedef VkDebugUtilsObjectTagInfoEXT VkDebugUtilsObjectTagInfoEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkSemaphoreSignalInfo_host
{
    VkStructureType sType;
    const void *pNext;
    VkSemaphore semaphore;
    uint64_t value;
} VkSemaphoreSignalInfo_host;
typedef VkSemaphoreSignalInfo VkSemaphoreSignalInfoKHR;
#else
typedef VkSemaphoreSignalInfo VkSemaphoreSignalInfo_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
typedef struct VkDebugUtilsMessengerCallbackDataEXT_host
{
    VkStructureType sType;
    const void *pNext;
    VkDebugUtilsMessengerCallbackDataFlagsEXT flags;
    const char *pMessageIdName;
    int32_t messageIdNumber;
    const char *pMessage;
    uint32_t queueLabelCount;
    const VkDebugUtilsLabelEXT *pQueueLabels;
    uint32_t cmdBufLabelCount;
    const VkDebugUtilsLabelEXT *pCmdBufLabels;
    uint32_t objectCount;
    const VkDebugUtilsObjectNameInfoEXT_host *pObjects;
} VkDebugUtilsMessengerCallbackDataEXT_host;
#else
typedef VkDebugUtilsMessengerCallbackDataEXT VkDebugUtilsMessengerCallbackDataEXT_host;
#endif

#if defined(USE_STRUCT_CONVERSION)
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
#else
typedef VkCopyDescriptorSet VkCopyDescriptorSet_host;
#endif


struct conversion_context;
VkResult convert_VkDeviceCreateInfo_struct_chain(struct conversion_context *ctx, const void *pNext, VkDeviceCreateInfo *out_struct) DECLSPEC_HIDDEN;
VkResult convert_VkInstanceCreateInfo_struct_chain(struct conversion_context *ctx, const void *pNext, VkInstanceCreateInfo *out_struct) DECLSPEC_HIDDEN;

/* For use by vkDevice and children */
struct vulkan_device_funcs
{
    VkResult (*p_vkAcquireNextImage2KHR)(VkDevice, const VkAcquireNextImageInfoKHR_host *, uint32_t *);
    VkResult (*p_vkAcquireNextImageKHR)(VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t *);
    VkResult (*p_vkAcquirePerformanceConfigurationINTEL)(VkDevice, const VkPerformanceConfigurationAcquireInfoINTEL *, VkPerformanceConfigurationINTEL *);
    VkResult (*p_vkAcquireProfilingLockKHR)(VkDevice, const VkAcquireProfilingLockInfoKHR_host *);
    VkResult (*p_vkAllocateCommandBuffers)(VkDevice, const VkCommandBufferAllocateInfo_host *, VkCommandBuffer *);
    VkResult (*p_vkAllocateDescriptorSets)(VkDevice, const VkDescriptorSetAllocateInfo_host *, VkDescriptorSet *);
    VkResult (*p_vkAllocateMemory)(VkDevice, const VkMemoryAllocateInfo_host *, const VkAllocationCallbacks *, VkDeviceMemory *);
    VkResult (*p_vkBeginCommandBuffer)(VkCommandBuffer, const VkCommandBufferBeginInfo_host *);
    VkResult (*p_vkBindAccelerationStructureMemoryNV)(VkDevice, uint32_t, const VkBindAccelerationStructureMemoryInfoNV_host *);
    VkResult (*p_vkBindBufferMemory)(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize);
    VkResult (*p_vkBindBufferMemory2)(VkDevice, uint32_t, const VkBindBufferMemoryInfo_host *);
    VkResult (*p_vkBindBufferMemory2KHR)(VkDevice, uint32_t, const VkBindBufferMemoryInfo_host *);
    VkResult (*p_vkBindImageMemory)(VkDevice, VkImage, VkDeviceMemory, VkDeviceSize);
    VkResult (*p_vkBindImageMemory2)(VkDevice, uint32_t, const VkBindImageMemoryInfo_host *);
    VkResult (*p_vkBindImageMemory2KHR)(VkDevice, uint32_t, const VkBindImageMemoryInfo_host *);
    VkResult (*p_vkBindOpticalFlowSessionImageNV)(VkDevice, VkOpticalFlowSessionNV, VkOpticalFlowSessionBindingPointNV, VkImageView, VkImageLayout);
    VkResult (*p_vkBuildAccelerationStructuresKHR)(VkDevice, VkDeferredOperationKHR, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR_host *, const VkAccelerationStructureBuildRangeInfoKHR * const*);
    VkResult (*p_vkBuildMicromapsEXT)(VkDevice, VkDeferredOperationKHR, uint32_t, const VkMicromapBuildInfoEXT_host *);
    void (*p_vkCmdBeginConditionalRenderingEXT)(VkCommandBuffer, const VkConditionalRenderingBeginInfoEXT_host *);
    void (*p_vkCmdBeginDebugUtilsLabelEXT)(VkCommandBuffer, const VkDebugUtilsLabelEXT *);
    void (*p_vkCmdBeginQuery)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags);
    void (*p_vkCmdBeginQueryIndexedEXT)(VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags, uint32_t);
    void (*p_vkCmdBeginRenderPass)(VkCommandBuffer, const VkRenderPassBeginInfo_host *, VkSubpassContents);
    void (*p_vkCmdBeginRenderPass2)(VkCommandBuffer, const VkRenderPassBeginInfo_host *, const VkSubpassBeginInfo *);
    void (*p_vkCmdBeginRenderPass2KHR)(VkCommandBuffer, const VkRenderPassBeginInfo_host *, const VkSubpassBeginInfo *);
    void (*p_vkCmdBeginRendering)(VkCommandBuffer, const VkRenderingInfo_host *);
    void (*p_vkCmdBeginRenderingKHR)(VkCommandBuffer, const VkRenderingInfo_host *);
    void (*p_vkCmdBeginTransformFeedbackEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *);
    void (*p_vkCmdBindDescriptorSets)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet *, uint32_t, const uint32_t *);
    void (*p_vkCmdBindIndexBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType);
    void (*p_vkCmdBindInvocationMaskHUAWEI)(VkCommandBuffer, VkImageView, VkImageLayout);
    void (*p_vkCmdBindPipeline)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline);
    void (*p_vkCmdBindPipelineShaderGroupNV)(VkCommandBuffer, VkPipelineBindPoint, VkPipeline, uint32_t);
    void (*p_vkCmdBindShadingRateImageNV)(VkCommandBuffer, VkImageView, VkImageLayout);
    void (*p_vkCmdBindTransformFeedbackBuffersEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *);
    void (*p_vkCmdBindVertexBuffers)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *);
    void (*p_vkCmdBindVertexBuffers2)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *, const VkDeviceSize *);
    void (*p_vkCmdBindVertexBuffers2EXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *, const VkDeviceSize *);
    void (*p_vkCmdBlitImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit *, VkFilter);
    void (*p_vkCmdBlitImage2)(VkCommandBuffer, const VkBlitImageInfo2_host *);
    void (*p_vkCmdBlitImage2KHR)(VkCommandBuffer, const VkBlitImageInfo2_host *);
    void (*p_vkCmdBuildAccelerationStructureNV)(VkCommandBuffer, const VkAccelerationStructureInfoNV_host *, VkBuffer, VkDeviceSize, VkBool32, VkAccelerationStructureNV, VkAccelerationStructureNV, VkBuffer, VkDeviceSize);
    void (*p_vkCmdBuildAccelerationStructuresIndirectKHR)(VkCommandBuffer, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR_host *, const VkDeviceAddress *, const uint32_t *, const uint32_t * const*);
    void (*p_vkCmdBuildAccelerationStructuresKHR)(VkCommandBuffer, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR_host *, const VkAccelerationStructureBuildRangeInfoKHR * const*);
    void (*p_vkCmdBuildMicromapsEXT)(VkCommandBuffer, uint32_t, const VkMicromapBuildInfoEXT_host *);
    void (*p_vkCmdClearAttachments)(VkCommandBuffer, uint32_t, const VkClearAttachment *, uint32_t, const VkClearRect *);
    void (*p_vkCmdClearColorImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue *, uint32_t, const VkImageSubresourceRange *);
    void (*p_vkCmdClearDepthStencilImage)(VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue *, uint32_t, const VkImageSubresourceRange *);
    void (*p_vkCmdCopyAccelerationStructureKHR)(VkCommandBuffer, const VkCopyAccelerationStructureInfoKHR_host *);
    void (*p_vkCmdCopyAccelerationStructureNV)(VkCommandBuffer, VkAccelerationStructureNV, VkAccelerationStructureNV, VkCopyAccelerationStructureModeKHR);
    void (*p_vkCmdCopyAccelerationStructureToMemoryKHR)(VkCommandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR_host *);
    void (*p_vkCmdCopyBuffer)(VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy_host *);
    void (*p_vkCmdCopyBuffer2)(VkCommandBuffer, const VkCopyBufferInfo2_host *);
    void (*p_vkCmdCopyBuffer2KHR)(VkCommandBuffer, const VkCopyBufferInfo2_host *);
    void (*p_vkCmdCopyBufferToImage)(VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy_host *);
    void (*p_vkCmdCopyBufferToImage2)(VkCommandBuffer, const VkCopyBufferToImageInfo2_host *);
    void (*p_vkCmdCopyBufferToImage2KHR)(VkCommandBuffer, const VkCopyBufferToImageInfo2_host *);
    void (*p_vkCmdCopyImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy *);
    void (*p_vkCmdCopyImage2)(VkCommandBuffer, const VkCopyImageInfo2_host *);
    void (*p_vkCmdCopyImage2KHR)(VkCommandBuffer, const VkCopyImageInfo2_host *);
    void (*p_vkCmdCopyImageToBuffer)(VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy_host *);
    void (*p_vkCmdCopyImageToBuffer2)(VkCommandBuffer, const VkCopyImageToBufferInfo2_host *);
    void (*p_vkCmdCopyImageToBuffer2KHR)(VkCommandBuffer, const VkCopyImageToBufferInfo2_host *);
    void (*p_vkCmdCopyMemoryToAccelerationStructureKHR)(VkCommandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR_host *);
    void (*p_vkCmdCopyMemoryToMicromapEXT)(VkCommandBuffer, const VkCopyMemoryToMicromapInfoEXT_host *);
    void (*p_vkCmdCopyMicromapEXT)(VkCommandBuffer, const VkCopyMicromapInfoEXT_host *);
    void (*p_vkCmdCopyMicromapToMemoryEXT)(VkCommandBuffer, const VkCopyMicromapToMemoryInfoEXT_host *);
    void (*p_vkCmdCopyQueryPoolResults)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags);
    void (*p_vkCmdCuLaunchKernelNVX)(VkCommandBuffer, const VkCuLaunchInfoNVX_host *);
    void (*p_vkCmdDebugMarkerBeginEXT)(VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT *);
    void (*p_vkCmdDebugMarkerEndEXT)(VkCommandBuffer);
    void (*p_vkCmdDebugMarkerInsertEXT)(VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT *);
    void (*p_vkCmdDispatch)(VkCommandBuffer, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchBase)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchBaseKHR)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDispatchIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize);
    void (*p_vkCmdDraw)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndexed)(VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t);
    void (*p_vkCmdDrawIndexedIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndexedIndirectCount)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndexedIndirectCountAMD)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndexedIndirectCountKHR)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndirect)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndirectByteCountEXT)(VkCommandBuffer, uint32_t, uint32_t, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndirectCount)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndirectCountAMD)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawIndirectCountKHR)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawMeshTasksEXT)(VkCommandBuffer, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDrawMeshTasksIndirectCountEXT)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawMeshTasksIndirectCountNV)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawMeshTasksIndirectEXT)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawMeshTasksIndirectNV)(VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t);
    void (*p_vkCmdDrawMeshTasksNV)(VkCommandBuffer, uint32_t, uint32_t);
    void (*p_vkCmdDrawMultiEXT)(VkCommandBuffer, uint32_t, const VkMultiDrawInfoEXT *, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdDrawMultiIndexedEXT)(VkCommandBuffer, uint32_t, const VkMultiDrawIndexedInfoEXT *, uint32_t, uint32_t, uint32_t, const int32_t *);
    void (*p_vkCmdEndConditionalRenderingEXT)(VkCommandBuffer);
    void (*p_vkCmdEndDebugUtilsLabelEXT)(VkCommandBuffer);
    void (*p_vkCmdEndQuery)(VkCommandBuffer, VkQueryPool, uint32_t);
    void (*p_vkCmdEndQueryIndexedEXT)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
    void (*p_vkCmdEndRenderPass)(VkCommandBuffer);
    void (*p_vkCmdEndRenderPass2)(VkCommandBuffer, const VkSubpassEndInfo *);
    void (*p_vkCmdEndRenderPass2KHR)(VkCommandBuffer, const VkSubpassEndInfo *);
    void (*p_vkCmdEndRendering)(VkCommandBuffer);
    void (*p_vkCmdEndRenderingKHR)(VkCommandBuffer);
    void (*p_vkCmdEndTransformFeedbackEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *);
    void (*p_vkCmdExecuteCommands)(VkCommandBuffer, uint32_t, const VkCommandBuffer *);
    void (*p_vkCmdExecuteGeneratedCommandsNV)(VkCommandBuffer, VkBool32, const VkGeneratedCommandsInfoNV_host *);
    void (*p_vkCmdFillBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t);
    void (*p_vkCmdInsertDebugUtilsLabelEXT)(VkCommandBuffer, const VkDebugUtilsLabelEXT *);
    void (*p_vkCmdNextSubpass)(VkCommandBuffer, VkSubpassContents);
    void (*p_vkCmdNextSubpass2)(VkCommandBuffer, const VkSubpassBeginInfo *, const VkSubpassEndInfo *);
    void (*p_vkCmdNextSubpass2KHR)(VkCommandBuffer, const VkSubpassBeginInfo *, const VkSubpassEndInfo *);
    void (*p_vkCmdOpticalFlowExecuteNV)(VkCommandBuffer, VkOpticalFlowSessionNV, const VkOpticalFlowExecuteInfoNV *);
    void (*p_vkCmdPipelineBarrier)(VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier_host *, uint32_t, const VkImageMemoryBarrier_host *);
    void (*p_vkCmdPipelineBarrier2)(VkCommandBuffer, const VkDependencyInfo_host *);
    void (*p_vkCmdPipelineBarrier2KHR)(VkCommandBuffer, const VkDependencyInfo_host *);
    void (*p_vkCmdPreprocessGeneratedCommandsNV)(VkCommandBuffer, const VkGeneratedCommandsInfoNV_host *);
    void (*p_vkCmdPushConstants)(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void *);
    void (*p_vkCmdPushDescriptorSetKHR)(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkWriteDescriptorSet_host *);
    void (*p_vkCmdPushDescriptorSetWithTemplateKHR)(VkCommandBuffer, VkDescriptorUpdateTemplate, VkPipelineLayout, uint32_t, const void *);
    void (*p_vkCmdResetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags);
    void (*p_vkCmdResetEvent2)(VkCommandBuffer, VkEvent, VkPipelineStageFlags2);
    void (*p_vkCmdResetEvent2KHR)(VkCommandBuffer, VkEvent, VkPipelineStageFlags2);
    void (*p_vkCmdResetQueryPool)(VkCommandBuffer, VkQueryPool, uint32_t, uint32_t);
    void (*p_vkCmdResolveImage)(VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve *);
    void (*p_vkCmdResolveImage2)(VkCommandBuffer, const VkResolveImageInfo2_host *);
    void (*p_vkCmdResolveImage2KHR)(VkCommandBuffer, const VkResolveImageInfo2_host *);
    void (*p_vkCmdSetAlphaToCoverageEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetAlphaToOneEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetBlendConstants)(VkCommandBuffer, const float[4]);
    void (*p_vkCmdSetCheckpointNV)(VkCommandBuffer, const void *);
    void (*p_vkCmdSetCoarseSampleOrderNV)(VkCommandBuffer, VkCoarseSampleOrderTypeNV, uint32_t, const VkCoarseSampleOrderCustomNV *);
    void (*p_vkCmdSetColorBlendAdvancedEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkColorBlendAdvancedEXT *);
    void (*p_vkCmdSetColorBlendEnableEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkBool32 *);
    void (*p_vkCmdSetColorBlendEquationEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkColorBlendEquationEXT *);
    void (*p_vkCmdSetColorWriteEnableEXT)(VkCommandBuffer, uint32_t, const VkBool32 *);
    void (*p_vkCmdSetColorWriteMaskEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkColorComponentFlags *);
    void (*p_vkCmdSetConservativeRasterizationModeEXT)(VkCommandBuffer, VkConservativeRasterizationModeEXT);
    void (*p_vkCmdSetCoverageModulationModeNV)(VkCommandBuffer, VkCoverageModulationModeNV);
    void (*p_vkCmdSetCoverageModulationTableEnableNV)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetCoverageModulationTableNV)(VkCommandBuffer, uint32_t, const float *);
    void (*p_vkCmdSetCoverageReductionModeNV)(VkCommandBuffer, VkCoverageReductionModeNV);
    void (*p_vkCmdSetCoverageToColorEnableNV)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetCoverageToColorLocationNV)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetCullMode)(VkCommandBuffer, VkCullModeFlags);
    void (*p_vkCmdSetCullModeEXT)(VkCommandBuffer, VkCullModeFlags);
    void (*p_vkCmdSetDepthBias)(VkCommandBuffer, float, float, float);
    void (*p_vkCmdSetDepthBiasEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthBiasEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthBounds)(VkCommandBuffer, float, float);
    void (*p_vkCmdSetDepthBoundsTestEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthBoundsTestEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthClampEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthClipEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthClipNegativeOneToOneEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthCompareOp)(VkCommandBuffer, VkCompareOp);
    void (*p_vkCmdSetDepthCompareOpEXT)(VkCommandBuffer, VkCompareOp);
    void (*p_vkCmdSetDepthTestEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthTestEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthWriteEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDepthWriteEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetDeviceMask)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetDeviceMaskKHR)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetDiscardRectangleEXT)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetEvent)(VkCommandBuffer, VkEvent, VkPipelineStageFlags);
    void (*p_vkCmdSetEvent2)(VkCommandBuffer, VkEvent, const VkDependencyInfo_host *);
    void (*p_vkCmdSetEvent2KHR)(VkCommandBuffer, VkEvent, const VkDependencyInfo_host *);
    void (*p_vkCmdSetExclusiveScissorNV)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetExtraPrimitiveOverestimationSizeEXT)(VkCommandBuffer, float);
    void (*p_vkCmdSetFragmentShadingRateEnumNV)(VkCommandBuffer, VkFragmentShadingRateNV, const VkFragmentShadingRateCombinerOpKHR[2]);
    void (*p_vkCmdSetFragmentShadingRateKHR)(VkCommandBuffer, const VkExtent2D *, const VkFragmentShadingRateCombinerOpKHR[2]);
    void (*p_vkCmdSetFrontFace)(VkCommandBuffer, VkFrontFace);
    void (*p_vkCmdSetFrontFaceEXT)(VkCommandBuffer, VkFrontFace);
    void (*p_vkCmdSetLineRasterizationModeEXT)(VkCommandBuffer, VkLineRasterizationModeEXT);
    void (*p_vkCmdSetLineStippleEXT)(VkCommandBuffer, uint32_t, uint16_t);
    void (*p_vkCmdSetLineStippleEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetLineWidth)(VkCommandBuffer, float);
    void (*p_vkCmdSetLogicOpEXT)(VkCommandBuffer, VkLogicOp);
    void (*p_vkCmdSetLogicOpEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetPatchControlPointsEXT)(VkCommandBuffer, uint32_t);
    VkResult (*p_vkCmdSetPerformanceMarkerINTEL)(VkCommandBuffer, const VkPerformanceMarkerInfoINTEL_host *);
    VkResult (*p_vkCmdSetPerformanceOverrideINTEL)(VkCommandBuffer, const VkPerformanceOverrideInfoINTEL_host *);
    VkResult (*p_vkCmdSetPerformanceStreamMarkerINTEL)(VkCommandBuffer, const VkPerformanceStreamMarkerInfoINTEL *);
    void (*p_vkCmdSetPolygonModeEXT)(VkCommandBuffer, VkPolygonMode);
    void (*p_vkCmdSetPrimitiveRestartEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetPrimitiveRestartEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetPrimitiveTopology)(VkCommandBuffer, VkPrimitiveTopology);
    void (*p_vkCmdSetPrimitiveTopologyEXT)(VkCommandBuffer, VkPrimitiveTopology);
    void (*p_vkCmdSetProvokingVertexModeEXT)(VkCommandBuffer, VkProvokingVertexModeEXT);
    void (*p_vkCmdSetRasterizationSamplesEXT)(VkCommandBuffer, VkSampleCountFlagBits);
    void (*p_vkCmdSetRasterizationStreamEXT)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetRasterizerDiscardEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetRasterizerDiscardEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetRayTracingPipelineStackSizeKHR)(VkCommandBuffer, uint32_t);
    void (*p_vkCmdSetRepresentativeFragmentTestEnableNV)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetSampleLocationsEXT)(VkCommandBuffer, const VkSampleLocationsInfoEXT *);
    void (*p_vkCmdSetSampleLocationsEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetSampleMaskEXT)(VkCommandBuffer, VkSampleCountFlagBits, const VkSampleMask *);
    void (*p_vkCmdSetScissor)(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetScissorWithCount)(VkCommandBuffer, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetScissorWithCountEXT)(VkCommandBuffer, uint32_t, const VkRect2D *);
    void (*p_vkCmdSetShadingRateImageEnableNV)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetStencilCompareMask)(VkCommandBuffer, VkStencilFaceFlags, uint32_t);
    void (*p_vkCmdSetStencilOp)(VkCommandBuffer, VkStencilFaceFlags, VkStencilOp, VkStencilOp, VkStencilOp, VkCompareOp);
    void (*p_vkCmdSetStencilOpEXT)(VkCommandBuffer, VkStencilFaceFlags, VkStencilOp, VkStencilOp, VkStencilOp, VkCompareOp);
    void (*p_vkCmdSetStencilReference)(VkCommandBuffer, VkStencilFaceFlags, uint32_t);
    void (*p_vkCmdSetStencilTestEnable)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetStencilTestEnableEXT)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetStencilWriteMask)(VkCommandBuffer, VkStencilFaceFlags, uint32_t);
    void (*p_vkCmdSetTessellationDomainOriginEXT)(VkCommandBuffer, VkTessellationDomainOrigin);
    void (*p_vkCmdSetVertexInputEXT)(VkCommandBuffer, uint32_t, const VkVertexInputBindingDescription2EXT *, uint32_t, const VkVertexInputAttributeDescription2EXT *);
    void (*p_vkCmdSetViewport)(VkCommandBuffer, uint32_t, uint32_t, const VkViewport *);
    void (*p_vkCmdSetViewportShadingRatePaletteNV)(VkCommandBuffer, uint32_t, uint32_t, const VkShadingRatePaletteNV *);
    void (*p_vkCmdSetViewportSwizzleNV)(VkCommandBuffer, uint32_t, uint32_t, const VkViewportSwizzleNV *);
    void (*p_vkCmdSetViewportWScalingEnableNV)(VkCommandBuffer, VkBool32);
    void (*p_vkCmdSetViewportWScalingNV)(VkCommandBuffer, uint32_t, uint32_t, const VkViewportWScalingNV *);
    void (*p_vkCmdSetViewportWithCount)(VkCommandBuffer, uint32_t, const VkViewport *);
    void (*p_vkCmdSetViewportWithCountEXT)(VkCommandBuffer, uint32_t, const VkViewport *);
    void (*p_vkCmdSubpassShadingHUAWEI)(VkCommandBuffer);
    void (*p_vkCmdTraceRaysIndirect2KHR)(VkCommandBuffer, VkDeviceAddress);
    void (*p_vkCmdTraceRaysIndirectKHR)(VkCommandBuffer, const VkStridedDeviceAddressRegionKHR_host *, const VkStridedDeviceAddressRegionKHR_host *, const VkStridedDeviceAddressRegionKHR_host *, const VkStridedDeviceAddressRegionKHR_host *, VkDeviceAddress);
    void (*p_vkCmdTraceRaysKHR)(VkCommandBuffer, const VkStridedDeviceAddressRegionKHR_host *, const VkStridedDeviceAddressRegionKHR_host *, const VkStridedDeviceAddressRegionKHR_host *, const VkStridedDeviceAddressRegionKHR_host *, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdTraceRaysNV)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t, uint32_t, uint32_t);
    void (*p_vkCmdUpdateBuffer)(VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, const void *);
    void (*p_vkCmdWaitEvents)(VkCommandBuffer, uint32_t, const VkEvent *, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier_host *, uint32_t, const VkImageMemoryBarrier_host *);
    void (*p_vkCmdWaitEvents2)(VkCommandBuffer, uint32_t, const VkEvent *, const VkDependencyInfo_host *);
    void (*p_vkCmdWaitEvents2KHR)(VkCommandBuffer, uint32_t, const VkEvent *, const VkDependencyInfo_host *);
    void (*p_vkCmdWriteAccelerationStructuresPropertiesKHR)(VkCommandBuffer, uint32_t, const VkAccelerationStructureKHR *, VkQueryType, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteAccelerationStructuresPropertiesNV)(VkCommandBuffer, uint32_t, const VkAccelerationStructureNV *, VkQueryType, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteBufferMarker2AMD)(VkCommandBuffer, VkPipelineStageFlags2, VkBuffer, VkDeviceSize, uint32_t);
    void (*p_vkCmdWriteBufferMarkerAMD)(VkCommandBuffer, VkPipelineStageFlagBits, VkBuffer, VkDeviceSize, uint32_t);
    void (*p_vkCmdWriteMicromapsPropertiesEXT)(VkCommandBuffer, uint32_t, const VkMicromapEXT *, VkQueryType, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteTimestamp)(VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteTimestamp2)(VkCommandBuffer, VkPipelineStageFlags2, VkQueryPool, uint32_t);
    void (*p_vkCmdWriteTimestamp2KHR)(VkCommandBuffer, VkPipelineStageFlags2, VkQueryPool, uint32_t);
    VkResult (*p_vkCompileDeferredNV)(VkDevice, VkPipeline, uint32_t);
    VkResult (*p_vkCopyAccelerationStructureKHR)(VkDevice, VkDeferredOperationKHR, const VkCopyAccelerationStructureInfoKHR_host *);
    VkResult (*p_vkCopyAccelerationStructureToMemoryKHR)(VkDevice, VkDeferredOperationKHR, const VkCopyAccelerationStructureToMemoryInfoKHR_host *);
    VkResult (*p_vkCopyMemoryToAccelerationStructureKHR)(VkDevice, VkDeferredOperationKHR, const VkCopyMemoryToAccelerationStructureInfoKHR_host *);
    VkResult (*p_vkCopyMemoryToMicromapEXT)(VkDevice, VkDeferredOperationKHR, const VkCopyMemoryToMicromapInfoEXT_host *);
    VkResult (*p_vkCopyMicromapEXT)(VkDevice, VkDeferredOperationKHR, const VkCopyMicromapInfoEXT_host *);
    VkResult (*p_vkCopyMicromapToMemoryEXT)(VkDevice, VkDeferredOperationKHR, const VkCopyMicromapToMemoryInfoEXT_host *);
    VkResult (*p_vkCreateAccelerationStructureKHR)(VkDevice, const VkAccelerationStructureCreateInfoKHR_host *, const VkAllocationCallbacks *, VkAccelerationStructureKHR *);
    VkResult (*p_vkCreateAccelerationStructureNV)(VkDevice, const VkAccelerationStructureCreateInfoNV_host *, const VkAllocationCallbacks *, VkAccelerationStructureNV *);
    VkResult (*p_vkCreateBuffer)(VkDevice, const VkBufferCreateInfo_host *, const VkAllocationCallbacks *, VkBuffer *);
    VkResult (*p_vkCreateBufferView)(VkDevice, const VkBufferViewCreateInfo_host *, const VkAllocationCallbacks *, VkBufferView *);
    VkResult (*p_vkCreateCommandPool)(VkDevice, const VkCommandPoolCreateInfo *, const VkAllocationCallbacks *, VkCommandPool *);
    VkResult (*p_vkCreateComputePipelines)(VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo_host *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateCuFunctionNVX)(VkDevice, const VkCuFunctionCreateInfoNVX_host *, const VkAllocationCallbacks *, VkCuFunctionNVX *);
    VkResult (*p_vkCreateCuModuleNVX)(VkDevice, const VkCuModuleCreateInfoNVX *, const VkAllocationCallbacks *, VkCuModuleNVX *);
    VkResult (*p_vkCreateDeferredOperationKHR)(VkDevice, const VkAllocationCallbacks *, VkDeferredOperationKHR *);
    VkResult (*p_vkCreateDescriptorPool)(VkDevice, const VkDescriptorPoolCreateInfo *, const VkAllocationCallbacks *, VkDescriptorPool *);
    VkResult (*p_vkCreateDescriptorSetLayout)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, const VkAllocationCallbacks *, VkDescriptorSetLayout *);
    VkResult (*p_vkCreateDescriptorUpdateTemplate)(VkDevice, const VkDescriptorUpdateTemplateCreateInfo_host *, const VkAllocationCallbacks *, VkDescriptorUpdateTemplate *);
    VkResult (*p_vkCreateDescriptorUpdateTemplateKHR)(VkDevice, const VkDescriptorUpdateTemplateCreateInfo_host *, const VkAllocationCallbacks *, VkDescriptorUpdateTemplate *);
    VkResult (*p_vkCreateEvent)(VkDevice, const VkEventCreateInfo *, const VkAllocationCallbacks *, VkEvent *);
    VkResult (*p_vkCreateFence)(VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *, VkFence *);
    VkResult (*p_vkCreateFramebuffer)(VkDevice, const VkFramebufferCreateInfo_host *, const VkAllocationCallbacks *, VkFramebuffer *);
    VkResult (*p_vkCreateGraphicsPipelines)(VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo_host *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateImage)(VkDevice, const VkImageCreateInfo *, const VkAllocationCallbacks *, VkImage *);
    VkResult (*p_vkCreateImageView)(VkDevice, const VkImageViewCreateInfo_host *, const VkAllocationCallbacks *, VkImageView *);
    VkResult (*p_vkCreateIndirectCommandsLayoutNV)(VkDevice, const VkIndirectCommandsLayoutCreateInfoNV_host *, const VkAllocationCallbacks *, VkIndirectCommandsLayoutNV *);
    VkResult (*p_vkCreateMicromapEXT)(VkDevice, const VkMicromapCreateInfoEXT_host *, const VkAllocationCallbacks *, VkMicromapEXT *);
    VkResult (*p_vkCreateOpticalFlowSessionNV)(VkDevice, const VkOpticalFlowSessionCreateInfoNV *, const VkAllocationCallbacks *, VkOpticalFlowSessionNV *);
    VkResult (*p_vkCreatePipelineCache)(VkDevice, const VkPipelineCacheCreateInfo *, const VkAllocationCallbacks *, VkPipelineCache *);
    VkResult (*p_vkCreatePipelineLayout)(VkDevice, const VkPipelineLayoutCreateInfo *, const VkAllocationCallbacks *, VkPipelineLayout *);
    VkResult (*p_vkCreatePrivateDataSlot)(VkDevice, const VkPrivateDataSlotCreateInfo *, const VkAllocationCallbacks *, VkPrivateDataSlot *);
    VkResult (*p_vkCreatePrivateDataSlotEXT)(VkDevice, const VkPrivateDataSlotCreateInfo *, const VkAllocationCallbacks *, VkPrivateDataSlot *);
    VkResult (*p_vkCreateQueryPool)(VkDevice, const VkQueryPoolCreateInfo *, const VkAllocationCallbacks *, VkQueryPool *);
    VkResult (*p_vkCreateRayTracingPipelinesKHR)(VkDevice, VkDeferredOperationKHR, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoKHR_host *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateRayTracingPipelinesNV)(VkDevice, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoNV_host *, const VkAllocationCallbacks *, VkPipeline *);
    VkResult (*p_vkCreateRenderPass)(VkDevice, const VkRenderPassCreateInfo *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateRenderPass2)(VkDevice, const VkRenderPassCreateInfo2 *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateRenderPass2KHR)(VkDevice, const VkRenderPassCreateInfo2 *, const VkAllocationCallbacks *, VkRenderPass *);
    VkResult (*p_vkCreateSampler)(VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *, VkSampler *);
    VkResult (*p_vkCreateSamplerYcbcrConversion)(VkDevice, const VkSamplerYcbcrConversionCreateInfo *, const VkAllocationCallbacks *, VkSamplerYcbcrConversion *);
    VkResult (*p_vkCreateSamplerYcbcrConversionKHR)(VkDevice, const VkSamplerYcbcrConversionCreateInfo *, const VkAllocationCallbacks *, VkSamplerYcbcrConversion *);
    VkResult (*p_vkCreateSemaphore)(VkDevice, const VkSemaphoreCreateInfo *, const VkAllocationCallbacks *, VkSemaphore *);
    VkResult (*p_vkCreateShaderModule)(VkDevice, const VkShaderModuleCreateInfo *, const VkAllocationCallbacks *, VkShaderModule *);
    VkResult (*p_vkCreateSwapchainKHR)(VkDevice, const VkSwapchainCreateInfoKHR_host *, const VkAllocationCallbacks *, VkSwapchainKHR *);
    VkResult (*p_vkCreateValidationCacheEXT)(VkDevice, const VkValidationCacheCreateInfoEXT *, const VkAllocationCallbacks *, VkValidationCacheEXT *);
    VkResult (*p_vkDebugMarkerSetObjectNameEXT)(VkDevice, const VkDebugMarkerObjectNameInfoEXT_host *);
    VkResult (*p_vkDebugMarkerSetObjectTagEXT)(VkDevice, const VkDebugMarkerObjectTagInfoEXT_host *);
    VkResult (*p_vkDeferredOperationJoinKHR)(VkDevice, VkDeferredOperationKHR);
    void (*p_vkDestroyAccelerationStructureKHR)(VkDevice, VkAccelerationStructureKHR, const VkAllocationCallbacks *);
    void (*p_vkDestroyAccelerationStructureNV)(VkDevice, VkAccelerationStructureNV, const VkAllocationCallbacks *);
    void (*p_vkDestroyBuffer)(VkDevice, VkBuffer, const VkAllocationCallbacks *);
    void (*p_vkDestroyBufferView)(VkDevice, VkBufferView, const VkAllocationCallbacks *);
    void (*p_vkDestroyCommandPool)(VkDevice, VkCommandPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyCuFunctionNVX)(VkDevice, VkCuFunctionNVX, const VkAllocationCallbacks *);
    void (*p_vkDestroyCuModuleNVX)(VkDevice, VkCuModuleNVX, const VkAllocationCallbacks *);
    void (*p_vkDestroyDeferredOperationKHR)(VkDevice, VkDeferredOperationKHR, const VkAllocationCallbacks *);
    void (*p_vkDestroyDescriptorPool)(VkDevice, VkDescriptorPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyDescriptorSetLayout)(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks *);
    void (*p_vkDestroyDescriptorUpdateTemplate)(VkDevice, VkDescriptorUpdateTemplate, const VkAllocationCallbacks *);
    void (*p_vkDestroyDescriptorUpdateTemplateKHR)(VkDevice, VkDescriptorUpdateTemplate, const VkAllocationCallbacks *);
    void (*p_vkDestroyDevice)(VkDevice, const VkAllocationCallbacks *);
    void (*p_vkDestroyEvent)(VkDevice, VkEvent, const VkAllocationCallbacks *);
    void (*p_vkDestroyFence)(VkDevice, VkFence, const VkAllocationCallbacks *);
    void (*p_vkDestroyFramebuffer)(VkDevice, VkFramebuffer, const VkAllocationCallbacks *);
    void (*p_vkDestroyImage)(VkDevice, VkImage, const VkAllocationCallbacks *);
    void (*p_vkDestroyImageView)(VkDevice, VkImageView, const VkAllocationCallbacks *);
    void (*p_vkDestroyIndirectCommandsLayoutNV)(VkDevice, VkIndirectCommandsLayoutNV, const VkAllocationCallbacks *);
    void (*p_vkDestroyMicromapEXT)(VkDevice, VkMicromapEXT, const VkAllocationCallbacks *);
    void (*p_vkDestroyOpticalFlowSessionNV)(VkDevice, VkOpticalFlowSessionNV, const VkAllocationCallbacks *);
    void (*p_vkDestroyPipeline)(VkDevice, VkPipeline, const VkAllocationCallbacks *);
    void (*p_vkDestroyPipelineCache)(VkDevice, VkPipelineCache, const VkAllocationCallbacks *);
    void (*p_vkDestroyPipelineLayout)(VkDevice, VkPipelineLayout, const VkAllocationCallbacks *);
    void (*p_vkDestroyPrivateDataSlot)(VkDevice, VkPrivateDataSlot, const VkAllocationCallbacks *);
    void (*p_vkDestroyPrivateDataSlotEXT)(VkDevice, VkPrivateDataSlot, const VkAllocationCallbacks *);
    void (*p_vkDestroyQueryPool)(VkDevice, VkQueryPool, const VkAllocationCallbacks *);
    void (*p_vkDestroyRenderPass)(VkDevice, VkRenderPass, const VkAllocationCallbacks *);
    void (*p_vkDestroySampler)(VkDevice, VkSampler, const VkAllocationCallbacks *);
    void (*p_vkDestroySamplerYcbcrConversion)(VkDevice, VkSamplerYcbcrConversion, const VkAllocationCallbacks *);
    void (*p_vkDestroySamplerYcbcrConversionKHR)(VkDevice, VkSamplerYcbcrConversion, const VkAllocationCallbacks *);
    void (*p_vkDestroySemaphore)(VkDevice, VkSemaphore, const VkAllocationCallbacks *);
    void (*p_vkDestroyShaderModule)(VkDevice, VkShaderModule, const VkAllocationCallbacks *);
    void (*p_vkDestroySwapchainKHR)(VkDevice, VkSwapchainKHR, const VkAllocationCallbacks *);
    void (*p_vkDestroyValidationCacheEXT)(VkDevice, VkValidationCacheEXT, const VkAllocationCallbacks *);
    VkResult (*p_vkDeviceWaitIdle)(VkDevice);
    VkResult (*p_vkEndCommandBuffer)(VkCommandBuffer);
    VkResult (*p_vkFlushMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange_host *);
    void (*p_vkFreeCommandBuffers)(VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *);
    VkResult (*p_vkFreeDescriptorSets)(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet *);
    void (*p_vkFreeMemory)(VkDevice, VkDeviceMemory, const VkAllocationCallbacks *);
    void (*p_vkGetAccelerationStructureBuildSizesKHR)(VkDevice, VkAccelerationStructureBuildTypeKHR, const VkAccelerationStructureBuildGeometryInfoKHR_host *, const uint32_t *, VkAccelerationStructureBuildSizesInfoKHR_host *);
    VkDeviceAddress (*p_vkGetAccelerationStructureDeviceAddressKHR)(VkDevice, const VkAccelerationStructureDeviceAddressInfoKHR_host *);
    VkResult (*p_vkGetAccelerationStructureHandleNV)(VkDevice, VkAccelerationStructureNV, size_t, void *);
    void (*p_vkGetAccelerationStructureMemoryRequirementsNV)(VkDevice, const VkAccelerationStructureMemoryRequirementsInfoNV_host *, VkMemoryRequirements2KHR_host *);
    VkDeviceAddress (*p_vkGetBufferDeviceAddress)(VkDevice, const VkBufferDeviceAddressInfo_host *);
    VkDeviceAddress (*p_vkGetBufferDeviceAddressEXT)(VkDevice, const VkBufferDeviceAddressInfo_host *);
    VkDeviceAddress (*p_vkGetBufferDeviceAddressKHR)(VkDevice, const VkBufferDeviceAddressInfo_host *);
    void (*p_vkGetBufferMemoryRequirements)(VkDevice, VkBuffer, VkMemoryRequirements_host *);
    void (*p_vkGetBufferMemoryRequirements2)(VkDevice, const VkBufferMemoryRequirementsInfo2_host *, VkMemoryRequirements2_host *);
    void (*p_vkGetBufferMemoryRequirements2KHR)(VkDevice, const VkBufferMemoryRequirementsInfo2_host *, VkMemoryRequirements2_host *);
    uint64_t (*p_vkGetBufferOpaqueCaptureAddress)(VkDevice, const VkBufferDeviceAddressInfo_host *);
    uint64_t (*p_vkGetBufferOpaqueCaptureAddressKHR)(VkDevice, const VkBufferDeviceAddressInfo_host *);
    VkResult (*p_vkGetCalibratedTimestampsEXT)(VkDevice, uint32_t, const VkCalibratedTimestampInfoEXT *, uint64_t *, uint64_t *);
    uint32_t (*p_vkGetDeferredOperationMaxConcurrencyKHR)(VkDevice, VkDeferredOperationKHR);
    VkResult (*p_vkGetDeferredOperationResultKHR)(VkDevice, VkDeferredOperationKHR);
    void (*p_vkGetDescriptorSetHostMappingVALVE)(VkDevice, VkDescriptorSet, void **);
    void (*p_vkGetDescriptorSetLayoutHostMappingInfoVALVE)(VkDevice, const VkDescriptorSetBindingReferenceVALVE_host *, VkDescriptorSetLayoutHostMappingInfoVALVE *);
    void (*p_vkGetDescriptorSetLayoutSupport)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, VkDescriptorSetLayoutSupport *);
    void (*p_vkGetDescriptorSetLayoutSupportKHR)(VkDevice, const VkDescriptorSetLayoutCreateInfo *, VkDescriptorSetLayoutSupport *);
    void (*p_vkGetDeviceAccelerationStructureCompatibilityKHR)(VkDevice, const VkAccelerationStructureVersionInfoKHR *, VkAccelerationStructureCompatibilityKHR *);
    void (*p_vkGetDeviceBufferMemoryRequirements)(VkDevice, const VkDeviceBufferMemoryRequirements_host *, VkMemoryRequirements2_host *);
    void (*p_vkGetDeviceBufferMemoryRequirementsKHR)(VkDevice, const VkDeviceBufferMemoryRequirements_host *, VkMemoryRequirements2_host *);
    VkResult (*p_vkGetDeviceFaultInfoEXT)(VkDevice, VkDeviceFaultCountsEXT_host *, VkDeviceFaultInfoEXT_host *);
    void (*p_vkGetDeviceGroupPeerMemoryFeatures)(VkDevice, uint32_t, uint32_t, uint32_t, VkPeerMemoryFeatureFlags *);
    void (*p_vkGetDeviceGroupPeerMemoryFeaturesKHR)(VkDevice, uint32_t, uint32_t, uint32_t, VkPeerMemoryFeatureFlags *);
    VkResult (*p_vkGetDeviceGroupPresentCapabilitiesKHR)(VkDevice, VkDeviceGroupPresentCapabilitiesKHR *);
    VkResult (*p_vkGetDeviceGroupSurfacePresentModesKHR)(VkDevice, VkSurfaceKHR, VkDeviceGroupPresentModeFlagsKHR *);
    void (*p_vkGetDeviceImageMemoryRequirements)(VkDevice, const VkDeviceImageMemoryRequirements *, VkMemoryRequirements2_host *);
    void (*p_vkGetDeviceImageMemoryRequirementsKHR)(VkDevice, const VkDeviceImageMemoryRequirements *, VkMemoryRequirements2_host *);
    void (*p_vkGetDeviceImageSparseMemoryRequirements)(VkDevice, const VkDeviceImageMemoryRequirements *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetDeviceImageSparseMemoryRequirementsKHR)(VkDevice, const VkDeviceImageMemoryRequirements *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetDeviceMemoryCommitment)(VkDevice, VkDeviceMemory, VkDeviceSize *);
    uint64_t (*p_vkGetDeviceMemoryOpaqueCaptureAddress)(VkDevice, const VkDeviceMemoryOpaqueCaptureAddressInfo_host *);
    uint64_t (*p_vkGetDeviceMemoryOpaqueCaptureAddressKHR)(VkDevice, const VkDeviceMemoryOpaqueCaptureAddressInfo_host *);
    void (*p_vkGetDeviceMicromapCompatibilityEXT)(VkDevice, const VkMicromapVersionInfoEXT *, VkAccelerationStructureCompatibilityKHR *);
    void (*p_vkGetDeviceQueue)(VkDevice, uint32_t, uint32_t, VkQueue *);
    void (*p_vkGetDeviceQueue2)(VkDevice, const VkDeviceQueueInfo2 *, VkQueue *);
    VkResult (*p_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI)(VkDevice, VkRenderPass, VkExtent2D *);
    VkResult (*p_vkGetDynamicRenderingTilePropertiesQCOM)(VkDevice, const VkRenderingInfo_host *, VkTilePropertiesQCOM *);
    VkResult (*p_vkGetEventStatus)(VkDevice, VkEvent);
    VkResult (*p_vkGetFenceStatus)(VkDevice, VkFence);
    VkResult (*p_vkGetFramebufferTilePropertiesQCOM)(VkDevice, VkFramebuffer, uint32_t *, VkTilePropertiesQCOM *);
    void (*p_vkGetGeneratedCommandsMemoryRequirementsNV)(VkDevice, const VkGeneratedCommandsMemoryRequirementsInfoNV_host *, VkMemoryRequirements2_host *);
    void (*p_vkGetImageMemoryRequirements)(VkDevice, VkImage, VkMemoryRequirements_host *);
    void (*p_vkGetImageMemoryRequirements2)(VkDevice, const VkImageMemoryRequirementsInfo2_host *, VkMemoryRequirements2_host *);
    void (*p_vkGetImageMemoryRequirements2KHR)(VkDevice, const VkImageMemoryRequirementsInfo2_host *, VkMemoryRequirements2_host *);
    void (*p_vkGetImageSparseMemoryRequirements)(VkDevice, VkImage, uint32_t *, VkSparseImageMemoryRequirements *);
    void (*p_vkGetImageSparseMemoryRequirements2)(VkDevice, const VkImageSparseMemoryRequirementsInfo2_host *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetImageSparseMemoryRequirements2KHR)(VkDevice, const VkImageSparseMemoryRequirementsInfo2_host *, uint32_t *, VkSparseImageMemoryRequirements2 *);
    void (*p_vkGetImageSubresourceLayout)(VkDevice, VkImage, const VkImageSubresource *, VkSubresourceLayout_host *);
    void (*p_vkGetImageSubresourceLayout2EXT)(VkDevice, VkImage, const VkImageSubresource2EXT *, VkSubresourceLayout2EXT_host *);
    VkResult (*p_vkGetImageViewAddressNVX)(VkDevice, VkImageView, VkImageViewAddressPropertiesNVX_host *);
    uint32_t (*p_vkGetImageViewHandleNVX)(VkDevice, const VkImageViewHandleInfoNVX_host *);
    VkResult (*p_vkGetMemoryHostPointerPropertiesEXT)(VkDevice, VkExternalMemoryHandleTypeFlagBits, const void *, VkMemoryHostPointerPropertiesEXT *);
    VkResult (*p_vkGetMemoryWin32HandleKHR)(VkDevice, const VkMemoryGetWin32HandleInfoKHR_host *, HANDLE *);
    VkResult (*p_vkGetMemoryWin32HandlePropertiesKHR)(VkDevice, VkExternalMemoryHandleTypeFlagBits, HANDLE, VkMemoryWin32HandlePropertiesKHR *);
    void (*p_vkGetMicromapBuildSizesEXT)(VkDevice, VkAccelerationStructureBuildTypeKHR, const VkMicromapBuildInfoEXT_host *, VkMicromapBuildSizesInfoEXT_host *);
    VkResult (*p_vkGetPerformanceParameterINTEL)(VkDevice, VkPerformanceParameterTypeINTEL, VkPerformanceValueINTEL *);
    VkResult (*p_vkGetPipelineCacheData)(VkDevice, VkPipelineCache, size_t *, void *);
    VkResult (*p_vkGetPipelineExecutableInternalRepresentationsKHR)(VkDevice, const VkPipelineExecutableInfoKHR_host *, uint32_t *, VkPipelineExecutableInternalRepresentationKHR *);
    VkResult (*p_vkGetPipelineExecutablePropertiesKHR)(VkDevice, const VkPipelineInfoKHR_host *, uint32_t *, VkPipelineExecutablePropertiesKHR *);
    VkResult (*p_vkGetPipelineExecutableStatisticsKHR)(VkDevice, const VkPipelineExecutableInfoKHR_host *, uint32_t *, VkPipelineExecutableStatisticKHR *);
    VkResult (*p_vkGetPipelinePropertiesEXT)(VkDevice, const VkPipelineInfoEXT_host *, VkBaseOutStructure *);
    void (*p_vkGetPrivateData)(VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t *);
    void (*p_vkGetPrivateDataEXT)(VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t *);
    VkResult (*p_vkGetQueryPoolResults)(VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void *, VkDeviceSize, VkQueryResultFlags);
    void (*p_vkGetQueueCheckpointData2NV)(VkQueue, uint32_t *, VkCheckpointData2NV *);
    void (*p_vkGetQueueCheckpointDataNV)(VkQueue, uint32_t *, VkCheckpointDataNV *);
    VkResult (*p_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)(VkDevice, VkPipeline, uint32_t, uint32_t, size_t, void *);
    VkResult (*p_vkGetRayTracingShaderGroupHandlesKHR)(VkDevice, VkPipeline, uint32_t, uint32_t, size_t, void *);
    VkResult (*p_vkGetRayTracingShaderGroupHandlesNV)(VkDevice, VkPipeline, uint32_t, uint32_t, size_t, void *);
    VkDeviceSize (*p_vkGetRayTracingShaderGroupStackSizeKHR)(VkDevice, VkPipeline, uint32_t, VkShaderGroupShaderKHR);
    void (*p_vkGetRenderAreaGranularity)(VkDevice, VkRenderPass, VkExtent2D *);
    VkResult (*p_vkGetSemaphoreCounterValue)(VkDevice, VkSemaphore, uint64_t *);
    VkResult (*p_vkGetSemaphoreCounterValueKHR)(VkDevice, VkSemaphore, uint64_t *);
    VkResult (*p_vkGetShaderInfoAMD)(VkDevice, VkPipeline, VkShaderStageFlagBits, VkShaderInfoTypeAMD, size_t *, void *);
    void (*p_vkGetShaderModuleCreateInfoIdentifierEXT)(VkDevice, const VkShaderModuleCreateInfo *, VkShaderModuleIdentifierEXT *);
    void (*p_vkGetShaderModuleIdentifierEXT)(VkDevice, VkShaderModule, VkShaderModuleIdentifierEXT *);
    VkResult (*p_vkGetSwapchainImagesKHR)(VkDevice, VkSwapchainKHR, uint32_t *, VkImage *);
    VkResult (*p_vkGetValidationCacheDataEXT)(VkDevice, VkValidationCacheEXT, size_t *, void *);
    VkResult (*p_vkInitializePerformanceApiINTEL)(VkDevice, const VkInitializePerformanceApiInfoINTEL *);
    VkResult (*p_vkInvalidateMappedMemoryRanges)(VkDevice, uint32_t, const VkMappedMemoryRange_host *);
    VkResult (*p_vkMapMemory)(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void **);
    VkResult (*p_vkMergePipelineCaches)(VkDevice, VkPipelineCache, uint32_t, const VkPipelineCache *);
    VkResult (*p_vkMergeValidationCachesEXT)(VkDevice, VkValidationCacheEXT, uint32_t, const VkValidationCacheEXT *);
    void (*p_vkQueueBeginDebugUtilsLabelEXT)(VkQueue, const VkDebugUtilsLabelEXT *);
    VkResult (*p_vkQueueBindSparse)(VkQueue, uint32_t, const VkBindSparseInfo_host *, VkFence);
    void (*p_vkQueueEndDebugUtilsLabelEXT)(VkQueue);
    void (*p_vkQueueInsertDebugUtilsLabelEXT)(VkQueue, const VkDebugUtilsLabelEXT *);
    VkResult (*p_vkQueuePresentKHR)(VkQueue, const VkPresentInfoKHR *);
    VkResult (*p_vkQueueSetPerformanceConfigurationINTEL)(VkQueue, VkPerformanceConfigurationINTEL);
    VkResult (*p_vkQueueSubmit)(VkQueue, uint32_t, const VkSubmitInfo *, VkFence);
    VkResult (*p_vkQueueSubmit2)(VkQueue, uint32_t, const VkSubmitInfo2_host *, VkFence);
    VkResult (*p_vkQueueSubmit2KHR)(VkQueue, uint32_t, const VkSubmitInfo2_host *, VkFence);
    VkResult (*p_vkQueueWaitIdle)(VkQueue);
    VkResult (*p_vkReleasePerformanceConfigurationINTEL)(VkDevice, VkPerformanceConfigurationINTEL);
    void (*p_vkReleaseProfilingLockKHR)(VkDevice);
    VkResult (*p_vkResetCommandBuffer)(VkCommandBuffer, VkCommandBufferResetFlags);
    VkResult (*p_vkResetCommandPool)(VkDevice, VkCommandPool, VkCommandPoolResetFlags);
    VkResult (*p_vkResetDescriptorPool)(VkDevice, VkDescriptorPool, VkDescriptorPoolResetFlags);
    VkResult (*p_vkResetEvent)(VkDevice, VkEvent);
    VkResult (*p_vkResetFences)(VkDevice, uint32_t, const VkFence *);
    void (*p_vkResetQueryPool)(VkDevice, VkQueryPool, uint32_t, uint32_t);
    void (*p_vkResetQueryPoolEXT)(VkDevice, VkQueryPool, uint32_t, uint32_t);
    VkResult (*p_vkSetDebugUtilsObjectNameEXT)(VkDevice, const VkDebugUtilsObjectNameInfoEXT_host *);
    VkResult (*p_vkSetDebugUtilsObjectTagEXT)(VkDevice, const VkDebugUtilsObjectTagInfoEXT_host *);
    void (*p_vkSetDeviceMemoryPriorityEXT)(VkDevice, VkDeviceMemory, float);
    VkResult (*p_vkSetEvent)(VkDevice, VkEvent);
    VkResult (*p_vkSetPrivateData)(VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t);
    VkResult (*p_vkSetPrivateDataEXT)(VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t);
    VkResult (*p_vkSignalSemaphore)(VkDevice, const VkSemaphoreSignalInfo_host *);
    VkResult (*p_vkSignalSemaphoreKHR)(VkDevice, const VkSemaphoreSignalInfo_host *);
    void (*p_vkTrimCommandPool)(VkDevice, VkCommandPool, VkCommandPoolTrimFlags);
    void (*p_vkTrimCommandPoolKHR)(VkDevice, VkCommandPool, VkCommandPoolTrimFlags);
    void (*p_vkUninitializePerformanceApiINTEL)(VkDevice);
    void (*p_vkUnmapMemory)(VkDevice, VkDeviceMemory);
    void (*p_vkUpdateDescriptorSetWithTemplate)(VkDevice, VkDescriptorSet, VkDescriptorUpdateTemplate, const void *);
    void (*p_vkUpdateDescriptorSetWithTemplateKHR)(VkDevice, VkDescriptorSet, VkDescriptorUpdateTemplate, const void *);
    void (*p_vkUpdateDescriptorSets)(VkDevice, uint32_t, const VkWriteDescriptorSet_host *, uint32_t, const VkCopyDescriptorSet_host *);
    VkResult (*p_vkWaitForFences)(VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t);
    VkResult (*p_vkWaitForPresentKHR)(VkDevice, VkSwapchainKHR, uint64_t, uint64_t);
    VkResult (*p_vkWaitSemaphores)(VkDevice, const VkSemaphoreWaitInfo *, uint64_t);
    VkResult (*p_vkWaitSemaphoresKHR)(VkDevice, const VkSemaphoreWaitInfo *, uint64_t);
    VkResult (*p_vkWriteAccelerationStructuresPropertiesKHR)(VkDevice, uint32_t, const VkAccelerationStructureKHR *, VkQueryType, size_t, void *, size_t);
    VkResult (*p_vkWriteMicromapsPropertiesEXT)(VkDevice, uint32_t, const VkMicromapEXT *, VkQueryType, size_t, void *, size_t);
};

/* For use by vkInstance and children */
struct vulkan_instance_funcs
{
    VkResult (*p_vkCreateDebugReportCallbackEXT)(VkInstance, const VkDebugReportCallbackCreateInfoEXT *, const VkAllocationCallbacks *, VkDebugReportCallbackEXT *);
    VkResult (*p_vkCreateDebugUtilsMessengerEXT)(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT *, const VkAllocationCallbacks *, VkDebugUtilsMessengerEXT *);
    VkResult (*p_vkCreateWin32SurfaceKHR)(VkInstance, const VkWin32SurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
    void (*p_vkDebugReportMessageEXT)(VkInstance, VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char *, const char *);
    void (*p_vkDestroyDebugReportCallbackEXT)(VkInstance, VkDebugReportCallbackEXT, const VkAllocationCallbacks *);
    void (*p_vkDestroyDebugUtilsMessengerEXT)(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks *);
    void (*p_vkDestroySurfaceKHR)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *);
    VkResult (*p_vkEnumeratePhysicalDeviceGroups)(VkInstance, uint32_t *, VkPhysicalDeviceGroupProperties *);
    VkResult (*p_vkEnumeratePhysicalDeviceGroupsKHR)(VkInstance, uint32_t *, VkPhysicalDeviceGroupProperties *);
    VkResult (*p_vkEnumeratePhysicalDevices)(VkInstance, uint32_t *, VkPhysicalDevice *);
    void (*p_vkSubmitDebugUtilsMessageEXT)(VkInstance, VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT_host *);
    VkResult (*p_vkCreateDevice)(VkPhysicalDevice, const VkDeviceCreateInfo *, const VkAllocationCallbacks *, VkDevice *);
    VkResult (*p_vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice, const char *, uint32_t *, VkExtensionProperties *);
    VkResult (*p_vkEnumerateDeviceLayerProperties)(VkPhysicalDevice, uint32_t *, VkLayerProperties *);
    VkResult (*p_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR)(VkPhysicalDevice, uint32_t, uint32_t *, VkPerformanceCounterKHR *, VkPerformanceCounterDescriptionKHR *);
    VkResult (*p_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)(VkPhysicalDevice, uint32_t *, VkTimeDomainEXT *);
    VkResult (*p_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)(VkPhysicalDevice, uint32_t *, VkCooperativeMatrixPropertiesNV *);
    void (*p_vkGetPhysicalDeviceFeatures)(VkPhysicalDevice, VkPhysicalDeviceFeatures *);
    void (*p_vkGetPhysicalDeviceFeatures2)(VkPhysicalDevice, VkPhysicalDeviceFeatures2 *);
    void (*p_vkGetPhysicalDeviceFeatures2KHR)(VkPhysicalDevice, VkPhysicalDeviceFeatures2 *);
    void (*p_vkGetPhysicalDeviceFormatProperties)(VkPhysicalDevice, VkFormat, VkFormatProperties *);
    void (*p_vkGetPhysicalDeviceFormatProperties2)(VkPhysicalDevice, VkFormat, VkFormatProperties2 *);
    void (*p_vkGetPhysicalDeviceFormatProperties2KHR)(VkPhysicalDevice, VkFormat, VkFormatProperties2 *);
    VkResult (*p_vkGetPhysicalDeviceFragmentShadingRatesKHR)(VkPhysicalDevice, uint32_t *, VkPhysicalDeviceFragmentShadingRateKHR *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties_host *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties2)(VkPhysicalDevice, const VkPhysicalDeviceImageFormatInfo2 *, VkImageFormatProperties2_host *);
    VkResult (*p_vkGetPhysicalDeviceImageFormatProperties2KHR)(VkPhysicalDevice, const VkPhysicalDeviceImageFormatInfo2 *, VkImageFormatProperties2_host *);
    void (*p_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties_host *);
    void (*p_vkGetPhysicalDeviceMemoryProperties2)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties2_host *);
    void (*p_vkGetPhysicalDeviceMemoryProperties2KHR)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties2_host *);
    void (*p_vkGetPhysicalDeviceMultisamplePropertiesEXT)(VkPhysicalDevice, VkSampleCountFlagBits, VkMultisamplePropertiesEXT *);
    VkResult (*p_vkGetPhysicalDeviceOpticalFlowImageFormatsNV)(VkPhysicalDevice, const VkOpticalFlowImageFormatInfoNV *, uint32_t *, VkOpticalFlowImageFormatPropertiesNV *);
    VkResult (*p_vkGetPhysicalDevicePresentRectanglesKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkRect2D *);
    void (*p_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties_host *);
    void (*p_vkGetPhysicalDeviceProperties2)(VkPhysicalDevice, VkPhysicalDeviceProperties2_host *);
    void (*p_vkGetPhysicalDeviceProperties2KHR)(VkPhysicalDevice, VkPhysicalDeviceProperties2_host *);
    void (*p_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR)(VkPhysicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *, uint32_t *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties2)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties2 *);
    void (*p_vkGetPhysicalDeviceQueueFamilyProperties2KHR)(VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties2 *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties)(VkPhysicalDevice, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t *, VkSparseImageFormatProperties *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties2)(VkPhysicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *, uint32_t *, VkSparseImageFormatProperties2 *);
    void (*p_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *, uint32_t *, VkSparseImageFormatProperties2 *);
    VkResult (*p_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)(VkPhysicalDevice, uint32_t *, VkFramebufferMixedSamplesCombinationNV *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceCapabilities2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR_host *, VkSurfaceCapabilities2KHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)(VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceFormats2KHR)(VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR_host *, uint32_t *, VkSurfaceFormat2KHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceFormatsKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkSurfaceFormatKHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfacePresentModesKHR)(VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkPresentModeKHR *);
    VkResult (*p_vkGetPhysicalDeviceSurfaceSupportKHR)(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32 *);
    VkResult (*p_vkGetPhysicalDeviceToolProperties)(VkPhysicalDevice, uint32_t *, VkPhysicalDeviceToolProperties *);
    VkResult (*p_vkGetPhysicalDeviceToolPropertiesEXT)(VkPhysicalDevice, uint32_t *, VkPhysicalDeviceToolProperties *);
    VkBool32 (*p_vkGetPhysicalDeviceWin32PresentationSupportKHR)(VkPhysicalDevice, uint32_t);
};

#define ALL_VK_DEVICE_FUNCS() \
    USE_VK_FUNC(vkAcquireNextImage2KHR) \
    USE_VK_FUNC(vkAcquireNextImageKHR) \
    USE_VK_FUNC(vkAcquirePerformanceConfigurationINTEL) \
    USE_VK_FUNC(vkAcquireProfilingLockKHR) \
    USE_VK_FUNC(vkAllocateCommandBuffers) \
    USE_VK_FUNC(vkAllocateDescriptorSets) \
    USE_VK_FUNC(vkAllocateMemory) \
    USE_VK_FUNC(vkBeginCommandBuffer) \
    USE_VK_FUNC(vkBindAccelerationStructureMemoryNV) \
    USE_VK_FUNC(vkBindBufferMemory) \
    USE_VK_FUNC(vkBindBufferMemory2) \
    USE_VK_FUNC(vkBindBufferMemory2KHR) \
    USE_VK_FUNC(vkBindImageMemory) \
    USE_VK_FUNC(vkBindImageMemory2) \
    USE_VK_FUNC(vkBindImageMemory2KHR) \
    USE_VK_FUNC(vkBindOpticalFlowSessionImageNV) \
    USE_VK_FUNC(vkBuildAccelerationStructuresKHR) \
    USE_VK_FUNC(vkBuildMicromapsEXT) \
    USE_VK_FUNC(vkCmdBeginConditionalRenderingEXT) \
    USE_VK_FUNC(vkCmdBeginDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkCmdBeginQuery) \
    USE_VK_FUNC(vkCmdBeginQueryIndexedEXT) \
    USE_VK_FUNC(vkCmdBeginRenderPass) \
    USE_VK_FUNC(vkCmdBeginRenderPass2) \
    USE_VK_FUNC(vkCmdBeginRenderPass2KHR) \
    USE_VK_FUNC(vkCmdBeginRendering) \
    USE_VK_FUNC(vkCmdBeginRenderingKHR) \
    USE_VK_FUNC(vkCmdBeginTransformFeedbackEXT) \
    USE_VK_FUNC(vkCmdBindDescriptorSets) \
    USE_VK_FUNC(vkCmdBindIndexBuffer) \
    USE_VK_FUNC(vkCmdBindInvocationMaskHUAWEI) \
    USE_VK_FUNC(vkCmdBindPipeline) \
    USE_VK_FUNC(vkCmdBindPipelineShaderGroupNV) \
    USE_VK_FUNC(vkCmdBindShadingRateImageNV) \
    USE_VK_FUNC(vkCmdBindTransformFeedbackBuffersEXT) \
    USE_VK_FUNC(vkCmdBindVertexBuffers) \
    USE_VK_FUNC(vkCmdBindVertexBuffers2) \
    USE_VK_FUNC(vkCmdBindVertexBuffers2EXT) \
    USE_VK_FUNC(vkCmdBlitImage) \
    USE_VK_FUNC(vkCmdBlitImage2) \
    USE_VK_FUNC(vkCmdBlitImage2KHR) \
    USE_VK_FUNC(vkCmdBuildAccelerationStructureNV) \
    USE_VK_FUNC(vkCmdBuildAccelerationStructuresIndirectKHR) \
    USE_VK_FUNC(vkCmdBuildAccelerationStructuresKHR) \
    USE_VK_FUNC(vkCmdBuildMicromapsEXT) \
    USE_VK_FUNC(vkCmdClearAttachments) \
    USE_VK_FUNC(vkCmdClearColorImage) \
    USE_VK_FUNC(vkCmdClearDepthStencilImage) \
    USE_VK_FUNC(vkCmdCopyAccelerationStructureKHR) \
    USE_VK_FUNC(vkCmdCopyAccelerationStructureNV) \
    USE_VK_FUNC(vkCmdCopyAccelerationStructureToMemoryKHR) \
    USE_VK_FUNC(vkCmdCopyBuffer) \
    USE_VK_FUNC(vkCmdCopyBuffer2) \
    USE_VK_FUNC(vkCmdCopyBuffer2KHR) \
    USE_VK_FUNC(vkCmdCopyBufferToImage) \
    USE_VK_FUNC(vkCmdCopyBufferToImage2) \
    USE_VK_FUNC(vkCmdCopyBufferToImage2KHR) \
    USE_VK_FUNC(vkCmdCopyImage) \
    USE_VK_FUNC(vkCmdCopyImage2) \
    USE_VK_FUNC(vkCmdCopyImage2KHR) \
    USE_VK_FUNC(vkCmdCopyImageToBuffer) \
    USE_VK_FUNC(vkCmdCopyImageToBuffer2) \
    USE_VK_FUNC(vkCmdCopyImageToBuffer2KHR) \
    USE_VK_FUNC(vkCmdCopyMemoryToAccelerationStructureKHR) \
    USE_VK_FUNC(vkCmdCopyMemoryToMicromapEXT) \
    USE_VK_FUNC(vkCmdCopyMicromapEXT) \
    USE_VK_FUNC(vkCmdCopyMicromapToMemoryEXT) \
    USE_VK_FUNC(vkCmdCopyQueryPoolResults) \
    USE_VK_FUNC(vkCmdCuLaunchKernelNVX) \
    USE_VK_FUNC(vkCmdDebugMarkerBeginEXT) \
    USE_VK_FUNC(vkCmdDebugMarkerEndEXT) \
    USE_VK_FUNC(vkCmdDebugMarkerInsertEXT) \
    USE_VK_FUNC(vkCmdDispatch) \
    USE_VK_FUNC(vkCmdDispatchBase) \
    USE_VK_FUNC(vkCmdDispatchBaseKHR) \
    USE_VK_FUNC(vkCmdDispatchIndirect) \
    USE_VK_FUNC(vkCmdDraw) \
    USE_VK_FUNC(vkCmdDrawIndexed) \
    USE_VK_FUNC(vkCmdDrawIndexedIndirect) \
    USE_VK_FUNC(vkCmdDrawIndexedIndirectCount) \
    USE_VK_FUNC(vkCmdDrawIndexedIndirectCountAMD) \
    USE_VK_FUNC(vkCmdDrawIndexedIndirectCountKHR) \
    USE_VK_FUNC(vkCmdDrawIndirect) \
    USE_VK_FUNC(vkCmdDrawIndirectByteCountEXT) \
    USE_VK_FUNC(vkCmdDrawIndirectCount) \
    USE_VK_FUNC(vkCmdDrawIndirectCountAMD) \
    USE_VK_FUNC(vkCmdDrawIndirectCountKHR) \
    USE_VK_FUNC(vkCmdDrawMeshTasksEXT) \
    USE_VK_FUNC(vkCmdDrawMeshTasksIndirectCountEXT) \
    USE_VK_FUNC(vkCmdDrawMeshTasksIndirectCountNV) \
    USE_VK_FUNC(vkCmdDrawMeshTasksIndirectEXT) \
    USE_VK_FUNC(vkCmdDrawMeshTasksIndirectNV) \
    USE_VK_FUNC(vkCmdDrawMeshTasksNV) \
    USE_VK_FUNC(vkCmdDrawMultiEXT) \
    USE_VK_FUNC(vkCmdDrawMultiIndexedEXT) \
    USE_VK_FUNC(vkCmdEndConditionalRenderingEXT) \
    USE_VK_FUNC(vkCmdEndDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkCmdEndQuery) \
    USE_VK_FUNC(vkCmdEndQueryIndexedEXT) \
    USE_VK_FUNC(vkCmdEndRenderPass) \
    USE_VK_FUNC(vkCmdEndRenderPass2) \
    USE_VK_FUNC(vkCmdEndRenderPass2KHR) \
    USE_VK_FUNC(vkCmdEndRendering) \
    USE_VK_FUNC(vkCmdEndRenderingKHR) \
    USE_VK_FUNC(vkCmdEndTransformFeedbackEXT) \
    USE_VK_FUNC(vkCmdExecuteCommands) \
    USE_VK_FUNC(vkCmdExecuteGeneratedCommandsNV) \
    USE_VK_FUNC(vkCmdFillBuffer) \
    USE_VK_FUNC(vkCmdInsertDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkCmdNextSubpass) \
    USE_VK_FUNC(vkCmdNextSubpass2) \
    USE_VK_FUNC(vkCmdNextSubpass2KHR) \
    USE_VK_FUNC(vkCmdOpticalFlowExecuteNV) \
    USE_VK_FUNC(vkCmdPipelineBarrier) \
    USE_VK_FUNC(vkCmdPipelineBarrier2) \
    USE_VK_FUNC(vkCmdPipelineBarrier2KHR) \
    USE_VK_FUNC(vkCmdPreprocessGeneratedCommandsNV) \
    USE_VK_FUNC(vkCmdPushConstants) \
    USE_VK_FUNC(vkCmdPushDescriptorSetKHR) \
    USE_VK_FUNC(vkCmdPushDescriptorSetWithTemplateKHR) \
    USE_VK_FUNC(vkCmdResetEvent) \
    USE_VK_FUNC(vkCmdResetEvent2) \
    USE_VK_FUNC(vkCmdResetEvent2KHR) \
    USE_VK_FUNC(vkCmdResetQueryPool) \
    USE_VK_FUNC(vkCmdResolveImage) \
    USE_VK_FUNC(vkCmdResolveImage2) \
    USE_VK_FUNC(vkCmdResolveImage2KHR) \
    USE_VK_FUNC(vkCmdSetAlphaToCoverageEnableEXT) \
    USE_VK_FUNC(vkCmdSetAlphaToOneEnableEXT) \
    USE_VK_FUNC(vkCmdSetBlendConstants) \
    USE_VK_FUNC(vkCmdSetCheckpointNV) \
    USE_VK_FUNC(vkCmdSetCoarseSampleOrderNV) \
    USE_VK_FUNC(vkCmdSetColorBlendAdvancedEXT) \
    USE_VK_FUNC(vkCmdSetColorBlendEnableEXT) \
    USE_VK_FUNC(vkCmdSetColorBlendEquationEXT) \
    USE_VK_FUNC(vkCmdSetColorWriteEnableEXT) \
    USE_VK_FUNC(vkCmdSetColorWriteMaskEXT) \
    USE_VK_FUNC(vkCmdSetConservativeRasterizationModeEXT) \
    USE_VK_FUNC(vkCmdSetCoverageModulationModeNV) \
    USE_VK_FUNC(vkCmdSetCoverageModulationTableEnableNV) \
    USE_VK_FUNC(vkCmdSetCoverageModulationTableNV) \
    USE_VK_FUNC(vkCmdSetCoverageReductionModeNV) \
    USE_VK_FUNC(vkCmdSetCoverageToColorEnableNV) \
    USE_VK_FUNC(vkCmdSetCoverageToColorLocationNV) \
    USE_VK_FUNC(vkCmdSetCullMode) \
    USE_VK_FUNC(vkCmdSetCullModeEXT) \
    USE_VK_FUNC(vkCmdSetDepthBias) \
    USE_VK_FUNC(vkCmdSetDepthBiasEnable) \
    USE_VK_FUNC(vkCmdSetDepthBiasEnableEXT) \
    USE_VK_FUNC(vkCmdSetDepthBounds) \
    USE_VK_FUNC(vkCmdSetDepthBoundsTestEnable) \
    USE_VK_FUNC(vkCmdSetDepthBoundsTestEnableEXT) \
    USE_VK_FUNC(vkCmdSetDepthClampEnableEXT) \
    USE_VK_FUNC(vkCmdSetDepthClipEnableEXT) \
    USE_VK_FUNC(vkCmdSetDepthClipNegativeOneToOneEXT) \
    USE_VK_FUNC(vkCmdSetDepthCompareOp) \
    USE_VK_FUNC(vkCmdSetDepthCompareOpEXT) \
    USE_VK_FUNC(vkCmdSetDepthTestEnable) \
    USE_VK_FUNC(vkCmdSetDepthTestEnableEXT) \
    USE_VK_FUNC(vkCmdSetDepthWriteEnable) \
    USE_VK_FUNC(vkCmdSetDepthWriteEnableEXT) \
    USE_VK_FUNC(vkCmdSetDeviceMask) \
    USE_VK_FUNC(vkCmdSetDeviceMaskKHR) \
    USE_VK_FUNC(vkCmdSetDiscardRectangleEXT) \
    USE_VK_FUNC(vkCmdSetEvent) \
    USE_VK_FUNC(vkCmdSetEvent2) \
    USE_VK_FUNC(vkCmdSetEvent2KHR) \
    USE_VK_FUNC(vkCmdSetExclusiveScissorNV) \
    USE_VK_FUNC(vkCmdSetExtraPrimitiveOverestimationSizeEXT) \
    USE_VK_FUNC(vkCmdSetFragmentShadingRateEnumNV) \
    USE_VK_FUNC(vkCmdSetFragmentShadingRateKHR) \
    USE_VK_FUNC(vkCmdSetFrontFace) \
    USE_VK_FUNC(vkCmdSetFrontFaceEXT) \
    USE_VK_FUNC(vkCmdSetLineRasterizationModeEXT) \
    USE_VK_FUNC(vkCmdSetLineStippleEXT) \
    USE_VK_FUNC(vkCmdSetLineStippleEnableEXT) \
    USE_VK_FUNC(vkCmdSetLineWidth) \
    USE_VK_FUNC(vkCmdSetLogicOpEXT) \
    USE_VK_FUNC(vkCmdSetLogicOpEnableEXT) \
    USE_VK_FUNC(vkCmdSetPatchControlPointsEXT) \
    USE_VK_FUNC(vkCmdSetPerformanceMarkerINTEL) \
    USE_VK_FUNC(vkCmdSetPerformanceOverrideINTEL) \
    USE_VK_FUNC(vkCmdSetPerformanceStreamMarkerINTEL) \
    USE_VK_FUNC(vkCmdSetPolygonModeEXT) \
    USE_VK_FUNC(vkCmdSetPrimitiveRestartEnable) \
    USE_VK_FUNC(vkCmdSetPrimitiveRestartEnableEXT) \
    USE_VK_FUNC(vkCmdSetPrimitiveTopology) \
    USE_VK_FUNC(vkCmdSetPrimitiveTopologyEXT) \
    USE_VK_FUNC(vkCmdSetProvokingVertexModeEXT) \
    USE_VK_FUNC(vkCmdSetRasterizationSamplesEXT) \
    USE_VK_FUNC(vkCmdSetRasterizationStreamEXT) \
    USE_VK_FUNC(vkCmdSetRasterizerDiscardEnable) \
    USE_VK_FUNC(vkCmdSetRasterizerDiscardEnableEXT) \
    USE_VK_FUNC(vkCmdSetRayTracingPipelineStackSizeKHR) \
    USE_VK_FUNC(vkCmdSetRepresentativeFragmentTestEnableNV) \
    USE_VK_FUNC(vkCmdSetSampleLocationsEXT) \
    USE_VK_FUNC(vkCmdSetSampleLocationsEnableEXT) \
    USE_VK_FUNC(vkCmdSetSampleMaskEXT) \
    USE_VK_FUNC(vkCmdSetScissor) \
    USE_VK_FUNC(vkCmdSetScissorWithCount) \
    USE_VK_FUNC(vkCmdSetScissorWithCountEXT) \
    USE_VK_FUNC(vkCmdSetShadingRateImageEnableNV) \
    USE_VK_FUNC(vkCmdSetStencilCompareMask) \
    USE_VK_FUNC(vkCmdSetStencilOp) \
    USE_VK_FUNC(vkCmdSetStencilOpEXT) \
    USE_VK_FUNC(vkCmdSetStencilReference) \
    USE_VK_FUNC(vkCmdSetStencilTestEnable) \
    USE_VK_FUNC(vkCmdSetStencilTestEnableEXT) \
    USE_VK_FUNC(vkCmdSetStencilWriteMask) \
    USE_VK_FUNC(vkCmdSetTessellationDomainOriginEXT) \
    USE_VK_FUNC(vkCmdSetVertexInputEXT) \
    USE_VK_FUNC(vkCmdSetViewport) \
    USE_VK_FUNC(vkCmdSetViewportShadingRatePaletteNV) \
    USE_VK_FUNC(vkCmdSetViewportSwizzleNV) \
    USE_VK_FUNC(vkCmdSetViewportWScalingEnableNV) \
    USE_VK_FUNC(vkCmdSetViewportWScalingNV) \
    USE_VK_FUNC(vkCmdSetViewportWithCount) \
    USE_VK_FUNC(vkCmdSetViewportWithCountEXT) \
    USE_VK_FUNC(vkCmdSubpassShadingHUAWEI) \
    USE_VK_FUNC(vkCmdTraceRaysIndirect2KHR) \
    USE_VK_FUNC(vkCmdTraceRaysIndirectKHR) \
    USE_VK_FUNC(vkCmdTraceRaysKHR) \
    USE_VK_FUNC(vkCmdTraceRaysNV) \
    USE_VK_FUNC(vkCmdUpdateBuffer) \
    USE_VK_FUNC(vkCmdWaitEvents) \
    USE_VK_FUNC(vkCmdWaitEvents2) \
    USE_VK_FUNC(vkCmdWaitEvents2KHR) \
    USE_VK_FUNC(vkCmdWriteAccelerationStructuresPropertiesKHR) \
    USE_VK_FUNC(vkCmdWriteAccelerationStructuresPropertiesNV) \
    USE_VK_FUNC(vkCmdWriteBufferMarker2AMD) \
    USE_VK_FUNC(vkCmdWriteBufferMarkerAMD) \
    USE_VK_FUNC(vkCmdWriteMicromapsPropertiesEXT) \
    USE_VK_FUNC(vkCmdWriteTimestamp) \
    USE_VK_FUNC(vkCmdWriteTimestamp2) \
    USE_VK_FUNC(vkCmdWriteTimestamp2KHR) \
    USE_VK_FUNC(vkCompileDeferredNV) \
    USE_VK_FUNC(vkCopyAccelerationStructureKHR) \
    USE_VK_FUNC(vkCopyAccelerationStructureToMemoryKHR) \
    USE_VK_FUNC(vkCopyMemoryToAccelerationStructureKHR) \
    USE_VK_FUNC(vkCopyMemoryToMicromapEXT) \
    USE_VK_FUNC(vkCopyMicromapEXT) \
    USE_VK_FUNC(vkCopyMicromapToMemoryEXT) \
    USE_VK_FUNC(vkCreateAccelerationStructureKHR) \
    USE_VK_FUNC(vkCreateAccelerationStructureNV) \
    USE_VK_FUNC(vkCreateBuffer) \
    USE_VK_FUNC(vkCreateBufferView) \
    USE_VK_FUNC(vkCreateCommandPool) \
    USE_VK_FUNC(vkCreateComputePipelines) \
    USE_VK_FUNC(vkCreateCuFunctionNVX) \
    USE_VK_FUNC(vkCreateCuModuleNVX) \
    USE_VK_FUNC(vkCreateDeferredOperationKHR) \
    USE_VK_FUNC(vkCreateDescriptorPool) \
    USE_VK_FUNC(vkCreateDescriptorSetLayout) \
    USE_VK_FUNC(vkCreateDescriptorUpdateTemplate) \
    USE_VK_FUNC(vkCreateDescriptorUpdateTemplateKHR) \
    USE_VK_FUNC(vkCreateEvent) \
    USE_VK_FUNC(vkCreateFence) \
    USE_VK_FUNC(vkCreateFramebuffer) \
    USE_VK_FUNC(vkCreateGraphicsPipelines) \
    USE_VK_FUNC(vkCreateImage) \
    USE_VK_FUNC(vkCreateImageView) \
    USE_VK_FUNC(vkCreateIndirectCommandsLayoutNV) \
    USE_VK_FUNC(vkCreateMicromapEXT) \
    USE_VK_FUNC(vkCreateOpticalFlowSessionNV) \
    USE_VK_FUNC(vkCreatePipelineCache) \
    USE_VK_FUNC(vkCreatePipelineLayout) \
    USE_VK_FUNC(vkCreatePrivateDataSlot) \
    USE_VK_FUNC(vkCreatePrivateDataSlotEXT) \
    USE_VK_FUNC(vkCreateQueryPool) \
    USE_VK_FUNC(vkCreateRayTracingPipelinesKHR) \
    USE_VK_FUNC(vkCreateRayTracingPipelinesNV) \
    USE_VK_FUNC(vkCreateRenderPass) \
    USE_VK_FUNC(vkCreateRenderPass2) \
    USE_VK_FUNC(vkCreateRenderPass2KHR) \
    USE_VK_FUNC(vkCreateSampler) \
    USE_VK_FUNC(vkCreateSamplerYcbcrConversion) \
    USE_VK_FUNC(vkCreateSamplerYcbcrConversionKHR) \
    USE_VK_FUNC(vkCreateSemaphore) \
    USE_VK_FUNC(vkCreateShaderModule) \
    USE_VK_FUNC(vkCreateSwapchainKHR) \
    USE_VK_FUNC(vkCreateValidationCacheEXT) \
    USE_VK_FUNC(vkDebugMarkerSetObjectNameEXT) \
    USE_VK_FUNC(vkDebugMarkerSetObjectTagEXT) \
    USE_VK_FUNC(vkDeferredOperationJoinKHR) \
    USE_VK_FUNC(vkDestroyAccelerationStructureKHR) \
    USE_VK_FUNC(vkDestroyAccelerationStructureNV) \
    USE_VK_FUNC(vkDestroyBuffer) \
    USE_VK_FUNC(vkDestroyBufferView) \
    USE_VK_FUNC(vkDestroyCommandPool) \
    USE_VK_FUNC(vkDestroyCuFunctionNVX) \
    USE_VK_FUNC(vkDestroyCuModuleNVX) \
    USE_VK_FUNC(vkDestroyDeferredOperationKHR) \
    USE_VK_FUNC(vkDestroyDescriptorPool) \
    USE_VK_FUNC(vkDestroyDescriptorSetLayout) \
    USE_VK_FUNC(vkDestroyDescriptorUpdateTemplate) \
    USE_VK_FUNC(vkDestroyDescriptorUpdateTemplateKHR) \
    USE_VK_FUNC(vkDestroyDevice) \
    USE_VK_FUNC(vkDestroyEvent) \
    USE_VK_FUNC(vkDestroyFence) \
    USE_VK_FUNC(vkDestroyFramebuffer) \
    USE_VK_FUNC(vkDestroyImage) \
    USE_VK_FUNC(vkDestroyImageView) \
    USE_VK_FUNC(vkDestroyIndirectCommandsLayoutNV) \
    USE_VK_FUNC(vkDestroyMicromapEXT) \
    USE_VK_FUNC(vkDestroyOpticalFlowSessionNV) \
    USE_VK_FUNC(vkDestroyPipeline) \
    USE_VK_FUNC(vkDestroyPipelineCache) \
    USE_VK_FUNC(vkDestroyPipelineLayout) \
    USE_VK_FUNC(vkDestroyPrivateDataSlot) \
    USE_VK_FUNC(vkDestroyPrivateDataSlotEXT) \
    USE_VK_FUNC(vkDestroyQueryPool) \
    USE_VK_FUNC(vkDestroyRenderPass) \
    USE_VK_FUNC(vkDestroySampler) \
    USE_VK_FUNC(vkDestroySamplerYcbcrConversion) \
    USE_VK_FUNC(vkDestroySamplerYcbcrConversionKHR) \
    USE_VK_FUNC(vkDestroySemaphore) \
    USE_VK_FUNC(vkDestroyShaderModule) \
    USE_VK_FUNC(vkDestroySwapchainKHR) \
    USE_VK_FUNC(vkDestroyValidationCacheEXT) \
    USE_VK_FUNC(vkDeviceWaitIdle) \
    USE_VK_FUNC(vkEndCommandBuffer) \
    USE_VK_FUNC(vkFlushMappedMemoryRanges) \
    USE_VK_FUNC(vkFreeCommandBuffers) \
    USE_VK_FUNC(vkFreeDescriptorSets) \
    USE_VK_FUNC(vkFreeMemory) \
    USE_VK_FUNC(vkGetAccelerationStructureBuildSizesKHR) \
    USE_VK_FUNC(vkGetAccelerationStructureDeviceAddressKHR) \
    USE_VK_FUNC(vkGetAccelerationStructureHandleNV) \
    USE_VK_FUNC(vkGetAccelerationStructureMemoryRequirementsNV) \
    USE_VK_FUNC(vkGetBufferDeviceAddress) \
    USE_VK_FUNC(vkGetBufferDeviceAddressEXT) \
    USE_VK_FUNC(vkGetBufferDeviceAddressKHR) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements2) \
    USE_VK_FUNC(vkGetBufferMemoryRequirements2KHR) \
    USE_VK_FUNC(vkGetBufferOpaqueCaptureAddress) \
    USE_VK_FUNC(vkGetBufferOpaqueCaptureAddressKHR) \
    USE_VK_FUNC(vkGetCalibratedTimestampsEXT) \
    USE_VK_FUNC(vkGetDeferredOperationMaxConcurrencyKHR) \
    USE_VK_FUNC(vkGetDeferredOperationResultKHR) \
    USE_VK_FUNC(vkGetDescriptorSetHostMappingVALVE) \
    USE_VK_FUNC(vkGetDescriptorSetLayoutHostMappingInfoVALVE) \
    USE_VK_FUNC(vkGetDescriptorSetLayoutSupport) \
    USE_VK_FUNC(vkGetDescriptorSetLayoutSupportKHR) \
    USE_VK_FUNC(vkGetDeviceAccelerationStructureCompatibilityKHR) \
    USE_VK_FUNC(vkGetDeviceBufferMemoryRequirements) \
    USE_VK_FUNC(vkGetDeviceBufferMemoryRequirementsKHR) \
    USE_VK_FUNC(vkGetDeviceFaultInfoEXT) \
    USE_VK_FUNC(vkGetDeviceGroupPeerMemoryFeatures) \
    USE_VK_FUNC(vkGetDeviceGroupPeerMemoryFeaturesKHR) \
    USE_VK_FUNC(vkGetDeviceGroupPresentCapabilitiesKHR) \
    USE_VK_FUNC(vkGetDeviceGroupSurfacePresentModesKHR) \
    USE_VK_FUNC(vkGetDeviceImageMemoryRequirements) \
    USE_VK_FUNC(vkGetDeviceImageMemoryRequirementsKHR) \
    USE_VK_FUNC(vkGetDeviceImageSparseMemoryRequirements) \
    USE_VK_FUNC(vkGetDeviceImageSparseMemoryRequirementsKHR) \
    USE_VK_FUNC(vkGetDeviceMemoryCommitment) \
    USE_VK_FUNC(vkGetDeviceMemoryOpaqueCaptureAddress) \
    USE_VK_FUNC(vkGetDeviceMemoryOpaqueCaptureAddressKHR) \
    USE_VK_FUNC(vkGetDeviceMicromapCompatibilityEXT) \
    USE_VK_FUNC(vkGetDeviceQueue) \
    USE_VK_FUNC(vkGetDeviceQueue2) \
    USE_VK_FUNC(vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI) \
    USE_VK_FUNC(vkGetDynamicRenderingTilePropertiesQCOM) \
    USE_VK_FUNC(vkGetEventStatus) \
    USE_VK_FUNC(vkGetFenceStatus) \
    USE_VK_FUNC(vkGetFramebufferTilePropertiesQCOM) \
    USE_VK_FUNC(vkGetGeneratedCommandsMemoryRequirementsNV) \
    USE_VK_FUNC(vkGetImageMemoryRequirements) \
    USE_VK_FUNC(vkGetImageMemoryRequirements2) \
    USE_VK_FUNC(vkGetImageMemoryRequirements2KHR) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements2) \
    USE_VK_FUNC(vkGetImageSparseMemoryRequirements2KHR) \
    USE_VK_FUNC(vkGetImageSubresourceLayout) \
    USE_VK_FUNC(vkGetImageSubresourceLayout2EXT) \
    USE_VK_FUNC(vkGetImageViewAddressNVX) \
    USE_VK_FUNC(vkGetImageViewHandleNVX) \
    USE_VK_FUNC(vkGetMemoryHostPointerPropertiesEXT) \
    USE_VK_FUNC(vkGetMemoryWin32HandleKHR) \
    USE_VK_FUNC(vkGetMemoryWin32HandlePropertiesKHR) \
    USE_VK_FUNC(vkGetMicromapBuildSizesEXT) \
    USE_VK_FUNC(vkGetPerformanceParameterINTEL) \
    USE_VK_FUNC(vkGetPipelineCacheData) \
    USE_VK_FUNC(vkGetPipelineExecutableInternalRepresentationsKHR) \
    USE_VK_FUNC(vkGetPipelineExecutablePropertiesKHR) \
    USE_VK_FUNC(vkGetPipelineExecutableStatisticsKHR) \
    USE_VK_FUNC(vkGetPipelinePropertiesEXT) \
    USE_VK_FUNC(vkGetPrivateData) \
    USE_VK_FUNC(vkGetPrivateDataEXT) \
    USE_VK_FUNC(vkGetQueryPoolResults) \
    USE_VK_FUNC(vkGetQueueCheckpointData2NV) \
    USE_VK_FUNC(vkGetQueueCheckpointDataNV) \
    USE_VK_FUNC(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR) \
    USE_VK_FUNC(vkGetRayTracingShaderGroupHandlesKHR) \
    USE_VK_FUNC(vkGetRayTracingShaderGroupHandlesNV) \
    USE_VK_FUNC(vkGetRayTracingShaderGroupStackSizeKHR) \
    USE_VK_FUNC(vkGetRenderAreaGranularity) \
    USE_VK_FUNC(vkGetSemaphoreCounterValue) \
    USE_VK_FUNC(vkGetSemaphoreCounterValueKHR) \
    USE_VK_FUNC(vkGetShaderInfoAMD) \
    USE_VK_FUNC(vkGetShaderModuleCreateInfoIdentifierEXT) \
    USE_VK_FUNC(vkGetShaderModuleIdentifierEXT) \
    USE_VK_FUNC(vkGetSwapchainImagesKHR) \
    USE_VK_FUNC(vkGetValidationCacheDataEXT) \
    USE_VK_FUNC(vkInitializePerformanceApiINTEL) \
    USE_VK_FUNC(vkInvalidateMappedMemoryRanges) \
    USE_VK_FUNC(vkMapMemory) \
    USE_VK_FUNC(vkMergePipelineCaches) \
    USE_VK_FUNC(vkMergeValidationCachesEXT) \
    USE_VK_FUNC(vkQueueBeginDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkQueueBindSparse) \
    USE_VK_FUNC(vkQueueEndDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkQueueInsertDebugUtilsLabelEXT) \
    USE_VK_FUNC(vkQueuePresentKHR) \
    USE_VK_FUNC(vkQueueSetPerformanceConfigurationINTEL) \
    USE_VK_FUNC(vkQueueSubmit) \
    USE_VK_FUNC(vkQueueSubmit2) \
    USE_VK_FUNC(vkQueueSubmit2KHR) \
    USE_VK_FUNC(vkQueueWaitIdle) \
    USE_VK_FUNC(vkReleasePerformanceConfigurationINTEL) \
    USE_VK_FUNC(vkReleaseProfilingLockKHR) \
    USE_VK_FUNC(vkResetCommandBuffer) \
    USE_VK_FUNC(vkResetCommandPool) \
    USE_VK_FUNC(vkResetDescriptorPool) \
    USE_VK_FUNC(vkResetEvent) \
    USE_VK_FUNC(vkResetFences) \
    USE_VK_FUNC(vkResetQueryPool) \
    USE_VK_FUNC(vkResetQueryPoolEXT) \
    USE_VK_FUNC(vkSetDebugUtilsObjectNameEXT) \
    USE_VK_FUNC(vkSetDebugUtilsObjectTagEXT) \
    USE_VK_FUNC(vkSetDeviceMemoryPriorityEXT) \
    USE_VK_FUNC(vkSetEvent) \
    USE_VK_FUNC(vkSetPrivateData) \
    USE_VK_FUNC(vkSetPrivateDataEXT) \
    USE_VK_FUNC(vkSignalSemaphore) \
    USE_VK_FUNC(vkSignalSemaphoreKHR) \
    USE_VK_FUNC(vkTrimCommandPool) \
    USE_VK_FUNC(vkTrimCommandPoolKHR) \
    USE_VK_FUNC(vkUninitializePerformanceApiINTEL) \
    USE_VK_FUNC(vkUnmapMemory) \
    USE_VK_FUNC(vkUpdateDescriptorSetWithTemplate) \
    USE_VK_FUNC(vkUpdateDescriptorSetWithTemplateKHR) \
    USE_VK_FUNC(vkUpdateDescriptorSets) \
    USE_VK_FUNC(vkWaitForFences) \
    USE_VK_FUNC(vkWaitForPresentKHR) \
    USE_VK_FUNC(vkWaitSemaphores) \
    USE_VK_FUNC(vkWaitSemaphoresKHR) \
    USE_VK_FUNC(vkWriteAccelerationStructuresPropertiesKHR) \
    USE_VK_FUNC(vkWriteMicromapsPropertiesEXT)

#define ALL_VK_INSTANCE_FUNCS() \
    USE_VK_FUNC(vkCreateDebugReportCallbackEXT) \
    USE_VK_FUNC(vkCreateDebugUtilsMessengerEXT) \
    USE_VK_FUNC(vkCreateWin32SurfaceKHR) \
    USE_VK_FUNC(vkDebugReportMessageEXT) \
    USE_VK_FUNC(vkDestroyDebugReportCallbackEXT) \
    USE_VK_FUNC(vkDestroyDebugUtilsMessengerEXT) \
    USE_VK_FUNC(vkDestroySurfaceKHR) \
    USE_VK_FUNC(vkEnumeratePhysicalDeviceGroups) \
    USE_VK_FUNC(vkEnumeratePhysicalDeviceGroupsKHR) \
    USE_VK_FUNC(vkEnumeratePhysicalDevices) \
    USE_VK_FUNC(vkSubmitDebugUtilsMessageEXT) \
    USE_VK_FUNC(vkCreateDevice) \
    USE_VK_FUNC(vkEnumerateDeviceExtensionProperties) \
    USE_VK_FUNC(vkEnumerateDeviceLayerProperties) \
    USE_VK_FUNC(vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT) \
    USE_VK_FUNC(vkGetPhysicalDeviceCooperativeMatrixPropertiesNV) \
    USE_VK_FUNC(vkGetPhysicalDeviceFeatures) \
    USE_VK_FUNC(vkGetPhysicalDeviceFeatures2) \
    USE_VK_FUNC(vkGetPhysicalDeviceFeatures2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceFormatProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceFormatProperties2) \
    USE_VK_FUNC(vkGetPhysicalDeviceFormatProperties2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceFragmentShadingRatesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceImageFormatProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceImageFormatProperties2) \
    USE_VK_FUNC(vkGetPhysicalDeviceImageFormatProperties2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceMemoryProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceMemoryProperties2) \
    USE_VK_FUNC(vkGetPhysicalDeviceMemoryProperties2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceMultisamplePropertiesEXT) \
    USE_VK_FUNC(vkGetPhysicalDeviceOpticalFlowImageFormatsNV) \
    USE_VK_FUNC(vkGetPhysicalDevicePresentRectanglesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceProperties2) \
    USE_VK_FUNC(vkGetPhysicalDeviceProperties2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties2) \
    USE_VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties2) \
    USE_VK_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV) \
    USE_VK_FUNC(vkGetPhysicalDeviceSurfaceCapabilities2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSurfaceFormats2KHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR) \
    USE_VK_FUNC(vkGetPhysicalDeviceToolProperties) \
    USE_VK_FUNC(vkGetPhysicalDeviceToolPropertiesEXT) \
    USE_VK_FUNC(vkGetPhysicalDeviceWin32PresentationSupportKHR)

#endif /* __WINE_VULKAN_THUNKS_H */
